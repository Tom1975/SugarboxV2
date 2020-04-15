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


class SugarboxApp : public QMainWindow, public ISoundFactory, public IFunctionInterface
{
   Q_OBJECT

public:
   explicit SugarboxApp(QWidget *parent = 0);
   virtual ~SugarboxApp();

   int RunApp();
   void InitAllActions();
   IFunctionInterface::Action* AddAction(IFunctionInterface::FunctionType id, std::function<void()> fn, const char* label_id);

   // IFunctionInterface interface
   virtual QAction* GiveAction(FunctionType func_type);

   Action* GetFirstAction(FunctionType&);
   Action* GetNextAction(FunctionType&);

   virtual bool PlusEnabled();
   virtual bool FdcPresent();
   virtual bool TapePresent();

   virtual void Exit();
   virtual void HardReset();
   virtual void Pause();
   virtual bool PauseEnabled();
   virtual void SetSpeed(int speedlimit);
   virtual void ConfigurationSettings();
   virtual void SaveAs(int drive);
   virtual void Eject(int drive);
   virtual bool DiskPresent(int drive);
   virtual void Flip(int drive);
   virtual void Insert(int drive);
   virtual void InsertBlank(int drive, IDisk::DiskType type);
   virtual void TapeRecord();
   virtual void TapePlay();
   virtual void TapeFastForward();
   virtual void TapeRewind();
   virtual void TapePause();
   virtual void TapeStop();
   virtual void TapeInsert();
   virtual void TapeSaveAs(Emulation::TapeFormat format);
   virtual bool IsQuickSnapAvailable();
   virtual void SnaLoad();
   virtual void SnaSave();
   virtual void SnaQuickLoad();
   virtual void SnaQuickSave();
   virtual void SnrLoad();
   virtual void SnrRecord();
   virtual void SnrStopRecord();
   virtual void SnrStopPlayback();
   virtual bool SnrIsRecording();
   virtual bool SnrIsReplaying();
   virtual void CprLoad();
   virtual bool IsAutoloadEnabled();
   virtual void ToggleAutoload();
   virtual bool IsSomethingInClipboard();
   virtual void AutoType();

   // ISoundFactory interface
   virtual ISound* GetSound(const char* name);
   virtual const char* GetSoundName(ISound*);

   virtual const char* GetFirstSoundName();
   virtual const char* GetNextSoundName();

   // Events
   void SizeChanged(int width, int height);
   void KeyboardHandler( int key, int scancode, int action, int mods);

   // Keyboard
   virtual void keyPressEvent(QKeyEvent * event_keyboard);
   void keyReleaseEvent(QKeyEvent *event_keyboard);

   // Display
   void FullScreenToggle();
public slots:
   void clear();

signals:
   void changed(const QMimeData *mimeData = nullptr);

protected:

   // Drag'n'drop
   void Drop(int count, const char** paths);
   void Drop(const char* paths);
   void dragEnterEvent(QDragEnterEvent *event) override;
   void dragMoveEvent(QDragMoveEvent *event) override;
   void dragLeaveEvent(QDragLeaveEvent *event) override;
   void dropEvent(QDropEvent *event) override;

   // Menu init
   void InitMenu();
   void InitFileDialogs();

   // Display gui
   void RunMainLoop();
   void DrawMainWindow();
   void DrawMenu();
   void DrawSubMenu(QMenu*, Function*, bool toplevel=false);
   void DrawPeripherals();
   void DrawStatusBar();
   void DrawOthers();

   bool AskForSaving(int drive);
   void InsertSelectFile(int drive);
   void InsertBlankDisk(int drive, IDisk::DiskType type);
   bool AskForSavingTape();
   void InsertSelectTape();

   // Actions
   std::map<IFunctionInterface::FunctionType, Action*> action_list_;
   std::map<IFunctionInterface::FunctionType, Action*>::iterator it_;

   // QT
   void createMenus();

   Ui::SugarboxApp *ui;
   QWidget *widget_;
   QVBoxLayout  *mainLayout_;
   QMenuBar *menubar_;
   std::vector<QMenu *> menu_list_;
   // Gui related
   enum {
      POPUP_NONE,
      POPUP_ASK_SAVE
   } PopupType;
   unsigned int PopupArg;
   std::function<void()> popup_associated_function_;


   //////////////
   // File dialogs
   enum {
      FD_SAVE_AS,
      FD_INSERT,
      FD_INSERT_TAPE,
      FD_SAVE_TAPE_AS,
      FD_INSERT_SNA,
      FD_SAVE_SNA,
      FD_LOAD_SNR,
      FD_RECORD_SNR,
      FD_LOAD_CPR
   } file_dialog_type_;

   std::map<std::string, const FormatType*> format_ext_map_;
   std::map<std::string, const FormatType*> format_ext_map_read_;
   char* write_disk_extension_;
   char* load_disk_extension_;
   char* load_tape_extension_;
   Emulation::TapeFormat format_;

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
   bool configuration_settings_;
};

