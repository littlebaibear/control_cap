#ifndef PGRCAM_H
#define PGRCAM_H

#include <QObject>
#include<QDebug>
#include <QThread>
#include <QTimer>
#include <QTime>
#include <QImage>
#include <QMutex>
#include<FlyCapture2.h>
#include<stdio.h>
using namespace FlyCapture2;

class PGRCam : public QObject
{
    Q_OBJECT
public:
    explicit PGRCam(QObject *parent = nullptr);
    //
    // variables
    //
    QMutex mutex;
    QImage grayImg;
    Image convertImg;

signals:
    void camError();
    void frameReady(Image img);
private:
    BusManager busMgr;
    PGRGuid guid;//用来存放相机地址
    Error error;
    Camera cam;//相机实例
    CameraInfo camInfo;
    Image rawImg;//相机获取的原始图像
    QThread *camThread;
    QTimer *camTimer;

    void printError(Error e);
    void printCameraInfo(CameraInfo *pCamInfo);
//    void printBuildInfo();

    void printProperty(PropertyType propType);
public slots:
    void openCam();
    void getFrame();
    void closeCam();
    void setCamProperty(PropertyType propType, double value);
};

#endif // PGRCAM_H
