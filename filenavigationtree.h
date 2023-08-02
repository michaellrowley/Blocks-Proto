#ifndef FILENAVIGATIONTREE_H
#define FILENAVIGATIONTREE_H

#include <QTreeView>
#include <QObject>
#include <QWidget>
#include <filesystem>
#include <QFileSystemModel>
#include <functional>
#include <memory>
#include <map>

class FileNavigationTree : public QTreeView
{
    Q_OBJECT
signals:
    void OpenSelectedFile();
public:
    FileNavigationTree(QWidget* const parent);
private:
    const std::function<void(const std::string&)> editorSpawnCallback;
protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
};

#endif // FILENAVIGATIONTREE_H
