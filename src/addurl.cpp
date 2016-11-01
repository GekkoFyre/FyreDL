#include "addurl.hpp"
#include "ui_addurl.h"
#include "default_var.hpp"
#include "dl_view.hpp"
#include "curl_multi.hpp"
#include "curl_easy.hpp"
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>

AddURL::AddURL(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddURL)
{
    ui->setupUi(this);
    routines = new GekkoFyre::CmnRoutines();

    ui->url_dest_lineEdit->setText(QDir::homePath());
    ui->file_dest_lineEdit->setText(QDir::homePath());
}

AddURL::~AddURL()
{
    delete ui;
    delete routines;
}

void AddURL::on_buttonBox_accepted()
{
    GekkoFyre::GkCurl::CurlInfo info;
    GekkoFyre::GkCurl::CurlInfoExt info_ext;
    GekkoFyre::GkCurl::CurlDlInfo dl_info;
    QString url_plaintext;

    switch (ui->inputTabWidget->currentIndex()) {
    case 0:
        if (ui->url_dest_lineEdit->text().isEmpty()) {
            QMessageBox::information(this, tr("Problem!"), tr("You must specify a destination directory."),
                                     QMessageBox::Ok);
            return AddURL::done(QDialog::Rejected);
        } else if (ui->url_plainTextEdit->toPlainText().isEmpty()) {
            QMessageBox::information(this, tr("Problem!"), tr("You must specify a URL to grab."),
                                     QMessageBox::Ok);
            return AddURL::done(QDialog::Rejected);
        } else {
            try {
                url_plaintext = ui->url_plainTextEdit->toPlainText();
                info = GekkoFyre::CurlEasy::verifyFileExists(url_plaintext);

                if (info.response_code == 200) {
                    info_ext = GekkoFyre::CurlEasy::curlGrabInfo(url_plaintext);
                    dl_info.dlStatus = GekkoFyre::DownloadStatus::Unknown;
                    dl_info.file_loc = ui->url_dest_lineEdit->text().toStdString().c_str();
                    dl_info.ext_info.content_length = info_ext.content_length;
                    dl_info.ext_info.effective_url = info_ext.effective_url;
                    dl_info.ext_info.response_code = info_ext.response_code;
                    dl_info.ext_info.status_ok = info_ext.status_ok;
                    routines->writeDownloadItem(dl_info);

                    emit sendDetails(dl_info.ext_info.effective_url, dl_info.ext_info.content_length, 0, 0, 0,
                                     0, dl_info.dlStatus, dl_info.ext_info.effective_url, dl_info.file_loc);
                    return AddURL::done(QDialog::Accepted);
                } else {
                    QMessageBox::information(this, tr("Error!"), QString("%1").arg(
                                                 QString::fromStdString(info.effective_url)), QMessageBox::Ok);
                    return AddURL::done(QDialog::Rejected);
                }
            } catch (const std::exception &e) {
                QMessageBox::warning(this, tr("Error!"), tr("%1").arg(e.what()), QMessageBox::Ok);
                return;
            }
        }
    case 1:
        if (ui->file_dest_lineEdit->text().isEmpty()) {
            QMessageBox::information(this, tr("Problem!"), tr("You must specify a destination directory."),
                                     QMessageBox::Ok);
            return AddURL::done(QDialog::Rejected);
        } else {
            info = GekkoFyre::CurlEasy::verifyFileExists(ui->file_dest_lineEdit->text());
            return AddURL::done(QDialog::Accepted);
        }
    default:
        QMessageBox::warning(this, tr("Internal Error!"), tr("An error was encountered within the "
                                                             "internals of the application! Please "
                                                             "restart the program."),
                             QMessageBox::Ok);
        return AddURL::done(QDialog::Rejected);
    }

    return AddURL::done(QDialog::Rejected);
}

void AddURL::on_buttonBox_rejected()
{
    this->close();
    return;
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
