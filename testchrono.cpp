#include "testchrono.h"
#include "ui_testchrono.h"
#include <QSerialPortInfo>
#include <QDebug>
#include <QThread>
#include <QTime>

#define ACK char(255)
#define AreYouThere 0xAA

TestChrono::TestChrono(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TestChrono)
{
    ui->setupUi(this);

    // Arduino Serial Port
    baudRate = 115200;
    waitTimeout = 1000;

    if(connectToArduino()) {
      qDebug() << QString("No Arduino ready to use !");
    }
    else {
        connect(&serialPort, SIGNAL(readyRead()),
                this, SLOT(onSerialDataAvailable()));
    }
}


TestChrono::~TestChrono() {
    if(serialPort.isOpen()) serialPort.close();
    delete ui;
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
    QByteArray    requestData;
    for(int i=0; i<serialPorts.size()&& !found; i++) {
        info = serialPorts.at(i);
        if(!info.portName().contains("tty")) continue;
        serialPort.setPortName(info.portName());
        if(serialPort.isOpen()) continue;
        serialPort.setBaudRate(115200);
        if(serialPort.open(QIODevice::ReadWrite)) {
            requestData = QByteArray(2, char(AreYouThere));
            QThread::sleep(2);// Give time to the Arduino to set up the serial comunication
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
                responseData += serialPort.readAll();
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
    responseData = serialPort.readAll();
    while(!serialPort.atEnd()) {
        responseData += serialPort.readAll();
    }
    ui->editTime->setText(QString(responseData));
}
