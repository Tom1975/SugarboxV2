#pragma once

#include <QWidget>
#include <QScrollBar>

class DisassemblyWidget : public QWidget
{
   Q_OBJECT

public:
   explicit DisassemblyWidget(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* e) override;

private:
   QWidget disasm_window_;
   QScrollBar vertical_sb_;
   QScrollBar horizontal_sb_;
};
