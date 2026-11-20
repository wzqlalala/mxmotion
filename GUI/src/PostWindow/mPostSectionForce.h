#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QPointer>
#include <QVector3D>

#include "mpostwindow_global.h"
#include "mPanelBase.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;

class QLineEdit;
class QTableWidget;
class QShowEvent;
class QHideEvent;
class QTableWidgetItem;
class QPushButton;
class QToolButton;
class QComboBox;
class QCheckBox;
class QLabel;
class QStackedLayout;
class QTextEdit;

namespace mxr
{
	class REBUILD_PostRend;
}

namespace MxFunctions
{
	class mChoiceGroup;
	class mPanelTable;
	class mPrimaryButton;
	class mComboActionRow;
	class mAxisCoordRow;
	class mAdvancedOptionsButton;
}

namespace PostData
{
	class mPostDataBase;
}

namespace PostWindow
{
	class FbdColorPicker;
	class mPostObjectEditController;
	class mPostPickWidgetController;

	/*
	* 截面力面板（V1.3 #fbdParameters：定义事务独立于计算/显示参数——
	* 编辑走缓冲（取消编辑/保存修改/即时保存统一操作区），缺必选单元的截面
	* 先入表标"待定义"；拾取/坐标系/求和点编辑缓冲经保存写回记录）。
	*/
	class MPOSTWINDOW_EXPORT mPostSectionForce : public mPanelBase
	{
		Q_OBJECT

	public:
		mPostSectionForce(QWidget* parent = Q_NULLPTR);
		~mPostSectionForce();

		/** 按当前视口初始化面板（仅后处理视口可用） */
		void newSectionForcePanel(int caseId);

		/** 面板显示/隐藏：拾取条对称收口（激活态解除+高亮清理/恢复） */
		void showEvent(QShowEvent* event) override;
		void hideEvent(QHideEvent* event) override;

	private:
		/** 截面数据（行渲染/编辑均以 id 关联到列表行） */
		struct SectionData
		{
			int		id = 0;
			QString	name;
			int		sumPoint = 0;		//0形心/1节点/2坐标
			QString	nodeName;			//sumPoint==1 的求和节点
			double	x = 0.0, y = 0.0, z = 0.0;	//sumPoint==2 的全局坐标
			QString	coordSys;
			QColor	color;
			QList<int>	elementIds;		//截面对象-单元编号
			QList<int>	nodeIds;		//截面对象-节点编号
			bool	incomplete = false;	//待定义（缺必选单元；V1.3）
		};

		/** 构建面板控件（语言文本由 slot_changeLanguage 设置） */
		void setupUI();

		/** 空表提示与列表显隐同步 */
		void refreshEmptyState();

		/** 选中联动：已选计数/按钮启停/编辑行名称高亮（V1.3：勾选仅批量，编辑走 _editingId） */
		void syncSelectionState();

		/** 当前唯一选中行号（无或选中多行返回 -1）——兼容保留（表格已无行选择态） */
		int singleSelectedRow() const;

		/** 当前编辑行的可视行号（按 _editingId 找；无返回 -1） */
		int editingRow() const;

		/******** 统一编辑契约（V1.3：定义事务缓冲） ********/
		/** id 对应的记录（无返回 nullptr） */
		SectionData* sectionRecord(int id);

		/** 缓冲装载到编辑区（sec=nullptr 置灰占位）；装载后调用方决定 beginEdit */
		void fillWorkingToUI(const SectionData* sec);

		/** 编辑缓冲规范化签名（dirty 检测） */
		static QString sectionSignature(const SectionData& d);

		/** 选中/新建/取消/保存（原型 select/beginCreate/cancelEdit/commit） */
		void selectSection(int id);
		void beginCreate();
		void cancelEdit();
		bool commitEdit(bool immediate);

		/** 编辑字段变更通知（dirty 重算/即时保存调度） */
		void notifyEditChanged();

		/** 按行数据渲染各单元格 */
		void renderRow(int row);

		/** 读回选中行属性到编辑区（V1.3 起由 fillWorkingToUI 承担；保留兼容壳） */
		void fillPropertyRows(int row);

		/** 求和点联动行显隐（节点/坐标行），并刷新折叠高度 */
		void updateSumPointExtraRows(int sumPoint);

		/** 新建默认截面颜色（按序循环取用） */
		QColor nextSectionColor() const;

		/** 找 id 对应的行内数据索引（不存在返回 -1） */
		int dataIndexById(int id) const;

