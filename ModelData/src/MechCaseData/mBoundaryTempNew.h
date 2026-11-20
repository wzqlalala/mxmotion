#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <unordered_map>
#include <vector>

#include "mModelStruct.h"
#include "mLoadBoundaryBase.h"

// 前向声明：RenderPickResultData 在 UI 模块中定义，数据类仅作为存储容器
struct RenderPickResultData;

namespace MechCaseData
{
	class MXCASEDATA_EXPORT mBoundaryTempNew :public mLoadBoundaryBase
	{
	public:
		/*
		* 构造函数
		*/
		mBoundaryTempNew(int id, MxFunctions::LoadBoundaryType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mBoundaryTempNew() {}

		/*
		* 析构函数
		*/
		~mBoundaryTempNew();

		/*
		* 拷贝
		*/
		std::shared_ptr<mLoadBoundaryBase> copyLoadBoundary(const int& id, const QString& name) override;

		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mBoundaryTempNew& obj);
		friend QDataStream& InStreammBoundaryTempNewV3(QDataStream& in, mBoundaryTempNew& obj);
		friend QDataStream& operator<<(QDataStream& out, mBoundaryTempNew& obj);

	public:
		// 主模型文件路径
		QString _mainModelFile;
		// 时间步
		QString _timeStep;
		// 主集合名称
		QString _mainSet;
		// 是否指定主模型集合
		bool _useMainSet{ false };
		// 智能拾取数据（兼容旧数据以及非 GUI 数据流）
		std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> _pickData;
	};
}
