#ifndef MCP23017_H
#define MCP23017_H

#include <QObject>
#include <stdint.h>

// ===============================================================
// MCP23017 Register Definitions (BANK = 0)
// ===============================================================
#define IODIRA   0x00
#define IODIRB   0x01
#define IPOLA    0x02
#define IPOLB    0x03
#define IOCON    0x0A
#define GPPUA    0x0C
#define GPPUB    0x0D
#define GPIOA    0x12
#define GPIOB    0x13
#define OLATA    0x14
#define OLATB    0x15

// ===============================================================
// PORT B INPUT BIT DEFINITIONS (B0..B4 inputs)
// ===============================================================
#define B0_PURGE_BIT           0
#define B1_ISOLATION_BIT       1
#define B2_GEN1_RF_ON_BIT      2
#define B3_GEN1_ILK_EN_BIT     3
#define B4_GEN_POWER_BIT       4

#define B0_PURGE_MASK          (1u << B0_PURGE_BIT)
#define B1_ISOLATION_MASK      (1u << B1_ISOLATION_BIT)
#define B2_GEN1_RF_ON_MASK     (1u << B2_GEN1_RF_ON_BIT)
#define B3_GEN1_ILK_EN_MASK    (1u << B3_GEN1_ILK_EN_BIT)
#define B4_GEN_POWER_MASK      (1u << B4_GEN_POWER_BIT)

#define PORTB_INPUT_MASK ( \
    B0_PURGE_MASK       | \
    B1_ISOLATION_MASK   | \
    B2_GEN1_RF_ON_MASK  | \
    B3_GEN1_ILK_EN_MASK | \
    B4_GEN_POWER_MASK )

// ===============================================================
// PORT A OUTPUT BIT DEFINITIONS (updated)
// ===============================================================
#define A0_DOOR_ILK_BIT        0
#define A0_DOOR_ILK_MASK       (1u << A0_DOOR_ILK_BIT)

#define A1_AIR_ILK_BIT         1
#define A1_AIR_ILK_MASK        (1u << A1_AIR_ILK_BIT)

#define A2_VAC1_ILK_BIT        2
#define A2_VAC1_ILK_MASK       (1u << A2_VAC1_ILK_BIT)

#define A3_VAC2_ILK_BIT        3
#define A3_VAC2_ILK_MASK       (1u << A3_VAC2_ILK_BIT)

// *** ORIGINAL OUTPUT MOVED TO A4 ***
#define A4_FRM_GEN0_ILK_BIT    4
#define A4_FRM_GEN0_ILK_MASK   (1u << A4_FRM_GEN0_ILK_BIT)


// ===============================================================
// MCP23017 CLASS
// ===============================================================
class Mcp23017 : public QObject
{
    Q_OBJECT

public:
    explicit Mcp23017(uint8_t i2cAddress, QObject *parent = 0);

    int Init();
    void PollInputs();
    void PrintPortA(void);
    void PrintPortB(void);
public slots:
    void GpioSimulation();

    // OUTPUT CONTROL (Port A)
    void SetFrmGen0Ilk(bool active);
    bool GetFrmGen0Ilk();

    void SetDoorIlk(bool active);
    void SetAirIlk(bool active);
    void SetVac1Ilk(bool active);
    void SetVac2Ilk(bool active);

    bool GetDoorIlk();
    bool GetAirIlk();
    bool GetVac1Ilk();
    bool GetVac2Ilk();

    // Port getters
    uint8_t GetPortA();
    uint8_t GetPortB();

    // Individual input getters (Port B)
    bool GetPurge();
    bool GetIsolation();
    bool GetGen1RfOn();
    bool GetGen1IlkEn();
    bool GetGenPower();

    // Input change callbacks
    void OnGenPowerChanged(bool active);
    void OnGen1RfOnChanged(bool active);
    void OnGen1IlkEnChanged(bool active);
    void OnPurgeChanged(bool active);
    void OnIsolationChanged(bool active);

signals:
    void PurgeChanged(bool active);
    void IsolationChanged(bool active);
    void Gen1RfOnChanged(bool active);
    void Gen1IlkEnChanged(bool active);
    void GenPowerChanged(bool active);

private:
    uint8_t addr;
    uint8_t m_lastPortBState;

    void EvaluateFrmGen0Ilk(uint8_t portB);
};

#endif // MCP23017_H
