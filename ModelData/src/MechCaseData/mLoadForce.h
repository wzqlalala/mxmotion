#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QVector3D>
#include <QMap>
#include <QVector>
#include <map>
#include "mLoadBoundaryBase.h"
#include "mModelStruct.h"

namespace MechCaseData
{
	class MXCASEDATA_EXPORT mLoadForce : public mLoadBoundaryBase
	{
	public:
		mLoadForce(int id, MxFunctions::LoadBoundaryType type, const QString& name = "");
		mLoadForce() {}
		~mLoadForce();

		std::shared_ptr<mLoadBoundaryBase> copyLoadBoundary(const int& id, const QString& name) override;
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mLoadForce& obj);
		friend QDataStream& InStreammLoadForceV3(QDataStream& in, mLoadForce& obj);
		friend QDataStream& operator<<(QDataStream& out, mLoadForce& obj);

	public:
		// 定义方式：集中力 0=各向分量, 1=方向与大小, 2=独立赋值, 3=文件；力矩 0=各向分量, 1=独立赋值, 2=文件
		int way_;

		// 分析场方式：true 时 mx 写出 field 关键字，无作用对象/大小/缩放因子（力矩不支持）
		bool isAnalysisFieldMode_ = false;

		// 方向控件完整UI状态（单例面板反显用）
		DirectionPickState dirPickState_;

		// 核心载荷数据（统一计算用）
		QVariant force_;       // 载荷大小（方向与大小模式）
		QVariant dir_;         // 最终归一化方向矢量（所有模式统一出口）
		QVariant force_x;      // X分量（各向分量/内部计算）
		QVariant force_y;      // Y分量
		QVariant force_z;      // Z分量

		// 独立赋值：统一数据结构
		// key=RenderPickResultData(节点ID, 部件ID, 0), value=[X分量, Y分量, Z分量]
		std::map<RenderPickResultData, QVector<QVariant>> nodeForceMap_;

		// 文件定义
		QString path_;
	};
}