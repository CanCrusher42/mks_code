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
VacuumChamber::VacuumChamber(QObject *parent)
    : QObject(parent),
      m_pressure_mT(P_MAX),
      m_valveAngle(10.0),
      m_isoValveOpen(false),
      m_speed(1.0),
      m_lowPressureFactor(1.0),
      m_purge(false),
      m_purgeRate_mTps(50000.0)
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
    m_valveAngle = clampd(angleDeg, 10.0, 90.0);
    qDebug()<<"Chamber Angle"<<m_valveAngle;
}

void VacuumChamber::setIsolationValve(bool open)
{
    m_isoValveOpen = open;
}

void VacuumChamber::setPurge(bool enabled)
{
    m_purge = enabled;
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
        return;

    // -------------------------------------------------------------------------
    // PURGE MODE → pressure increases
    // -------------------------------------------------------------------------
    if (m_purge)
    {
        m_pressure_mT += (m_purgeRate_mTps * dt);
        m_pressure_mT = clampd(m_pressure_mT, P_MIN, P_MAX);

        emit pressureChanged_mT(m_pressure_mT);
        emit pressureChanged_Torr(currentPressure_Torr());
        emit pressureChanged_Volts(pressureVolts());
        return;
    }

    // -------------------------------------------------------------------------
    // PUMP-DOWN MODE → pressure decreases
    // -------------------------------------------------------------------------
    double valveFactor =
        clampd((m_valveAngle - 10.0) / (90.0 - 10.0), 0.0, 1.0);

    if (valveFactor <= 0.0)
        return;

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

    m_pressure_mT -= effectiveRate * dt;
    m_pressure_mT = clampd(m_pressure_mT, P_MIN, P_MAX);

    emit pressureChanged_mT(m_pressure_mT);
    emit pressureChanged_Torr(currentPressure_Torr());
    emit pressureChanged_Volts(pressureVolts());
}
