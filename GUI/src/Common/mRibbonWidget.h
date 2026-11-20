#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

/*
* Ribbon 基础控件（自 mMenuWidget.h 下沉共享层）
* 前处理菜单栏与各功能模块菜单贡献者（后处理/曲线等）共用的 Ribbon 页与选项组；
* 依赖仅 Qt 与 mCalcLineEdit（MxFunctions 接口头），不含任何业务模块头。
*/

#include "mCommon_global.h"

//QT
#include <QWidget>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QButtonGroup>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QSlider>
#include <QList>
#include <QSizePolicy>

#include <memory>
#include "mCheckComboBox.h"

namespace MxFunctions
{
	class mCalcLineEdit;
}

namespace MechMainWindow
{
	class mGroupWidget;
	class RibbonPage;

	/*
	* 组间分隔线覆盖层：透明浮层贴在 viewport 之上、所有组控件之下方鼠标穿透，
	* 画"满高"竖线（含页容器上下留白段）。覆盖层绘制在子控件之后，线不会被
	* 任何不透明子控件盖住——这是分隔线反复"消失/只剩一截"的最终解决路径
	*/
	class COMMON_EXPORT mRibbonSeparatorOverlay : public QWidget
	{
		Q_OBJECT
	public:
		mRibbonSeparatorOverlay(RibbonPage* page, QWidget* parent);
	protected:
		void paintEvent(QPaintEvent*) override;
	private:
		RibbonPage* m_page_ = nullptr;
	};

	/*
	* Ribbon 页：横向滚动容器，承载若干 mGroupWidget，超宽时显示左右翻页按钮
	*/
	class COMMON_EXPORT RibbonPage : public QScrollArea
	{
		Q_OBJECT
	public:
		/*
		* 构造函数
		*/
		RibbonPage(QWidget* parent = nullptr);

		/*
		* 析构函数
		*/
		~RibbonPage();

		/*
		* 返回所有Group的宽度，即整页的菜单长度
		*/
		int totalWidth();

		/*
		* 更新菜单栏的相关位置设置
		*/
		void updateMenuGeometry();

		/*
		* 保存group
		*/
		void appendGroup(mGroupWidget* group);

		/*
		* 供分隔线覆盖层读取组列表
		*/
		const QList<mGroupWidget*>& groups() const { return groupList; }

		QHBoxLayout* contentLayout() const;
		QWidget* contentWidget() const { return m_widget_; }
		void applyMetrics();



	protected:
		void resizeEvent(QResizeEvent* event)override;

		/*
		* 竖向滑轮防护：页高与内容最小高按准则严格相等，竖向滚动范围恒为 0；
		* 这里再显式忽略竖向滑轮，即使个别页混入最小高异常的子控件把范围撑回来，
		* 菜单栏也不会被滑轮上下移动（横向滑轮仍交由基类处理）
		*/
		void wheelEvent(QWheelEvent* event) override;

		/*
		* 绘图事件
		*/
		virtual void paintEvent(QPaintEvent* event) override;

	private Q_SLOTS:
		/*
		* 点击菜单栏左移
		*/
		void slot_LeftScrollButtonClicked();

		/*
		* 点击菜单栏右移
		*/
		void slot_RightScrollButtonClicked();
	private:
		// ScrollArea的页面
		QWidget* m_widget_ = nullptr;

		// 添加的所有Group
		QList<mGroupWidget*> groupList;

		// 左、右移动页面的按钮
		QToolButton* m_leftScrollBtn_ = nullptr;
		QToolButton* m_rightScrollBtn_ = nullptr;

		// 组间分隔线覆盖层（浮在组之上画满高竖线）
		mRibbonSeparatorOverlay* m_sepOverlay_ = nullptr;
		bool m_isRightScrollBtnShow_ = false;
		bool m_isLeftScrollBtnShow_ = false;

		// 显示控件的最大宽度
		int m_totalWidth_ = 0;

		// 滑动条最小和最大值
		int m_minScrollValue_;
		int m_maxScrollValue_;

		// 控制是左边按钮还是右边按钮显示
		int m_XBase_ = 0;
	};

	/*
	* Ribbon 选项组：标题 + 左侧自由布局 + 右侧网格布局，提供各类控件的添加接口
	*/
	class COMMON_EXPORT mGroupWidget : public QWidget
	{
		Q_OBJECT
	public:
		/*
		* 构造函数
		*/
		mGroupWidget(const QString& title, QWidget* parent = nullptr);

		/*
		* 析构函数
		*/
		~mGroupWidget();

		/*
		* 添加输入布局
		*/
		QBoxLayout* addBoxLayout(QBoxLayout* layout, int row = -1, int col = -1, int rowspan = 1, int columnspan = 1);

		/*
		* 添加普通按钮：文字+图标（返回全自绘按钮 mRibbonButton*，继承 QToolButton）
		*/
		QToolButton* addToolBtn(const QString& buttonName, const QString& icon, int row = -1, int col = -1, int rowspan = 1, int columnSpan = 1, QSize iconSize = QSize(12, 12), int extrawidth = 0);

