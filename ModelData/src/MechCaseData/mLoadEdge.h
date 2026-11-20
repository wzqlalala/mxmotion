#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QVector3D>
#include <QVariant>
#include "mLoadBoundaryBase.h"
#include "mModelStruct.h"

namespace MechCaseData
{
	class MXCASEDATA_EXPORT mLoadEdge : public mLoadBoundaryBase
	{
	public:
		mLoadEdge(int id, MxFunctions::LoadBoundaryType type, const QString& name = "");
		mLoadEdge() {}
		~mLoadEdge();

		std::shared_ptr<mLoadBoundaryBase> copyLoadBoundary(const int& id, const QString& name) override;
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		QString getDirectionModeEng();

		friend QDataStream& operator>>(QDataStream& in, mLoadEdge& obj);
		friend QDataStream& InStreammLoadEdgeV3(QDataStream& in, mLoadEdge& obj);
		friend QDataStream& operator<<(QDataStream& out, mLoadEdge& obj);

	public:
		QString distributionType_;	// 分布类型 当前只有均匀分布
		QString defineWay_;			// 定义方式 统一赋值/独立赋值
		QString directionMode_;		// 方向模式 面内法向/面外法向/边切向/通用
		QString size_;				// 载荷值

		DirectionPickState dirPickState_;  // 方向控件完整UI状态（通用模式反显用）
		QVariant dir_;                      // 通用模式自定义方向矢量（归一化，仅存值不参与渲染）
	};
}