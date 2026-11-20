#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QLineEdit>
#include <QVector>
#include <QVector3D>
#include <QMouseEvent>
#include <QPointer>
#include <QColor>
#include <QList>
#include <QHash>
#include "mxr/basic/PublicStruct.h"
#include "mPostMeasureData.h"

#include "mpostwindow_global.h"
#include "mPanelBase.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
namespace PostData { struct mMeasureLabelSettings; struct mMeasureRecord; class mPostDataManager; class mPostModelData; }
using namespace MechMainWindow;


namespace mxr
{
	class REBUILD_PostRend;
    enum class SelectionInfoType : uint8_t;
    enum class PickFilter : uint64_t;
    class CCSMRendWidget;
}

namespace PostData
{
	class mPostDataBase;
}

namespace PlotView
{
	class mPlotView;
}

class QComboBox;
class QPushButton;
class QVBoxLayout;
class QTableWidget;
class QStackedLayout;
class QLabel;
class QCheckBox;
class QRadioButton;
class QSpinBox;
class QTableWidgetItem;
class QHideEvent;

namespace MxFunctions
{
	class mPanelTable;
}

namespace PostWindow
{
	class FbdColorPicker;
	class mPostObjectEditController;

    /*
    * 测量工具面板（按"前后处理整合原型"重构）：
    * - 顶部全宽主操作按钮「测量数据管理及曲线生成」打开复核弹窗；
    * - 测量数据列表（管理折叠组）：类型筛选 + 名称搜索等分并排、复选/名称/类型/显隐(眼睛图标)/颜色
    *   列表，全选/显示/隐藏/删除，颜色格点击弹出 FbdColorPicker 取色；
    * - 测量工具（工具折叠组）：标签内容"自定义"弹窗、单个「测量数据」下拉（设计 11 项，
    *   默认"节点云图"）、单个全宽「选择节点」拾取按钮（按序收集节点）、
    *   自动创建/自动分组 + 创建按钮（测量结果写入测量数据列表）；
    * - 测量数据管理及曲线生成弹窗：测量记录检索列表 + X轴（时间/帧号）选择，
    *   绘制按钮把所选（或全部显示）记录按帧计算为曲线发送到曲线窗口。
    */
	class MPOSTWINDOW_EXPORT mPostMeasureTool : public mPanelBase
	{
		Q_OBJECT

	public:
		/** 构造函数 */
		mPostMeasureTool(QWidget *parent = Q_NULLPTR);

		/** 析构函数 */
		~mPostMeasureTool();

		/** 根据当前面板是否显示设置拾取开关及信号连接 */
		void setPanelPickAndSignal(bool isshow);

        /** 初始化界面 */
        void newMeasureToolPanel(int caseId);

        /** 树联动：定位到指定记录进编辑（清搜索/类型筛选保证目标行可见；镜像缺失时
		 *  从全局管理器兜底重载；内部走统一编辑契约——dirty 弹三选一确认，被"继续
		 *  编辑"拦截时编辑对象不变）。返回 false=记录不存在 */
        bool editMeasureRecord(int id);

        /** 当前编辑记录 id（-1 无编辑对象；树"测量"分支高亮跟随用） */
        int currentEditingMeasureId() const;

        /** 树联动：设置记录显隐（面板眼睛列同款链路：镜像写回+行渲染+3D 标注同步） */
        void setMeasureRecordVisible(int id, bool visible);

        /** 树联动：删除单条记录（编辑记录被删回落首条/退出编辑态，与勾选删除语义一致）。
		 *  返回 false=记录不存在 */
        bool deleteMeasureRecord(int id);

	private:

		//标签/记录结构定义已移至 PostData 层（PostData::mMeasureLabelSettings /
		//PostData::mMeasureRecord，随结果数据库缓存；面板持有镜像）
		using LabelSettings = PostData::mMeasureLabelSettings;
		using MeasureRecord = PostData::mMeasureRecord;

		/** 构建面板控件（语言文本由 slot_changeLanguage 设置） */
		void setupUI();

