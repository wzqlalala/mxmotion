#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
/**************************************************************************
版    权:	2021 MaiXi SoftWare 保留所有权利.
开发人员:    王飞奇
日    期:    2021年6月29日18:51:14
版    本：   MxSimV1
开发环境：	VS2017+QT5.13.0+OCC7.3.0
功能描述:    网格剖分多线程
历史版本：   1
1.
日    期：   2021年6月29日18:51:14
开发人员：   王飞奇
修改内容：   第一次创建
2.
**************************************************************************/
#include <QObject>
#include <QThread>
#include <QHash>
#include"mModelData.h"
#include"mViewportModelManager.h"
#include"mViewportModel.h"
//【OCC摘除·S10/P0-1 终局2】OCC 重头（MeshGeoData 双路径基建，162 活跃行）
// 经 mMeshThread.h 污染全部 mesh 面板 TU；本头仅指针用法（setMeshEntity），前置声明即可。
// cpp 侧需完整类型处自行 include。
//#include"MeshEntity.h"
class MeshEntity;
#include "MXGeoEntity.h" //【OCC摘除·S10/P0-1 终局2】MeshParam/MeshShapeType 中性定义（原经 MeshEntity.h 传递）
#include "mBatchMeshTask.h" //【rebase 裁决·union】队友批处理剖分（9bec0481）依赖
//#include "mGeoDataModelSet.h"
//#include "MXMesh.h"
//#include "mMeshMain.h"
class PreInterface;

namespace MBasicFunction
{
	class mOutputMessage;
}

using namespace MBasicFunction;
namespace MGUIMesh
{
	//网格剖分多线程
	class mMeshThread : public QObject
	{
		Q_OBJECT

	public:
		/*************************************************
		函数名称：MeshThread
		函数功能：构造函数
		输入参数：QObject
		输出参数：无
		*************************************************/
		mMeshThread(QObject *parent);
		/*************************************************
		函数名称：~MeshThread
		函数功能：析构函数
		输入参数：无
		输出参数：无
		*************************************************/
		~mMeshThread();

		/*************************************************
		函数名称：setMeshParameter
		函数功能：将面板中的参数传到该类中来，用于网格剖分（一维，二维，三维剖分）
		输入参数：网格尺寸 lenth,单元类型 type，网格维度 dim
		输出参数：无
		*************************************************/
		void setMeshParameter(double lenth, int type, int dim, int meshorder, set<int> GeoIDs);

		/*************************************************
		函数名称：setMeshParameter
		函数功能：将面板中的参数传到该类中来，用于网格剖分（一维，二维，三维剖分）
		输入参数：网格最大尺寸,网格最小尺寸,是否使用光滑,是否使用曲率,是否使用邻近,光滑率,曲率,邻近度
		输出参数：无
		*************************************************/
		void setMeshParameter(double lcMax, double lcMin, bool smoothBool, bool curvatureBool, bool proximityBool, double smoothFactor, double curvatureFac, double proximityFac, int type, int dim, int meshorder, set<int> GeoIDs);

		/*************************************************
		函数名称：setSolidMeshParameter
		函数功能：将面板中的参数传到该类中来，用于网格剖分（一维，二维，三维剖分）
		输入参数：网格最大尺寸,网格最小尺寸,是否使用光滑,是否使用曲率,是否使用邻近,光滑率,曲率,邻近度
		输出参数：无
		*************************************************/
		void setSolidMeshParameter(double lcMax , int meshorder, set<int> GeoIDs,int dim);

        //网格多子节点生成-获取所有子节点参数表
        void getAllMeshParamMap();
        //获取所有自适应子节点参数表
        void getAutoResultMap(std::unordered_map<int, std::vector<MeshParam>>& solidAutoMap, std::unordered_map<int, MeshParam>& autoResultMap);

		/*
		* 边抑制/缝合数据设置
		*/
		void setSutureData(bool isSuppress, set<int> ids) { _isSuppress = isSuppress; _sutureID = ids; }

		/*当前toggle边赋值*/
		void setSutureID(set<int> ids) { _sutureID = ids; }

		/*获取当前toggle边*/
		set<int> getSutereID() { return _sutureID; }

	public slots:
		//一键生成
		void slot_createMeshAuto();

		//根据partId生成网格
		void slot_createMeshByPartId(vector<int> partIds);

