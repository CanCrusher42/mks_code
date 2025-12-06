#include "mcp23017.h"
#include "vacuumchamber.h"
#include <cmath>

// Physical limits
static const double P_MAX = 700000.0;   // 700 Torr in mT
static const double P_MIN = 50.0;       // 0.05 Torr in mT

// Vacuum interlock thresholds
static const double ILK1_THRESHOLD_mT = 600000.0; // < 600 Torr
static const double ILK2_THRESHOLD_mT = 100000.0; // < 100 Torr

// Fixed timestep for simulation: 200 ms
static const double FIXED_DT = 0.2;

// -----------------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------------
VacuumChamber::VacuumChamber(QObject *parent, Mcp23017 *gpio)
    : QObject(parent),
      m_gpio(gpio),
      m_pressure_mT(P_MAX),
      m_valveAngle(10.0),
      m_isoValveOpen(false),
      m_speed(2.0),
      m_lowPressureFactor(1.0),
      m_purge(false),
      m_purgeRate_mTps(50000.0),
      m_leakFactor(0.08)   // <-- NEW (5% default leak)

{
}

// -----------------------------------------------------------------------------
// Interlock getters
// -----------------------------------------------------------------------------
bool VacuumChamber::vacuumInterlock1() const
{
    return (m_pressure_mT < ILK1_THRESHOLD_mT);
}

bool VacuumChamber::vacuumInterlock2() const
{
    return (m_pressure_mT < ILK2_THRESHOLD_mT);
}

// -----------------------------------------------------------------------------
// Initialization Setters
// -----------------------------------------------------------------------------
void VacuumChamber::setStartPressure_mT(double mT)
{
    m_pressure_mT = clampd(mT, P_MIN, P_MAX);
}

void VacuumChamber::setStartPressure_Torr(double torr)
{
    m_pressure_mT = clampd(torr * 1000.0, P_MIN, P_MAX);
}

// -----------------------------------------------------------------------------
// Setters
// -----------------------------------------------------------------------------
void VacuumChamber::setValveAngle(double angleDeg)
{
    m_valveAngle = clampd(angleDeg, 0.0, 90.0);
   // qDebug()<<"New Chamber Angle"<<m_valveAngle;
}

void VacuumChamber::setIsolationValve(bool open)
{
    m_isoValveOpen = open;
    qDebug()<<"Iso Open: "<<m_isoValveOpen;
}

void VacuumChamber::setPurge(bool enabled)
{
    m_purge = enabled;
    qDebug()<<"Chamber Pruge Enable: "<<m_purge;
}

void VacuumChamber::setSpeed(double speed)
{
    if (speed <= 0.0)
        speed = 1.0;

    m_speed = speed;
}

void VacuumChamber::setLowPressureFactor(double factor)
{
    m_lowPressureFactor = clampd(factor, 0.1, 5.0);
}

void VacuumChamber::setPurgeRate(double mT_per_sec)
{
    if (mT_per_sec < 0)
        mT_per_sec = 0;

    m_purgeRate_mTps = mT_per_sec;
}

void VacuumChamber::setLeakFactor(double factor)   // <-- NEW
{
    if (factor < 0.0) factor = 0.0;
    if (factor > 1.0) factor = 1.0;
    m_leakFactor = factor;
}

// -----------------------------------------------------------------------------
// Getters
// -----------------------------------------------------------------------------
double VacuumChamber::currentPressure_mT() const
{
    return m_pressure_mT;
}

double VacuumChamber::currentPressure_Torr() const
{
    return m_pressure_mT / 1000.0;
}

double VacuumChamber::pressureVolts() const
{
    double torr = currentPressure_Torr();
    return clampd(torr, 0.0, 10.0);   // 10V = 10 Torr max
}

