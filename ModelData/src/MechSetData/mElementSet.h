#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXSetData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mSetBase.h"
#include "mModelStruct.h"

namespace MechSetData
{
	class MXSETDATA_EXPORT mElementSet : public mSetBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mElementSet(int id,MxFunctions::SetType type, const QString& name = "", bool internal = false, bool isUserRename = false);

		/*
		* 仅用于输入输出流
		*/
		mElementSet() {}

		/*
		* 析构函数
		*/
		~mElementSet();

		/*
		* 拷贝分析集合
		*/
		mElementSet* copySet(const int& id, const QString& name) override;

		/*
		* 获取集合数据
		*/
		std::vector<RenderPickResultData> getElementSetData();

		/*
		* 设置集合数据
		*/
		void setElementSetData(std::vector<RenderPickResultData> data);

		/*
		* 是否排序
		*/
		bool getOrder();

		/*
		* 设置排序
		*/
		void setOrder(bool order);

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mElementSet& obj);
		friend QDataStream& InStreamElementV3(QDataStream& in, mElementSet& obj);

		friend QDataStream& operator<<(QDataStream& out, mElementSet& obj);

	private:
		//是否排序，从小到大
		bool _isOrder = false;

		std::vector<RenderPickResultData> _ids;
	};
}
