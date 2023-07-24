#include "codeeditor.h"
#include "configuration.h"
#include <QFile>
#include <QDebug>
#include <QKeyEvent>
#include <QPushButton>
#include <algorithm>
#include <math.h>
#include "ui_annotationeditor.h"
#include "annotation.h"

CodeEditor::CodeEditor(Project& project, const std::string& path, QWidget* const parent) :
    filePath(path), activeProject(project)
{
    this->setParent(parent);

    this->setReadOnly(true);

    // TIL: https://www.qtcentre.org/threads/39941-readonly-QTextEdit-with-visible-Cursor
    this->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);

    // Setup shortcuts:
    std::unique_ptr<QAction> annotateAction = std::make_unique<QAction>(this);
    QAction* const annotateActionPtr = annotateAction.get();
    this->keyBindings["ADD_ANNOTATION"] = std::move(annotateAction);
    annotateActionPtr->setShortcut(QKeySequence(Qt::Key_Semicolon));
    QObject::connect(annotateActionPtr, SIGNAL(triggered()), SLOT(BeginAnnotation()));
    this->addAction(annotateActionPtr);

    this->LoadFile(this->filePath);
}

void CodeEditor::AnnotationSubmit() {
    Ui_Dialog* const editorDialog = this->activeAnnotationData.editor.get();
    QDialog* const parentDialog = this->activeAnnotationData.editorParentDialog.get();
    if (editorDialog == nullptr || parentDialog == nullptr) {
        return;
    }

    // Update the Annotation structure:
    Annotation& editableAnnotation = this->activeAnnotationData.activeAnnotation;
    editableAnnotation.contents = editorDialog->plainTextEdit->toPlainText().toStdString();
    editableAnnotation.linesOccupied = std::count(
        editableAnnotation.contents.cbegin(),
        editableAnnotation.contents.cend(),
        '\n'
    ) + 1;

    const QStringList keywordList = QString::fromStdString(editableAnnotation.contents).split('#');
    for (std::size_t i = 1; i < keywordList.size(); i++) {
        const std::string iterativeKeyword = keywordList[i].split(' ')[0].toStdString();
        editableAnnotation.keywords.push_back(iterativeKeyword);
    }

    // Close the QDialog:
    parentDialog->accept();
}

void CodeEditor::BeginAnnotation() {
    std::size_t lineReference = static_cast<std::size_t>(this->textCursor().blockNumber());
    lineReference = this->activeProject.get().annotations.ResolveToCodeLineRef(this->filePath, lineReference);
    std::string duplicateAnnotationContents("");
    bool isEdit = false;
    try {
        const Annotation duplicateAnnotation =
            this->activeProject.get().annotations.GetAnnotation(this->filePath, lineReference);
        duplicateAnnotationContents = duplicateAnnotation.contents;
        isEdit = true;
    } catch (...) {
        // The above code will throw if there was not an annotation at lineReference.
        isEdit = false; // Default value anyway.
    }
    this->activeAnnotationData.activeAnnotation = {
        .contents = isEdit ? duplicateAnnotationContents : "",
        .linesOccupied = 0,
        .lineRef = lineReference,
        .fileRef = this->filePath,
        .keywords = std::vector<std::string>(0)
    };


    // UI Setup:
    this->activeAnnotationData.editor = std::make_unique<Ui_Dialog>();

    QDialog* const newDialog = (
        this->activeAnnotationData.editorParentDialog =
        std::make_unique<QDialog>(this)
    ).get();
    newDialog->setAttribute(Qt::WA_DeleteOnClose, false);
    Ui_Dialog* const editorDialog = this->activeAnnotationData.editor.get();
    editorDialog->setupUi(newDialog);
    editorDialog->plainTextEdit->setPlainText(QString::fromStdString(duplicateAnnotationContents));
    editorDialog->label->setText(isEdit ? "Edit Annotation" : "New Annotation");


    QObject::connect(editorDialog->okBtn, SIGNAL(clicked()), SLOT(AnnotationSubmit()));
    std::unique_ptr<QAction> submitBindAction = std::make_unique<QAction>(nullptr);
    QAction* const bindPtr = submitBindAction.get();
    bindPtr->setShortcut(QKeySequence(Qt::Key_Shift + Qt::Key_Enter));
    QObject::connect(bindPtr, SIGNAL(triggered()), SLOT(AnnotationSubmit()));
    editorDialog->plainTextEdit->addAction(bindPtr);
//    newDialog->addAction(bindPtr);
    if (newDialog->exec() != QDialog::Accepted) {
        return;
    }
    submitBindAction.release();

    this->activeAnnotationData.editor.release();
    this->activeAnnotationData.editorParentDialog.release();

    // Add the annotation:
    if (isEdit) {
        this->activeProject.get().annotations.RemoveAnnotation(this->filePath, lineReference);
    }
    this->activeProject.get().annotations.AddNewAnnotation(this->activeAnnotationData.activeAnnotation);
    this->LoadFile(this->filePath);
}

