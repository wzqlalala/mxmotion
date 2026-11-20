#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include "mxr/data/SelectDataStruct.h"
#include "mBasicEnum.h"

#include <memory>
#include <set>

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QLabel>
#include <QToolButton>
#include <QMdiSubWindow>

#include "mPanelBase.h"
#include "mPhysicalFieldEnum.h"
#include "mPickPointWidget.h"

using namespace MxFunctions;
using namespace MechCommon;
namespace MechPreData
{
	class mViewportModel;
}
namespace MechStructOptionData
{
	class mRunnerOptimizationDesignData;
}
namespace MechMainWindow
{

    using RunnerPickWidget = mPickPointWidget<MechStructOptionData::mRunnerOptimizationDesignData, QVector3D>;
    using RunnerPickMultiIdWidget = mPickMultiIdWidget<MechStructOptionData::mRunnerOptimizationDesignData>;

	//流道优化设计域面板
	class MMAINWINDOW_EXPORT mRunnerOptimizationDesignPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mRunnerOptimizationDesignPanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mRunnerOptimizationDesignPanel();

        /*
        * 创建流道优化设计域
        */
        void createRunnerOptimizationDesign(QString type = "实体");

		/*
		* 编辑流道优化设计域(从树上点击过来的)
		*/
		void editRunnerOptimizationDesignPanel(int id);

		/*
		* 取消编辑流道优化设计域
		*/
		void uneditRunnerOptimizationDesignPanel();

	private:

        /*
        * 点数据，设置颜色
        */
        void updatePointRender(QString str, QVector3D point, QVector3D color);

        /*
        *点数据，设置颜色
        */
        void updatePointRender(QString str, QVector<int> points, QVector3D color);

        /*
        * 获取拾取一堆节点
        */
        bool getPickPathData(QVector<int> &ids);

        //更新属性下拉框
        void updatePropertyCombobox(QComboBox* combobox);

        //更新设计域下拉框
        void updateDomainCombobox(QComboBox* combobox);

	private slots:

        /*
        * 截面属性改变
        */
        void slot_propertyChanged(const QString& text);

        /*
        * 对称约束复选框改变
        */
        void slot_checkPatternChanged(int state);

        /*
        * 对称约束类型改变
        */
        void slot_comboPatternChanged(const QString& text);

        /*
        * 拔模复选框改变
        */
        void slot_checkDraftChanged(int state);

        /*
        * 挤出复选框改变
        */
        void slot_checkExtrudeChanged(int state);

        /*
        * 挤出类型改变
        */
        void slot_comboExtrudeChanged(const QString& text);

        /*
        * 自支撑复选框改变
        */
        void slot_checkSupportChanged(int state);

        /*
        * 模式重复复选框改变
        */
        void slot_checkPatternRepChanged(int state);

        /*
        * 模式重复类型改变
        */
        void slot_comboPatternRepTypeChanged(const QString& text);

        /*
        * 模式重复参考设计域改变
        */
        void slot_comboPatternRepDomainChanged(const QString& text);

        /*
        * 模式重复坐标系改变
        */
        void slot_comboPatternRepCoordChanged(const QString& text);

	protected:

		//数据
        MechStructOptionData::mRunnerOptimizationDesignData* _optimizationDesignData;

		//总布局
		QVBoxLayout* _mainLayout;

        //设计域属性
        QComboBox* _comboDesignDomainType;  // 设计域类型下拉框
        QComboBox* _comboProperty;          // 截面属性下拉框

        // --- 达西惩罚项 UI 控件 ---
        QCheckBox* m_checkMaxPermeability{ nullptr }; ///< 最大渗透率开关
        mCalcLineEdit* m_lineMaxPermeability{ nullptr }; ///< 最大渗透率输入框

        QCheckBox* m_checkMinPermeability{ nullptr }; ///< 最小渗透率开关
        mCalcLineEdit* m_lineMinPermeability{ nullptr }; ///< 最小渗透率输入框

