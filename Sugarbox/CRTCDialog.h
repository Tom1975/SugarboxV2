#pragma once


#include <QDialog>
#include <QListWidgetItem>

#include "ValueEditCtrl.h"

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

protected:
   // Menu action
   void UpdateDebug();

private:
   void Cleanup();

   Emulation* emu_handler_;
   Settings* settings_;

   MultiLanguage* language_;
   QWidget* parent_;

   // automatic shortcuts
   std::map<unsigned int, std::function<void()> > shortcuts_;

   // Groups
   QGroupBox* informations_group_;
   QGroupBox* register_group_;
   QGroupBox* counters_group_;

   // Layouts
   QGridLayout* layout_reg_;
   QGridLayout* layout_counters_;

   QGridLayout* layout_;

   // informations
   QComboBox* type_crtc_;
   // Counters
   QLineEdit* vcc_;
   QLineEdit* hcc_;

   std::vector<ValueEditCtrl*> edit_list_;

   template<typename T>
   void CreateEditValue(QWidget* parent, QGridLayout* layout, QString l, int row, int column, ValueEditCtrl::RegisterType reg_type, T* ptr_current);

};
