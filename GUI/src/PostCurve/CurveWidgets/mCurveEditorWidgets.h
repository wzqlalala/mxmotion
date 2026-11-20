#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 属性页通用编辑控件
//
// 属性页中颜色/线型/勾选/数值等编辑形态在此集中提供：
//   - mCurveColorButton   ：实心色块按钮（按钮体即当前颜色），点击弹 QColorDialog
//   - mCurveFontButton    ：字体设置按钮（绘图选项同款），点击弹 QFontDialog 选择字体
//   - mCurveTextButton    ：文本按钮（回显当前文本，超宽省略号截断），点击弹"编辑字符"
//                           多行文本弹窗（白底标题栏+编辑区+取消/保存，参考软件同款）
//   - mCurvePenStyleCombo ：线型下拉（绘图选项曲线页同款图标）
//   - mCurveLineWidthCombo：线宽下拉（绘图选项曲线页同款 1~4px 线宽样本图标）
//   - mCurveSymbolStyleCombo：符号样式下拉（绘图选项曲线页同款 10 种符号图标）
//   - mCurveBorderStyleCombo：边框样式下拉（无 + 线型图标，图表编辑器"边框样式"行）
//   - mCurveBorderWidthCombo：边框线宽下拉（无 + 线宽样本图标，图例"边框线宽"行）
//   - mCurveCheckBox      ：勾选框，选中=蓝底方块+白色对勾（对勾自绘叠加）
//   - mCurveSciDoubleSpin ：科学计数法数字框（dB 参考值等极小值可键入 2e-11 如实保存回显）
//   - createCurveSpinBox / createCurveDoubleSpin：无上下调节按钮的数字框（对齐设计图）
//   - createCurveSciDoubleSpin：科学计数法数字框工厂（无上下调节按钮，同上形态）
// 它们都是"值部件"：提供取值/设值 + 用户变更信号，页面负责与模型双向同步。
// =============================================================================

#include "../PostCurveGlobal.h"

//QT
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QColor>
#include <QFont>
#include <QListWidget>
#include <QFrame>
#include <QValidator>
#include <QDoubleSpinBox>

class QSpinBox;
class QDoubleSpinBox;
class QPaintEvent;
class QResizeEvent;

namespace PostCurve
{
	//颜色选择按钮：按钮体为当前颜色的实心色块（填满控件列，与输入框等宽等高），
	//点击弹出 QColorDialog。注意区分两条赋值路径：
	//   setColor()      程序赋值（模型->控件回填），不发信号，避免回环；
	//   用户点击选色   发 colorChanged，页面 commit 到命令栈
	//
	//两种形态（B02，默认=实心色块 24x24，2D 各页行为零变化）：
	//   默认       setFixedSize(24,24) 小方块（构造即定，无需任何调用）；
	//   bar 模式   setBarMode(true)——通栏色条（宽度随表单拉伸、高度=按钮默认
	//              行高，同行输入框；整块填充+描边由 paintEvent 按 rect() 自
	//              适应，点击弹同款取色对话框），供需要"通栏色条"形态的页面
	//              显式启用（现消费方=三维注释页 文字颜色/背景颜色两行）
	class POSTCURVE_EXPORT mCurveColorButton : public QPushButton
	{
		Q_OBJECT
	public:
		explicit mCurveColorButton(const QColor& color = Qt::black, QWidget* parent = nullptr);

		QColor color() const;
		void setColor(const QColor& color);   //编程赋值（回填），不发射 colorChanged
		void setBarMode(bool on);             //通栏色条形态（默认 false=24x24 实心色块）

	signals:
		void colorChanged(const QColor& color);   //用户选择颜色后发射（同色不发）

	private slots:
		void onClicked();    //弹取色对话框，有效且不同的颜色才更新并发信号

	protected:
		void paintEvent(QPaintEvent* event) override;   //整块填充当前颜色+描边，hover 蓝边

	private:
		QColor _color;       //当前颜色
		bool _barMode = false;   //通栏色条形态（false=默认 24x24 实心色块）
	};

