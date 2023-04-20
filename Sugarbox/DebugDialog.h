#pragma once


#include <QDialog>
#include <QListWidgetItem>

#include "IUpdate.h"
#include "DebugInterface.h"
#include "DisassemblyWidget.h"
#include "Emulation.h"
#include "Z80Desassember.h"

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
    void SetEmulator(Emulation* emu_handler);
    virtual void SetAddress(unsigned int addr);
    virtual bool event(QEvent *event);
    void Break();

    bool eventFilter(QObject* watched, QEvent* event);

   public slots:
      void on_set_top_address_clicked();
      void on_dbg_pause_clicked();
      void on_dbg_step__clicked();
      void on_dbg_run_clicked();
      void DasmShowContextMenu(const QPoint &pos);
      void StackToDasm();
      void AddBreakpoint();
      void RemoveBreakpoint();
      void on_add_bp_clicked();
      void on_remove_bp_clicked();
      void on_clear_bp_clicked();
      void on_bpAddress_returnPressed();
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
   DisassemblyWidget* disassembly_widget_;
   Ui::DebugDialog *ui;
   Emulation* emu_handler_;

};
