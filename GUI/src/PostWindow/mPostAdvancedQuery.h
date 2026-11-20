#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

//QT
#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QPointer>

#include "mPanelBase.h"
#include "mCalcLineEdit.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mTableDataWindow.h"
#include "mPostResultDataStruct.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


namespace PostData
{
	class mPostDataBase;
	class mPostDataManager;
	class mPostModelData;
}

namespace MechPreData
{
	class mViewportModel;
	class mViewportPrivate;
	class mPostGUIData;
}

namespace MIOFile
{
	class mMxdbThread;
}

namespace PostWindow
{
	class MPOSTWINDOW_EXPORT mPostAdvancedQueryPanel : public mPanelBase
	{
		Q_OBJECT
	public:
		explicit mPostAdvancedQueryPanel(QWidget* parent = nullptr);

		//更新面板
		void updateAdvancedQueryPanel();

	private:
		//合并QHash
		void mergeHash(QHash<int, double>& hash, QHash<int, double>& hash1, QHash<int, double>& hash2);

		void mergeHash(QHash<int, float>& hash, QHash<int, float>& hash1, QHash<int, float>& hash2);

		void mergeHash(QHash<int, double>& hash, QHash<int, QHash<int, double>>& hash1, QHash<int, QHash<int, double>>& hash2);

		void mergeHash(QHash<int, float>& hash, QHash<int, QHash<int, float>>& hash1, QHash<int, QHash<int, float>>& hash2);

		/** 设置表头 */
		void setTabelHeader();

		/** 判断参数输入合理性 */
		bool isReasonable();

		//清除过滤，显示全部部件
		void clearFilterAndShowAllPart();

		//筛选部件
		void filterPart(QHash<int, std::set<int>>& hash, QPointer<PostData::mPostModelData> modelData, mxr::IPostModelRenderDataProxy* model_proxy, int f, bool flag = true);

		//过滤部件
		void filterPartModel(QList<int> ids, int f);

		//筛选节点/单元
		void filterNodeOrElement(QHash<int, double>& results, PostData::Compones& compone);

		//统一读数入口：native 懒加载（发布后经 postDB 取 Compones）/ mxdb 新旧三分支，
		//失败落输出窗口并返回 false（四块查询组合共用，替代各自散落的读数代码）
		bool readQueryCompones(QPointer<PostData::mPostDataBase> modeldb, const QString& postname,
			const QString& postPathName, int frame, const QString& varstr, const QString& compostr,
			const QString& layerstr, const QString& complex, PostData::Compones& compone);

		//生成部件表格
		void generatePartTable(QHash<int, std::set<int>>& hash, QPointer<PostData::mPostModelData> modelData, mxr::IPostModelRenderDataProxy* model_proxy, MechPreData::mPostGUIData* guidata);

		//生成节点/单元表格
		void generateNodeOrElementTable(QHash<int, double>& results, QPointer<PostData::mPostModelData> modelData, MechPreData::mPostGUIData* guidata, int f, bool flag = true);

		//过滤模型
		void filterModel(double value, int type);

		//添加表格数据 color为最后一项显示颜色
		void addRow(QTableWidget* table, const QStringList& rowData, QColor color);

	private slots:
		/** 筛选类型变化 */
		void slot_applyTypeChange(const QString& text);

		/** 筛选方式变化 */
		void slot_valTypeChange(const QString& text);

		//查询
		void slot_query();

		//排序
		void slot_sortColumn(int col);

	signals:

	protected:
		void hideEvent(QHideEvent* event) override;

	private:
		QVBoxLayout*							_mainLayout;

		QComboBox*								_comboApply;
		QComboBox*								_comboType;
		QComboBox*								_comboValType;
		QLineEdit*								_lineEditValue;
		QCheckBox*								_checkThreshold;
		QSpinBox*								_spinThreshold;
		QComboBox*								_comboRange;
		QPushButton*							_btnQuery;

		bool                                    _isEle = false;             //标识当前云图变量是否作用在单元  true作用在单元 false 作用在节点
		QString                                 _curVar;                    //当前变量
		mTableWidget*							_tableWidget;
	};
}