	//字体设置按钮（绘图选项同款形态与交互）：文字左对齐显示当前字体族名，
	//字体值等于"默认基准字体"时回显"默认"（基准默认为默认构造 QFont；模型自带
	//显式默认值的传该默认值，如图例 legendDefaultFont()——模型值本身不变，
	//仅显示按"默认"），点击弹 QFontDialog"选择字体"。与取色按钮同款双路径：
	//   setFontValue()   程序赋值（模型->控件回填），不发信号，避免回环；
	//   用户弹窗选字体   发 fontChanged，页面 commit 到命令栈。
	//不用 QFontComboBox：其内部行编辑器/QCompleter 在应用退出销毁阶段会触发
	//Qt 5.13 的 "No style available without QApplication" 断言（原属性页
	//字体族下拉同因弃用）
	class POSTCURVE_EXPORT mCurveFontButton : public QPushButton
	{
		Q_OBJECT
	public:
		explicit mCurveFontButton(QWidget* parent = nullptr);
		explicit mCurveFontButton(const QFont& unsetFont, QWidget* parent = nullptr);

		const QFont& fontValue() const { return _font; }
		void setFontValue(const QFont& font);   //编程赋值（回填），不发射 fontChanged

	signals:
		void fontChanged(const QFont& font);   //用户弹窗确认后发射

	private:
		void applyFont(const QFont& font);   //更新内部值与按钮显示（文字=族名并按该字体渲染）

		QFont _unsetFont;    //按"默认"回显的基准字体值（模型该值=未显式选择）
		QFont _font;         //当前字体
	};

	//文本编辑按钮（"编辑字符"）：文字左对齐回显当前文本（超宽按省略号截断，
	//完整文本进 tooltip），点击弹"编辑字符"多行文本弹窗（白底标题栏 logo+标题+
	//关闭、多行编辑区、右下"取消/保存"，参考软件同款形态——页眉/页脚文本、
	//图表表头等模板文本可能很长且含 {} 占位，参数栏行内不放输入框）。
	//与字体/取色按钮同款双路径：
	//   setTextValue()   程序赋值（模型->控件回填），不发信号，避免回环；
	//   用户弹窗保存     发 textCommitted，页面 commit 到命令栈（文本无变化不发，
	//                    防同值反复压撤回栈）
	class POSTCURVE_EXPORT mCurveTextButton : public QPushButton
	{
		Q_OBJECT
	public:
		explicit mCurveTextButton(QWidget* parent = nullptr);

		const QString& textValue() const { return _text; }
		void setTextValue(const QString& text);   //编程赋值（回填），不发射 textCommitted

	signals:
		void textCommitted(const QString& text);   //用户弹窗保存且文本有变化时发射

	protected:
		void resizeEvent(QResizeEvent* event) override;   //宽度变化时重算省略号回显

	private:
		void applyText(const QString& text);   //更新内部值并重算回显
		void updateDisplay();   //按当前宽度重算按钮回显（省略号截断+tooltip 全文）

		QString _text;       //当前完整文本（按钮显示的是截断形态）
	};

	//勾选框：指示器方块由 FoldSection 的 QSS 绘制（选中=蓝底），
	//白色对勾由 paintEvent 叠加——QSS 无法绘制对勾字形，也不依赖资源图标颜色。
	class POSTCURVE_EXPORT mCurveCheckBox : public QCheckBox
	{
		Q_OBJECT
	public:
		explicit mCurveCheckBox(QWidget* parent = nullptr);

	protected:
		void paintEvent(QPaintEvent* event) override;   //选中且可用时叠加白色对勾
	};

	//线型下拉：与绘图选项曲线页同款线型图标（项 :/mMainWidgets/Icon/NVH/ 下的
	//实线/虚线/点划线1/点划线2，纯图标无文字），索引语义（重要——两处枚举不同，
	//由各属性页负责换算）：
	//   曲线侧   index 0..3 直接映射 PlotView::TecLinePattern(0..3)
	//   基准线侧 index 0..3 需 +1 映射 Qt::PenStyle(1..4)
	class POSTCURVE_EXPORT mCurvePenStyleCombo : public QComboBox
	{
		Q_OBJECT
	public:
		explicit mCurvePenStyleCombo(QWidget* parent = nullptr);

		int penStyleIndex() const;             //0实线 1虚线 2点线 3点划线
		void setPenStyleIndex(int index);      //越界索引忽略
	};

	//线宽下拉：与绘图选项曲线页同款线宽样本图标（1px/2px/3px/4px），
	//索引 0..3 = 线宽 1..4（曲线页 kLineWidth 提交 idx+1，回填 idx=宽-1）
	class POSTCURVE_EXPORT mCurveLineWidthCombo : public QComboBox
	{
		Q_OBJECT
	public:
		explicit mCurveLineWidthCombo(QWidget* parent = nullptr);

