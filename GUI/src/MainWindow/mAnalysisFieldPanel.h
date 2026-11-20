#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <QWidget>

#include "mPanelBase.h"
#include "mCalcLineEdit.h"
#include "mPhysicalFieldEnum.h"
#include "mBasicEnum.h"
#include "mAnalysisField.h"
#include "mDiscreteDataControl.h"
#include "mPointCloudDataControl.h"

using namespace MxFunctions;

class QVBoxLayout;
class QComboBox;
class QSpinBox;
class QLineEdit;
class QToolButton;
class QPushButton;
class QLabel;
class QTimer;

namespace MechDiscreteFieldData
{
	class mAnalysisField;
}

namespace MechMainWindow
{
	//分析场参数面板
	class MMAINWINDOW_EXPORT mAnalysisFieldPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		mAnalysisFieldPanel(QWidget* parent = Q_NULLPTR);

		~mAnalysisFieldPanel();

		//新建分析场面板
		void newAnalysisFieldPanel();

		//编辑分析场
		void editAnalysisFieldPanel(int fieldID = 0);

		//取消编辑分析场面板（面板数据写回数据对象，拾取控件失活）
		void uneditAnalysisFieldPanel();

		/*
		* 场数据被外部销毁（树上删除场节点等）时的安全收尾：
		* 若被删的正是当前编辑场，丢弃悬垂指针、安全关闭数据表对话框（解绑+异步销毁）。
		* 必须在 deleteByPhysicalType 释放数据对象之前调用；传 -1 表示即将删除全部场。
		* 之后按既有约定走 uneditAnalysisFieldPanel() 复位界面。
		*/
		void onFieldDataAboutToBeDestroyed(int fieldID);

		/*
		* 把当前面板控件值写回数据对象（行数据已就地存储，无需同步）。
		* 用于写文件前提交编辑中未落盘的数据——LineEdit 内容只在退出编辑时才回写，
		* 编辑中直接保存会导致默认场值等字段丢失。
		*/
		void commitEditingData();

	public slots:
		/*
		* 点云映射到目标对象：读取映射空间参数(坐标系/缩放/目标集合)+映射控制参数(容差/算法)+点云数据，
		* 执行映射生成对象离散数据并切换数据类型。
		*/
		void slot_mapPointCloudToTarget();

	/*
	* 渲染点云开关（点云数据弹窗内"渲染云图"按钮触发，仅点云数据类型）。
	* 点位置 + 场值经 LoadParticleCloudRender 粒子云渲染。
	*/
	void slot_renderPointCloud();

	/*
	* 渲染场值云图开关（离散数据弹窗内"渲染云图"按钮触发，离散数据类型）。
	* 节点离散经 enableMeshEleLoadCloudChart 节点通道（参考温度载荷面板）；
	* 单元/单元面离散经 enableMeshEleLoadCloudChart 单元通道（参考压力载荷面板）。
	*/
	void slot_renderFieldCloudChart();

	private slots:
		//定义方式/数据类型/位置/缩放方式变化时统一刷新显隐状态机
		void syncVisibility();

		//位置切换统一处理：离散选项标签、目标集合列表、拾取类型、数据清理、表格列头刷新
		void onLocationChanged();

		//数据类型切换：同步拾取数据到数据对象并刷新入口按钮
		void onDataTypeChanged();

		//分量数量变化：联动表格模型列
		void onComponentCountChanged(int count);

		//坐标系下拉框填充（第一项恒为"全局坐标系"）
		void refreshCoordinateCombo();

		//目标集合下拉框填充（第一项恒为"整个模型"）
		void refreshTargetRegionCombo();

	//底部动作按钮
	void slot_bottomAction();

	//浏览文本文档
	void slot_browseTextFile();

		//文本文档导入转换为内联定义场
		void slot_importTextFileToInline();

	private:
		void initUI();

		//延迟恢复面板垂直 Maximum 策略（setMainWidget 换入时会重置为 Expanding），
		//使面板按内容高度布局，多余空间交给 Info Dock 底部 stretch spacer 吸收
		void scheduleHeightPolicy();

