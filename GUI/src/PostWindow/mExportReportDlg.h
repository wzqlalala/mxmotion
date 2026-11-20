#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

#include <QWidget>
#include <QPoint>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QToolButton;

namespace PostData
{
	class mPostReportContext;
}

namespace PostWindow
{
	/*
	* "输出报告"对话框（Ribbon"报告"页-报告导出按钮，设计图01.1~01.3）：
	* 按曲线文档视图页分组、以临时离屏视图按统一标准尺寸重渲染图表截图为
	* 报告上下文（20261103-R10，不借界面卡片），经 PostIOFile
	* 内置渲染器导出 PPT/HTML/Word 报告。可选导出模板（默认模板=随程序发布
	* 的报告母版/自定义模板文件）；"将每个图表导出成单独图片"三格式通用
	* （勾选=每图窗一图，不勾=每视图页合成一张整页大图）。
	*/
	class MPOSTWINDOW_EXPORT mExportReportDlg : public QWidget
	{
		Q_OBJECT
	public:
		//单例弹出（应用级模态；重复调用置前）
		static void showInstance(QWidget* parent = nullptr);

	private:
		explicit mExportReportDlg(QWidget* parent = nullptr);

		void buildUi();

		//按视图页分组收集曲线文档全部图表的截图到报告上下文（图片粒度取
		//context.imagePerChart）：20261103-R10 起截图来源=临时离屏视图按统一
		//标准尺寸（1515x765）重渲染，不借界面卡片（零界面干扰、与卡片显示
		//状态无关）；无图表的页不出现，全部页无图表返回 false
		bool collectContext(PostData::mPostReportContext& context) const;

		//按文件过滤串取默认后缀（".pptx"/".html"/".docx"）
		static QString defaultSuffixForFilter(const QString& filter);

		//导出文件类型变化：模板行可用项（HTML 仅默认模板）与模板路径行联动
		void updateTemplateRow();

	private slots:
		void slot_browsePath();
		void slot_browseTemplate();
		void slot_formatChanged(int index);
		void slot_templateModeChanged(int index);
		void slot_export();

	protected:
		//无边框窗口拖动（按下点落在标题栏内才起拖）
		void mousePressEvent(QMouseEvent* event) override;
		void mouseMoveEvent(QMouseEvent* event) override;
		void mouseReleaseEvent(QMouseEvent* event) override;

	private:
		QWidget* _titleBar = nullptr;
		QComboBox* _formatCombo = nullptr;
		QComboBox* _templateCombo = nullptr;      //导出文件模板：默认模板/自定义
		QLineEdit* _templatePathEdit = nullptr;   //自定义模板文件路径（选"自定义"时可用）
		QToolButton* _templateBrowseBtn = nullptr; //自定义模板浏览按钮（随模板选择联动启停）
		QLineEdit* _pathEdit = nullptr;
		QCheckBox* _perImageCheck = nullptr;      //将每个图表导出成单独图片（三格式通用）
		QPoint _dragOffset;
	};
}
