#pragma once


#include <QDialog>
#include <QListWidgetItem>

#include "IUpdate.h"
#include "DebugInterface.h"
#include "Emulation.h"
#include "FlagHandler.h"
#include "SettingsValues.h"
#include "Settings.h"
#include "MultiLanguage.h"

namespace Ui {
class MemoryDialog;
}

class MemoryDialog : public QDialog, public DebugInterface, IUpdate
{
    Q_OBJECT

public:
    explicit MemoryDialog(QWidget *parent = 0);
    ~MemoryDialog();

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

    // Update the view
    virtual void Update();

protected:
   // Menu action
   void UpdateDebug();


private:
   Ui::MemoryDialog *ui;
   Emulation* emu_handler_;
   FlagHandler* flag_handler_;
   Settings* settings_;
   MultiLanguage* language_;
   QWidget* parent_;

   // automatic shortcuts
   std::map<unsigned int, std::function<void()> > shortcuts_;
};
