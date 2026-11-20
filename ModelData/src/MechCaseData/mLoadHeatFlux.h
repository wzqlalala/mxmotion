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
	class MXCASEDATA_EXPORT mLoadHeatFlux : public mLoadBoundaryBase
	{
	public:
		/*
		* 构造函数
		*/
		mLoadHeatFlux(int id, MxFunctions::LoadBoundaryType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mLoadHeatFlux() {}

		/*
		* 析构函数
		*/
		~mLoadHeatFlux();

		/*
		* 拷贝
		*/
		std::shared_ptr<mLoadBoundaryBase> copyLoadBoundary(const int& id, const QString& name) override;

		/*
		* 数据完整性判断
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mLoadHeatFlux& obj);
		friend QDataStream& InStreammLoadHeatFluxV3(QDataStream& in, mLoadHeatFlux& obj);
		friend QDataStream& operator<<(QDataStream& out, mLoadHeatFlux& obj);

	public:
		QVariant pa_;
		int way_ = 0;          // 热流类型：0-点热流 1-面热流 2-体热流
		int defineType_ = 0;   // 定义方式：0-统一赋值 1-独立赋值 2-分析场
		// 独立赋值映射：key=RenderPickResultData(节点ID/单元ID,部件ID,面ID/0), value=[热流密度]
		std::map<RenderPickResultData, QVector<QVariant>> cellFaceData_;
	};
}