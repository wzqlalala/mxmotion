#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QMap>
#include <QVector>
#include <map>
#include "mModelStruct.h"
#include "mLoadBoundaryBase.h"

namespace MechCaseData
{
	class MXCASEDATA_EXPORT mBoundaryDisplacement :public mLoadBoundaryBase
	{
	public:
		/*
		* 构造函数
		*/
		mBoundaryDisplacement(int id,MxFunctions::LoadBoundaryType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mBoundaryDisplacement() {}

		/*
		* 析构函数
		*/
		~mBoundaryDisplacement();

		/*
		* 拷贝
		*/
		std::shared_ptr<mLoadBoundaryBase> copyLoadBoundary(const int& id, const QString& name) override;

		friend QDataStream& operator>>(QDataStream& in, mBoundaryDisplacement& obj);
		friend QDataStream& InStreammBoundaryDisplacementV3(QDataStream& in, mBoundaryDisplacement& obj);
		friend QDataStream& operator<<(QDataStream& out, mBoundaryDisplacement& obj);
	public:
		int _definitionMode = 0;
		QVariant _U1;
		QVariant _U2;
		QVariant _U3;
		QVariant _UR1;
		QVariant _UR2;
		QVariant _UR3;
		// key=完整 RenderPickResultData，value=[U1,U2,U3,UR1,UR2,UR3]
		// 无效 QVariant 表示该分量未输入；必须保留所有拾取对象的行
		std::map<RenderPickResultData, QVector<QVariant>> _independentTable;
	};
}