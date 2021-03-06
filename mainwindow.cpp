#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    serialSelect = ui->comboBoxSerialSelect;

    serial = new SerialPort(this);


    motor1 = new QDCWidget(this);
    motor2 = new QDCWidget(this);
    motor3 = new QDCWidget(this);

    motor1->setEmergencyShortcutButton(QKeySequence(Qt::Key_1));
    motor2->setEmergencyShortcutButton(QKeySequence(Qt::Key_2));
    motor3->setEmergencyShortcutButton(QKeySequence(Qt::Key_3));

    model1 = new QDCModel("mot1", "h1", "cu1", motor1, this);
    model2 = new QDCModel("mot2", "h2", "cu2", motor2, this);
    model3 = new QDCModel("mot3", "h3", "cu3", motor3, this);

    timer = new QTimer(this);
    timer->setInterval(TIMER_INTERVAL);

    motors.append(motor1);
    motors.append(motor2);
    motors.append(motor3);

    models.append(model1);
    models.append(model2);
    models.append(model3);

    ui->verticalLayout->addWidget(motor1);
    ui->verticalLayout->addWidget(motor2);
    ui->verticalLayout->addWidget(motor3);

    motor1->setGroupLabel(tr("Motor 1"));
    motor2->setGroupLabel(tr("Motor 2"));
    motor3->setGroupLabel(tr("Motor 3"));

    scan_for_serial_devices();
    plumbing();
}

MainWindow::~MainWindow()
{
    delete timer;

    models.clear();

    if(serial->isOpen()) {
        serial->close();
    }
    delete serial;

    delete motor3;
    delete motor2;
    delete motor1;
    delete ui;
}

void MainWindow::onLineRead(QByteArray data)
{
    for(auto model : models)
        model->updateData(data);

    const auto jsonDoc = QJsonDocument::fromJson(data);
    const auto jsonObj = jsonDoc.object();
    const auto driver1Error = jsonObj.value("err1").toInt();
    const auto driver2Error = jsonObj.value("err2").toInt();
    if(driver1Error) informStatusBar(tr("Driver 1 experienced an error"));
    if(driver2Error) informStatusBar(tr("Driver 2 experienced an error"));
}

void MainWindow::onConnectionClosedSerialError(QSerialPort::SerialPortError e, const QString &stringError)
{
    qDebug() << e << stringError;
    set_connection_state(false);
    scan_for_serial_devices();
}

void MainWindow::update_motor_driver()
{
    const int mot1 = motor1->getPwmValue();
    const int mot2 = motor2->getPwmValue();
    const int mot3 = motor3->getPwmValue();

    const auto cmd = QString(R"({"mot1":%1,"mot2":%2,"mot3":%3})").arg(mot1)
            .arg(mot2).arg(mot3);
    const auto dataCmd = cmd.toLatin1();

    serial->write(dataCmd + "\n");
}

void MainWindow::informStatusBar(const QString &msg)
{
    constexpr int TIMEOUT = 3000;
    ui->statusBar->showMessage(msg, TIMEOUT);
}

void MainWindow::on_action_Exit_triggered()
{
    QApplication::quit();
}


void MainWindow::on_pushButtonConnect_clicked()
{
    if(!isConnected)
    {
        auto portName = serialSelect->currentText();
        serial->setPortName(portName);

        if(serial->open())
            set_connection_state(true);
    }
    else
    {
        set_connection_state(false);
        serial->close();
    }
}

void MainWindow::on_pushButtonReload_clicked()
{
    if(isConnected) return;

    scan_for_serial_devices();
}

void MainWindow::plumbing()
{
    connect(serial, SIGNAL(lineRead(QByteArray)), this, SLOT(onLineRead(QByteArray)));
    connect(serial, SIGNAL(connectionClosed(QSerialPort::SerialPortError,QString)), this, SLOT(onConnectionClosedSerialError(QSerialPort::SerialPortError,QString)));

    connect(timer, SIGNAL(timeout()), this, SLOT(update_motor_driver()));
}

void MainWindow::set_connection_state(bool state)
{
    isConnected = state;

    for( auto dcWidget : motors )
        dcWidget->setDefaultView();

    if(state)
    {
        ui->pushButtonConnect->setText(tr("Disco&nnect"));
        ui->pushButtonReload->setEnabled(false);

        QTimer::singleShot(2000, this, [&](){timer->start();});
    }
    else
    {
        ui->pushButtonConnect->setText(tr("Co&nnect"));
        ui->pushButtonReload->setEnabled(true);

        timer->stop();
    }
}

void MainWindow::scan_for_serial_devices()
{
    serialSelect->clear();
    for(auto port : QSerialPortInfo::availablePorts())
        serialSelect->addItem(port.portName());
}

void MainWindow::on_actionAbout_author_triggered()
{
    QString msg = "<p><h3>QDCMotorController</h3></p>"
                  "<p>Control DC motors via serial port.</p>"
                  "<p>&copy; <a href=\"mailto:krzysztof.pawel.kruk@gmail.com\">"
                  "Krzysztof Kruk</a> 2016</p>";
    QMessageBox::about(this, tr("About"), msg);
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    QMessageBox::aboutQt(this, tr("About Qt"));
}
