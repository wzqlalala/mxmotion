#include "mPostSectionForce.h"
#include "mFbdColorPicker.h"
#include "mPostObjectEditors.h"
#include "mPostDataManager.h"
#include "mPostDataBase.h"
#include "mPostModelData.h"
#include "mPostGUIData.h"
#include "mIMxdbFile1.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"
#include "mPostGeneralPickUp.h"		//mPost 拾取栈工厂（单元/节点/求和节点拾取条）
#include "mChoiceGroup.h"
#include "mPrimaryButton.h"
#include "mComboActionRow.h"
#include "mAxisCoordRow.h"
#include "mAdvancedOptionsButton.h"
#include "mPanelTable.h"
#include "PickCommonFunction.h"
#include "UiMetrics.h"		//MxFunctions::UiMetrics::fieldInset（折叠组左右缝隙随共享档位）
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/label/mSymbolController.h"
#include "mxr/data/SelectDataStruct.h"

#include <QLineEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QToolButton>
#include <QTimer>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QStackedLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QDoubleValidator>
#include <QIntValidator>
#include <QItemSelectionModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QTextEdit>
#include <QRegularExpression>
#include <QScreen>
#include <QGuiApplication>
#include <QMouseEvent>
#include <set>
#include <functional>
#include <algorithm>

using namespace MechPreData;
using namespace PostData;

namespace
{
	//“通用结构软件界面UI设计规则”标准档令牌
	const char* kText       = "#424B57";	//正文
	const char* kMuted      = "#788391";	//辅助文字
	const char* kDisabled   = "#A6AFBB";	//禁用文字
	const char* kCtrlBorder = "#DCE2E9";	//控件描边
	const char* kPrimary    = "#1890FF";	//主操作蓝
	const char* kSelBg      = "#E8F3FF";	//选中底色
	const char* kSelText    = "#176BA6";	//选中文字
	const char* kHoverBg    = "#F0F7FF";	//悬停底色
	const char* kDanger     = "#D95151";	//删除红

	//显示设置力/力矩分量默认色（初始化与恢复默认共用）
	const QColor kDsCompColors[] =
		{ QColor("#FF0000"), QColor("#00CC00"), QColor("#0000FF"), QColor("#FF00FF"),
		  QColor("#FFFFFF"), QColor("#00FFFF"), QColor("#FFFF00") };

	//显示设置分量组位掩码（X/Y/Z/X+Y/Y+Z/Z+X/X+Y+Z；X=1/Y=2/Z=4）
	const int kCompGroupMask[7] = { 1, 2, 4, 1 | 2, 2 | 4, 4 | 1, 1 | 2 | 4 };
	//分量组名（显示设置行名与符号数值前缀共用）
	const char* kCompGroupNames[7] = { "X", "Y", "Z", "X+Y", "Y+Z", "Z+X", "X+Y+Z" };
	//截面力ForceMoment符号固定名（复用同一符号整体重建）
	const char kSectionForceSymbolName[] = "SectionForce_ForceMoment";

	//求和点选项（0形心/1节点/2坐标），单元格按此id存取，语言切换时按id重译
	QString sumPointText(int id, int lang)
	{
		static const char* cn[] = { "形心", "节点", "坐标" };
		static const char* en[] = { "Centroid", "Node", "Coordinate" };
		return QString(lang == 0 ? cn[id] : en[id]);
	}

	//求和点单元格文本：形心/节点名(未指定)/坐标值，对齐设计稿
	QString sumPointCellText(int sumPoint, const QString& nodeName, double x, double y, double z, int lang)
	{
		switch (sumPoint)
		{
		case 1:
			return nodeName.isEmpty() ? (lang == 0 ? QString::fromUtf8("未指定节点") : QString("Unspecified")) : nodeName;
		case 2:
			return QString("%1,%2,%3").arg(x).arg(y).arg(z);
		default:
			return sumPointText(0, lang);
		}
	}

	//编号录入解析："1, 2, 5-10"（支持中英文逗号/分号/空白分隔与 a-b 范围）
	bool parseIdRanges(const QString& text, QList<int>& out)
	{
		out.clear();
		static const QRegularExpression kSplitter(QString::fromUtf8("[,，;；\\s]+"));
		const QStringList parts = text.split(kSplitter, QString::SkipEmptyParts);
		for (const QString& raw : parts)
		{
			const QString part = raw.trimmed();
			if (part.contains(QLatin1Char('-')))
			{
				const QStringList ab = part.split(QLatin1Char('-'));
				if (ab.size() != 2) return false;
				bool okA = false, okB = false;
				const long long a = ab.at(0).toLongLong(&okA);
				const long long b = ab.at(1).toLongLong(&okB);
				if (!okA || !okB || a < 1 || b < a || b - a > 100000) return false;
				for (long long v = a; v <= b; ++v)
					out.append(int(v));
			}
			else
			{
				bool ok = false;
				const long long v = part.toLongLong(&ok);
				if (!ok || v < 1) return false;
				out.append(int(v));
			}
		}
		return !out.isEmpty();
	}

	//主操作蓝按钮（新建/计算/确认）：公共控件 mPrimaryButton（MxFunctions）
	//分段格（求和点/计算类型/载荷类型/符号尺寸/符号位置）：公共控件 mChoiceGroup（MxFunctions）
}

