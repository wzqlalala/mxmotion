#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

//QT
#include <QWidget>
#include <QVBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QPushButton>

#include "mPanelBase.h"

namespace MechMainWindow
{
	class mManagerTablePopupDialog;
	class mMenuWidget;

	/*
	* 管理器面板模板基类：对齐"载荷管理器原型"HTML——
	* 顶部蓝色"配置工况载荷"入口（点击弹出对象×工况勾选矩阵对话框），
	* 下方为筛选（名称/类型）+ 勾选列表（名称☑/类型/编辑）+ 右对齐的全选/删除按钮。
	* 载荷/边界/预定义场/接触/模型修改五类仅数据通路不同，由Traits提供。
	* 本模板类代码全部在头文件实现，无需显式实例化文件。
	*/
	template<typename Traits>
	class mManagerPanelTpl : public mPanelBase
	{
	public:
		explicit mManagerPanelTpl(QWidget* parent = Q_NULLPTR);
		~mManagerPanelTpl() override;

		/*
		* 显示管理器并刷新列表
		*/
		void showManager();

		/*
		* 供宿主注入菜单栏（弹出的矩阵按工况类型校验按钮可用性需要）
		*/
		void setMenuWidget(mMenuWidget* menuWidget) { _menuWidget = menuWidget; }

	private:
		/*
		* 重建类型下拉框（全部类型+当前模型中实际存在的类型），保留原选中项
		*/
		void refreshTypeCombo();

		/*
		* 按当前筛选条件刷新表格与全选按钮
		*/
		void refreshList();

		/*
		* 同步全选按钮文字：全部勾选时"取消全选"，否则"全选"；无勾选行时禁用删除按钮
		*/
		void syncActionButtons();

		/*
		* 取行首列（名称）表格项，勾选状态存于该项
		*/
		QTableWidgetItem* nameItemOfRow(int row);

		void slot_typeFilterChanged();
		void slot_nameFilterChanged(const QString& text);
		void slot_tableItemChanged(QTableWidgetItem* item);
		void slot_selectAllBtn();
		void slot_deleteBtn();
		void slot_matrixEntryBtn();

	private:
		//总布局
		QVBoxLayout* _mainLayout = nullptr;

		//折叠框：载荷列表
		FoldSection* _foldWidget = nullptr;

		//配置工况载荷入口按钮（蓝色，点击弹出矩阵对话框）
		QPushButton* _btnMatrixEntry = nullptr;

		//类型筛选（全部类型+实际存在的类型）
		QComboBox* _comboType = nullptr;

		//名称筛选
		QLineEdit* _editNameFilter = nullptr;

		//对象表格：名称(复选框)/类型/操作(编辑)
		QTableWidget* _tableData = nullptr;

		//全选按钮
		QPushButton* _btnSelectAll = nullptr;

		//删除按钮
		QPushButton* _btnDelete = nullptr;

		//配置工况矩阵弹出对话框（首次点击入口时创建，随面板销毁）
		mManagerTablePopupDialog* _matrixDialog = nullptr;

		//宿主注入的菜单栏（矩阵工况可用性校验需要，可为空）
		mMenuWidget* _menuWidget = nullptr;

		//联动勾选进行中标志（防止setCheckState递归触发itemChanged）
		bool _syncingCheck = false;
	};
}

#include "mManagerPanelTpl.inl"
