#include "mOutputInfoWindow.h"
#include "UiMetrics.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"
#include "mTitleBar.h"
#include "mPythonAgent.h"

//Qt
#include <QComboBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QFont>
#include <QSplitter>
#include <QTextStream>
#include <QProcess>
#include <QDir>

#include "mMechGlobalSignals.h"
#include "mDataStatusManager.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mOtherFunction.h"

#include "mPythonAgent.h"

using namespace MechPreData;
using namespace MechCommon;
using namespace MAppConfiguration;
namespace MechMainWindow
{
    mOutputInfoWindow* mOutputInfoWindow::_instance = nullptr;
    mOutputInfoWindow* mOutputInfoWindow::getInstance()
    {
        if (_instance == nullptr)
        {
            _instance = new mOutputInfoWindow;
            _instance->setObjectName("OutputDock");
        }
        return _instance;
    }

    mOutputInfoWindow::mOutputInfoWindow(QWidget* parent) :QWidget(parent)
    {
        //初始化日志文件
        this->initLogFile();
        //设置允许的位置
        this->setStyleSheet("QTextEdit { font-size: 14px; }");

        _error = new QCheckBox("错误", this);
        _warning = new QCheckBox("警告", this);
        _normalInfo = new QCheckBox("常态信息", this);
        _dynamicInfo = new QCheckBox("动态信息", this);
        _error->setChecked(true);
        _warning->setChecked(true);
        _normalInfo->setChecked(true);
        _dynamicInfo->setChecked(true);

        QString checkBoxStyle = "QCheckBox { border: none; background: transparent; }"
            "QCheckBox::indicator { width: 14px; height: 14px; }";
        _error->setStyleSheet(checkBoxStyle);
        _warning->setStyleSheet(checkBoxStyle);
        _normalInfo->setStyleSheet(checkBoxStyle);
        _dynamicInfo->setStyleSheet(checkBoxStyle);

        connect(_dynamicInfo, &QCheckBox::stateChanged, this, &mOutputInfoWindow::slot_tableInfoDataShow);
        connect(_error, &QCheckBox::stateChanged, this, &mOutputInfoWindow::slot_tableInfoDataShow);
        connect(_warning, &QCheckBox::stateChanged, this, &mOutputInfoWindow::slot_tableInfoDataShow);
        connect(_normalInfo, &QCheckBox::stateChanged, this, &mOutputInfoWindow::slot_tableInfoDataShow);

        _textEdit = new mChineseTextEdit(this);
        _textEdit->setReadOnly(true);
        // 使用新宋体/宋体：Windows 下保证中英文严格等宽（中文宽度 = 2 × 英文宽度）
        QFont font("NSimSun", 10);
        if (!QFontInfo(font).exactMatch())
            font.setFamily("SimSun");
        font.setStyleHint(QFont::TypeWriter);
        _textEdit->setFont(font);
        // 统一 Tab Stop 为 8 个空格，与命令行保持一致
        _textEdit->setTabStopDistance(QFontMetrics(font).horizontalAdvance(' ') * 8);

        _commandEdit = new QLineEdit(this);
        _commandEdit->setFixedHeight(28);
        _commandEdit->setStyleSheet(QStringLiteral("font-family:'%1'")
            .arg(MxFunctions::UiMetrics::uiFontFamily()));
        _commandEdit->setPlaceholderText("请输入Python命令");
        _commandEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        _tableWidget = new QWidget(this);
        _tableWidget->setStyleSheet("alternate-background-color:#ECF5FF;");
        _tableWidget->setMinimumWidth(144);

        clearTable = new QPushButton("清空常态信息", this);
        connect(clearTable, &QPushButton::clicked, this, &mOutputInfoWindow::slot_clearNormalInfo);
        QHBoxLayout* tableHLayout = new QHBoxLayout();
        tableHLayout->setContentsMargins(0, 0, 0, 0);
        tableHLayout->addWidget(_error);
        tableHLayout->addWidget(_warning);
        tableHLayout->addWidget(_normalInfo);
        tableHLayout->addWidget(_dynamicInfo);
        tableHLayout->addStretch();
        tableHLayout->addWidget(clearTable);

        _table = new QTableWidget(this);
        _table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        _table->setSelectionBehavior(QAbstractItemView::SelectItems);
        _table->setSelectionMode(QAbstractItemView::ExtendedSelection);
        _table->horizontalHeader()->setStretchLastSection(true);
        _table->verticalHeader()->setVisible(false);
        _table->verticalHeader()->setDefaultSectionSize(fontMetrics().height() + 10);
        _table->setColumnCount(6);
        QStringList headerText1;
        headerText1 << QString("序号") << QString("类型") << QString("模块") << QString("对象") << QString("说明") << QString("id");
        _table->setHorizontalHeaderLabels(headerText1);
        _table->hideColumn(5);

        //设置交替换行
        _table->setAlternatingRowColors(true);

        QVBoxLayout* tableLayout = new QVBoxLayout();
        tableLayout->setContentsMargins(5, 5, 5, 5);
        tableLayout->addLayout(tableHLayout);
        tableLayout->addWidget(_table);
        _tableWidget->setLayout(tableLayout);

        QWidget* leftWidget = new QWidget();
        leftWidget->setMinimumWidth(144);
        QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);
        leftLayout->setContentsMargins(5, 5, 5, 5);

