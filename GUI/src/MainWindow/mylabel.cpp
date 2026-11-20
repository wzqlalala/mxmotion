

#include "mylabel.h"
#include "mOtherFunction.h"
#include<QDebug>

#pragma execution_character_set("utf-8")

MyLabel::MyLabel(QWidget* parent)
    :QLabel(parent)
{

}

MyLabel::~MyLabel()
{

}

void MyLabel::setTime(double second)
{
    m_time = second;
    update();
}

//��дpaint�¼�
void MyLabel::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);

    QPainter painter(this);
    //���ÿ����
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    drawTimer(&painter);
}

bool MyLabel::event(QEvent* e)
{
    return QLabel::event(e);
}

void MyLabel::drawTimer(QPainter* p)
{
    //����������εĴ�С���ڻ���Բ�ν�����
    QRect rcOut = this->rect();
    //adjust�ĸ�������dx1,dy1��������Ͻǵĵ�,dx2,dy2��������½ǵĵ�
    rcOut.adjust(10, 10, -10, -10);
    QSize outSize = rcOut.size();
    //��ȡ������ε����Ͻ�����
    int out_X = rcOut.x();
    int out_Y = rcOut.y();

    //�����ھ��εĴ�С���ڻ���ʱ��
    QRect rcInt = this->rect();
    rcInt.adjust(20, 90, -20, -90);
    QSize inSize = rcInt.size();
    int in_X = rcInt.x();
    int in_Y = rcInt.y();

    //���ھ���
    //�����α߿�ɫ��ͬ������ı���ɫ����ͬ�Ļ����г���һ�������ξ���
    p->setPen(QPen(QColor(68, 85, 123), 1, Qt::SolidLine));
    //���ƾ���
   /* p->drawRect(rcInt);*/


    // 1. 背景灰环
    p->setPen(QPen(QColor(230, 230, 230), 10, Qt::SolidLine,
        Qt::RoundCap));
    p->drawEllipse(rcOut);

    //����ʱ��
    //���û�����ʽ
    QFont font;
    //��С����������label�Ĵ�С����
    font.setPointSize(45);
    p->setFont(font);
    QPen pen;
    pen.setColor(QColor("black"));
    pen.setWidth(1);
    pen.setStyle(Qt::SolidLine);
    p->setPen(pen);

    QString timer;
    if (m_time == 0)
    {
        timer = "00";
        //�������֣����ھ��ε�size�����־���
        p->drawText(in_X, in_Y, inSize.width(), inSize.height(), Qt::AlignVCenter | Qt::AlignHCenter, timer);
    }
    else {
        int time = m_time; //��1����ΪС��100ʱ����0�ˣ����ǻ��ƻ�δ����
        timer = MxFunctions::mNumberToString(time);
        if (time < 10)
        {
            timer = "0" + MxFunctions::mNumberToString(time);
        }
        p->drawText(in_X, in_Y, inSize.width(), inSize.height(), Qt::AlignVCenter | Qt::AlignHCenter, timer);
    }

    int startAngle = 90 * 16;                 //��ʼ�ĽǶȣ�0����ͼ�ε������
    int spanAngle = m_time / 99 * (-360 * 16);  //���ǵĽǶ�

    //绘制圆形进度条
      //QLinearGradient  参数：渐变的开始坐标，渐变的结束坐标
    QLinearGradient gradient(out_X, out_Y, outSize.height(), out_Y);//左右渐变
    //QLinearGradient gradient(out_X, out_Y, out_X, outSize.height()+out_Y);//上下渐变
    gradient.setColorAt(0, QColor("#E3521B"));
    gradient.setColorAt(1.0, QColor("#8BE31B"));


    QPen penForeground;
    penForeground.setBrush(gradient);
    penForeground.setWidth(15);
    penForeground.setJoinStyle(Qt::RoundJoin);
    penForeground.setCapStyle(Qt::RoundCap);
    p->setPen(penForeground);
    //绘制圆环，用到外圈的size
    p->drawArc(rcOut, startAngle, spanAngle);
}


