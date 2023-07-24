#ifndef ANNOTATION_H
#define ANNOTATION_H
#include <stdio.h>
#include <string>
#include <vector>

struct Annotation {
    std::string contents;
    std::size_t linesOccupied;
    std::size_t lineRef;
    std::string fileRef;
    std::vector<std::string> keywords;
};

class AnnotationCollection {
private:
    std::unordered_map<std::string /* File Path */, std::vector<Annotation>> annotations;
    std::vector<Annotation>::const_iterator GetAnnotationIter(const std::string& path, const std::size_t lineRef);
public:
    // Reverse of ResolveToCodeLineRef.
    std::size_t ResolveToEditLineRef(const std::string& path, const std::size_t codeLineRef);
    // Takes a line (including annotations) and converts it to the nearest (fwd.) line number
    // with actual immutable data/code on it.
    std::size_t ResolveToCodeLineRef(const std::string& path, const std::size_t rawLineRef);
    void AddNewAnnotation(const Annotation& annotationData);
    void RemoveAnnotation(const std::string& path, const std::size_t lineRef);
    std::vector<Annotation> GetAnnotations(const std::string& path);
    Annotation GetAnnotation(const std::string& path, const std::size_t lineRef);
};

#endif // ANNOTATION_H