		int lineWidthIndex() const;            //0..3 -> 1..4px
		void setLineWidthIndex(int index);     //越界索引忽略
	};

	//符号样式下拉：与绘图选项曲线页同款 10 种符号图标（标识无/添加(圆)/方/菱形/
	//三角/十字/倒三角/星形/交叉/圆点），索引直接对应模型 kSymbolStyle 0..9
	//（0无 1圆 2方 3菱 4三角 5十字 6倒三角 7星 8交叉 9点）
	class POSTCURVE_EXPORT mCurveSymbolStyleCombo : public QComboBox
	{
		Q_OBJECT
	public:
		explicit mCurveSymbolStyleCombo(QWidget* parent = nullptr);

		int symbolStyleIndex() const;          //0..9
		void setSymbolStyleIndex(int index);   //越界索引忽略
	};

	//边框样式下拉（线型语义，图表编辑器"边框样式"行用）：0无(∅) 1实线 2虚线
	//3划线(长虚线) 4点划线，1..4 项用线型图标，索引直接对应
	//CurveLegendData::borderStyle 与引擎 mPlotLegendItem::setBorderStyle
	class POSTCURVE_EXPORT mCurveBorderStyleCombo : public QComboBox
	{
		Q_OBJECT
	public:
		explicit mCurveBorderStyleCombo(QWidget* parent = nullptr);

		int borderStyleIndex() const;          //0无 1实线 2虚线 3划线 4点划线
		void setBorderStyleIndex(int index);   //越界索引忽略
	};

	//边框线宽下拉（宽度语义，图例参数栏"边框线宽"行用）：0无(∅，无边框)
	//1..4px，1..4 项用线宽样本图标，索引/值即线宽 px，提交
	//CurveLegendData::borderWidth（引擎 mPlotLegendItem::setBorderWidth）
	class POSTCURVE_EXPORT mCurveBorderWidthCombo : public QComboBox
	{
		Q_OBJECT
	public:
		explicit mCurveBorderWidthCombo(QWidget* parent = nullptr);

		int borderWidthIndex() const;          //0无 1..4px
		void setBorderWidthIndex(int index);   //越界索引忽略
	};

	//科学计数法数字框：在普通数字框交互上支持键入科学计数法（如 2e-11）并如实
	//保存回显。QDoubleSpinBox 原生校验不接受 'e'，且 setValue 会按 decimals 对值
	//四舍五入（4 位小数直接把 2e-11 舍成 0——模型 dbRef 默认值即因此回显成
	//"0.0000"），故重写三处：
	//   validate/valueFromText：接受 "±d.ddde±dd" 完整形态，输入途中的半成品
	//                           （"2e""2e-""1."等）给 Intermediate 便于继续键入；
	//   textFromValue         ：常规量级按显示精度定点回显（外观同普通数字框），
	//                           定点会丢量级的极小值转科学计数法并裁掉无效零
	//内部 decimals 固定 12 位存值，构造入参 decimals 语义为"显示精度"；
	//dB10/dB20 参考值（kDbRef，1e-12~1e9）即用此件。与普通数字框逐键提交
	//不同，本件关闭 keyboardTracking——键入/删除过程文本保持原样不发提交，
	//回车/失焦时才解释并 commit（防止逐键 commit 的模型回显把编辑中的
	//"2e-1"重写成定点 "0.2000"，见构造函数注释）
	class POSTCURVE_EXPORT mCurveSciDoubleSpin : public QDoubleSpinBox
	{
		Q_OBJECT
	public:
		explicit mCurveSciDoubleSpin(int displayDecimals, double min, double max,
			double singleStep = 1.0, QWidget* parent = nullptr);

	protected:
		QValidator::State validate(QString& text, int& pos) const override;
		double valueFromText(const QString& text) const override;
		QString textFromValue(double value) const override;

	private:
		double parsedValue(const QString& text) const;   //文本求值（失败返回 NaN）

		int _displayDecimals;    //显示精度（定点小数位/科学计数法尾数有效位）
	};