namespace PostWindow
{
	mPostSectionForce::mPostSectionForce(QWidget* parent)
		: mPanelBase(parent)
		, _postRend(nullptr)
		, _postDB(nullptr)
	{
		setupUI();

		// 与云图面板统一：折叠区注册到基类统一管理
		m_foldWidgets.append(_manageFold);
		m_foldWidgets.append(_calcFold);

		// 切换界面语言
		connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)), this, SLOT(slot_changeLanguage(int)));
		slot_changeLanguage(MAppConfiguration::mAppConfiguration::getInstance()->_language);

		this->setMinimumWidth(300);
	}

	mPostSectionForce::~mPostSectionForce()
	{
	}

	void mPostSectionForce::setupUI()
	{
		_mainLayout = new QVBoxLayout(this);
		_mainLayout->setContentsMargins(2, 0, 2, 0);

		//分量色表预填默认值：显示设置弹窗懒创建期间，力/力矩符号刷新即可取色
		for (int i = 0; i < 7; ++i)
		{
			_dsForceColors.append(kDsCompColors[i]);
			_dsMomentColors.append(kDsCompColors[i]);
		}

		/********************截面管理********************/
        		_manageFold = new FoldSection(QString::fromUtf8("截面管理"), this);
		//左右缝隙随共享档位 fieldInset（同云图/剖切：控件距参数栏边界留缝，勿清 0 贴边），垂直 10 保持本面板节律
		if (auto* form = _manageFold->layout())
		{
			form->setContentsMargins(MxFunctions::UiMetrics::fieldInset(), 10, MxFunctions::UiMetrics::fieldInset(), 10);
		}

		//搜索框
		_searchEdit = new QLineEdit(this);
		_searchEdit->setObjectName("fbdSearchEdit");
		_searchEdit->setClearButtonEnabled(true);
		_searchEdit->setFixedHeight(26);
		_manageFold->addRow(_searchEdit);
		connect(_searchEdit, &QLineEdit::textChanged, this, &mPostSectionForce::slot_searchChanged);

		//截面列表（复选/名称/求和点/坐标系/颜色；共享管理表格件 mPanelTable：
		//样式/行高/表头/行线与颜色格委托统一（UiPalette/UiMetrics），空态覆盖层
		//内置——两文案区分"暂无截面"与"没有匹配"（搜索全隐藏，对齐测量/标签口径））
		_table = new MxFunctions::mPanelTable(0, 5, this);
		_table->setObjectName("fbdTable");
		_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
		_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
		_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
		_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
		_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
		_table->setColumnWidth(0, 26);
		_table->setColumnWidth(2, 48);
		_table->setColumnWidth(3, 60);
		_table->setColumnWidth(4, 44);
		_table->setSelectionBehavior(QAbstractItemView::SelectRows);
		_table->setSelectionMode(QAbstractItemView::NoSelection);	//V1.3：勾选仅批量，编辑走行点击
		_table->setColorColumn(4);
		_table->horizontalHeader()->setSortIndicatorShown(false);
		_table->horizontalHeader()->setSortIndicator(1, Qt::AscendingOrder);
		_table->setFixedHeight(190);
		_manageFold->addRow(_table);

		connect(_table, &QTableWidget::itemChanged, this, &mPostSectionForce::slot_itemChanged);
		connect(_table, &QTableWidget::cellClicked, this, [this](int row, int col) {
			if (col == 4)
				slot_showColorPopup(row);
			else if (col != 0)
			{
				//名称/求和点/坐标系格点击=选中编辑（V1.3：勾选列仅批量）
				QTableWidgetItem* nameIt = _table->item(row, 1);
				if (nameIt)
					selectSection(nameIt->data(Qt::UserRole).toInt());
			}
			});

	//表尾：全选（扁平文字钮）/已选计数靠左，删除/新建靠右
	//主操作蓝/删除红按设计令牌直接设到控件（控件级样式在此处可靠生效）
	QWidget* footerRow = new QWidget(this);
	QHBoxLayout* footerLayout = new QHBoxLayout(footerRow);
	footerLayout->setContentsMargins(0, 0, 0, 0);
	footerLayout->setSpacing(6);
	_btnSelectAll = new QPushButton(footerRow);
	_btnSelectAll->setStyleSheet(
		"QPushButton{border:0;background:transparent;color:#424B57;border-radius:2px;}"
		"QPushButton:hover{background:#F0F7FF;}"
		"QPushButton:disabled{color:#A6AFBB;background:transparent;}");
	_btnSelectAll->setFixedSize(44, 24);
	_btnSelectAll->setEnabled(false);
	footerLayout->addWidget(_btnSelectAll);
	_labelSelCount = new QLabel(footerRow);
	_labelSelCount->setStyleSheet(QString("color:%1;font-size:11px;").arg(kMuted));
	footerLayout->addWidget(_labelSelCount);
	footerLayout->addStretch();
	_btnDelete = new QPushButton(footerRow);
	_btnDelete->setStyleSheet(QString("QPushButton:enabled{color:%1;}").arg(kDanger));
	_btnDelete->setFixedSize(56, 26);
	_btnDelete->setEnabled(false);
	footerLayout->addWidget(_btnDelete);
		_btnNew = new MxFunctions::mPrimaryButton(QString(), footerRow);
		_btnNew->setFixedSize(56, 26);
		footerLayout->addWidget(_btnNew);
	_manageFold->addRow(footerRow);

		connect(_btnSelectAll, &QPushButton::clicked, this, &mPostSectionForce::slot_selectAllClicked);
		connect(_btnNew, &QPushButton::clicked, this, &mPostSectionForce::slot_newButton);
		connect(_btnDelete, &QPushButton::clicked, this, &mPostSectionForce::slot_deleteButton);

		//截面名称
		_nameEdit = new QLineEdit(this);
		_nameEdit->setObjectName("fbdNameEdit");
		_nameEdit->setMaxLength(80);
		_nameEdit->setFixedHeight(26);
		_nameEdit->setEnabled(false);
		_manageFold->addRow(QString::fromUtf8("截面名称"), _nameEdit);
		connect(_nameEdit, &QLineEdit::editingFinished, this, &mPostSectionForce::slot_nameEditFinished);

		//单元（必选）/节点（可选）拾取行——mPost 拾取栈拾取条（云图/向量图/爆炸/阵列
		//同款）：点击条体激活拾取/再点停止（开关语义），详情弹窗（分组+逐项删/按类型
		//整删）与清空钮内建；收割/高亮/重入回显由控制器自管。行尾并排"编号"小钮保留
		//手工编号录入入口（弹窗预填现有编号，支持 替换/添加/移除）
		auto makePickBar = [this](mPostPickWidgetController*& controller, const QString& objectName,
			MxFunctions::PickDataType type, mxr::PickFilter filter, bool single, const QString& tip) {
			controller = mPostGeneralPickUp::createPickWidget(CBBlue, this);
			controller->widget()->setObjectName(objectName);
			controller->setSelectSetBtnVisible(false);				//后处理无集合语义
			controller->setKeepHighlightOnDeactivate(false);		//解除拾取即清视口高亮（组合高亮由失活槽回写）
			if (single)
				controller->setPickMode(PickOne);					//求和节点=单节点覆盖式拾取
			controller->setAllowedTypes([type]() {
				return QSet<MxFunctions::PickDataType>{ type };	//激活时工具栏白名单仅本类型
				});
			controller->setDefultTip(tip);
			//激活/失活收口（云图/向量图同款）：激活=按条类型设视口拾取过滤器（无效后
			//处理上下文立即失活回弹）；失活=清过滤器+恢复工具栏全部拾取类型；全部失活
			//时面板回写组合高亮（互斥切到兄弟条时此写随即被新激活条覆盖，无残留污染）
			connect(controller, &mPostPickWidgetController::pickActiveChanged, this,
				[this, controller, filter](bool active) {
					mxr::REBUILD_PostRend* postRend = getCVM ? getCVM->getPostRenderWidget() : nullptr;
					if (active)
					{
						if (postRend == nullptr || postRend->getPostRender() == nullptr)
						{
							controller->setPickActive(false);
							return;
						}
						postRend->setPickFilter(filter);
					}
					else
					{
						if (postRend != nullptr)
							postRend->setPickFilter(mxr::PickFilter::PickNothing);
						emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>());
						if (!anyPickBarActive())
							refreshCombinedHighlight();
					}
					});
			};
		//收割：拾取条数据 → 编辑缓冲（V1.3 契约：拾取全走缓冲，保存写回记录）
		auto wireHarvest = [this](mPostPickWidgetController* controller, MxFunctions::PickDataType type, int target) {
			connect(controller, &mPostPickWidgetController::dataChanged, this,
				[this, controller, type, target]() {
					if (_loadingWorking || _editingId < 0)
						return;	//回显装载/无编辑对象期间不回写（displayData 不发信号，双保险）
					QList<int> ids;
					const auto& picked = controller->getAllData().pickedData;
					const auto it = picked.find(type);
					if (it != picked.end())
					{
						for (const auto& d : it->second)
							ids.append(d.result_id);
						std::sort(ids.begin(), ids.end());
					}
					if (target == 0) _working.elementIds = ids;
					else if (target == 1) _working.nodeIds = ids;
					else _working.nodeName = ids.isEmpty() ? QString() : QString::number(ids.first());
					if (target == 2)
					{
						notifyEditChanged();
						//求和节点决定合力简化中心，已计算时实时重绘
						if (_hasCalcResult)
							refreshForceMomentSymbols();
						return;
					}
					updatePickHint();
					notifyEditChanged();
					invalidateCalcResult();	//拾取变化后已计算结果失效，需重新点击计算
					});
			};
		//"编号"小钮：打开手工编号录入弹窗（target 0单元/1节点/2求和节点）
		auto makeIdsButton = [this](QToolButton*& btn, const QString& objectName, int target, const QString& tip) {
			btn = new QToolButton(this);
			btn->setObjectName(objectName);
			btn->setCursor(Qt::PointingHandCursor);
			btn->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/mSectionWindow/bianji.png")));
			btn->setIconSize(QSize(16, 16));
			btn->setFixedSize(26, 26);
			btn->setToolTip(tip);
			//父链无 QSS 传播，padding:0 仍显式声明（描边钮内容区防御，云图小钮实证）
			btn->setStyleSheet(QString("QToolButton{background:#FFFFFF;border:1px solid %1;border-radius:2px;padding:0;}"
				"QToolButton:hover{border-color:%2;}").arg(kCtrlBorder).arg(kPrimary));
			if (target == 2)
				connect(btn, &QToolButton::clicked, this, &mPostSectionForce::slot_showSumNodePopup);
			else
				connect(btn, &QToolButton::clicked, this, [this, target]() { slot_showPickPopup(target); });
			};
		makePickBar(_elemPick, QStringLiteral("fbdElemPick"), MxFunctions::PickDataType::Element,
			mxr::PickFilter::PickMeshElement, false, QString::fromUtf8("选择单元"));
		wireHarvest(_elemPick, MxFunctions::PickDataType::Element, 0);
		makeIdsButton(_btnElemIds, QStringLiteral("fbdElemIdsBtn"), 0, QString::fromUtf8("单元编号录入"));
		makePickBar(_nodePick, QStringLiteral("fbdNodePick"), MxFunctions::PickDataType::Node,
			mxr::PickFilter::PickMeshNode, false, QString::fromUtf8("选择节点"));
		wireHarvest(_nodePick, MxFunctions::PickDataType::Node, 1);
		makeIdsButton(_btnNodeIds, QStringLiteral("fbdNodeIdsBtn"), 1, QString::fromUtf8("节点编号录入"));
		{
			PanelLabel* elemLabel = new PanelLabel(QString::fromUtf8("单元（必选）"));
			_manageFold->addRow(elemLabel, _elemPick->widget(), _btnElemIds);
			PanelLabel* nodeLabel = new PanelLabel(QString::fromUtf8("节点（可选）"));
			_manageFold->addRow(nodeLabel, _nodePick->widget(), _btnNodeIds);
		}

		//拾取进度提示
		_pickHintLabel = new QLabel(this);
		_pickHintLabel->setStyleSheet(
			QString("QLabel{color:%1;font-size:11px;border:0;background:transparent;}").arg(kMuted));
		_pickHintLabel->setText(QString::fromUtf8("必须拾取单元；仅拾取节点不能切出。"));
		_pickHintLabel->hide();
		_manageFold->addRow(_pickHintLabel);

		//坐标系——公共控件 mComboActionRow（下拉伸展＋H×H"＋"动作钮，各自带边框 6px 间隙，
		//对齐原型 coordinate-composite；"＋"待接入置灰）
		_coordRow = new MxFunctions::mComboActionRow(this);
		_comboCoord = _coordRow->combo();
		_comboCoord->setObjectName("fbdCoordCombo");
		_comboCoord->setEnabled(false);
		_btnAddCoord = _coordRow->actionButton();
		_btnAddCoord->setObjectName("fbdAddCoordBtn");
		_btnAddCoord->setEnabled(false);
		_manageFold->addRow(QString::fromUtf8("坐标系"), _coordRow);
		connect(_comboCoord, &QComboBox::currentTextChanged, this, &mPostSectionForce::slot_coordChanged);

		//求和点（形心/节点/坐标）——公共控件 mChoiceGroup（共框圆点单选，原型 mirror-planes）
		_sumPointChoice = new MxFunctions::mChoiceGroup(this);
		_manageFold->addRow(QString::fromUtf8("求和点"), _sumPointChoice);
		connect(_sumPointChoice, &MxFunctions::mChoiceGroup::currentIndexChanged, this, &mPostSectionForce::slot_sumPointToggled);

		//求和节点行（求和点=节点时显示）——mPost 拾取栈拾取条（PickOne 单选：后拾取
		//覆盖先拾取）；行尾"编号"小钮=单节点编号录入弹窗。原类型下拉固定"节点"随
		//mObjectPickComposite 复合行一并取消（拾取条白名单即类型）。本行为条件显隐行
		//（setRowVisible 行级摘除），[拾取条+小钮]必须包一层容器——多控件直接 addRow
		//的 field 槽是布局，takeRow 摘行会删布局且只藏 label，行结构将被破坏
		makePickBar(_sumNodePick, QStringLiteral("fbdSumNodePick"), MxFunctions::PickDataType::Node,
			mxr::PickFilter::PickMeshNode, true, QString::fromUtf8("选择节点"));
		wireHarvest(_sumNodePick, MxFunctions::PickDataType::Node, 2);
		makeIdsButton(_btnSumNodeIds, QStringLiteral("fbdSumNodeIdsBtn"), 2, QString::fromUtf8("节点编号录入"));
		_sumNodeRow = new QWidget(this);
		{
			QHBoxLayout* rowLayout = new QHBoxLayout(_sumNodeRow);
			rowLayout->setContentsMargins(0, 0, 0, 0);
			rowLayout->setSpacing(MxFunctions::UiMetrics::rowGap());
			rowLayout->addWidget(_sumNodePick->widget());
			rowLayout->addWidget(_btnSumNodeIds);
		}
		_manageFold->addRow(QString::fromUtf8("求和节点"), _sumNodeRow);
		_manageFold->setRowVisible(_sumNodeRow, false);

		//全局坐标行（求和点=坐标时显示）——公共控件 mAxisCoordRow（X/Y/Z 三段
		//[selected 徽章|输入框]共框，原型 deformation-axis-controls）
		_coordInput = new MxFunctions::mAxisCoordRow(this);
		for (QLineEdit* le : { _coordInput->xEdit(), _coordInput->yEdit(), _coordInput->zEdit() })
			le->setValidator(new QDoubleValidator(le));
		_manageFold->addRow(QString::fromUtf8("全局坐标"), _coordInput);
		_manageFold->setRowVisible(_coordInput, false);
		connect(_coordInput, &MxFunctions::mAxisCoordRow::editingFinished, this, &mPostSectionForce::slot_coordEditFinished);

		/******** 统一编辑契约操作区（V1.3：定义区之后、计算与显示之前） ********/
		_editor = new mPostObjectEditController(this);
		_editor->setSnapshotFn([this]()
			{
				return _editingId >= 0 ? sectionSignature(_working) : QString();
			});
		_editor->setSaveFn([this](bool immediate) { return commitEdit(immediate); });
		_editor->setCancelFn([this]() { cancelEdit(); });
		_editor->setCurrentNameFn([this]()
			{
				const SectionData* rec = sectionRecord(_editingId);
				return rec ? rec->name : QString::fromUtf8("当前截面");
			});
		connect(_editor, &mPostObjectEditController::saveRequested, this,
			[this](bool immediate) { if (!immediate) commitEdit(false); });
		connect(_editor, &mPostObjectEditController::cancelRequested, this, [this]() { cancelEdit(); });
		_editor->attachFooter(nullptr, -1, _manageFold);
		_manageFold->addRow(_editor->footerWidget());

		_mainLayout->addWidget(_manageFold);

		/********************计算与显示********************/
		_calcFold = new FoldSection(QString::fromUtf8("计算与显示"), this);
		//左右缝隙随共享档位 fieldInset（同上，勿清 0 贴边）
		if (auto* form = _calcFold->layout())
		{
			form->setContentsMargins(MxFunctions::UiMetrics::fieldInset(), 10, MxFunctions::UiMetrics::fieldInset(), 10);
		}

		//计算类型（自由体力/合力）——公共控件 mChoiceGroup（原型 mirror-planes）
		_calcTypeChoice = new MxFunctions::mChoiceGroup(this);
		_calcFold->addRow(QString::fromUtf8("计算类型"), _calcTypeChoice);
		//计算类型切换后按新模式重绘力/力矩符号（已计算时实时重绘；自由体力逐节点/合力整体简化）
		connect(_calcTypeChoice, &MxFunctions::mChoiceGroup::currentIndexChanged, this,
			[this](int) { if (_hasCalcResult) refreshForceMomentSymbols(); });

		//载荷类型（自由力/外力/支反力）——公共控件 mChoiceGroup
		_loadTypeChoice = new MxFunctions::mChoiceGroup(this);
		_calcFold->addRow(QString::fromUtf8("载荷类型"), _loadTypeChoice);

		//显示设置——公共控件 mAdvancedOptionsButton（原型 advanced-options-button：caption 左、› 右）
		_btnDisplay = new MxFunctions::mAdvancedOptionsButton(QString::fromUtf8("设置"), this);
		_calcFold->addRow(QString::fromUtf8("显示设置"), _btnDisplay);

		//忽略节点集 + 容差
		QWidget* toleranceRow = new QWidget(this);
		QHBoxLayout* toleranceLayout = new QHBoxLayout(toleranceRow);
		toleranceLayout->setContentsMargins(0, 0, 0, 0);
		toleranceLayout->setSpacing(6);
		_checkIgnoreNode = new QCheckBox(toleranceRow);
		_checkIgnoreNode->setObjectName("fbdIgnoreNodeCheck");
		_checkIgnoreNode->setChecked(true);
		toleranceLayout->addWidget(_checkIgnoreNode);
		toleranceLayout->addStretch();
		QLabel* toleranceLabel = new QLabel(QString::fromUtf8("容差"), toleranceRow);
		toleranceLayout->addWidget(toleranceLabel);
		_editTolerance = new QLineEdit(toleranceRow);
		_editTolerance->setFixedWidth(150);
		_editTolerance->setFixedHeight(26);
		_editTolerance->setText("0.00001");
		_editTolerance->setValidator(new QDoubleValidator(0.0, 1e9, 8, _editTolerance));
		toleranceLayout->addWidget(_editTolerance);
		_calcFold->addRow(toleranceRow);

		//清除/计算（靠右；初始禁用，tooltip 说明启用条件）
		QWidget* execRow = new QWidget(this);
		QHBoxLayout* execLayout = new QHBoxLayout(execRow);
		execLayout->setContentsMargins(0, 0, 0, 0);
		execLayout->setSpacing(6);
		execLayout->addStretch();
		_btnClear = new QPushButton(execRow);
		_btnClear->setFixedSize(56, 26);
		_btnClear->setEnabled(false);
		execLayout->addWidget(_btnClear);
		_btnCalc = new MxFunctions::mPrimaryButton(QString(), execRow);
		_btnCalc->setFixedSize(56, 26);
		_btnCalc->setEnabled(false);
		execLayout->addWidget(_btnCalc);
		_calcFold->addRow(execRow);

		_mainLayout->addWidget(_calcFold);

		_mainLayout->addStretch();

		//计算：按当前截面与显示设置绘制力/力矩（绘制仅在计算后进行）；清除：移除已绘制符号
		connect(_btnCalc, &QPushButton::clicked, this, &mPostSectionForce::slot_calcClicked);
		connect(_btnClear, &QPushButton::clicked, this, &mPostSectionForce::slot_clearClicked);
		//忽略节点集：勾选后选定节点视为未选中（提示联动，已计算时实时重绘）
		connect(_checkIgnoreNode, &QCheckBox::toggled, this, [this](bool) {
			updatePickHint();
			if (_hasCalcResult)
				refreshForceMomentSymbols();
			});
		connect(_btnDisplay, &MxFunctions::mAdvancedOptionsButton::clicked, this, &mPostSectionForce::slot_showDisplayPopup);
	}

	void mPostSectionForce::newSectionForcePanel(int caseId)
	{
		_caseId = caseId;

		//面板作用于当前后处理视口（与测量工具一致）：无后处理视口时仅切换面板不初始化
		auto viewportPrivate = VMManager->getCurrentViewportPrivate();
		if (viewportPrivate == nullptr)
			return;

		if (viewportPrivate->getViewportType() != ViewportPost3D)
		{
			_postRend = nullptr;
			_postDB = nullptr;
			return;
		}

		mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
		if (postRend == nullptr)
			return;

		_postRend = postRend;
		QString postname = postRend->getPostDataName();
		_postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
		//视口拾取过滤/收割由拾取条激活会话自管；组合高亮重锚到新视口
		refreshCombinedHighlight();
		invalidateCalcResult();	//视口/工况切换后已计算结果失效（原视口已绘符号随其视口保留）
	}

	void mPostSectionForce::refreshEmptyState()
	{
		//空态覆盖层由共享表格件内置（按可见行数自动切换两文案）
		_table->updateEmptyState();
	}

	int mPostSectionForce::singleSelectedRow() const
	{
		const auto rows = _table->selectionModel()->selectedRows();
		return rows.size() == 1 ? rows.at(0).row() : -1;
	}

	int mPostSectionForce::dataIndexById(int id) const
	{
		for (int i = 0; i < _sections.size(); ++i)
			if (_sections.at(i).id == id) return i;
		return -1;
	}

	void mPostSectionForce::renderRow(int row)
	{
		QTableWidgetItem* nameIt = _table->item(row, 1);
		if (!nameIt) return;
		int di = dataIndexById(nameIt->data(Qt::UserRole).toInt());
		if (di < 0) return;
		const SectionData& sec = _sections.at(di);

		int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;

		//V1.3：缺必选单元的截面标"（待定义）"
		if (sec.incomplete)
		{
			nameIt->setText(sec.name + QString::fromUtf8("（待定义）"));
			nameIt->setToolTip(sec.name + QString::fromUtf8("：请补充单元和参数后保存"));
		}
		else
		{
			nameIt->setText(sec.name);
			nameIt->setToolTip(sec.name);
		}
		QTableWidgetItem* sumIt = _table->item(row, 2);
		sumIt->setData(Qt::UserRole, sec.sumPoint);
		sumIt->setText(sumPointCellText(sec.sumPoint, sec.nodeName, sec.x, sec.y, sec.z, lang));
		_table->item(row, 3)->setText(sec.coordSys);
		_table->item(row, 4)->setBackground(QBrush(sec.color));
	}

	void mPostSectionForce::syncSelectionState()
	{
		//“已选 N”按复选计数；删除/全选/计算随之启停
		int checked = 0;
		for (int r = 0; r < _table->rowCount(); ++r)
			if (_table->item(r, 0)->checkState() == Qt::Checked) ++checked;

		int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
		_labelSelCount->setText(lang == 0 ? QString::fromUtf8("已选 %1").arg(checked)
			: QString("%1 selected").arg(checked));
		_btnDelete->setEnabled(checked > 0);
		_btnSelectAll->setEnabled(_table->rowCount() > 0);
		_btnCalc->setEnabled(_table->rowCount() > 0);

		//编辑行（_editingId）的名称以蓝色显示，其余行正文色
		const int activeRow = editingRow();
		for (int r = 0; r < _table->rowCount(); ++r)
		{
			if (_table->item(r, 1))
				_table->item(r, 1)->setForeground(QBrush(QColor(r == activeRow ? kSelText : kText)));
		}
	}

	int mPostSectionForce::editingRow() const
	{
		if (_editingId < 0) return -1;
		for (int r = 0; r < _table->rowCount(); ++r)
		{
			QTableWidgetItem* nameIt = _table->item(r, 1);
			if (nameIt && nameIt->data(Qt::UserRole).toInt() == _editingId)
				return r;
		}
		return -1;
	}

	mPostSectionForce::SectionData* mPostSectionForce::sectionRecord(int id)
	{
		const int di = dataIndexById(id);
		return di < 0 ? nullptr : &_sections[di];
	}

	void mPostSectionForce::fillWorkingToUI(const SectionData* sec)
	{
		int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
		const bool valid = sec != nullptr;
		const int prevId = _editingId;
		//切换编辑对象=结束拾取会话（互斥模式：避免激活条收割到旧截面的容器数据）
		deactivatePickBars();
		_loadingWorking = true;
		_working = valid ? *sec : SectionData{};
		_editingId = valid ? sec->id : -1;

		_nameEdit->setEnabled(valid);
		if (_elemPick)
		{
			_elemPick->widget()->setPickWidgetEnabled(valid);
			_nodePick->widget()->setPickWidgetEnabled(valid);
			_sumNodePick->widget()->setPickWidgetEnabled(valid);
		}
		if (_btnElemIds)
		{
			_btnElemIds->setEnabled(valid);
			_btnNodeIds->setEnabled(valid);
			_btnSumNodeIds->setEnabled(valid);
		}
		_coordRow->setEnabled(valid);
		_sumPointChoice->setEnabled(valid);

		_nameEdit->blockSignals(true);
		_comboCoord->blockSignals(true);
		_sumPointChoice->blockSignals(true);
		if (valid)
		{
			_nameEdit->setText(_working.name);
			_sumPointChoice->setCurrentIndex(_working.sumPoint);
			_comboCoord->setCurrentText(_working.coordSys);
			_coordInput->setValues(_working.x, _working.y, _working.z);
		}
		else
		{
			_nameEdit->clear();
			_comboCoord->setCurrentIndex(0);
		}
		_sumPointChoice->blockSignals(false);
		_nameEdit->blockSignals(false);
		_comboCoord->blockSignals(false);

		updateSumPointExtraRows(valid ? _working.sumPoint : -1);
		updatePickHint();
		refreshPickBars();			//拾取条回显当前截面缓冲（displayData 不置脏不发信号）
		refreshCombinedHighlight();	//视口高亮跟随当前截面（单元+节点组合）
		//切换编辑截面后已计算结果失效（同截面的其他刷新不清除）
		if ((valid ? _working.id : -1) != _calcSectionId)
			invalidateCalcResult();
		_nameEdit->setPlaceholderText(lang == 0 ? QString::fromUtf8("请先新建截面") : QString("Create a section first"));
		_loadingWorking = false;
		if (prevId != _editingId)
			syncSelectionState();
	}

	void mPostSectionForce::fillPropertyRows(int row)
	{
		//V1.3 兼容壳：外部仍按行号调用时转走到缓冲装载
		const int di = row >= 0 ? dataIndexById(_table->item(row, 1)->data(Qt::UserRole).toInt()) : -1;
		fillWorkingToUI(di >= 0 ? &_sections[di] : nullptr);
	}

	void mPostSectionForce::updateSumPointExtraRows(int sumPoint)
	{
		//求和点切离"节点"：先收口拾取会话再摘行（激活态残留会占用全局互斥）
		if (sumPoint != 1 && _sumNodePick && _sumNodePick->isPickActive())
			_sumNodePick->setPickActive(false);
		//拾取条可见标志与行显隐同步（条隐藏期间拒绝激活；面板显隐周期后行恢复时
		//条内控件一并就位——hideEvent 曾把条 widget 藏掉，行回来了条还是藏的）
		if (_sumNodePick)
			_sumNodePick->setPickWidgetVisible(sumPoint == 1);
		_manageFold->setRowVisible(_sumNodeRow, sumPoint == 1);
		_manageFold->setRowVisible(_coordInput, sumPoint == 2);
		_coordInput->setEnabled(sumPoint == 2);
		_manageFold->refreshLayout();
		//条隐藏会按惯例清共享容器高亮（v1 语义），组合高亮补回
		if (sumPoint != 1 && !anyPickBarActive())
			refreshCombinedHighlight();
	}

	QColor mPostSectionForce::nextSectionColor() const
	{
		//新建截面默认主蓝（对齐设计稿），颜色列点击可改
		return QColor(kPrimary);
	}

	void mPostSectionForce::slot_changeLanguage(int language)
	{
		int lang = language;

		_manageFold->setTitle(lang == 0 ? QString::fromUtf8("截面管理") : QString("Section Management"));
		_searchEdit->setPlaceholderText(lang == 0 ? QString::fromUtf8("搜索截面名称") : QString("Search sections"));
		_table->setEmptyTexts(lang == 0 ? QString::fromUtf8("暂无截面，点击“新建”开始定义。")
			: QString("No sections yet. Click \"New\" to define one."),
			lang == 0 ? QString::fromUtf8("没有匹配的截面") : QString("No matching sections"));
		_btnSelectAll->setText(lang == 0 ? QString::fromUtf8("全选") : QString("All"));
		_btnDelete->setText(lang == 0 ? QString::fromUtf8("删除") : QString("Del"));
		_btnNew->setText(lang == 0 ? QString::fromUtf8("新建") : QString("New"));

		_table->setHorizontalHeaderItem(0, new QTableWidgetItem(QString()));
		_table->setHorizontalHeaderItem(1, new QTableWidgetItem(lang == 0 ? QString::fromUtf8("名称") : QString("Name")));
		_table->setHorizontalHeaderItem(2, new QTableWidgetItem(lang == 0 ? QString::fromUtf8("求和点") : QString("Sum Point")));
		_table->setHorizontalHeaderItem(3, new QTableWidgetItem(lang == 0 ? QString::fromUtf8("坐标系") : QString("Coord Sys")));
		_table->setHorizontalHeaderItem(4, new QTableWidgetItem(lang == 0 ? QString::fromUtf8("颜色") : QString("Color")));
		if (_editor)
			_editor->retranslate(lang == 1);

		//拾取进度提示随语言刷新；拾取弹窗懒创建（未创建时跳过，弹出前会按当前语言刷新）
		updatePickHint();
		//拾取条空态文案与"编号"小钮提示（拾取后条内文字为栈标准"已选择N个"，不随语言切换）
		if (_elemPick)
		{
			_elemPick->setDefultTip(lang == 0 ? QString::fromUtf8("选择单元") : QString("Select Elements"));
			_nodePick->setDefultTip(lang == 0 ? QString::fromUtf8("选择节点") : QString("Select Nodes"));
			_sumNodePick->setDefultTip(lang == 0 ? QString::fromUtf8("选择节点") : QString("Pick Node"));
		}
		if (_btnElemIds)
		{
			_btnElemIds->setToolTip(lang == 0 ? QString::fromUtf8("单元编号录入") : QString("Enter element IDs"));
			_btnNodeIds->setToolTip(lang == 0 ? QString::fromUtf8("节点编号录入") : QString("Enter node IDs"));
			_btnSumNodeIds->setToolTip(lang == 0 ? QString::fromUtf8("节点编号录入") : QString("Enter node ID"));
		}
		if (_pickPopup)
			applyPickPopupLanguage(lang);
		_btnAddCoord->setText(QString::fromUtf8("＋"));
		_btnAddCoord->setToolTip(lang == 0 ? QString::fromUtf8("快捷创建坐标系（待接入）") : QString("Quick-create CSYS (pending)"));

		//列表整行按数据重渲染（求和点列文本随语言变化）
		for (int r = 0; r < _table->rowCount(); ++r)
			renderRow(r);

		_comboCoord->blockSignals(true);
		_comboCoord->clear();
		_comboCoord->addItem(lang == 0 ? QString::fromUtf8("全局坐标系") : QString("Global CS"));
		_comboCoord->blockSignals(false);

		//求和节点拾取弹窗
		if (_sumNodePopup)
		{
			_snTitleText->setText(lang == 0 ? QString::fromUtf8("选择求和节点") : QString("Select Sum Node"));
			_snIdsText->setText(lang == 0 ? QString::fromUtf8("实体编号") : QString("Entity ID"));
			_snEdit->setPlaceholderText(lang == 0 ? QString::fromUtf8("请输入一个节点编号") : QString("Enter a node number"));
			_snOkBtn->setText(lang == 0 ? QString::fromUtf8("确认") : QString("OK"));
		}

		//共框单选组（求和点/计算类型/载荷类型）：setOptions 换语言文案后恢复原选中
		//（setOptions 找不到原文案会回落第 0 项并发信号，须屏蔽信号后显式回选）
		auto retranslateChoice = [](MxFunctions::mChoiceGroup* choice, const QStringList& options, int index)
		{
			choice->blockSignals(true);
			choice->setOptions(options);
			if (index >= 0)
				choice->setCurrentIndex(index);
			choice->blockSignals(false);
		};
		retranslateChoice(_sumPointChoice,
			{ sumPointText(0, lang), sumPointText(1, lang), sumPointText(2, lang) },
			_editingId >= 0 ? _working.sumPoint : _sumPointChoice->currentIndex());

		_calcFold->setTitle(lang == 0 ? QString::fromUtf8("计算与显示") : QString("Calculation & Display"));
		retranslateChoice(_calcTypeChoice,
			lang == 0 ? QStringList{ QString::fromUtf8("自由体力"), QString::fromUtf8("合力") }
			: QStringList{ QString("Free Body Force"), QString("Resultant") },
			_calcTypeChoice->currentIndex());
		retranslateChoice(_loadTypeChoice,
			lang == 0 ? QStringList{ QString::fromUtf8("自由力"), QString::fromUtf8("外力"), QString::fromUtf8("支反力") }
			: QStringList{ QString("Free Force"), QString("External Force"), QString("Reaction Force") },
			_loadTypeChoice->currentIndex());
		_btnDisplay->setCaption(lang == 0 ? QString::fromUtf8("设置") : QString("Settings"));
		_checkIgnoreNode->setText(lang == 0 ? QString::fromUtf8("忽略节点集") : QString("Ignore node sets"));
		_btnClear->setText(lang == 0 ? QString::fromUtf8("清除") : QString("Clear"));
		_btnCalc->setText(lang == 0 ? QString::fromUtf8("计算") : QString("Calc"));
		_btnClear->setToolTip(lang == 0 ? QString::fromUtf8("清除已绘制的力与力矩") : QString("Clear drawn forces and moments"));
		_btnCalc->setToolTip(lang == 0 ? QString::fromUtf8("按当前截面拾取与显示设置绘制力与力矩（临时数据）")
			: QString("Draw forces and moments from current section picks and display settings (temp data)"));

		//显示设置弹窗
		if (_displayPopup)
			applyDisplayPopupLanguage(lang);

		syncSelectionState();
		refreshEmptyState();
	}

	void mPostSectionForce::slot_searchChanged(const QString& text)
	{
		for (int r = 0; r < _table->rowCount(); ++r)
		{
			QTableWidgetItem* nameIt = _table->item(r, 1);
			bool hit = text.isEmpty() || (nameIt && nameIt->text().contains(text, Qt::CaseInsensitive));
			_table->setRowHidden(r, !hit);
		}
	}

	void mPostSectionForce::slot_selectAllClicked()
	{
		//存在未勾选则全勾，否则全不勾（仅可见行）
		bool allChecked = _table->rowCount() > 0;
		for (int r = 0; r < _table->rowCount(); ++r)
		{
			if (_table->isRowHidden(r)) continue;
			if (_table->item(r, 0)->checkState() != Qt::Checked) { allChecked = false; break; }
		}
		const Qt::CheckState next = allChecked ? Qt::Unchecked : Qt::Checked;

		_table->blockSignals(true);
		for (int r = 0; r < _table->rowCount(); ++r)
		{
			if (_table->isRowHidden(r)) continue;
			_table->item(r, 0)->setCheckState(next);
		}
		_table->blockSignals(false);
		syncSelectionState();
	}

	void mPostSectionForce::slot_newButton()
	{
		beginCreate();
	}

	void mPostSectionForce::slot_deleteButton()
	{
		//移除勾选行；V1.3：当前编辑截面被删则回落首个截面，删空退出编辑态
		QList<int> rows;
		for (int r = 0; r < _table->rowCount(); ++r)
			if (_table->item(r, 0)->checkState() == Qt::Checked)
				rows.append(r);

		if (rows.isEmpty()) return;
		std::sort(rows.begin(), rows.end(), std::greater<int>());
		bool editingRemoved = false;
		{
			MxFunctions::mPanelTable::RowUpdateGuard guard(_table);
			for (int r : rows)
			{
				const int id = _table->item(r, 1)->data(Qt::UserRole).toInt();
				if (id == _editingId)
					editingRemoved = true;
				int di = dataIndexById(id);
				if (di >= 0) _sections.removeAt(di);
				_table->removeRow(r);
			}
		}
		_table->horizontalHeader()->setSortIndicatorShown(false);

		if (editingRemoved)
		{
			if (!_sections.isEmpty())
			{
				fillWorkingToUI(&_sections.first());
				_editor->beginEdit();
			}
			else
			{
				fillWorkingToUI(nullptr);
				_editor->endEdit();
			}
		}

		refreshEmptyState();
		syncSelectionState();
		refreshCombinedHighlight();	//删除后视口高亮跟随剩余编辑截面（无编辑对象则清空）
		invalidateCalcResult();	//截面删除后已计算结果与符号一并失效
	}

	void mPostSectionForce::slot_itemChanged(QTableWidgetItem* item)
	{
		if (!item || item->column() != 0) return;
		syncSelectionState();
	}

	void mPostSectionForce::slot_nameEditFinished()
	{
		//V1.3：名称写编辑缓冲，保存时写回记录（表格名称在提交后更新）
		if (_editingId < 0) return;
		QString text = _nameEdit->text().simplified();
		if (text.isEmpty()) return;
		if (_working.name == text) return;
		_working.name = text;
		notifyEditChanged();
	}

	void mPostSectionForce::slot_sumPointToggled(int id)
	{
		if (_editingId < 0) return;
		_working.sumPoint = id;
		updateSumPointExtraRows(id);
		//求和点决定合力简化中心（形心/节点/坐标），已计算时实时重绘
		if (_hasCalcResult)
			refreshForceMomentSymbols();
		notifyEditChanged();
	}

	void mPostSectionForce::slot_coordChanged(const QString& text)
	{
		if (_editingId < 0) return;
		_working.coordSys = text;
		notifyEditChanged();
	}

	void mPostSectionForce::slot_coordEditFinished()
	{
		if (_editingId < 0) return;
		_coordInput->values(_working.x, _working.y, _working.z);
		//求和点=坐标：坐标变化联动合力简化中心，已计算时实时重绘
		if (_hasCalcResult)
			refreshForceMomentSymbols();
		notifyEditChanged();
	}

	mPostSectionForce::SectionData* mPostSectionForce::activeSection()
	{
		//V1.3：全部消费方（拾取/计算/符号/视口同步）按“当前编辑缓冲”语义工作
		return _editingId >= 0 ? &_working : nullptr;
	}

	// ============ 统一编辑契约（V1.3：定义事务缓冲独立于计算/显示） ============

	QString mPostSectionForce::sectionSignature(const SectionData& d)
	{
		QStringList elems, nodes;
		for (int id : d.elementIds) elems.append(QString::number(id));
		for (int id : d.nodeIds) nodes.append(QString::number(id));
		return d.name + QChar(1) + QString::number(d.sumPoint) + QChar(1)
			+ d.nodeName + QChar(1)
			+ QString::number(d.x) + QChar(1) + QString::number(d.y) + QChar(1) + QString::number(d.z) + QChar(1)
			+ d.coordSys + QChar(1) + d.color.name() + QChar(1)
			+ elems.join(QChar(2)) + QChar(1) + nodes.join(QChar(2));
	}

	void mPostSectionForce::notifyEditChanged()
	{
		if (!_loadingWorking && _editor)
			_editor->notifyEditChanged();
	}

	void mPostSectionForce::selectSection(int id)
	{
		if (id == _editingId) return;
		const auto action = [this, id]()
		{
			SectionData* rec = sectionRecord(id);
			if (!rec) return;
			fillWorkingToUI(rec);
			_editor->beginEdit();
			const int row = editingRow();
			if (row >= 0)
				_table->scrollToItem(_table->item(row, 1));
		};
		_editor->checkSwitch(action);
	}

	void mPostSectionForce::beginCreate()
	{
		const auto action = [this]()
		{
			//V1.3：登记唯一 ID+默认名先入表（待定义：未拾取单元可保存基本参数）
			int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
			SectionData sec;
			sec.id = _nextId++;
			QStringList used;
			for (const SectionData& s : _sections) used.append(s.name);
			sec.name = postObjectUniqueName(lang == 0
				? QString::fromUtf8("截面%1").arg(sec.id) : QString("Section%1").arg(sec.id), used);
			sec.coordSys = lang == 0 ? QString::fromUtf8("全局坐标系") : QString("Global CS");
			sec.color = nextSectionColor();
			sec.incomplete = true;
			_sections.append(sec);

			QTableWidgetItem* nameIt = nullptr;
			{
				MxFunctions::mPanelTable::RowUpdateGuard guard(_table);
				int r = _table->rowCount();
				_table->insertRow(r);
				_table->setItem(r, 0, MxFunctions::mPanelTable::makeCheckItem());
				nameIt = MxFunctions::mPanelTable::makeNameItem(QString(), sec.id);
				_table->setItem(r, 1, nameIt);
				_table->setItem(r, 2, MxFunctions::mPanelTable::makeTextItem(QString()));
				_table->setItem(r, 3, MxFunctions::mPanelTable::makeTextItem(QString()));
				_table->setItem(r, 4, MxFunctions::mPanelTable::makeTextItem(QString()));
				renderRow(r);
			}
			_table->horizontalHeader()->setSortIndicatorShown(false);
			const int visualRow = _table->row(nameIt);
			if (visualRow >= 0)
				_table->scrollToItem(_table->item(visualRow, 1));

			fillWorkingToUI(&_sections.last());
			_editor->beginEdit();
			refreshEmptyState();
			syncSelectionState();
			_nameEdit->setFocus();
		};
		_editor->checkSwitch(action);
	}

	void mPostSectionForce::cancelEdit()
	{
		SectionData* rec = sectionRecord(_editingId);
		if (!rec) return;
		fillWorkingToUI(rec);
		_editor->markSaved();
	}

	bool mPostSectionForce::commitEdit(bool immediate)
	{
		SectionData* rec = sectionRecord(_editingId);
		if (!rec) return false;

		//校验（原型 validate：名称非空/≤80/查重；非即时保存要求至少一个单元；
		//求和点=节点须有效编号；求和点=坐标须有限数值——输入框验证器已兜底）
		int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
		const QString name = _working.name.simplified();
		QString problem;
		if (name.isEmpty())
			problem = lang == 0 ? QString::fromUtf8("请输入截面名称。") : QString("Enter a section name.");
		else
		{
			for (const SectionData& s : _sections)
				if (s.id != _editingId && s.name == name)
				{
					problem = lang == 0 ? QString::fromUtf8("截面名称已存在。") : QString("Section name already exists.");
					break;
				}
		}
		if (problem.isEmpty() && !immediate && _working.elementIds.isEmpty())
			problem = lang == 0 ? QString::fromUtf8("请先选择至少一个单元，节点选择不能替代单元。")
				: QString("Pick at least one element; nodes alone cannot define the section.");
		if (problem.isEmpty() && _working.sumPoint == 1)
		{
			bool ok = false;
			const long long nodeId = _working.nodeName.toLongLong(&ok);
			if (!ok || nodeId < 1)
				problem = lang == 0 ? QString::fromUtf8("请指定有效的求和节点。") : QString("Specify a valid sum node.");
		}
		if (!problem.isEmpty())
		{
			if (!immediate)
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(problem);
				_nameEdit->setFocus();
			}
			return false;
		}

		//写回：缺必选单元的记录保持"待定义"，可保存有效基本参数
		_working.name = name;
		_working.incomplete = _working.elementIds.isEmpty();
		*rec = _working;
		if (!immediate)
			fillWorkingToUI(rec);	//保存后回填（含名称归一）
		_editor->markSaved();
		const int row = editingRow();
		if (row >= 0)
			renderRow(row);
		syncSelectionState();
		return true;
	}

	void mPostSectionForce::showEvent(QShowEvent* event)
	{
		QWidget::showEvent(event);
		//拾取条对称恢复（云图/向量图/阵列同款）；求和节点条按行显隐状态恢复（行被
		//摘除时不可恢复条可见，否则条会以无布局孤儿控件形态浮出）。各条 eventFilter
		//只恢复各自高亮形态（v1 共享容器语义=互相覆盖），延迟一拍回写组合高亮保证
		//单元+节点同显
		for (mPostPickWidgetController* c : { _elemPick, _nodePick })
			if (c) c->setPickWidgetVisible(true);
		if (_sumNodePick)
			_sumNodePick->setPickWidgetVisible(_working.sumPoint == 1);
		QTimer::singleShot(0, this, [this]() {
			if (isVisible() && !anyPickBarActive())
				refreshCombinedHighlight();
			});
	}

	void mPostSectionForce::hideEvent(QHideEvent* event)
	{
		QWidget::hideEvent(event);
		//面板退出事件：激活拾取解除+各自高亮清理由栈收口（云图/向量图/阵列同款）
		for (mPostPickWidgetController* c : { _elemPick, _nodePick, _sumNodePick })
			if (c) c->setPickWidgetVisible(false);
	}

	bool mPostSectionForce::anyPickBarActive() const
	{
		return (_elemPick && _elemPick->isPickActive())
			|| (_nodePick && _nodePick->isPickActive())
			|| (_sumNodePick && _sumNodePick->isPickActive());
	}

	void mPostSectionForce::deactivatePickBars()
	{
		for (mPostPickWidgetController* c : { _elemPick, _nodePick, _sumNodePick })
			if (c && c->isPickActive())
				c->setPickActive(false);	//失活即提交数据并按惯例清理高亮
	}

	void mPostSectionForce::refreshPickBars()
	{
		if (!_elemPick) return;
		//displayData：写入模型+刷新UI，不置脏不发 dataChanged（回显专用口径）
		auto buildData = [](MxFunctions::PickDataType type, const QList<int>& ids) {
			PickWidgetData data;
			if (ids.isEmpty()) return data;
			std::vector<RenderPickResultData> vec;
			vec.reserve(ids.size());
			for (int id : ids)
				vec.push_back(RenderPickResultData(id, 0));	//part_id=0 与容器收割口径互逆一致
			data.pickedData[type] = std::move(vec);
			return data;
			};
		_elemPick->displayData(buildData(MxFunctions::PickDataType::Element, _working.elementIds));
		_nodePick->displayData(buildData(MxFunctions::PickDataType::Node, _working.nodeIds));
		bool snOk = false;
		const long long sn = _working.nodeName.toLongLong(&snOk);
		_sumNodePick->displayData(buildData(MxFunctions::PickDataType::Node,
			(snOk && sn > 0) ? QList<int>{ int(sn) } : QList<int>{}));
	}

	void mPostSectionForce::refreshCombinedHighlight()
	{
		//v1 高亮语义=共享选中容器整体：激活拾取条独占容器（拾取会话期间只显示本条
		//数据）；全部失活时由面板把当前截面的单元+节点组合写回，保持"视口高亮跟随
		//当前截面"的既有行为。刷新走 WithoutRTRC：不发 ReadyToReadContainer，避免
		//激活控制器（含其他面板）回读本写入造成跨条数据污染
		mxr::REBUILD_PostRend* rend = getCVM ? getCVM->getPostRenderWidget() : nullptr;
		if (!rend || !rend->getSelectionContainer())
			return;
		//无结果帧数据时代理为空，UpdatePostWidgetHighlight 系列会解引用空指针，跳过
		if (!rend->getPostRender() || !rend->getPostRender()->frameProxy(rend->getPostRender()->currentFrame()))
			return;
		MechPreData::ClearPostWidgetMeshPick(rend);
		SectionData* sec = activeSection();
		if (sec)
		{
			//以当前截面集合同步视口选择容器（单元dim=2/part_id=0照mPostSetPanel按编号高亮先例）
			std::vector<MeshSelectInfoData> elems, nodes;
			for (int id : sec->elementIds)
			{
				MeshSelectInfoData d;
				d.part_id = 0;
				d.dim = 2;
				d.data_id.element_id = id;
				d.data_id.node_id = id;
				elems.push_back(d);
			}
			for (int id : sec->nodeIds)
			{
				MeshSelectInfoData d;
				d.data_id.node_id = id;
				d.data_id.element_id = id;
				nodes.push_back(d);
			}
			auto* meshSelectData = rend->getSelectionContainer()->getMeshSelectData();
			if (!elems.empty())
				meshSelectData->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshElement, elems);
			if (!nodes.empty())
				meshSelectData->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshNode, nodes);
		}
		MechPreData::UpdatePostWidgetHighlightWithoutRTRC(rend);
		rend->updatePaint();
	}

	QHash<int, QList<int>> mPostSectionForce::collectElementNodeMap()
	{
		SectionData* sec = activeSection();
		QHash<int, QList<int>> out;
		if (!sec || sec->elementIds.isEmpty() || !_postRend || !_postRend->getPostRender())
			return out;
		auto* frameProxy = _postRend->getPostRender()->frameProxy(_postRend->getPostRender()->currentFrame());   // S5 门面直接替换：getCurFrameDataProxy→frameProxy(currentFrame())（F8 现取）
		if (!frameProxy || !frameProxy->getModelRenderDataProxy())
			return out;
		auto* modelProxy = frameProxy->getModelRenderDataProxy();

		//逐单元收集其全部节点（真实ID，单元内去重）
		for (int eleId : sec->elementIds)
		{
			int partId = -1;
			if (!modelProxy->getPartIdByEleId(eleId, partId))
				continue;
			std::vector<int> nodes;
			modelProxy->getNodeIdsByEleIdWithoutBuild(partId, eleId, nodes);
			std::set<int> uniq(nodes.cbegin(), nodes.cend());
			QList<int> list;
			list.reserve(int(uniq.size()));
			for (int nodeId : uniq)
				list.append(nodeId);
			out.insert(eleId, list);
		}
		return out;
	}

	QList<int> mPostSectionForce::collectElementNodeIds()
	{
		//合并各单元节点（全局去重）
		std::set<int> uniq;
		const auto eleNodeMap = collectElementNodeMap();
		for (auto it = eleNodeMap.cbegin(); it != eleNodeMap.cend(); ++it)
			for (int nodeId : it.value())
				uniq.insert(nodeId);
		QList<int> out;
		out.reserve(int(uniq.size()));
		for (int nodeId : uniq)
			out.append(nodeId);
		return out;
	}

	QList<int> mPostSectionForce::collectOutputNodeIds()
	{
		SectionData* sec = activeSection();
		if (!sec || sec->elementIds.isEmpty())
			return {};
		//勾选忽略节点集：选中的节点视为未选中，按只有单元的逻辑输出全部单元节点
		const bool ignoreNodes = !_checkIgnoreNode || _checkIgnoreNode->isChecked();
		//未限定节点：输出所选单元的全部节点；有限定节点：仅输出位于所选单元上的节点（交集）
		if (ignoreNodes || sec->nodeIds.isEmpty())
			return collectElementNodeIds();

		const QList<int> elemNodes = collectElementNodeIds();
		if (elemNodes.isEmpty())
			return {};
		std::set<int> elemSet(elemNodes.cbegin(), elemNodes.cend());
		QList<int> out;
		for (int nodeId : sec->nodeIds)
			if (elemSet.count(nodeId) > 0)
				out.append(nodeId);
		return out;
	}

	QString mPostSectionForce::formatSigValueText(double value) const
	{
		//弹窗懒创建（控件为空）时按默认：科学计数法、3位
		int prec = 3;
		if (_dsPrecEdit)
		{
			bool ok = false;
			const int v = _dsPrecEdit->text().toInt(&ok);
			if (ok && v >= 0 && v <= 15)
				prec = v;
		}
		const int notation = _dsNotationCombo ? _dsNotationCombo->currentIndex() : 0;
		//科学计数法=定点指数（小数点后prec位）、小数=定点小数（小数点后prec位）、常规=prec位有效数字
		const char fmt = notation == 1 ? 'f' : notation == 2 ? 'g' : 'e';
		return QString::number(value, fmt, prec);
	}

	QHash<int, QVector3D> mPostSectionForce::readSectionNodalForceField()
	{
		SectionData* sec = activeSection();
		if (!sec || sec->elementIds.isEmpty() || !_postRend || !_postDB)
			return {};
		auto* guidata = getCVM->getGUIDataByPostRender(_postRend.data());
		if (!guidata)
			return {};

		//按帧缓存：显示设置联动重绘（分量/尺寸/精度等）直接复用，免全量重读文件
		const int frame = guidata->_cloudFrame;
		if (_nodalForceFrame == frame)
			return _nodalForceField;

		//X/Y/Z分量索引按变量分量表定位（无对应名时回落前三分量）
		const QStringList compos = _postDB->getVariableCompos(QString("ElementNodalForce"));
		int ix = compos.indexOf(QString("X"));
		int iy = compos.indexOf(QString("Y"));
		int iz = compos.indexOf(QString("Z"));
		if (ix < 0 || iy < 0 || iz < 0)
		{
			ix = 0;
			iy = 1;
			iz = 2;
		}

		//仅解码所选单元（reader对其余单元seekg跳过），出参读取不写数据库缓存、不污染云图当前显示态
		QHash<int, QHash<int, QVector<double>>> eleNodeVals;
		MIOFile::mIMxdbFile1 reader;
		const std::vector<int> eleFilter(sec->elementIds.cbegin(), sec->elementIds.cend());
		if (!reader.readElementNodeCompValues(_postRend->getPostDataName(), frame,
			QString("ElementNodalForce"), eleNodeVals, eleFilter))
			return {};

		//按所选单元逐节点求和（自由体力约定：网格点合力=所属截面单元在该点的贡献之和）
		QHash<int, QVector3D> field;
		for (auto it = eleNodeVals.cbegin(); it != eleNodeVals.cend(); ++it)
		{
			for (auto nit = it->cbegin(); nit != it->cend(); ++nit)
			{
				const QVector<double>& comps = nit.value();
				auto compAt = [&comps](int idx) {
					return (idx >= 0 && idx < comps.size()) ? comps.at(idx) : 0.0;
					};
				field[nit.key()] += QVector3D(compAt(ix), compAt(iy), compAt(iz));
			}
		}
		_nodalForceField = field;
		_nodalForceFrame = frame;
		return field;
	}

	void mPostSectionForce::refreshForceMomentSymbols()
	{
		if (_dsBulkUpdating || !_postRend)
			return;
		auto* symbolController = _postRend->GetSymbolController();
		if (!symbolController)
			return;

		//显示设置勾选的分量组（力/力矩各7组，X/Y/Z/X+Y/Y+Z/Z+X/X+Y+Z）
		QList<int> forceGroups, momentGroups;
		for (int i = 0; i < _dsForceChecks.size(); ++i)
			if (_dsForceChecks.at(i)->isChecked())
				forceGroups.append(i);
		for (int i = 0; i < _dsMomentChecks.size(); ++i)
			if (_dsMomentChecks.at(i)->isChecked())
				momentGroups.append(i);
		//显示设置弹窗懒创建：未创建时按默认分量组（力X/力矩Z）
		if (_dsForceChecks.isEmpty())
			forceGroups = { 0 };
		if (_dsMomentChecks.isEmpty())
			momentGroups = { 2 };

		//输出节点与坐标（未变形坐标+当前帧位移，对齐测量工具的节点定位方式）
		const QList<int> nodeIds = (forceGroups.isEmpty() && momentGroups.isEmpty())
			? QList<int>() : collectOutputNodeIds();
		mPostModelData* modeldata = nullptr;
		QHash<int, QVector3D> dis;
		QHash<int, QVector3D> nodalForce;	//真实节点力（真实节点id→矢量；云图变量为ElementNodalForce时读取）
		if (!nodeIds.isEmpty() && _postDB)
		{
			if (auto* guidata = getCVM->getGUIDataByPostRender(_postRend.data()))
			{
				modeldata = _postDB->getModelData(guidata->_cloudFrame);
				if (modeldata)
				{
					dis = _postDB->getDisplacementDataByFrameID(guidata->_cloudFrame);
					if (guidata->_cloudVarName == QString("ElementNodalForce"))
						nodalForce = readSectionNodalForceField();
				}
			}
		}

		auto nodePos = [&modeldata, &dis](int nodeId) {
			return modeldata->getVectorById(nodeId)
				+ dis.value(modeldata->_nodedata.nodeidmap.value(nodeId));
			};

		//合力模式：简化中心与三分量合计只算一次（与分量组无关）——数据来自输出点
		//（有限定节点且未忽略节点集时为交集节点）；合力=ΣFi，合力矩=ΣMi+Σ(ri×Fi)
		//简化中心按求和点设置：形心=所选单元全部节点均值（默认，不随节点限定移动）；
		//节点=求和节点位置（节点不存在时回落形心）；坐标=输入的全局坐标
		QVector3D center(0, 0, 0), forceSum(0, 0, 0), momentSum(0, 0, 0);
		const bool resultantMode = modeldata && _calcTypeChoice->currentIndex() == 1;
		if (resultantMode)
		{
			bool centerSet = false;
			SectionData* secR = activeSection();
			if (secR && secR->sumPoint == 1 && !secR->nodeName.isEmpty())
			{
				bool ok = false;
				const int sumNodeId = secR->nodeName.toInt(&ok);
				if (ok && modeldata->_nodedata.nodeidmap.contains(sumNodeId))
				{
					center = nodePos(sumNodeId);
					centerSet = true;
				}
			}
			else if (secR && secR->sumPoint == 2)
			{
				center = QVector3D(float(secR->x), float(secR->y), float(secR->z));
				centerSet = true;
			}
			if (!centerSet)
			{
				QList<int> centerNodes = collectElementNodeIds();
				if (centerNodes.isEmpty())
					centerNodes = nodeIds;	//兜底：单元节点不可得时退回输出点均值
				for (int nodeId : centerNodes)
					center += nodePos(nodeId);
				center /= float(centerNodes.size());
			}

			for (int nodeId : nodeIds)
			{
				const QVector3D pos = nodePos(nodeId);
				const QVector3D fi = nodalForce.value(nodeId);
				forceSum += fi;
				//单元节点力无力矩分量，合力矩=Σ(ri×Fi)
				momentSum += QVector3D::crossProduct(pos - center, fi);
			}
		}

		//每个勾选分量组独立一个ForceMoment符号（名称带组索引），各组箭头颜色对应各自色块；
		//未勾选/无条目的组删除对应符号
		int drawnGroups = 0;
		for (int g = 0; g < 7; ++g)
		{
			for (int side = 0; side < 2; ++side)	//0力/1力矩
			{
				const bool isForce = (side == 0);
				const QList<int>& groups = isForce ? forceGroups : momentGroups;
				const QString name = QString::fromLatin1(isForce ? "SectionForce_F_%1" : "SectionForce_M_%1").arg(g);
				if (!modeldata || !groups.contains(g))
				{
					if (symbolController->isExistSymbol(name))
						symbolController->deleteSymbol(std::vector<QString>{ name });
					continue;
				}

				const int mask = kCompGroupMask[g];
				const char* prefix = isForce ? "F" : "M";
				QVector<std::tuple<QVector3D, QVector3D, QString>> entries;
				if (resultantMode)
				{
					//合力条目：位置为简化中心，方向为该组分量合力方向，文本为合力的模（不带方向前缀）
					const QVector3D& total = isForce ? forceSum : momentSum;
					QVector3D vec(0, 0, 0);
					for (int a = 0; a < 3; ++a)
						if (mask & (1 << a))
							vec[a] = total[a];
					const double value = double(vec.length());
					if (value > 0.0)
						entries.append(std::make_tuple(center, vec.normalized(),
							QString("%1=%2").arg(QLatin1String(prefix)).arg(formatSigValueText(value))));
				}
				else if (isForce)
				{
					//自由体力条目：逐节点取真实节点力按组掩码投影（无数据/零值节点不绘制），
					//文本按分量组名加前缀（FX=/FY=…）
					for (int nodeId : nodeIds)
					{
						const QVector3D pos = nodePos(nodeId);
						const QVector3D full = nodalForce.value(nodeId);
						QVector3D vec(0, 0, 0);
						for (int a = 0; a < 3; ++a)
							if (mask & (1 << a))
								vec[a] = full[a];
						const double value = double(vec.length());
						if (value <= 0.0)
							continue;
						entries.append(std::make_tuple(pos, vec.normalized(),
							QString("%1%2=%3").arg(QLatin1String(prefix))
								.arg(QLatin1String(kCompGroupNames[g])).arg(formatSigValueText(value))));
					}
				}
				//自由体力模式的力矩：单元节点力无力矩数据，不产生条目（entries为空时符号一并删除）；
				//力矩仅在合力模式下创建（Σ ri×Fi，作用于求和点）

				if (entries.isEmpty())
				{
					if (symbolController->isExistSymbol(name))
						symbolController->deleteSymbol(std::vector<QString>{ name });
					continue;
				}

				auto* fm = symbolController->getOrCreateSymbolData<mxr::ForceMoment>(name);
				fm->_forcePosNorText = isForce ? entries : QVector<std::tuple<QVector3D, QVector3D, QString>>();
				fm->_momentPosNorText = isForce ? QVector<std::tuple<QVector3D, QVector3D, QString>>() : entries;
				//颜色取该组色块色（渲染端顶点色为0-1归一化标度，redF/greenF/blueF 传入）
				const QColor c = (isForce ? _dsForceColors : _dsMomentColors).at(g);
				const QVector3D col(c.redF(), c.greenF(), c.blueF());
				if (isForce)
					fm->_forceColor = col;
				else
					fm->_momentColor = col;
				fm->_textColor = QVector3D(0, 0, 0);
				fm->_enableLineWidth = true;
				fm->_lineWidth = 1.5f;
				//符号尺寸：自动=固定屏幕尺寸自适应（默认值，忽略输入）；统一=固定屏幕尺寸取输入值；缩放=世界尺寸取输入值（随视图缩放）
				//显示设置弹窗懒创建：未创建时尺寸控件无选项，按自动档默认值（与分量复选的懒创建兜底同型）
				const int sizeMode = _dsSizeChoice ? _dsSizeChoice->currentIndex() : 0;
				if (sizeMode <= 0)
				{
					fm->_size = 1.5f;
					fm->_isFixedPixel = true;
				}
				else
				{
					const double v = _dsSizeEdit->text().toDouble();
					fm->_size = v > 0.0 ? float(v) : 1.0f;
					fm->_isFixedPixel = sizeMode != 2;	//缩放=世界尺寸
				}
				//符号形式：锚点位置=头部/中点/尾部（_centerPos：1尾部/2中部/3头部）、样式=箭头/线条
				//显示设置弹窗懒创建：未创建时控件无选项，按默认（头部、箭头）
				const int posIdx = _dsPosChoice ? _dsPosChoice->currentIndex() : 0;
				fm->_centerPos = posIdx == 1 ? 2 : (posIdx == 2 ? 1 : 3);
				fm->_arwSymType = _dsShapeCombo && _dsShapeCombo->currentIndex() == 1;
				fm->_depthState = false;		//不被模型遮挡
				fm->_textDepthState = false;
				fm->_showState = true;
				symbolController->createSymbol(fm);
				++drawnGroups;
			}
		}

		//无任何输出组时整体清理（单元清空/分量全不勾等）
		if (drawnGroups == 0)
		{
			clearForceMomentSymbols();
			return;
		}
		symbolController->UpdateAllSymbol();
		_postRend->updatePaint();
	}

	void mPostSectionForce::clearForceMomentSymbols()
	{
		if (!_postRend)
			return;
		auto* symbolController = _postRend->GetSymbolController();
		if (!symbolController)
			return;
		//按分量组分符号清理（力/力矩各7组），并兼容清理旧版整体符号
		std::vector<QString> names;
		for (int g = 0; g < 7; ++g)
		{
			const QString f = QString::fromLatin1("SectionForce_F_%1").arg(g);
			const QString m = QString::fromLatin1("SectionForce_M_%1").arg(g);
			if (symbolController->isExistSymbol(f))
				names.push_back(f);
			if (symbolController->isExistSymbol(m))
				names.push_back(m);
		}
		if (symbolController->isExistSymbol(kSectionForceSymbolName))
			names.push_back(QString::fromLatin1(kSectionForceSymbolName));
		if (names.empty())
			return;
		symbolController->deleteSymbol(names);
		symbolController->UpdateAllSymbol();
		_postRend->updatePaint();
	}

	void mPostSectionForce::invalidateCalcResult()
	{
		_hasCalcResult = false;
		_calcSectionId = -1;
		_nodalForceField.clear();	//已计算结果失效时节点力场缓存一并失效（重算时按新拾取重读）
		_nodalForceFrame = -1;
		if (_btnClear)
			_btnClear->setEnabled(false);
		clearForceMomentSymbols();
	}

	void mPostSectionForce::slot_calcClicked()
	{
		//计算前提交激活拾取条的在途数据（栈标准口径；_working 已随 dataChanged 即时同步，兜底）
		mPostPickWidgetController::commitActivePickWidget();
		int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
		SectionData* sec = activeSection();
		if (!sec)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(lang == 0
				? QString::fromUtf8("请先新建并选中截面")
				: QString("Create and select a section first"));
			return;
		}
		if (sec->elementIds.isEmpty())
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(lang == 0
				? QString::fromUtf8("请先拾取单元")
				: QString("Please pick elements first"));
			return;
		}
		if (!_postRend || !_postDB)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(lang == 0
				? QString::fromUtf8("无可用后处理视口或结果数据")
				: QString("No post viewport or result data available"));
			return;
		}

		//节点力数据依托云图变量ElementNodalForce（变量未切换时无数据可读）
		auto* guidata = getCVM->getGUIDataByPostRender(_postRend.data());
		if (!guidata || guidata->_cloudVarName != QString("ElementNodalForce"))
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(lang == 0
				? QString::fromUtf8("请先将云图变量切换为ElementNodalForce再计算截面力")
				: QString("Switch the contour variable to ElementNodalForce before calculating section force"));
			return;
		}

		//绘制仅在点击计算后进行；显示设置改动在已计算状态下实时重绘
		refreshForceMomentSymbols();
		_hasCalcResult = true;
		_calcSectionId = sec->id;
		_btnClear->setEnabled(true);
	}

	void mPostSectionForce::slot_clearClicked()
	{
		invalidateCalcResult();
	}


	void mPostSectionForce::ensurePickPopup()
	{
		if (_pickPopup) return;

		_pickPopup = new QWidget(this, Qt::Popup | Qt::FramelessWindowHint);
		//样式必须带 ID 选择器：无选择器规则会级联到全部子部件
		_pickPopup->setObjectName("pickPopup");
		_pickPopup->setAttribute(Qt::WA_StyledBackground, true);
		_pickPopup->setStyleSheet("#pickPopup{background:#FFFFFF;border:1px solid #DCE2E9;border-radius:3px;}");
		_pickPopup->setFixedSize(300, 176);

		auto* popupLayout = new QVBoxLayout(_pickPopup);
		popupLayout->setContentsMargins(1, 1, 1, 1);
		popupLayout->setSpacing(0);

		//标题栏
		QWidget* head = new QWidget(_pickPopup);
		head->setFixedHeight(28);
		auto* headLayout = new QHBoxLayout(head);
		headLayout->setContentsMargins(12, 0, 6, 0);
		_pickTitleText = new QLabel(QString::fromUtf8("选择单元"), head);
		_pickTitleText->setStyleSheet("border:0;background:transparent;font-weight:600;color:#424B57;");
		headLayout->addWidget(_pickTitleText);
		headLayout->addStretch();
		QPushButton* btnClose = new QPushButton(QString::fromUtf8("×"), head);
		btnClose->setStyleSheet("QPushButton{border:0;background:transparent;color:#788391;font-size:14px;}"
			"QPushButton:hover{background:#DCE2E9;}");
		btnClose->setFixedSize(22, 22);
		headLayout->addWidget(btnClose);
		popupLayout->addWidget(head);

		auto* body = new QWidget(_pickPopup);
		auto* bodyLayout = new QVBoxLayout(body);
		bodyLayout->setContentsMargins(12, 12, 12, 12);
		bodyLayout->setSpacing(8);
		popupLayout->addWidget(body, 1);

		const QString labelQss = QString("QLabel{color:%1;background:transparent;font-size:12px;border:0;}").arg(kText);

		//选择操作（替换/添加/移除）
		QWidget* opRow = new QWidget(body);
		QHBoxLayout* opLayout = new QHBoxLayout(opRow);
		opLayout->setContentsMargins(0, 0, 0, 0);
		opLayout->setSpacing(8);
		_pickOpText = new QLabel(QString::fromUtf8("选择操作"), opRow);
		_pickOpText->setStyleSheet(labelQss);
		opLayout->addWidget(_pickOpText);
		_pickOpCombo = new QComboBox(opRow);
		_pickOpCombo->setFixedHeight(26);
		opLayout->addWidget(_pickOpCombo, 1);
		bodyLayout->addWidget(opRow);

		_pickIdsText = new QLabel(QString::fromUtf8("实体编号"), body);
		_pickIdsText->setStyleSheet(labelQss);
		bodyLayout->addWidget(_pickIdsText);

		_pickEntryEdit = new QTextEdit(body);
		_pickEntryEdit->setPlaceholderText(QString::fromUtf8("例如：1, 2, 5-10"));
		_pickEntryEdit->setMinimumHeight(26);	//与求和节点弹窗单行输入框同高
		_pickEntryEdit->setStyleSheet("QTextEdit{background:#FFFFFF;border:1px solid #DCE2E9;border-radius:2px;}"
			"QTextEdit:focus{border:1px solid #1890FF;}");
		bodyLayout->addWidget(_pickEntryEdit, 1);

		//确认（靠底靠右）
		QWidget* okRow = new QWidget(body);
		QHBoxLayout* okLayout = new QHBoxLayout(okRow);
		okLayout->setContentsMargins(0, 0, 0, 0);
		okLayout->setSpacing(6);
		okLayout->addStretch();
		_pickOkBtn = new MxFunctions::mPrimaryButton(QString(), okRow);
		_pickOkBtn->setFixedSize(64, 26);
		okLayout->addWidget(_pickOkBtn);
		bodyLayout->addWidget(okRow);

		_pickOpCombo->addItems({ QString::fromUtf8("替换"), QString::fromUtf8("添加"), QString::fromUtf8("移除") });

		connect(btnClose, &QPushButton::clicked, _pickPopup, &QWidget::close);
		connect(_pickOkBtn, &QPushButton::clicked, this, &mPostSectionForce::slot_confirmPick);
	}

	void mPostSectionForce::applyPickPopupLanguage(int lang)
	{
		if (!_pickTitleText) return;

		_pickTitleText->setText(_pickTarget == 0 ? (lang == 0 ? QString::fromUtf8("选择单元") : QString("Select Elements"))
			: (lang == 0 ? QString::fromUtf8("选择节点") : QString("Select Nodes")));
		_pickOpText->setText(lang == 0 ? QString::fromUtf8("选择操作") : QString("Operation"));
		_pickIdsText->setText(_pickTarget == 0 ? (lang == 0 ? QString::fromUtf8("实体编号") : QString("Element IDs"))
			: (lang == 0 ? QString::fromUtf8("节点编号") : QString("Node IDs")));
		_pickOkBtn->setText(lang == 0 ? QString::fromUtf8("确认") : QString("OK"));
		_pickOpCombo->blockSignals(true);
		_pickOpCombo->setItemText(0, lang == 0 ? QString::fromUtf8("替换") : QString("Replace"));
		_pickOpCombo->setItemText(1, lang == 0 ? QString::fromUtf8("添加") : QString("Add"));
		_pickOpCombo->setItemText(2, lang == 0 ? QString::fromUtf8("移除") : QString("Remove"));
		_pickOpCombo->blockSignals(false);
	}

	void mPostSectionForce::updatePickHint()
	{
		if (!_pickHintLabel) return;
		int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
		SectionData* sec = activeSection();
		const bool valid = sec != nullptr;
		const bool hasElem = valid && !sec->elementIds.isEmpty();
		const bool hasNode = valid && !sec->nodeIds.isEmpty();

		//进度提示：未拾取单元→必选提示；仅单元（或勾选忽略节点集）→按全部单元节点；单元+节点→交集说明
		//（拾取条自身文字"选择X/已选择N个"由栈管理，此处只管规则提示行）
		const bool ignoreNodes = _checkIgnoreNode && _checkIgnoreNode->isChecked();
		QString hint;
		if (!valid || !hasElem)
			hint = lang == 0 ? QString::fromUtf8("必须拾取单元；仅拾取节点不能切出。")
				: QString("Elements are required; nodes alone cannot cut the section.");
		else if (!hasNode)
			hint = lang == 0 ? QString::fromUtf8("未限定节点:输出所选单元的全部节点。")
				: QString("No node limit: all nodes of the selected elements are output.");
		else if (ignoreNodes)
			hint = lang == 0 ? QString::fromUtf8("已勾选忽略节点集:选定节点不参与，按所选单元的全部节点计算。")
				: QString("Ignore node sets checked: picked nodes are skipped, all nodes of the selected elements are used.");
		else
		{
			const int picked = sec->nodeIds.size();
			//位于所选单元上的节点数：按单元—节点拓扑对限定节点取交集
			const int onUnits = collectOutputNodeIds().size();
			hint = lang == 0
				? QString::fromUtf8("已选 %1 个节点，其中 %2 个位于所选单元上；仅输出交集节点.").arg(picked).arg(onUnits)
				: QString("%1 nodes selected, %2 of them on the selected elements; only intersection nodes are output.")
					.arg(picked).arg(onUnits);
		}
		_pickHintLabel->setText(hint);
		_pickHintLabel->setVisible(valid);
	}

	void mPostSectionForce::slot_showPickPopup(int target)
	{
		SectionData* sec = activeSection();
		if (!sec)
			return;	//无编辑中截面不弹（按钮已置灰，兜底）
		deactivatePickBars();	//弹窗打开退出瞬时拾取态（原型互斥模式）

		if (!_pickPopup) ensurePickPopup();
		_pickTarget = target;
		applyPickPopupLanguage(MAppConfiguration::mAppConfiguration::getInstance()->_language);

		//预填现有编号（逗号分隔）
		const QList<int>& ids = target == 0 ? sec->elementIds : sec->nodeIds;
		QStringList parts;
		for (int id : ids)
			parts << QString::number(id);
		_pickEntryEdit->setPlainText(parts.join(QString::fromUtf8(", ")));
		_pickOpCombo->setCurrentIndex(0);

		//锚定"编号"小钮行右侧、与按钮纵向对齐；越界时回落到屏幕内
		QWidget* anchor = target == 0 ? _btnElemIds : _btnNodeIds;
		const QPoint btnGlobal = anchor->mapToGlobal(QPoint(0, 0));
		QPoint pos(mapToGlobal(QPoint(width(), 0)).x(), btnGlobal.y() - 2);
		QScreen* scr = QGuiApplication::screenAt(pos);
		const QRect avail = (scr ? scr : QGuiApplication::primaryScreen())->availableGeometry();
		if (pos.x() + _pickPopup->width() > avail.right())
			pos.setX(avail.right() - _pickPopup->width() - 4);
		if (pos.x() < avail.left())
			pos.setX(avail.left() + 4);
		pos.setY(qBound(avail.top(), pos.y(), qMax(avail.top(), avail.bottom() - _pickPopup->height())));
		_pickPopup->move(pos);
		_pickPopup->show();
	}

	void mPostSectionForce::slot_confirmPick()
	{
		SectionData* sec = activeSection();
		if (!sec)
		{
			if (_pickPopup) _pickPopup->close();
			return;
		}

		int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
		QList<int> ids;
		const QString text = _pickEntryEdit->toPlainText().trimmed();
		if (!text.isEmpty() && !parseIdRanges(text, ids))
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(lang == 0
				? QString::fromUtf8("编号格式无效，请输入正整数编号或范围（如 1, 2, 5-10）")
				: QString("Invalid IDs. Use positive integers or ranges (e.g. 1, 2, 5-10)"));
			return;
		}

		//按选择操作回写：0替换/1添加/2移除
		QList<int>& targetIds = _pickTarget == 0 ? sec->elementIds : sec->nodeIds;
		switch (_pickOpCombo->currentIndex())
		{
		case 1:
			for (int id : ids)
				if (!targetIds.contains(id))
					targetIds.append(id);
			break;
		case 2:
			for (int id : ids)
				targetIds.removeOne(id);
			break;
		default:
			targetIds = ids;
			break;
		}
		std::sort(targetIds.begin(), targetIds.end());

		if (_pickPopup) _pickPopup->close();
		updatePickHint();
		notifyEditChanged();	//V1.3：编号回写编辑缓冲
		refreshPickBars();			//拾取条回显编号结果
		refreshCombinedHighlight();	//回写后视口高亮跟随当前截面的单元/节点集合
		invalidateCalcResult();	//编号回写后已计算结果失效，需重新点击计算
	}
	void mPostSectionForce::ensureSumNodePopup()
	{
		if (_sumNodePopup) return;

		_sumNodePopup = new QWidget(this, Qt::Popup | Qt::FramelessWindowHint);
		//样式必须带 ID 选择器：无选择器规则会级联到全部子部件
		_sumNodePopup->setObjectName("snPopup");
		_sumNodePopup->setAttribute(Qt::WA_StyledBackground, true);
		_sumNodePopup->setStyleSheet("#snPopup{background:#FFFFFF;border:1px solid #DCE2E9;border-radius:3px;}");
		_sumNodePopup->setFixedSize(300, 150);

		auto* popupLayout = new QVBoxLayout(_sumNodePopup);
		popupLayout->setContentsMargins(1, 1, 1, 1);
		popupLayout->setSpacing(0);

		//标题栏
		QWidget* head = new QWidget(_sumNodePopup);
		head->setFixedHeight(28);
		auto* headLayout = new QHBoxLayout(head);
		headLayout->setContentsMargins(12, 0, 6, 0);
		_snTitleText = new QLabel(QString::fromUtf8("选择求和节点"), head);
		_snTitleText->setStyleSheet("border:0;background:transparent;font-weight:600;color:#424B57;");
		headLayout->addWidget(_snTitleText);
		headLayout->addStretch();
		QPushButton* btnClose = new QPushButton(QString::fromUtf8("×"), head);
		btnClose->setStyleSheet("QPushButton{border:0;background:transparent;color:#788391;font-size:14px;}"
			"QPushButton:hover{background:#DCE2E9;}");
		btnClose->setFixedSize(22, 22);
		headLayout->addWidget(btnClose);
		popupLayout->addWidget(head);

		auto* body = new QWidget(_sumNodePopup);
		auto* bodyLayout = new QVBoxLayout(body);
		bodyLayout->setContentsMargins(12, 12, 12, 12);
		bodyLayout->setSpacing(8);
		popupLayout->addWidget(body, 1);

		const QString labelQss = QString("QLabel{color:%1;background:transparent;font-size:12px;border:0;}").arg(kText);

		_snIdsText = new QLabel(QString::fromUtf8("实体编号"), body);
		_snIdsText->setStyleSheet(labelQss);
		bodyLayout->addWidget(_snIdsText);

		_snEdit = new QLineEdit(body);
		_snEdit->setFixedHeight(26);
		_snEdit->setPlaceholderText(QString::fromUtf8("请输入一个节点编号"));
		_snEdit->setValidator(new QIntValidator(1, 2147483647, _snEdit));
		bodyLayout->addWidget(_snEdit);

		//确认
		QWidget* okRow = new QWidget(body);
		QHBoxLayout* okLayout = new QHBoxLayout(okRow);
		okLayout->setContentsMargins(0, 0, 0, 0);
		okLayout->setSpacing(6);
		okLayout->addStretch();
		_snOkBtn = new MxFunctions::mPrimaryButton(QString(), okRow);
		_snOkBtn->setFixedSize(64, 26);
		okLayout->addWidget(_snOkBtn);
		bodyLayout->addWidget(okRow);

		connect(btnClose, &QPushButton::clicked, _sumNodePopup, &QWidget::close);
		connect(_snOkBtn, &QPushButton::clicked, this, &mPostSectionForce::slot_confirmSumNode);
	}

	void mPostSectionForce::slot_showSumNodePopup()
	{
		SectionData* sec = activeSection();
		if (!sec)
			return;	//无编辑中截面不弹
		deactivatePickBars();	//弹窗打开退出瞬时拾取态（原型互斥模式）

		if (!_sumNodePopup) ensureSumNodePopup();
		int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
		_snTitleText->setText(lang == 0 ? QString::fromUtf8("选择求和节点") : QString("Select Sum Node"));
		_snIdsText->setText(lang == 0 ? QString::fromUtf8("实体编号") : QString("Entity ID"));
		_snEdit->setPlaceholderText(lang == 0 ? QString::fromUtf8("请输入一个节点编号") : QString("Enter a node number"));
		_snOkBtn->setText(lang == 0 ? QString::fromUtf8("确认") : QString("OK"));
		_snEdit->setText(sec->nodeName);

		//锚定求和节点"编号"小钮行右侧、与按钮纵向对齐；越界时回落到屏幕内
		const QPoint btnGlobal = _btnSumNodeIds->mapToGlobal(QPoint(0, 0));
		QPoint pos(mapToGlobal(QPoint(width(), 0)).x(), btnGlobal.y() - 2);
		QScreen* scr = QGuiApplication::screenAt(pos);
		const QRect avail = (scr ? scr : QGuiApplication::primaryScreen())->availableGeometry();
		if (pos.x() + _sumNodePopup->width() > avail.right())
			pos.setX(avail.right() - _sumNodePopup->width() - 4);
		if (pos.x() < avail.left())
			pos.setX(avail.left() + 4);
		pos.setY(qBound(avail.top(), pos.y(), qMax(avail.top(), avail.bottom() - _sumNodePopup->height())));
		_sumNodePopup->move(pos);
		_sumNodePopup->show();
	}

	void mPostSectionForce::slot_confirmSumNode()
	{
		SectionData* sec = activeSection();
		if (!sec)
		{
			if (_sumNodePopup) _sumNodePopup->close();
			return;
		}


		int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
		const QString text = _snEdit->text().trimmed();
		bool ok = false;
		const long long id = text.toLongLong(&ok);
		if (text.isEmpty() || !ok || id < 1)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(lang == 0
				? QString::fromUtf8("请输入一个有效的节点编号")
				: QString("Please enter a valid node number"));
			return;
		}

		sec->nodeName = QString::number(id);
		if (_sumNodePopup) _sumNodePopup->close();
		refreshPickBars();		//求和节点拾取条回显编号
		notifyEditChanged();	//V1.3：求和节点写编辑缓冲
		//求和点=节点：求和节点变化联动合力简化中心，已计算时实时重绘
		if (_hasCalcResult)
			refreshForceMomentSymbols();
	}
	void mPostSectionForce::slot_showDsColorPopup(int index)
	{
		deactivatePickBars();	//弹窗打开退出瞬时拾取态（原型互斥模式）
		if (!_colorPicker)
			ensureColorPopup();
		_colorTarget = 1;
		_dsColorIndex = index;
		const QColor c = index < 7 ? _dsForceColors.at(index) : _dsMomentColors.at(index - 7);
		_colorPicker->setCurrentColor(c);

		//锚定色块下方
		QLabel* sw = index < 7 ? _dsForceSwatches.at(index) : _dsMomentSwatches.at(index - 7);
		const QPoint pos = sw->mapToGlobal(QPoint(sw->width() + 4, 0));
		_colorPicker->popupAt(pos);
	}

	void mPostSectionForce::applyDsSwatchColor(int index, const QColor& color)
	{
		if (index < 0 || index > 13)
			return;
		const QString style = QString("background:%1;border:1px solid %2;border-radius:2px;")
			.arg(color.name()).arg(kCtrlBorder);
		if (index < 7)
		{
			_dsForceColors[index] = color;
			_dsForceSwatches.at(index)->setStyleSheet(style);
		}
		else
		{
			_dsMomentColors[index - 7] = color;
			_dsMomentSwatches.at(index - 7)->setStyleSheet(style);
		}
		if (_hasCalcResult)
			refreshForceMomentSymbols();	//已计算时取色实时反映到力/力矩符号（批量回填期间由守卫跳过）
	}

	bool mPostSectionForce::eventFilter(QObject* watched, QEvent* event)
	{
		//显示设置弹窗的色块点击 → 打开取色（光标手型在创建时已设置）
		if (event->type() == QEvent::MouseButtonPress)
		{
			const int fi = _dsForceSwatches.indexOf(qobject_cast<QLabel*>(watched));
			if (fi >= 0)
			{
				slot_showDsColorPopup(fi);
				return true;	//消费按压：避免事件上传触发显示设置弹窗自动关闭（连带关闭取色器）
			}
			const int mi = _dsMomentSwatches.indexOf(qobject_cast<QLabel*>(watched));
			if (mi >= 0)
			{
				slot_showDsColorPopup(7 + mi);
				return true;	//同上
			}
		}
		return QWidget::eventFilter(watched, event);
	}
	void mPostSectionForce::ensureColorPopup()
	{
		if (_colorPicker)
			return;
		_colorPicker = new FbdColorPicker(this);
		connect(_colorPicker, &FbdColorPicker::colorChanged, this, [this](const QColor& color) {
			//显示设置色块取色
			if (_colorTarget == 1)
			{
				applyDsSwatchColor(_dsColorIndex, color);
				return;
			}
			//实时同步到截面数据与列表颜色格
			const int di = dataIndexById(_colorSectionId);
			if (di < 0)
				return;
			_sections[di].color = color;
			for (int r = 0; r < _table->rowCount(); ++r)
			{
				QTableWidgetItem* nameIt = _table->item(r, 1);
				if (nameIt && nameIt->data(Qt::UserRole).toInt() == _colorSectionId)
				{
					_table->item(r, 4)->setBackground(QBrush(color));
					break;
				}
			}
			//V1.3：编辑中截面的快捷换色同步缓冲且不计 dirty（原型 rebase）
			if (_colorSectionId == _editingId)
			{
				_working.color = color;
				if (_editor)
					_editor->rebase();
			}
			});
	}
	void mPostSectionForce::slot_showColorPopup(int row)
	{
		deactivatePickBars();	//弹窗打开退出瞬时拾取态（原型互斥模式）
		if (!_colorPicker)
			ensureColorPopup();

		QTableWidgetItem* nameIt = _table->item(row, 1);
		if (!nameIt)
			return;
		_colorSectionId = nameIt->data(Qt::UserRole).toInt();
		const int di = dataIndexById(_colorSectionId);
		if (di < 0)
			return;
		_colorPicker->setCurrentColor(_sections.at(di).color);

		//锚定颜色单元格下方
		QTableWidgetItem* colorIt = _table->item(row, 4);
		const QPoint pos = _table->visualItemRect(colorIt).bottomLeft()
			+ _table->viewport()->mapToGlobal(QPoint(0, 0));
		_colorPicker->popupAt(pos);
	}