		/** 构建"标签内容"弹窗（Qt::Popup，懒创建） */
		void ensureLabelPopup();

		/** 构建"测量数据管理及曲线生成"弹窗（Qt::Popup，懒创建） */
		void ensureReviewPopup();

		/** 构建颜色拾取弹窗（懒创建，按取色目标分发：0记录颜色/1标签颜色/2填充颜色） */
		void ensureColorPopup();

		/** 弹窗贴锚点控件下方显示（越界收回屏内） */
		void popupUnder(QWidget* popup, QWidget* anchor);

		/** 弹窗在屏幕居中显示 */
		void popupCenter(QWidget* popup);

		/** 色块按钮换色（内联样式刷新底色） */
		void applySwatchColor(QPushButton* swatch, const QColor& color);

		/** 解析节点id的坐标（后处理按当前帧位移后位置，前处理按几何顶点） */
		bool nodeCoordinate(int nodeId, QVector3D& out);

		/** 获取拾取结果中的单个节点ID */
		int getPickResultID(mxr::SelectionInfoType selType);

        // 获取拾取结果的所有ID
        bool getPickResultIDs(mxr::SelectionInfoType selType, std::vector<int>& outAllIds);

		/** 当前测量数据类型的计算分支（0两点距离/1三点夹角/2三点圆半径/-1待接入） */
		int currentCalcKind() const;

		/** 当前计算分支所需节点数（待接入类型按 1 个计） */
		int requiredNodeCount() const;

		/** 测量数据显示文本（按语言，设计 11 项） */
		QString typeName(int typeIndex, int lang) const;

		/** 当前计算分支所需节点是否已拾取齐备（自动创建判断用，不弹提示） */
		bool inputsComplete() const;

		/** 按当前输入创建测量记录：计算结果并生成记录数据（失败弹提示，返回false表示未创建） */
		bool buildRecordFromInput(MeasureRecord& rec);

		/** 记录对象摘要文本（节点/单元/部件/面编号，按语言） */
		QString recordObjectText(const MeasureRecord& rec, int lang) const;

		/** 记录标签内容摘要文本（复核弹窗 tooltip 用，按语言） */
		QString recordLabelSummary(const MeasureRecord& rec, int lang) const;

		/** 记录主值（向量类分支按幅值分量组合取值，标量类取原始值；幅值"无"返 0） */
		double measurePrimaryValue(const MeasureRecord& rec) const;

		/** 记录主值文本（角度按设置换算弧度并带单位；向量类幅值"无"返回空） */
		QString measurePrimaryText(const MeasureRecord& rec, int lang) const;

		/** 记录结果全文（按记录标签设置现场格式化——结构修：不落定格文本） */
		QString formatMeasureText(const MeasureRecord& rec, int lang) const;

		/** 3D 标签文本（0.40 语义）：单节点测量（位置/节点云图）首行=拾取节点号
		 *  （恒显）可并名称（显示名称），位置其后按幅值分量逐行取值（显示前缀
		 *  开关决定带不带 "X = " 标注）；多节点测量标题=#记录序号（显示编号）+
		 *  测量项命名（显示名称）+主值（显示前缀开关决定带不带 "Mag = " 标注） */
		QString formatMeasureLabelLines(const MeasureRecord& rec, int lang) const;

		/** 展开/收起复核列表中某记录的详情行（对齐原型 ▶ 展开详情） */
		void toggleReviewDetail(int recordId);

		/** 在复核列表 row 处插入记录的详情行（18 项标签字段） */
		void insertReviewDetailRow(int row, const MeasureRecord& rec);

		/** 收起/展开绘图区（对齐原型 measurementPlotCollapse） */
		void slot_plotCollapseClicked();

		/** 面板隐藏（切到其他面板/折叠收起宿主）时退出拾取模式 */
		void hideEvent(QHideEvent* event) override;

		/** 标签弹窗色块按压过滤：消费按压打开取色器，避免标签弹窗被连带关闭丢失取色 */
		bool eventFilter(QObject* watched, QEvent* event) override;

		/** 退出拾取模式：取消全部拾取按钮并恢复 PickNothing */
		void exitPickMode();

