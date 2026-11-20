#pragma execution_character_set("utf-8")

#include "mAnalysisFieldPanel.h"
#include "mFieldManager.h"
#include "mAnalysisFieldTextIO.h"
#include "mModelData.h"
#include "mMechGlobalSignals.h"
#include "mGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mDataStatusManager.h"
#include "mCoordinateManager.h"
#include "mCoordinateBase.h"
#include "mSetManager.h"
#include "mSetBase.h"
#include "mNode.h"
#include "mElementSet.h"
#include "mElementSurface.h"
#include "mMeshData.h"
#include "mDataElement.h"
#include "mMapRelationManager.h"

#include <FieldTransferCore/Model.hpp>
#include <FieldTransferCore/DataBoundaries.hpp>
#include <FieldTransferCore/Diagnostic.hpp>
#include <FieldTransferMapping/Mapping.hpp>

#include "mxr/view/CCSMRendWidget.h"
#include "mxr/view/CCSMPreRender.h"
#include "mxr/view/LoadParticleCloudRender.h"
#include "mxr/data/MeshElementExtract.h"

#include <QDebug>
#include <QSet>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QToolButton>
#include <QPushButton>
#include <QLabel>
#include <QRegularExpressionValidator>
#include <QFileDialog>
#include <QFile>
#include <QFrame>
#include <QTimer>
#include <QVector3D>
#include <QShowEvent>

#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <memory>

using namespace MechDiscreteFieldData;
using namespace MechPreData;

namespace MechMainWindow
{
	namespace
	{
		//位置 -> 离散数据控件拾取对象描述（拾取类型白名单/标识列/业务key/视口拾取过滤器）。
		//过滤器序号与 mMechGlobalSignals::activePickSig 索引一致：5=节点、7=单元面、8=单元
		mDiscreteDataControl::PickSpec makeDiscretePickSpec(MxFunctions::PickDataType type)
		{
			mDiscreteDataControl::PickSpec spec;
			switch (type)
			{
			case MxFunctions::PickDataType::Node:
				spec.typeName = QStringLiteral("节点");
				spec.idHeaders = QStringList{ QStringLiteral("节点编号") };
				spec.keyOf = [](const RenderPickResultData& d) { return qMakePair(d.result_id, -1); };
				spec.pickTypes = { MxFunctions::PickDataType::Node };
				spec.pickFilter = 5;
				break;
			case MxFunctions::PickDataType::ElementFace:
				spec.typeName = QStringLiteral("单元面");
				spec.idHeaders = QStringList{ QStringLiteral("单元编号"), QStringLiteral("面编号") };
				spec.keyOf = [](const RenderPickResultData& d) { return qMakePair(d.result_id, d.result_id2); };
				spec.pickTypes = { MxFunctions::PickDataType::ElementFace };
				spec.pickFilter = 7;
				break;
			default:
				spec.typeName = QStringLiteral("单元");
				spec.idHeaders = QStringList{ QStringLiteral("单元编号") };
				spec.keyOf = [](const RenderPickResultData& d) { return qMakePair(d.result_id, -1); };
				spec.pickTypes = { MxFunctions::PickDataType::Element };
				spec.pickFilter = 8;
				break;
			}
			return spec;
		}

		//数据对象行 -> 控件行（离散模式）。编号缺失/非法的行不进控件
		//（点云坐标行等非本类型数据自然被过滤）；分量长度由控件按当前分量数规整。
		QVector<mDiscreteDataControl::Row> fieldRowsToControlRows(
			const QList<AnalysisFieldRow>& rows, MxFunctions::PickDataType type)
		{
			QVector<mDiscreteDataControl::Row> out;
			out.reserve(rows.size());
			const bool isFace = (type == MxFunctions::PickDataType::ElementFace);
			for (const auto& r : rows)
			{
				bool okA = false;
				const int a = isFace ? r.elementId.toInt(&okA) : r.id.toInt(&okA);
				if (!okA || a <= 0)
					continue;
				int b = -1;
				if (isFace)
				{
					bool okB = false;
					b = r.faceId.toInt(&okB);
					if (!okB || b <= 0)
						continue;
				}
				out.append(qMakePair(qMakePair(a, b), r.values));
			}
			return out;
		}

		//控件行 -> 数据对象行（离散模式）。编号未填（-1）的临时行不落盘
		QList<AnalysisFieldRow> controlRowsToFieldRows(
			const QVector<mDiscreteDataControl::Row>& rows, MxFunctions::PickDataType type)
		{
			QList<AnalysisFieldRow> out;
			out.reserve(rows.size());
			const bool isFace = (type == MxFunctions::PickDataType::ElementFace);
			for (const auto& r : rows)
			{
				if (r.first.first <= 0)
					continue;
				AnalysisFieldRow row;
				if (isFace)
				{
					row.elementId = QString::number(r.first.first);
					row.faceId = r.first.second > 0 ? QString::number(r.first.second) : QString();
				}
				else
				{
					row.id = QString::number(r.first.first);
				}
				row.values = r.second;
				out.append(std::move(row));
			}
			return out;
		}

		//数据对象行 -> 控件行（点云模式）：X/Y/Z 占控件前 3 列（D2 固定坐标列），
		//分量随分量数量补齐/截断；坐标以字符串透传，保留用户输入精度
		QVector<mPointCloudDataControl::Row> pointCloudRowsToControlRows(
			const QList<AnalysisFieldRow>& rows, int componentCount)
		{
			QVector<mPointCloudDataControl::Row> out;
			out.reserve(rows.size());
			for (const auto& r : rows)
			{
				QStringList values;
				values.reserve(3 + componentCount);
				values << r.x << r.y << r.z;
				for (int c = 0; c < componentCount; ++c)
					values.append(c < r.values.size() ? r.values.at(c) : QString());
				out.append(std::move(values));
			}
			return out;
		}

		//控件行 -> 数据对象行（点云模式）：前 3 列回写 X/Y/Z，其余为分量
		QList<AnalysisFieldRow> controlRowsToPointCloudRows(
			const QVector<mPointCloudDataControl::Row>& rows)
		{
			QList<AnalysisFieldRow> out;
			out.reserve(rows.size());
			for (const auto& v : rows)
			{
				AnalysisFieldRow row;
				row.x = v.size() > 0 ? v.at(0) : QString();
				row.y = v.size() > 1 ? v.at(1) : QString();
				row.z = v.size() > 2 ? v.at(2) : QString();
				for (int c = 3; c < v.size(); ++c)
					row.values.append(v.at(c));
				out.append(std::move(row));
			}
			return out;
		}
	}

	mAnalysisFieldPanel::mAnalysisFieldPanel(QWidget* parent)
		: mPanelBase(parent)
	{
		initUI();
	}

	mAnalysisFieldPanel::~mAnalysisFieldPanel()
	{
		//吸附式弹窗控件（离散/点云）与面板同生共死：弹窗为宿主顶层窗口子窗口，
		//基类监视宿主销毁，析构收尾（隐藏弹窗/摘除事件过滤器）由各自析构函数承担
	}

