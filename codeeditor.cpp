#include "codeeditor.h"
#include "configuration.h"
#include <QFile>
#include <QDebug>
#include <QKeyEvent>
#include <QPushButton>
#include <QTextDocument>
#include <QScrollBar>
#include <QTextBlock>
#include <algorithm>
#include <math.h>
#include "ui_annotationeditor.h"
#include "annotation.h"
#include "utils.h"

CodeEditor::CodeEditor(Project& project, const std::string& path, QWidget* const parent) :
    filePath(path), activeProject(project)
{
    this->setParent(parent);

    this->setReadOnly(true);

    // https://www.qtcentre.org/threads/39941-readonly-QTextEdit-with-visible-Cursor
    this->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);

    // Setup shortcuts:
    std::unique_ptr<QAction> generalActionUniquePtr;
    QAction* generalActionPtr = nullptr;

    NEW_KEYBIND("TGL_BOOKMARK", QKeySequence(Qt::Key_B), ToggleBookmark, this)
    NEW_KEYBIND("ADD_ANNOTATION", QKeySequence(Qt::Key_Semicolon), BeginAnnotation, this)
    NEW_KEYBIND("DEL_ANNOTATION", QKeySequence(Qt::Key_Backspace), DeleteAnnotation, this)
    NEW_KEYBIND("RLD_ANNOTATION", QKeySequence(Qt::Key_R), ReloadFile, this)

    this->ReloadFile();
}

void CodeEditor::ToggleBookmark() {
    std::size_t lineReference = static_cast<std::size_t>(this->textCursor().blockNumber());
    lineReference = this->activeProject.get().annotations.ResolveToCodeLineRef(this->filePath, lineReference);

    // Try to delete:
    try {
        this->activeProject.get().bookmarks.RemoveBookmark(this->filePath, lineReference);
        this->ReloadFile();
        return;
    } catch (...) {}

    // The bookmark wasn't present, add it:
    this->activeProject.get().bookmarks.AddBookmark(
        Bookmark(this->filePath, lineReference)
    );
    this->ReloadFile();
}

void CodeEditor::DeleteAnnotation() {
    std::size_t lineReference = static_cast<std::size_t>(this->textCursor().blockNumber());
    lineReference = this->activeProject.get().annotations.ResolveToCodeLineRef(this->filePath, lineReference);
    try {
        this->activeProject.get().annotations.RemoveAnnotation(this->filePath, lineReference);
    } catch (...) {
        // If an annotation didn't exist at that address, perhaps we
        // ought to try disabling the bookmark, it would feel more
        // intuitive for the user to be able to use <backspace> for
        // removing annotations *and* bookmarks.
        try {
            this->activeProject.get().bookmarks.RemoveBookmark(this->filePath, lineReference);
        } catch (...) {
            // Nope, nothing here.
            return;
        }
    }
    this->ReloadFile();
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

    // Close the QDialog:
    parentDialog->accept();
}

