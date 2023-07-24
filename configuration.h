#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QKeyEvent>

namespace Config {
    namespace Keybinds {
        const static int BeginAnnotation = Qt::Key_Semicolon;
    };
    namespace Style {
        namespace UI {};
        namespace HTML {
            const static std::string UniversalText = "font-size: 20px;font-family: 'Courier New';";
            const static std::string AnnotationMarker = "background-color: rgba(150, 150, 230, 1); color:black;";
            const static std::string AnnotationContents = "color: rgba(255, 255, 255, 0.7);";
            const static std::string AnnotationToken = "color: rgba(30, 200, 30, 1);text-decoration: underline;";//font-weight: bold;";

            const static std::string CodeMarker = "color: rgba(255, 255, 255, 0.5);";
        };
    };
};

#endif // CONFIGURATION_H
