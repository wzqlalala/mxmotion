#include "mMotionRLPanel.h"
#include "mMechGlobalSignals.h"
#include "mAppConfiguration.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mMotionRLManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>

using namespace MechMotionData;
using namespace MxFunctions;

namespace MechMainWindow
{
    mMotionRLPanel::mMotionRLPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        m_mainLayout = new QVBoxLayout(this);
        m_mainLayout->setContentsMargins(0, 0, 0, 0);
        m_mainLayout->setSpacing(4);

        // ==================== 任务定义 ====================
        FoldSection* foldTask = new FoldSection("任务定义", this);
        m_edtTaskName = new QLineEdit();
        m_edtTaskName->setPlaceholderText("强化学习任务名称");
        foldTask->addRow(new QLabel("任务名称:"), m_edtTaskName);

        m_cmbAlgorithm = new QComboBox();
        m_cmbAlgorithm->addItems({ "PPO", "SAC", "DDPG", "TD3", "A3C" });
        foldTask->addRow(new QLabel("算法:"), m_cmbAlgorithm);

        m_cmbEnvType = new QComboBox();
        m_cmbEnvType->addItems({ "单刚体", "多体系统", "柔性体", "刚柔耦合" });
        foldTask->addRow(new QLabel("环境类型:"), m_cmbEnvType);
        m_mainLayout->addWidget(foldTask);

        // ==================== 状态空间 ====================
        FoldSection* foldState = new FoldSection("状态空间 (观测变量)", this);
        m_tableStateSpace = new QTableWidget(0, 3);
        m_tableStateSpace->setHorizontalHeaderLabels(QStringList() << "变量名" << "维度" << "范围");
        m_tableStateSpace->horizontalHeader()->setStretchLastSection(true);
        m_tableStateSpace->setMinimumHeight(100);
        foldState->addRow(m_tableStateSpace);
        m_mainLayout->addWidget(foldState);

        // ==================== 动作空间 ====================
        FoldSection* foldAction = new FoldSection("动作空间 (控制变量)", this);
        m_tableActionSpace = new QTableWidget(0, 3);
        m_tableActionSpace->setHorizontalHeaderLabels(QStringList() << "变量名" << "维度" << "范围");
        m_tableActionSpace->horizontalHeader()->setStretchLastSection(true);
        m_tableActionSpace->setMinimumHeight(100);
        foldAction->addRow(m_tableActionSpace);
        m_mainLayout->addWidget(foldAction);

        // ==================== 网络参数 ====================
        FoldSection* foldNet = new FoldSection("网络参数", this);
        m_spbHiddenUnits = new QSpinBox(); m_spbHiddenUnits->setRange(8, 4096); m_spbHiddenUnits->setValue(256);
        m_spbNumLayers = new QSpinBox(); m_spbNumLayers->setRange(1, 10); m_spbNumLayers->setValue(3);
        m_spbLearningRate = new QDoubleSpinBox(); m_spbLearningRate->setRange(1e-6, 1.0); m_spbLearningRate->setDecimals(6); m_spbLearningRate->setValue(3e-4); m_spbLearningRate->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbDiscountFactor = new QDoubleSpinBox(); m_spbDiscountFactor->setRange(0.0, 1.0); m_spbDiscountFactor->setDecimals(4); m_spbDiscountFactor->setValue(0.99); m_spbDiscountFactor->setButtonSymbols(QAbstractSpinBox::NoButtons);
        foldNet->addRow(new QLabel("隐藏单元数:"), m_spbHiddenUnits);
        foldNet->addRow(new QLabel("网络层数:"), m_spbNumLayers);
        foldNet->addRow(new QLabel("学习率:"), m_spbLearningRate);
        foldNet->addRow(new QLabel("折扣因子 γ:"), m_spbDiscountFactor);
        m_mainLayout->addWidget(foldNet);

        // ==================== 训练参数 ====================
        FoldSection* foldTrain = new FoldSection("训练参数", this);
        m_spbMaxEpisodes = new QSpinBox(); m_spbMaxEpisodes->setRange(1, 1000000); m_spbMaxEpisodes->setValue(10000);
        m_spbMaxSteps = new QSpinBox(); m_spbMaxSteps->setRange(1, 1000000); m_spbMaxSteps->setValue(1000);
        m_spbRewardScale = new QDoubleSpinBox(); m_spbRewardScale->setRange(1e-6, 1e6); m_spbRewardScale->setDecimals(4); m_spbRewardScale->setValue(1.0); m_spbRewardScale->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_chkUseCurriculum = new QCheckBox("启用课程学习");
        foldTrain->addRow(new QLabel("最大回合数:"), m_spbMaxEpisodes);
        foldTrain->addRow(new QLabel("每回合最大步数:"), m_spbMaxSteps);
        foldTrain->addRow(new QLabel("奖励缩放:"), m_spbRewardScale);
        foldTrain->addRow(m_chkUseCurriculum);
        m_mainLayout->addWidget(foldTrain);

