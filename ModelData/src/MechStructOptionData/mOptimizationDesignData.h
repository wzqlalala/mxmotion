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
	class MXSTRUCTOPTIONDATA_EXPORT mOptimizationDesignData
	{
		
	public:

		/*
		* 重载构造函数
		*/
		mOptimizationDesignData(int id, QString name = "");

		/*
		* 析构函数
		*/
		~mOptimizationDesignData();

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
		mOptimizationDesignData* copyData(const int& id, const QString& name);

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info);

		friend QDataStream& operator>>(QDataStream& in, mOptimizationDesignData& mgr);
		friend QDataStream& InStreammOptimizationDesignDataV3(QDataStream& in, mOptimizationDesignData& obj);
		friend QDataStream& operator<<(QDataStream& out, mOptimizationDesignData& mgr);

	private:

		//ID
		int _id;

		//名字
		QString _name;

		//描述
		QString _describe;


	public:
		/******************************************设计域属性**************************************************/
		// 设计域类型 ("实体" / "壳")
		QString _designDomainType;

		// 优化方式 ("厚度" / "加筋" / "形貌")
		QString _optimizationMethod;

		// 基准厚度
		QString _baseThickness;


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

		//应力约束
		bool _stressConstraintStatus;
		QString _stressConstraintValue;

		/******************************************加强筋参数**************************************************/
		
		// 加强筋高度
		bool _ribHeightStatus;
		QString _ribHeight;

		// 加强筋宽度
		bool _ribWidthStatus;
		QString _ribWidth;


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

		/******************************************形貌参数**************************************************/
		// 起筋参数
		QString _morphUpLimit;      // 上限
		QString _morphDownLimit;    // 下限
		QString _morphInitRatio;    // 初始起筋比

		// 起筋模式
		QString _morphMode;         // 起筋模式
		QString _morphMinWidth;     // 最小宽度
		QString _morphAngle;        // 起筋角度
		QString _morphHeight;       // 起筋高度
		QString _morphDirection;    // 起筋方向

		// 起筋界限
		bool _morphBoundStatus;
		int _morphOriginId;
		QVector3D _morphOrigin;
		int _morphDirPoint1Id;
		QVector3D _morphDirPoint1;
		int _morphDirPoint2Id;
		QVector3D _morphDirPoint2;

		/******************************************形状参数**************************************************/
		// 形状分割选项
		bool _shapeSplitDomain;     // 按域分割
		bool _shapeSplitPart;       // 按部件分割
		bool _shapeKeepHandle;      // 保留控制柄
		bool _shape2DSplit;         // 2D域分割

		// 变形定义
		QString _shapeDeformMode;   // 变形方式：交互/平移/旋转/缩放/移动至节点

		// 平移/缩放参数
		QString _shapeTx;
		QString _shapeTy;
		QString _shapeTz;

		// 旋转参数
		QString _shapeRotAngle;     // 旋转角度
		QVector3D _shapeRotPick;    // 旋转中心点
		QString _shapeRotAxis;      // 旋转轴：X轴/Y轴/Z轴/自定义
		QString _shapeCustomAxis;   // 自定义中心轴

		// 移动至节点
		QString _shapeTargetNode;   // 节点ID

		// 变形参数
		QString _shapeInitValue;    // 初始值
		QString _shapeMinValue;     // 下限值
		QString _shapeMaxValue;     // 上限值
		QString _shapeMoveLimit;    // 移动限制

	};
}
