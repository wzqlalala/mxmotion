#include "mfunctionpanel.h"
#include <QtWidgets>
#include <muParser.h>
#include <cmath>

#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
//MxFunctions
#include "mGlobalSignals.h"
#include "mContainerOperation.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
//label
#include "mxr/label/mSymbolController.h"

//BasicData
#include "mPhysicalFieldEnum.h"
#include "mCoordinateManager.h"
#include "mModelData.h"
#include "mMeshData.h"

//Geo
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（冻结岛保档，同主线处理）
#include <mMechGlobalSignals.h>
//DockWidget
#include "mFunctionBase.h"
#include "mFunctionManager.h"
//
#include "mDockLayoutManager.h"
#include "DockWidget.h"
#include "mPlotStructs.h"
//表格数据
#include "mTableDataWindow.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mPanelBase.h"
#include "mCurveWindow.h"
#include "../PreData/LowCPlusDefine.h"
#include "mOtherFunction.h"

using namespace MechFunctionData;
// 三次样条插值类（自然边界条件）
class CubicSpline
{
public:
    CubicSpline(const QVector<double>& x, const QVector<double>& y)
    {
        int n = x.size();
        a = y;
        b.resize(n); c.resize(n); d.resize(n);
        QVector<double> h(n - 1);
        for (int i = 0; i < n - 1; ++i) h[i] = x[i + 1] - x[i];

        QVector<double> alpha(n - 1);
        for (int i = 1; i < n - 1; ++i)
            alpha[i] = 3.0 * ((a[i + 1] - a[i]) / h[i] - (a[i] - a[i - 1]) / h[i - 1]);

        QVector<double> l(n), mu(n), z(n);
        l[0] = 1; mu[0] = z[0] = 0;
        for (int i = 1; i < n - 1; ++i) {
            l[i] = 2 * (x[i + 1] - x[i - 1]) - h[i - 1] * mu[i - 1];
            mu[i] = h[i] / l[i];
            z[i] = (alpha[i] - h[i - 1] * z[i - 1]) / l[i];
        }
        l[n - 1] = 1; z[n - 1] = c[n - 1] = 0;

        for (int j = n - 2; j >= 0; --j) {
            c[j] = z[j] - mu[j] * c[j + 1];
            b[j] = (a[j + 1] - a[j]) / h[j] - h[j] * (c[j + 1] + 2 * c[j]) / 3.0;
            d[j] = (c[j + 1] - c[j]) / (3.0 * h[j]);
        }
        xs = x;
    }

    double eval(double x) const
    {
        int n = xs.size();
        if (x <= xs.front()) return a.front();
        if (x >= xs.back()) return a.back();

        int i = 0;
        for (; i < n - 1; ++i)
            if (x <= xs[i + 1]) break;

        double dx = x - xs[i];
        return a[i] + b[i] * dx + c[i] * dx * dx + d[i] * dx * dx * dx;
    }

private:
    QVector<double> a, b, c, d, xs;
};

namespace {
    // 如果单元格为空就新建一个 "0"
    inline QTableWidgetItem* ensureItem(QTableWidget* tw, int r, int c)
    {
        if (r < 0 || c < 0 || r >= tw->rowCount() || c >= tw->columnCount())
            return nullptr;                       // 越界直接返回空
        if (!tw->item(r, c)) {
            const QSignalBlocker b(tw);
            tw->setItem(r, c, new QTableWidgetItem("0"));
        }
        else if (tw->item(r, 0) != nullptr && tw->item(r, 0)->text() == "")
        {
            const QSignalBlocker b(tw);
            tw->item(r, 0)->setText(MxFunctions::mNumberToString(r+1));
            if (tw->item(r, 1)) tw->item(r, 1)->setText("0");
            if (tw->item(r, 2)) tw->item(r, 2)->setText("0");
        }
        else if (tw->item(r, 1) != nullptr && tw->item(r, 1)->text() == "")
        {
            const QSignalBlocker b(tw);
            tw->item(r, 1)->setText("0");
        }
        else if (tw->item(r, 2)!=nullptr&&tw->item(r, 2)->text() == "")
        {
            const QSignalBlocker b(tw);
            tw->item(r, 2)->setText("0");
        }
        else if (tw->item(r, 3) != nullptr && tw->item(r, 3)->text() == "")
        {
            const QSignalBlocker b(tw);
            tw->item(r, 3)->setText("0");
        }
        else if (tw->item(r, 4) != nullptr && tw->item(r, 4)->text() == "")
        {
            const QSignalBlocker b(tw);
            tw->item(r, 4)->setText("10");
        }
        return tw->item(r, c);
    }
} // namespace
namespace MechMainWindow {

    QTimer* mFunctionPanel::g_debounce = nullptr;

    /* ========== 构造/析构 ========== */
    mFunctionPanel::mFunctionPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        setBackgroundRole(QPalette::Window);
        setAutoFillBackground(true);
        this->setStyleSheet(
            "mFunctionPanel { background-color: white; }"

            /* 关键：把 QComboBox 的下拉列表也刷成白底黑字 */
            "QComboBox QAbstractItemView {"
            "    background-color: white;"     /* 白底   */
            "    color: black;"                /* 黑字   */
            "    selection-background-color: #3399ff;" /* 悬停高亮，可自定 */
            "    selection-color: black;"      /* 悬停时文字保持黑色 */
            "}"
            "QComboBox  ,"        
            "QLineEdit  ,"
            "QSpinBox  {"
            "    font-size: 12px;"      /* 悬停时文字保持黑色 */
            "}"
            "QPushButton::hover{background-color: #299DFF;}"

        );

