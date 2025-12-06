#include <QDebug>
#include "mks_pid.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <math.h>
#define I2C_ADAPTER "/dev/i2c-0"
#define I2C_DEVICE  0x00

#define USE_ANGLE
typedef unsigned char   u8;


extern "C" {
// Global file descriptor used to talk to the I2C bus:
int i2c_fd = -1;
// Default RPi B device name for the I2C bus exposed on GPIO2,3 pins (GPIO2=SDA, GPIO3=SCL):
const char *i2c_fname = "/dev/i2c-1";

// Returns a new file descriptor for communicating with the I2C bus:
int i2c_init(void) {
    if ((i2c_fd = open(i2c_fname, O_RDWR)) < 0) {
        char err[200];
        sprintf(err, "open('%s') in i2c_init", i2c_fname);
        perror(err);
        return -1;
    }

    // NOTE we do not call ioctl with I2C_SLAVE here because we always use the I2C_RDWR ioctl operation to do
    // writes, reads, and combined write-reads. I2C_SLAVE would be used to set the I2C slave address to communicate
    // with. With I2C_RDWR operation, you specify the slave address every time. There is no need to use normal write()
    // or read() syscalls with an I2C device which does not support SMBUS protocol. I2C_RDWR is much better especially
    // for reading device registers which requires a write first before reading the response.

    return i2c_fd;
}

void i2c_close(void) {
    close(i2c_fd);
}

// Write to an I2C slave device's register:
int i2c_write(u8 slave_addr, u8 reg, u8 data) {
    //int retval;
    u8 outbuf[4];

    struct i2c_msg msgs[1];
    struct i2c_rdwr_ioctl_data msgset[1];

    outbuf[0] = reg;
    outbuf[1] = data;
    outbuf[2] = 0x33;
    outbuf[3] = 0x42;

    msgs[0].addr = slave_addr;
    msgs[0].flags = 0;
    msgs[0].len = 2;
    msgs[0].buf = outbuf;

    msgset[0].msgs = msgs;
    msgset[0].nmsgs = 1;


    if (ioctl(i2c_fd, I2C_RDWR, &msgset) < 0) {
        perror("ioctl(I2C_RDWR) in i2c_write");
        return -1;
    }

    return 0;
}

// Read the given I2C slave device's register and return the read value in `*result`:
int i2c_read(u8 slave_addr, u8 reg, u8 *result) {
    //int retval;
    u8 outbuf[1], inbuf[1];
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msgset[1];

    msgs[0].addr = slave_addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = outbuf;

    msgs[1].addr = slave_addr;
    msgs[1].flags = I2C_M_RD | I2C_M_NOSTART;
    msgs[1].len = 1;
    msgs[1].buf = inbuf;

    msgset[0].msgs = msgs;
    msgset[0].nmsgs = 2;

    outbuf[0] = reg;

    inbuf[0] = 0;

    *result = 0;
    if (ioctl(i2c_fd, I2C_RDWR, &msgset) < 0) {
        perror("ioctl(I2C_RDWR) in i2c_read");
        return -1;
    }

    *result = inbuf[0];
    return 0;
}

int i2c_write_dac(uint16_t dac)
{
    return i2c_write(0x60, (dac>>8 & 0xF), (dac & 0xFF) );

}

}













#define MAX_BAR 100
#define MAX_DAC 4096
mks_pid::mks_pid(QObject *parent) : QObject(parent)
{

//    rspQueue.clear();
    rspQueue = QList<QString>();
    cmdQueue = QList<QString>();
    sp = new SerialPort(this);

    initValues();


    i2c_init();

    i2c_write_dac(0x300);
    dacUpdateCounter = 0;
    //i2c_write(0x60,01,03);
}



#if 0
// ---------------------------------------------
// MCP23017 Register Definitions (BANK = 0)
// ---------------------------------------------
#define IODIRA   0x00
#define IODIRB   0x01
#define IPOLA    0x02
#define IPOLB    0x03
#define IOCON    0x0A   // same as 0x0B in BANK=0 mode
#define GPPUA    0x0C
#define GPPUB    0x0D
#define GPIOA    0x12
#define GPIOB    0x13
#define OLATA    0x14
#define OLATB    0x15

// ---------------------------------------------
// MCP23017 I2C address (A2..A0 = 001) => 0x21
// You used 33 decimal; keep that here.
// ---------------------------------------------
#define MCP23017_ADDR   33   // 0x21

