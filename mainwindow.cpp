#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "codeeditor.h"
#include <stdio.h>
#include <QFile>
#include <QTextEdit>
#include <QString>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , windowLayout(std::make_unique<QVBoxLayout>(this)), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->mainEditor = std::make_unique<CodeEditor>(
        std::ref(currentCodebase),
        "/Users/forseti/Desktop/GitHub/blocks/main.cpp",
        this
    );
    CodeEditor* const mainEditorPtr = this->mainEditor.get();
    mainEditorPtr->setAttribute(Qt::WA_DeleteOnClose, false);
    mainEditorPtr->setParent(this->ui->centralwidget);

    QVBoxLayout* const layoutPtr = this->windowLayout.get();
    layoutPtr->addWidget(mainEditorPtr);
    ui->centralwidget->setLayout(layoutPtr);
}

MainWindow::~MainWindow()
{
    delete ui;
}

