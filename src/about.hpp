#ifndef ABOUT_HPP
#define ABOUT_HPP

#include <QDialog>

namespace Ui {
class About;
}

class About : public QDialog
{
    Q_OBJECT

public:
    explicit About(QWidget *parent = 0);
    ~About();

private slots:
    void on_close_buttonBox_rejected();

private:
    Ui::About *ui;
};

#endif // ABOUT_HPP