        buildUI();        // 构造与旧 ui 完全一致
        initData();
        connectSignals(); // 信号槽
        onTypeChanged(2); // 默认解析页
    }

    mFunctionPanel::~mFunctionPanel() = default;

    /* ========== 外部旧入口 ========== */
    void mFunctionPanel::newFunctionPanel(int type)
    {
        if (!getCVM)
        {
            return;
        }
        if (type == 0) type = 2; //函数屏蔽了一些，默认应为2

        //新建
        int id = getCVM->getModelData()->getManager<mFunctionManager>(Physical_Function).createID();
        _funcData = new mFunctionBase(id, getCVM->getModelData()->createNameByType(Physical_Function,"Function"));
        getCVM->getModelData()->addPhysicalField<mFunctionBase>(_funcData);
  
        //创建成功发送消息
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建函数 %1").arg(_funcData->getFunctionName()), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateFunctionSig(true, _funcData->getFunctionId());

        onTypeChanged(2);
        getFunctionTypeBox()->setCurrentIndex(type);
        //editFunctionPanel(_coorData->getCoordinateId());
    }

    QComboBox* mFunctionPanel::getFunctionTypeBox()
    {
        return comboBox;
    }
    void mFunctionPanel::editFunctionPanel(int funcId)
    {
        comboBox->blockSignals(true);
        lineEdit->blockSignals(true);
        lineEdit_2->blockSignals(true);
        spinSamplesAna->blockSignals(true);
        lineEdit_3->blockSignals(true);
        tableWidget->blockSignals(true);
        tableWidget_interp->blockSignals(true);
        comboBox_5->blockSignals(true);
        comboBox_4->blockSignals(true);
        tableWidget_3->blockSignals(true);
        lineEdit_4->blockSignals(true);
        lineEdit_9->blockSignals(true);
        lineEdit_10->blockSignals(true);
        tableWidget_table->blockSignals(true);
        comboBox_xAxisType->blockSignals(true);
        comboBox_yAxisType->blockSignals(true);
        comboBox_extrapTable->blockSignals(true);
        m_lineEdit_x1->blockSignals(true);
        m_lineEdit_x2->blockSignals(true);
        m_lineEdit_x3->blockSignals(true);
        m_lineEdit_x4->blockSignals(true);

        _funcData= getCVM->getModelData()->getManager<mFunctionManager>(Physical_Function).getFunctionData(funcId);
        int type = _funcData->getFunctionType();
        comboBox->setCurrentIndex(type);
        //切换函数类型下拉框，界面跟着切换
        {
            m_exprFold->setVisible(false);
            m_discFold->setVisible(false);
            m_interpFold->setVisible(false);
            m_extrapFold->setVisible(false);
            m_interpDataFold->setVisible(false);
            m_pieceFold->setVisible(false);
            m_stepFold->setVisible(false);
            m_tableFold->setVisible(false);
            m_tableDataFold->setVisible(false);
            m_periodicParamFold->setVisible(false);
            m_periodicDataFold->setVisible(false);
            m_modulatedFold->setVisible(false);
            m_expDecayFold->setVisible(false);

            m_labExtrapTable->setParent(nullptr);
            comboBox_extrapTable->setParent(nullptr);

            /* 2. 变量折叠体仅解析、分段需要 */
            m_varFold->setVisible(type == 0 || type == 3);

            /* 3. 变量范围折叠体也仅解析、分段需要 */
            m_rangeFold->setVisible(type == 0 || type == 4);

            switch (type) {
            case 0: m_exprFold->setVisible(true);
                m_interpDataFold->setVisible(false);
                m_statFold->setVisible(false);
                break;
            case 1: m_discFold->setVisible(true);
                m_interpDataFold->setVisible(false);
                m_statFold->setVisible(false);
                break;
            case 2:
                m_extrapFold->setVisible(true);    // 插值方式（含 光滑/阶跃）
                m_interpDataFold->setVisible(true);  // 插值数据
                m_statFold->setVisible(false);
                break;
            case 3: m_pieceFold->setVisible(true);
                m_interpDataFold->setVisible(false);
                m_statFold->setVisible(false);
                break;
            case 4: m_stepFold->setVisible(true);
                m_rangeFold->setVisible(false);   // 加这一行
                m_interpDataFold->setVisible(false);
                m_statFold->setVisible(false);
                break;
            case 5:
                m_tableFold->setVisible(true);
                m_tableDataFold->setVisible(true);
                m_statFold->setVisible(false);
                // 普通表格：显示 x轴/y轴，隐藏 x1
                if (m_labXAxisType->parent() == nullptr) m_tableFold->addRow(m_labXAxisType, comboBox_xAxisType);
                if (m_labYAxisType->parent() == nullptr) m_tableFold->addRow(m_labYAxisType, comboBox_yAxisType);
                if (m_labExtrapTable->parent() == nullptr) m_tableFold->addRow(m_labExtrapTable, comboBox_extrapTable);
                m_labX1->setParent(nullptr);
                m_lineEdit_x1->setParent(nullptr);
                m_labX2->setParent(nullptr);
                m_lineEdit_x2->setParent(nullptr);
                m_labX3->setParent(nullptr);
                m_lineEdit_x3->setParent(nullptr);
                m_labX4->setParent(nullptr);
                m_lineEdit_x4->setParent(nullptr);
                // 恢复3列表格
                tableWidget_table->setColumnCount(3);
                tableWidget_table->setHorizontalHeaderLabels({ "序号", "x", "y" });
                break;
            case 6:
                m_tableFold->setVisible(true);
                m_tableDataFold->setVisible(true);
                m_statFold->setVisible(false);
                // 可平移表格：隐藏 x轴/y轴/x2，显示 x1
                m_labXAxisType->setParent(nullptr);
                comboBox_xAxisType->setParent(nullptr);
                m_labYAxisType->setParent(nullptr);
                comboBox_yAxisType->setParent(nullptr);
                m_labX2->setParent(nullptr);
                m_lineEdit_x2->setParent(nullptr);
                if (m_labX1->parent() == nullptr) m_tableFold->addRow(m_labX1, m_lineEdit_x1);
                if (m_labExtrapTable->parent() == nullptr) m_tableFold->addRow(m_labExtrapTable, comboBox_extrapTable);
                // 恢复3列表格
                tableWidget_table->setColumnCount(3);
                tableWidget_table->setHorizontalHeaderLabels({ "序号", "x", "y" });
                break;
            case 7:
                m_tableFold->setVisible(true);
                m_tableDataFold->setVisible(true);
                m_statFold->setVisible(false);
                // 可缩放表格：隐藏 x轴/y轴，显示 x1/x2
                m_labXAxisType->setParent(nullptr);
                comboBox_xAxisType->setParent(nullptr);
                m_labYAxisType->setParent(nullptr);
                comboBox_yAxisType->setParent(nullptr);
                if (m_labX1->parent() == nullptr) m_tableFold->addRow(m_labX1, m_lineEdit_x1);
                if (m_labX2->parent() == nullptr) m_tableFold->addRow(m_labX2, m_lineEdit_x2);
                if (m_labExtrapTable->parent() == nullptr) m_tableFold->addRow(m_labExtrapTable, comboBox_extrapTable);
                m_labX3->setParent(nullptr);
                m_lineEdit_x3->setParent(nullptr);
                m_labX4->setParent(nullptr);
                m_lineEdit_x4->setParent(nullptr);
                // 恢复3列表格
                tableWidget_table->setColumnCount(3);
                tableWidget_table->setHorizontalHeaderLabels({ "序号", "x", "y" });
                break;
            case 8:
                m_tableFold->setVisible(true);
                m_tableDataFold->setVisible(true);
                m_statFold->setVisible(false);
                // 幂函数：隐藏 x轴/y轴/外推方式，显示 x1/x2/x3/x4
                m_labXAxisType->setParent(nullptr);
                comboBox_xAxisType->setParent(nullptr);
                m_labYAxisType->setParent(nullptr);
                comboBox_yAxisType->setParent(nullptr);
                if (m_labX1->parent() == nullptr) m_tableFold->addRow(m_labX1, m_lineEdit_x1);
                if (m_labX2->parent() == nullptr) m_tableFold->addRow(m_labX2, m_lineEdit_x2);
                if (m_labX3->parent() == nullptr) m_tableFold->addRow(m_labX3, m_lineEdit_x3);
                if (m_labX4->parent() == nullptr) m_tableFold->addRow(m_labX4, m_lineEdit_x4);
                // 幂函数表格改为2列
                tableWidget_table->setColumnCount(2);
                tableWidget_table->setHorizontalHeaderLabels({ "序号", "A" });
                break;
            case 9:
                m_periodicParamFold->setVisible(true);
                m_periodicDataFold->setVisible(true);
                m_statFold->setVisible(false);
                break;
            case 10:
                m_modulatedFold->setVisible(true);
                m_statFold->setVisible(false);
                break;
            case 11:
                m_expDecayFold->setVisible(true);
                m_statFold->setVisible(false);
                break;
            }
            if (m_tableFold) {
                QTimer::singleShot(0, m_tableFold, [this]() { m_tableFold->refreshLayout(); });
            }
        }
        updatePanel();
        
        onAutoRefresh();
        comboBox->blockSignals(false);
        lineEdit->blockSignals(false);
        lineEdit_2->blockSignals(false);
        spinSamplesAna->blockSignals(false);
        lineEdit_3->blockSignals(false);
        tableWidget->blockSignals(false);
        tableWidget_interp->blockSignals(false);
        comboBox_5->blockSignals(false);
        comboBox_4->blockSignals(false);
        tableWidget_3->blockSignals(false);
        lineEdit_4->blockSignals(false);
        lineEdit_9->blockSignals(false);
        lineEdit_10->blockSignals(false);
        tableWidget_table->blockSignals(false);
        comboBox_xAxisType->blockSignals(false);
        comboBox_yAxisType->blockSignals(false);
        comboBox_extrapTable->blockSignals(false);
        m_lineEdit_x1->blockSignals(false);
        m_lineEdit_x2->blockSignals(false);
        m_lineEdit_x3->blockSignals(false);
        m_lineEdit_x4->blockSignals(false);
        m_lineEditPeriodicFreq->blockSignals(false);
        m_lineEditPeriodicT0->blockSignals(false);
        m_lineEditPeriodicA0->blockSignals(false);
        tableWidget_periodic->blockSignals(false);
        m_lineEditModulatedA0->blockSignals(false);
        m_lineEditModulatedA->blockSignals(false);
        m_lineEditModulatedT0->blockSignals(false);
        m_lineEditModulatedF1->blockSignals(false);
        m_lineEditModulatedF2->blockSignals(false);
        m_lineEditExpDecayA0->blockSignals(false);
        m_lineEditExpDecayAm->blockSignals(false);
        m_lineEditExpDecayT0->blockSignals(false);
        m_lineEditExpDecayTd->blockSignals(false);

    }
    void mFunctionPanel::updatePanel()
    {
        comboBox->blockSignals(true);
        lineEdit->blockSignals(true);
        lineEdit_2->blockSignals(true);
        spinSamplesAna->blockSignals(true);
        lineEdit_3->blockSignals(true);
        tableWidget->blockSignals(true);
        tableWidget_interp->blockSignals(true);
        comboBox_5->blockSignals(true);
        comboBox_4->blockSignals(true);
        tableWidget_3->blockSignals(true);
        lineEdit_4->blockSignals(true);
        lineEdit_9->blockSignals(true);
        lineEdit_10->blockSignals(true);
        tableWidget_table->blockSignals(true);
        comboBox_xAxisType->blockSignals(true);
        comboBox_yAxisType->blockSignals(true);
        comboBox_extrapTable->blockSignals(true);
        m_lineEdit_x1->blockSignals(true);
        m_lineEdit_x2->blockSignals(true);
        m_lineEdit_x3->blockSignals(true);
        m_lineEdit_x4->blockSignals(true);
        m_lineEditPeriodicFreq->blockSignals(true);
        m_lineEditPeriodicT0->blockSignals(true);
        m_lineEditPeriodicA0->blockSignals(true);
        tableWidget_periodic->blockSignals(true);
        m_lineEditModulatedA0->blockSignals(true);
        m_lineEditModulatedA->blockSignals(true);
        m_lineEditModulatedT0->blockSignals(true);
        m_lineEditModulatedF1->blockSignals(true);
        m_lineEditModulatedF2->blockSignals(true);
        m_lineEditExpDecayA0->blockSignals(true);
        m_lineEditExpDecayAm->blockSignals(true);
        m_lineEditExpDecayT0->blockSignals(true);
        m_lineEditExpDecayTd->blockSignals(true);

        int type = _funcData->getFunctionType();
        switch (type) {
        case 0:
        {
            //解析
            AnalyticCol ana = _funcData->analytic();
            lineEdit->setText(MxFunctions::mNumberToString(ana.xMin));
            lineEdit_2->setText(MxFunctions::mNumberToString(ana.xMax));
            spinSamplesAna->setValue(ana.simple);
            lineEdit_3->setText(ana.expr);
            break;
        }
        case 1:
        {
            DiscreteCol dis = _funcData->discrete();
            //离散
            int size = dis.idx.size();

            tableWidget->clearContents();
            tableWidget->setRowCount(size);
            for (int i = 0; i < size; i++)
            {
                QTableWidgetItem* item = new
                    QTableWidgetItem(MxFunctions::mNumberToString(dis.idx[i]));
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                QTableWidgetItem* item1 = new
                    QTableWidgetItem(MxFunctions::mNumberToString(dis.x[i]));
                QTableWidgetItem* item2 = new
                    QTableWidgetItem(MxFunctions::mNumberToString(dis.y[i]));
                tableWidget->setItem(i, 0, item);
                tableWidget->setItem(i, 1, item1);
                tableWidget->setItem(i, 2, item2);
            }

            break;
        }
        case 2:
        {
            //插值
            InterpCol inte = _funcData->interp();
            int size = (int)inte.x.size();
            tableWidget_interp->clearContents();
            tableWidget_interp->setRowCount(size);
            for (int i = 0; i < size; i++)
            {
                QTableWidgetItem* item = new
                    QTableWidgetItem(MxFunctions::mNumberToString(i+1));
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                QTableWidgetItem* item1 = new
                    QTableWidgetItem(MxFunctions::mNumberToString(inte.x[i]));
                QTableWidgetItem* item2 = new
                    QTableWidgetItem(MxFunctions::mNumberToString(inte.y[i]));
                tableWidget_interp->setItem(i, 0, item);
                tableWidget_interp->setItem(i, 1, item1);
                tableWidget_interp->setItem(i, 2, item2);
            }
            // 0-临近点插值 1-线性 2-分段三次 3-三次样条 4-对数插值 5-光滑 6-阶跃
            comboBox_5->setCurrentIndex(qBound(0, inte.interpMode, 6));
            comboBox_4->setCurrentIndex(qBound(0, inte.extrapMode, 1));
            // 光滑/阶跃：表头 序号/时间/幅值 且无外推方式；其余：序号/x/y 且有外推方式
            {
                const bool isSmoothStep = (inte.interpMode == 5 || inte.interpMode == 6);
                tableWidget_interp->setHorizontalHeaderLabels(
                    isSmoothStep ? QStringList{ "序号", "时间", "幅值" }
                                 : QStringList{ "序号", "x", "y" });
                if (isSmoothStep)
                {
                    label_13->setParent(nullptr);
                    comboBox_4->setParent(nullptr);
                }
                else if (comboBox_4->parent() == nullptr)
                {
                    m_extrapFold->addRow(label_13, comboBox_4);
                }
            }
            break;
        }
        case 3:
        {
            //分段
            PiecewiseCol pie = _funcData->piecewise();
            int size = pie.idx.size();
            tableWidget_3->clearContents();
            tableWidget_3->setRowCount(size);
            for (int i = 0; i < size; i++)
            {
                QTableWidgetItem* item = new
                    QTableWidgetItem(MxFunctions::mNumberToString(pie.idx[i]));
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                QTableWidgetItem* item1 = new
                    QTableWidgetItem(MxFunctions::mNumberToString(pie.start[i]));
                QTableWidgetItem* item2 = new
                    QTableWidgetItem(MxFunctions::mNumberToString(pie.end[i]));
                QTableWidgetItem* item3 = new
                    QTableWidgetItem(pie.expr[i]);
                QTableWidgetItem* item4 = new
                    QTableWidgetItem(MxFunctions::mNumberToString(pie.sampleCount[i]));
                tableWidget_3->setItem(i, 0, item);
                tableWidget_3->setItem(i, 1, item1);
                tableWidget_3->setItem(i, 2, item2);
                tableWidget_3->setItem(i, 3, item3);
                tableWidget_3->setItem(i, 4, item4);
            }
            break;
        }
        case 4:
        {
            //阶跃
            StepCol ste = _funcData->step();
            lineEdit_4->setText(MxFunctions::mNumberToString(ste.pos));
            lineEdit_9->setText(MxFunctions::mNumberToString(ste.v0));
            lineEdit_10->setText(MxFunctions::mNumberToString(ste.v1));
            break;
        }
        case 5:
        case 6:
        case 7:
        {
            //表格 / 可平移表格
            TableCol tbl = _funcData->table();
            int size = (int)tbl.x.size();
            tableWidget_table->setColumnCount(3);
            tableWidget_table->setHorizontalHeaderLabels({ "序号", "x", "y" });
            tableWidget_table->clearContents();
            tableWidget_table->setRowCount(size);
            if (tbl.y.size() != size)//防止越界
            {
                tbl.y.resize(size);
            }
            for (int i = 0; i < size; i++)
            {
                QTableWidgetItem* item = new
                    QTableWidgetItem(MxFunctions::mNumberToString(i + 1));
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                QTableWidgetItem* item1 = new
                    QTableWidgetItem(MxFunctions::mNumberToString(tbl.x[i]));
                QTableWidgetItem* item2 = new QTableWidgetItem(MxFunctions::mNumberToString(tbl.y[i]));
                tableWidget_table->setItem(i, 0, item);
                tableWidget_table->setItem(i, 1, item1);
                tableWidget_table->setItem(i, 2, item2);
            }
            comboBox_xAxisType->setCurrentIndex(tbl.xAxisType);
            comboBox_yAxisType->setCurrentIndex(tbl.yAxisType);
            comboBox_extrapTable->setCurrentIndex(tbl.extrapMode);
            m_lineEdit_x1->setText(MxFunctions::mNumberToString(tbl.x1));
            m_lineEdit_x2->setText(MxFunctions::mNumberToString(tbl.x2));
            break;
        }
        case 8:
        {
            //幂函数
            TableCol tbl = _funcData->table();
            int size = (int)tbl.x.size();
            tableWidget_table->setColumnCount(2);
            tableWidget_table->setHorizontalHeaderLabels({ "序号", "A" });
            tableWidget_table->clearContents();
            tableWidget_table->setRowCount(size);
            for (int i = 0; i < size; i++)
            {
                QTableWidgetItem* item = new
                    QTableWidgetItem(MxFunctions::mNumberToString(i + 1));
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                QTableWidgetItem* item1 = new
                    QTableWidgetItem(MxFunctions::mNumberToString(tbl.x[i]));
                tableWidget_table->setItem(i, 0, item);
                tableWidget_table->setItem(i, 1, item1);
            }
            m_lineEdit_x1->setText(MxFunctions::mNumberToString(tbl.x1));
            m_lineEdit_x2->setText(MxFunctions::mNumberToString(tbl.x2));
            m_lineEdit_x3->setText(MxFunctions::mNumberToString(tbl.x3));
            m_lineEdit_x4->setText(MxFunctions::mNumberToString(tbl.x4));
            break;
        }
        case 9:
        {
            // 周期函数
            PeriodicCol per = _funcData->periodic();
            m_lineEditPeriodicFreq->setText(MxFunctions::mNumberToString(per.freq));
            m_lineEditPeriodicT0->setText(MxFunctions::mNumberToString(per.t0));
            m_lineEditPeriodicA0->setText(MxFunctions::mNumberToString(per.A0));

            int size = static_cast<int>(per.A.size());
            tableWidget_periodic->clearContents();
            tableWidget_periodic->setRowCount(size);
            for (int i = 0; i < size; ++i)
            {
                auto* idxItem = new QTableWidgetItem(MxFunctions::mNumberToString(i + 1));
                idxItem->setFlags(idxItem->flags() & ~Qt::ItemIsEditable);
                tableWidget_periodic->setItem(i, 0, idxItem);
                tableWidget_periodic->setItem(i, 1, new QTableWidgetItem(MxFunctions::mNumberToString(per.A[i])));
                tableWidget_periodic->setItem(i, 2, new QTableWidgetItem(MxFunctions::mNumberToString(per.B[i])));
            }
            break;
        }
        case 10:
        {
            // 调制函数
            ModulatedCol mod = _funcData->modulated();
            m_lineEditModulatedA0->setText(MxFunctions::mNumberToString(mod.A0));
            m_lineEditModulatedA->setText(MxFunctions::mNumberToString(mod.A));
            m_lineEditModulatedT0->setText(MxFunctions::mNumberToString(mod.t0));
            m_lineEditModulatedF1->setText(MxFunctions::mNumberToString(mod.f1));
            m_lineEditModulatedF2->setText(MxFunctions::mNumberToString(mod.f2));
            break;
        }
        case 11:
        {
            // 指数衰减函数
            ExpDecayCol ed = _funcData->expDecay();
            m_lineEditExpDecayA0->setText(MxFunctions::mNumberToString(ed.A0));
            m_lineEditExpDecayAm->setText(MxFunctions::mNumberToString(ed.Am));
            m_lineEditExpDecayT0->setText(MxFunctions::mNumberToString(ed.t0));
            m_lineEditExpDecayTd->setText(MxFunctions::mNumberToString(ed.td));
            break;
        }
        }


        comboBox->blockSignals(false);
        lineEdit->blockSignals(false);
        lineEdit_2->blockSignals(false);
        spinSamplesAna->blockSignals(false);
        lineEdit_3->blockSignals(false);
        tableWidget->blockSignals(false);
        tableWidget_interp->blockSignals(false);
        comboBox_5->blockSignals(false);
        comboBox_4->blockSignals(false);
        tableWidget_3->blockSignals(false);
        lineEdit_4->blockSignals(false);
        lineEdit_9->blockSignals(false);
        lineEdit_10->blockSignals(false);
        tableWidget_table->blockSignals(false);
        comboBox_xAxisType->blockSignals(false);
        comboBox_yAxisType->blockSignals(false);
        comboBox_extrapTable->blockSignals(false);
        m_lineEdit_x1->blockSignals(false);
        m_lineEdit_x2->blockSignals(false);
        m_lineEdit_x3->blockSignals(false);
        m_lineEdit_x4->blockSignals(false);
        m_lineEditPeriodicFreq->blockSignals(false);
        m_lineEditPeriodicT0->blockSignals(false);
        m_lineEditPeriodicA0->blockSignals(false);
        tableWidget_periodic->blockSignals(false);
        m_lineEditModulatedA0->blockSignals(false);
        m_lineEditModulatedA->blockSignals(false);
        m_lineEditModulatedT0->blockSignals(false);
        m_lineEditModulatedF1->blockSignals(false);
        m_lineEditModulatedF2->blockSignals(false);

    }
    /* ========== 构造 UI（控件名 100% 对齐旧 .ui） ========== */
    void mFunctionPanel::buildUI()
    {
        auto* mainLay = new QVBoxLayout(this);
        mainLay->setSpacing(2);

        /* 1. 类型 */
        m_typeFold = new FoldSection("类型", this);
        label = new PanelLabel("类型");
        label->setObjectName("label");
        comboBox = new QComboBox(this);    comboBox->setObjectName("comboBox");
        comboBox->installEventFilter(this);   // 加这一行
        comboBox->addItems({ "解析", "离散", "插值", "分段", "阶跃", "表格", "可平移表格", "可缩放表格", "幂函数", "周期函数", "调制函数", "指数衰减函数" });
        //隐藏 解析，离散，分段，阶跃，调用处用下标索引，不能直接删除
        {
            //隐藏
            QListView* view = qobject_cast<QListView*>(comboBox->view());
            if (view) {
                view->setRowHidden(0, true);
                view->setRowHidden(1, true);
                view->setRowHidden(3, true);
                view->setRowHidden(4, true);
            }

            //同时禁用隐藏项，防止鼠标滚轮还能滚动到
            QStandardItemModel* model = qobject_cast<QStandardItemModel*>(comboBox->model());
            if (model) {
                model->item(0)->setFlags(model->item(0)->flags() & ~Qt::ItemIsEnabled);
                model->item(1)->setFlags(model->item(1)->flags() & ~Qt::ItemIsEnabled);
                model->item(3)->setFlags(model->item(3)->flags() & ~Qt::ItemIsEnabled);
                model->item(4)->setFlags(model->item(4)->flags() & ~Qt::ItemIsEnabled);
            }
        }
        m_typeFold->addRow(label, comboBox);
        mainLay->addWidget(m_typeFold);

        /* 2. 变量 */
        m_varFold = new FoldSection("变量", this);
        label_2 = new PanelLabel("变量"); label_2->setObjectName("label_2");
        label_5 = new QLabel("x");    label_5->setObjectName("label_5");
        auto* varLay = new QHBoxLayout;
        varLay->addWidget(label_2); varLay->addWidget(label_5);
        m_varFold->addRow(varLay);
        mainLay->addWidget(m_varFold);

        /* 3. 变量范围 */
        m_rangeFold = new FoldSection("变量范围", this);
        label_3 = new PanelLabel("变量范围"); label_3->setObjectName("label_3");
        lineEdit = new QLineEdit(this);       lineEdit->setObjectName("lineEdit");
        label_6 = new QLabel("一");     label_6->setObjectName("label_6");
        lineEdit_2 = new QLineEdit(this);     lineEdit_2->setObjectName("lineEdit_2");
        auto* rangeLay = new QHBoxLayout;
        rangeLay->addWidget(label_3); rangeLay->addWidget(lineEdit);
        rangeLay->addWidget(label_6); rangeLay->addWidget(lineEdit_2);
        m_rangeFold->addRow(rangeLay);
        mainLay->addWidget(m_rangeFold);

        /* 4. 表达式（解析） */
        m_exprFold = new FoldSection("表达式", this);
        /* ---- 采样点数 ---- */
        auto* labSamplesAna = new PanelLabel("采样点数");
        spinSamplesAna = new QSpinBox(this);
        spinSamplesAna->setObjectName("spinSamplesAna");
        spinSamplesAna->setRange(10, 10000);
        spinSamplesAna->setValue(10);          // 默认 100 点
        m_exprFold->addRow(labSamplesAna, spinSamplesAna);
        connect(spinSamplesAna, QOverload<int>::of(&QSpinBox::valueChanged),
            [this] { onAutoRefresh(); });

        label_4 = new PanelLabel("表达式"); label_4->setObjectName("label_4");
        lineEdit_3 = new QLineEdit(this);     lineEdit_3->setObjectName("lineEdit_3");
        m_exprFold->addRow(label_4, lineEdit_3);
        auto* exprBtnLay = new QHBoxLayout;
        pushButton_10 = new QPushButton("导出曲线"); pushButton_10->setObjectName("pushButton_10");
        pushButton = new QPushButton("绘制曲线");    pushButton->setObjectName("pushButton");
        connect(pushButton, &QPushButton::clicked, this, [=]() {
           auto cdock= mDockLayoutManager::getInstance()->m_docks["Curve"];
           bool visible = cdock->toggleViewAction()->isChecked();
           if(!visible)  cdock->toggleViewAction()->trigger();
        });


        exprBtnLay->addWidget(pushButton_10); exprBtnLay->addWidget(pushButton);
        m_exprFold->addRow(exprBtnLay);
        mainLay->addWidget(m_exprFold);

        /* 5. 离散数据 */
        QString scrollBarStyle = R"(
QTableWidget
{
    alternate-background-color:#ECF5FF;
}
/*=============== 垂直滚动条 ===============*/
QTableWidget QScrollBar:vertical {
    background: rgba(0,0,0,0);   /* 轨道全透明 */
    width: 6px;                  /* 默认细线 */
    margin: 0px 0px 0px 0px;
    border: none;
}

/* 滑块 */
QTableWidget QScrollBar::handle:vertical {
    background: rgba(0,0,0,35);  /* 35% 灰 */
    min-height: 20px;
    border-radius: 3px;          /* 圆角 */
}

/* 悬浮时 */
QTableWidget QScrollBar::handle:vertical:hover {
    background: rgba(0,0,0,55);  /* 加深 */
    width: 8px;                  /* 稍微变宽 */
}

/* 去掉两端箭头按钮 */
QTableWidget QScrollBar::add-line:vertical,
QTableWidget QScrollBar::sub-line:vertical {
    border: none;
    background: none;
    height: 0px;
}

/* 滑块上下空白区（可省） */
QTableWidget QScrollBar::add-page:vertical,
QTableWidget QScrollBar::sub-page:vertical {
    background: none;
}

/*=============== 水平滚动条（同风格） ===============*/
QTableWidget QScrollBar:horizontal {
    background: rgba(0,0,0,0);
    height: 6px;
    margin: 0px;
    border: none;
}
QTableWidget QScrollBar::handle:horizontal {
    background: rgba(0,0,0,35);
    min-width: 20px;
    border-radius: 3px;
}
QTableWidget QScrollBar::handle:horizontal:hover {
    background: rgba(0,0,0,55);
    height: 8px;
}
QTableWidget QScrollBar::add-line:horizontal,
QTableWidget QScrollBar::sub-line:horizontal {
    border: none;
    background: none;
    width: 0px;
}
)";
        m_discFold = new FoldSection("离散数据", this);
        tableWidget =new mTableWidget();
        tableWidget->setStyleSheet(scrollBarStyle);
        tableWidget->setColumnCount(2);
        tableWidget->setObjectName("tableWidget");
        tableWidget->setHorizontalHeaderLabels({ "x", "f(x)" });
        m_discFold->addRow(tableWidget);

        auto* discBtnLay = new QHBoxLayout;
        pushButton_5 = new QPushButton();  pushButton_5->setObjectName("pushButton_5");
        pushButton_5->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        pushButton_6 = new QPushButton();  pushButton_6->setObjectName("pushButton_6");
        pushButton_6->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        pushButton_7 = new QPushButton("上移"); pushButton_7->setObjectName("pushButton_7");
        pushButton_8 = new QPushButton("下移"); pushButton_8->setObjectName("pushButton_8");
        discBtnLay->addWidget(pushButton_5); discBtnLay->addWidget(pushButton_6);
        discBtnLay->addWidget(pushButton_7); discBtnLay->addWidget(pushButton_8);
        m_discFold->addRow(discBtnLay);

        auto* discBtnLay2 = new QHBoxLayout;
        //pushButton_2 = new QPushButton("导入数据"); pushButton_2->setObjectName("pushButton_2");
        pushButton_3 = new QPushButton("导出数据"); pushButton_3->setObjectName("pushButton_3");
        pushButton_4 = new QPushButton("绘制曲线"); pushButton_4->setObjectName("pushButton_4");

        connect(pushButton_4, &QPushButton::clicked, this, [=]() {
            auto cdock = mDockLayoutManager::getInstance()->m_docks["Curve"];
            bool visible = cdock->toggleViewAction()->isChecked();
            if (!visible)  cdock->toggleViewAction()->trigger();
        });
        discBtnLay2->addWidget(pushButton_2); discBtnLay2->addWidget(pushButton_3);
        discBtnLay2->addWidget(pushButton_4);
        m_discFold->addRow(discBtnLay2);
        mainLay->addWidget(m_discFold);

        /* 6. 插值方式 折叠体：只留参数控件 */
        m_interpFold = new FoldSection("插值方式", this);

        auto* labSamples = new QLabel("采样点数");
        auto* spinSamples = new QSpinBox;
        spinSamples->setObjectName("spinSamples");
        spinSamples->setRange(10, 10000);
        spinSamples->setValue(10);
        m_interpFold->addRow(labSamples, spinSamples);

        //label_14 = new PanelLabel("插值方式");
        //comboBox_5 = new QComboBox;
        //comboBox_5->installEventFilter(this);   // 插值方式
        //comboBox_5->setObjectName("comboBox_5");
        //comboBox_5->addItems({ "临近点插值", "线性", "分段三次", "三次样条", "对数插值"});
        //m_interpFold->addRow(label_14, comboBox_5);

        //mainLay->addWidget(m_interpFold);          // 先完成“插值方式”

        /* 插值数据 折叠体：表格 + 按钮栏 + 导出（一张表、一套按钮） */
        m_interpDataFold = new FoldSection("插值数据", this);

        /* ---- 表格 （只 new 一次）---- */
        tableWidget_interp = new mTableWidget();
        tableWidget_interp->setStyleSheet(scrollBarStyle);
        tableWidget_interp->setColumnCount(3);
        for (int i = 0; i < 3; i++)
        {
            tableWidget_interp->setItem(0, i, new QTableWidgetItem());
        }
        tableWidget_interp->setObjectName("tableWidget_interp");
        tableWidget_interp->setHorizontalHeaderLabels({ "序号", "x", "y" });
        tableWidget_interp->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        tableWidget_interp->verticalHeader()->setVisible(false);
        tableWidget_interp->setMinimumHeight(120);
        m_interpDataFold->addRow(tableWidget_interp);

        /* ---- 按钮栏 ---- */
        auto* interpBtnLay = new QHBoxLayout;
        auto* interpBtnLay2 = new QHBoxLayout;
        pushButton_addInterp = new QPushButton("");
        pushButton_addInterp->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        pushButton_delInterp = new QPushButton("");
        pushButton_delInterp->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        pushButton_upInterp = new QPushButton("上移");
        pushButton_downInterp = new QPushButton("下移");
        pushButton_drawInterp = new QPushButton("绘制曲线");   // <-- 新增
        connect(pushButton_drawInterp, &QPushButton::clicked, this, [=]() {
            auto cdock = mDockLayoutManager::getInstance()->m_docks["Curve"];
            bool visible = cdock->toggleViewAction()->isChecked();
            if (!visible) cdock->toggleViewAction()->trigger();
            
        });
        //pushButton_trashInterp = new QPushButton("删除");
        interpBtnLay->addWidget(pushButton_addInterp);
        interpBtnLay->addWidget(pushButton_delInterp);
        interpBtnLay->addWidget(pushButton_upInterp);
        interpBtnLay->addWidget(pushButton_downInterp);
        //interpBtnLay->addWidget(pushButton_drawInterp);            // <-- 新增
        //interpBtnLay->addWidget(pushButton_trashInterp);
        m_interpDataFold->addRow(interpBtnLay);

        /* ---- 导出按钮 ---- */
        auto* pushButton_exportInterpOnly = new QPushButton("导出数据");
        pushButton_exportInterpOnly->setObjectName("pushButton_exportInterpOnly");
        interpBtnLay2->addWidget(pushButton_exportInterpOnly);
        interpBtnLay2->addWidget(pushButton_drawInterp);
        m_interpDataFold->addRow(interpBtnLay2);
        mainLay->addWidget(m_interpDataFold);   // 再插“插值数据”折叠体

        /* 6.5 表格数据 折叠体 */
        m_tableDataFold = new FoldSection("表格数据", this);
        tableWidget_table = new mTableWidget();
        tableWidget_table->setStyleSheet(scrollBarStyle);
        tableWidget_table->setColumnCount(3);
        for (int i = 0; i < 3; i++)
        {
            tableWidget_table->setItem(0, i, new QTableWidgetItem());
        }
        tableWidget_table->setObjectName("tableWidget_table");
        tableWidget_table->setHorizontalHeaderLabels({ "序号", "x", "y" });
        tableWidget_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        tableWidget_table->verticalHeader()->setVisible(false);
        tableWidget_table->setMinimumHeight(120);
        m_tableDataFold->addRow(tableWidget_table);

        auto* tableBtnLay = new QHBoxLayout;
        auto* tableBtnLay2 = new QHBoxLayout;
        pushButton_addTable = new QPushButton("");
        pushButton_addTable->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        pushButton_delTable = new QPushButton("");
        pushButton_delTable->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        pushButton_upTable = new QPushButton("上移");
        pushButton_downTable = new QPushButton("下移");
        pushButton_drawTable = new QPushButton("绘制曲线");
        connect(pushButton_drawTable, &QPushButton::clicked, this, [=]() {
            auto cdock = mDockLayoutManager::getInstance()->m_docks["Curve"];
            bool visible = cdock->toggleViewAction()->isChecked();
            if (!visible) cdock->toggleViewAction()->trigger();
        });
        tableBtnLay->addWidget(pushButton_addTable);
        tableBtnLay->addWidget(pushButton_delTable);
        tableBtnLay->addWidget(pushButton_upTable);
        tableBtnLay->addWidget(pushButton_downTable);
        m_tableDataFold->addRow(tableBtnLay);

        pushButton_exportTable = new QPushButton("导出数据");
        pushButton_exportTable->setObjectName("pushButton_exportTable");
        tableBtnLay2->addWidget(pushButton_exportTable);
        tableBtnLay2->addWidget(pushButton_drawTable);
        m_tableDataFold->addRow(tableBtnLay2);
        mainLay->addWidget(m_tableDataFold);

        /*  插值方式（原5种 + 新增 光滑/阶跃）+ 外推方式 */
        m_extrapFold = new FoldSection("插值方式", this);
        label_14 = new PanelLabel("插值方式");
        comboBox_5 = new QComboBox;
        comboBox_5->installEventFilter(this);
        comboBox_5->setObjectName("comboBox_5");
        comboBox_5->addItems({ "临近点插值", "线性", "分段三次", "三次样条" ,"对数插值","光滑","阶跃"});
        //保留线性、对数插值、光滑、阶跃，调用处用下标索引，不能直接删除
        {
            //隐藏
            QListView* view = qobject_cast<QListView*>(comboBox_5->view());
            if (view) {
                view->setRowHidden(0, true);
                view->setRowHidden(2, true);
                view->setRowHidden(3, true);
            }

            //同时禁用隐藏项，防止鼠标滚轮还能滚动到
            QStandardItemModel* model = qobject_cast<QStandardItemModel*>(comboBox_5->model());
            if (model) {
                model->item(0)->setFlags(model->item(0)->flags() & ~Qt::ItemIsEnabled);
                model->item(2)->setFlags(model->item(2)->flags() & ~Qt::ItemIsEnabled);
                model->item(3)->setFlags(model->item(3)->flags() & ~Qt::ItemIsEnabled);
            }
        }
        m_extrapFold->addRow(label_14, comboBox_5);

        label_13 = new PanelLabel("外推方式");
        comboBox_4 = new QComboBox;
        comboBox_4->installEventFilter(this);
        comboBox_4->setObjectName("comboBox_4");
        comboBox_4->addItems({ "常数外推", "线性外推" });
        m_extrapFold->addRow(label_13, comboBox_4);
        mainLay->addWidget(m_extrapFold);

        /* 8. 分段数据 */
        m_pieceFold = new FoldSection("分段数据", this);
        tableWidget_3 = new mTableWidget();   // 多 1 列放采样点数
        tableWidget_3->setStyleSheet(scrollBarStyle);
        tableWidget_3->setColumnCount(5);
        for (int i = 0; i < 5; i++)
        {
            tableWidget_3->setItem(0, i, new QTableWidgetItem());
        }
        tableWidget_3->setObjectName("tableWidget_3");
        // ↓↓↓ 在这里一次性写完 ↓↓↓
        tableWidget_3->setColumnCount(5);
        tableWidget_3->setHorizontalHeaderLabels({ "序号", "起始", "结束", "表达式", "采样点数" });
        tableWidget_3->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

        tableWidget_3->verticalHeader()->setVisible(false);
        m_pieceFold->addRow(tableWidget_3);


        /* ---- 分段按钮栏 ---- */
        auto* pieceBtnLay = new QHBoxLayout;
        pushButton_addPiece = new QPushButton("");
        pushButton_addPiece->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        pushButton_delPiece = new QPushButton("");
        pushButton_delPiece->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        pushButton_upPiece = new QPushButton("上移");
        pushButton_downPiece = new QPushButton("下移");
        //pushButton_trashPiece = new QPushButton("删除");
        pieceBtnLay->addWidget(pushButton_addPiece);
        pieceBtnLay->addWidget(pushButton_delPiece);
        pieceBtnLay->addWidget(pushButton_upPiece);
        pieceBtnLay->addWidget(pushButton_downPiece);
        //pieceBtnLay->addWidget(pushButton_trashPiece);
        m_pieceFold->addRow(pieceBtnLay);
        /* ===== 新增一行：导出 / 绘制 ===== */
        auto* pieceBtnLay2 = new QHBoxLayout;
        pushButton_13 = new QPushButton("导出数据"); pieceBtnLay2->addWidget(pushButton_13);
        pushButton_14 = new QPushButton("绘制曲线"); pieceBtnLay2->addWidget(pushButton_14);
        m_pieceFold->addRow(pieceBtnLay2);
        connect(pushButton_14, &QPushButton::clicked, this, [=]() {
            auto cdock = mDockLayoutManager::getInstance()->m_docks["Curve"];
            bool visible = cdock->toggleViewAction()->isChecked();
            if (!visible)  cdock->toggleViewAction()->trigger();
        });
  
       
        mainLay->addWidget(m_pieceFold);

        /* 9. 阶跃参数 */
        m_stepFold = new FoldSection("阶跃参数", this);
        label_9 = new PanelLabel("阶跃位置"); label_9->setObjectName("label_9");
        lineEdit_4 = new QLineEdit;       lineEdit_4->setObjectName("lineEdit_4");
        label_23 = new PanelLabel("起始值");  label_23->setObjectName("label_23");
        lineEdit_9 = new QLineEdit;       lineEdit_9->setObjectName("lineEdit_9");
        label_24 = new PanelLabel("终止值");  label_24->setObjectName("label_24");
        lineEdit_10 = new QLineEdit;      lineEdit_10->setObjectName("lineEdit_10");
        m_stepFold->addRow(label_9, lineEdit_4);
        m_stepFold->addRow(label_23, lineEdit_9);
        m_stepFold->addRow(label_24, lineEdit_10);
        auto* stepBtnLay = new QHBoxLayout;
        pushButton_11 = new QPushButton("导出曲线"); pushButton_11->setObjectName("pushButton_11");
        pushButton_12 = new QPushButton("绘制曲线"); pushButton_12->setObjectName("pushButton_12");
        connect(pushButton_12, &QPushButton::clicked, this, [=]() {
            auto cdock = mDockLayoutManager::getInstance()->m_docks["Curve"];
            bool visible = cdock->toggleViewAction()->isChecked();
            if (!visible)  cdock->toggleViewAction()->trigger();
        });
        stepBtnLay->addWidget(pushButton_11); stepBtnLay->addWidget(pushButton_12);
        m_stepFold->addRow(stepBtnLay);
        mainLay->addWidget(m_stepFold);

        /* 10. 表格函数参数 */
        m_tableFold = new FoldSection("表格参数", this);
        m_labXAxisType = new PanelLabel("x轴类型");
        comboBox_xAxisType = new QComboBox;
        comboBox_xAxisType->installEventFilter(this);
        comboBox_xAxisType->addItems({ "线性", "对数" });
        m_tableFold->addRow(m_labXAxisType, comboBox_xAxisType);

        m_labYAxisType = new PanelLabel("y轴类型");
        comboBox_yAxisType = new QComboBox;
        comboBox_yAxisType->installEventFilter(this);
        comboBox_yAxisType->addItems({ "线性", "对数" });
        m_tableFold->addRow(m_labYAxisType, comboBox_yAxisType);

        m_labExtrapTable = new PanelLabel("外推方式");
        comboBox_extrapTable = new QComboBox;
        comboBox_extrapTable->installEventFilter(this);
        comboBox_extrapTable->addItems({ "常数外推", "线性外推" });
        m_tableFold->addRow(m_labExtrapTable, comboBox_extrapTable);

        m_labX1 = new PanelLabel("偏移量");
        m_lineEdit_x1 = new QLineEdit(this);
        m_lineEdit_x1->setObjectName("lineEdit_x1");
        m_tableFold->addRow(m_labX1, m_lineEdit_x1);
        /* 初始隐藏 x1（普通表格时不显示） */
        m_labX1->setParent(nullptr);
        m_lineEdit_x1->setParent(nullptr);

        m_labX2 = new PanelLabel("缩放值");
        m_lineEdit_x2 = new QLineEdit(this);
        m_lineEdit_x2->setObjectName("lineEdit_x2");
        m_tableFold->addRow(m_labX2, m_lineEdit_x2);
        /* 初始隐藏 x2（普通/可平移表格时不显示） */
        m_labX2->setParent(nullptr);
        m_lineEdit_x2->setParent(nullptr);

        m_labX3 = new PanelLabel("横坐标下限");
        m_lineEdit_x3 = new QLineEdit(this);
        m_lineEdit_x3->setObjectName("lineEdit_x3");
        m_tableFold->addRow(m_labX3, m_lineEdit_x3);
        /* 初始隐藏 x3（普通/可平移/可缩放表格时不显示） */
        m_labX3->setParent(nullptr);
        m_lineEdit_x3->setParent(nullptr);

        m_labX4 = new PanelLabel("横坐标上限");
        m_lineEdit_x4 = new QLineEdit(this);
        m_lineEdit_x4->setObjectName("lineEdit_x4");
        m_tableFold->addRow(m_labX4, m_lineEdit_x4);
        /* 初始隐藏 x4（普通/可平移/可缩放表格时不显示） */
        m_labX4->setParent(nullptr);
        m_lineEdit_x4->setParent(nullptr);

        mainLay->addWidget(m_tableFold);

        /* 10.5 周期函数参数 */
        m_periodicParamFold = new FoldSection("参数数据", this);
        m_labPeriodicFreq = new PanelLabel("频率(f)");
        m_lineEditPeriodicFreq = new QLineEdit(this);
        m_lineEditPeriodicFreq->setObjectName("lineEditPeriodicFreq");
        m_periodicParamFold->addRow(m_labPeriodicFreq, m_lineEditPeriodicFreq);

        m_labPeriodicT0 = new PanelLabel("起始时间(t0)");
        m_lineEditPeriodicT0 = new QLineEdit(this);
        m_lineEditPeriodicT0->setObjectName("lineEditPeriodicT0");
        m_periodicParamFold->addRow(m_labPeriodicT0, m_lineEditPeriodicT0);

        m_labPeriodicA0 = new PanelLabel("初始幅值(A0)");
        m_lineEditPeriodicA0 = new QLineEdit(this);
        m_lineEditPeriodicA0->setObjectName("lineEditPeriodicA0");
        m_periodicParamFold->addRow(m_labPeriodicA0, m_lineEditPeriodicA0);

        m_labPeriodicFormula = new QLabel();
        {
            QPixmap pm(":/mMainWidgets/Icon/function/zhouqihanshu");
            m_labPeriodicFormula->setPixmap(pm.scaled(pm.size() * 0.6, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        m_periodicParamFold->addRow(m_labPeriodicFormula);

        /* 10.6 周期函数数据 */
        m_periodicDataFold = new FoldSection("周期函数数据", this);
        tableWidget_periodic = new mTableWidget();
        tableWidget_periodic->setStyleSheet(scrollBarStyle);
        tableWidget_periodic->setColumnCount(3);
        for (int i = 0; i < 3; i++)
        {
            tableWidget_periodic->setItem(0, i, new QTableWidgetItem());
        }
        tableWidget_periodic->setObjectName("tableWidget_periodic");
        tableWidget_periodic->setHorizontalHeaderLabels({ "序号", "幅值1(An)", "幅值2(Bn)" });
        tableWidget_periodic->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        tableWidget_periodic->verticalHeader()->setVisible(false);
        tableWidget_periodic->setMinimumHeight(120);
        m_periodicDataFold->addRow(tableWidget_periodic);

        auto* periodicBtnLay = new QHBoxLayout;
        pushButton_addPeriodic = new QPushButton("");
        pushButton_addPeriodic->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        pushButton_delPeriodic = new QPushButton("");
        pushButton_delPeriodic->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        pushButton_upPeriodic = new QPushButton("上移");
        pushButton_downPeriodic = new QPushButton("下移");
        periodicBtnLay->addWidget(pushButton_addPeriodic);
        periodicBtnLay->addWidget(pushButton_delPeriodic);
        periodicBtnLay->addWidget(pushButton_upPeriodic);
        periodicBtnLay->addWidget(pushButton_downPeriodic);
        m_periodicDataFold->addRow(periodicBtnLay);
        mainLay->addWidget(m_periodicDataFold);

        mainLay->addWidget(m_periodicParamFold);

        /* 10.7 调制函数参数 */
        m_modulatedFold = new FoldSection("参数数据", this);
        m_labModulatedA0 = new PanelLabel("初始幅值(A0)");
        m_lineEditModulatedA0 = new QLineEdit(this);
        m_lineEditModulatedA0->setObjectName("lineEditModulatedA0");
        m_modulatedFold->addRow(m_labModulatedA0, m_lineEditModulatedA0);

        m_labModulatedA = new PanelLabel("幅值(A)");
        m_lineEditModulatedA = new QLineEdit(this);
        m_lineEditModulatedA->setObjectName("lineEditModulatedA");
        m_modulatedFold->addRow(m_labModulatedA, m_lineEditModulatedA);

        m_labModulatedT0 = new PanelLabel("起始时间(t0)");
        m_lineEditModulatedT0 = new QLineEdit(this);
        m_lineEditModulatedT0->setObjectName("lineEditModulatedT0");
        m_modulatedFold->addRow(m_labModulatedT0, m_lineEditModulatedT0);

        m_labModulatedF1 = new PanelLabel("频率1(f1)");
        m_lineEditModulatedF1 = new QLineEdit(this);
        m_lineEditModulatedF1->setObjectName("lineEditModulatedF1");
        m_modulatedFold->addRow(m_labModulatedF1, m_lineEditModulatedF1);

        m_labModulatedF2 = new PanelLabel("频率2(f2)");
        m_lineEditModulatedF2 = new QLineEdit(this);
        m_lineEditModulatedF2->setObjectName("lineEditModulatedF2");
        m_modulatedFold->addRow(m_labModulatedF2, m_lineEditModulatedF2);

        m_labModulatedFormula = new QLabel();
        {
            QPixmap pm(":/mMainWidgets/Icon/function/tiaozhihanshu");
            m_labModulatedFormula->setPixmap(pm.scaled(pm.size() * 0.6, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        m_modulatedFold->addRow(m_labModulatedFormula);

        mainLay->addWidget(m_modulatedFold);

        /* 10.8 指数衰减函数参数 */
        m_expDecayFold = new FoldSection("参数数据", this);
        m_labExpDecayA0 = new PanelLabel("初始幅值(A0)");
        m_lineEditExpDecayA0 = new QLineEdit(this);
        m_lineEditExpDecayA0->setObjectName("lineEditExpDecayA0");
        m_expDecayFold->addRow(m_labExpDecayA0, m_lineEditExpDecayA0);

        m_labExpDecayAm = new PanelLabel("最大幅值(Am)");
        m_lineEditExpDecayAm = new QLineEdit(this);
        m_lineEditExpDecayAm->setObjectName("lineEditExpDecayAm");
        m_expDecayFold->addRow(m_labExpDecayAm, m_lineEditExpDecayAm);

        m_labExpDecayT0 = new PanelLabel("起始时间(t0)");
        m_lineEditExpDecayT0 = new QLineEdit(this);
        m_lineEditExpDecayT0->setObjectName("lineEditExpDecayT0");
        m_expDecayFold->addRow(m_labExpDecayT0, m_lineEditExpDecayT0);

        m_labExpDecayTd = new PanelLabel("衰减时间(td)");
        m_lineEditExpDecayTd = new QLineEdit(this);
        m_lineEditExpDecayTd->setObjectName("lineEditExpDecayTd");
        m_expDecayFold->addRow(m_labExpDecayTd, m_lineEditExpDecayTd);

        m_labExpDecayFormula = new QLabel();
        {
            QPixmap pm(":/mMainWidgets/Icon/function/zhishushuaijian");
            m_labExpDecayFormula->setPixmap(pm.scaled(pm.size() * 0.6, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        m_expDecayFold->addRow(m_labExpDecayFormula);

        mainLay->addWidget(m_expDecayFold);

        /* 11. 统计 */
        m_statFold = new FoldSection("统计", this);
        QLabel* labSt = new QLabel("总选择：");
        QLabel* labCt = new QLabel("无选择");
        labCt->setObjectName("labelPickStates"); // 与旧代码绑定一致
        m_statFold->addRow(labSt, labCt);
        mainLay->addWidget(m_statFold);

        mainLay->addStretch();

        /* 原函数：初始化表头、采样点数等 */
        setupExtraControls();

        /*用来禁止滚轮会滑动采样点数的值 在 buildUI() 里把采样旋钮找出来 */
       // auto* spinSamplesAna = findChild<QSpinBox*>("spinSamplesAna");
        //auto* spinSamples = findChild<QSpinBox*>("spinSamples");
        auto* spinSamplesPiece = findChild<QSpinBox*>("spinSamplesPiece");
        auto* spinSamplesStep = findChild<QSpinBox*>("spinSamplesStep");
        for (auto* sp : { spinSamplesAna, spinSamples, spinSamplesPiece, spinSamplesStep })   // 已统一
            if (sp) sp->installEventFilter(this);

    }
    void mFunctionPanel::initData()
    {
        //解析
        lineEdit->setText("0");
        lineEdit_2->setText("0");
        spinSamplesAna->setValue(10);
        lineEdit_3->setText("");
        //离散
        tableWidget->clearContents();
        tableWidget->setRowCount(3);
        for (int i = 0; i < 3; i++)
        {
            QTableWidgetItem* item = new
                QTableWidgetItem(MxFunctions::mNumberToString(i+1));
            QTableWidgetItem* item1 = new
                QTableWidgetItem(MxFunctions::mNumberToString(0));
            QTableWidgetItem* item2 = new
                QTableWidgetItem(MxFunctions::mNumberToString(0));
            tableWidget->setItem(i, 0, item);
            tableWidget->setItem(i, 1, item1);
            tableWidget->setItem(i, 2, item2);
        }
        //插值
        tableWidget_interp->clearContents();
        tableWidget_interp->setRowCount(1);
        QTableWidgetItem* item = new
            QTableWidgetItem(MxFunctions::mNumberToString(1));
        QTableWidgetItem* item1 = new
            QTableWidgetItem(MxFunctions::mNumberToString(0));
        QTableWidgetItem* item2 = new
            QTableWidgetItem(MxFunctions::mNumberToString(0));
        tableWidget_interp->setItem(0, 0, item);
        tableWidget_interp->setItem(0, 1, item1);
        tableWidget_interp->setItem(0, 2, item2);
        comboBox_5->setCurrentIndex(0);
        comboBox_4->setCurrentIndex(0);
        //分段
        tableWidget_3->setRowCount(2);
        for (int i = 0; i < 2; i++)
        {
            QTableWidgetItem* item = new QTableWidgetItem(MxFunctions::mNumberToString(i + 1));
            QTableWidgetItem* item1 = new QTableWidgetItem(MxFunctions::mNumberToString(0));
            QTableWidgetItem* item2 = new QTableWidgetItem(MxFunctions::mNumberToString(0));
            QTableWidgetItem* item3 = new QTableWidgetItem(MxFunctions::mNumberToString(0));
            QTableWidgetItem* item4 = new QTableWidgetItem(MxFunctions::mNumberToString(10));
            tableWidget_3->setItem(i, 0, item);
            tableWidget_3->setItem(i, 1, item1);
            tableWidget_3->setItem(i, 2, item2);
            tableWidget_3->setItem(i, 3, item3);
            tableWidget_3->setItem(i, 4, item4);
        }
        //阶跃
        lineEdit_4->setText("");
        lineEdit_9->setText("");
        lineEdit_10->setText("");
        //表格 / 可平移表格
        tableWidget_table->clearContents();
        tableWidget_table->setRowCount(3);
        for (int i = 0; i < 3; i++)
        {
            auto* idxItem = new QTableWidgetItem(MxFunctions::mNumberToString(i + 1));
            idxItem->setFlags(idxItem->flags() & ~Qt::ItemIsEditable);
            tableWidget_table->setItem(i, 0, idxItem);
            tableWidget_table->setItem(i, 1, new QTableWidgetItem("0"));
            tableWidget_table->setItem(i, 2, new QTableWidgetItem("0"));
        }
        comboBox_xAxisType->setCurrentIndex(0);
        comboBox_yAxisType->setCurrentIndex(0);
        comboBox_extrapTable->setCurrentIndex(0);
        m_lineEdit_x1->setText("0");
        m_lineEdit_x2->setText("1");
        m_lineEdit_x3->setText("0");
        m_lineEdit_x4->setText("10");
        //周期函数
        m_lineEditPeriodicFreq->setText("1");
        m_lineEditPeriodicT0->setText("0");
        m_lineEditPeriodicA0->setText("0");
        tableWidget_periodic->clearContents();
        tableWidget_periodic->setRowCount(3);
        for (int i = 0; i < 3; i++)
        {
            auto* idxItem = new QTableWidgetItem(MxFunctions::mNumberToString(i + 1));
            idxItem->setFlags(idxItem->flags() & ~Qt::ItemIsEditable);
            tableWidget_periodic->setItem(i, 0, idxItem);
            tableWidget_periodic->setItem(i, 1, new QTableWidgetItem("0"));
            tableWidget_periodic->setItem(i, 2, new QTableWidgetItem("0"));
        }
        //调制函数
        m_lineEditModulatedA0->setText("0");
        m_lineEditModulatedA->setText("0");
        m_lineEditModulatedT0->setText("0");
        m_lineEditModulatedF1->setText("0");
        m_lineEditModulatedF2->setText("0");
        //指数衰减函数
        m_lineEditExpDecayA0->setText("0");
        m_lineEditExpDecayAm->setText("0");
        m_lineEditExpDecayT0->setText("0");
        m_lineEditExpDecayTd->setText("0");
    }
    /* ========== 信号槽连接（与旧代码事件名完全一致） ========== */
    void mFunctionPanel::connectSignals()
    {
        /* 类型切换 */

        /* 300 ms 防抖刷新 */
        auto refresh = [this] {
            if (!g_debounce) {
                g_debounce = new QTimer(this);
                g_debounce->setSingleShot(true);
                connect(g_debounce, &QTimer::timeout, this, &mFunctionPanel::onAutoRefresh);
            }
            g_debounce->start(300);
        };
        /* 所有输入框均触发刷新 */
        for (auto* ed : { lineEdit, lineEdit_2, lineEdit_3, lineEdit_4,
                         lineEdit_9, lineEdit_10, m_lineEdit_x1, m_lineEdit_x2, m_lineEdit_x3, m_lineEdit_x4,
                         m_lineEditPeriodicFreq, m_lineEditPeriodicT0, m_lineEditPeriodicA0,
                         m_lineEditModulatedA0, m_lineEditModulatedA, m_lineEditModulatedT0,
                         m_lineEditModulatedF1, m_lineEditModulatedF2,
                         m_lineEditExpDecayA0, m_lineEditExpDecayAm, m_lineEditExpDecayT0,
                         m_lineEditExpDecayTd })
            connect(ed, &QLineEdit::textChanged, refresh);



        /* 离散按钮 */
        connect(pushButton_5, &QPushButton::clicked, this, &mFunctionPanel::onInsertRow);
        connect(pushButton_6, &QPushButton::clicked, this, &mFunctionPanel::onDeleteRow);
        connect(pushButton_7, &QPushButton::clicked, this, &mFunctionPanel::onMoveUp);
        connect(pushButton_8, &QPushButton::clicked, this, &mFunctionPanel::onMoveDown);
        connect(pushButton_9, &QPushButton::clicked, this, &mFunctionPanel::onTrashRow);

        /* 绘制/导出 统一走刷新槽（内部再区分类型） */
        for (auto* btn : { pushButton, pushButton_4, pushButton_12, pushButton_14 })
            connect(btn, &QPushButton::clicked, this, &mFunctionPanel::onAutoRefresh);

        /* 统一同步数据 */
        auto sync = [this] { syncDataToFuncData(); };
        /* 所有 QLineEdit */
        for (auto* le : { lineEdit, lineEdit_2, lineEdit_3, lineEdit_4, lineEdit_9, lineEdit_10, m_lineEdit_x1, m_lineEdit_x2, m_lineEdit_x3, m_lineEdit_x4,
                           m_lineEditPeriodicFreq, m_lineEditPeriodicT0, m_lineEditPeriodicA0,
                           m_lineEditModulatedA0, m_lineEditModulatedA, m_lineEditModulatedT0,
                           m_lineEditModulatedF1, m_lineEditModulatedF2,
                           m_lineEditExpDecayA0, m_lineEditExpDecayAm, m_lineEditExpDecayT0,
                           m_lineEditExpDecayTd })
            connect(le, &QLineEdit::textEdited, sync);
        /* 所有 QComboBox */
        connect(comboBox_5, QOverload<int>::of(&QComboBox::currentIndexChanged), sync);
        connect(comboBox_5, QOverload<int>::of(&QComboBox::currentIndexChanged), [this] { onAutoRefresh(); });
        connect(comboBox_4, QOverload<int>::of(&QComboBox::currentIndexChanged), sync);
        connect(comboBox_4, QOverload<int>::of(&QComboBox::currentIndexChanged), [this] { onAutoRefresh(); });
        /* 所有插值方式都显示插值数据表格；光滑(5)/阶跃(6) 切换表头并隐藏外推方式 */
        connect(comboBox_5, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
            const bool isSmoothStep = (idx == 5 || idx == 6);
            tableWidget_interp->setHorizontalHeaderLabels(
                isSmoothStep ? QStringList{ "序号", "时间", "幅值" }
                             : QStringList{ "序号", "x", "y" });
            if (isSmoothStep)
            {
                label_13->setParent(nullptr);
                comboBox_4->setParent(nullptr);
            }
            else if (comboBox_4->parent() == nullptr)
            {
                m_extrapFold->addRow(label_13, comboBox_4);
            }
            });
        /* 表格同步数据 */
        connect(tableWidget, &QTableWidget::cellChanged, sync); // 离散
        connect(tableWidget_interp, &QTableWidget::cellChanged, sync); // 插值专用表
        connect(tableWidget_interp, &QTableWidget::cellChanged,this, &mFunctionPanel::onAutoRefresh);//刷新
        connect(tableWidget_3, &QTableWidget::cellChanged, sync); // 分段
        connect(tableWidget_periodic, &QTableWidget::cellChanged, sync); // 周期
        connect(tableWidget_periodic, &QTableWidget::cellChanged, this, &mFunctionPanel::onAutoRefresh); // 周期刷新
        connect(tableWidget_3, &QTableWidget::itemChanged, [=](QTableWidgetItem* item) {
            
            if (!item) return;

            int r = item->row();
            int c = item->column();

            // 只处理 第3列 或 第2列
            if (c != 2 && c != 1) return;

            const QSignalBlocker b(tableWidget_3); // 防止循环信号

            if (c == 2) { // 修改了第n行第3列 → 同步到第n+1行第2列
                if (r + 1 >= tableWidget_3->rowCount()) return;
                auto* dst = tableWidget_3->item(r + 1, 1);
                if (!dst) {
                    dst = new QTableWidgetItem("");
                    tableWidget_3->setItem(r + 1, 1, dst);
                }
                dst->setText(item->text());
            }
            else if (c == 1) { // 修改了第n+1行第2列 → 反向同步到第n行第3列
                if (r == 0) return;
                auto* dst = tableWidget_3->item(r - 1, 2);
                if (!dst) {
                    dst = new QTableWidgetItem("");
                    tableWidget_3->setItem(r - 1, 2, dst);
                }
                dst->setText(item->text());
            }
            }); // 分段

        // 文件：mFunctionPanel.cpp
        /* 导出按钮 */
        connect(pushButton_10, &QPushButton::clicked, this, &mFunctionPanel::onExportAnalytic);
        connect(pushButton_3, &QPushButton::clicked, this, &mFunctionPanel::onExportDiscrete);
        /* 插值导出——用全新的按钮 */
        auto* btnInterpExport = findChild<QPushButton*>("pushButton_exportInterpOnly");
        if (btnInterpExport)
            connect(btnInterpExport, &QPushButton::clicked, this, &mFunctionPanel::onExportInterp);

        connect(pushButton_13, &QPushButton::clicked,this, &mFunctionPanel::onExportPiecewise);
        connect(pushButton_11, &QPushButton::clicked, this, &mFunctionPanel::onExportStep);

        /* 插值页按钮 */
        connect(pushButton_addInterp, &QPushButton::clicked, this, &mFunctionPanel::onInterpAddRow);
        connect(pushButton_delInterp, &QPushButton::clicked, this, &mFunctionPanel::onInterpDelRow);
        connect(pushButton_upInterp, &QPushButton::clicked, this, &mFunctionPanel::onInterpMoveUp);
        connect(pushButton_downInterp, &QPushButton::clicked, this, &mFunctionPanel::onInterpMoveDown);
        connect(pushButton_trashInterp, &QPushButton::clicked, this, &mFunctionPanel::onInterpTrashRow);
        /* 让“绘制曲线”按钮直接触发刷新，复用 doInterp() */
        connect(pushButton_drawInterp, &QPushButton::clicked,this, &mFunctionPanel::onAutoRefresh);
        /* 分段页按钮 */
        connect(pushButton_addPiece, &QPushButton::clicked, this, &mFunctionPanel::onPieceAddRow);
        connect(pushButton_delPiece, &QPushButton::clicked, this, &mFunctionPanel::onPieceDelRow);
        connect(pushButton_upPiece, &QPushButton::clicked, this, &mFunctionPanel::onPieceMoveUp);
        connect(pushButton_downPiece, &QPushButton::clicked, this, &mFunctionPanel::onPieceMoveDown);
        connect(pushButton_trashPiece, &QPushButton::clicked, this, &mFunctionPanel::onPieceTrashRow);

        /* 表格页按钮 */
        connect(pushButton_addTable, &QPushButton::clicked, this, &mFunctionPanel::onTableAddRow);
        connect(pushButton_delTable, &QPushButton::clicked, this, &mFunctionPanel::onTableDelRow);
        connect(pushButton_upTable, &QPushButton::clicked, this, &mFunctionPanel::onTableMoveUp);
        connect(pushButton_downTable, &QPushButton::clicked, this, &mFunctionPanel::onTableMoveDown);
        connect(pushButton_exportTable, &QPushButton::clicked, this, &mFunctionPanel::onTableExport);
        connect(pushButton_drawTable, &QPushButton::clicked, this, &mFunctionPanel::onAutoRefresh);

        /* 周期页按钮 */
        connect(pushButton_addPeriodic, &QPushButton::clicked, this, &mFunctionPanel::onPeriodicAddRow);
        connect(pushButton_delPeriodic, &QPushButton::clicked, this, &mFunctionPanel::onPeriodicDelRow);
        connect(pushButton_upPeriodic, &QPushButton::clicked, this, &mFunctionPanel::onPeriodicMoveUp);
        connect(pushButton_downPeriodic, &QPushButton::clicked, this, &mFunctionPanel::onPeriodicMoveDown);


        /* 表格页 combobox 同步 */
        connect(comboBox_xAxisType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
            sync();
            onAutoRefresh();
            });
        connect(comboBox_yAxisType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
            sync();
            onAutoRefresh();
            });
        connect(comboBox_extrapTable, QOverload<int>::of(&QComboBox::currentIndexChanged), sync);
        connect(m_lineEdit_x1, &QLineEdit::textEdited, sync);
        connect(m_lineEdit_x2, &QLineEdit::textEdited, sync);
        connect(m_lineEdit_x3, &QLineEdit::textEdited, sync);
        connect(m_lineEdit_x4, &QLineEdit::textEdited, sync);
        connect(tableWidget_table, &QTableWidget::cellChanged, sync);
        connect(tableWidget_table, &QTableWidget::cellChanged, this, &mFunctionPanel::onAutoRefresh);
        connect(m_lineEditPeriodicFreq, &QLineEdit::textEdited, sync);
        connect(m_lineEditPeriodicT0, &QLineEdit::textEdited, sync);
        connect(m_lineEditPeriodicA0, &QLineEdit::textEdited, sync);
        connect(tableWidget_periodic, &QTableWidget::cellChanged, sync);
        connect(tableWidget_periodic, &QTableWidget::cellChanged, this, &mFunctionPanel::onAutoRefresh);

        //保存函数面板数据
        connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int type) {
            int functionType = comboBox->currentIndex();
            _funcData->setFunctionType(functionType);
            updatePanel();
            onTypeChanged(type);
            });


    }

    /* ========== 折叠显隐控制（与旧 stackedWidget 索引对齐） ========== */
    void mFunctionPanel::onTypeChanged(int idx)
    {
        m_exprFold->setVisible(false);
        m_discFold->setVisible(false);
        m_interpFold->setVisible(false);
        m_extrapFold->setVisible(false);
        m_interpDataFold->setVisible(false);
        m_pieceFold->setVisible(false);
        m_stepFold->setVisible(false);
        m_tableFold->setVisible(false);
        m_tableDataFold->setVisible(false);
        m_periodicParamFold->setVisible(false);
        m_periodicDataFold->setVisible(false);
        m_modulatedFold->setVisible(false);
        m_expDecayFold->setVisible(false);

        /* 2. 变量折叠体仅解析、分段需要 */
        m_varFold->setVisible(idx == 0 || idx == 3);

        /* 3. 变量范围折叠体也仅解析、分段需要 */
        m_rangeFold->setVisible(idx == 0 || idx == 4);

        //默认隐藏外推方式
        m_labExtrapTable->setParent(nullptr);
        comboBox_extrapTable->setParent(nullptr);

        switch (idx) {
        case 0: m_exprFold->setVisible(true);
            m_interpDataFold->setVisible(false);
            m_statFold->setVisible(false);
            break;
        case 1: m_discFold->setVisible(true);
            m_interpDataFold->setVisible(false);
            m_statFold->setVisible(false);
            break;
        case 2:
        {
            m_extrapFold->setVisible(true);    
            m_interpDataFold->setVisible(true);
            const bool isSmoothStep = (comboBox_5->currentIndex() == 5 || comboBox_5->currentIndex() == 6);
            // 光滑/阶跃：表头 序号/时间/幅值；其余：序号/x/y
            tableWidget_interp->setHorizontalHeaderLabels(
                isSmoothStep ? QStringList{ "序号", "时间", "幅值" }
                             : QStringList{ "序号", "x", "y" });
            // 光滑(5)/阶跃(6) 不显示外推方式，其余插值方式显示
            if (isSmoothStep)
            {
                label_13->setParent(nullptr);
                comboBox_4->setParent(nullptr);
            }
            else if (comboBox_4->parent() == nullptr)
            {
                m_extrapFold->addRow(label_13, comboBox_4);
            }
            m_statFold->setVisible(false);
            break;
        }
        case 3: m_pieceFold->setVisible(true);
            m_interpDataFold->setVisible(false);
            m_statFold->setVisible(false);
            break;
        case 4: m_stepFold->setVisible(true);
            m_rangeFold->setVisible(false);   // 加这一行
            m_interpDataFold->setVisible(false);
            m_statFold->setVisible(false);
            break;
        case 5:
            m_tableFold->setVisible(true);
            m_tableDataFold->setVisible(true);
            m_statFold->setVisible(false);
            // 普通表格：显示 x轴/y轴/外推方式，隐藏 x1
            if (m_labXAxisType->parent() == nullptr) m_tableFold->addRow(m_labXAxisType, comboBox_xAxisType);
            if (m_labYAxisType->parent() == nullptr) m_tableFold->addRow(m_labYAxisType, comboBox_yAxisType);
            if (m_labExtrapTable->parent() == nullptr) m_tableFold->addRow(m_labExtrapTable, comboBox_extrapTable);
            m_labX1->setParent(nullptr);
            m_lineEdit_x1->setParent(nullptr);
            m_labX2->setParent(nullptr);
            m_lineEdit_x2->setParent(nullptr);
            m_labX3->setParent(nullptr);
            m_lineEdit_x3->setParent(nullptr);
            m_labX4->setParent(nullptr);
            m_lineEdit_x4->setParent(nullptr);
            // 恢复3列表格
            tableWidget_table->setColumnCount(3);
            tableWidget_table->setHorizontalHeaderLabels({ "序号", "x", "y" });
            break;
        case 6:
            m_tableFold->setVisible(true);
            m_tableDataFold->setVisible(true);
            m_statFold->setVisible(false);
            // 可平移表格：隐藏 x轴/y轴/x2，显示 x1/外推方式
            m_labXAxisType->setParent(nullptr);
            comboBox_xAxisType->setParent(nullptr);
            m_labYAxisType->setParent(nullptr);
            comboBox_yAxisType->setParent(nullptr);
            m_labX2->setParent(nullptr);
            m_lineEdit_x2->setParent(nullptr);
            if (m_labX1->parent() == nullptr) m_tableFold->addRow(m_labX1, m_lineEdit_x1);
            if (m_labExtrapTable->parent() == nullptr) m_tableFold->addRow(m_labExtrapTable, comboBox_extrapTable);
            // 恢复3列表格
            tableWidget_table->setColumnCount(3);
            tableWidget_table->setHorizontalHeaderLabels({ "序号", "x", "y" });
            break;
        case 7:
            m_tableFold->setVisible(true);
            m_tableDataFold->setVisible(true);
            m_statFold->setVisible(false);
            // 可缩放表格：隐藏 x轴/y轴，显示 x1/x2/外推方式
            m_labXAxisType->setParent(nullptr);
            comboBox_xAxisType->setParent(nullptr);
            m_labYAxisType->setParent(nullptr);
            comboBox_yAxisType->setParent(nullptr);
            if (m_labX1->parent() == nullptr) m_tableFold->addRow(m_labX1, m_lineEdit_x1);
            if (m_labX2->parent() == nullptr) m_tableFold->addRow(m_labX2, m_lineEdit_x2);
            if (m_labExtrapTable->parent() == nullptr) m_tableFold->addRow(m_labExtrapTable, comboBox_extrapTable);
            m_labX3->setParent(nullptr);
            m_lineEdit_x3->setParent(nullptr);
            m_labX4->setParent(nullptr);
            m_lineEdit_x4->setParent(nullptr);
            // 恢复3列表格
            tableWidget_table->setColumnCount(3);
            tableWidget_table->setHorizontalHeaderLabels({ "序号", "x", "y" });
            break;
        case 8:
            m_tableFold->setVisible(true);
            m_tableDataFold->setVisible(true);
            m_statFold->setVisible(false);
            // 幂函数：隐藏 x轴/y轴/外推方式，显示 x1/x2/x3/x4
            m_labXAxisType->setParent(nullptr);
            comboBox_xAxisType->setParent(nullptr);
            m_labYAxisType->setParent(nullptr);
            comboBox_yAxisType->setParent(nullptr);
            if (m_labX1->parent() == nullptr) m_tableFold->addRow(m_labX1, m_lineEdit_x1);
            if (m_labX2->parent() == nullptr) m_tableFold->addRow(m_labX2, m_lineEdit_x2);
            if (m_labX3->parent() == nullptr) m_tableFold->addRow(m_labX3, m_lineEdit_x3);
            if (m_labX4->parent() == nullptr) m_tableFold->addRow(m_labX4, m_lineEdit_x4);
            // 幂函数表格改为2列
            tableWidget_table->setColumnCount(2);
            tableWidget_table->setHorizontalHeaderLabels({ "序号", "A" });
            break;
        case 9:
            m_periodicParamFold->setVisible(true);
            m_periodicDataFold->setVisible(true);
            m_statFold->setVisible(false);
            break;
        case 10:
            m_modulatedFold->setVisible(true);
            m_statFold->setVisible(false);
            break;
        case 11:
            m_expDecayFold->setVisible(true);
            m_statFold->setVisible(false);
            break;
        }
        if (m_tableFold) {
            QTimer::singleShot(0, m_tableFold, [this]() { m_tableFold->refreshLayout(); });
        }
        onAutoRefresh(); // 立即刷新一次
    }

    /* ========== 防抖刷新入口：根据当前类型调用对应实现 ========== */
    void mFunctionPanel::onAutoRefresh()
    {
        if (!comboBox) return;
        int t = comboBox->currentIndex();
        switch (t) {
        case 0: doAnalytic();  break;
        case 1: doDiscrete();  break;
        case 2: doInterp();    break;
        case 3: doPiecewise(); break;
        case 4: doStep();      break;
        case 5: doTable();     break;
        case 6: doTable();     break;
        case 7: doTable();     break;
        case 8: doTable();     break;
        case 9: doPeriodic();   break;
        case 10: doModulated(); break;
        case 11: doExpDecay();  break;
        }
    }

    /* ========== 五种函数实现（完整搬迁自你旧代码） ========== */

    /*================== 解析函数 ==================*/
    void mFunctionPanel::doAnalytic()
    {
        bool okX0 = false, okX1 = false;
        double x0 = lineEdit->text().toDouble(&okX0);
        double x1 = lineEdit_2->text().toDouble(&okX1);
      

        QString expr = lineEdit_3->text().trimmed();
        try {
            lineEdit_3->setStyleSheet("color:black");
            mu::Parser p;
            double curX = 0.0;
            p.DefineVar("x", &curX);
            p.SetExpr(expr.toStdString());
            p.Eval();
   
        }
        catch (mu::Parser::exception_type& e)
        {
            lineEdit_3->setStyleSheet("color:red");
            qDebug() << "Parser error:" << e.GetMsg().c_str();
        }
        if (!okX0 || !okX1 || x0 >= x1) return;
        if (expr.isEmpty()) return;

        /* 取解析页采样点数 */
        QSpinBox* spin = findChild<QSpinBox*>("spinSamplesAna");
        int N = spin ? spin->value() : 10;
        QVector<double> x(N), y(N);
        double dx = (x1 - x0) / (N - 1);

        try {
            mu::Parser p;
            double curX = 0.0;
            p.DefineVar("x", &curX);
            p.SetExpr(expr.toStdString());
            for (int i = 0; i < N; ++i) {
                curX = x0 + i * dx;
                x[i] = curX;
                y[i] = p.Eval();
            }
            // 立即刷新曲线窗口（与你旧代码一致）
            mCurveWindow::getInstance()->updateCurve(x, y, PlotView::LineChart, "解析数据");
            mCurveWindow::getInstance()->setAxisAuto(AxisId::X_Bottom, true);
            mCurveWindow::getInstance()->setAxisAuto(AxisId::Y_Left, true);
        }
        catch (mu::Parser::exception_type& e) {
            qDebug() << "Parser error:" << e.GetMsg().c_str();
        }

        ///* 同时写入数据对象（供外部保存/复用） */
        //auto& ac = _funcData->analytic();
        //ac.xMin = x0;
        //ac.xMax = x1;
        //ac.expr = expr;
        //ac.simple = N;
        //qDebug() << "解析区间" << ac.xMin << ac.xMax << "表达式" << ac.expr;
    }

    /*================== 离散数据 ==================*/
    void mFunctionPanel::doDiscrete()
    {
        int rows = tableWidget->rowCount();
        QVector<double> x, y;

        for (int r = 0; r < rows; ++r) {
            auto* itX = tableWidget->item(r, 1);   // 第 1 列：x
            auto* itY = tableWidget->item(r, 2);   // 第 2 列：y
            if (!itX || !itY) continue;

            bool okX, okY;
            double xv = itX->text().toDouble(&okX);
            double yv = itY->text().toDouble(&okY);
            if (okX && okY) { x.push_back(xv); y.push_back(yv); }
        }
        if (x.size() < 2) return;          // 至少两个点

        /* 立即刷新曲线窗口（与原代码一致） */
        mCurveWindow::getInstance()->updateCurve(x, y, PlotView::LineChart, "离散数据", true);
        mCurveWindow::getInstance()->setAxisAuto(AxisId::X_Bottom, true);
        mCurveWindow::getInstance()->setAxisAuto(AxisId::Y_Left, true);

        ///* ===== 写回 BasicData：三列一一对应 ===== */
        //auto& d = _funcData->discrete();
        //d.idx.clear(); d.x.clear(); d.y.clear();
        //for (int i = 0; i < x.size(); ++i) {
        //    d.idx.push_back(i + 1);
        //    d.x.push_back(x[i]);
        //    d.y.push_back(y[i]);
        //}
    }

    /*================== 插值函数 ==================*/
    void mFunctionPanel::doInterp()
    {
        // ========== 1. 读取表格中的点 ==========
        int rows = tableWidget_interp->rowCount();
        QVector<double> px, py;
        for (int r = 0; r < rows; ++r)
        {
            auto* itX = ensureItem(tableWidget_interp, r, 1);
            auto* itY = ensureItem(tableWidget_interp, r, 2);
            bool okX, okY;
            double xv = itX->text().toDouble(&okX);
            double yv = itY->text().toDouble(&okY);
            if (okX && okY) { px.append(xv); py.append(yv); }
        }
        if (px.size() < 2) return;

        // ========== 2. 按 x 升序排列 ==========
        QVector<int> idx(px.size());
        for (int i = 0; i < idx.size(); ++i) idx[i] = i;
        std::sort(idx.begin(), idx.end(), [&](int a, int b) { return px[a] < px[b]; });
        QVector<double> sx, sy;
        for (int i = 0; i < idx.size(); ++i)
        {
            sx.append(px[idx[i]]);
            sy.append(py[idx[i]]);
        }

        // ========== 3. 获取算法选项 ==========
        int alg = comboBox_5->currentIndex();   // 0-最近邻 1-线性 2-分段三次 3-三次样条 4-对数 6-阶跃
        int extrap = comboBox_4->currentIndex(); //0-常数 1-线性

        // ========== 4. 若需要样条，先构造（要求至少3个点） ==========
        std::unique_ptr<CubicSpline> spline;
        if (alg == 2 || alg == 3 || alg == 5)
        {
            if (sx.size() >= 3)
                spline = std::make_unique<CubicSpline>(sx, sy);
            else
                alg = 1;   // 点数不足，降级为线性
        }

        // ========== 5. 准备画图数据（仅限原始数据范围） ==========
        QVector<double> plotX, plotY;

        // 添加第一个原始点
        plotX.append(sx.first());
        // 计算该点的 y 值（直接使用原始值即可，但因后续可能有阶跃/样条等，统一用插值计算）
        // 第一个点必然在范围内，直接用插值公式（会落在第一段）
        {
            double xi = sx.first();
            // 找段：这里一定 k=0
            int k = 0;
            while (k < sx.size() - 2 && sx[k + 1] < xi) ++k;
            // 根据 alg 计算 y
            if (alg == 0)
                plotY.append((xi - sx[k] < sx[k + 1] - xi) ? sy[k] : sy[k + 1]);
            else if (alg == 1)
            {
                double t = (xi - sx[k]) / (sx[k + 1] - sx[k]);
                plotY.append(sy[k] + t * (sy[k + 1] - sy[k]));
            }
            else if (alg == 4)
            {
                if (sx[k] > 0 && sx[k + 1] > 0 && xi > 0)
                {
                    double t = (log(xi) - log(sx[k])) / (log(sx[k + 1]) - log(sx[k]));
                    plotY.append(sy[k] + t * (sy[k + 1] - sy[k]));
                }
                else
                {
                    double t = (xi - sx[k]) / (sx[k + 1] - sx[k]);
                    plotY.append(sy[k] + t * (sy[k + 1] - sy[k]));
                }
            }
            else if (alg == 6)
                plotY.append(sy[k]);
            else if (spline)
                plotY.append(spline->eval(xi));
            else
            {
                double t = (xi - sx[k]) / (sx[k + 1] - sx[k]);
                plotY.append(sy[k] + t * (sy[k + 1] - sy[k]));
            }
        }

        // ========== 6. 逐段生成中间插值点 ==========
        for (int i = 0; i < sx.size() - 1; ++i)
        {
            double xL = sx[i];
            double xR = sx[i + 1];

            // 自动决定这一段画多少个内部点（不包括左右端点）
            double segLen = xR - xL;
            int nPts = qMax(10, qMin(200, static_cast<int>(segLen * 0.5)));
            // 如果区间极短（xL == xR），nPts = 0，直接跳过
            if (segLen <= 0.0) continue;

            for (int j = 1; j <= nPts; ++j)
            {
                double xi = xL + segLen * j / (nPts + 1);   // 均匀分布在 (xL, xR) 内

                // 找到包含 xi 的区间 k
                int k = 0;
                while (k < sx.size() - 2 && sx[k + 1] < xi) ++k;

                // 根据 alg 计算 yi
                double yi = 0.0;
                if (alg == 0)
                {
                    yi = (xi - sx[k] < sx[k + 1] - xi) ? sy[k] : sy[k + 1];
                }
                else if (alg == 1)
                {
                    double t = (xi - sx[k]) / (sx[k + 1] - sx[k]);
                    yi = sy[k] + t * (sy[k + 1] - sy[k]);
                }
                else if (alg == 4)
                {
                    if (sx[k] > 0 && sx[k + 1] > 0 && xi > 0)
                    {
                        double t = (log(xi) - log(sx[k])) / (log(sx[k + 1]) - log(sx[k]));
                        yi = sy[k] + t * (sy[k + 1] - sy[k]);
                    }
                    else
                    {
                        double t = (xi - sx[k]) / (sx[k + 1] - sx[k]);
                        yi = sy[k] + t * (sy[k + 1] - sy[k]);
                    }
                }
                else if (alg == 6)
                {
                    yi = sy[k];
                }
                else if (spline)
                {
                    yi = spline->eval(xi);
                }
                else
                {
                    // 未知模式回退线性
                    double t = (xi - sx[k]) / (sx[k + 1] - sx[k]);
                    yi = sy[k] + t * (sy[k + 1] - sy[k]);
                }

                plotX.append(xi);
                plotY.append(yi);
            }

            // 在当前段结束后，添加右端点（注意最后一个段的右端点会在循环结束后统一添加，避免重复）
            if (i == sx.size() - 2)   // 最后一段
            {
                plotX.append(sx.last());
                // 计算最后一点的 y（用同样方式）
                double xi = sx.last();
                int k = 0;
                while (k < sx.size() - 2 && sx[k + 1] < xi) ++k;
                if (alg == 0)
                    plotY.append((xi - sx[k] < sx[k + 1] - xi) ? sy[k] : sy[k + 1]);
                else if (alg == 1)
                {
                    double t = (xi - sx[k]) / (sx[k + 1] - sx[k]);
                    plotY.append(sy[k] + t * (sy[k + 1] - sy[k]));
                }
                else if (alg == 4)
                {
                    if (sx[k] > 0 && sx[k + 1] > 0 && xi > 0)
                    {
                        double t = (log(xi) - log(sx[k])) / (log(sx[k + 1]) - log(sx[k]));
                        plotY.append(sy[k] + t * (sy[k + 1] - sy[k]));
                    }
                    else
                    {
                        double t = (xi - sx[k]) / (sx[k + 1] - sx[k]);
                        plotY.append(sy[k] + t * (sy[k + 1] - sy[k]));
                    }
                }
                else if (alg == 6)
                    plotY.append(sy[k]);
                else if (spline)
                    plotY.append(spline->eval(xi));
                else
                {
                    double t = (xi - sx[k]) / (sx[k + 1] - sx[k]);
                    plotY.append(sy[k] + t * (sy[k + 1] - sy[k]));
                }
            }
        }
        /* ---------- 7. 绘图 ---------- */
        mCurveWindow::getInstance()->updateCurve(plotX, plotY, PlotView::LineChart, "插值曲线");
        mCurveWindow::getInstance()->setAxisAuto(AxisId::X_Bottom, true);
        mCurveWindow::getInstance()->setAxisAuto(AxisId::Y_Left, true);

        ///* ---------- 8. 写回 BasicData ---------- */
        auto& ic = _funcData->interp();
        ic.x.clear(); ic.y.clear();
        for (int r = 0; r < sx.size(); ++r)
        {
            ic.x.push_back(sx[r]);
            ic.y.push_back(sy[r]);
        }
        ic.interpMode = alg;
        ic.extrapMode = extrap;
    }

    /*================== 分段函数 ==================*/
    void mFunctionPanel::doPiecewise()
    {
        int rows = tableWidget_3->rowCount();
        QVector<QVector<double>> xData;      // 多段 x
        QVector<QVector<double>> yData;      // 多段 y
        QStringList titles;

        for (int r = 0; r < rows; ++r)
        {
            /* 保证 4 列都有 item，空就当 0 */
            auto* itS = ensureItem(tableWidget_3, r, 1);   // 起始
            auto* itE = ensureItem(tableWidget_3, r, 2);   // 结束
            auto* itX = ensureItem(tableWidget_3, r, 3);   // 表达式
            QTableWidgetItem* itSamples = ensureItem(tableWidget_3, r, 4);//采样点数

            bool okS, okE;
            double s = itS->text().toDouble(&okS);
            double e = itE->text().toDouble(&okE);
            QString expr = itX->text().trimmed();
           
            if (!okS || !okE || expr.isEmpty() || s >= e) continue;

           
            int samples = itSamples->text().toInt();
            const int N = samples;
            QVector<double> x(N), y(N);
            double dx = (e - s) / (N - 1);

            try {
                mu::Parser p;
                double curX = 0.0;
                p.DefineVar("x", &curX);
                p.SetExpr(expr.toStdString());
                for (int i = 0; i < N; ++i) {
                    curX = s + i * dx;
                    x[i] = curX;
                    y[i] = p.Eval();
                }
                xData.push_back(x);
                yData.push_back(y);
                titles << QString("段%1").arg(r + 1);
            }
            catch (mu::Parser::exception_type& e) {
                qDebug() << "Piecewise parser error:" << e.GetMsg().c_str();
            }
        }

        if (xData.isEmpty()) return;

        /* 多段一次性绘制 */
        mCurveWindow::getInstance()->updateMultiCurves(xData, yData,
            PlotView::LineChart,
            titles,
            true,   // 清空旧曲线
            true);  // 显示图例
        mCurveWindow::getInstance()->setAxisAuto(AxisId::X_Bottom, true);
        mCurveWindow::getInstance()->setAxisAuto(AxisId::Y_Left, true);

        ///* ===== 写回 BasicData ===== */
        //auto& pc = _funcData->piecewise();
        //pc.idx.clear(); pc.start.clear(); pc.end.clear(); pc.expr.clear();
        //for (int r = 0; r < rows; ++r) {
        //    auto* itS = ensureItem(tableWidget_3, r, 1);
        //    auto* itE = ensureItem(tableWidget_3, r, 2);
        //    auto* itX = ensureItem(tableWidget_3, r, 3);
        //    pc.idx.push_back(r + 1);
        //    pc.start.push_back(itS->text().toDouble());
        //    pc.end.push_back(itE->text().toDouble());
        //    pc.expr.push_back(itX->text());
        //}
        ///* 写回 BasicData 时，统一用第一行的采样点数当代表值（或你自己定规则） */
        //QTableWidgetItem* itSamples0 = ensureItem(tableWidget_3, 0, 4);   // 取第 0 行
        //pc.sampleCount = itSamples0->text().toInt();
    }
    //阶跃
    void mFunctionPanel::doStep()
    {
        bool okPos, okV0, okV1, okX0=true, okX1=true;
        double pos = lineEdit_4->text().toDouble(&okPos);
        double v0 = lineEdit_9->text().toDouble(&okV0);
        double v1 = lineEdit_10->text().toDouble(&okV1);
        double x0 =pos-10;   // 变量范围下限
        double x1 = pos+10; // 变量范围上限

        if (!okPos || !okV0 || !okV1 || !okX0 || !okX1 || x0 >= x1) return;

        /* ---- 保证阶跃位置在区间内部 ---- */
        if (pos <= x0) pos = x0 + (x1 - x0) * 0.001;   // 太小就稍微挪一点
        if (pos >= x1) pos = x1 - (x1 - x0) * 0.001;

        QSpinBox* spin = findChild<QSpinBox*>("spinSamplesStep");
     /*   int N = spin ? spin->value() : 100;*/
        int N = 2;

        /* 左侧段 [x0, pos] 恒为 v0 */
        QVector<double> xLeft(N), yLeft(N);
     /*   double dxLeft = (pos - x0) / (N - 1);
        for (int i = 0; i < N; ++i) {
            xLeft[i] = x0 + i * dxLeft;
            yLeft[i] = v0;
        }*/
        xLeft[0] = x0; yLeft[0] = v0;
        xLeft[1] = pos; yLeft[1] = v0;

        /* 右侧段 [pos, x1] 恒为 v1 */
        QVector<double> xRight(N), yRight(N);
      /*  double dxRight = (x1 - pos) / (N - 1);
        for (int i = 0; i < N; ++i) {
            xRight[i] = pos + i * dxRight;
            yRight[i] = v1;
        }*/
        xRight[0] = pos; yRight[0] = v1;
        xRight[1] = x1; yRight[1] = v1;
        /* 多段绘制 */
        QVector<QVector<double>> xData = { xLeft, xRight };
        QVector<QVector<double>> yData = { yLeft, yRight };
        QVector<double> xLR;
        xLR.append(xLeft);
        xLR.append(xRight);
        QVector<double> yLR;
        yLR.append(yLeft);
        yLR.append(yRight);
        mCurveWindow::getInstance()->updateCurve(
            xLR, yLR,
            PlotView::LineChart,
            /*QStringList() << "阶跃左侧" << "阶跃右侧",*/
            "阶跃",
            true,   // 清空旧曲线
            true);  // 显示图例
        mCurveWindow::getInstance()->setAxisAuto(AxisId::X_Bottom, true);
        mCurveWindow::getInstance()->setAxisAuto(AxisId::Y_Left, true);
        ///* 写回 BasicData */
        //auto& sc = _funcData->step();
        //sc.xMin = x0; sc.xMax = x1;
        //sc.pos = pos;
        //sc.v0 = v0; sc.v1 = v1;
    }

    /*================== 幂函数 ==================*/
    void mFunctionPanel::doPowerTable()
    {
        /* ---------- 1. 读系数（幂指数 = 行索引） ---------- */
        int rows = tableWidget_table->rowCount();
        QVector<double> coeff;
        for (int r = 0; r < rows; ++r)
        {
            auto* itA = ensureItem(tableWidget_table, r, 1); // 系数 Ai
            bool okA;
            double ai = itA->text().toDouble(&okA);
            if (okA) { coeff.append(ai); }
        }
        if (coeff.size() < 1) return;

        /* ---------- 2. 读参数 ---------- */
        double x1 = m_lineEdit_x1->text().toDouble();
        double x2 = m_lineEdit_x2->text().toDouble();
        double x3 = m_lineEdit_x3->text().toDouble();
        double x4 = m_lineEdit_x4->text().toDouble();
        if (x2 == 0) x2 = 1.0; // 防止除零
        if (x4 <= x3) return;

        /* ---------- 3. 采样 ---------- */
        int N = 10;
        QVector<double> x(N), y(N);
        double dx = (x4 - x3) / (N - 1);

        /* ---------- 4. 计算 y = Σ Ai * ((x - x1) / x2)^i ---------- */
        for (int i = 0; i < N; ++i)
        {
            double xi = x3 + i * dx;
            double yi = 0.0;
            double t = (xi - x1) / x2;
            for (int j = 0; j < coeff.size(); ++j)
            {
                yi += coeff[j] * std::pow(t, j); // j 为幂指数
            }
            x[i] = xi;
            y[i] = yi;
        }

        /* ---------- 5. 绘图 ---------- */
        mCurveWindow::getInstance()->updateCurve(x, y, PlotView::LineChart, "幂函数曲线");
        mCurveWindow::getInstance()->setAxisAuto(AxisId::X_Bottom, true);
        mCurveWindow::getInstance()->setAxisAuto(AxisId::Y_Left, true);
    }

    /*================== 表格函数 ==================*/
    void mFunctionPanel::doTable()
    {
        if (comboBox->currentIndex() == 8) {
            doPowerTable();
            return;
        }
        /* ---------- 1. 读点 ---------- */
        int rows = tableWidget_table->rowCount();
        QVector<double> px, py;
        for (int r = 0; r < rows; ++r)
        {
            auto* itX = ensureItem(tableWidget_table, r, 1);
            auto* itY = ensureItem(tableWidget_table, r, 2);
            bool okX, okY;
            double xv = itX->text().toDouble(&okX);
            double yv = itY->text().toDouble(&okY);
            if (okX && okY) { px.append(xv); py.append(yv); }
        }
        if (px.size() < 2) return;

        /* ---------- 2. 按 x 排序 ---------- */
        QVector<int> idx(px.size());
        for (int i = 0; i < idx.size(); ++i) idx[i] = i;
        std::sort(idx.begin(), idx.end(), [&](int a, int b)
        { return px[a] < px[b]; });
        QVector<double> sx, sy;
        for (int i = 0; i < idx.size(); ++i)
        {
            sx.append(px[idx[i]]);
            sy.append(py[idx[i]]);
        }

        /* ---------- 3. 采样设置 ---------- */
        int N = 10;
        int extrap = comboBox_extrapTable->currentIndex(); // 0-常数 1-线性

        double x0 = sx.first(), x1 = sx.last();
        QVector<double> x(N), y(N);
        double dx = (x1 - x0) / (N - 1);

        /* ---------- 4. 计算 ---------- */
        for (int i = 0; i < N; ++i)
        {
            double xi = x0 + i * dx;

            if (extrap == 0) // 常数外推
            {
                if (xi < sx.front()) { y[i] = sy.front(); x[i] = xi; continue; }
                if (xi > sx.back()) { y[i] = sy.back(); x[i] = xi; continue; }
            }
            else if (extrap == 1) // 线性外推
            {
                if (xi < sx.front())
                {
                    double t = (xi - sx.front()) / (sx[1] - sx[0]);
                    y[i] = sy.front() + t * (sy[1] - sy.front());
                    x[i] = xi;
                    continue;
                }
                if (xi > sx.back())
                {
                    double t = (xi - sx[sx.size() - 2]) / (sx.back() - sx[sx.size() - 2]);
                    y[i] = sy[sy.size() - 2] + t * (sy.back() - sy[sy.size() - 2]);
                    x[i] = xi;
                    continue;
                }
            }

            int k = 0;
            while (k < sx.size() - 2 && sx[k + 1] < xi) ++k;

            double t = (xi - sx[k]) / (sx[k + 1] - sx[k]);
            y[i] = sy[k] + t * (sy[k + 1] - sy[k]);
            x[i] = xi;
        }

        /* ---------- 5. 应用偏移量与缩放（可平移/可缩放表格） ---------- */
        if (comboBox->currentIndex() == 6) {
            double x1 = m_lineEdit_x1->text().toDouble();
            for (int i = 0; i < N; ++i)
                x[i] += x1;
        }
        else if (comboBox->currentIndex() == 7) {
            double x1 = m_lineEdit_x1->text().toDouble();
            double x2 = m_lineEdit_x2->text().toDouble();
            for (int i = 0; i < N; ++i)
                x[i] = x[i] * x2 + x1;
        }

        /* ---------- 6. 绘图 ---------- */
        mCurveWindow::getInstance()->updateCurve(x, y, PlotView::LineChart, "表格曲线");

        int xAxisType = comboBox_xAxisType->currentIndex();
        int yAxisType = comboBox_yAxisType->currentIndex();
        auto* curveWin = mCurveWindow::getInstance();
        curveWin->setAxisLog(X_Bottom, xAxisType == 1);
        curveWin->setAxisLog(Y_Left, yAxisType == 1);

        mCurveWindow::getInstance()->setAxisAuto(AxisId::X_Bottom, true);
        mCurveWindow::getInstance()->setAxisAuto(AxisId::Y_Left, true);
    }

    /*================== 周期函数 ==================*/
    void mFunctionPanel::doPeriodic()
    {
        /* ---------- 1. 读参数 ---------- */
        bool okFreq, okT0, okA0;
        double freq = m_lineEditPeriodicFreq->text().toDouble(&okFreq);
        double t0 = m_lineEditPeriodicT0->text().toDouble(&okT0);
        double A0 = m_lineEditPeriodicA0->text().toDouble(&okA0);
        if (!okFreq || !okT0 || !okA0) return;
        if (freq == 0) return;

        /* ---------- 2. 读谐波系数 ---------- */
        int rows = tableWidget_periodic->rowCount();
        QVector<double> A, B;
        for (int r = 0; r < rows; ++r)
        {
            auto* itA = ensureItem(tableWidget_periodic, r, 1);
            auto* itB = ensureItem(tableWidget_periodic, r, 2);
            bool okA, okB;
            double av = itA->text().toDouble(&okA);
            double bv = itB->text().toDouble(&okB);
            if (okA && okB) {
                A.append(av);
                B.append(bv);
            }
        }
        if (A.isEmpty()) return;

        /* ---------- 3. 计算显示范围（至少一个周期） ---------- */
        int order = A.size();
        const double PI = acos(-1.0);
        double omega = 2.0 * PI * freq;
        double period = 1.0 / qAbs(freq);
        double tStart = t0;
        double tEnd = t0 + qMax(period, order * 0.1 * period); // 显示 1~max(order,1) 个周期
        int N = 200;
        QVector<double> x(N), y(N);
        double dt = (tEnd - tStart) / (N - 1);

        /* ---------- 4. 计算 y = A0 + Σ[An·cos(n·ω·(t-t0)) + Bn·sin(n·ω·(t-t0))] ---------- */
        for (int i = 0; i < N; ++i)
        {
            double t = tStart + i * dt;
            double yi = A0;
            for (int n = 0; n < order; ++n)
            {
                double phase = (n + 1) * omega * (t - t0);
                yi += A[n] * cos(phase) + B[n] * sin(phase);
            }
            x[i] = t;
            y[i] = yi;
        }

        /* ---------- 5. 绘图 ---------- */
        mCurveWindow::getInstance()->updateCurve(x, y, PlotView::LineChart, "周期函数曲线");
        mCurveWindow::getInstance()->setAxisAuto(AxisId::X_Bottom, true);
        mCurveWindow::getInstance()->setAxisAuto(AxisId::Y_Left, true);
    }

    /*================== 调制函数 ==================*/
    void mFunctionPanel::doModulated()
    {
        /* ---------- 1. 读参数 ---------- */
        bool okA0, okA, okT0, okF1, okF2;
        double A0 = m_lineEditModulatedA0->text().toDouble(&okA0);
        double A  = m_lineEditModulatedA->text().toDouble(&okA);
        double t0 = m_lineEditModulatedT0->text().toDouble(&okT0);
        double f1 = m_lineEditModulatedF1->text().toDouble(&okF1);
        double f2 = m_lineEditModulatedF2->text().toDouble(&okF2);
        if (!okA0 || !okA || !okT0 || !okF1 || !okF2) return;

        /* ---------- 2. 计算显示范围 ---------- */
        const double PI = acos(-1.0);
        double w1 = 2.0 * PI * f1;
        double w2 = 2.0 * PI * f2;
        /* 显示区间：取两个频率中较小者的 4 个周期，保证波形完整 */
        double fMin = qMin(qAbs(f1), qAbs(f2));
        if (fMin <= 0.0) fMin = qMax(qAbs(f1), qAbs(f2));
        if (fMin <= 0.0) fMin = 1.0;
        double tStart = 0.0;
        double tEnd = t0 + 4.0 / fMin;
        if (tEnd <= tStart) tEnd = tStart + 1.0;
        int N = 500;
        QVector<double> x(N), y(N);
        double dt = (tEnd - tStart) / (N - 1);

        /* ---------- 3. 计算 y = A0 + A*sin(w1*(t-t0))*sin(w2*(t-t0)) (t>t0) ---------- */
        for (int i = 0; i < N; ++i)
        {
            double t = tStart + i * dt;
            double yi = A0;
            if (t > t0)
                yi += A * sin(w1 * (t - t0)) * sin(w2 * (t - t0));
            x[i] = t;
            y[i] = yi;
        }

        /* ---------- 4. 绘图 ---------- */
        mCurveWindow::getInstance()->updateCurve(x, y, PlotView::LineChart, "调制函数曲线");
        mCurveWindow::getInstance()->setAxisAuto(AxisId::X_Bottom, true);
        mCurveWindow::getInstance()->setAxisAuto(AxisId::Y_Left, true);
    }

    /*================== 指数衰减函数 ==================*/
    void mFunctionPanel::doExpDecay()
    {
        /* ---------- 1. 读参数 ---------- */
        bool okA0, okAm, okT0, okTd;
        double A0 = m_lineEditExpDecayA0->text().toDouble(&okA0);
        double Am = m_lineEditExpDecayAm->text().toDouble(&okAm);
        double t0 = m_lineEditExpDecayT0->text().toDouble(&okT0);
        double td = m_lineEditExpDecayTd->text().toDouble(&okTd);
        if (!okA0 || !okAm || !okT0 || !okTd) return;
        if (td == 0.0) return;

        /* ---------- 2. 计算显示范围 ---------- */
        double tStart = 0.0;
        double tEnd = t0 + 5.0 * qAbs(td); // 显示 5 个衰减时间
        if (tEnd <= tStart) tEnd = tStart + 1.0;
        int N = 500;
        QVector<double> x(N), y(N);
        double dt = (tEnd - tStart) / (N - 1);

        /* ---------- 3. 计算 y = A0 + Am*exp(-(t-t0)/td) (t>=t0) ---------- */
        for (int i = 0; i < N; ++i)
        {
            double t = tStart + i * dt;
            double yi = A0;
            if (t >= t0)
                yi += Am * exp(-(t - t0) / td);
            x[i] = t;
            y[i] = yi;
        }

        /* ---------- 4. 绘图 ---------- */
        mCurveWindow::getInstance()->updateCurve(x, y, PlotView::LineChart, "指数衰减函数曲线");
        mCurveWindow::getInstance()->setAxisAuto(AxisId::X_Bottom, true);
        mCurveWindow::getInstance()->setAxisAuto(AxisId::Y_Left, true);
    }

    /*================== 表格行操作（与旧逻辑 100% 一致） ==================*/
    void mFunctionPanel::onInsertRow()
    {
        int row = tableWidget->currentRow();
        if (row < 0) row = tableWidget->rowCount() - 1;
        row++;                                    // 插在选中行之后
        tableWidget->insertRow(row);

        // 给新行填默认值
        auto* idx = new QTableWidgetItem(MxFunctions::mNumberToString(row + 1));
        idx->setFlags(idx->flags() & ~Qt::ItemIsEditable);
        tableWidget->setItem(row, 0, idx);
        tableWidget->setItem(row, 1, new QTableWidgetItem("0"));
        tableWidget->setItem(row, 2, new QTableWidgetItem("0"));

        // 重新编号整张表（关键）
        for (int i = 0; i < tableWidget->rowCount(); ++i)
            tableWidget->item(i, 0)->setText(MxFunctions::mNumberToString(i + 1));

        tableWidget->setCurrentCell(row, 1);
        onAutoRefresh();
    }

    void mFunctionPanel::onDeleteRow()
    {
        int row = tableWidget->currentRow();
        if (row < 0) {
            QMessageBox::information(this, "提示", "请先选中要删除的行！");
            return;
        }
        tableWidget->removeRow(row);
        // 重排序号
        for (int i = 0; i < tableWidget->rowCount(); ++i)
            tableWidget->item(i, 0)->setText(MxFunctions::mNumberToString(i + 1));
        onAutoRefresh();
    }

    void mFunctionPanel::onMoveUp()
    {
        int r = tableWidget->currentRow();
        if (r < 1) return;

        /* 1. 在 r-1 处插入空行 */
        tableWidget->insertRow(r - 1);

        /* 2. 复制数据（跳过序号列） */
        for (int c = 1; c < 3; ++c)
            tableWidget->setItem(r - 1, c,
                new QTableWidgetItem(tableWidget->item(r + 1, c)->text()));

        /* 3. 补写序号 */
        auto* idxItem = new QTableWidgetItem(MxFunctions::mNumberToString(r));   // 新第 r-1 行序号应为 r
        idxItem->setFlags(idxItem->flags() & ~Qt::ItemIsEditable);
        tableWidget->setItem(r - 1, 0, idxItem);

        /* 4. 删除原来的行 */
        tableWidget->removeRow(r + 1);

        /* 5. 重排所有序号（简单暴力） */
        for (int i = 0; i < tableWidget->rowCount(); ++i)
            tableWidget->item(i, 0)->setText(MxFunctions::mNumberToString(i + 1));

        tableWidget->setCurrentCell(r - 1, 1);
        onAutoRefresh();
    }

    void mFunctionPanel::onMoveDown()
    {
        int r = tableWidget->currentRow();
        if (r < 0 || r >= tableWidget->rowCount() - 1) return;

        /* 1. 在 r+2 处插入空行 */
        tableWidget->insertRow(r + 2);

        /* 2. 复制数据（跳过序号列） */
        for (int c = 1; c < 3; ++c)
            tableWidget->setItem(r + 2, c,
                new QTableWidgetItem(tableWidget->item(r, c)->text()));

        /* 3. 补写序号 */
        auto* idxItem = new QTableWidgetItem(MxFunctions::mNumberToString(r + 3)); // 新行序号
        idxItem->setFlags(idxItem->flags() & ~Qt::ItemIsEditable);
        tableWidget->setItem(r + 2, 0, idxItem);

        /* 4. 删除原来的行 */
        tableWidget->removeRow(r);

        /* 5. 重排所有序号 */
        for (int i = 0; i < tableWidget->rowCount(); ++i)
            tableWidget->item(i, 0)->setText(MxFunctions::mNumberToString(i + 1));

        tableWidget->setCurrentCell(r + 1, 1);
        onAutoRefresh();
    }

    void mFunctionPanel::onTrashRow()
    {
        int r = tableWidget->currentRow();
        if (r < 0) return;
        tableWidget->removeRow(r);
        /* 重排序号 */
        for (int i = 0; i < tableWidget->rowCount(); ++i)
            tableWidget->item(i, 0)->setText(MxFunctions::mNumberToString(i + 1));
        onAutoRefresh();
    }

    /* ========== 原 setupExtraControls 内容 ========== */
    void mFunctionPanel::setupExtraControls()
    {
        /*==================== 离散页 tableWidget ====================*/
        tableWidget->setColumnCount(3);
        tableWidget->setHorizontalHeaderLabels(QStringList() << "序号" << "x" << "y");
        tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        tableWidget->verticalHeader()->setVisible(false);
        tableWidget->setRowCount(3);
        for (int i = 0; i < 3; ++i) {
            auto* it = new QTableWidgetItem(MxFunctions::mNumberToString(i + 1));
            it->setFlags(it->flags() & ~Qt::ItemIsEditable);
            tableWidget->setItem(i, 0, it);
        }

        /*==================== 分段页 tableWidget_3 + 采样点数旋钮 ====================*/
        //tableWidget_3->setColumnCount(4);
        tableWidget_3->setHorizontalHeaderLabels(QStringList() << "序号" << "起始" << "结束" << "表达式");
        tableWidget_3->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        tableWidget_3->verticalHeader()->setVisible(false);
        tableWidget_3->setRowCount(2);
        for (int i = 0; i < 2; ++i) {
            auto* it = new QTableWidgetItem(MxFunctions::mNumberToString(i + 1));
            it->setFlags(it->flags() & ~Qt::ItemIsEditable);
            tableWidget_3->setItem(i, 0, it);
            tableWidget_3->setItem(i, 4, new QTableWidgetItem("10"));  // ★新增
        }

        /* ↓↓↓ 列宽比例调整 ↓↓↓ */
        tableWidget_3->setColumnWidth(0, 40);   // 序号
        tableWidget_3->setColumnWidth(1, 60);   // 起始
        tableWidget_3->setColumnWidth(2, 60);   // 结束
        tableWidget_3->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch); // 表达式自适应
        tableWidget_3->setColumnWidth(4, 80);   // 采样点数

        /*==================== 统一表格实时刷新 ====================*/
        auto tableRefresh = [this](int, int) { onAutoRefresh(); };
        connect(tableWidget, &QTableWidget::cellChanged, this, tableRefresh);
        connect(tableWidget_2, &QTableWidget::cellChanged, this, tableRefresh);
        connect(tableWidget_3, &QTableWidget::cellChanged, this, tableRefresh);
        /* 给所有已存在的旧行补“采样点数”单元格，默认 10 */
        for (int i = 0; i < tableWidget_3->rowCount(); ++i)
            ensureItem(tableWidget_3, i, 4)->setText("10");
    }

    void mFunctionPanel::syncDataToFuncData()
{
        int functionType = _funcData->getFunctionType();

    _funcData->setXRange(lineEdit->text().toDouble(),
                         lineEdit_2->text().toDouble());

    switch (functionType) {
    case 0: { /* 解析 */
        auto &ac = _funcData->analytic();
        ac.expr = lineEdit_3->text().trimmed();
        break;
    }
    case 1: { /* 离散 */
        auto& d = _funcData->discrete();
        d.idx.clear(); d.x.clear(); d.y.clear();
        for (int r = 0; r < tableWidget->rowCount(); ++r) {
            auto* itX = ensureItem(tableWidget, r, 1);
            auto* itY = ensureItem(tableWidget, r, 2);
            bool okX, okY;
            double xv = itX->text().toDouble(&okX);
            double yv = itY->text().toDouble(&okY);
            if (okX && okY) {
                d.idx.push_back(r + 1);
                d.x.push_back(xv);
                d.y.push_back(yv);
            }
        }
        break;
    }
    case 2: { /* 插值 */
        auto& ic = _funcData->interp();
        ic.x.clear(); ic.y.clear();
        for (int r = 0; r < tableWidget_interp->rowCount(); ++r) {
            auto* itX = ensureItem(tableWidget_interp, r, 1);
            auto* itY = ensureItem(tableWidget_interp, r, 2);
            bool okX, okY;
            double xv = itX->text().toDouble(&okX);
            double yv = itY->text().toDouble(&okY);
            if (okX && okY) {
                ic.x.push_back(xv);
                ic.y.push_back(yv);
            }
        }
        ic.interpMode = comboBox_5->currentIndex();   // 0-临近点 1-线性 2-分段三次 3-三次样条 4-对数 5-光滑 6-阶跃
        ic.extrapMode = comboBox_4->currentIndex();   // 0-常数外推 1-线性外推
        break;
    }
    case 3: { /* 分段 */
        auto& pc = _funcData->piecewise();
        pc.idx.clear();
        pc.start.clear();
        pc.end.clear();
        pc.expr.clear();
        pc.sampleCount.clear();

        for (int r = 0; r < tableWidget_3->rowCount(); ++r) {
            // 确保 4 列都有 QTableWidgetItem，空就当 0
            auto* itIdx = ensureItem(tableWidget_3, r, 0);   // 序号列
            auto* itStart = ensureItem(tableWidget_3, r, 1);   // 起始
            auto* itEnd = ensureItem(tableWidget_3, r, 2);   // 结束
            auto* itExpr = ensureItem(tableWidget_3, r, 3);   // 表达式
            auto* itSample = ensureItem(tableWidget_3, r, 4);   // 表达式

            bool okS = false, okE = false;
            double s = itStart->text().toDouble(&okS);
            double e = itEnd->text().toDouble(&okE);
            if (!okS || !okE || s > e) continue;   // 数据非法就跳过

            pc.idx.push_back(r + 1);
            pc.start.push_back(s);
            pc.end.push_back(e);
            pc.expr.push_back(itExpr->text());
            pc.sampleCount.push_back(itSample->text().toInt());

        }
        break;
    }
    case 4: { /* 阶跃 */
        auto &sc = _funcData->step();
        sc.xMin = lineEdit_4->text().toDouble() -10;
        sc.xMax = lineEdit_4->text().toDouble()+10;
        sc.pos  = lineEdit_4->text().toDouble();
        sc.v0   = lineEdit_9->text().toDouble();
        sc.v1   = lineEdit_10->text().toDouble();
        break;
    }
    case 5:
    case 6:
    case 7:
    { /* 表格 / 可平移表格 */
        auto& tbl = _funcData->table();
        tbl.x.clear(); tbl.y.clear();
        for (int r = 0; r < tableWidget_table->rowCount(); ++r) {
            auto* itX = ensureItem(tableWidget_table, r, 1);
            auto* itY = ensureItem(tableWidget_table, r, 2);
            bool okX, okY;
            double xv = itX->text().toDouble(&okX);
            double yv = itY->text().toDouble(&okY);
            if (okX && okY) {
                tbl.x.push_back(xv);
                tbl.y.push_back(yv);
            }
        }
        tbl.xAxisType = comboBox_xAxisType->currentIndex();
        tbl.yAxisType = comboBox_yAxisType->currentIndex();
        tbl.extrapMode = comboBox_extrapTable->currentIndex();
        tbl.x1 = m_lineEdit_x1->text().toDouble();
        tbl.x2 = m_lineEdit_x2->text().toDouble();
        tbl.x3 = m_lineEdit_x3->text().toDouble();
        tbl.x4 = m_lineEdit_x4->text().toDouble();
        break;
    }
    case 8:
    { /* 幂函数 */
        auto& tbl = _funcData->table();
        tbl.x.clear(); tbl.y.clear();
        for (int r = 0; r < tableWidget_table->rowCount(); ++r) {
            auto* itA = ensureItem(tableWidget_table, r, 1);
            bool okA;
            double a = itA->text().toDouble(&okA);
            if (okA) {
                tbl.x.push_back(a);
            }
            tbl.y.push_back(0);
        }
        tbl.xAxisType = 0;
        tbl.yAxisType = 0;
        tbl.extrapMode = 0;
        tbl.x1 = m_lineEdit_x1->text().toDouble();
        tbl.x2 = m_lineEdit_x2->text().toDouble();
        tbl.x3 = m_lineEdit_x3->text().toDouble();
        tbl.x4 = m_lineEdit_x4->text().toDouble();
        break;
    }
    case 9:
    { /* 周期函数 */
        auto& per = _funcData->periodic();
        per.freq = m_lineEditPeriodicFreq->text().toDouble();
        per.t0 = m_lineEditPeriodicT0->text().toDouble();
        per.A0 = m_lineEditPeriodicA0->text().toDouble();
        per.A.clear(); per.B.clear();
        for (int r = 0; r < tableWidget_periodic->rowCount(); ++r) {
            auto* itA = ensureItem(tableWidget_periodic, r, 1);
            auto* itB = ensureItem(tableWidget_periodic, r, 2);
            bool okA, okB;
            double av = itA->text().toDouble(&okA);
            double bv = itB->text().toDouble(&okB);
            if (okA && okB) {
                per.A.push_back(av);
                per.B.push_back(bv);
            }
        }
        per.order = static_cast<int>(per.A.size());
        break;
    }
    case 10:
    { /* 调制函数 */
        auto& mod = _funcData->modulated();
        mod.A0 = m_lineEditModulatedA0->text().toDouble();
        mod.A  = m_lineEditModulatedA->text().toDouble();
        mod.t0 = m_lineEditModulatedT0->text().toDouble();
        mod.f1 = m_lineEditModulatedF1->text().toDouble();
        mod.f2 = m_lineEditModulatedF2->text().toDouble();
        break;
    }
    case 11:
    { /* 指数衰减函数 */
        auto& ed = _funcData->expDecay();
        ed.A0 = m_lineEditExpDecayA0->text().toDouble();
        ed.Am = m_lineEditExpDecayAm->text().toDouble();
        ed.t0 = m_lineEditExpDecayT0->text().toDouble();
        ed.td = m_lineEditExpDecayTd->text().toDouble();
        break;
    }
    }

    switch (functionType)
    {
    case 0: {
        //解析
        double min = lineEdit->text().toDouble();
        double max = lineEdit_2->text().toDouble();
        int simple = spinSamplesAna->value();
        QString express = lineEdit_3->text();
        AnalyticCol ana = AnalyticCol{ min,max,express,simple };
        _funcData->setAnalytic(ana);
        break;
    }
    case 1:
    {
        //离散
        int n = tableWidget->rowCount();
        std::vector<int> idx(n);
        for (int i = 0; i < n; ++i) {
            idx[i] = i + 1;
        }
        std::vector<double> x(n);
        for (int i = 0; i < n; ++i) {
            if (tableWidget->item(i, 0))
                x[i] = tableWidget->item(i, 1)->text().toDouble();
            else
                x[i] = 0;
        }
        std::vector<double> y(n);
        for (int i = 0; i < n; ++i) {
            if (tableWidget->item(i, 1))
                y[i] = tableWidget->item(i, 2)->text().toDouble();
            else
                y[i] = 0;
        }
        DiscreteCol dis = { idx,x,y };
        _funcData->setDiscrete(dis);
    }
    }
}

    static QString askSaveName(QWidget* parent, const QString& dft)
    {
        return QFileDialog::getSaveFileName(parent, "导出数据", dft, "Text Files (*.txt)");
    }

#define OUT_HEADER(ts, tp) \
    ts << "# FunctionType: " << (tp) << "\n" \
       << "# Variable: x\n" \
       << "# Range: " << _funcData->xMin() << " ~ " << _funcData->xMax() << "\n"

    /* 1. 解析 */
    void mFunctionPanel::onExportAnalytic()
    {
        const auto& ac = _funcData->analytic();
        if (ac.expr.isEmpty()) return;
        QString fn = askSaveName(this, "analytic.txt");
        if (fn.isEmpty()) return;
        QFile f(fn);  if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
        QTextStream ts(&f);
        OUT_HEADER(ts, "Analytic");
        ts << "# Expression: " << ac.expr << "\n";
        ts << "# x\ty\n";

        mu::Parser p;  double x = 0;  p.DefineVar("x", &x);
        p.SetExpr(ac.expr.toStdString());
        double dx = (ac.xMax - ac.xMin) / 199;
        for (int i = 0; i < 200; ++i) {
            x = ac.xMin + i * dx;
            ts << x << '\t' << p.Eval() << '\n';
        }
    }

    /* 2. 离散 */
    void mFunctionPanel::onExportDiscrete()
    {
        const auto& d = _funcData->discrete();
        if (d.x.empty()) return;
        QString fn = askSaveName(this, "discrete.txt");
        if (fn.isEmpty()) return;
        QFile f(fn);  if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
        QTextStream ts(&f);
        OUT_HEADER(ts, "Discrete");
        ts << "# x\ty\n";
        for (int i = 0; i < d.x.size(); ++i) ts << d.x[i] << '\t' << d.y[i] << '\n';
    }

    /* 3. 插值 */
    void mFunctionPanel::onExportInterp()
    {
        const auto& ic = _funcData->interp();
        if (ic.x.empty()) return;

        QString fn = askSaveName(this, "interp.txt");
        if (fn.isEmpty()) return;
        QFile f(fn);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
        QTextStream ts(&f);
        OUT_HEADER(ts, "Interp");
        ts << "# InterpMode: " << ic.interpMode
            << "  ExtrapMode: " << ic.extrapMode << "\n";
        ts << "# x\ty\n";
        for (size_t i = 0; i < ic.x.size(); ++i)
            ts << ic.x[i] << '\t' << ic.y[i] << '\n';
    }

    /* 4. 分段 */
    void mFunctionPanel::onExportPiecewise()
    {
        const auto& pc = _funcData->piecewise();
        if (pc.start.empty()) return;

        QString fn = askSaveName(this, "piecewise.txt");
        if (fn.isEmpty()) return;
        QFile f(fn);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
        QTextStream ts(&f);
        OUT_HEADER(ts, "Piecewise");
        ts << "# SegmentIndex\tStart\tEnd\tExpression\n";

        for (int i = 0; i < pc.start.size(); ++i) {
            // 确保单元格非空
            auto* itStart = ensureItem(tableWidget_3, i, 1);
            auto* itEnd = ensureItem(tableWidget_3, i, 2);
            auto* itExpr = ensureItem(tableWidget_3, i, 3);

            ts << (i + 1) << '\t'
                << itStart->text() << '\t'
                << itEnd->text() << '\t'
                << itExpr->text() << '\n';
        }
    }

    /* 5. 阶跃 */
    void mFunctionPanel::onExportStep()
    {
        const auto& sc = _funcData->step();
        QString fn = askSaveName(this, "step.txt");
        if (fn.isEmpty()) return;
        QFile f(fn);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
        QTextStream ts(&f);
        OUT_HEADER(ts, "Step");
        ts << "# StepPos: " << sc.pos << "  V0: " << sc.v0 << "  V1: " << sc.v1 << "\n";
        ts << "# x\ty\n";

        // 用顶部“变量范围”作为左右区间
        double x0 = sc.xMin;
        double x1 = sc.xMax;
        int N = 100;
        double dxL = (sc.pos - x0) / (N - 1);
        double dxR = (x1 - sc.pos) / (N - 1);
        for (int i = 0; i < N; ++i) ts << (x0 + i * dxL) << '\t' << sc.v0 << '\n';
        for (int i = 0; i < N; ++i) ts << (sc.pos + i * dxR) << '\t' << sc.v1 << '\n';
    }
    /* ========== 插值页 5 按钮 ========== */
    void mFunctionPanel::onInterpAddRow()
    {
        int current = tableWidget_interp->currentRow();
        int rc = tableWidget_interp->rowCount();
        if (current == -1 || current == rc - 1)
        {
            tableWidget_interp->insertRow(rc);
            auto* it = new QTableWidgetItem(MxFunctions::mNumberToString(rc + 1));
            it->setFlags(it->flags() & ~Qt::ItemIsEditable);
            tableWidget_interp->setItem(rc, 0, it);
            tableWidget_interp->setItem(rc, 1, new QTableWidgetItem("0"));
            tableWidget_interp->setItem(rc, 2, new QTableWidgetItem("0"));
            onAutoRefresh();
        }
        else
        {
            tableWidget_interp->insertRow(current+1);
            auto* it = new QTableWidgetItem(MxFunctions::mNumberToString(current + 2));
            it->setFlags(it->flags() & ~Qt::ItemIsEditable);
            tableWidget_interp->setItem(current+1, 0, it);
            tableWidget_interp->setItem(current+1, 1, new QTableWidgetItem("0"));
            tableWidget_interp->setItem(current+1, 2, new QTableWidgetItem("0"));
            for (int i = current + 2; i < tableWidget_interp->rowCount(); i++)
            {
                tableWidget_interp->item(i, 0)->setText(MxFunctions::mNumberToString(i+1));
            }
            
            onAutoRefresh();
        }
     
    }
    void mFunctionPanel::onInterpDelRow()
    {
        int r = tableWidget_interp->currentRow();
        if (r < 0) return;
        tableWidget_interp->removeRow(r);
        /* 重排序号 */
        for (int i = 0; i < tableWidget_interp->rowCount(); ++i)
            tableWidget_interp->item(i, 0)->setText(MxFunctions::mNumberToString(i + 1));
        onAutoRefresh();
    }
    void mFunctionPanel::onInterpMoveUp()
    {
        int r = tableWidget_interp->currentRow();
        if (r < 1) return;

        tableWidget_interp->insertRow(r - 1);
        /* 只搬 x、y 两列 */
        for (int c = 1; c < 3; ++c)
            tableWidget_interp->setItem(r - 1, c,
                new QTableWidgetItem(tableWidget_interp->item(r + 1, c)->text()));
        
        
        /* 3. 补写序号 */
        auto* idxItem = new QTableWidgetItem(MxFunctions::mNumberToString(r));   // 新第 r-1 行序号应为 r
        idxItem->setFlags(idxItem->flags() & ~Qt::ItemIsEditable);
        tableWidget_interp->setItem(r - 1, 0, idxItem);

        tableWidget_interp->removeRow(r + 1);

        /* 重排所有序号 */
        for (int i = 0; i < tableWidget_interp->rowCount(); ++i)
            tableWidget_interp->item(i, 0)->setText(MxFunctions::mNumberToString(i + 1));

        tableWidget_interp->setCurrentCell(r - 1, 1);
        onAutoRefresh();
    }
    void mFunctionPanel::onInterpMoveDown()
    {
        int r = tableWidget_interp->currentRow();
        if (r < 0 || r >= tableWidget_interp->rowCount() - 1) return;
        tableWidget_interp->insertRow(r + 2);
        for (int c = 0; c < 3; ++c)
            tableWidget_interp->setItem(r + 2, c,
                new QTableWidgetItem(tableWidget_interp->item(r, c)->text()));
        tableWidget_interp->removeRow(r);
        /* 5. 重排所有序号 */
        for (int i = 0; i < tableWidget_interp->rowCount(); ++i)
            tableWidget_interp->item(i, 0)->setText(MxFunctions::mNumberToString(i + 1));
        tableWidget_interp->setCurrentCell(r + 1, 1);
        onAutoRefresh();
    }
    void mFunctionPanel::onInterpTrashRow()
    {
        int r = tableWidget_interp->currentRow();
        if (r < 0) return;
        tableWidget_interp->removeRow(r);
        for (int i = 0; i < tableWidget_interp->rowCount(); ++i)
            tableWidget_interp->item(i, 0)->setText(MxFunctions::mNumberToString(i + 1));
        onAutoRefresh();
    }

    /* ========== 分段页 5 按钮 ========== */
    void mFunctionPanel::onPieceAddRow()
    {
        int current = tableWidget_3->currentRow();
        int rc = tableWidget_3->rowCount();
  
        if (current == -1 || current == rc - 1)
        {
         
            QString bufCol2;
            if (rc == 0)
                bufCol2 = "0";
            else
                bufCol2 = tableWidget_3->item(rc - 1, 2)->text();
            tableWidget_3->insertRow(rc);
            auto* it = new QTableWidgetItem(MxFunctions::mNumberToString(rc + 1));
            it->setFlags(it->flags() & ~Qt::ItemIsEditable);
            tableWidget_3->setItem(rc, 0, it);
            tableWidget_3->setItem(rc, 1, new QTableWidgetItem(bufCol2));
            tableWidget_3->setItem(rc, 2, new QTableWidgetItem(MxFunctions::mNumberToString(0)));
            tableWidget_3->setItem(rc, 4, new QTableWidgetItem(MxFunctions::mNumberToString(10)));
            // 其余三列留空，由用户填写
           
        }
        else
        {
            QString bufCol1 = tableWidget_3->item(current, 2)->text();
            QString bufCol2 = tableWidget_3->item(current+1, 1)->text();
            tableWidget_3->insertRow(current+1);
            auto* it = new QTableWidgetItem(MxFunctions::mNumberToString(current + 2));
            it->setFlags(it->flags() & ~Qt::ItemIsEditable);
            tableWidget_3->setItem(current + 1, 0, it);
            tableWidget_3->setItem(current + 1, 1, new QTableWidgetItem(bufCol1));
            tableWidget_3->setItem(current + 1, 2, new QTableWidgetItem(bufCol2));
            tableWidget_3->setItem(current + 1, 4, new QTableWidgetItem(MxFunctions::mNumberToString(10)));

            for (int i = current + 2; i < tableWidget_3->rowCount(); i++)
            {
                tableWidget_3->item(i, 0)->setText(MxFunctions::mNumberToString(i + 1));
            }

           

        }
        onAutoRefresh();
    }
    void mFunctionPanel::onPieceDelRow()
    {
        int r = tableWidget_3->currentRow();
        if (r < 0) return;
        if (r == tableWidget_3->rowCount() - 1)
        {
            tableWidget_3->removeRow(r);
            /* 重排序号 */
            for (int i = 0; i < tableWidget_3->rowCount(); ++i)
                tableWidget_3->item(i, 0)->setText(MxFunctions::mNumberToString(i + 1));
        }
        else
        {
            QString bufCol1 = tableWidget_3->item(r + 1, 1)->text();
            QString bufCol2 = tableWidget_3->item(r + 1, 2)->text();
            tableWidget_3->removeRow(r);
            /* 重排序号 */
            for (int i = 0; i < tableWidget_3->rowCount(); ++i)
                tableWidget_3->item(i, 0)->setText(MxFunctions::mNumberToString(i + 1));
            if(r!=0)
                tableWidget_3->item(r-1, 2)->setText(bufCol1);
        }

        onAutoRefresh();
    }
    void mFunctionPanel::onPieceMoveUp()
    {
        int r = tableWidget_3->currentRow();
        if (r < 1) return;

        /* 1. 缓存第1、2列，防止删除行后丢失 */
        QStringList bufCol1;
        QStringList bufCol2;
        int rows = tableWidget_3->rowCount();
        for (int r = 0; r < rows; ++r)
        {
            bufCol1 << tableWidget_3->item(r, 1)->text();
            bufCol2 << tableWidget_3->item(r, 2)->text();
        }
           
        

        tableWidget_3->insertRow(r - 1);
        for (int c = 0; c < 5; ++c)
        {
            if (c == 1 || c == 2)
                continue;//区间不上下移动
            tableWidget_3->setItem(r - 1, c,
                new QTableWidgetItem(tableWidget_3->item(r + 1, c)->text()));
        }
          
        tableWidget_3->removeRow(r + 1);
        /* 5. 把第1、2列写回新位置 */
        /* 6. 重排所有序号 */
        for (int i = 0; i < tableWidget_3->rowCount(); ++i)
        {
            tableWidget_3->setItem(i, 1, new QTableWidgetItem(bufCol1[i]));
            tableWidget_3->setItem(i, 2, new QTableWidgetItem(bufCol2[i]));
            tableWidget_3->item(i, 0)->setText(MxFunctions::mNumberToString(i + 1));
        }
           
        tableWidget_3->setCurrentCell(r - 1, 1);
        onAutoRefresh();
    }
    void mFunctionPanel::onPieceMoveDown()
    {
        int r = tableWidget_3->currentRow();
        if (r < 0 || r >= tableWidget_3->rowCount() - 1) return;
        /* 1. 缓存第1、2列，防止删除行后丢失 */
        QStringList bufCol1;
        QStringList bufCol2;
        int rows = tableWidget_3->rowCount();
        for (int r = 0; r < rows; ++r)
        {
            bufCol1 << tableWidget_3->item(r, 1)->text();
            bufCol2 << tableWidget_3->item(r, 2)->text();
        }

        tableWidget_3->insertRow(r + 2);
        for (int c = 0; c < 5; ++c)
        {
            if (c == 1 || c == 2)
                continue;//区间不上下移动
            tableWidget_3->setItem(r + 2, c,
                new QTableWidgetItem(tableWidget_3->item(r, c)->text()));
        }
          
        tableWidget_3->removeRow(r);
        /* 5. 重排所有序号,把第1、2列写回新位置 */
        for (int i = 0; i < tableWidget_3->rowCount(); ++i)
        {
            tableWidget_3->setItem(i, 1, new QTableWidgetItem(bufCol1[i]));
            tableWidget_3->setItem(i, 2, new QTableWidgetItem(bufCol2[i]));
            tableWidget_3->item(i, 0)->setText(MxFunctions::mNumberToString(i + 1));
        }
  
        tableWidget_3->setCurrentCell(r + 1, 1);
        onAutoRefresh();
    }
    void mFunctionPanel::onPieceTrashRow()
    {
        int r = tableWidget_3->currentRow();
        if (r < 0) return;
        tableWidget_3->removeRow(r);
        for (int i = 0; i < tableWidget_3->rowCount(); ++i)
            tableWidget_3->item(i, 0)->setText(MxFunctions::mNumberToString(i + 1));
            //ensureItem(tableWidget_3, i, 4)->setText("10");   // 默认采样点数
        onAutoRefresh();
    }

    /* ========== 表格页 5 按钮 ========== */
    void mFunctionPanel::onTableAddRow()
    {
        int current = tableWidget_table->currentRow();
        int rc = tableWidget_table->rowCount();
        int cc = tableWidget_table->columnCount();
        if (current == -1 || current == rc - 1)
        {
            tableWidget_table->insertRow(rc);
            auto* it = new QTableWidgetItem(MxFunctions::mNumberToString(rc + 1));
            it->setFlags(it->flags() & ~Qt::ItemIsEditable);
            tableWidget_table->setItem(rc, 0, it);
            for (int c = 1; c < cc; ++c)
                tableWidget_table->setItem(rc, c, new QTableWidgetItem("0"));
            onAutoRefresh();
        }
        else
        {
            tableWidget_table->insertRow(current + 1);
            auto* it = new QTableWidgetItem(MxFunctions::mNumberToString(current + 2));
            it->setFlags(it->flags() & ~Qt::ItemIsEditable);
            tableWidget_table->setItem(current + 1, 0, it);
            for (int c = 1; c < cc; ++c)
                tableWidget_table->setItem(current + 1, c, new QTableWidgetItem("0"));
            for (int i = current + 2; i < tableWidget_table->rowCount(); i++)
            {
                tableWidget_table->item(i, 0)->setText(MxFunctions::mNumberToString(i + 1));
            }
            onAutoRefresh();
        }
    }
    void mFunctionPanel::onTableDelRow()
    {
        int r = tableWidget_table->currentRow();
        if (r < 0) return;
        tableWidget_table->removeRow(r);
        for (int i = 0; i < tableWidget_table->rowCount(); ++i)
            tableWidget_table->item(i, 0)->setText(MxFunctions::mNumberToString(i + 1));
        onAutoRefresh();
    }
    void mFunctionPanel::onTableMoveUp()
    {
        int r = tableWidget_table->currentRow();
        if (r < 1) return;

        int cc = tableWidget_table->columnCount();
        tableWidget_table->insertRow(r - 1);
        for (int c = 1; c < cc; ++c)
            tableWidget_table->setItem(r - 1, c,
                new QTableWidgetItem(tableWidget_table->item(r + 1, c)->text()));

        auto* idxItem = new QTableWidgetItem(MxFunctions::mNumberToString(r));
        idxItem->setFlags(idxItem->flags() & ~Qt::ItemIsEditable);
        tableWidget_table->setItem(r - 1, 0, idxItem);

        tableWidget_table->removeRow(r + 1);

        for (int i = 0; i < tableWidget_table->rowCount(); ++i)
            tableWidget_table->item(i, 0)->setText(MxFunctions::mNumberToString(i + 1));

        tableWidget_table->setCurrentCell(r - 1, 1);
        onAutoRefresh();
    }
    void mFunctionPanel::onTableMoveDown()
    {
        int r = tableWidget_table->currentRow();
        if (r < 0 || r >= tableWidget_table->rowCount() - 1) return;
        int cc = tableWidget_table->columnCount();
        tableWidget_table->insertRow(r + 2);
        for (int c = 1; c < cc; ++c)
            tableWidget_table->setItem(r + 2, c,
                new QTableWidgetItem(tableWidget_table->item(r, c)->text()));

        auto* idxItem = new QTableWidgetItem(MxFunctions::mNumberToString(r + 3));
        idxItem->setFlags(idxItem->flags() & ~Qt::ItemIsEditable);
        tableWidget_table->setItem(r + 2, 0, idxItem);

        tableWidget_table->removeRow(r);

        for (int i = 0; i < tableWidget_table->rowCount(); ++i)
            tableWidget_table->item(i, 0)->setText(MxFunctions::mNumberToString(i + 1));

        tableWidget_table->setCurrentCell(r + 1, 1);
        onAutoRefresh();
    }
    void mFunctionPanel::onTableExport()
    {
        const auto& tbl = _funcData->table();
        if (tbl.x.empty()) return;

        QString fn = askSaveName(this, "table.txt");
        if (fn.isEmpty()) return;
        QFile f(fn);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
        QTextStream ts(&f);
        OUT_HEADER(ts, "Table");
        ts << "# xAxisType: " << (tbl.xAxisType == 0 ? "linear" : "log")
           << "  yAxisType: " << (tbl.yAxisType == 0 ? "linear" : "log")
           << "  ExtrapMode: " << (tbl.extrapMode == 0 ? "constant" : "linear") << "\n";
        ts << "# x\ty\n";
        for (size_t i = 0; i < tbl.x.size(); ++i)
            ts << tbl.x[i] << '\t' << tbl.y[i] << '\n';
    }

    /* ========== 周期页 4 按钮 ========== */
    void mFunctionPanel::onPeriodicAddRow()
    {
        int current = tableWidget_periodic->currentRow();
        int rc = tableWidget_periodic->rowCount();
        if (current == -1 || current == rc - 1)
        {
            tableWidget_periodic->insertRow(rc);
            auto* it = new QTableWidgetItem(MxFunctions::mNumberToString(rc + 1));
            it->setFlags(it->flags() & ~Qt::ItemIsEditable);
            tableWidget_periodic->setItem(rc, 0, it);
            tableWidget_periodic->setItem(rc, 1, new QTableWidgetItem("0"));
            tableWidget_periodic->setItem(rc, 2, new QTableWidgetItem("0"));
            onAutoRefresh();
        }
        else
        {
            tableWidget_periodic->insertRow(current + 1);
            auto* it = new QTableWidgetItem(MxFunctions::mNumberToString(current + 2));
            it->setFlags(it->flags() & ~Qt::ItemIsEditable);
            tableWidget_periodic->setItem(current + 1, 0, it);
            tableWidget_periodic->setItem(current + 1, 1, new QTableWidgetItem("0"));
            tableWidget_periodic->setItem(current + 1, 2, new QTableWidgetItem("0"));
            for (int i = current + 2; i < tableWidget_periodic->rowCount(); ++i)
                tableWidget_periodic->item(i, 0)->setText(MxFunctions::mNumberToString(i + 1));
            onAutoRefresh();
        }
    }
    void mFunctionPanel::onPeriodicDelRow()
    {
        int r = tableWidget_periodic->currentRow();
        if (r < 0) return;
        tableWidget_periodic->removeRow(r);
        for (int i = 0; i < tableWidget_periodic->rowCount(); ++i)
            tableWidget_periodic->item(i, 0)->setText(MxFunctions::mNumberToString(i + 1));
        onAutoRefresh();
    }
    void mFunctionPanel::onPeriodicMoveUp()
    {
        int r = tableWidget_periodic->currentRow();
        if (r < 1) return;

        tableWidget_periodic->insertRow(r - 1);
        for (int c = 1; c < 3; ++c)
            tableWidget_periodic->setItem(r - 1, c,
                new QTableWidgetItem(tableWidget_periodic->item(r + 1, c)->text()));

        auto* idxItem = new QTableWidgetItem(MxFunctions::mNumberToString(r));
        idxItem->setFlags(idxItem->flags() & ~Qt::ItemIsEditable);
        tableWidget_periodic->setItem(r - 1, 0, idxItem);

        tableWidget_periodic->removeRow(r + 1);

        for (int i = 0; i < tableWidget_periodic->rowCount(); ++i)
            tableWidget_periodic->item(i, 0)->setText(MxFunctions::mNumberToString(i + 1));

        tableWidget_periodic->setCurrentCell(r - 1, 1);
        onAutoRefresh();
    }
    void mFunctionPanel::onPeriodicMoveDown()
    {
        int r = tableWidget_periodic->currentRow();
        if (r < 0 || r >= tableWidget_periodic->rowCount() - 1) return;

        tableWidget_periodic->insertRow(r + 2);
        for (int c = 1; c < 3; ++c)
            tableWidget_periodic->setItem(r + 2, c,
                new QTableWidgetItem(tableWidget_periodic->item(r, c)->text()));

        auto* idxItem = new QTableWidgetItem(MxFunctions::mNumberToString(r + 3));
        idxItem->setFlags(idxItem->flags() & ~Qt::ItemIsEditable);
        tableWidget_periodic->setItem(r + 2, 0, idxItem);

        tableWidget_periodic->removeRow(r);

        for (int i = 0; i < tableWidget_periodic->rowCount(); ++i)
            tableWidget_periodic->item(i, 0)->setText(MxFunctions::mNumberToString(i + 1));

        tableWidget_periodic->setCurrentCell(r + 1, 1);
        onAutoRefresh();
    }

    // 事件过滤器，防止滚轮滑动导致切换函数面板及采样点数值被改变
    bool mFunctionPanel::eventFilter(QObject* obj, QEvent* ev)
    {
        // 1. 屏蔽所有 QSpinBox 滚轮
        if (ev->type() == QEvent::Wheel && qobject_cast<QSpinBox*>(obj))
        {
            ev->ignore();
            return true;
        }
        // 2. 屏蔽函数类型、插值方式、外推方式三个下拉框滚轮
        if (ev->type() == QEvent::Wheel &&
            (obj == comboBox || obj == comboBox_5 || obj == comboBox_4))
        {
            ev->ignore();
            return true;
        }
        // 3. 屏蔽表格函数下拉框滚轮
        if (ev->type() == QEvent::Wheel &&
            (obj == comboBox_xAxisType || obj == comboBox_yAxisType || obj == comboBox_extrapTable))
        {
            ev->ignore();
            return true;
        }
        return mPanelBase::eventFilter(obj, ev);
    }

 
} // namespace MechMainWindow