#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXDiscreteFieldData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mModelEnum.h"
#include "mBasicEnum.h"
#include "mFieldBase.h"

#include <map>

//Qt
#include <QString>
#include<QStringList>
#include<QVector3D>


namespace MechDiscreteFieldData
{
	class MXDISCRETEFIELDDATA_EXPORT mDiscreteField : public mFieldBase
	{
	public:
		/*
		* 含参构造函数
		*/
		mDiscreteField(int id, const QString& name = "");

		/*
		* 默认构造函数（供反序列化使用）
		*/
		mDiscreteField() {};

		/*
		* 析构函数
		*/
		~mDiscreteField();

		/*
		* 场子类型：离散场
		*/
		MxFunctions::FieldType getFieldType() const override { return MxFunctions::FieldType::Discrete; }

		/*
		* 拷贝离散场数据
		*/
		mDiscreteField* copyDiscreteField();

		/*
		* 拷贝场数据（基类接口）
		*/
		mFieldBase* copyField() override { return copyDiscreteField(); }

		/*
		* 离散场名称
		*/
		void setDiscreteSpaceName(const QString& name);
		QString getDiscreteSpaceName() const;

		/*
		* 单元或节点位置
		*/
		void setLocate(MxFunctions::PickDataType locate);
		MxFunctions::PickDataType getLocate();

		/*
		* 空间类型
		*/
		void setSpaceType(MxFunctions::DiscreteSpace type);
		MxFunctions::DiscreteSpace getSpaceType() const;

		/*
		* 默认角度
		*/
		void setDefaultAngel(const QString& angel);
		QString getDefaultAngel() const;

		/*
		* 离散角度单元映射
		*/
		void setElementAngelMap(const std::map<int, QString>& map);
		std::map<int, QString> getElementAngelMap() const;

		/*
		* 默认厚度
		*/
		void setDefaultThickness(const QString& thickness);
		QString getDefaultThickness() const;

		/*
		* 离散厚度单元映射
		*/
		void setElementThicknessMap(const std::map<int, QString>& map);
		std::map<int, QString> getElementThicknessMap() const;

		/*
		* 离散厚度节点映射
		*/
		void setNodeThicknessMap(const std::map<int, QString>& map);
		std::map<int, QString> getNodeThicknessMap() const;

		/*
		* 坐标定义类型
		*/
		void setCoorType(MxFunctions::CoordinateType type);
		MxFunctions::CoordinateType getCoorType() const;

		/*
		* 方向参考点
		*/
		void setDirectPoint1(const QVector3D& point);
		QVector3D getDirectPoint1() const;

		void setDirectPoint2(const QVector3D& point);
		QVector3D getDirectPoint2() const;


		void setDirectPoint3(const QVector3D& point);
		QVector3D getDirectPoint3() const;

		/*
		* 离散坐标系对应离散值 单元映射
		*/
		void setElementCoorMap(const std::map<int, QStringList>& map);
		std::map<int, QStringList> getElementCoorMap() const;


		/*
		* 判断节点状态
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mDiscreteField& mgr);
		friend QDataStream& InStreammDiscreteFieldV3(QDataStream& in, mDiscreteField& obj);
		friend QDataStream& operator<<(QDataStream& out, mDiscreteField& mgr);

	public:
		//单元或节点  设置节点枚举 Element、Node
		MxFunctions::PickDataType _locate = MxFunctions::PickDataType::Node;

		//空间类型
		MxFunctions::DiscreteSpace _spaceType = MxFunctions::DiscreteSpace::AngelSpace;

		//默认角度
		QString _defaultAngel;

		//离散角度（离散属性）
		std::map<int, QString> _elementAngelMap;

		//默认厚度
		QString _defaultThickness;

		//离散厚度（离散属性）
		std::map<int, QString> _elementThicknessMap;
		std::map<int, QString> _nodeThicknessMap;

		//坐标系类型
		MxFunctions::CoordinateType _coorType = MxFunctions::CoordinateType::Cartesian;

		//方向参考点
		QVector3D _directPoint1;
		QVector3D _directPoint2;
		QVector3D _directPoint3;

		//离散坐标系 对应离散值
		std::map<int, QStringList> _elementCoorMap;

	};
}
