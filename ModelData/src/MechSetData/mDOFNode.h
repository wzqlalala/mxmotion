#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXSetData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mSetBase.h"
#include "mModelStruct.h"

namespace MechSetData
{
	class MXSETDATA_EXPORT mDOFNode : public mSetBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mDOFNode(int id, MxFunctions::SetType type, const QString& name = "", bool internal = false, bool isUserRename = false);

		/*
		* 仅用于输入输出流
		*/
		mDOFNode() {}

		/*
		* 析构函数
		*/
		~mDOFNode();

		/*
		* 拷贝分析集合
		*/
		mDOFNode* copySet(const int& id, const QString& name) override;

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
		* 自由度
		*/
		int dof() const;

		/*
		* 设置自由度
		*/
		void setDof(int idof);

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info) override;


		friend QDataStream& operator>>(QDataStream& in, mDOFNode& obj);
		friend QDataStream& InStreammDOFNodeV3(QDataStream& in, mDOFNode& obj);

		friend QDataStream& operator<<(QDataStream& out, mDOFNode& obj);

	private:
		//是否排序，从小到大
		bool _isOrder = false;

		std::vector<RenderPickResultData> _ids;

		//自由度
		int _idof = 1;
	};
}
