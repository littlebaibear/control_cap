#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
//file operation
#include <QDir>
#include <QDateTime>
//#include <QThread>
#include <QDebug>
#include <QTimer>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QDir>
#include <QDateTime>
#include <QTime>
#include <QImage>
#include <QThread>
#include <QMutex>
//#include <QCoreApplication>
/*****thirdparty******/
//#include <FlyCapture2.h>
#include "serialwriteread.h"
#include "pgrcam.h"

//using namespace FlyCapture2;
namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void sysInit(); //界面初始化

private slots:
    void slotResetUi();//接收串口可用信号后重置界面状态
signals:
    void subValueReady(int subValue);
    void sub2ValueReady(int subValue);
    void mainValueReady(int mainValue);
    void main2ValueReady(int mainValue);
    void camOpen(bool openCam);
    void startMotor(double period, int dutuRate, bool stepMove, bool forwardReverse, int pulseNum);
    void stopMotor();
    void setLight(int mainValue, int subValue);
    void getCurrentFrame();
    void lightReady();
    void signalOpenCam();
    void signalCloseCam();
    void signalGetFrame();
    void glassReady(bool glassDetected);
private:
    //
    // variabel
    //
    Ui::MainWindow *ui;
//    QSerialPort global_port;//创建全局的串口对象
//    QTimer *serialTimer, *camTimer, *uiTimer;
    bool firstReset = true;
    Image srcImg;
    bool objectReadyIn = true;
    bool objectNotIn = true;
    bool objectReadyLeave = false;
    int glassId = 0;
    int picsId = 1;
    bool latestCheck = false;
    bool lastCheck = false;
    bool dispLastGlassId = true;
    QMutex mutex;

    //
    // function
    //
    void saveImg(QImage &Img, QString &str);
    void connectSerailCtrl(SerialWriteRead *serial);
    void connectCamCtrl(PGRCam *pgrcam);
    void lightModeSet(int mainvalue, int main2value, int subvalue);
    void Delay_MSec(unsigned int msec);
    void mkFilePath(QString dir);
    const char* mkImgSaveName(QString imgDir);
    void autoDetect(SerialWriteRead *serial, PGRCam *pgrcam, bool glassDetected);
    void lightSetAndCap(PGRCam* pgrcam);
    const char *lightSetAndOneCap(PGRCam* pgrcam, int delayMSec, int mainValue, int main2value, int subValue, const char *lightModeChar);
};

#endif // MAINWINDOW_H
