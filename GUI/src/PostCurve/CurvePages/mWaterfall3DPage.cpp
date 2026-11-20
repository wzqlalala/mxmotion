#include "mWaterfall3DPage.h"
#include "../CurveWidgets/mCurveFoldSection.h"
#include "../CurveWidgets/mCurveEditorWidgets.h"   //mCurveCheckBox（自身样式表兜底置灰）

//QT
#include <QLabel>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace PostCurve
{
	namespace
	{
		//页面公共骨架：页顶部加粗标题+纵向堆叠 FoldSection（mCurve3DPropPage/
		//mAnnotation3DPropPage 同款口径：边距/间距用布局默认值）
		QVBoxLayout* pageLayout(QWidget* host)
		{
			QVBoxLayout* layout = new QVBoxLayout(host);
			return layout;
		}

		//页顶部加粗标题（设计稿工具页每页顶部形态，如"瀑布图"）
		QLabel* titleLabel(const QString& text, QWidget* parent)
		{
			QLabel* label = new QLabel(text, parent);
			QFont f = label->font();
			f.setBold(true);
			label->setFont(f);
			return label;
		}

		//整行勾选框（mCurveCheckBox：自身样式表兜底 :disabled 文字置灰）
		QCheckBox* createRowCheck(const QString& text, QWidget* parent)
		{
			mCurveCheckBox* check = new mCurveCheckBox(parent);
			check->setText(text);
			return check;
		}

		//置灰占位勾选框（构造即禁用固定勾选态，不连 commit/信号、不进回填；
		//R31/R33/R34 同款工厂）
		QCheckBox* frozenRowCheck(const QString& text, bool checked, QWidget* parent)
		{
			QCheckBox* check = createRowCheck(text, parent);
			check->setChecked(checked);
			check->setEnabled(false);
			return check;
		}

		//置灰占位按钮（mAnnotation3DPropPage::frozenButton 同款：可收缩+置灰观感
		//自身样式表兜底——全局 QSS 有 QPushButton:disabled 规则，但按钮行需显式
		//minimumWidth(1) 覆盖 QSS width:80px 度量才允许窄面板压缩）
		QPushButton* frozenButton(const QString& text, QWidget* parent)
		{
			QPushButton* btn = new QPushButton(text, parent);
			btn->setEnabled(false);
			btn->setStyleSheet(QStringLiteral(
				"QPushButton { padding-left: 2px; padding-right: 2px; }"
				"QPushButton:disabled { background-color:#F5F7FA; color:#A8ABB2; border-color:#E4E7ED; }"));
			btn->setMinimumWidth(1);
			return btn;
		}

		//置灰占位下拉（单项静态值=设计稿截图值，不连 commit）
		QComboBox* frozenCombo(const QString& text, QWidget* parent)
		{
			QComboBox* combo = new QComboBox(parent);
			combo->addItem(text);
			combo->setEnabled(false);
			return combo;
		}

		//一行双控件容器（勾选框占左/值件占右同框；mAnnotation3DPropPage::pairRow
		//同款——边距归零不改 FoldSection 行高，行走 addRow(容器) 单参重载）
		QWidget* pairRow(QWidget* first, QWidget* second, QWidget* parent)
		{
			QWidget* host = new QWidget(parent);
			QHBoxLayout* lay = new QHBoxLayout(host);
			lay->setContentsMargins(0, 0, 0, 0);
			lay->setSpacing(MxFunctions::UiMetrics::rowGap());
			lay->addWidget(first);
			lay->addWidget(second, 1);
			return host;
		}

		//一行多按钮容器（mAnnotation3DPropPage::buttonRow 同款：等宽拉伸）
		QWidget* buttonRow(const QVector<QPushButton*>& btns, QWidget* parent)
		{
			QWidget* host = new QWidget(parent);
			QHBoxLayout* lay = new QHBoxLayout(host);
			lay->setContentsMargins(0, 0, 0, 0);
			lay->setSpacing(MxFunctions::UiMetrics::rowGap());
			for (QPushButton* btn : btns)
				lay->addWidget(btn, 1);
			return host;
		}
	}

	mWaterfall3DPage::mWaterfall3DPage(QWidget* parent)
		: QWidget(parent)
	{
		QVBoxLayout* layout = pageLayout(this);
		layout->addWidget(titleLabel(QStringLiteral("瀑布图"), this));

		//▾ 绘图类型（5 行全占位；下拉值=设计稿截图值静态写死）
		mCurveFoldSection* plotFold = new mCurveFoldSection(QStringLiteral("绘图类型"), this);
		QComboBox* plotTypeCombo = frozenCombo(QStringLiteral("频率(Frequency)"), this);
		plotFold->addRow(QStringLiteral("绘图类型"), plotTypeCombo);
		plotFold->setRowEnabled(plotTypeCombo, false);   //行标签一并置灰
		QComboBox* sliceAxisCombo = frozenCombo(QStringLiteral("时间"), this);
		plotFold->addRow(QStringLiteral("切片坐标"), sliceAxisCombo);
		plotFold->setRowEnabled(sliceAxisCombo, false);
		QComboBox* respCombo = frozenCombo(QStringLiteral("3D曲线 1"), this);
		plotFold->addRow(QStringLiteral("响应曲线"), respCombo);
		plotFold->setRowEnabled(respCombo, false);
		QComboBox* inputCombo = frozenCombo(QStringLiteral("无"), this);
		plotFold->addRow(QStringLiteral("输入曲线"), inputCombo);
		plotFold->setRowEnabled(inputCombo, false);
		plotFold->addRow(frozenRowCheck(QStringLiteral("输入矢量为每分钟转数"), false, this));

		//▾ 瀑布图切片（3 行全占位：勾选框+值件同框——勾选态与值=截图态）
		mCurveFoldSection* sliceFold = new mCurveFoldSection(QStringLiteral("瀑布图切片"), this);
		QSpinBox* countSpin = new QSpinBox(this);
		countSpin->setRange(0, 999999);
		countSpin->setValue(12);
		countSpin->setEnabled(false);
		sliceFold->addRow(pairRow(frozenRowCheck(QStringLiteral("切片数量"), true, this), countSpin, this));
		QDoubleSpinBox* stepSpin = new QDoubleSpinBox(this);
		stepSpin->setDecimals(2);
		stepSpin->setRange(0.0, 999999.0);
		stepSpin->setValue(0.01);
		stepSpin->setEnabled(false);
		sliceFold->addRow(pairRow(frozenRowCheck(QStringLiteral("切片步长"), false, this), stepSpin, this));
		sliceFold->addRow(frozenRowCheck(QStringLiteral("等值着色"), false, this));

		//▾ 参数（5 行全占位）
		mCurveFoldSection* paramFold = new mCurveFoldSection(QStringLiteral("参数"), this);
		QComboBox* sampleLenCombo = frozenCombo(QStringLiteral("64"), this);
		paramFold->addRow(QStringLiteral("采样长度"), sampleLenCombo);
		paramFold->setRowEnabled(sampleLenCombo, false);
		QComboBox* windowCombo = frozenCombo(QStringLiteral("汉窗(Hanning)"), this);
		paramFold->addRow(QStringLiteral("窗函数"), windowCombo);
		paramFold->setRowEnabled(windowCombo, false);
		QComboBox* ampCombo = frozenCombo(QStringLiteral("幅值谱"), this);
		paramFold->addRow(QStringLiteral("幅值类型"), ampCombo);
		paramFold->setRowEnabled(ampCombo, false);
		QComboBox* weightCombo = frozenCombo(QStringLiteral("无(None)"), this);
		paramFold->addRow(QStringLiteral("频率计权"), weightCombo);
		paramFold->setRowEnabled(weightCombo, false);
		paramFold->addRow(frozenRowCheck(QStringLiteral("展示全部功能"), false, this));

		//底部 [应用][撤销] 两钮（设计稿应用=主蓝钮；按占位纪律构造即禁用、
		//置灰观感自身样式表兜底，色样随 R31/R33/R34 先例统一置灰）
		QPushButton* applyBtn = frozenButton(QStringLiteral("应用"), this);
		QPushButton* undoBtn = frozenButton(QStringLiteral("撤销"), this);

		layout->addWidget(plotFold);
		layout->addWidget(sliceFold);
		layout->addWidget(paramFold);
		layout->addWidget(buttonRow({ applyBtn, undoBtn }, this));
		layout->addStretch(1);
	}
}
