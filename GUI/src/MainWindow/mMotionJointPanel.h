#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include "mxr/data/SelectDataStruct.h"
#include "mBasicEnum.h"
#include "mPhysicalFieldEnum.h"
#include "mPointPickerWidget.h"
#include "mDirectionSelectorWidget.h"

#include <memory>
#include <set>

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QLabel>
#include <QToolButton>
#include <QMdiSubWindow>
#include <QCheckBox>
#include <QComboBox>

#include "mPanelBase.h"
#include "mCalcLineEdit.h"

namespace MechPreData
{
    class mViewportModel;
}
namespace MechMotionData
{
    class mMotionJointBase;
    class mMotionJointBasic;
    class mMotionJointUniversal;
    class mMotionJointScrew;
    class mMotionJointPerpendicular;
    class mMotionJointPointOnCurve;
}
namespace MechMainWindow
{
    //运动副面板
    class MMAINWINDOW_EXPORT mMotionJointPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 默认构造函数
        */
        mMotionJointPanel(QWidget* parent = Q_NULLPTR);

        /*
        * 析构函数
        */
        ~mMotionJointPanel();

        /*
        * 新建运动副
        */
        void newMotionJointPanel(MxFunctions::JointType type);

        /*
        * 编辑运动副(从树上点击过来的)
        */
        void editMotionJointPanel(int MotionJointID = 0);

        /*
        * 取消编辑运动副
        */
        void uneditMotionJointPanel();
        /*
        * 数据对象即将被删除（树删除该类型前调用，此时对象仍有效）：置空编辑指针
        * 防悬垂——面板刷新链读已释放对象是未定义行为（衬套删除崩溃同源）
        */
        void onMotionJointDataAboutToBeDestroyed(int id = -1);


    private:

        void refreshBodyComboBoxes();

        void updateUIFromData();

        //专门处理类型切换与数据迁移的核心逻辑
        void handleTypeChange(MxFunctions::JointType newType);
        void refreshSurfaceFaceLabel();  ///< 刷新约束面数量标签（点在面上副）
        void refreshCurveEdgeLabel();    ///< 刷新约束边数量标签（点在线上副）
        void refreshCurveIEdgeLabel();   ///< 刷新约束边数量标签（线在线上副，动作体侧）
        void refreshCurveJEdgeLabel();   ///< 刷新约束边数量标签（线在线上副，基本体侧）

        // 摩擦/限位支持范围与数据同步
        bool supportsFriction(MxFunctions::JointType type) const;
        bool supportsLimit(MxFunctions::JointType type) const;
        void updateFrictionLimitVisibility();
        void syncFrictionDataFromUI();
        void syncLimitDataFromUI();

        // 万向节矢量归一化与正交校验
        QVector3D normalizeVector(const QVector3D& v) const;
        bool isOrthogonal(const QVector3D& a, const QVector3D& b) const;
        bool validateUniversalVectors() const;

        /*
        * 连接信号
        */
        void connectSig();

        /*
        * 断开连接信号
        */
        void disconnectSig();

    private slots:
        //切换视口	
        void slot_subWindowActivated(QMdiSubWindow* subWindow);

        //拾取信号
        void slot_finishedPick();
    protected:

        //真正的运动体数据
        MechMotionData::mMotionJointBase* m_motionJointData;

        //总布局
        QVBoxLayout* m_mainLayout;

        // ==================== 界面控件 ====================
        // 1. 类型
        QComboBox* m_cmbJointType;

        // 2. 动作实体 (Action Body)
        QComboBox* m_cmbActionBody;
        QToolButton* m_btnPickAction;

        mPointPickerWidget* m_originPicker;          ///< 原点拾取控件
        mDirectionSelectorWidget* m_axisDirSelector; ///< 指定矢量方向控件

