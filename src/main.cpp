// src/main.cpp
#include <QApplication>
#include <QFile> // Needed for loading the stylesheet
#include <QTextStream> // Needed for reading the file
#include <QStyleFactory> // Optional: For setting a base style
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));

    // --- Load and Apply Stylesheet ---
    QFile styleFile(":/styles/stylesheet.qss"); // <<< ONLY THIS LINE for loading
    // QFile styleFile("stylesheet.qss"); // <<< MAKE SURE THIS IS COMMENTED OUT OR DELETED

    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&styleFile);
        QString styleSheet = stream.readAll();
        app.setStyleSheet(styleSheet);
        styleFile.close();
    } else {
        qWarning("Could not open stylesheet file!"); // <<< This is the warning you see
    }
    // --- End Stylesheet ---


    QCoreApplication::setOrganizationName("YourCompanyName");
    QCoreApplication::setApplicationName("NoteApp");
    QCoreApplication::setApplicationVersion("0.1");

    MainWindow mainWindow;
    mainWindow.show();
    return app.exec();
}