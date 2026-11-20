#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXSetData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mSetBase.h"
#include "mModelStruct.h"
#include <mBasicEnum.h>

namespace MechSetData
{
	class MXSETDATA_EXPORT mMixSet : public mSetBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mMixSet(int id, MxFunctions::SetType type, const QString& name = "", bool internal = false, bool isUserRename = false);

		/*
		* 仅用于输入输出流
		*/
		mMixSet() {}

		/*
		* 析构函数
		*/
		~mMixSet();

		/*
		* 获取集合数据
		*/
		std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> getMixSetData();

		/*
		* 设置集合数据
		*/
		void setMixSetData(const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data);

		/*
		* 拷贝分析集合
		*/
		mSetBase* copySet(const int& id, const QString& name)override;

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info)override;

		/*
		* 清空集合数据(通过拾取类型和数据)
		*/
		void replaceMixSetData(MxFunctions::PickDataType pickDataType, std::vector<RenderPickResultData> data, std::vector<int> ids);

		/*
		* 获取集合数据(通过拾取类型和数据)
		*/
		void getMixSetData(MxFunctions::PickDataType& pickDataType, std::vector<RenderPickResultData>& data, std::vector<int>& ids);

		friend QDataStream& operator>>(QDataStream& in, mMixSet& obj);
		friend QDataStream& InStreamMixSetV3(QDataStream& in, mMixSet& obj);

		friend QDataStream& operator<<(QDataStream& out, mMixSet& obj);

	private:
		std::unordered_map<MxFunctions::PickDataType,std::vector<RenderPickResultData>> _ids;
	};
}
