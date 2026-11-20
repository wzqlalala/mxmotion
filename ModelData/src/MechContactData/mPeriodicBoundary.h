#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXContactData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mModelEnum.h"
#include "mModelStruct.h"

#include <QString>
#include <QVector3D>

using namespace MxFunctions;
namespace MechContactData
{
	class MXCONTACTDATA_EXPORT mPeriodicBoundary
	{
	public:
		/*
		* 含参构造函数
		* id       编号
		* type     周期性边界类型（平移边界/旋转边界）
		* name     名称
		* isUserRename 是否用户自定义名称
		*/
		mPeriodicBoundary(int id, MxFunctions::PeriodicBoundaryType type, const QString& name = "", bool isUserRename = false);

		/*
		* 析构函数
		*/
		~mPeriodicBoundary();

		/*
		* 设置ID
		*/
		void setPeriodicBoundaryId(int id);

		/*
		* 获取ID
		*/
		int getPeriodicBoundaryId() const;

		/*
		* 获取是否为用户重命名
		*/
		bool getIsUserRename() const;

		/*
		* 设置名称
		*/
		void setPeriodicBoundaryName(const QString& name, bool isUserRename = true);

		/*
		* 获取名称
		*/
		QString getPeriodicBoundaryName() const;

		/*
		* 设置类型
		*/
		void setPeriodicBoundaryType(MxFunctions::PeriodicBoundaryType type);

		/*
		* 获取类型
		*/
		MxFunctions::PeriodicBoundaryType getPeriodicBoundaryType() const;

		/*
		* 类型转中文字符串
		*/
		QString periodicBoundaryTypeToString() const;

		/*
		* 设置描述
		*/
		void setDescribe(const QString& describe);

		/*
		* 获取描述
		*/
		QString getDescribe() const;

		/*
		* 拷贝
		*/
		mPeriodicBoundary* copyPeriodicBoundary(const int& id, const QString& name);

		/*
		* 设置抑制状态
		*/
		void setSuppressed(bool suppressed);

		/*
		* 获取抑制状态
		*/
		bool isSuppressed() const;

		/*
		* 设置，获取拾取状态
		*/
		void setPickStateMaster(int pickstate);
		int getPickStateMaster();
		void setPickStateSlave(int pickstate);
		int getPickStateSlave();

		/*
		* 属性设置：自动计算开关
		*/
		bool isAutoCalc() const;
		void setAutoCalc(bool autoCalc);

		/*
		* 属性设置：手动指定数值
		*/
		double getManualValue() const;
		void setManualValue(double value);

		/*
		* 属性设置：调整开关
		*/
		bool isAdjust() const;
		void setAdjust(bool adjust);

		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info);

		friend QDataStream& operator>>(QDataStream& in, mPeriodicBoundary& obj);
		friend QDataStream& InStreammPeriodicBoundaryV3(QDataStream& in, mPeriodicBoundary& obj);
		friend QDataStream& operator<<(QDataStream& out, mPeriodicBoundary& obj);

	public:
		bool m_autoCalc = true;    // 默认自动计算
		double m_manualValue = 1.0;// 默认手动值（正数）
		bool m_adjust = true;      // 默认勾选调整

		int _id;
		QString _name;
		MxFunctions::PeriodicBoundaryType _type;
		QString _describe;
		bool _isUserRename;
		bool _isSuppressed;

		//具有通用拾取控件的面板 需要设置此参数 0表示“无” 1表示“无临时数据” 2表示“有临时数据”
		int _pickStateMaster;   //主面
		int _pickStateSlave;	//从面
		/************* 边界面（参考接触） *************/
		// 主面方向
		bool masterDir = true;
		// 从面方向
		bool slaveDir = true;

		/************* 方向/轴（参考离散场方向点） *************/
		// 起始点（ID与坐标）
		int startPointId = -1;
		QVector3D startPoint;
		// 方向点/轴上第二点（ID与坐标）
		int endPointId = -1;
		QVector3D endPoint;

		/************* 方向控件UI状态（仅用于面板反显） *************/
		DirectionPickState dirPickState;

		/************* 平移/旋转量 *************/
		// 平移距离或旋转角度（QString，平移与旋转共用）
		QString value = "";
	};
}
