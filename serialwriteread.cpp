#include "serialwriteread.h"


SerialWriteRead::SerialWriteRead(QObject *parent) : QObject(parent)
{
    deviceNum = "01";
    serialThread = new QThread();
    serialTimer = new QTimer;
    serialInit();
//    this->moveToThread(serialThread);
//    serialTimer->moveToThread(serialThread);
//    local_port.moveToThread(serialThread);
//    serialThread->start();
    connect(serialTimer, &QTimer::timeout, this, &SerialWriteRead::slotPIR);
}



/*---------------workings------------*/
//响应串口选择后初始化串口
void SerialWriteRead::slotSerialCombo(QString portName)
{
    //重新开启串口
    if (local_port.isOpen()) {
        local_port.clear();
        local_port.close();
//        local_port.deleteLater();
//        serialInit();
    }
    local_port.setPortName(portName);
    qDebug() << "---------Serial set-----";
//    qDebug() << "串口方法所在线程地址: " << QThread::currentThreadId();
    if (local_port.open(QIODevice::ReadWrite)) {
        qDebug() << "port has been opened";
        //串口可用时检验PIR返回是否正常
        slotPIR();
        //串口可用时启动计时器
        serialTimer->start(50);
        emit SerialAvailable(true);
    } else {
        serialTimer->stop();
        qDebug() << "open failed";
        emit SerialAvailable(false);
    }
    qDebug() << endl;
}
//响应主灯
void SerialWriteRead::slotMainSlider(int mainValue)
{
//    QElapsedTimer time;
//    time.start();
    qDebug() << "-------Main--------";
//    qDebug() << "串口方法所在线程id: " << QThread::currentThreadId();
    QString cmdNum = "01";
//    //int 转16进制字符串
//    QString mainValueStr = QString("%1").arg(mainValue, 4, 16, QLatin1Char("0")).toUpper();
//    int mainValue = 0;
    sendData(cmdNum, mainValue);
    readData();
//    int milsec = time.elapsed();
//    qDebug() << "耗时 " << milsec << "ms" ;
//    qDebug() << "*************";
    qDebug() << endl;
}
void SerialWriteRead::slotMainSlider2(int mainValue)
{
    qDebug() << "-------Main2--------";
    QString cmdNum = "02";
    sendData(cmdNum, mainValue);
    readData();
    qDebug() << endl;
}

//响应辅灯
void SerialWriteRead::slotSubSlider(int subValue)
{
    qDebug() << "-------Sub--------";
    QString cmdNum = "0e";
    sendData(cmdNum, subValue);
    readData();
    qDebug() << endl;
}
void SerialWriteRead::slotSubSlider2(int subValue)
{
    qDebug() << "-------Sub2--------";
    QString cmdNum = "0f";
    sendData(cmdNum, subValue);
    readData();
    qDebug() << endl;
}
//响应周期
void SerialWriteRead::slotPeriodSpin(double periodValue)
{
    qDebug() << "-------Period------";
    QString cmdNum = "04";
    sendData(cmdNum, int(periodValue * 100));
    readData();
    qDebug() << endl;
}
//响应占空比
void SerialWriteRead::slotDutyRateSpin(double periodValue, int DRValue)
{
    qDebug() << "-------DRrate------";
    QString cmdNum = "05";
    sendData(cmdNum, int(periodValue * DRValue) - 1);
    readData();
    qDebug() << endl;
}
//设置参数并启动电机
void SerialWriteRead::slotStartMotor(double periodValue, int DRValue, bool stepMove, bool forwardReverse, int pulseNum)
{
    qDebug() << u8"启动电机";
    slotStopMotor();
    slotPeriodSpin(periodValue);
    slotDutyRateSpin(periodValue, DRValue);
    //设置方向
    if(forwardReverse) {
        sendData("0a", 0);
        qDebug() << "forward";
    } else {
        sendData("0a", 1);
        qDebug() << "reverse";
    }
    readData();
    if (stepMove) {
        sendData("0c", pulseNum);//固定脉冲
        readData();
        sendData("08", 1);
        qDebug() << "step";
    } else {
        sendData("08", 2); //无限脉冲
        qDebug() << "Non-stop";
    }
    readData();
    qDebug() << endl;
}


//停止电机
void SerialWriteRead::slotStopMotor()
{
    qDebug() << u8"停止电机 ";
    sendData("08", 0);
    readData();
    qDebug() << endl;
}

//


//全开光
void SerialWriteRead::slotSetLight(int mainValue, int subValue)
{
    slotMainSlider(mainValue);
    slotSubSlider(subValue);
    emit lightSet();
}

//定时返回PIR的值
void SerialWriteRead::slotPIR()
{
    qDebug() << "-------PIR------";
    int sendValue = 0;
    QString cmdNum = "03";
    sendData(cmdNum, sendValue);
    QByteArray receiveData = readData();
    if (receiveData.isEmpty()) {
        //串口正常,但是PIR返回异常时,认为串口不可用,关闭定时器,初始化界面状态
        serialTimer->stop();
        emit PIRReady(0, false);
        return;
    }
    QByteArray PIRData;
    PIRData.resize(2);
    PIRData[0] = receiveData[2];
    PIRData[1] = receiveData[3];
//    PIRData[0] = 0x38;
//    PIRData[1] = 0x00;
    int PIRValue = PIRData.toHex().toInt(nullptr, 16);
    PIRValue = ((PIRValue & 0x00FF) << 8) | (PIRValue >> 8);
    emit PIRReady(PIRValue, true);
    qDebug() << endl;
}





