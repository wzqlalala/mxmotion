#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

//QT
#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QTableWidget>
#include <QList>
#include <QPair>
#include <QHash>

#include "mPanelBase.h"
#include "mCalcLineEdit.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mPostDataBase.h"

#include "DerivedCase/mPostDerivedCaseTypes.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


using namespace PostData;

namespace PostWindow
{
	/*
	* 线性叠加面板：界面/交互对齐前后处理整合原型 V1.3 派生载荷-线性叠加
	* （#derivedLoadParameters，与包络/步序列共用 mountPostDerivedCase 骨架）：
	*  - 折叠组「线性叠加定义」：派生工况名（自动命名"线性叠加-N"，空名/重名校验）；
	*  - 折叠组「工况」：搜索框 + 来源工况管理表（复选|原生工况|帧|缩放系数，
	*    原生行=工况×帧；「派生工况」分组行可引用会话内其它定义，当前定义行
	*    禁用并标"当前"）+ 全选/取消 + 已选计数 + 批量缩放连体控件 + 反馈行；
	*  - 生成：叠加勾选来源的云图（派生引用按两层深度展开，环引用拒绝）。
	*  派生工况定义集存 mPostDerivedCaseStore（按后处理模型实例分桶的进程级
	*  存储，树"派生工况"分支/会话存取共用，批次A0 收敛——名称域/引用校验
	*  为 store 单一实现）；生成参数即定义内容（原 guidata 勾选记忆死字段
	*  _linearFrames/_linearFrameRatios 已于批次C 收口删除）。
	*/
	class MPOSTWINDOW_EXPORT mPostLinearSuperpositionPanel : public mPanelBase
	{
		Q_OBJECT
	public:
		explicit mPostLinearSuperpositionPanel(QWidget* parent = nullptr);

		//新建线性叠加（结果页工况组入口）：每次入口=新建一个派生工况定义
		//（原型 createDerivedCase 语义），并重建来源工况行集
		void newLinearPanel(int caseid);

		//通过视口名截取云图视口
		QString getCloudViewName(QString view);

		//设置当前视图（树链/视口联动重入：绑定源云图并恢复定义与勾选状态）
		void setCurViewName(const QString& viewName);

	//重建来源工况行集（按当前云图变量取帧）
	void setTableData(QPointer<mPostDataBase> postDB);

	/*
	* 按定义 id 装载进入编辑（树"派生工况"分支单击回编入口，2026-11-12
	* 批次A0）：绑定当前后处理上下文后装载目标定义；无后处理视口时行集
	* 为空，仅装载名称与派生引用行。
	*/
	void editDefinitionById(int definitionId);

        //设置显示模式
        void setShowMode();

	private slots:
		//生成线性叠加
		void slot_generateLinear();

		//语言切换全量刷新
		void slot_changeLanguage(int language);

	private:
		//——数据结构——
		//原生来源行（工况×帧，行级勾选+缩放系数）
		struct NativeRow
		{
			int frameId = 0;
			QString caseName;                       //原生工况名
			QString frameText;                      //帧文本（"N:Time = x.xxE+xx"）
			QString scale = QStringLiteral("1");    //缩放系数文本（有限数值校验）
			bool enabled = false;                   //参与叠加
		};
		//派生引用行（会话内其它定义作为来源；defId=store 定义 id）
		struct DerivedRow
		{
			int defId = -1;
			QString scale = QStringLiteral("1");
			bool enabled = false;
		};

		//——UI 构建/刷新——
		void setupUI();
		void applyLanguageTexts();
		void renderTable();                          //按 _rows/_defs 重渲管理表（含派生分组与空态）
		void updateActionsState();                   //全选文案/已选计数/批量缩放可用态
		void setFeedback(const QString& text, bool error = false);   //反馈行（空文案整行收起）
		bool commitDefinition();                     //名称/勾选/缩放变更即时提交（校验链对齐原型 commit）
		void applyEditingDefToRows();                //当前编辑定义的启用来源应用回行集
		void updateCurrentRowName();                 //当前定义行名称单元格就地刷新（免整表重渲）

		//——数据辅助——
		//当前模型实例的线性叠加定义集（store 桶 kind=Superposition 视图）
		QList<DerivedCaseDefinition> currentDefs() const;
		/*
		* 勾选来源 → 帧号×合并比例。批次C 收敛：展开走 mPostDerivedCaseStore::
		* expandDefinition 单一实现（与步序列/包络同源；私有递归已删），线性叠加
		* 语义=同帧比例相加（frameId 归并）；环/超两层/引用失效/系数非法返回
		* false 并经反馈行提示。
		*/
		bool collectSelectedFrameScales(QHash<int, double>& frameScales);
		//同步 _derivedRows 与定义集（按 defId 对齐保留既有勾选；当前编辑行恒不启用防自引用）
		void syncDerivedRows();

		/*
		* 派生引用行级禁用预判（批次C，对齐步序列/包络面板）：以"当前勾选的派生
		* 引用 + candidate 行"模拟 store::checkReferences，返回禁用原因文案
		* （空串=可用）——替代"勾完整体提交才报错"的兜底体验。
		*/
		QString derivedRowDeniedReason(int candidateDefId) const;

	private:
		QPointer<mxr::REBUILD_PostRend>			_curPostRend{};					//当前激活窗口
		QPointer<mPostDataBase>				_db = nullptr;					//当前结果库

		//——数据——
		QList<NativeRow>					_rows;							//原生来源行集
		QList<DerivedRow>					_derivedRows;					//派生引用行集（与定义集平行）
		QString								_rowsPostKey;					//行集所属结果库（漂移防护）
		int									_editingId = -1;				//当前编辑定义（store 定义 id）
		int									_storeKey = -1;					//store 桶键（modelDataID；定义集随模型实例隔离，批次A0）
		bool								_rendering = false;				//重渲中（屏蔽控件信号回环）
		bool								_langEn = false;				//英文界面（语言切换全量刷新）
		bool								_fallbackCreated = false;		//setCurViewName 兜底刚建定义（newLinearPanel emit 判重）

		//——界面——
		QVBoxLayout*						_mainLayout;
		FoldSection*						_definitionFold = nullptr;		//线性叠加定义
		FoldSection*						_nativeFold = nullptr;			//工况

		PanelLabel*							_labelName = nullptr;			//派生工况标签（语言切换刷新）
		QLineEdit*							_editName = nullptr;			//派生工况名
		QLineEdit*							_editSearch = nullptr;			//搜索工况或帧
		QTableWidget*						_tableLinear = nullptr;			//来源工况管理表
		QPushButton*						_btnSelectAll = nullptr;		//全选/取消
		QLabel*								_labelCount = nullptr;			//已选 N 项（筛选外 M 项）
		QLabel*								_labelBatch = nullptr;			//批量缩放标签（语言切换刷新）
		QLineEdit*							_editBatchScale = nullptr;		//批量缩放系数
		QPushButton*						_btnSetScale = nullptr;			//设置
		QLabel*								_labelFeedback = nullptr;		//反馈行
		QPushButton*						_buttonGenerate = nullptr;		//生成
	};
}
