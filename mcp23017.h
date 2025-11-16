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

//0 Yellow   Pin 7(6)  Purge
//1 Orange   Pin 8(7)  Isolation
//2 Red      Pin 13(12)GN0_RF1 GEN_RF1
//3 Brown    Pin 16(15)GN0_ILK
//4 Black    Pin 15(14)GEN_PWER
// ===============================================================
// Port B Inputs
// ===============================================================
#define B0_PURGE_BIT            0
#define B1_ISOLATION_BIT        1
#define B2_GEN1_RF_ON_BIT       2
#define B3_GEN1_ILK_EN_BIT      3
#define B4_GEN_POWER_BIT        4

/*
#define B0_PURGE_BIT            0
#define B1_ISOLATION_BIT        1
#define B2_GEN1_RF_ON_BIT       2
#define B3_GEN1_ILK_EN_BIT      3
#define B4_GEN_POWER_BIT        4
*/


#define B0_PURGE_MASK           (1u << B0_PURGE_BIT)
#define B1_ISOLATION_MASK       (1u << B1_ISOLATION_BIT)
#define B2_GEN1_RF_ON_MASK      (1u << B2_GEN1_RF_ON_BIT)
#define B3_GEN1_ILK_EN_MASK     (1u << B3_GEN1_ILK_EN_BIT)
#define B4_GEN_POWER_MASK       (1u << B4_GEN_POWER_BIT)



#define PORTB_INPUT_MASK  ( \
    B4_GEN_POWER_MASK  |    \
    B3_GEN1_ILK_EN_MASK|    \
    B2_GEN1_RF_ON_MASK |    \
    B1_ISOLATION_MASK  |    \
    B0_PURGE_MASK)

// ===============================================================
// Port A Outputs
// ===============================================================
#define A0_FRM_GEN0_ILK_BIT    0
#define A0_FRM_GEN0_ILK_MASK   (1u << A0_FRM_GEN0_ILK_BIT)

// ===============================================================
// MCP23017 CLASS (QObject-Based)
// ===============================================================
class Mcp23017 : public QObject
{
    Q_OBJECT

public:
    explicit Mcp23017(uint8_t i2cAddress, QObject *parent = 0);

    int Init();
    void PollInputs();
public slots:
    void GpioSimulation();

    // Output control
    void SetFrmGen0Ilk(bool active);
    bool GetFrmGen0Ilk();

    void OnGenPowerChanged(bool active);
    void OnGen1RfOnChanged(bool active);

    void OnGen1IlkEnChanged(bool active);
    void OnPurgeChanged(bool active);
    void OnIsolationChanged(bool active);

signals:
    void GenPowerChanged(bool active);
    void Gen1RfOnChanged(bool active);
    void Gen1IlkEnChanged(bool active);
    void PurgeChanged(bool active);
    void IsolationChanged(bool active);

private:
    uint8_t addr;
    uint8_t m_lastPortBState;

    // Internal evaluation
    void EvaluateFrmGen0Ilk(uint8_t portB);
};

#endif
