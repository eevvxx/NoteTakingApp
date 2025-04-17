// include/MainWindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QModelIndex>
#include <QPoint> // Needed for context menu position

// Forward declarations
class QWidget;
class QTextEdit;
class QListView; // Added back for notebookListView
class QTreeView;
class QSplitter;
class QStandardItemModel;
class QLabel;
class QToolButton;
class QAction;
// Remove CustomSplitter/Handle forward declarations if not used elsewhere

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Slots for handling selections in the new lists
    void onNotebookSelected(const QModelIndex &index);
    void onSectionSelected(const QModelIndex &index);
    void onPageSelected(const QModelIndex &index);

    // Slots for the new "Add" buttons
    void addNotebook();
    void addSection(); // Will add top-level section or under group
    void addPage();    // Will add top-level page or under parent page

    // Context Menu Slots
    void showSectionContextMenu(const QPoint &pos);
    void showPageContextMenu(const QPoint &pos);

    // New Action Slots
    void addSectionGroup();
    void addSubpage();
    void promoteSubpage();

    // Keep old slots if still relevant
    // void handleNewNote(); // Maybe replaced by addPage/addSubpage
    // void handleNoteSelection(const QModelIndex &index); // Replaced by onPageSelected

private:
    void setupUI();
    void createActions();
    void createMenus();
    // void createToolbars(); // Toolbar might be removed or repurposed
    void createStatusBar();
    void loadInitialData(); // Helper to populate models initially

    // --- New UI Structure ---
    // Splitters
    QSplitter *topLevelSplitter;    // Separates panels from editor
    QSplitter *panelsSplitter;      // Separates Notebooks from Sections/Pages
    QSplitter *sectionPageSplitter; // Separates Sections from Pages

    // Panel Widgets (Containers)
    QWidget *notebookPanel;
    QWidget *sectionPanel;
    QWidget *pagePanel;

    // List/Tree Views
    QListView *notebookListView; // Keep as ListView
    QTreeView *sectionTreeView;  // Changed to QTreeView
    QTreeView *pageTreeView;     // Changed to QTreeView

    // Models (QStandardItemModel supports hierarchy)
    QStandardItemModel *notebookModel;
    QStandardItemModel *sectionModel;
    QStandardItemModel *pageModel;

    // Editor
    QTextEdit *noteEditor;
    // --- End New UI Structure ---


    // Actions
    QAction *exitAction;
    // Context Menu Actions
    QAction *addSectionGroupAction;
    QAction *addSectionAction; // Re-use for context menu
    QAction *addPageAction;    // Re-use for context menu
    QAction *addSubpageAction;
    QAction *promoteSubpageAction;
    // QAction *deleteItemAction; // Consider adding later
    // QAction *renameItemAction; // Consider adding later

    // Menus
    QMenu *fileMenu;
    // QMenu *viewMenu; // Remove if toggle action is gone
};

#endif // MAINWINDOW_H
