#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"
#include "mPanelBase.h"
#include "PostCurveStruct.h"

#include <QPointer>

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;

class QComboBox;
class QTableWidget;
class QPushButton;
class QLabel;
class QTreeWidget;
class QTreeWidgetItem;
class QHideEvent;
class QCheckBox;
class QFrame;

namespace PostData
{
	class mPostDataBase;
}

namespace MIOFile
{
	class mIONativeFile;
	class mIOVtkFile;
}

namespace PostWindow
{
	class mPostPickWidgetController;

	/*
	* 曲线提取参数面板（结果分析组"曲线提取"入口）。
	*
	* 界面基准：V1.3 原型 resultCurveExtract/curveExtractParameters（前处理原型内嵌
	* JS 运行时构建；控件级几何与令牌按 V1.3 CSS 规则提取）。V1.3 对齐变更：
	* 主文本/标签色统一 #303313、控件圆角 4px、目标树选中节点文字保持主文本色。
	* 结构：折叠组"曲线提取"（工况/变量/对象选择三行）+ 组外 分量/层 两张勾选表
	* （表头=标题+全选/取消/反选）+ 绘制选项行（下拉+绘制主按钮）+ 目标树（仅
	* "现有曲线图"时显示）+ 状态行（空文案收起）。
	*
	* 对象选择（2026-11-18 换 mPost 拾取栈拾取条，云图/向量图/爆炸/阵列同款）：
	* 条体点击=激活/停止拾取，详情弹窗/清空钮/计数文案内建；对象类型=当前变量
	* 固有属性（getVariableType：1=单元，否则=节点），不出类型下拉。
	*
	* 数据：工况/变量/分量/层取自当前后处理视口结果库（与云图面板同源）；
	* 勾选按变量独立保存（原型 selectedByVariable 语义）。
 * 引擎链路：绘制=readTimeVarCurve 提取全部帧时间历程，经曲线模块对外门面
 * mCurveDocController::addCurveBatch 落图（枚举序同绘制选项下拉）。落点页
 * （"新视图页/现有曲线图"布局除外）=当前视图页绑定的曲线页（无则新建并绑定
 * +页名对齐——绑定使卡片登记挂到当前视图页）；绘制后经工厂 registerChartsOnPage
 * 静默登记（用户终态裁定：不自动显示——不占空格、不浮出、不动页布局，呈现集
 * 冻结不含新卡），显示时机完全由用户自行调整页面布局决定。
	*/
	class MPOSTWINDOW_EXPORT mPostCurveExtractPanel : public mPanelBase
	{
		Q_OBJECT
	public:
		explicit mPostCurveExtractPanel(QWidget* parent = nullptr);

		/** 打开/刷新面板（入口点击与视口切换；无后处理视口数据时保持空态） */
		void updateCurveExtractPanel();

protected:
	/** 面板隐藏=退出拾取态并收起拾取条（宿主 setMainWidget 切换面板时触发） */
	void hideEvent(QHideEvent* event) override;

	/** 面板重显=恢复拾取条可见（hideEvent 隐藏期的对称恢复，云图/爆炸/阵列同款） */
	void showEvent(QShowEvent* event) override;

	private:
		/** 构建面板控件（语言文本由 slot_changeLanguage 设置） */
		void setupUI();

		/** 表格行内复选框（cellWidget 容器内居中；无复选行返回 nullptr） */
		static QCheckBox* rowCheckBox(QTableWidget* table, int row);

		/** 勾选表格组：表头（标题+全选/取消/反选）+ 复选/文本 两列正文 */
		struct CheckTable
		{
			QFrame* root = nullptr;             //整组容器（表头+表格；QFrame 承载 QSS 边框）
			QLabel* caption = nullptr;          //表头左侧标题（分量/层）
			QTableWidget* table = nullptr;
			QPushButton* btnAll = nullptr;
			QPushButton* btnNone = nullptr;
			QPushButton* btnInvert = nullptr;
		};
		CheckTable buildCheckTable(const char* objectName);

		/** 批量动作：0 全选 / 1 取消 / 2 反选（写回按变量独立保存的勾选集合） */
		void applyTableAction(CheckTable& ct, int action);

		/** 按当前变量刷新勾选表（items 为空时显示空态行；勾选按变量独立恢复） */
		void refreshCheckTable(const CheckTable& ct, const QStringList& items,
			QHash<QString, QSet<QString>>& stateByVar);

