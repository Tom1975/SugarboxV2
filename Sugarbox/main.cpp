#ifdef WIN32
   #pragma comment(linker, " /ENTRY:mainCRTStartup")
#endif

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include "SugarboxApp.h"

class MenuStyle : public QProxyStyle
{
public:
   int styleHint(StyleHint stylehint, const QStyleOption *opt, const QWidget *widget, QStyleHintReturn *returnData) const
   {
      if (stylehint == QStyle::SH_MenuBar_AltKeyNavigation)
         return 0;

      return QProxyStyle::styleHint(stylehint, opt, widget, returnData);
   }
};

int main(int argc, char *argv[])
{
   Q_INIT_RESOURCE(resources);


   QApplication app(argc, argv);

   app.setStyle(new MenuStyle());
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