		/** 自动创建：勾选自动创建且当前必填输入齐备时直接创建 */
		void maybeAutoCreate();

		/** 状态行刷新拾取进度（已选节点清单 + 还需数量/待接入提示） */
		void updatePickStatus();

		/** 状态行写入（空文案整行收起，对齐原型空态不留白） */
		void setStatusLabel(const QString& text);

		/** 记录 id 对应的列表数据下标（不存在返回 -1） */
		int dataIndexById(int id) const;

		/** 记录 id 不在面板镜像时从全局管理器兜底重载（树侧入口面板可能未打开过；
		 *  返回重载后记录是否可达） */
		bool ensureRecordLoaded(int id);

		/******** 统一编辑契约（V1.3：新建即入表待定义，保存完成同一记录） ********/
		/** 当前编辑记录指针（无返回 nullptr） */
		MeasureRecord* editingRecord();

		/** 当前编辑记录的可视行号（无返回 -1） */
		int editingRowIndex() const;

		/** 编辑缓冲签名（类型+拾取序；dirty 检测） */
		QString editingSignature() const;

		void beginCreateMeasure();
		void selectMeasureRecord(int id);
		void cancelEditMeasure();
		bool commitEditMeasure(bool immediate);
		void notifyMeasureEditChanged();

		/** 插入一条记录行（新建/完成共用；返回名称单元格） */
		QTableWidgetItem* insertRecordRow(const MeasureRecord& rec);

		/** 按行数据渲染管理列表各单元格 */
		void renderRow(int row);

		/** 管理列表空表提示与显隐同步 */
		void refreshEmptyState();

		/** 管理列表联动：过滤显行、已选计数与按钮启停、状态行文案 */
		void syncManagerState();

		/** 管理列表按当前筛选条件显隐行 */
		void applyManagerFilter();

		/** 勾选行整行高亮（对齐原型 tr:has(input:checked) 选中底色） */
		void applyCheckedRowBackground(int row);

		/** 勾选记录批量显隐（显示/隐藏按钮） */
		void setCheckedRecordsVisible(bool visible);

		/** 拾取按钮文案随当前类型拾取域联动（节点/单元/几何体/几何面；armed=进行中文案） */
		QString pickButtonLabel(bool armed, int lang) const;

		/** 测量记录按帧计算并绘制到详情弹窗右侧 PlotView（单条：仅选中记录；多条：全部显示记录） */
		void plotRecords(const QList<int>& recordIds);

		/** 当前绘制目标记录集合（单条曲线=选中行；多条=全部显示记录） */
		QList<int> currentPlotRecordIds() const;

		/** 3D 测量标注全量重建（可见记录的两点连线+结果文字；显隐/颜色/创建/删除/换帧后调用；仅后处理视口） */
		void updateMeasureAnnotations();

		/** 记录按指定帧重算原始结果（FR-04 随帧重算；rawValues 同款契约：向量类
		 *  {X,Y,Z}/标量类{值}/角度类{度}，plotRecords 同口径原语——真实位移，不含
		 *  变形系数/爆炸偏移）。curModel/curDis 传重建函数已取的当前帧数据（避免
		 *  逐记录 QHash 拷贝），firstModel/firstDis 传预取的首帧基准（delta 类
		 *  1/3/4/6 需要）；传 nullptr 时内部自取（tooltip/详情行单条拉模式路径）。
		 *  返回空向量=当前帧取值失败，调用方回退创建帧快照 */
		QVector<double> computeRawValuesForFrame(const MeasureRecord& rec, int frameId,
			PostData::mPostModelData* curModel, const QHash<int, QVector3D>* curDis,
			PostData::mPostModelData* firstModel, const QHash<int, QVector3D>* firstDis) const;

		/** 复核弹窗已展开详情行随帧重算刷新（FR-04：弹窗可见时随重建同步重算
		 *  "测量结果"字段；不可见时不刷，下次打开 renderReviewTable 现场格式化） */
		void refreshExpandedReviewDetails();

