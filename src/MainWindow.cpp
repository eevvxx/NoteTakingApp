// src/MainWindow.cpp
#include "MainWindow.h" // Include the header file for our main window

// Include necessary Qt headers
#include <QtWidgets> // Includes most common widgets (QLabel, QPushButton, Layouts, etc.)
#include <QStandardItemModel> // For the tree view data
#include <QStandardItem> // For items within the model
#include <QTreeView>     // Using TreeView now for sections/pages
#include <QMenu>         // For context menus
#include <QInputDialog>  // For getting names for new items
#include <QDebug> // For printing debug messages
#include <QMessageBox> // For showing warnings

// Constructor
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) // Call the base class constructor
{
    // Initialize models first (parented to 'this' for auto memory management)
    notebookModel = new QStandardItemModel(this);
    sectionModel = new QStandardItemModel(this);
    pageModel = new QStandardItemModel(this);

    setupUI(); // Create the UI elements
    createActions(); // Create menu/toolbar actions
    createMenus(); // Create the main menu bar
    createStatusBar(); // Create the status bar at the bottom
    loadInitialData(); // Populate models with placeholder data

    // Set window properties
    setWindowTitle(tr("Note Taking App")); // Use tr() for potential translation
    setMinimumSize(800, 600); // Set a reasonable minimum size
    resize(1100, 800); // Set a default startup size
}

// Destructor
MainWindow::~MainWindow()
{
    // Qt's parent-child mechanism handles deleting child widgets and models
}

