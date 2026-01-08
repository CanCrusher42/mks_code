#include "mcp23017.h"
#include <unistd.h>
#include <QDebug>

// External low-level I2C functions
extern "C" int i2c_write(uint8_t addr, uint8_t reg, uint8_t data);
extern "C" int i2c_read(uint8_t addr, uint8_t reg, uint8_t *result);

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
    int result = 0;

    // Internal signals/slots
    connect(this, SIGNAL(PurgeChanged(bool)),       this, SLOT(OnPurgeChanged(bool)));
    connect(this, SIGNAL(IsolationChanged(bool)),   this, SLOT(OnIsolationChanged(bool)));
    connect(this, SIGNAL(Gen1RfOnChanged(bool)),    this, SLOT(OnGen1RfOnChanged(bool)));
    connect(this, SIGNAL(Gen1IlkEnChanged(bool)),   this, SLOT(OnGen1IlkEnChanged(bool)));
    connect(this, SIGNAL(GenPowerChanged(bool)),    this, SLOT(OnGenPowerChanged(bool)));

    // Disable sequential addressing
    result = i2c_write(addr, IOCON, 0x20);
    usleep(10);
    if (result < 0) return result;

    // PORT A: all outputs, no pullups
    result += i2c_write(addr, IODIRA, 0x00);
    result += i2c_write(addr, IPOLA,  0x00);
    result += i2c_write(addr, GPPUA,  0x00);

    // Drive A0–A3 HIGH at startup
    uint8_t initialA =
            A0_DOOR_ILK_MASK |
            A1_AIR_ILK_MASK  |
            A2_VAC1_ILK_MASK |
            A3_VAC2_ILK_MASK;

    result += i2c_write(addr, OLATA, initialA);

    if (result < 0) return result;

    // PORT B: inputs B0..B4
    result += i2c_write(addr, IODIRB, PORTB_INPUT_MASK);
    result += i2c_write(addr, IPOLB,  PORTB_INPUT_MASK);
    result += i2c_write(addr, GPPUB,  PORTB_INPUT_MASK);
    result += i2c_write(addr, OLATB,  0x00);
    if (result < 0) return result;

    // Read initial Port B
    uint8_t initial = 0;
    i2c_read(addr, GPIOB, &initial);
    m_lastPortBState = initial;

    qDebug() << "MCP23017 Init, PortB initial =" << QString::number(m_lastPortBState, 16);

    return result;
}

// ===============================================================
// Poll Inputs
// ===============================================================
void Mcp23017::PollInputs()
{
    uint8_t portB = 0;
    uint8_t portA = 0;
    if (i2c_read(addr, GPIOB, &portB) != 0)
        return;
//    if (i2c_read(addr, GPIOA, &portA) != 0)
//        return;

    uint8_t changed = portB ^ m_lastPortBState;
    if (changed) {
  //      PrintPortB();
        if (changed & B0_PURGE_MASK)
            emit PurgeChanged(portB & B0_PURGE_MASK);

        if (changed & B1_ISOLATION_MASK)
            emit IsolationChanged(portB & B1_ISOLATION_MASK);

        if (changed & B2_GEN1_RF_ON_MASK)
            emit Gen1RfOnChanged(portB & B2_GEN1_RF_ON_MASK);

        if (changed & B3_GEN1_ILK_EN_MASK)
            emit Gen1IlkEnChanged(portB & B3_GEN1_ILK_EN_MASK);

        if (changed & B4_GEN_POWER_MASK)
            emit GenPowerChanged(portB & B4_GEN_POWER_MASK);

        m_lastPortBState = portB;
    }

/*
    changed = (portA ^ m_lastPortAState) & PORTA_INPUT_MASK;
    if (changed) {
        PrintPortA();
        m_lastPortAState = portA;
    } */

}

// ===============================================================
// Simulation Engine
// ===============================================================
void Mcp23017::GpioSimulation()
{
    PollInputs();

    uint8_t portB = 0;
    if (i2c_read(addr, GPIOB, &portB) != 0)
        return;

    EvaluateFrmGen0Ilk(portB);
}

// ===============================================================
// Evaluate FRM_GEN0_ILK (Port A4)
// ===============================================================
void Mcp23017::EvaluateFrmGen0Ilk(uint8_t portB)
{
    static int cnt = 0;
    bool genPower = (portB & B4_GEN_POWER_MASK);
    bool genIlkEn = (portB & B3_GEN1_ILK_EN_MASK);

    bool outputState = !(genPower && genIlkEn);
//    if (!outputState)
//        qDebug()<<"GenIlk Set";
    if ((cnt++ % 50)==0)
    {
//        PrintPortA();
//        PrintPortB();
    }
    //SetFrmGen0Ilk(true);
//   SetFrmGen0Ilk(outputState);
}

