#include "mSectionQuery3DPage.h"
#include "../CurveWidgets/mCurveFoldSection.h"
#include "../CurveWidgets/mCurveEditorWidgets.h"   //mCurveCheckBox

//QT
#include <QLabel>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QSpinBox>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace PostCurve
{
	namespace
	{
		QVBoxLayout* pageLayout(QWidget* host)
		{
			QVBoxLayout* layout = new QVBoxLayout(host);
			return layout;
		}

		QLabel* titleLabel(const QString& text, QWidget* parent)
		{
			QLabel* label = new QLabel(text, parent);
			QFont f = label->font();
			f.setBold(true);
			label->setFont(f);
			return label;
		}

		QCheckBox* createRowCheck(const QString& text, QWidget* parent)
		{
			mCurveCheckBox* check = new mCurveCheckBox(parent);
			check->setText(text);
			return check;
		}

		QCheckBox* frozenRowCheck(const QString& text, bool checked, QWidget* parent)
		{
			QCheckBox* check = createRowCheck(text, parent);
			check->setChecked(checked);
			check->setEnabled(false);
			return check;
		}

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

		QComboBox* frozenCombo(const QString& text, QWidget* parent)
		{
			QComboBox* combo = new QComboBox(parent);
			combo->addItem(text);
			combo->setEnabled(false);
			return combo;
		}

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

	mSectionQuery3DPage::mSectionQuery3DPage(QWidget* parent)
		: QWidget(parent)
	{
		QVBoxLayout* layout = pageLayout(this);
		layout->addWidget(titleLabel(QStringLiteral("数据查询"), this));

		//▾ 截面管理（勾选行+空列表+四钮行全占位；按钮行=设计稿一行四钮，
		//第三钮文案"全选 / 清空"）
		mCurveFoldSection* sectionFold = new mCurveFoldSection(QStringLiteral("截面管理"), this);
		sectionFold->addRow(frozenRowCheck(QStringLiteral("显示截面"), true, this));
		QListWidget* sectionList = new QListWidget(this);
		sectionList->setEnabled(false);
		sectionList->setMinimumHeight(100);
		sectionFold->addRow(sectionList);
		QPushButton* addBtn = frozenButton(QStringLiteral("新增"), this);
		QPushButton* delBtn = frozenButton(QStringLiteral("删除"), this);
		QPushButton* clearAllBtn = frozenButton(QStringLiteral("全选 / 清空"), this);
		QPushButton* invertBtn = frozenButton(QStringLiteral("反选"), this);
		sectionFold->addRow(buttonRow({ addBtn, delBtn, clearAllBtn, invertBtn }, this));

		//▾ 截取定义（3 行全占位）
		mCurveFoldSection* defFold = new mCurveFoldSection(QStringLiteral("截取定义"), this);
		QComboBox* typeCombo = frozenCombo(QStringLiteral("阶次截取"), this);
		defFold->addRow(QStringLiteral("截取类型"), typeCombo);
		defFold->setRowEnabled(typeCombo, false);
		QSpinBox* orderSpin = new QSpinBox(this);
		orderSpin->setRange(0, 999999);
		orderSpin->setValue(1);
		orderSpin->setEnabled(false);
		defFold->addRow(QStringLiteral("阶次"), orderSpin);
		defFold->setRowEnabled(orderSpin, false);
		QDoubleSpinBox* tolSpin = new QDoubleSpinBox(this);
		tolSpin->setDecimals(2);
		tolSpin->setRange(0.0, 999999.0);
		tolSpin->setValue(0.15);
		tolSpin->setEnabled(false);
		defFold->addRow(QStringLiteral("容差"), tolSpin);
		defFold->setRowEnabled(tolSpin, false);

		//▾ 创建截面曲线（3 下拉+主钮全占位）
		mCurveFoldSection* createFold = new mCurveFoldSection(QStringLiteral("创建截面曲线"), this);
		QComboBox* xAxisCombo = frozenCombo(QStringLiteral("X"), this);
		createFold->addRow(QStringLiteral("曲线横坐标"), xAxisCombo);
		createFold->setRowEnabled(xAxisCombo, false);
		QComboBox* yAxisCombo = frozenCombo(QStringLiteral("Z"), this);
		createFold->addRow(QStringLiteral("曲线纵坐标"), yAxisCombo);
		createFold->setRowEnabled(yAxisCombo, false);
		QComboBox* windowCombo = frozenCombo(QStringLiteral("当前窗口"), this);
		createFold->addRow(QStringLiteral("创建窗口"), windowCombo);
		createFold->setRowEnabled(windowCombo, false);
		createFold->addRow(buttonRow({ frozenButton(QStringLiteral("创建截面曲线"), this) }, this));

		layout->addWidget(sectionFold);
		layout->addWidget(defFold);
		layout->addWidget(createFold);
		layout->addStretch(1);
	}
}