/*****************private function***********/
//初始化
void SerialWriteRead::serialInit()
{
    local_port.setBaudRate(QSerialPort::Baud115200);//设置波特率
    local_port.setParity(QSerialPort::NoParity);//奇偶检验
    local_port.setDataBits(QSerialPort::Data8);//数据位
    local_port.setStopBits(QSerialPort::OneStop);//停止位
    local_port.setFlowControl(QSerialPort::NoFlowControl);//流控制
}
//发送命令
void SerialWriteRead::sendData(QString cmdNum, int data)
{
    //数据字节高低位互换
    QString dataStr = highAndLowConvert(data);
    //连接命令
    QByteArray cmdHex = concatCmdStr(cmdNum, dataStr);
    //发送
    local_port.write(cmdHex);
    qDebug() << u8"串口号 " << local_port.portName();
    //qDebug() << u8"命令字 " << cmdNum << dataStr ;
    qDebug() << u8"命令字 " << cmdHex.toHex() ;
    //发送后等待其发送完毕
    if(local_port.waitForBytesWritten(100)) {
        qDebug() << u8"发送成功 ";
    } else {
        qDebug() << u8"发送失败.超时 ";
        emit sendFail();
    }
}
//接收返回
QByteArray SerialWriteRead::readData()
{
    //等待数据完整后获取
    QByteArray buf = "";
    if (local_port.waitForReadyRead(100)) {
        buf = local_port.readAll();
        local_port.readyRead();
        qDebug() << u8"接收成功 ";
        qDebug() << "成功返回值 " << buf.toHex();
        return buf;
    } else  {
        buf = local_port.readAll();
        qDebug() << u8"接收失败 ";
        qDebug() << u8"失败返回值 " << buf.toHex();
        return buf;
        emit receiveFail();
    }
    //直接获取
    // QByteArray buf = local_port.readAll();
    // return buf;
}

//连接命令
QByteArray SerialWriteRead::concatCmdStr(QString cmdNum, QString dataStr)
{
    QString cmdStr;
    cmdStr.append(deviceNum);
    cmdStr.append(cmdNum);
    cmdStr.append(dataStr);
    cmdStr.append(crc16ForModbus(cmdStr));//计算校验码,并在内部高低位转换
    QByteArray cmdStr_hex = QSTring2Hex(cmdStr);//字符串转换成单片机能读取的字节数组
    return cmdStr_hex;
}
// 表示16进制数值的字符串转16进制字节数组
QByteArray SerialWriteRead::QSTring2Hex(QString cmdStr)
{
    QByteArray cmdHex;
    int hexdata, lowhexdata;
    int hexdatalen = 0;
    int len = cmdStr.length();
    cmdHex.resize(len / 2);
    char lstr, hstr;
    for(int i = 0; i < len;) {
        //将第一个不为' '的字符给hstr
        hstr = cmdStr[i].toLatin1();
        if (hstr == ' ') {
            i++;
            continue;
        }
        i++;
        if(i >= len) {
            break;
        }
        //当i<len时,将下一个字符赋值给lstr
        lstr = cmdStr[i].toLatin1();
        //将hstr和lstr转换为0-15的对应数值
        hexdata = ConvertHexChar(hstr);
        lowhexdata = ConvertHexChar(lstr);
        if((hexdata == 16) || (lowhexdata == 16)) {
            break;
        } else {
            hexdata = hexdata * 16 + lowhexdata;
        }
        i++;
        cmdHex[hexdatalen] = (char)hexdata;
        hexdatalen++;
    }
    cmdHex.resize(hexdatalen);
    return cmdHex;
}
// 单个字符转16进制
char SerialWriteRead::ConvertHexChar(char c)
{
    if((c >= '0') && (c <= '9')) {
        return c - 0x30;
    } else if((c >= 'A') && (c <= 'F')) {
        return c - 'A' + 10;
    } else if((c >= 'a') && (c <= 'f')) {
        return c - 'a' + 10;
    } else {
        return -1;
    }
}

//高低位互换
QString SerialWriteRead::highAndLowConvert(int &sendValue)
{
    sendValue = ((sendValue & 0x00FF) << 8) | (sendValue >> 8);
    QString sendValueStr = QString("%1").arg(sendValue, 4, 16, QLatin1Char('0')).toUpper();
    return sendValueStr;
}
//计算校验码
QString SerialWriteRead::crc16ForModbus(QString &str)
{
    static const quint16 crc16Table[] = {
        0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
        0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
        0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
        0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
        0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
        0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
        0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
        0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
        0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
        0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
        0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
        0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
        0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
        0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
        0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
        0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
        0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
        0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
        0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
        0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
        0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
        0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
        0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
        0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
        0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
        0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
        0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
        0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
        0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
        0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
        0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
        0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
    };
    QByteArray hexdata = QByteArray::fromHex(str.toLatin1());
    quint8 buf ;
    quint16 crc16 = 0xFFFF;//CRC字节初始化
    for(auto i = 0; i < hexdata.size(); i++) {
        //计算CRC
        buf = hexdata.at(i)^crc16;
        crc16 >>= 8;
        crc16 ^= crc16Table[buf];
    }
    quint8 crcHi = crc16 >> 8;
    quint8 crcLo = crc16 & 0xFF;
    crc16 = (crcLo << 8) | crcHi;//校验码也需要高低位互换
    return QString("%1").arg(crc16, 4, 16, QLatin1Char('0')).toUpper();
}
