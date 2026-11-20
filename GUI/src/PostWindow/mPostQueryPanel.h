#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"
#include "mPanelBase.h"
//对象选择拾取条仅作指针成员（_pickController），头内前向声明——按值包含
//mPostGeneralPickUp.h 会把 RebuildRender 头拖进无渲染依赖的编译单元；
//工厂调用 createPickWidget 在本类 .cpp 包含（mPostVectorPlot.h 同款口径）

#include <QHash>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QStringList>

class QComboBox;
class QPushButton;
class QToolButton;
class QHideEvent;
class QShowEvent;
class QFrame;
class QLabel;
class QScrollArea;
class QTableWidget;
class QVBoxLayout;

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;

namespace PostData
{
    class mPostDataBase;
    struct Compones;
    class mPostModelData;
}

namespace PostWindow
{
	class mPostQueryResultsDock;
	class mPostPickWidgetController;

	/*
	* 结果查询参数面板（结果页"结果分析"组"查询"入口）。
	*
	* 界面基准：V1.3 原型 resultQuery/queryParameters（前处理原型内嵌 JS 运行时
	* 构建；控件级几何与令牌按 V1.3 CSS 规则提取，data-post-query 样式块）。
 * 结构：折叠组"查询对象及坐标系"（对象选择=类型下拉+mPost 通用拾取条单行并排、
	* 坐标系下拉+"＋"占位钮、查询值=高级选项钮"已选 N 项 ›"开弹窗）+ 折叠组
	* "查询结果"（节点/单元/部件数据三行"N项 ›"钮）+ 查询值弹窗（分组复选清单）
	* + 查询结果停靠窗（视口底部浮层：三页签+表格+清除/导出）。
	*
	* 数据：查询字段目录/默认勾选按原型 fields/selected 常量；云图值/向量值表头
	* 变量名取当前视口 guidata（缺失回退"未指定变量"）。引擎未接入：拾取为武装
	* 态视觉，结果表按原型空态（"请先选择查询值"/"暂无查询结果"），对外留
	* setResults 推送口（原型 window.CAEPostQuery.setResults 契约）。
	*/
	class MPOSTWINDOW_EXPORT mPostQueryPanel : public mPanelBase
	{
		Q_OBJECT
	public:
		explicit mPostQueryPanel(QWidget* parent = nullptr);
		~mPostQueryPanel() override;

		/** 打开/刷新面板（入口点击与视口切换；无后处理视口数据时保持默认目录） */
		void updateQueryPanel();

		/** 对象类型（0 节点 / 1 单元 / 2 部件） */
		int objectType() const;

		/** 当前坐标系选择值（"分析坐标系"/"全局坐标系"） */
		QString coordinateSystem() const;

		/** 当前对象类型下勾选的查询字段 key 列表（按目录序） */
		QStringList selectedFields() const;

		/*
		* 引擎推送查询结果（原型 CAEPostQuery.setResults 契约）：
		* objectType 同上；records 每条为 key→显示文本映射（key 须在当前字段目录内）。
		* 推送后对应页签计数/表格刷新，停靠窗自动展开到该页签。
		*/
		void setResults(int objectType, const QList<QHash<QString, QString>>& records);

protected:
	/** 面板隐藏=退出拾取武装态并收起弹窗/停靠窗（宿主切换面板时触发） */
	void hideEvent(QHideEvent* event) override;

	/** 切回面板：隐藏期 setPickWidgetVisible(false) 的对称恢复（拾取栈口径） */
	void showEvent(QShowEvent* event) override;

	/** 视口尺寸变化时停靠窗跟随重排 */
	bool eventFilter(QObject* watched, QEvent* event) override;

	private:
		/** 构建面板控件（语言文本由 applyLanguageTexts 设置） */
		void setupUI();

		/** 查询值弹窗内容按当前对象类型重建（分组复选清单） */
		void rebuildFieldOptions();

		/** 弹窗分组内加变量小标题（"云图：变量"/"向量：未指定变量"，部件结果组专用） */
		void appendVariableHeading(QVBoxLayout* groupLayout, bool contour);

		/** 刷新查询值钮摘要（"已选 N 项"+tooltip 明细） */
		void refreshFieldSummary();

		/** 刷新拾取条空态默认文案（随对象类型/语言；切换先退出武装态） */
		void refreshPickButton();

		/** 退出拾取武装态（Esc/切类型/开弹窗/开结果停靠窗共用；失活收口由拾取栈信号链完成） */
		void deactivatePick();

		/** 拾取栈数据变化 → 增量吸收进面板记录并实时查询（原容器到达槽的栈版） */
		void absorbPickedData();

		/** 清渲染选择容器该类型桶并重建高亮（结果停靠窗"清除"用；面板记录由调用方清） */
		void clearPickedHighlight(int objectType);

		/** 刷新三行结果入口计数与可用态（"N项"） */
		void refreshResultEntries();

		/** 当前云图变量名（guidata；弹窗部件组"云图：变量"小标题用；列名不带后缀） */
		QString cloudVariableName() const;

		/** 当前云图分量名（guidata->_cloudCompo；弹窗部件组"向量：分量"小标题用） */
		QString cloudCompoName() const;

		/** 字段显示名：结果字段带"（变量名）"后缀（原型 fieldCaption） */
		QString fieldCaption(const QString& key, const QString& baseText) const;

		/** 弹窗选项文案（部件结果字段缩短为 最大值/最大值编号 等，原型 optionCaption） */
		QString optionCaption(const QString& key, const QString& baseText) const;

		/** 查询值弹窗锚点定位（贴触发钮下方，越界收回屏内） */
		void popupUnder(QWidget* popup, QWidget* anchor);

