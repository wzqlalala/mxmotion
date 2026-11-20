#include "mMotionControlInputPanel.h"
#include "mAppConfiguration.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mMotionControlInputManager.h"
#include "mMechGlobalSignals.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QPushButton>

using namespace MechMotionData;
using namespace MxFunctions;

namespace MechMainWindow
{
    mMotionControlInputPanel::mMotionControlInputPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        m_mainLayout = new QVBoxLayout(this);
        m_mainLayout->setContentsMargins(0, 0, 0, 0);
        m_mainLayout->setSpacing(4);

        // ==================== 基础信息 ====================
        FoldSection* foldBasic = new FoldSection("基础信息", this);
        m_edtName = new QLineEdit();
        m_edtName->setPlaceholderText("控制输入名称");
        foldBasic->addRow(new QLabel("名称:"), m_edtName);

        m_cmbFunctionType = new QComboBox();
        m_cmbFunctionType->addItems({ "常量", "线性", "二次多项式", "三次多项式", "自定义曲线" });
        foldBasic->addRow(new QLabel("函数类型:"), m_cmbFunctionType);
        m_mainLayout->addWidget(foldBasic);

        // ==================== 参数堆叠区 ====================
        m_stkParams = new QStackedWidget(this);

        // 0: 常量
        QWidget* pageConst = new QWidget();
        QFormLayout* layConst = new QFormLayout(pageConst);
        layConst->setContentsMargins(12, 10, 12, 10);
        m_spbConstValue = new QDoubleSpinBox();
        m_spbConstValue->setRange(-1e9, 1e9);
        m_spbConstValue->setDecimals(4);
        m_spbConstValue->setButtonSymbols(QAbstractSpinBox::NoButtons);
        layConst->addRow(new QLabel("常量值:"), m_spbConstValue);
        m_stkParams->addWidget(pageConst);

        // 1: 线性 a + b*t
        QWidget* pageLin = new QWidget();
        QFormLayout* layLin = new QFormLayout(pageLin);
        layLin->setContentsMargins(12, 10, 12, 10);
        m_spbLinA = new QDoubleSpinBox(); m_spbLinA->setRange(-1e9, 1e9); m_spbLinA->setDecimals(4); m_spbLinA->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbLinB = new QDoubleSpinBox(); m_spbLinB->setRange(-1e9, 1e9); m_spbLinB->setDecimals(4); m_spbLinB->setButtonSymbols(QAbstractSpinBox::NoButtons);
        layLin->addRow(new QLabel("常数项 a:"), m_spbLinA);
        layLin->addRow(new QLabel("一次项 b:"), m_spbLinB);
        m_stkParams->addWidget(pageLin);

        // 2: 二次
        QWidget* pageQuad = new QWidget();
        QFormLayout* layQuad = new QFormLayout(pageQuad);
        layQuad->setContentsMargins(12, 10, 12, 10);
        m_spbQuadA = new QDoubleSpinBox(); m_spbQuadA->setRange(-1e9, 1e9); m_spbQuadA->setDecimals(4); m_spbQuadA->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbQuadB = new QDoubleSpinBox(); m_spbQuadB->setRange(-1e9, 1e9); m_spbQuadB->setDecimals(4); m_spbQuadB->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbQuadC = new QDoubleSpinBox(); m_spbQuadC->setRange(-1e9, 1e9); m_spbQuadC->setDecimals(4); m_spbQuadC->setButtonSymbols(QAbstractSpinBox::NoButtons);
        layQuad->addRow(new QLabel("常数项 a:"), m_spbQuadA);
        layQuad->addRow(new QLabel("一次项 b:"), m_spbQuadB);
        layQuad->addRow(new QLabel("二次项 c:"), m_spbQuadC);
        m_stkParams->addWidget(pageQuad);

        // 3: 三次
        QWidget* pageCubic = new QWidget();
        QFormLayout* layCubic = new QFormLayout(pageCubic);
        layCubic->setContentsMargins(12, 10, 12, 10);
        m_spbCubicA = new QDoubleSpinBox(); m_spbCubicA->setRange(-1e9, 1e9); m_spbCubicA->setDecimals(4); m_spbCubicA->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbCubicB = new QDoubleSpinBox(); m_spbCubicB->setRange(-1e9, 1e9); m_spbCubicB->setDecimals(4); m_spbCubicB->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbCubicC = new QDoubleSpinBox(); m_spbCubicC->setRange(-1e9, 1e9); m_spbCubicC->setDecimals(4); m_spbCubicC->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbCubicD = new QDoubleSpinBox(); m_spbCubicD->setRange(-1e9, 1e9); m_spbCubicD->setDecimals(4); m_spbCubicD->setButtonSymbols(QAbstractSpinBox::NoButtons);
        layCubic->addRow(new QLabel("常数项 a:"), m_spbCubicA);
        layCubic->addRow(new QLabel("一次项 b:"), m_spbCubicB);
        layCubic->addRow(new QLabel("二次项 c:"), m_spbCubicC);
        layCubic->addRow(new QLabel("三次项 d:"), m_spbCubicD);
        m_stkParams->addWidget(pageCubic);

