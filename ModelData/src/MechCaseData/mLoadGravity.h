#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"
#include "mLoadBoundaryBase.h"

#include <QString>
#include <QVector3D>
#include <QVariant>
#include <unordered_map>
#include <vector>

namespace MechCaseData
{

	class MXCASEDATA_EXPORT mLoadGravity : public mLoadBoundaryBase
	{
	public:
		/*
		* 构造函数
		*/
		mLoadGravity(int id, MxFunctions::LoadBoundaryType type, const QString& name = "");
		/*
		* 仅用于输入输出流
		*/
		mLoadGravity() {}

		/*
		* 析构函数
		*/
		~mLoadGravity();

		/*
		* 拷贝
		*/
		std::shared_ptr<mLoadBoundaryBase> copyLoadBoundary(const int& id, const QString& name) override;

		/*
		* 数据完整性判断
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mLoadGravity& obj);
		friend QDataStream& InStreammLoadGravityV3(QDataStream& in, mLoadGravity& obj);
		friend QDataStream& operator<<(QDataStream& out, mLoadGravity& obj);
	public:
		// 定义方式：0=各向分量, 1=方向与大小
		int way_ = 0;

		// 方向控件完整UI状态（单例面板反显用）
		DirectionPickState dirPickState_;

		// 重力加速度大小（方向与大小模式）
		QVariant magnitude_;

		// 最终归一化方向矢量（所有模式统一出口）
		QVariant dir_;

		// 各向分量
		QVariant V1;
		QVariant V2;
		QVariant V3;

		// 是否启用局部施加
		bool isCornerAddChecked = false;
	};
}