void CodeEditor::BeginAnnotation() {
    const QTextBlock block = this->textCursor().block();
    std::size_t lineReference = static_cast<std::size_t>(block.blockNumber());
    lineReference = this->activeProject.get().annotations.ResolveToCodeLineRef(this->filePath, lineReference);

    // Implied 'editing' if there already exists an annotation at the user's chosen line:
    std::string duplicateAnnotationContents("");
    std::size_t duplicateAnnotationHeight = 0;
    bool isEdit = false;
    try {
        const Annotation duplicateAnnotation =
            this->activeProject.get().annotations.GetAnnotation(this->filePath, lineReference);
        duplicateAnnotationContents = duplicateAnnotation.contents;
        duplicateAnnotationHeight = duplicateAnnotation.linesOccupied;
        isEdit = true;
    } catch (...) {
        // The above code will throw if there was not an annotation at lineReference.
        isEdit = false; // Default value anyway.
    }
    this->activeAnnotationData.activeAnnotation = {
        .contents = isEdit ? duplicateAnnotationContents : "",
        .linesOccupied = duplicateAnnotationHeight,
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
    const std::string ctaText = isEdit ? "Edit Annotation" : "New Annotation";
    editorDialog->label->setText(QString::fromStdString(ctaText));
    editorDialog->okBtn->setText(QString::fromStdString(ctaText.substr(0, ctaText.find(' '))));

    // Bind the submit button to write the annotation to memory:
    QObject::connect(editorDialog->okBtn, SIGNAL(clicked()), SLOT(AnnotationSubmit()));

    if (newDialog->exec() != QDialog::Accepted) {
        return;
    }
    this->activeAnnotationData.editor.release();
    this->activeAnnotationData.editorParentDialog.release();

    // Add the annotation:
    if (isEdit) {
        this->activeProject.get().annotations.RemoveAnnotation(this->filePath, lineReference);
    }
    if (this->activeAnnotationData.activeAnnotation.contents.length() != 0) {
        this->activeProject.get().annotations.AddNewAnnotation(this->activeAnnotationData.activeAnnotation);
    }
    this->LoadFile(this->filePath);
}

std::string CodeEditor::HTMLFormatAnnotation(const Annotation& sample, const std::string& linePrefix) {
    std::string formatted = QString::fromStdString(sample.contents).toHtmlEscaped().replace("\n",
        QString::fromStdString("\n<span style=\"" + Config::Style::HTML::AnnotationMarker + "\">" + linePrefix + "</span> ")).toStdString();

    // Currently, this just involves tagging keywords in the sample's contents and HTML encoding everything user-supplied.
    std::size_t hashPos = 0, keywordEndPos = 0;
    bool firstIter = true;
    while ((hashPos = formatted.find('#', firstIter ? 0 : hashPos + 1)) != std::string::npos) {
        if (firstIter) {
            firstIter = false;
        }
        const std::string tokenStyle = "<span style=\"" + Config::Style::HTML::AnnotationToken + "\">";
        formatted.insert(hashPos, tokenStyle);
        keywordEndPos = hashPos + tokenStyle.length();
        for (; keywordEndPos < formatted.length(); keywordEndPos++) {
            const char& keywordEndChar = formatted[keywordEndPos];

            std::unordered_map<char, char>::const_iterator cutoffChar = Annotation::OppositeCutoffs.cend();
            if (hashPos > 0) {
                cutoffChar = Annotation::OppositeCutoffs.find(formatted[hashPos - 1]);

                if (cutoffChar != Annotation::OppositeCutoffs.cend() &&
                    cutoffChar->second == keywordEndChar) {
                    break;
                }
            }
            if (cutoffChar == Annotation::OppositeCutoffs.cend() &&
                    std::find(Annotation::CutoffChars.cbegin(), Annotation::CutoffChars.cend(),
                              keywordEndChar) != Annotation::CutoffChars.cend()) {
                break;
            }
        }
        formatted.insert(keywordEndPos, "</span>");
        if (!Config::Style::DisplayKeywordHashtag) {
            formatted.erase(hashPos + tokenStyle.length(), 1);
        }
        hashPos = keywordEndPos;
    }

    return formatted;
}

std::string CodeEditor::AnnotateCode(const std::string& codeStr,
    const std::vector<Annotation>& applicableAnnotations) {

    const QStringList codeLines = QString::fromStdString(codeStr).split('\n');

    const std::size_t combinedSize = codeLines.size() + applicableAnnotations.size();

    QStringList annotatedCode(combinedSize);
    std::vector<std::size_t> annotatedLines;
    const QString annotationPrefix = ">";

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

    std::vector<Bookmark> existingBookmarks = this->activeProject.get().bookmarks.GetBookmarks(this->filePath);
    for (std::size_t writeLine = 0, codeLineIndex = 0;
         writeLine < combinedSize && codeLineIndex < codeLinesCount;
         writeLine++) {

        const bool isBookmark = (existingBookmarks.size() > 0 && existingBookmarks.front().lineRef == codeLineIndex);

        // Remember: The annotatedLines vector is *sorted* due to the applicableAnnotations vector being sorted.
        //           (in ascending order)
        if (annotatedLines.size() > 0 && annotatedLines[0] == writeLine) {
            annotatedLines.erase(annotatedLines.begin());
            continue;
        }

        if (isBookmark) {
            existingBookmarks.erase(existingBookmarks.begin());
        }

        const QString lineNumber = QString::number(codeLineIndex);
        const QString& spacesBuf = cachedSpaces[cachedSpaces.size() - lineNumber.length()];

        annotatedCode[writeLine] =
                "<span style=\"" + QString::fromStdString(Config::Style::HTML::CodeMarker) +
                (isBookmark ? QString::fromStdString(Config::Style::HTML::BookmarkMarker) : QString("")) +
                "\">" +
                lineNumber + spacesBuf + " |</span> " +
                codeLines[codeLineIndex].toHtmlEscaped();

        ++codeLineIndex;
    }
    return annotatedCode.join('\n').toStdString();
}

void CodeEditor::ReloadFile() {
    this->LoadFile(this->filePath);
}

void CodeEditor::Reload() {
    this->LoadFile(this->filePath);
}

void CodeEditor::LoadFile(const std::string& relativePath) {
    const std::string path = this->activeProject.get().GetCodebasePath() + relativePath;
    const int previousScrollValue = this->verticalScrollBar()->value();

    // Open the specified file and read its contents into a byte array:
    QFile fileObj(QString::fromStdString(path));
    fileObj.open(QIODevice::ReadOnly | QIODevice::Text);
    QByteArray rawFileContents = fileObj.readAll();
    fileObj.close();

    // Cast that byte array into a string and format it:
    const std::string fileContentsStr = rawFileContents.toStdString();
    const std::vector<Annotation> annotationsVec = this->activeProject.get().annotations.GetAnnotations(relativePath);
    const std::string annotatedContents = this->AnnotateCode(fileContentsStr, annotationsVec);

    const QString editorHTML = QString::fromStdString("<style>* {white-space: pre; " +
        Config::Style::HTML::UniversalText +
        "}</style><p>" +
        annotatedContents +
        "</p>");

    // Set QTextArea contents to the HTML-formatted string:
    this->setHtml(editorHTML);

    // Correct the selected line:
    this->verticalScrollBar()->setValue(previousScrollValue);
}
