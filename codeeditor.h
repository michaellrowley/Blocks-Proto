#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QTextBrowser>
#include <QMainWindow>
#include <QAction>
#include <QDialog>
#include <QObject>
#include <QWidget>
#include <memory>
#include "annotation.h"
#include "ui_annotationeditor.h"
#include "project.h"

class CodeEditor : public QTextBrowser
{
    Q_OBJECT
public:
    CodeEditor(Project& project, const std::string& path, QWidget* const parent = nullptr);
    void LoadFile(const std::string& relativePath);
    void Reload();

private:
    std::string filePath;
    std::reference_wrapper<Project> activeProject;

    std::unordered_map<std::string, std::vector<std::unique_ptr<QAction>>> keyBindings;

    struct {
        Annotation activeAnnotation;
        std::unique_ptr<Ui_Dialog> editor; // For getting the form's state to write to activeAnnotation.
        std::unique_ptr<QDialog> editorParentDialog; // For closing the window.
    } activeAnnotationData;

    std::string AnnotateCode(const std::string& codeStr, const std::vector<Annotation>& applicableAnnotations);
    std::string HTMLFormatAnnotation(const Annotation& sample, const std::string& linePrefix = "| ");

private slots:
    void ReloadFile();
    void ToggleBookmark();
    void DeleteAnnotation();
    void BeginAnnotation();
    void AnnotationSubmit();
 };

#endif // CODEEDITOR_H