        // ==================== 路径与操作 ====================
        FoldSection* foldPath = new FoldSection("路径与操作", this);
        QHBoxLayout* layDataset = new QHBoxLayout();
        layDataset->setContentsMargins(0, 0, 0, 0);
        m_edtDatasetPath = new QLineEdit();
        m_edtDatasetPath->setPlaceholderText("数据集保存路径...");
        m_btnBrowseDataset = new QToolButton(); m_btnBrowseDataset->setText("..."); m_btnBrowseDataset->setFixedWidth(30);
        layDataset->addWidget(m_edtDatasetPath); layDataset->addWidget(m_btnBrowseDataset);
        foldPath->addRow(new QLabel("数据集路径:"), layDataset);

        QHBoxLayout* layModel = new QHBoxLayout();
        layModel->setContentsMargins(0, 0, 0, 0);
        m_edtSaveModelPath = new QLineEdit();
        m_edtSaveModelPath->setPlaceholderText("模型保存路径...");
        m_btnBrowseModelPath = new QToolButton(); m_btnBrowseModelPath->setText("..."); m_btnBrowseModelPath->setFixedWidth(30);
        layModel->addWidget(m_edtSaveModelPath); layModel->addWidget(m_btnBrowseModelPath);
        foldPath->addRow(new QLabel("模型保存路径:"), layModel);

        QHBoxLayout* layOps = new QHBoxLayout();
        m_btnStartTrain = new QToolButton(); m_btnStartTrain->setText("开始");
        m_btnStopTrain = new QToolButton(); m_btnStopTrain->setText("停止");
        layOps->addWidget(m_btnStartTrain);
        layOps->addWidget(m_btnStopTrain);
        foldPath->addRow(new QLabel("训练控制:"), layOps);

        m_progressTrain = new QProgressBar();
        m_progressTrain->setRange(0, 100);
        m_progressTrain->setValue(0);
        foldPath->addRow(new QLabel("训练进度:"), m_progressTrain);
        m_mainLayout->addWidget(foldPath);

        m_mainLayout->addStretch();

        // ==================== 信号绑定 ====================
        connect(m_btnBrowseDataset, &QToolButton::clicked, this, &mMotionRLPanel::slot_browseDatasetPath);
        connect(m_btnBrowseModelPath, &QToolButton::clicked, this, &mMotionRLPanel::slot_browseModelPath);
        connect(m_btnStartTrain, &QToolButton::clicked, this, &mMotionRLPanel::slot_startTraining);
        connect(m_btnStopTrain, &QToolButton::clicked, this, &mMotionRLPanel::slot_stopTraining);
    }

    mMotionRLPanel::~mMotionRLPanel() {}

    void mMotionRLPanel::newRLPanel()
    {
        setEditing(true);
        connectSig();

        int newId = getCVM->getModelData()->getManager<mMotionRLManager>(Physical_MotionRL).createID();
        m_data = new mMotionRL(newId);
        m_data->setName(getCVM->getModelData()->createNameByType(Physical_MotionRL, "强化学习任务"));
        getCVM->getModelData()->addPhysicalField<mMotionRL>(m_data);

        // 默认UI
        m_edtTaskName->setText(m_data->getName());
        m_cmbAlgorithm->setCurrentIndex(0);
        m_cmbEnvType->setCurrentIndex(0);

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionRLSig(true, m_data->getId());
    }

    void mMotionRLPanel::editRLPanel(int id)
    {
        setEditing(true);
        m_data = getCVM->getModelData()->getManager<mMotionRLManager>(Physical_MotionRL).getData(id);
        if (!m_data)
            return;

        connectSig();

        // 简单反显
        m_edtTaskName->setText(m_data->getName());
        m_cmbAlgorithm->setCurrentIndex(static_cast<int>(m_data->getAlgorithm()));
        m_cmbEnvType->setCurrentIndex(static_cast<int>(m_data->getEnvType()));
    }

    //数据对象即将被删除（树删除前调用）：置空编辑指针防悬垂（周期边界同款约定）
    void mMotionRLPanel::onMotionRLDataAboutToBeDestroyed(int id)
    {
        if (id >= 0)
        {
            if (m_data == nullptr || id != m_data->getId())
                return;
        }
        m_data = nullptr;
    }

    void mMotionRLPanel::uneditRLPanel()
    {
        if (!isEditing())
            return;
        setEditing(false);
        m_data = nullptr;
        disconnectSig();
    }
    void mMotionRLPanel::connectSig() {}
    void mMotionRLPanel::disconnectSig() {}

    void mMotionRLPanel::slot_browseDatasetPath()
    {
        QString dir = QFileDialog::getExistingDirectory(this, "选择数据集目录");
        if (!dir.isEmpty()) m_edtDatasetPath->setText(dir);
    }
    void mMotionRLPanel::slot_browseModelPath()
    {
        QString dir = QFileDialog::getExistingDirectory(this, "选择模型保存目录");
        if (!dir.isEmpty()) m_edtSaveModelPath->setText(dir);
    }
    void mMotionRLPanel::slot_startTraining()
    {
        m_progressTrain->setValue(0);
        //QMessageBox::information(this, "提示", "训练已开始（演示）");
    }
    void mMotionRLPanel::slot_stopTraining()
    {
        //QMessageBox::information(this, "提示", "训练已停止（演示）");
    }
}