		/** 当前编辑截面数据指针（无选中行返回 nullptr） */
		SectionData* activeSection();

		/** 构建拾取编号弹窗 */
		void ensurePickPopup();

		/** 构建显示设置弹窗 */
		void ensureDisplayPopup();

		/** 显示设置弹窗文本中英文切换 */
		void applyDisplayPopupLanguage(int lang);

		/** 拾取编号弹窗文本中英文切换 */
		void applyPickPopupLanguage(int lang);

		/** 构建求和节点拾取弹窗 */
		void ensureSumNodePopup();

		/** 构建颜色拾取弹窗（懒创建，按取色目标同步截面颜色或显示设置色块） */
		void ensureColorPopup();

		/** 显示设置色块点击：弹出取色（index 0-6力分量/7-13力矩分量） */
		void slot_showDsColorPopup(int index);

		/** 取色结果应用到显示设置色块（样式与颜色列表同步） */
		void applyDsSwatchColor(int index, const QColor& color);

		/** 显示设置色块点击过滤（打开取色） */
		bool eventFilter(QObject* watched, QEvent* event) override;

		/** 按当前编辑截面刷新拾取进度提示（必选单元/节点交集说明）显隐 */
		void updatePickHint();

		/******** mPost 拾取条（单元/节点/求和节点；收割-回显-组合高亮） ********/
		/** 三条拾取条任一处于激活态 */
		bool anyPickBarActive() const;

		/** 失活全部激活拾取条（弹窗打开/切换编辑截面时退出瞬时态） */
		void deactivatePickBars();

		/** 编辑缓冲回显三条拾取条（displayData 不置脏不发信号） */
		void refreshPickBars();

		/** 组合高亮：全部失活时把当前截面单元+节点写回选择容器并重画
		* （v1 高亮=共享选中容器整体，激活条独占；失活态由面板恢复组合视图） */
		void refreshCombinedHighlight();

		/** 所选单元→其全部节点真实ID映射（各单元内去重） */
		QHash<int, QList<int>> collectElementNodeMap();

		/** 所选单元的全部节点真实ID（去重，跨部件） */
		QList<int> collectElementNodeIds();

		/** 汇总当前截面的输出节点真实ID：勾选忽略节点集或未限定节点→所选单元全部节点；否则→与所选单元节点的交集 */
		QList<int> collectOutputNodeIds();

		/** 按显示设置勾选的分量组以临时力/力矩数据重建ForceMoment符号（无输出节点/分量时移除符号） */
		void refreshForceMomentSymbols();

		/** 按数据精度+记数法格式化符号数值文本（弹窗懒创建时按默认：科学计数法3位） */
		QString formatSigValueText(double value) const;

		/** 读取当前帧ElementNodalForce节点力场：所选单元的单元节点值逐节点求和（真实节点id→力矢量）；失败返回空 */
		QHash<int, QVector3D> readSectionNodalForceField();

		/** 移除力/力矩符号（无符号时静默） */
		void clearForceMomentSymbols();

		/** 失效已计算结果：移除符号、复位清除按钮与计算截面标记（拾取/切截面/删除/视口切换时调用） */
		void invalidateCalcResult();

	private slots:
		/** 切换中英文 */
		void slot_changeLanguage(int language);

		/** 搜索框过滤截面列表 */
		void slot_searchChanged(const QString& text);

		/** 全选按钮：全勾/全不勾可见截面 */
		void slot_selectAllClicked();

		/** 新建按钮 */
		void slot_newButton();

		/** 删除按钮：移除勾选行 */
		void slot_deleteButton();

		/** 列表复选/数据变化：同步已选计数 */
		void slot_itemChanged(QTableWidgetItem* item);

		/** 截面名称行编辑完成：回写列表 */
		void slot_nameEditFinished();

		/** 求和点单选切换：回写列表并联动的行 */
		void slot_sumPointToggled(int id);

		/** 坐标系下拉切换：回写列表 */
		void slot_coordChanged(const QString& text);

		/** 求和坐标行编辑完成：回写列表 */
		void slot_coordEditFinished();

		/** 拾取单元/节点按钮：弹出编号录入弹窗（0单元/1节点） */
		void slot_showPickPopup(int target);

		/** 拾取弹窗确认：按选择操作（替换/添加/移除）回写截面数据 */
		void slot_confirmPick();

		/** 求和节点按钮：弹出单节点编号录入弹窗 */
		void slot_showSumNodePopup();

