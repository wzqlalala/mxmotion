#pragma once
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <map>
#include <QVariant>
#include <QVector3D>
#include "mModelStruct.h"

#include "mLoadBoundaryBase.h"

namespace MechCaseData
{
	class MXCASEDATA_EXPORT mLoadPressure : public mLoadBoundaryBase
	{
	public:
		/*
		* 构造函数
		*/
		mLoadPressure(int id, MxFunctions::LoadBoundaryType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mLoadPressure() {}

		/*
		* 析构函数
		*/
		~mLoadPressure();

		/*
		* 拷贝
		*/
		std::shared_ptr<mLoadBoundaryBase> copyLoadBoundary(const int& id, const QString& name) override;

		/*
		* 数据完整性判断
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mLoadPressure& obj);
		friend QDataStream& InStreammLoadPressureV3(QDataStream& in, mLoadPressure& obj);
		friend QDataStream& operator<<(QDataStream& out, mLoadPressure& obj);

	public:
		// 压强类型：0-常规压力，1-净水压力
		int pressureType_ = 0;

		// 常规压力
		QVariant pa_;               // 统一赋值压强
		int way_ = 0;               // 定义方式：0=统一赋值，1=独立赋值，2=文件赋值
		// 分析场方式（仅常规压力）：true 时 mx 写出 field 关键字，无作用对象/大小/缩放因子
		bool isAnalysisFieldMode_ = false;
		// 独立赋值映射：key=RenderPickResultData(单元ID,部件ID,面ID), value=[压强值]
		std::map<RenderPickResultData, QVector<QVariant>> cellFaceMap_;
		QString path_;              // 文件赋值路径

		// 净水压力
		QVector3D depthDirection_ = { 1, 0, 0 };	// 深度方向向量（默认X向，与 directionSelectorWidget 默认 X轴 保持一致）
		DirectionPickState depthDirPickState_;		// 深度方向控件完整UI状态（单例面板反显用）
		QVariant zeroDepth_;							// 零压强深度
		QVariant refDepth_;								// 参考深度
		QVariant refPressure_;							// 参考压强值
	};
}