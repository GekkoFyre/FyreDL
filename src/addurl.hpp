#ifndef ADDURL_HPP
#define ADDURL_HPP

#include "cmnroutines.hpp"
#include <QDialog>
#include <QString>

namespace Ui {
class AddURL;
}

class AddURL : public QDialog
{
    Q_OBJECT

public:
    explicit AddURL(QWidget *parent = 0);
    ~AddURL();

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void on_file_dest_toolButton_clicked();
    void on_file_import_toolButton_clicked();
    void on_url_dest_toolButton_clicked();

signals:
    void sendDetails(const std::string &fileName, const double &fileSize, const int &downloaded,
                     const double &progress, const int &upSpeed, const int &downSpeed,
                     const GekkoFyre::DownloadStatus &status, const std::string &url,
                     const std::string &destination);

private:
    Ui::AddURL *ui;
    GekkoFyre::CmnRoutines *routines;

    QString browseForDir();
};

#endif // ADDURL_HPP
