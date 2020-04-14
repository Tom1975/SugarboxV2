#ifdef WIN32
   #pragma comment(linker, " /ENTRY:mainCRTStartup")
#endif

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include "SugarboxApp.h"

int main(int argc, char *argv[])
{
   Q_INIT_RESOURCE(resources);


   QApplication app(argc, argv);
   QCoreApplication::setOrganizationName("Amiga");
   QCoreApplication::setApplicationName("Amiga");
   QCoreApplication::setApplicationVersion(QT_VERSION_STR);
   QCommandLineParser parser;
   parser.setApplicationDescription(QCoreApplication::applicationName());
   parser.addHelpOption();
   parser.addVersionOption();
   parser.addPositionalArgument("file", "The file to open.");
   parser.process(app);

   SugarboxApp mainWin;


   mainWin.show();
   mainWin.RunApp();
   

   return app.exec();
}

