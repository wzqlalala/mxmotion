#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mPanelBase.h"

#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QMdiSubWindow>
#include <QVBoxLayout>
#include <set>

namespace MechMotionData {
    class mMotionAutoFlex;
}

class QProcess;

namespace MechMainWindow { class mPickWidgetController; }

namespace MechMainWindow {

    /**
     * @brief 自动柔性体编辑面板
     */
    class MMAINWINDOW_EXPORT mMotionAutoFlexPanel : public mPanelBase {
        Q_OBJECT
    public:
        mMotionAutoFlexPanel(QWidget* parent = Q_NULLPTR);
        ~mMotionAutoFlexPanel();

        void newMotionAutoFlexPanel();
        void editMotionAutoFlexPanel(int id);
        void uneditMotionAutoFlexPanel();
        /*
        * 数据对象即将被删除（树删除该类型前调用，此时对象仍有效）：置空编辑指针
        * 防悬垂——面板刷新链读已释放对象是未定义行为（衬套删除崩溃同源）
        */
        void onMotionAutoFlexDataAboutToBeDestroyed(int id = -1);


    private:
        void updateUIFromData();
        void refreshBodyList();
        void refreshMaterialList();
        void launchMnfSolver(const QString& mxPath);   //【MNF 一键】mx→结构求解器→mnf
        //【附着节点】拾取控件读写：控件中全局节点 ID 集合 / ID 列表回显控件
        std::set<int> attachNodeIdsFromWidget() const;
        void showAttachNodesInWidget(const std::vector<int>& globalIds);
        //自动附着（GUI 数据侧）：ensureTetraMesh 兜底 + 运动副 origin 容差收组，
        //结果（全局 ID 平铺）回显控件并写回数据对象；outGlobalIds 返回供调用方
        bool autoAttachFromJoints(std::vector<int>& outGlobalIds);
        void setupDampingUI();

        void connectSig();
        void disconnectSig();

    private slots:
        void slot_save();
        void slot_cancel();
        void slot_exportMx();   //【MNF 离线缩减】当前条目 → .mx + 一键调结构求解器出 mnf
        void slot_subWindowActivated(QMdiSubWindow* subWindow);
        void slot_finishedPick();
        void slot_dampingTypeChanged(int index);
        void loadMaterialPropsToSpins();   //【材料直输】所选材料属性 → E/ν/ρ 直输框

    protected:
        MechMotionData::mMotionAutoFlex* m_flexData;

        QVBoxLayout* m_mainLayout;

        // 基础信息
        QComboBox* m_cmbTargetBody;
        QComboBox* m_cmbMaterial;
        //【材料直输】E/ν/ρ 绑定所选材料对象：选材自动带出，保存写回材料属性。
        // 用 mCalcLineEdit（表达式计算输入，无上下箭头，回车生效）
        MxFunctions::mCalcLineEdit* m_spbMatE = nullptr;
        MxFunctions::mCalcLineEdit* m_spbMatNu = nullptr;
        MxFunctions::mCalcLineEdit* m_spbMatRho = nullptr;
        QPushButton* m_btnPickAttach = nullptr;
        QLineEdit* m_edtAttachPoints = nullptr;   //兼容旧数据反显/序列化（主输入改拾取控件）
        //【附着节点拾取控件】同边界/集合面板的 mGeneralPickUp 通用控件：
        //可见节点 ID 列表、可增删；空时生成mx/保存自动按运动副收组并回显
        mPickWidgetController* m_attachPickWidget = nullptr;
        QCheckBox* m_chkEnabled;

        // 网格参数
        QDoubleSpinBox* m_spbElementSize;
        QComboBox* m_cmbMeshType;
        QDoubleSpinBox* m_spbMinElementSize;
        QCheckBox* m_chkCurvatureRefinement;

        // 模态分析参数
        QSpinBox* m_spbModeCount;
        QDoubleSpinBox* m_spbFreqCutoff;
        QComboBox* m_cmbSolverType;
        QDoubleSpinBox* m_spbFreqLower;
        QDoubleSpinBox* m_spbFreqUpper;
        QCheckBox* m_chkRigidBodyModeFilter;

        // 界面连接参数
        QComboBox* m_cmbConnectionType;
        QLineEdit* m_edtInterfaceDofs;
        QDoubleSpinBox* m_spbConnectionRadius;

        // 输出控制
        QCheckBox* m_chkOutputModeShapes;
        QCheckBox* m_chkOutputStressStrain;
        QComboBox* m_cmbOutputFormat;

        // 阻尼参数
        QComboBox* m_cmbDampingType;
        //参数行 label+编辑器成对（运动副面板同款）：类型切换 ensurePairInLayout
        //动态 addRow/setParent(nullptr)，不套 QWidget 行容器
        QLabel* m_lblModalDampingRatio;
        QDoubleSpinBox* m_spbModalDampingRatio;
        QLabel* m_lblRayleighMass;
        QDoubleSpinBox* m_spbRayleighMass;
        QLabel* m_lblRayleighStiff;
        QDoubleSpinBox* m_spbRayleighStiffness;
        FoldSection* m_foldDamping = nullptr;

        QMetaObject::Connection conn_geo;

        //【MNF 一键】结构求解器进程（mx→mnf；空=空闲。面板为常驻单例，进程随面板析构）
        QProcess* m_mnfProc = nullptr;
        QString m_mnfMxPath;   //当前求解对应的 mx 全路径（完成后在其目录找 mnf）
    };

}
