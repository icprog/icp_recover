#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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

private:
    Ui::MainWindow *ui;

public slots:
    void PushButtonCheckHour();
    void PushButtonCheckDay();
    void PushButtonRecoveryHour();
    void PushButtonRecoveryDay();
    void PushButtonRecoveryOneInRange();
    void PushButtonRecoveryAllInRange();


};

#endif // MAINWINDOW_H
