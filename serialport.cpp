#include "serialport.h"
#include <QDebug>
#include <QDir>

SerialPort::SerialPort(QObject *parent) : QObject(parent)
{

}


QString SerialPort::findUsbDeviceByVidPid(const QString& vendorIdHex, const QString& productIdHex)
{
    QDir devDir("/dev");
    QStringList candidates = devDir.entryList(QStringList() << "ttyUSB*" << "ttyACM*",
                                              QDir::System | QDir::Readable | QDir::Files);

    QString normalizedVendorId = vendorIdHex.toLower().remove("0x");
    QString normalizedProductId = productIdHex.toLower().remove("0x");

    foreach (const QString& dev, candidates) {
        QString fullPath = "/dev/" + dev;
        QString cmd = "udevadm info -q all -n " + fullPath;

        FILE* fp = popen(cmd.toUtf8().data(), "r");
        if (!fp)
            continue;

        QString foundVid, foundPid;
        char buffer[512];
        while (fgets(buffer, sizeof(buffer), fp)) {
            QString line = QString::fromUtf8(buffer).trimmed();

            if (line.startsWith("E: ID_VENDOR_ID="))
                foundVid = line.section('=', 1, 1).trimmed().toLower();
            else if (line.startsWith("E: ID_MODEL_ID="))
                foundPid = line.section('=', 1, 1).trimmed().toLower();

            if (!foundVid.isEmpty() && !foundPid.isEmpty())
                break;
        }
        pclose(fp);

        if ((foundVid == normalizedVendorId) && (foundPid==normalizedProductId)) {
            return fullPath;
        }
    }

    return QString();  // No match
}

