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
    void closeEvent(QCloseEvent *event);

public slots:
    void onSerialDataAvailable();

protected:
    int  connectToArduino();
    int  writeRequest(QByteArray requestData);

private slots:
    void on_startButton_clicked();
    void on_stopButton_clicked();

    void on_newPeriodButton_clicked();

private:
    Ui::TestChrono        *ui;
    QSerialPort            serialPort;
    QSerialPort::BaudRate  baudRate;
    int                    waitTimeout;
    QByteArray             responseData;
};

#endif // TESTCHRONO_H
