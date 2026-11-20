#include "mPointSeek3DPage.h"
#include "../CurveCore/CurveDocument.h"
#include "../CurveCore/mCurveDocController.h"
#include "../CurveCore/mPointSeekCore.h"
#include "../CurveView/mCurveViewportFactory.h"
#include "../CurveWidgets/mCurveFoldSection.h"

//QT
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPair>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTextStream>
#include <QToolButton>
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

		//置灰占位按钮（坐标数据表三钮专用[添加行/删除末行/清空]——20261117
		//用户拍板保持置灰不动；导出表格钮 T47-4 起接线为活钮但构造期同款
		//置灰占位、选中曲线后启用；mCurveStyle3DPage::frozenButton 同款）
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

		//导出默认文件名清洗（T47-4）：曲线名剔除 Windows 文件名非法字符
		//\ / : * ? " < > |（直接移除不留占位——名称多段被清洗后仍可读）
		QString sanitizeFileNameText(const QString& name)
		{
			static const QString illegal = QStringLiteral("\\/:*?\"<>|");
			QString cleaned = name;
			for (QChar ch : illegal)
				cleaned.remove(ch);
			return cleaned;
		}

		//CSV 落盘（T47-4；编码/分隔符/换行对齐模块先例）：QTextStream
		//UTF-8+GenerateByteOrderMark（Excel 直开中文表头不乱码，mChannelMapCore
		//同款）；**二进制模式**写（文本内容已显式 CRLF——mDynStiffnessMapping
		//先例：Text 模式会把 "\n" 再翻译成 "\r\n" 叠加成 "\r\r\n" 脏数据）；
		//失败经 err 返回由调用方警告框，不抛不崩
		bool writeTableCsv(const QString& filePath, const QString& csv, QString* err)
		{
			QFile file(filePath);
			if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
			{
				if (err)
					*err = QStringLiteral("CSV 无法写入: %1").arg(file.errorString());
				return false;
			}
			QTextStream out(&file);
			out.setCodec("UTF-8");
			out.setGenerateByteOrderMark(true);   //UTF-8 BOM（Excel 直开不乱码）
			out << csv;
			return true;
		}

		//导航小工具钮（R47 翻活：构造期禁用占位，选中曲线后按列可用性启停；
		//窄面板压缩显式 minimumWidth(1)+紧凑内边距，置灰观感与占位期一致——
		//样式表 :disabled 规则沿用既有置灰口径，启用态交还全局 QToolButton QSS）
		QToolButton* navToolButton(const QString& text, QWidget* parent)
		{
			QToolButton* btn = new QToolButton(parent);
			btn->setText(text);
			btn->setEnabled(false);
			btn->setStyleSheet(QStringLiteral(
				"QToolButton { padding-left: 1px; padding-right: 1px; }"
				"QToolButton:disabled { background-color:#F5F7FA; color:#A8ABB2; border-color:#E4E7ED; }"));
			btn->setMinimumWidth(1);
			return btn;
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

		//导航按钮网格行：每格一个控件+拉伸权重（极值列占 2 格宽、X轴/Y轴列各
		//占 2 格宽，对齐设计稿 6 单位列网格——首行 6 个等宽步进钮，∩/U 行首钮
		//宽格=其余钮 2 倍；列归属见头注释：∩/U 宽钮=极值列全局跳点，◄∩/►∩ 与
		//◄U/►U 四对=X轴/Y轴列局部峰谷导航）
		QWidget* gridRow(const QVector<QPair<QWidget*, int>>& cells, QWidget* parent)
		{
			QWidget* host = new QWidget(parent);
			QHBoxLayout* lay = new QHBoxLayout(host);
			lay->setContentsMargins(0, 0, 0, 0);
			lay->setSpacing(MxFunctions::UiMetrics::rowGap());
			for (const auto& cell : cells)
				lay->addWidget(cell.first, cell.second);
			return host;
		}

		//三列小节头（极值/X轴/Y轴，居中）
		QWidget* headerRow(const QVector<QString>& headers, QWidget* parent)
		{
			QWidget* host = new QWidget(parent);
			QHBoxLayout* lay = new QHBoxLayout(host);
			lay->setContentsMargins(0, 0, 0, 0);
			lay->setSpacing(MxFunctions::UiMetrics::rowGap());
			for (const QString& h : headers)
			{
				QLabel* label = new QLabel(h, host);
				label->setAlignment(Qt::AlignCenter);
				lay->addWidget(label, 2);
			}
			return host;
		}
	}

	mPointSeek3DPage::mPointSeek3DPage(QWidget* parent)
		: QWidget(parent)
	{
		buildUi();

		//上下文维护（全局监听，mCurveStyle3DPage 先例）：本图表节点及其后代
		//增删/属性变化即整体刷新（下拉保 id 选择/表格重填/按钮启停/当前点失效
		//复查）；删除通知到达时被删节点已不可查（chartNodeId 失效）：按潜在
		//本图表事件接受（refreshAll 幂等，多刷一次无副作用）。回填全程阻塞
		//信号，无回环
		connect(mCurveDocController::getInstance(), &mCurveDocController::sig_nodePropertyChanged,
			this, [this](int nodeId, const QString&) { onDocNodeChanged(nodeId); });
		connect(mCurveDocController::getInstance(), &mCurveDocController::sig_nodeAdded,
			this, [this](int nodeId) { onDocNodeChanged(nodeId); });
		connect(mCurveDocController::getInstance(), &mCurveDocController::sig_nodeRemoved,
			this, [this](int nodeId) { onDocNodeChanged(nodeId); });
		//当前激活三维图切换（mDatum3DPage 同款：工厂广播为主路径）——下拉/
		//表格随激活图刷新；工厂全局仅一个寻点标记，新图寻点时旧图标记被工厂
		//状态替换清除，本页不代管
		connect(mCurveViewportFactory::getInstance(),
			&mCurveViewportFactory::sig_currentChartChanged,
			this, [this](int chartId) { setChart(chartId); });

		//构造初绑：以当前激活三维图为初始上下文（showEvent 挂载时再兜底一次）
		setChart(mCurveViewportFactory::getInstance()->currentChartId());
	}

	void mPointSeek3DPage::buildUi()
	{
		QVBoxLayout* layout = pageLayout(this);
		layout->addWidget(titleLabel(QStringLiteral("数值"), this));

		//▾ 寻点与坐标（曲线下拉+三列导航网格；构造期整页置灰，选中曲线后按
		//规则启停。步进钮字形=设计稿 ◄ ► ∩ U）
		_seekFold = new mCurveFoldSection(QStringLiteral("寻点与坐标"), this);
		_curveCombo = new QComboBox(this);
		_seekFold->addRow(QStringLiteral("曲线"), _curveCombo);
		_curveCombo->setEnabled(false);   //构造期占位：无绑定图表/无曲线时禁用
		connect(_curveCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			if (_syncing)
				return;
			//换曲线：当前点失效 → 标记与高亮清除（视图态；不触发任何模型
			//操作），表格重填、按钮按新曲线数据启停
			_curveId = (idx >= 0 && idx < _comboIds.size()) ? _comboIds.at(idx) : -1;
			_currentPoint = -1;
			mCurveViewportFactory::getInstance()->setPointSeekMarker(_chartId, -1, -1);
			refreshTable();
			refreshButtons();
			applyHighlight(-1);
		});

		//列头（极值/X轴/Y轴）
		_seekFold->addRow(headerRow({ QStringLiteral("极值"), QStringLiteral("X轴"), QStringLiteral("Y轴") }, this));
		//首行：每列一对前后步进钮（6 格等宽）——语义=各列步进序步进（极值=
		//极值点集切换——局部峰∪谷间跳转，T47-2 20261117 用户对齐修正/X=曲线
		//类型按序号·其余类型升序稳定序，T47-3/Y 同），止于端点不环绕
		QVector<QPair<QWidget*, int>> arrowCells;
		const QVector<QString> arrows = {
			QStringLiteral("\u25C4"), QStringLiteral("\u25BA"),
			QStringLiteral("\u25C4"), QStringLiteral("\u25BA"),
			QStringLiteral("\u25C4"), QStringLiteral("\u25BA") };
		for (const QString& a : arrows)
		{
			QToolButton* btn = navToolButton(a, this);
			_stepBtns.append(btn);
			arrowCells.append({ btn, 1 });
		}
		_seekFold->addRow(gridRow(arrowCells, this));
		//步进钮语义分派：格序 [极值◄ 极值► X◄ X► Y◄ Y►] → 列 0/0/1/1/2/2，
		//方向 -1/+1（mPointSeekCore::stepSeek 单一实现；步进序类型感知——
		//选中曲线 series.plot3dType 传入，曲线类型 X 列按序号，T47-3）
		for (int k = 0; k < _stepBtns.size(); ++k)
		{
			const int column = k / 2;          //0 极值 1 X轴 2 Y轴
			const int dir = (k % 2 == 0) ? -1 : 1;
			connect(_stepBtns.at(k), &QToolButton::clicked, this, [this, column, dir]()
			{
				CurveDocument* doc = mCurveDocController::getInstance()->document();
				const CurveNode* c = (_curveId > 0) ? doc->node(_curveId) : nullptr;
				if (!c || _curveId <= 0)
					return;
				const int target = mPointSeekCore::stepSeek(c->series.plot3dType,
					column, c->series.xData, c->series.yData, c->series.zData,
					_currentPoint, dir);
				if (target >= 0)
					seekTo(target);
			});
		}
		//∩ 行：[∩(2 格)=极值全局最高点][X◄∩][X►∩][Y◄∩][Y►∩]=X/Y 局部峰导航
		//（沿下标序当前点前/后最近一个局部峰；无峰原地不动不环绕）
		QVector<QPair<QWidget*, int>> peakCells;
		_peakBtn = navToolButton(QStringLiteral("\u2229"), this);
		peakCells.append({ _peakBtn, 2 });
		for (const QString& t : { QStringLiteral("\u25C4\u2229"), QStringLiteral("\u25BA\u2229"),
			QStringLiteral("\u25C4\u2229"), QStringLiteral("\u25BA\u2229") })
		{
			QToolButton* btn = navToolButton(t, this);
			_peakStepBtns.append(btn);
			peakCells.append({ btn, 1 });
		}
		_seekFold->addRow(gridRow(peakCells, this));
		connect(_peakBtn, &QToolButton::clicked, this, [this]()
		{
			CurveDocument* doc = mCurveDocController::getInstance()->document();
			const CurveNode* c = (_curveId > 0) ? doc->node(_curveId) : nullptr;
			if (!c)
				return;
			const int target = mPointSeekCore::globalMaxIndex(mPointSeekCore::ColValue,
				c->series.xData, c->series.yData, c->series.zData);
			if (target >= 0)
				seekTo(target);
		});
		for (int k = 0; k < _peakStepBtns.size(); ++k)
		{
			const int column = (k < 2) ? mPointSeekCore::ColX : mPointSeekCore::ColY;
			const int dir = (k % 2 == 0) ? -1 : 1;
			connect(_peakStepBtns.at(k), &QToolButton::clicked, this, [this, column, dir]()
			{
				CurveDocument* doc = mCurveDocController::getInstance()->document();
				const CurveNode* c = (_curveId > 0) ? doc->node(_curveId) : nullptr;
				if (!c)
					return;
				const int target = mPointSeekCore::nearestExtremum(column,
					c->series.xData, c->series.yData, c->series.zData,
					_currentPoint, true, dir);
				if (target >= 0)
					seekTo(target);
			});
		}
		//U 行：[U(2 格)=极值全局最低点][X◄U][X►U][Y◄U][Y►U]=X/Y 局部谷导航
		//（口径同 ∩ 行镜像）
		QVector<QPair<QWidget*, int>> uCells;
		_valleyBtn = navToolButton(QStringLiteral("U"), this);
		uCells.append({ _valleyBtn, 2 });
		for (const QString& t : { QStringLiteral("\u25C4U"), QStringLiteral("\u25BAU"),
			QStringLiteral("\u25C4U"), QStringLiteral("\u25BAU") })
		{
			QToolButton* btn = navToolButton(t, this);
			_valleyStepBtns.append(btn);
			uCells.append({ btn, 1 });
		}
		_seekFold->addRow(gridRow(uCells, this));
		connect(_valleyBtn, &QToolButton::clicked, this, [this]()
		{
			CurveDocument* doc = mCurveDocController::getInstance()->document();
			const CurveNode* c = (_curveId > 0) ? doc->node(_curveId) : nullptr;
			if (!c)
				return;
			const int target = mPointSeekCore::globalMinIndex(mPointSeekCore::ColValue,
				c->series.xData, c->series.yData, c->series.zData);
			if (target >= 0)
				seekTo(target);
		});
		for (int k = 0; k < _valleyStepBtns.size(); ++k)
		{
			const int column = (k < 2) ? mPointSeekCore::ColX : mPointSeekCore::ColY;
			const int dir = (k % 2 == 0) ? -1 : 1;
			connect(_valleyStepBtns.at(k), &QToolButton::clicked, this, [this, column, dir]()
			{
				CurveDocument* doc = mCurveDocController::getInstance()->document();
				const CurveNode* c = (_curveId > 0) ? doc->node(_curveId) : nullptr;
				if (!c)
					return;
				const int target = mPointSeekCore::nearestExtremum(column,
					c->series.xData, c->series.yData, c->series.zData,
					_currentPoint, false, dir);
				if (target >= 0)
					seekTo(target);
			});
		}

		//▾ 坐标数据表（添加行/删除末行/清空三钮保持置灰——20261117 用户拍板；
		//导出表格钮 T47-4 接线：构造期置灰占位，选中曲线且数据非空后启用；
		//表格=选中曲线全部点只读四列，行序=下标序）
		_tableFold = new mCurveFoldSection(QStringLiteral("坐标数据表"), this);
		QPushButton* addRowBtn = frozenButton(QStringLiteral("添加行"), this);
		QPushButton* delLastBtn = frozenButton(QStringLiteral("删除末行"), this);
		QPushButton* clearBtn = frozenButton(QStringLiteral("清空"), this);
		_exportBtn = frozenButton(QStringLiteral("导出表格"), this);
		_tableFold->addRow(buttonRow({ addRowBtn, delLastBtn, clearBtn, _exportBtn }, this));
		//导出 CSV：保存框（默认名=曲线名.csv，非法字符清洗；取消=空路径直接
		//返回零副作用）→ mPointSeekCore::exportTableCsv 装配（表头三列无序号/
		//'f',4/非有限空串单一实现）→ writeTableCsv 落盘（UTF-8 BOM+CRLF），
		//写失败警告框不崩溃
		connect(_exportBtn, &QPushButton::clicked, this, [this]()
		{
			CurveDocument* doc = mCurveDocController::getInstance()->document();
			const CurveNode* c = (_curveId > 0) ? doc->node(_curveId) : nullptr;
			if (!c)
				return;
			const QString filePath = QFileDialog::getSaveFileName(this,
				QStringLiteral("导出表格"),
				sanitizeFileNameText(c->name) + QStringLiteral(".csv"),
				QStringLiteral("CSV (*.csv)"));
			if (filePath.isEmpty())
				return;   //取消无副作用
			QString err;
			if (!writeTableCsv(filePath, mPointSeekCore::exportTableCsv(
				c->series.xData, c->series.yData, c->series.zData), &err))
				QMessageBox::warning(this, QStringLiteral("导出表格"), err);
		});
		_table = new QTableWidget(0, 4, this);
		_table->setHorizontalHeaderLabels({ QStringLiteral("序号"), QStringLiteral("横坐标"),
			QStringLiteral("纵坐标"), QStringLiteral("高度") });
		//只读表（不引入编辑语义）：禁编辑触发器即可，控件保持启用——置灰调色
		//板会吞掉行高亮的主题蓝前景（任务口径：加粗+蓝高亮用条目字体/前景实现）
		_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
		_table->setSelectionBehavior(QAbstractItemView::SelectRows);
		_table->setMinimumHeight(120);
		_table->verticalHeader()->setVisible(false);
		_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
		for (int col = 1; col < 4; ++col)
			_table->horizontalHeader()->setSectionResizeMode(col, QHeaderView::Stretch);
		_tableFold->addRow(_table);

		layout->addWidget(_seekFold);
		layout->addWidget(_tableFold);
		layout->addStretch(1);
	}

	void mPointSeek3DPage::showEvent(QShowEvent* event)
	{
		QWidget::showEvent(event);
		//挂载=show：以当前激活三维图兜底刷新一次（宏内挂载等广播错窗口径，
		//mDatum3DPage 同款；sig_currentChartChanged 跟随为主路径）
		setChart(mCurveViewportFactory::getInstance()->currentChartId());
	}

	void mPointSeek3DPage::setChart(int chartId)
	{
		if (_chartId == chartId)
			return;   //同值守卫（showEvent 与广播双路径去重）
		_chartId = chartId;
		refreshAll();
	}

	void mPointSeek3DPage::onDocNodeChanged(int nodeId)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (_chartId > 0 && !doc->node(_chartId))
		{
			_chartId = -1;   //绑定图表已被删除：清空上下文（下拉空整页禁用）
			refreshAll();
			return;
		}
		if (_chartId <= 0)
			return;
		//本图表节点或其后代变化即刷新（属性变化含 kName 改名与 "data" 数据
		//直写——后者触发表格重填与当前点失效复查）；删除通知节点已摘除不可
		//查，按潜在本图表事件接受（refreshAll 幂等）
		if (nodeId == _chartId || doc->chartNodeId(nodeId) == _chartId
			|| doc->chartNodeId(nodeId) <= 0)
			refreshAll();
	}

	void mPointSeek3DPage::refreshAll()
	{
		_syncing = true;
		const int prevCurveId = _curveId;
		refreshCombo();
		refreshTable();
		refreshButtons();
		if (_curveId != prevCurveId)
		{
			//选择随刷新改变（曲线已删/换激活图/自动选首条）：当前点失效——
			//页面侧状态与高亮清除；工厂标记**不动**（换图=旧图标记保留至新
			//寻点，对齐"工厂全局仅一个标记"口径；曲线删除=工厂 AboutToBeRemoved
			//联动已自清）
			_currentPoint = -1;
			applyHighlight(-1);
		}
		else
		{
			//选择未变：数据改写场景当前点有效性复查（下标越界/该点非 finite
			//→ 标记与高亮一并清；仍有效→表格重填后按当前点重亮）
			revalidateCurrentPoint();
			if (_currentPoint >= 0)
				applyHighlight(_currentPoint);
		}
		//滚动联动（T47-4）：当前点在表则滚至当前行可见（初始选中带当前点/
		//数据改写保留点同口径）；无当前点（换曲线/清除）滚回顶部
		if (_currentPoint >= 0)
			scrollToCurrentRow();
		else
			_table->scrollToTop();
		_syncing = false;
	}

	void mPointSeek3DPage::refreshCombo()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();

		//重建前捕获当前选择（跨刷新保持，按 id 重定位——重名允许，id 键定）
		int keepId = _curveId;

		//当前激活三维图全部 Curve 子节点（树序；四类 plot3dType 均列，无类型
		//过滤——数值页寻点覆盖面=图内全部内容）
		_comboIds.clear();
		if (_chartId > 0)
		{
			for (int childId : doc->childNodeIds(_chartId))
			{
				const CurveNode* c = doc->node(childId);
				if (c && c->type == CurveNodeType::Curve)
					_comboIds.append(childId);
			}
		}
		{
			//回填全程阻塞信号（currentIndexChanged 不触发改选链路）；空下拉
			//控件禁用（无激活三维图/无曲线=整页禁用口径）
			const QSignalBlocker blocker(_curveCombo);
			_curveCombo->clear();
			for (int id : _comboIds)
			{
				const CurveNode* c = doc->node(id);
				if (!c)
					continue;
				//item 文本=节点本名（R23 重名允许），itemData=节点 id 键定选择
				_curveCombo->addItem(c->name, id);
			}
			//保持当前选择（按 id 重定位；落空=未选中/曲线已删/换图）→ 自动选
			//首条（有曲线时保页面有目标；选中联动不产标记，无副作用）
			int restore = _comboIds.indexOf(keepId);
			if (restore < 0 && !_comboIds.isEmpty())
				restore = 0;
			if (restore >= 0)
				_curveCombo->setCurrentIndex(restore);
			_curveCombo->setEnabled(!_comboIds.isEmpty());
		}
		_curveId = (_curveCombo->currentIndex() >= 0
			&& _curveCombo->currentIndex() < _comboIds.size())
			? _comboIds.at(_curveCombo->currentIndex()) : -1;
	}

	void mPointSeek3DPage::refreshTable()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		const CurveNode* c = (_curveId > 0) ? doc->node(_curveId) : nullptr;

		//表格重填（幂等）：行序=下标序——网格类（plot3dRows*plot3dCols==N）
		//的模型数组本身即行主序摊平（定义曲线弹窗数值网格态装配口径），下标
		//序=行主序摊平序，无需重排；线/柱状（rows=cols=0）同理即点序
		_table->setRowCount(0);
		if (!c)
			return;
		const int n = qMin(c->series.xData.size(),
			qMin(c->series.yData.size(), c->series.zData.size()));
		_table->setRowCount(n);
		for (int i = 0; i < n; ++i)
		{
			//四列 'f',4 定长（横坐标=xData/纵坐标=yData/高度=zData）；序号列
			//1 基整数。条目只读（可选不可编辑，配合 NoEditTriggers）
			auto* idxItem = new QTableWidgetItem(QString::number(i + 1));
			idxItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			_table->setItem(i, 0, idxItem);
			auto* xItem = new QTableWidgetItem(
				QString::number(c->series.xData.at(i), 'f', 4));
			xItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			_table->setItem(i, 1, xItem);
			auto* yItem = new QTableWidgetItem(
				QString::number(c->series.yData.at(i), 'f', 4));
			yItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			_table->setItem(i, 2, yItem);
			auto* zItem = new QTableWidgetItem(
				QString::number(c->series.zData.at(i), 'f', 4));
			zItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			_table->setItem(i, 3, zItem);
		}
		//滚动联动（T47-4）：无当前点（换曲线刷新/清除态）表格滚回顶部；有
		//当前点由 refreshAll 尾部/seekTo 侧滚至当前行
		if (_currentPoint < 0)
			_table->scrollToTop();
	}

	void mPointSeek3DPage::refreshButtons()
	{
		//三列启停（mPointSeekCore::columnEnabled 单一口径；20261117 用户拍板：
		//曲线类型 Y 列固定禁用替代恒值推断口径）——选中曲线 plot3dType==1
		//（三维曲线）→ Y 轴列固定整列灰（首行 ◄►+∩/U 组一并）；其余类型
		//（瀑布/曲面/柱状）与其余列（极值/X轴）按恒值兜底判定（恒值列整列灰
		//含该列 ∩/U 组）；未选中/无数据全灰（构造期占位观感一致）
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		const CurveNode* c = (_curveId > 0) ? doc->node(_curveId) : nullptr;
		bool colOk[3] = { false, false, false };
		if (c)
		{
			const int plot3dType = c->series.plot3dType;
			colOk[0] = mPointSeekCore::columnEnabled(plot3dType, mPointSeekCore::ColValue,
				c->series.xData, c->series.yData, c->series.zData);
			colOk[1] = mPointSeekCore::columnEnabled(plot3dType, mPointSeekCore::ColX,
				c->series.xData, c->series.yData, c->series.zData);
			colOk[2] = mPointSeekCore::columnEnabled(plot3dType, mPointSeekCore::ColY,
				c->series.xData, c->series.yData, c->series.zData);
		}
		//首行步进钮 6 枚：格序 [极值◄ 极值► X◄ X► Y◄ Y►]
		for (int k = 0; k < _stepBtns.size(); ++k)
			_stepBtns.at(k)->setEnabled(colOk[k / 2]);
		//极值列 ∩/U（全局最高/最低）
		_peakBtn->setEnabled(colOk[0]);
		_valleyBtn->setEnabled(colOk[0]);
		//X/Y 列局部峰导航 [X◄∩ X►∩ Y◄∩ Y►∩] 与谷导航 [X◄U X►U Y◄U Y►U]
		for (int k = 0; k < _peakStepBtns.size(); ++k)
			_peakStepBtns.at(k)->setEnabled(colOk[(k < 2) ? 1 : 2]);
		for (int k = 0; k < _valleyStepBtns.size(); ++k)
			_valleyStepBtns.at(k)->setEnabled(colOk[(k < 2) ? 1 : 2]);
		//导出表格钮（T47-4 接线）：选中曲线且数据非空启用（与三列启停口径
		//独立——恒值/断口数据同样可导出）；其余三钮保持置灰（20261117 用户拍板）
		_exportBtn->setEnabled(c && mPointSeekCore::pointCount(
			c->series.xData, c->series.yData, c->series.zData) > 0);
	}

	void mPointSeek3DPage::applyHighlight(int pointIndex)
	{
		//表格高亮行重摆（全表先归位再点亮目标行——幂等，行数大时 O(N) 可接受；
		//高亮=QTableWidgetItem 字体加粗+主题蓝前景（用户截图首行样式），不用
		//selectRow 以免引入编辑/选择语义）
		const QFont plainFont = _table->font();
		QFont boldFont = plainFont;
		boldFont.setBold(true);
		const QColor defaultColor = _table->palette().color(QPalette::Active, QPalette::Text);
		const QColor themeBlue = curveHighlightBlue();   //选中/hover 主题蓝（CurveDefine 单一色源）
		for (int r = 0; r < _table->rowCount(); ++r)
		{
			const bool hot = (r == pointIndex);
			for (int col = 0; col < 4; ++col)
			{
				QTableWidgetItem* item = _table->item(r, col);
				if (!item)
					continue;
				item->setFont(hot ? boldFont : plainFont);
				item->setForeground(hot ? themeBlue : defaultColor);
			}
		}
	}

	void mPointSeek3DPage::seekTo(int pointIndex)
	{
		if (pointIndex < 0)
		{
			//清除口径：当前点/工厂标记/高亮一并清（换曲线/数据失效路径共用）
			_currentPoint = -1;
			mCurveViewportFactory::getInstance()->setPointSeekMarker(_chartId, -1, -1);
			applyHighlight(-1);
			return;
		}
		//守卫：落点须为有效寻点（下标界内且三坐标 finite——NaN 断口点不可
		//停驻；寻点核心产出的下标已满足，此处防御）
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		const CurveNode* c = (_curveId > 0) ? doc->node(_curveId) : nullptr;
		if (!c || !mPointSeekCore::isValidPoint(pointIndex,
			c->series.xData, c->series.yData, c->series.zData))
		{
			seekTo(-1);
			return;
		}
		_currentPoint = pointIndex;
		//工厂转发画布标记（视图态：不进命令栈、不写文档、不序列化、不触发
		//rebuild；跨图寻点旧图标记由工厂状态替换清除）+表格高亮行随动
		mCurveViewportFactory::getInstance()->setPointSeekMarker(_chartId, _curveId, pointIndex);
		applyHighlight(pointIndex);
		scrollToCurrentRow();   //滚动联动（T47-4）：当前行 EnsureVisible 最小滚动
	}

	void mPointSeek3DPage::scrollToCurrentRow()
	{
		//表格滚动到当前寻点行（T47-4）：EnsureVisible=最小滚动（目标行已在
		//视口内不动）；无当前点/行号越界/刷新期条目未建均安全跳过
		if (_currentPoint < 0 || _currentPoint >= _table->rowCount())
			return;
		QTableWidgetItem* item = _table->item(_currentPoint, 0);
		if (item)
			_table->scrollToItem(item, QAbstractItemView::EnsureVisible);
	}

	void mPointSeek3DPage::revalidateCurrentPoint()
	{
		//文档侧数据改写（数值页编辑提交/定义曲线应用/撤销重做等）后：当前
		//下标越界或该点非 finite → 标记与高亮清除；仍有效 → 高亮照常（画布
		//标记数据坐标渲染期从文档现读，下一帧自然跟随新值，无需重发）
		if (_currentPoint < 0)
			return;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		const CurveNode* c = (_curveId > 0) ? doc->node(_curveId) : nullptr;
		const bool stillValid = c && mPointSeekCore::isValidPoint(_currentPoint,
			c->series.xData, c->series.yData, c->series.zData);
		if (!stillValid)
			seekTo(-1);
	}
}
