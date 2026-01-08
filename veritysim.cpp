#include "veritysim.h"
#include <QDebug>
// Opens UART
// Behaves like a Verity System
/*
Commands I may need to create
Command                            Response

wafer[lot][^sp]111222[^CR]       ACK_wfr[Lot][^CR][^NUL]
start[][^sp]Config_name[^cr]     ACK_start[^CR]run[^CR]NOTrdy[^CR]
start Data Stream                       ACK_data  ?? Part of the config file
stop[^CR]                                    ACK_stop[^CR]RDY[^CR]
rst[^CR]                                      ACK_reset[^CR]RDY[^CR]
tst [^CR]                                      ACK_test[^CR]

Events                                Value?
EP Hit                                ENDP^CR
DataSteam                        TRENDS data1^CR'
InstrumentError                 ERR[^CR]
*/




VeritySim::VeritySim(QObject *parent) : QObject(parent)
{
    //    rspQueue.clear();
        rspQueue = QList<QString>();
        cmdQueue = QList<QString>();
        // Open Serial Port to 9600
        sp = new SerialPort(this);
        //Bus 001 Device 005: ID 0403:6001 Future Technology Devices International, Ltd FT232 Serial (UART) IC
        // This is the Null Modem Cable with yellow stipe
        int good = sp->Open(QString("0403"),QString("6001"),(QSerialPort::BaudRate)QSerialPort::Baud9600);
        //int good = sp->Open(QString("0403"),QString("6001"),(QSerialPort::BaudRate)QSerialPort::Baud19200);
        if (good!= 0)
        {
            qCritical()<< "ERROR: OPENING SERIAL PORT";

        } else
        {
            qDebug()<<"VER SERIAL PORT ISOPEN";
        }
        eventTimer = new QTimer(this);
        eventTimer->setSingleShot(true);

        dataTimer = new QTimer(this);
        dataTimer->setInterval(1000);

        connect(eventTimer, SIGNAL(timeout()), this, SLOT(on_Event()));
        connect(dataTimer, SIGNAL(timeout()), this, SLOT(on_DataTimer()));
        //initValues();

    trigger1 = 0;
}


void VeritySim::on_Event()
{
    qDebug()<<"SENDING EP";
   sp->Write("ENDP\r");
}

void VeritySim::on_DataTimer()
{
    data1 = data1+10.0;
    data2 = data2+20.0;
        //trend[0][^sp]+000490.40000, +000590.40000[^CR]
    QString data = QString("%1, -%2").arg(data1, 11, 'f', 4, QChar('0')).arg(data2, 11, 'f', 4, QChar('0'));
    data.prepend("trend[0] ");
    data.append('\r');
    sp->Write(data);
}


int VeritySim::ProcessTst()
{
    qDebug()<<"Sending ACK_test";
    sp->Write("ACK_tst\r");
    return 0;

}

int VeritySim::ProcessStop()
{
    qDebug()<<"Process Stop Command";
    StopTimers();
    sp->Write("ACK_stop\r");
    sp->Write("RDY\r");
    return 0;
}


int VeritySim::ProcessStart()
{
    qDebug()<<"SENDING BACK 3 COMMANDS";
    sp->Write("ACK_start\r");
    sp->Write("run\r");
    sp->Write("NOTrdy\r");
    data1 = data2 = 0.0;

    if (dataTimer && (!dataTimer->isActive())) {
           dataTimer->start();
    }

    eventTimer->start(14000);
    return 0;
}

int VeritySim::ProcessWafer()
{
    sp->Write("ACK_wfr[lot]\r");
    sp->Write((uint8_t)0);
    return 0;
}


int VeritySim::ProcessRst()
{
    StopTimers();
    sp->Write("ACK_rst\r");
    sp->Write("RDY\r");
    return 0;
}

int VeritySim::ProcessNewToken(QString & token)
{
    //token.remove(' ');
    token.remove('\r');
    token.remove('\n');

    if ( token.contains("tst") || token.contains("test"))
    {
        qDebug()<<"PROCESSING TST";
            ProcessTst();
    }
    else if ( token.contains("stop") )
    {
            ProcessStop();
    }
    else if ( token.contains("rst") )
    {
            ProcessRst();
    }
    else if ( token.contains("start[]") )
    {
            ProcessStart();
    }
    else if ( token.contains("wafer[lot] ") )
    {
            ProcessWafer();
    } else
    {
        qDebug()<<"UNKNOWN Verity Token"<< token;
    }
return 0;

}
QString VeritySim::NewTokenAvail(void)
{
    char newCmd[1024];
    QString token;
    int len = sizeof(newCmd);

    int tokenSize = sp->ReadCr(100, newCmd, &len );

    if (0 < tokenSize) {
        trigger1 = 1;
        QByteArray tmp(newCmd, tokenSize);
        tmp.append('\0');          // ensure null termination

        token = QString::fromLatin1(tmp.constData());
        qDebug()<<"Token = "<<token;
    }
    return token;
}

void VeritySim::VerityCheck(void)
{
    QString newToken = NewTokenAvail();
    if (newToken.size()>0)
    {
        qDebug()<<"\n\n------------------------- VER TOKEN: = "<<newToken;
        ProcessNewToken(newToken);
    }
}

void VeritySim::StopTimers(void)
{
    if (dataTimer && (dataTimer->isActive())) {
           dataTimer->stop();
    }

    if (eventTimer && dataTimer->isActive()) {
           dataTimer->stop();
    }
}
