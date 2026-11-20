#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - NTF 曲线对话框（单例，NTF 设计稿 01.2/02.2）
//
// 入口：Ribbon"NVH曲线"页 -> "曲线"组 ->"NTF曲线"按钮。
//
// 流程：导入 pch 结果文件 + 模板配置生成的 NTF CSV 映射文件（6 列结构，
// Y轴内容列恒空）-> NTF 形式从 CSV 元信息自动识别（只读回显：声压 /
// 声压+激励点IPI）-> 选窗口布局 -> "创建"：逐行生成 实测曲线 + 目标值
// 曲线（一图一行，按布局格数分页落图，整体一次撤销）。
//
// 行级分派（声压+激励点IPI 形式的 CSV 同时含两类行，按行结果类型分派）：
//   声压行（PRESSURE，分量令牌 REL|FX）：实测 = 声压通道幅值原样；
//         目标值取自 CSV（模板提取自 pch $PRESSURE 标签的 dB 值），
//         有则生成常数目标横线，空则不生成；
//         Y 轴标题 Pressures
//   激励点行（ACCELERATION，分量令牌 MAG|X-Trans 等）：实测 = 加速度通道
//         幅值原样；目标值空，不生成目标线；Y 轴标题 Accelerations
//
// 配置文件无效 / 无 NTF 参数时按设计稿模式弹模态提示，文案带具体形式
// （如"导入的配置文件中无NTF声压参数"）。
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
    class POSTCURVE_EXPORT mNtfCurveDlg : public QDialog
    {
        Q_OBJECT
    public:
        static mNtfCurveDlg* getInstance(QWidget* parent = nullptr);
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
        explicit mNtfCurveDlg(QWidget* parent = nullptr);
        void buildUi();
        //创建按钮可用性随两份文件就绪状态刷新
        void updateCreateButtonState();

        static mNtfCurveDlg* m_instance;
        bool _needCenter = true;          //本次运行首开居中于父窗口
        QWidget* _titleBar = nullptr;     //标题栏（拖动事件过滤对象）
        bool _dragging = false;
        QPoint _dragOffset;

        QLineEdit* _resultEdit = nullptr;   //导入结果文件路径（pch）
        QLineEdit* _configEdit = nullptr;   //导入配置文件路径（csv）
        QLabel* _formValue = nullptr;       //NTF形式只读回显（--/声压/声压+激励点IPI）
        MechMainWindow::mImageGridComboBox* _layoutGrid = nullptr;   //窗口布局格数
        QPushButton* _createBtn = nullptr;  //蓝色主按钮（两份文件就绪前置灰）

        QString _pchFilePath;               //已解析成功的 pch 路径（store 登记键）
        QString _configFilePath;            //已读取成功的 csv 路径
        DynStiffForm _form = DynStiffForm::SoundPressure;   //CSV 自动识别的形式
        QList<DynStiffRow> _rows;           //CSV 数据行（创建时逐行消费）
    };
}
