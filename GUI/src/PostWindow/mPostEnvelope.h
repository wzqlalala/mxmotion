#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"
#include "mPanelBase.h"

#include "DerivedCase/mPostDerivedCaseTypes.h"

#include <QList>
#include <QPointer>
#include <QString>

class QComboBox;
class QHideEvent;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QTableWidgetItem;
class QVBoxLayout;

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;

namespace PostData
{
	class mPostDataBase;
}

namespace PostWindow
{
	/*
	* 包络派生工况参数面板（结果页"工况"组"包络"入口）。
	*
	* 界面基准：V1.3 原型 mountPostDerivedCase(kind=envelope)（前处理原型内嵌
	* CAEPostDerivedLoad 脚本；#derivedLoadParameters/superposition-* 系 CSS），
	* 与步序列面板同构（共用原型派生载荷布局），差异仅在定义组多"包络类型"
	* 下拉、派生来源单行展开（帧文本"0：包络"，无第 N 步展开）。
	* 结构：折叠组"包络定义"（派生工况名称行 + 包络类型行[最小值 Min/最大值
	* Max/绝对值最大 Extreme/极差 Range，默认 Max]）+ 折叠组"工况"（搜索框 +
	* 来源工况管理表[复选/原生工况/帧/缩放系数，派生来源带"派生工况"组头与
	* 行尾编辑钮] + 操作行[全选/已选计数/批量缩放系数连体框] + 状态反馈行）。
	*
	* 数据：原生来源 = 当前后处理结果库 工况×帧 笛卡尔积；派生来源 = 既有
	* 包络定义（每定义单行）。定义存 mPostDerivedCaseStore（按后处理模型实例
	* 分桶的进程级存储，树"派生工况"分支/会话存取共用，批次A0 收敛——校验/
	* 展开为 store 单一实现）；引擎契约同原型"派生载荷定义：保存配置，
	* 不计算或伪造结果"。校验：名称非空/不重名、来源可用、系数有限数值、
	* 派生引用不超过两层且不得循环。
	*/
	class MPOSTWINDOW_EXPORT mPostEnvelopePanel : public mPanelBase
	{
		Q_OBJECT
	public:
		explicit mPostEnvelopePanel(QWidget* parent = nullptr);
		~mPostEnvelopePanel() override;

		/*
		* 菜单入口激活（每次点击"包络"按钮）：解析当前后处理数据上下文并
		* 新建一条"包络-N"定义进入编辑（原型 createDerivedCase 语义）。
		*/
		void activateFromMenu();

		/** 面板已打开时的上下文刷新（视口/结果数据变化后重析来源；无定义不动作） */
		void updateEnvelopePanel();

	/** 当前编辑中的定义 id（无编辑返回 -1；ui-shot/调试用） */
	int editingDefinitionId() const { return _editingId; }

	/*
	* 按定义 id 装载进入编辑（树"派生工况"分支单击回编入口，2026-11-12
	* 批次A0 公开化；与表内编辑钮同一语义）。
	*/
	void editDefinition(int definitionId);

	protected:
		/** 面板隐藏=收起瞬时态（本面板无拾取态，仅清反馈；宿主切换面板时触发） */
		void hideEvent(QHideEvent* event) override;

	private:
		/** 构建面板控件（语言文本由 applyLanguageTexts 设置） */
		void setupUI();

		/** 名称输入框 QSS（invalid=true 红框，原型 aria-invalid） */
		void applyNameEditQss(bool invalid);

		/** 包络类型选项文案（中英；index 与 DerivedCaseDefinition::envelopeType 对应） */
		QString trEnvelopeOption(int index) const;

		/** 禁用原因/派生行帧文案（中英，rebuildSources 与错误反馈同源） */
		QString trSelfSourceDenied() const;
		QString trUnconfiguredDenied() const;
		QString trEnvelopeFrame() const;

		/** 解析激活后处理视口的结果库（无结果数据保持空来源表） */
		void resolveContext();

		/** 来源行显示态重建：原生 工况×帧 + 派生定义展开 + 当前定义自身"当前"行 */
		void rebuildSources();

		/*
		* 保存当前编辑（校验通过写回定义并清反馈；失败反馈行报错，原型 commit）。
		* 返回是否保存成功（生成钮入口据此决定是否继续；既有调用方忽略返回值）。
		*/
		bool commit();

		/** 全量重绘来源表（搜索过滤/定义切换后；保持展开行序：原生组在前） */
		void renderTable();

	/** 仅刷新操作行（全选文案/已选计数/批量缩放可用态；勾选变化不整表重建） */
	void refreshActions();

	/** 新建"包络-N"定义（名称查重递增）进入编辑 */
	void newDefinition();

