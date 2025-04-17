import os
import sys
import logging
import shutil
from datetime import datetime
from pathlib import Path
import zipfile
from tqdm import tqdm
import hashlib

backup_dir = Path('user_backup')
backup_dir.mkdir(exist_ok=True)

logging.basicConfig(
    filename=backup_dir / 'backup_restore.log',
    level=logging.INFO,
    format='%(asctime)s - %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S'
)

ignored_items = set()
ignore_log_path = backup_dir / 'ignored_items.log'
ignored_sizes = set()

if ignore_log_path.exists():
    with open(ignore_log_path, 'r') as f:
        for line in f:
            line = line.strip()
            if line.endswith('MB') or line.endswith('GB'):
                ignored_sizes.add(line)
            else:
                ignored_items.add(line)

def format_table(headers, rows, widths=None):
    if not widths:
        widths = [max(len(str(row[i])) for row in [headers] + rows) + 2 for i in range(len(headers))]
    
    h_line = '+' + '+'.join('-' * w for w in widths) + '+'
    header_str = '|' + '|'.join(f"{str(h):^{w}}" for h, w in zip(headers, widths)) + '|'
    row_lines = []
    for row in rows:
        row_lines.append(h_line)
        row_lines.append('|' + '|'.join(f"{str(cell):^{w}}" for cell, w in zip(row, widths)) + '|')
    return '\n'.join([h_line, header_str] + row_lines + [h_line])

def save_ignored_items():
    with open(ignore_log_path, 'w') as f:
        for item in sorted(ignored_items | ignored_sizes):
            f.write(f"{item}\n")

def get_next_backup_number():
    backup_dir = Path('user_backup')
    existing_backups = list(backup_dir.glob('[0-9][0-9]__*.zip'))
    if not existing_backups:
        return "01"
    numbers = [int(b.name[:2]) for b in existing_backups]
    next_num = max(numbers) + 1
    return f"{next_num:02d}"

def calculate_file_hash(file_path):
    sha256_hash = hashlib.sha256()
    with open(file_path, "rb") as f:
        for byte_block in iter(lambda: f.read(4096), b""):
            sha256_hash.update(byte_block)
    return sha256_hash.hexdigest()

def get_file_size_mb(file_path):
    return os.path.getsize(file_path) / (1024 * 1024)

def parse_size_limit(size_str):
    size_str = size_str.upper().strip()
    if size_str.endswith('MB'):
        return float(size_str[:-2])
    elif size_str.endswith('GB'):
        return float(size_str[:-2]) * 1024
    return float(size_str)

def should_ignore_size(file_path):
    file_size_mb = get_file_size_mb(file_path)
    for size_limit in ignored_sizes:
        limit_mb = parse_size_limit(size_limit)
        if file_size_mb >= limit_mb:
            return True
    return False

