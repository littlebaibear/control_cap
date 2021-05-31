#ifndef SERIALWRITEREAD_H
#define SERIALWRITEREAD_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>
#include <QThread>

#include <QTimer>
#include <QElapsedTimer>

class SerialWriteRead : public QObject
{
    Q_OBJECT
public:
    /*------------functions---------------*/
    explicit SerialWriteRead( QObject *parent = nullptr);
    void slotSerialCombo( QString portName);//响应主界面的串口选择

    void slotMainSlider(int mainValue);//响应主界面的主光改变
    void slotSubSlider(int subValue);//响应主界面的辅光改变
    void slotMainSlider2(int mainValue);
    void slotSubSlider2(int subValue);

    void slotPeriodSpin(double periodValue = 100); //响应主界面周期改变
    void slotDutyRateSpin(double periodValue, int DRValue);//响应主界面占空比改变

    void slotPIR();//响应类内定时器向主界面发送PIR返回值
//    void show_func_id();
//    void slotUpdatePort();//点击后更新串口的字符串列表

    void slotStartMotor(double periodValue, int DRValue, bool stepMove, bool forwardReverse, int pulseNum); //设置电机,并启动电机
//    void slotStepMove(double periodValue, int DRValue,  bool forwardReverse, int  pulseNum); //按照指定脉冲步进                                                                                                                                            )
    void slotStopMotor();//停止电机

    void slotSetLight(int mainValue, int subValue);//全开光
    /*-------------variables-------------*/
    QSerialPort local_port;
    QTimer *serialTimer;

private:
    /*-----------------functions----------------*/
    void serialInit();//初始化串口
    void sendData(QString cmdNum, int data);//16进制字符串发送命令
    QByteArray readData();//16进制数据接收返回
    QByteArray concatCmdStr(QString cmdNum, QString dataStr);
    QByteArray QSTring2Hex(QString cmdStr);//字符串转16进制
    char ConvertHexChar(char c);//单个字符转16进制
    QString highAndLowConvert(int &sendValue);//高低位互换
    QString crc16ForModbus(QString &str);// 校验码计算
//    void show_slots_id();
    /*-------------variable----------------*/
    QString deviceNum;
    QThread *serialThread;




signals:
    void PIRReady(int PIRValue, bool ctrlSuccess); //向外界发送PIR的信号
    void SerialAvailable(bool availabel);//当串口可读写时返回给主界面
    /****成功信号******/
    void newPortUsable(QStringList portList);//当串口可以更新时,发送携带新串口字符串列表的信号给主界面
    void lightSet();
    /*****失败信号******/
    void sendFail();
    void receiveFail();
};

#endif // SERIALWRITEREAD_H
