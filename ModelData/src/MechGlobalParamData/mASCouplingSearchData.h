#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXGlobalParamData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QDataStream>
#include <QList>
#include "mModelStruct.h"
#include "mBasicEnum.h"
#include "mModelEnum.h"

namespace MechGlobalParamData
{	
	enum MainSurface
	{
		FLuidToStructure = 1, //声腔面
		StructureToFluid	//结构面
	};

	class MXGLOBALPARAMDATA_EXPORT mASCouplingSearchData
	{
		
	public:

		/*
		* 重载构造函数
		*/
		mASCouplingSearchData();

		/*
		* 析构函数
		*/
		~mASCouplingSearchData();

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info);

		//自动搜索开关
		bool getAutosearch() const { return _autosearch; }
		void setAutosearch(bool val) { _autosearch = val; }

		//主面
		QString getMainSurface() const { return _mainSurface; }
		void setMainSurface(const QString& val) { _mainSurface = val; }

		//耦合面搜索夹角使能
		bool getAngelChecked() const { return _angelChecked; }
		void setAngelChecked(bool val) { _angelChecked = val; }
		QString getSearchAngel() const { return _searchAngel; }
		void setSearchAngel(const QString& val) { _searchAngel = val; }

		//面单元外法线方向延伸比例使能
		bool getOutChecked() const { return _outChecked; }
		void setOutChecked(bool val) { _outChecked = val; }
		QString getOuterScale() const { return _outerScale; }
		void setOuterScale(const QString& val) { _outerScale = val; }

		//面单元内法线方向延伸比例使能
		bool getInnerChecked() const { return _innerChecked; }
		void setInnerChecked(bool val) { _innerChecked = val; }
		QString getInnerScale() const { return _innerScale; }
		void setInnerScale(const QString& val) { _innerScale = val; }

		//面单元横向延伸比例使能
		bool getHorizontalChecked() const { return _horizontalChecked; }
		void setHorizontalChecked(bool val) { _horizontalChecked = val; }
		QString getHorizontalScale() const { return _horizontalScale; }
		void setHorizontalScale(const QString& val) { _horizontalScale = val; }

		//包围盒延伸比例使能
		bool getBoundingBoxChecked() const { return _boundingBoxChecked; }
		void setBoundingBoxChecked(bool val) { _boundingBoxChecked = val; }
		QString getBoundingBoxScale() const { return boundingBoxScale; }
		void setBoundingBoxScale(const QString& val) { boundingBoxScale = val; }
		
		friend QDataStream& operator>>(QDataStream& in, mASCouplingSearchData& mgr);
		friend QDataStream& InStreammASCouplingSearchDataV3(QDataStream& in, mASCouplingSearchData& obj);
		friend QDataStream& operator<<(QDataStream& out, mASCouplingSearchData& mgr);


	public:
		std::vector<QString> _filePath;

		//自动搜索开关
		bool _autosearch; // true-开、false-关
		
		//主面
		QString _mainSurface;

		bool _angelChecked = false;
		//耦合面搜索夹角
		QString _searchAngel;

		bool _outChecked = false;
		//面单元外法线方向延伸比例
		QString _outerScale;

		bool _innerChecked = false;
		//面单元内法线方向延伸比例
		QString _innerScale;

		bool _horizontalChecked = false;
		//面单元横向延伸比例
		QString _horizontalScale;

		bool _boundingBoxChecked = false;
		//包围盒延伸比例
		QString boundingBoxScale;
	};
}