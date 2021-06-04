#include "mainwindow.h"
#include "ui_mainwindow.h"
/*********custom class*********/


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowIcon(QIcon(":/imgs/contorl32.ico"));
    this->setWindowTitle("port control and cap");
    sysInit();
//    char StepPulse[2] = {20, 5};
//    unsigned short int *p = (unsigned short int *)StepPulse;
//    qDebug() << StepPulse;
    qRegisterMetaType<CameraInfo>("CameraInfo");
    qRegisterMetaType<Image>("Image");
    qRegisterMetaType<PropertyType>("PropertyType");
    qDebug() << u8"主线程对象的地址:" << QThread::currentThreadId() << endl;
    SerialWriteRead *serial = new SerialWriteRead;
    PGRCam *pgrcam = new PGRCam;
    //设置串口相关控件的信号与槽
    connectSerailCtrl(serial);
    connectCamCtrl(pgrcam);
    //设置相机相关控件的信号与槽
    //检测模式重新设置界面状态
    connect(ui->manualCheck, &QCheckBox::clicked, this, [ = ] {
        QObject::disconnect(this, &MainWindow::glassReady, this, nullptr);//取消信号与lambda槽函数
        ui->LightState->setEnabled(true);
        ui->PIRState->setEnabled(true);
        ui->MotorState->setEnabled(true);
        ui->camSetState->setEnabled(true);
//        ui->manualCapBtn->setEnabled(true);
        emit stopMotor();
        emit mainValueReady(0);
        emit subValueReady(0);
        emit main2ValueReady(0);
        emit sub2ValueReady(0);
    });
    connect(ui->autoCheck, &QCheckBox::clicked, this, [ = ] {
        ui->LightState->setEnabled(false);
        ui->MotorState->setEnabled(false);
        ui->manualCapBtn->setEnabled(false);
        emit stopMotor();
        emit mainValueReady(0);
        emit subValueReady(0);
        emit main2ValueReady(0);
        emit sub2ValueReady(0);
        emit startMotor(ui->periodSpin->value(), ui->DutyRateSpin->value(), false, true, 0);
        //在自动检测过程中设计逻辑
        connect(this, &MainWindow::glassReady, this, [ = ](bool glassDetected)
        {
            autoDetect(serial, pgrcam, glassDetected);
        });
    });
    //
}
MainWindow::~MainWindow()
{
    delete ui;
}
/*----------------functions-----------*/
void MainWindow::sysInit()
{
    ui->serialCombo->clear();
    ui->serialCombo->addItem(u8"串口选择");
    //auto 是C++11标准中的关键字,用来自动获取变量的类型
    //获取可用串口
    const auto infos = QSerialPortInfo::availablePorts();
    QStringList portList;
    for (const QSerialPortInfo &info : infos) {
        QSerialPort serial;
        serial.setPort(info);
        if  (serial.open(QIODevice::ReadWrite)) {
            ui->serialCombo->addItem(info.portName());
            serial.close();
            // portList << info.portName();//返回可用串口列表,应在主界面写
        }
    }
    int light_min = 0;
    int light_max = 4095;
    ui->MainSlider->setRange(light_min, light_max);
    //    ui->mainLabel->setText(QString::number(ui->MainSlider->value()));
    ui->SubSlider->setRange(light_min, light_max);
    ui->MainSlider2->setRange(light_min, light_max);
    ui->SubSlider2->setRange(light_min, light_max);
    ui->periodSpin->setRange(0.04, 355);
    ui->periodSpin->setSingleStep(0.05);
    ui->periodSpin->setSuffix("ms");
    ui->DutyRateSpin->setRange(0, 100);
    ui->DutyRateSpin->setSuffix("%");
    ui->shutterSpin->setRange(0.01, 23);
    ui->shutterSpin->setSuffix("ms");
    ui->gainSpin->setRange(0, 47);
    ui->gainSpin->setSuffix("dB");
    ui->frameSpin->setRange(1, 43);
    ui->frameSpin->setSuffix("fps");
    //
    ui->modeState->setEnabled(false);
    ui->LightState->setEnabled(false);
    ui->MotorState->setEnabled(false);
    ui->PIRState->setEnabled(false);
    ui->pulseSpin->setEnabled(false);
    ui->camSetState->setEnabled(false);
    ui->manualCapBtn->setEnabled(false);
}
void MainWindow::connectSerailCtrl(SerialWriteRead * serial)
{
    //点击更新串口
    //接收串口初始化信号
    connect(ui->serialCombo, &QComboBox::currentTextChanged, serial, &SerialWriteRead::slotSerialCombo); //直接进行值传递不会引发子线程调用父线程的值的错误警告
    //返回串口可用标志
    connect(serial, &SerialWriteRead::SerialAvailable, this, [ = ](bool portAvailabel) {
        if(portAvailabel) {
            //可用即设置
            ui->portStateLabel->setText("port available");
            firstReset = true;//希望在下一次读取PIR时,可以重置界面状态
        } else {
            ui->portStateLabel->setText("port unavailable");
            //            sysInit();
        }
    });
    //一直读取PIR状态并显示
    connect(serial, &SerialWriteRead::PIRReady, this, [ = ](int PIRValue, bool ctrlSuccess) {
        if (ctrlSuccess) {
            //PIR返回有效值时,重置界面
            this->slotResetUi();
            firstReset = false;//仅在第一次读到PIR正确返回时重置界面状态
            //默认返回高电平即1
            ui->checkPIR1->setChecked(!(PIRValue & 0x08));//低电平检测到物体,取反赋值给显示
            ui->checkPIR2->setChecked(!(PIRValue & 0x10));
            ui->checkPIR3->setChecked(!(PIRValue & 0x20));
            int numOfDetected = ui->checkPIR1->isChecked() + ui->checkPIR2->isChecked() + ui->checkPIR3->isChecked();
            qDebug() << "num of detected: " << numOfDetected << endl;
            glassStatus = ui->checkPIR1->isChecked()&ui->checkPIR2->isChecked()&ui->checkPIR3->isChecked();
            if(glassStatus & lastGlassStatus) {
                emit glassReady(true);
            } else {
                emit glassReady(false);
            }
            lastGlassStatus = glassStatus;//解决PIR误触发的问题
        } else {
            //            serialTimer->stop();
            //暂停串口控件与串口设置的连接,重新初始化界面后再连接,避免
            disconnect(ui->serialCombo, &QComboBox::currentTextChanged, serial, &SerialWriteRead::slotSerialCombo);
            ui->portStateLabel->setText("port availabel,but no return,please reselect");
            sysInit();
            connect(ui->serialCombo, &QComboBox::currentTextChanged, serial, &SerialWriteRead::slotSerialCombo);
            //            QMessageBox::warning(this, "warning", "串口可用,但无法控制 ");
        }
    });
    //串口可用后,响应主辅光源调整信号
    connect(this, &MainWindow::mainValueReady, serial, &SerialWriteRead::slotMainSlider);
    connect(this, &MainWindow::main2ValueReady, serial, &SerialWriteRead::slotMainSlider2);
    connect(this, &MainWindow::subValueReady, serial, &SerialWriteRead::slotSubSlider);
    connect(this, &MainWindow::sub2ValueReady, serial, &SerialWriteRead::slotSubSlider2);
    connect(ui->MainSlider, &QSlider::sliderReleased, this, [ = ] {
        ui->mainLabel->setText(QString::number(ui->MainSlider->value()));
        emit mainValueReady(ui->MainSlider->value());
    });
    connect(ui->MainSlider2, &QSlider::sliderReleased, this, [ = ] {
        ui->main2Label->setText(QString::number(ui->MainSlider2->value()));
        emit main2ValueReady(ui->MainSlider2->value());
    });
    connect(ui->SubSlider, &QSlider::sliderReleased, this, [ = ] {
        ui->subLabel->setText(QString::number(ui->SubSlider->value()));
        emit subValueReady(ui->SubSlider->value());
    });
    connect(ui->SubSlider2, &QSlider::sliderReleased, this, [ = ] {
        ui->sub2Label->setText(QString::number(ui->SubSlider2->value()));
        emit sub2ValueReady(ui->SubSlider2->value());
    });
    connect(ui->allModeBtn, &QRadioButton::clicked, this, [ = ] {
        int mainvalue = 4095, main2value = 4095, subvalue = 4095;
        //mainvalue = 4095, subvalue = 4095;
        lightModeSet(mainvalue, main2value, subvalue);
    });
    connect(ui->mainModeBtn, &QRadioButton::clicked, this, [ = ] {
        int mainvalue = 4095, main2value = 4095, subvalue = 0;
        //mainvalue = 4095, subvalue = 0;
        lightModeSet(mainvalue, main2value, subvalue);
    });
    connect(ui->subModeBtn, &QRadioButton::clicked, this, [ = ] {
        int mainvalue = 0, main2value = 0, subvalue = 4095;
        //mainvalue = 0, subvalue = 4095;
        lightModeSet(mainvalue, main2value, subvalue);
    });
    //串口可用后,响应电机控制
    connect(ui->stepMoveCheck, &QCheckBox::stateChanged, this, [ = ] {
        ui->pulseSpin->setEnabled(ui->stepMoveCheck->isChecked());
    });
    connect(this, &MainWindow::startMotor, serial, &SerialWriteRead::slotStartMotor);
    connect(this, &MainWindow::stopMotor, serial, &SerialWriteRead::slotStopMotor);
    connect(ui->forwardBtn, &QPushButton::clicked, this, [ = ]() {
        emit startMotor(ui->periodSpin->value(), ui->DutyRateSpin->value(), ui->stepMoveCheck->isChecked(), true, ui->pulseSpin->value());
    });
    connect(ui->reverseBtn, &QPushButton::clicked, this, [ = ] {
        emit startMotor(ui->periodSpin->value(), ui->DutyRateSpin->value(), ui->stepMoveCheck->isChecked(), false, ui->pulseSpin->value());
    });
    connect(ui->motorStopBtn, &QPushButton::clicked, serial, &SerialWriteRead::slotStopMotor);
    //    emit stopMotor();
}
void MainWindow::connectCamCtrl(PGRCam * pgrcam)
{
    // 主线程->cam
    connect(this, &MainWindow::signalOpenCam, pgrcam, &PGRCam::openCam);
    connect(this, &MainWindow::signalCloseCam, pgrcam, &PGRCam::closeCam);
    connect(this, &MainWindow::signalGetFrame, pgrcam, &PGRCam::getFrame);
    connect(this, &MainWindow::signalSetCamProperty, pgrcam, &PGRCam::setCamProperty);
    //cam->主线程控件
    connect(pgrcam, &PGRCam::camError, this, [ = ] {
        ui->camStateLabel->setText("wrong connect");
        ui->camSetState->setEnabled(false);
        ui->manualCapBtn->setEnabled(false);
    });
//    connect(pgrcam, &PGRCam::frameReady, this, [ = ](Image img) {
//        srcImg = img;
//        qDebug() << "frame ready";
//    });
    //主线程控件->主线程,cam
    connect(ui->shutterSpin, &QDoubleSpinBox::editingFinished, [ = ]() {
        emit signalSetCamProperty(SHUTTER, ui->shutterSpin->value());
    });
    connect(ui->gainSpin, &QDoubleSpinBox::editingFinished, [ = ]() {
        emit signalSetCamProperty(GAIN, ui->gainSpin->value());
    });
    connect(ui->frameSpin, &QDoubleSpinBox::editingFinished, [ = ]() {
        emit signalSetCamProperty(FRAME_RATE, ui->frameSpin->value());
    });
    connect(ui->openCamBtn, &QPushButton::clicked, this, [ = ] {
        if (ui->openCamBtn->text() == "open camera")
        {
            ui->openCamBtn->setText("close camera");
            ui->camStateLabel->setText("cam connected");
            ui->camSetState->setEnabled(true);
            ui->manualCapBtn->setEnabled(true);
            emit signalOpenCam();//触发相机打开
        } else
        {
            ui->openCamBtn->setText("open camera");
            ui->camStateLabel->setText("cam disconnected");
            ui->camSetState->setEnabled(false);
            ui->manualCapBtn->setEnabled(false);
            emit signalCloseCam();//触发相机关闭
        }
    });
    connect(ui->manualCapBtn, &QPushButton::clicked, this, [ = ] {
        emit signalGetFrame();
        QDateTime dateTime = QDateTime::currentDateTime(); //获取系统当前的时间
        QString date = dateTime.toString("MM-dd-hhmm");//格式化时间
        QString filePath = "./manual/" + date;
        mkFilePath(filePath);
        QDir dir(filePath);
        QString imgPath = dir.absoluteFilePath("temp.bmp");
        std::string str = imgPath.toStdString();
        const char* ch = str.c_str();
        //Error error = srcImg.Save(ch);
        mutex.lock();
        Error error = pgrcam->convertImg.Save(ch);
        if (error != PGRERROR_OK)
        {
            QMessageBox::warning(this, "warning", "save fail");
            error.PrintErrorTrace();
            return;
        }
        mutex.unlock();
        QPixmap pixmap(ch);
        QSize qSize = ui->currentPic->size();
        pixmap = pixmap.scaled(qSize, Qt::KeepAspectRatio);
        ui->currentPic->setPixmap(pixmap);
    });
}
/*----------------slots------------*/
void MainWindow::slotResetUi()
{
    if (firstReset) {
        //        ui->LightState->setEnabled(true);
        //        ui->MotorState->setEnabled(true);
        //        ui->PIRState->setEnabled(true);
        ui->modeState->setEnabled(true);
        ui->manualCheck->setChecked(false);
        ui->autoCheck->setChecked(false);
        ui->portStateLabel->setText("port available");
    } else {}
}
void MainWindow::lightModeSet(int mainvalue, int main2value, int subvalue)
{
    ui->mainLabel->setText(QString::number(mainvalue));
    emit mainValueReady(mainvalue);
    ui->main2Label->setText(QString::number(main2value));
    emit main2ValueReady(main2value);
    ui->subLabel->setText(QString::number(subvalue));
    emit subValueReady(subvalue);
    ui->sub2Label->setText(QString::number(subvalue));
    emit sub2ValueReady(subvalue);
}
void MainWindow::Delay_MSec(unsigned int msec)
{
    QEventLoop loop;//定义一个新的事件循环
    QTimer::singleShot(msec, &loop, SLOT(quit())); //创建单次定时器，槽函数为事件循环的退出函数
    loop.exec();//事件循环开始执行，程序会卡在这里，直到定时时间到，本循环被退出
}

