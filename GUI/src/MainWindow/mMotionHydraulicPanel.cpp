#include "mMotionHydraulicPanel.h"
#include "mMechGlobalSignals.h"
#include "mAppConfiguration.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mMotionHydraulicManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QFileDialog>

using namespace MechMotionData;
using namespace MxFunctions;

namespace MechMainWindow
{
    mMotionHydraulicPanel::mMotionHydraulicPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        m_mainLayout = new QVBoxLayout(this);
        m_mainLayout->setContentsMargins(0, 0, 0, 0);
        m_mainLayout->setSpacing(4);

        // ==================== 基础信息 ====================
        FoldSection* foldBasic = new FoldSection("基础信息", this);
        m_edtName = new QLineEdit();
        m_edtName->setPlaceholderText("液压元件名称");
        foldBasic->addRow(new QLabel("名称:"), m_edtName);

        m_cmbHydraulicType = new QComboBox();
        m_cmbHydraulicType->addItems({ "液压油缸", "液压阀", "液压泵", "液压管路", "蓄能器" });
        foldBasic->addRow(new QLabel("元件类型:"), m_cmbHydraulicType);

        QHBoxLayout* layModel = new QHBoxLayout();
        layModel->setContentsMargins(0, 0, 0, 0);
        m_edtModelFile = new QLineEdit();
        m_edtModelFile->setPlaceholderText("选择液压模型文件...");
        m_btnBrowseModel = new QToolButton();
        m_btnBrowseModel->setText("...");
        m_btnBrowseModel->setFixedWidth(30);
        layModel->addWidget(m_edtModelFile);
        layModel->addWidget(m_btnBrowseModel);
        foldBasic->addRow(new QLabel("模型文件:"), layModel);

        m_mainLayout->addWidget(foldBasic);

        // ==================== 流体参数 ====================
        FoldSection* foldFluid = new FoldSection("流体参数", this);
        m_spbFluidDensity = new QDoubleSpinBox(); m_spbFluidDensity->setRange(0, 1e6); m_spbFluidDensity->setDecimals(4); m_spbFluidDensity->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbFluidViscosity = new QDoubleSpinBox(); m_spbFluidViscosity->setRange(0, 1e6); m_spbFluidViscosity->setDecimals(6); m_spbFluidViscosity->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbBulkModulus = new QDoubleSpinBox(); m_spbBulkModulus->setRange(0, 1e12); m_spbBulkModulus->setDecimals(2); m_spbBulkModulus->setButtonSymbols(QAbstractSpinBox::NoButtons);
        foldFluid->addRow(new QLabel("密度 (kg/m³):"), m_spbFluidDensity);
        foldFluid->addRow(new QLabel("动力粘度 (Pa·s):"), m_spbFluidViscosity);
        foldFluid->addRow(new QLabel("体积弹性模量 (Pa):"), m_spbBulkModulus);
        m_mainLayout->addWidget(foldFluid);

        // ==================== 端口与管道 ====================
        FoldSection* foldHyd = new FoldSection("端口与管道", this);
        m_tabHydraulic = new QTabWidget(this);
        m_tablePorts = new QTableWidget(0, 4);
        setupFluidTable(m_tablePorts);
        m_tablePorts->setHorizontalHeaderLabels(QStringList() << "端口名称" << "类型" << "通径 (mm)" << "说明");
        m_tablePipes = new QTableWidget(0, 5);
        setupFluidTable(m_tablePipes);
        m_tablePipes->setHorizontalHeaderLabels(QStringList() << "管段编号" << "起点端口" << "终点端口" << "长度 (m)" << "直径 (mm)");
        m_tabHydraulic->addTab(m_tablePorts, "液压端口");
        m_tabHydraulic->addTab(m_tablePipes, "管路连接");
        foldHyd->addRow(m_tabHydraulic);
        m_mainLayout->addWidget(foldHyd);

        m_mainLayout->addStretch();

        connect(m_btnBrowseModel, &QToolButton::clicked, this, &mMotionHydraulicPanel::slot_browseModelFile);
    }

    mMotionHydraulicPanel::~mMotionHydraulicPanel() {}

    void mMotionHydraulicPanel::newHydraulicPanel()
    {
        setEditing(true);
        connectSig();

        int newId = getCVM->getModelData()->getManager<mMotionHydraulicManager>(Physical_MotionHydraulic).createID();
        m_data = new mMotionHydraulic(newId);
        m_data->setName(getCVM->getModelData()->createNameByType(Physical_MotionHydraulic, "液压元件"));
        getCVM->getModelData()->addPhysicalField<mMotionHydraulic>(m_data);

        // 默认UI
        m_edtName->setText(m_data->getName());
        m_cmbHydraulicType->setCurrentIndex(0);

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionHydraulicSig(true, m_data->getId());
    }

    void mMotionHydraulicPanel::editHydraulicPanel(int id)
    {
        setEditing(true);
        m_data = getCVM->getModelData()->getManager<mMotionHydraulicManager>(Physical_MotionHydraulic).getData(id);
        if (!m_data)
            return;

        connectSig();

        // 简单反显
        m_edtName->setText(m_data->getName());
        m_cmbHydraulicType->setCurrentIndex(static_cast<int>(m_data->getHydraulicType()));
    }

    //数据对象即将被删除（树删除前调用）：置空编辑指针防悬垂（周期边界同款约定）
    void mMotionHydraulicPanel::onMotionHydraulicDataAboutToBeDestroyed(int id)
    {
        if (id >= 0)
        {
            if (m_data == nullptr || id != m_data->getId())
                return;
        }
        m_data = nullptr;
    }

    void mMotionHydraulicPanel::uneditHydraulicPanel()
    {
        if (!isEditing())
            return;
        setEditing(false);
        m_data = nullptr;
        disconnectSig();
    }
    void mMotionHydraulicPanel::connectSig() {}
    void mMotionHydraulicPanel::disconnectSig() {}

    void mMotionHydraulicPanel::setupFluidTable(QTableWidget* table)
    {
        table->horizontalHeader()->setStretchLastSection(true);
        table->setMinimumHeight(120);
    }

    void mMotionHydraulicPanel::slot_browseModelFile()
    {
        QString fn = QFileDialog::getOpenFileName(this, "选择液压模型文件", QString(), "AMESim (*.ame);;Simulink (*.slx);;All Files (*)");
        if (!fn.isEmpty()) m_edtModelFile->setText(fn);
    }
    void mMotionHydraulicPanel::slot_browseParamFile() {}
}
