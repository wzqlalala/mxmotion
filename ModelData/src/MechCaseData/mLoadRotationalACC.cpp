#include "mLoadRotationalACC.h"
#include <QDataStream>

using namespace MxFunctions;
namespace MechCaseData
{
	mLoadRotationalACC::mLoadRotationalACC(int id, MxFunctions::RotateAnalysyType type, const QString& name)
		:mRoatateAnalyseBase(id, type, name)
	{

	}

	mLoadRotationalACC::~mLoadRotationalACC()
	{

	}

	std::shared_ptr<mRoatateAnalyseBase> mLoadRotationalACC::copyRotateAnalyse(const int& id, const QString& name)
	{
		std::shared_ptr<mLoadRotationalACC> load = std::make_shared<mLoadRotationalACC>(id, getRotateAnalyseType(), name);
		load->_startPoint1 = _startPoint1;
		load->_directPoint2 = _directPoint2;
		load->_rotationalPart = _rotationalPart;
		load->way_ = way_;
		load->_accCurveType = _accCurveType;
		load->_tableHeaders = _tableHeaders;
		load->_tableData = _tableData;

		load->_constantTableHeaders = _constantTableHeaders;
		load->_constantTableData = _constantTableData;
		load->_timeCurveTableHeaders = _timeCurveTableHeaders;
		load->_timeCurveTableData = _timeCurveTableData;
		load->_dirPickState = _dirPickState;

		return load;
	}

	QString mLoadRotationalACC::getSelectLoadType()
	{
		if (way_ == 0)
		{
			return QString("load");
		}
		else if (way_ == 1)
		{
			return QString("disp");
		}
		else if (way_ == 2)
		{
			return QString("vel");
		}
		else if (way_ == 3)
		{
			return QString("acc");
		}
		return QString();
	}

	MxFunctions::BrowserTreeStatus mLoadRotationalACC::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
	}

	static void writeQStringListList(QDataStream& out, const QList<QStringList>& data)
	{
		out << data.size();
		for (const QStringList& row : data)
		{
			out << row;
		}
	}

	static void readQStringListList(QDataStream& in, QList<QStringList>& data)
	{
		int size = 0;
		in >> size;
		data.clear();
		data.reserve(size);
		for (int i = 0; i < size; ++i)
		{
			QStringList row;
			in >> row;
			data.append(row);
		}
	}

	QDataStream& operator<<(QDataStream& out, mLoadRotationalACC& obj)
	{
		out << static_cast<mRoatateAnalyseBase&>(obj);
		out << obj._startPoint1 << obj._directPoint2;
		out << obj._rotationalPart;
		out << static_cast<qint32>(obj.way_);
		out << static_cast<qint32>(obj._accCurveType);
		out << obj._tableHeaders;
		writeQStringListList(out, obj._tableData);
		out << obj._constantTableHeaders;
		writeQStringListList(out, obj._constantTableData);
		out << obj._timeCurveTableHeaders;
		writeQStringListList(out, obj._timeCurveTableData);
		out << obj._dirPickState.mode;
		out << obj._dirPickState.reversed;
		out << obj._dirPickState.startPoint;
		out << obj._dirPickState.endPoint;
		out << obj._dirPickState.selectedObject;
		out << obj._dirPickState.directInputDir;;
		return out;
	}

	QDataStream& operator>>(QDataStream& in, mLoadRotationalACC& obj)
	{
		in >> static_cast<mRoatateAnalyseBase&>(obj);
		in >> obj._startPoint1 >> obj._directPoint2;
		in >> obj._rotationalPart;
		qint32 way = 0;
		in >> way;
		obj.way_ = way;
		qint32 accCurveType = 0;
		in >> accCurveType;
		obj._accCurveType = accCurveType;
		in >> obj._tableHeaders;
		readQStringListList(in, obj._tableData);
		in >> obj._constantTableHeaders;
		readQStringListList(in, obj._constantTableData);
		in >> obj._timeCurveTableHeaders;
		readQStringListList(in, obj._timeCurveTableData);
		in >> obj._dirPickState.mode;
		in >> obj._dirPickState.reversed;
		in >> obj._dirPickState.startPoint;
		in >> obj._dirPickState.endPoint;
		in >> obj._dirPickState.selectedObject;
		in >> obj._dirPickState.directInputDir;
		return in;
	}
}
