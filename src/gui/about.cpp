#include "about.hpp"
#include "ui_about.h"
#include <libtorrent/version.hpp>

About::About(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::About)
{
    ui->setupUi(this);
    ui->libtorrent_version_label->setText(QString("Built with libtorrent (") + libtorrent::version() + QString(")"));
}

About::~About()
{
    delete ui;
}

void About::on_close_buttonBox_rejected()
{
    this->close();
}
