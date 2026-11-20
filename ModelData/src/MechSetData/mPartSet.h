#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXSetData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mSetBase.h"
#include "mModelStruct.h"

namespace MechSetData
{
	class MXSETDATA_EXPORT mPartSet : public mSetBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mPartSet(int id , MxFunctions::SetType type, const QString& name = "", bool internal = false, bool isUserRename = false);

		/*
		* 仅用于输入输出流
		*/
		mPartSet() {}

		/*
		* 析构函数
		*/
		~mPartSet();

		/*
		* 拷贝分析集合
		*/
		mPartSet* copySet(const int& id, const QString& name) override;

		/*
		* 获取集合数据
		*/
		std::vector<int> getPartSetData();

		/*
		* 设置集合数据
		*/
		void setPartSetData(std::vector<int> data);

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mPartSet& obj);
		friend QDataStream& InStreamPartSetV3(QDataStream& in, mPartSet& obj);

		friend QDataStream& operator<<(QDataStream& out, mPartSet& obj);
	private:
		std::vector<int> _ids;
	};
}