        // 4: 自定义曲线
        QWidget* pageCurve = new QWidget();
        QVBoxLayout* layCurve = new QVBoxLayout(pageCurve);
        layCurve->setContentsMargins(12, 10, 12, 10);
        m_tableCurve = new QTableWidget(0, 2);
        m_tableCurve->setHorizontalHeaderLabels(QStringList() << "时间 t" << "数值 f(t)");
        m_tableCurve->horizontalHeader()->setStretchLastSection(true);
        m_tableCurve->setMinimumHeight(120);
        layCurve->addWidget(m_tableCurve);
        QPushButton* btnAddRow = new QPushButton("添加行");
        QPushButton* btnDelRow = new QPushButton("删除行");
        QHBoxLayout* layBtns = new QHBoxLayout();
        layBtns->addWidget(btnAddRow);
        layBtns->addWidget(btnDelRow);
        layCurve->addLayout(layBtns);
        m_stkParams->addWidget(pageCurve);

        FoldSection* foldParams = new FoldSection("参数设置", this);
        foldParams->addRow(m_stkParams);
        m_mainLayout->addWidget(foldParams);

        // ==================== 时间范围 ====================
        FoldSection* foldTime = new FoldSection("时间范围", this);
        m_spbTimeStart = new QDoubleSpinBox(); m_spbTimeStart->setRange(0, 1e9); m_spbTimeStart->setDecimals(4); m_spbTimeStart->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbTimeEnd   = new QDoubleSpinBox(); m_spbTimeEnd->setRange(0, 1e9);   m_spbTimeEnd->setDecimals(4);   m_spbTimeEnd->setButtonSymbols(QAbstractSpinBox::NoButtons);
        foldTime->addRow(new QLabel("开始时间:"), m_spbTimeStart);
        foldTime->addRow(new QLabel("结束时间:"), m_spbTimeEnd);
        m_mainLayout->addWidget(foldTime);

        m_mainLayout->addStretch();

