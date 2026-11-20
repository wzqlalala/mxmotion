#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mBasicEnum.h"

#include <vector>
#include <QStringList>

//QT
#include <QWidget>
#include <QListWidget>
#include <QListWidgetItem>
#include <QToolButton>
#include <QPushButton>
#include <QMdiSubWindow>

#include "mPanelBase.h"

namespace MechMainWindow
{
	//批量定义运动体面板：部件列表（全部/增加/减少/清空 + 视口拾取联动）+ 确定/取消。
	//"确定"对列表中每个部件 1:1 创建运动体（batchCreateMotionBodies）。
	//单独成面板而非在 mMotionBodyPanel 加模式位：批量会话无质量/质心/设置、不持
	//数据对象，且只有部件一种拾取会话（无质心控件互斥的接管/让位复杂度）
	class MMAINWINDOW_EXPORT mMotionBodyBatchPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mMotionBodyBatchPanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mMotionBodyBatchPanel();

		/*
		* 进入批量定义会话（清空列表、激活部件拾取）
		*/
		void newBatchPanel();

		/*
		* 退出会话（取消拾取、清理高亮，不建体）
		*/
		void uneditBatchPanel();

		/*
		* 批量创建运动体核心：每个部件一个运动体（1:1），运动体名=部件名，
		* 与存量或批内重名时追加 -2/-3；已属于其它运动体的部件跳过
		* （部件名记入 skippedNames）。返回实际创建个数。
		* 本面板"确定"与菜单栏"一键生成运动体"共用
		*/
		static int batchCreateMotionBodies(const std::vector<int>& partIds, QStringList* skippedNames = nullptr);

	private:

		// 部件列表操作（mPropertyPanel/mMotionBodyPanel 同款控件，纯列表操作不写映射）
		void slot_allPartBtn();
		void slot_addPartBtn();
		void slot_reducePartBtn();
		void slot_clearPartBtn();
		void slot_ok();
		void slot_cancel();

		// 视口拾取联动（纯部件会话简化版）：列表内容 = 拾取选择集
		void slot_finishedPick();
		void activatePartPick();
		void clearPartSelection();
		// 拾取选择集 ← 列表 差集同步（按钮改动列表后保持视口高亮一致）
		void syncPickSetToList();
		// 当前列表行的部件ID集合（名称→ID 解析，无效名跳过）
		std::vector<int> listPartIds() const;

		/*
		* 连接信号
		*/
		void connectSig();

		/*
		* 断开连接信号
		*/
		void disconnectSig();

	private slots:
		//切换视口（dev 版信号 currentViewportChanged(int)，同 mElementPanel/mSelectInfoWindow 适配口径）
		void slot_subWindowActivated(int viewportID);

	protected:
		// 列表 viewport 按下事件过滤：点列表任意位置（含空白处）即接管部件拾取会话
		bool eventFilter(QObject* obj, QEvent* ev) override;

	private:
		//总布局
		QVBoxLayout* m_mainLayout;

		// 部件列表（全部/增加/减少/清空，行内编辑+名称补全）
		QListWidget* m_listPart;
		QToolButton* m_btnAllPart;
		QToolButton* m_btnAddPart;
		QToolButton* m_btnReducePart;
		QToolButton* m_btnClearPart;

		// 确定/取消
		QPushButton* m_btnOk;
		QPushButton* m_btnCancel;

		//拾取高亮数据改变的连接
		QMetaObject::Connection conn_geo;

		// 上次拾取会话中的部件选择集（列表↔拾取集同步的比对基准）
		std::vector<int> m_lastPickPartData;
	};
}
