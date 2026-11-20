#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

//坐标拾取复合控件（自 mMeshPanel.h 析出）：前处理 25 个网格面板与
//后处理 路径结果/切面 面板共用；纯头内联实现，随消费方编译
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QString>
#include <vector>
#include <utility>

#include "mPanelBase.h"     //PanelLabel
#include "mBasicEnum.h"     //MxFunctions::PickDataType
#include "mModelStruct.h"   //RenderPickResultData

namespace MechMainWindow
{
	struct PosPickWidget1
	{
		QLabel* labelname = nullptr;
		QLabel* labelpicked = nullptr;
		QPushButton* btnok = nullptr;
		QPushButton* btnclear = nullptr;
		QPushButton* btnadd = nullptr;
		QHBoxLayout* layout = nullptr;
		QWidget* propwid = nullptr;
		QString name_;
		std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>nodes_;
		std::vector<std::pair<MxFunctions::PickDataType, int>>nodes_id;
		bool _picked = false;
		void setState(bool picked)
		{
			_picked = picked;
			if (picked)
			{
				labelpicked->setStyleSheet("color: black; font-weight: normal;");
		              labelpicked->setText("已选择");
			}
			else
			{
				labelpicked->setStyleSheet("color: red; font-weight: bold;");
				labelpicked->setText("当前未选择");
			}
		}
		void initWidget(const QString& name, bool hasadd = false)
		{
			name_ = name;
			labelname = new PanelLabel();
			labelpicked = new QLabel();
			btnok = new QPushButton();
			btnclear = new QPushButton();
			btnadd = new QPushButton();
			layout = new QHBoxLayout();
			propwid = new QWidget();

			labelname->setText(name);
			labelpicked->setMinimumWidth(160);
			labelpicked->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
			btnok->setFixedWidth(26);
			btnclear->setFixedWidth(26);
			btnadd->setFixedWidth(26);
			btnok->setIcon(QIcon("")); btnok->setToolTip(name); btnok->setIcon(QIcon(":/mMainWidgets/Icon/tihuan3x.png"));
			btnclear->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png")); btnclear->setToolTip("清空选择");
			btnadd->setIcon(QIcon("")); btnadd->setToolTip("添加" + name.mid(2)); btnadd->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
			layout->setMargin(0);
			layout->setSpacing(10);
			layout->addWidget(labelname);
			layout->addWidget(labelpicked);
			layout->addWidget(btnok);
			layout->addWidget(btnadd);
			layout->addWidget(btnclear);

			propwid->setMinimumWidth(250);
			propwid->setWindowFlags(Qt::Widget);
			propwid->setContentsMargins(0, 0, 0, 0);
			propwid->setLayout(layout);
		}
		void hideAddBtn(bool hide)
		{
			btnadd->setVisible(!hide);
		}
		void clear()
		{
			nodes_.clear();
			if (labelname)
			{
				delete labelname;
				labelname = nullptr;
			}
			if (labelpicked)
			{
				delete labelpicked;
				labelpicked = nullptr;
			}
			if (btnok)
			{
				delete btnok;
				btnok = nullptr;
			}
			if (btnclear)
			{
				delete btnclear;
				btnclear = nullptr;
			}
			if (layout)
			{
				delete layout;
				layout = nullptr;
			}
			if (propwid)
			{
				delete propwid;
				propwid = nullptr;
			}
		}
	};
}
