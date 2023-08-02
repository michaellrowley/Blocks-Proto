#include "bookmark.h"
#include <algorithm>
#include <QFile>
#include <QJsonArray>

QJsonObject Bookmark::SerializeToJSON(const Config::VR_Specifications conformingSpecification) const {
    QJsonObject serialized;
    switch (conformingSpecification) {
        case Config::VR_Specifications::BLOCKS: {
            // In Blocks, the JSON schema is just the entire relevant structure.
            // There is some overlap with Snippet's members (lineRef, fileRef, contents, tags, etc)
            // but the naming of those variables differs wildly and Blocks is open
            // to more rapid incremental adjustments (including member types).
//            serialized["file"] = this->fileRef.c_str();
            serialized["line"] = static_cast<qint64>(this->lineRef);
            break;
        }
        case Config::VR_Specifications::SNIPPET: // Snippet doesn't support bookmarks.
        default:
            throw std::runtime_error("Invalid specification");
            break;
    }

    return serialized;
}

BookmarkCollection::BookmarkCollection() {}

BookmarkCollection::BookmarkCollection(QJsonObject bookmarksJSON, Config::VR_Specifications specification) {
    if (specification != Config::VR_Specifications::BLOCKS) {
        throw std::runtime_error("Invalid/Unsupported specification whilst deserializing to bookmarks");
    }

    bookmarksJSON = bookmarksJSON["bookmarks"].toObject();

    const QStringList filePaths = bookmarksJSON.keys();

    for (const QString& iterativeFile : filePaths) {

        const QJsonObject fileEntry = bookmarksJSON[iterativeFile].toObject();
        const QString filePath = fileEntry["file"].toString();
        const QJsonArray fileBookmarks = fileEntry["bookmarks"].toArray();

        for (const QJsonValueRef& iterativeBookmark : fileBookmarks) {
            const QJsonObject bookmarkObject = iterativeBookmark.toObject();

            Bookmark bookmarkVar (
                filePath.toStdString(), static_cast<std::size_t>(bookmarkObject["line"].toInt())
            );

            this->AddBookmark(std::move(bookmarkVar));
        }
    }
}

void BookmarkCollection::AddBookmark(const Bookmark& bookmarkData) {
    std::unordered_map<std::string, std::vector<Bookmark>>::iterator fileElement = this->bookmarks.find(bookmarkData.fileRef);
    if (fileElement == this->bookmarks.cend()) {
        this->bookmarks[bookmarkData.fileRef] = std::vector<Bookmark>{bookmarkData};
    }
    else {
        for (std::size_t i = 0; i < fileElement->second.size(); i++) {
            if (bookmarkData.lineRef < fileElement->second[i].lineRef) {
                fileElement->second.insert(fileElement->second.begin() + i, bookmarkData);
                return;
            }
        }
        fileElement->second.push_back(bookmarkData);
    }
}

void BookmarkCollection::RemoveBookmark(const std::string& fileRef, std::size_t lineRef) {
    std::unordered_map<std::string, std::vector<Bookmark>>::iterator file = this->bookmarks.find(fileRef);
    if (file == this->bookmarks.end()) {
        throw std::runtime_error("Unable to locate bookmark");
    }

    for (std::vector<Bookmark>::iterator sample = file->second.begin(); sample < file->second.end(); sample++) {
        if (sample->lineRef == lineRef) {
            file->second.erase(sample);
            if (file->second.size() == 0) {
                this->bookmarks.erase(file);
            }
            return;
        }
    }

    throw std::runtime_error("Unable to locate bookmark");
}

std::vector<Bookmark> BookmarkCollection::GetBookmarks(const std::string& fileRef) const {
    std::unordered_map<std::string, std::vector<Bookmark>>::const_iterator file = this->bookmarks.find(fileRef);
    return (file == this->bookmarks.cend()) ? std::vector<Bookmark>{} : file->second;
}

std::vector<Bookmark> BookmarkCollection::GetBookmarks() const {
    std::vector<Bookmark> bookmarks;

    for (std::unordered_map<std::string, std::vector<Bookmark>>::const_iterator bookmarkIterator = this->bookmarks.cbegin();
                    bookmarkIterator != this->bookmarks.cend(); bookmarkIterator++) {

        std::for_each(bookmarkIterator->second.cbegin(), bookmarkIterator->second.cend(), [&bookmarks](const Bookmark& bookmark) {
            bookmarks.push_back(bookmark);
        });

    }

    return bookmarks;
}

std::unordered_map<std::string, std::vector<Bookmark>> BookmarkCollection::GetRawBookmarks() const {
    return this->bookmarks;
}

void BookmarkCollection::AddToModel(QStandardItemModel* const model, const std::string& basePath) const {
    std::size_t rowIndex = model->rowCount();
    for (const std::pair<std::string, std::vector<Bookmark>>& bookmarkCollection : this->bookmarks) {
        // Read the annotated file's contents. TODO: Find a way to only read relevant lines (not the entire contents).
        QFile fileObj(QString::fromStdString(basePath + bookmarkCollection.first));
        fileObj.open(QIODevice::ReadOnly | QIODevice::Text);
        QByteArray rawFileContents = fileObj.readAll();
        fileObj.close();

        // Cast the file's data (in the form of bytes) to a string and format it:
        const QStringList fileLines = QString::fromStdString(rawFileContents.toStdString()).split('\n');

        for (const Bookmark& bookmark : bookmarkCollection.second) {

            if (fileLines.length() <= bookmark.lineRef) {
                throw std::runtime_error("OOB bookmark");
            }

            model->setItem(rowIndex, 0, new QStandardItem(QString::fromStdString(bookmark.fileRef)));
            model->setItem(rowIndex, 1, new QStandardItem(QString::number(bookmark.lineRef)));
            model->setItem(rowIndex, 2, new QStandardItem(fileLines[bookmark.lineRef].simplified()));
            ++rowIndex;
        }
    }
}
