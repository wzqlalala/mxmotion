#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXGlobalParamData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QDataStream>

#include "mGenericAssignData.h"
#include "mGenericMatrixData.h"
#include "mModalContriData.h"
#include "mNodeContriData.h"
#include "mPanelContriData.h"
#include "mTPContriData.h"
#include "mERPData.h"
#include "mPeakFilterData.h"
#include "mGenericPartData.h"
#include "mASCouplingSearchData.h"
#include "mSolveControlData.h"

namespace MechGlobalParamData
{	
	class mGenericParamData;
	class mGenericAssignData;
	class mGenericMatrixData;
	class mGenericPartData;
	class mASCouplingSearchData;
	class mSolveControlData;
	class mGlobalOutputControlData;
	class MXGLOBALPARAMDATA_EXPORT mGlobalParam
	{	
	public:

		/*
		* 重载构造函数
		*/
		mGlobalParam();

		/*
		* 析构函数
		*/
		~mGlobalParam();

		/************************超单元参数相关接口开始******************************/
		mGenericParamData* getGenericParamData(int id);
		mGenericParamData* getGenericParamData(const QString& name) const;
		QString getGenericParamNameByID(int id) const;
		int getGenericParamIDByName(const QString& name) const;
		void addGenericParamData(mGenericParamData* body);
		bool existGenericParamName(const QString& name) const;
		void resetGenericParamName(const QString& oldName, const QString& newName);
		int copyGenericParamData(const int& id);
		void deleteGenericParamData(int id);
		void deleteAllGenericParamData();
		std::vector<int> getAllGenericParamIDs() const;
		QStringList getAllGenericParamNames() const;
		int createGenericParamID();
		/************************超单元参数相关接口结束*******************************/



		/************************超单元调用相关接口开始*******************************/
		void deleteGenericAssignData();
		mGenericAssignData* createGenericAssignData();
		mGenericAssignData* getGenericAssignData();
		/***********************超单元调用相关接口结束*****************************/



		/************************超单元矩阵名称相关接口开始*******************************/
		void deleteGenericMatrixData();
		mGenericMatrixData* createGenericMatrixData();
		mGenericMatrixData* getGenericMatrixData();
		/************************超单元矩阵名称相关接口结束*******************************/



		/************************模态贡献量相关接口开始*******************************/
		void deleteModalContriData();
		mModalContriData* createModalContriData();
		mModalContriData* getModalContriData();
		/************************模态贡献量相关接口结束*******************************/



		/************************节点贡献量相关接口开始*******************************/
		void deleteNodeContriData();
		mNodeContriData* createNodeContriData();
		mNodeContriData* getNodeContriData();
		/************************节点贡献量相关接口结束*******************************/

		/************************面板贡献量相关接口开始*******************************/
		void deletePanelContriData();
		mPanelContriData* createPanelContriData();
		mPanelContriData* getPanelContriData();
		/************************面板贡献量相关接口结束*******************************/

		/************************传递路径贡献量相关接口开始*******************************/
		void deleteTPContriData();
		mTPContriData* createTPContriData();
		mTPContriData* getTPContriData();
		/************************传递路径贡献量相关接口结束*******************************/

		/************************等效辐射功率相关接口开始*******************************/
		void deleteERPData();
		mERPData* createERPData();
		mERPData* getERPData();
		/************************等效辐射功率相关接口结束*******************************/

		/************************峰值筛选相关接口开始*******************************/
		mPeakFilterData* getPeakFilterData(int id);
		mPeakFilterData* getPeakFilterData(const QString& name) const;
		QString getPeakFilterNameByID(int id) const;
		int getPeakFilterIDByName(const QString& name) const;
		void addPeakFilterData(mPeakFilterData* body);
		bool existPeakFilterName(const QString& name) const;
		void resetPeakFilterName(const QString& oldName, const QString& newName);
		int copyPeakFilterData(const int& id);
		void deletePeakFilterData(int id);
		void deleteAllPeakFilterData();
		std::vector<int> getAllPeakFilterIDs() const;
		QStringList getAllPeakFilterNames() const;
		int createPeakFilterID();
		/************************峰值筛选相关接口结束*******************************/


		/************************超单元部件相关接口开始*******************************/
		void deleteGenericPartData();
		mGenericPartData* createGenericPartData();
		mGenericPartData* getGenericPartData();
		/***********************超单元部件相关接口结束*****************************/

		/************************声固耦合对搜索相关接口开始*******************************/
		void deleteASCouplingSearchData();
		mASCouplingSearchData* createASCouplingSearchData();
		mASCouplingSearchData* getASCouplingSearchData();
		/***********************声固耦合对搜索相关接口结束*****************************/

		/************************求解控制相关接口开始*******************************/
		void deleteSolveControlData();
		mSolveControlData* createSolveControlData();
		mSolveControlData* getSolveControlData();
		/***********************求解控制相关接口结束*****************************/

		/************************全局输出控制相关接口开始*******************************/
		void deleteGlobalOutputControlData();
		mGlobalOutputControlData* createGlobalOutputControlData();
		mGlobalOutputControlData* getGlobalOutputControlData();
		/***********************全局输出控制相关接口结束*****************************/

		friend QDataStream& operator>>(QDataStream& in, mGlobalParam& mgr);
		friend QDataStream& InStreammGlobalParamV3(QDataStream& in, mGlobalParam& mgr);
		friend QDataStream& operator<<(QDataStream& out, mGlobalParam& mgr);

	private:
		//超单元参数
		int _maxGenericParamID;
		std::map<int, mGenericParamData*> _genericParaData;

		//超单元调用
		mGenericAssignData* _genericAssignData;

		//超单元矩阵名称
		mGenericMatrixData* _genericMatrixData;

		//模态贡献量
		mModalContriData* _modalContriData;

		//节点贡献量
		mNodeContriData* _mNodeContriData;

		//面板贡献量
		mPanelContriData* _mPanelContriData;

		//传递路径贡献量
		mTPContriData* _mTPContriData;

		//等效辐射功率
		mERPData* _mERPData;

		//峰值筛选
		int _maxPeakFilterID;
		std::map<int, mPeakFilterData*> _peakFilterData;

		//超单元部件
		mGenericPartData* _genericPartData;

		//声固耦合对搜索
		mASCouplingSearchData* _ascouplingSearchData;

		//求解控制
		mSolveControlData* _solveControlData;

		//全局输出控制
		mGlobalOutputControlData* _globalOutputControlData;
	};
}