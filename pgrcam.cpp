#include "pgrcam.h"

PGRCam::PGRCam(QObject *parent) : QObject(parent)
{
    camThread = new QThread;
    camTimer = new QTimer;
    this->moveToThread(camThread);
    camTimer->moveToThread(camThread);
    camThread->start();
//    connect(camTimer, &QTimer::timeout, this, &PGRCam::getFrame);
}
void PGRCam::openCam()
{
//    printBuildInfo();
    unsigned int numCameras;
    error = busMgr.GetNumOfCameras(&numCameras);
    if (error != PGRERROR_OK) {
        printError(error);
        return ;
    }
    if (numCameras < 1) {
        qDebug() << "No camera detected." << endl;
//        return;
    } else {
        qDebug() << "Number of cameras detected: " << numCameras << endl;
    }
    error = busMgr.GetCameraFromIndex(0, &guid);
    if (error != PGRERROR_OK) {
        printError(error);
        return ;
    }
    qDebug() << "Running the first camera." << endl;
    const int k_numImages = 100;
    error = cam.Connect(&guid);
    if (error != PGRERROR_OK) {
        printError(error);
        return ;
    }
    error = cam.GetCameraInfo(&camInfo);
    if (error != PGRERROR_OK) {
        printError(error);
        return ;
    }
    printCameraInfo(&camInfo);
    qDebug() << "Starting capture..." << endl;
    error = cam.StartCapture();
    if (error != PGRERROR_OK) {
        printError(error);
        return ;
    }
    printProperty(FRAME_RATE);
    printProperty(SHUTTER);
    camTimer->start(130);
}
void PGRCam::getFrame()
{
    QTime timer;
    timer.start();
    //
    error = cam.RetrieveBuffer(&rawImg);
    if (error != PGRERROR_OK) {
        qDebug() << "Error grabbing image "  << endl;
        printError(error);
        return;
    } else {
        qDebug() << "Grabbed image "  << endl;
    }
    mutex.lock();
    error = rawImg.Convert(PIXEL_FORMAT_RGB, &convertImg); //转换成8位图片
    if (error != PGRERROR_OK) {
        qDebug() << "Convert image "  << endl;
        printError(error);
        return;
    }
    mutex.unlock();
//    unsigned char* data = convertImg.GetData();
//    int width = convertImg.GetCols();
//    int height = convertImg.GetRows();
//    int bytePerLine = width;
//    grayImg = QImage(data, width, height, bytePerLine, QImage::Format_Grayscale8);
    qDebug() << "cap one frame spend time:" << timer.elapsed() << "ms" << endl;
//    emit frameReady(convertImg);
}
void PGRCam::closeCam()
{
    camTimer->stop();
    qDebug() << "stop cap and release cam "  << endl;
    error = cam.StopCapture();
    if (error != PGRERROR_OK) {
        printError(error);
        return;
    }
    error = cam.Disconnect();
    if (error != PGRERROR_OK) {
        printError(error);
        return;
    }
}

void PGRCam::printError(Error e)
{
    e.PrintErrorTrace();
    camTimer->stop();
    emit camError();
}
void PGRCam::printCameraInfo(CameraInfo* pCamInfo)
{
    qDebug() << endl;
    qDebug() << "*** CAMERA INFORMATION ***" << endl;
    qDebug() << "Serial number -" << pCamInfo->serialNumber << endl;
    qDebug() << "Camera model - " << pCamInfo->modelName << endl;
    qDebug() << "Camera vendor - " << pCamInfo->vendorName << endl;
    qDebug() << "Sensor - " << pCamInfo->sensorInfo << endl;
    qDebug() << "Resolution - " << pCamInfo->sensorResolution << endl;
    qDebug() << "Firmware version - " << pCamInfo->firmwareVersion << endl;
    qDebug() << "Firmware build time - " << pCamInfo->firmwareBuildTime << endl << endl;
}
void PGRCam::printProperty(PropertyType propType = FRAME_RATE)
{
    PropertyInfo propInfo;
    propInfo.type = propType;
    error = cam.GetPropertyInfo( &propInfo );
    if (error != PGRERROR_OK) {
        error.PrintErrorTrace();
        qDebug() << propType << "not supported!";
        return ;
    }
    if ( propInfo.present == true ) {
        // Get the frame rate
        Property prop;
        prop.type = propType;
        error = cam.GetProperty( &prop );
        if (error != PGRERROR_OK) {
            error.PrintErrorTrace();
            qDebug() << propType << "not supported!";
        } else {
            // Set the frame rate.
            // Note that the actual recording frame rate may be slower,
            // depending on the bus speed and disk writing speed.
            qDebug() << propType << "is supported!";
            qDebug() << "prop value is " << prop.absValue << endl;
        }
    }
}

void PGRCam::setCamProperty(PropertyType propType, double value)
{
    PropertyInfo propInfo;
    propInfo.type = propType;
    error = cam.GetPropertyInfo( &propInfo );
    if (error != PGRERROR_OK) {
        error.PrintErrorTrace();
        qDebug() << propType << "not supported!";
        return ;
    }
    if ( propInfo.present == true ) {
        // set the frame rate
        Property prop;
        prop.type = propType;
        error = cam.GetProperty( &prop );
        if (error != PGRERROR_OK) {
            error.PrintErrorTrace();
            qDebug() << propType << "not supported!";
            return;
        }
        prop.autoManualMode = false;
        prop.absControl = true;
        prop.absValue = value;
        error = cam.SetProperty(&prop);
        if (error != PGRERROR_OK) {
            error.PrintErrorTrace();
            qDebug() << propType << "set failed";
            return;
        } else {
            qDebug() << propType << "current value is " << prop.absValue << endl;
        }
    }
}
