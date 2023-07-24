#include "annotation.h"
#include <QDebug>
#include <algorithm>

void AnnotationCollection::AddNewAnnotation(const Annotation& annotationData) {
    const std::string& filePath = annotationData.fileRef;
    std::unordered_map<std::string, std::vector<Annotation>>::iterator annotationsVec;
    if ((annotationsVec = this->annotations.find(filePath)) == this->annotations.end()) {
        // Need to *insert* an annotation:
        annotationsVec = this->annotations.insert(
            annotationsVec,
            std::pair<std::string, std::vector<Annotation>>(
                filePath, std::vector<Annotation>{annotationData}
            )
        );
    }
    else {
        // Append to the iterator:
        annotationsVec->second.push_back(annotationData);
    }

    // Sort the annotations vector into ascending order:
    std::sort(annotationsVec->second.begin(), annotationsVec->second.end(),
        [](const Annotation& a, const Annotation& b) {
            return a.lineRef < b.lineRef;
        }
    );
}

std::vector<Annotation> AnnotationCollection::GetAnnotations(const std::string& path) {
    std::unordered_map<std::string, std::vector<Annotation>>::iterator matchingVec;
    if ((matchingVec = this->annotations.find(path)) == this->annotations.end()) {
        return std::vector<Annotation>(0); // Return an empty array upon failure.
    }
    return matchingVec->second;
}

void AnnotationCollection::RemoveAnnotation(const std::string& path, const std::size_t lineRef) {
    std::unordered_map<std::string, std::vector<Annotation>>::iterator matchingVec;
    if ((matchingVec = this->annotations.find(path)) == this->annotations.end()) {
        throw std::runtime_error("Unable to find annotation file entry");
    }
    std::vector<Annotation>& fileAnnotations = matchingVec->second;
    fileAnnotations.erase(this->GetAnnotationIter(path, lineRef));
}

std::vector<Annotation>::const_iterator AnnotationCollection::GetAnnotationIter(
        const std::string& path, const std::size_t lineRef) {
    std::unordered_map<std::string, std::vector<Annotation>>::iterator matchingVec;
    if ((matchingVec = this->annotations.find(path)) == this->annotations.end()) {
        throw std::runtime_error("Unable to find annotation");
    }

    std::vector<Annotation>& fileAnnotations = matchingVec->second;

    const std::vector<Annotation>::const_iterator matchingAnnotation = std::find_if(
        fileAnnotations.begin(), fileAnnotations.end(), [&lineRef](Annotation& sample){
            return sample.lineRef == lineRef;
        }
    );
    if (matchingAnnotation == fileAnnotations.cend()) {
        throw std::runtime_error("Unable to find annotation");
    }

    return matchingAnnotation;
}

Annotation AnnotationCollection::GetAnnotation(const std::string& path,
                                               const std::size_t lineRef) {
    return *this->GetAnnotationIter(path, lineRef);
}

std::size_t AnnotationCollection::ResolveToEditLineRef(const std::string& path,
                                                       const std::size_t codeLineRef) {

    const std::vector<Annotation> annotations = this->GetAnnotations(path);
    std::size_t adjustedLineRef = codeLineRef/* + delta (= 0)*/;
    for (const Annotation& iterativeAnnotation : annotations) {
        const std::size_t curDelta = adjustedLineRef - codeLineRef;
        if (codeLineRef > iterativeAnnotation.lineRef + curDelta) {
            adjustedLineRef += 1;//iterativeAnnotation.linesOccupied;
        }
        else {
            break;
        }
    }

    return adjustedLineRef;
}

std::size_t AnnotationCollection::ResolveToCodeLineRef(const std::string& path,
                                                       const std::size_t rawLineRef) {
    const std::vector<Annotation> annotations = this->GetAnnotations(path);
    std::size_t currentDelta = 0;
    for (const Annotation& iterativeAnnotation : annotations) {
        const std::size_t iterativeLinesOccupied = iterativeAnnotation.linesOccupied;
        const std::size_t annotationLineRef = iterativeAnnotation.lineRef;
        const std::size_t codeLineRef = rawLineRef - currentDelta;
        if (annotationLineRef >= codeLineRef) {
            break;
        }
        else if (annotationLineRef < codeLineRef + iterativeLinesOccupied) {
            const std::size_t overstep = codeLineRef - annotationLineRef;
            if (overstep <= iterativeLinesOccupied) {
                return codeLineRef - overstep;
            }
            else {
                currentDelta += iterativeLinesOccupied;
            }
            continue;
        }
        else {
            continue;
        }
    }

    return rawLineRef - currentDelta;
}