        // --- 设计域参数 UI 控件 ---
        QCheckBox* m_checkMinMember{ nullptr }; ///< 最小成员尺寸开关
        mCalcLineEdit* m_lineMinMember{ nullptr }; ///< 最小成员尺寸输入框

        QCheckBox* m_checkMaxMember{ nullptr }; ///< 最大成员尺寸开关
        mCalcLineEdit* m_lineMaxMember{ nullptr }; ///< 最大成员尺寸输入框

        QCheckBox* m_checkMinGap{ nullptr }; ///< 最小间隙开关
        mCalcLineEdit* m_lineMinGap{ nullptr }; ///< 最小间隙输入框

        // --- 对称约束组 UI 控件 ---
        QCheckBox* m_checkPattern{ nullptr }; // 对称状态复选框
        QComboBox* m_comboPattern{ nullptr }; // 对称方式下拉框
        PanelLabel* m_lblPatternNum{ nullptr }; // 周向对称数量 Label
        mCalcLineEdit* m_linePatternNum{ nullptr }; // 周向对称数量 输入框

        RunnerPickWidget* m_originWidget;
        RunnerPickWidget* m_dirPoint1Widget;
        RunnerPickWidget* m_dirPoint2Widget;

        // --- 拔模约束组 UI 控件 ---
        QCheckBox* m_checkDraftType{ nullptr };        ///< 拔模方式复选框
        QComboBox* m_comboDraftType{ nullptr };        ///< 拔模方式下拉框

        PanelLabel* m_lblNoHole{ nullptr };            ///< 无孔约束标签
        QComboBox* m_comboNoHole{ nullptr };           ///< 无孔约束下拉框

        RunnerPickWidget* m_startPointWidget{ nullptr };     ///< 起始点拾取组件
        RunnerPickWidget* m_draftDirWidget{ nullptr };       ///< 方向点拾取组件

        // ==========================================
        // 挤出约束组 UI 控件
        // ==========================================
        QCheckBox* m_checkExtrude{ nullptr };        ///< 挤出方式复选框
        QComboBox* m_comboExtrudeType{ nullptr }; ///< 挤出方式下拉框

        RunnerPickMultiIdWidget* m_path1Widget{ nullptr }; ///< 路径1拾取组件
        RunnerPickMultiIdWidget* m_path2Widget{ nullptr }; ///< 路径2拾取组件

        // ==========================================
        // 自支撑约束组 UI 控件
        // ==========================================
        QCheckBox* m_checkSupportStatus{ nullptr };      ///< 自支撑约束开启状态开关
        mCalcLineEdit* m_lineOverhangAngle{ nullptr };   ///< 悬垂角输入框

        RunnerPickWidget* m_supportStartWidget{ nullptr }; ///< 支撑起始点拾取组件
        RunnerPickWidget* m_supportDirWidget{ nullptr };   ///< 支撑方向点拾取组件

        // ==========================================
        // 模式重复组 UI 控件
        // ==========================================
        QCheckBox* m_checkPatternRepStatus{ nullptr };
        QComboBox* m_comboDomainType{ nullptr };
        PanelLabel* m_lblDomain{ nullptr };
        QComboBox* m_comboDomain{ nullptr };
        QComboBox* m_comboCoordType{ nullptr };

        // 拾取控件
        RunnerPickWidget* m_repBasePointWidget{ nullptr };
        RunnerPickWidget* m_repDirPoint1Widget{ nullptr };
        RunnerPickWidget* m_repDirPoint2Widget{ nullptr };
        RunnerPickWidget* m_repDirPoint3Widget{ nullptr };

        mCalcLineEdit* m_lineScaleX{ nullptr };
        mCalcLineEdit* m_lineScaleY{ nullptr };
        mCalcLineEdit* m_lineScaleZ{ nullptr };
 };
}