// -----------------------------------------------------------------------------
// 200ms Simulation Update (dt = 0.2 seconds)
// -----------------------------------------------------------------------------
void VacuumChamber::update()
{
    const double dt = FIXED_DT;

    if (!m_isoValveOpen)
    {
        // ---------------------------------------------------------
        // LEAK-BACK STILL OCCURS even with isolation closed
        // ---------------------------------------------------------
        double fullDropRate = (P_MAX - P_MIN) / 60.0;  // matches pump model
        double leakRate = fullDropRate * m_leakFactor;

        m_pressure_mT += leakRate * dt;
        m_pressure_mT = clampd(m_pressure_mT, P_MIN, P_MAX);



        emit pressureChanged_mT(m_pressure_mT);
        emit pressureChanged_Torr(currentPressure_Torr());
        emit pressureChanged_Volts(pressureVolts());

        return;
    }

    // -------------------------------------------------------------------------
    // PURGE MODE → pressure increases
    // -------------------------------------------------------------------------
    if (m_purge)
    {
        qDebug()<<"Updating Prs with PURGE";
        m_pressure_mT += (m_purgeRate_mTps * dt);

        // --- LEAK-BACK ALSO APPLIES HERE ---
        double fullDropRate = (P_MAX - P_MIN) / 60.0;
        double leakRate = fullDropRate * m_leakFactor;
        m_pressure_mT += leakRate * dt;

        m_pressure_mT = clampd(m_pressure_mT, P_MIN, P_MAX);

        emit pressureChanged_mT(m_pressure_mT);
        emit pressureChanged_Torr(currentPressure_Torr());
        emit pressureChanged_Volts(pressureVolts());
        return;
    }

    // -------------------------------------------------------------------------
    // PUMP-DOWN MODE → pressure decreases
    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------
    // PUMP-DOWN MODE → pressure decreases
    // -------------------------------------------------------------------------
    double valveFactor =
            clampd((m_valveAngle - 10.0) / (90.0 - 10.0), 0.0, 1.0);

    if (valveFactor > 0.0)
    {
        double pressureNorm =
            clampd((m_pressure_mT - P_MIN) / (P_MAX - P_MIN), 0.0, 1.0);

        double pressureFactor =
            0.1 + pressureNorm * m_lowPressureFactor;

        const double baseTime = 60.0;
        double fullDropRate = (P_MAX - P_MIN) / baseTime;

        double effectiveRate =
            fullDropRate *
            valveFactor *
            m_speed *
            pressureFactor;

        // Faster pump down from atmosphere
        if (m_pressure_mT > 5000.0)
            effectiveRate *= 4.0;

        // --- Extra inertia below 1 Torr (1000 mT) ---
        if (m_pressure_mT < 1000.0)
        {
            double lowPScale =
                clampd((m_pressure_mT - P_MIN) / (1000.0 - P_MIN), 0.0, 1.0);

            // 1.0 at 1T, down to ~0.25 near base pressure
            double inertiaScale = 0.25 + 0.75 * lowPScale;
            effectiveRate *= inertiaScale;
        }

        m_pressure_mT -= effectiveRate * dt;
    }


    // -------------------------------------------------------------------------
    // LEAK-BACK ALWAYS ACTIVE
    // -------------------------------------------------------------------------
    double fullDropRate = (P_MAX - P_MIN) / 60.0;

    double leakRate = fullDropRate * m_leakFactor;
    // --- Extra inertia below 1 Torr (1000 mT) ---
    if (m_pressure_mT < 1000.0)
    {
        leakRate *= 0.02;
    }
    m_pressure_mT += leakRate * dt;

    m_pressure_mT = clampd(m_pressure_mT, P_MIN, P_MAX);

   // qDebug()<<"P="<<m_pressure_mT<<"  "<<currentPressure_Torr();

    emit pressureChanged_mT(m_pressure_mT);
    emit pressureChanged_Torr(currentPressure_Torr());
    emit pressureChanged_Volts(pressureVolts());

    m_gpio->SetVac1Ilk(currentPressure_Torr()< 600);
    m_gpio->SetVac2Ilk(currentPressure_Torr()< 200);

}
