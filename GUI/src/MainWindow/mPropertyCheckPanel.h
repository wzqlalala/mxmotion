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
namespace MechMainWindow
{
	//截面属性面板
	class MMAINWINDOW_EXPORT mPropertyCheckPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mPropertyCheckPanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mPropertyCheckPanel();

		template<typename T>
		T* toType(MechPropertyData::mPropertyBase* base)
		{
			return dynamic_cast<T*>(base);
		};

		/*
		* 新建属性
		*/
		void checkPropertyPanel(int type);

		/*
		* 取消编辑属性
		*/
		void uneditPropertyCheckPanel();


	private slots:

		//部件集合下拉框变化
		void slot_partSetComboBoxChanged(const QString& text);

		//单元集合下拉框变化
		void slot_eleSetComboBoxChanged(const QString& text);

		//添加槽函数
		void slot_allPartBtn();

		//添加部件槽函数
		void slot_addPartBtn();

		//减少部件槽函数
		void slot_reducePartBtn();

		//清空部件槽函数
		void slot_clearPartBtn();

		//所有槽函数
		void slot_allIdBtn();

		//添加槽函数
		void slot_addIdBtn();

		//减少槽函数
		void slot_reduceIdBtn();

		//清空槽函数
		void slot_clearIdBtn();

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

		/*
		* 添加显示部件的属性
		*/
		void appendPartProperty(QString partName);

		//根据QListWidget填充QTableWidgetItem
		void fillEleTableWidget();

		//根据Id填充QListWidgetItem
		void fillListWidget(const QList<int>& ids);

		//获取Id
		QList<int> collectIdsFromListWidget();

		//获取Id
		QList<int> parseIdsFromText(const QString& text);

		//高亮
		void highlightPart(std::vector<int> ids);

		//不高亮
		void unHighlightPart(std::vector<int> ids);

		//高亮
		void highlightEle(std::vector<mxr::MeshSelectInfoData> ids);

		//不高亮
		void unHighlightEle(std::vector<mxr::MeshSelectInfoData> ids);
	protected:

		//总布局
		QVBoxLayout* _mainLayout;

		//当前检查的类型（0：截面属性；1：单元）
		int _checkType;

		//当前视口数据
		MechPreData::mViewportPrivate* _currentViewportPrivate;

		//部件集合下拉框
		QComboBox* _comboPartSet;

		//部件的数据模型
		QStringListModel* _partModel;

		//赋予属性的部件列表
		QListWidget* _listAssignPart;

		//单元集合下拉框
		QComboBox* _comboEleSet;

		//赋予单元的单元列表
		QListWidget* _listAssignEle;

		//属性表格
		QTableWidget* _tableProperty;

		//单元表格
		QTableWidget* _tableEle;

		//拾取信号连接
		QMetaObject::Connection conn;

		//上次拾取部件数据
		std::vector<int> _lastPickPartData;

		//上次拾取单元数据
		std::vector<mxr::MeshSelectInfoData> _lastPickElementData;
	};
}