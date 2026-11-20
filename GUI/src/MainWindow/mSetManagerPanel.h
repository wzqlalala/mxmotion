#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <memory>

//QT
#include <QWidget>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QStackedWidget>
#include <QPushButton>

#include "mPanelBase.h"

#include "mPhysicalFieldEnum.h"

namespace MechSetData
{
	class mSetBase;
}

namespace MechMainWindow
{
	//集合管理器面板：左键树上"集合"节点打开；筛选（类型/状态/名称）、勾选批量删除、重命名
	class MMAINWINDOW_EXPORT mSetManagerPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mSetManagerPanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mSetManagerPanel();

		/*
		* 显示集合管理器并刷新列表页
		*/
		void showSetManager();

	private slots:

		//类型筛选变化
		void slot_typeFilterChanged();

		//状态筛选变化
		void slot_stateFilterChanged();

		//名称筛选变化
		void slot_nameFilterChanged(const QString& text);

		//表格项数据变化：更新提示，并在多选场景下联动勾选
		void slot_tableItemChanged(QTableWidgetItem* item);

		//全选/取消全选
		void slot_selectAllBtn();

		//删除勾选的集合
		void slot_deleteBtn();

		//保存重命名
		void slot_saveRenameBtn();

		//从集合参数页返回列表页
		void slot_backToListBtn();

	private:

		/*
		* 集合是否为空（无拾取数据）
		*/
		bool isSetEmpty(MechSetData::mSetBase* set);

		/*
		* 集合是否未被使用（无任何功能映射引用）
		*/
		bool isSetUnused(int setID);

		/*
		* 集合成员数量（按类型的拾取/参数数量）
		*/
		int getSetMemberCount(MechSetData::mSetBase* set);

		/*
		* 集合状态文本：空集合/未使用/已使用
		*/
		QString getSetStateText(MechSetData::mSetBase* set, int setID);

		/*
		* 集合类型中文名
		*/
		QString getSetTypeNameByType(MxFunctions::SetType type);

		/*
		* 按当前筛选条件刷新表格与数量提示
		*/
		void refreshSetList();

		/*
		* 进入指定集合的参数页（重命名）
		*/
		void showRenamePage(int setID);

		/*
		* 同步全选按钮文字：全部勾选时"取消全选"，否则"全选"
		*/
		void syncSelectAllBtnText();

		/*
		* 取行首列（集合名称）表格项，勾选状态存于该项
		*/
		QTableWidgetItem* nameItemOfRow(int row);

	private:

		//总布局
		QVBoxLayout* _mainLayout;

		//列表页/集合参数页切换
		QStackedWidget* _stackedWidget;

		//===== 列表页 =====

		//列表页容器
		QWidget* _listPage;

		//折叠框：集合管理器
		FoldSection* _foldWidget;

		//类型筛选（全部类型+各集合类型）
		QComboBox* _comboType;

		//状态筛选（全部状态/未使用/空集合）
		QComboBox* _comboState;

		//名称筛选
		QLineEdit* _editNameFilter;

		//集合表格：集合名称(复选框)/类型/操作(编辑)
		QTableWidget* _tableSets;

		//全选按钮
		QPushButton* _btnSelectAll;

		//删除按钮
		QPushButton* _btnDelete;

		//===== 集合参数页（重命名） =====

		//参数页容器
		QWidget* _renamePage;

		//标题：集合参数
		QLabel* _labelRenameTitle;

		//返回列表按钮
		QPushButton* _btnBack;

		//集合名称（可编辑）
		QLineEdit* _editRenameName;

		//类型（只读）
		QLineEdit* _editRenameType;

		//状态（只读）
		QLineEdit* _editRenameState;

		//成员数量（只读）
		QLineEdit* _editRenameCount;

		//保存重命名按钮
		QPushButton* _btnSaveRename;

		//重命名提示（名称为空/已存在）
		QLabel* _labelRenameTip;

		//参数页当前集合ID
		int _renameSetID = -1;

		//联动勾选进行中标志（防止setCheckState递归触发itemChanged）
		bool _syncingCheck = false;
	};
}
