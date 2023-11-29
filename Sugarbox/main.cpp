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


   // Configuration
   QCommandLineOption configTypeOption(QStringList() << "cfg" << "config",
      QCoreApplication::translate("main", "Select <configuration> to load"),
      QCoreApplication::translate("main", "configuration name"));
   parser.addOption(configTypeOption);

    
   // CSL file
   QCommandLineOption cslFileOption(QStringList() << "s" << "csl",
      QCoreApplication::translate("main", "Launch emulation and run <csl> script"),
      QCoreApplication::translate("main", "csl"));
   parser.addOption(cslFileOption);

   // Cartridge 
   QCommandLineOption cartOption(QStringList() << "cart" << "cartridge",
      QCoreApplication::translate("main", "<path> of the cartridge to insert"),
      QCoreApplication::translate("main", "path"));
   parser.addOption(cartOption);

   // Debugger ON
   QCommandLineOption debugOnStart(QStringList() << "d" << "debug",
      QCoreApplication::translate("main", "Start with debugger on and break."));
   parser.addOption(debugOnStart);

   parser.process(app);
   qDebug() << parser.values(cslFileOption);

   SugarboxInitialisation init;

   SugarboxApp mainWin;

   mainWin.show();

   init._debug_start = parser.isSet(debugOnStart);
   init._hardware_configuration = parser.isSet(configTypeOption) ? parser.values(configTypeOption)[0].toUtf8().data() : "";
   init._script_to_run = parser.isSet(cslFileOption) ? parser.values(cslFileOption)[0].toUtf8().data() : "";
   init._cart_inserted = parser.isSet(cartOption) ? parser.values(cartOption)[0].toUtf8().data() : "";

   mainWin.RunApp(init);

   return app.exec();
}
