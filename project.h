#ifndef PROJECT_H
#define PROJECT_H
#include "bookmark.h"
#include "annotation.h"
#include "configuration.h"
#include <QJsonObject>
#include <filesystem>

class Project {
    std::string codebasePath;
public:
    Project(const std::filesystem::path& codebasePath);
    Project(const std::filesystem::path& codebasePath, const QJsonObject& projectJSON,
            Config::VR_Specifications specification);

    AnnotationCollection annotations;
    BookmarkCollection bookmarks;
    std::string GetCodebasePath() const;
    QJsonObject SerializeToJSON(const Config::VR_Specifications& specification) const;
private:
};

#endif // PROJECT_H
