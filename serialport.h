#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <QObject>
#include <QSerialPort>
#include <qserialport.h>
/*
 * Ring Buffer - MUST be a power of 2
 */

#define RING_BUF ( 1 << 12 )
#define RING_MSK ( RING_BUF - 1 )


class SerialPort : public QObject
{
    Q_OBJECT
public:
    int fd;
    explicit SerialPort(QObject *parent = nullptr);
    int  Open();
    int  Open(const QString vid,const QString pid, QSerialPort::BaudRate rate);
    int  Open(const QString dev,QSerialPort::BaudRate rate);
    void Ring( long int timeout );
    int Read( long int timeout, char *buf, int *len );
    int Peek( long int timeout, char *buf, int *len );


   int ReadCr(long int timeout, char *buf, int *len );
   int PeekCr( long int timeout );

   QString findUsbDeviceByVidPid(const QString& vendorIdHex, const QString& productIdHex);

public slots:
    void Write();
    void Write(uint8_t c);
    void Write(QString);
signals:

private slots:
        void onReadyRead();
private:

    QSerialPort *serial;
    char buf[RING_BUF] ;

    struct {
      char buf[RING_BUF] ;
      unsigned int head ;
      unsigned int tail ;
    } ring ;


};

#endif // SERIALPORT_H
