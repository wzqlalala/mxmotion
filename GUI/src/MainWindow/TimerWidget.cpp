#include "TimerWidget.h"
#include "ui_TimerWidget.h"
#include <QTimer>
#include "mLicense.h"
#include "mylabel.h"
#include "mAppConfiguration.h"
TimerWidget::TimerWidget(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::TimerWidget)
{
    ui->setupUi(this);
    initForm();
    connect(m_timer, &QTimer::timeout, this, &TimerWidget::timeEnd);
}

TimerWidget::~TimerWidget()
{
    delete ui;
}

void TimerWidget::initForm()
{
    //设置背景色，qss
    QString strStyle = "QWidget#TimerWidget{background-color:rgb(68,85,123);}";
    setStyleSheet(strStyle);

    setFixedSize(300, 300);
    ui->m_timer->setFixedSize(250, 250);
    ui->m_startBtn->setText("开始");
    ui->m_startBtn->hide();
    m_timer = new QTimer(this);
    m_time = 99;
    Common::mLicense* license = new Common::mLicense(MAppConfiguration::mAppConfiguration::getInstance()->_workingDir);
    //0正确  1配置文件失败  2文件不存在 3不正确 4许可证过期
    //state代表许可证的状态，自己开发时刻设为0 
    int state = license->checkLicense();
    int Day = license->getRemainDay();
    if (Day == -1)
        Day = 0;
    ui->m_timer->setTime(Day);
}

void TimerWidget::setTimer(int day)
{
    ui->m_timer->setTime(day);
}

void TimerWidget::on_m_startBtn_clicked()
{
    //每10毫秒执行一次
    //每10毫秒刷新一次为了让进度条更流畅
    m_timer->start(10);
}

void TimerWidget::timeEnd()
{
    //如果小于0停止计时
    if (m_time < 0)
    {
        m_timer->stop();
        return;
    }
    ui->m_timer->setTime(m_time);
    m_time--;
}

