#include "mOMxsesFile.h"

//Qt
#include <QFileInfo>
#include <QSaveFile>
#include <QMutexLocker>

//全局信号
#include "mGlobalSignals.h"
#include "mMechGlobalSignals.h"

using namespace MechCommon;
using namespace MxFunctions;

//会话格式版本号（V4：页内图表卡片条目、曲线文档快照、后处理结果文件与树条目）
const quint32 MXSES_VERSION = 0x4000;

namespace MechIOFile
{
	mOMxsesFile::mOMxsesFile(const QString& name, const MxsesFileData& data, int compressLevel)
		: _fileName(name), _data(data), _compressLevel(compressLevel)
	{
		//转化成绝对路径
		QFileInfo finfo(_fileName);
		_fileName = finfo.absoluteFilePath();
	}

	mOMxsesFile::~mOMxsesFile()
	{
	}

	QString mOMxsesFile::getFileName()
	{
		return _fileName;
	}

	void mOMxsesFile::run()
	{
		if (_cancelRequested) {
			return;
		}

		QMutexLocker locker(&mutex);

		//整体序列化会话数据
		QByteArray dataBuffer;
		QDataStream dataStream(&dataBuffer, QIODevice::WriteOnly);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
		dataStream.setVersion(QDataStream::Qt_5_13);
#else
		dataStream.setVersion(QDataStream::Qt_5_12);
#endif
		dataStream << _data;

		if (_cancelRequested) {
			mGlobalSignals::getInstance()->outputMessageSig(QString("写入任务已取消： %1").arg(_fileName));
			return;
		}

		//压缩数据（与 mxcae 相同的 4MB 分块 qCompress 容器）
		const int blockSize = 4 * 1024 * 1024;
		int numBlocks = (dataBuffer.size() + blockSize - 1) / blockSize;
		QVector<QByteArray> compressedBlocks(numBlocks);

#pragma omp parallel for
		for (int i = 0; i < numBlocks; ++i) {
			if (_cancelRequested) {
				continue;
			}
			QByteArray chunk = dataBuffer.mid(i * blockSize, blockSize);
			QByteArray compressedData = qCompress(chunk, _compressLevel);
			compressedBlocks[i] = compressedData;
		}

		if (_cancelRequested) {
			mGlobalSignals::getInstance()->outputMessageSig(QString("写入任务已取消： %1").arg(_fileName));
			return;
		}

		QByteArray allCompressed;
		for (const QByteArray& block : compressedBlocks) {
			quint32 blen = block.size();
			allCompressed.append(reinterpret_cast<const char*>(&blen), sizeof(blen));
			allCompressed.append(block);
		}

		QSaveFile file(_fileName);
		if (_cancelRequested || !file.open(QIODevice::WriteOnly)) {
			if (_cancelRequested) {
				mGlobalSignals::getInstance()->outputMessageSig(QString("写入任务已取消： %1").arg(_fileName));
				return;
			}
			emit writeMxsesErrorSig(FileNotOpen); 
			return;
		}

		QDataStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
		out.setVersion(QDataStream::Qt_5_13);
#else
		out.setVersion(QDataStream::Qt_5_12);
#endif

		//写入文件头
		out << MXSES_VERSION;

		//写入压缩后的数据块
		out << allCompressed;

		if (_cancelRequested) {
			mGlobalSignals::getInstance()->outputMessageSig(QString("写入任务已取消： %1").arg(_fileName));
			return;
		}

		if (!file.commit()) {
			qWarning("Failed to commit save file: %s", qPrintable(file.errorString()));
			return;
		}
	}
}
