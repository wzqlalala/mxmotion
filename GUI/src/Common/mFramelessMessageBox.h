#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

/*
* 自绘标题栏消息框（mFramelessMessageBox）
*
* 背景：QMessageBox 是系统原生标题栏窗口，无法跟随主题换肤，与主窗/首选项/
* 各业务对话框的自绘标题栏风格割裂。全工程提示框统一改用本类：
*   - 静态接口与 QMessageBox 完全兼容（information/warning/critical/question/
*     about，含旧代码 Qt4 字符串按钮文本重载），调用点仅改类名即可；
*   - 标题栏/阴影/边缘缩放/最大化动画由 mFramelessDialog 基类提供，
*     与首选项窗口完全同款；
*   - 顺带提供 QInputDialog::getInt 兼容接口（表头固定行数等少量输入提示）。
*
* 内容区三形态（2026-11-05 用户圈定：原型形态只给曲线切换确认框，其余
* ~137 处调用点保持通用形态；转换设置输入框为 add-chart-type-extract-convert
* 新增第三形态，仅 getConvertCount 使用）：
*   - 通用形态（默认，prototypeStyle=false）：图标+文字一行、按钮盒右下，
*     边距/间距取 UI 档位公式；标题栏随窗口头默认档（三键）；
*   - 原型形态（prototypeStyle=true）：三段式——白标题栏 34 只留✕（logo
*     14×16、标题 13px/600）/ 正文灰带 85（图标 25+文字两行）/ 白按钮带 35
*    （按钮 44×26、距 11、右距 9），外框 400×156（原型供图系 1.5× 放大稿，
*     ÷1.5 实测标定，与白卡家族常数互证）。警告取入库橙色圆感叹号
*    （Icon/promptWarning.png，原型供图裁切），默认键=主按钮形态（蓝底白字，
*     挂 mxBtnNoTheme 豁免约定防主题重发覆盖）；
*   - 转换设置形态（convertInputStyle=true）：原型标题栏族同上但标题带自带
*     档 62 + 纯白正文一行（label 左、QSpinBox 右侧定宽 300×24）+ 取消/继续
*     右下（取消左/继续右，继续=主键），无图标无灰带，外框 482×174
*    （2026-11-06 新原型原生 1:1 设计稿实测，非放大稿折算）。
*
* 两形态共用（缺陷修复性质，与形态无关）：
*   - 模态期间置顶：PostCurve 弹窗族（打开曲线/模板曲线/图层管理等）统一
*     WindowStaysOnTopHint 常驻置顶，应用模态只禁输入不改层级——提示框不
*     置顶会被盖住点不了（假死）；
*   - 固定尺寸窗的原生首帧 +16/+39 收拢在 mFramelessWindowHelper（全局）。
*/

#include "mCommon_global.h"
#include "mFramelessDialog.h"

#include <QDialogButtonBox>
#include <QMessageBox>

class QHBoxLayout;
class QLabel;
class QSpinBox;

namespace MechMainWindow
{
	class COMMON_EXPORT mFramelessMessageBox : public mFramelessDialog
	{
		Q_OBJECT
	public:
		/*
		* 以下四组静态接口与 QMessageBox 同名重载一一对应（含默认参数），
		* 返回值语义相同：点击的标准按钮；Esc 返回 escape 按钮。
		* prototypeStyle=true 时按原型三段式装配（默认通用形态）。
		*/
		static QMessageBox::StandardButton information(QWidget* parent, const QString& title,
			const QString& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok,
			QMessageBox::StandardButton defaultButton = QMessageBox::NoButton,
			bool prototypeStyle = false);
		static QMessageBox::StandardButton warning(QWidget* parent, const QString& title,
			const QString& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok,
			QMessageBox::StandardButton defaultButton = QMessageBox::NoButton,
			bool prototypeStyle = false);
		static QMessageBox::StandardButton critical(QWidget* parent, const QString& title,
			const QString& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok,
			QMessageBox::StandardButton defaultButton = QMessageBox::NoButton,
			bool prototypeStyle = false);
		static QMessageBox::StandardButton question(QWidget* parent, const QString& title,
			const QString& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok,
			QMessageBox::StandardButton defaultButton = QMessageBox::NoButton,
			bool prototypeStyle = false);
		static void about(QWidget* parent, const QString& title, const QString& text,
			bool prototypeStyle = false);

