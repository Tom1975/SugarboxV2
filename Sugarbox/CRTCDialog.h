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
class CRTCDialog;
}

class CRTCDialog : public QDialog, public DebugInterface, IUpdate
{
    Q_OBJECT

public:
    explicit CRTCDialog(QWidget *parent = 0);
    ~CRTCDialog();

    // Init dialog
    void SetEmulator(Emulation* emu_handler, MultiLanguage* language);
    void SetSettings(Settings* settings);
    virtual void SetAddress(unsigned int addr);

    virtual bool event(QEvent *event);

    bool eventFilter(QObject* watched, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;

    // Update the view
    virtual void Update();

public slots:



protected:
   // Menu action
   void UpdateDebug();

private:
   Ui::CRTCDialog *ui;
   Emulation* emu_handler_;
   Settings* settings_;
   MultiLanguage* language_;
   QWidget* parent_;

   // automatic shortcuts
   std::map<unsigned int, std::function<void()> > shortcuts_;


   QGroupBox* registerGroup_;
};