		//根据子节点id生成网格,线面体
		void slot_createMesh(int id, QStringList list);

		//根据网格设置生成
		void slot_createMeshBySetting();

		//根据全局尺寸生成
		void slot_createMesh1();
		
		//网格自适应子节点
		void slot_createMeshAutoAdapt(int dim, const std::set<int>& geo_ids, MeshParam mesh_param);

		//自定义加密子节点
		void slot_createMeshCustomEncryption();

        //局部尺寸子节点
        void slot_createMeshLocalSize(int id, QStringList list, std::map<int, double> ids2SizesMap);
        //边抑制子节点
        void slot_createMeshEdgeSuppress(int id, QStringList list, bool isSuppress);
        /*
		* 几何面剖分接口
		* @IdAndSize  第一个int为面id,第二个为网格尺寸
		* @index  0三角形，1四边形
		* @Dim    剖分维度
		* @order  阶数
		*/
		void slot_meshForMidSurface(QHash<int, double> IdAndSize, int index, int Dim, int order);

		/*************************************************
		函数名称：slot_generateMesh
		函数功能：生成网格
		输入参数：mode ,第一次剖分为0，大于一次剖分为1
		输出参数：无
		*************************************************/
		void slot_generateMesh(int mode);

		/*************************************************
		函数名称：slot_generateDirectMesh
		函数功能：直接生成网格避免异步问题（create by ysq）
		输入参数：mode ,第一次剖分为0，大于一次剖分为1
		输出参数：无
		*************************************************/
		void slot_generateDirectMesh(int mode);

		/*************************************************
		函数名称：slot_adjustLineNodeNum
		函数功能：调整网格
		输入参数：mode中0为左键加，1为右键减
		输出参数：无
		*************************************************/
		void slot_adjustLineNodeNum(int mode,int lineId);

		/*************************************************
		函数名称：slot_adjustFromNodeNum
		函数功能：通过节点个数调整网格
		输入参数：oneorall中0为一条边，1为全部边，num为线上的节点个数
		输出参数：无
		*************************************************/
		void slot_adjustFromNodeNum(int oneorall, int num, int lineId);

		/*************************************************
		函数名称：slot_adjustFromAdjustSizeNum
		函数功能：通过尺寸调整网格
		输入参数：oneorall中0为一条边，1为全部边，size为尺寸
		输出参数：无
		*************************************************/
		void slot_adjustFromAdjustSizeNum(int oneorall, double size);

		/*************************************************
		函数名称：slot_biasLineFactor
		函数功能：偏置网格节点
		输入参数：mode中0为左键加，1为右键减
		输出参数：无
		*************************************************/
		void slot_biasLineFactor(int mode, int lineId);

		/*************************************************
		函数名称：slot_biasFromFactor
		函数功能：偏置网格节点
		输入参数：oneorall中0为一条边，1为全部边，factor为偏置因子
		输出参数：无
		*************************************************/
		void slot_biasFromFactor(int oneorall, double factor, int LineId);

		/*************************************************
		函数名称：slot_saveMeshDataToDataBase
		函数功能：保存剖分网格后的数据到数据库中
		输入参数：dim为保存的维度
		输出参数：无
		*************************************************/
		void slot_saveMeshDataToDataBase(int dim);

		/*************************************************
		函数名称：slot_rejectMesh
		函数功能：撤销当前的网格剖分，删除数据
		输入参数：dim为保存的维度
		输出参数：无
		*************************************************/
		void slot_rejectMesh(int dim);

        /*************************************************
        函数名称：slot_createMeshBatch
        函数功能：批量剖分
        输入参数：task为批量任务配置（maxSize>0 时覆盖自动计算尺寸）
        输出参数：无
        *************************************************/
        void slot_createMeshBatch(const MGUIMesh::BatchMeshTask& task);

	private:
		/*************************************************
		函数名称：SaveMeshDataToMeshData
		函数功能：保存网格数据到临时数据库中,用在第一次剖分时
		输入参数：模型set
		输出参数：无
		*************************************************/
		//void SaveMeshDataToMeshData(MDataGeo::GeoDataModelSet* set, int i = 0);

		/*************************************************
		函数名称：SaveMeshDataToMeshData
		函数功能：保存网格数据到临时数据库中,用在大于一次剖分时
		输入参数：无
		输出参数：无
		*************************************************/
		void SaveMeshDataToMeshData();

