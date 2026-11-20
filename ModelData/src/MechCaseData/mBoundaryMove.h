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
#include "mBasicEnum.h"

// 前向声明：RenderPickResultData 在 UI 模块中定义，数据类仅作为存储容器
struct RenderPickResultData;

namespace MechCaseData
{
	class MXCASEDATA_EXPORT mBoundaryMove :public mLoadBoundaryBase
	{
	public:
		/*
		* 构造函数
		*/
		mBoundaryMove(int id, MxFunctions::LoadBoundaryType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mBoundaryMove() {}

		/*
		* 析构函数
		*/
		~mBoundaryMove();

		/*
		* 拷贝
		*/
		std::shared_ptr<mLoadBoundaryBase> copyLoadBoundary(const int& id, const QString& name) override;

		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mBoundaryMove& obj);
		friend QDataStream& InStreammBoundaryMoveV3(QDataStream& in, mBoundaryMove& obj);
		friend QDataStream& operator<<(QDataStream& out, mBoundaryMove& obj);
	public:
		// 主模型文件路径
		QString _mainModelFile;
		// 主模型时间步
		QString _timeStep;
		// 主模型集合名称
		QString _mainSet;
		// 是否指定主模型集合
		bool _useMainSet{ false };
		// 智能拾取数据
		std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> _pickData;
	};

}