		/*
		* Qt4 兼容重载：字符串按钮文本（旧代码 QMessageBox::warning(p,t,text,"确定")
		* 或完整 8 参形态 warning(p,t,text,b0,b1,b2,defaultNum,escapeNum)）。
		* 返回点击序号（0=button0Text，1=button1Text，2=button2Text），
		* Esc 返回 escapeButtonNumber。
		*/
		static int information(QWidget* parent, const QString& title, const QString& text,
			const QString& button0Text, const QString& button1Text = QString(),
			const QString& button2Text = QString(), int defaultButtonNumber = 0,
			int escapeButtonNumber = 1, bool prototypeStyle = false);
		static int warning(QWidget* parent, const QString& title, const QString& text,
			const QString& button0Text, const QString& button1Text = QString(),
			const QString& button2Text = QString(), int defaultButtonNumber = 0,
			int escapeButtonNumber = 1, bool prototypeStyle = false);
		static int critical(QWidget* parent, const QString& title, const QString& text,
			const QString& button0Text, const QString& button1Text = QString(),
			const QString& button2Text = QString(), int defaultButtonNumber = 0,
			int escapeButtonNumber = 1, bool prototypeStyle = false);

		/*
		* QInputDialog::getInt 兼容：整数输入提示框。ok 输出是否点击了确定。
		*/
		static int getInt(QWidget* parent, const QString& title, const QString& label,
			int value = 0, int minValue = -2147483647, int maxValue = 2147483647,
			int step = 1, bool* ok = nullptr);

		/*
		* 原型"转换设置"输入框（图表类型切换提取数值，add-chart-type-extract-convert）：
		* 第三形态——原型标题栏族（logo+标题+只留✕，带高自带档 62）+ 纯白正文
		* 一行（label 13px 左、QSpinBox 右侧定宽 300×24）+ 取消/继续右下（取消
		* 左/继续右，继续=蓝底主键 mxBtnNoTheme），无图标无灰带（区别于确认框
		* 三段式）。外框固定 482×174（2026-11-06 新原型原生 1:1 实测内框
		* 480×172；旧 400×145 折算口径作废，见 .cpp kConvert* 常数组注）。
		* ok 输出是否点击了"继续"；取消/✗/Esc 为 false（返回值取 value 原样）。
		*/
		static int getConvertCount(QWidget* parent, const QString& title, const QString& label,
			int value, int minValue, int maxValue, bool* ok = nullptr);

	private:
		mFramelessMessageBox(QWidget* parent, QMessageBox::Icon icon, const QString& title,
			const QString& text, QMessageBox::StandardButtons buttons,
			QMessageBox::StandardButton defaultButton, bool prototypeStyle = false,
			bool convertInputStyle = false);

		/*
		* 组装内容区（按形态分派）：通用=图标+文字一行+按钮盒右下；
		* 原型=白标题栏/灰带（图标+文字）/白按钮带三段式；
		* 转换设置=白标题栏/纯白正文一行（label+输入框）/白按钮带
		*/
		void buildPlainContent(QMessageBox::Icon icon, const QString& text,
			QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton);
		void buildPrototypeContent(QMessageBox::Icon icon, const QString& text,
			QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton);
		void buildConvertContent(const QString& label,
			QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton);

		/*
		* 弹出并取结果：相对父窗居中，模态 exec；原型形态另定固定尺寸
		*/
		QMessageBox::StandardButton execute(int stringButtonCount);

		/*
		* QMessageBox 标准按钮 → QDialogButtonBox 标准按钮映射
		*（两套枚举大量同值，但不保证全覆盖，逐位显式映射）
		*/
		static QDialogButtonBox::StandardButton mapStandardButton(QMessageBox::StandardButton b);

	private slots:
		void onButtonClicked(class QAbstractButton* button);

	private:
		QLabel* _iconLabel = nullptr;
		QLabel* _textLabel = nullptr;
		QSpinBox* _spinBox = nullptr;
		QHBoxLayout* _bodyLay = nullptr;   // 图标+文字行（getInt 再往文字列接输入框）
		QDialogButtonBox* _buttonBox = nullptr;
		QMessageBox::StandardButton _result = QMessageBox::Cancel;
		// Qt4 字符串按钮形态：0=未用（标准按钮形态）否则按钮总数
		int _stringButtons = 0;
		// 原型三段式形态开关（默认通用形态），见文件头注释
		bool _prototypeStyle = false;
		// 转换设置输入形态（getConvertCount 专用，同原型标题栏档+固定宽，正文无灰带）
		bool _convertInputStyle = false;
	};
}
