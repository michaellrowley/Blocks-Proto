#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QMdiArea>
#include <memory>
#include <vector>
#include "codeeditor.h"
#include "project.h"
#include "filenavigationtree.h"
#include <QFileSystemModel>
#include <QStandardItemModel>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *const ui;
    QMdiArea* const MDIArea;
    std::unique_ptr<QFileSystemModel> codebaseModel;
    std::unique_ptr<FileNavigationTree> codebaseBrowseTree;

    std::unordered_map<std::string, std::vector<std::unique_ptr<QAction>>> keyBindings;

    Project currentCodebase;
    void SpawnCodeViewer(const std::string& filePath);
    QMdiSubWindow* AddSubWindow(QWidget* const widget);
    void AddBindings(QWidget* const widget);

    std::string ToRelativePath(const std::string& fullPath) const;
    std::string ToFullPath(const std::string& relPath) const;
public slots:
    void ReloadAll();
    void ImportProject();
    void ExportProject();
    void OpenBookmarks();
    void OpenAnnotations();
    void OpenSelectedFile();
};

#endif // MAINWINDOW_H