        // 万向节四个矢量方向控件
        mDirectionSelectorWidget* m_actionSpinDirSelector;
        mDirectionSelectorWidget* m_actionPinDirSelector;
        mDirectionSelectorWidget* m_baseSpinDirSelector;
        mDirectionSelectorWidget* m_basePinDirSelector;

        // 垂直约束两个矢量方向控件
        mDirectionSelectorWidget* m_actionVecDirSelector;
        mDirectionSelectorWidget* m_baseVecDirSelector;

        // 3. 基本实体 (Base Body) - 极其清爽，只留拾取
        QComboBox* m_cmbBaseBody;
        QToolButton* m_btnPickBase;

        // 折叠框
        FoldSection* m_foldAction;
        FoldSection* m_foldBase;
        FoldSection* m_foldFriction;
        FoldSection* m_foldLimit;

        // 4. 摩擦 (Friction)
        QCheckBox* m_chkEnableFriction;
        QCheckBox* m_chkAllowSliding;  // 线在线上副：允许滑动（true=相切+滑移；false=无滑滚动，默认）
        MxFunctions::mCalcLineEdit* m_spbStaticMu;
        MxFunctions::mCalcLineEdit* m_spbDynamicMu;
        MxFunctions::mCalcLineEdit* m_spbFrictionRadius;
        QLabel* m_lblFrictionRadius;

        // 5. 限位 (Limit)
        QCheckBox* m_chkEnableLimit;
        QCheckBox* m_chkEnableLowerLimit;
        QCheckBox* m_chkEnableUpperLimit;
        MxFunctions::mCalcLineEdit* m_spbLowerLimit;
        MxFunctions::mCalcLineEdit* m_spbUpperLimit;
        QCheckBox* m_chkPenaltyOnly;
        MxFunctions::mCalcLineEdit* m_spbLowerCushion;
        MxFunctions::mCalcLineEdit* m_spbUpperCushion;
        MxFunctions::mCalcLineEdit* m_spbLowerLimitStiffness;
        MxFunctions::mCalcLineEdit* m_spbUpperLimitStiffness;
        MxFunctions::mCalcLineEdit* m_spbLowerLimitDamping;
        MxFunctions::mCalcLineEdit* m_spbUpperLimitDamping;
        QLabel* m_lblLowerLimit;
        QLabel* m_lblUpperLimit;
        QLabel* m_lblLowerCushion;
        QLabel* m_lblUpperCushion;
        QLabel* m_lblLowerLimitStiffness;
        QLabel* m_lblUpperLimitStiffness;
        QLabel* m_lblLowerLimitDamping;
        QLabel* m_lblUpperLimitDamping;

        // 6. 运动副额外参数 (螺纹副导程等)
        FoldSection* m_foldJointParams;

        // 点在面上副：约束曲面（几何面多选拾取控件，面ID直存数据类不挂映射）
        FoldSection* m_foldSurface = nullptr;
        mPickWidgetController* m_surfaceFaceController = nullptr;
        QLabel* m_lblFaceCount = nullptr;

        // 点在线上副：约束曲线（几何边多选拾取控件，边ID直存数据类不挂映射）
        FoldSection* m_foldCurve = nullptr;
        mPickWidgetController* m_curveEdgeController = nullptr;
        QLabel* m_lblEdgeCount = nullptr;

        // 线在线上副：约束曲线I（动作体侧，几何边多选）
        FoldSection* m_foldCurveI = nullptr;
        mPickWidgetController* m_curveIEdgeController = nullptr;
        QLabel* m_lblEdgeICount = nullptr;

        // 线在线上副：约束曲线J（基本体侧，几何边多选）
        FoldSection* m_foldCurveJ = nullptr;
        mPickWidgetController* m_curveJEdgeController = nullptr;
        QLabel* m_lblEdgeJCount = nullptr;

        QLabel* m_lblLead;
        MxFunctions::mCalcLineEdit* m_spbLead;

        //拾取高亮数据改变的连接
        QMetaObject::Connection conn_geo;
    };
}
