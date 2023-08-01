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
   QCoreApplication::setOrganizationName("Sugarbox");
   QCoreApplication::setApplicationName("Sugarbox");
   QCoreApplication::setApplicationVersion("2.0.1");
   QCommandLineParser parser;
   parser.setApplicationDescription(QCoreApplication::applicationName());
   parser.addHelpOption();
   parser.addVersionOption();

   QCommandLineOption cslFileOption(QStringList() << "s" << "csl",
      QCoreApplication::translate("main", "Launch emulation and run <csl> script"),
      QCoreApplication::translate("main", "csl"));

   parser.addOption(cslFileOption);

   parser.process(app);
   qDebug() << parser.values(cslFileOption);

   SugarboxApp mainWin;

   mainWin.show();
   mainWin.RunApp();

   // Handle options
   if (parser.isSet(cslFileOption))
   {
      std::filesystem::path csl_path(parser.values(cslFileOption)[0].toUtf8().data());
      mainWin.GetEmulation()->AddScript(csl_path);

   }

   return app.exec();
}