	//数字框工厂：隐藏上下调节按钮（设计图为纯输入框形态），键盘上下键/滚轮仍可调值
	POSTCURVE_EXPORT QSpinBox* createCurveSpinBox(int min, int max, QWidget* parent = nullptr);
	POSTCURVE_EXPORT QDoubleSpinBox* createCurveDoubleSpin(int decimals, double min, double max,
		double singleStep = 1.0, QWidget* parent = nullptr);
	//科学计数法数字框工厂（形态同上），入参语义见 mCurveSciDoubleSpin
	POSTCURVE_EXPORT mCurveSciDoubleSpin* createCurveSciDoubleSpin(int displayDecimals,
		double min, double max, double singleStep = 1.0, QWidget* parent = nullptr);

	//曲线标签模板变量表（显示名与模板片段一一对应，供各标签模板下拉共用）：
	//X/Y 侧来源信息占位符 + 计算值（Max Y / At X）；渲染求值待实现，
	//模板文本先入模型（kLabelText/kLabelPrefix/kLabelSuffix）
	POSTCURVE_EXPORT void curveLabelVariables(QStringList* names, QStringList* snippets);

	//图表类型下拉填充（图表属性编辑器弹窗与参数栏图表页共用，
	//文案取 chartTypeDisplayNames()）：0 XY曲线图 / 1 柱状图 / 2 复数图
	//已接渲染（切换经 kChartType 命令，图表全量重绘）；3 极坐标图 /
	//4 蛛网图 引擎暂无对应绘制能力，置灰占位防误选（tooltip 说明）
	POSTCURVE_EXPORT void setupChartTypeCombo(QComboBox* combo);

	//图表类型切换全流程（参考软件交互，属性编辑器弹窗与参数栏图表页共用）：
	//R30 起三维图（chartType=5）不参与类型切换——modelIdx==5 或 idx==5 一律
	//拒绝（idx≠modelIdx 时 QSignalBlocker 回退下拉后 return，无确认框、无曲线
	//清理、零提交）；其余类型：①程序回填（下拉与模型同值）或图表无曲线时
	//不弹窗直接切型；②窗口挂有曲线时弹"提示"确认框——"取消"回退下拉回显
	//不提交，"继续"将类型切换与清空全部曲线子节点（截图流程③，窗口变空图表）
	//放进同一撤销宏提交，Ctrl+Z 整批恢复
	POSTCURVE_EXPORT void applyChartTypeSwitch(QComboBox* combo, int chartNodeId, int idx);

	//曲线标签模板下拉（参考软件属性编辑器交互，截图03/曲线标签组）：
	//   - 点击下拉箭头：弹出勾选变量面板（顶部三个操作按钮：全选/全取消/互换），
	//     勾选项按列表顺序以"~"拼接成模板文本（如 "{y.MXRequest}~{y.MXComponent}"）；
	//   - 箭头前的编辑区可直接输入任意文本（可编辑下拉）；
	//   - setTemplateText 程序回填不发信号，并按"文本包含变量片段"回设勾选态；
	//     用户勾选/编辑后发 templateTextChanged，页面负责提交模型
	class POSTCURVE_EXPORT mCurveTemplateCombo : public QComboBox
	{
		Q_OBJECT
	public:
		explicit mCurveTemplateCombo(QWidget* parent = nullptr);

		//设置可勾选的模板变量（等长一一对应，顺序即拼接顺序）；
		//构造时已默认装配 curveLabelVariables，仅特殊下拉需要重设
		void setVariables(const QStringList& names, const QStringList& snippets);

		QString templateText() const;
		void setTemplateText(const QString& text);   //程序回填（不发射 templateTextChanged）

	signals:
		void templateTextChanged(const QString& text);   //用户勾选/编辑导致文本变化

	protected:
		void showPopup() override;   //拦截自带下拉列表，弹自绘勾选面板

	private:
		void rebuildText();            //按勾选顺序以"~"拼接片段，回写编辑区并发信号
		void syncCheckedFromText(const QString& text);   //按文本包含片段回设勾选态
		void setItemsChecked(Qt::CheckState state);      //全选/全取消公共实现（末尾统一重建文本）
		void checkAll();             //面板"全选"按钮
		void checkNone();            //面板"全取消"按钮
		void checkInvert();          //面板"互换"按钮（逐项取反）

		QListWidget* _list = nullptr;    //勾选面板内的变量列表（勾选态存 UserRole 片段）
		QFrame* _panel = nullptr;        //Qt::Popup 无边框面板
		bool _syncing = false;                 //程序拼装/回填期间屏蔽 itemChanged 联动
	};
}