void mPostSectionForce::ensureDisplayPopup()
{
	if (_displayPopup) return;

	_displayPopup = new QWidget(this, Qt::Popup | Qt::FramelessWindowHint);
	//样式必须带 ID 选择器：无选择器规则会级联到全部子部件（子部件被画出边框/背景）
	_displayPopup->setObjectName("dsPopup");
	_displayPopup->setAttribute(Qt::WA_StyledBackground, true);
	_displayPopup->setStyleSheet("#dsPopup{background:#FFFFFF;border:1px solid #DCE2E9;border-radius:3px;}");
	_displayPopup->setFixedSize(300, 500);

	auto* popupLayout = new QVBoxLayout(_displayPopup);
	popupLayout->setContentsMargins(1, 1, 1, 1);
	popupLayout->setSpacing(0);

	//标题栏（标题左、关闭右）
	QWidget* head = new QWidget(_displayPopup);
	head->setFixedHeight(28);
	auto* headLayout = new QHBoxLayout(head);
	headLayout->setContentsMargins(10, 0, 6, 0);
	_dsTitleText = new QLabel(QString::fromUtf8("显示设置"), head);
	_dsTitleText->setStyleSheet("border:0;background:transparent;font-weight:600;color:#424B57;");
	headLayout->addWidget(_dsTitleText);
	headLayout->addStretch();
	QPushButton* btnClose = new QPushButton(QString::fromUtf8("×"), head);
	btnClose->setStyleSheet("QPushButton{border:0;background:transparent;color:#788391;font-size:14px;}"
		"QPushButton:hover{background:#DCE2E9;}");
	btnClose->setFixedSize(22, 22);
	headLayout->addWidget(btnClose);
	popupLayout->addWidget(head);

	auto* body = new QWidget(_displayPopup);
	auto* bodyLayout = new QVBoxLayout(body);
	//弹窗定高500：上下留白摊到区块间距上，避免底部堆积明显空白
	bodyLayout->setContentsMargins(12, 12, 12, 12);
	bodyLayout->setSpacing(14);
	popupLayout->addWidget(body, 1);

	const QString labelQss = QString("QLabel{color:%1;background:transparent;font-size:12px;border:0;}").arg(kText);

	//力分量/力矩分量双列：表头复选控制整列，行为复选+名称+右侧可点击取色的色块；
	//defaultCheckIndex指定默认勾选的分量行（力列默认X，力矩列默认Z）
	auto makeCompColumn = [this, &labelQss](QWidget* parent, const QString& head,
		QLabel** headText, QCheckBox** allCheck, QList<QCheckBox*>* rows,
		QList<QLabel*>* swatches, QList<QColor>* colors, int defaultCheckIndex) {
		QWidget* col = new QWidget(parent);
		auto* colLayout = new QVBoxLayout(col);
		colLayout->setContentsMargins(0, 0, 0, 0);
		colLayout->setSpacing(0);

		//表头
		QWidget* headRow = new QWidget(col);
		headRow->setFixedHeight(26);
		auto* headRowLayout = new QHBoxLayout(headRow);
		headRowLayout->setContentsMargins(6, 0, 6, 0);
		headRowLayout->setSpacing(4);
		QLabel* headLabel = new QLabel(head, headRow);
		headLabel->setStyleSheet(labelQss);
		headRowLayout->addWidget(headLabel);
		*headText = headLabel;
		QCheckBox* all = new QCheckBox(headRow);
		headRowLayout->addWidget(all);
		headRowLayout->addStretch();
		colLayout->addWidget(headRow);
		*allCheck = all;

		for (int i = 0; i < 7; ++i)
		{
			QWidget* row = new QWidget(col);
			row->setFixedHeight(26);
			auto* rowLayout = new QHBoxLayout(row);
			rowLayout->setContentsMargins(6, 0, 6, 0);
			rowLayout->setSpacing(4);
			QCheckBox* cb = new QCheckBox(row);
			cb->setChecked(i == defaultCheckIndex);
			rowLayout->addWidget(cb);
			QLabel* name = new QLabel(QString::fromLatin1(kCompGroupNames[i]), row);
			name->setStyleSheet(labelQss);
			rowLayout->addWidget(name);
			rowLayout->addStretch();
			QLabel* swatch = new QLabel(row);
			swatch->setFixedSize(20, 20);
			swatch->setCursor(Qt::PointingHandCursor);
			swatch->setStyleSheet(QString("background:%1;border:1px solid %2;border-radius:2px;")
				.arg(kDsCompColors[i].name()).arg(kCtrlBorder));
			swatch->installEventFilter(this);
			rowLayout->addWidget(swatch);
			colLayout->addWidget(row);
			rows->append(cb);
			swatches->append(swatch);
			//色表在setupUI已预填默认值（弹窗创建前符号刷新取色用），此处按索引覆写
			if (i < colors->size())
				(*colors)[i] = kDsCompColors[i];
			else
				colors->append(kDsCompColors[i]);
		}

		//表头复选全勾/全不勾；行复选变化回写表头勾选态并联动力/力矩符号刷新
		connect(all, &QCheckBox::clicked, this, [rows](bool on) { for (auto* cb : *rows) cb->setChecked(on); });
		auto syncAll = [all, rows]() {
			bool allOn = true;
			for (auto* cb : *rows)
				if (!cb->isChecked()) { allOn = false; break; }
			all->setChecked(allOn);
			};
		for (auto* cb : *rows)
		{
			connect(cb, &QCheckBox::toggled, this, syncAll);
			connect(cb, &QCheckBox::toggled, this, [this](bool) { if (_hasCalcResult) refreshForceMomentSymbols(); });
		}
		syncAll();
		return col;
		};
	QWidget* columns = new QWidget(body);
	QHBoxLayout* columnsLayout = new QHBoxLayout(columns);
	columnsLayout->setContentsMargins(0, 0, 0, 0);
	columnsLayout->setSpacing(12);
	columnsLayout->addWidget(makeCompColumn(columns, QString::fromUtf8("力分量"), &_dsForceHeadText, &_dsForceAll, &_dsForceChecks, &_dsForceSwatches, &_dsForceColors, 0), 1);
	columnsLayout->addWidget(makeCompColumn(columns, QString::fromUtf8("力矩分量"), &_dsMomentHeadText, &_dsMomentAll, &_dsMomentChecks, &_dsMomentSwatches, &_dsMomentColors, 2), 1);
	bodyLayout->addWidget(columns);

	//符号尺寸（自动/统一/缩放分段 + 数值输入；标签与控件行紧凑成组；自动时输入置灰）
	QWidget* sizeGroup = new QWidget(body);
	QVBoxLayout* sizeGroupLayout = new QVBoxLayout(sizeGroup);
	sizeGroupLayout->setContentsMargins(0, 0, 0, 0);
	sizeGroupLayout->setSpacing(2);
	_dsSymSizeText = new QLabel(QString::fromUtf8("符号尺寸"), sizeGroup);
	_dsSymSizeText->setStyleSheet(labelQss);
	sizeGroupLayout->addWidget(_dsSymSizeText);
	QWidget* sizeRow = new QWidget(sizeGroup);
	QHBoxLayout* sizeLayout = new QHBoxLayout(sizeRow);
	sizeLayout->setContentsMargins(0, 0, 0, 0);
	sizeLayout->setSpacing(6);
		_dsSizeChoice = new MxFunctions::mChoiceGroup(sizeRow);
		sizeLayout->addWidget(_dsSizeChoice, 1);
		_dsSizeEdit = new QLineEdit(sizeRow);
		_dsSizeEdit->setFixedWidth(70);
		_dsSizeEdit->setFixedHeight(26);
		_dsSizeEdit->setText("1");
		_dsSizeEdit->setValidator(new QDoubleValidator(_dsSizeEdit));
		_dsSizeEdit->setEnabled(false);
		sizeLayout->addWidget(_dsSizeEdit);
		sizeGroupLayout->addWidget(sizeRow);
		bodyLayout->addWidget(sizeGroup);
		connect(_dsSizeChoice, &MxFunctions::mChoiceGroup::currentIndexChanged, _dsSizeEdit, &QWidget::setDisabled);
		//符号尺寸变更（已计算时实时重绘）：自动=固定屏幕自适应；统一=固定屏幕取输入值；缩放=世界尺寸随视图缩放
		connect(_dsSizeChoice, &MxFunctions::mChoiceGroup::currentIndexChanged, this,
			[this](int) { if (_hasCalcResult) refreshForceMomentSymbols(); });
		connect(_dsSizeEdit, &QLineEdit::editingFinished, this, [this]() { if (_hasCalcResult) refreshForceMomentSymbols(); });

	//符号形式（头部/中点/尾部分段 + 形状下拉；标签与控件行紧凑成组）
	QWidget* shapeGroup = new QWidget(body);
	QVBoxLayout* shapeGroupLayout = new QVBoxLayout(shapeGroup);
	shapeGroupLayout->setContentsMargins(0, 0, 0, 0);
	shapeGroupLayout->setSpacing(2);
	_dsSymShapeText = new QLabel(QString::fromUtf8("符号形式"), shapeGroup);
	_dsSymShapeText->setStyleSheet(labelQss);
	shapeGroupLayout->addWidget(_dsSymShapeText);
	QWidget* shapeRow = new QWidget(shapeGroup);
	QHBoxLayout* shapeLayout = new QHBoxLayout(shapeRow);
	shapeLayout->setContentsMargins(0, 0, 0, 0);
	shapeLayout->setSpacing(6);
		_dsPosChoice = new MxFunctions::mChoiceGroup(shapeRow);
		shapeLayout->addWidget(_dsPosChoice, 1);
		_dsShapeCombo = new QComboBox(shapeRow);
		_dsShapeCombo->setFixedHeight(26);
		_dsShapeCombo->setFixedWidth(70);
		shapeLayout->addWidget(_dsShapeCombo);
		shapeGroupLayout->addWidget(shapeRow);
		bodyLayout->addWidget(shapeGroup);
		//符号位置/形状变更（已计算时实时重绘）
		connect(_dsPosChoice, &MxFunctions::mChoiceGroup::currentIndexChanged, this,
			[this](int) { if (_hasCalcResult) refreshForceMomentSymbols(); });
		connect(_dsShapeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
			[this](int) { if (_hasCalcResult) refreshForceMomentSymbols(); });

	//数据精度 / 求和点尺寸（标签在上、控件在下，各占一半；"3"与"科学计数法"连体无缝）
	QWidget* precGrid = new QWidget(body);
	QGridLayout* precLayout = new QGridLayout(precGrid);
	precLayout->setContentsMargins(0, 0, 0, 0);
	precLayout->setHorizontalSpacing(10);
	precLayout->setVerticalSpacing(4);
	_dsPrecText = new QLabel(QString::fromUtf8("数据精度"), precGrid);
	_dsPrecText->setStyleSheet(labelQss);
	precLayout->addWidget(_dsPrecText, 0, 0);
	QWidget* precRow = new QWidget(precGrid);
	QHBoxLayout* precRowLayout = new QHBoxLayout(precRow);
	precRowLayout->setContentsMargins(0, 0, 0, 0);
	precRowLayout->setSpacing(0);
	_dsPrecEdit = new QLineEdit(precRow);
	_dsPrecEdit->setFixedWidth(36);
	_dsPrecEdit->setFixedHeight(26);
	_dsPrecEdit->setText("3");
	_dsPrecEdit->setValidator(new QIntValidator(0, 15, _dsPrecEdit));
	//右侧直角无边，与下拉框连体（保留悬停/聚焦态反馈）
	_dsPrecEdit->setStyleSheet(
		QString("QLineEdit{background:#FFFFFF;border:1px solid %1;border-right:0;"
			"border-top-left-radius:2px;border-bottom-left-radius:2px;"
			"border-top-right-radius:0;border-bottom-right-radius:0;}"
			"QLineEdit:hover{border:1px solid #909399;border-right:0;}"
			"QLineEdit:focus{border:1px solid #1385F6;border-right:0;}"
			"QLineEdit:disabled{background:#F5F7FA;color:#A8ABB2;border:1px solid %1;border-right:0;}")
		.arg(kCtrlBorder));
	precRowLayout->addWidget(_dsPrecEdit);
	_dsNotationCombo = new QComboBox(precRow);
	_dsNotationCombo->setFixedHeight(26);
	//左侧直角，其左边框作为连体分隔线
	_dsNotationCombo->setStyleSheet(
		QString("QComboBox{background:#FFFFFF;border:1px solid %1;"
			"border-top-left-radius:0;border-bottom-left-radius:0;"
			"border-top-right-radius:2px;border-bottom-right-radius:2px;}"
			"QComboBox:disabled{background:#F5F7FA;border:1px solid %1;color:#A6AFBB;}"
			"QComboBox::drop-down{subcontrol-origin:padding;subcontrol-position:top right;width:26px;border:0;}")
		.arg(kCtrlBorder));
	precRowLayout->addWidget(_dsNotationCombo, 1);
	precLayout->addWidget(precRow, 1, 0);
	//数据精度/记数法变更（已计算时实时重绘）
	connect(_dsPrecEdit, &QLineEdit::editingFinished, this, [this]() { if (_hasCalcResult) refreshForceMomentSymbols(); });
	connect(_dsNotationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
		[this](int) { if (_hasCalcResult) refreshForceMomentSymbols(); });
	_dsSumSizeText = new QLabel(QString::fromUtf8("求和点尺寸"), precGrid);
	_dsSumSizeText->setStyleSheet(labelQss);
	precLayout->addWidget(_dsSumSizeText, 0, 1);
	QWidget* sumSizeRow = new QWidget(precGrid);
	QHBoxLayout* sumSizeLayout = new QHBoxLayout(sumSizeRow);
	sumSizeLayout->setContentsMargins(0, 0, 0, 0);
	sumSizeLayout->setSpacing(4);
	_dsSumSizeEdit = new QLineEdit(sumSizeRow);
	_dsSumSizeEdit->setFixedWidth(56);
	_dsSumSizeEdit->setFixedHeight(26);
	_dsSumSizeEdit->setText("0.4");
	_dsSumSizeEdit->setValidator(new QDoubleValidator(_dsSumSizeEdit));
	sumSizeLayout->addWidget(_dsSumSizeEdit);
	QLabel* pctLabel = new QLabel(QString::fromUtf8("%"), sumSizeRow);
	pctLabel->setStyleSheet(labelQss);
	sumSizeLayout->addWidget(pctLabel);
	sumSizeLayout->addStretch();
	precLayout->addWidget(sumSizeRow, 1, 1);
	precLayout->setColumnStretch(0, 1);
	precLayout->setColumnStretch(1, 1);
	bodyLayout->addWidget(precGrid);
	bodyLayout->addStretch();

	//显示数值/标签前缀 + 恢复默认（靠底）
	QWidget* dsBottom = new QWidget(body);
	QHBoxLayout* dsBottomLayout = new QHBoxLayout(dsBottom);
	dsBottomLayout->setContentsMargins(0, 0, 0, 0);
	dsBottomLayout->setSpacing(12);
	_dsShowValues = new QCheckBox(QString::fromUtf8("显示数值"), dsBottom);
	dsBottomLayout->addWidget(_dsShowValues);
	_dsLabelPrefix = new QCheckBox(QString::fromUtf8("标签前缀"), dsBottom);
	dsBottomLayout->addWidget(_dsLabelPrefix);
	dsBottomLayout->addStretch();
		_dsRestoreBtn = new QPushButton(dsBottom);
		//原型 fbdDisplayReset=普通 fbd-button（白底描边），非主蓝
		_dsRestoreBtn->setStyleSheet(
			QString("QPushButton{background:#FFFFFF;border:1px solid %1;border-radius:2px;color:%2;}"
				"QPushButton:hover{background:%3;}"
				"QPushButton:disabled{color:%4;background:#F5F7FA;}")
			.arg(kCtrlBorder, kText, kHoverBg, kDisabled));
		_dsRestoreBtn->setFixedSize(64, 26);
		dsBottomLayout->addWidget(_dsRestoreBtn);
	bodyLayout->addWidget(dsBottom);

	//语言文本（记数法/形状下拉项）
	_dsNotationCombo->addItem(QString::fromUtf8("科学计数法"));
    _dsNotationCombo->addItem(QString::fromUtf8("小数"));
    _dsNotationCombo->addItem(QString::fromUtf8("常规"));
	_dsShapeCombo->addItem(QString::fromUtf8("箭头"));
    _dsShapeCombo->addItem(QString::fromUtf8("线条"));
	//接线
	connect(btnClose, &QPushButton::clicked, _displayPopup, &QWidget::close);
	connect(_dsRestoreBtn, &QPushButton::clicked, this, &mPostSectionForce::slot_displayRestoreDefaults);
}

