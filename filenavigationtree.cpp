#include "filenavigationtree.h"
#include <QMouseEvent>

FileNavigationTree::FileNavigationTree(QWidget* const parent) : QTreeView(parent) {
    this->setSortingEnabled(true);
}

void FileNavigationTree::mouseDoubleClickEvent(QMouseEvent *event) {
    emit OpenSelectedFile();
}
