#pragma once

#include <functional>
#include <map>

#include <QMainWindow>
#include <QtWidgets>

#include "Emulation.h"
#include "Display.h"
#include "ALSoundMixer.h"

#include "ISound.h"
#include "Machine.h"
#include "Motherboard.h"
#include "Snapshot.h"
#include "ConfigurationManager.h"
#include "MultiLanguage.h"
#include "Functions.h"
#include "SettingsList.h"
#include "DlgSettings.h"
#include "SoundControl.h"

namespace Ui {
   class SugarboxApp;
}


class SugarboxApp : public QMainWindow, public ISoundFactory, public IFunctionInterface, public INotifier, public ISettingsChange
{
   Q_OBJECT

public:
   explicit SugarboxApp(QWidget *parent = 0);
   virtual ~SugarboxApp();

   int RunApp();

   // Actions initialization
   void InitAllActions();
   IFunctionInterface::Action* AddAction(IFunctionInterface::FunctionType id, std::function<void()> fn, const char* label_id, std::function<bool()>enabled = nullptr, std::function<bool()>checked = nullptr);

   // IFunctionInterface interface
   virtual QAction* GetAction(FunctionType func_type);

   Action* GetFirstAction(FunctionType&);
   Action* GetNextAction(FunctionType&);

   virtual bool PlusEnabled();
   virtual bool FdcPresent();
   virtual bool TapePresent();

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
   void keyReleaseEvent(QKeyEvent *event_keyboard);

   

   // Display
   void FullScreenToggle();
public slots:
   void clear();
   void UpdateMenu();
   virtual void ChangeSettings(MachineSettings*);

signals:
   void changed(const QMimeData *mimeData = nullptr);
   void MenuChanged();
   void SettingsChanged();

protected:

   // Drag'n'drop
   void dragEnterEvent(QDragEnterEvent *event) override;
   void dragMoveEvent(QDragMoveEvent *event) override;
   void dragLeaveEvent(QDragLeaveEvent *event) override;
   void dropEvent(QDropEvent *event) override;

   // Menu init
   void InitMenu();
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
   // Gui related
   enum {
      POPUP_NONE,
      POPUP_ASK_SAVE
   } PopupType;
   unsigned int PopupArg;
   std::function<void()> popup_associated_function_;


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
   char str_speed_[16];
   int counter_;

   // Functions
   MultiLanguage language_;
   FunctionList functions_list_;

   // Opened / close windows
   DiskBuilder disk_builder_;
   DlgSettings dlg_settings_;
   SoundControl sound_control_;

   ConfigurationManager key_mgr, key_mgr_out;

};

