#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "codeeditor.h"
#include "utils.h"
#include <functional>
#include <stdio.h>
#include <QFile>
#include <QListView>
#include <QHeaderView>
#include <QTextEdit>
#include <QMdiSubWindow>
#include <QJsonDocument>
#include <QStandardItemModel>
#include <QFileDialog>
#include <QString>
#include <QMdiArea>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
    MDIArea(new QMdiArea(this)), currentCodebase("/Users/forseti/Desktop/GitHub/"),
    codebaseModel(std::make_unique<QFileSystemModel>(this)),
    codebaseBrowseTree(new FileNavigationTree(this)) {

    this->MDIArea->setAttribute(Qt::WA_DeleteOnClose, true);
    this->codebaseBrowseTree->setAttribute(Qt::WA_DeleteOnClose, true);

    ui->setupUi(this);
    this->setCentralWidget(this->MDIArea);
    this->MDIArea->show();

    // Setup the file browser:
    QFileSystemModel* const codeModel = this->codebaseModel.get();
    const QModelIndex rootIndex = codeModel->setRootPath(
        QString::fromStdString(this->currentCodebase.GetCodebasePath())
    );
    codeModel->setResolveSymlinks(false);
    this->codebaseBrowseTree->setModel(codeModel);
    this->codebaseBrowseTree->setRootIndex(rootIndex);
    QObject::connect(this->codebaseBrowseTree.get(), SIGNAL(OpenSelectedFile()), this, SLOT(OpenSelectedFile()));
    QMdiSubWindow* const navSubWindow = this->AddSubWindow(this->codebaseBrowseTree.get());
    navSubWindow->setWindowTitle("Project Navigation");
    navSubWindow->resize(600, 400);
}

QMdiSubWindow* MainWindow::AddSubWindow(QWidget* const widget) {
    // Create the window:
    QMdiSubWindow* const subWindow = this->MDIArea->addSubWindow(widget);

    this->AddBindings(reinterpret_cast<QWidget*>(widget));

    return subWindow;
}

void MainWindow::SpawnCodeViewer(const std::string& filePath) {
    // Create a memory-tracked CodeEditor (derived from QTextEdit):
    const std::string relPath = this->ToRelativePath(filePath);

    CodeEditor* const mainEditorsPtr = new CodeEditor(std::ref(currentCodebase), relPath, this);
    mainEditorsPtr->setAttribute(Qt::WA_DeleteOnClose, true);
    mainEditorsPtr->show();

    // Spawn the CodeEditor as a sub window of the MDI area:
    QMdiSubWindow* const editorSubWindow = this->AddSubWindow(mainEditorsPtr);
    editorSubWindow->resize(400, 400);
    editorSubWindow->setWindowTitle(/*"Code Viewer: "*/"\'" + QString::fromStdString(filePath).split('/').back() + "\'");
    editorSubWindow->show();
}

std::string MainWindow::ToRelativePath(const std::string& fullPath) const {
    const std::size_t baseLen = this->currentCodebase.GetCodebasePath().length();
    if (fullPath.length() <= baseLen) {
        throw std::runtime_error("Invalid 'fullPath' length");
    }
    return fullPath.substr(baseLen);
}

