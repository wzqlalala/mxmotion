#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXMapRelation_global.h"
#include <unordered_map>
#include <set>
#include <functional>
#include <QList>

#include "mPhysicalFieldEnum.h"
#include "mMapRelationEnum.h"
#include "mMechMapRelationStruct.h"

namespace MechMapRelation
{
	class MXMAPRELATION_EXPORT mMapRelationManager
	{
	public:
		mMapRelationManager();

		~mMapRelationManager();

		friend QDataStream& operator>>(QDataStream& in, mMapRelationManager& obj);
		friend QDataStream& InStreammMapRelationManagerV3(QDataStream& in, mMapRelationManager& mgr);
		friend QDataStream& operator<<(QDataStream& out, mMapRelationManager& obj);
		friend QDataStream& InStreamMapRelationV3(QDataStream& in, mMapRelationManager& obj);

		/*
		* 添加映射关系
		*/
		void appendMapRelation(std::pair<MxFunctions::PhysicalFieldType, int> id1, std::pair<MxFunctions::PhysicalFieldType, int> id2, MxFunctions::MapRelationType type = MxFunctions::MapRelation_None, double scale = 1.0);

		/*
		* 添加表格类型映射关系
		*/
		void appendTableMapRelation(std::pair<MxFunctions::PhysicalFieldType, int> id1, std::pair<MxFunctions::PhysicalFieldType, int> id2, int row, MxFunctions::MapRelationType type = MxFunctions::MapRelation_None);

		/*
		* 删除映射关系
		*/
		void deleteMapRelation(std::pair<MxFunctions::PhysicalFieldType, int> id);
		//该函数用于载荷边界、接触的集合、设计主从域、分析与载荷边界
		void deleteMapRelation(std::pair<MxFunctions::PhysicalFieldType, int> phyData, std::pair<MxFunctions::PhysicalFieldType, int> resultData);

		/*
		* 改变表格类型映射关系
		*/
		void updateTabelMapRelation(std::pair<MxFunctions::PhysicalFieldType, int> id1, std::pair<MxFunctions::PhysicalFieldType, int> id2, int row, MxFunctions::MapRelationType type = MxFunctions::MapRelation_None);

		/*
		* 删除表格类型映射关系
		*/
		void deleteTabelMapRelation(std::pair<MxFunctions::PhysicalFieldType, int> id1, std::pair<MxFunctions::PhysicalFieldType, int> id2, int row);

		/*
		* 查找映射关系
		*/
		template<typename T>
		T getMapRelation(std::pair<MxFunctions::PhysicalFieldType, int> id, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType type = MxFunctions::MapRelation_None);

		/*
		* 清空临时数据
		*/
		void clearDeleteDataMap() { deleteDataMap.clear(); }

		/*
		* 获取临时数据
		*/
		std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>> getDeleteDataMap() { return deleteDataMap; }

		/*
		* 改变该分析下的缩放因子
		*/
		void setScaleByCase(std::pair<MxFunctions::PhysicalFieldType, int> phyData, int caseID, double scale);

		/*
		* 获取该分析下的缩放因子
		*/
		double getScaleByCase(std::pair<MxFunctions::PhysicalFieldType, int> phyData, int caseID);

		/*
		* 拷贝映射关系
		*/
		void copyMapRelation(MxFunctions::PhysicalFieldType type, int oldID, int copyID, int caseID);

		/*
		* 清空多体引用的某一角色（写侧“先清后加”用）
		*/
		void clearMotionRole(MxFunctions::PhysicalFieldType srcType, int srcID,
			MxFunctions::PhysicalFieldType dstType, MxFunctions::MapRelationType role);

		/*
		* 获取坐标系映射
		*/
		std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>> getCoordinateMap(int coorID);

		/*
		* 获取反映射（截面属性、部件、材料）
		*/
		std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>> getPhysicalFieldMap(MxFunctions::PhysicalFieldType type, int phyID);

	private:
		//添加表格映射
		void appendTableMapByRow(std::unordered_map<int, QList<int>>& mapList, MxFunctions::PhysicalFieldType type,
			int phyID, int resultID, std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>>& map, int row);

