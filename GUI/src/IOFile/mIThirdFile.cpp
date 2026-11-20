#include "mIThirdFile.h"
#include "mIMxFile.h"
//QT
#include <QCoreApplication>
#include <QFileInfo>
#include <QTextStream>
#include <QDirModel>
#include "mWidgetOperation.h"
#include "mLanguageVarible.h"

//MxFunctions
#include "mGlobalSignals.h"
//using namespace MechPreData;
using namespace MechCommon;
namespace MechIOFile
{
	mIThirdFile::mIThirdFile(const QString& filename, bool isMeshFile) 
		:_fileName(filename), _isMeshFile(isMeshFile)
	{
		_isSuccessRead = true;
		_mxFile = new mIMxFile;
	}

	mIThirdFile::~mIThirdFile()
	{
		if (_mxFile != nullptr)
		{
			delete _mxFile;
			_mxFile = nullptr;
		}
	}

	bool mIThirdFile::isSuccessReadFile()
	{
		return _isSuccessRead;
	}

	QString mIThirdFile::getReadFileName()
	{
		return _fileName;
	}

	AnalysisModule mIThirdFile::getMxFileType()
	{
		QFileInfo finfo(_fileName);
		QString fsuffix = finfo.suffix();
		if (fsuffix == QString("fem") || fsuffix == QString("bdf") || fsuffix == QString("inp") || fsuffix == QString("cdb"))
		{
			return ImplicitAnalysis;
		}
		else if (fsuffix == QString("k"))
		{
			return ExplicitAnalysis;
		}
		else if (fsuffix == QString("msh"))
		{
			return FluidIncompressibleAnalysis;
		}
		else if (fsuffix == QString("cas"))
		{
			return FluidCompressibleAnalysis;
		}
		else if (fsuffix == QString("mx"))
		{
			QFile file(_fileName);
			if (file.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				QTextStream reader(&file);
				//读取第一行
				QString firstline = reader.readLine();
				if (firstline.contains("implicit", Qt::CaseInsensitive))
				{
					return ImplicitAnalysis;
				}
				else if (firstline.contains("explicit", Qt::CaseInsensitive))
				{
					return ExplicitAnalysis;
				}
				else if (firstline.contains("electromagnetic", Qt::CaseInsensitive))
				{
					return MagnetAnalysis;
				}
				else if (firstline.contains("incompressibleflowheattransfer", Qt::CaseInsensitive))
				{
					return FluidIncompressibleAnalysis;
				}
				else if (firstline.contains("compressibleflowheattransfer", Qt::CaseInsensitive))
				{
					return FluidCompressibleAnalysis;
				}
			}
			file.close();
		}
		return UnknownAnalysis;
	}

	void mIThirdFile::run()
	{
		//判断文件是否存在
		QFileInfo finfo(_fileName);
		if (!finfo.exists())
		{
			emit readSolveErrorSig(FileNotExist);
			_isSuccessRead = false;
			return;
		}

		//判断文件类型
		bool isread = false;
		QString fsuffix = finfo.suffix();
		if (fsuffix == QString("mx"))	//需进行判断
		{
			mGlobalSignals::getInstance()->outputMessageSig("开始读文件...");
			if (_isMeshFile)
			{
				_mxFile->setIsMeshFile();
			}
			_mxFile->scanMxFile(_fileName);
			isread = _mxFile->readMxFile(_fileName);
		}
		else
		{
			_isSuccessRead = false;
		}

		//发送信号
		if (isread)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputStatusBarSig("成功导入文件");
			emit readFileSuccessSig(_fileName);
		}
		else
		{
			emit readSolveErrorSig(FileNotOpen);
		}
	}
}