		/** 停靠窗懒创建/换宿主视口（无后处理视口时隐藏并断开宿主） */
		void ensureResultsDock();

		/** 停靠窗贴当前视口底部重排（无宿主视口不动作） */
		void repositionResultsDock();

		/** 三类型停靠窗列全量重算（勾选/语言变化后；空列→单列"查询值"空态） */
		void refreshDockColumns();

		/** 停靠窗页签/操作钮文案刷新（建后与语言切换共用） */
		void applyDockTexts();

		/** 结果页签标题（节点/单元/部件数据） */
		QString tabTitle(int objectType) const;

		/** 当前页签数据导出 CSV（BOM+引号转义+CRLF；空数据不可达） */
		void exportResults(int objectType);

		/** 指定对象类型勾选的字段 key 列表（按目录序；供查表/刷新列） */
		QStringList selectedFieldsFor(int objectType) const;

		/** 关闭查询值弹窗 */
		void closeValuesPopup();

		/** 语言文案全量刷新（含折叠组标题/行标签/弹窗目录重建） */
		void applyLanguageTexts();

		/******** 查询引擎接入（原型 setResults 的面板内引擎侧） ********/

		/** 查询执行：已拾取对象 × 勾选字段 → setResults 填表 */
		void runQuery(int objectType);

		/** 该类型是否已有拾取对象 */
		bool hasPickedObjects(int objectType) const;

		/** 指定变量分量的结果读取（native/vtk/mxdb 新旧四分支，协议同高级查询
		 readQueryCompones；云图值传 guidata 当前变量分量，向量值逐分量轮读） */
		bool readVarCompones(int frame, const QString& varstr, const QString& compostr,
			const QString& layerstr, const QString& complexStr, PostData::Compones& compone) const;

		/** Compones 摊平为 id→值（实体/壳层平均，同高级查询 mergeHash 口径） */
		static QHash<int, double> flattenCompones(const PostData::Compones& compone);

	private slots:
		/** 对象类型切换：退武装态→拾取钮/弹窗目录/摘要联动（原型 type change） */
		void slot_chooseObjectType(int index);

		/** 查询值弹窗开合（原型 trigger click：再点关闭） */
		void slot_valuesOpenClicked();

		/** 结果入口点击：停靠窗展开到对应页签（原型 openResults） */
		void slot_resultEntryClicked(int objectType);

		/** 无参包装：展开节点数据页签（ui-shot -Invoke 驱动） */
		void slot_openNodeResults();

		/** 语言切换 */
		void slot_changeLanguage(int language);

		/** 云图重绘（切帧/切变量）：有查询数据的页签按新数据重查 */
		void slot_cloudRedrawn(const QString& viewName);

		/* ui-shot 自动化验证驱动（产品交互不走此口）：注入一个拾取对象并实时查询。
		* 勿用 #ifdef _DEBUG 包裹——moc 不带编译器内建宏（_DEBUG）跑预处理，
		* 包裹后槽不进元对象，invokeMethod 寻址不到 */
		void slot_seedPickForShot(int objectType, int id);

	private:
		QVBoxLayout*						_mainLayout = nullptr;
		FoldSection*						_objectFold = nullptr;     //查询对象及坐标系
		FoldSection*						_resultsFold = nullptr;    //查询结果

		QComboBox*							_comboObjectType = nullptr;//对象选择左段（节点/单元/部件类型下拉）
		mPostPickWidgetController*			_pickController = nullptr;//对象选择右段（mPost 通用拾取条，云图/向量图/阵列/爆炸同款）
		QComboBox*							_comboCoordSystem = nullptr;//坐标系（分析坐标系/全局坐标系）
		QToolButton*						_btnCoordAdd = nullptr;     //新建坐标系"＋"（占位禁用）
		QPushButton*						_btnValuesOpen = nullptr;   //查询值"已选 N 项 ›"
		QLabel*								_labelValuesSummary = nullptr;//"已选 N 项"文本（钮内左段）

		QPushButton*						_btnResultEntry[3] = {};    //节点/单元/部件数据入口（"N项 ›"）
		QLabel*								_labelResultSummary[3] = {};//"N项"文本（钮内左段）

		//查询值弹窗（Qt::Popup 懒创建；内容随对象类型重建）
		QFrame*								_valuesPopup = nullptr;
		QLabel*								_valuesPopupTitle = nullptr;
		QScrollArea*						_valuesPopupScroll = nullptr;
		QWidget*								_valuesPopupContent = nullptr;

		//停靠窗挂视口宿主树（非面板子件），宿主先于面板销毁时随树释放：须 QPointer 自动置空，析构/hideEvent 等处解引用才不悬空
		QPointer<mPostQueryResultsDock>		_resultsDock;              //视口底部结果停靠窗（懒创建）
		QPointer<QWidget>					_dockHostViewport;         //停靠窗几何基准（osg 渲染表面；停靠窗挂其父件同级叠加）

		//勾选字段（按对象类型独立；原型 selected 三 Set）
		QSet<QString>						_selectedFields[3];

		//上下文
		QPointer<PostData::mPostDataBase>	_postDB;
		bool								_langEn = false;

		//已拾取查询对象（按类型独立；拾取序去重，实时查询的数据源；拾取条数据变化
		//经 absorbPickedData 增量吸收，重入高亮由拾取栈控制器自管）
		QList<int>							_pickedNodes;      //节点真实ID
		QList<QPair<int, int>>				_pickedElements;   //(单元真实ID, 所属部件ID[-1未知])
		QList<int>							_pickedParts;      //部件ID
	};
}
