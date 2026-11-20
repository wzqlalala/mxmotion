#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"
#include <QString>
#include <QStringList>
#include <QDataStream>
#include "mRoatateAnalyseBase.h"
#include<QVector3D>

namespace MechCaseData
{
	class MXCASEDATA_EXPORT mLoadRotationalSpeed : public mRoatateAnalyseBase
	{
	public:
		/*
		* 构造函数
		*/
		mLoadRotationalSpeed(int id, MxFunctions::RotateAnalysyType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mLoadRotationalSpeed() {}

		/*
		* 析构函数
		*/
		~mLoadRotationalSpeed();

		/*
		* 拷贝
		*/
		std::shared_ptr<mRoatateAnalyseBase> copyRotateAnalyse(const int& id, const QString& name) override;

		/*
		* 获取当前所选类型
		*/
		QString getSelectLoadType();

		/*
		* 数据完整性判断
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		friend MXCASEDATA_EXPORT QDataStream& operator>>(QDataStream& in, mLoadRotationalSpeed& obj);
		friend MXCASEDATA_EXPORT QDataStream& operator<<(QDataStream& out, mLoadRotationalSpeed& obj);

	public:
		QVector3D _startPoint1;
		QVector3D _directPoint2;

		QString _rotationalPart;

		int way_ = 2;  //载荷类型 0力 1位移 2速度 3加速度

		// 0：常数；1：曲线；2：时间曲线
		int _speedCurveType = 0;
		QStringList _tableHeaders;
		QList<QStringList> _tableData;

		// 各曲线类型独立保存的表格数据，用于切换页签后回显
		// 表格中不再输入起始点/方向点，起始点和方向点统一从面板上的控件获取
		QStringList _constantTableHeaders = QStringList{ "旋转速度" };
		QList<QStringList> _constantTableData;
		QStringList _curveTableHeaders = QStringList{ "旋转速度" };
		QList<QStringList> _curveTableData;
		QStringList _timeCurveTableHeaders = QStringList{ "时间", "旋转速度" };
		QList<QStringList> _timeCurveTableData;

		// 方向拾取控件 UI 状态，用于反显
		DirectionPickState _dirPickState;
	};
}
