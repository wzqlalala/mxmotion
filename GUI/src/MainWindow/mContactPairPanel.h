#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <memory>

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QMdiSubWindow>
#include <QStringListModel>
#include <QStackedWidget>
#include <QListWidget>
#include <QLabel>
#include <QTableWidget>

#include "mPanelBase.h"

#include "mGlobalVarible.h"
#include "mPhysicalFieldEnum.h"

using namespace MxFunctions;
namespace MechPropertyData
{
	class mPropertyBase;
	class mTorsionBar;
}
namespace MechPreData
{
	class mViewportPrivate;
}
namespace mxr
{
	struct MeshSelectInfoData;
}
namespace MGeoData
{
    struct mTiedFaces;
}
namespace MechMainWindow
{
	class ContactPairTable : public QTableWidget
	{
		Q_OBJECT
	public:
		ContactPairTable(QWidget* parent = nullptr);
		~ContactPairTable();

		void setAllPartTransparent(bool transparent = true);

		void setData(int method, std::vector<MGeoData::mTiedFaces> faces);

		// 用于 mContactPairPanel 调用
		void setMethod(int method); // method = 1/2/3 决定使用哪组数据

		QWidget* getWidget() { return _widget; }  // 返回带过滤排序按钮的 widget

		/// 操作功能
		void sortByColumnAndOrder(int col, Qt::SortOrder order);
		void filterByColumn(int col, const QString& text);

	protected:
		//void showEvent(QShowEvent *event) override;
		//void hideEvent(QHideEvent* event) override;

	signals:
		void sig_dataChanged(); // 在合并/删除后通知外部

	protected slots:
		void onTypeChanged(const QString& text);
		void onExchangeClicked(); // 使用 sender() 找行
		void onDeleteClicked();
		void onMergeClicked();
		void onCreateClicked();
		void onItemSelectionChanged();

	protected:
		void createWidgetUI();
		void refreshTable(); // 根据 currentMethod 的 data vector 刷新整个表格
		void fillRowFromData(int row, const MGeoData::mTiedFaces& f, int pairIndex); // 填单行，pairIndex 用于默认名称
		int findRowOfWidget(QWidget* w); // 根据 cell widget 查 row
		std::vector<std::vector<MGeoData::mTiedFaces>>& currentFaces(); // 返回当前 method 的 vector 引用
		void computeAutoMergeForFaces2();

	private:
		QWidget* _widget = nullptr;

		// 原始数据
		//std::vector<MGeoData::mTiedFaces> _faces1;
		//std::vector<MGeoData::mTiedFaces> _faces2;
		//std::vector<MGeoData::mTiedFaces> _faces3;
		std::vector<std::vector<MGeoData::mTiedFaces>> _faces1;
		std::vector<std::vector<MGeoData::mTiedFaces>> _faces2;
		std::vector<std::vector<MGeoData::mTiedFaces>> _faces3;

		QColor _mainColor;
		QColor _secondColor;
		// 合并后的数据
		//std::vector<QPair<std::vector<MGeoData::mTiedFaces>, std::vector<MGeoData::mTiedFaces>>> _mergeface1;
		//std::vector<QPair<std::vector<MGeoData::mTiedFaces>, std::vector<MGeoData::mTiedFaces>>> _mergeface2;
		//std::vector<QPair<std::vector<MGeoData::mTiedFaces>, std::vector<MGeoData::mTiedFaces>>> _mergeface3;

		// 过滤、排序
		QComboBox* _filterCol;
		QComboBox* _sortCol;
		QComboBox* _sortOrder;
		QLineEdit* _filterText;
		QComboBox* _methodSelect = nullptr; // 选择 方式1/2/3
		QPushButton* _btnMainColor;
		QPushButton* _btnSecondColor;

		int currentMethod = 1;

		QHash<int, int> _partAlpha;
	};
	//接触对搜索面板
	class MMAINWINDOW_EXPORT mContactPairPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mContactPairPanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mContactPairPanel();

		template<typename T>
		T* toType(MechPropertyData::mPropertyBase* base)
		{
			return dynamic_cast<T*>(base);
		};

		/*
		* 新建属性
		*/
		void editContactPairPanel();

		/*
		* 取消编辑属性
		*/
		void uneditContactPairPanel();


	private slots:

		//开始搜索
		void slot_startSearch();

		//清空部件槽函数
		void slot_clearPartBtn();

		//切换视口	
		void slot_subWindowActivated(int viewportID);

		//拾取信号
		void slot_finishedPick();

	protected:

		/*
		* 连接信号
		*/
		void connectSig();

		/*
		* 断开连接信号
		*/
		void disconnectSig();

		//高亮
		void highlightPart(std::vector<int> ids);

		//不高亮
		void unHighlightPart(std::vector<int> ids);
	protected:

		//总布局
		QVBoxLayout* _mainLayout;

		//当前视口数据
		MechPreData::mViewportPrivate* _currentViewportPrivate;

		QDoubleSpinBox* _toleranceEdit;

		QDoubleSpinBox* _angleEdit;

		//部件内部搜索下拉框
		QComboBox* _comboPartSearch;

		//开始搜索按钮
		QPushButton* _startSearchButton;

		//表格
		ContactPairTable* _table;

		//拾取信号连接
		QMetaObject::Connection conn;

		//上次拾取部件数据
		std::vector<int> _lastPickPartData;
	};
}