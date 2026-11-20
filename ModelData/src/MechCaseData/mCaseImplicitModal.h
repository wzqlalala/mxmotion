#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>

#include "mCaseBase.h"

namespace MechCaseData
{
	class MXCASEDATA_EXPORT mCaseImplicitModal : public mCaseBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mCaseImplicitModal(int id, MxFunctions::CaseType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mCaseImplicitModal() {}

		/*
		* 析构函数
		*/
		~mCaseImplicitModal();

		/*
		* 拷贝分析工况
		*/
		mCaseImplicitModal* copyCase(const int& id, const QString& name) override;

		friend QDataStream& operator>>(QDataStream& in, mCaseImplicitModal& obj);
		friend QDataStream& InStreammCaseImplicitModalV3(QDataStream& in, mCaseImplicitModal& obj);
		friend QDataStream& operator<<(QDataStream& out, mCaseImplicitModal& obj);

	public:
		//模态类型，存储的字符串为模态分析/复模态分析,下拉框
		QString _modalType;

		//模态求解器，存储的字符串为自动多重子结构/兰索斯（只有模态分析可用）,下拉框
		QString _modalSolver;

		//几何大变形（只有模态分析可用）
		bool _nlgeo;

		//复模态计算方法,存储的字符串为无/精确法/估计法（只有复模态分析可用）,下拉框
		QString _complexMethod;

		//////////////////////////////////////////////模态分析的频率范围
		//最小频率（可以为空）
		QString _minFrequency;

		//最大频率（可以为空）
		QString _maxFrequency;

		//阶数（可以为空）
		QString _order;

		//容差（可以为空）
		QString _tolerance;

		//放大系数（可以为空）,兰索斯不可用放大系数
		QString _ampffact;

		//特征向量,下拉框(无/广义质量/最大位移)，自动多重子结构不可用特征向量
		QString _normalization;

		//////////////////////////////////////////////复模态分析的复模态值
		//根数目（不能为空）
		QString _rootNum;

		//////////////////////////////////////////////复模态分析的阻尼参数
		//结构阻尼G（可以为空）
		QString _dampingG;

		//////////////////////////////////////////////复模态分析的转子动力学分析部分
		//参考坐标系
		bool _rotationalChecked = false;
		QString _standCoor = "静止坐标系";
		bool _picChecked = false;
		QString _picValue;

	};
}
