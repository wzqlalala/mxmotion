#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

//QT
#include <QWidget>
#include <QVBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QHash>
#include <QStringList>
#include <QVector3D>

#include "mPanelBase.h"

//共享 UI 库（MxFunctions）：系数定义共框圆点单选 mChoiceGroup、坐标系复合行 mComboActionRow
namespace MxFunctions
{
	class mChoiceGroup;
	class mComboActionRow;
}

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


namespace MechPreData
{
	class mPostGUIData;
}

namespace PostData
{
	class mPostDataBase;
}

namespace mxr
{
	class REBUILD_PostRend;
}

namespace PostWindow
{
	class FbdColorPicker;
	/*
	* 按 guidata 应用变形到渲染管线（云图重绘与变形控制面板共用的落地点）：
	* setDeformationScale + setIsShowInitialShape + 变形后重算图例最值。
	* guidata 为数据源（界面状态须先写回 guidata）：
	*   _shapeType——同时显示/仅显示初始构型/仅显示当前构型（对齐 2.0"显示构型"三态；
	*   "仅显示初始构型"=变形系数置 0 只看未变形构型）；
	*   _initialDisplayMode——不显示/线框/边线/特征线/仅显示初始构型（线框/边线/特征线
	*   经 setInitialShapeMode 切换无变形线形几何：面线+线/边界线全量（自由边+特征边）/
	*   仅特征边；颜色经 setInitialShapeColor：部件色表/网格默认黑/自定义色）；
	*   _deformFactor——变形系数（XYZ 分量）。
	* postRend-目标视口渲染 guidata-视口界面数据 updateRange-是否重算图例最值（云图重绘链路传 true）
	*/
	void MPOSTWINDOW_EXPORT applyPostDeformation(mxr::REBUILD_PostRend* postRend, MechPreData::mPostGUIData* guidata, bool updateRange = true);

	/*
	* 按变形变量切换变形数据源（云图重绘链路 slot_showCloud 与变形控制面板共用）：
	* 渲染端变形数据源固定为数据库位移哈希 getDisplacementDataByFrameID（见 REBUILD_DataProxy），
	* 故把 guidata->_deformVariable 指定变量的前三分量矢量场写入该哈希（覆盖前缓存原始
	* 位移到 guidata->_origDisplacement，切回默认变量恢复）。key 域为连续节点id，与位移哈希同域。
	* 支持新格式 mxdb（mIMxdbFile1 出参重载，不污染云图缓存）与懒加载 OP2/Altair
	* （按 postName 在对象树找 reader；其 readVarDatas 会覆盖帧结果数据，读变形分量前先
	* 重发布云图当前分量兜底）。旧格式 mxdb 暂不支持（出参键域与位移哈希可能不一致）。
	* postRend-目标视口渲染 guidata-视口界面数据 返回 false=变量场读取失败（调用方不重建代理、回滚界面选择）。
	* 注意：动画帧代理的位移在帧创建时已物化，已生成的动画不随变量切换刷新，重新生成动画即用新场（TODO）。
	*/
	bool MPOSTWINDOW_EXPORT applyDeformVariableField(mxr::REBUILD_PostRend* postRend, MechPreData::mPostGUIData* guidata);

	/*
	* 按变量读前三分量组装节点矢量场（键为连续节点id，与位移哈希同域）。
	* 变形变量切换与向量图渲染共用同一读场实现；懒加载链路（OP2/RST/RTH/ODB/H3D/
	* d3plot/VTK）经云图面板 reader 重发布后取分量，读取前后各恢复一次云图当前分量
	* 防显示态被覆盖；新格式 mxdb 走 mIMxdbFile1 出参重载（不污染云图缓存）；
	* 旧格式 mxdb 不支持（出参键域与位移哈希可能不一致）。返回空=读取失败。
	* postDB-结果数据库 postName-结果名（mxdb 出参重载寻库用） varName-变量名
	* frame-帧号 guidata-云图视口界面数据（云图态保护用）
	*/
	QHash<int, QVector3D> MPOSTWINDOW_EXPORT readNodeVectorField(PostData::mPostDataBase* postDB,
		const QString& postName, const QString& varName, int frame, const MechPreData::mPostGUIData* guidata);

	//变形控制纯逻辑（供 tests/PostWindow 无头断言）
	namespace detail
	{
		//变形变量列表过滤：保留节点变量（varType==0）且前三分量为空间矢量分量
		//（X/Y/Z 或 T1/T2/T3，兼容 mxdb 与 OP2 命名）的变量——张量（Stress 的 XX/YY/ZZ）、
		//标量与非节点变量不可作变形场，剔除；两表缺失的变量名（如伪变量 SolidColor）剔除，保持原顺序
		QStringList MPOSTWINDOW_EXPORT filterDeformVariables(const QStringList& varNames,
			const QHash<QString, QStringList>& composByName, const QHash<QString, int>& varTypeByName);

