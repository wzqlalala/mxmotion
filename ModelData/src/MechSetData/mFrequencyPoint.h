#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXSetData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mSetBase.h"
#include "mModelStruct.h"

namespace MechSetData
{
	class MXSETDATA_EXPORT mFrequencyPoint : public mSetBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mFrequencyPoint(int id, MxFunctions::SetType type, const QString& name = "", bool internal = false, bool isUserRename = false);

		/*
		* 仅用于输入输出流
		*/
		mFrequencyPoint() {}

		/*
		* 析构函数
		*/
		~mFrequencyPoint();

		/*
		* 拷贝分析集合
		*/
		mFrequencyPoint* copySet(const int& id, const QString& name) override;

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

		/***************频率点集数据操作************************/
		int orderCount() const;
		double orderSecondValueAt(int index) const;
		bool setOrderSecondValueAt(int index, double value);
		void clearOrders();
		void addOrder(int key, double value);
		bool insertOrder(int index, int key, double value);
		bool removeOrderAt(int index);

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info) override;


		friend QDataStream& operator>>(QDataStream& in, mFrequencyPoint& obj);
		friend QDataStream& InStreamFrequencyPointV3(QDataStream& in, mFrequencyPoint& obj);

		friend QDataStream& operator<<(QDataStream& out, mFrequencyPoint& obj);
		
	private:
		//是否排序，从小到大
		bool _isOrder = false;

		std::vector<RenderPickResultData> _ids;

		/********************频率点集数据***************************/
		QList<QPair<int, double>> _order;
	};
}
