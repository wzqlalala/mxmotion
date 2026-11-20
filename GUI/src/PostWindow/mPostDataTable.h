#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
//自绘标题栏对话框基类：标题栏/阴影/边框缩放/最大化动画由基类提供
#include "mFramelessDialog.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;

namespace Ui
{
	class mPostDataTable;
}

namespace PostWindow
{
	class mPostDataTable : public MechMainWindow::mFramelessDialog
	{
		Q_OBJECT

	public:
		/** 默认构造函数 */
		mPostDataTable(QWidget *parent = Q_NULLPTR);

		/** 析构函数 */
		~mPostDataTable();

		/** 设置表格数据 */
		void setTableData(const QStringList& names, const QString& xText, QVector<QString> yText, const QVector<double>& xval, const QList<QPair<QString, QVector<double>>>& yvals);

		/** 设置表格数据 */
		void setTableData(const QStringList& names, const QString& xText, QVector<QString> yText, const QStringList& xval, const QList<QPair<QString, QVector<double>>>& yvals);

        /** 设置表格数据 */
        //核心功能	将多条具有公共 X 轴的曲线数据（宽表格式）填充到 QTableWidget 中，用于数据展示与对比。
        //    参数说明
        //    names	各曲线的名称（用作表格列标题）。
        //    xText	X 轴的物理含义 / 单位标签（用作第一列标题）。
        //    xval	公共的 X 轴数值序列（决定表格行数）。
        //    yvals	二维数组，外层索引对应不同曲线，内层索引对应 X 轴上的 Y 值。
        //    前置条件	yvals.size() 应与 names.size() 一致；yvals[i].size() 宜等于 xval.size()。
        //    后置效果	表格被清空并重新构建；首列为 X 轴数据，后续各列为曲线数据；表格自动调整列宽并设为只读。
		void setTableData(const QStringList& names, const QString& xText, const QVector<double>& xval, const QVector<QVector<double>>& yvals);
	private slots:
		/** 导出数据 */
		void slot_exportDatas();

		/** 翻译 */
		void slot_changeLanguage(int language);
	private:

		Ui::mPostDataTable*				_ui;			//UI
	};
}
