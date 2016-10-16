#include "settings.hpp"
#include "ui_settings.h"
#include <QFont>

Settings::Settings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Settings)
{
    ui->setupUi(this);

    // http://stackoverflow.com/questions/9701983/qt-how-to-create-a-setting-window-like-in-gtk
    // http://www.qtcentre.org/threads/25823-Font-size-increase-in-QtableWidget
    // http://stackoverflow.com/questions/19434391/in-qt-how-to-resize-icons-in-a-table
    QFont font;
    QSize size;
    font.setPointSize(14);
    font.setFamily("Arial");
    size.setHeight(48);
    size.setWidth(48);
    const int rowCount = ui->settings_list_widget->count();
    ui->settings_list_widget->setIconSize(size);
    for (int i = 0; i < rowCount; ++i) {
        QListWidgetItem *selectedItem = ui->settings_list_widget->item(i);
        selectedItem->setFont(font);
    }
}

Settings::~Settings()
{
    delete ui;
}

void Settings::on_buttonBox_accepted()
{
    // 'Save' has been selected
}

void Settings::on_buttonBox_rejected()
{
    // 'Cancel' has been selected
}
