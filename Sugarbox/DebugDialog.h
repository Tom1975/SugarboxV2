#pragma once


#include <QDialog>
#include <QListWidgetItem>

#include "IUpdate.h"
#include "DebugInterface.h"
#include "DisassemblyWidget.h"
#include "Emulation.h"
#include "Z80Desassember.h"
#include "FlagHandler.h"
#include "SettingsValues.h"
#include "Settings.h"
#include "MultiLanguage.h"

namespace Ui {
class DebugDialog;
}

class DebugDialog : public QDialog, public DebugInterface, IUpdate
{
    Q_OBJECT

public:
    explicit DebugDialog(QWidget *parent = 0);
    ~DebugDialog();

    // Init dialog
    void SetEmulator(Emulation* emu_handler, MultiLanguage* language);
    void SetFlagHandler(FlagHandler* flag_handler);
    void SetSettings(Settings* settings);

    virtual void SetAddress(unsigned int addr);
    virtual bool event(QEvent *event);

    bool eventFilter(QObject* watched, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;

   public slots:
      void on_set_top_address_clicked();
      void on_dasm_address_returnPressed();
      void on_dbg_pause_clicked();
      void on_dbg_step_in_clicked();
      void on_dbg_step_clicked();
      void on_dbg_run_clicked();
      void DasmShowContextMenu(const QPoint &pos);
      void StackToDasm();
      void AddBreakpoint();
      void RemoveBreakpoint();
      void itemDoubleClicked(QListWidgetItem*);

    // Update the view
    virtual void Update();
    virtual void UpdateDisassembly(unsigned int offset);

protected:
   // Menu action
   
   // Disassembly
   unsigned int Disassemble(Z80Desassember* z80, unsigned int offset, std::string& str_asm);
   void UpdateDebug();


private:
   Ui::DebugDialog *ui;
   Emulation* emu_handler_;
   FlagHandler* flag_handler_;
   Settings* settings_;
   MultiLanguage* language_;
   QWidget* parent_;

   ///////////////////////////////
   // Register data
   class IRegisterView
   {
   public:
      virtual std::string& GetLabel() = 0;
      virtual std::string& GetValue() = 0;
   };

   template<typename T>
   class RegisterView :public IRegisterView
   {
   public:
      RegisterView(std::string label, T*);

      virtual std::string& GetLabel();
      virtual std::string& GetValue();

   protected:
      std::string label_;
      std::string value_;
      T* register_;
   };

   class FlagView: public RegisterView<Z80::Register>
   {
   public:
      FlagView(std::string label, Z80::Register* reg);
      std::string& GetValue() override;
   private:
   };

   std::vector< IRegisterView* > register_list_;

   // automatic shortcuts
   std::map<unsigned int, std::function<void()> > shortcuts_;
};
