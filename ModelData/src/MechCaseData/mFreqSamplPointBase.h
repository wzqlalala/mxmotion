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

using namespace CommonData;
namespace MechCaseData
{
	struct FreqSamplPoint1
	{
		double startFreq;   // 起始频率
		double stepFreq;    // 频率增量
		int    intervalNum; // 间隔个数
	};

	struct FreqSamplPoint2
	{
		double startFreq;   // 起始频率
		double stepFreq;    // 频率增量
		int    totalPoints; // 采样点总数
	};

	struct FreqSamplPoint3
	{
		double startFreq;   // 起始频率
		double endFreq;     // 终止频率
		int    refineNum;   // 加密个数
		double expandValue; // 拓展量
		int    diffType;    // 差值类型
	};

	struct FreqSamplPoint4
	{
		double lowerFreq;   // 下限频率
		double stepFreq;    // 频率增量
		double shiftValue;  // 频移量
		int    sampleNum;   // 采样点数量
	};

	class MXCASEDATA_EXPORT mFreqSamplPointBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mFreqSamplPointBase(int id, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mFreqSamplPointBase() {}

		/*
		* 析构函数
		*/
		~mFreqSamplPointBase();

		
		/*
		* 设置ID
		*/
		void setFreqSamplPointId(int id);

		/*
		* 获取ID
		*/
		int getFreqSamplPointId();

		/*
		* 设置名
		*/
		void setFreqSamplPointName(const QString& name);

		/*
		* 获取名
		*/
		QString getFreqSamplPointName();

		/*
		* 拷贝
		*/
		mFreqSamplPointBase* copyFreqSamplPoint(const int& id, const QString& name);

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info);

		friend QDataStream& operator>>(QDataStream& in, mFreqSamplPointBase& obj);
		friend QDataStream& InStreammFreqSamplPointBaseV3(QDataStream& in, mFreqSamplPointBase& obj);
		friend QDataStream& operator<<(QDataStream& out, mFreqSamplPointBase& obj);

	private:
		//ID
		int _id;

		//名字
		QString _freqSamplPointName;

	public:
		//勾选状态
		QList<bool> _checked;

		//存储5个表格的数据
		QVector<double> _freqSamplPoint;
		FreqSamplPoint1 _freqSamplPoint1;
		FreqSamplPoint2 _freqSamplPoint2;
		FreqSamplPoint3 _freqSamplPoint3;
		FreqSamplPoint4 _freqSamplPoint4;
	};
}