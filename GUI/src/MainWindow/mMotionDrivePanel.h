#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include "mxr/data/SelectDataStruct.h"
#include "mBasicEnum.h"
#include "mPhysicalFieldEnum.h"
#include "mMotionDriveBase.h"

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
#include <QComboBox>
#include <QStackedLayout>
#include <QPushButton>
#include <QTableWidget>

#include "mPanelBase.h"
#include "mCalcLineEdit.h"

namespace MechPreData
{
	class mViewportModel;
}
namespace MechMotionData
{
    class mMotionDriveBase;
}
namespace MechMainWindow
{
	class mMotionExprBuilderWidget;

	//驱动面板
	class MMAINWINDOW_EXPORT mMotionDrivePanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mMotionDrivePanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mMotionDrivePanel();

		/*
		* 新建驱动（mode：预设控制模式，菜单"目标×模式"入口使用；加速度模式面板暂不支持）
		*/
		void newMotionDrivePanel(MxFunctions::DriveTargetType type, MechMotionData::DriveControlMode mode = MechMotionData::DriveControlMode::Velocity);

		/*
		* 编辑驱动(从树上点击过来的)
		*/
		void editMotionDrivePanel(int id = 0);

		/*
		* 取消编辑驱动
		*/
		void uneditMotionDrivePanel();
        /*
        * 数据对象即将被删除（树删除该类型前调用，此时对象仍有效）：置空编辑指针
        * 防悬垂——面板刷新链读已释放对象是未定义行为（衬套删除崩溃同源）
        */
        void onMotionDriveDataAboutToBeDestroyed(int id = -1);


	private:

        void refreshTargetList(MxFunctions::DriveTargetType type);

        //按模板类型刷新参数区标签与可见性
        void refreshTemplateParams();

        //按模板参数生成表达式并写入数据
        void applyTemplate();

        //按方案求解时长采样表达式并画预览曲线
        void previewExpressionCurve(const QString& expr);

        //从表格控件收集数据：合法行按时间排序写入驱动，非法/不足提示
        void syncDriveTableFromWidget();

        //表格预览：按方案时长采样插值画曲线
        void previewDriveTable();

        //按数据还原表格控件
        void restoreTableWidget();

        void updateUIFromData();
        void refreshExcitationUnitLabel();   ///< 激励数值单位标签（°/°每秒/mm 系）随方向与控制模式刷新

        void autoLockMotionType(int targetId);

        /**
         * @brief 触发三维视口高亮
         */
        void triggerHighlight();

        //专门处理类型切换与数据迁移的核心逻辑
        void handleTargetTypeChange(MxFunctions::DriveTargetType newType);

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
	       MechMotionData::mMotionDriveBase* m_motionDriveData;

	       // 上次高亮的运动副 ID，用于清除旧符号
	       int m_lastTargetJointId = -1;

		//总布局
		QVBoxLayout* m_mainLayout;

        // ==================== 界面控件 ====================
        // 1. 驱动目标层
        QComboBox* m_cmbTargetType;     ///< [运动副驱动 / 运动体驱动]
        QComboBox* m_cmbTargetObject;   ///< 具体绑定的 ID 列表

        // 2. 物理语义层
        QComboBox* m_cmbMotionType;     ///< [平动 / 转动]
        QComboBox* m_cmbControlMode;    ///< [位置 / 速度 / 加速度]

        // 3. 驱动激励层
        QComboBox* m_cmbInputType;      ///< [常数值 / 函数表达式 / 函数模板]
        QStackedLayout* m_stkInputMode; ///< 切换激励输入
        MxFunctions::mCalcLineEdit* m_spbConstant; ///< 常数表达式输入框
        mMotionExprBuilderWidget* m_exprBuilder;  ///< 表达式构造器（公用控件）

        // 函数模板页（参数化生成表达式，落库仍是表达式字符串）
        QComboBox* m_cmbTemplate;       ///< 正弦/余弦/斜坡/阶跃/多项式

        // 表格驱动页（时间-值两列编辑器）
        QTableWidget* m_tblDrive;       ///< 时间-值表格
        QPushButton* m_btnTblAddRow;    ///< 添加行
        QPushButton* m_btnTblDelRow;    ///< 删除行
        QPushButton* m_btnTblPreview;   ///< 表格曲线预览
        QLabel* m_lblTblHint;           ///< 表格校验/提示
        QLabel* m_lblExcitationUnit = nullptr; ///< 激励数值单位标签（随方向/控制模式切换）
        MxFunctions::mCalcLineEdit* m_tplParam[4];   ///< 模板参数
        QLabel* m_lblTplParam[4];       ///< 参数标签（随模板类型更新）
        QLabel* m_lblTemplateExpr;      ///< 生成的表达式（只读展示）
        QPushButton* m_btnPreviewTpl;   ///< 模板曲线预览

		//拾取高亮数据改变的连接
		QMetaObject::Connection conn_geo;
	};
}
