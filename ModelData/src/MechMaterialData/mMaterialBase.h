#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXMaterialData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mModelEnum.h"
#include "mUnitSystemManager.h"

#include <QString>
#include <QSet>
#include <QHash>
#include <QVariant>
#include <QStringList>
#include <QColor>
#include <memory>
#include <QDataStream>
#include <unordered_map>

using namespace CommonData;
namespace MechMaterialData
{
	enum class CurveType
	{
		NoCorrelation,//无相关性
		TemperatureCorrelation,//温度相关
		FrequencyDependence,//频率相关
	};
	struct Curves
	{
		CurveType correlation;
		QVector<QVector<double>>curves;
	};

	inline QDataStream& InStreamMatCurvesV3(QDataStream& in, Curves& obj)
	{
		typename std::underlying_type<CurveType>::type rawKey;
		in >> rawKey;
		obj.correlation = static_cast<CurveType>(rawKey);
		in >> obj.curves;
		return in;
	}

	inline QDataStream& operator>>(QDataStream& in, Curves& obj)
	{
		InStreamMatCurvesV3(in, obj);
		return in;
	}

	inline QDataStream& operator<<(QDataStream& out, const Curves& obj)
	{
		out << static_cast<typename std::underlying_type<CurveType>::type>(obj.correlation);
		out << obj.curves;
		return out;
	}

	class MXMATERIALDATA_EXPORT mMaterialBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mMaterialBase(int id, QColor color, MxFunctions::MaterialType type = MxFunctions::MaterialType::NONEMATERIALTYPE, const QString& name = "");

		/*
		* 重载构造函数（兼容旧调用，使用默认颜色）
		*/
		mMaterialBase(int id, MxFunctions::MaterialType type, const QString& name = "");

		/*
		* 根据ID生成循环颜色
		*/
		static QColor generateColorById(int id);

		/*
		* 仅用于输入输出流
		*/
		mMaterialBase() {}

		/*
		* 析构函数
		*/
		~mMaterialBase();

		/*
		* 注册材料元数据类型
		*/
		static void registerMeta();

		/*
		* 设置材料ID
		*/
		void setMaterialId(int id);

		/*
		* 获取材料ID
		*/
		int getMaterialId();

		/*
		* 设置材料名
		*/
		void setMaterialName(const QString& name);

		/*
		* 获取材料名
		*/
		QString getMaterialName();

		/*
		* 设置材料本构类型
		*/
		void setMaterialType(MxFunctions::MaterialType type);

		/*
		* 获取材料类型
		*/
		MxFunctions::MaterialType getMaterialType();

		/*
		* 设置材料颜色
		*/
		void setMatColor(const QColor& color);

		/*
		* 获取材料颜色
		*/
		QColor getMatColor();

		/*
		* 设置属性
		*/
		void setProperty(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& props);
		void setLibProperty(const QHash<MxFunctions::MaterialClass, QHash<QString, QPair<QVector<Unit>, QVariant>>>& props);


		/*
		* 添加属性
		*/
		void addProperty(const MxFunctions::MaterialClass& mattype, const MxFunctions::MaterialPropType& propname, const QVariant& property);
		void addLibProperty(const MxFunctions::MaterialClass& mattype, const QString& propname, const QVector<Unit>& units, const QVariant& property);

		/*
		* 获取所有属性
		*/
		QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>> getAllProps();
		QHash<MxFunctions::MaterialClass, QHash<QString, QPair<QVector<Unit>, QVariant>>> getAllLibProps();

		/*
		* 拷贝材料
		*/
		std::shared_ptr<mMaterialBase> copyMaterial(const int& id, const QString& name);

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info);

		/*
		* 响应单位制
		*/
		void responseUnitChanged(const int& unitidx, mUnitSystemManager* unitsystemmanager);

		/*
		* 转换曲线数据
		*/
		QVariant transCurveData(const int& unitidx, QVariant data, const MaterialPropType& proptype, mUnitSystemManager* unitsystemmanager);
		QVariant transCurveData(const int& unitidx, QVariant data, const QVector<Unit>& units, mUnitSystemManager* unitsystemmanager);

		/*
		* 曲线转换
		*/
		static QVariant transCurve2Varint(const QVector<QVector<double>>& curves, const CurveType& correlation);
		static Curves transVatint2Curve(const QVariant& curves);

		friend QDataStream& operator>>(QDataStream& in, mMaterialBase& obj);
		friend QDataStream& InStreammMaterialBaseV3(QDataStream& in, mMaterialBase& obj);
		friend QDataStream& InStreamMatBaseV3(QDataStream& in, mMaterialBase& obj);

		friend QDataStream& operator<<(QDataStream& out, mMaterialBase& obj);

	private:
		/*
		* 获取曲线应该转换的单位
		*/
		Unit getTarUnit(const int& unitidx, const Unit& unit);
	private:
		//材料ID
		int _id;

		//材料名字
		QString _materialName;

		//材料本构类型
		MxFunctions::MaterialType _materialType;

		//属性大类-属性字段-属性值
		QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>> _props;

		//材料库属性大类-属性字段-属性单位-属性值
		QHash<MxFunctions::MaterialClass, QHash<QString, QPair<QVector<Unit>, QVariant>>> _libprops;

		//材料颜色
		QColor _color;
	};
}

Q_DECLARE_METATYPE(MechMaterialData::Curves)