	void mAnalysisFieldPanel::initUI()
	{
		QVBoxLayout* mainLayout = new QVBoxLayout(this);
		mainLayout->setContentsMargins(0, 0, 0, 0);
		mainLayout->setSpacing(0);

		//==================== 分组一：定义 ====================
		_defineFold = new FoldSection(QStringLiteral("定义"), this);
		_locateCombo = new QComboBox(this);
		_locateCombo->addItems({ QStringLiteral("节点"), QStringLiteral("单元"), QStringLiteral("单元面") });
		_locateCombo->setCurrentIndex(0);	//默认节点
		_componentCountSpin = new QSpinBox(this);
		_componentCountSpin->setMinimum(1);
		_componentCountSpin->setMaximum(999999);
		_componentCountSpin->setValue(1);
		_defineFold->addRow(new PanelLabel(QStringLiteral("位置")), _locateCombo);
		_defineFold->addRow(new PanelLabel(QStringLiteral("分量数量")), _componentCountSpin);
		m_foldWidgets.append(_defineFold);
		mainLayout->addWidget(_defineFold);

		//==================== 分组二：场数据 ====================
		_dataFold = new FoldSection(QStringLiteral("场数据"), this);

		_methodCombo = new QComboBox(this);
		_methodCombo->addItems({ QStringLiteral("内联"), QStringLiteral("文本文档") });
		_defineMethodLabel = new PanelLabel(QStringLiteral("定义方式"));
		_dataFold->addRow(_defineMethodLabel, _methodCombo);

		//文件路径行（只读 + 浏览按钮），仅"文本文档"显示；
		//行的插入/移除统一由 rebuildFieldDataRows() 管理，避免 setVisible 隐藏后残留布局空隙
		_filePathLabel = new PanelLabel(QStringLiteral("文件路径"));
		_fileRowWidget = new QWidget(this);
		QHBoxLayout* fileLayout = new QHBoxLayout(_fileRowWidget);
		fileLayout->setContentsMargins(0, 0, 0, 0);
		_filePathEdit = new QLineEdit(_fileRowWidget);
		_filePathEdit->setReadOnly(true);
		_filePathEdit->setPlaceholderText(QStringLiteral("请选择文件"));
		QToolButton* browseBtn = new QToolButton(_fileRowWidget);
		browseBtn->setText(QStringLiteral("..."));
		connect(browseBtn, &QToolButton::clicked, this, &mAnalysisFieldPanel::slot_browseTextFile);
		fileLayout->addWidget(_filePathEdit);
		fileLayout->addWidget(browseBtn);

		_startRowLabel = new PanelLabel(QStringLiteral("起始行"));
		_startRowSpin = new QSpinBox(this);
		_startRowSpin->setMinimum(1);
		_startRowSpin->setMaximum(999999999);
		_startRowSpin->setValue(1);

		_endRowLabel = new PanelLabel(QStringLiteral("结束行"));
		_endRowSpin = new QSpinBox(this);
		_endRowSpin->setMinimum(1);
		_endRowSpin->setMaximum(999999999);
		_endRowSpin->setValue(1);

		_startColumnLabel = new PanelLabel(QStringLiteral("起始列"));
		_startColumnSpin = new QSpinBox(this);
		_startColumnSpin->setMinimum(1);
		_startColumnSpin->setMaximum(999999999);
		_startColumnSpin->setValue(1);

		_dataTypeCombo = new QComboBox(this);
		_dataTypeCombo->addItems({ QStringLiteral("点云"), QStringLiteral("节点离散") });
		_dataTypeLabel = new PanelLabel(QStringLiteral("数据类型"));
		_dataFold->addRow(_dataTypeLabel, _dataTypeCombo);

		//离散数据吸附式弹窗控件（仅内联+离散显示）：弹窗内表格直接驱动拾取与编辑，
		//取代原"通用拾取控件 + 数据表对话框"组合，形态与集中力/压力等载荷面板一致。
		//拾取对象描述随"位置"联动（节点/单元/单元面），由 syncDiscreteControlState 统一下发
		mDiscreteDataControl::PickSpec discreteSpec = makeDiscretePickSpec(currentPickType());
		_discreteControl = new mDiscreteDataControl(
			_locateCombo->currentText() + QStringLiteral("离散数据"),
			QStringList(), &discreteSpec, this);
		_discreteControl->setComponentCount(_componentCountSpin->value());
		//离散数据支持云图渲染：弹窗工具栏显示"渲染云图"按钮（开关仅保留在弹窗内）
		_discreteControl->setRenderButtonVisible(true, QStringLiteral("渲染云图"));
		_discreteControl->setRenderHandler([this]() {
			//开云图前清空视口拾取选中态/高亮，避免拾取高亮遮挡云图（参考压力载荷面板）
			if (!_cloudOn && _discreteControl)
				_discreteControl->clearPickingSelection();
			slot_renderFieldCloudChart();
			});
		//弹窗内导入/导出与原数据表对话框同源（解析/校验/导出共用 mAnalysisFieldTextIO）
		_discreteControl->setImportHandler([this](const mDiscreteDataControl::ImportParams& params) {
			importDiscreteTextFile(params);
			});
		_discreteControl->setExportHandler([this](const QString& filePath) {
			exportDiscreteTextFile(filePath);
			});
		//行数据变化（拾取追加/删除/清空/单元格编辑；拾取会话确认后统一补发）：
		//整表写回数据对象并联动树状态
		connect(_discreteControl, &mDiscreteDataControl::dataChanged,
			this, [this]() {
				if (!_analysisFieldData || !_discreteControl)
					return;
				//拾取会话进行中不回写（控件会话内不发 dataChanged，此处双保险）
				if (_discreteControl->isPicking())
					return;
				syncDiscreteControlToField();
			});
		//入口按钮从控件中剖分出来直接嵌入表单行（启动按钮样式与载荷面板同步），
		//控件本体隐藏（弹窗是宿主主窗口的子窗口，不受影响）
		_discreteEntryBtn = _discreteControl->entryButton();
		if (_discreteEntryBtn)
		{
			_discreteControl->detachEntryButton(this);
			_discreteControl->hide();
		}

		//数据入口行标签（点云/离散共用，文案随模式刷新；
		//行的插入/移除由 rebuildFieldDataRows() 集中管理）
		_dataEntryLabel = new PanelLabel(QStringLiteral("点云数据"));

		//点云数据吸附式弹窗控件（仅内联+点云显示，2.2.6 D2 形态）：X/Y/Z 固定坐标列 +
		//分量可变列、无拾取入口；弹窗内表格承担编辑，取代原"标签 + 图标按钮 + 数据表对话框"组合。
		//分量列数随"分量数量"联动，由 syncPointCloudControlState 统一下发
		_pointCloudControl = new mPointCloudDataControl(QStringLiteral("点云数据"), this);
		_pointCloudControl->setComponentCount(_componentCountSpin->value());
		//表下提供"渲染云图"（D2：点云不提供"绘制曲线"），触发粒子云渲染开关
		_pointCloudControl->setRenderButtonVisible(true, QStringLiteral("渲染云图"));
		_pointCloudControl->setRenderHandler([this]() {
			slot_renderPointCloud();
			});
		//X/Y/Z 坐标列输入期校验：非数值拒绝写入并提示（空串视为未输入，
		//映射/渲染时由业务跳过无效行，与分量列"留空=未输入"语义一致）
		_pointCloudControl->setValueCellValidator([this](int column, const QString& text) -> QString {
			if (column < 3 && !text.isEmpty())
			{
				bool ok = false;
				text.toDouble(&ok);
				if (!ok)
					return QStringLiteral("坐标必须为数值");
			}
			return QString();
			});
		//弹窗内导入/导出与离散弹窗同源（解析/校验/导出共用 mAnalysisFieldTextIO）
		_pointCloudControl->setImportHandler([this](const mPointCloudDataControl::ImportParams& params) {
			importPointCloudTextFile(params);
			});
		_pointCloudControl->setExportHandler([this](const QString& filePath) {
			exportPointCloudTextFile(filePath);
			});
		//行数据变化（编辑/增删/清空；导入回灌由 setRows 承担）：整表写回数据对象并联动树状态。
		//控件弹窗只在"内联 + 点云"页面可达，其余页面的残留信号直接忽略
		connect(_pointCloudControl, &mPointCloudDataControl::dataChanged,
			this, [this]() {
				if (!_analysisFieldData || !_pointCloudControl)
					return;
				if (isTextFileMethod() || !isPointCloud())
					return;
				syncPointCloudControlToField();
			});
		//表头分量增减入口（D7/D8）：−/＋ 经"分量数量"spin 走统一联动
		//（数据对象/点云控件/离散控件的分量同步收口在 onComponentCountChanged）
		connect(_pointCloudControl, &mPointCloudDataControl::componentCountChangeRequested,
			this, [this](int delta) {
				_componentCountSpin->setValue(qMax(1, _componentCountSpin->value() + delta));
			});
		//入口按钮从控件中剖分出来直接嵌入表单行（与离散数据入口同一形态），
		//控件本体隐藏（弹窗是宿主主窗口的子窗口，不受影响）
		_pointCloudEntryBtn = _pointCloudControl->entryButton();
		if (_pointCloudEntryBtn)
		{
			_pointCloudControl->detachEntryButton(this);
			_pointCloudControl->hide();
		}

		//默认场值（仅内联+点云显示；行的插入/移除由 rebuildFieldDataRows() 集中管理）
		//多分量默认值按逗号/空白分隔（映射时拆分），不能整体当表达式求值，
		//故保留 QLineEdit 仅做字符白名单限制（数字/正负号/小数点/eE/分隔符）
		_defaultValueLabel = new PanelLabel(QStringLiteral("默认场值"));
		_defaultValueEdit = new QLineEdit(this);
		_defaultValueEdit->setPlaceholderText(QStringLiteral("请输入默认场值"));
		_defaultValueEdit->setValidator(new QRegularExpressionValidator(
			QRegularExpression(QStringLiteral("^[0-9eE+\\-.,，\\s]*$")), _defaultValueEdit));

		m_foldWidgets.append(_dataFold);
		mainLayout->addWidget(_dataFold);

		//==================== 分组三：映射空间定义 ====================
		_mapSpaceFold = new FoldSection(QStringLiteral("映射空间定义"), this);

		_coordinateCombo = new QComboBox(this);
		//坐标系名来自模型（mx 导入后可能很长）：QComboBox 最小尺寸默认按最长条目计算，
		//会把整行最小宽度撑爆参数栏导致右侧被裁切；改为按固定字符数计宽，超长名称在框内截断
		_coordinateCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
		_coordinateCombo->setMinimumContentsLength(10);
		_mapSpaceFold->addRow(new PanelLabel(QStringLiteral("坐标系")), _coordinateCombo);

		_scaleModeCombo = new QComboBox(this);
		_scaleModeCombo->addItems({ QStringLiteral("保持原始尺寸"), QStringLiteral("等比缩放"), QStringLiteral("非等比缩放") });
		_mapSpaceFold->addRow(new PanelLabel(QStringLiteral("缩放方式")), _scaleModeCombo);

		//缩放系数允许负值（镜像缩放），留空按默认 1.0 处理（textOrDefault 兜底）
		_uniformScaleLabel = new PanelLabel(QStringLiteral("缩放系数"));
		_uniformScaleEdit = new mCalcLineEdit(QStringLiteral("1"), mCalcLineEdit::CalcMode::Standard, this);

		_scaleXLabel = new PanelLabel(QStringLiteral("X 方向系数"));
		_scaleXEdit = new mCalcLineEdit(QStringLiteral("1"), mCalcLineEdit::CalcMode::Standard, this);

		_scaleYLabel = new PanelLabel(QStringLiteral("Y 方向系数"));
		_scaleYEdit = new mCalcLineEdit(QStringLiteral("1"), mCalcLineEdit::CalcMode::Standard, this);

		_scaleZLabel = new PanelLabel(QStringLiteral("Z 方向系数"));
		_scaleZEdit = new mCalcLineEdit(QStringLiteral("1"), mCalcLineEdit::CalcMode::Standard, this);
		//缩放系数行的插入/移除统一由 rebuildMapSpaceRows() 管理

		_targetRegionCombo = new QComboBox(this);
		//集合名来自模型（同坐标系下拉框，超长名称不撑爆行宽）
		_targetRegionCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
		_targetRegionCombo->setMinimumContentsLength(10);
		_mapSpaceFold->addRow(new PanelLabel(QStringLiteral("目标集合")), _targetRegionCombo);

		m_foldWidgets.append(_mapSpaceFold);
		mainLayout->addWidget(_mapSpaceFold);

		//==================== 分组四：映射控制 ====================
		//本分组标签文字较长，不用 PanelLabel（固定 90px），改为按内容定宽的普通 QLabel，
		//由 QFormLayout 统一对齐组内标签列，右侧输入框随之收窄（仅组内对齐，不与其他分组对齐）
		_mapControlFold = new FoldSection(QStringLiteral("映射控制"), this);

		_toleranceTypeCombo = new QComboBox(this);
		_toleranceTypeCombo->addItems({ QStringLiteral("相对容差"), QStringLiteral("绝对容差") });
		_mapControlFold->addRow(new QLabel(QStringLiteral("容差类型"), this), _toleranceTypeCombo);

		//容差为距离/比值，非负；留空按各自默认值处理（textOrDefault 兜底）
		_positiveToleranceEdit = new mCalcLineEdit(QStringLiteral("0.5"), mCalcLineEdit::CalcMode::NonNegative, this);
		_mapControlFold->addRow(new QLabel(QStringLiteral("正向法线搜索距离容差"), this), _positiveToleranceEdit);

		_negativeToleranceEdit = new mCalcLineEdit(QStringLiteral("0.5"), mCalcLineEdit::CalcMode::NonNegative, this);
		_mapControlFold->addRow(new QLabel(QStringLiteral("负向法线搜索距离容差"), this), _negativeToleranceEdit);

		_neighborhoodToleranceEdit = new mCalcLineEdit(QStringLiteral("0.1"), mCalcLineEdit::CalcMode::NonNegative, this);
		_mapControlFold->addRow(new QLabel(QStringLiteral("邻域搜索距离容差"), this), _neighborhoodToleranceEdit);

		_interpolationToleranceEdit = new mCalcLineEdit(QStringLiteral("0.5"), mCalcLineEdit::CalcMode::NonNegative, this);
		_mapControlFold->addRow(new QLabel(QStringLiteral("插值容差"), this), _interpolationToleranceEdit);

		_mappingAlgorithmCombo = new QComboBox(this);
		_mappingAlgorithmCombo->addItems({ QStringLiteral("表面映射"), QStringLiteral("体映射") });
		_mapControlFold->addRow(new QLabel(QStringLiteral("目标面映射算法"), this), _mappingAlgorithmCombo);

		m_foldWidgets.append(_mapControlFold);
		mainLayout->addWidget(_mapControlFold);

		mainLayout->addStretch();

		//==================== 底部动作区 ====================
		_bottomWidget = new QWidget(this);
		QVBoxLayout* bottomLayout = new QVBoxLayout(_bottomWidget);
		bottomLayout->setContentsMargins(12, 8, 12, 8);
		QFrame* separator = new QFrame(_bottomWidget);
		separator->setFrameShape(QFrame::HLine);
		separator->setStyleSheet(QStringLiteral("color: #dcdfe6;"));
		bottomLayout->addWidget(separator);
		_bottomActionBtn = new QPushButton(QStringLiteral("映射至目标对象"), _bottomWidget);
		_bottomActionBtn->setFixedHeight(32);
		_bottomActionBtn->setStyleSheet(QStringLiteral(
			"QPushButton { background-color: #409EFF; color: #ffffff; border: none; border-radius: 3px; }"
			"QPushButton:hover { background-color: #66b1ff; }"
			"QPushButton:pressed { background-color: #3a8ee6; }"));
		connect(_bottomActionBtn, &QPushButton::clicked, this, &mAnalysisFieldPanel::slot_bottomAction);
		bottomLayout->addWidget(_bottomActionBtn);
		mainLayout->addWidget(_bottomWidget);

		//==================== 信号连接 ====================
		connect(_locateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &mAnalysisFieldPanel::onLocationChanged);
		connect(_componentCountSpin, QOverload<int>::of(&QSpinBox::valueChanged),
			this, &mAnalysisFieldPanel::onComponentCountChanged);
		connect(_methodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &mAnalysisFieldPanel::syncVisibility);
		connect(_dataTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &mAnalysisFieldPanel::onDataTypeChanged);
		connect(_scaleModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &mAnalysisFieldPanel::syncVisibility);

		//坐标系增删改时动态刷新坐标系下拉框
		connect(MechCommon::mMechGlobalSignals::getInstance(),
			&MechCommon::mMechGlobalSignals::createUpdateCoordinateSig,
			this, &mAnalysisFieldPanel::refreshCoordinateCombo);

		refreshCoordinateCombo();
		refreshTargetRegionCombo();
		syncVisibility();
	}

