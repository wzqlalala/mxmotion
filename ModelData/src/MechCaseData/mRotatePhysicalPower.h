#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include "mLoadBoundaryBase.h"
#include<QVector3D>
#include <QVariant>
#include <QVector>
#include <map>
#include <QStringList>
#include <unordered_map>
#include <vector>

#include "mModelStruct.h"

namespace MechCaseData
{
	class MXCASEDATA_EXPORT mRotatePhysicalPower : public mLoadBoundaryBase
	{
	public:
		/*
		* 构造函数
		*/
		mRotatePhysicalPower(int id, MxFunctions::LoadBoundaryType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mRotatePhysicalPower() {}

		/*
		* 析构函数
		*/
		~mRotatePhysicalPower();

		/*
		* 拷贝
		*/
		std::shared_ptr<mLoadBoundaryBase> copyLoadBoundary(const int& id, const QString& name) override;

		/*
		* 获取当前所选载荷类型
		*/
		QString getSelectLoadType();

		/*
		* 数据完整性判断
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mRotatePhysicalPower& obj);
		friend QDataStream& InStreammRotatePhysicalPowerV3(QDataStream& in, mRotatePhysicalPower& obj);
		friend QDataStream& operator<<(QDataStream& out, mRotatePhysicalPower& obj);

	public:
			QVector3D _startPoint1;
			QVector3D _directPoint2;
	
			// 类型：0离心力(转速) 1旋转加速度
			int _rotateType = 0;
	
			// 定义方式：0统一赋值 1独立赋值
			int _defineType = 0;
	
			// 统一赋值大小（离心力类型时内部统一按 RPM 存储，旋转加速度类型时为角加速度值）
			QVariant _uniformValue;
		
			// 独立赋值：key=RenderPickResultData(单元ID, 部件ID, -1), value=[值, 起始点(X,Y,Z), 方向点(X,Y,Z)] 用于回显和持久化
			std::map<RenderPickResultData, QVector<QVariant>> _elementCoorMap;
	
			// 方向控件完整 UI 状态（单例面板反显用）
			DirectionPickState _dirPickState;
		
			// 当前拾取对象，用于回显（独立赋值模式下为单元）
			std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> _picks;
	};
}
