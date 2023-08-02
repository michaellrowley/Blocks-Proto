#include <QJsonArray>
#include "project.h"

Project::Project(const std::filesystem::path& codebasePath) : codebasePath(codebasePath.string()) {
    if (!std::filesystem::exists(codebasePath)) {
        throw std::runtime_error("Invalid codebase path passed to Project::Project (constructor).");
    }
}

Project::Project(const std::filesystem::path& basePath, const QJsonObject& projectJSON,
                 Config::VR_Specifications specification) :
        codebasePath(basePath), annotations(projectJSON, specification),
        bookmarks(projectJSON, specification) {

    if (!std::filesystem::exists(codebasePath)) {
        throw std::runtime_error("Invalid codebase path passed to Project::Project (constructor).");
    }
}

std::string Project::GetCodebasePath() const {
    return this->codebasePath;
}

QJsonObject Project::SerializeToJSON(const Config::VR_Specifications& specification) const {
    QJsonObject result;

    switch (specification) {
        case Config::VR_Specifications::BLOCKS: {
#define STRINGIFY(VAL) #VAL
#define INSERT_COMPONENTS(NAME, CAPITALIZED_NAME) \
            const std::unordered_map<std::string, std::vector<CAPITALIZED_NAME>> NAME##Map = this->NAME.GetRaw##CAPITALIZED_NAME##s(); \
            QJsonObject NAME##Json; \
            for (const std::pair<std::string, std::vector<CAPITALIZED_NAME>>& specificFile : NAME##Map) { \
                QJsonObject fileObject; \
                fileObject["file"] = specificFile.first.c_str(); \
                QJsonArray NAME##Array; \
                for (const CAPITALIZED_NAME& iterative##CAPITALIZED_NAME : specificFile.second) { \
                    QJsonObject NAME##Object = iterative##CAPITALIZED_NAME.SerializeToJSON(specification); \
                    NAME##Array.push_front(NAME##Object); \
                } \
                fileObject[STRINGIFY(NAME)] = NAME##Array; \
                NAME##Json[specificFile.first.c_str()] = fileObject; \
            } \
            result[STRINGIFY(NAME)] = NAME##Json;

            INSERT_COMPONENTS(annotations, Annotation)
            INSERT_COMPONENTS(bookmarks, Bookmark)
#undef INSERT_COMPONENTS

            break;
        }
    default:
        throw std::runtime_error("Unsupported serialization specification");
    }

    return result;
}