// --- Setup the main UI structure ---
void MainWindow::setupUI()
{
    // --- Create Panel Widgets (Containers) ---
    notebookPanel = new QWidget();
    notebookPanel->setObjectName("notebookPanel"); // For QSS styling
    sectionPanel = new QWidget();
    sectionPanel->setObjectName("sectionPanel");
    pagePanel = new QWidget();
    pagePanel->setObjectName("pagePanel");

    // --- Create Headers for each Panel ---
    // Notebook Header
    QWidget *notebookHeader = new QWidget();
    notebookHeader->setObjectName("panelHeader");
    QHBoxLayout *notebookHeaderLayout = new QHBoxLayout(notebookHeader);
    notebookHeaderLayout->setContentsMargins(5, 3, 5, 3); // Small margins
    QLabel *notebookLabel = new QLabel(tr("Notebooks"));
    QToolButton *addNotebookButton = new QToolButton();
    addNotebookButton->setObjectName("addButton"); // For QSS
    addNotebookButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder)); // Folder icon
    addNotebookButton->setToolTip(tr("Add Notebook"));
    notebookHeaderLayout->addWidget(notebookLabel);
    notebookHeaderLayout->addStretch(); // Push button to the right
    notebookHeaderLayout->addWidget(addNotebookButton);

    // Section Header
    QWidget *sectionHeader = new QWidget();
    sectionHeader->setObjectName("panelHeader");
    QHBoxLayout *sectionHeaderLayout = new QHBoxLayout(sectionHeader);
    sectionHeaderLayout->setContentsMargins(5, 3, 5, 3);
    QLabel *sectionLabel = new QLabel(tr("Sections")); // Title will be updated
    sectionLabel->setObjectName("sectionHeaderLabel"); // To find it later
    QToolButton *addSectionButton = new QToolButton();
    addSectionButton->setObjectName("addButton");
    addSectionButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    addSectionButton->setToolTip(tr("Add Section"));
    sectionHeaderLayout->addWidget(sectionLabel);
    sectionHeaderLayout->addStretch();
    sectionHeaderLayout->addWidget(addSectionButton);

    // Page Header
    QWidget *pageHeader = new QWidget();
    pageHeader->setObjectName("panelHeader");
    QHBoxLayout *pageHeaderLayout = new QHBoxLayout(pageHeader);
    pageHeaderLayout->setContentsMargins(5, 3, 5, 3);
    QLabel *pageLabel = new QLabel(tr("Pages")); // Title will be updated
    pageLabel->setObjectName("pageHeaderLabel"); // To find it later
    QToolButton *addPageButton = new QToolButton();
    addPageButton->setObjectName("addButton");
    addPageButton->setIcon(style()->standardIcon(QStyle::SP_FileIcon)); // Document icon
    addPageButton->setToolTip(tr("Add Page"));
    pageHeaderLayout->addWidget(pageLabel);
    pageHeaderLayout->addStretch();
    pageHeaderLayout->addWidget(addPageButton);

    // --- Create List Views ---
    notebookListView = new QListView();
    notebookListView->setObjectName("notebookListView"); // For QSS
    notebookListView->setModel(notebookModel); // Assign the model
    notebookListView->setEditTriggers(QAbstractItemView::NoEditTriggers); // Don't allow editing in place

    sectionTreeView = new QTreeView(); // Changed to QTreeView
    sectionTreeView->setObjectName("sectionTreeView"); // Renamed object name
    sectionTreeView->setModel(sectionModel);
    sectionTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    sectionTreeView->setHeaderHidden(true); // Hide header

    pageTreeView = new QTreeView(); // Changed to QTreeView
    pageTreeView->setObjectName("pageTreeView"); // Renamed object name
    pageTreeView->setModel(pageModel);
    pageTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers); // Renamed variable
    pageTreeView->setHeaderHidden(true); // Typically hide header for simple lists/trees

    // Enable context menus
    sectionTreeView->setContextMenuPolicy(Qt::CustomContextMenu); // Renamed variable
    pageTreeView->setContextMenuPolicy(Qt::CustomContextMenu);    // Renamed variable


    // --- Layout Panels (Add Header and Tree View to each Panel) ---
    // Notebook Panel Layout
    QVBoxLayout *notebookLayout = new QVBoxLayout(notebookPanel);
    notebookLayout->setContentsMargins(0, 0, 0, 0); // No external margins
    notebookLayout->setSpacing(0); // No space between header and list
    notebookLayout->addWidget(notebookHeader);
    notebookLayout->addWidget(notebookListView); // Notebooks remain a ListView

    // Section Panel Layout
    QVBoxLayout *sectionLayout = new QVBoxLayout(sectionPanel);
    sectionLayout->setContentsMargins(0, 0, 0, 0);
    sectionLayout->setSpacing(0);
    sectionLayout->addWidget(sectionHeader);
    sectionLayout->addWidget(sectionTreeView); // Changed to TreeView

    // Page Panel Layout
    QVBoxLayout *pageLayout = new QVBoxLayout(pagePanel);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);
    pageLayout->addWidget(pageHeader);
    pageLayout->addWidget(pageTreeView); // Changed to TreeView

    // --- Create Editor ---
    noteEditor = new QTextEdit();
    noteEditor->setObjectName("noteEditor"); // For QSS
    noteEditor->setAcceptRichText(true); // Enable rich text features

    // --- Assemble Splitters ---
    // Inner splitter for Sections and Pages
    sectionPageSplitter = new QSplitter(Qt::Horizontal);
    sectionPageSplitter->setObjectName("sectionPageSplitter");
    sectionPageSplitter->addWidget(sectionPanel);
    sectionPageSplitter->addWidget(pagePanel);
    sectionPageSplitter->setStretchFactor(0, 0); // Section panel fixed size initially
    sectionPageSplitter->setStretchFactor(1, 1); // Page panel takes space

    // Middle splitter for Notebooks and the Section/Page group
    panelsSplitter = new QSplitter(Qt::Horizontal);
    panelsSplitter->setObjectName("panelsSplitter");
    panelsSplitter->addWidget(notebookPanel);
    panelsSplitter->addWidget(sectionPageSplitter); // Add the inner splitter
    panelsSplitter->setStretchFactor(0, 0); // Notebook panel fixed size initially
    panelsSplitter->setStretchFactor(1, 1); // Section/Page group takes space

    // Top-level splitter for the entire panel group and the editor
    topLevelSplitter = new QSplitter(Qt::Horizontal);
    topLevelSplitter->setObjectName("topLevelSplitter");
    topLevelSplitter->addWidget(panelsSplitter); // Add the 3-panel group
    topLevelSplitter->addWidget(noteEditor);
    topLevelSplitter->setStretchFactor(0, 0); // Panel group fixed size initially
    topLevelSplitter->setStretchFactor(1, 1); // Editor takes most space

    // Set initial sizes (adjust these proportions as needed)
    topLevelSplitter->setSizes({600, 500}); // Panels group vs Editor width
    panelsSplitter->setSizes({200, 400});   // Notebooks vs Sections/Pages width
    sectionPageSplitter->setSizes({200, 200}); // Sections vs Pages width

    // Set the main layout for the window
    setCentralWidget(topLevelSplitter);

    // --- Connect Signals and Slots ---
    // Connect selection changes in views to update other views/editor
    connect(notebookListView->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::onNotebookSelected);
    connect(sectionTreeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::onSectionSelected); // Changed to TreeView
    connect(pageTreeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::onPageSelected);       // Changed to TreeView

    // Connect context menu requests
    connect(sectionTreeView, &QWidget::customContextMenuRequested, this, &MainWindow::showSectionContextMenu);
    connect(pageTreeView, &QWidget::customContextMenuRequested, this, &MainWindow::showPageContextMenu);

    // Connect "Add" buttons to their respective slots
    connect(addNotebookButton, &QToolButton::clicked, this, &MainWindow::addNotebook);
    connect(addSectionButton, &QToolButton::clicked, this, &MainWindow::addSection);
    connect(addPageButton, &QToolButton::clicked, this, &MainWindow::addPage);
}