		/** 求和节点弹窗确认：校验单节点编号并回写 */
		void slot_confirmSumNode();

		/** 颜色单元格点击：弹出取色弹窗 */
		void slot_showColorPopup(int row);


		/** 显示设置按钮：弹出/关闭显示设置弹窗 */
		void slot_showDisplayPopup();

		/** 显示设置弹窗：恢复默认参数 */
		void slot_displayRestoreDefaults();

		/** 计算按钮：按当前截面与显示设置绘制力/力矩（绘制仅在计算后进行） */
		void slot_calcClicked();

		/** 清除按钮：移除已绘制的力/力矩符号并复位计算态 */
		void slot_clearClicked();

	private:
		QVBoxLayout*					_mainLayout{};		//主布局

		//--------- 截面管理 ---------
		FoldSection*					_manageFold{};
		QLineEdit*						_searchEdit{};		//搜索截面名称
		MxFunctions::mPanelTable*			_table{};			//截面列表（复选/名称/求和点/坐标系/颜色；共享管理表格件，空态覆盖层内置）
		QPushButton*					_btnSelectAll{};	//全选
		QLabel*							_labelSelCount{};	//已选 N
		QPushButton*					_btnDelete{};		//删除
		MxFunctions::mPrimaryButton*	_btnNew{};			//新建
		QLineEdit*						_nameEdit{};		//截面名称
		mPostPickWidgetController*		_elemPick{};		//单元（必选）拾取条（mPost 栈；激活拾取/详情/清空内建）
		QToolButton*					_btnElemIds{};		//单元行"编号"小钮（手工编号录入弹窗入口）
		mPostPickWidgetController*		_nodePick{};		//节点（可选）拾取条
		QToolButton*					_btnNodeIds{};		//节点行"编号"小钮
		QLabel*							_pickHintLabel{};	//拾取进度提示（文案随拾取状态切换）
		MxFunctions::mComboActionRow*	_coordRow{};		//坐标系行（下拉＋"＋"复合，公共控件）
		QComboBox*						_comboCoord{};		//坐标系下拉（_coordRow 子件）
		QPushButton*					_btnAddCoord{};		//坐标系"＋"（_coordRow 子件，待接入置灰）
		MxFunctions::mChoiceGroup*		_sumPointChoice{};	//求和点（形心/节点/坐标，共框圆点单选）
		QWidget*						_sumNodeRow{};		//求和节点行容器（[拾取条+编号小钮]，条件显隐须整行单控件）
		mPostPickWidgetController*		_sumNodePick{};		//求和节点拾取条（PickOne 单选；求和点=节点时显示）
		QToolButton*					_btnSumNodeIds{};	//求和节点行"编号"小钮（单节点录入弹窗入口）
		MxFunctions::mAxisCoordRow*		_coordInput{};		//全局坐标 X/Y/Z 三输入行（公共控件）

		//--------- 计算与显示 ---------
		FoldSection*					_calcFold{};
		MxFunctions::mChoiceGroup*		_calcTypeChoice{};	//计算类型（自由体力/合力，共框圆点单选）
		MxFunctions::mChoiceGroup*		_loadTypeChoice{};	//载荷类型（自由力/外力/支反力）
		MxFunctions::mAdvancedOptionsButton* _btnDisplay{};	//显示设置行（"设置 ›"，公共控件）
		QCheckBox*						_checkIgnoreNode{};	//忽略节点集
		QLineEdit*						_editTolerance{};	//容差
		QPushButton*					_btnClear{};		//清除
		MxFunctions::mPrimaryButton*	_btnCalc{};			//计算

		QList<SectionData>				_sections;			//行数据（行 UserRole 存 id 关联）
		int								_nextId{ 1 };		//截面 id 计数
		int								_caseId{ -1 };		//当前工况（面板上下文）

		//--------- 统一编辑契约（V1.3：定义事务缓冲独立于计算/显示） ---------
		mPostObjectEditController*		_editor{};
		SectionData						_working;			//当前编辑缓冲（activeSection 返回此对象）
		int								_editingId{ -1 };	//当前编辑截面 id（-1 无）
		bool							_loadingWorking{ false };	//缓冲装载期间屏蔽变更通知

		QPointer<mxr::REBUILD_PostRend> _postRend;			//当前后处理视口(视口删除时随QObject析构自动置空)
		QPointer<PostData::mPostDataBase> _postDB;			//当前后处理数据库

