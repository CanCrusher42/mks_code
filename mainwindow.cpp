#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QSerialPortInfo>

#define VERITY
#define TVC
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

 #ifdef TVC
    tvcTimer= new QTimer(this);
    simTimer = new QTimer(this);
    mcpTimer = new QTimer(this);
    mks = new mks_pid(this);
    gpio = new Mcp23017(0x20<<0, this);

#endif
#ifdef VERITY
    verTimer = new QTimer(this);
    ver = new VeritySim(this);
#endif

#define CHAMBER
#ifdef CHAMBER
    chamber = new VacuumChamber(this,gpio);

    // optional: initialize
    chamber->setStartPressure_Torr(700);
    chamber->setIsolationValve(gpio->GetIsolation());
    chamber->setPurge(gpio->GetPurge());
    chamber->setValveAngle(90);
    chamber->setSpeed(1.0);

    // Create and start the 200ms simulation timer
    chamberTimer = new QTimer(this);
    connect(chamberTimer, SIGNAL(timeout()), chamber,  SLOT(update()));
    chamberTimer->start(200);
#endif

#ifdef TVC
    connect(tvcTimer, SIGNAL(timeout()), mks, SLOT(ProcessNewTvsCommand()));
    connect(simTimer, SIGNAL(timeout()), mks, SLOT(UpdateSimulation()));
    connect(mcpTimer, SIGNAL(timeout()), gpio, SLOT(GpioSimulation()));

    connect(mks,SIGNAL(AngleChanged(double)), chamber, SLOT(setValveAngle(double)));
    connect(gpio, SIGNAL(PurgeChanged(bool)), chamber, SLOT(setPurge(bool)));
    connect(gpio, SIGNAL(IsolationChanged(bool)), chamber, SLOT(setIsolationValve(bool)));
    connect(chamber, SIGNAL(pressureChanged_Torr(double)), mks, SLOT(onUpdatePressure(double)));

    connect (chamber, SIGNAL(updateVac1Hp(bool)), gpio, SLOT(SetVac1Ilk(bool)));
    connect (chamber, SIGNAL(updateVac2Lp(bool)), gpio, SLOT(SetVac2Ilk(bool)));
//    connect(this, SIGNAL(GenPowerChanged(bool)), this, SLOT(OnGenPowerChanged(bool)));
//    connect(this, SIGNAL(Gen1RfOnChanged(bool)), this, SLOT(OnGen1RfOnChanged(bool)));
//    connect(this, SIGNAL(Gen1IlkEnChanged(bool)), this, SLOT(OnGen1IlkEnChanged(bool)));



    simTimer->start(200);
    tvcTimer->start(100);
    mcpTimer->start(150);
#endif
#ifdef VERITY
    connect(verTimer, SIGNAL(timeout()), ver, SLOT(VerityCheck()));
    verTimer->start(250);
#endif
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::myfunction()
{
    mks->Write();

    qDebug() << "update"    ;
}

/*
bool MainWindow::RunVerity()
{
    //ver = new VeritySim(this);
//    while(1)
 //   {
//        ver->VerityCheck();
//    }
    return true;
}*/

bool MainWindow::Test1()
{
QString response;
QString expected = "AAAAA";
int result = 0;


//    mks->AddCommand(QString("P 020.3"));
 //   mks->AddCommand(QString("R6"));

    while (1)
    {   mks->ProcessNewTvsCommand();
        QCoreApplication::processEvents() ;

    }
    while(mks->IsRespQueueEmpty())
    {
        qDebug("Queue Empty");
        QCoreApplication::processEvents() ;
    };
    response = mks->ReadResponseQueue();
    qDebug()<<"Reponse = "<<response;
    expected = "AAAAA";
    result += response.compare(expected);
    Q_ASSERT(response.compare(expected)==0);
    return true;
}

/*
    qDebug()<<"QSerial Ports";
    const auto serialPortInfos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &portInfo : serialPortInfos) {
            qDebug() << "\n"
                     << "Port:" << portInfo.portName() << "\n"
                     << "Location:" << portInfo.systemLocation() << "\n"
                     << "Description:" << portInfo.description() << "\n"
                     << "Manufacturer:" << portInfo.manufacturer() << "\n"
                     << "Serial number:" << portInfo.serialNumber() << "\n"
                     << "Vendor Identifier:"
                     << (portInfo.hasVendorIdentifier()
                         ? QByteArray::number(portInfo.vendorIdentifier(), 16)
                         : QByteArray()) << "\n"
                     << "Product Identifier:"
                     << (portInfo.hasProductIdentifier()
                         ? QByteArray::number(portInfo.productIdentifier(), 16)
                         : QByteArray());
        }
*/