// ---------------------------------------------
// Port B bit assignments
// ---------------------------------------------
#define B0_GEN_POWER_BIT      0
#define B1_GEN1_RF_ON_BIT     1
#define B2_GEN1_ILK_EN_BIT    2
#define B3_PURGE_BIT          3
#define B4_ISOLATION_BIT      4

#define B0_GEN_POWER_MASK     (1u << B0_GEN_POWER_BIT)
#define B1_GEN1_RF_ON_MASK    (1u << B1_GEN1_RF_ON_BIT)
#define B2_GEN1_ILK_EN_MASK   (1u << B2_GEN1_ILK_EN_BIT)
#define B3_PURGE_MASK         (1u << B3_PURGE_BIT)
#define B4_ISOLATION_MASK     (1u << B4_ISOLATION_BIT)

// All input bits on Port B (B0..B4)
#define PORTB_INPUT_MASK      ( B0_GEN_POWER_MASK  \
                              | B1_GEN1_RF_ON_MASK \
                              | B2_GEN1_ILK_EN_MASK\
                              | B3_PURGE_MASK      \
                              | B4_ISOLATION_MASK )

// ---------------------------------------------
// Configure MCP23017
// ---------------------------------------------
int mks_pid::Configure23017(void)
{
    uint8_t addr = MCP23017_ADDR;

    // 0) Configure IOCON – disable sequential op (SEQOP=1)
    i2c_write(addr, IOCON, 0x20);

    // 1) Configure Directions: 0=output, 1=input
    // Port A: all outputs
    i2c_write(addr, IODIRA, 0x00);

    // Port B: B0..B4 inputs, B5..B7 outputs
    // bits 4..0 = 1 (input), bits 7..5 = 0 (output) => 0b00011111 = 0x1F
    i2c_write(addr, IODIRB, 0x1F);

    // 2) Configure Polarity Inversion
    // Port A: normal
    i2c_write(addr, IPOLA, 0x00);

    // Port B: invert only B0..B4 so active-low signals become active-high in SW
    i2c_write(addr, IPOLB, PORTB_INPUT_MASK);

    // 3) Configure Pull-ups
    // Port A: no pullups
    i2c_write(addr, GPPUA, 0x00);

    // Port B: enable pullups on B0..B4 (typical for switches / active-low inputs)
    i2c_write(addr, GPPUB, PORTB_INPUT_MASK);

    // 4) Clear Outputs
    i2c_write(addr, OLATA, 0x00);
    i2c_write(addr, OLATB, 0x00);

    // 5) Initialize last state (so first poll has a baseline)
    uint8_t current = 0;
    if (i2c_read(addr, GPIOB, &current) == 0) {
        m_lastPortBState = current;
    } else {
        // if read fails, default to 0
        m_lastPortBState = 0;
    }

    return 0;
}

// ---------------------------------------------
// Poll inputs on Port B and call handlers on change
// Call this periodically from your other routine
// ---------------------------------------------
void mks_pid::Poll23017Inputs(void)
{
    uint8_t addr = MCP23017_ADDR;
    uint8_t current = 0;

    if (i2c_read(addr, GPIOB, &current) != 0) {
        // handle error if you want
        return;
    }

    // current already has IPOLB applied, so bits are active-high in SW.
    uint8_t changed = current ^ m_lastPortBState;
    if (changed == 0) {
        // nothing changed
        return;
    }

    // Check each named bit
    if (changed & B0_GEN_POWER_MASK) {
        bool active = (current & B0_GEN_POWER_MASK) != 0;
        OnGenPowerChanged(active);
    }

    if (changed & B1_GEN1_RF_ON_MASK) {
        bool active = (current & B1_GEN1_RF_ON_MASK) != 0;
        OnGen1RfOnChanged(active);
    }

    if (changed & B2_GEN1_ILK_EN_MASK) {
        bool active = (current & B2_GEN1_ILK_EN_MASK) != 0;
        OnGen1IlkEnChanged(active);
    }

    if (changed & B3_PURGE_MASK) {
        bool active = (current & B3_PURGE_MASK) != 0;
        OnPurgeChanged(active);
    }

    if (changed & B4_ISOLATION_MASK) {
        bool active = (current & B4_ISOLATION_MASK) != 0;
        OnIsolationChanged(active);
    }

    // Update last known state
    m_lastPortBState = current;
}


void mks_pid::OnGenPowerChanged(bool active)
{
    // TODO: handle GEN_POWER input change
    // active = true  -> active-high in software
    // active = false -> inactive
}

