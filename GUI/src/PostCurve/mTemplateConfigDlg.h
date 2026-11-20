#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 模板配置对话框（单例；动刚度 01.1 / VTF 01.1-02.1 / NTF 01.1-02.1 设计稿）
//
// 入口：Ribbon"NVH曲线"页 -> "曲线"组 ->"模板配置"按钮。
//
// 流程：导入 .pch 结果文件 -> 选 模板类型（动刚度曲线/VTF曲线/NTF曲线）+ 形式
// （动刚度：位移默认/速度/加速度；VTF：速度默认/加速度；
//   NTF：声压默认/声压+激励点IPI）-> "保存"：从 pch 提取该形式全部数据块，
// 目标值列填默认参数（动刚度 10000N/mm；VTF 速度 0.1 mm/s、加速度 100 mm/s^2；
// NTF 声压行提取自 pch $PRESSURE 标签的 dB 值、激励点 IPI 行留空=不生成目标线），
// 弹出保存对话框落成 CSV 映射文件（动刚度/VTF 默认名 X_Mapping.csv，
// NTF 默认名 NTF_声压_mapping.csv——小写 mapping 对齐设计稿），供各曲线
// 对话框作模板。
//
// 提取失败（pch 中无该形式的参数）时按设计稿弹模态提示，文案带具体形式
// （如"没有提取到动刚度位移参数"/"没有提取到NTF声压参数"）。
//
// 窗口规格同 mOpenCurveDlg：无边框白卡片（52px 标题栏 + ✕）、蓝色主按钮、
// DlgGeom 会话几何记忆、置顶（避免被曲线页面遮挡）。
// =============================================================================

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QRadioButton>
#include <QPushButton>

#include "PostCurveGlobal.h"
#include "mDynStiffnessMapping.h"

class QLabel;
class QButtonGroup;

namespace PostCurve
{
    class POSTCURVE_EXPORT mTemplateConfigDlg : public QDialog
    {
        Q_OBJECT
    public:
        static mTemplateConfigDlg* getInstance(QWidget* parent = nullptr);
        //唤起已存在实例（含首开居中/置顶/激活，模式同 mOpenCurveDlg::showInstance）
        static void showInstance(QWidget* parent = nullptr);

    protected:
        //标题栏拖动（事件过滤器挂在标题栏上，模式同 mOpenCurveDlg）
        bool eventFilter(QObject* watched, QEvent* event) override;

    private slots:
        void slot_browseFile();   //导入结果文件（.pch）
        void slot_save();         //保存：提取数据块 -> 选位置 -> 写 CSV

    private:
        explicit mTemplateConfigDlg(QWidget* parent = nullptr);
        void buildUi();
        //模板类型切换（动刚度曲线/VTF曲线/NTF曲线）：形式单选组显隐与默认值
        //联动（动刚度：位移默认；VTF：速度默认；NTF：声压默认）
        void applyTemplateKindUi();
        CurveTemplateKind currentKind() const;   //模板选择 -> 模板类型
        DynStiffForm currentForm() const;        //单选组 -> 形式枚举

        static mTemplateConfigDlg* m_instance;
        bool _needCenter = true;          //本次运行首开居中于父窗口
        QWidget* _titleBar = nullptr;     //标题栏（拖动事件过滤对象）
        bool _dragging = false;
        QPoint _dragOffset;

        QLineEdit* _fileEdit = nullptr;       //导入结果文件路径
        QComboBox* _templateCombo = nullptr;  //模板选择（动刚度曲线/VTF曲线/NTF曲线）
        QLabel* _formLabel = nullptr;         //形式行标签（动刚度形式/VTF形式/NTF形式）
        QRadioButton* _dispRadio = nullptr;   //形式：位移（仅动刚度，默认）
        QRadioButton* _velRadio = nullptr;    //形式：速度（动刚度/VTF）
        QRadioButton* _accRadio = nullptr;    //形式：加速度（动刚度/VTF）
        QRadioButton* _spRadio = nullptr;     //形式：声压（仅 NTF，默认）
        QRadioButton* _spIpiRadio = nullptr;  //形式：声压+激励点IPI（仅 NTF）
        QButtonGroup* _formGroup = nullptr;
        QPushButton* _saveBtn = nullptr;      //蓝色主按钮（解析成功前禁用）

        QString _pchFilePath;                 //已解析成功的 pch 路径（store 登记键）
    };
}
