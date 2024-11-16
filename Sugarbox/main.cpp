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

class MyMSGEventFilter : public QAbstractNativeEventFilter
{
public:
   MyMSGEventFilter(SugarboxApp * app) : app_(app)
   {
   }

   bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override
   {
      static bool deadkey = 0;
      static unsigned int scancode = 0;
      if (eventType == "windows_generic_MSG") {
         MSG* msg = static_cast<MSG*>(message);
         MSG peekedMsg;
         if (msg->message == WM_KEYDOWN || msg->message == WM_SYSKEYDOWN)
         {
            if (app_->GetEmulation()->GetKeyboardHandler()->IsDeadKey(((msg->lParam >> 16) & 0xFF)))
            {
               // deadkey ?!
               deadkey = true;
               scancode = (msg->lParam >> 16) & 0xFF;
               app_->GetEmulation()->GetKeyboardHandler()->SendScanCode(scancode, true);
               return true;
            }
         }

         if (msg->message == WM_KEYUP && deadkey && ((msg->lParam >> 16) & 0xFF) == scancode)
         {
            deadkey = false;
            app_->GetEmulation()->GetKeyboardHandler()->SendScanCode((msg->lParam >> 16) & 0xFF, false);
            
            return true;
         }
         else if (msg->message == WM_DEADCHAR)
         {
            PeekMessage(&peekedMsg, msg->hwnd, WM_DEADCHAR, WM_DEADCHAR, PM_REMOVE);

            int dbg = 1;
            deadkey = true;
            scancode = (msg->lParam >> 16) & 0xFF;
            app_->GetEmulation()->GetKeyboardHandler()->SendScanCode(scancode, true);
            return true;
         }
         /*MSG* msg = static_cast<MSG*>(message);
         MSG peekedMsg;
         switch (msg->message)
         {
         case WM_KEYDOWN:
         case WM_SYSKEYDOWN:
            PeekMessage(&peekedMsg, msg->hwnd, WM_DEADCHAR, WM_DEADCHAR, PM_REMOVE);
            return 0;
         case WM_CHAR:
         case WM_DEADCHAR:
            if (msg->lParam & 0x40000000) //message is repeated via the key holding down
               return 1;
         }*/
         // ...
      }
      else if (eventType == "windows_dispatcher_MSG") {
         MSG* msg = static_cast<MSG*>(message);
         // ...
      }
      return false;
   }
protected:
   SugarboxApp* app_;
};

int main(int argc, char *argv[])
{
   Q_INIT_RESOURCE(resources);


   QApplication app(argc, argv);
   SugarboxInitialisation init;
   SugarboxApp mainWin;

   app.setStyle(new MenuStyle());

   MyMSGEventFilter my_filter(&mainWin);
   app.installNativeEventFilter(&my_filter);

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

   mainWin.show();

   init._debug_start = parser.isSet(debugOnStart);
   init._hardware_configuration = parser.isSet(configTypeOption) ? parser.values(configTypeOption)[0].toUtf8().data() : "";
   init._script_to_run = parser.isSet(cslFileOption) ? parser.values(cslFileOption)[0].toUtf8().data() : "";
   init._cart_inserted = parser.isSet(cartOption) ? parser.values(cartOption)[0].toUtf8().data() : "";

   mainWin.RunApp(init);

   return app.exec();
}