		/** 清空 3D 测量标注（记录清空后调用） */
		void clearMeasureAnnotations();

		/** 曲线弹窗空表提示与绘制按钮启停同步 */
		void syncReviewState();

		/** 曲线 Y 轴选项按绘制目标记录适用性重建：全为向量类分支给 X/Y/Z/合量，
		 *  含标量类型仅"结果"——不把标量结果当作分量（需求 V1.1 §5） */
		void refreshYAxisOptions();

		/** 重建曲线弹窗记录列表（按弹窗内筛选条件） */
		void renderReviewTable();

		/** 清空全部测量记录并复位计数 */
		void clearRecords();

		/** 测量记录缓存写回（面板镜像 → 结果数据库） */
		void syncMeasureCache();

		/** 从结果数据库加载测量记录缓存并重建列表行 */
		void loadMeasureCache();

		/** 测量历史管理器（全局会话级；面板/视口未就绪时返回空） */
		PostData::mPostDataManager* measureManager() const;

		/** [调试] ui-shot 数据态注入：取当前结果前两个节点伪造两点距离记录并勾选，
		 *  供 3D 标注/详情弹窗数据态的自动化验证（免拾取交互）。
		 *  经 mUiShotController invokeMethod 按名调用，必须 Q_INVOKABLE */
		Q_INVOKABLE void slot_debugInjectDistanceRecord();

		/** 弹窗文本中英文切换（弹窗懒创建，未创建时跳过） */
		void applyPopupsLanguage(int lang);

	private slots:

		/** 切换中英文 */
		void slot_changeLanguage(int language);

		/** 详情弹窗 X/Y 轴下拉切换：已绘制过则按新轴变量立即重绘（原型 D9） */
		void slot_reviewAxisChanged(int index);

		/** 选择「测量数据」下拉（切换即取消拾取并清空已选） */
		void slot_dataTypeChanged(int index);

		/** 「选择节点」拾取按钮：点击进入/退出拾取 */
		void slot_pickNodeClicked(bool checked = false);

		/** 接收拾取信号 */
		void slot_finishPick(mxr::FinishPickInfo info);

		/** 创建按钮：按当前输入计算并加入测量数据列表 */
		void slot_createButton();

		/** 搜索框过滤测量记录 */
		void slot_searchChanged(const QString& text);

		/** 类型筛选切换测量记录 */
		void slot_typeFilterChanged(int index);

		/** 全选按钮：全勾/全不勾可见记录 */
		void slot_selectAllClicked();

		/** 显示/隐藏按钮：勾选记录批量显隐 */
		void slot_showSelectedClicked();
		void slot_hideSelectedClicked();

		/** 删除按钮：移除勾选记录 */
		void slot_deleteButton();

		/** 列表复选变化：同步已选计数 */
		void slot_itemChanged(QTableWidgetItem* item);

		/** 颜色单元格点击：弹出取色弹窗 */
		void slot_showColorPopup(int row);

		/** 标签内容按钮：弹出/关闭标签内容弹窗（打开时载入草稿） */
		void slot_labelButtonClicked();

		/** 标签内容弹窗：标签颜色/填充颜色色块取色 */
		void slot_labelColorClicked();
		void slot_fillColorClicked();

		/** 标签内容弹窗：恢复默认参数（仅草稿，确认后生效） */
		void slot_labelReset();

		/** 标签内容弹窗：确认并保存标签设置 */
		void slot_labelConfirm();

		/** 测量数据管理及曲线生成按钮：弹出曲线弹窗（打开时重建列表） */
		void slot_showReviewPopup();

		/** 曲线弹窗：搜索/类型筛选刷新列表 */
		void slot_reviewSearchChanged(const QString& text);
		void slot_reviewTypeChanged(int index);

		/** 曲线弹窗：绘制按钮（单条曲线=仅所选记录，否则全部显示记录） */
		void slot_plotClicked();

		/** 云图/动画换帧全局通知（postCloudFrameChanged，GUI 全局单例信号、不带
		 *  视口名）：守卫 → 播放态门控（Q3 落帧口径：播放中不刷，停止/暂停/拖帧
		 *  落定即刷）→ 本视口帧号去重（同帧重应用/他视口换帧不重建）→
		 *  updateMeasureAnnotations 批量重建 */
		void slot_onCloudFrameChanged();