	/** 当前勾选值列表（按表格行序） */
	QStringList checkedValues(const CheckTable& ct) const;

	/** 表内可勾选行数（空态行无复选框不计；区分"有项未勾"与"变量本身无分量"） */
	int checkableRowCount(const CheckTable& ct) const;

	/** 拾取条可用态/空态文案刷新（无变量置灰+清数据；文案随对象类型与语言） */
	void refreshPickBar();

	/** 拾取条数据收割为提取对象清单（按变量类型取节点/单元 result_id 装入
	    _pickIds，绘制前调用；拾取条激活期间数据已逐次入库，getAllData 即全集） */
	void harvestPickedIds();

	/** 重建绘制目标树（仅"现有曲线图"绘制选项下可见；无页显示空态） */
	void rebuildTargetTree();

		/** 状态行（空文案整行收起） */
		void setStatus(const QString& text);

	/** 当前结果库（无后处理视口/数据时返回 nullptr） */
	PostData::mPostDataBase* currentPostDB() const;

	/** native/vtk 数据逐帧提取时间历程（查询面板四分支协议前置两支）：帧×key
	    readVarDatas→getComponeValues 层平均摊平，按拾取 id 组装 cname→(X=帧时间,Y)；
	    某帧缺值不占位（各曲线点数独立）；至少产出一条曲线返回 true */
	bool extractCurvesByFrame(const QString& var, const QList<MIOFile::CurveKey>& keys,
		MIOFile::mIONativeFile* nativeReader, MIOFile::mIOVtkFile* vtkReader,
		QStringList& cnames, QHash<QString, QPair<QVector<double>, QVector<double>>>& curves);

	/** 结果库变化（导入/切视口）后的全量刷新：工况→变量→表→拾取 */
	void refreshFromResultData();

	/** 取变量分量表；旧版 mxdb0 导入链个别变量 _compoNames 空而 _nCompo>0（按名
	    索引的一切读取落空——分量表空/取值被拦），此况合成 C0..Cn 写回库内帧信息
	    后重取（convertFrameVariables 新链同款兜底名） */
	QStringList composOfVariable(const QString& var);

	/** 绘制落点页（"当前页"）保障：当前视图页已绑定的曲线页（页对齐反查）；
	    无绑定页时新建曲线页并绑定到当前视图页+页名对齐视图页名（NVH
	    resolveCurrentViewPageTarget 同口径——绑定使本批图表卡片登记挂到当前
	    视图页，openChart 按绑定解析落页）。无视图页上下文返回 -1（addCurveBatch
	    自建未绑定页，静默不呈现） */
	int ensureCurrentViewCurvePage();

		/** 语言文案全量刷新（含下拉选项与表格标题） */
		void applyLanguageTexts();

private slots:
	void slot_chooseCase(const QString& text);
	void slot_chooseVariable(const QString& text);
	void slot_drawClicked();
	void slot_changeLanguage(int language);

	private:
		QVBoxLayout*						_mainLayout = nullptr;
		FoldSection*						_mainFold = nullptr;

		QComboBox*							_comboCase = nullptr;
		QComboBox*							_comboVariable = nullptr;

		//对象选择拾取条（mPost 拾取栈，云图同款；激活/收割/高亮/详情弹窗控制器自管）
		mPostPickWidgetController*			_pickController = nullptr;

		CheckTable							_tableComponent;         //分量
		CheckTable							_tableLayer;             //层

		QComboBox*							_comboLayout = nullptr;  //绘制选项
		QLabel*								_labelDraw = nullptr;    //绘制选项行标签（组外）
		QPushButton*						_btnDraw = nullptr;
		QTreeWidget*						_targetTree = nullptr;    //绘制目标（现有曲线图）
		QLabel*								_statusLabel = nullptr;

		//当前结果库与变量上下文
		QPointer<PostData::mPostDataBase>	_postDB;
		QString								_curVariable;

		//勾选状态按变量独立保存（原型 selectedByVariable 语义）
		QHash<QString, QSet<QString>>		_checkedCompoByVar;
		QHash<QString, QSet<QString>>		_checkedLayerByVar;

	//变量对象类型（getVariableType：1=单元，其余=节点）
	bool								_curVarElement = false;

	//提取对象清单（绘制时由 harvestPickedIds 从拾取条装入；节点/单元 id）
	QVector<int>						_pickIds;

	//绘制目标（曲线模块图表节点id；空=未选）
	QString								_selectedTargetViewport;
	bool								_langEn = false;
	};
}
