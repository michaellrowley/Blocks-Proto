#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <memory>
#include "codeeditor.h"
#include "project.h"

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
    Project currentCodebase;
    std::unique_ptr<QVBoxLayout> windowLayout;
    std::unique_ptr<CodeEditor> mainEditor;
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
