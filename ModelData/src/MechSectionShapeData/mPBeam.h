#pragma once
#include <QtCore/qglobal.h>

//#ifndef BEAMDATA_EXPORT
//#  ifdef SECTIONSHAPEDATA_LIB          // CMake 里已定义
//#    define BEAMDATA_EXPORT Q_DECL_EXPORT
//#  else
//#    define BEAMDATA_EXPORT Q_DECL_IMPORT
//#  endif
//#endif
#include "MXSectionShapeData_global.h"
//#include "mdataproperty_global.h"
#include <QObject>
#include "QVector2D"
#include <qhash.h>
#include "mPhysicalFieldEnum.h"
//#include "mguibeam_global.h"


// #include <AIS_InteractiveObject.hxx>

using namespace MxFunctions;
namespace MechSectionShapeData
{
	class MXSECTIONSHAPEDATA_EXPORT mPBeam : public QObject
	{
	public:
		mPBeam();
		~mPBeam() = default;
		//获取单例指针
		static mPBeam* getInstance();
		virtual void setSecType(SecType t);
		virtual void setName(QString name);
		QString setDefaultNameForBeam(SecType);

		
		QString _name;
		SecType _Sectype;
		QHash<QString, QList<double>>    _nameParaListPair;
		QHash<QString, QList<double>>    _nameDataListPair;
		QHash<QString, QList<QVector2D>> _namePointListPair;
		  QString                        _deleteName{};
		  QList<QString>                 _beamNamelist{};
		 

		// 读文件的梁截面参数
		 QHash<QString, QList<double>> _ioNameParaListPair;

		 //临时容器，用来存储新添加的几种截面
		 QHash<QString, QList<double>> _tempNameParaListPair;

	private:
		static mPBeam* _instance;
		
		
	};
}

