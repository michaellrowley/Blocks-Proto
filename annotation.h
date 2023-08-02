#ifndef ANNOTATION_H
#define ANNOTATION_H
#include <stdio.h>
#include <string>
#include <vector>
#include <QJsonObject>
#include <QStandardItemModel>
#include <QTreeView>
#include "configuration.h"

struct Annotation {
    std::string contents;
    std::size_t linesOccupied;
    std::size_t lineRef;
    std::string fileRef;
    std::vector<std::string> keywords;

    const inline static std::vector<char> CutoffChars = {
        ' ', '\t', '\n', '\r', '\v', '.'
    };
    const inline static std::unordered_map<char, char> OppositeCutoffs = {
        { '\'', '\'' },
        { '(', ')' },
        { '[', ']' },
        { '{', '}' },
        { '\"', '\"' },
        { '*', '*' }
    };
    QJsonObject SerializeToJSON(const Config::VR_Specifications conformingSpecification) const;

    void UpdateKeywords();
};

class AnnotationCollection {
private:
    std::unordered_map<std::string /* File Path */, std::vector<Annotation>> annotations;
    std::vector<Annotation>::const_iterator GetAnnotationIter(const std::string& path, const std::size_t lineRef) const;

public:

    AnnotationCollection();
    AnnotationCollection(QJsonObject annotationsJSON, Config::VR_Specifications specification);

    // Reverse of ResolveToCodeLineRef.
    std::size_t ResolveToEditLineRef(const std::string& path, const std::size_t codeLineRef) const;
    // Takes a line (including annotations) and converts it to the nearest (fwd.) line number
    // with actual immutable data/code on it.
    std::size_t ResolveToCodeLineRef(const std::string& path, const std::size_t rawLineRef) const;
    void AddNewAnnotation(Annotation annotationData);
    void RemoveAnnotation(const std::string& path, const std::size_t lineRef);
    std::vector<Annotation> GetAnnotations(const std::string& path) const;
    std::vector<Annotation> GetAnnotations() const;
    std::unordered_map<std::string, std::vector<Annotation>> GetRawAnnotations() const;
    Annotation GetAnnotation(const std::string& path, const std::size_t lineRef) const;

    void AddToModel(QStandardItemModel* const model, const std::string& basePath) const;
};

#endif // ANNOTATION_H