        leftLayout->addWidget(_commandEdit);

        leftLayout->addWidget(_textEdit, 1);

        CollapsibleSplitter* splitter = new CollapsibleSplitter(this);
        splitter->setSizes({ 800, 400 });
        splitter->setHandleWidth(30);
        splitter->addWidget(leftWidget);
        splitter->addWidget(_tableWidget);
        splitter->setCollapsible(0, false);  // 禁止左侧折叠到 0

        QHBoxLayout* mainLayout = new QHBoxLayout(this);
        mainLayout->setContentsMargins(5, 5, 5, 5);
        mainLayout->addWidget(splitter, 1);

        this->setLayout(mainLayout);

        connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(outputMessageSig(const QString&, bool, bool)), this, SLOT(outputMessage(const QString&, bool, bool)));
        connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(outputMessageSig(int, const QString&, bool, bool)), this, SLOT(outputMessage(int, const QString&, bool, bool)));
        connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::updateOutPutInfoTable, this, &mOutputInfoWindow::slot_updateOutPutInfoTable);
        connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::resetNameUpdateOutPutInfoTable, this, &mOutputInfoWindow::slot_resetName);
        connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::updataNormalInfo, this, &mOutputInfoWindow::slot_updataNormalInfo);
        connect(MechPreData::mViewportModelManager::getInstance(), &MechPreData::mViewportModelManager::currentViewportChanged,
            this, [this](int) { slot_tableInfoDataShow(0); });
        //行编辑框的回车信号与槽
        connect(_commandEdit, SIGNAL(returnPressed()), this, SLOT(enterKeySlot()));
        updateClearBtnState();
    }

    void mOutputInfoWindow::slot_updateOutPutInfoTable(const QHash<QString, QSet<int>>& deleteInfoData, const QList<outputInfoData>& infoData)
    {
        //删除段永不受显示过滤影响：过期行必须离开表格与计数。此前整个槽被
        //"动态信息"复选框前置拦截，未勾选时旧告警行（如求解完成后应消失的
        //"多体方案未求解"）删除被跳过而永久残留；显示过滤只作用于下方新增段
        for (int row = _table->rowCount() - 1; row >= 0; --row)
        {
            QTableWidgetItem* item1 = _table->item(row, 1);
            QTableWidgetItem* item2 = _table->item(row, 2);
            QTableWidgetItem* item5 = _table->item(row, 5);
            QString id = item5->text();
            if (item2 && item5 &&
                deleteInfoData.constFind(item2->text()) != deleteInfoData.cend() &&
                deleteInfoData[item2->text()].find(item5->text().toInt()) != deleteInfoData[item2->text()].cend())
            {
                _count[item1->text()]--;
                _table->removeRow(row);
            }
        }

        //更新层号
        int nrow = _table->rowCount();
        for (int i = 0; i < nrow; ++i)
        {
            QTableWidgetItem* item = _table->item(i, 0);
            if (item)
            {
                item->setText(MxFunctions::mNumberToString(i + 1));
            }
        }

        //新增段受显示过滤（与 slot_tableInfoDataShow 整表重建口径一致）
        if (!_dynamicInfo->isChecked())
        {
            mGlobalSignals::getInstance()->outPutInfoShowSig();
            return;
        }

        set<QString> type;
        if (_error->isChecked())
            type.insert("错误");
        if (_warning->isChecked())
            type.insert("警告");

        if (type.size() == 0)
        {
            mGlobalSignals::getInstance()->outPutInfoShowSig();
            return;
        }

        for (const outputInfoData& data : infoData) {
            if (type.find(data._type) == type.cend())
                continue;

            _count[data._type]++;
            _table->insertRow(nrow);
            _table->setItem(nrow, 0, new QTableWidgetItem(MxFunctions::mNumberToString(nrow + 1)));
            _table->setItem(nrow, 1, new QTableWidgetItem(data._type));
            _table->setItem(nrow, 2, new QTableWidgetItem(data._module));
            _table->setItem(nrow, 3, new QTableWidgetItem(data._object));
            _table->setItem(nrow, 4, new QTableWidgetItem(data._explain));
            _table->setItem(nrow, 5, new QTableWidgetItem(MxFunctions::mNumberToString(data._ID)));
            nrow++;
        }

        mGlobalSignals::getInstance()->outPutInfoShowSig();
        updateClearBtnState();
    }

    void mOutputInfoWindow::slot_resetName(const QString& module, const int& phyID, const QString& newName)
    {
        for (int row = _table->rowCount() - 1; row >= 0; --row)
        {
            QTableWidgetItem* item2 = _table->item(row, 2);
            QTableWidgetItem* item3 = _table->item(row, 3);
            QTableWidgetItem* item5 = _table->item(row, 5);
            if (item2 && item3 && item5 &&
                item2->text() == module &&
                item5->text().toInt() == phyID)
            {
                item3->setText(newName);
            }
        }
    }

    void mOutputInfoWindow::slot_updataNormalInfo(QList<outputInfoData>& infoData)
    {
        if (!_normalInfo->isChecked())
            return;

        int nrow = _table->rowCount();
        set<QString> type;
        if (_error->isChecked())
            type.insert("错误");
        if (_warning->isChecked())
            type.insert("警告");

        if (type.size() == 0)
            return;
        for (const outputInfoData& data : infoData) {
            if (type.find(data._type) == type.cend())
                continue;

            _count[data._type]++;
            _table->insertRow(nrow);
            _table->setItem(nrow, 0, new QTableWidgetItem(MxFunctions::mNumberToString(nrow + 1)));
            _table->setItem(nrow, 1, new QTableWidgetItem(data._type));
            _table->setItem(nrow, 2, new QTableWidgetItem(data._module));
            _table->setItem(nrow, 3, new QTableWidgetItem(data._object));
            _table->setItem(nrow, 4, new QTableWidgetItem(data._explain));
            _table->setItem(nrow, 5, new QTableWidgetItem(MxFunctions::mNumberToString(data._ID)));
            nrow++;
        }

        mGlobalSignals::getInstance()->outPutInfoShowSig();
        updateClearBtnState();
    }

    void mOutputInfoWindow::slot_clearNormalInfo()
    {
        if (!getCVM)
            return;

        getCVM->getDataStatusManager()->clearNormalInfoData();
        if (!_normalInfo->isChecked())
            return;

        _count["警告"] = 0;
        _count["错误"] = 0;
        if (_dynamicInfo->isChecked())
        {
            const QList<outputInfoData>& infoData = getCVM->getDataStatusManager()->getDynamicInfoData();

            int nrow = 0;
            set<QString> type;
            if (_error->isChecked())
                type.insert("错误");
            if (_warning->isChecked())
                type.insert("警告");

            _table->setRowCount(0);
            if (type.size() == 0)
            {
                mGlobalSignals::getInstance()->outPutInfoShowSig();
                return;
            }

            for (const outputInfoData& data : infoData) {
                if (type.find(data._type) == type.cend())
                    continue;

                _count[data._type]++;
                _table->insertRow(nrow);
                _table->setItem(nrow, 0, new QTableWidgetItem(MxFunctions::mNumberToString(nrow + 1)));
                _table->setItem(nrow, 1, new QTableWidgetItem(data._type));
                _table->setItem(nrow, 2, new QTableWidgetItem(data._module));
                _table->setItem(nrow, 3, new QTableWidgetItem(data._object));
                _table->setItem(nrow, 4, new QTableWidgetItem(data._explain));
                _table->setItem(nrow, 5, new QTableWidgetItem(MxFunctions::mNumberToString(data._ID)));
                nrow++;
            }
        }
        else
        {
            _table->setRowCount(0);
        }

        mGlobalSignals::getInstance()->outPutInfoShowSig();
        
    }

    void mOutputInfoWindow::updateClearBtnState()
    {
        if (!getCVM || !clearTable)
            return;

        auto* manager = getCVM->getDataStatusManager();
        if (!manager)
            return;

        bool hasData = !manager->getNormalInfoData().isEmpty() || !manager->getDynamicInfoData().isEmpty();
        clearTable->setEnabled(hasData);
    }

    //void mOutputInfoWindow::slot_clearNormalInfo()
    //{
    //    if (!getCVM)
    //        return;

    //    auto* manager = getCVM->getDataStatusManager();
    //    if (!manager)
    //        return;

    //    manager->clearNormalInfoData();
    //    manager->updataDynamicInfoData().clear();  // 注意接口名是 updata，不是 update

    //    slot_tableInfoDataShow(0);
    //   
    //}

    void mOutputInfoWindow::slot_tableInfoDataShow(int state)
    {
        if (!getCVM)
            return;

        const QList<outputInfoData>& dynamicInfoData = getCVM->getDataStatusManager()->getDynamicInfoData();
        const QList<outputInfoData>& normalInfoData = getCVM->getDataStatusManager()->getNormalInfoData();

        set<QString> type;
        if (_error->isChecked())
            type.insert("错误");
        if (_warning->isChecked())
            type.insert("警告");

        QList<outputInfoData> infoData;
        if (_normalInfo->isChecked())
            infoData += normalInfoData;
        if (_dynamicInfo->isChecked())
            infoData += dynamicInfoData;

        _table->setRowCount(0);
        _count["警告"] = 0;
        _count["错误"] = 0;
        if (type.size() == 0 || infoData.size() == 0)
        {
            mGlobalSignals::getInstance()->outPutInfoShowSig();
            return;
        }

        int nrow = 0;
        for (const outputInfoData& data : infoData) {
            if (type.find(data._type) == type.cend())
                continue;

            _count[data._type]++;
            _table->insertRow(nrow);
            _table->setItem(nrow, 0, new QTableWidgetItem(MxFunctions::mNumberToString(nrow + 1)));
            _table->setItem(nrow, 1, new QTableWidgetItem(data._type));
            _table->setItem(nrow, 2, new QTableWidgetItem(data._module));
            _table->setItem(nrow, 3, new QTableWidgetItem(data._object));
            _table->setItem(nrow, 4, new QTableWidgetItem(data._explain));
            _table->setItem(nrow, 5, new QTableWidgetItem(MxFunctions::mNumberToString(data._ID)));
            nrow++;
        }

        mGlobalSignals::getInstance()->outPutInfoShowSig();
        updateClearBtnState();
    }


    mOutputInfoWindow::~mOutputInfoWindow()
    {
        if (_textEdit)
        {
            delete _textEdit; _textEdit = nullptr;
        }
        if (_error)
        {
            delete _error; _error = nullptr;
        }
        if (_warning)
        {
            delete _warning; _warning = nullptr;
        }
        if (_normalInfo)
        {
            delete _normalInfo; _normalInfo = nullptr;
        }
        if (_dynamicInfo)
        {
            delete _dynamicInfo; _dynamicInfo = nullptr;
        }
        if (_table)
        {
            delete _table; _table = nullptr;
        }
        if (_tableWidget)
        {
            delete _tableWidget; _tableWidget = nullptr;
        }
    }

    std::pair<int, int> mOutputInfoWindow::getErrorWarningCount()
    {
        return { _count["警告"],_count["错误"] };
    }

    void mOutputInfoWindow::showTableInfo()
    {
        if (_error->isChecked() && _warning->isChecked() && _dynamicInfo->isChecked())
            return;

        disconnect(_dynamicInfo, &QCheckBox::stateChanged, this, &mOutputInfoWindow::slot_tableInfoDataShow);
        disconnect(_error, &QCheckBox::stateChanged, this, &mOutputInfoWindow::slot_tableInfoDataShow);
        disconnect(_warning, &QCheckBox::stateChanged, this, &mOutputInfoWindow::slot_tableInfoDataShow);

        _error->setChecked(true);
        _warning->setChecked(true);
        _dynamicInfo->setChecked(true);

        connect(_dynamicInfo, &QCheckBox::stateChanged, this, &mOutputInfoWindow::slot_tableInfoDataShow);
        connect(_error, &QCheckBox::stateChanged, this, &mOutputInfoWindow::slot_tableInfoDataShow);
        connect(_warning, &QCheckBox::stateChanged, this, &mOutputInfoWindow::slot_tableInfoDataShow);

        slot_tableInfoDataShow(0);
    }

    void mOutputInfoWindow::outputMessage(int Type, const QString& msg, bool statusBar, bool outputTime)
    {
        // 获取当前光标，清除选中状态并移动到末尾，防止修改历史选中文本的颜色
        QTextCursor cursor = _textEdit->textCursor();
        cursor.clearSelection();
        cursor.movePosition(QTextCursor::End);
        _textEdit->setTextCursor(cursor);

        // 根据消息类型设置对应的文本颜色
        MxFunctions::OutputMessageType type = MxFunctions::OutputMessageType(Type);
        switch (type)
        {
        case MxFunctions::NormalMessage:
            _textEdit->setTextColor(NormalMessageColor);
            break;
        case MxFunctions::WarningMessage:
            _textEdit->setTextColor(WarningMessageColor);
            break;
        case MxFunctions::ErrorMessage:
            _textEdit->setTextColor(ErrorMessageColor);
            break;
        case MxFunctions::PythonCommand:
            _textEdit->setTextColor(PythonMessageColor);
            break;
        default:
            _textEdit->setTextColor(NormalMessageColor);
            break;
        }

        _time = QTime::currentTime().toString("hh:mm:ss");
        QString outputText;

        // 组装输出文本并写入日志文件
        if (outputTime)
        {
            outputText = QString(">>  %1").arg(msg);
            outputText = QString("%1  [  %2  ]").arg(outputText, _time);
            this->writeLogFile(msg + "[" + _time + "]");
        }
        else
        {
            outputText = QString("%1").arg(msg);
            this->writeLogFile(msg);
        }

        // 在面板追加最终文本
        _textEdit->append(outputText);

        // 在状态栏上输出信息
        if (statusBar)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputStatusBarSig(msg);
        }
    }

    void mOutputInfoWindow::outputMessage(const QString& msg, bool statusBar, bool outputTime)
    {
        outputMessage(0, msg, statusBar, outputTime);
    }

    void mOutputInfoWindow::enterKeySlot()
    {
        QString linetext = _commandEdit->text();
        if (linetext.isEmpty())
        {
            return;
        }

        //判断是否包含括号，若不包含则在最后添加一个空括号
        const int lasti = linetext.size() - 1;
        if (linetext.at(lasti) != ")")
        {
            linetext += "()";
        }

        //发送命令
        MechPreData::mPythonAgent::getInstance()->submitCommand(linetext);

        //清空行编辑框
        _commandEdit->clear();
    }

    void mOutputInfoWindow::initLogFile()
    {
        //获取当前日期时间
        QString dt = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");

        //获取当前工作目录
#ifdef _WIN32
        //QString wd = QDir(qApp->applicationDirPath() + "/../LogFiles").absolutePath();
        QString wd = mAppConfiguration::getInstance()->_logFileDir + "/v" + QApplication::applicationVersion();
#else
        //QString wd = QDir(MAppConfiguration::mAppConfiguration::getInstance()->_workingDir + "/../LogFiles").absolutePath();
        QString wd = mAppConfiguration::getInstance()->_logFileDir + "/v" + QApplication::applicationVersion();
#endif
        QDir dir;
        if (!dir.exists(wd))
        {
            dir.mkpath(wd);
        }

        //建立文件
        QString fname = wd + "/" + dt + ".log";
        QFile file(fname);
        file.open(QIODevice::WriteOnly | QIODevice::Truncate);
        file.close();

        _logfile = fname;

        _file.setFileName(fname);
        _file.open(QIODevice::WriteOnly | QIODevice::Append);
    }

    void mOutputInfoWindow::writeLogFile(const QString& text)
    {
        /*QFile file(_logfile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Append))
        {
            QTextStream writer(&file);
            writer << text << endl;
        }
        file.close();*/

        QTextStream writer(&_file);
        writer.setCodec("UTF-8");
        writer << text << endl;
    }
}