void mks_pid::OnGen1RfOnChanged(bool active)
{
    // TODO: handle GEN1_RF_ON input change
}

void mks_pid::OnGen1IlkEnChanged(bool active)
{
    // TODO: handle GEN1_ILK_EN input change
}

void mks_pid::OnPurgeChanged(bool active)
{
    // TODO: handle PURGE input change
}

void mks_pid::OnIsolationChanged(bool active)
{
    // TODO: handle ISOLATION input change
}


#endif


void mks_pid::initValues(void)
{
    mks_setting.pidEnabled = false;
    mks_setting.angle    = 0.0;
    mks_setting.gain     = 0.0;
    mks_setting.lead     = 0.0;
    mks_setting.pressure = 1000.0;
    mks_setting.setPressure = 0.0;
    mks_setting.setAngle    = 40.0;


    simPid.N0Start = 1000.0; // Initial quantity - Atomspeare
    simPid.N0Delta = 0.0; // Initial quantity
    simPid.dt = 0.5;     // Time step
    simPid.N = 1000; // Quantity at time t
    simPid.t = 0; // Current time
    simPid.direction = 0;
    simPid.angle = 40.0;

    //sp->Open()  ;
    // Using the StarTech null modem USB cable
    //Bus 001 Device 003: ID 067b:23c3 Prolific Technology, Inc. USB-Serial Controller
    //int good = sp->Open(QString("0403"),QString("6001"),(QSerialPort::BaudRate)QSerialPort::Baud9600);
    int good = sp->Open(QString("067b"),QString("23c3"),(QSerialPort::BaudRate)QSerialPort::Baud9600);
    if (good!= 0)
    {
        qCritical()<< "ERROR: OPENING TVC SERIAL PORT";

    } else {
        qDebug()<<"Opened Serial Port in mks_pid::initValues() ";
    }

}
void mks_pid::Write()
{
       sp->Write();
}

void mks_pid::onUpdatePressure(double newPressure)
{
    mks_setting.pressure=(float)newPressure;
//    qDebug()<<"new pressure"<<newPressure;
}

void mks_pid::UpdateDac()
{
    //pressure 0-100.0 or 0-1000
    //DAC 0 - 4096


    // MAX DAC = 4096
    // .pressure = 0-
    uint16_t dacValue = (MAX_DAC-1);

    if (mks_setting.pressure>10.0)
        dacValue = (MAX_DAC-1);
    else
       {
        dacValue = (uint16_t)round(mks_setting.pressure/10.00 * (MAX_DAC-1));
      }

    if (dacValue != lastDac)
        qDebug()<<"* Press = "<<mks_setting.pressure<<"  Dac =  "<<  dacValue << " Angle = "<<mks_setting.angle;
    else if ((dacUpdateCounter++ % 0x40)==0)
        qDebug()<<"Press = "<<mks_setting.pressure<<"  Dac =  "<<  dacValue << " Angle = "<<mks_setting.angle;
    i2c_write_dac(dacValue);
    lastDac = dacValue;

}
void mks_pid::UpdatePressureSwitches()
{
    /*
    gpio->SetVac1Ilk((int)(mks_setting.pressure < 600));

    if ( mks_setting.pressure 300 ) {
        pressureSwitch2 = 0;
    } else  {
        pressureSwitch2 = 1;
    }*/
}

void mks_pid::UpdatePressure()
{
   UpdatePressureSwitches();
}

void mks_pid::UpdateSimulation()
{
   UpdatePressure();

   UpdateDac();
}

void mks_pid::ProcessNewTvsCommand()
{
   char buf[200];
   int len1 = 1;
   static uint count = 0;
   int press;

   QString cmd,rsp;

   //qDebug()<<"PROCESS";
   len1 = sp->PeekCr(50);
   if (len1>0)
   {
    len1 = sp->Read(0, buf, &len1);
    if (len1>0) {
       if (buf[0]!= '\n')   {
            commandBuffer.append(QString::fromLatin1(buf, len1));
       }
     }
   }

   int cr = commandBuffer.indexOf('\r');
   if (cr>=0) {
     //  qDebug()<<"ADDING CMD "<<len1;
       cmdQueue.append(commandBuffer.left(cr+1));
       commandBuffer.remove(0,cr+1);
   }

   if (!cmdQueue.isEmpty())
   {

       cmd = cmdQueue.takeFirst();
       ProcessNewIdealCommand(cmd);
       //ProcessNewCommand(cmd);
      if (!rspQueue.isEmpty())
      {
        rsp = rspQueue.takeFirst();
        sp->Write(rsp);
      }
   }
}