// --- Populate Models with Initial Placeholder Data ---
void MainWindow::loadInitialData()
{
    // Placeholder Notebooks
    QList<QStandardItem *> notebookItems;
    // Using standard icons (might need adjustment based on theme availability)
    notebookItems.append(new QStandardItem(style()->standardIcon(QStyle::SP_DirIcon), "Aspekte B1-B2"));
    notebookItems.append(new QStandardItem(style()->standardIcon(QStyle::SP_DirIcon), "Fouks_B1_B2"));
    notebookItems.append(new QStandardItem(style()->standardIcon(QStyle::SP_DirIcon), "VHK B2"));
    notebookItems.append(new QStandardItem(style()->standardIcon(QStyle::SP_DirIcon), "PC3"));
    notebookModel->invisibleRootItem()->appendRows(notebookItems);

    // Select the first notebook to trigger loading sections (if any)
    if (notebookModel->rowCount() > 0) {
        notebookListView->setCurrentIndex(notebookModel->index(0, 0));
        // The connect() statement ensures onNotebookSelected is called
    }
}

// --- Create Actions (for menus, shortcuts, context menus) ---
void MainWindow::createActions()
{
    // File Actions
    exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcut(QKeySequence::Quit); // Standard Ctrl+Q / Cmd+Q
    exitAction->setStatusTip(tr("Exit the application"));
    connect(exitAction, &QAction::triggered, this, &QWidget::close); // Connect to window's close slot

    // Context Menu Actions
    addSectionGroupAction = new QAction(tr("Add Section Group..."), this);
    connect(addSectionGroupAction, &QAction::triggered, this, &MainWindow::addSectionGroup);

    addSectionAction = new QAction(tr("Add Section..."), this); // Action for context menu
    connect(addSectionAction, &QAction::triggered, this, &MainWindow::addSection); // Connects to the same slot as button

    addPageAction = new QAction(tr("Add Page..."), this); // Action for context menu
    connect(addPageAction, &QAction::triggered, this, &MainWindow::addPage); // Connects to the same slot as button

    addSubpageAction = new QAction(tr("Add Subpage..."), this);
    connect(addSubpageAction, &QAction::triggered, this, &MainWindow::addSubpage);

    promoteSubpageAction = new QAction(tr("Promote Subpage"), this);
    connect(promoteSubpageAction, &QAction::triggered, this, &MainWindow::promoteSubpage);

    // Add icons later if desired
}

// --- Create Menu Bar ---
void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    // Add Save, Open etc. actions here later
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);
    // Removed View menu as toggle action is gone
}

// --- Create Status Bar ---
void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

// --- Slot Implementations ---