		/*
		* 添加普通按钮：文字+图标+外部布局
		*/
		QToolButton* addToolBtn(const QString& buttonName, const QString& icon, QLayout* layout, QSize iconSize = QSize(12, 12), int extrawidth = 0);

		/*
		* 添加全高下拉按钮（规则 1.3 全高按钮 + 文字右侧箭头）：
		* 返回全自绘 mRibbonButton（Mode::Big，继承 QToolButton），箭头与文字共用基线，
		* 点击经 showMenu 按规则 1.0 定位展开 menu()；外部只需 setMenu 挂菜单
		*/
		QToolButton* addDropDownButton(const QString& buttonName, int row = -1, int col = -1);

		/*
		* 添加输入行编辑框
		*/
		QLineEdit* addLineEdit(QLineEdit* lineEdit, int row = -1, int col = -1, int rowspan = 1, int columnspan = 1);

		/*
		* 添加输入行编辑框
		*/
		MxFunctions::mCalcLineEdit* addCalcLineEdit(MxFunctions::mCalcLineEdit* lineEdit, int row = -1, int col = -1, int rowspan = 1, int columnspan = 1);

		/*
		* 添加文字label
		*/
		QLabel* addLabel(const QString& labelText, int row = -1, int col = -1, int rowspan = 1, int columnspan = 1);

		/*
		* 添加下拉框
		*/
        MxFunctions::mCheckComboBox* addCombobox(const QString& objectName, int row = -1, int col = -1, int rowspan = 1, int columnSpan = 1);

		/*
		* 添加buttonGroup\QRadioButton
		*/
		QButtonGroup* addButtonGroup(const QStringList& names, int row = -1, int col = -1);

		/*
		* 添加doubleSpinBox
		*/
		QDoubleSpinBox* addDoubleSpinBox(const QString& objectName, int row = -1, int col = -1, int rowspan = 1, int columnspan = 1);

		/*
		* 添加图标按钮
		*/
		QToolButton* addIconToolButton(const QString& objectName, const QString& icon, int row = -1, int col = -1);

		/*
		* 添加复选框
		*/
		QCheckBox* addCheckBox(const QString& name, int row = -1, int col = -1, int rowspan = 1, int columnspan = 1);

		/*
		* 添加滑动条
		*/
		QSlider* addSlider(const QString& name, int row = -1, int col = -1, int rowspan = 1, int columnspan = 1);

		/*
		* 添加文字按钮
		*/
		QPushButton* addPushButton(const QString& name, int row = -1, int col = -1, int rowspan = 1, int columnspan = 1);

		/*
		* 添加自定义控件
		*/
		QWidget* addWidget(QWidget* widget, int row = -1, int col = -1, int rowspan = 1, int columnspan = 1);

		/*
		* 整理布局，一列要对齐
		*/
		void updateLayout();

		/*
		* 是否在本组右缘绘制组间分隔线（由 RibbonPage::appendGroup 按是否为末组设置）
		*/
		void setDrawSeparator(bool on);
		bool drawsSeparator() const { return m_drawSeparator_; }

		/*
		* 同排按钮等宽开关：开启后 updateLayout 把网格内同一行 rowspan=1 的按钮
		* 统一为该行最宽一钮的宽度。帧数控制这类"单排文字按钮"组用它消除文字
		* 长短造成的宽度参差；默认关闭，不影响既有分组外观
		*/
		void setUniformRowButtons(bool on) { m_uniformRowButtons_ = on; }
		bool uniformRowButtons() const { return m_uniformRowButtons_; }

		/*
		* 按当前档位重设组的间距/内容区高/标题高。
		* activateNow=false 时只失效布局不激活：ThemeManager::applyMetrics 在
		* 整树收口阶段统一激活一趟，避免每组一次同步重排 × 多轮的切档卡顿
		*/
		void applyMetrics(bool activateNow = true);

		/*
		* 获取按钮对象
		*/
		QList<std::shared_ptr<QAbstractButton>> getButtons() { return m_buttons; };
	protected:
		/*
		* 绘图事件
		*/
		virtual void paintEvent(QPaintEvent* event);
	public:


		/*
		*放置按钮的布局
		*/
		QHBoxLayout* m_hLayout_;

		QHBoxLayout* m_L_hLayout_;
		QGridLayout* m_R_gridLayout_;

		/*
		*标题
		*/
		QLabel* m_groupTitleLabel_;

		/*
		* 所有的按钮（全自绘 mRibbonButton，经 QAbstractButton 基类持有）
		*/
		QList<std::shared_ptr<QAbstractButton>> m_buttons;

		/*
		* 是否绘制本组右缘的组间分隔线（末组为 false）
		*/
		bool m_drawSeparator_ = true;

		/*
		* 同排按钮等宽（见 setUniformRowButtons）
		*/
		bool m_uniformRowButtons_ = false;
	};
}
