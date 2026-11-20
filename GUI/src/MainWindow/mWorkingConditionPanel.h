#pragma once
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

//BasicData
#include "mLoadBoundaryBase.h"
#include "mLoadBoundaryManager.h"

#include "mPredefinedFieldBase.h"
#include "mPredefinedFieldManager.h"

#include "mRoatateAnalyseBase.h"
#include "mRotateAnalyseManager.h"
#include "mRotateAnalysePanelBase.h"

#include "mLoadForce.h"
#include "mMixSet.h"

#include "mPanelBase.h"
#include "mLoadPanelBase.h"
#include "mLoadConvectionPanel.h"
#include "mLoadHeatFluxPanel.h"
#include "mLoadPressurePanel.h"
#include "mLoadRadiationPanel.h"
#include "mLoadSurfacePanel.h"
#include "mBoundaryPanelBase.h"
#include "mPredefinedFieldPanelBase.h"
#include <memory>

#include "mLoadHeatFlux.h"
#include "mLoadRadiation.h"
#include "mLoadConvection.h"

#include "mLoadTransient.h"
#include "mLoadFreqdomain.h"
#include "mLoadCombined.h"
#include "mLoadSpectrum.h"
#include "mLoadSurface.h"

#include "mLoadRotationalACC.h"
#include "mLoadRotationalSpeed.h"

#include "mBoundaryFixed.h"
#include "mBoundaryDisplacement.h"
#include "mBoundaryTemp.h"
#include "mBoundaryVirtual.h"

#include "mPredefinedFieldInitTemp.h"
#include "mPredefinedFieldTempLoad.h"

using namespace MechCaseData;
namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mWorkingConditionPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mWorkingConditionPanel(QWidget* parent = nullptr);

        /*
        * 析构函数
        */
        ~mWorkingConditionPanel();

        /*
        * 创建载荷
        */
        void createLoad(MxFunctions::LoadBoundaryType type);

        /*
        * 编辑载荷
        */
        void editLoad(const int& loadID, const int& caseId);

        /*
        * 取消载荷编辑
        */
        void unEditLoad();

        /*
        * 创建边界
        */
        void createBoundary(MxFunctions::LoadBoundaryType type);

        /*
        * 编辑边界
        */
        void editBoundary(const int& boudnaryID, const int& caseId);

        /*
        * 取消边界编辑
        */
        void unEditBoundary();

        /*
        * 创建预定义场
        */
        void createPredefinedField(MxFunctions::PredefinedFieldType type);

        /*
        * 编辑预定义场
        */
        void editPredefinedField(const int& fieldID, const int& caseId);

        /*
        * 取消预定义场编辑
        */
        void unEditPredefinedField();

        /*
        * 创建旋转分析
        */
        void createRotateAnalyse(MxFunctions::RotateAnalysyType type);

        /*
        * 编辑旋转分析
        */
        void editRotateAnalyse(const int& rotateAnalyseID, const int& caseId);

        /*
        * 取消旋转分析编辑
        */
        void unEditRotateAnalyse();

        /*
        * 重建温度载荷预定义场面板（定义类型改变时）
        */
        void rebuildTempLoadPanel();

        /*
        * 重建集中力，力矩面板（定义类型改变时）
        */
        void rebuildLoadPanel();

        /*
        * 重建边界面板（定义方式改变导致折叠框数量变化时）
        */
        void rebuildBoundaryPanel();

    private:
        /*
        * 载荷面板
        */
        void loadPanel(MxFunctions::LoadBoundaryType type, int caseId);

        /*
        * 边界面板
        */
        void boundaryPanel(MxFunctions::LoadBoundaryType type, int caseId);

        /*
        * 预定义场面板
        */
        void predefinedFieldPanel(MxFunctions::PredefinedFieldType type, int caseId);

        /*
        * 旋转分析面板
        */
        void rotateAnalysePanel(MxFunctions::RotateAnalysyType type, int caseId);

        /*
        * 连接信号
        */
        void connectSig(mLoadPanelBase* load);
        void connectSig(mPredefinedFieldPanelBase* field);
        void connectSig(mBoundaryPanelBase* boundary);

        /*
        * 断开信号
        */
        void disconnectSig(mLoadPanelBase* load);
        void disconnectSig(mPredefinedFieldPanelBase* field);
        void disconnectSig(mBoundaryPanelBase* boundary);

        /*
        * 创建内部集合
        */
        mMixSet* createSet();

        /*
        * 获取新的载荷名称
        */
        QString getNewLoadName(const QString& loadname);

        /*
        * 获取新的边界名称
        */
        QString getNewBoundaryName(const QString& boundaryname);

        /*
        * 获取新的预定义场名称
        */
        QString getNewPredefinedFieldName(const QString& fieldname);

        /*
        * 获取新的旋转分析名称
        */
        QString getNewRotateAnalyseName(const QString& rotateAnalyseName);

    public slots:
        /*
        * 温度载荷定义类型改变
        */
        void slot_onTempLoadDefineTypeChanged();

        /*
        * 初始温度定义方式改变（独立赋值不显示作用对象折叠框，需重建面板）
        */
        void slot_onInitTempDefineTypeChanged();

        /*
        * 集中力/力矩/压力定义方式改变（文件模式需重建面板）
        */
        void slot_onForceDefineTypeChanged();
        void slot_onTorqueDefineTypeChanged();
        void slot_onPressureDefineTypeChanged();
        void slot_onRotatePhysicalPowerDefineTypeChanged();
        void slot_onSurfaceDefineTypeChanged();
        void slot_onHeatFluxDefineTypeChanged();
        void slot_onRadiationDefineTypeChanged();
        void slot_onConvectionDefineTypeChanged();
        // 位移转角边界定义方式切换（独立赋值 <-> 各向分量 时重建折叠框）
        void slot_onBoundaryDisplacementDefineModeChanged();
        // 温度边界定义方式切换（统一赋值 <-> 分析场 时重建折叠框）
        void slot_onBoundaryTempDefineTypeChanged();

    private:
        //四类对象（载荷/边界/预定义场/旋转分析）共用本面板与同一 m_foldWidgets，
        //mPanelBase 的 _isEditing 仅为面板级总开关，切出时无法区分当前是哪类对象在编辑，
        //unEditInfoWindow 又会依次调用各 unEdit*，先被调用的会误清共享标志导致真正的
        //收尾函数被跳过（折叠框残留叠加），故按对象类型单独记录编辑状态
        enum class EditingType { None, Load, Boundary, PredefinedField, RotateAnalyse };

        int id = -1;
        QVBoxLayout* _mainLayout;
        EditingType _editingType = EditingType::None;

        std::shared_ptr<MechCaseData::mLoadBoundaryBase> _loadBoundaryData;
        std::shared_ptr<MechCaseData::mPredefinedFieldBase> _predefinedFieldData;
        std::shared_ptr<MechCaseData::mRoatateAnalyseBase> _rotateAnalyseData;
    };
}