// Slot called when a different notebook is selected
void MainWindow::onNotebookSelected(const QModelIndex &index)
{
    sectionModel->clear(); // Clear previous sections
    pageModel->clear();    // Clear previous pages
    noteEditor->clear();   // Clear editor

    // Find header labels using findChild (safer than assuming order)
    QLabel* sectionHeaderLabel = findChild<QLabel*>("sectionHeaderLabel");
    QLabel* pageHeaderLabel = findChild<QLabel*>("pageHeaderLabel");

    if (!index.isValid()) {
        // No valid notebook selected, reset headers
        if(sectionHeaderLabel) sectionHeaderLabel->setText(tr("Sections"));
        if(pageHeaderLabel) pageHeaderLabel->setText(tr("Pages"));
        return;
    }

    // Get selected notebook name
    QString notebookName = notebookModel->data(index, Qt::DisplayRole).toString();
    qDebug() << "Notebook selected:" << notebookName;

    // Update header labels
    if(sectionHeaderLabel) sectionHeaderLabel->setText(notebookName);
    if(pageHeaderLabel) pageHeaderLabel->setText(tr("Pages")); // Reset page header

    // --- Placeholder Section Loading ---
    // In a real app, load sections for 'notebookName' from storage (file/db)
    QList<QStandardItem *> sectionItems;
    if (notebookName == "Aspekte B1-B2") {
        sectionItems.append(new QStandardItem(style()->standardIcon(QStyle::SP_DirLinkIcon), "1")); // Different icon?
        sectionItems.append(new QStandardItem(style()->standardIcon(QStyle::SP_DirLinkIcon), "2"));
        sectionItems.append(new QStandardItem(style()->standardIcon(QStyle::SP_DirLinkIcon), "3"));
    } else if (notebookName == "PC3") {
         sectionItems.append(new QStandardItem(style()->standardIcon(QStyle::SP_DirLinkIcon), "Chapter A"));
         sectionItems.append(new QStandardItem(style()->standardIcon(QStyle::SP_DirLinkIcon), "Chapter B"));
    }
    // Add items to the section model
    sectionModel->invisibleRootItem()->appendRows(sectionItems);
    // --- End Placeholder ---

     // Automatically select the first section if sections were loaded
      if (sectionModel->rowCount() > 0) {
         sectionTreeView->setCurrentIndex(sectionModel->index(0, 0)); // Changed to TreeView
         // onSectionSelected will be called automatically
     }
}

// Slot called when a different section is selected
void MainWindow::onSectionSelected(const QModelIndex &index)
{
    pageModel->clear(); // Clear previous pages
    noteEditor->clear(); // Clear editor

    QLabel* pageHeaderLabel = findChild<QLabel*>("pageHeaderLabel");

    if (!index.isValid()) {
        // No valid section selected
        if(pageHeaderLabel) pageHeaderLabel->setText(tr("Pages"));
        return;
    }

    // Get selected section name and current notebook name (from section header)
    QString sectionName = sectionModel->data(index, Qt::DisplayRole).toString();
    QLabel* sectionHeaderLabel = findChild<QLabel*>("sectionHeaderLabel");
    QString currentNotebook = sectionHeaderLabel ? sectionHeaderLabel->text() : "Unknown Notebook";

    qDebug() << "Section selected:" << sectionName << "in notebook" << currentNotebook;

    // Update page header label
    if(pageHeaderLabel) pageHeaderLabel->setText(sectionName);

    // --- Placeholder Page Loading ---
    // In a real app, load pages for 'sectionName' in 'currentNotebook' from storage
    QList<QStandardItem *> pageItems;
     if (currentNotebook == "Aspekte B1-B2" && sectionName == "1") {
        pageItems.append(new QStandardItem(style()->standardIcon(QStyle::SP_FileIcon), "1"));
        pageItems.append(new QStandardItem(style()->standardIcon(QStyle::SP_FileIcon), "Explain"));
    } else if (currentNotebook == "Aspekte B1-B2" && sectionName == "2") {
        pageItems.append(new QStandardItem(style()->standardIcon(QStyle::SP_FileIcon), "2a"));
        pageItems.append(new QStandardItem(style()->standardIcon(QStyle::SP_FileIcon), "2b Notes"));
    } else if (currentNotebook == "PC3" && sectionName == "Chapter A") {
        pageItems.append(new QStandardItem(style()->standardIcon(QStyle::SP_FileIcon), "Intro"));
        pageItems.append(new QStandardItem(style()->standardIcon(QStyle::SP_FileIcon), "Topic 1"));
    }
    // Add items to the page model
    pageModel->invisibleRootItem()->appendRows(pageItems);
    // --- End Placeholder ---

      // Automatically select the first page if pages were loaded
      if (pageModel->rowCount() > 0) {
         pageTreeView->setCurrentIndex(pageModel->index(0, 0)); // Changed to TreeView
         // onPageSelected will be called automatically
     }
}

