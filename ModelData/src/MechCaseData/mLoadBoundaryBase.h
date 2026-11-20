#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mModelEnum.h"
#include <memory>
#include <QString>
#include <QVariant>
#include <QVector>
#include "mUnitSystemManager.h"
#include "mBasicEnum.h"

using namespace CommonData;
namespace MechCaseData
{
	class MXCASEDATA_EXPORT mLoadBoundaryBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mLoadBoundaryBase(int id, MxFunctions::LoadBoundaryType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mLoadBoundaryBase() {}

		/*
		* 析构函数
		*/
		~mLoadBoundaryBase();


		/*
		* 设置载荷边界ID
		*/
		void setLoadBoundaryId(int id);

		/*
		* 获取载荷边界ID
		*/
		int getLoadBoundaryId();

		/*
		* 设置载荷边界名
		*/
		void setLoadBoundaryName(const QString& name);

		/*
		* 获取载荷边界名
		*/
		QString getLoadBoundaryName();

		/*
		* 设置载荷边界类型
		*/
		void setLoadBoundaryType(MxFunctions::LoadBoundaryType type);

		/*
		* 获取载荷边界类型
		*/
		MxFunctions::LoadBoundaryType getLoadBoundaryType();


		/*
		* 判断是载荷还是边界 0 载荷 1边界
		*/
		int isLoadOrBoundary();

		/*
		* 设置抑制状态
		*/
		void setSuppressed(bool suppressed);

		/*
		* 获取抑制状态
		*/
		bool isSuppressed() const;

		/*
		* 设置拾取状态
		*/
		void setPickState(int pickstate);

		/*
		* 获取抑制状态
		*/
		int getPickState();

		/*
		* 拷贝载荷边界
		*/
		virtual std::shared_ptr<mLoadBoundaryBase> copyLoadBoundary(const int& id, const QString& name) = 0;


		/*
		* 判断数据完整性
		*/
		virtual MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) { return MxFunctions::BrowserTreeStatus::Tree_Yes; };

		friend QDataStream& operator>>(QDataStream& in, mLoadBoundaryBase& obj);
		friend QDataStream& InStreammLoadBoundaryBaseV3(QDataStream& in, mLoadBoundaryBase& obj);
		friend QDataStream& operator<<(QDataStream& out, mLoadBoundaryBase& obj);

	private:
		//载荷边界ID
		int _id;

		//载荷边界名字
		QString _loadBoundaryName;

		//载荷边界类型
		MxFunctions::LoadBoundaryType _loadBoundaryType;

		//是否被抑制
		bool _isSuppressed;

		//具有通用拾取控件的面板 需要设置此参数 0表示“无” 1表示“无临时数据” 2表示“有临时数据”
		int _pickState;
	};
}