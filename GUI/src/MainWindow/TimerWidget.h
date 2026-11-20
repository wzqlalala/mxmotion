#ifndef TIMERWIDGET_H
#define TIMERWIDGET_H

#include <QWidget>

class QTimer;

namespace Ui {
    class TimerWidget;
}

class TimerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TimerWidget(QWidget* parent = nullptr);
    ~TimerWidget();

    void initForm();
    void setTimer(int day);
private slots:
    void on_m_startBtn_clicked();
    void timeEnd();
private:
    Ui::TimerWidget* ui;
    double m_time;
    QTimer* m_timer;
};

#endif // TIMERWIDGET_H