// ===============================================================
// Helper: Write 1 bit on PORT A (read-modify-write)
// ===============================================================
static void writePortA(uint8_t addr, uint8_t mask, bool active)
{
    uint8_t portA = 0;
    if (i2c_read(addr, OLATA, &portA) != 0)
        return;

    if (active)
        portA |= mask;
    else
        portA &= ~mask;

    i2c_write(addr, OLATA, portA);
}

// ===============================================================
// Port A Output Setters/Getters
// ===============================================================
void Mcp23017::SetFrmGen0Ilk(bool active)  { writePortA(addr, A4_FRM_GEN0_ILK_MASK, active); }
bool Mcp23017::GetFrmGen0Ilk()             { return GetPortA() & A4_FRM_GEN0_ILK_MASK; }

void Mcp23017::SetDoorIlk(bool active)     { writePortA(addr, A0_DOOR_ILK_MASK, active); }
void Mcp23017::SetAirIlk(bool active)      { writePortA(addr, A1_AIR_ILK_MASK, active); }
void Mcp23017::SetVac1Ilk(bool active)     { writePortA(addr, A2_VAC1_ILK_MASK, active); }
void Mcp23017::SetVac2Ilk(bool active)     { writePortA(addr, A3_VAC2_ILK_MASK, active); }

bool Mcp23017::GetDoorIlk()                { return GetPortA() & A0_DOOR_ILK_MASK; }
bool Mcp23017::GetAirIlk()                 { return GetPortA() & A1_AIR_ILK_MASK; }
bool Mcp23017::GetVac1Ilk()                { return GetPortA() & A2_VAC1_ILK_MASK; }
bool Mcp23017::GetVac2Ilk()                { return GetPortA() & A3_VAC2_ILK_MASK; }

// ===============================================================
// Port Getters
// ===============================================================
uint8_t Mcp23017::GetPortA()
{
    uint8_t portA = 0;
    i2c_read(addr, OLATA, &portA);
    return portA;
}

uint8_t Mcp23017::GetPortB()
{
    uint8_t portB = 0;
    i2c_read(addr, GPIOB, &portB);
    return portB;
}

// ===============================================================
// Input Getters
// ===============================================================
bool Mcp23017::GetPurge()        { return GetPortB() & B0_PURGE_MASK; }
bool Mcp23017::GetIsolation()    { return GetPortB() & B1_ISOLATION_MASK; }
bool Mcp23017::GetGen1RfOn()     { return GetPortB() & B2_GEN1_RF_ON_MASK; }
bool Mcp23017::GetGen1IlkEn()    { return GetPortB() & B3_GEN1_ILK_EN_MASK; }
bool Mcp23017::GetGenPower()     { return GetPortB() & B4_GEN_POWER_MASK; }

// ===============================================================
// Input change callbacks
// ===============================================================
void Mcp23017::OnGenPowerChanged(bool active)    { qDebug() << "[GPIO] GEN_POWER changed =" << active; }
void Mcp23017::OnGen1RfOnChanged(bool active)    { qDebug() << "[GPIO] GEN1_RF_ON changed =" << active; }
void Mcp23017::OnGen1IlkEnChanged(bool active)   { qDebug() << "[GPIO] GEN1_ILK_EN changed =" << active; }
void Mcp23017::OnPurgeChanged(bool active)       { qDebug() << "[GPIO] PURGE changed =" << active; }
void Mcp23017::OnIsolationChanged(bool active)   { qDebug() << "[GPIO] ISOLATION changed =" << active; }

void Mcp23017::PrintPortA()
{
   int a =  GetPortA();
   qDebug()<<"PortA = 0x" << Qt::hex << a;
   qDebug()<<"  DOOR = "<< ((a>>A0_DOOR_ILK_BIT) & 1);
   qDebug()<<"  AIR  = "<< ((a>>A1_AIR_ILK_BIT) & 1);
   qDebug()<<"  VAC1 = "<< ((a>>A2_VAC1_ILK_BIT) & 1);
   qDebug()<<"  VAC2  = "<< ((a>>A3_VAC2_ILK_BIT) & 1);
   qDebug()<<"  GEN0ILK  = "<< ((a>>A4_FRM_GEN0_ILK_BIT) & 1);
}

void Mcp23017::PrintPortB()
{
   uint16_t b = GetPortB();
   qDebug()<<"PortB = 0x" << Qt::hex << b;
   qDebug()<<"  Purge = "<<((b >> B0_PURGE_BIT) & 1);
   qDebug()<<"  Iso   = "<<((b >> B1_ISOLATION_BIT) & 1);
   qDebug()<<"  GEN_RF_ON = "<<((b >> B2_GEN1_RF_ON_BIT) & 1);
   qDebug()<<"  GEN ILK   = "<<((b >> B3_GEN1_ILK_EN_BIT) & 1);
   qDebug()<<"  GEN PWR   = "<<((b >> B4_GEN_POWER_BIT) & 1);

}