		//更新表格映射
		void updateTableMapByRow(std::unordered_map<int, QList<int>>& mapList, MxFunctions::PhysicalFieldType type,
			int phyID, int resultID, std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>>& map ,int row);

		//删除表格映射
		void deleteTableMapByRow(std::unordered_map<int, QList<int>>& mapList, MxFunctions::PhysicalFieldType type,
			int phyID, int resultID, std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>>& map, int row);

	private:
		/******************** 多体实体引用映射（通用存储）********************
		* 源类型(Physical_Motion*) -> 源ID -> 角色(目标类型,映射类型) -> 目标ID集合。
		* 同一目标类型有多角色（如运动副的动作体/基本体）靠 MapRelationType 区分；
		* 一对一角色用 MapRelation_None。随管理器一并序列化。
		*********************************************************************/
		using MotionRoleKey = std::pair<MxFunctions::PhysicalFieldType, MxFunctions::MapRelationType>;
		struct MotionRoleHash {
			size_t operator()(const MotionRoleKey& k) const {
				return std::hash<int>()((int(k.first) << 16) ^ (int(k.second) & 0xFFFF));
			}
		};
		std::unordered_map<MxFunctions::PhysicalFieldType,
			std::unordered_map<int,
			std::unordered_map<MotionRoleKey, std::set<int>, MotionRoleHash>>> _motionRefMap;

		//是否为多体源类型（可作 _motionRefMap 的第一级键）
		static bool isMotionSource(MxFunctions::PhysicalFieldType type);

		//追加一条多体引用（同角色集合内去重）
		void appendMotionRef(MxFunctions::PhysicalFieldType srcType, int srcID,
			MxFunctions::PhysicalFieldType dstType, MxFunctions::MapRelationType role, int dstID);

		//删除源实体的全部多体引用
		void deleteMotionRefs(MxFunctions::PhysicalFieldType srcType, int srcID);

		//删除源实体某角色下的一个目标
		void deleteMotionRef(MxFunctions::PhysicalFieldType srcType, int srcID,
			MxFunctions::PhysicalFieldType dstType, MxFunctions::MapRelationType role, int dstID);

		//查询某角色目标集合（不存在返回空）
		const std::set<int>& getMotionRefs(MxFunctions::PhysicalFieldType srcType, int srcID,
			MxFunctions::PhysicalFieldType dstType, MxFunctions::MapRelationType role) const;

		//拷贝源实体的全部多体引用到新ID
		void copyMotionRefs(MxFunctions::PhysicalFieldType srcType, int oldID, int copyID);

	private:
		//添加截面属性-材料，截面类型，坐标系，离散场
		void appendProperty(int propertyID, int matID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType type = MxFunctions::MapRelation_None);

		//删除截面属性
		void deleteProperty(int propertyID);

		//获取截面属性映射关系
		int getPropertyMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType type);

		//添加部件-截面属性
		void appendPart(int partID, int resultID, MxFunctions::PhysicalFieldType phyType);

		//删除部件
		void deletePart(int phyID);

		//添加载荷-集合,坐标系，函数, 分析工况，载荷，边界条件
		void appendLoadBoundary(int phyID, int resultID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType type, double scale);

		//删除载荷
		void deleteLoadBoundary(int phyID);

