#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 动刚度曲线对话框（单例，设计稿 01.2/02.2/03.2）
//
// 入口：Ribbon"NVH曲线"页 -> "曲线"组 ->"动刚度曲线"按钮。
//
// 流程：导入 pch 结果文件 + 模板配置生成的 CSV 映射文件 -> 动刚度形式从
// CSV 元信息自动识别（只读回显）-> 选窗口布局 -> "创建"：逐行生成
// 实测曲线 + 目标值曲线（一图一行，按布局格数分页落图，整体一次撤销）。
//
// 三种形式的曲线计算（f=通道频率，|H|=通道幅值，K=目标值数值）：
//   位移：实测 kd = 1/|H|（动刚度 Ka=F(ω)/x(ω)，|H| 为位移导纳幅值；
//         实部/虚部格式按复数模取幅值，避免 1/实部 过零爆尖峰）
//         目标 = 常数 K（水平目标线）
//   速度：实测 = |H| 原样（Velocity Mobility = v(ω)/F(ω)）
//         目标 = 2πf/K
//   加速度：实测 = |H| 原样（IPI = a(ω)/F(ω)）
//         目标 = (2πf)²/K
//
// 配置文件无效 / 无动刚度参数时按设计稿弹模态提示，文案带具体形式
// （如"导入的配置文件中无动刚度位移参数"）。
// =============================================================================

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

#include "PostCurveGlobal.h"
#include "mDynStiffnessMapping.h"

namespace MechMainWindow { class mImageGridComboBox; }

namespace PostCurve
{
    class POSTCURVE_EXPORT mDynStiffnessDlg : public QDialog
    {
        Q_OBJECT
    public:
        static mDynStiffnessDlg* getInstance(QWidget* parent = nullptr);
        //唤起已存在实例（含首开居中/置顶/激活，模式同 mOpenCurveDlg::showInstance）
        static void showInstance(QWidget* parent = nullptr);

    protected:
        //标题栏拖动（事件过滤器挂在标题栏上，模式同 mOpenCurveDlg）
        bool eventFilter(QObject* watched, QEvent* event) override;

    private slots:
        void slot_browseResultFile();   //导入结果文件（.pch）
        void slot_browseConfigFile();   //导入配置文件（.csv 模板）
        void slot_create();             //创建：逐行生成 实测+目标值 曲线

    private:
        explicit mDynStiffnessDlg(QWidget* parent = nullptr);
        void buildUi();
        //创建按钮可用性随两份文件就绪状态刷新
        void updateCreateButtonState();

        static mDynStiffnessDlg* m_instance;
        bool _needCenter = true;          //本次运行首开居中于父窗口
        QWidget* _titleBar = nullptr;     //标题栏（拖动事件过滤对象）
        bool _dragging = false;
        QPoint _dragOffset;

        QLineEdit* _resultEdit = nullptr;   //导入结果文件路径（pch）
        QLineEdit* _configEdit = nullptr;   //导入配置文件路径（csv）
        QLabel* _formValue = nullptr;       //动刚度形式只读回显（--/位移/速度/加速度）
        MechMainWindow::mImageGridComboBox* _layoutGrid = nullptr;   //窗口布局格数
        QPushButton* _createBtn = nullptr;  //蓝色主按钮（两份文件就绪前置灰）

        QString _pchFilePath;               //已解析成功的 pch 路径（store 登记键）
        QString _configFilePath;            //已读取成功的 csv 路径
        DynStiffForm _form = DynStiffForm::Displacement;   //CSV 自动识别的形式
        QList<DynStiffRow> _rows;           //CSV 数据行（创建时逐行消费）
    };
}
