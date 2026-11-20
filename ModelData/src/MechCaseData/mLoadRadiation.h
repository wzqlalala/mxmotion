#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <map>
#include <QVariant>
#include "mModelStruct.h"

#include "mLoadBoundaryBase.h"

namespace MechCaseData
{
	class MXCASEDATA_EXPORT mLoadRadiation : public mLoadBoundaryBase
	{
	public:
		/*
		* 构造函数
		*/
		mLoadRadiation(int id, MxFunctions::LoadBoundaryType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mLoadRadiation() {}

		/*
		* 析构函数
		*/
		~mLoadRadiation();

		/*
		* 拷贝
		*/
		std::shared_ptr<mLoadBoundaryBase> copyLoadBoundary(const int& id, const QString& name) override;

		/*
		* 数据完整性判断
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mLoadRadiation& obj);
		friend QDataStream& InStreammLoadRadiationV3(QDataStream& in, mLoadRadiation& obj);
		friend QDataStream& operator<<(QDataStream& out, mLoadRadiation& obj);

	public:
		QVariant V1;
		QVariant V2;

		// 定义方式：0-统一赋值 1-独立赋值 2-分析场
		int defineType_ = 0;
		// 独立赋值映射：key=RenderPickResultData(单元ID,部件ID,面ID), value=[环境温度, 热辐射系数]
		std::map<RenderPickResultData, QVector<QVariant>> cellFaceData_;
	};
}