void MainWindow::mkFilePath(QString filePath)
{
    QDir tempdir(filePath), dir;
    if (tempdir.exists()) {
        return;
    } else if (dir.mkpath(filePath)) {
        return;
    } else {
        QMessageBox::warning(this, u8"warning", u8"创建文件夹失败");
    }
}
const char*  MainWindow::mkImgSaveName(QString imgDir)
{
    QDir dir(imgDir);
    QString imgPath = dir.absoluteFilePath(QString("%1.%2").arg(QString::number(picsId)).arg("bmp"));
    std::string str = imgPath.toStdString();
    const char* ch = str.c_str();
    return ch;
}

void MainWindow::autoDetect(SerialWriteRead* serial, PGRCam* pgrcam, bool glassDetected)
{
    int delayMSec = 50;//电机停止稳定延时
    if (glassDetected) {
        if (glassId == 0) {
            glassId = 1;
        }
        serial->serialTimer->stop();//停止检测PIR状态
        qDebug() << "glass detected";
        //1.检测到glass时,停止/减速
        objectNotIn = false;
        emit stopMotor();
        int extraDelay = 300;
        int speedReduce = 1;
        int firstMovePulse = 15000;
        for (int i = 1; i <= 6; i++) {
            if(objectReadyIn) {
                //首次需要移动?mm左右使玻璃进入视场 1/250r<->4mm<->180pix
                emit startMotor(ui->periodSpin->value() * speedReduce, ui->DutyRateSpin->value(), true, true, firstMovePulse);
                Delay_MSec(extraDelay + static_cast<unsigned int>(firstMovePulse * ui->periodSpin->value() * speedReduce)); //保证移动完成,并预留信号发送时间extraDelay
                objectReadyIn = false;
                objectNotIn = true;
//                emit stopMotor();//到位后,停止
//                Delay_MSec(delayMSec);
            } else {
                Delay_MSec(extraDelay + static_cast<unsigned int>(ui->periodSpin->value()*ui->pulseSpin->value() * speedReduce));//除首次外,每次次延时设置光源并拍照
            }
            lightSetAndCap(pgrcam);
            picsId++;
            //移动?mm左右,使玻璃下一部分进入视场
            if (i != 6) {
                emit startMotor(ui->periodSpin->value() * speedReduce, ui->DutyRateSpin->value(), true, true, ui->pulseSpin->value());
                Delay_MSec(extraDelay + static_cast<unsigned int>(ui->periodSpin->value()*ui->pulseSpin->value() * speedReduce));
            }
        }
        serial->serialTimer->start(50);
//                Delay_MSec(static_cast<unsigned int>(ui->periodSpin->value()*ui->pulseSpin->value()));
    } else if(objectNotIn) {
        //物体不在时,重设光源和电机状态,定时器状态,
        lightModeSet(0, 0, 0);
        emit startMotor(ui->periodSpin->value(), ui->DutyRateSpin->value(), false, true, 0);
        objectNotIn = false;//但只触发一次光源设置和电机启动
        objectReadyIn = true;
        picsId = 1;
        glassId++;
        serial->serialTimer->start(50);
    }
}
void MainWindow::lightSetAndCap(PGRCam* pgrcam)
{
    QTime timer;
    timer.start();
    int delayMSec = 0;//图片数据传输完成延时
    //all
    const char*ch = lightSetAndOneCap(pgrcam, delayMSec, 4095, 4095, 4095, "all");
//    QPixmap pixmap(ch);
//    QSize qSize = ui->allPic->size();
//    pixmap = pixmap.scaled(qSize, Qt::KeepAspectRatio);
//    ui->allPic->setPixmap(pixmap);
    qDebug() << "one shot" << timer.elapsed() << "ms" << endl;
    //main
    ch = lightSetAndOneCap(pgrcam, delayMSec, 4095, 4095, 0, "main");
//    pixmap =  QPixmap(ch);
//    qSize = ui->mainPic->size();
//    pixmap = pixmap.scaled(qSize, Qt::KeepAspectRatio);
//    ui->mainPic->setPixmap(pixmap);
    //sub
    ch = lightSetAndOneCap(pgrcam, delayMSec, 0, 0, 4095, "sub");
//    pixmap = QPixmap(ch);
//    qSize = ui->subPic->size();
//    pixmap = pixmap.scaled(qSize, Qt::KeepAspectRatio);
//    ui->subPic->setPixmap(pixmap);
    //main1
    ch = lightSetAndOneCap(pgrcam, delayMSec, 4095, 0, 0, "main1");
    ch = lightSetAndOneCap(pgrcam, delayMSec, 0, 4095, 0, "main2");
    qDebug() << " whole shot" << timer.elapsed() << "ms" << endl;
//    lightSetAndOneCap(delayMSec,);
}
const char *MainWindow::lightSetAndOneCap(PGRCam *pgrcam, int delayMSec, int mainValue, int main2value, int subValue, const char *lightModeChar)
{
    int delayLight = 100;//光源稳定延时
    QTime timer;
    timer.start();
    lightModeSet(mainValue, main2value, subValue);
    qDebug() << u8"光源设置耗时" << timer.elapsed() << "ms" << endl;
    Delay_MSec(delayLight);//保证光源稳定
    emit signalGetFrame();
    Delay_MSec(delayMSec);//保证图像数据以传达
    //查询是否已有图片文件夹
    if (dispLastGlassId) {
        dispLastGlassId = false;
        QDir tempdir("./auto");
        QStringList  glassFolders;
        // mFolderPath = tempdir.fromNativeSeparators(mFolderPath);//  "\\"转为"/"
        if (!tempdir.exists()) {
            qDebug() << u8"文件夹不存在";
        } else {
            tempdir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
            tempdir.setSorting(QDir::Name);
            glassFolders = tempdir.entryList();
        }
        if (!glassFolders.isEmpty()) {
            qDebug() << u8"上一张玻璃编号" << glassFolders.back();
            glassId = glassFolders.size() + 1;
        }
    }
    QString imgDir = "./auto/glass" + QString::number(glassId) + "/" + lightModeChar;
    mkFilePath(imgDir);//创建文件夹
    QDir dir(imgDir);
    QString imgPath = dir.absoluteFilePath(QString("%1.%2").arg(QString::number(picsId)).arg("bmp"));
    std::string str = imgPath.toStdString();
    const char* ch = str.c_str();
    //Error error = srcImg.Save(ch);
    mutex.lock();
    Error error = pgrcam->convertImg.Save(ch);
    if (error != PGRERROR_OK) {
        error.PrintErrorTrace();
        return "";
    }
    mutex.unlock();
    return ch;
}
