#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"
#include <QWidget>
#include "mPanelBase.h"
//选择部件拾取条仅作指针成员（_symmetryPickController/_axialPickController），
//头内前向声明——按值包含 mPostGeneralPickUp.h 会把 RebuildRender 头拖进
//无渲染依赖的编译单元（mPostVectorPlot.h 同款轻量化口径）；工厂调用在 .cpp 包含

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow {}
using namespace MechMainWindow;

class QVBoxLayout;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QGroupBox;
class QHideEvent;
class QShowEvent;

namespace MxFunctions
{
    class mComboActionRow;
    class mChoiceGroup;
    class mJoinedPairRow;
    class mPrimaryButton;
    class mGroupBox;
}

namespace PostWindow
{
    class mPostPartScopeField;
    class mPostPickWidgetController;

    /*
    * 阵列与回转面板（V1.3 原型 #symmetryParameters，控件全部换共享 UI 库）：
    * 镜像与阵列折叠组：选择部件（mPost 通用拾取条）+ 操作选择与顺序（固定面板，
    *   勾选驱动三个骑线分组启停、行序同步分组顺序）+ 平面镜像/平面阵列/圆周阵列
    *   （mGroupBox 细线框）+ 底部操作行 + 状态行；
    * 轴向回转折叠组：选择部件 + 旋转轴 + 阵列数量 + 总角度 + 底部操作行 + 状态行。
    */
    class MPOSTWINDOW_EXPORT mPostParrayMimagePanel : public mPanelBase
    {
        Q_OBJECT
    public:
        explicit mPostParrayMimagePanel(QWidget* parent = nullptr);

        void newParrayMimagePanel(int caseId);

    signals:
        void returnSig();

    private slots:
        void slot_changeLanguage(int language);

        void slot_clear();
        void slot_apply();
        void slot_axialClear();
        void slot_axialApply();

    protected:
        /* 面板显隐与拾取条收口成对（云图/向量图同款：hideEvent = 面板退出事件，
        * 激活态拾取解除 + 弹窗随宿主隐藏；showEvent 对称恢复——
        * setPickWidgetVisible(true/false) 一对） */
        void hideEvent(QHideEvent* event) override;
        void showEvent(QShowEvent* event) override;

    private:
        // —— 构建辅助 ——
        mPostPickWidgetController* buildPickRow(const QString& objectName);
        void buildOrderPanel();                     // 操作选择与顺序（固定面板）
        MxFunctions::mGroupBox* buildActionGroup(const QString& objectName); // 骑线细线框分组
        void buildMirrorRows(MxFunctions::mGroupBox* box);
        void buildPlanarRows(MxFunctions::mGroupBox* box);
        void buildCircularRows(MxFunctions::mGroupBox* box);

        // —— 行为辅助 ——
        void initialUI();                   // 构造期默认态（原型：三操作全禁用）
        void resetDefaultOrder();           // 执行顺序恢复默认：镜像、平面、圆周
        void refreshOrderButtons();         // 首行上移/末行下移禁用
        void syncGroupOrder();              // 操作表行序 → 分组行序（原型 sync 语义）
        void updateExecuteEnabled();        // 执行钮随任一操作启用解禁
        void updateAdaptiveView();          // 自适应视图（视口适配模型）
        void setSymmetryStatus(const QString& text);   // 状态行（空=隐藏）
        void setAxialStatus(const QString& text);

        // 行标签显式登记表（语言切换全量重刷——嵌套结构按 findChildren 序号
        // 猜行必漂移，见 mPostSectionCut 缺陷修复留档）
        struct RegLabel { PanelLabel* widget; QString zh; QString en; };
        QList<RegLabel> _regLabels;
        PanelLabel* regLabel(const QString& zh, const QString& en);

        QVBoxLayout* _mainLayout{};

        FoldSection* _mainFold{};           // 镜像与阵列（主参数折叠组）
        FoldSection* _axialFold{};          // 轴向回转

        // 选择部件（mPost 通用拾取条 ×2，云图/向量图"对象选择"同款；两条共享一个
        // 隐形范围控制器 _partScope——仅作"所有部件"本地化空态文案来源，范围弹窗不可达）
        mPostPickWidgetController* _symmetryPickController{};
        mPostPickWidgetController* _axialPickController{};
        mPostPartScopeField* _partScope{};

        // 操作选择与顺序（固定面板；行顺序即执行顺序，从上到下）
        QWidget* _orderPanel{};
        QLabel* _orderHeading{};
        QVBoxLayout* _orderListLayout{};

        // —— 三个动作分组（mGroupBox 无勾选图标；启用只由操作表勾选控制）——
        MxFunctions::mGroupBox* _groupBox_mirrorEnable{ nullptr };
        MxFunctions::mGroupBox* _groupBox_rectEnable{ nullptr };
        MxFunctions::mGroupBox* _groupBox_circularEnable{ nullptr };
        // 操作表勾选（与分组启用双向联动）
        QCheckBox* _checkBox_orderMirror{ nullptr };
        QCheckBox* _checkBox_orderRect{ nullptr };
        QCheckBox* _checkBox_orderCircular{ nullptr };

        // —— 坐标系复合行（下拉伸展 + "＋"待接入置灰）——
        MxFunctions::mComboActionRow* _coordMirror{ nullptr };
        MxFunctions::mComboActionRow* _coordPlanar{ nullptr };
        MxFunctions::mComboActionRow* _coordCircular{ nullptr };

        // 平面镜像：三连单选格 X-Y | Y-Z | Z-X（三面互斥，默认 X-Y）
        MxFunctions::mChoiceGroup* _mirrorPlanes{ nullptr };

        // 平面阵列：数量|距离 1:2 拼接行 ×3 + 列头
        MxFunctions::mJoinedPairRow* _pairX{ nullptr };
        MxFunctions::mJoinedPairRow* _pairY{ nullptr };
        MxFunctions::mJoinedPairRow* _pairZ{ nullptr };
        QLabel* _headCount{ nullptr };
        QLabel* _headDistance{ nullptr };

        // 圆周阵列
        MxFunctions::mChoiceGroup* _circularAxis{ nullptr };   // X轴/Y轴/Z轴（默认 X）
        QLineEdit* _editCircularCount{ nullptr };
        QLineEdit* _editCircularAngle{ nullptr };

        // 底部操作行 + 状态行（镜像与阵列）
        QCheckBox* _checkBox_adaptiveView{ nullptr };
        QPushButton* _pushButton_clear{};
        MxFunctions::mPrimaryButton* _pushButton_apply{};

        QLabel* _symmetryStatus{ nullptr };

        // 轴向回转
        MxFunctions::mChoiceGroup* _axialAxis{ nullptr };      // X轴/Y轴/Z轴（默认 X）
        QLineEdit* _editAxialCount{ nullptr };
        QLineEdit* _editAxialAngle{ nullptr };
        QCheckBox* _checkBox_axialadaptiveView{ nullptr };
        QPushButton* _pushButton_axialClear{};
        MxFunctions::mPrimaryButton* _pushButton_axialApply{};
        QLabel* _axialStatus{ nullptr };
    };
}