		//获取载荷映射关系
		int getLoadMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType type);

		//删除材料
		void deleteMaterial(int phyID);

		//添加接触-集合
		void appendContact(int phyID, int resultID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType type);

		//删除接触
		void deleteContact(int phyID);

		//获取接触映射关系
		int getContactMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType type);

		//添加周期性边界-集合
		void appendPeriodicBoundary(int phyID, int resultID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee);

		//删除周期性边界
		void deletePeriodicBoundary(int phyID);

		//获取周期性边界映射关系
		int getPeriodicBoundaryMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee);

		//添加模型修复-集合
		void appendModelFix(int phyID, int resultID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee);

		//删除模型修复
		void deleteModelFix(int phyID);

		//获取模型修复映射关系
		int getModelFixMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee);

		//删除集合
		void deleteSet(int phyID);

		//删除截面形状
		void deletePropertyShape(int phyID);

		//删除函数
		void deleteFunction(int phyID);

		//删除坐标系
		void deleteCoordinate(int phyID);

		//添加坐标系-离散场
		void appendCoordinate(int phyID, int resultID, MxFunctions::PhysicalFieldType phyType);

		//获取坐标系映射关系
		int getCoordinateMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType);

		//删除分析工况
		void deleteCase(int phyID);

		//添加分析工况-分析工况，函数
		void appendCase(int phyID, int resultID, MxFunctions::PhysicalFieldType phyTyp, MxFunctions::MapRelationType typee);

		//获取分析映射关系
		int getCaseMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee);

		//获取分析映射关系
		QList<int> getCaseListMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee);

		//添加连接-材料，集合，坐标系
		void appendConnector(int connectID, int matID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee);

		//删除连接
		void deleteConnector(int phyID);

		//获取连接映射关系
		int getConnectorMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee);

		//添加连接副-坐标系，连接副，部件
		void appendDeputy(int deputyID, int resultID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee);

		//删除连接副
		void deleteDeputy(int phyID);

		//获取连接副映射关系
		int getDeputyMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee);

		//添加预定义场-集合，函数
		void appendPredefinedField(int phyID, int resultID, MxFunctions::PhysicalFieldType phyType);

		//删除预定义场
		void deletePredefinedField(int phyID);

		//获取预定义场映射关系
		int getPredefinedFieldMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee);

		//添加优化目标-分析工况
		void appendOptimationGoal(int phyID, int resultID, MxFunctions::PhysicalFieldType phyTyp);

		//删除优化目标
		void deleteOptimationGoal(int phyID);

		//获取优化目标映射关系
		int getOptimationGoalMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType);

		//添加设计域-截面属性
		void appendOptimationDesign(int phyID, int resultID, MxFunctions::PhysicalFieldType phyTyp);

		//删除设计域
		void deleteOptimationDesign(int phyID);

		//获取设计域映射关系
		int getOptimationDesignMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType);

		//添加优化约束-截面属性
		void appendOptimationConstraint(int phyID, int resultID, MxFunctions::PhysicalFieldType phyTyp);

		//删除优化约束
		void deleteOptimationConstraint(int phyID);

		//获取优化约束映射关系
		int getOptimationConstraintMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType);

		//删除频率点
		void deleteFreq(int phyID);

		//添加层合结构-材料，集合
		void appendLaminate(int phyID, int resultID, MxFunctions::PhysicalFieldType phyTyp);

		//删除层合结构
		void deleteLaminate(int phyID);

		//获取层合结构映射关系
		int getLaminateMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType);

		//添加模态贡献量-集合
		void appendModalContri(int phyID, int resultID, MxFunctions::PhysicalFieldType phyTyp, MxFunctions::MapRelationType typee);

		//删除模态贡献量
		void deleteModalContri(int phyID);

		//获取模态贡献量映射关系
		int getModalContriMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee);

		//添加节点贡献量-集合
		void appendNodeContri(int phyID, int resultID, MxFunctions::PhysicalFieldType phyTyp, MxFunctions::MapRelationType typee);

		//删除节点贡献量
		void deleteNodeContri(int phyID);

		//获取节点贡献量映射关系
		int getNodeContriMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee);

		//添加传递路径贡献量-集合
		void appendTPContri(int phyID, int resultID, MxFunctions::PhysicalFieldType phyTyp, MxFunctions::MapRelationType typee);

		//删除传递路径贡献量
		void deleteTPContri(int phyID);

		//获取传递路径贡献量映射关系
		int getTPContriMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee);

		//添加面板贡献量-集合
		void appendPanelContri(int phyID, int resultID, MxFunctions::PhysicalFieldType phyTyp, MxFunctions::MapRelationType typee);

		//删除面板贡献量
		void deletePanelContri(int phyID);

		//获取面板贡献量映射关系
		int getPanelContriMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee);

		//删除非结构质量
		void deleteNSM(int phyID);

		//添加转子动力-部件、分析工况
		void appendRotateAnalyse(int phyID, int resultID, MxFunctions::PhysicalFieldType phyType);

		//删除转子动力
		void deleteRotateAnalyse(int phyID);

		//获取转子动力映射关系
		int getRotateAnalyseMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType);

		//删除峰值筛选
		void deletePeakFilter(int phyID);

		//删除超单元参数
		void deleteGenericParam(int phyID);

		//删除超单元部件
		void deleteGenericPart(int phyID);

		//删除离散场
		void deleteDiscreteField(int phyID);

		//添加微观组分-材料、函数
		void appendMicroconstituent(int phyID, int resultID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee);

		//添加分析场-坐标系，集合（坐标系/目标集合不存数据类，仅作映射关系）
		void appendField(int phyID, int resultID, MxFunctions::PhysicalFieldType phyType);

		//获取分析场映射关系
		int getFieldMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType);

		//删除微观组分
		void deleteMicroconstituent(int phyID);

		//获取微观组分
		int getMicroconstituentMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee);
	private:
		//截面属性 
		std::unordered_map<int, int> _propertyToMat;		//材料
		std::unordered_map<int, QList<int>> _propertyToListMat;		//多个材料
		std::unordered_map<int, int> _propertyToShape;	//截面类型
		std::unordered_map<int, QList<int>> _propertyToListShape;	//多个截面类型
		std::unordered_map<int, int> _propertyToCoordinate;		//坐标系
		std::unordered_map<int, int> _propertyToLaminate;		//层合板
		std::unordered_map<int, MxFunctions::mDiscreteFieldMappingData> _propertyToDiscreteField;  //离散场

		//部件 截面属性
		std::unordered_map<int, int> _partToProperty;

		//超单元部件 部件
		std::unordered_map<int, QList<int>> _genericPartToListPart;//部件（不重复）

		//载荷边界
		std::unordered_map<int, int> _loadBcToSet;  //集合
		std::unordered_map<int, int> _loadBcToCoordinate;  //坐标系
		std::unordered_map<int, MxFunctions::mFunctionMappingData> _loadBcToFunction;  //函数
		std::unordered_map<int, std::unordered_map<int, double>> _loadBcToCase;  //分析工况
		std::unordered_map<int, int> _loadBcToLoadBC;  //载荷边界
		std::unordered_map<int, QList<int>> _loadToListLoad;	//多个载荷边界
		std::unordered_map<int, QList<int>> _loadBcToListFunction;  //多个函数
		std::unordered_map<int, int> _loadBcToConnector;   //连接
		std::unordered_map<int, MxFunctions::mAnalysisFieldMappingData> _loadBcToAnalysisField;   //分析场（热学载荷）

		//分析工况
		std::unordered_map<int, int> _caseToFunction;  //函数
		std::unordered_map<int, int> _caseToCase;  //分析工况
		std::unordered_map<int, MxFunctions::mSetMappingData> _caseToSet;  //集合
		std::unordered_map<int, QList<MxFunctions::mRandomCaseMappingData>> _RandomCase; //随机振动分析（分析、分析、函数）
		std::unordered_map<int, QList<int>> _caseToListLoadBC;  //响应谱分析（载荷）
		std::unordered_map<int, QList<int>> _caseToListFreq;  //频率点
		std::unordered_map<int, QList<int>> _caseToListNSM;  //非结构质量（不重复）
		std::unordered_map<int, QList<int>> _caseToListModelFix;  //模型修复（不重复）
		std::unordered_map<int, QList<int>> _caseToListPeakFilter;  //峰值筛选（不重复）
		std::unordered_map<int, int> _caseToGenericParam;  //超单元参数

		//连接
		std::unordered_map<int, int> _connectorToMat; //材料
		std::unordered_map<int, MxFunctions::mMappingData> _connectorToSet; //集合
		std::unordered_map<int, MxFunctions::mCoordMappingData> _connectorToCoordinate; //坐标系

		//连接副
		std::unordered_map<int, int> _deputyToCoordinate; //坐标系
		std::unordered_map<int, int> _deputyToDeputy; //连接副
		std::unordered_map<int, QList<int>> _deputyToListPart;  //部件（不重复）

		//预定义场
		std::unordered_map<int, int> _predefinedFieldToSet; //集合
		std::unordered_map<int, int> _predefinedFieldToFunction; //函数
		std::unordered_map<int, std::unordered_map<int, double>> _predefinedFieldToCase;  //分析工况

		//分析场（Physical_Field）
		std::unordered_map<int, int> _fieldToCoordinate; //坐标系（-1 = 全局坐标系）
		std::unordered_map<int, int> _fieldToSet; //目标集合（-1 = 整个模型）

		//接触 
		std::unordered_map<int, MxFunctions::mMappingData> _contactToSet;  //集合
		std::unordered_map<int, int> _contactToContact; //接触
		std::unordered_map<int, std::unordered_map<int, double>> _contactToCase;  //分析工况
		std::unordered_map<int, int> _contactToFunction; //函数

		//转子动力
		std::unordered_map<int, int> _rotateToPart;  //部件
		std::unordered_map<int, std::unordered_map<int, double>> _rotateToCase;  //分析工况

		//周期性边界
		std::unordered_map<int, MxFunctions::mMappingData> _periodicBoundaryToSet;  //集合

		//层合结构
		std::unordered_map<int, int> _laminateToMat;  //材料
		std::unordered_map<int, int> _laminateToSet;  //集合
		std::unordered_map<int, int> _laminateToDiscreteField;  //离散场
		std::unordered_map<int, QList<int>> _laminateToLayerList;  //层合板（不重复）

		//优化目标 
		std::unordered_map<int, int> _goalToCase;  //分析工况
		std::unordered_map<int, QList<int>> _goalToListCase;	//分析工况

		//设计域
		std::unordered_map<int, int> _designToProperty;  //截面属性
		std::unordered_map<int, int> _designToSet;		 //集合
		std::unordered_map<int, int> _designToDesign;    //设计域

		//优化约束
		std::unordered_map<int, int> _constraintToProperty;  //截面属性
		std::unordered_map<int, int> _constraintToCase;  //分析工况

		//集合
		std::unordered_map<int, QList<int>> _setToListSet;	//集合（不重复）

		//坐标系
		std::unordered_map<int, int> _coordinateToDiscreteField;  //离散场

		//模态贡献量
		std::unordered_map<int, MxFunctions::mModalContriMappingData> _modalContriToSet; //集合
		
		//节点贡献量
		std::unordered_map<int, MxFunctions::mNodeContriMappingData> _nodeContriToSet; //集合

		//传递路径贡献量
		std::unordered_map<int, MxFunctions::mTPContriMappingData> _tpContriToSet; //集合

		//面板贡献量
		std::unordered_map<int, MxFunctions::mPanelContriMappingData> _panelContriToSet; //集合

		//模型修复
		std::unordered_map<int, int> _modelFixToSet; //集合

		//微观组分
		std::unordered_map<int, MxFunctions::mMatMappingData> _microconstituentToMat; //材料
		std::unordered_map<int, int> _microconstituentToFunction; //函数

		//部件 超单元部件 转子动力
		std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>> _partMap;

		//集合     载荷边界 接触 分析 连接 集合 模态贡献量 节点贡献量 传递路径贡献量 面板贡献量 非结构质量 周期性边界 设计域
		std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>> _setMap;

		//坐标系   载荷边界 截面属性 连接
		std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>> _coordinateMap;

		//函数   载荷边界 分析工况 微观组分
		std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>> _functionMap;

		//分析工况   载荷边界 分析工况 优化目标 预定义场 接触 转子动力
		std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>> _caseMap;

		//截面属性 部件 设计域 非结构质量
		std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>> _propertyMap;

		//材料     截面属性 连接 微观组分
		std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>> _matMap;

		//截面类型  截面属性
		std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>> _propertyShapeMap;

		//载荷边界  载荷边界 分析
		std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>> _loadBcMap;

		//连接  载荷边界
		std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>> _connectorMap;

		//频率点  分析
		std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>> _freqMap;

		//层合结构  层合板
		std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>> _laminateMap;

		//非结构质量  分析
		std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>> _nsmMap;

		//模型修复 分析
		std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>> _modelFixMap;

		//峰值筛选  分析
		std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>> _peakFilterMap;

		//超单元参数  分析
		std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>> _genericParamMap;

		//连接副  连接副
		std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>> _deputyMap;

		//离散场 截面属性 坐标系 层合结构
		std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>> _discreteFieldMap;

		//接触 接触
		std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>> _contactMap;

	private:
		//临时数据
		std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>> deleteDataMap;
	};

	template<>
	inline int mMapRelationManager::getMapRelation<int>(std::pair<MxFunctions::PhysicalFieldType, int> id, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType type)
	{
		int firstID = id.second;
		switch (id.first)
		{
		case MxFunctions::Physical_LoadBoundary:
			return getLoadMapRelation(firstID, phyType, type);
		case MxFunctions::Physical_Property:
			return getPropertyMapRelation(firstID, phyType, type);
		case MxFunctions::Physical_Part:
			if (phyType == MxFunctions::Physical_Property)
			{
				if (_partToProperty.find(firstID) == _partToProperty.cend())
					return -1;

				return _partToProperty[firstID];
			}
			break;
		case MxFunctions::Physical_Contact:
			return getContactMapRelation(firstID, phyType, type);
		case MxFunctions::Physical_Connector:
			return getConnectorMapRelation(firstID, phyType, type);
		case MxFunctions::Physical_Case:
			return getCaseMapRelation(firstID, phyType, type);
		case MxFunctions::Physical_OptimationGoal:
			return getOptimationGoalMapRelation(firstID, phyType);
		case MxFunctions::Physical_OptimationDesign:
			return getOptimationDesignMapRelation(firstID, phyType);
		case MxFunctions::Physical_OptimationConstraint:
			return getOptimationConstraintMapRelation(firstID, phyType);
		case MxFunctions::Physical_Laminate:
			return getLaminateMapRelation(firstID, phyType);
		case MxFunctions::Physical_ModalContri:
			return getModalContriMapRelation(firstID, phyType, type);
		case MxFunctions::Physical_NodeContri:
			return getNodeContriMapRelation(firstID, phyType, type);
		case MxFunctions::Physical_TPContri:
			return getTPContriMapRelation(firstID, phyType, type);
		case MxFunctions::Physical_PanelContri:
			return getPanelContriMapRelation(firstID, phyType, type);
		case MxFunctions::Physical_Deputy:
			return getDeputyMapRelation(firstID, phyType, type);
		case MxFunctions::Physical_PredefinedField:
			return getPredefinedFieldMapRelation(firstID, phyType, type);
		case MxFunctions::Physical_Coordinate:
			return getCoordinateMapRelation(firstID, phyType);
		case MxFunctions::Physical_Field:
			return getFieldMapRelation(firstID, phyType);
		case MxFunctions::Physical_PeriodicBoundary:
			return getPeriodicBoundaryMapRelation(firstID, phyType, type);
		case MxFunctions::Physical_ModelFix:
			return getModelFixMapRelation(firstID, phyType, type);
		case MxFunctions::Physical_RotateAnalyse:
			return getRotateAnalyseMapRelation(firstID, phyType);
		default:
			//多体实体引用：返回该角色下首个目标（空集返回-1）
			if (isMotionSource(id.first))
			{
				const auto& refs = getMotionRefs(id.first, firstID, phyType, type);
				if (!refs.empty())
					return *refs.begin();
			}
			break;
		}

		return -1;
	}


	template<>
	inline QList<int> mMapRelationManager::getMapRelation<QList<int>>(std::pair<MxFunctions::PhysicalFieldType, int> id, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType type)
	{
		QList<int> resultData;
		int phyID = id.second;
		switch (id.first)
		{
		case MxFunctions::Physical_Property:
			if (phyType == MxFunctions::Physical_Material)
			{
				if (_propertyToListMat.find(phyID) != _propertyToListMat.cend())
				{
					resultData = _propertyToListMat[phyID];
				}
			}
			else if (phyType == MxFunctions::Physical_Part)
			{
				if (_propertyMap.find(phyID) != _propertyMap.cend() && _propertyMap[phyID].find(MxFunctions::Physical_Part) != _propertyMap[phyID].cend())
				{
					for (const auto& it : _propertyMap[phyID][MxFunctions::Physical_Part])
					{
						resultData.append(it);
					}
				}
			}
			else if (phyType == MxFunctions::Physical_PropertyShape)
			{
				if (_propertyToListShape.find(phyID) != _propertyToListShape.cend())
				{
					resultData = _propertyToListShape[phyID];
				}
			}
			break;
		case MxFunctions::Physical_LoadBoundary:
			if (phyType == MxFunctions::Physical_LoadBoundary)
			{
				if (_loadToListLoad.find(phyID) != _loadToListLoad.cend())
				{
					resultData = _loadToListLoad[phyID];
				}
			}
			else if (phyType == MxFunctions::Physical_Function)
			{
				if (_loadBcToListFunction.find(phyID) != _loadBcToListFunction.cend())
				{
					resultData = _loadBcToListFunction[phyID];
				}
			}
			break;
		case MxFunctions::Physical_Case:
			return getCaseListMapRelation(phyID, phyType, type);
		case MxFunctions::Physical_Laminate:
			if (phyType == MxFunctions::Physical_Laminate)
			{
				if (_laminateToLayerList.find(phyID) != _laminateToLayerList.cend())
				{
					resultData = _laminateToLayerList[phyID];
				}
			}
			break;
		case MxFunctions::Physical_GenericPart:
			if (phyType == MxFunctions::Physical_Part)
			{
				if (_genericPartToListPart.find(phyID) != _genericPartToListPart.cend())
				{
					resultData = _genericPartToListPart[phyID];
				}
			}
			break;
		case MxFunctions::Physical_Set:
			if (phyType == MxFunctions::Physical_Set)
			{
				if (_setToListSet.find(phyID) != _setToListSet.cend())
				{
					resultData = _setToListSet[phyID];
				}
			}
			break;
		case MxFunctions::Physical_Deputy:
			if (phyType == MxFunctions::Physical_Part)
			{
				if (_deputyToListPart.find(phyID) != _deputyToListPart.cend())
				{
					resultData = _deputyToListPart[phyID];
				}
			}
			break;
		default:
			//多体实体引用：返回该角色下的目标集合
			if (isMotionSource(id.first))
			{
				const auto& refs = getMotionRefs(id.first, phyID, phyType, type);
				for (int v : refs)
					resultData.append(v);
			}
			break;
		}

		return resultData;
	}

	template<>
	inline std::set<int> mMapRelationManager::getMapRelation<std::set<int>>(std::pair<MxFunctions::PhysicalFieldType, int> id, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType type)
	{
		std::set<int> resultData;
		int phyID = id.second;
		switch (id.first)
		{
		case MxFunctions::Physical_Case:
			if (_caseMap.find(phyID) != _caseMap.cend())
			{
				resultData = _caseMap[phyID][phyType];
			}
			break;
		case MxFunctions::Physical_Set:
			if (_setMap.find(phyID) != _setMap.cend())
			{
				return _setMap[phyID][phyType];
			}
		default:
			//多体实体引用：返回该角色下的目标集合
			if (isMotionSource(id.first))
				return getMotionRefs(id.first, phyID, phyType, type);
			break;
		}

		return resultData;
	}

	template<>
	inline std::unordered_map<int, double> mMapRelationManager::getMapRelation<std::unordered_map<int, double>>(std::pair<MxFunctions::PhysicalFieldType, int> id, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType type)
	{
		std::unordered_map<int, double> resultData;
		int phyID = id.second;
		switch (id.first)
		{
		case MxFunctions::Physical_LoadBoundary:
			if (phyType == MxFunctions::Physical_Case)
			{
				if (_loadBcToCase.find(phyID) != _loadBcToCase.cend())
				{
					resultData = _loadBcToCase[phyID];
				}
			}
			break;
		case MxFunctions::Physical_PredefinedField:
			if (phyType == MxFunctions::Physical_Case)
			{
				if (_predefinedFieldToCase.find(phyID) != _predefinedFieldToCase.cend())
				{
					resultData = _predefinedFieldToCase[phyID];
				}
			}
			break;
		case MxFunctions::Physical_Contact:
			if (phyType == MxFunctions::Physical_Case)
			{
				if (_contactToCase.find(phyID) != _contactToCase.cend())
				{
					resultData = _contactToCase[phyID];
				}
			}
			break;
		case MxFunctions::Physical_RotateAnalyse:
			if (phyType == MxFunctions::Physical_Case)
			{
				if (_rotateToCase.find(phyID) != _rotateToCase.cend())
				{
					resultData = _rotateToCase[phyID];
				}
			}
			break;
		default:
			break;
		}

		return resultData;
	}

}

