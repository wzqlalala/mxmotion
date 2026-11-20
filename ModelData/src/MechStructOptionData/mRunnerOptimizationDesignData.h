#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXStructOptionData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QDataStream>
#include <QVector3D>
#include <QVector>
#include "mModelEnum.h"

namespace MechStructOptionData
{	
	class MXSTRUCTOPTIONDATA_EXPORT mRunnerOptimizationDesignData
	{
		
	public:

		/*
		* 重载构造函数
		*/
		mRunnerOptimizationDesignData(int id, QString name = "");

		/*
		* 析构函数
		*/
		~mRunnerOptimizationDesignData();

		/*
		* 设置ID
		*/
		void setId(int id);

		/*
		* 获取ID
		*/
		int getId();

		/*
		* 设置名
		*/
		void setName(const QString& name);

		/*
		* 获取名
		*/
		QString getName();

		/*
		 * 设置描述
		 */
		void setDescribe(const QString& describe);

		/*
		 * 获取描述
		 */
		QString getDescribe();

		/*
		* 拷贝
		*/
		mRunnerOptimizationDesignData* copyData(const int& id, const QString& name);

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info);

		friend QDataStream& operator>>(QDataStream& in, mRunnerOptimizationDesignData& mgr);
		friend QDataStream& InStreammRunnerOptimizationDesignDataV3(QDataStream& in, mRunnerOptimizationDesignData& obj);
		friend QDataStream& operator<<(QDataStream& out, mRunnerOptimizationDesignData& mgr);

	private:

		//ID
		int _id;

		//名字
		QString _name;

		//描述
		QString _describe;


	public:
		/******************************************设计域属性**************************************************/
		// 设计域类型 ("实体")
		QString _designDomainType;

		/******************************************达西惩罚项**************************************************/
		// 最大渗透率
		bool _maxPermeabilityStatus;
		QString _maxPermeability;

		// 最小渗透率
		bool _minPermeabilityStatus;
		QString _minPermeability;

		/******************************************设计域参数**************************************************/

		//最小成员尺寸
		bool _minMemberStatus;
		QString _minMemberSize;

		//最大成员尺寸
		bool _maxMemberStatus;
		QString _maxMemberSize;

		//最小间隙
		bool _minGapStatus;
		QString _minGapSize;

		/******************************************对称约束**************************************************/
		//对称约束状态
		bool _patternStatus;

		//对称方式(单平面/双平面/三平面/周向/周向+平面)
		QString _patternType;

		//周向对称数量
		QString _patternNum;

		//原点
		int _originId;
		QVector3D _origin;

		//方向点1
		int _dirPoint1Id;
		QVector3D _dirPoint1;

		//方向点2
		int _dirPoint2Id;
		QVector3D _dirPoint2;

		/******************************************拔模约束**************************************************/
		//拔模约束状态
		bool _liftingStatus;

		//拔模方式(单向/双向)
		QString _liftingType;

		//无孔约束方式(开启/关闭)
		QString _nohole;

		//起始点
		int _startPointId;
		QVector3D _startPoint;

		//方向点
		int _dirPointId;
		QVector3D _dirPoint;

		/******************************************挤出约束**************************************************/
	    //拔模约束状态
		bool _extrusionStatus;

		//挤出方式(平行/扭转)
		QString _extrusionType;

		//路径1(存储节点ID)
		QVector<int> _path1;

		//路径2(存储节点ID)
		QVector<int> _path2;

		/******************************************自支撑约束**************************************************/
		// 自支撑约束状态
		bool _supportStatus;

		// 悬垂角（输入角度）
		QString _overhangAngle;

		// 支撑起始点
		int _supportStartPointId;
		QVector3D _supportStartPoint;

		// 支撑方向点
		int _supportDirPointId;
		QVector3D _supportDirPoint;

		/******************************************模式重复**************************************************/
		// 模式重复状态
		bool _patternRepStatus;

		// 设计域类型 (主设计域/从设计域)
		QString _domainType;

		//坐标系(全局坐标系/指定点)
		QString _coordType;

		// 基准点
		int _repBasePointId;
		QVector3D _repBasePoint;

		// 方向点1
		int _repDirPoint1Id;
		QVector3D _repDirPoint1;

		// 方向点2
		int _repDirPoint2Id;
		QVector3D _repDirPoint2;

		// 方向点3
		int _repDirPoint3Id;
		QVector3D _repDirPoint3;

		// 缩放系数 X, Y, Z
		QString _scaleX;
		QString _scaleY;
		QString _scaleZ;
	};
}
