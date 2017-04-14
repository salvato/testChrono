#include "testchrono.h"
#include "ui_testchrono.h"
#include <QSerialPortInfo>
#include <QDebug>
#include <QThread>
#include <QTime>
#include <QCloseEvent>
#include <QtMath>
#include <QMessageBox>



#define ACK char(255)
#define AreYouThere    0xAA
#define Stop           0x01
#define Start          0x02
#define NewPeriod      0x11
#define StopSending    0x81



TestChrono::TestChrono(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TestChrono)
{
    ui->setupUi(this);
    responseData.clear();

    // Arduino Serial Port
    baudRate = QSerialPort::Baud115200;
    waitTimeout = 1000;

    if(connectToArduino()) {
        QMessageBox::critical(this, "Error", "No Arduino ready to use !");
        qDebug() << QString("No Arduino ready to use !");
        ui->centralWidget->setDisabled(true);
    }
    else {
        responseData.clear();
        connect(&serialPort, SIGNAL(readyRead()),
                this, SLOT(onSerialDataAvailable()));
        on_newPeriodButton_clicked();
    }
}


TestChrono::~TestChrono() {
    if(serialPort.isOpen()) {
        serialPort.waitForBytesWritten(1000);
        serialPort.close();
    }
    delete ui;
}


void
TestChrono::closeEvent(QCloseEvent *event) {
    if(serialPort.isOpen()) {
        QByteArray requestData;
        requestData.append(char(StopSending));
        serialPort.write(requestData.append(char(127)));
    }
    event->accept();
}


int
TestChrono::connectToArduino() {
    QList<QSerialPortInfo> serialPorts = QSerialPortInfo::availablePorts();
    if(serialPorts.isEmpty()) {
        qDebug() << QString("No serial port available");
        return -1;
    }
    bool found = false;
    QSerialPortInfo info;
    QByteArray requestData;
    for(int i=0; i<serialPorts.size()&& !found; i++) {
        info = serialPorts.at(i);
        if(!info.portName().contains("tty")) continue;
        serialPort.setPortName(info.portName());
        if(serialPort.isOpen()) continue;
        serialPort.setBaudRate(115200);
        serialPort.setDataBits(QSerialPort::Data8);
        if(serialPort.open(QIODevice::ReadWrite)) {
            // Arduino will be reset upon a serial connectiom
            // so give time to set it up before communicating.
            QThread::sleep(3);
            requestData = QByteArray(2, char(AreYouThere));
            if(writeRequest(requestData) == 0) {
                found = true;
                break;
            }
            else
                serialPort.close();
        }
    }
    if(!found)
        return -1;
    qDebug() << "Arduino found at: " << info.portName();
    return 0;
}


int
TestChrono::writeRequest(QByteArray requestData) {
    if(!serialPort.isOpen()) {
        qDebug() << QString("serial port %1 has been closed")
                    .arg(serialPort.portName());
        return -1;
    }
    serialPort.write(requestData.append(char(127)));
    if (serialPort.waitForBytesWritten(waitTimeout)) {
        if (serialPort.waitForReadyRead(waitTimeout)) {
            responseData = serialPort.readAll();
            while(serialPort.waitForReadyRead(1))
                responseData.append(serialPort.readAll());
            if (responseData.at(0) != ACK) {
                QString response(responseData);
                qDebug() << tr("NACK on Command %1: expecting %2 read %3")
                            .arg(int(requestData.at(0)))
                            .arg(int(ACK))
                            .arg(int(response.at(0).toLatin1()));
            }
        }
        else {// Read timeout
            qDebug() << QString(" Wait read response timeout %1 %2")
                       .arg(QTime::currentTime().toString())
                       .arg(serialPort.portName());
            return -1;
        }
    }
    else {// Write timeout
        qDebug() << QString(" Wait write request timeout %1 %2")
                 .arg(QTime::currentTime().toString())
                 .arg(serialPort.portName());
        return -1;
    }
    return 0;
}


void
TestChrono::onSerialDataAvailable() {
    responseData.append(serialPort.readAll());
    while(!serialPort.atEnd()) {
        responseData.append(serialPort.readAll());
    }
    while(responseData.count() > 8) {
        qint32 val = 0;
        long imin, isec, icent;
        for(int i=0; i<4; i++)
            val += quint8(responseData.at(i)) << i*8;
        isec = val/100;
        icent = 10*((val - int(val/100)*100)/10);
//        icent = val - int(val/100)*100;
        QString sVal = QString("%1:%2")
                      .arg(isec, 2, 10, QLatin1Char('0'))
                      .arg(icent, 2, 10, QLatin1Char('0'));
        ui->editTimePossesso->setText(QString(sVal));

        val = 0;
        for(int i=4; i<8; i++)
            val += quint8(responseData.at(i)) << i*8;
        imin = val/6000;
        isec = (val-imin*6000)/100;
        icent = 10*((val - isec*100)/10);
        if(imin > 0) {
            sVal = QString("%1")
                   .arg(imin, 2, 10, QLatin1Char('0'));
            ui->editTimeMinPeriodo->setText(QString(sVal));
            sVal = QString("%1")
                    .arg(isec, 2, 10, QLatin1Char('0'));
            ui->editTimeSecPeriodo->setText(QString(sVal));
        }
        else {
            sVal = QString("%1")
                   .arg(isec, 2, 10, QLatin1Char('0'));
            ui->editTimeMinPeriodo->setText(QString(sVal));
            sVal = QString("%1")
                   .arg(icent, 2, 10, QLatin1Char('0'));
            ui->editTimeSecPeriodo->setText(QString(sVal));
        }
        ui->editTimeSecPeriodo->setText(QString(sVal));
        responseData.remove(0, 8);
    }
}


void
TestChrono::on_startButton_clicked() {
    QByteArray requestData;
    requestData.append(char(Start));
    requestData.append(char(24));
    serialPort.write(requestData.append(char(127)));
}


void
TestChrono::on_stopButton_clicked() {
    QByteArray requestData;
    requestData.append(char(Stop));
    serialPort.write(requestData.append(char(127)));
}


void
TestChrono::on_newPeriodButton_clicked() {
    QByteArray requestData;
    requestData.append(char(NewPeriod));
    requestData.append(char(10));// Ten minutes
//    requestData.append(char(1));// One minute for tests
    requestData.append(char(24));// 24 seconds
    serialPort.write(requestData.append(char(127)));
}