// Slot called when a different page is selected
void MainWindow::onPageSelected(const QModelIndex &index)
{
    noteEditor->clear(); // Clear previous content

    if (!index.isValid()) return; // No valid page selected

    // Get names for context
    QString pageName = pageModel->data(index, Qt::DisplayRole).toString();
    QLabel* pageHeaderLabel = findChild<QLabel*>("pageHeaderLabel");
    QLabel* sectionHeaderLabel = findChild<QLabel*>("sectionHeaderLabel");
    QString currentSection = pageHeaderLabel ? pageHeaderLabel->text() : "Unknown Section";
    QString currentNotebook = sectionHeaderLabel ? sectionHeaderLabel->text() : "Unknown Notebook";

    qDebug() << "Page selected:" << pageName << "in section" << currentSection << "of notebook" << currentNotebook;

    // --- Placeholder Content Loading ---
    // In a real app, load content for 'pageName' (likely using IDs stored in the model item)
    noteEditor->setPlainText(tr("Content for page '%1' in section '%2' of notebook '%3'.\n\nReplace this with actual loaded content.")
                             .arg(pageName).arg(currentSection).arg(currentNotebook));
    // --- End Placeholder ---
}

// Slot for the "Add Notebook" button
void MainWindow::addNotebook()
{
    qDebug() << "Add Notebook clicked";
    bool ok;
    QString text = QInputDialog::getText(this, tr("Add Notebook"),
                                         tr("Notebook name:"), QLineEdit::Normal,
                                         "", &ok);
    if (ok && !text.isEmpty()) {
        notebookModel->appendRow(new QStandardItem(style()->standardIcon(QStyle::SP_DirIcon), text));
        // Select the newly added notebook
        notebookListView->setCurrentIndex(notebookModel->index(notebookModel->rowCount() - 1, 0));
    }
}

 // Slot for the "Add Section" button (and context menu action)
 void MainWindow::addSection()
 {
     // This slot now handles adding sections both from the button (always top-level)
     // and the context menu (potentially under a group).
     // We'll add the logic to determine the parent later in the context menu handler.
     // For now, the button press adds to the root.

     QModelIndex currentNotebookIndex = notebookListView->currentIndex();
     if (!currentNotebookIndex.isValid()) {
         QMessageBox::warning(this, tr("Add Section"), tr("Please select a notebook first."));
         return;
     }
     QString currentNotebookName = notebookModel->data(currentNotebookIndex).toString();
     qDebug() << "Add Section clicked for notebook:" << currentNotebookName;

     bool ok;
     QString text = QInputDialog::getText(this, tr("Add Section"),
                                          tr("Section name:"), QLineEdit::Normal,
                                          "", &ok);
      if (ok && !text.isEmpty()) {
         QStandardItem *newItem = new QStandardItem(style()->standardIcon(QStyle::SP_DirLinkIcon), text);
         QModelIndex currentIndex = sectionTreeView->currentIndex();
         QStandardItem *parentItem = sectionModel->invisibleRootItem(); // Default to root

         if (currentIndex.isValid()) {
             QStandardItem *selectedItem = sectionModel->itemFromIndex(currentIndex);
             // Check if selected item is a group (e.g., has children or bold font)
             // Using bold font check as a simple indicator for groups for now
             if (selectedItem && selectedItem->font().bold()) {
                 parentItem = selectedItem; // Add as child of the group
             }
         }

         parentItem->appendRow(newItem);
         QModelIndex newIndex = sectionModel->indexFromItem(newItem);
         sectionTreeView->setCurrentIndex(newIndex);
         sectionTreeView->expand(newIndex.parent()); // Expand the parent
      }
 }

 // Slot for the "Add Page" button (and context menu action)
 void MainWindow::addPage()
 {
     // Similar to addSection, this handles button (top-level) and context menu (subpage)
     // Context menu logic will handle parenting. Button adds to root.

     QModelIndex currentSectionIndex = sectionTreeView->currentIndex(); // Changed to TreeView
      if (!currentSectionIndex.isValid()) {
          QMessageBox::warning(this, tr("Add Page"), tr("Please select a section first."));
         return;
     }
     QString currentSectionName = sectionModel->data(currentSectionIndex).toString();
     qDebug() << "Add Page clicked for section:" << currentSectionName;

     bool ok;
     QString text = QInputDialog::getText(this, tr("Add Page"),
                                          tr("Page name:"), QLineEdit::Normal,
                                          "", &ok);
      if (ok && !text.isEmpty()) {
         QStandardItem *newItem = new QStandardItem(style()->standardIcon(QStyle::SP_FileIcon), text);
         QModelIndex currentIndex = pageTreeView->currentIndex();
         QStandardItem *parentItem = pageModel->invisibleRootItem(); // Default to root

         // If context menu triggered on an item, add as sibling
         // We differentiate between button press and context menu implicitly:
         // If an item is selected when this slot runs, assume it might be context menu on item.
         // Button press usually clears selection or doesn't rely on it here.
         // The showPageContextMenu logic enables 'Add Page' on items.
         if (currentIndex.isValid() && currentIndex.parent().isValid()) { // If selected item is a subpage
             parentItem = pageModel->itemFromIndex(currentIndex.parent()); // Add to its parent (sibling)
         } else if (currentIndex.isValid() && !currentIndex.parent().isValid()) { // If selected item is top-level
              parentItem = pageModel->invisibleRootItem(); // Add to root (sibling)
         }
         // If no index is valid (button press or context menu on empty space), parent remains root.


         parentItem->appendRow(newItem);
         QModelIndex newIndex = pageModel->indexFromItem(newItem);
         pageTreeView->setCurrentIndex(newIndex);
         pageTreeView->expand(newIndex.parent()); // Expand the parent
      }
 }


 // --- Context Menu Implementations ---

 void MainWindow::showSectionContextMenu(const QPoint &pos)
 {
     QModelIndex index = sectionTreeView->indexAt(pos);
     QMenu contextMenu(tr("Section Actions"), this);

     // Determine context
     bool onItemGroup = index.isValid() && sectionModel->itemFromIndex(index)->hasChildren(); // Simple check for group
     bool onItem = index.isValid();

     // Add actions based on context
     contextMenu.addAction(addSectionGroupAction); // Always allow adding a group? Or only on empty space? For now, always.
     addSectionAction->setEnabled(onItemGroup || !onItem); // Enable if on a group or empty space
     contextMenu.addAction(addSectionAction);

     // Add Delete/Rename later
     // if (onItem) {
     //     contextMenu.addSeparator();
     //     contextMenu.addAction(deleteItemAction);
     //     contextMenu.addAction(renameItemAction);
     // }

     contextMenu.exec(sectionTreeView->viewport()->mapToGlobal(pos));
 }

 void MainWindow::showPageContextMenu(const QPoint &pos)
 {
     QModelIndex index = pageTreeView->indexAt(pos);
     QMenu contextMenu(tr("Page Actions"), this);

     // Determine context
     bool onItem = index.isValid();
     bool isSubpage = onItem && index.parent().isValid(); // Check if it has a valid parent (not root)
     QStandardItem *item = onItem ? pageModel->itemFromIndex(index) : nullptr;

     // Add actions based on context
     addPageAction->setEnabled(!onItem || isSubpage); // Allow adding top-level if on empty space or subpage
     contextMenu.addAction(addPageAction);
     addSubpageAction->setEnabled(onItem); // Can only add subpage if an item is selected
     contextMenu.addAction(addSubpageAction);
     promoteSubpageAction->setEnabled(isSubpage); // Can only promote if it's a subpage
     contextMenu.addAction(promoteSubpageAction);

     // Add Delete/Rename later
     // if (onItem) {
     //     contextMenu.addSeparator();
     //     contextMenu.addAction(deleteItemAction);
     //     contextMenu.addAction(renameItemAction);
     // }

     contextMenu.exec(pageTreeView->viewport()->mapToGlobal(pos));
 }


 // --- New Action Slot Implementations ---

 void MainWindow::addSectionGroup()
 {
     QModelIndex currentNotebookIndex = notebookListView->currentIndex();
     if (!currentNotebookIndex.isValid()) {
         QMessageBox::warning(this, tr("Add Section Group"), tr("Please select a notebook first."));
         return;
     }

     bool ok;
     QString text = QInputDialog::getText(this, tr("Add Section Group"),
                                          tr("Group name:"), QLineEdit::Normal,
                                          "", &ok);
     if (ok && !text.isEmpty()) {
         QStandardItem *newItem = new QStandardItem(style()->standardIcon(QStyle::SP_DirClosedIcon), text); // Use a folder icon
         // Make it bold to visually distinguish?
         QFont font = newItem->font();
         font.setBold(true);
         newItem->setFont(font);
         // newItem->setFlags(newItem->flags() & ~Qt::ItemIsEditable); // Prevent editing group name easily?

         sectionModel->invisibleRootItem()->appendRow(newItem);
         sectionTreeView->setCurrentIndex(sectionModel->indexFromItem(newItem));
         sectionTreeView->expand(sectionModel->index(0,0).parent());
     }
 }


 void MainWindow::addSubpage()
 {
     QModelIndex currentPageIndex = pageTreeView->currentIndex();
     if (!currentPageIndex.isValid()) {
         QMessageBox::warning(this, tr("Add Subpage"), tr("Please select a parent page first."));
         return;
     }

     QStandardItem *parentItem = pageModel->itemFromIndex(currentPageIndex);
     if (!parentItem) return; // Should not happen if index is valid

     QString currentSectionName = pageTreeView->windowTitle(); // Get section from header? Less reliable.
     qDebug() << "Add Subpage clicked for page:" << parentItem->text() << "in section" << currentSectionName;


     bool ok;
     QString text = QInputDialog::getText(this, tr("Add Subpage"),
                                          tr("Subpage name:"), QLineEdit::Normal,
                                          "", &ok);
     if (ok && !text.isEmpty()) {
         QStandardItem *newItem = new QStandardItem(style()->standardIcon(QStyle::SP_FileIcon), text);
         parentItem->appendRow(newItem); // Append as child
         pageTreeView->expand(currentPageIndex); // Ensure parent is expanded
         pageTreeView->setCurrentIndex(pageModel->indexFromItem(newItem));
     }
 }

 void MainWindow::promoteSubpage()
 {
     QModelIndex currentSubpageIndex = pageTreeView->currentIndex();
     if (!currentSubpageIndex.isValid() || !currentSubpageIndex.parent().isValid()) {
         QMessageBox::warning(this, tr("Promote Subpage"), tr("Please select a subpage to promote."));
         return;
     }

     QStandardItem *subpageItem = pageModel->itemFromIndex(currentSubpageIndex);
     QStandardItem *parentItem = subpageItem->parent();
     if (!parentItem || parentItem == pageModel->invisibleRootItem()) {
         // Already a top-level item or something went wrong
         return;
     }

     // Determine the new parent (grandparent or root)
     QStandardItem *grandparentItem = parentItem->parent();
     if (!grandparentItem) { // Parent is top-level, promote to root
         grandparentItem = pageModel->invisibleRootItem();
     }

     // Take the row from the original parent and append it to the new parent
     // takeRow returns a QList<QStandardItem*>, we expect only one item (our subpage)
     QList<QStandardItem*> row = parentItem->takeRow(currentSubpageIndex.row());
     if (!row.isEmpty()) {
         grandparentItem->appendRow(row); // Append the item (and its children, if any)
         pageTreeView->setCurrentIndex(pageModel->indexFromItem(row.first())); // Select the promoted item
         pageTreeView->expand(pageModel->indexFromItem(grandparentItem)); // Ensure new parent is expanded
     }
 }


 // Make sure there are no stray characters after this final brace