		//是否默认（文件位移）变量：_deformVariable 为空、列表为空或等于列表首项
		bool MPOSTWINDOW_EXPORT isDefaultDeformVariable(const MechPreData::mPostGUIData* guidata);

		//三分量标量场组装节点矢量场：取三分量键并集，单分量缺失补 0；键域与输入一致（连续节点id）
		QHash<int, QVector3D> MPOSTWINDOW_EXPORT assembleNodeVectorField(const QHash<int, double>& compoX,
			const QHash<int, double>& compoY, const QHash<int, double>& compoZ);
	}

	/*
	* 变形控制参数面板（V1.3 原型 #deformationParameters，控件收敛共享 UI 库）：
	* 折叠组"变形控制"（变形变量/缩放策略/系数定义[mChoiceGroup]/变形系数或
	* X·Y·Z方向系数三行/坐标系[mComboActionRow]）
	* + 折叠组"初始构型显示"（初始构型 4 项/颜色+取色弹窗）。
	* 无树节点：面板作用于当前激活云图视口的 guidata；入口为 Ribbon"结果"页
	* "变形控制"按钮与云图面板"变形系数"行的齿轮按钮（均经总线 clickDeformationSig）。
	*/
	class MPOSTWINDOW_EXPORT mPostDeformationPanel : public mPanelBase
	{
		Q_OBJECT
	public:
		explicit mPostDeformationPanel(QWidget* parent = nullptr);

		//打开面板：载入当前激活云图视口的变形设置（caseid 预留，与 hub createXxx 族签名一致）
		void newDeformationPanel(int caseid = -1);

		//按视口名载入设置（面板跟随视口内容时由外部调用）
		void setCurViewName(const QString& viewName);

private slots:
	/** 应用变形（系数输入回车） */
	void slot_applyDeformation();

	/** 变形变量切换（读变量矢量场写位移哈希+重建当前帧代理；失败回滚到上一生效变量） */
	void slot_chooseDeformVariable(const QString& text);

		/** 系数定义切换：等比例/分方向（即时保存模式与当前输入，不触发渲染） */
		void slot_chooseScaleMode();

		/** 缩放策略/坐标系切换：仅保存选项，不触发渲染 */
		void slot_saveDeformOptions();

		/** 初始构型显示模式切换（立即应用） */
		void slot_chooseInitialDisplay(const QString& text);

		/** 初始构型颜色来源切换 */
		void slot_chooseInitialColorMode(const QString& text);

	/** 自定义颜色拾取（共享取色弹窗 FbdColorPicker 锚定色块弹出，实时生效） */
	void slot_pickInitialColor();

	private:
		//guidata -> 界面回填（变量列表从云图 guidata 克隆，坐标系跟随云图坐标系）
		void loadDeformationSettings();

		//界面 -> guidata（不触发渲染）
		void syncToGUIData();

		//等比例/分方向行显隐（行级 setRowVisible 同步折叠高度帽）
		void updateScaleModePages();

		//自定义颜色按钮显隐
		void updateInitialColorVisibility();

		//自定义色应用：写 guidata+刷色块+即时渲染（取色弹窗实时回调）
		void applyInitialColor(const QColor& color);

		//取当前视口渲染与 guidata（任一为空返回 false）
		bool currentContext(mxr::REBUILD_PostRend** postRend, MechPreData::mPostGUIData** guidata);

	QString									_lastDeformVariable{};		//上一生效变形变量（读场失败回滚用）

		QVBoxLayout*							_mainLayout;

		//--------- 变形控制 ---------
		QComboBox*								_comboDeformVariable{};		//变形变量
		QComboBox*								_comboScaleStrategy{};		//缩放策略（变形系数/模型比例/模型单位）
		MxFunctions::mChoiceGroup*				_scaleModeChoice{};			//系数定义（等比例/分方向，共框圆点单选）
		QLineEdit*								_editUniformScale{};			//变形系数（等比例，整行输入）
		QLineEdit*								_editScaleX{};				//X方向系数（分方向，独立行）
		QLineEdit*								_editScaleY{};				//Y方向系数
		QLineEdit*								_editScaleZ{};				//Z方向系数
		MxFunctions::mComboActionRow*			_coordRow{};				//坐标系复合行（下拉伸展＋"＋"动作钮）
		QComboBox*								_comboCoordSystem{};			//坐标系（_coordRow->combo()）
		QPushButton*							_btnCoordQuickCreate{};		//坐标系快捷创建＋（_coordRow->actionButton()，待接入置灰）

		//--------- 初始构型显示 ---------
		QComboBox*								_comboInitialDisplay{};		//不显示/线框/边线/特征线（V1.3 原型 4 项）
		QComboBox*								_comboInitialColorMode{};	//部件/网格/自定义
		QPushButton*							_btnInitialColor{};			//自定义颜色色块 26×26
		FbdColorPicker*							_colorPicker = nullptr;		//共享取色弹窗（懒创建）
	};
}