		//把控件值同步到数据对象
		void syncUiToField();

		//把数据对象值反显到控件
		void syncFieldToUi();

		/*
		* 离散模式（内联 + 离散数据类型）状态联动：吸附式弹窗控件随"位置"
		* 刷新标题/拾取对象描述（节点/单元/单元面，含视口拾取过滤器）、
		* 随"分量数量"刷新分量列，并把数据对象行（唯一数据源）回灌控件。
		* 拾取会话进行中不回灌行数据（容器即会话状态，由视口交互驱动）。
		*/
		void syncDiscreteControlState();

		//离散模式行数据整表写回数据对象（控件为该模式的编辑面），并联动树状态
		void syncDiscreteControlToField();

		//弹窗内"导入"：解析 + 整表校验 + 写回（与文本文档转内联同一套规则）
		void importDiscreteTextFile(const mDiscreteDataControl::ImportParams& params);

		//弹窗内"导出"：控件当前行导出文本文件
		void exportDiscreteTextFile(const QString& filePath);

		/*
		* 点云模式（内联 + 点云数据类型）状态联动：弹窗控件随"分量数量"刷新
		* X/Y/Z + 分量列，并把数据对象行（唯一数据源）回灌控件
		* （行结构：X/Y/Z 占控件前 3 列，分量随分量数量补齐/截断）。
		*/
		void syncPointCloudControlState();

		//点云模式行数据整表写回数据对象（弹窗控件为该模式的编辑面），并联动树状态
		void syncPointCloudControlToField();

		//点云弹窗内"导入"：解析 + 整表校验 + 写回（点云最少 3 列 X,Y,Z，分量…）
		void importPointCloudTextFile(const mPointCloudDataControl::ImportParams& params);

		//点云弹窗内"导出"：控件当前行导出文本文件（表头 X,Y,Z,Value1,…）
		void exportPointCloudTextFile(const QString& filePath);

		//行数据被（拾取/表格）就地修改后的联动收尾：树状态
		void notifyRowsEdited();

		//当前位置对应的集合类型：节点→Node，单元→Element，单元面→ElementSurface
		MxFunctions::SetType currentSetType() const;

		//当前位置对应的拾取类型：节点→Node，单元→Element，单元面→ElementFace
		MxFunctions::PickDataType currentPickType() const;

		//当前是否为点云数据类型
		bool isPointCloud() const;

		//当前是否为文本文档定义方式
		bool isTextFileMethod() const;

		/*
		* 场数据/映射空间分组内条件显隐行的集中管理：
		* 切换定义方式/数据类型/缩放方式时把控件从布局移除（或重新插入原位），
		* 避免 setVisible 隐藏后 QFormLayout 残留行空隙。
		*/
		void rebuildFieldDataRows();
		void rebuildMapSpaceRows();

		//整表行数据校验（导入等批量写入前调用）；返回空串合法，否则返回首条错误提示
		QString validateRowsForImport(const QList<MechDiscreteFieldData::AnalysisFieldRow>& rows) const;

		//关闭全部云图渲染（粒子云 + 单元云图通道），并恢复拾取高亮。
		//切换数据类型/位置、退出编辑、删除数据对象等场景统一收尾。
		void clearCloudRender();

	private:
		MechDiscreteFieldData::mAnalysisField* _analysisFieldData = nullptr;
		int _analysisFieldID = -1;

		//定义分组
		FoldSection* _defineFold = nullptr;
		QComboBox* _locateCombo = nullptr;
		QSpinBox* _componentCountSpin = nullptr;