void mks_pid::AddCommand(QString cmd)
{
    qDebug()<<"Adding Command: "<<cmd;
    cmdQueue.append(cmd);
}


bool mks_pid::ProcessNewIdealCommand(QString command)
{
    bool goodConversion;
    QString rspCmd;
    int reqResponse;
    float reqValue;
    rspCmd.clear();

    if (!command.contains("pos"))
        qDebug()<<"Ideal Cmd:"<<command;

    command.remove("\n");
    command.remove("\r");
    if (command.contains("pos",  Qt::CaseInsensitive))
    {
        rspCmd = QString::number(mks_setting.angle,'f',1);
        rspCmd.append('\r');
 //       qDebug()<<"P";
//        qDebug()<<"Sending Angle back "<<rspCmd;
    } else
    if (command.contains("open",  Qt::CaseInsensitive))
    {
        mks_setting.angle = 90.0;
        emit AngleChanged((double)90.0);


    } else
    if (command.contains("close",  Qt::CaseInsensitive))
    {
        mks_setting.angle = 0.0;
        emit AngleChanged((double)0.0);

     } else
    if (command.contains("ang",  Qt::CaseInsensitive))
        {
            QString angString = command.mid(command.indexOf("ang")+4);
         //   qDebug()<<"ANGLE STRING "<<angString;
            bool ok;
            double ang = angString.toDouble((&ok));

            if (ok) {
           //     qDebug()<<"NEW ANGLE"<<ang;
                mks_setting.angle = ang;
                emit AngleChanged(ang);
            } else

            {
                qDebug()<< " ---- ERROR Bad Angle in command"<<command;
            }
        } else
    {
        qDebug()<<"---------- ERROR:: UNKNOWN IDEAL COMMAND ----- "<<command;
    }


    //if (!rspCmd.isEmpty())
   // {
   //     qDebug()<<" ERROR:: UNKNOWN IDEAL COMMAND "<<command;
   // } else
    if (!rspCmd.isEmpty())
    {
      // qDebug()<<"sp "<<rspCmd;
       sp->Write(rspCmd);

    }
    return true;
}

