#include "mMotionElectroExportPanel.h"
#include "mMechGlobalSignals.h"
#include "mAppConfiguration.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mMotionElectroExportManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QFileDialog>

using namespace MechMotionData;
using namespace MxFunctions;

namespace MechMainWindow
{
    mMotionElectroExportPanel::mMotionElectroExportPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        m_mainLayout = new QVBoxLayout(this);
        m_mainLayout->setContentsMargins(0, 0, 0, 0);
        m_mainLayout->setSpacing(4);

        // ==================== 基础信息 ====================
        FoldSection* foldBasic = new FoldSection("基础信息", this);

        m_edtName = new QLineEdit();
        m_edtName->setPlaceholderText("导出配置名称");
        foldBasic->addRow(new QLabel("名称:"), m_edtName);

        m_cmbExportType = new QComboBox();
        m_cmbExportType->addItems({ "Simulink 导出", "FMU 导出", "通用接口导出" });
        foldBasic->addRow(new QLabel("导出类型:"), m_cmbExportType);

        QHBoxLayout* layFile = new QHBoxLayout();
        layFile->setContentsMargins(0, 0, 0, 0);
        m_edtPortListFile = new QLineEdit();
        m_edtPortListFile->setPlaceholderText("选择端口列表文件...");
        m_btnBrowseFile = new QToolButton();
        m_btnBrowseFile->setText("...");
        m_btnBrowseFile->setFixedWidth(30);
        layFile->addWidget(m_edtPortListFile);
        layFile->addWidget(m_btnBrowseFile);
        foldBasic->addRow(new QLabel("端口列表:"), layFile);

        m_mainLayout->addWidget(foldBasic);

        // ==================== 端口配置 ====================
        FoldSection* foldPorts = new FoldSection("端口配置", this);
        m_tabPorts = new QTabWidget(this);
        m_tableInputPorts = new QTableWidget(0, 5);
        setupPortTable(m_tableInputPorts);
        m_tableInputPorts->setHorizontalHeaderLabels(QStringList() << "序号" << "端口名称" << "缩放系数" << "偏移量" << "单位");
        m_tableOutputPorts = new QTableWidget(0, 5);
        setupPortTable(m_tableOutputPorts);
        m_tableOutputPorts->setHorizontalHeaderLabels(QStringList() << "序号" << "端口名称" << "缩放系数" << "偏移量" << "单位");
        m_tabPorts->addTab(m_tableInputPorts, "输入端口");
        m_tabPorts->addTab(m_tableOutputPorts, "输出端口");
        foldPorts->addRow(m_tabPorts);
        m_mainLayout->addWidget(foldPorts);

        m_mainLayout->addStretch();

        connect(m_btnBrowseFile, &QToolButton::clicked, this, &mMotionElectroExportPanel::slot_browsePortListFile);
    }

    mMotionElectroExportPanel::~mMotionElectroExportPanel() {}

    void mMotionElectroExportPanel::newElectroExportPanel()
    {
        setEditing(true);
        connectSig();

        int newId = getCVM->getModelData()->getManager<mMotionElectroExportManager>(Physical_MotionElectroExport).createID();
        m_data = new mMotionElectroExport(newId);
        m_data->setName(getCVM->getModelData()->createNameByType(Physical_MotionElectroExport, "机电导出"));
        getCVM->getModelData()->addPhysicalField<mMotionElectroExport>(m_data);

        // 默认UI
        m_edtName->setText(m_data->getName());
        m_cmbExportType->setCurrentIndex(0);

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionElectroExportSig(true, m_data->getId());
    }

    void mMotionElectroExportPanel::editElectroExportPanel(int id)
    {
        setEditing(true);
        m_data = getCVM->getModelData()->getManager<mMotionElectroExportManager>(Physical_MotionElectroExport).getData(id);
        if (!m_data)
            return;

        connectSig();

        // 简单反显
        m_edtName->setText(m_data->getName());
        m_cmbExportType->setCurrentIndex(static_cast<int>(m_data->getExportType()));
    }

    //数据对象即将被删除（树删除前调用）：置空编辑指针防悬垂（周期边界同款约定）
    void mMotionElectroExportPanel::onMotionElectroExportDataAboutToBeDestroyed(int id)
    {
        if (id >= 0)
        {
            if (m_data == nullptr || id != m_data->getId())
                return;
        }
        m_data = nullptr;
    }

    void mMotionElectroExportPanel::uneditElectroExportPanel()
    {
        if (!isEditing())
            return;
        setEditing(false);
        m_data = nullptr;
        disconnectSig();
    }
    void mMotionElectroExportPanel::connectSig() {}
    void mMotionElectroExportPanel::disconnectSig() {}

    void mMotionElectroExportPanel::setupPortTable(QTableWidget* table)
    {
        table->horizontalHeader()->setStretchLastSection(true);
        table->setMinimumHeight(120);
    }

    void mMotionElectroExportPanel::slot_browsePortListFile()
    {
        QString fn = QFileDialog::getOpenFileName(this, "选择端口列表文件", QString(), "Text Files (*.txt *.csv);;All Files (*)");
        if (!fn.isEmpty()) m_edtPortListFile->setText(fn);
    }
}