int  SerialPort::Open(const QString vid,const QString pid, QSerialPort::BaudRate rate)
{
    serial = new QSerialPort(this);
    QString portName = findUsbDeviceByVidPid(vid,pid);
    if (portName.isEmpty())
    {
        qDebug()<<"Can not find serial device.  Vid="<<vid<<" pid="<<pid;;
        return -1;
    }
    serial->setPortName(portName);


    serial->setBaudRate((QSerialPort::BaudRate) rate);
    serial->setDataBits( (QSerialPort::DataBits) 8);
    serial->setParity( QSerialPort::NoParity);
    serial->setStopBits( QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    QString BaudRate="Unknown";
    if (rate == (QSerialPort::Baud9600)) BaudRate = "9600";
    else if (rate == (QSerialPort::Baud19200)) BaudRate = "19200";
    else if (rate == (QSerialPort::Baud38400)) BaudRate = "38400";
    qDebug()<<" Vid="<<vid<<" pid="<<pid<<" PortName="<<portName<< " Rate:"<<BaudRate;
    if (!serial->open(QIODevice::ReadWrite)) {
        qCritical() << "Error opening serial port:" <<  serial->errorString();
    }
    connect(serial, &QSerialPort::readyRead,
            this, &SerialPort::onReadyRead);

    return 0;
}

int  SerialPort::Open(const QString dev, QSerialPort::BaudRate rate)
{
    serial = new QSerialPort(this);
#ifdef Q_OS_WIN32
    qDebug()<<"COM3";
    serial->setPortName("COM3");
#endif
#ifdef Q_OS_LINUX

    QString portName = dev;
    qDebug()<<"Selecting "<<dev;
    serial->setPortName(portName);
    qDebug()<<"Verity Selecting "<<portName;

#endif

    serial->setBaudRate((QSerialPort::BaudRate) rate);
    serial->setDataBits( (QSerialPort::DataBits) 8);
    serial->setParity( QSerialPort::NoParity);
    serial->setStopBits( QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);
    if (!serial->open(QIODevice::ReadWrite)) {
           qCritical() << "Error opening serial port:" <<  serial->errorString();

       }
    connect(serial, &QSerialPort::readyRead,
            this, &SerialPort::onReadyRead);
    return 0;


}
int SerialPort::Open()
{
    serial = new QSerialPort(this);
#ifdef Q_OS_WIN32
    qDebug()<<"COM3";
    serial->setPortName("COM3");
#endif
#ifdef Q_OS_LINUX

    QString portName = "/dev/ttyUSB0";
    QString xx = findUsbDeviceByVidPid("067b","23c3");
  //    qDebug()<<"Selecting "<<xx;
 //   qDebug()<<"Selecting /dev/ttyUSB0";
    serial->setPortName(portName);
    qDebug()<<"Verity Selecting "<<portName;
//    serial->setPortName("/dev/ttyAMA3");

#endif

    serial->setBaudRate(QSerialPort::Baud9600);
    serial->setDataBits( (QSerialPort::DataBits) 8);
    serial->setParity( QSerialPort::NoParity);
    serial->setStopBits( QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);
    if (!serial->open(QIODevice::ReadWrite)) {
           qCritical() << "Error opening serial port:" <<  serial->errorString();

       }

    connect(serial, &QSerialPort::readyRead,
            this, &SerialPort::onReadyRead);
    return 0;
}

void SerialPort::onReadyRead()
{
    QByteArray data = serial->readAll();

    for (int i = 0; i < data.size(); ++i)
    {
        ring.buf[ring.head] = data[i];
        ring.head = (ring.head + 1) & RING_MSK;
    }
}


void SerialPort::Write()
{
       serial->write("ABC\n");
}

void SerialPort::Write(uint8_t c)
{
       serial->write(QByteArray(1, uchar(c)));
}


void SerialPort::Write(QString cmd)
{
       serial->write(cmd.toLatin1().data());
       serial->flush();
}
/*
 *
 */
void SerialPort::Ring( long int timeout )
{
 return;
  int  i, actual ;
  //fd_set rfds ;
  //struct timeval tv ;
  unsigned int used ;

//  if( fd < 0 ) return ;

  /* Get bytes used in circular buffer */
  used = ring.head - ring.tail ;
  if( used > RING_BUF ) used += RING_BUF ;
/*
  FD_ZERO( &rfds ) ;
  FD_SET( fd, &rfds ) ;

  tv.tv_sec = 0 ;
  tv.tv_usec = timeout ; */
  serial->waitForReadyRead(1) ;
  if (serial->bytesAvailable()==0)
      return;

  //qDebug()<<"READING BYTES "<<serial->bytesAvailable();
  actual = serial->read(buf, RING_BUF ) ;
  //qDebug()<<"Actual "<<actual;
  if( actual > -1 ) {
    for( i = 0 ; i < actual ; ++i ) {
      ring.buf[ring.head] = buf[i] ;
      ring.head++ ;
      ring.head &= RING_MSK ;
    }
  }

  return ;

}


/*
 *
 */
int SerialPort::Peek( long int timeout, char *buf, int *len )
{

  unsigned int index ;
  int used ;

  //if( fd < 0 ) return( -1 ) ;
  if (!serial || !serial->isOpen())
      return -1;
  //Ring( timeout ) ;

  if( 0 == *len ) return( 0 ) ;
  if( *len >= RING_BUF ) return( -1 ) ;

  /* Get bytes used in circular buffer */
  used = ring.head - ring.tail ;
  if( used < 0 ) used += RING_BUF ;
  if( *len > used ) return( 0 ) ;

  index = ring.tail ;

  for( int i = 0 ; i < *len ; ++i ) {
    buf[i] = ring.buf[index] ;
    index++ ;
    index &= RING_MSK ;
  }

  return( *len ) ;

}

/*
 *
 */
int SerialPort::PeekCr( long int timeout )
{

  unsigned int index ;
  unsigned int count ;

  //if( fd < 0 ) return( -1 ) ;
  if (!serial || !serial->isOpen())
      return -1;

  //Ring( timeout ) ;

  index = ring.tail ;
  count = 0 ;

  while( index != ring.head ) {
    if( ring.buf[index] == '\r' ) {
      count++ ;
      return( (int)count ) ;
    }
    count++ ;
    index++ ;
    index &= RING_MSK ;
  }

  return( 0 ) ;

}

/*
 *
 */
int SerialPort::ReadCr( long int timeout, char *buf, int *len )
{

  unsigned int index ;
  unsigned int count ;

  //if( fd < 0 ) return( -1 ) ;
  if (!serial || !serial->isOpen())
      return -1;

  if( 0 == *len ) return( 0 ) ;
  if( *len >= RING_BUF ) return( -1 ) ;

  //Ring( timeout ) ;

  index = ring.tail ;
  count = 0 ;

  while( index != ring.head ) {
    char c = ring.buf[index] ;

    if( count >= (unsigned int)(*len) ) return( 0 ) ;

    buf[count] = c ;
    count++ ;

    index++ ;
    index &= RING_MSK ;

    if( c == '\r' ) {
      ring.tail = ( ring.tail + count ) & RING_MSK ;
      return( (int)count ) ;
    }
  }

  return( 0 ) ;

}

/*
 *
 */
int SerialPort::Read( long int timeout, char *buf, int *len )
{

  int i, used ;

  //Ring( timeout ) ;

  //qDebug()<<"LEN = "<<*len;
  if( 0 == *len ) return( 0 ) ;
  if( *len >= RING_BUF ) return( -1 ) ;

  /* Get bytes used in circular buffer */
  used = ring.head - ring.tail ;
  if( used < 0 ) used += RING_BUF ;
  if( *len > used ) return( 0 ) ;

  for( i = 0 ; i < *len ; ++i ) {
    buf[i] = ring.buf[ring.tail] ;
    ring.tail++ ;
    ring.tail &= RING_MSK ;
  }

  return( *len ) ;

}

