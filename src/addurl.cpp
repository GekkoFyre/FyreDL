#include "addurl.hpp"
#include "ui_addurl.h"
#include "dl_view.hpp"
#include <QMessageBox>
#include <QFileDialog>

AddURL::AddURL(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddURL)
{
    ui->setupUi(this);
    routines = new GekkoFyre::CmnRoutines();
}

AddURL::~AddURL()
{
    delete ui;
    delete routines;
}

void AddURL::on_buttonBox_accepted()
{
    GekkoFyre::CmnRoutines::CurlInfo info;
    GekkoFyre::CmnRoutines::CurlInfoExt info_ext;
    QString url_plaintext;

    switch (ui->inputTabWidget->currentIndex()) {
    case 0:
        if (ui->url_dest_lineEdit->text().isEmpty()) {
            QMessageBox::information(this, tr("Problem!"), tr("You must specify a destination directory."),
                                     QMessageBox::Ok);
            return;
        } else if (ui->url_plainTextEdit->toPlainText().isEmpty()) {
            QMessageBox::information(this, tr("Problem!"), tr("You must specify a URL to grab."),
                                     QMessageBox::Ok);
            return;
        } else {
            url_plaintext = ui->url_plainTextEdit->toPlainText();
            info = routines->verifyFileExists(url_plaintext);

            if (info.response_code == 200) {
                downloadModel *dlModel = new downloadModel();
                dlModel->insertRows(0, 1, QModelIndex());

                QModelIndex index = dlModel->index(0, 0, QModelIndex());
                dlModel->setData(index, routines->extractFilename(url_plaintext), Qt::DisplayRole);

                info_ext = routines->curlGrabInfo(url_plaintext);

                index = dlModel->index(0, 1, QModelIndex());
                dlModel->setData(index, QString::number(routines->bytesToKilobytes(info_ext.content_length)), Qt::DisplayRole);

                index = dlModel->index(0, 2, QModelIndex());
                dlModel->setData(index, QString::number(0), Qt::DisplayRole);

                index = dlModel->index(0, 3, QModelIndex());
                dlModel->setData(index, QString::number(0), Qt::DisplayRole);

                index = dlModel->index(0, 4, QModelIndex());
                dlModel->setData(index, QString::number(0), Qt::DisplayRole);

                index = dlModel->index(0, 5, QModelIndex());
                dlModel->setData(index, QString::number(0), Qt::DisplayRole);

                index = dlModel->index(0, 6, QModelIndex());
                dlModel->setData(index, tr("Paused"), Qt::DisplayRole);

                index = dlModel->index(0, 7, QModelIndex());
                dlModel->setData(index, ui->url_dest_lineEdit->text(), Qt::DisplayRole);

                delete[] info.effective_url;
                delete[] info_ext.effective_url;
                delete[] info_ext.status_msg;
                delete dlModel;

                return;
            } else {
                QMessageBox::information(this, tr("Error!"), QString("%1").arg(
                                             info.effective_url), QMessageBox::Ok);
                return;
            }
        }
    case 1:
        if (ui->file_dest_lineEdit->text().isEmpty()) {
            QMessageBox::information(this, tr("Problem!"), tr("You must specify a destination directory."),
                                     QMessageBox::Ok);
            return;
        } else {
            // info = routines->verifyFileExists(ui->file_dest_lineEdit->text());

            // QModelIndex index = dlModel->index(0, 0, QModelIndex());
            // index = dlModel->index(0, 7, QModelIndex());
            // dlModel->setData(index, ui->file_dest_lineEdit->text(), Qt::DisplayRole);
        }
    default:
        QMessageBox::warning(this, tr("Internal Error!"), tr("An error was encountered within the "
                                                             "internals of the application! Please "
                                                             "restart the program."),
                             QMessageBox::Ok);
        return;
    }

    return;
}

void AddURL::on_buttonBox_rejected()
{
    AddURL::done(1);
}

void AddURL::on_file_dest_toolButton_clicked()
{
    QString dirDest = browseForDir();
    ui->file_dest_lineEdit->setText(dirDest);
    return;
}

void AddURL::on_file_import_toolButton_clicked()
{
    QMessageBox::information(this, tr("What do we have here..."), tr("It appears that this functionality "
                                                                     "is not yet implemented!"),
                             QMessageBox::Ok);
}

void AddURL::on_url_dest_toolButton_clicked()
{
    QString dirDest = browseForDir();
    ui->url_dest_lineEdit->setText(dirDest);
    return;
}

QString AddURL::browseForDir()
{
    // http://doc.qt.io/qt-5/qfiledialog.html
    QString dirDest =
            QFileDialog::getExistingDirectory(this, tr("Choose destination..."), QDir::homePath(),
                                              QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    return dirDest;
}