std::string MainWindow::ToFullPath(const std::string& relPath) const {
    return this->currentCodebase.GetCodebasePath() + relPath;
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::OpenSelectedFile() {
    FileNavigationTree* const tree = this->codebaseBrowseTree.get();
    QItemSelectionModel* const treeSelectionModel = tree->selectionModel();
    const QModelIndexList selectedIndexes = treeSelectionModel->selectedIndexes();

    QModelIndexList::const_reference selectedFileIndex = selectedIndexes.back();

    const QFileSystemModel* const localModel = reinterpret_cast<QFileSystemModel* const>(tree->model());
    const std::string pathStr = localModel->filePath(selectedFileIndex).toStdString();
    this->SpawnCodeViewer(pathStr);
}

void MainWindow::OpenBookmarks() {
    // Lots of dynamic allocation that isn't tracked here, note that from what I've read in Qt's documentation,
    // each of these nontracked allocations have their ownership transfered to whichever function they are passed
    // to currently.
    QTreeView* const listView = new QTreeView(this);
    listView->setAttribute(Qt::WA_DeleteOnClose, true);
    QStandardItemModel* itemModel = new QStandardItemModel(this);
    itemModel->setHorizontalHeaderLabels({"File", "Line #", "Code"});

    // Add the bookmarks' information into a model that can be sent to listView:
    this->currentCodebase.bookmarks.AddToModel(itemModel, this->currentCodebase.GetCodebasePath());

    // Apply the model to listView and then spawn a subwindow:
    listView->setModel(itemModel);
    listView->setSortingEnabled(true);
    listView->setEditTriggers(QAbstractItemView::NoEditTriggers); // Force readonly
    QMdiSubWindow* const newWindow = this->AddSubWindow(listView);
    newWindow->setWindowTitle(QString::number(itemModel->rowCount()) + " bookmark(s)");
    newWindow->show();
}

void MainWindow::OpenAnnotations() {
    // Lots of dynamic allocation that isn't tracked here, note that from what I've read in Qt's documentation,
    // each of these nontracked allocations have their ownership transfered to whichever function they are passed
    // to currently.
    QTreeView* const listView = new QTreeView(this);
    listView->setAttribute(Qt::WA_DeleteOnClose, true);
    QStandardItemModel* itemModel = new QStandardItemModel(this);

    itemModel->setHorizontalHeaderLabels({"File", "Line #", "Code", "Annotation"});
    this->currentCodebase.annotations.AddToModel(itemModel, this->currentCodebase.GetCodebasePath());

    listView->setModel(itemModel);
    listView->setSortingEnabled(true);
    listView->setEditTriggers(QAbstractItemView::NoEditTriggers); // Force readonly
    QMdiSubWindow* const newWindow = this->AddSubWindow(listView);
    newWindow->setWindowTitle(QString::number(itemModel->rowCount()) + " annotation(s)");
    newWindow->show();
}

void MainWindow::ExportProject() {
    const QUrl exportLocation = QFileDialog::getSaveFileUrl(this, "Export Location");
    QJsonObject jsonFormatted = this->currentCodebase.SerializeToJSON(Config::VR_Specifications::BLOCKS);

    QFile outputFile(exportLocation.toLocalFile());
    outputFile.open(QFile::OpenModeFlag::NewOnly);
    outputFile.write(QJsonDocument(jsonFormatted).toJson(QJsonDocument::Compact));
}

void MainWindow::ImportProject() {
    // Open the project file:
    const QUrl importLocation = QFileDialog::getOpenFileUrl(this, "Import Location");
    QFile inputFile(importLocation.toLocalFile());
    inputFile.open(QIODevice::ReadOnly | QIODevice::Text);

    // Read the file's contents and perform some minor cleanup:
    QByteArray fileBytes = inputFile.readAll();
    const QJsonDocument projectJSON = QJsonDocument::fromJson(fileBytes);
    fileBytes.clear();
    inputFile.close();

    if (!projectJSON.isObject() || projectJSON.isNull() || projectJSON.isEmpty()) {
        return;
    }

    Project newCodebase(this->currentCodebase.GetCodebasePath(), projectJSON.object(), Config::VR_Specifications::BLOCKS);
    this->currentCodebase = newCodebase; // moving ownership isn't required.

    this->ReloadAll();
}

void MainWindow::ReloadAll() {
    // Refresh the annotation/bookmark views:
    const QList<QMdiSubWindow*> subWindows = this->MDIArea->subWindowList();
    for (QMdiSubWindow* iterativeWindow : subWindows) {
        QTreeView* const treeView = qobject_cast<QTreeView*>(iterativeWindow->widget());
        if (treeView == nullptr) {
            CodeEditor* const codeWindow = qobject_cast<CodeEditor*>(iterativeWindow->widget());
            if (codeWindow != nullptr) {
                codeWindow->Reload();
            }
            continue;
        }

        const QString windowTitle = iterativeWindow->windowTitle();
        QStandardItemModel* const treeModel = reinterpret_cast<QStandardItemModel*>(treeView->model());
        if (windowTitle.endsWith(" annotation(s)")) {
            treeModel->clear();
            treeModel->setHorizontalHeaderLabels({"File", "Line #", "Code", "Annotation"});
            this->currentCodebase.annotations.AddToModel(treeModel, this->currentCodebase.GetCodebasePath());
            iterativeWindow->setWindowTitle(QString::number(treeModel->rowCount()) + " annotation(s)");
        }
        else if (windowTitle.endsWith(" bookmark(s)")) {
            treeModel->clear();
            treeModel->setHorizontalHeaderLabels({"File", "Line #", "Code"});
            this->currentCodebase.bookmarks.AddToModel(treeModel, this->currentCodebase.GetCodebasePath());
            iterativeWindow->setWindowTitle(QString::number(treeModel->rowCount()) + " bookmark(s)");
        }
        else {
            continue;
        }
    }
}

void MainWindow::AddBindings(QWidget* const widget) {
    // Assign the universal bindings:
    std::unique_ptr<QAction> generalActionUniquePtr;
    QAction* generalActionPtr = nullptr;
    NEW_KEYBIND("OPN_BOOKMARKS", QKeySequence(Qt::SHIFT | Qt::Key_B), OpenBookmarks, widget);
    NEW_KEYBIND("OPN_ANNOTATIONS", QKeySequence(Qt::SHIFT | Qt::Key_Semicolon), OpenAnnotations, widget);
    NEW_KEYBIND("EXPORT", QKeySequence(Qt::SHIFT | Qt::Key_E), ExportProject, widget);
    NEW_KEYBIND("IMPORT", QKeySequence(Qt::SHIFT | Qt::Key_I), ImportProject, widget);
    NEW_KEYBIND("RELOAD", QKeySequence(Qt::SHIFT | Qt::Key_R), ReloadAll, widget);
}
