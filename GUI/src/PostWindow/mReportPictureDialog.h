#pragma once

//自绘标题栏对话框基类：标题栏/阴影/边框缩放/最大化动画由基类提供
#include "mFramelessDialog.h"
#include <QListWidgetItem>
#include <QResizeEvent>

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


class QListWidget;
class QLabel;
class QLineEdit;
class QPushButton;
class QToolButton;

namespace PostWindow
{
	class mPostExportWordPanel;

	/*
	* 截图列表对话框（结构由原 UI/UI_ReportPictureDialog.h 内联，控件即成员，
	* 与 mPostNotesPanel 等面板保持一致风格）：
	* 左侧截图列表 + 上移/下移/删除/关闭工具行，右侧标题输入与预览。
	*/
	class mReportPictureDialog : public MechMainWindow::mFramelessDialog
	{
		Q_OBJECT

	public:
		/** 构造函数 */
		mReportPictureDialog(QWidget *parent = Q_NULLPTR);

		/** 更新截图列表 */
		void updatePictureList(const QStringList& names);

		/** 设置图片所在目录 */
		void setImageDir(const QString& dir);

		/** 获取图例名称 */
		QString getPictureLegendName(const QString& name);

	private:
		/** 重写resize函数 */
		void resizeEvent(QResizeEvent *event);

		/** 截图所在目录（带结尾分隔符） */
		QString legendImageDir() const;

	private slots:
		/** 单击截图列表 */
		void slot_clickPictureList(QListWidgetItem* item);

		/** 标题 */
		void slot_titleEdited(const QString& text);

        /** 上移 */
        void slot_moveUp();

        /** 下移 */
        void slot_moveDown();

        /** 删除 */
        void slot_delete();

        /** 中英文切换 */
        void slot_changeLanguage(int language);

	private:
		QListWidget*						_listWidget;		//截图列表
		QLabel*								_label;			//列表标题"截图列表"
		QToolButton*						_toolButton;	//上移
		QToolButton*						_toolButton_2;	//下移
		QToolButton*						_toolButton_3;	//删除
		QPushButton*						_pushButton;	//关闭
		QLabel*								_label_4;		//"标题"
		QLineEdit*							_lineEdit;		//图片标题输入
		QLabel*								_label_2;		//"预览"
		QLabel*								_label_3;		//预览图像

	       PostWindow::mPostExportWordPanel*	_wordReport;
		QString								_imageDir{};		//图片所在目录
	};
}