void mPostSectionForce::applyDisplayPopupLanguage(int lang)
{
	if (!_dsTitleText) return;

	_dsTitleText->setText(lang == 0 ? QString::fromUtf8("显示设置") : QString("Display Settings"));
	_dsForceHeadText->setText(lang == 0 ? QString::fromUtf8("力分量") : QString("Force"));
	_dsMomentHeadText->setText(lang == 0 ? QString::fromUtf8("力矩分量") : QString("Moment"));
		_dsSymSizeText->setText(lang == 0 ? QString::fromUtf8("符号尺寸") : QString("Symbol Size"));
		//分段组换语言文案后恢复原选中（屏蔽信号防回落第 0 项的假切换；先取索引再 setOptions）
		const int sizeIdx = _dsSizeChoice->currentIndex();
		_dsSizeChoice->blockSignals(true);
		_dsSizeChoice->setOptions(lang == 0
			? QStringList{ QString::fromUtf8("自动"), QString::fromUtf8("统一"), QString::fromUtf8("缩放") }
		: QStringList{ QString("Auto"), QString("Uniform"), QString("Scale") });
		_dsSizeChoice->setCurrentIndex(sizeIdx);
		_dsSizeChoice->blockSignals(false);
		_dsSymShapeText->setText(lang == 0 ? QString::fromUtf8("符号形式") : QString("Symbol Shape"));
		const int posIdx = _dsPosChoice->currentIndex();
		_dsPosChoice->blockSignals(true);
		_dsPosChoice->setOptions(lang == 0
			? QStringList{ QString::fromUtf8("头部"), QString::fromUtf8("中点"), QString::fromUtf8("尾部") }
		: QStringList{ QString("Head"), QString("Middle"), QString("Tail") });
		_dsPosChoice->setCurrentIndex(posIdx);
		_dsPosChoice->blockSignals(false);
	_dsShapeCombo->blockSignals(true);
	_dsShapeCombo->setItemText(0, lang == 0 ? QString::fromUtf8("箭头") : QString("Arrow"));
	_dsShapeCombo->setItemText(1, lang == 0 ? QString::fromUtf8("线条") : QString("Line"));
	_dsShapeCombo->blockSignals(false);
	_dsPrecText->setText(lang == 0 ? QString::fromUtf8("数据精度") : QString("Precision"));
	_dsNotationCombo->blockSignals(true);
	_dsNotationCombo->setItemText(0, lang == 0 ? QString::fromUtf8("科学计数法") : QString("Scientific"));
	_dsNotationCombo->setItemText(1, lang == 0 ? QString::fromUtf8("小数") : QString("Decimal"));
	_dsNotationCombo->setItemText(2, lang == 0 ? QString::fromUtf8("常规") : QString("General"));
	_dsNotationCombo->blockSignals(false);
	_dsSumSizeText->setText(lang == 0 ? QString::fromUtf8("求和点尺寸") : QString("Sum Point Size"));
	_dsShowValues->setText(lang == 0 ? QString::fromUtf8("显示数值") : QString("Show Values"));
	_dsLabelPrefix->setText(lang == 0 ? QString::fromUtf8("标签前缀") : QString("Label Prefix"));
	_dsRestoreBtn->setText(lang == 0 ? QString::fromUtf8("恢复默认") : QString("Reset"));
}

