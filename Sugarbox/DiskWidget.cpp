#include "DiskWidget.h"

#include <QMenu>
#include <QPainter>
#include <QResizeEvent>

DiskWidget::DiskWidget(QWidget* parent) :
   QWidget(parent),
   emulation_(nullptr),
   disk_a_protection_(this),
   disk_b_protection_(this),
   disk_a_display_(0, this),
   disk_b_display_(1, this),
   menu_(nullptr)
{
   
   protected_icon_on_.addPixmap(QPixmap(":/Resources/protec_off.png"), QIcon::Normal);
   protected_icon_off_.addPixmap(QPixmap(":/Resources/protec_on.png"), QIcon::Selected);

   disk_a_protection_.setIcon(protected_icon_on_);
   disk_b_protection_.setIcon(protected_icon_on_);

   status_layout_ = new QHBoxLayout(this);
   status_layout_->setSpacing(0);
   status_layout_->setContentsMargins(0, 0, 0, 0);

   status_layout_->addWidget(&disk_a_protection_);
   status_layout_->addWidget(&disk_a_display_);
   status_layout_->addWidget(&disk_b_protection_);
   status_layout_->addWidget(&disk_b_display_);
}

void DiskWidget::SetEmulation(Emulation* emulation, Settings* settings, Function* menu)
{
   menu_ = menu;
   emulation_ = emulation;

   connect(&disk_a_protection_, &QToolButton::released, this, [=]() {if (emulation_) { emulation_->GetEngine()->GetFDC()->SetWriteProtection(!emulation_->GetEngine()->GetFDC()->IsDiskWriteProtected(0), 0); }});
   connect(&disk_b_protection_, &QToolButton::released, this, [=]() {if (emulation_) { emulation_->GetEngine()->GetFDC()->SetWriteProtection(!emulation_->GetEngine()->GetFDC()->IsDiskWriteProtected(1), 1); }});

   disk_a_display_.SetEmulation(emulation_, settings);
   disk_b_display_.SetEmulation(emulation_, settings);

   Update();
}

void DiskWidget::Update()
{
   disk_a_display_.Update();
   disk_b_display_.Update();

   // Set status of protection button
   if (emulation_)
   {
      disk_a_protection_.setIcon(emulation_->GetEngine()->GetFDC()->IsDiskWriteProtected(0)?protected_icon_on_: protected_icon_off_);
      disk_b_protection_.setIcon(emulation_->GetEngine()->GetFDC()->IsDiskWriteProtected(1) ? protected_icon_on_ : protected_icon_off_);
   }
}

QSize	DiskWidget::sizeHint() const
{
   return QWidget::sizeHint();
}


void DiskWidget::paintEvent(QPaintEvent* event)
{
   QPainter painter(this);
}


void DiskWidget::resizeEvent(QResizeEvent* e)
{
   QWidget::resizeEvent(e);
}

void DiskWidget::CreateSubMenu(QMenu* menu, Function* function, bool toplevel)
{
   if (function->IsNode())
   {
      QMenu* submenu;
      if (function->IsAvailable())
      {
         submenu = (toplevel) ? menu : menu->addMenu(tr(function->GetLabel()));
         for (unsigned int submenu_index = 0; submenu_index < function->NbSubMenu(); submenu_index++)
         {
            Function* subfunction = function->GetMenu(submenu_index);
            CreateSubMenu(submenu, function->GetMenu(submenu_index));
         }
      }
   }
   else
   {
      // Item
      menu->addAction(function->GetAction());
   }
}

void DiskWidget::mousePressEvent(QMouseEvent* event)
{
   if (menu_)
   {
      // Open menu
      QPoint globalPos = mapToGlobal(event->pos());

      // Create menu and insert some actions
      QMenu myMenu;

      CreateSubMenu(&myMenu, menu_, true);

      // Show context menu at handling position
      myMenu.exec(globalPos);
   }
   QWidget::mousePressEvent(event);

}