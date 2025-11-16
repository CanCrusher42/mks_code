#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QSerialPort>
#include <serialport.h>
#include <mks_pid.h>
#include <veritysim.h>
#include "mcp23017.h"
#include "vacuumchamber.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    bool Test1();
    bool Test2(mks_pid * pid);
    bool Test3(mks_pid * pid);
    bool Test4(mks_pid * pid);

    bool RunVerity();
public slots:
    void myfunction();
private:
    Ui::MainWindow *ui;


    QTimer *tvcTimer;
    QTimer *simTimer;
    QTimer *verTimer;
    QTimer *mcpTimer;
    QTimer *chamberTimer;

    VacuumChamber *chamber;
    Mcp23017 *gpio ;
    mks_pid *mks;
    VeritySim *ver;

};
#endif // MAINWINDOW_H