void mPostSectionForce::slot_showDisplayPopup()
{
	deactivatePickBars();	//弹窗打开退出瞬时拾取态（原型互斥模式）
	if (!_displayPopup) ensureDisplayPopup();
	applyDisplayPopupLanguage(MAppConfiguration::mAppConfiguration::getInstance()->_language);

	if (_displayPopup->isVisible())
	{
		_displayPopup->close();
		return;
	}

	//锚定"设置"按钮行右侧、与按钮纵向对齐；越界时回落到屏幕内
	const QPoint btnGlobal = _btnDisplay->mapToGlobal(QPoint(0, 0));
	QPoint pos(mapToGlobal(QPoint(width(), 0)).x(), btnGlobal.y() - 2);
	QScreen* scr = QGuiApplication::screenAt(pos);
	const QRect avail = (scr ? scr : QGuiApplication::primaryScreen())->availableGeometry();
	if (pos.x() + _displayPopup->width() > avail.right())
		pos.setX(avail.right() - _displayPopup->width() - 4);
	if (pos.x() < avail.left())
		pos.setX(avail.left() + 4);
	pos.setY(qBound(avail.top(), pos.y(), qMax(avail.top(), avail.bottom() - _displayPopup->height())));
	_displayPopup->move(pos);
	_displayPopup->show();
}