std::string CodeEditor::HTMLFormatAnnotation(const Annotation& sample, const std::string& linePrefix) {
    std::string formatted = QString::fromStdString(sample.contents).toHtmlEscaped().replace("\n",
        QString::fromStdString("\n<span style=\"" + Config::Style::HTML::AnnotationMarker + "\">" + linePrefix + "</span> ")).toStdString();

    // Currently, this just involves tagging keywords in the sample's contents.
    std::size_t hashPos = 0;
    bool firstIter = true;
    while ((hashPos = formatted.find('#', firstIter ? 0 : hashPos + 1)) != std::string::npos) {
        if (firstIter) {
            firstIter = false;
        }
        const std::string tokenStyle = "<span style=\"" + Config::Style::HTML::AnnotationToken + "\">";
        formatted.insert(hashPos, tokenStyle);
        hashPos += tokenStyle.length();
        std::size_t keywordEndPos = hashPos;
        for (; keywordEndPos < formatted.length(); keywordEndPos++) {
            if (std::isspace(formatted[keywordEndPos])) {
                break;
            }
        }
        if (keywordEndPos == formatted.length() - 1) {
            formatted.append("</span>");
        }
        else {
            formatted.insert(keywordEndPos, "</span>");
        }
    }

    return formatted;
}

std::string CodeEditor::AnnotateCode(const std::string& codeStr,
    const std::vector<Annotation>& applicableAnnotations) {

    const QStringList codeLines = QString::fromStdString(codeStr).split('\n');

    const std::size_t combinedSize = codeLines.size() + applicableAnnotations.size();

    QStringList annotatedCode(combinedSize);
    std::vector<std::size_t> annotatedLines;
    const QString annotationPrefix = " ";

    const std::size_t codeLinesCount = codeLines.size();
    const std::uint16_t maxLineLength = codeLinesCount > 9 ? std::log10(static_cast<double>(codeLinesCount)) + 1: 1; // https://stackoverflow.com/a/1489928
    std::vector<QString> cachedSpaces(std::max(static_cast<std::size_t>(maxLineLength), static_cast<std::size_t>(annotationPrefix.length())));
    for (std::size_t spaces = 0; spaces < cachedSpaces.size(); spaces++) {
        for (std::size_t i = 0; i < spaces; i++) {
            cachedSpaces[spaces] += ' ';
        }
    }

    const QString fullAnnotationPrefix = annotationPrefix + cachedSpaces[cachedSpaces.size() - (annotationPrefix.length())] + " |";
    for (const Annotation& iterativeAnnotation : applicableAnnotations) {
        const std::size_t writePos =
                this->activeProject.get().annotations.ResolveToEditLineRef(
                    this->filePath, iterativeAnnotation.lineRef
                );

        annotatedCode[writePos] =
            "<span style=\"" + QString::fromStdString(Config::Style::HTML::AnnotationMarker) +
            "\">" + fullAnnotationPrefix + "</span> <span style=\"" + QString::fromStdString(Config::Style::HTML::AnnotationContents) + "\">" +
            QString::fromStdString(this->HTMLFormatAnnotation(iterativeAnnotation, fullAnnotationPrefix.toStdString())) + "</span>";

        annotatedLines.push_back(writePos);
    }

    for (std::size_t writeLine = 0, codeLineIndex = 0;
         writeLine < combinedSize && codeLineIndex < codeLinesCount;
         writeLine++) {

        // Remember: The annotatedLines vector is *sorted* due to the applicableAnnotations vector being sorted.
        //           (in ascending order)
        if (annotatedLines.size() > 0 && annotatedLines[0] == writeLine) {
            annotatedLines.erase(annotatedLines.begin());
            continue;
        }

        const QString lineNumber = QString::number(codeLineIndex);
        const QString& spacesBuf = cachedSpaces[cachedSpaces.size() - lineNumber.length()];

        annotatedCode[writeLine] =
                "<span style=\"" + QString::fromStdString(Config::Style::HTML::CodeMarker) + "\">" +
                lineNumber + spacesBuf + " | </span>" +
                codeLines[codeLineIndex].toHtmlEscaped();

        ++codeLineIndex;
    }
    return annotatedCode.join('\n').toStdString();
}

void CodeEditor::LoadFile(const std::string& path) {
    QFile fileObj(QString::fromStdString(path));
    fileObj.open(QIODevice::ReadOnly | QIODevice::Text);
    QByteArray rawFileContents = fileObj.readAll();
    fileObj.close();

    const std::string fileContentsStr = rawFileContents.toStdString();
    const std::vector<Annotation> annotationsVec = this->activeProject.get().annotations.GetAnnotations(path);
    const std::string annotatedContents = this->AnnotateCode(fileContentsStr, annotationsVec);

    const QString editorHTML = QString::fromStdString(
                "<style>* {white-space: pre; " +
        Config::Style::HTML::UniversalText +
        "}</style><p>" +
        annotatedContents + "</p>");

    this->setHtml(editorHTML);
}