def create_backup(comment=''):
    temp_dir = None
    try:
        backup_dir = Path('user_backup')
        backup_dir.mkdir(exist_ok=True)
        
        next_num = get_next_backup_number()
        formatted_comment = comment.replace(' ', '_') if comment else 'backup'
        backup_name = f"{next_num}__{formatted_comment}.zip"
        backup_path = backup_dir / backup_name

        total_files = 0
        files_to_backup = []
        original_hashes = {}
        ignore_paths = {Path(item).resolve() if not item.startswith('/') else Path(item[1:]).resolve() for item in ignored_items}

        for item in os.listdir('.'):
            if item not in ['user_backup', os.path.basename(__file__), 'backup_restore.log']:
                item_path = Path(item).resolve()
                should_ignore = (item_path in ignore_paths or 
                               any(item_path.is_relative_to(ignore_path) for ignore_path in ignore_paths))
                
                if not should_ignore:
                    if item_path.is_file():
                        if not should_ignore_size(item_path):
                            total_files += 1
                            files_to_backup.append(item_path)
                            original_hashes[str(item_path)] = calculate_file_hash(item_path)
                    else:
                        for file_path in item_path.rglob('*'):
                            if file_path.is_file():
                                file_resolved = file_path.resolve()
                                should_ignore_file = (file_resolved in ignore_paths or 
                                                    any(file_resolved.is_relative_to(ignore_path) for ignore_path in ignore_paths) or
                                                    should_ignore_size(file_resolved))
                                if not should_ignore_file:
                                    total_files += 1
                                    files_to_backup.append(file_path)
                                    original_hashes[str(file_path)] = calculate_file_hash(file_path)

        if total_files == 0:
            print("No files to backup after applying ignore list!")
            return False

        print("\nCreating backup archive...")
        with zipfile.ZipFile(backup_path, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
            with tqdm(total=total_files, desc="Creating backup", unit="files") as pbar:
                for file_path in files_to_backup:
                    # Normalize the path to use forward slashes
                    arcname = str(file_path.relative_to(Path.cwd())).replace('\\', '/')
                    archive.write(file_path, arcname)
                    pbar.update(1)

        print("\nVerifying backup integrity...")
        temp_dir = Path('temp_verify')
        if temp_dir.exists():
            shutil.rmtree(temp_dir)
        temp_dir.mkdir()

        verified_count = 0
        with zipfile.ZipFile(backup_path, 'r') as archive:
            print("Testing ZIP archive integrity...")
            try:
                archive.testzip()
            except Exception as e:
                raise Exception(f"ZIP file is corrupted: {str(e)}")

            print("Verifying individual files...")
            with tqdm(total=total_files, desc="Verifying files", unit="files") as pbar:
                for file_path in files_to_backup:
                    # Normalize the path to match what was written to the archive
                    zip_path = str(file_path.relative_to(Path.cwd())).replace('\\', '/')
                    try:
                        archive.extract(zip_path, temp_dir)
                        temp_file_path = temp_dir / zip_path
                        if calculate_file_hash(temp_file_path) == original_hashes[str(file_path)]:
                            verified_count += 1
                        else:
                            raise Exception(f"File verification failed: {file_path}")
                        pbar.update(1)
                    except Exception as e:
                        raise Exception(f"Verification failed for {file_path}: {str(e)}")

        if verified_count != total_files:
            raise Exception(f"Only {verified_count}/{total_files} files verified successfully")

        print(f"\nBackup verification completed successfully! ({verified_count}/{total_files} files verified)")
        
        size_mb = round(os.path.getsize(backup_path) / (1024 * 1024), 2)
        log_msg = f"{backup_name} - {size_mb} MB (Verified {verified_count} files)"
        logging.info(log_msg)
        print(f"Backup created and verified: {backup_name}")
        return True

    except Exception as e:
        logging.error(f"Backup failed: {str(e)}")
        print(f"Error creating backup: {str(e)}")
        if backup_path.exists():
            try:
                backup_path.unlink()
                print("Incomplete backup file was removed.")
            except:
                print("Warning: Could not remove incomplete backup file.")
        return False
    finally:
        if temp_dir and temp_dir.exists():
            try:
                shutil.rmtree(temp_dir, ignore_errors=True)
            except:
                print("Warning: Could not remove temporary verification directory.")
    temp_dir = None
    try:
        backup_dir = Path('user_backup')
        backup_dir.mkdir(exist_ok=True)
        
        next_num = get_next_backup_number()
        formatted_comment = comment.replace(' ', '_') if comment else 'backup'
        backup_name = f"{next_num}__{formatted_comment}.zip"
        backup_path = backup_dir / backup_name

        total_files = 0
        files_to_backup = []
        original_hashes = {}
        ignore_paths = {Path(item).resolve() if not item.startswith('/') else Path(item[1:]).resolve() for item in ignored_items}

        for item in os.listdir('.'):
            if item not in ['user_backup', os.path.basename(__file__), 'backup_restore.log']:
                item_path = Path(item).resolve()
                should_ignore = (item_path in ignore_paths or 
                               any(item_path.is_relative_to(ignore_path) for ignore_path in ignore_paths))
                
                if not should_ignore:
                    if item_path.is_file():
                        if not should_ignore_size(item_path):
                            total_files += 1
                            files_to_backup.append(item_path)
                            original_hashes[str(item_path)] = calculate_file_hash(item_path)
                    else:
                        for file_path in item_path.rglob('*'):
                            if file_path.is_file():
                                file_resolved = file_path.resolve()
                                should_ignore_file = (file_resolved in ignore_paths or 
                                                    any(file_resolved.is_relative_to(ignore_path) for ignore_path in ignore_paths) or
                                                    should_ignore_size(file_resolved))
                                if not should_ignore_file:
                                    total_files += 1
                                    files_to_backup.append(file_path)
                                    original_hashes[str(file_path)] = calculate_file_hash(file_path)

        if total_files == 0:
            print("No files to backup after applying ignore list!")
            return False

        print("\nCreating backup archive...")
        with zipfile.ZipFile(backup_path, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
            with tqdm(total=total_files, desc="Creating backup", unit="files") as pbar:
                for file_path in files_to_backup:
                    archive.write(file_path, str(file_path.relative_to(Path.cwd())))
                    pbar.update(1)

        print("\nVerifying backup integrity...")
        temp_dir = Path('temp_verify')
        if temp_dir.exists():
            shutil.rmtree(temp_dir)
        temp_dir.mkdir()

        verified_count = 0
        with zipfile.ZipFile(backup_path, 'r') as archive:
            print("Testing ZIP archive integrity...")
            try:
                archive.testzip()
            except Exception as e:
                raise Exception(f"ZIP file is corrupted: {str(e)}")

            print("Verifying individual files...")
            with tqdm(total=total_files, desc="Verifying files", unit="files") as pbar:
                for file_path in files_to_backup:
                    zip_path = str(file_path.relative_to(Path.cwd()))
                    try:
                        archive.extract(zip_path, temp_dir)
                        temp_file_path = temp_dir / zip_path
                        if calculate_file_hash(temp_file_path) == original_hashes[str(file_path)]:
                            verified_count += 1
                        else:
                            raise Exception(f"File verification failed: {file_path}")
                        pbar.update(1)
                    except Exception as e:
                        raise Exception(f"Verification failed for {file_path}: {str(e)}")

        if verified_count != total_files:
            raise Exception(f"Only {verified_count}/{total_files} files verified successfully")

        print(f"\nBackup verification completed successfully! ({verified_count}/{total_files} files verified)")
        
        size_mb = round(os.path.getsize(backup_path) / (1024 * 1024), 2)
        log_msg = f"{backup_name} - {size_mb} MB (Verified {verified_count} files)"
        logging.info(log_msg)
        print(f"Backup created and verified: {backup_name}")
        return True

    except Exception as e:
        logging.error(f"Backup failed: {str(e)}")
        print(f"Error creating backup: {str(e)}")
        if backup_path.exists():
            try:
                backup_path.unlink()
                print("Incomplete backup file was removed.")
            except:
                print("Warning: Could not remove incomplete backup file.")
        return False
    finally:
        if temp_dir and temp_dir.exists():
            try:
                shutil.rmtree(temp_dir, ignore_errors=True)
            except:
                print("Warning: Could not remove temporary verification directory.")

def create_backup_with_ignore(comment=''):
    return create_backup(comment)

def handle_ignore_operations():
    while True:
        print("\n=== Ignore Operations ===")
        print("1. Add File")
        print("2. Add Folder")
        print("3. Ignore File Size")
        print("4. View Ignored Files and Folders")
        print("0. Return to Main Menu")
        
        choice = input("\nEnter your choice (0-4): ")

        if choice == '1':
            file_name = input("Enter file name (e.g., 'core.py'): ").strip()
            if file_name:
                ignored_items.add(file_name)
                save_ignored_items()
                print(f"File '{file_name}' added to ignore list")

        elif choice == '2':
            print("Instructions: Enter folder path starting with '/' (e.g., '/core' for core folder)")
            folder_name = input("Enter folder path: ").strip()
            if folder_name:
                ignored_items.add(folder_name)
                save_ignored_items()
                print(f"Folder '{folder_name}' added to ignore list")

        elif choice == '3':
            size_limit = input("Enter size limit (e.g., '50MB' or '1GB'): ").strip()
            if size_limit.upper().endswith(('MB', 'GB')) and size_limit[:-2].replace('.', '').isdigit():
                ignored_sizes.add(size_limit.upper())
                save_ignored_items()
                print(f"Size limit '{size_limit}' added to ignore list")
            else:
                print("Invalid format! Use e.g., '50MB' or '1GB'")

        elif choice == '4':
            if not (ignored_items or ignored_sizes):
                print("No files, folders, or sizes currently ignored!")
            else:
                print("\nIgnored Files, Folders, and Sizes:")
                current_time = datetime.now().strftime('%Y-%m-%d %H:%M')
                ignore_list = [[i+1, "ignored", item, current_time] for i, item in enumerate(sorted(ignored_items | ignored_sizes))]
                print(format_table(
                    ['#', 'Status', 'Item', 'Added'],
                    ignore_list
                ))

        elif choice == '0':
            break

        else:
            print("Invalid choice! Please enter a number between 0 and 4.")

def restore_backup(backup_file):
    try:
        backup_path = Path('user_backup') / backup_file
        
        ignore_paths = {Path(item).resolve() if not item.startswith('/') else Path(item[1:]).resolve() for item in ignored_items}

        items_to_check = []
        for item in os.listdir('.'):
            item_path = Path(item).resolve()
            if item not in ['user_backup', os.path.basename(__file__), 'backup_restore.log']:
                should_ignore = (item_path in ignore_paths or 
                               any(item_path.is_relative_to(ignore_path) for ignore_path in ignore_paths))
                if not should_ignore:
                    items_to_check.append(item)

        if not items_to_check:
            print("No files or folders to delete in the current directory (after applying ignore list).")
            items_to_delete = []
        else:
            print("\nThe following files and folders are in the current directory (ignore list applied):")
            for idx, item in enumerate(items_to_check, 1):
                item_path = Path(item)
                item_type = "Folder" if item_path.is_dir() else "File"
                print(f"{idx}. {item} ({item_type})")
            
            print("\nEnter the numbers of the items you want to KEEP (e.g., '1 3 5'), or press Enter to delete all listed:")
            keep_input = input("Your choice: ").strip()
            
            if keep_input:
                try:
                    keep_indices = [int(i) - 1 for i in keep_input.split()]
                    if any(i < 0 or i >= len(items_to_check) for i in keep_indices):
                        print("Invalid selection! One or more numbers are out of range. Aborting restore.")
                        return False
                    items_to_keep = [items_to_check[i] for i in keep_indices]
                    items_to_delete = [item for item in items_to_check if item not in items_to_keep]
                except ValueError:
                    print("Invalid input! Please enter numbers separated by spaces. Aborting restore.")
                    return False
            else:
                items_to_delete = items_to_check

        if items_to_delete:
            print("\nCleaning unselected items from current directory...")
            for item in items_to_delete:
                try:
                    if os.path.isfile(item):
                        os.remove(item)
                    else:
                        shutil.rmtree(item)
                    print(f"Deleted: {item}")
                except PermissionError:
                    print(f"Unable to delete {item}. Please close any programs using it and press Enter to continue...")
                    input()
                    os.remove(item) if os.path.isfile(item) else shutil.rmtree(item)
                    print(f"Deleted: {item}")
            print("Unselected items cleaned.")
        else:
            print("\nAll listed items selected to keep. Proceeding with restore...")

        with zipfile.ZipFile(backup_path, 'r') as archive:
            total_files = len(archive.namelist())
            with tqdm(total=total_files, desc="Restoring backup", unit="files") as pbar:
                for file in archive.namelist():
                    archive.extract(file)
                    pbar.update(1)

        current_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        deleted_items_str = ", ".join(items_to_delete) if items_to_delete else "None"
        log_msg = f"{backup_file} - Restored (Deleted items: {deleted_items_str})"
        logging.info(log_msg)
        print("\nRestore completed successfully!")
        return True

    except Exception as e:
        logging.error(f"Restore failed: {str(e)}")
        print(f"Error during restore: {str(e)}")
        return False
    
def list_backups():
    backup_dir = Path('user_backup')
    if not backup_dir.exists():
        return []
    
    backups = []
    for f in backup_dir.glob('*.zip'):
        size_mb = round(f.stat().st_size / (1024 * 1024), 2)
        created = datetime.fromtimestamp(f.stat().st_mtime).strftime('%Y-%m-%d %H:%M')
        comment = f.stem.split('__', 1)[1] if '__' in f.stem else ''
        backups.append([f.name, created, comment, f"{size_mb} MB"])
    
    sorted_backups = sorted(backups, key=lambda x: x[1])  # Sort oldest to newest
    numbered_backups = [[i + 1] + backup for i, backup in enumerate(sorted_backups)]  # Number from 1 (oldest) to n (newest)
    return numbered_backups

def regenerate_log():
    try:
        log_path = Path('user_backup') / 'backup_restore.log'
        backups = list_backups()
        
        if not backups:
            print("No backups found to regenerate log!")
            return False

        with open(log_path, 'w') as f:
            f.write("Available Backups:\n")
            f.write(format_table(
                ['#', 'Backup Name', 'Created', 'Comment', 'Size'],
                [[i+1] + backup for i, backup in enumerate(backups)]
            ))
        print("Log file has been regenerated successfully!")
        return True
    except Exception as e:
        print(f"Error regenerating log: {str(e)}")
        return False

def handle_log_operations():
    while True:
        print("\n=== Log Operations ===")
        print("1. View Log")
        print("2. Regenerate Log")
        print("3. Back to Main Menu")
        
        choice = input("\nEnter your choice (1-3): ")
        
        if choice == '1':
            log_path = Path('user_backup') / 'backup_restore.log'
            if log_path.exists():
                with open(log_path, 'r') as f:
                    print("\n=== Backup Log History ===")
                    print(f.read())
            else:
                print("No log file found!")

        elif choice == '2':
            confirm = input("This will clear the existing log and create a new one. Continue? (y/n): ")
            if confirm.lower() == 'y':
                regenerate_log()

        elif choice == '3':
            break
        else:
            print("Invalid choice! Please enter a number between 1 and 3.")

def delete_backup(backup_file):
    try:
        backup_path = Path('user_backup') / backup_file
        if backup_path.exists():
            os.remove(backup_path)
            log_msg = f"{backup_file} - Deleted"
            logging.info(log_msg)
            print(f"Backup {backup_file} was successfully deleted!")
            return True
        return False
    except Exception as e:
        logging.error(f"Delete failed: {str(e)}")
        print(f"Error deleting backup: {str(e)}")
        return False

def mark_backup_as_bad():
    backups = list_backups()
    if not backups:
        print("No backups found!")
        return

    print("\nAvailable Backups:")
    print(format_table(
        ['#', 'Backup Name', 'Created', 'Size'],
        [[row[0], row[1], row[2], row[4]] for row in backups]
    ))

    try:
        idx = int(input("\nSelect backup number to mark as bad: ")) - 1
        if 0 <= idx < len(backups):
            old_name = backups[idx][1]
            backup_path = Path('user_backup') / old_name
            number = old_name.split('__')[0]
            comment = old_name.split('__')[1].replace('.zip', '') if '__' in old_name else 'backup'
            new_name = f"{number}__[BAD-ERROR]_{comment}.zip"
            new_path = Path('user_backup') / new_name

            if backup_path.exists():
                backup_path.rename(new_path)
                log_msg = f"Marked as bad: {old_name} -> {new_name}"
                logging.info(log_msg)
                print(f"Backup marked as bad: {new_name}")
            else:
                print("Backup file not found!")
        else:
            print("Invalid selection! Number out of range.")
    except ValueError:
        print("Invalid selection! Please enter a valid number.")
    except Exception as e:
        print(f"Error marking backup: {str(e)}")

def main():
    while True:
        print("\n=== Backup/Restore Application ===")
        print("1. Create Backup")
        print("2. Create Ignore List")
        print("3. Restore Backup")
        print("4. Log Operations")
        print("5. Delete Backup")
        print("6. Mark Backup as Bad")  # New option
        print("0. Exit")
        
        choice = input("\nEnter your choice (0-6): ")

        if choice == '1':
            comment = input("Enter a comment for the backup (or press Enter to skip): ").strip()
            create_backup(comment)

        elif choice == '2':
            handle_ignore_operations()

        elif choice == '3':
            backups = list_backups()
            if not backups:
                print("No backups found!")
                continue

            print("\nAvailable Backups:")
            print(format_table(
                ['#', 'Backup Name', 'Created', 'Size'],
                [[row[0], row[1], row[2], row[4]] for row in backups]
            ))

            try:
                idx = int(input("\nSelect backup number to restore: ")) - 1
                if 0 <= idx < len(backups):
                    careful = input("Are you sure you want to delete all files and folders in the current directory and restore from the backup? (y/n): ")
                    if careful.lower() == 'y':
                        restore_backup(backups[idx][1])
                else:
                    print("Invalid selection! Number out of range.")
            except ValueError:
                print("Invalid selection! Please enter a valid number.")

        elif choice == '4':
            handle_log_operations()

        elif choice == '5':
            backups = list_backups()
            if not backups:
                print("No backups found!")
                continue
            print("\nAvailable Backups:")
            print(format_table(
                ['#', 'Backup Name', 'Created', 'Comment', 'Size'],
                [[row[0], row[1], row[2], row[3], row[4]] for row in backups]
            ))
            try:
                idx = int(input("\nSelect backup number to delete: ")) - 1
                if 0 <= idx < len(backups):
                    confirm = input(f"Are you sure you want to delete backup '{backups[idx][1]}'? (y/n): ")
                    if confirm.lower() == 'y':
                        delete_backup(backups[idx][1])
                else:
                    print("Invalid selection! Number out of range.")
            except ValueError:
                print("Invalid selection!")

        elif choice == '6':  # New case for marking bad backups
            mark_backup_as_bad()

        elif choice == '0':
            print("Goodbye!")
            break

        else:
            print("Invalid choice! Please enter a number between 0 and 6.")
            
if __name__ == "__main__":
    main()