#include "mPeakFilterPanel.h"

#include "mCalcLineEdit.h"
#include "mInfoWindow.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mGlobalParam.h"
#include "mTableDataWindow.h"
#include "mDockLayoutManager.h"
#include "mCaseManager.h"
#include "mCaseImplicitFrequency.h"
#include "mCaseBase.h"
#include "mDataStatusManager.h"

#include <QTimer>
#include <QPointer>
#include <QGridLayout>
#include <functional>
#include <algorithm>

using namespace MechCommon;
using namespace MechCaseData;
namespace MechMainWindow
{
    mPeakFilterPanel::mPeakFilterPanel(QWidget* parent)
        : mPanelBase(parent), data(nullptr)
    {
        _mainLayout = new QVBoxLayout(this);

        /**********************************************峰值筛选**********************************************************/

        FoldSection* peakFilterFold = new FoldSection("峰值筛选参数", this);

        // 响应类型
        _comboRespType = new QComboBox(this);
        _comboRespType->addItems(QStringList() << "disp" << "vel" << "acc");
        peakFilterFold->addRow(new PanelLabel("响应类型"), _comboRespType);
        connect(_comboRespType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (data)
            {
                data->_respType = _comboRespType->currentText();
            }
            });

        // 识别类型
        _comboIdentType = new QComboBox(this);
        _comboIdentType->addItems(QStringList() << "db" << "dba" << "non");
        peakFilterFold->addRow(new PanelLabel("识别类型"), _comboIdentType);
        connect(_comboIdentType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (data)
            {
                data->_identType = _comboIdentType->currentText();
            }
            });

        // 提取个数
        _editExtractCount = new mCalcLineEdit("", this, false, false);
        peakFilterFold->addRow(new PanelLabel("提取个数"), _editExtractCount);
        connect(_editExtractCount, &QLineEdit::editingFinished, this, [this]() {
            if (data)
                data->_extractCount = _editExtractCount->text();
            });

        // 最小间距
        _editMinSpacing = new mCalcLineEdit("", this, false, false);
        peakFilterFold->addRow(new PanelLabel("最小间距"), _editMinSpacing);
        connect(_editMinSpacing, &QLineEdit::editingFinished, this, [this]() {
            if (data)
                data->_minSpacing = _editMinSpacing->text();
            });

        // 最大间距
        _editMaxSpacing = new mCalcLineEdit("", this, false, false);
        peakFilterFold->addRow(new PanelLabel("最大间距"), _editMaxSpacing);
        connect(_editMaxSpacing, &QLineEdit::editingFinished, this, [this]() {
            if (data)
                data->_maxSpacing = _editMaxSpacing->text();
            });

        // 起始频率
        _editStartFreq = new mCalcLineEdit("", this, false, false);
        peakFilterFold->addRow(new PanelLabel("起始频率"), _editStartFreq);
        connect(_editStartFreq, &QLineEdit::editingFinished, this, [this]() {
            if (data)
                data->_startFreq = _editStartFreq->text();
            });

        // 终止频率
        _editEndFreq = new mCalcLineEdit("", this, false, false);
        peakFilterFold->addRow(new PanelLabel("终止频率"), _editEndFreq);
        connect(_editEndFreq, &QLineEdit::editingFinished, this, [this]() {
            if (data)
                data->_endFreq = _editEndFreq->text();
            });

        // 应用到全局
        _checkApplyGlobal = new QCheckBox("应用到全局", this);
        peakFilterFold->addRow(_checkApplyGlobal);
        connect(_checkApplyGlobal, &QCheckBox::clicked, this, [this](bool checked) {
            if (data)
            {
                data->_applyToGlobal = checked;
                if (checked)
                {
                    applyGlobalToAllFreqCases();
                }
                else
                {
                    removeGlobalFromAllFreqCases();
                }
            }
            });

        m_foldWidgets.append(peakFilterFold);
        _mainLayout->addWidget(peakFilterFold);
        _mainLayout->addStretch();

        // 连接表格数据变化信号
        connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig,
            this, &mPeakFilterPanel::slot_tableDataChanged);
    }

    mPeakFilterPanel::~mPeakFilterPanel()
    {
    }

    void mPeakFilterPanel::createPeakFilter()
    {
        if (!getCVM) return;

        auto globalParam = getCVM->getModelData()->getGlobalParam();
        int newId = globalParam->createPeakFilterID();
        QString name = QString("PeakOut_%1").arg(newId);
        data = new mPeakFilterData(newId, name);
        globalParam->addPeakFilterData(data);

        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建峰值筛选"), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdatePeakFilterSig(true, newId);

        initTable();
    }

    void mPeakFilterPanel::editPeakFilter(int peakFilterID)
    {
        if (!getCVM) return;
        data = getCVM->getModelData()->getGlobalParam()->getPeakFilterData(peakFilterID);
        if (data == nullptr) return;

        // 填充面板
        int respIndex = _comboRespType->findText(data->_respType);
        if (respIndex >= 0) _comboRespType->setCurrentIndex(respIndex);

        int identIndex = _comboIdentType->findText(data->_identType);
        if (identIndex >= 0) _comboIdentType->setCurrentIndex(identIndex);

        _editExtractCount->setExpression(data->_extractCount);
        _editMinSpacing->setExpression(data->_minSpacing);
        _editMaxSpacing->setExpression(data->_maxSpacing);
        _editStartFreq->setExpression(data->_startFreq);
        _editEndFreq->setExpression(data->_endFreq);
        _checkApplyGlobal->setChecked(data->_applyToGlobal);

        initTable();
    }

    void mPeakFilterPanel::initTable()
    {
        if (!data) return;

        mTableDataWindow::getInstance()->resetInitialTable();
        auto table = mTableDataWindow::getInstance()->getTableWidget();
        table->setSizeMode(mTableWidget::SizeMode::DynamicSize);
        mTableDataWindow::getInstance()->setUseCurve(false);

        // 将 _tableData 转换为列格式
        QVector<QVector<double>> colData(3);
        for (int r = 0; r < data->_tableData.size(); ++r)
        {
            for (int c = 0; c < 3 && c < data->_tableData[r].size(); ++c)
            {
                colData[c].append(data->_tableData[r][c]);
            }
        }
        mTableDataWindow::getInstance()->setTableData(colData);
        mTableDataWindow::getInstance()->setHorizontalHeaders(QStringList() << "响应点" << "自由度" << "截断值");
        mDockLayoutManager::getInstance()->activateDock("Table");
    }

    void mPeakFilterPanel::slot_tableDataChanged(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (!data) return;

        // 直接通过表格控件读取原始数据，绕过 getTableData 的 X/Y 曲线逻辑
        auto table = mTableDataWindow::getInstance()->getTableWidget();
        int rows = table->rowCount();
        int cols = table->columnCount();

        if (rows > 0 && cols >= 3)
        {
            data->_tableData.resize(rows);
            for (int r = 0; r < rows; ++r)
            {
                data->_tableData[r].resize(3);
                for (int c = 0; c < 3; ++c)
                {
                    QTableWidgetItem* item = table->item(r, c);
                    if (item)
                    {
                        bool ok = false;
                        double val = item->text().toDouble(&ok);
                        data->_tableData[r][c] = ok ? val : 0;
                    }
                    else
                    {
                        data->_tableData[r][c] = 0;
                    }
                }
            }
        }
        else
        {
            data->_tableData.clear();
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_PeakFilter, data->getId());
    }

    void mPeakFilterPanel::applyGlobalToAllFreqCases()
    {
        if (!data || !_checkApplyGlobal->isChecked() || !getCVM) return;

        int peakFilterId = data->getId();
        auto modelData = getCVM->getModelData();
        auto& caseMgr = modelData->getManager<MechCaseData::mCaseManager>(Physical_Case);

        std::vector<int> allCaseIds = caseMgr.getAllCaseID();
        for (int caseId : allCaseIds)
        {
            mCaseBase* caseBase = caseMgr.getCaseData(caseId);
            if (!caseBase) continue;

            mCaseImplicitFrequency* freqCase = dynamic_cast<mCaseImplicitFrequency*>(caseBase);
            if (!freqCase) continue;

            QList<int> existingIds = modelData->getMapRelationManager()
                .getMapRelation<QList<int>>({ Physical_Case, caseId }, Physical_PeakFilter);

            if (!existingIds.contains(peakFilterId))
            {
                int row = existingIds.size();
                modelData->getMapRelationManager().appendTableMapRelation(
                    { Physical_Case, caseId }, { Physical_PeakFilter, peakFilterId }, row);
            }

            // 无论之前是否已有峰值筛选，都发送信号以触发所有工况面板刷新列表
            emit mMechGlobalSignals::getInstance()->createUpdateCasePeakFilterSig(true, caseId);
        }
    }

    void mPeakFilterPanel::removeGlobalFromAllFreqCases()
    {
        if (!data || !getCVM) return;

        int peakFilterId = data->getId();
        auto modelData = getCVM->getModelData();
        auto& caseMgr = modelData->getManager<MechCaseData::mCaseManager>(Physical_Case);

        std::vector<int> allCaseIds = caseMgr.getAllCaseID();
        for (int caseId : allCaseIds)
        {
            mCaseBase* caseBase = caseMgr.getCaseData(caseId);
            if (!caseBase) continue;

            mCaseImplicitFrequency* freqCase = dynamic_cast<mCaseImplicitFrequency*>(caseBase);
            if (!freqCase) continue;

            QList<int> existingIds = modelData->getMapRelationManager()
                .getMapRelation<QList<int>>({ Physical_Case, caseId }, Physical_PeakFilter);

            int row = existingIds.indexOf(peakFilterId);
            if (row >= 0)
            {
                modelData->getMapRelationManager().deleteTabelMapRelation(
                    { Physical_Case, caseId }, { Physical_PeakFilter, peakFilterId }, row);
            }

            emit mMechGlobalSignals::getInstance()->createUpdateCasePeakFilterSig(true, caseId);
        }
    }
}
