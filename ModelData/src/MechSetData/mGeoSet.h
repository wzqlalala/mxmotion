#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXSetData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mSetBase.h"
#include "mModelStruct.h"

namespace MechSetData
{
	class MXSETDATA_EXPORT mGeoSet : public mSetBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mGeoSet(int id,MxFunctions::SetType type, const QString& name = "", bool internal = false, bool isUserRename = false);

		/*
		* 仅用于输入输出流
		*/
		mGeoSet() {}

		/*
		* 析构函数
		*/
		~mGeoSet();

		/*
		* 拷贝分析集合
		*/
		mGeoSet* copySet(const int& id, const QString& name) override;

		/*
		* 获取集合数据
		*/
		std::vector<int> getGeoSetData();

		/*
		* 设置集合数据
		*/
		void setGeoSetData(std::vector<int> data);

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mGeoSet& obj);
		friend QDataStream& InStreamGeoSetV3(QDataStream& in, mGeoSet& obj);

		friend QDataStream& operator<<(QDataStream& out, mGeoSet& obj);
	private:
		std::vector<int> _ids;
	};
}
