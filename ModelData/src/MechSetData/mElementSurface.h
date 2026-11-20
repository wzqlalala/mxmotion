#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXSetData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mSetBase.h"
#include "mModelStruct.h"

namespace MechSetData
{
	class MXSETDATA_EXPORT mElementSurface : public mSetBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mElementSurface(int id,MxFunctions::SetType type, const QString& name = "", bool internal = false, bool isUserRename = false);

		/*
		* 仅用于输入输出流
		*/
		mElementSurface() {}

		/*
		* 析构函数
		*/
		~mElementSurface();

		/*
		* 拷贝分析集合
		*/
		mElementSurface* copySet(const int& id, const QString& name) override;

		/*
		* 获取集合数据
		*/
		std::vector<RenderPickResultData> getElementSurfaceData();

		/*
		* 设置集合数据
		*/
		void setElementSurfaceData(std::vector<RenderPickResultData> data);

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mElementSurface& obj);
		friend QDataStream& InStreamElementSurfaceV3(QDataStream& in, mElementSurface& obj);

		friend QDataStream& operator<<(QDataStream& out, mElementSurface& obj);

	private:
		std::vector<RenderPickResultData> _ids;
	};
}
