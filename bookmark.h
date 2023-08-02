#ifndef BOOKMARK_H
#define BOOKMARK_H
#include <iostream>
#include <QJsonObject>
#include <map>
#include <QStandardItemModel>
#include <QTreeView>
#include "configuration.h"

struct Bookmark {
    std::string fileRef;
    std::size_t lineRef;
    Bookmark(const std::string& fileRef, const std::size_t lineRef) :
        fileRef(fileRef), lineRef(lineRef) {}
    QJsonObject SerializeToJSON(const Config::VR_Specifications conformingSpecification) const;
};

struct BookmarkCollection {
public: // Default:
    BookmarkCollection();
    BookmarkCollection(QJsonObject bookmarksJSON, Config::VR_Specifications specification);

    void AddBookmark(const Bookmark& bookmarkData);
    void RemoveBookmark(const std::string& fileRef, std::size_t lineRef);
    std::vector<Bookmark> GetBookmarks(const std::string& fileRef) const;
    std::vector<Bookmark> GetBookmarks() const;
    std::unordered_map<std::string, std::vector<Bookmark>> GetRawBookmarks() const;

    void AddToModel(QStandardItemModel* const model, const std::string& basePath) const;
private:
    std::unordered_map<std::string, std::vector<Bookmark>> bookmarks;
};

//typedef std::vector<Bookmark> BookmarkCollection;

#endif // BOOKMARK_H
