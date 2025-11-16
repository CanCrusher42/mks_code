#ifndef VACUUMCHAMBER_H
#define VACUUMCHAMBER_H
#include <QDebug>
#include <QObject>

class VacuumChamber : public QObject
{
    Q_OBJECT

public:
    explicit VacuumChamber(QObject *parent = nullptr);

    // Getters
    double currentPressure_mT() const;
    double currentPressure_Torr() const;
    double pressureVolts() const;

    // Vacuum Interlocks
    bool vacuumInterlock1() const;   // TRUE if < 600 Torr
    bool vacuumInterlock2() const;   // TRUE if < 100 Torr

public slots:
    // Setters
    void setValveAngle(double angleDeg);
    void setIsolationValve(bool open);
    void setPurge(bool enabled);
    void setSpeed(double speed);
    void setLowPressureFactor(double factor);
    void setPurgeRate(double mT_per_sec);

    // Starting pressure initialization
    void setStartPressure_mT(double mT);
    void setStartPressure_Torr(double torr);

    // 200ms simulation tick
    // Internally uses dt = 0.2 seconds
    void update();

signals:
    void pressureChanged_mT(double newPressure);
    void pressureChanged_Torr(double newPressure);
    void pressureChanged_Volts(double volts);

private:
    // Replacement for std::clamp (Qt4 safe)
    inline double clampd(double v, double lo, double hi) const
    {
        return (v < lo) ? lo : ((v > hi) ? hi : v);
    }

private:
    double m_pressure_mT;
    double m_valveAngle;
    bool   m_isoValveOpen;
    double m_speed;
    double m_lowPressureFactor;
    bool   m_purge;
    double m_purgeRate_mTps;
};

#endif // VACUUMCHAMBER_H
