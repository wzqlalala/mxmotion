#include "mDatum3DPage.h"
#include "../CurveCore/CurveDocument.h"
#include "../CurveCore/mCurveDocController.h"
#include "../CurveView/mCurveViewportFactory.h"   //currentChartId/sig_currentChartChanged（宿主挂载链零改动口径下页面自刷上下文）
#include "../CurveWidgets/mCurveEditorWidgets.h"   //mCurveColorButton/mCurveCheckBox/createCurveDoubleSpin 行工厂

//QT
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QSlider>
#include <QPushButton>
#include <QUndoStack>
#include <QVBoxLayout>

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

		//整行勾选框（勾选框在左、文字在右，§5.2 起勾选行统一形态）
		QCheckBox* createRowCheck(const QString& text, QWidget* parent)
		{
			mCurveCheckBox* check = new mCurveCheckBox(parent);
			check->setText(text);
			return check;
		}

		//按钮可收缩化（B02 根治，mCurveStyle3DPage::makeShrinkable 同款）
		void makeShrinkable(QPushButton* btn)
		{
			btn->setMinimumWidth(1);
			btn->setStyleSheet(QStringLiteral("QPushButton { padding-left: 2px; padding-right: 2px; }"));
		}

		//置灰占位按钮（构造即禁用，不连 commit/信号、不进回填；R31/R37 同款，
		//自身样式表兜底置灰观感）
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

		//置灰占位下拉（构造即禁用固定值，不进回填）
		QComboBox* frozenCombo(const QString& text, QWidget* parent)
		{
			QComboBox* combo = new QComboBox(parent);
			combo->addItem(text);
			combo->setEnabled(false);
			return combo;
		}

		//一行双控件容器（mAnnotation3DPropPage::pairRow 同款：第二控件占满余宽）
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

	mDatum3DPage::mDatum3DPage(QWidget* parent)
		: QWidget(parent)
	{
		buildUi();

		//上下文维护（全局监听三信号，mCurveStyle3DPage 先例）：本图表节点及其
		//后代增删/属性变化即重载列表与回显。本页自身提交触发的通知同样进入
		//（回显全程阻塞信号，同值赋值不回环，幂等收敛）；增删通知到达时被删
		//节点已不可查（chartNodeId 失效）：失败按潜在本图表事件接受（refreshAll
		//幂等，多刷一次无副作用）。图表删除兜底=清空上下文→列表清空全钮禁用
		//不崩溃
		auto onDocChanged = [this](int nodeId)
		{
			CurveDocument* doc = mCurveDocController::getInstance()->document();
			if (_chartId > 0 && !doc->node(_chartId))
			{
				_chartId = -1;   //绑定图表已被删除：清空上下文（列表清空整组禁用）
				refreshAll();
				return;
			}
			if (_chartId <= 0)
				return;
			if (nodeId == _chartId || doc->chartNodeId(nodeId) == _chartId
				|| doc->chartNodeId(nodeId) <= 0)
				refreshAll();
		};
		connect(mCurveDocController::getInstance(), &mCurveDocController::sig_nodePropertyChanged,
			this, onDocChanged);
		connect(mCurveDocController::getInstance(), &mCurveDocController::sig_nodeAdded,
			this, onDocChanged);
		connect(mCurveDocController::getInstance(), &mCurveDocController::sig_nodeRemoved,
			this, onDocChanged);

		//跟随激活三维图切换（宿主挂载链零改动口径）：Ribbon"基准"钮仅三维图激活
		//时可用（syncToolBtns 联动），页面缓存复用——激活图表切换时在此重绑，
		//等效 mCurveStyle3DPage 的"挂载前 setChart"。setChart 仅接受有效三维图
		//（-1/非三维图保持现绑定，防止卡片切换过渡态冲空页面）
		connect(mCurveViewportFactory::getInstance(),
			&mCurveViewportFactory::sig_currentChartChanged,
			this, [this](int chartId) { setChart(chartId); });

		//构造即绑定当前激活图表（懒创建发生在点击"基准"钮时=三维图激活态）
		setChart(mCurveViewportFactory::getInstance()->currentChartId());
		//无绑定构造（工厂无激活图表）也收敛到禁用基线：Qt 控件默认 enabled，
		//buildUi 后未经 refreshAll 校正的提交钮/属性行不得带电出页（refreshAll
		//幂等，已绑定路径零变化）
		refreshAll();
	}

	void mDatum3DPage::buildUi()
	{
		QVBoxLayout* layout = pageLayout(this);
		layout->addWidget(titleLabel(QStringLiteral("基准"), this));

		//▾ 基准管理（显示基准+名称框同行/勾选列表/三钮行）
		_manageFold = new mCurveFoldSection(QStringLiteral("基准管理"), this);
		//显示基准（生效）：图表级总开关 kDatum3DVisible（仅 Chart 节点放行），
		//关=隐藏本图表全部基准面；回显=父图表现值（refreshAll QSignalBlocker）
		_showCheck = createRowCheck(QStringLiteral("显示基准"), this);
		_showCheck->setToolTip(QStringLiteral("图表级基准总开关"));
		//名称框（生效）：当前选中基准本名，编辑失焦（editingFinished=失焦/回车）
		//且变更才一次提交 kName（防逐键压撤回栈；与模型现值比对"无变化不发"，
		//对齐注释页文字行纪律）
		_nameEdit = new QLineEdit(this);
		_manageFold->addRow(pairRow(_showCheck, _nameEdit, this));
		connect(_showCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			if (_syncing || _chartId <= 0)
				return;
			mCurveDocController::getInstance()->setNodeProperty(_chartId,
				CurveProp::kDatum3DVisible, on);
		});
		connect(_nameEdit, &QLineEdit::editingFinished, this, [this]()
		{
			if (_syncing)
				return;
			const int datumId = currentDatumId();
			if (datumId <= 0)
				return;
			CurveDocument* doc = mCurveDocController::getInstance()->document();
			const QString text = _nameEdit->text();
			if (text.isEmpty() || text == doc->nodeProperty(datumId, CurveProp::kName).toString())
				return;   //无变化不发（零改动失焦不压栈；空名拒绝防"基准 N"丢失）
			mCurveDocController::getInstance()->setNodeProperty(datumId,
				CurveProp::kName, text);
		});

		//基准列表（生效）：条目=本图表全部 DatumPlane3D 子节点树序（UserRole
		//携带节点 id，禁行号隐式约定）；条目勾选=该基准 kVisible 即时单键提交；
		//单击条目=选中→属性行回显
		_datumList = new QListWidget(this);
		_datumList->setMinimumHeight(90);
		_manageFold->addRow(_datumList);
		connect(_datumList, &QListWidget::itemChanged, this, [this](QListWidgetItem* item)
		{
			//条目文本不可编辑，itemChanged 仅勾选态变化可达；回填期间信号已阻塞，
			//_syncing 双保险
			if (!item || _syncing)
				return;
			const int datumId = item->data(Qt::UserRole).toInt();
			if (datumId <= 0)
				return;
			mCurveDocController::getInstance()->setNodeProperty(datumId,
				CurveProp::kVisible, item->checkState() == Qt::Checked);
		});
		connect(_datumList, &QListWidget::itemSelectionChanged,
			this, [this]() { refreshProps(); });

		//新增/复制/删除（生效钮，B02 可收缩化；启停随选中态/列表空态见
		//refreshList/refreshProps）
		_addBtn = new QPushButton(QStringLiteral("新增"), this);
		_copyBtn = new QPushButton(QStringLiteral("复制"), this);
		_delBtn = new QPushButton(QStringLiteral("删除"), this);
		for (QPushButton* b : { _addBtn, _copyBtn, _delBtn })
			makeShrinkable(b);
		_manageFold->addRow(buttonRow({ _addBtn, _copyBtn, _delBtn }, this));
		connect(_addBtn, &QPushButton::clicked, this, [this]() { addDatum(); });
		connect(_copyBtn, &QPushButton::clicked, this, [this]() { copyCurrent(); });
		connect(_delBtn, &QPushButton::clicked, this, [this]() { deleteCurrent(); });

		//▾ 位置与属性（10 行；生效=类型行外的 9 行，类型可用但零提交）
		_propFold = new mCurveFoldSection(QStringLiteral("位置与属性"), this);
		//类型：单选项"参考平面"下拉，可用但变更零提交——本期无第二类型（需求
		//拍板口径）；不连 commit（单选项下拉程序化 setCurrentIndex 也不产生
		//currentIndexChanged，双保险）
		_typeCombo = new QComboBox(this);
		_typeCombo->addItem(QStringLiteral("参考平面"));
		_propFold->addRow(QStringLiteral("类型"), _typeCombo);
		//法向方向（kNormalAxis 0=X/1=Y/2=Z，默认 X）：变更即时提交（渲染全量
		//重建按模型现值重放，平面朝向随改）
		_normalCombo = new QComboBox(this);
		_normalCombo->addItems({ QStringLiteral("X轴"), QStringLiteral("Y轴"), QStringLiteral("Z轴") });
		_propFold->addRow(QStringLiteral("法向方向"), _normalCombo);
		connect(_normalCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commitCurrent(CurveProp::kNormalAxis, idx);
		});
		//X/Y/Z坐标（kPosX/kPosY/kPosZ 双精度 spin；参考平面渲染只消费法向轴
		//对应坐标，另两轴字段入模保留——需求备案）
		const QVector<QString> coordNames = {
			QStringLiteral("X坐标"), QStringLiteral("Y坐标"), QStringLiteral("Z坐标") };
		QDoubleSpinBox* coordSpins[3] = { nullptr, nullptr, nullptr };
		const char* const coordKeys[3] = { CurveProp::kPosX, CurveProp::kPosY, CurveProp::kPosZ };
		for (int i = 0; i < 3; ++i)
		{
			//口径对齐 mAxis3DPropPage 轴范围行（4 位小数无按钮 spin）
			coordSpins[i] = createCurveDoubleSpin(4, -999999.0, 999999.0, 1.0, this);
			coordSpins[i]->setValue(0.0);
			_propFold->addRow(coordNames.at(i), coordSpins[i]);
			connect(coordSpins[i], static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
				this, [this, key = coordKeys[i]](double v)
			{
				commitCurrent(key, v);
			});
		}
		_posXSpin = coordSpins[0];
		_posYSpin = coordSpins[1];
		_posZSpin = coordSpins[2];
		//移动增量值框+横向滑条（占位纪律 R31/R37：构造即禁用、不连 commit/信号、
		//不进回填；值=设计稿截图值静态成型）
		_stepSpin = new QDoubleSpinBox(this);
		_stepSpin->setDecimals(1);
		_stepSpin->setRange(0.0, 999999.0);
		_stepSpin->setValue(0.1);
		_stepSpin->setEnabled(false);
		_propFold->addRow(QStringLiteral("移动增量"), _stepSpin);
		_propFold->setRowEnabled(_stepSpin, false);
		_stepSlider = new QSlider(Qt::Horizontal, this);
		_stepSlider->setRange(0, 100);
		_stepSlider->setValue(10);
		_stepSlider->setEnabled(false);
		_propFold->addRow(_stepSlider);
		//文字（kText 生效）：单行编辑，失焦（editingFinished=失焦/回车）且变更
		//才一次提交（防逐键压撤回栈，对齐 mAnnotation3DPropPage 文本编辑区纪律；
		//空=渲染取基准本名）
		_textEdit = new QLineEdit(this);
		_propFold->addRow(QStringLiteral("文字"), _textEdit);
		connect(_textEdit, &QLineEdit::editingFinished, this, [this]()
		{
			if (_syncing)
				return;
			const int datumId = currentDatumId();
			if (datumId <= 0)
				return;
			CurveDocument* doc = mCurveDocController::getInstance()->document();
			const QString text = _textEdit->text();
			if (text == doc->nodeProperty(datumId, CurveProp::kText).toString())
				return;   //无变化不发（零改动失焦不压栈；空文字合法=取本名）
			mCurveDocController::getInstance()->setNodeProperty(datumId,
				CurveProp::kText, text);
		});
		//标签位置（kLabelPos 9 宫格，默认中心=4；语义对齐 2D 基准线 labelPos；
		//初显=设计稿截图默认值"中心"——选中基准后按模型回显覆盖）
		_labelPosCombo = new QComboBox(this);
		_labelPosCombo->addItems({ QStringLiteral("左上"), QStringLiteral("中上"),
			QStringLiteral("右上"), QStringLiteral("左中"), QStringLiteral("中心"),
			QStringLiteral("右中"), QStringLiteral("左下"), QStringLiteral("中下"),
			QStringLiteral("右下") });
		_labelPosCombo->setCurrentIndex(4);
		_propFold->addRow(QStringLiteral("标签位置"), _labelPosCombo);
		connect(_labelPosCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commitCurrent(CurveProp::kLabelPos, idx);
		});
		//基准颜色（kColor 生效）：通栏色条形态照设计稿（B02 bar 形态）
		_colorBtn = new mCurveColorButton(QColor(221, 147, 96), this);
		_colorBtn->setBarMode(true);
		_propFold->addRow(QStringLiteral("基准颜色"), _colorBtn);
		connect(_colorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
		{
			commitCurrent(CurveProp::kColor, c);
		});
		//不透明度（kDatumOpacity 生效）：0..1 双精度 spin，默认 0.25
		_opacitySpin = createCurveDoubleSpin(2, 0.0, 1.0, 0.05, this);
		_opacitySpin->setValue(0.25);
		_propFold->addRow(QStringLiteral("不透明度"), _opacitySpin);
		connect(_opacitySpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, [this](double v)
		{
			commitCurrent(CurveProp::kDatumOpacity, v);
		});
		//反向裁剪（占位纪律：构造即禁用、不连 commit/信号、不进回填）
		_reverseClipCheck = createRowCheck(QStringLiteral("反向裁剪"), this);
		_reverseClipCheck->setEnabled(false);
		_propFold->addRow(_reverseClipCheck);

		//▾ 创建截面曲线（3 下拉+主钮=四控件整组占位，纪律同上）
		_createFold = new mCurveFoldSection(QStringLiteral("创建截面曲线"), this);
		QComboBox* xAxisCombo = frozenCombo(QStringLiteral("X"), this);
		_createFold->addRow(QStringLiteral("曲线横坐标"), xAxisCombo);
		_createFold->setRowEnabled(xAxisCombo, false);
		QComboBox* yAxisCombo = frozenCombo(QStringLiteral("Z"), this);
		_createFold->addRow(QStringLiteral("曲线纵坐标"), yAxisCombo);
		_createFold->setRowEnabled(yAxisCombo, false);
		QComboBox* windowCombo = frozenCombo(QStringLiteral("当前窗口"), this);
		_createFold->addRow(QStringLiteral("创建窗口"), windowCombo);
		_createFold->setRowEnabled(windowCombo, false);
		_createFold->addRow(buttonRow({ frozenButton(QStringLiteral("创建截面曲线"), this) }, this));

		layout->addWidget(_manageFold);
		layout->addWidget(_propFold);
		layout->addWidget(_createFold);
		layout->addStretch(1);
	}

	void mDatum3DPage::setChart(int chartNodeId)
	{
		//仅有效三维图（chartType=5）重绑；-1/非三维图保持现绑定不冲空（挂载链
		//只在三维图激活时可点；卡片切换过渡态/二维图激活不冲掉已挂页面上下文；
		//图表删除清空走控制器 sig_nodeRemoved 兜底）。同 id 重绑幂等刷新一次
		//（外部状态可能已变，刷新无害）
		if (chartNodeId <= 0)
			return;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		const CurveNode* n = doc->node(chartNodeId);
		if (!n || n->type != CurveNodeType::Chart || n->chart.chartType != 5)
			return;
		_chartId = chartNodeId;
		refreshAll();
	}

	void mDatum3DPage::showEvent(QShowEvent* event)
	{
		QWidget::showEvent(event);
		//挂载即刷新上下文（宿主 setMainWidget→show；幂等——换图表后重挂工具页
		//经此补一次对齐，sig_currentChartChanged 跟随为主路径，此处兜底）
		setChart(mCurveViewportFactory::getInstance()->currentChartId());
	}

	int mDatum3DPage::currentDatumId() const
	{
		const int row = _datumList->currentRow();
		if (row < 0 || row >= _datumIds.size())
			return -1;
		return _datumIds.at(row);
	}

	void mDatum3DPage::commitCurrent(const QString& prop, const QVariant& value)
	{
		if (_syncing)
			return;
		const int datumId = currentDatumId();
		if (datumId <= 0)
			return;
		mCurveDocController::getInstance()->setNodeProperty(datumId, prop, value);
	}

	void mDatum3DPage::addDatum()
	{
		if (_syncing || _chartId <= 0)
			return;
		mCurveDocController* controller = mCurveDocController::getInstance();
		//addDatumPlane3D=宏"新建基准面"（出厂默认值入档，一次 Ctrl+Z 整体撤销；
		//守卫拒绝 -1 时静默返回）。nodeAdded 信号已触发全局监听 refreshAll
		//（保持旧选中），此处显式选中新项→属性行回显
		const int newId = controller->addDatumPlane3D(_chartId);
		if (newId <= 0)
			return;
		refreshList();
		const int row = _datumIds.indexOf(newId);
		if (row >= 0)
			_datumList->setCurrentRow(row);   //itemSelectionChanged → refreshProps
	}

	void mDatum3DPage::copyCurrent()
	{
		if (_syncing)
			return;
		const int srcId = currentDatumId();
		if (srcId <= 0 || _chartId <= 0)
			return;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		const CurveNode* src = doc->node(srcId);
		if (!src || src->type != CurveNodeType::DatumPlane3D)
			return;   //绑定异常形态防御
		mCurveDocController* controller = mCurveDocController::getInstance();
		//宏"复制基准面"包 新节点+逐键克隆（嵌套宏并入顶层，一次 Ctrl+Z 整体
		//撤销）；名不克隆——新节点默认名按计数器续"基准 N"（addDatumPlane3D
		//内已按 nextDefaultNumber 命名，kName 零提交）
		controller->undoStack()->beginMacro(QStringLiteral("复制基准面"));
		const int newId = controller->addDatumPlane3D(_chartId);
		if (newId > 0)
		{
			controller->setNodeProperty(newId, CurveProp::kVisible, src->datumPlane3D.visible);
			controller->setNodeProperty(newId, CurveProp::kColor, src->datumPlane3D.color);
			controller->setNodeProperty(newId, CurveProp::kText, src->datumPlane3D.text);
			controller->setNodeProperty(newId, CurveProp::kLabelPos, src->datumPlane3D.labelPos);
			controller->setNodeProperty(newId, CurveProp::kPosX, src->datumPlane3D.posX);
			controller->setNodeProperty(newId, CurveProp::kPosY, src->datumPlane3D.posY);
			controller->setNodeProperty(newId, CurveProp::kPosZ, src->datumPlane3D.posZ);
			controller->setNodeProperty(newId, CurveProp::kDatumType, src->datumPlane3D.type);
			controller->setNodeProperty(newId, CurveProp::kNormalAxis, src->datumPlane3D.normalAxis);
			controller->setNodeProperty(newId, CurveProp::kDatumOpacity, src->datumPlane3D.opacity);
			controller->setNodeProperty(newId, CurveProp::kReverseClip, src->datumPlane3D.reverseClip);
		}
		controller->undoStack()->endMacro();
		if (newId > 0)
		{
			//插后选中（选中新项+属性行回显；宏期间的全局监听刷新保持旧选中）
			refreshList();
			const int row = _datumIds.indexOf(newId);
			if (row >= 0)
				_datumList->setCurrentRow(row);
		}
	}

	void mDatum3DPage::deleteCurrent()
	{
		if (_syncing)
			return;
		const int datumId = currentDatumId();
		if (datumId <= 0)
			return;
		//removeNode=命令栈单命令可撤销（树右键"删除基准面"同 API）；节点删除
		//信号经全局监听 refreshAll 收敛（列表换代/属性行禁用/按钮启停）
		mCurveDocController::getInstance()->removeNode(datumId);
	}

	void mDatum3DPage::refreshAll()
	{
		_syncing = true;
		refreshList();
		refreshProps();
		_syncing = false;
	}

	void mDatum3DPage::refreshList()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		const int keepId = currentDatumId();

		//树序全部 DatumPlane3D 子节点重建（全程阻塞信号：itemChanged/
		//itemSelectionChanged 在回填期间不得触发提交或属性行重载）
		_datumIds.clear();
		if (_chartId > 0)
		{
			for (int childId : doc->childNodeIds(_chartId))
			{
				const CurveNode* c = doc->node(childId);
				if (c && c->type == CurveNodeType::DatumPlane3D)
					_datumIds.append(childId);
			}
		}
		{
			const QSignalBlocker blocker(_datumList);
			_datumList->clear();
			for (int id : _datumIds)
			{
				const CurveNode* c = doc->node(id);
				if (!c)
					continue;
				QListWidgetItem* item = new QListWidgetItem(c->name, _datumList);
				item->setData(Qt::UserRole, id);
				item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
				item->setCheckState(c->datumPlane3D.visible ? Qt::Checked : Qt::Unchecked);
			}
			//保持当前选中基准（重绑/外部变化后属性行不跳目标）；无选中选首项
			int restore = -1;
			if (keepId > 0)
				restore = _datumIds.indexOf(keepId);
			else if (!_datumIds.isEmpty())
				restore = 0;
			if (restore >= 0)
				_datumList->setCurrentRow(restore);
		}
		//列表启停随绑定图表（无绑定=禁用；有图表无基准=可用空列表合法）

		//按钮启停：新增=有绑定图表（addDatumPlane3D 内再守卫 chartType）；复制/
		//删除=有选中基准；图表无效全禁用（删除兜底不崩溃）
		const bool hasChart = _chartId > 0 && doc->node(_chartId) != nullptr;
		_datumList->setEnabled(hasChart);
		_addBtn->setEnabled(hasChart);
		const bool hasDatum = currentDatumId() > 0 && doc->node(currentDatumId()) != nullptr;
		_copyBtn->setEnabled(hasDatum);
		_delBtn->setEnabled(hasDatum);
	}

	void mDatum3DPage::refreshProps()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		const int datumId = currentDatumId();
		const CurveNode* n = (datumId > 0) ? doc->node(datumId) : nullptr;
		const bool hasDatum = n != nullptr && n->type == CurveNodeType::DatumPlane3D;

		//生效行回显全程阻塞信号（R30 教训：回显触发提交=毒化模型）。所有属性行
		//先查选中节点 nodeProperty（节点属性变化——非本页提交来源也一致——经
		//全局监听进入本路径重载）。占位行（移动增量值框+滑条/反向裁剪/创建截面
		//曲线组）不在回填路径——构造时静态值一次成型，永不回写
		const CurveNode* chart = (_chartId > 0) ? doc->node(_chartId) : nullptr;
		const bool hasChart = chart != nullptr && chart->type == CurveNodeType::Chart;
		_showCheck->setEnabled(hasChart);   //无绑定图表=总开关禁用（无消费目标）
		{
			const QSignalBlocker blockShow(_showCheck);
			const QSignalBlocker blockName(_nameEdit);
			const QSignalBlocker blockNormal(_normalCombo);
			const QSignalBlocker blockX(_posXSpin);
			const QSignalBlocker blockY(_posYSpin);
			const QSignalBlocker blockZ(_posZSpin);
			const QSignalBlocker blockText(_textEdit);
			const QSignalBlocker blockLabel(_labelPosCombo);
			const QSignalBlocker blockColor(_colorBtn);
			const QSignalBlocker blockOpacity(_opacitySpin);
			//显示基准=图表级键（父图表现值；图表无效保持控件原态不误写）
			if (hasChart)
				_showCheck->setChecked(chart->chart.datum3DVisible);
			if (hasDatum)
			{
				_nameEdit->setText(n->name);
				_normalCombo->setCurrentIndex(qBound(0, n->datumPlane3D.normalAxis, 2));
				//双精度回显带有限性防御（脏流 NaN 可穿透模型层 qBound，T41-2
				//评审建议④同源：非有限值回退出厂默认，模型值不动）
				const double px = n->datumPlane3D.posX;
				const double py = n->datumPlane3D.posY;
				const double pz = n->datumPlane3D.posZ;
				_posXSpin->setValue(qIsFinite(px) ? qBound(-999999.0, px, 999999.0) : 0.0);
				_posYSpin->setValue(qIsFinite(py) ? qBound(-999999.0, py, 999999.0) : 0.0);
				_posZSpin->setValue(qIsFinite(pz) ? qBound(-999999.0, pz, 999999.0) : 0.0);
				_textEdit->setText(n->datumPlane3D.text);
				_labelPosCombo->setCurrentIndex(qBound(0, n->datumPlane3D.labelPos, 8));
				_colorBtn->setColor(n->datumPlane3D.color);
				const double op = n->datumPlane3D.opacity;
				_opacitySpin->setValue(qIsFinite(op) ? qBound(0.0, op, 1.0) : 0.25);
			}
		}

		//名称框/属性行启停：无选中基准整组禁用（名称框随选中态）；类型下拉恒
		//可用（可用但变更零提交口径，不参与整组启停）
		const bool rowOk = hasDatum;
		_nameEdit->setEnabled(rowOk);
		_propFold->setRowEnabled(_normalCombo, rowOk);
		_propFold->setRowEnabled(_posXSpin, rowOk);
		_propFold->setRowEnabled(_posYSpin, rowOk);
		_propFold->setRowEnabled(_posZSpin, rowOk);
		_propFold->setRowEnabled(_textEdit, rowOk);
		_propFold->setRowEnabled(_labelPosCombo, rowOk);
		_propFold->setRowEnabled(_colorBtn, rowOk);
		_propFold->setRowEnabled(_opacitySpin, rowOk);
	}
}