    private:

		QVBoxLayout*					_mainLayout{};		//主布局

		//--------- 测量数据列表 ---------
		FoldSection*					_manageFold{};		//管理折叠组
		QComboBox*						_typeFilterCombo{};	//类型筛选
		QLineEdit*						_searchEdit{};		//搜索名称
		MxFunctions::mPanelTable*				_table{};			//记录列表（复选/名称/类型/显隐/颜色；共享管理表格件，空态覆盖层内置）
		QPushButton*					_btnSelectAll{};	//全选
		QPushButton*					_btnShow{};			//显示
		QPushButton*					_btnHide{};			//隐藏
		QPushButton*					_btnDelete{};		//删除
		QLabel*							_managerStatusLabel{};	//管理状态行（共N条/已选M条）

		//--------- 测量工具 ---------
		FoldSection*					_toolFold{};		//工具折叠组
		PanelLabel*						_labelContent{};	//标签内容行标签
		QPushButton*					_btnLabel{};		//标签内容-自定义（条目按钮）
		QLabel*							_labelBtnText{};	//自定义文字（按钮内左对齐）
		PanelLabel*						_dataLabel{};		//测量数据标签
		QComboBox*						_comboBox{};		//测量数据（设计 11 项单下拉）
		PanelLabel*						_pickLabel{};		//选择节点标签
		QPushButton*					_btnPickNode{};		//选择节点（全宽拾取按钮，拾取中"正在拾取节点…"）

		QCheckBox*						_checkAutoCreate{};	//自动创建（拾取齐备即创建）
		QCheckBox*						_checkAutoGroup{};	//自动分组（原型置灰，待接入）
		QPushButton*					_btnCreate{};		//创建
		QLabel*							_statusLabel{};		//测量状态行（拾取中/最新结果）

		QPushButton*					_btnManager{};		//测量数据管理及曲线生成

		//--------- 测量记录 ---------
		QList<MeasureRecord>			_records;			//记录数据（行 UserRole 存 id 关联）
		int								_nextId{ 1 };		//记录 id 计数
		QHash<int, int>					_typeCounters;		//各类型已创建计数（名称 两点距离_N）
		LabelSettings					_labelSettings;		//当前标签内容设置
		QList<int>						_pickedIds;			//当前拾取收集的节点id（按拾取序）

		//--------- 统一编辑契约（V1.3） ---------
		mPostObjectEditController*		_editor{ nullptr };	//统一编辑契约控制器
		QPushButton*					_btnNew{ nullptr };	//新建（管理操作行，主蓝）
		QSet<int>						_incompleteIds;		//待定义记录（缺拾取引用）
		int								_editingId{ -1 };	//当前编辑记录 id（-1 无）
		bool							_loadingEdit{ false };	//记录装载期间屏蔽变更通知

		QMetaObject::Connection			_pickConn{};		//拾取信号连接
		QMetaObject::Connection			_frameConn{};		//帧变化 → 3D 测量标注跟随当前帧位置
		QMetaObject::Connection			_cameraConn{};		//相机变化 → 自动隐藏标签重评估
		QMetaObject::Connection			_deformConn{};		//变形系数变化 → 标注位置随渲染变形重算
		QMetaObject::Connection			_interpConn{};		//【审计 C-01】参数化插值档推进 → 标注随插值变形逐 tick 重算（unify-anim-playback：Parametric playTick 不发行域事件，frame/deform 两钩子均不可达）
		QMetaObject::Connection			_cloudFrameConn{};	//postCloudFrameChanged → 换帧标注跟随（GUI 全局单例信号；不随面板显隐断开——隐藏期间换帧标注仍跟随）
		int								_annotatedFrame{ -1 };	//上次重建时本视口 _cloudFrame（换帧去重缓存）
		QSet<QString>					_measureFontKeys{};	//已提交 3D 标注文字的记录 key（mMeasure_<id>）
		QHash<int, int>					_recordObjectIds;	//台账 id 平行表（unify-addobject-arch C3：recordId→MeasureAnnotation 台账 objectId；MeasureRecord=PostData 数据层结构不动）
		bool							_reviewPlotted{};	//详情弹窗已出过曲线（轴切换重绘的门控）

