#include "mcp23017.h"
#include <unistd.h>
#include <QDebug>

// External low-level I2C functions
extern "C" int i2c_write(uint8_t addr, uint8_t reg, uint8_t data);
extern "C" int i2c_read(uint8_t addr, uint8_t reg, uint8_t *result);
const QString names[] = {
    "PURGE",        //0
    "ISOLATION",   //1
    "GN0_RF1-GEN_RF1",         //2
    "GEN1_ILK_EN",  //3
    "GEN_POWER",         //4
    "XXXXXXXX"     //5
};

// ===============================================================
// Constructor
// ===============================================================
Mcp23017::Mcp23017(uint8_t i2cAddress, QObject *parent)
    : QObject(parent),
      addr(i2cAddress),
      m_lastPortBState(0)
{

    Init();


}

// ===============================================================
// Initialization
// ===============================================================
int Mcp23017::Init()
{


    int result =0;

    connect(this, SIGNAL(GenPowerChanged(bool)), this, SLOT(OnGenPowerChanged(bool)));
    connect(this, SIGNAL(Gen1RfOnChanged(bool)), this, SLOT(OnGen1RfOnChanged(bool)));
    connect(this, SIGNAL(Gen1IlkEnChanged(bool)), this, SLOT(OnGen1IlkEnChanged(bool)));
    connect(this, SIGNAL(PurgeChanged(bool)), this, SLOT(OnPurgeChanged(bool)));
    connect(this, SIGNAL(IsolationChanged(bool)), this, SLOT(OnIsolationChanged(bool)));

    // Disable sequential addressing (SEQOP = 1)
    result = i2c_write(addr, IOCON, 0x20);
    usleep(10);

    if (result<0)
    {
        qDebug()<<"ERROR WRITTING TO I2C";
        return result;
    }
    // ------------ Port A Configuration ------------
    result += i2c_write(addr, IODIRA, 0x00);
    result +=i2c_write(addr, IPOLA,  0x00);
    result +=i2c_write(addr, GPPUA,  0x00);
    result +=i2c_write(addr, OLATA,  0x00);
    if (result<0)
        return result;

    // ------------ Port B Configuration ------------
    result +=i2c_write(addr, IODIRB, 0x1F);  // B0..B4 inputs
    result +=i2c_write(addr, IPOLB,  PORTB_INPUT_MASK); // invert inputs (active-low -> active-high)
    result +=i2c_write(addr, GPPUB,  PORTB_INPUT_MASK);
    result +=i2c_write(addr, OLATB,  0x00);
    if (result<0)
        return result;

    // Initialize last state
    uint8_t initial = 0;
    result +=i2c_read(addr, GPIOB, &initial);
    m_lastPortBState = initial;
    qDebug()<<"Initial GPIO ="<<QString::number(m_lastPortBState, 16);
    return result;
}

// ===============================================================
// Poll Inputs and Emit Qt Signals
// ===============================================================
void Mcp23017::PollInputs()
{
    uint8_t portB = 0;
    if (i2c_read(addr, GPIOB, &portB) != 0)
        return;
  //  qDebug()<<"Data = "<<portB;
    uint8_t changed = portB ^ m_lastPortBState;
    if (!changed)
        return;
    qDebug()<<"Change Detected Port B = "<< QString::number(portB, 16)<<"  Was="<<QString::number(m_lastPortBState, 16);

    if (changed & B0_PURGE_MASK)
        emit PurgeChanged(portB & B0_PURGE_MASK);

    if (changed & B1_ISOLATION_MASK)
        emit IsolationChanged(portB & B1_ISOLATION_MASK);

    if (changed & B3_GEN1_ILK_EN_MASK)
        emit Gen1IlkEnChanged(portB & B3_GEN1_ILK_EN_MASK);


    if (changed & B4_GEN_POWER_MASK)
        emit GenPowerChanged(portB & B4_GEN_POWER_MASK);

    if (changed & B2_GEN1_RF_ON_MASK)
        emit Gen1RfOnChanged(portB & B2_GEN1_RF_ON_MASK);

    m_lastPortBState = portB;
}



// ===============================================================
// GPIO Simulation Engine
// ===============================================================
void Mcp23017::GpioSimulation()
{
    //qDebug()<< "POLLING";
    PollInputs();
    uint8_t portB = 0;
    if (i2c_read(addr, GPIOB, &portB) != 0)
        return;

    EvaluateFrmGen0Ilk(portB);
}

// ===============================================================
// Simulation Rule: FRM_GEN0_ILK
// ===============================================================
void Mcp23017::EvaluateFrmGen0Ilk(uint8_t portB)
{
    bool genPower = (portB & B4_GEN_POWER_MASK);
    bool genIlkEn = (portB & B3_GEN1_ILK_EN_MASK);

    SetFrmGen0Ilk(genPower && genIlkEn);
}

// ===============================================================
// Output Control (read-modify-write)
// ===============================================================
void Mcp23017::SetFrmGen0Ilk(bool active)
{
    uint8_t portA = 0;

    if (i2c_read(addr, OLATA, &portA) != 0)
        return;

    if (active)
        portA |= A0_FRM_GEN0_ILK_MASK;
    else
        portA &= ~A0_FRM_GEN0_ILK_MASK;

    i2c_write(addr, OLATA, portA);
}

bool Mcp23017::GetFrmGen0Ilk()
{
    uint8_t portA = 0;
    if (i2c_read(addr, OLATA, &portA) != 0)
        return false;

    return (portA & A0_FRM_GEN0_ILK_MASK);
}


void Mcp23017::OnGenPowerChanged(bool active)
{
    // TODO: handle GEN_POWER input change
    qDebug()<<"Generator Power  = "<<active;
    // active = true  -> active-high in software
    // active = false -> inactive
}

void Mcp23017::OnGen1RfOnChanged(bool active)
{
    // TODO: handle GEN1_RF_ON input change
    qDebug()<<"Gen 1 Rf On  Changed = "<<active;
}

void Mcp23017::OnGen1IlkEnChanged(bool active)
{
    // TODO: handle GEN1_ILK_EN input change
    qDebug()<<"Gen 1 Interlocl Changed = "<<active;
}

void Mcp23017::OnPurgeChanged(bool active)
{
    qDebug()<<"Purge Changed = "<<active;
    // TODO: handle PURGE input change
}

void Mcp23017::OnIsolationChanged(bool active)
{

    // TODO: handle ISOLATION input change
    qDebug()<<"Isolation Changed = "<<active;
}
