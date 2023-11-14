#pragma once

#include <functional>
#include <map>
#include <filesystem>

#include <QMainWindow>
#include <QtWidgets>

#include "Emulation.h"
#include "Display.h"
#include "ALSoundMixer.h"

#include "ISound.h"
#include "Machine.h"
#include "Motherboard.h"
#include "Snapshot.h"
#include "DebugDialog.h"
#include "ConfigurationManager.h"
#include "MultiLanguage.h"
#include "Functions.h"
#include "SettingsList.h"
#include "Settings.h"
#include "DlgSettings.h"
#include "DebugSocket.h"
#include "FlagHandler.h"
#include "TapeWidget.h"
#include "DiskWidget.h"
#include "SCLPlayer.h"
#include "Script.h"
#include "SoundWidget.h"

namespace Ui {
   class SugarboxApp;
}

class SugarboxApp : public QMainWindow, public ISoundFactory, public IFunctionInterface, public INotifier, public ISettingsChange, public IDebugerStopped, public ITapeInsertionCallback
{
   Q_OBJECT

public:
   explicit SugarboxApp(QWidget *parent = 0);
   virtual ~SugarboxApp();

   int RunApp(Sugarboxinitialisation& init);

   // Actions initialization
   void InitAllActions();
   IFunctionInterface::Action* AddAction(IFunctionInterface::FunctionType id, std::function<void()> fn, const char* label_id, std::function<bool()>enabled = nullptr, std::function<bool()>checked = nullptr);

   // IFunctionInterface interface
   virtual QAction* GetAction(FunctionType func_type);

   Action* GetFirstAction(FunctionType&);
   Action* GetNextAction(FunctionType&);

   virtual void NotifyStop();

   virtual bool PlusEnabled();
   virtual bool FdcPresent();
   virtual bool TapePresent();

   Emulation* GetEmulation() { return emulation_; }

   // Actions
   virtual void Pause();
   virtual bool PauseEnabled();
   virtual bool CheckSpeed(int speedlimit);
   virtual void SetSpeed(int speedlimit);
   virtual void ConfigurationSettings();
   virtual void SaveAs(int drive);
   virtual void Eject(int drive);
   virtual void Insert(int drive);
   virtual void InsertBlank(int drive, IDisk::DiskType type);
   virtual void LoadCsl();
   virtual void TapeInsert();
   virtual void TapeSaveAs();
   virtual void SnaLoad();
   virtual void SnaSave();
   virtual void SnrLoad();
   virtual void SnrRecord();
   virtual void CprLoad();
   virtual void ToggleAutoload();
   virtual bool IsSomethingInClipboard();
   virtual void AutoType();
   virtual void OpenDebugger();

   // INotifier 
   virtual void DiskLoaded();

   // ISoundFactory interface
   virtual ISound* GetSound(const char* name);
   virtual const char* GetSoundName(ISound*);

   virtual const char* GetFirstSoundName();
   virtual const char* GetNextSoundName();

   // Events
   void SizeChanged(int width, int height);

   // Keyboard
   virtual void keyPressEvent(QKeyEvent * event_keyboard);
   virtual void keyReleaseEvent(QKeyEvent *event_keyboard);
   
   // Display
   void FullScreenToggle();

public slots:
   void clear();
   void UpdateMenu();
   virtual void ChangeSettings(MachineSettings*);
   void Display();

signals:
   void changed(const QMimeData *mimeData = nullptr);
   void MenuChanged();
   void SettingsChanged();

protected:

   void InitSettings();

   // Drag'n'drop
   void dragEnterEvent(QDragEnterEvent *event) override;
   void dragMoveEvent(QDragMoveEvent *event) override;
   void dragLeaveEvent(QDragLeaveEvent *event) override;
   void dropEvent(QDropEvent *event) override;

   // Menu init
   void InitMenu();
   void InitStatusBar();
   void InitFileDialogs();

   // Display gui
   void CreateActions();
   void CreateStatusBar();

   void CreateSubMenu(QMenu*, Function*, bool toplevel=false);
   void DrawStatusBar();

   bool AskForSaving(int drive);
   bool AskForSavingTape();

   // Actions
   std::map<IFunctionInterface::FunctionType, Action*> action_list_;
   std::map<IFunctionInterface::FunctionType, Action*>::iterator it_;

   // QT
   void createMenus();

   Ui::SugarboxApp *ui;
   QWidget *widget_;
   QVBoxLayout  *mainLayout_;
   QMenuBar *menubar_;

   //////////////
   // File dialogs
   std::map<QString, const FormatType*> format_ext_map_;
   std::map<QString, const FormatType*> format_ext_map_read_;
   QString save_disk_extension_;
   QString load_disk_extension_;

   std::map<QString, Emulation::TapeFormat> format_tape_save_;
   QString save_tape_extension_; 
   QString load_tape_extension_;
   

   // Keyboard handler
   IKeyboard* keyboard_handler_;

   // Screen position
   const float main_display_width = 768;
   const float main_display_height = 544;
   const float toolbar_height = 50;
   const float status_height = 50;
   const float peripherals_width = 100;

   float window_width_;
   float window_height_;

   // Emulation and so on
   ConfigurationManager config_manager_;
   Emulation* emulation_;
   CDisplay display_;
   ALSoundMixer sound_mixer_;

   // counters
   unsigned int old_speed_;
   int counter_;

   // Functions
   MultiLanguage language_;
   FunctionList functions_list_;

   // Opened / close windows
   DiskBuilder disk_builder_;
   DlgSettings dlg_settings_;

   ConfigurationManager key_mgr, key_mgr_out;

   // Debugger

   DebugSocket* debugger_link_;

   DebugDialog debug_;

   // Flag handler
   FlagHandler flag_handler_;

   // Settings
   Settings settings_;

   // Status bar
   QLabel status_speed_;
   TapeWidget status_tape_;
   DiskWidget status_disk_;
   SoundWidget status_sound_;

   // SSM
   void EnableSSM();
   void DisableSSM();
   void CustomFunction(unsigned int i);
   bool ssm_first_;
   unsigned short ssm_last_address_;
   unsigned char ll_;

   // Script player
   ScriptContext script_context_;
   SCLPlayer scl_player_;

};

