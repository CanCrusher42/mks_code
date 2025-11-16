#ifndef MKS_PID_H
#define MKS_PID_H

#include <QObject>
#include "serialport.h"

struct mks_struct
{
    float angle;
    float lead;
    float gain;
    float pressure;
    float setPressure;
    float setAngle;
    bool pidEnabled;
    int  loopCount;
};

struct _simPid {
    double N0Delta; // Initial quantity
    double N0Start;

    double lambda = 0.1; // Decay constant
    double t_max = 50.0; // Maximum time
    double dt = 1.0;     // Time step
    double N; // Quantity at time t
    double t; // Current time
    int   direction;
    float angle;
};

class mks_pid : public QObject
{
    Q_OBJECT
public:
    explicit mks_pid(QObject *parent = nullptr);
    void Open(void);
    void initValues(void) ;

    bool ProcessNewCommand(QString command);
    bool ProcessNewIdealCommand(QString command);
    int ProcessReponseRequest(int reqResponse);
    bool IsRespQueueEmpty();
    QString ReadResponseQueue();
    float GetSlopeAtPressureP1(float pres, float ang);
    float GetSlopeAtPressureP2(float pres, float ang);
    float GetSlopeAtPressureP3(float pres, float ang);




    int Configure23017(void);
    void Poll23017Inputs(void);   // call this from your other routine

private:
    uint8_t m_lastPortBState = 0;

    void OnGenPowerChanged(bool active);
    void OnGen1RfOnChanged(bool active);
    void OnGen1IlkEnChanged(bool active);
    void OnPurgeChanged(bool active);
    void OnIsolationChanged(bool active);


signals:
    void AngleChanged(double);



public slots:
    void Write();
    void UpdateDac(void);
    void ProcessNewTvsCommand();
    void UpdatePressure();
    void AddCommand(QString cmd);
    void UpdateSimulation();
signals:

private:
        SerialPort *sp;
        QString commandBuffer;
        QList<QString> cmdQueue;
        QList<QString> rspQueue;
        QString response;
        struct mks_struct mks_setting;
        struct _simPid simPid;
};

#endif // MKS_PID_H
