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

   void UpdateRegister(QString str, unsigned int i);

protected:
   // Menu action
   void UpdateDebug();

private:
   //Ui::CRTCDialog *ui;
   Emulation* emu_handler_;
   Settings* settings_;
   unsigned char* crtc_list_;

   MultiLanguage* language_;
   QWidget* parent_;

   // automatic shortcuts
   std::map<unsigned int, std::function<void()> > shortcuts_;

   // Groups
   QGroupBox* informations_group_;
   QGroupBox* register_group_;
   QGroupBox* counters_group_;

   QGridLayout* layout_reg_;
   QGridLayout* layout_counters_;

   QGridLayout* layout_;

   QLabel* register_label_[18];
   QLineEdit* register_edit_[18];

   // informations
   QComboBox* type_crtc_;
   // Registers
   QLineEdit* registers_[18];
   // Counters
   QLineEdit* vcc_;
   QLineEdit* hcc_;
};
