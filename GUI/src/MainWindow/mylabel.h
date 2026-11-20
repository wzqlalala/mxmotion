#ifndef MYLABEL_H
#define MYLABEL_H

#include <QObject>
#include <QLabel>
#include <QPainter>
class MyLabel : public QLabel
{
    Q_OBJECT

public:
    MyLabel(QWidget* parent = Q_NULLPTR);
    ~MyLabel();
public:
    //��ʱ������ ����ʱ
    void setTime(double second);
signals:

public slots:

protected:
    void paintEvent(QPaintEvent*);
    virtual bool event(QEvent* e);

private:
    void drawTimer(QPainter*);//����ʱ��
private:
    double m_time;//����ʱʱ��
};
#endif // MYLABEL_H