		//--------- 拾取编号弹窗 ---------
		QWidget*						_pickPopup{};		//弹窗（Qt::Popup，点击外部自动关闭）
		QLabel*							_pickTitleText{};	//标题（选择单元/选择节点）
		QLabel*							_pickOpText{};		//选择操作标签
		QComboBox*						_pickOpCombo{};		//选择操作（替换/添加/移除）
		QLabel*							_pickIdsText{};		//编号标签（实体编号/节点编号）
		QTextEdit*						_pickEntryEdit{};	//编号录入
		MxFunctions::mPrimaryButton*	_pickOkBtn{};		//确认
		int								_pickTarget{ 0 };	//拾取目标：0单元/1节点

		//--------- 求和节点拾取弹窗 ---------
		QWidget*						_sumNodePopup{};	//弹窗（Qt::Popup，点击外部自动关闭）
		QLabel*							_snTitleText{};		//标题"选择求和节点"
		QLabel*							_snIdsText{};		//实体编号标签
		QLineEdit*						_snEdit{};			//节点编号输入（单个）
		MxFunctions::mPrimaryButton*	_snOkBtn{};			//确认

		//--------- 颜色拾取弹窗 ---------
		FbdColorPicker*					_colorPicker{};		//取色弹窗（可复用控件 mFbdColorPicker）
		int								_colorSectionId{ -1 };	//正在取色的截面id

		//--------- 显示设置弹窗 ---------
		QWidget*						_displayPopup{};	//弹窗（Qt::Popup，点击外部自动关闭）
		QLabel*							_dsTitleText{};		//标题"显示设置"
		QLabel*							_dsForceHeadText{};	//力分量表头
		QCheckBox*						_dsForceAll{};		//力分量表头复选（控制整列）
		QList<QCheckBox*>				_dsForceChecks;		//力分量行复选（X/Y/Z/X+Y/Y+Z/Z+X/X+Y+Z）
		QLabel*							_dsMomentHeadText{};	//力矩分量表头
		QCheckBox*						_dsMomentAll{};		//力矩分量表头复选（控制整列）
		QList<QCheckBox*>				_dsMomentChecks;	//力矩分量行复选
		QLabel*							_dsSymSizeText{};	//符号尺寸
		MxFunctions::mChoiceGroup*		_dsSizeChoice{};	//符号尺寸分段（自动/统一/缩放，公共控件）
		QLineEdit*						_dsSizeEdit{};		//符号尺寸数值（自动时置灰）
		QLabel*							_dsSymShapeText{};	//符号形式
		MxFunctions::mChoiceGroup*		_dsPosChoice{};		//符号位置分段（头部/中点/尾部，公共控件）
		QComboBox*						_dsShapeCombo{};	//符号形状（箭头）
		QLabel*							_dsPrecText{};		//数据精度
		QLineEdit*						_dsPrecEdit{};		//数据精度值
		QComboBox*						_dsNotationCombo{};	//记数法（科学计数法）
		QLabel*							_dsSumSizeText{};	//求和点尺寸
		QLineEdit*						_dsSumSizeEdit{};	//求和点尺寸值（%）
		QCheckBox*						_dsShowValues{};	//显示数值
		QCheckBox*						_dsLabelPrefix{};	//标签前缀
		QPushButton*					_dsRestoreBtn{};	//恢复默认

		//--------- 显示设置分量色块 ---------
		QList<QLabel*>					_dsForceSwatches{};		//力分量色块（点击取色）
		QList<QLabel*>					_dsMomentSwatches{};	//力矩分量色块
		QList<QColor>					_dsForceColors{};		//力分量当前颜色
		QList<QColor>					_dsMomentColors{};		//力矩分量当前颜色
		int								_colorTarget{ 0 };		//取色目标：0截面颜色/1显示设置色块
		int								_dsColorIndex{ -1 };	//色块索引：0-6力分量/7-13力矩分量
		bool							_dsBulkUpdating{ false };	//显示设置批量回填（恢复默认）期间抑制符号重建
		bool							_hasCalcResult{ false };	//已点击计算（力/力矩仅在计算后绘制，显示设置联动重绘以此为门控）
		int								_calcSectionId{ -1 };		//已计算结果的截面id（切换编辑截面即失效）
		QHash<int, QVector3D>			_nodalForceField{};			//节点力场缓存（真实节点id→矢量；显示设置联动重绘免重读文件）
		int								_nodalForceFrame{ -1 };		//缓存对应的帧号（-1无缓存；拾取/切截面/清除时随失效清空）
	};
}