		//场数据分组
		FoldSection* _dataFold = nullptr;
		PanelLabel* _defineMethodLabel = nullptr;
		PanelLabel* _dataTypeLabel = nullptr;
		QComboBox* _methodCombo = nullptr;
		QWidget* _fileRowWidget = nullptr;
		QLineEdit* _filePathEdit = nullptr;
		QSpinBox* _startRowSpin = nullptr;
		QSpinBox* _endRowSpin = nullptr;
		QSpinBox* _startColumnSpin = nullptr;
		QLabel* _startRowLabel = nullptr;
		QLabel* _endRowLabel = nullptr;
		QLabel* _startColumnLabel = nullptr;
		QLabel* _filePathLabel = nullptr;
		QComboBox* _dataTypeCombo = nullptr;
		//离散数据吸附式弹窗控件（仅内联+离散显示）：弹窗内表格直接驱动拾取与编辑，
		//取代原"通用拾取控件 + 数据表对话框"组合，形态与集中力/压力等载荷面板一致
		mDiscreteDataControl* _discreteControl = nullptr;
		//从控件剖分出、嵌入表单行的启动按钮（点击经控件转发弹窗开关，样式与载荷面板同步）
		QPushButton* _discreteEntryBtn = nullptr;
		//点云数据吸附式弹窗控件（仅内联+点云显示，2.2.6 D2 形态）：X/Y/Z 固定坐标列 +
		//分量可变列、无拾取入口；弹窗内表格承担编辑，取代原数据表对话框
		mPointCloudDataControl* _pointCloudControl = nullptr;
		//从控件剖分出、嵌入表单行的启动按钮（与离散数据入口同一形态）
		QPushButton* _pointCloudEntryBtn = nullptr;
		//数据入口行标签（点云"点云数据"，离散"XX离散数据"）
		PanelLabel* _dataEntryLabel = nullptr;
		QLineEdit* _defaultValueEdit = nullptr;
		QLabel* _defaultValueLabel = nullptr;

	//映射空间定义分组
	FoldSection* _mapSpaceFold = nullptr;
	QComboBox* _coordinateCombo = nullptr;
	QComboBox* _scaleModeCombo = nullptr;
	//数值输入统一用 mCalcLineEdit：限制字符类型、支持科学计数法与表达式（焦点离开时求值）
	mCalcLineEdit* _uniformScaleEdit = nullptr;
	QLabel* _uniformScaleLabel = nullptr;
	mCalcLineEdit* _scaleXEdit = nullptr;
	mCalcLineEdit* _scaleYEdit = nullptr;
	mCalcLineEdit* _scaleZEdit = nullptr;
		QLabel* _scaleXLabel = nullptr;
		QLabel* _scaleYLabel = nullptr;
		QLabel* _scaleZLabel = nullptr;
		QComboBox* _targetRegionCombo = nullptr;

	//映射控制分组
	FoldSection* _mapControlFold = nullptr;
	QComboBox* _toleranceTypeCombo = nullptr;
	mCalcLineEdit* _positiveToleranceEdit = nullptr;
	mCalcLineEdit* _negativeToleranceEdit = nullptr;
	mCalcLineEdit* _neighborhoodToleranceEdit = nullptr;
	mCalcLineEdit* _interpolationToleranceEdit = nullptr;
	QComboBox* _mappingAlgorithmCombo = nullptr;

		//底部动作区
		QWidget* _bottomWidget = nullptr;
		QPushButton* _bottomActionBtn = nullptr;

		//云图渲染开关状态（弹窗"渲染云图"按钮为无状态触发，由面板记录当前开关）
		bool _cloudOn = false;

		//scheduleHeightPolicy() 本轮事件循环内是否已排期（合并重复触发）
		bool _policyScheduled = false;

		//点云行数据快照：点云行存储（数据对象 _rows）在映射后被映射结果（离散行）覆盖，
		//若用户在离散界面切回"点云"类型，从快照恢复点云数据，避免 X/Y/Z 列显示为空。
		//快照只在映射成功时刷新；离散侧的手动编辑/拾取不回写快照。
		QList<MechDiscreteFieldData::AnalysisFieldRow> _pointCloudBackup;

	protected:
		//换入显示时恢复按内容收缩的垂直策略（setMainWidget 换入会重置为 Expanding）
		void showEvent(QShowEvent* event) override;
	};
}