		/*************************************************
		函数名称：SaveTempNodeData
		函数功能：保存剖分网格的临时节点数据
		输入参数：无
		输出参数：无
		*************************************************/
		void SaveTempNodeData(int i = 0);


	signals:
		// 边抑制/缝合请求，由工作线程发出，由主线程的 mMeshPanel 接收并转发给 mGlobalSignals
		void meshSutureRequested(bool isSuppress, std::set<int> geoIds);

		//剖分完成，ensureMesh
		void done(const QString& result, vector<int> partIds = {});
		/*************************************************
		函数名称：outputMessageSig
		函数功能：二维剖分的时候，最后点确定保存数据的时候保存1维的线单元数据到临时数据库中
		输入参数：无
		输出参数：无
		*************************************************/
		void outputMessageSig(const QString& msg);

		/*************************************************
		函数名称：finishedMeshSig
		函数功能：剖分完网格后发送信号激活面板按钮
		输入参数：无
		输出参数：dim
		*************************************************/
		void finishedMeshSig(int dim);

		/*************************************************
		函数名称：finishedDirectMeshSig
		函数功能：直接剖分完网格后发送信号
		输入参数：无
		输出参数：dim
		*************************************************/
		void finishedDirectMeshSig(int dim);

		/*************************************************
		函数名称：finishedOKSig
		函数功能：确定完网格后发送信号激活面板按钮
		输入参数：无
		输出参数：type
		*************************************************/
		void finishedOKSig(int dim);

		/*调整偏置完成后发送信号
		* @dim 维度
		* @num 0调整 1偏置
		*/
		void finishedAdjust(int dim, int num, int LineID);

		/*
		*调整或者偏置失败信号
		*/
		void gameOverSig(int dim);

		/*
		* 存储待剖分几何对象的对象
		*/
		void setMeshEntity(MeshEntity*entity);

	private:
		//*********************************************几何ID****************************************//
		set<int> _GeoIDs;
		//*********************************************当前toggle边ID********************************//
		set<int> _sutureID;
		//*********************************************是否抑制（false 为缝合）************************//
		bool _isSuppress = false;
		//*********************************************模型set表****************************************//
		//QList<MDataGeo::GeoDataModelSet*> _setList;
		//*********************************************网格最大尺寸****************************************//
		double _lcMax = -1;
		//*********************************************网格最小尺寸****************************************//
		double _lcMin = -1;
		//*********************************************是否使用光滑****************************************//
		bool _smoothBool = false;
		//*********************************************是否使用曲率****************************************//
		bool _curvatureBool = false;
		//*********************************************是否使用邻近****************************************//
		bool _proximityBool = false;
		//*********************************************光滑率****************************************//
		double _smoothFactor = -1;
		//*********************************************曲率****************************************//
		double _curvatureFac = -1;
		//*********************************************邻近度****************************************//
		double _proximityFac = -1;
		//*********************************************单元类型****************************************//
		int _type = -1;
		//*********************************************网格维度****************************************//
		int _dim = -1;
		//*********************************************网格线程类型************************************//
		//MDataGeo::MeshThreadType _MeshThreadType = MDataGeo::GenerateMesh;
		PreInterface *_mesh = nullptr;
		QVector<PreInterface*>_vMesh;
		//********************************网格剖分过程中的部件编号和部件名称*****************************//
		QHash<int, QString> _partIDName;
		//**********************************上次确定剖分的GModel编号*************************************//
		int _lastNumberGmodel{ 0 };
		//**********************************剖分的阶次*************************************//
		int _meshorder = 1;

		QStringList _finishMeshList;
        //**********************************子节点网格参数Map*************************************//
        std::unordered_map<int, std::pair<double, MeshShapeType>> solidResultMap;
        std::unordered_map<int, std::pair<double, MeshShapeType>> faceResultMap;
        std::unordered_map<int, double> lineResultMap;
        std::unordered_map<int, double> localResultMap;
        std::unordered_map<int, MeshParam> solidAutoResultMap;
        std::unordered_map<int, MeshParam> faceAutoResultMap;
        std::unordered_map<int, MeshParam> lineAutoResultMap;
public:
		MechModelData::mModelData* modelData = getCVM->getModelData();
		mMeshData* meshdata = modelData->getMeshData();
	};
}
