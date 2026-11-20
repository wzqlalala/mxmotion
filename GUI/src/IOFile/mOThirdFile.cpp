#include "mOThirdFile.h"
#include "mOMxFile.h"
#include "mMechGlobalSignals.h"


//QT
#include <QCoreApplication>
#include <QFileInfo>

using namespace MechCommon;
namespace MechIOFile
{
    mOThirdFile::mOThirdFile(const QString& filename) :_fileName(filename)
    {
        _isSuccessWrite = false;
        _mxFile = nullptr;
    }

    mOThirdFile::~mOThirdFile()
    {
        if (_mxFile != nullptr)
        {
            delete _mxFile;
            _mxFile = nullptr;
        }
    }

    bool mOThirdFile::isWriteSuccess()
    {
        return _isSuccessWrite;
    }

    QString mOThirdFile::getOutFilePath()
    {
        return _fileName;
    }

    void mOThirdFile::setCaseFilter(const std::vector<int>& caseIDs)
    {
        //须在start()前调用；run()在子线程中把过滤列表透传给写文件器
        _caseFilter = caseIDs;
    }

    void mOThirdFile::run()
    {
        //写前通知界面，无参数，阻塞等待界面处理完成
        emit MechCommon::mMechGlobalSignals::getInstance()->requestPrepareWriteFile();

        QFileInfo finfo(_fileName);

        //判断分析模块
        bool iswrite = false;

        //判断文件类型
        QString fsuffix = finfo.suffix();
        if (fsuffix == QString("mx"))
        {
            _mxFile = new mOMxFile(_fileName);
            _mxFile->setCaseFilter(_caseFilter);
            iswrite = _mxFile->writeMxFile();
        }

        if (iswrite)
        {
            emit writeFileSuccessSig(_fileName);
            _isSuccessWrite = true;
        }
    }
}