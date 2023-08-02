#include "annotation.h"
#include <algorithm>
#include <QJsonArray>
#include <QFile>

void AnnotationCollection::AddNewAnnotation(Annotation annotationData) {

    // Handle the linesOccupied member calculation here to avoid code duplication:
    annotationData.linesOccupied = std::count(
        annotationData.contents.cbegin(),
        annotationData.contents.cend(),
        '\n'
    ) + 1;

    annotationData.UpdateKeywords();

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

std::vector<Annotation> AnnotationCollection::GetAnnotations(const std::string& path) const {
    std::unordered_map<std::string, std::vector<Annotation>>::const_iterator matchingVec = this->annotations.find(path);
    if (matchingVec == this->annotations.cend()) {
        return std::vector<Annotation>(0); // Return an empty array upon failure.
    }
    return matchingVec->second;
}

std::vector<Annotation> AnnotationCollection::GetAnnotations() const {
    std::vector<Annotation> annotations;

    for (std::unordered_map<std::string, std::vector<Annotation>>::const_iterator i = this->annotations.cbegin();
         i != this->annotations.cend(); i++) {
        for (const Annotation& annotation : i->second) {
            annotations.push_back(annotation);
        }
    }

    return annotations;
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
        const std::string& path, const std::size_t lineRef) const {
    std::unordered_map<std::string, std::vector<Annotation>>::const_iterator matchingVec = this->annotations.find(path);
    if (matchingVec == this->annotations.end()) {
        throw std::runtime_error("Unable to find annotation");
    }

    const std::vector<Annotation>& fileAnnotations = matchingVec->second;

    const std::vector<Annotation>::const_iterator matchingAnnotation = std::find_if(
        fileAnnotations.begin(), fileAnnotations.end(), [&lineRef](const Annotation& sample){
            return sample.lineRef == lineRef;
        }
    );
    if (matchingAnnotation == fileAnnotations.cend()) {
        throw std::runtime_error("Unable to find annotation");
    }

    return matchingAnnotation;
}

Annotation AnnotationCollection::GetAnnotation(const std::string& path,
                                               const std::size_t lineRef) const {
    return *this->GetAnnotationIter(path, lineRef);
}

std::unordered_map<std::string, std::vector<Annotation>> AnnotationCollection::GetRawAnnotations() const {
    return this->annotations;
}

std::size_t AnnotationCollection::ResolveToEditLineRef(const std::string& path,
                                                       const std::size_t codeLineRef) const {

    const std::vector<Annotation> annotations = this->GetAnnotations(path);
    std::size_t adjustedLineRef = codeLineRef/* + delta (= 0)*/;
    for (const Annotation& iterativeAnnotation : annotations) {
        const std::size_t curDelta = adjustedLineRef - codeLineRef;
        if (codeLineRef > iterativeAnnotation.lineRef + curDelta) {
            adjustedLineRef += 1;
        }
        else {
            break;
        }
    }

    return adjustedLineRef;
}

std::size_t AnnotationCollection::ResolveToCodeLineRef(const std::string& path,
                                                       const std::size_t rawLineRef) const {
    const std::vector<Annotation> annotations = this->GetAnnotations(path);
    std::size_t currentDelta = 0;
    for (const Annotation& iterativeAnnotation : annotations) {
        const std::size_t iterativeLinesOccupied = iterativeAnnotation.linesOccupied;
        const std::size_t annotationLineRef = iterativeAnnotation.lineRef;
        const std::size_t codeLineRef = rawLineRef - currentDelta;
        // Check if the iterative annotation is past where the user is looking and
        // if so, break as all of the annotation lineRefs/linesOccupied have been
        // accounted for.
        if (annotationLineRef >= codeLineRef) {
            break;
        }
        else if (annotationLineRef < codeLineRef + iterativeLinesOccupied) {
            // Account for part/all of the annotation's occupied lines:
            const std::size_t overstep = codeLineRef - annotationLineRef;
            if (overstep <= iterativeLinesOccupied) {
                // Part:
                return codeLineRef - overstep;
            }
            else {
                // All:
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

QJsonObject Annotation::SerializeToJSON(const Config::VR_Specifications conformingSpecification) const {

    QJsonObject serialized;
    switch (conformingSpecification) {
        case Config::VR_Specifications::SNIPPET: {
            // In Snippet, the JSON schema outlines:
            // string("id") - DB table index
            // string("filename") - *Relative* path & filename of this file
            // int("linenum") - Line number *indexed from 1*
            // string("txt") - Annotation contents
            // string("author") - User ID of the author
            // string("timestamp") - Most recent edit timestamp
            // string("ctimestamp") - Creation timestamp
            // string[]("tags") - Tokens/Keywords associated w/ this annotation
            // string("version") - *File* version (not Blocks/Snippet version)

            //...
            throw std::runtime_error("Unsupported");

            break;
        }
        case Config::VR_Specifications::BLOCKS: {
            // In Blocks, the JSON schema is just the entire relevant structure.
            // There is some overlap with Snippet's members (lineRef, fileRef, contents, tags, etc)
            // but the naming of those variables differs wildly and Blocks is open
            // to more rapid incremental adjustments (including member types).
//            serialized["file"] = this->fileRef.c_str();
            serialized["line"] = static_cast<qint64>(this->lineRef);
            serialized["contents"] = this->contents.c_str();
            QJsonArray keywordsArr = {};
            for (const std::string& keyword : this->keywords) {
                // Inserting at the start to maintain ordering, not required but a nice touch:
                keywordsArr.insert(0, keyword.c_str());
            }
            serialized["keywords"] = keywordsArr;
            break;
        }
        default:
            throw std::runtime_error("Invalid specification");
            break;
    }

    return serialized;
}

void AnnotationCollection::AddToModel(QStandardItemModel* const model, const std::string& basePath) const {
    std::size_t rowIndex = model->rowCount();
    for (const std::pair<std::string, std::vector<Annotation>>& annotationSet : this->annotations) {
        // Read the annotated file's contents. TODO: Find a way to only read the lines in question (not the entire contents).
        QFile fileObj(QString::fromStdString(basePath + annotationSet.first));
        fileObj.open(QIODevice::ReadOnly | QIODevice::Text);
        QByteArray rawFileContents = fileObj.readAll();
        fileObj.close();

        // Cast the file's bytes to a string
        const QStringList fileLines = QString::fromStdString(rawFileContents.toStdString()).split('\n');

        // Iterate over and insert the annotations
        for (const Annotation& annotation : annotationSet.second) {
            if (fileLines.length() <= annotation.lineRef) {
                throw std::runtime_error("OOB annotation");
            }
            model->setItem(rowIndex, 0, new QStandardItem(QString::fromStdString(annotation.fileRef)));
            model->setItem(rowIndex, 1, new QStandardItem(QString::number(annotation.lineRef)));
            model->setItem(rowIndex, 2, new QStandardItem(fileLines[annotation.lineRef].simplified()));
            model->setItem(rowIndex, 3, new QStandardItem(QString::fromStdString(annotation.contents)));
            ++rowIndex;
        }
    }
}

AnnotationCollection::AnnotationCollection() {}

AnnotationCollection::AnnotationCollection(QJsonObject annotationsJSON, Config::VR_Specifications specification) {
    if (specification != Config::VR_Specifications::BLOCKS) {
        throw std::runtime_error("Invalid/Unsupported specification whilst deserializing to annotation");
    }

    annotationsJSON = annotationsJSON["annotations"].toObject();

    const QStringList filePaths = annotationsJSON.keys();

    for (const QString& iterativeFile : filePaths) {

        const QJsonObject fileEntry = annotationsJSON[iterativeFile].toObject();
        const QString filePath = fileEntry["file"].toString();
        const QJsonArray fileAnnotations = fileEntry["annotations"].toArray();

        for (const QJsonValueRef& iterativeAnnotation : fileAnnotations) {
            const QJsonObject annotationObject = iterativeAnnotation.toObject();

            // No need to fill 'keywords' or 'linesOccupied' as these members are
            // dynamically calculated by the addition function (AddNewAnnotation())
            Annotation annotationVar {
                .contents = annotationObject["contents"].toString().toStdString(),
                .lineRef = static_cast<std::size_t>(annotationObject["line"].toInt()),
                .fileRef = filePath.toStdString()
            };

            this->AddNewAnnotation(std::move(annotationVar));
        }
    }
}

void Annotation::UpdateKeywords() {
    this->keywords.clear();

    // Search for all of the tokens contained in the annotation's contents
    std::size_t searchPos = 0;
    bool firstSearch = true;
    const std::string annotationContents = this->contents;
    while ((searchPos = annotationContents.find('#', firstSearch ? 0 : searchPos + 1)) != std::string::npos) {
        if (firstSearch) {
            firstSearch = false;
        }
        std::unordered_map<char, char>::const_iterator oppositeCutoff = Annotation::OppositeCutoffs.cend();
        if (searchPos > 0) {
            const char preceedingChar = annotationContents[searchPos - 1];
            oppositeCutoff = Annotation::OppositeCutoffs.find(preceedingChar);
        }
        std::size_t cutoffIndex = 0; // Initializing out of habit, it makes no difference.
        for (cutoffIndex = searchPos + 1;
             cutoffIndex < annotationContents.length(); cutoffIndex++) {
            if (oppositeCutoff == Annotation::OppositeCutoffs.cend() &&
                std::find(Annotation::CutoffChars.cbegin(), Annotation::CutoffChars.cend(), annotationContents[cutoffIndex]) !=
                    Annotation::CutoffChars.cend()) {
                break;
            }
        }
        const std::size_t tokenLength = (cutoffIndex - 1) - ++searchPos;
        if (tokenLength > 0) {
            this->keywords.push_back(annotationContents.substr(searchPos, tokenLength));
        }
    }
    const QStringList keywordList = QString::fromStdString(this->contents).split('#');
    for (std::size_t i = 1; i < keywordList.size(); i++) {
        QString iterativeKeyword = keywordList[i];
        for (const char& cutoffChar : Annotation::CutoffChars) {
            iterativeKeyword = iterativeKeyword.split(cutoffChar)[0];
        }
        this->keywords.push_back(iterativeKeyword.toStdString());
    }
}