bool mks_pid::ProcessNewCommand(QString command)
{
    bool goodConversion;
    QString rspCmd;
    int reqResponse;
    float reqValue;
    qDebug()<<"Processing New Command:"<<command;

    command.remove("\n");
    command.remove("\r");
    rspCmd = command;

    // Split the string by comma
    QStringList cmdList = command.split(" ");

    if (cmdList.length() == 1)
    {
        QString newCommand;
        newCommand = cmdList.at(0);

        //qDebug()<<"Command Length = 1  Size = "<< newCommand.length();
        if (newCommand.length()>3)
        {
            if (newCommand.startsWith("S1"))
            {
                newCommand.insert(2,QString(" "));
                cmdList = newCommand.split(" ");
            } else
            if (newCommand.startsWith("V"))
            {
                newCommand.insert(1,QString(" "));
                cmdList = newCommand.split(" ");
            }
        }
    }



    switch(cmdList.at(0).toStdString()[0])
    {
        case 'L': case 'l':
            reqValue = cmdList.at(1).toFloat(&goodConversion);
            qDebug()<<"Lead Command:"<<command<<" Lead = "<<cmdList.at(1)<< " - "<<reqValue;
            if (goodConversion)
            {
                sp->Write(command);
                mks_setting.lead = reqValue;
            } 
            else
            {
                response = "Bad Floating Point in Lead Command";
                qDebug()<< response;
            }
         break;

        case 'G': case 'g':
        reqValue = cmdList.at(1).toFloat(&goodConversion);
        
            qDebug()<<"Gain Command:"<<command<< " Gain = " << cmdList.at(1) << " - " << reqValue;
            if (goodConversion)
            {
                sp->Write(command);
                mks_setting.gain = reqValue;
            } 
            else
            {
                response = "Bad Floating Point in Gain Command";
                qDebug()<< response;
            }
        break;

        case 'S': case 's':
            initValues();
            reqValue = cmdList.at(1).toFloat(&goodConversion);
       
            qDebug()<<"Set Pressure Command:"<<command<<" Pressure = " << cmdList.at(1)<< " Percent - "<<reqValue;

            if (goodConversion)
            {
                //sp->Write(command);
                qDebug()<<" Setting mks_setting.setPressure = "<<reqValue;
                mks_setting.setPressure = reqValue;
                mks_setting.loopCount = 0.0;
                simPid.N0Start = mks_setting.pressure;
                simPid.N0Delta = (mks_setting.setPressure - mks_setting.pressure) ;

                simPid.t = 0;
                qDebug()<<" Setting simPid.N0Start = "<<simPid.N0Start;
                qDebug()<<" Setting simPid.N0Delta = "<<simPid.N0Delta;

                if (simPid.N0Delta>0)
                    simPid.direction = 1;
                else
                    simPid.direction = -1;
                simPid.t = 0;
            } 
            else
            {
                response = "Bad Floating Point in Set Command";
                qDebug()<< response;
            }
            break;

        case 'V':
            // As Angle gets larger (90 == open), the speed at which the pressure goes down increases.
            qDebug()<<"SET ANGLE COMMAND DISABLING PID AS WELL";
            mks_setting.pidEnabled = 0;
            simPid.direction = 0;
        break;

        case 'P': case 'p':
            reqValue = cmdList.at(1).toFloat(&goodConversion);
            qDebug()<<"Set Valve Position Command:"<<command<<" Angle = " << cmdList.at(1)<< " - "<<reqValue;

            if (goodConversion)
            {
                //sp->Write(command);
                mks_setting.angle = cmdList.at(1).toFloat();
                simPid.angle = mks_setting.angle;

            }
            else
            {
                response = "Bad Floating Point in Set Command";
                qDebug()<< response;
            } 
            break;



        case 'O': case 'o':
            qDebug()<<"Open Command:"<<command;
        break;

        case 'C': case 'c':
            qDebug()<<"Close Command:"<<command;
        break;

        case 'D': case 'd': case 'A': case 'a':
            qDebug()<<"";
            qDebug()<<"D or A command:"<<command;
            mks_setting.pidEnabled = 1;
        break;

        case 'R': case 'r':
           // qDebug()<<"Reponse Command?:"<<command;
            rspCmd.remove(QChar('r'), Qt::CaseInsensitive);
            reqResponse = rspCmd.toInt(&goodConversion);
            //qDebug()<<"Number = "<<rspCmd<<" "<<reqResponse;
            if (goodConversion)
            {
                ProcessReponseRequest(reqResponse);
            }
            else
            {
                response = "Bad Int conversion in Response Command";
                qDebug()<< response;
            }

        break;


    default:
        qDebug()<<"Unkown Command"<<command;
        response = "ERROR: Unknown Command" + command;
    }
    return true;
}

int mks_pid::ProcessReponseRequest(int parameter)
{
    QString response = "NA";
    static int counter = 0;
   // qDebug()<<"Processing R " << parameter;
    switch (parameter)
    {
        case 2:  // Get Gain
            //response = QString("G ") + QString::number(mks_setting.gain,'f',2);
            response = QString("G %1").arg(mks_setting.gain,5,'f',1,'0');
        break;

        case 3:  // Get Lead
            //response = QString("L ") + QString::number(mks_setting.lead,'f',2);
            response = QString("L %1").arg(mks_setting.lead,5,'f',1,'0');
        break;

        case 4:  // Get Analog Set Point
            //response = QString("A ") + QString::number(mks_setting.setPoint,'f',2);

            response = QString("A %1").arg(mks_setting.setPressure,5,'f',1,'0');
        break;

        case 5:  // Get Pressure
            //response = QString("P ") + QString::number(mks_setting.pressure,'g',5);
          //  if ((counter++ % 10) == 0)
//                mks_setting.pressure = mks_setting.pressure + 0.1;

            response = QString("P %1").arg(mks_setting.pressure,5,'f',1,'0');

        break;

        case 6:  // Get Position
            //response = QString("V ") + QString::number(mks_setting.angle,'f',2);
        if ((counter % 10) == 0)
            mks_setting.angle = mks_setting.angle + 1.0;
        if (mks_setting.angle>88.0)
        {
            mks_setting.angle = 0;
        }

            response = QString("V %1").arg(mks_setting.angle,5,'f',1,'0');
        break;

    }

    qDebug()<<"Sending back "<<response;
    rspQueue.append(QString(response));

    return 0;
}

bool mks_pid::IsRespQueueEmpty()
{
    qDebug()<<"IS";
    return rspQueue.isEmpty();
}

QString mks_pid::ReadResponseQueue()
{
    qDebug()<<"RD";
    return rspQueue.first();
}

