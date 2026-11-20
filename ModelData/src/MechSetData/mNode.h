#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXSetData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mSetBase.h"
#include "mModelStruct.h"

namespace MechSetData
{
	class MXSETDATA_EXPORT mNode : public mSetBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mNode(int id, MxFunctions::SetType type, const QString& name = "", bool internal = false, bool isUserRename = false);

		/*
		* 仅用于输入输出流
		*/
		mNode() {}

		/*
		* 析构函数
		*/
		~mNode();

		/*
		* 拷贝分析集合
		*/
		mNode* copySet(const int& id, const QString& name) override;

		/*
		* 获取集合数据
		*/
		std::vector<RenderPickResultData> getNodeData();

		/*
		* 设置集合数据
		*/
		void setNodeData(std::vector<RenderPickResultData> data);

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


		friend QDataStream& operator>>(QDataStream& in, mNode& obj);
		friend QDataStream& InStreamNodeV3(QDataStream& in, mNode& obj);

		friend QDataStream& operator<<(QDataStream& out, mNode& obj);

	private:
		//是否排序，从小到大
		bool _isOrder = false;

		std::vector<RenderPickResultData> _ids;
	};
}
