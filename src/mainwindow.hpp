#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_action_Open_a_File_triggered();

    void on_actionEnter_URL_triggered();

    void on_actionE_xit_triggered();

    void on_action_About_triggered();

    void on_action_Documentation_triggered();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_HPP
