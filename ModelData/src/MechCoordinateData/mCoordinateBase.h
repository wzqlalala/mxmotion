#pragma once
//解决中文乱码
#pragma execution_character_Coordinate("utf-8")

#include <QString>
#include <vector>

#include "MXCoordinateData_global.h"
#include "mPhysicalFieldEnum.h"

#include <QVector3D>
#include <QVector>
#include <QString>


namespace MechCoordinateData
{
	enum class TransformType
	{
		Translation,
		Rotation,
		Flip
	};

	struct TransformStep
	{
		TransformType type;
		QVector3D value;    // Translation：偏移值；Rotation：以度为单位的 (rx, ry, rz)；Flip：unused（或可放描述）
		QVector<int> flipMap; // 仅当 type==Flip 时使用，大小应为 3，元素取值 ±1, ±2, ±3
		QString description;

		friend QDataStream& operator>>(QDataStream& in, const TransformStep& mgr);
		friend QDataStream& InStreamTransformStepV3(QDataStream& in, TransformStep& obj);
		friend QDataStream& operator<<(QDataStream& out, TransformStep& mgr);
	};
	class MXCOORDINATEDATA_EXPORT mCoordinateBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mCoordinateBase(int id, MxFunctions::CoordinateType type = MxFunctions::CoordinateType::Cartesian, const QString& name = "");


		/*
		* 仅用于输入输出流
		*/
		mCoordinateBase() {}

		/*
		* 析构函数
		*/
		~mCoordinateBase();

		/*
		* 设置坐标系ID
		*/
		void setCoordinateId(int id);

		/*
		* 获取坐标系ID
		*/
		int getCoordinateId();

		/*
		* 设置坐标系名
		*/
		void setCoordinateName(const QString& name);

		/*
		* 获取坐标系名
		*/
		QString getCoordinateName();

		/*
		* 设置坐标系类型
		*/
		void setCoordinateType(MxFunctions::CoordinateType type);
		/*
		* 获取坐标系类型
		*/
		MxFunctions::CoordinateType getCoordinateType();

		/*
		* 获取坐标系类型的字符串
		*/
		QString getCoordinateTypeStr();

		/*
		设置方法（"三点" 或 "基准轴"） 
		*/ 
		void setMethod(const QString& method);

		/*
		获取方法 
		*/ 
		QString getMethod() const;

		/*
		设置原点 
		*/ 
		void setOrigin(const QVector3D& origin);

		/*
		获取原点 
		*/ 
		QVector3D getOrigin() const;

		/*
		设置主轴索引 
		*/ 
		void setMainIndex(int index);

		/*
		获取主轴索引 
		*/
		int getMainIndex() const;

		/*
		设置方向索引 
		*/ 
		void setDirIndex(int index);

		/*
		获取方向索引 
		*/ 
		int getDirIndex() const;

		/*
		设置 X 点（用于三点方法） 
		*/ 
		void setXPoint(const QVector3D& point);

		/*
		获取 X 点 
		*/ 
		QVector3D getXPoint() const;

		/*
		设置 XY 点（用于三点方法） 
		*/ 
		void setXYPoint(const QVector3D& point);

		/*
		获取 XY 点
		*/ 
		QVector3D getXYPoint() const;

		/*
		设置 X 线（用于基准轴方法） 
		*/ 
		void setXLine(const QVector3D& line);

		/*
		获取 X 线 
		*/ 
		QVector3D getXLine() const;

		/*
		设置 Y 线（用于基准轴方法） 
		*/ 
		void setYLine(const QVector3D& line);

		/*
		获取 Y 线 
		*/ 
		QVector3D getYLine() const;

		/*
		设置主轴基于 
		*/ 
		void setMainBase(int base);

		/*
		获取主轴基于 
		*/ 
		int getMainBase() const;

		/*
		设置方向基于 
		*/ 
		void setDirBase(int base);

		/*
		获取方向基于 
		*/ 
		int getDirBase() const;

		/*
		设置主轴反向
		*/
		void setMainReverse();

		/*
		设置主轴朝向反向
		*/
		void setDirReverse();

		/*
		设置轴点反向
		*/
		void setXPointReverse();

		//通过字符串获取变换信息
		TransformStep getTransformStep(const QString& type, const QString& value);

		//通过字符串获取变换信息
		void getTransformStep(int idx, QString& type, QString& value);

		//替换变换
		void replaceTransformStep(int idx, const QString& value);

		//添加变换
		void addTransformStep(const QString& type, const QString& value);

		//添加变换
		void addTransformStep(const TransformStep& step) { _transformQueue.append(step); }

		//更新变换值
		void replaceTransformStep(const TransformStep& step) { _transformQueue.append(step); }

		//移除变换
		void removeTransformStep(int idx) { if (idx >= 0 && idx < _transformQueue.size()) _transformQueue.removeAt(idx); }

		//移动变换
		void moveTransformStep(int from, int to) { if (from >= 0 && to >= 0 && from < _transformQueue.size() && to < _transformQueue.size()) _transformQueue.move(from, to); }

		//清空变换
		void clearTransformSteps() { _transformQueue.clear(); }

		//返回变换
		const QVector<TransformStep>& getTransformQueue() const { return _transformQueue; }

		/*
		获取 XYZ 轴矢量（根据方法计算） 
		*/ 
		QVector<QVector3D> getXYZAxis(bool &success, QString &msg, QVector3D& origin) const;

		/*
		* 拷贝坐标系
		*/
		virtual mCoordinateBase* copyCoordinate(const int& id, const QString& name);

		friend QDataStream& operator>>(QDataStream& in, mCoordinateBase& mgr);
		friend QDataStream& InStreammCoordinateBaseV3(QDataStream& in, mCoordinateBase& obj);
		friend QDataStream& operator<<(QDataStream& out, mCoordinateBase& mgr);

		public:
		/*
		铺层读文件时临时需要的几个参数，
		其他地方用不到
		*/
		int _normaldirection;       //法向
		int _rotAngleType;          //角度类型
		QString _rotAngle;           //转角(固定角度)
		QString _rotFieldName;      //离散场名（暂无，保留）

	private:
		//坐标系ID
		int _id;

		//坐标系名字
		QString _coordinateName;

		//坐标系类型
		MxFunctions::CoordinateType _coordinateType;

		//方式  基准轴/三点
		QString _method;

		//原点
		QVector3D _origin;

		//主轴（0为X方向；1为Y方向；2为Z方向）
		int _mainIndex = 0;

		//主轴朝向（0为X/Y方向；1为Y/Z方向；）
		int _dirIndex = 0;

		////////三点
		QVector3D _xPoint, _xyPoint;
		int _xReverse = 1;

		///////基准轴
		QVector3D _xLine, _yLine;

		//主轴基于(0 为拾取数据；1为全局X轴，2为全局Y轴，3为全局Z轴)
		int _mainBase = 0;
		int _mainReverse = 1;

		//主轴朝向基于(0 为拾取数据；1为全局X轴，2为全局Y轴，3为全局Z轴)
		int _dirBase = 0;
		int _dirReverse = 1;

		//变换队列
		QVector<TransformStep> _transformQueue;
	};
}