	MxFunctions::PickDataType mAnalysisFieldPanel::currentPickType() const
	{
		switch (_locateCombo->currentIndex())
		{
		case 0: return MxFunctions::PickDataType::Node;
		case 2: return MxFunctions::PickDataType::ElementFace;
		default: return MxFunctions::PickDataType::Element;
		}
	}

	MxFunctions::SetType mAnalysisFieldPanel::currentSetType() const
	{
		switch (_locateCombo->currentIndex())
		{
		case 0: return MxFunctions::Node;
		case 2: return MxFunctions::ElementSurface;
		default: return MxFunctions::Element;
		}
	}

	bool mAnalysisFieldPanel::isPointCloud() const
	{
		return _dataTypeCombo->currentIndex() == 0;
	}

	bool mAnalysisFieldPanel::isTextFileMethod() const
	{
		return _methodCombo->currentIndex() == 1;
	}

	void mAnalysisFieldPanel::rebuildFieldDataRows()
	{
		const bool textFile = isTextFileMethod();
		const bool pointCloud = isPointCloud();
		const bool inlineDiscrete = !textFile && !pointCloud;
		QFormLayout* form = _dataFold->layout();

		//用 takeRow 把各行从布局中取出（不删除控件），再按当前模式重新插入。
		//先按字段控件取行，最后兜底按行号清掉残留的固定行（定义方式/数据类型）。
		form->takeRow(_methodCombo);
		form->takeRow(_dataTypeCombo);
		form->takeRow(_fileRowWidget);
		form->takeRow(_startRowSpin);
		form->takeRow(_endRowSpin);
		form->takeRow(_startColumnSpin);
		if (_pointCloudEntryBtn)
			form->takeRow(_pointCloudEntryBtn);
		if (_discreteEntryBtn)
			form->takeRow(_discreteEntryBtn);
		form->takeRow(_defaultValueEdit);
		while (form->rowCount() > 0)
			form->takeRow(0);	//兜底清理（如随行删除的"定义方式"标签等）

		//按当前模式重新组装：
		// 内联：    定义方式 → 数据类型 → 数据入口 → (默认场值)
		// 文本文档：定义方式 → 文件路径 → 数据类型 → 起始行/结束行/起始列 → (默认场值)
		//数据入口点云/离散均为"标签 + 吸附式弹窗启动按钮"（同一形态，2.2.6 D2/D3）
		int row = 0;
		form->insertRow(row++, _defineMethodLabel, _methodCombo);
		if (textFile)
		{
			form->insertRow(row++, _filePathLabel, _fileRowWidget);
		}
		form->insertRow(row++, _dataTypeLabel, _dataTypeCombo);
		if (textFile)
		{
			form->insertRow(row++, _startRowLabel, _startRowSpin);
			form->insertRow(row++, _endRowLabel, _endRowSpin);
			form->insertRow(row++, _startColumnLabel, _startColumnSpin);
		}
		if (!textFile)
		{
			//数据入口行：点云"点云数据"，离散"XX离散数据"
			_dataEntryLabel->setText(pointCloud ? QStringLiteral("点云数据")
				: _locateCombo->currentText() + QStringLiteral("离散数据"));
			if (pointCloud)
			{
				if (_pointCloudEntryBtn)
					form->insertRow(row++, _dataEntryLabel, _pointCloudEntryBtn);
			}
			else if (_discreteEntryBtn)
				form->insertRow(row++, _dataEntryLabel, _discreteEntryBtn);
		}
		if (pointCloud)
			form->insertRow(row++, _defaultValueLabel, _defaultValueEdit);

		//被移出布局的控件统一隐藏，防止游离残留
		_filePathLabel->setVisible(textFile);
		_fileRowWidget->setVisible(textFile);
		_startRowLabel->setVisible(textFile);
		_startRowSpin->setVisible(textFile);
		_endRowLabel->setVisible(textFile);
		_endRowSpin->setVisible(textFile);
		_startColumnLabel->setVisible(textFile);
		_startColumnSpin->setVisible(textFile);
		_dataEntryLabel->setVisible(!textFile);
		if (_pointCloudEntryBtn)
			_pointCloudEntryBtn->setVisible(!textFile && pointCloud);
		if (_discreteEntryBtn)
			_discreteEntryBtn->setVisible(inlineDiscrete);
		_defaultValueLabel->setVisible(pointCloud);
		_defaultValueEdit->setVisible(pointCloud);

		_dataFold->refreshLayout();
	}

	void mAnalysisFieldPanel::rebuildMapSpaceRows()
	{
		const int scaleMode = _scaleModeCombo->currentIndex();
		QFormLayout* form = _mapSpaceFold->layout();

		form->takeRow(_uniformScaleEdit);
		form->takeRow(_scaleXEdit);
		form->takeRow(_scaleYEdit);
		form->takeRow(_scaleZEdit);

		if (scaleMode == 1)
			form->insertRow(2, _uniformScaleLabel, _uniformScaleEdit);
		else if (scaleMode == 2)
		{
			form->insertRow(2, _scaleXLabel, _scaleXEdit);
			form->insertRow(3, _scaleYLabel, _scaleYEdit);
			form->insertRow(4, _scaleZLabel, _scaleZEdit);
		}

		_uniformScaleLabel->setVisible(scaleMode == 1);
		_uniformScaleEdit->setVisible(scaleMode == 1);
		_scaleXLabel->setVisible(scaleMode == 2);
		_scaleXEdit->setVisible(scaleMode == 2);
		_scaleYLabel->setVisible(scaleMode == 2);
		_scaleYEdit->setVisible(scaleMode == 2);
		_scaleZLabel->setVisible(scaleMode == 2);
		_scaleZEdit->setVisible(scaleMode == 2);

		_mapSpaceFold->refreshLayout();
	}

	void mAnalysisFieldPanel::syncVisibility()
	{
		const bool textFile = isTextFileMethod();
		const bool pointCloud = isPointCloud();
		const bool inlineDiscrete = !textFile && !pointCloud;

		//切到"文本文档"后数据入口（含云图开关）不再可用，
		//残留云图属于上一模式，必须关闭，否则切页后视口仍显示旧云图
		if (textFile && _cloudOn)
			clearCloudRender();

		//隐藏折叠块前先把内容控件高度上限归零：FoldWidget 初始化时给内容设了
		//maximumHeight=sizeHint，只 setVisible(false) 时布局可能仍按旧高度占位，
		//导致隐藏分组原位置留空白；显示时先解除上限，由 refreshLayout() 延迟重算。
		auto setFoldContentVisible = [](FoldSection* fold, bool visible) {
			if (!fold || !fold->contentWidget())
				return;
			if (visible)
				fold->contentWidget()->setMaximumHeight(QWIDGETSIZE_MAX);
			fold->setVisible(visible);
			if (!visible)
				fold->contentWidget()->setMaximumHeight(0);
			};

		//场数据分组：条件行改为移除/重插，避免隐藏后残留布局空隙
		rebuildFieldDataRows();

		//离散数据吸附式弹窗：仅"内联 + 离散"页面接入（入口按钮已在表单行中），
		//随位置/分量数量/行数据联动；切离该页面时关闭弹窗并结束拾取会话
		if (inlineDiscrete)
			syncDiscreteControlState();
		else if (_discreteControl)
			_discreteControl->closePopup();

		//点云数据吸附式弹窗：仅"内联 + 点云"页面接入（文本文档模式无数据入口行），
		//随分量数量/行数据联动；切离该页面（文本文档/离散）时关闭弹窗
		if (pointCloud && !textFile)
			syncPointCloudControlState();
		else if (_pointCloudControl)
			_pointCloudControl->closePopup();

		//映射分组（仅点云，含文本文档+点云）
		setFoldContentVisible(_mapSpaceFold, pointCloud);
		setFoldContentVisible(_mapControlFold, pointCloud);

		//缩放方式子状态（同样改为移除/重插）
		rebuildMapSpaceRows();

		//底部动作按钮
		if (textFile)
		{
			_bottomWidget->setVisible(true);
			_bottomActionBtn->setText(QStringLiteral("导入数据转换为内联定义场"));
		}
		else if (pointCloud)
		{
			_bottomWidget->setVisible(true);
			_bottomActionBtn->setText(QStringLiteral("映射至目标对象"));
		}
		else
		{
			_bottomWidget->setVisible(false);
		}

		//数据入口标签文案（点云"点云数据"，离散"XX离散数据"）；
		//入口摘要（点击编辑数据表/已定义 N 行数据）由两控件随行数据自行刷新
		if (_dataEntryLabel)
			_dataEntryLabel->setText(pointCloud ? QStringLiteral("点云数据")
				: _locateCombo->currentText() + QStringLiteral("离散数据"));

		//隐藏分组内容高度已归零，跳过其刷新；显示的分组由 refreshLayout()
		//延迟重算内容高度（先解除上限再按 sizeHint 锁定）
		for (auto* fold : m_foldWidgets)
		{
			if (!fold->isVisible())
				continue;
			fold->refreshLayout();
		}

		scheduleHeightPolicy();
	}

	void mAnalysisFieldPanel::scheduleHeightPolicy()
	{
		//换入时 mInfoWindow::setMainWidget() 会把面板策略重置为 Expanding，面板撑满
		//Info Dock，行减少后底部留大片空白。这里在布局稳定后恢复 Maximum 策略：
		//面板按内容高度布局，多余空间由 setMainWidget 追加的底部 stretch spacer 吸收。
		//FoldSection::refreshLayout() 内部是 5ms 延迟刷新，等它执行完再恢复策略。
		if (_policyScheduled)
			return;	//本轮事件循环内合并为一次
		_policyScheduled = true;
		QTimer::singleShot(30, this, [this]() {
			_policyScheduled = false;
			setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
			updateGeometry();
			});
	}

	void mAnalysisFieldPanel::showEvent(QShowEvent* event)
	{
		mPanelBase::showEvent(event);
		//换入显示时 setMainWidget 已把策略重置为 Expanding，重新申请按内容收缩
		scheduleHeightPolicy();
	}

	void mAnalysisFieldPanel::onLocationChanged()
	{
		//位置切换后原云图不再对应新位置类型，统一关闭
		clearCloudRender();

		//① 离散选项标签随位置联动
		_dataTypeCombo->setItemText(1, _locateCombo->currentText() + QStringLiteral("离散"));

		//② 目标集合列表按位置重新填充
		refreshTargetRegionCombo();

		//④ 离散数据行与位置类型绑定：切换位置后旧类型行不再合法，统一清空。
		//控件行先清再关弹窗：closePopup 结束拾取会补发一次 dataChanged，
		//若旧类型行未清会按新位置类型误转换写回数据对象
		if (!isPointCloud() && !isTextFileMethod() && _discreteControl)
		{
			if (_analysisFieldData && !_analysisFieldData->rowStorage().isEmpty())
				_analysisFieldData->setRows({});
			_discreteControl->setRows({});
			_discreteControl->closePopup();
		}

		if (_analysisFieldData)
			_analysisFieldData->setLocate(currentPickType());

	//⑥ 显隐/控件状态统一刷新：离散控件的标题/拾取对象描述（节点/单元/单元面，
	//含视口拾取过滤器）在 syncVisibility -> syncDiscreteControlState 内联动刷新
	syncVisibility();
}

	void mAnalysisFieldPanel::onDataTypeChanged()
	{
		//点云/离散切换后原云图通道不再适用，统一关闭
		clearCloudRender();
		if (!_analysisFieldData)
			return;	//无数据对象（如删除流程收尾）时只复位界面，不触碰行存储
		//切回点云时行存储里仍是映射生成的离散行（无 X/Y/Z），
		//从映射前快照恢复点云行；无快照（从未映射过）则保持现状
		if (isPointCloud() && _analysisFieldData && !_pointCloudBackup.isEmpty())
		{
			_analysisFieldData->setRows(_pointCloudBackup);
		}
		syncVisibility();
	}

