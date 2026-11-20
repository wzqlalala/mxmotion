#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QList>
#include "mLoadBoundaryBase.h"

namespace MechCaseData
{
	//谱载荷
	class MXCASEDATA_EXPORT mLoadSpectrum : public mLoadBoundaryBase
	{
	public:
		/*
		* 构造函数
		*/
		mLoadSpectrum(int id, MxFunctions::LoadBoundaryType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mLoadSpectrum() {}

		/*
		* 析构函数
		*/
		~mLoadSpectrum();

		/*
		* 拷贝
		*/
		std::shared_ptr<mLoadBoundaryBase> copyLoadBoundary(const int& id, const QString& name) override;

		/*
		* 数据完整性判断
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mLoadSpectrum& obj);
		friend QDataStream& InStreammLoadSpectrumV3(QDataStream& in, mLoadSpectrum& obj);
		friend QDataStream& operator<<(QDataStream& out, mLoadSpectrum& obj);

	public:
		
		QString type_;//谱类型
		QList<QString> spectrumCurveList_;//普曲线
		QList<QString> dampValueList_;//阻尼值

	};
}