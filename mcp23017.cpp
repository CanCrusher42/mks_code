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

    // Connect internal signals/slots
    connect(this, SIGNAL(PurgeChanged(bool)),       this, SLOT(OnPurgeChanged(bool)));
    connect(this, SIGNAL(IsolationChanged(bool)),   this, SLOT(OnIsolationChanged(bool)));
    connect(this, SIGNAL(Gen1RfOnChanged(bool)),    this, SLOT(OnGen1RfOnChanged(bool)));
    connect(this, SIGNAL(Gen1IlkEnChanged(bool)),   this, SLOT(OnGen1IlkEnChanged(bool)));
    connect(this, SIGNAL(GenPowerChanged(bool)),    this, SLOT(OnGenPowerChanged(bool)));

    // -------------------------------------------------------------
    // Disable sequential addressing (IOCON.SEQOP = 1)
    // -------------------------------------------------------------
    result = i2c_write(addr, IOCON, 0x20);
    usleep(10);
    if (result < 0) return result;

    // -------------------------------------------------------------
    // PORT A CONFIGURATION (OUTPUTS)
    // -------------------------------------------------------------
    result += i2c_write(addr, IODIRA, 0x00);   // all outputs
    result += i2c_write(addr, IPOLA,  0x00);
    result += i2c_write(addr, GPPUA,  0x00);
    result += i2c_write(addr, OLATA,  0x00);
    if (result < 0) return result;

    // -------------------------------------------------------------
    // PORT B CONFIGURATION (INPUTS B0..B4)
    // -------------------------------------------------------------
    result += i2c_write(addr, IODIRB, 0x1F);               // B0..B4 inputs
    result += i2c_write(addr, IPOLB, PORTB_INPUT_MASK);    // invert active low → active high
    result += i2c_write(addr, GPPUB, PORTB_INPUT_MASK);    // weak pullups
    result += i2c_write(addr, OLATB, 0x00);
    if (result < 0) return result;

    // Initialize last state
    uint8_t initial = 0;
    i2c_read(addr, GPIOB, &initial);
    m_lastPortBState = initial;

    qDebug() << "MCP23017 Init, PortB initial =" << QString::number(m_lastPortBState, 16);

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

    uint8_t changed = portB ^ m_lastPortBState;
    if (!changed)
        return;

    qDebug() << "PortB changed: new ="
             << QString::number(portB, 16)
             << "old ="
             << QString::number(m_lastPortBState, 16);

    // Bit 0 — PURGE
    if (changed & B0_PURGE_MASK)
        emit PurgeChanged(portB & B0_PURGE_MASK);

    // Bit 1 — ISOLATION
    if (changed & B1_ISOLATION_MASK)
        emit IsolationChanged(portB & B1_ISOLATION_MASK);

    // Bit 2 — GEN1_RF_ON
    if (changed & B2_GEN1_RF_ON_MASK)
        emit Gen1RfOnChanged(portB & B2_GEN1_RF_ON_MASK);

    // Bit 3 — GEN1_ILK_EN
    if (changed & B3_GEN1_ILK_EN_MASK)
        emit Gen1IlkEnChanged(portB & B3_GEN1_ILK_EN_MASK);

    // Bit 4 — GEN_POWER
    if (changed & B4_GEN_POWER_MASK)
        emit GenPowerChanged(portB & B4_GEN_POWER_MASK);

    m_lastPortBState = portB;
}


// ===============================================================
// GPIO Simulation Engine
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
// Simulation Logic for FRM_GEN0_ILK (Port A bit 0)
// ===============================================================
void Mcp23017::EvaluateFrmGen0Ilk(uint8_t portB)
{
    bool genPower = (portB & B4_GEN_POWER_MASK);
    bool genIlkEn = (portB & B3_GEN1_ILK_EN_MASK);

    bool outputState = (genPower && genIlkEn);

  //  qDebug() << "[SIM] GEN_POWER=" << genPower
  //           << "ILK_EN=" << genIlkEn
  //           << "→ FRM_GEN0_ILK =" << outputState;

    SetFrmGen0Ilk(outputState);
}


// ===============================================================
// Port A Output Control (read-modify-write)
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
// Individual Input Getters (Port B)
// ===============================================================
bool Mcp23017::GetPurge()           { return GetPortB() & B0_PURGE_MASK; }
bool Mcp23017::GetIsolation()       { return GetPortB() & B1_ISOLATION_MASK; }
bool Mcp23017::GetGen1RfOn()        { return GetPortB() & B2_GEN1_RF_ON_MASK; }
bool Mcp23017::GetGen1IlkEn()       { return GetPortB() & B3_GEN1_ILK_EN_MASK; }
bool Mcp23017::GetGenPower()        { return GetPortB() & B4_GEN_POWER_MASK; }


// ===============================================================
// Slots for Input Change Events
// ===============================================================
void Mcp23017::OnGenPowerChanged(bool active)
{
    qDebug() << "[GPIO] GEN_POWER changed =" << active;
}

void Mcp23017::OnGen1RfOnChanged(bool active)
{
    qDebug() << "[GPIO] GEN1_RF_ON changed =" << active;
}

void Mcp23017::OnGen1IlkEnChanged(bool active)
{
    qDebug() << "[GPIO] GEN1_ILK_EN changed =" << active;
}

void Mcp23017::OnPurgeChanged(bool active)
{
    qDebug() << "[GPIO] PURGE changed =" << active;
}

void Mcp23017::OnIsolationChanged(bool active)
{
    qDebug() << "[GPIO] ISOLATION changed =" << active;
}
