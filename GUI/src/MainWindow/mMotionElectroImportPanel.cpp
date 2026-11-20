#include "mMotionElectroImportPanel.h"
#include "mMechGlobalSignals.h"
#include "mAppConfiguration.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mMotionElectroImportManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QFileDialog>

using namespace MechMotionData;
using namespace MxFunctions;

namespace MechMainWindow
{
    mMotionElectroImportPanel::mMotionElectroImportPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        m_mainLayout = new QVBoxLayout(this);
        m_mainLayout->setContentsMargins(0, 0, 0, 0);
        m_mainLayout->setSpacing(4);

        // ==================== 基础信息 ====================
        FoldSection* foldBasic = new FoldSection("基础信息", this);

        m_cmbCoSimType = new QComboBox();
        m_cmbCoSimType->addItems({ "Simulink", "FMU", "自定义接口" });
        foldBasic->addRow(new QLabel("耦合类型:"), m_cmbCoSimType);

        m_edtPlatform = new QLineEdit("Simulink");
        m_edtPlatform->setReadOnly(true);
        foldBasic->addRow(new QLabel("外部平台:"), m_edtPlatform);

        // 模型文件行
        QHBoxLayout* layModel = new QHBoxLayout();
        layModel->setContentsMargins(0, 0, 0, 0);
        m_edtModelFile = new QLineEdit();
        m_edtModelFile->setPlaceholderText("选择模型文件...");
        m_btnBrowseModel = new QToolButton();
        m_btnBrowseModel->setText("...");
        m_btnBrowseModel->setFixedWidth(30);
        layModel->addWidget(m_edtModelFile);
        layModel->addWidget(m_btnBrowseModel);
        foldBasic->addRow(new QLabel("模型文件:"), layModel);

        // 头文件行
        QHBoxLayout* layHeader = new QHBoxLayout();
        layHeader->setContentsMargins(0, 0, 0, 0);
        m_edtHeaderFile = new QLineEdit();
        m_edtHeaderFile->setPlaceholderText("选择头文件...");
        m_btnBrowseHeader = new QToolButton();
        m_btnBrowseHeader->setText("...");
        m_btnBrowseHeader->setFixedWidth(30);
        layHeader->addWidget(m_edtHeaderFile);
        layHeader->addWidget(m_btnBrowseHeader);
        foldBasic->addRow(new QLabel("头文件:"), layHeader);

        m_mainLayout->addWidget(foldBasic);

        // ==================== 端口配置 ====================
        FoldSection* foldPorts = new FoldSection("端口配置", this);
        m_tabPorts = new QTabWidget(this);
        m_tableInputPorts = new QTableWidget(0, 4);
        setupPortTable(m_tableInputPorts);
        m_tableInputPorts->setHorizontalHeaderLabels(QStringList() << "端口名称" << "数据类型" << "维度" << "说明");
        m_tableOutputPorts = new QTableWidget(0, 4);
        setupPortTable(m_tableOutputPorts);
        m_tableOutputPorts->setHorizontalHeaderLabels(QStringList() << "端口名称" << "数据类型" << "维度" << "说明");
        m_tabPorts->addTab(m_tableInputPorts, "输入端口");
        m_tabPorts->addTab(m_tableOutputPorts, "输出端口");
        foldPorts->addRow(m_tabPorts);
        m_mainLayout->addWidget(foldPorts);

        m_mainLayout->addStretch();

        // ==================== 信号绑定 ====================
        connect(m_btnBrowseModel, &QToolButton::clicked, this, &mMotionElectroImportPanel::slot_browseModelFile);
        connect(m_btnBrowseHeader, &QToolButton::clicked, this, &mMotionElectroImportPanel::slot_browseHeaderFile);
        connect(m_cmbCoSimType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mMotionElectroImportPanel::slot_coSimTypeChanged);
    }

    mMotionElectroImportPanel::~mMotionElectroImportPanel() {}

    void mMotionElectroImportPanel::newElectroImportPanel()
    {
        setEditing(true);
        connectSig();

        int newId = getCVM->getModelData()->getManager<mMotionElectroImportManager>(Physical_MotionElectroImport).createID();
        m_data = new mMotionElectroImport(newId);
        m_data->setName(getCVM->getModelData()->createNameByType(Physical_MotionElectroImport, "机电导入"));
        getCVM->getModelData()->addPhysicalField<mMotionElectroImport>(m_data);

        // 默认UI
        m_cmbCoSimType->setCurrentIndex(0);
        m_edtPlatform->setText("Simulink");

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionElectroImportSig(true, m_data->getId());
    }

    void mMotionElectroImportPanel::editElectroImportPanel(int id)
    {
        setEditing(true);
        m_data = getCVM->getModelData()->getManager<mMotionElectroImportManager>(Physical_MotionElectroImport).getData(id);
        if (!m_data)
            return;

        connectSig();

        // 简单反显名称和类型
        m_cmbCoSimType->setCurrentIndex(static_cast<int>(m_data->getCoSimType()));
        QStringList platforms = { "Simulink", "FMU", "自定义接口" };
        int idx = static_cast<int>(m_data->getCoSimType());
        if (idx >= 0 && idx < platforms.size())
            m_edtPlatform->setText(platforms[idx]);
    }

    //数据对象即将被删除（树删除前调用）：置空编辑指针防悬垂（周期边界同款约定）
    void mMotionElectroImportPanel::onMotionElectroImportDataAboutToBeDestroyed(int id)
    {
        if (id >= 0)
        {
            if (m_data == nullptr || id != m_data->getId())
                return;
        }
        m_data = nullptr;
    }

    void mMotionElectroImportPanel::uneditElectroImportPanel()
    {
        if (!isEditing())
            return;
        setEditing(false);
        m_data = nullptr;
        disconnectSig();
    }
    void mMotionElectroImportPanel::connectSig() {}
    void mMotionElectroImportPanel::disconnectSig() {}

    void mMotionElectroImportPanel::setupPortTable(QTableWidget* table)
    {
        table->horizontalHeader()->setStretchLastSection(true);
        table->setMinimumHeight(120);
    }

    void mMotionElectroImportPanel::slot_browseModelFile()
    {
        QString fn = QFileDialog::getOpenFileName(this, "选择模型文件", QString(), "Model Files (*.slx *.mdl *.fmu);;All Files (*)");
        if (!fn.isEmpty()) m_edtModelFile->setText(fn);
    }
    void mMotionElectroImportPanel::slot_browseHeaderFile()
    {
        QString fn = QFileDialog::getOpenFileName(this, "选择头文件", QString(), "Header Files (*.h *.hpp);;All Files (*)");
        if (!fn.isEmpty()) m_edtHeaderFile->setText(fn);
    }
    void mMotionElectroImportPanel::slot_coSimTypeChanged(int index)
    {
        QStringList platforms = { "Simulink", "FMU", "自定义接口" };
        if (index >= 0 && index < platforms.size())
            m_edtPlatform->setText(platforms[index]);
    }
}
