#include "mPostGenerateAnimation.h"

#include <QFormLayout>
#include <algorithm>

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mDockLayoutManager.h"
#include "mPostInfoHub.h"

//MxFunctions
#include "mGlobalSignals.h"

//BasicData
#include "mPhysicalFieldEnum.h"

//Post
#include "mPostDataBase.h"

//窗口
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mOtherFunction.h"

using namespace MxFunctions;
using namespace MechCommon;
using namespace PostData;
using namespace MechPreData;

namespace PostWindow
{
	mPostGenerateAnimationPanel::mPostGenerateAnimationPanel(QWidget* parent) : mPanelBase(parent)
	{
		_curPostRend = nullptr;

		_mainLayout = new QVBoxLayout(this);
		this->setStyleSheet(
			"mPostAnimationPanel { background-color: white; }"

			"QComboBox QAbstractItemView {"
			"    background-color: white;"     /* 白底   */
			"    color: black;"                /* 黑字   */
			"    selection-background-color: #3399ff;" /* 悬停高亮，可自定 */
			"    selection-color: black;"      /* 悬停时文字保持黑色 */
			"}");

		/********************生成动画********************/
		FoldSection* generateFold = new FoldSection("生成动画", this);
		_comboCase = new QComboBox();
		_comboAniType = new QComboBox();
		_comboVarType = new QComboBox();
		_lineEditStart = new QLineEdit();
		_lineEditEnd = new QLineEdit();
		_buttonGenerate = new QPushButton("生成");
		QLabel* label = new QLabel("-");

		//工况
		generateFold->addRow(new PanelLabel("工况"), _comboCase);
		_comboCase->addItem("All");
		_comboCase->setEnabled(false);

		//类型
		generateFold->addRow(new PanelLabel("类型"), _comboAniType);
		QStringList animate = { "瞬态动画","线性动画","正弦动画" };
		_comboAniType->addItems(animate);
		_comboAniType->setCurrentIndex(0);

		//控制变量
		generateFold->addRow(new PanelLabel("控制变量"), _comboVarType);
		QStringList type = { "帧","时间"};
		_comboVarType->addItems(type);
		_comboVarType->setCurrentIndex(0);

		//帧/时间
		generateFold->addRow(new PanelLabel(""), _lineEditStart, label, _lineEditEnd);
		_lineEditStart->setAlignment(Qt::AlignCenter);
		_lineEditEnd->setAlignment(Qt::AlignCenter);
		QDoubleValidator* validator = new QDoubleValidator(this);
		_lineEditStart->setValidator(validator);
		_lineEditEnd->setValidator(validator);

		//生成
		generateFold->addRow(_buttonGenerate);
		connect(_buttonGenerate, SIGNAL(clicked()), this, SLOT(slot_generateAnimation()));
		
		generateFold->refreshLayout();
		m_foldWidgets.append(generateFold);
		_mainLayout->addWidget(generateFold);
		_mainLayout->addStretch();

		//connect
		connect(_comboAniType, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slot_chooseAniType(const QString&)));
		connect(_comboVarType, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slot_chooseVarType(const QString&)));
		connect(_lineEditStart, &QLineEdit::editingFinished, this, &mPostGenerateAnimationPanel::slot_setStartEdit);
		connect(_lineEditEnd, &QLineEdit::editingFinished, this, &mPostGenerateAnimationPanel::slot_setEndEdit);
	}

	void mPostGenerateAnimationPanel::newGenerateAniPanel(int caseid)
	{
		QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(caseid);
		if (postDB == nullptr) return;

		updatePanel();

		//填充帧信息
		QList<int> frameIDs = postDB->getFrameIds();
		int min = *std::min_element(frameIDs.begin(), frameIDs.end());
		int max = *std::max_element(frameIDs.begin(), frameIDs.end());
		_lineEditStart->setText(MxFunctions::mNumberToString(min));
		_lineEditEnd->setText(MxFunctions::mNumberToString(max));

		//存储时间信息
		int nframe = frameIDs.size();
		for (int i = 0; i < nframe; ++i)
		{
			double time = postDB->getFrameTime(i + 1);
			_frameTimes[i + 1] = time;
		}
		_startFrame = min;
		_endFrame = max;
		_startFrameTime = _frameTimes.value(_frameTimes.firstKey());
		_endFrameTime = _frameTimes.value(_frameTimes.lastKey());

		//创建动画节点
		MechCommon::mMechGlobalSignals::getInstance()->createAnimationSig(2);

		//禁用动画菜单栏
		MechCommon::mMechGlobalSignals::getInstance()->setAnimationMenuStatus(false);


		////生成动画
		//bool res = autoGenerateAnimation();

		////新建动画节点和视口
		////if (res)
		//{
		//	//删除动画节点
		//}
	}

	int mPostGenerateAnimationPanel::getClosestFrame(double inputTime)
	{
		if (_frameTimes.isEmpty())
			return -1;

		// 获取首帧和末帧
		int firstFrame = _frameTimes.firstKey();
		int lastFrame = _frameTimes.lastKey();
		double firstTime = _frameTimes.value(firstFrame);
		double lastTime = _frameTimes.value(lastFrame);

		if (inputTime <= firstTime)
			return firstFrame;
		if (inputTime >= lastTime)
			return lastFrame;

		// 正常查找
		int bestFrame = -1;
		double minDiff = std::numeric_limits<double>::max();

		for (auto it = _frameTimes.begin(); it != _frameTimes.end(); ++it)
		{
			double diff = std::abs(it.value() - inputTime);
			if (diff < minDiff)
			{
				minDiff = diff;
				bestFrame = it.key();
			}
			if (diff == 0.0)
				return bestFrame;
		}
		return bestFrame;
	}

	void mPostGenerateAnimationPanel::slot_setStartEdit()
	{
		QString start = _lineEditStart->text();
		if (_comboVarType->currentText() == "帧")
		{
			int startframe = start.toInt();
			if (_frameTimes.contains(startframe))
			{
				_startFrame = startframe;
				_startFrameTime = _frameTimes[_startFrame];
			}
			else
			{
				int frame = _frameTimes.firstKey();
				_lineEditStart->setText(MxFunctions::mNumberToString(frame));
				_startFrame = frame;
				_startFrameTime = _frameTimes[_startFrame];
			}
		}
		else if (_comboVarType->currentText() == "时间")
		{
			double inputTime = start.toDouble();
			int frame = getClosestFrame(inputTime);
			if (_frameTimes.contains(frame))
			{
				_startFrameTime = _frameTimes[frame];
				_startFrame = frame;
				_lineEditStart->setText(MxFunctions::mNumberToString(_startFrameTime));
			}
		}
	}

	void mPostGenerateAnimationPanel::slot_setEndEdit()
	{
		QString end = _lineEditEnd->text();
		if (_comboVarType->currentText() == "帧")
		{
			int endframe = end.toInt();
			if (_frameTimes.contains(endframe))
			{
				_endFrame = endframe;
				_endFrameTime = _frameTimes[_endFrame];
			}
			else
			{
				int frame = _frameTimes.lastKey();
				_lineEditEnd->setText(MxFunctions::mNumberToString(frame));
				_endFrame = frame;
				_endFrameTime = _frameTimes[_endFrame];
			}
		}
		else if (_comboVarType->currentText() == "时间")
		{
			double inputTime = end.toDouble();
			int frame = getClosestFrame(inputTime);
			if (_frameTimes.contains(frame))
			{
				_endFrameTime = _frameTimes[frame];
				_endFrame = frame;
				_lineEditEnd->setText(MxFunctions::mNumberToString(_endFrameTime));
			}
		}
	}

	void mPostGenerateAnimationPanel::slot_generateAnimation()
	{
		QString aniType = _comboAniType->currentText();
		QString varType, start, end;
		if (aniType == "瞬态动画")
		{
			varType = _comboVarType->currentText();
			start = _lineEditStart->text();
			end = _lineEditEnd->text();
		}

		mPostInfoHub::getInstance()->slot_generateAnimation(aniType, varType, start, end);
	}

	void mPostGenerateAnimationPanel::updateGenerateAnimationGUI(QString ani, QString var, QString start, QString end)
	{
		_comboAniType->blockSignals(true);
		_comboVarType->blockSignals(true);
		_lineEditStart->blockSignals(true);
		_lineEditEnd->blockSignals(true);

		_comboAniType->setCurrentText(ani);
		_comboVarType->setCurrentText(var);
		_lineEditStart->setText(start);
		_lineEditEnd->setText(end);

		_comboAniType->blockSignals(false);
		_comboVarType->blockSignals(false);
		_lineEditStart->blockSignals(false);
		_lineEditEnd->blockSignals(false);
	}

	void mPostGenerateAnimationPanel::updatePanel()
	{
		auto fold = m_foldWidgets[0];

		while (fold->layout()->rowCount() < 4)
		{
			fold->removeRow(2);

			_comboAniType->blockSignals(true);
			_comboAniType->setCurrentIndex(0);
			_comboAniType->blockSignals(false);

			_comboVarType = new QComboBox();
			_lineEditStart = new QLineEdit();
			_lineEditEnd = new QLineEdit();
			_buttonGenerate = new QPushButton("生成");

			QLabel* label = new QLabel("-");
			//控制变量
			fold->addRow(new QLabel("控制变量"), _comboVarType);
			QStringList type = { "帧","时间" };
			_comboVarType->addItems(type);
			_comboVarType->setCurrentIndex(0);

			//帧/时间
			fold->addRow(new QLabel(""), _lineEditStart, label, _lineEditEnd);
			_lineEditStart->setAlignment(Qt::AlignCenter);
			_lineEditEnd->setAlignment(Qt::AlignCenter);
			QDoubleValidator* validator = new QDoubleValidator(this);
			_lineEditStart->setValidator(validator);
			_lineEditEnd->setValidator(validator);
			_lineEditStart->setText(MxFunctions::mNumberToString(_startFrame));
			_lineEditEnd->setText(MxFunctions::mNumberToString(_endFrame));

			//生成
			fold->addRow(_buttonGenerate);
			connect(_buttonGenerate, SIGNAL(clicked()), this, SLOT(slot_generateAnimation()));

			connect(_comboVarType, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slot_chooseVarType(const QString&)));
			connect(_lineEditStart, &QLineEdit::editingFinished, this, &mPostGenerateAnimationPanel::slot_setStartEdit);
			connect(_lineEditEnd, &QLineEdit::editingFinished, this, &mPostGenerateAnimationPanel::slot_setEndEdit);
		
		}

		QTimer::singleShot(0, fold, [=]() {
			fold->refreshLayout();
		});
	}

	void mPostGenerateAnimationPanel::slot_chooseAniType(const QString& type)
	{
		auto fold = m_foldWidgets[0];
		if (type == "瞬态动画")
		{
			fold->removeRow(2);

			_comboVarType = new QComboBox();
			_lineEditStart = new QLineEdit();
			_lineEditEnd = new QLineEdit();
			_buttonGenerate = new QPushButton("生成");

			QLabel* label = new QLabel("-");
			//控制变量
			fold->addRow(new QLabel("控制变量"), _comboVarType);
			QStringList type = { "帧","时间" };
			_comboVarType->addItems(type);
			_comboVarType->setCurrentIndex(0);

			//帧/时间
			fold->addRow(new QLabel(""), _lineEditStart, label, _lineEditEnd);
			_lineEditStart->setAlignment(Qt::AlignCenter);
			_lineEditEnd->setAlignment(Qt::AlignCenter);
			QDoubleValidator* validator = new QDoubleValidator(this);
			_lineEditStart->setValidator(validator);
			_lineEditEnd->setValidator(validator);
			_lineEditStart->setText(MxFunctions::mNumberToString(_startFrame));
			_lineEditEnd->setText(MxFunctions::mNumberToString(_endFrame));

			//生成
			fold->addRow(_buttonGenerate);
			connect(_buttonGenerate, SIGNAL(clicked()), this, SLOT(slot_generateAnimation()));

			connect(_comboVarType, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slot_chooseVarType(const QString&)));
			connect(_lineEditStart, &QLineEdit::editingFinished, this, &mPostGenerateAnimationPanel::slot_setStartEdit);
			connect(_lineEditEnd, &QLineEdit::editingFinished, this, &mPostGenerateAnimationPanel::slot_setEndEdit);
		}
		else
		{
			fold->removeRow(4);
			fold->removeRow(3);
			fold->removeRow(2);

			_buttonGenerate = new QPushButton("生成");
			//生成
			fold->addRow(_buttonGenerate);
			connect(_buttonGenerate, SIGNAL(clicked()), this, SLOT(slot_generateAnimation()));
		}
	}

	void mPostGenerateAnimationPanel::slot_chooseVarType(const QString& type)
	{
		if (type == "帧")
		{
			_lineEditStart->setText(MxFunctions::mNumberToString(_startFrame));
			_lineEditEnd->setText(MxFunctions::mNumberToString(_endFrame));
		}
		else if (type == "时间")
		{
			_lineEditStart->setText(MxFunctions::mNumberToString(_startFrameTime));
			_lineEditEnd->setText(MxFunctions::mNumberToString(_endFrameTime));
		}
	}

}
