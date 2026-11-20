#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCoordinateData_global.h"
#include <map>
#include <vector>
#include <QString>
#include <QStringList>

namespace MechCoordinateData
{
	class mCoordinateBase;
	class MXCOORDINATEDATA_EXPORT mCoordinateManager
	{
	public:
		mCoordinateManager();

		~mCoordinateManager();
	
		/*
		* 通过id获取坐标系数据
		*/
		mCoordinateBase* getCoordinateData(int id);

		/*
		* 通过名称获取坐标系数据
		*/
		mCoordinateBase* getCoordinateData(const QString& name);

		/*
		* 通过id获取名称
		*/
		QString getCoordinateNameByID(int id);

		/*
		* 通过名称获取id
		*/
		int getIDByCoordinateName(QString name);

		/*
		* 添加坐标系数据
		*/
		void AddCoordinateData(mCoordinateBase* base);

		/*
		* 判断坐标系名是否存在
		*/
		bool ExistCoordinateName(const QString& name);

		/*
		* 重命名坐标系
		*/
		void resetName(const QString& oldName, const QString& newName);

		/*
		* 拷贝坐标系
		*/
		int copyCoordinateData(const int& id);

		/*
		* 删除所有坐标系
		*/
		void deleteAllCoordinate();

		/*
		* 删除坐标系
		*/
		void deleteCoordinate(const int& id);

		/*
		* 获取所有坐标系ID(是否包含全局坐标系)
		*/
		std::vector<int> getAllCoordinateID(bool containGlobalCoord = true);

		/*
		* 获取所有坐标系名称
		*/
		QStringList getAllCoordinateNames();

		/*
		* 获取所有坐标系ID组成的string(传递给渲染用)
		*/
		std::vector<QString> getAllCoorinateIDStr();

		/*
		* 获取所有坐标系ID(不包括全局坐标系)组成的string(传递给渲染用)
		*/
		std::vector<QString> getAllCoorinateIDStrExceptGlobal();

		/*
		* 返回创建的新ID
		*/
		int createID();

		friend QDataStream& operator>>(QDataStream& in, mCoordinateManager& mgr);
		friend QDataStream& InStreammCoordinateManagerV3(QDataStream& in, mCoordinateManager& mgr);
		friend QDataStream& operator<<(QDataStream& out, mCoordinateManager& mgr);

		/*
		* 设置所有节点的节点坐标系（统一设置）：传入所有节点ID列表，以及统一参考/分析坐标系ID
		*/
		void setUnifiedNodeCoordinate(const std::vector<int>& nodeIds, int refId, int anaId);

		/*
		* 添加/替换单个节点坐标系数据
		*/
		void addNodeCoordinate(int nodeId, int refId, int anaId);

		/*
		* 获取所有节点坐标系数据
		*/
		std::map<int, std::pair<int, int>> getNodeCoordinateData() const;

		/*
		* 获取所有节点坐标系数据（名称形式，便于 UI 显示）
		*/
		std::map<int, std::pair<QString, QString>> getNodeCoordinateNameData();

		/*
		* 清空节点坐标系数据
		*/
		void clearNodeCoordinateData();

	private:
		std::map<int, mCoordinateBase*> _coordinateData;//id为-1是全局坐标系，不能删除不能编辑不能进行任何操作

		int _maxID;

		// 节点坐标系数据：节点ID -> <参考坐标系ID， 分析坐标系ID>
		std::map<int, std::pair<int, int>> _nodeCoordinateData;
	};
}