        QHash<QPushButton*, mxr::PickFilter>      _buttonPickFilters;             // 按钮对应的拾取类型


        bool                            _isCurrPostViewport;                        // 当前是不是后处理窗口
        bool                            _isCurrStateValid;                          // 当前是不是处于可用状态
        // 前处理时
        QPointer<mxr::CCSMRendWidget>   _preRend;           // 前处理视口(视口删除时随QObject析构自动置空)


        // 后处理时
        QPointer<mxr::REBUILD_PostRend> _postRend;		//暂存视口(同上)
        QPointer<PostData::mPostDataBase>         _postDB;

		//--------- 标签内容弹窗 ---------
		QWidget*						_labelPopup{};		//弹窗（Qt::Popup，点击外部自动关闭）
		QComboBox*						_lpFontCombo{};		//字号
		QPushButton*					_lpColorSwatch{};	//标签颜色色块
		QCheckBox*						_lpShowId{};		//显示编号
		QCheckBox*						_lpShowName{};		//显示名称
		QComboBox*						_lpMagnitudeCombo{};	//幅值
		QComboBox*						_lpFormatCombo{};	//数值格式（科学/小数）
		QSpinBox*						_lpPrecisionSpin{};	//数值精度
		QRadioButton*					_lpDegreeRadio{};	//角度-度数
		QRadioButton*					_lpRadianRadio{};	//角度-弧度
		QCheckBox*						_lpFillCheck{};		//填充
		QPushButton*					_lpFillSwatch{};	//填充颜色色块
		QCheckBox*						_lpAutoHideCheck{};	//自动隐藏
		QCheckBox*						_lpPrefixCheck{};	//显示前缀
		QPushButton*					_lpResetBtn{};		//恢复默认
		QPushButton*					_lpOkBtn{};			//确认
		LabelSettings					_labelDraft{};		//弹窗编辑中的标签草稿（确认后写回_labelSettings）

		//--------- 颜色拾取弹窗 ---------
		FbdColorPicker*					_colorPicker{};		//取色弹窗（可复用控件 mFbdColorPicker）
		int								_colorTarget{ -1 };	//取色目标：0记录颜色/1标签颜色/2填充颜色
		int								_colorRecordId{ -1 };	//正在取色的记录id

		//--------- 测量数据管理及曲线生成弹窗 ---------
		QWidget*						_reviewPopup{};		//弹窗（Qt::Popup，点击外部自动关闭）
		QComboBox*						_rvTypeCombo{};		//类型筛选
		QLineEdit*						_rvSearchEdit{};	//搜索名称
		MxFunctions::mPanelTable*				_rvTable{};			//记录列表（实体▶可展开/编号/颜色/类型/坐标系；共享管理表格件）
		QCheckBox*						_rvLiveLink{};		//实时关联（原型置灰，待接入）
		QCheckBox*						_rvSingleCurve{};	//单条曲线（仅绘制所选记录）
		QComboBox*						_rvTargetCombo{};	//绘制位置（预览图/曲线窗口）
		QComboBox*						_rvXCombo{};		//X轴（时间/帧号）
		QComboBox*						_rvYCombo{};		//Y轴（X/Y/Z/合量）
		QPushButton*					_rvPlotBtn{};		//绘制
		QPushButton*					_btnCollapsePlot{};	//收起绘图/展开绘图
		QWidget*						_plotSection{};		//绘图区（右栏，可收起）
		QWidget*						_plotPlaceholder{};	//绘图空态占位（暂无测量结果）
		PlotView::mPlotView*			_rvPlotView{};		//详情弹窗右侧绘图（PlotView，绘制位置=预览图）
		QSet<int>						_expandedReviewIds;	//复核列表已展开详情的记录 id
    };
}
