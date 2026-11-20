#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>


#include <map>
#include<QStringList>
#include <QDataStream>
#include "mModelEnum.h"

namespace MechCaseData
{
	class MXCASEDATA_EXPORT mNsmUnderCaseData
	{
	public:
		/*
		* 重载构造函数
		*/
		mNsmUnderCaseData(int id);

		/*
		* 仅用于输入输出流
		*/
		mNsmUnderCaseData() {}

		/*
		* 析构函数
		*/
		~mNsmUnderCaseData();

		/*
		* 拷贝非结构质量属性
		*/
		mNsmUnderCaseData* copyNsmUnderCaseData(const int & id);


		QStringList getNsmStrs() { return _nsmStrs; }
		void setNsmStrs(QStringList strs) { _nsmStrs = strs; }

		void appendNsm(QString str) { _nsmStrs.append(str); }
		void removeNsm(QString str) { _nsmStrs.removeOne(str); }
		/*
		* 判读数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info);

		friend QDataStream& operator>>(QDataStream& in, mNsmUnderCaseData& mgr);
		friend QDataStream& operator<<(QDataStream& out, mNsmUnderCaseData& mgr);

	private:

		int _caseId;

		QStringList _nsmStrs;
	};

	inline QDataStream& operator<<(QDataStream& out, mNsmUnderCaseData& mgr)
	{
		out << mgr._caseId << mgr._nsmStrs;
		return out;
	}

	inline QDataStream& operator>>(QDataStream& in, mNsmUnderCaseData& mgr)
	{
		in >> mgr._caseId >> mgr._nsmStrs;
		return in;
	}
}