void mPostSectionForce::slot_displayRestoreDefaults()
{
	//批量回填期间抑制逐项触发符号重建，末尾统一刷新一次
	_dsBulkUpdating = true;
	//力分量默认仅勾选X，力矩分量默认仅勾选Z
	for (int i = 0; i < _dsForceChecks.size(); ++i)
		_dsForceChecks.at(i)->setChecked(i == 0);
	for (int i = 0; i < _dsMomentChecks.size(); ++i)
		_dsMomentChecks.at(i)->setChecked(i == 2);
		_dsSizeChoice->setCurrentIndex(0);
		_dsSizeEdit->setText("1");
		_dsPosChoice->setCurrentIndex(0);
	_dsShapeCombo->setCurrentIndex(0);
	_dsPrecEdit->setText("3");
	_dsNotationCombo->setCurrentIndex(0);
	_dsSumSizeEdit->setText("0.4");
	_dsShowValues->setChecked(false);
	_dsLabelPrefix->setChecked(false);
	for (int i = 0; i < _dsForceSwatches.size(); ++i)
		applyDsSwatchColor(i, kDsCompColors[i]);
	for (int i = 0; i < _dsMomentSwatches.size(); ++i)
		applyDsSwatchColor(7 + i, kDsCompColors[i]);
	_dsBulkUpdating = false;
	if (_hasCalcResult)
		refreshForceMomentSymbols();
}

}
