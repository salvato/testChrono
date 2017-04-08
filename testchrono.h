#ifndef TESTCHRONO_H
#define TESTCHRONO_H

#include <QMainWindow>
#include <QSerialPort>

namespace Ui {
class TestChrono;
}

class TestChrono : public QMainWindow
{
    Q_OBJECT

public:
    explicit TestChrono(QWidget *parent = 0);
    ~TestChrono();

public slots:
    void onSerialDataAvailable();

protected:
    int  connectToArduino();
    int  writeRequest(QByteArray requestData);

private:
    Ui::TestChrono *ui;
    QSerialPort     serialPort;
    int             baudRate;
    int             waitTimeout;
    QByteArray      responseData;
};

#endif // TESTCHRONO_H
