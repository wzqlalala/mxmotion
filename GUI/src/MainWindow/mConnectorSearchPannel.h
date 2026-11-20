#pragma once
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include <QObject>
#include <memory>
#include "mConnector.h"
#include "mPanelBase.h"
#include "mPickWidgetController.h"

//MechModelData
#include "mConnectorRigid.h"
#include "mConnectorFlexible.h"

using namespace MechConnectorData;
namespace MechMainWindow
{
	class ConnectorSearchTable : public QTableWidget
	{
		Q_OBJECT
	public:
		ConnectorSearchTable(QWidget* parent = nullptr);
		~ConnectorSearchTable();

		void setRigidConnData(const QVector<std::shared_ptr<MechConnectorData::mConnectorRigid>>& rigid);
		void setFlexibleConnData(const QVector<std::shared_ptr<MechConnectorData::mConnectorFlexible>>& flexible);

		QWidget* getWidget() { return _widget; }  // 返回带过滤排序按钮的 widget

		/// 操作功能
		void sortByColumnAndOrder(int col, Qt::SortOrder order);
		void filterByColumn(int col, const QString& text);
	
	private:
		//创建连接渲染
		void createRigidConnRend(const QVector<std::shared_ptr<MechConnectorData::mConnectorRigid>>& rigid);
		void createFlexibleConnRend(const QVector<std::shared_ptr<MechConnectorData::mConnectorFlexible>>& flexible);
		void assignConnector(const QString& newsymbol, const QVector3D& masterpos, const QVector<QVector3D>& slavepos, const ConnectorType& conntype);

		//删除连接渲染
		void deleteConnRend(const std::vector<QString>& rendNames);

	protected slots:
		void onTypeChanged(const QString& text);
		void onDeleteClicked();
		void onCreateClicked();
		void onItemSelectionChanged();

	protected:
		void createWidgetUI();
		void refreshTable(); 
		void fillRowFromRigidData(int row, std::shared_ptr<MechConnectorData::mConnectorRigid>connRigid);
		void fillRowFromFlexibleData(int row, std::shared_ptr<MechConnectorData::mConnectorFlexible>connRigid);
		int findRowOfWidget(QWidget* w); // 根据 cell widget 查 row

	private:
		QWidget* _widget = nullptr;

		//刚性连接
		QVector<std::shared_ptr<MechConnectorData::mConnectorRigid>>_connRigid;

		//柔性连接
		QVector<std::shared_ptr<MechConnectorData::mConnectorFlexible>>_connFlexible;

		//渲染连接名
		QSet<QString>_connRendNames;

		// 过滤、排序
		QComboBox* _filterCol;
		QComboBox* _sortCol;
		QComboBox* _sortOrder;
		QLineEdit* _filterText;
	};
	class MMAINWINDOW_EXPORT mConnectorSearchPannel : public mPanelBase
	{
		Q_OBJECT

	public:
		mConnectorSearchPannel(QWidget* parent = nullptr);
		~mConnectorSearchPannel();

		/*
		* 连接搜索编辑
		*/
		void editSearchConnector();

		/*
		* 取消搜索编辑
		*/
		void unEditSearchConnector();

	private:
		
		/*
		* 获取区域拾取的节点ID列表
		*/
		std::vector<int> getPickedNodeIds();

		/*
		* 获取部件拾取的部件ID列表（过滤非网格部件）
		*/
		std::vector<int> getPickedPartIds();

		/*
		* 构造数据
		*/
		void constructConnectorData();

		/*
		* 计算主从节点对,返回部件上的节点ID
		*/
		QHash<int, std::vector<int>> calMain2Subs();
		QHash<int, int> calMain2Sub();

		/*
		* 创建连接名称
		*/
		QString createConnName(const QString&name);

	public slots:
		/*
		* 开始搜索
		*/
		void slot_startSearch();

	private slots:
		/*
		* 连接类型切换
		*/
		void slot_swicthConnType(int i);

		/*
		* 连接方式切换
		*/
		void slot_swicthConnWay(int i);

		/*
		* 容差输入完成
		*/
		void slot_editFinished();

		/*
		* 主/从节点拾取控件激活状态变化
		*/
		void slot_pickActiveChanged(bool isActive);

		/*
		* 主节点区域拾取数据变化
		*/
		void slot_areaDataChanged();

		/*
		* 从节点部件拾取数据变化
		*/
		void slot_partDataChanged();
	private:
		QVBoxLayout* _mainLayout;
		FoldSection* propFold1;
		FoldSection* propFold2;
		FoldSection* propFold3;

		ComboWidget _connType;
		ComboWidget _connWay;

		//通用拾取控件：节点区域（刚性为主节点区域/柔性为从节点区域）
		mPickWidgetController* _areaPickController = nullptr;

		//通用拾取控件：部件（刚性为从节点部件/柔性为主节点部件）
		mPickWidgetController* _partPickController = nullptr;

		InputWidget _allowrance;
		SingleBtn _searchbtn;

		//刚性连接
		QVector<std::shared_ptr<MechConnectorData::mConnectorRigid>>_connRigid;

		//柔性连接
		QVector<std::shared_ptr<MechConnectorData::mConnectorFlexible>>_connFlexible;

		//所有连接名称
		QSet<QString>_allConnNames;

		//表格
		ConnectorSearchTable* _table;
	};

}