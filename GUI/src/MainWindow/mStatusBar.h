#pragma once
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <QWidget>
#include <QStatusBar>
#include <QLabel>
#include <QToolButton>
#include <QMenu>
#include <QActionGroup>
#include <QHBoxLayout>
#include <QDialog>
#include <QMessageBox>
#include <QFontMetrics>
#include <QStyle>
#include <QProgressBar>

#include "UiPalette.h" //setSelection 内联富文本取色板

namespace MechMainWindow
{
    class mStatusBar : public QStatusBar {
        Q_OBJECT
    public:
        explicit mStatusBar(QWidget* parent = nullptr);

        ~mStatusBar();

        // 外部接口
        void setMessage(const QString& msg);

        void setSelection(const QString& sel) {
            m_lastSelection = sel; // 主题切换时按缓存值重生成富文本（内联色随主题）
            const QString shown = sel.isEmpty() ? QStringLiteral("无") : sel;
            selLabel->setText(QString("<a style='color:%2' href=' '>选择: %1</a >")
                .arg(shown, MxFunctions::UiPalette::label().name()));
        }

        void setUnit(const QString& unit) {
            unitButton->setText("单位: " + unit);
        }

    signals:
        void unitChanged(const QString& unit);

    public slots:
        void showMessageDialog();

        void showSelectionDialog();


        /*
        * 输出永久信息
        */
        void slot_outputPermanentInfo(const QString& msg);

        /*
        * 设置选择状态栏的消息
        */
        void slot_updateSelectInfo(QString text);

        /*
        * 输出信息
        */
        void slot_outputMessage(const QString& msg);

        /*
        * 清除信息
        */
        void slot_clearMessage();

        /*
        * 进度条显示
        */
        void setProgressBarValue(int);

        /*
        * 表格输出信息显示
        */
        void outPutInfoShow();

        QProgressBar* getStatusBar();

        /*
        * 主题重发（规则 0.2，附 A 状态栏条款）：ThemeManager::applyAll 遍历顶层窗时
        * 经元方法调用（Common 不反向链接 MainWindow 类型），构造时也会自调一次。幂等。
        */
        Q_INVOKABLE void applyTheme();
    private:
        /*
        * 单位制菜单
        */
        QMenu* getUnitsMenu();

    private:
        QLabel* createClickableLabel(const QString& text) {
            QLabel* label = new QLabel(QString("<a style='color:#606266' href=' '>%1</a>").arg(text));
            label->setTextFormat(Qt::RichText);
            label->setTextInteractionFlags(Qt::TextBrowserInteraction);
            label->setOpenExternalLinks(false);
            label->setCursor(Qt::PointingHandCursor);
            return label;
        }

        void adjustLabelWidth(QLabel* label) {
            label->setMinimumWidth(label->sizeHint().width());
        }

        void adjustButtonWidth(QToolButton* btn) {
            btn->setMinimumWidth(btn->sizeHint().width());
        }

        void progressBarHideOrShow(int);
        void paintEvent(QPaintEvent* event) override;

        /*
        * 事件过滤：监听 qApp 的 StyleChange/ApplicationPaletteChange——全局
        * mApplicationStart.qss 在主窗构造之后才经 qApp->setStyleSheet 加载，
        * 会重新抛光所有控件并盖掉构造期写入的样式，此时补一次 applyTheme 重发
        */
        bool eventFilter(QObject* watched, QEvent* event) override;
    private:
        QLabel* infoLabel;
        QLabel* msgLabel;
        QLabel* selLabel;
        QToolButton* unitButton;
        QHBoxLayout* layout;
        QProgressBar* _progressBar;
        QTimer* _timer = nullptr;
        QTimer* _timerProgressBar = nullptr;
        QString m_lastSelection; //最近一次选择信息原文（主题切换重生成富文本用）
    };
}