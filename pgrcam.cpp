#include "pgrcam.h"

PGRCam::PGRCam(QObject *parent) : QObject(parent)
{
    camThread = new QThread;
    camTimer = new QTimer;
    this->moveToThread(camThread);
    camTimer->moveToThread(camThread);
    camThread->start();
    error = busMgr.GetNumOfCameras(&numCameras);
    if (error != PGRERROR_OK) {
        printError(error);
    }
//    connect(camTimer, &QTimer::timeout, this, &PGRCam::getFrame);
}
void PGRCam::openCam()
{
//    printBuildInfo();
    if (numCameras < 1) {
        qDebug() << "No camera detected." << endl;
//        return;
    } else {
        qDebug() << "Number of cameras detected: " << numCameras << endl;
    }
    cam = new  Camera*[numCameras];
    // 激活所有相机
    for (unsigned int uiCamera = 0; uiCamera < numCameras; uiCamera++) {
        cam[uiCamera] = new Camera();
        error = busMgr.GetCameraFromIndex(uiCamera, &guid);
        if (error != PGRERROR_OK) {
            printError(error);
            return ;
        }
        error = cam[uiCamera]->Connect(&guid);
        if (error != PGRERROR_OK) {
            printError(error);
            return ;
        }
        qDebug() << "******************************************";
        qDebug() << "Running the " << uiCamera << "th camera.";
//        const int k_numImages = 100;
        error = cam[uiCamera]->GetCameraInfo(&camInfo);
        if (error != PGRERROR_OK) {
            printError(error);
            return ;
        }
        printCameraInfo(&camInfo);
        qDebug() << "Starting capture..." << endl;
        qDebug() << "******************************************" << endl << endl;
        error = cam[uiCamera]->StartCapture();
        if (error != PGRERROR_OK) {
            printError(error);
            return ;
        }
    }
    camTimer->start(130);
}
void PGRCam::getFrame(unsigned int uiCamera)
{
    QTime timer;
    timer.start();
    //
    error = cam[uiCamera]->RetrieveBuffer(&rawImg);
    if (error != PGRERROR_OK) {
        qDebug() << "Error grabbing image "  << endl;
        printError(error);
        return;
    } else {
        qDebug() << "Grabbed image from the" << uiCamera << "th camera";
    }
    mutex.lock();
    error = rawImg.Convert(PIXEL_FORMAT_RGB, &convertImg); //转换成8位图片
    int imgSize = convertImg.GetDataSize();
    qDebug() << u8"camera 线程中图片的缓存大小: " << imgSize;
    if (error != PGRERROR_OK) {
        qDebug() << "Convert image fail "  << endl;
        printError(error);
        return;
    } else {
        qDebug() << "Convert image success...";
    }
    mutex.unlock();
//    unsigned char* data = convertImg.GetData();
//    int width = convertImg.GetCols();
//    int height = convertImg.GetRows();
//    int bytePerLine = width;
//    grayImg = QImage(data, width, height, bytePerLine, QImage::Format_Grayscale8);
//    error = rawImg.ReleaseBuffer();
//    if (error != PGRERROR_OK) {
//        printError(error);
//        return;
//    } else {
//        qDebug() << "Release the buffer associated with the Image...";
//    }
    qDebug() << "cap one frame spend time:" << timer.elapsed() << "ms" << endl;
//    emit frameReady(convertImg);
}
void PGRCam::closeCam()
{
    camTimer->stop();
    for (unsigned int uiCamera = 0; uiCamera < numCameras; uiCamera++) {
        qDebug() << "stop cap and release cam "  << QString::number(uiCamera) << endl;
        error = cam[uiCamera]->StopCapture();
        if (error != PGRERROR_OK) {
            printError(error);
            return;
        }
        error = cam[uiCamera]->Disconnect();
        if (error != PGRERROR_OK) {
            printError(error);
            return;
        }
        delete cam[uiCamera];
    }
    delete [] cam;
}

void PGRCam::printError(Error e)
{
    e.PrintErrorTrace();
    camTimer->stop();
    emit camError();
}
void PGRCam::printCameraInfo(CameraInfo* pCamInfo)
{
    qDebug() << "*** CAMERA INFORMATION ***" << endl;
    qDebug() << "Serial number -" << pCamInfo->serialNumber << endl;
    qDebug() << "Camera model - " << pCamInfo->modelName << endl;
    qDebug() << "Camera vendor - " << pCamInfo->vendorName << endl;
    qDebug() << "Sensor - " << pCamInfo->sensorInfo << endl;
    qDebug() << "Resolution - " << pCamInfo->sensorResolution << endl;
    qDebug() << "Firmware version - " << pCamInfo->firmwareVersion << endl;
    qDebug() << "Firmware build time - " << pCamInfo->firmwareBuildTime << endl;
}
void PGRCam::printProperty(PropertyType propType, unsigned int uiCamera)
{
    PropertyInfo propInfo;
    propInfo.type = propType;
    error = cam[uiCamera]->GetPropertyInfo( &propInfo );
    if (error != PGRERROR_OK) {
        error.PrintErrorTrace();
        qDebug() << propType << "not supported!";
        return ;
    }
    if ( propInfo.present == true ) {
        // Get the frame rate
        Property prop;
        prop.type = propType;
        error = cam[uiCamera]->GetProperty( &prop );
        if (error != PGRERROR_OK) {
            error.PrintErrorTrace();
            qDebug() << propType << "not supported!";
        } else {
            // Set the frame rate.
            // Note that the actual recording frame rate may be slower,
            // depending on the bus speed and disk writing speed.
            qDebug() << propType << "is supported!";
            qDebug() << propType << " value is " << prop.absValue << endl;
        }
    }
}

void PGRCam::setCamProperty(PropertyType propType, double value, unsigned int uiCamera)
{
    PropertyInfo propInfo;
    propInfo.type = propType;
    error = cam[uiCamera]->GetPropertyInfo( &propInfo );
    if (error != PGRERROR_OK) {
        error.PrintErrorTrace();
        qDebug() << propType << "not supported!";
        return ;
    }
    if ( propInfo.present == true ) {
        // set the frame rate
        Property prop;
        prop.type = propType;
        error = cam[uiCamera]->GetProperty( &prop );
        if (error != PGRERROR_OK) {
            error.PrintErrorTrace();
            qDebug() << propType << "not supported!";
            return;
        }
        prop.autoManualMode = false;
        prop.absControl = true;
        prop.absValue = value;
        error = cam[uiCamera]->SetProperty(&prop);
        if (error != PGRERROR_OK) {
            error.PrintErrorTrace();
            qDebug() << propType << "set failed";
            return;
        } else {
            qDebug() << propType << "current value is " << prop.absValue << endl;
        }
    }
}