		/*
		* 引用校验（原型 validateDefinition）：以"当前编辑定义的启用来源 +
		* candidate 行"模拟，检测自身/循环引用与两层派生引用上限。
		* 返回空串=可用；否则为禁用原因文案。
		*/
		QString referenceDeniedReason(const QString& candidateRowId, int candidateDerivedId);

		/** 有效有限数值判定（原型 validNumber） */
		static bool validNumber(const QString& value);

		/** 反馈行显示（error=true 红字；空文本隐藏并收起行占位，原型 message） */
		void showMessage(const QString& text, bool error = false);

		/** 语言文案全量刷新（折叠组标题/表头/按钮/下拉选项/占位符） */
		void applyLanguageTexts();

	private slots:
		/** 搜索过滤（原型 nativeSearch input → renderNative） */
		void slot_searchChanged(const QString& text);

		/*
		* 生成（软件实际能力，超原型）：commit 落盘 → 引擎流式归约物化单帧包络
		* （appendDerivedFrame"0: 包络"）→ 成功后当前云图视口工况下拉切到派生工况
		* （标准级联接管显示）；失败/取消反馈行报错（引擎已回滚）。
		*/
		void slot_generateEnvelope();

		/** 全选/取消（原型 lowerAll：可见可选行全切换） */
		void slot_toggleAll();

		/** 批量缩放系数应用到全部启用行（原型 applyScale） */
		void slot_applyBatchScale();

		/** 包络类型切换（原型 envelope select change → draft.envelope + commit） */
		void slot_envelopeChanged(int index);

		/** 表格单元格变化（复选勾选 → 行启用 + commit；原型 check change） */
		void slot_itemChanged(QTableWidgetItem* item);

		/** 语言切换 */
		void slot_changeLanguage(int language);

	private:
		//来源行显示态（renderTable 的数据源，勾选/系数编辑直写本表）
		struct SourceRow
		{
			QString id;					//原生 "工况|帧"；派生 "derived:<定义id>|0|current"
			QString name;				//工况显示名（缺失来源带"（来源不可用）"后缀）
			QString frame;				//帧文本（派生来源为"0：包络"/"—"）
			QString scale = QStringLiteral("1");	//缩放系数（保持输入文本）
			bool enabled = false;		//参与勾选
			bool isDerived = false;		//派生工况来源（行尾带编辑钮）
			int derivedId = -1;			//派生来源定义 id
			bool isCurrent = false;		//当前编辑定义自身行（禁勾）
			bool unconfigured = false;	//未配置派生定义占位行（禁勾，显示"待配置"）
			bool missing = false;		//来源已不存在（原 saved 行无当前来源）
			QString disabledReason;		//禁用原因（当前/待配置/循环或超层）
		};

	//定义存储：mPostDerivedCaseStore（进程级，按后处理模型实例分桶，批次A0 收敛；
	//本面板仅消费 kind=Envelope 的定义集，名称域/引用校验/序列化由 store 单一实现）
	QList<DerivedCaseDefinition> kindDefinitions() const;

	QVBoxLayout*						_mainLayout = nullptr;
		FoldSection*						_definitionFold = nullptr;	//包络定义
		FoldSection*						_nativeFold = nullptr;		//工况

		PanelLabel*							_nameLabel = nullptr;		//"派生工况"标签
		QLineEdit*							_editName = nullptr;		//派生工况名称
		PanelLabel*							_envelopeLabel = nullptr;	//"包络类型"标签
		QComboBox*							_comboEnvelope = nullptr;	//包络类型下拉
		QLineEdit*							_searchEdit = nullptr;		//搜索工况或帧
		QTableWidget*						_table = nullptr;			//来源工况管理表
		QPushButton*						_btnSelectAll = nullptr;	//全选/取消
		QLabel*								_countLabel = nullptr;		//已选 N 项（筛选外 N 项）
		QLabel*								_scaleLabel = nullptr;		//"缩放系数"批量块标签
		QLineEdit*							_batchScaleEdit = nullptr;	//批量缩放系数输入
		QPushButton*						_btnApplyScale = nullptr;	//设置（连体框右段）
		QLabel*								_feedback = nullptr;		//状态反馈行
		QPushButton*						_buttonGenerate = nullptr;	//主操作蓝按钮（生成）

		int									_editingId = -1;			//当前编辑定义
		QList<SourceRow>					_rows;						//当前编辑来源行
		QString								_nativeQuery;				//搜索词（跨定义保持）
		int									_storeKey = -1;				//store 桶键（modelDataID；_postDB 有效时随上下文更新）

		QPointer<PostData::mPostDataBase>	_postDB;					//当前结果库上下文
		bool								_rebuilding = false;		//renderTable 信号阻塞标记
		bool								_langEn = false;
	};
}
