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
#include <QDoubleSpinBox>

#include "mPanelBase.h"

namespace MechMainWindow
{
	/*
	* 引用面板模板基类：按工况勾选引用对象（可编辑缩放系数）。
	* 筛选（类型/名称）、全部引用/全部取消仅作用于当前筛选结果。
	* HasScale为true时表格含"缩放系数"列（载荷/边界），false时不含（预定义场/接触/模型修改）。
	* 载荷/边界/预定义场/接触/模型修改五类仅数据通路不同，由Traits提供。
	* 本模板类代码全部在头文件实现，无需显式实例化文件。
	*/
	template<typename Traits, bool HasScale>
	class mReferencePanelTpl : public mPanelBase
	{
	public:
		explicit mReferencePanelTpl(QWidget* parent = Q_NULLPTR);
		~mReferencePanelTpl() override;

		/*
		* 显示指定工况的引用面板并刷新列表
		*/
		void showReference(int caseID);

	private:
		/*
		* 重建类型下拉框（全部类型+当前工况支持且实际存在的类型），保留原选中项
		*/
		void refreshTypeCombo();

		/*
		* 按当前筛选条件刷新表格
		*/
		void refreshList();

		/*
		* 设置行的引用状态（勾选、缩放系数与编辑按钮使能），数据层改动由dataChange控制
		*/
		void setRowReferenced(int row, bool referenced, bool dataChange);

		/*
		* 取行首列（名称）表格项，勾选状态存于该项
		*/
		QTableWidgetItem* nameItemOfRow(int row);

		void slot_typeFilterChanged();
		void slot_nameFilterChanged(const QString& text);
		void slot_tableItemChanged(QTableWidgetItem* item);
		void slot_referenceAllBtn();
		void slot_cancelAllBtn();

	private:
		//总布局
		QVBoxLayout* _mainLayout = nullptr;

		//折叠框：xxx引用
		FoldSection* _foldWidget = nullptr;

		//类型筛选（全部类型+当前工况可引用的类型）
		QComboBox* _comboType = nullptr;

		//名称筛选
		QLineEdit* _editNameFilter = nullptr;

		//对象表格：名称(复选框)/类型/[缩放系数]/操作(编辑)
		QTableWidget* _tableData = nullptr;

		//全部引用按钮
		QPushButton* _btnRefAll = nullptr;

		//全部取消按钮
		QPushButton* _btnCancelAll = nullptr;

		//当前工况id
		int _caseId = 0;

		//批量/程序化改动标志（防止setCheckState、setValue递归触发槽函数）
		bool _syncing = false;
	};
}

#include "mReferencePanelTpl.inl"
