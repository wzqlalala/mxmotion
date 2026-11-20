#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - VTF 曲线对话框（单例，VTF 设计稿 01.2/02.2）
//
// 入口：Ribbon"NVH曲线"页 -> "曲线"组 ->"VTF曲线"按钮。
//
// 流程：导入 pch 结果文件 + 模板配置生成的 VTF CSV 映射文件（5 列，无
// Y轴内容列）-> VTF 形式从 CSV 元信息自动识别（只读回显）-> 选窗口布局
// -> "创建"：逐行生成 实测曲线 + 目标值曲线（一图一行，按布局格数分页
// 落图，整体一次撤销；页名 Velocities-N/Accelerations-N）。
//
// 两种形式的曲线计算（H=通道幅值，K=目标值数值）：
//   速度：实测 = H 原样（速度传递率，Velocity），目标 = 常数 K 横线
//         （默认 0.1 mm/s）
//   加速度：实测 = H 原样（加速度传递率，Acceleration），目标 = 常数 K 横线
//         （默认 100 mm/s^2；设计稿：每个视口目标值允许设置不同的值——
//         逐行取各自 Target 列）
//
// 与动刚度曲线的差异：实测曲线不做变换（VTF 即通道本身）、目标线恒为
// 常数横线、无 Y轴内容列、Y 轴标题自动设为 Velocities/Accelerations。
//
// 配置文件无效 / 无 VTF 参数时按设计稿模式弹模态提示，文案带具体形式
// （如"导入的配置文件中无VTF速度参数"）。
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
    class POSTCURVE_EXPORT mVtfCurveDlg : public QDialog
    {
        Q_OBJECT
    public:
        static mVtfCurveDlg* getInstance(QWidget* parent = nullptr);
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
        explicit mVtfCurveDlg(QWidget* parent = nullptr);
        void buildUi();
        //创建按钮可用性随两份文件就绪状态刷新
        void updateCreateButtonState();

        static mVtfCurveDlg* m_instance;
        bool _needCenter = true;          //本次运行首开居中于父窗口
        QWidget* _titleBar = nullptr;     //标题栏（拖动事件过滤对象）
        bool _dragging = false;
        QPoint _dragOffset;

        QLineEdit* _resultEdit = nullptr;   //导入结果文件路径（pch）
        QLineEdit* _configEdit = nullptr;   //导入配置文件路径（csv）
        QLabel* _formValue = nullptr;       //VTF形式只读回显（--/速度/加速度）
        MechMainWindow::mImageGridComboBox* _layoutGrid = nullptr;   //窗口布局格数
        QPushButton* _createBtn = nullptr;  //蓝色主按钮（两份文件就绪前置灰）

        QString _pchFilePath;               //已解析成功的 pch 路径（store 登记键）
        QString _configFilePath;            //已读取成功的 csv 路径
        DynStiffForm _form = DynStiffForm::Velocity;   //CSV 自动识别的形式
        QList<DynStiffRow> _rows;           //CSV 数据行（创建时逐行消费）
    };
}