	void mAnalysisFieldPanel::onComponentCountChanged(int count)
	{
		if (_analysisFieldData)
		{
			_analysisFieldData->setComponentCount(count);
			//分量变化需就地调整行数据 values 长度（数据对象为唯一数据源）
			_analysisFieldData->normalizeRowValues();
		}
		//离散控件分量列同步伸缩（各自规整行数据，两侧保持一致）
		if (_discreteControl)
			_discreteControl->setComponentCount(count);
		//点云控件分量列同步伸缩（X/Y/Z 固定，可变分量列在 3 列之后增减）
		if (_pointCloudControl)
			_pointCloudControl->setComponentCount(count);
	}

	void mAnalysisFieldPanel::refreshCoordinateCombo()
	{
		if (!getCVM)
			return;
		QStringList names = getCVM->getModelData()
			->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate)
			.getAllCoordinateNames();	//第一项即"全局坐标系"(ID=-1)
		const QString current = _coordinateCombo->currentText();
		_coordinateCombo->blockSignals(true);
		_coordinateCombo->clear();
		_coordinateCombo->addItems(names);
		const int idx = _coordinateCombo->findText(current);
		_coordinateCombo->setCurrentIndex(idx >= 0 ? idx : 0);
		_coordinateCombo->blockSignals(false);
	}

	void mAnalysisFieldPanel::refreshTargetRegionCombo()
	{
		if (!getCVM)
			return;
		auto& setMgr = getCVM->getModelData()
			->getManager<MechSetData::mSetManager>(Physical_Set);
		const QString current = _targetRegionCombo->currentText();
		_targetRegionCombo->blockSignals(true);
		_targetRegionCombo->clear();
		_targetRegionCombo->addItem(QStringLiteral("整个模型"));	//固定第一项
		_targetRegionCombo->addItems(setMgr.getAllSetNamesByTypes(currentSetType()));
		const int idx = _targetRegionCombo->findText(current);
		_targetRegionCombo->setCurrentIndex(idx >= 0 ? idx : 0);
		_targetRegionCombo->blockSignals(false);
	}

	void mAnalysisFieldPanel::syncDiscreteControlState()
	{
		if (!_discreteControl)
			return;
		//标题/拾取对象描述随"位置"联动（节点/单元/单元面，含视口拾取过滤器）
		_discreteControl->setPopupTitle(_locateCombo->currentText() + QStringLiteral("离散数据"));
		_discreteControl->setPickSpec(makeDiscretePickSpec(currentPickType()));
		//分量列数随"分量数量"联动
		_discreteControl->setComponentCount(_componentCountSpin->value());
		//拾取会话进行中行数据由视口交互驱动，不整体重灌
		//（setPickSpec 切换描述时已结束会话，此处防御外部直接调用）
		if (_discreteControl->isPicking())
			return;
		//数据对象行（唯一数据源）-> 控件行（点云坐标行等非本类型数据自然被过滤）
		QVector<mDiscreteDataControl::Row> rows;
		if (_analysisFieldData)
			rows = fieldRowsToControlRows(_analysisFieldData->rowStorage(), currentPickType());
		_discreteControl->setRows(rows);
	}

	void mAnalysisFieldPanel::syncDiscreteControlToField()
	{
		//离散模式以弹窗控件为编辑面：行数据（拾取追加/删除/清空/单元格编辑）
		//整表写回数据对象（编号未填的临时行不落盘），并联动树状态
		if (!_analysisFieldData || !_discreteControl)
			return;
		_analysisFieldData->setRows(controlRowsToFieldRows(_discreteControl->rows(), currentPickType()));
		notifyRowsEdited();
	}

	void mAnalysisFieldPanel::syncPointCloudControlState()
	{
		if (!_pointCloudControl)
			return;
		//分量列数随"分量数量"联动（X/Y/Z 固定 3 列由控件内部维护）
		_pointCloudControl->setComponentCount(_componentCountSpin->value());
		//数据对象行（唯一数据源）-> 控件行（X/Y/Z 占前 3 列，分量随分量数量补齐/截断）
		QVector<mPointCloudDataControl::Row> rows;
		if (_analysisFieldData)
			rows = pointCloudRowsToControlRows(_analysisFieldData->rowStorage(), _componentCountSpin->value());
		_pointCloudControl->setRows(rows);
	}

	void mAnalysisFieldPanel::syncPointCloudControlToField()
	{
		//点云模式以弹窗控件为编辑面：行数据（编辑/增删/清空）
		//整表写回数据对象，并联动树状态
		if (!_analysisFieldData || !_pointCloudControl)
			return;
		_analysisFieldData->setRows(controlRowsToPointCloudRows(_pointCloudControl->rows()));
		notifyRowsEdited();
	}

	void mAnalysisFieldPanel::importDiscreteTextFile(const mDiscreteDataControl::ImportParams& params)
	{
		//弹窗内"导入"：与原数据表对话框/文本文档转内联共用同一解析与校验实现
		const QString path = params.filePath.trimmed();
		if (path.isEmpty())
		{
			mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("请先选择要导入的文本文件"), true);
			return;
		}
		QString error;
		QList<AnalysisFieldRow> rows = parseAnalysisFieldTextFile(path,
			params.startRow, params.endRow, params.startColumn,
			false, currentPickType(), _componentCountSpin->value(), error);
		if (!error.isEmpty())
		{
			mGlobalSignals::getInstance()->outputMessageSig(error, true);
			return;
		}
		if (rows.isEmpty())
		{
			mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("未识别到可导入的数据"), true);
			return;
		}
		//追加模式以控件当前行为基础合并（控件为离散模式的编辑面）
		QList<AnalysisFieldRow> newRows;
		if (params.append && _discreteControl)
		{
			newRows = controlRowsToFieldRows(_discreteControl->rows(), currentPickType());
			newRows.reserve(newRows.size() + rows.size());
		}
		newRows.append(rows);
		//导入前整表校验（与手动编辑单元格的规则一致；追加模式校验合并后的全集）
		const QString validateError = validateRowsForImport(newRows);
		if (!validateError.isEmpty())
		{
			mGlobalSignals::getInstance()->outputMessageSig(validateError, true);
			return;
		}
		if (_analysisFieldData)
			_analysisFieldData->setRows(std::move(newRows));
		//控件行随数据对象整体回灌（setRows 不发 dataChanged）
		syncDiscreteControlState();
		mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("已导入 %1 行数据").arg(rows.size()), true);
	}

	void mAnalysisFieldPanel::exportDiscreteTextFile(const QString& filePath)
	{
		//弹窗内"导出"：控件当前行导出文本文件（表头/分隔与原数据表对话框一致）
		const QString path = filePath.trimmed();
		if (path.isEmpty())
		{
			mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("请先选择导出文件路径"), true);
			return;
		}
		const QList<AnalysisFieldRow> rows = _discreteControl
			? controlRowsToFieldRows(_discreteControl->rows(), currentPickType())
			: QList<AnalysisFieldRow>();
		QString error;
		if (!exportAnalysisFieldTextFile(path, rows, false,
			currentPickType(), _componentCountSpin->value(), error))
		{
			mGlobalSignals::getInstance()->outputMessageSig(error, true);
			return;
		}
		mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("已导出 %1 行数据").arg(rows.size()), true);
	}

	void mAnalysisFieldPanel::importPointCloudTextFile(const mPointCloudDataControl::ImportParams& params)
	{
		//弹窗内"导入"：与离散导入/文本文档转内联共用同一解析与校验实现
		//（点云最少 3 列 X,Y,Z，分量…）
		const QString path = params.filePath.trimmed();
		if (path.isEmpty())
		{
			mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("请先选择要导入的文本文件"), true);
			return;
		}
		QString error;
		QList<AnalysisFieldRow> rows = parseAnalysisFieldTextFile(path,
			params.startRow, params.endRow, params.startColumn,
			true, currentPickType(), _componentCountSpin->value(), error);
		if (!error.isEmpty())
		{
			mGlobalSignals::getInstance()->outputMessageSig(error, true);
			return;
		}
		if (rows.isEmpty())
		{
			mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("未识别到可导入的数据"), true);
			return;
		}
		//追加模式以控件当前行为基础合并（控件为点云模式的编辑面）
		QList<AnalysisFieldRow> newRows;
		if (params.append && _pointCloudControl)
		{
			newRows = controlRowsToPointCloudRows(_pointCloudControl->rows());
			newRows.reserve(newRows.size() + rows.size());
		}
		newRows.append(rows);
		//导入前整表校验（与手动编辑单元格的规则一致；追加模式校验合并后的全集）
		const QString validateError = validateRowsForImport(newRows);
		if (!validateError.isEmpty())
		{
			mGlobalSignals::getInstance()->outputMessageSig(validateError, true);
			return;
		}
		if (_analysisFieldData)
			_analysisFieldData->setRows(std::move(newRows));
		//控件行随数据对象整体回灌（setRows 不发 dataChanged）
		syncPointCloudControlState();
		mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("已导入 %1 行数据").arg(rows.size()), true);
	}

	void mAnalysisFieldPanel::exportPointCloudTextFile(const QString& filePath)
	{
		//弹窗内"导出"：控件当前行导出文本文件（表头 X,Y,Z,Value1,…）
		const QString path = filePath.trimmed();
		if (path.isEmpty())
		{
			mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("请先选择导出文件路径"), true);
			return;
		}
		const QList<AnalysisFieldRow> rows = _pointCloudControl
			? controlRowsToPointCloudRows(_pointCloudControl->rows())
			: QList<AnalysisFieldRow>();
		QString error;
		if (!exportAnalysisFieldTextFile(path, rows, true,
			currentPickType(), _componentCountSpin->value(), error))
		{
			mGlobalSignals::getInstance()->outputMessageSig(error, true);
			return;
		}
		mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("已导出 %1 行数据").arg(rows.size()), true);
	}

	void mAnalysisFieldPanel::notifyRowsEdited()
	{
		//行数据已就地写入数据对象，这里只做联动收尾：树状态。
		//入口摘要（已定义 N 行数据）由弹窗控件随自身行数据刷新。
		if (_analysisFieldData && getCVM)
			getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Field, _analysisFieldID);
	}

	QString mAnalysisFieldPanel::validateRowsForImport(const QList<AnalysisFieldRow>& rows) const
	{
		//批量导入前的整表校验，与手动编辑单元格的规则保持一致：
		//点云校验坐标数值；离散校验编号存在性 + 表内无重复
		if (isPointCloud())
		{
			for (int i = 0; i < rows.size(); ++i)
			{
				const auto& r = rows.at(i);
				bool ok = false;
				r.x.toDouble(&ok);
				if (!ok) return QStringLiteral("第 %1 行 X 坐标必须为数值").arg(i + 1);
				r.y.toDouble(&ok);
				if (!ok) return QStringLiteral("第 %1 行 Y 坐标必须为数值").arg(i + 1);
				r.z.toDouble(&ok);
				if (!ok) return QStringLiteral("第 %1 行 Z 坐标必须为数值").arg(i + 1);
			}
			return QString();
		}

		mMeshData* meshData = (getCVM && getCVM->getModelData()) ? getCVM->getModelData()->getMeshData() : nullptr;
		if (meshData == nullptr)
			return QString();	//无网格数据时不做存在性校验（与手动编辑行为一致）

		const MxFunctions::PickDataType type = currentPickType();
		auto elementExists = [meshData](int eleId) -> bool {
			for (int partId : meshData->getAllPartIds())
			{
				mPartData* part = meshData->getPartData(partId);
				if (part != nullptr && part->getElementTypeByElement(eleId) >= 0)
					return true;
			}
			return false;
		};

		QSet<QString> seen;
		seen.reserve(rows.size());
		for (int i = 0; i < rows.size(); ++i)
		{
			const auto& r = rows.at(i);
			if (type == MxFunctions::PickDataType::Node)
			{
				bool ok = false;
				const int id = r.id.toInt(&ok);
				if (!ok || id <= 0)
					return QStringLiteral("第 %1 行节点编号必须为正整数").arg(i + 1);
				if (id > meshData->getMaxNodeId() || meshData->getPartIdByNodeId(id).empty())
					return QStringLiteral("第 %1 行节点 %2 不存在").arg(i + 1).arg(id);
				if (seen.contains(r.id))
					return QStringLiteral("第 %1 行节点 %2 重复").arg(i + 1).arg(id);
				seen.insert(r.id);
			}
			else if (type == MxFunctions::PickDataType::Element)
			{
				bool ok = false;
				const int id = r.id.toInt(&ok);
				if (!ok || id <= 0)
					return QStringLiteral("第 %1 行单元编号必须为正整数").arg(i + 1);
				if (!elementExists(id))
					return QStringLiteral("第 %1 行单元 %2 不存在").arg(i + 1).arg(id);
				if (seen.contains(r.id))
					return QStringLiteral("第 %1 行单元 %2 重复").arg(i + 1).arg(id);
				seen.insert(r.id);
			}
			else
			{
				bool ok = false;
				const int eleId = r.elementId.toInt(&ok);
				if (!ok || eleId <= 0)
					return QStringLiteral("第 %1 行单元编号必须为正整数").arg(i + 1);
				if (!elementExists(eleId))
					return QStringLiteral("第 %1 行单元 %2 不存在").arg(i + 1).arg(eleId);
				const int faceId = r.faceId.toInt(&ok);
				if (!ok || faceId <= 0)
					return QStringLiteral("第 %1 行面编号必须为正整数").arg(i + 1);
				const QString key = r.elementId + QLatin1Char('|') + r.faceId;
				if (seen.contains(key))
					return QStringLiteral("第 %1 行单元 %2 面 %3 重复").arg(i + 1).arg(eleId).arg(faceId);
				seen.insert(key);
			}
		}
		return QString();
	}

	void mAnalysisFieldPanel::syncUiToField()
	{
		if (!_analysisFieldData)
			return;

		//数值输入框允许留空：空文本按默认值处理，避免 toDouble() 静默写成 0
		auto textOrDefault = [](const QLineEdit* edit, double def) {
			bool ok = false;
			const double v = edit->text().trimmed().toDouble(&ok);
			return ok ? v : def;
		};

		_analysisFieldData->setLocate(currentPickType());
		_analysisFieldData->setComponentCount(_componentCountSpin->value());
		_analysisFieldData->setDefineMethod(isTextFileMethod()
			? mAnalysisField::DefineMethod::TextFile : mAnalysisField::DefineMethod::Inline);
		_analysisFieldData->setDataType(isPointCloud()
			? mAnalysisField::DataType::PointCloud : mAnalysisField::DataType::ObjectDiscrete);
		_analysisFieldData->setTextFilePath(_filePathEdit->text());
		_analysisFieldData->setTextStartRow(_startRowSpin->value());
		_analysisFieldData->setTextEndRow(_endRowSpin->value());
		_analysisFieldData->setTextStartColumn(_startColumnSpin->value());
		_analysisFieldData->setDefaultValue(_defaultValueEdit->text());

		//坐标系/目标集合不存数据类，走映射关系（参考集中力对坐标系的映射）。
		//"全局坐标系"/"整个模型"均等价于 -1（无映射目标）。
		if (getCVM && getCVM->getModelData())
		{
			auto& mapRel = getCVM->getModelData()->getMapRelationManager();

			const QString coordinateName = _coordinateCombo->currentText();
			int coorID = -1;
			if (!coordinateName.isEmpty() && coordinateName != QStringLiteral("全局坐标系"))
			{
				coorID = getCVM->getModelData()
					->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate)
					.getIDByCoordinateName(coordinateName);
			}
			mapRel.appendMapRelation({ Physical_Field, _analysisFieldID }, { Physical_Coordinate, coorID });

			const QString targetRegion = _targetRegionCombo->currentText();
			int setID = -1;
			if (!targetRegion.isEmpty() && targetRegion != QStringLiteral("整个模型"))
			{
				auto* setData = getCVM->getModelData()
					->getManager<MechSetData::mSetManager>(Physical_Set)
					.getSetData(targetRegion);
				if (setData)
					setID = setData->getSetId();
			}
			mapRel.appendMapRelation({ Physical_Field, _analysisFieldID }, { Physical_Set, setID });
		}

		_analysisFieldData->setScaleMode(static_cast<mAnalysisField::ScaleMode>(_scaleModeCombo->currentIndex()));
		_analysisFieldData->setUniformScale(textOrDefault(_uniformScaleEdit, 1.0));
		_analysisFieldData->setScaleX(textOrDefault(_scaleXEdit, 1.0));
		_analysisFieldData->setScaleY(textOrDefault(_scaleYEdit, 1.0));
		_analysisFieldData->setScaleZ(textOrDefault(_scaleZEdit, 1.0));

		_analysisFieldData->setToleranceType(static_cast<mAnalysisField::ToleranceType>(_toleranceTypeCombo->currentIndex()));
		_analysisFieldData->setPositiveTolerance(textOrDefault(_positiveToleranceEdit, 0.5));
		_analysisFieldData->setNegativeTolerance(textOrDefault(_negativeToleranceEdit, 0.5));
		_analysisFieldData->setNeighborhoodTolerance(textOrDefault(_neighborhoodToleranceEdit, 0.1));
		_analysisFieldData->setInterpolationTolerance(textOrDefault(_interpolationToleranceEdit, 0.5));
		_analysisFieldData->setMappingAlgorithm(static_cast<mAnalysisField::MappingAlgorithm>(_mappingAlgorithmCombo->currentIndex()));

		//行数据直接由拾取控件 dataChanged / 表格模型（绑定同一存储）就地写入数据对象，
		//此处无需做任何行级同步
	}

	void mAnalysisFieldPanel::syncFieldToUi()
	{
		if (!_analysisFieldData)
			return;

		const QList<QComboBox*> combos = { _locateCombo, _methodCombo, _dataTypeCombo,
			_scaleModeCombo, _coordinateCombo, _targetRegionCombo,
			_toleranceTypeCombo, _mappingAlgorithmCombo };
		for (auto* combo : combos)
			combo->blockSignals(true);
		_componentCountSpin->blockSignals(true);

		switch (_analysisFieldData->getLocate())
		{
		case MxFunctions::PickDataType::Node: _locateCombo->setCurrentIndex(0); break;
		case MxFunctions::PickDataType::ElementFace: _locateCombo->setCurrentIndex(2); break;
		default: _locateCombo->setCurrentIndex(1); break;
		}
		refreshTargetRegionCombo();
		_dataTypeCombo->setItemText(1, _locateCombo->currentText() + QStringLiteral("离散"));
		_componentCountSpin->setValue(qMax(1, _analysisFieldData->getComponentCount()));
		_methodCombo->setCurrentIndex(_analysisFieldData->getDefineMethod() == mAnalysisField::DefineMethod::TextFile ? 1 : 0);
		_dataTypeCombo->setCurrentIndex(_analysisFieldData->getDataType() == mAnalysisField::DataType::PointCloud ? 0 : 1);
		_filePathEdit->setText(_analysisFieldData->getTextFilePath());
		_startRowSpin->setValue(qMax(1, _analysisFieldData->getTextStartRow()));
		_endRowSpin->setValue(qMax(1, _analysisFieldData->getTextEndRow()));
		_startColumnSpin->setValue(qMax(1, _analysisFieldData->getTextStartColumn()));
		_defaultValueEdit->setText(_analysisFieldData->getDefaultValue());

		refreshCoordinateCombo();
		//坐标系/目标集合从映射关系反显（-1 即"全局坐标系"/"整个模型"）
		auto* modelData = (getCVM && getCVM->getModelData()) ? getCVM->getModelData() : nullptr;
		if (modelData == nullptr)
		{
			for (auto* combo : combos)
				combo->blockSignals(false);
			_componentCountSpin->blockSignals(false);
			return;
		}
		auto& coorMgr = modelData
			->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate);
		auto& setMgr = modelData
			->getManager<MechSetData::mSetManager>(Physical_Set);
		auto& mapRel = modelData->getMapRelationManager();

		const int coorID = mapRel.getMapRelation<int>({ Physical_Field, _analysisFieldID }, Physical_Coordinate);
		const QString coorName = coorMgr.getCoordinateNameByID(coorID);
		if (coorID <= 0 || coorName.isEmpty())
			_coordinateCombo->setCurrentIndex(0);
		else
		{
			const int idx = _coordinateCombo->findText(coorName);
			_coordinateCombo->setCurrentIndex(idx >= 0 ? idx : 0);
		}

		_scaleModeCombo->setCurrentIndex(static_cast<int>(_analysisFieldData->getScaleMode()));
		//mCalcLineEdit 程序化设值必须走 setExpression：仅 setText 会被下次获得焦点时
		//按内部 _expression 缓存还原为旧值
		_uniformScaleEdit->setExpression(QString::number(_analysisFieldData->getUniformScale()));
		_scaleXEdit->setExpression(QString::number(_analysisFieldData->getScaleX()));
		_scaleYEdit->setExpression(QString::number(_analysisFieldData->getScaleY()));
		_scaleZEdit->setExpression(QString::number(_analysisFieldData->getScaleZ()));

		refreshTargetRegionCombo();
		const int setID = mapRel.getMapRelation<int>({ Physical_Field, _analysisFieldID }, Physical_Set);
		const QString setName = setMgr.getSetNameByID(setID);
		if (setID <= 0 || setName.isEmpty())
			_targetRegionCombo->setCurrentIndex(0);
		else
		{
			const int idx = _targetRegionCombo->findText(setName);
			_targetRegionCombo->setCurrentIndex(idx >= 0 ? idx : 0);
		}

		_toleranceTypeCombo->setCurrentIndex(static_cast<int>(_analysisFieldData->getToleranceType()));
		_positiveToleranceEdit->setExpression(QString::number(_analysisFieldData->getPositiveTolerance()));
		_negativeToleranceEdit->setExpression(QString::number(_analysisFieldData->getNegativeTolerance()));
		_neighborhoodToleranceEdit->setExpression(QString::number(_analysisFieldData->getNeighborhoodTolerance()));
		_interpolationToleranceEdit->setExpression(QString::number(_analysisFieldData->getInterpolationTolerance()));
		_mappingAlgorithmCombo->setCurrentIndex(static_cast<int>(_analysisFieldData->getMappingAlgorithm()));

		for (auto* combo : combos)
			combo->blockSignals(false);
		_componentCountSpin->blockSignals(false);

		syncVisibility();
	}

	void mAnalysisFieldPanel::newAnalysisFieldPanel()
	{
		//重置界面到默认状态
		uneditAnalysisFieldPanel();
		if (!getCVM)
			return;
		int fieldID = getCVM->getModelData()
			->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field)
			.createID();
		_analysisFieldData = new MechDiscreteFieldData::mAnalysisField(fieldID);
		QString name = getCVM->getModelData()->createNameByType(Physical_Field, "AnalysisField");
		_analysisFieldData->setAnalysisFieldName(name);
		getCVM->getModelData()->addPhysicalField<MechDiscreteFieldData::mAnalysisField>(_analysisFieldData);
		_analysisFieldID = fieldID;
		setEditing(true);
		setTitle(name);

		syncFieldToUi();
		//弹窗控件行数据随 syncVisibility -> sync{Discrete,PointCloud}ControlState 回灌
		emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateFieldSig(true, fieldID);
	}

	void mAnalysisFieldPanel::editAnalysisFieldPanel(int fieldID)
	{
		if (!getCVM)
			return;
		auto* fieldData = getCVM->getModelData()
			->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field)
			.getAnalysisFieldData(fieldID);
		if (!fieldData)
			return;	//场数据不存在时保持面板原状，不动 _analysisFieldData/_analysisFieldID
		_analysisFieldData = fieldData;
		_analysisFieldID = fieldID;
		setEditing(true);
		setTitle(_analysisFieldData->getAnalysisFieldName());

		syncFieldToUi();
		//离散/点云模式行数据随 syncVisibility -> sync{Discrete,PointCloud}ControlState 回灌控件
		emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateFieldSig(false, fieldID);
	}

	void mAnalysisFieldPanel::onFieldDataAboutToBeDestroyed(int fieldID)
	{
		//必须在数据对象 delete 之前调用：此时 _analysisFieldData 仍有效，
		//安全关闭数据表对话框（解绑行存储 + deleteLater），随后的界面复位
		//交给 uneditAnalysisFieldPanel()，避免两处各写一半复位逻辑。
		if (fieldID >= 0 && fieldID != _analysisFieldID)
			return;
		//吸附式弹窗控件可能仍开着弹窗/拾取会话：先结束并关闭（离散控件 closePopup 结束
		//拾取会补发一次 dataChanged，行数据同步时数据对象此刻仍有效），再丢弃悬垂行数据
		if (_discreteControl)
		{
			_discreteControl->closePopup();
			_discreteControl->setRows({});
		}
		if (_pointCloudControl)
		{
			_pointCloudControl->closePopup();
			_pointCloudControl->setRows({});
		}
		_analysisFieldData = nullptr;
		_analysisFieldID = -1;
		_pointCloudBackup.clear();
	}

	void mAnalysisFieldPanel::commitEditingData()
	{
		//编辑中直接保存时把控件值写回数据对象（与 uneditAnalysisFieldPanel 的回写一致，
		//但不退出编辑态）。行数据已就地存储在数据对象中，无需同步。
		if (!_analysisFieldData)
			return;
		syncUiToField();
		if (getCVM)
			getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Field, _analysisFieldID);
	}

	void mAnalysisFieldPanel::uneditAnalysisFieldPanel()
	{
		//退出编辑时统一关闭云图渲染并恢复拾取高亮
		clearCloudRender();
		//数据可能已被外部删除（如树上删除场节点），先重新获取，存在才回写
		if (_analysisFieldData && getCVM)
		{
			_analysisFieldData = getCVM->getModelData()
				->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field)
				.getAnalysisFieldData(_analysisFieldID);
		}
		//退出编辑前把控件值写回数据对象。
		//行数据已在编辑期间就地写入（拾取 dataChanged / 表格绑定同一存储），无需再同步。
		if (_analysisFieldData)
		{
			syncUiToField();
			if (getCVM)
				getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Field, _analysisFieldID);
		}
		//吸附式弹窗控件收尾：离散控件结束拾取会话，两者关闭弹窗并清空行，
		//避免下一场编辑复用残留
		if (_discreteControl)
		{
			_discreteControl->closePopup();
			_discreteControl->setRows({});
		}
		if (_pointCloudControl)
		{
			_pointCloudControl->closePopup();
			_pointCloudControl->setRows({});
		}
		_analysisFieldData = nullptr;
		_analysisFieldID = -1;
		setEditing(false);

		//重置界面到默认状态
		_locateCombo->blockSignals(true);
		_methodCombo->blockSignals(true);
		_dataTypeCombo->blockSignals(true);
		_scaleModeCombo->blockSignals(true);
		_componentCountSpin->blockSignals(true);
		_locateCombo->setCurrentIndex(0);
		_methodCombo->setCurrentIndex(0);
		_dataTypeCombo->setCurrentIndex(0);
		_dataTypeCombo->setItemText(1, QStringLiteral("节点离散"));
		_scaleModeCombo->setCurrentIndex(0);
		_componentCountSpin->setValue(1);
		_locateCombo->blockSignals(false);
		_methodCombo->blockSignals(false);
		_dataTypeCombo->blockSignals(false);
		_scaleModeCombo->blockSignals(false);
		_componentCountSpin->blockSignals(false);
		_coordinateCombo->setCurrentIndex(0);
		_targetRegionCombo->setCurrentIndex(0);
		_filePathEdit->clear();
		_startRowSpin->setValue(1);
		_endRowSpin->setValue(1);
		_startColumnSpin->setValue(1);
		_defaultValueEdit->clear();
		_uniformScaleEdit->setExpression(QStringLiteral("1"));
		_scaleXEdit->setExpression(QStringLiteral("1"));
		_scaleYEdit->setExpression(QStringLiteral("1"));
		_scaleZEdit->setExpression(QStringLiteral("1"));
		_toleranceTypeCombo->setCurrentIndex(0);
		_positiveToleranceEdit->setExpression(QStringLiteral("0.5"));
		_negativeToleranceEdit->setExpression(QStringLiteral("0.5"));
		_neighborhoodToleranceEdit->setExpression(QStringLiteral("0.1"));
		_interpolationToleranceEdit->setExpression(QStringLiteral("0.5"));
		_mappingAlgorithmCombo->setCurrentIndex(0);
		setTitle(QStringLiteral("分析场"));
		syncVisibility();
	}

	void mAnalysisFieldPanel::slot_browseTextFile()
	{
		QString path = QFileDialog::getOpenFileName(this, QStringLiteral("选择数据文件"),
			QString(), QStringLiteral("文本文件 (*.txt)"));
		if (!path.isEmpty())
			_filePathEdit->setText(path);
	}

	void mAnalysisFieldPanel::slot_importTextFileToInline()
	{
		const QString path = _filePathEdit->text().trimmed();
		if (path.isEmpty() || !QFile::exists(path))
		{
			mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("请先选择有效的文本文件"), true);
			return;
		}
		QString error;
		QList<AnalysisFieldRow> rows = parseAnalysisFieldTextFile(path,
			_startRowSpin->value(), _endRowSpin->value(), _startColumnSpin->value(),
			isPointCloud(), currentPickType(), _componentCountSpin->value(), error);
		if (!error.isEmpty())
		{
			mGlobalSignals::getInstance()->outputMessageSig(error, true);
			return;
		}
		if (rows.isEmpty())
		{
			mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("未识别到可导入的数据"), true);
			return;
		}
		//导入前整表校验（与手动编辑单元格的规则一致）
		const QString validateError = validateRowsForImport(rows);
		if (!validateError.isEmpty())
		{
			mGlobalSignals::getInstance()->outputMessageSig(validateError, true);
			return;
		}
		//移动语义：直接接管解析结果，避免 N 行 × 分量 的全量深拷贝
		if (_analysisFieldData)
			_analysisFieldData->setRows(std::move(rows));
		//定义方式置回"内联"（currentIndexChanged 信号会连带触发 syncVisibility，
		//离散/点云控件行随 sync{Discrete,PointCloud}ControlState 整体回灌，无旧数据残留）
		_methodCombo->setCurrentIndex(0);
		mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("文本文档已导入并转换为内联定义场"), true);
	}

	void mAnalysisFieldPanel::slot_bottomAction()
	{
		if (isTextFileMethod())
			slot_importTextFileToInline();
		else
			slot_mapPointCloudToTarget();
	}

	void mAnalysisFieldPanel::slot_mapPointCloudToTarget()
	{
		using mAnalysisField = MechDiscreteFieldData::mAnalysisField;

		if (!_analysisFieldData || !getCVM || !getCVM->getModelData())
			return;
		if (!isPointCloud())
			return;

		//目标对象可能很多，rowStorage() 返回的是 QList 引用；这里不整体拷贝，
		//避免数据量大时 N 行 × 分量 的深拷贝阻塞界面。后续构建点云 Set 时按行迭代。
		const QList<AnalysisFieldRow>& cloudRows = _analysisFieldData->rowStorage();
		if (cloudRows.isEmpty())
		{
			mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("点云数据为空，无法映射"), true);
			return;
		}
		mMeshData* meshData = getCVM->getModelData()->getMeshData();
		if (meshData == nullptr || meshData->_nodedata.nodeamount <= 0 || meshData->_nodedata.xyz == nullptr)
		{
			mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("当前模型无网格数据，无法映射"), true);
			return;
		}

		//先把面板映射空间/映射控制参数写回数据对象，保证读取的是界面最新值
		syncUiToField();

		const int compCount = _componentCountSpin->value();
		const MxFunctions::PickDataType pickType = currentPickType();


		//==================== 第一步：收集目标对象（目标集合/整个模型） ====================
		//目标集合在映射关系中维护（-1 即"整个模型"）
		auto& mapRel = getCVM->getModelData()->getMapRelationManager();
		const int targetSetID = mapRel.getMapRelation<int>({ Physical_Field, _analysisFieldID }, Physical_Set);
		MechSetData::mSetBase* targetSet = nullptr;
		if (targetSetID > 0)
		{
			targetSet = getCVM->getModelData()
				->getManager<MechSetData::mSetManager>(Physical_Set)
				.getSetData(targetSetID);
			if (targetSet == nullptr)
			{
				mGlobalSignals::getInstance()->outputMessageSig(
					QStringLiteral("目标集合（ID=%1）不存在，无法映射").arg(targetSetID), true);
				return;
			}
		}

		// 目标实体清单：result_id=节点/单元真实ID，result_id2=面序号（仅单元面）
		std::vector<RenderPickResultData> targets;
		if (targetSet != nullptr)
		{
			switch (pickType)
			{
			case MxFunctions::PickDataType::Node:
				if (auto* nodeSet = dynamic_cast<MechSetData::mNode*>(targetSet))
					targets = nodeSet->getNodeData();
				break;
			case MxFunctions::PickDataType::ElementFace:
				if (auto* surfaceSet = dynamic_cast<MechSetData::mElementSurface*>(targetSet))
					targets = surfaceSet->getElementSurfaceData();
				break;
			default:
				if (auto* eleSet = dynamic_cast<MechSetData::mElementSet*>(targetSet))
					targets = eleSet->getElementSetData();
				break;
			}
		}
		else
		{
			//整个模型：按位置类型取全部目标对象
			switch (pickType)
			{
			case MxFunctions::PickDataType::Node:
				for (int i = 0; i < meshData->_nodedata.nodeamount; ++i)
					targets.emplace_back(meshData->_nodedata.realids[i]);
				break;
			case MxFunctions::PickDataType::ElementFace:
				for (int partId : meshData->getAllPartIds())
				{
					mPartData* part = meshData->getPartData(partId);
					if (part == nullptr) continue;
					const auto& eleData = part->_parteledata;
					for (const auto& pair : eleData.elenodeposmap)
					{
						std::vector<RenderPickResultData> faces =
							meshData->getSurfaceByElementID(partId, pair.first);
						for (const auto& f : faces)
							targets.push_back(f);
					}
				}
				break;
			default:
				for (int partId : meshData->getAllPartIds())
				{
					mPartData* part = meshData->getPartData(partId);
					if (part == nullptr) continue;
					const auto& eleData = part->_parteledata;
					for (int i = 0; i < eleData.currentelenum; ++i)
						targets.emplace_back(eleData.realids[i], partId);
				}
				break;
			}
		}
		if (targets.empty())
		{
			mGlobalSignals::getInstance()->outputMessageSig(
				QStringLiteral("目标对象为空（目标集合无数据或模型无对应网格对象），无法映射"), true);
			return;
		}

		//==================== 第二步：收集目标对象节点，计算代表点坐标 ====================
		auto nodeCoord = [meshData](int nodeId, double& x, double& y, double& z) -> bool {
			auto it = meshData->_nodedata.nodeidmap.constFind(nodeId);
			if (it == meshData->_nodedata.nodeidmap.constEnd())
				return false;
			const int idx = it.value() * 3;
			x = meshData->_nodedata.xyz[idx];
			y = meshData->_nodedata.xyz[idx + 1];
			z = meshData->_nodedata.xyz[idx + 2];
			return true;
		};

		const ftc::TargetLocationKind targetKind =
			pickType == MxFunctions::PickDataType::Node ? ftc::TargetLocationKind::Node :
			pickType == MxFunctions::PickDataType::ElementFace ? ftc::TargetLocationKind::Face :
			ftc::TargetLocationKind::Cell;

		ftc::TargetMeshData targetMesh;
		std::vector<std::uint32_t> targetLocIds;
		std::vector<double> targetLocCoords;
		targetLocIds.reserve(targets.size());
		targetLocCoords.reserve(targets.size() * 3);
		//目标位置行索引 -> 目标实体（用于生成离散行；Face 目标位置复用形心节点 ID 需另行记录面序号）
		std::vector<RenderPickResultData> targetEntities;
		targetEntities.reserve(targets.size());
		std::unordered_set<std::uint32_t> usedLocIds;

		int skippedNoCoord = 0, skippedDup = 0, skippedOverflow = 0;
		for (const auto& t : targets)
		{
			std::vector<int> nodeIds;
			if (pickType == MxFunctions::PickDataType::Node)
			{
				nodeIds.push_back(t.result_id);
			}
			else if (pickType == MxFunctions::PickDataType::ElementFace)
			{
				QVector<int> faceNodes = meshData->getNodeIndexBySurface(t);
				nodeIds.assign(faceNodes.begin(), faceNodes.end());
			}
			else
			{
				mPartData* part = (t.part_id >= 0) ? meshData->getPartData(t.part_id) : nullptr;
				if (part == nullptr)
				{
					//集合数据未记录部件时按真实 ID 反查部件（跨部件遍历匹配）
					for (int partId : meshData->getAllPartIds())
					{
						mPartData* candidate = meshData->getPartData(partId);
						if (candidate != nullptr && candidate->getElementTypeByElement(t.result_id) >= 0)
						{
							part = candidate;
							break;
						}
					}
				}
				if (part == nullptr)
				{
					++skippedNoCoord;
					continue;
				}
				nodeIds = part->getNodesByElementId(t.result_id);
			}
			if (nodeIds.empty())
			{
				++skippedNoCoord;
				continue;
			}

			double cx = 0.0, cy = 0.0, cz = 0.0;
			int validNodes = 0;
			std::vector<std::uint32_t> topoNodes;
			topoNodes.reserve(nodeIds.size());
			for (int nodeId : nodeIds)
			{
				double x = 0.0, y = 0.0, z = 0.0;
				if (!nodeCoord(nodeId, x, y, z))
					continue;
				cx += x; cy += y; cz += z;
				++validNodes;
				topoNodes.push_back(static_cast<std::uint32_t>(nodeId));
				if (usedLocIds.insert(static_cast<std::uint32_t>(nodeId)).second)
				{
					targetMesh.node_ids.push_back(static_cast<std::uint32_t>(nodeId));
					targetMesh.node_coordinates.insert(targetMesh.node_coordinates.end(), { x, y, z });
				}
			}
			if (validNodes == 0)
			{
				++skippedNoCoord;
				continue;
			}
			cx /= validNodes; cy /= validNodes; cz /= validNodes;

			std::uint32_t locId = 0;
			if (targetKind == ftc::TargetLocationKind::Face)
			{
				//Face 目标位置 ID 需全局唯一：单元真实ID可能超出 uint32 或与节点区间重叠的
				//情形由 usedLocIds 去重保护；这里直接取单元真实ID
				if (t.result_id < 0)
				{
					++skippedOverflow;
					continue;
				}
				locId = static_cast<std::uint32_t>(t.result_id);
				if (!usedLocIds.insert(locId).second)
				{
					++skippedDup;
					continue;
				}
				ftc::MeshEntityTopology face;
				face.id = locId;
				face.type = topoNodes.size() == 3 ? ftc::EntityType::Tri3 : ftc::EntityType::Quad4;
				face.node_ids = topoNodes;
				targetMesh.faces.push_back(std::move(face));
			}
			else if (targetKind == ftc::TargetLocationKind::Cell)
			{
				if (t.result_id < 0)
				{
					++skippedOverflow;
					continue;
				}
				locId = static_cast<std::uint32_t>(t.result_id);
				if (!usedLocIds.insert(locId).second)
				{
					++skippedDup;
					continue;
				}
				//单元类型：按节点数近似（受 ftc 支持的 Tri3/Quad4/Tet4/Pyra5/Penta6/Hex8 约束）
				ftc::EntityType cellType;
				switch (topoNodes.size())
				{
				case 4: cellType = ftc::EntityType::Tet4; break;
				case 5: cellType = ftc::EntityType::Pyra5; break;
				case 6: cellType = ftc::EntityType::Penta6; break;
				case 8: cellType = ftc::EntityType::Hex8; break;
				default:
					++skippedNoCoord;
					continue;
				}
				ftc::MeshEntityTopology cell;
				cell.id = locId;
				cell.type = cellType;
				cell.node_ids = topoNodes;
				targetMesh.cells.push_back(std::move(cell));
			}
			else
			{
				locId = static_cast<std::uint32_t>(t.result_id);
			}

			targetLocIds.push_back(locId);
			targetLocCoords.insert(targetLocCoords.end(), { cx, cy, cz });
			targetEntities.push_back(t);
		}
		if (targetLocIds.empty())
		{
			mGlobalSignals::getInstance()->outputMessageSig(
				QStringLiteral("目标对象均无有效节点坐标，无法映射"), true);
			return;
		}

		//==================== 第三步：点云 -> 源 Set（应用缩放 + 坐标系变换） ====================
		//缩放系数（保持原始尺寸时全部为 1）
		double sx = 1.0, sy = 1.0, sz = 1.0;
		const mAnalysisField::ScaleMode scaleMode =
			static_cast<mAnalysisField::ScaleMode>(_scaleModeCombo->currentIndex());
		if (scaleMode == mAnalysisField::ScaleMode::Uniform)
			sx = sy = sz = _analysisFieldData->getUniformScale();
		else if (scaleMode == mAnalysisField::ScaleMode::NonUniform)
		{
			sx = _analysisFieldData->getScaleX();
			sy = _analysisFieldData->getScaleY();
			sz = _analysisFieldData->getScaleZ();
		}

		//坐标系（-1 即全局坐标系）：点云坐标定义在该坐标系下，映射前转到全局笛卡尔坐标
		const int coorID = mapRel.getMapRelation<int>({ Physical_Field, _analysisFieldID }, Physical_Coordinate);
		MechCoordinateData::mCoordinateBase* coordinate = nullptr;
		QVector3D coorOrigin, coorAxisX, coorAxisY, coorAxisZ;
		MxFunctions::CoordinateType coorType = MxFunctions::Cartesian;
		if (coorID > 0)
		{
			coordinate = getCVM->getModelData()
				->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate)
				.getCoordinateData(coorID);
			if (coordinate != nullptr)
			{
				bool axisOk = false;
				QString axisMsg;
				const QVector<QVector3D> axes = coordinate->getXYZAxis(axisOk, axisMsg, coorOrigin);
				if (!axisOk || axes.size() < 3)
				{
					mGlobalSignals::getInstance()->outputMessageSig(
						QStringLiteral("坐标系 %1 轴向计算失败：%2").arg(coorID).arg(axisMsg), true);
					return;
				}
				coorAxisX = axes[0];
				coorAxisY = axes[1];
				coorAxisZ = axes[2];
				coorType = coordinate->getCoordinateType();
			}
		}

		ftc::MpifDocumentData docData;
		docData.version = ftc::MpifVersion{};
		docData.solution_time = std::nullopt;
		ftc::SetData setData;
		setData.id = 1;
		setData.name = "PointCloud";
		setData.nodes.variables.push_back({ "Field", static_cast<std::uint16_t>(compCount) });
		setData.nodes.records.reserve(cloudRows.size());

		int skippedBadRow = 0;
		std::uint32_t cloudId = 0;
		for (const auto& row : cloudRows)
		{
			bool okX = false, okY = false, okZ = false;
			double px = row.x.toDouble(&okX);
			double py = row.y.toDouble(&okY);
			double pz = row.z.toDouble(&okZ);
			if (!okX || !okY || !okZ)
			{
				++skippedBadRow;
				continue;
			}
			px *= sx; py *= sy; pz *= sz;
			if (coordinate != nullptr)
			{
				//局部坐标 -> 全局坐标：柱/球坐标先转局部笛卡尔，再经坐标系轴向量旋转到全局
				double lx = px, ly = py, lz = pz;
				if (coorType == MxFunctions::Cylindrical)
				{
					lx = px * std::cos(py);
					ly = px * std::sin(py);
					lz = pz;
				}
				else if (coorType == MxFunctions::Spherical)
				{
					lx = px * std::sin(pz) * std::cos(py);
					ly = px * std::sin(pz) * std::sin(py);
					lz = px * std::cos(pz);
				}
				px = coorOrigin.x() + coorAxisX.x() * lx + coorAxisY.x() * ly + coorAxisZ.x() * lz;
				py = coorOrigin.y() + coorAxisX.y() * lx + coorAxisY.y() * ly + coorAxisZ.y() * lz;
				pz = coorOrigin.z() + coorAxisX.z() * lx + coorAxisY.z() * ly + coorAxisZ.z() * lz;
			}

			ftc::NodeRecord rec;
			rec.id = ++cloudId;
			rec.coordinates = { px, py, pz };
			rec.values.reserve(compCount);
			for (int c = 0; c < compCount; ++c)
			{
				bool okV = false;
				const double v = (c < row.values.size())
					? row.values.at(c).toDouble(&okV) : 0.0;
				rec.values.push_back(okV ? v : 0.0);
			}
			setData.nodes.records.push_back(std::move(rec));
		}
		if (setData.nodes.records.empty())
		{
			mGlobalSignals::getInstance()->outputMessageSig(
				QStringLiteral("点云数据无有效坐标行，无法映射"), true);
			return;
		}

		docData.sets.push_back(std::move(setData));
		auto document = ftc::MpifDocument::create(std::move(docData));
		if (!document)
		{
			mGlobalSignals::getInstance()->outputMessageSig(
				QStringLiteral("点云源数据构建失败"), true);
			return;
		}
		const ftc::MpifDocument& doc = *document.value_if();
		const auto setViewOpt = doc.find_set_by_id(1);
		if (!setViewOpt.has_value())
		{
			mGlobalSignals::getInstance()->outputMessageSig(
				QStringLiteral("点云源 Set 获取失败"), true);
			return;
		}
		const ftc::SetView& setView = *setViewOpt;

		//==================== 第四步：执行映射（参考 runMpifMappingDemo） ====================
		ftc::TargetLocationSetData targetData;
		targetData.kind = targetKind;
		targetData.location_ids = std::move(targetLocIds);
		targetData.coordinates = std::move(targetLocCoords);
		targetData.topology = std::move(targetMesh);
		auto targetSetObj = ftc::TargetLocationSet::create(std::move(targetData));
		if (!targetSetObj)
		{
			mGlobalSignals::getInstance()->outputMessageSig(
				QStringLiteral("目标对象位置集合创建失败"), true);
			return;
		}

		ftm::MappingOptions options;
		options.source_kind = ftc::EntityKind::Node;
		//表面映射 -> 最近邻；体映射 -> 单元插值（需目标单元拓扑支持，失败时回报错误）
		options.method = _mappingAlgorithmCombo->currentIndex() == 0
			? ftm::MappingMethod::Nearest : ftm::MappingMethod::ElementInterpolation;
		options.outside_domain_policy = ftm::OutsideDomainPolicy::Nearest;
		//容差：相对容差按面板映射控制参数；绝对容差保持默认极小值
		options.relative_tolerance = _analysisFieldData->getNeighborhoodTolerance();
		options.absolute_tolerance = _analysisFieldData->getInterpolationTolerance();

		auto plan = ftm::MappingPlan::prepare(setView, targetSetObj.value_if()->view(), options);
		if (!plan)
		{
			mGlobalSignals::getInstance()->outputMessageSig(
				QStringLiteral("映射计划准备失败（目标对象拓扑可能不支持所选映射算法）"), true);
			return;
		}
		ftm::FieldBinding binding;
		binding.source_name = "Field";
		binding.target_name = "Field";
		auto output = plan.value_if()->apply_field(setView, binding);
		if (!output)
		{
			mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("映射执行失败"), true);
			return;
		}
		const ftm::MappingOutput& out = *output.value_if();
		const auto fieldResult = out.fields().find_field("Field");
		if (!fieldResult)
		{
			mGlobalSignals::getInstance()->outputMessageSig(
				QStringLiteral("映射结果中未找到场数据"), true);
			return;
		}
		const auto& field = *fieldResult.value_if();
		const auto values = field.values();
		const auto status = out.report().location_status();

		//==================== 第五步：生成对象离散数据并切换数据类型 ====================
		//默认场值：域外目标对象使用默认值（空则置 0）；多分量按逗号/空白拆分
		QStringList defaultValues;
		{
			const QString defText = _defaultValueEdit->text().trimmed();
			QStringList parts = defText.split(QRegExp("[,，\\s]+"), QString::SkipEmptyParts);
			for (int c = 0; c < compCount; ++c)
				defaultValues.append(c < parts.size() ? parts.at(c) : QString());
		}

		QList<AnalysisFieldRow> mappedRows;
		mappedRows.reserve((int)targetEntities.size());
		int outsideCount = 0;
		for (std::size_t i = 0; i < targetEntities.size(); ++i)
		{
			AnalysisFieldRow row;
			const bool isOutside = (i < status.size())
				&& status[i] == ftm::LocationMappingStatus::OutsideDomain;
			if (isOutside)
				++outsideCount;
			if (pickType == MxFunctions::PickDataType::ElementFace)
			{
				row.elementId = QString::number(targetEntities[i].result_id);
				row.faceId = QString::number(targetEntities[i].result_id2);
			}
			else
			{
				row.id = QString::number(targetEntities[i].result_id);
			}
			row.values.reserve(compCount);
			for (int c = 0; c < compCount; ++c)
			{
				if (isOutside)
					row.values.append(defaultValues.at(c));
				else
					row.values.append(QString::number(values[i * compCount + c]));
			}
			mappedRows.append(std::move(row));
		}

		//点云行数据将被映射结果整体覆盖，先快照备份（QList 隐式共享，备份代价为引用计数）；
		//用户在离散界面切回"点云"类型时由 onDataTypeChanged() 从快照恢复 X/Y/Z
		_pointCloudBackup = cloudRows;

		//移动语义：直接接管映射结果，避免 N 行 × 分量 的全量深拷贝
		_analysisFieldData->setRows(std::move(mappedRows));
		//切换数据类型为对象离散（currentIndexChanged 连带触发 onDataTypeChanged -> syncVisibility，
		//离散控件随之回灌映射结果行并刷新拾取对象描述，点云弹窗随页面切换关闭，映射完成后视口立即可见）
		_dataTypeCombo->setCurrentIndex(1);
		if (getCVM)
			getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Field, _analysisFieldID);

		QString msg = QStringLiteral("映射完成：%1 个点云点 -> %2 个目标对象（%3离散）")
			.arg(setView.nodes().records.size())
			.arg(mappedRows.size())
			.arg(_locateCombo->currentText());
		if (outsideCount > 0)
			msg += QStringLiteral("，其中 %1 个目标对象位于点云域外，已按默认场值填充").arg(outsideCount);
		if (skippedBadRow > 0 || skippedNoCoord > 0 || skippedDup > 0 || skippedOverflow > 0)
			msg += QStringLiteral("；跳过：无效点云行 %1，无坐标目标 %2，重复目标 %3，ID 溢出 %4")
				.arg(skippedBadRow).arg(skippedNoCoord).arg(skippedDup).arg(skippedOverflow);
		mGlobalSignals::getInstance()->outputMessageSig(msg, true);
	}

	void mAnalysisFieldPanel::slot_renderPointCloud()
	{
		//点云渲染开关：点位置 + 场值经 LoadParticleCloudRender 粒子云渲染（仅点云数据类型）
		_cloudOn = !_cloudOn;

		auto* rendWidget = getCVM ? getCVM->getPreRenderWidget() : nullptr;
		if (rendWidget == nullptr)
		{
			_cloudOn = false;
			return;
		}

		if (!_cloudOn)
		{
			clearCloudRender();
			mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("已关闭点云渲染"), true);
			return;
		}

		if (!_analysisFieldData)
		{
			_cloudOn = false;
			mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("当前无分析场数据，无法渲染"), true);
			return;
		}
		const auto& rows = _analysisFieldData->rowStorage();
		if (rows.isEmpty())
		{
			_cloudOn = false;
			mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("点云数据为空，无法渲染"), true);
			return;
		}
		auto lpcRender = rendWidget->getLoadParticleCloudRender();
        lpcRender->setLPCColorTable(rendWidget->getColorTableRenderPoint());
		if (lpcRender == nullptr)
		{
			_cloudOn = false;
			return;
		}

		//点云行内坐标 + 第 0 分量场值（粒子云为标量通道）
		std::vector<std::pair<QVector3D, float>> posValue;
		posValue.reserve(rows.size());
		int skipped = 0;
		for (const auto& row : rows)
		{
			bool okX = false, okY = false, okZ = false, okV = false;
			const double x = row.x.toDouble(&okX);
			const double y = row.y.toDouble(&okY);
			const double z = row.z.toDouble(&okZ);
			const double v = row.values.isEmpty() ? 0.0 : row.values.first().toDouble(&okV);
			if (!okX || !okY || !okZ || !okV)
			{
				++skipped;
				continue;
			}
			posValue.emplace_back(QVector3D(x, y, z), static_cast<float>(v));
		}
		if (posValue.empty())
		{
			_cloudOn = false;
			mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("无有效点云数据行，无法渲染"), true);
			return;
		}
		std::vector<float> pointSizes(posValue.size(), 6.0f);
		lpcRender->setLPCColorTable(rendWidget->getColorTableRender());
		lpcRender->createNodeLoadCloud(posValue, pointSizes);
		rendWidget->updatePaint();
        rendWidget->update();
		mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("点云渲染完成：%1 个点%2")
			.arg(posValue.size())
			.arg(skipped > 0 ? QStringLiteral("，跳过无效行 %1").arg(skipped) : QString()), true);
	}

	void mAnalysisFieldPanel::slot_renderFieldCloudChart()
	{
		//场值云图开关（离散数据类型）：
		//节点走 LoadParticleCloudRender 粒子云（位置+场值）；
		//单元/单元面走 enableMeshEleLoadCloudChart 云图通道（参考压力/温度载荷面板）。
		_cloudOn = !_cloudOn;

		auto* rendWidget = getCVM ? getCVM->getPreRenderWidget() : nullptr;
		if (rendWidget == nullptr)
		{
			_cloudOn = false;
			return;
		}

		if (!_cloudOn)
		{
			clearCloudRender();
			mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("已关闭云图渲染"), true);
			return;
		}

		if (!_analysisFieldData)
		{
			_cloudOn = false;
			mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("当前无分析场数据，无法渲染"), true);
			return;
		}
		mMeshData* meshData = getCVM->getModelData() ? getCVM->getModelData()->getMeshData() : nullptr;
		if (meshData == nullptr)
		{
			_cloudOn = false;
			mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("当前模型无网格数据，无法渲染"), true);
			return;
		}

		const MxFunctions::PickDataType pickType = currentPickType();
		const auto& rows = _analysisFieldData->rowStorage();
		if (rows.isEmpty())
		{
			_cloudOn = false;
			mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("场数据为空，无法渲染"), true);
			return;
		}

		//场值取第 0 分量渲染（云图为标量通道）
		auto rowValue = [](const AnalysisFieldRow& row, bool& ok) -> double {
			ok = false;
			if (row.values.isEmpty())
				return 0.0;
			return row.values.first().toDouble(&ok);
		};

		if (pickType == MxFunctions::PickDataType::Node)
		{
			//节点离散：走节点云图通道（参考温度载荷面板）。
			//渲染侧节点通道按 {partID, 节点连续ID} 索引：一个节点可能属于多个部件，需逐部件传入
			if (rendWidget->getPreRender() == nullptr)
			{
				_cloudOn = false;
				return;
			}
			std::vector<std::pair<double, std::pair<int, int>>> meshNodeIDs;
			meshNodeIDs.reserve(rows.size());
			int skipped = 0;
			for (const auto& row : rows)
			{
				bool okId = false, okV = false;
				const int nodeId = row.id.toInt(&okId);
				const double v = rowValue(row, okV);
				if (!okId || !okV)
				{
					++skipped;
					continue;
				}
				const int contId = meshData->_nodedata.nodeidmap.value(nodeId, -1);
				if (contId < 0)
				{
					++skipped;
					continue;
				}
				for (int partId : meshData->getPartIdsByNodeid(nodeId))
					meshNodeIDs.emplace_back(v, std::make_pair(partId, contId));
			}
			if (meshNodeIDs.empty())
			{
				_cloudOn = false;
				mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("无有效场数据行，无法渲染"), true);
				return;
			}
			rendWidget->getPreRender()->enableMeshEleLoadCloudChart(true, {}, meshNodeIDs);
			//云图开启时拾取选中态/高亮已在渲染入口统一清空（渲染回调内 clearPickingSelection）
			rendWidget->updatePaint();
			mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("云图渲染完成：%1 个节点%2")
				.arg(rows.size() - skipped)
				.arg(skipped > 0 ? QStringLiteral("，跳过无效行 %1").arg(skipped) : QString()), true);
			return;
		}

		//单元/单元面离散：走单元云图通道（参考压力/温度载荷面板）
		if (rendWidget->getPreRender() == nullptr)
		{
			_cloudOn = false;
			return;
		}
		std::vector<std::pair<double, RenderPickResultData>> meshIDs;
		meshIDs.reserve(rows.size());
		int skipped = 0;
		for (const auto& row : rows)
		{
			bool okV = false;
			const double v = rowValue(row, okV);
			if (!okV)
			{
				++skipped;
				continue;
			}
			bool okId = false;
			if (pickType == MxFunctions::PickDataType::ElementFace)
			{
				RenderPickResultData d;
				d.result_id = row.elementId.toInt(&okId);
				if (!okId)
				{
					++skipped;
					continue;
				}
				d.result_id2 = row.faceId.toInt(&okId);
				if (!okId)
				{
					++skipped;
					continue;
				}
				//按单元真实 ID 反查部件（渲染侧按 part_id 索引）
				for (int partId : meshData->getAllPartIds())
				{
					mPartData* part = meshData->getPartData(partId);
					if (part != nullptr && part->getElementTypeByElement(d.result_id) >= 0)
					{
						d.part_id = partId;
						break;
					}
				}
				meshIDs.emplace_back(v, d);
			}
			else
			{
				RenderPickResultData d;
				d.result_id = row.id.toInt(&okId);
				if (!okId)
				{
					++skipped;
					continue;
				}
				for (int partId : meshData->getAllPartIds())
				{
					mPartData* part = meshData->getPartData(partId);
					if (part != nullptr && part->getElementTypeByElement(d.result_id) >= 0)
					{
						d.part_id = partId;
						break;
					}
				}
				meshIDs.emplace_back(v, d);
			}
		}
		if (meshIDs.empty())
		{
			_cloudOn = false;
			mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("无有效场数据行，无法渲染"), true);
			return;
		}
		rendWidget->getPreRender()->enableMeshEleLoadCloudChart(true, mxr::pickRefsOf(meshIDs));
		//云图开启时拾取选中态/高亮已在渲染入口统一清空（渲染回调内 clearPickingSelection）
		rendWidget->updatePaint();
		mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("云图渲染完成：%1 个对象%2")
			.arg(meshIDs.size())
			.arg(skipped > 0 ? QStringLiteral("，跳过无效行 %1").arg(skipped) : QString()), true);
	}

	void mAnalysisFieldPanel::clearCloudRender()
	{
		auto* rendWidget = getCVM ? getCVM->getPreRenderWidget() : nullptr;
		const bool wasCloudOn = _cloudOn;
		if (rendWidget != nullptr)
		{
			if (auto lpcRender = rendWidget->getLoadParticleCloudRender())
				lpcRender->deleteNodeLoadCloud();
			//单元云图通道按接口约定传空数据清空（与压力/温度面板一致）
			if (rendWidget->getPreRender())
				rendWidget->getPreRender()->enableMeshEleLoadCloudChart(false, {});
			rendWidget->updatePaint();
		}
		_cloudOn = false;
		//开云图期间拾取选中态被清空：关云图后离散模式按当前行回种选中态高亮
		//（行数据不变，仅刷新视口；随后若有清行/复位流程，回种会被再次清掉）
		if (wasCloudOn && _discreteControl && !isPointCloud() && !isTextFileMethod())
			_discreteControl->seedSelectionFromRows();
	}
}
