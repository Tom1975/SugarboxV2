#pragma once

#include <QWidget>
#include <QScrollBar>

class DisassemblyWidget : public QWidget
{
   Q_OBJECT

public:
   explicit DisassemblyWidget(QWidget* parent = nullptr);

   void SetDisassemblyInfo(unsigned int max_adress);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* e) override;

    void ComputeScrollArea();

public slots:
    void OnValueChange(int valueScrollBar);


private:
   QWidget disasm_window_;
   QScrollBar vertical_sb_;
   QScrollBar horizontal_sb_;

   unsigned int max_adress_;
   unsigned int current_address_;
   
};
