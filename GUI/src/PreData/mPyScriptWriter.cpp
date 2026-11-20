#include "mPyScriptWriter.h"

//MBasicFunction
#include "mGlobalVarible.h"

//QT
#include <QFile>
#include <QTextStream>

using namespace MxFunctions;
namespace MechPreData
{
	mPyScriptWriter::mPyScriptWriter(const QString& filename) :_fileName(filename)
	{

	}

	mPyScriptWriter::~mPyScriptWriter()
	{

	}

	void mPyScriptWriter::run()
	{
		QFile file(_fileName);
		if (file.open(QIODevice::Text | QIODevice::WriteOnly | QIODevice::Truncate))
		{
			QTextStream writer(&file);
			const int pycount = _pythonCommands.size();
			for (int i = 0; i < pycount; ++i)
			{
				QString pycmd = _pythonCommands.at(i);
				writer << pycmd << "\n";
			}
			file.close();
		}
	}
}