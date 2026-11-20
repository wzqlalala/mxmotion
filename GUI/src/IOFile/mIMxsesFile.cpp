#include "mIMxsesFile.h"

//Qt
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>

//MechCommon
#include "mGlobalSignals.h"

using namespace MechCommon;
using namespace MxFunctions;

//会话格式版本号（V4：页内图表卡片条目、曲线文档快照、后处理结果文件与树条目）
static const quint32 MXSES_VERSION = 0x4000;

namespace MechIOFile
{
	mIMxsesFile::mIMxsesFile(const QString& name) : _filename(name)
	{
	}

	mIMxsesFile::~mIMxsesFile()
	{
	}

	QString mIMxsesFile::getFileName()
	{
		return _filename;
	}

	bool mIMxsesFile::isSuccessReadFile()
	{
		return _isSuccessRead;
	}

	void mIMxsesFile::run()
	{
		mGlobalSignals::getInstance()->outputMessageSig(QString("开始读取文件： %1").arg(_filename));
		QFile file(_filename);
		if (!file.open(QIODevice::ReadOnly)) {
			emit readMxsesErrorSig(FileNotOpen);
			return;
		}

		QDataStream in(&file);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
		in.setVersion(QDataStream::Qt_5_13);
#else
		in.setVersion(QDataStream::Qt_5_12);
#endif

		//文件头只写版本号（与 mxcae 的 WARE_VERSION 同机制）
		quint32 version = 0;
		in >> version;
		//版本高于当前程序时拒绝
		if (version > MXSES_VERSION) {
			emit readMxsesErrorSig(FileVersionFalse);
			return;
		}
		//记录读入文件的版本号，供 operator>> 分发版本函数（仿 FILE_VERSION）
		MXSES_FILE_VERSION = version;

		if (_cancelRequested) {
			_isSuccessRead = false;
			file.close();
			return;
		}

		QByteArray compressedData;
		in >> compressedData;
		file.close();

		//解压缩数据
		QByteArray input = compressedData;
		QByteArray decompressed;
		int pos = 0;
		while (pos + 4 <= input.size()) {
			if (_cancelRequested) {
				_isSuccessRead = false;
				return;
			}
			quint32 blockLen;
			memcpy(&blockLen, input.constData() + pos, 4);
			pos += 4;
			if (pos + blockLen > input.size()) break;
			QByteArray block = input.mid(pos, blockLen);
			QByteArray original = qUncompress(block);
			decompressed.append(original);
			pos += blockLen;
		}

		//反序列化会话数据（模型数据保留为chunk，不实例化 mModelData）
		QDataStream dataStream(decompressed);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
		dataStream.setVersion(QDataStream::Qt_5_13);
#else
		dataStream.setVersion(QDataStream::Qt_5_12);
#endif

		try {
			QMutexLocker locker(&mutex);
			MxsesFileData data;
			dataStream >> data;
			//数据读取失败或无任何模型说明文件格式不匹配/损坏
			if (dataStream.status() != QDataStream::Ok || data.models.isEmpty()) {
				emit readMxsesErrorSig(FileVersionFalse);
				return;
			}
			if (_cancelRequested) {
				_isSuccessRead = false;
				return;
			}
			_data = data;
			_isSuccessRead = true;
		}
		catch (...) {
			emit readMxsesErrorSig(UnknownError);
		}
	}
}
