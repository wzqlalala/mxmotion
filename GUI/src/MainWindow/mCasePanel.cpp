
#include "mCasePanel.h"
#include "ui_mCasePanel.h"
#include "mAppConfiguration.h"


#include "mPythonAgent.h"
#include "mPyScriptWriter.h"
#include "mPyScriptReader.h"

//Qt
#include <QComboBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include "mOtherFunction.h"

using namespace MechPreData;

namespace MechMainWindow
{
	mCasePanel::mCasePanel(QWidget *parent)
		: mPanelBase(parent)
	{
		_ui = new Ui::mCasePanel;
		_ui->setupUi(this);

		_caseTypeLabel = new QLabel();

		FoldSection* foldSection = new FoldSection("定义", parent);

		foldSection->addRow(new QLabel("物理类型"), new QLabel("结构"));
		foldSection->addRow(new QLabel("分析类型"), _caseTypeLabel);
		m_foldWidgets.append(foldSection);

		foldSection = new FoldSection("其他", parent);
		foldSection->addRow(new QLabel("其他"), new QLabel("其他"));
		m_foldWidgets.append(foldSection);

		for (auto foldWidget : m_foldWidgets)
		{
			_ui->verticalLayout->addWidget(foldWidget);
		}
		_ui->verticalLayout->addStretch();

		setCaseData("线性静态分析", 1);
	}

	mCasePanel::~mCasePanel()
	{
		if (_ui != nullptr)
		{
			delete _ui;
		}
	}

	void mCasePanel::setCaseData(QString name, int Type)
	{
		_caseTypeLabel->setText(MxFunctions::mNumberToString(Type));
	}

	
}