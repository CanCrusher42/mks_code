#ifndef VERITYSIM_H
#define VERITYSIM_H

#include <QObject>
#include "serialport.h"
#include <QTimer>

class VeritySim : public QObject
{
    Q_OBJECT
public:
    explicit VeritySim(QObject *parent = nullptr);

public slots:
    void VerityCheck(void);
    void on_Event(void);
    void on_DataTimer(void);
signals:


private:
        SerialPort *sp;
        QList<QString> cmdQueue;
        QList<QString> rspQueue;
        QString response;

        QString NewTokenAvail(void);
        int ProcessNewToken(QString & token);
        int ProcessTst();
        int ProcessStop();
        int ProcessRst();
        int ProcessStart();
        int ProcessWafer();
        void StopTimers();
        QTimer *eventTimer;
        QTimer *dataTimer;
        double data1, data2;
};

#endif // VERITYSIM_H
