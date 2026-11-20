#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include "mxr/data/SelectDataStruct.h"
#include "mBasicEnum.h"
#include "mPointPickerWidget.h"
#include "mCalcLineEdit.h"

#include <memory>
#include <set>
#include <vector>

//QT
#include <QWidget>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QLabel>
#include <QToolButton>
#include <QMdiSubWindow>

#include "mPanelBase.h"

namespace MechPreData
{
	class mViewportModel;
}
namespace MechMotionData
{
	class mMotionBody;
}
namespace MechMainWindow
{
	//运动体面板(包含数据)
	class MMAINWINDOW_EXPORT mMotionBodyPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mMotionBodyPanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mMotionBodyPanel();

		/*
		* 新建运动体
		*/
		void newMotionBodyPanel();

		/*
		* 编辑运动体(从树上点击过来的)
		*/
		void editMotionBodyPanel(int MotionBodyID = 0);

		/*
		* 取消编辑运动体
		*/
		void uneditMotionBodyPanel();
        /*
        * 数据对象即将被删除（树删除该类型前调用，此时对象仍有效）：置空编辑指针
        * 防悬垂——面板刷新链读已释放对象是未定义行为（衬套删除崩溃同源）
        */
        void onMotionBodyDataAboutToBeDestroyed(int id = -1);


	private:

        void performAutoCalculation();

        // 部件分配列表操作（截面属性面板 mPropertyPanel 同款：列表 + 全部/增加/减少/清空）
        void slot_allPartBtn();
        void slot_addPartBtn();
        void slot_reducePartBtn();
        void slot_clearPartBtn();
        // 部件变更统一收尾：自动模式重算 + 全量反显 + 通知刷新
        void finishPartAssignmentChanged();

        // 视口拾取选择集同步（截面属性面板 mPropertyPanel 同款）：
        // 拾取会话选择集与本面板部件集合双向互通，操作后刷新 _lastPickPartData
        void highlightPart(std::vector<int> ids);
        void unHighlightPart(std::vector<int> ids);
        void clearPartSelection();

        // 部件列表的"激活"接管：失活质心拾取控件（保留其值与临时高亮），
        // 恢复部件拾取模式并按当前数据重建选择集与比对基准——
        // 质心点拾取会整体替换共享容器（部件集被清），交还会话后必须重建
        void activatePartPick();

        // 部件列表激活态可视反馈（激活=红系边框，同 mMotionPanelColors 体色语义）：
        // 让用户能看出当前拾取会话归列表还是质心控件
        void setPartPickActiveVisual(bool active);

        // 挂起态临时高亮：部件以独立键 "MotionBodyPartTemp" 的额外渲染层重绘
        // （同拾取控件"被顶下保留临时高亮"语义），不进共享选中容器，
        // 质心点拾取整体替换容器时该层不受影响；接管/退出时清除
        void renderPartTempHighlight();
        void clearPartTempHighlight();


        void updateUIFromData();

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
		// 列表 viewport 按下事件过滤：点列表任意位置（含空白处）即接管部件拾取会话
		bool eventFilter(QObject* obj, QEvent* ev) override;

		//真正的运动体数据
        MechMotionData::mMotionBody* m_motionBodyData;

		//总布局
		QVBoxLayout* m_mainLayout;

        // --- 界面控件 ---
        // 1. 运动体对象（截面属性面板 mPropertyPanel 同款：已分配部件列表 + 右侧竖排四按钮）
        QListWidget* m_listAssignPart;   ///< 已分配部件列表（多选）
        QToolButton* m_btnAllPart;       ///< 全部部件（一键纳入全部部件）
        QToolButton* m_btnAddPart;       ///< 增加（行内编辑 + 名称自动补全选部件）
        QToolButton* m_btnReducePart;    ///< 减少（移除列表选中项）
        QToolButton* m_btnClearPart;     ///< 清空

        // 2. 质量属性
        QComboBox* m_cmbMassOption;///< 质量属性选项 (0: 自动计算, 1: 用户自定义)
        mPointPickerWidget* m_cmPicker; ///< 质心坐标通用拾取控件
        MxFunctions::mCalcLineEdit* m_leMass;///< 质量
        MxFunctions::mCalcLineEdit* m_leIxx;///< 惯性矩分量
        MxFunctions::mCalcLineEdit* m_leIyy;
        MxFunctions::mCalcLineEdit* m_leIzz;
        MxFunctions::mCalcLineEdit* m_leIxy;
        MxFunctions::mCalcLineEdit* m_leIxz;
        MxFunctions::mCalcLineEdit* m_leIyz;

        // 3. 设置
        QCheckBox* m_chkIsFixed;///< 不使用运动副而固定运动体
        MxFunctions::mCalcLineEdit* m_leInitVelX; ///< 初始线速度X（全局系）
        MxFunctions::mCalcLineEdit* m_leInitVelY; ///< 初始线速度Y
        MxFunctions::mCalcLineEdit* m_leInitVelZ; ///< 初始线速度Z
        MxFunctions::mCalcLineEdit* m_leInitOmegaX; ///< 初始角速度X（全局系，°/s）
        MxFunctions::mCalcLineEdit* m_leInitOmegaY; ///< 初始角速度Y
        MxFunctions::mCalcLineEdit* m_leInitOmegaZ; ///< 初始角速度Z

		//拾取高亮数据改变的连接
		QMetaObject::Connection conn_geo;

        // 上次视口拾取会话中的部件选择集（拾取增量/减量比对的基准，同 mPropertyPanel::_lastPickData）
        std::vector<int> m_lastPickPartData;
	};
}