        // ==================== 信号绑定 ====================
        connect(m_cmbFunctionType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
            m_stkParams->setCurrentIndex(idx);
            if (m_data) {
                m_data->setInputType(static_cast<MechMotionData::ControlInputType>(idx));
                writeParamsToData();
            }
        });
        connect(m_edtName, &QLineEdit::textChanged, this, [this](const QString& text) {
            if (!m_data) return;
            m_data->setName(text);
            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionControlInputSig(false, m_data->getId());
        });
        // 数值参数：任何改动即时写回
        auto writeBack = [this](double) { writeParamsToData(); };
        connect(m_spbConstValue, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, writeBack);
        connect(m_spbLinA, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, writeBack);
        connect(m_spbLinB, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, writeBack);
        connect(m_spbQuadA, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, writeBack);
        connect(m_spbQuadB, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, writeBack);
        connect(m_spbQuadC, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, writeBack);
        connect(m_spbCubicA, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, writeBack);
        connect(m_spbCubicB, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, writeBack);
        connect(m_spbCubicC, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, writeBack);
        connect(m_spbCubicD, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, writeBack);
        connect(m_spbTimeStart, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, writeBack);
        connect(m_spbTimeEnd, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, writeBack);
        connect(m_tableCurve, &QTableWidget::itemChanged, this, [this](QTableWidgetItem*) {
            if (!m_blockTableSignal) writeParamsToData();
        });
        connect(btnAddRow, &QPushButton::clicked, this, [this]() {
            int row = m_tableCurve->rowCount();
            m_blockTableSignal = true;
            m_tableCurve->insertRow(row);
            m_tableCurve->setItem(row, 0, new QTableWidgetItem("0.0"));
            m_tableCurve->setItem(row, 1, new QTableWidgetItem("0.0"));
            m_blockTableSignal = false;
            writeParamsToData();
        });
        connect(btnDelRow, &QPushButton::clicked, this, [this]() {
            int row = m_tableCurve->currentRow();
            if (row >= 0) {
                m_tableCurve->removeRow(row);
                writeParamsToData();
            }
        });
    }

    mMotionControlInputPanel::~mMotionControlInputPanel() {}

    void mMotionControlInputPanel::newControlInputPanel()
    {
        setEditing(true);
        connectSig();

        int newId = getCVM->getModelData()->getManager<mMotionControlInputManager>(Physical_MotionControlInput).createID();
        m_data = new mMotionControlInput(newId);
        m_data->setName(getCVM->getModelData()->createNameByType(Physical_MotionControlInput, "控制输入"));
        getCVM->getModelData()->addPhysicalField<mMotionControlInput>(m_data);

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionControlInputSig(true, m_data->getId());
    }

    void mMotionControlInputPanel::editControlInputPanel(int id)
    {
        setEditing(true);
        m_data = getCVM->getModelData()->getManager<mMotionControlInputManager>(Physical_MotionControlInput).getData(id);
        if (!m_data)
            return;

        connectSig();

        restoreUIFromData();
    }

    //数据对象即将被删除（树删除前调用）：置空编辑指针防悬垂（周期边界同款约定）
    void mMotionControlInputPanel::onMotionControlInputDataAboutToBeDestroyed(int id)
    {
        if (id >= 0)
        {
            if (m_data == nullptr || id != m_data->getId())
                return;
        }
        m_data = nullptr;
    }

    void mMotionControlInputPanel::uneditControlInputPanel()
    {
        if (!isEditing())
            return;
        setEditing(false);
        m_data = nullptr;
        disconnectSig();
    }
    void mMotionControlInputPanel::connectSig() {}
    void mMotionControlInputPanel::disconnectSig() {}
    void mMotionControlInputPanel::slot_functionTypeChanged(int index) {}

    void mMotionControlInputPanel::writeParamsToData()
    {
        if (!m_data)
            return;

        switch (m_cmbFunctionType->currentIndex()) {
        case 0:
            m_data->setPolyCoeffs({ m_spbConstValue->value() });
            break;
        case 1:
            m_data->setPolyCoeffs({ m_spbLinA->value(), m_spbLinB->value() });
            break;
        case 2:
            m_data->setPolyCoeffs({ m_spbQuadA->value(), m_spbQuadB->value(), m_spbQuadC->value() });
            break;
        case 3:
            m_data->setPolyCoeffs({ m_spbCubicA->value(), m_spbCubicB->value(), m_spbCubicC->value(), m_spbCubicD->value() });
            break;
        case 4: {
            QVector<QPair<double, double>> pts;
            for (int r = 0; r < m_tableCurve->rowCount(); ++r) {
                auto* it = m_tableCurve->item(r, 0);
                auto* iv = m_tableCurve->item(r, 1);
                if (!it || !iv) continue;
                bool okT = false, okV = false;
                const double t = it->text().toDouble(&okT);
                const double v = iv->text().toDouble(&okV);
                if (okT && okV)
                    pts.append({ t, v });
            }
            m_data->setCurvePoints(pts);
            break;
        }
        }
        m_data->setTimeStart(m_spbTimeStart->value());
        m_data->setTimeEnd(m_spbTimeEnd->value());
    }

    void mMotionControlInputPanel::restoreUIFromData()
    {
        if (!m_data)
            return;

        // 还原期间全部信号静默，避免中间态写回
        const QSignalBlocker bName(m_edtName);
        const QSignalBlocker bType(m_cmbFunctionType);
        const QSignalBlocker bConst(m_spbConstValue);
        const QSignalBlocker bLinA(m_spbLinA);
        const QSignalBlocker bLinB(m_spbLinB);
        const QSignalBlocker bQuadA(m_spbQuadA);
        const QSignalBlocker bQuadB(m_spbQuadB);
        const QSignalBlocker bQuadC(m_spbQuadC);
        const QSignalBlocker bCubicA(m_spbCubicA);
        const QSignalBlocker bCubicB(m_spbCubicB);
        const QSignalBlocker bCubicC(m_spbCubicC);
        const QSignalBlocker bCubicD(m_spbCubicD);
        const QSignalBlocker bTs(m_spbTimeStart);
        const QSignalBlocker bTe(m_spbTimeEnd);
        m_blockTableSignal = true;
        const QSignalBlocker bTable(m_tableCurve);

        int typeIdx = static_cast<int>(m_data->getInputType());
        if (typeIdx < 0 || typeIdx > 4) typeIdx = 0;
        m_cmbFunctionType->setCurrentIndex(typeIdx);
        m_stkParams->setCurrentIndex(typeIdx);

        const auto& coeffs = m_data->getPolyCoeffs();
        auto coeffAt = [&coeffs](int i) -> double {
            return (i >= 0 && i < coeffs.size()) ? coeffs[i] : 0.0;
        };
        m_spbConstValue->setValue(coeffAt(0));
        m_spbLinA->setValue(coeffAt(0));
        m_spbLinB->setValue(coeffAt(1));
        m_spbQuadA->setValue(coeffAt(0));
        m_spbQuadB->setValue(coeffAt(1));
        m_spbQuadC->setValue(coeffAt(2));
        m_spbCubicA->setValue(coeffAt(0));
        m_spbCubicB->setValue(coeffAt(1));
        m_spbCubicC->setValue(coeffAt(2));
        m_spbCubicD->setValue(coeffAt(3));

        m_spbTimeStart->setValue(m_data->getTimeStart());
        m_spbTimeEnd->setValue(m_data->getTimeEnd());

        m_tableCurve->setRowCount(0);
        const auto& pts = m_data->getCurvePoints();
        for (const auto& p : pts) {
            int row = m_tableCurve->rowCount();
            m_tableCurve->insertRow(row);
            m_tableCurve->setItem(row, 0, new QTableWidgetItem(QString::number(p.first)));
            m_tableCurve->setItem(row, 1, new QTableWidgetItem(QString::number(p.second)));
        }
        m_blockTableSignal = false;

        m_edtName->setText(m_data->getName());
    }
}
