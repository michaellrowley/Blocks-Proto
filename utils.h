#ifndef UTILS_H
#define UTILS_H
#include "annotation.h"
#include "bookmark.h"

#define NEW_KEYBIND(ACTION_STRNAME, ACTION_KEYSEQ, ACTION_SLOT, TARGET) \
    generalActionUniquePtr = std::make_unique<QAction>(this); \
    generalActionPtr = generalActionUniquePtr.get(); \
    generalActionPtr->setShortcut(ACTION_KEYSEQ); \
    generalActionPtr->setShortcutContext(Qt::WidgetWithChildrenShortcut); \
    QObject::connect(generalActionPtr, SIGNAL(triggered()), SLOT(ACTION_SLOT())); \
    TARGET->addAction(generalActionPtr); \
    this->keyBindings[ACTION_STRNAME].push_back(std::move(generalActionUniquePtr));

#endif // UTILS_H
