#include "mMotionAutoFlexPanel.h"
#include "mMotionRefMap.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#ifndef MX_MOTION_OCC_COMPAT
#include "mInfoWindow.h"
#else
#include "mGetSetTypeTemplate.h"
#include "mMeshData.h"
#endif
#include "mMotionFlexBridge.h"
#include "mMotionFlexMxExport.h"
#include "mGeneralPickUp.h"
#include "mPickWidgetController.h"
#include "PickCommonFunction.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QSet>
#include <QDir>
#include <QRegularExpression>
#include <QProcess>
#include <QThread>
#include <QDateTime>
#include <QCoreApplication>
#include <QTextCodec>
#include <QTimer>
#include <algorithm>
#include <set>

#include "mModelData.h"
#include "mMotionAutoFlex.h"
#include "mMotionAutoFlexManager.h"
#include "mMotionBody.h"
#include "mMotionBodyManager.h"
#include "mMaterialManager.h"
#include "mMaterialBase.h"

#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QToolButton>
#include <QMessageBox>
#include <QLabel>
#include <QPushButton>

using namespace MechPreData;
using namespace MechMotionData;
using namespace MxFunctions;

namespace MechMainWindow {

    mMotionAutoFlexPanel::mMotionAutoFlexPanel(QWidget* parent)
        : mPanelBase(parent)
        , m_flexData(nullptr)
        , m_mainLayout(nullptr)
    {
        m_mainLayout = new QVBoxLayout(this);

        // 输入框统一去上下箭头（美观：纯输入，滚轮/键盘仍可调值）
        auto noButtons = [](QDoubleSpinBox* sb) { sb->setButtonSymbols(QAbstractSpinBox::NoButtons); return sb; };

        // ==================== 1. 基础信息 ====================
        FoldSection* foldBasic = new FoldSection("基础信息", this);

        m_cmbTargetBody = new QComboBox();
        foldBasic->addRow(new PanelLabel("目标刚体:"), m_cmbTargetBody);

        m_cmbMaterial = new QComboBox();

        //【附着节点·拾取控件】同边界/集合面板的 mGeneralPickUp 通用控件：
        //节点 ID 列表可见、可增删；留空时"生成mx/保存"自动按运动副收组并回显
        m_attachPickWidget = mGeneralPickUp::createPickWidget(CBBlue);
        m_attachPickWidget->setDefultTip(QString::fromUtf8("附着节点（留空=自动按运动副收组）"));
        m_attachPickWidget->setPickFilter({ (int)MxFunctions::PickDataType::Node });
#ifdef MX_MOTION_OCC_COMPAT
        m_attachPickWidget->setAllowedTypes(&MechCommon::pickOnlyNode);
        connect(m_attachPickWidget,&mPickWidgetController::dataChanged,this,[this] {
            setProperty("mxFlexAttachmentSelectionEdited",true);
        });
#endif
        connect(m_attachPickWidget, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
#ifdef MX_MOTION_OCC_COMPAT
            if(isActive && getCVM) m_attachPickWidget->setAllowedParts(mMotionRefMap::getSet(
                Physical_MotionBody,m_cmbTargetBody->currentData().toInt(),Physical_Part));
#endif
            if (isActive)
                emit MechCommon::mMechGlobalSignals::getInstance()->activePickSig(5);   // 节点拾取模式
            });
        foldBasic->addRow(new PanelLabel("附着节点:"), m_attachPickWidget->widget());

        m_chkEnabled = new QCheckBox("启用");
        m_chkEnabled->setChecked(true);
        foldBasic->addRow(new PanelLabel("状态:"), m_chkEnabled);

        m_mainLayout->addWidget(foldBasic);

        // ==================== 1.5 材料（独立折叠框，按行排列） ====================
        {
            FoldSection* foldMaterial = new FoldSection(QString::fromUtf8("材料"), this);
            //【材料接通+直输】下拉选材（空模型自动播种默认钢）；E/ν/ρ 直输框绑定所选
            // 材料：选材自动带出参数，可直接改，保存时写回材料对象（材料自带持久化）
            m_cmbMaterial->setToolTip(QString::fromUtf8("柔体材料（各向同性）。下方参数可直接修改，保存时写回所选材料"));
            foldMaterial->addRow(new PanelLabel(QString::fromUtf8("选择材料:")), m_cmbMaterial);

            m_spbMatE = new MxFunctions::mCalcLineEdit(
                "210000", MxFunctions::mCalcLineEdit::CalcMode::NoEmptyAndPositive, this);
            m_spbMatE->setToolTip(QString::fromUtf8("弹性模量 E，单位 MPa（默认钢 210000，支持表达式）"));
            foldMaterial->addRow(new PanelLabel(QString::fromUtf8("弹性模量 MPa:")), m_spbMatE);

            m_spbMatNu = new MxFunctions::mCalcLineEdit(
                "0.3", MxFunctions::mCalcLineEdit::CalcMode::NoEmptyAndPositive, this);
            m_spbMatNu->setToolTip(QString::fromUtf8("泊松比 ν（钢 0.3，须 <0.5）"));
            foldMaterial->addRow(new PanelLabel(QString::fromUtf8("泊松比:")), m_spbMatNu);

            m_spbMatRho = new MxFunctions::mCalcLineEdit(
                "7.85e-6", MxFunctions::mCalcLineEdit::CalcMode::NoEmptyAndPositive, this);
            m_spbMatRho->setToolTip(QString::fromUtf8("密度 ρ，单位 kg/mm³（钢 7.85e-6，支持表达式）"));
            foldMaterial->addRow(new PanelLabel(QString::fromUtf8("密度 kg/mm³:")), m_spbMatRho);

            // 切换材料 → 直输框带出该材料参数
            connect(m_cmbMaterial, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
                loadMaterialPropsToSpins();
            });
            m_mainLayout->addWidget(foldMaterial);
        }

        // ==================== 2. 网格参数 ====================
        FoldSection* foldMesh = new FoldSection("网格参数", this);

        m_spbElementSize = new QDoubleSpinBox();
        m_spbElementSize->setRange(0.01, 1e6);
        m_spbElementSize->setDecimals(3);
        m_spbElementSize->setValue(5.0);
        m_spbElementSize->setSuffix(" mm");
        foldMesh->addRow(new PanelLabel("单元尺寸:"), noButtons(m_spbElementSize));

        m_cmbMeshType = new QComboBox();
        m_cmbMeshType->addItem("线性", static_cast<int>(FlexMeshType::Linear));
        m_cmbMeshType->addItem("二次", static_cast<int>(FlexMeshType::Quadratic));
        m_cmbMeshType->setVisible(false);   //【暂隐】当前固定 tet4 线性（Tet10 升级在遗留清单）
        m_cmbMeshType->setParent(this);   //【暂隐】未接通：不进布局（不留空标签）

        m_spbMinElementSize = new QDoubleSpinBox();
        m_spbMinElementSize->setRange(0.001, 1e6);
        m_spbMinElementSize->setDecimals(3);
        m_spbMinElementSize->setValue(1.0);
        m_spbMinElementSize->setSuffix(" mm");
        m_spbMinElementSize->setVisible(false);   //【暂隐】自动剖分只消费单元尺寸
        m_spbMinElementSize->setParent(this);   //【暂隐】未接通

        m_chkCurvatureRefinement = new QCheckBox("启用");
        m_chkCurvatureRefinement->setChecked(true);
        m_chkCurvatureRefinement->setVisible(false);   //【暂隐】未接通
        m_chkCurvatureRefinement->setParent(this);   //【暂隐】未接通

        m_mainLayout->addWidget(foldMesh);

        // ==================== 3. 模态分析参数 ====================
        FoldSection* foldModal = new FoldSection("模态分析参数", this);

        m_spbModeCount = new QSpinBox();
        m_spbModeCount->setRange(1, 1000);
        m_spbModeCount->setValue(10);
        m_spbModeCount->setButtonSymbols(QAbstractSpinBox::NoButtons);
        foldModal->addRow(new PanelLabel("模态数:"), m_spbModeCount);

        m_spbFreqCutoff = new QDoubleSpinBox();
        m_spbFreqCutoff->setRange(0.0, 1e6);
        m_spbFreqCutoff->setDecimals(2);
        m_spbFreqCutoff->setValue(0.0);   // 0=不截断（默认；此前 100 会静默砍掉高阶模态）
        m_spbFreqCutoff->setSuffix(" Hz");
        m_spbFreqCutoff->setToolTip(QString::fromUtf8("丢弃高于该频率的模态（缩减前截断）。0=不截断；低于一阶模态会报错"));
        foldModal->addRow(new PanelLabel("截断频率:"), noButtons(m_spbFreqCutoff));

        m_cmbSolverType = new QComboBox();
        m_cmbSolverType->addItem("Lanczos", static_cast<int>(FlexSolverType::Lanczos));
        m_cmbSolverType->addItem("子空间迭代", static_cast<int>(FlexSolverType::Subspace));
        m_cmbSolverType->setVisible(false);   //【暂隐】求解器自动选路（小问题稠密/大问题子空间迭代），不消费此选项
        m_cmbSolverType->setParent(this);   //【暂隐】自动选路

        m_spbFreqLower = new QDoubleSpinBox();
        m_spbFreqLower->setRange(0.0, 1e6);
        m_spbFreqLower->setDecimals(2);
        m_spbFreqLower->setValue(0.0);
        m_spbFreqLower->setSuffix(" Hz");
        m_spbFreqLower->setVisible(false);   //【暂隐】未接通
        m_spbFreqLower->setParent(this);   //【暂隐】未接通

        m_spbFreqUpper = new QDoubleSpinBox();
        m_spbFreqUpper->setRange(0.0, 1e6);
        m_spbFreqUpper->setDecimals(2);
        m_spbFreqUpper->setValue(1e6);
        m_spbFreqUpper->setSuffix(" Hz");
        m_spbFreqUpper->setVisible(false);   //【暂隐】未接通
        m_spbFreqUpper->setParent(this);   //【暂隐】未接通

        m_chkRigidBodyModeFilter = new QCheckBox("启用");
        m_chkRigidBodyModeFilter->setChecked(true);
        m_chkRigidBodyModeFilter->setVisible(false);   //【暂隐】CB 夹持基天然无刚体模态，无意义
        m_chkRigidBodyModeFilter->setParent(this);   //【暂隐】CB 夹持基天然无刚体模态

        m_mainLayout->addWidget(foldModal);

        // ==================== 4. 界面连接参数 ====================
        FoldSection* foldConn = new FoldSection("界面连接参数", this);

        m_cmbConnectionType = new QComboBox();
        m_cmbConnectionType->addItem("RBE2 (刚性)", static_cast<int>(FlexConnectionType::RBE2));
        m_cmbConnectionType->addItem("RBE3 (柔性)", static_cast<int>(FlexConnectionType::RBE3));
        m_cmbConnectionType->addItem("焊缝连接", static_cast<int>(FlexConnectionType::Weld));
        m_cmbConnectionType->setVisible(false);   //【暂隐】固定用点焊接焊（RBE2 语义）
        m_cmbConnectionType->setParent(this);   //【暂隐】固定点焊接焊

        m_edtInterfaceDofs = new QLineEdit("X,Y,Z,RX,RY,RZ");
        m_edtInterfaceDofs->setPlaceholderText("如: X,Y,Z,RX,RY,RZ");
        m_edtInterfaceDofs->setVisible(false);   //【暂隐】接口固定全 6 自由度焊接
        m_edtInterfaceDofs->setParent(this);   //【暂隐】固定全 6 自由度焊接

        m_spbConnectionRadius = new QDoubleSpinBox();
        m_spbConnectionRadius->setRange(0.0, 1e6);
        m_spbConnectionRadius->setDecimals(2);
        m_spbConnectionRadius->setValue(0.0);   // 0=自动（按 3x 单元平均边长收节点）
        m_spbConnectionRadius->setSuffix(" mm");
        m_spbConnectionRadius->setToolTip(QString::fromUtf8("0=自动：按 3x 单元平均边长自动收连接节点；大于 0 时按给定半径收"));
        foldConn->addRow(new PanelLabel("影响半径:"), noButtons(m_spbConnectionRadius));

        m_mainLayout->addWidget(foldConn);

        // ==================== 5. 输出控制 ====================
        FoldSection* foldOutput = new FoldSection("输出控制", this);

        m_chkOutputModeShapes = new QCheckBox("输出");
        m_chkOutputModeShapes->setChecked(true);
        foldOutput->addRow(new PanelLabel("模态振型:"), m_chkOutputModeShapes);

        m_chkOutputStressStrain = new QCheckBox("输出");
        m_chkOutputStressStrain->setChecked(true);
        foldOutput->addRow(new PanelLabel("应力/应变:"), m_chkOutputStressStrain);

        m_cmbOutputFormat = new QComboBox();
        m_cmbOutputFormat->addItem("MNF (ADAMS)", static_cast<int>(FlexOutputFormat::MNFF));
        m_cmbOutputFormat->addItem("RST (ANSYS)", static_cast<int>(FlexOutputFormat::RST));
        m_cmbOutputFormat->addItem("OP2 (NASTRAN)", static_cast<int>(FlexOutputFormat::OP2));
        foldOutput->addRow(new PanelLabel("输出格式:"), m_cmbOutputFormat);

        //【暂隐】输出控制整组：MNF/RST/OP2 导出与应力应变恢复未实现（遗留清单）
        foldOutput->setVisible(false);
        m_mainLayout->addWidget(foldOutput);

        // ==================== 6. 阻尼参数 ====================
        //折叠区与参数行在 setupDampingUI 构建；参数行不在此挂载，
        //由 slot_dampingTypeChanged 按当前类型动态 addRow（见其注释）
        setupDampingUI();

        // 阻尼类型切换
        connect(m_cmbDampingType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mMotionAutoFlexPanel::slot_dampingTypeChanged);
        //【刚柔耦合】目标刚体切换：预览其体网格表面（无网格时清预览）
        connect(m_cmbTargetBody, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
            const int bodyId = m_cmbTargetBody->currentData().toInt();
#ifdef MX_MOTION_OCC_COMPAT
            const auto selected = attachNodeIdsFromWidget();
            showAttachNodesInWidget(std::vector<int>(selected.begin(), selected.end()));
#endif
            if (m_cmbTargetBody->currentIndex()<0 || bodyId < 0 || !mMotionFlexBridge::previewFlexBodyMesh(getCVM ? getCVM->getModelData() : nullptr, bodyId, QStringLiteral("__flex_preview__"))) {
                mMotionFlexBridge::clearFlexBodyPreview();
            }
            if (getCVM && getCVM->getPreRenderWidget())
                getCVM->getPreRenderWidget()->updatePaint();
        });
        slot_dampingTypeChanged(0);

        // ==================== 7. 操作按钮 ====================
        QWidget* btnWidget = new QWidget(this);
        QHBoxLayout* btnLayout = new QHBoxLayout(btnWidget);
        btnLayout->setContentsMargins(0, 0, 0, 0);

        //【MNF 离线缩减】生成 mx：当前条目 → 结构求解器输入（CMS/CB + mnf 输出），
        //求解器跑 mx 得 .mnf 后供多体侧导入（离线路径与求解时在线缩减并存）
        QPushButton* mxBtn = new QPushButton(QString::fromUtf8("导出mnf"));
        mxBtn->setToolTip(QString::fromUtf8("导出本条目为 .mx（体网格+材料+附着接口+CB 缩减设置），\n交结构隐式求解器生成 .mnf 柔性体中性文件"));
        QPushButton* saveBtn = new QPushButton("保存");
        QPushButton* cancelBtn = new QPushButton("取消");
        btnLayout->addWidget(mxBtn);
        btnLayout->addStretch();
        btnLayout->addWidget(saveBtn);
        btnLayout->addWidget(cancelBtn);

        m_mainLayout->addWidget(btnWidget);
        m_mainLayout->addStretch();

        connect(mxBtn, &QPushButton::clicked, this, &mMotionAutoFlexPanel::slot_exportMx);
        connect(saveBtn, &QPushButton::clicked, this, &mMotionAutoFlexPanel::slot_save);
        connect(cancelBtn, &QPushButton::clicked, this, &mMotionAutoFlexPanel::slot_cancel);
    }

    mMotionAutoFlexPanel::~mMotionAutoFlexPanel()
    {
        if (m_mainLayout != nullptr)
        {
            delete m_mainLayout;
            m_mainLayout = nullptr;
        }
    }

    void mMotionAutoFlexPanel::newMotionAutoFlexPanel()
    {
        setEditing(true);
#ifdef MX_MOTION_OCC_COMPAT
        m_attachPickWidget->setPickActive(false);
        showAttachNodesInWidget({});
#endif
        m_flexData = nullptr;
        refreshBodyList();
        refreshMaterialList();

        const QSignalBlocker b1(m_cmbTargetBody); m_cmbTargetBody->setCurrentIndex(0);
        {
            const QSignalBlocker b2(m_cmbMaterial); m_cmbMaterial->setCurrentIndex(0);
        }
        loadMaterialPropsToSpins();   //【材料直输】新建默认带出默认钢参数
        m_chkEnabled->setChecked(true);

        // 网格参数
        const QSignalBlocker b4(m_spbElementSize); m_spbElementSize->setValue(5.0);
        const QSignalBlocker b5(m_cmbMeshType); m_cmbMeshType->setCurrentIndex(0);
        const QSignalBlocker b6(m_spbMinElementSize); m_spbMinElementSize->setValue(1.0);
        m_chkCurvatureRefinement->setChecked(true);

        // 模态分析参数
        const QSignalBlocker b7(m_spbModeCount); m_spbModeCount->setValue(10);
        const QSignalBlocker b8(m_spbFreqCutoff); m_spbFreqCutoff->setValue(0.0);   // 0=不截断
        const QSignalBlocker b9(m_cmbSolverType); m_cmbSolverType->setCurrentIndex(0);
        const QSignalBlocker b10(m_spbFreqLower); m_spbFreqLower->setValue(0.0);
        const QSignalBlocker b11(m_spbFreqUpper); m_spbFreqUpper->setValue(1e6);
        m_chkRigidBodyModeFilter->setChecked(true);

        // 界面连接参数
        const QSignalBlocker b12(m_cmbConnectionType); m_cmbConnectionType->setCurrentIndex(0);
        const QSignalBlocker b13(m_edtInterfaceDofs); m_edtInterfaceDofs->setText("X,Y,Z,RX,RY,RZ");
        const QSignalBlocker b14(m_spbConnectionRadius); m_spbConnectionRadius->setValue(10.0);

        // 输出控制
        m_chkOutputModeShapes->setChecked(true);
        m_chkOutputStressStrain->setChecked(true);
        const QSignalBlocker b15(m_cmbOutputFormat); m_cmbOutputFormat->setCurrentIndex(0);

        // 阻尼参数（默认=验证配置 Rayleigh(10, 2e-4)；combo 索引=枚举值：0无/1模态比/2瑞利）
        const QSignalBlocker b16(m_cmbDampingType); m_cmbDampingType->setCurrentIndex(2);
        const QSignalBlocker b17(m_spbModalDampingRatio); m_spbModalDampingRatio->setValue(0.02);
        const QSignalBlocker b18(m_spbRayleighMass); m_spbRayleighMass->setValue(10.0);
        const QSignalBlocker b19(m_spbRayleighStiffness); m_spbRayleighStiffness->setValue(2.0e-4);
        slot_dampingTypeChanged(2);

        connectSig();

        int flexID = getCVM->getModelData()->getManager<mMotionAutoFlexManager>(Physical_MotionAutoFlex).createID();
        m_flexData = new mMotionAutoFlex(flexID);
        m_flexData->setName(getCVM->getModelData()->createNameByType(Physical_MotionAutoFlex, "自动柔性体"));
        getCVM->getModelData()->addPhysicalField<mMotionAutoFlex>(m_flexData);

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionAutoFlexSig(true, m_flexData->getId());
    }

    void mMotionAutoFlexPanel::editMotionAutoFlexPanel(int id)
    {
        setEditing(true);
        if (!getCVM) return;
        m_flexData = getCVM->getModelData()->getManager<mMotionAutoFlexManager>(Physical_MotionAutoFlex).getData(id);
        if (!m_flexData) return;

        refreshBodyList();
        refreshMaterialList();
        updateUIFromData();
        connectSig();
    }

    //数据对象即将被删除（树删除前调用）：置空编辑指针防悬垂（周期边界同款约定）
    void mMotionAutoFlexPanel::onMotionAutoFlexDataAboutToBeDestroyed(int id)
    {
        if (id >= 0)
        {
            if (m_flexData == nullptr || id != m_flexData->getId())
                return;
        }
        m_flexData = nullptr;
#ifdef MX_MOTION_OCC_COMPAT
        m_attachPickWidget->detachActive();
        showAttachNodesInWidget({});
#endif
    }

    void mMotionAutoFlexPanel::uneditMotionAutoFlexPanel()
    {
        if (!isEditing())
            return;
#ifdef MX_MOTION_OCC_COMPAT
        m_attachPickWidget->setPickActive(false);
        showAttachNodesInWidget({});
#endif
        setEditing(false);
        disconnectSig();
        m_flexData = nullptr;
    }

    void mMotionAutoFlexPanel::updateUIFromData()
    {
        if (!m_flexData) return;

        int bodyIndex = m_cmbTargetBody->findData(mMotionRefMap::get(Physical_MotionAutoFlex, m_flexData->getId(), Physical_MotionBody));
        const QSignalBlocker b1(m_cmbTargetBody);
        m_cmbTargetBody->setCurrentIndex(bodyIndex != -1 ? bodyIndex : 0);

        int matIndex = m_cmbMaterial->findData(m_flexData->getMaterialId());
        {
            const QSignalBlocker b2(m_cmbMaterial);
            m_cmbMaterial->setCurrentIndex(matIndex != -1 ? matIndex : 0);
        }
        loadMaterialPropsToSpins();   //【材料直输】带出所选材料参数

        QVector<int> pts = m_flexData->getAttachPoints();
        showAttachNodesInWidget(std::vector<int>(pts.begin(), pts.end()));

        m_chkEnabled->setChecked(m_flexData->isEnabled());

        // 网格参数
        const QSignalBlocker b4(m_spbElementSize);
        m_spbElementSize->setValue(m_flexData->getElementSize());

        int meshIdx = m_cmbMeshType->findData(static_cast<int>(m_flexData->getMeshType()));
        const QSignalBlocker b5(m_cmbMeshType);
        m_cmbMeshType->setCurrentIndex(meshIdx != -1 ? meshIdx : 0);

        const QSignalBlocker b6(m_spbMinElementSize);
        m_spbMinElementSize->setValue(m_flexData->getMinElementSize());

        m_chkCurvatureRefinement->setChecked(m_flexData->getCurvatureRefinement());

        // 模态分析参数
        const QSignalBlocker b7(m_spbModeCount);
        m_spbModeCount->setValue(m_flexData->getModeCount());

        const QSignalBlocker b8(m_spbFreqCutoff);
        m_spbFreqCutoff->setValue(m_flexData->getFreqCutoff());

        int solverIdx = m_cmbSolverType->findData(static_cast<int>(m_flexData->getSolverType()));
        const QSignalBlocker b9(m_cmbSolverType);
        m_cmbSolverType->setCurrentIndex(solverIdx != -1 ? solverIdx : 0);

        const QSignalBlocker b10(m_spbFreqLower);
        m_spbFreqLower->setValue(m_flexData->getFreqLower());

        const QSignalBlocker b11(m_spbFreqUpper);
        m_spbFreqUpper->setValue(m_flexData->getFreqUpper());

        m_chkRigidBodyModeFilter->setChecked(m_flexData->getRigidBodyModeFilter());

        // 界面连接参数
        int connIdx = m_cmbConnectionType->findData(static_cast<int>(m_flexData->getConnectionType()));
        const QSignalBlocker b12(m_cmbConnectionType);
        m_cmbConnectionType->setCurrentIndex(connIdx != -1 ? connIdx : 0);

        // 自由度位掩码转字符串
        int dofs = m_flexData->getInterfaceDofs();
        QStringList dofList;
        if (dofs & 0x01) dofList << "X";
        if (dofs & 0x02) dofList << "Y";
        if (dofs & 0x04) dofList << "Z";
        if (dofs & 0x08) dofList << "RX";
        if (dofs & 0x10) dofList << "RY";
        if (dofs & 0x20) dofList << "RZ";
        const QSignalBlocker b13(m_edtInterfaceDofs);
        m_edtInterfaceDofs->setText(dofList.join(","));

        const QSignalBlocker b14(m_spbConnectionRadius);
        m_spbConnectionRadius->setValue(m_flexData->getConnectionRadius());

        // 输出控制
        m_chkOutputModeShapes->setChecked(m_flexData->getOutputModeShapes());
        m_chkOutputStressStrain->setChecked(m_flexData->getOutputStressStrain());

        int fmtIdx = m_cmbOutputFormat->findData(static_cast<int>(m_flexData->getOutputFormat()));
        const QSignalBlocker b15(m_cmbOutputFormat);
        m_cmbOutputFormat->setCurrentIndex(fmtIdx != -1 ? fmtIdx : 0);

        // 阻尼参数
        int dampIdx = m_cmbDampingType->findData(static_cast<int>(m_flexData->getDampingType()));
        const QSignalBlocker b16(m_cmbDampingType);
        m_cmbDampingType->setCurrentIndex(dampIdx != -1 ? dampIdx : 0);
        slot_dampingTypeChanged(dampIdx != -1 ? dampIdx : 0);

        const QSignalBlocker b17(m_spbModalDampingRatio);
        m_spbModalDampingRatio->setValue(m_flexData->getModalDampingRatio());

        const QSignalBlocker b18(m_spbRayleighMass);
        m_spbRayleighMass->setValue(m_flexData->getRayleighMass());

        const QSignalBlocker b19(m_spbRayleighStiffness);
        m_spbRayleighStiffness->setValue(m_flexData->getRayleighStiffness());
    }

    void mMotionAutoFlexPanel::refreshBodyList()
    {
        if (!getCVM) return;
        m_cmbTargetBody->clear();
        auto& mgr = getCVM->getModelData()->getManager<mMotionBodyManager>(Physical_MotionBody);
        for (int id : mgr.getAllIDs()) {
            auto* body = mgr.getData(id);
            if (body) {
                m_cmbTargetBody->addItem(body->getName(), id);
            }
        }
    }

    void mMotionAutoFlexPanel::loadMaterialPropsToSpins()
    {
        //【材料直输】所选材料的 E/ν/ρ → 直输框；缺属性/无选中 → 内置钢默认
        double E = 2.1e5, nu = 0.3, rho = 7.85e-6;
        const int matId = m_cmbMaterial->currentData().toInt();
        if (matId >= 0 && m_cmbMaterial->currentIndex()>=0 && getCVM) {
            auto& matMgr = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material);
            if (auto mat = matMgr.getMaterialData(matId)) {
                auto all = mat->getAllProps();
                for (auto itC = all.constBegin(); itC != all.constEnd(); ++itC) {
                    const auto& props = itC.value();
                    if (props.contains(MxFunctions::MatProp_YoungsModulus)) {
                        const QVariant v = props.value(MxFunctions::MatProp_YoungsModulus);
                        if (v.isValid() && v.canConvert<double>()) E = v.toDouble();
                    }
                    if (props.contains(MxFunctions::MatProp_Botson)) {
                        const QVariant v = props.value(MxFunctions::MatProp_Botson);
                        if (v.isValid() && v.canConvert<double>()) nu = v.toDouble();
                    }
                    if (props.contains(MxFunctions::MatProp_Density)) {
                        const QVariant v = props.value(MxFunctions::MatProp_Density);
                        if (v.isValid() && v.canConvert<double>()) rho = v.toDouble();
                    }
                }
            }
        }
        const QSignalBlocker b1(m_spbMatE); m_spbMatE->setText(QString::number(E));
        const QSignalBlocker b2(m_spbMatNu); m_spbMatNu->setText(QString::number(nu));
        const QSignalBlocker b3(m_spbMatRho); m_spbMatRho->setText(QString::number(rho));
    }

    void mMotionAutoFlexPanel::refreshMaterialList()
    {
        if (!getCVM) return;
        m_cmbMaterial->clear();
        //【材料直输】去掉"无"：材料必选（空模型自动播种默认钢），参数可直接改并写回
        auto& matMgr = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material);
        //多体形态下"材料"功能页不可达（刚柔耦合一站式闭环），模型尚无任何材料时
        //兜底种子一个默认各向同性材料，避免下拉仅有"无"。构造方式与
        //mMaterialPanel::createMaterial 一致（空属性，参数仍由用户在面板中填写）
        if (matMgr.getAllMatID().empty())
        {
            const int matID = matMgr.createID();
            auto mat = std::make_shared<MechMaterialData::mMaterialBase>(
                matID, MxFunctions::LinearElastic_Isotropic, QString::fromUtf8("默认材料"));
            //【刚柔耦合】默认钢参数（口径与质量补算 calcCalculatePartMass 一致：
            // mm-MPa-kg/mm³：E=2.1e5 MPa、nu=0.3、rho=7.85e-6 kg/mm³），
            // 否则空属性材料会让柔性体生成在材料取值处失败。
            mat->addProperty(MxFunctions::MAT_Isotropy, MxFunctions::MatProp_YoungsModulus, 2.1e5);
            mat->addProperty(MxFunctions::MAT_Isotropy, MxFunctions::MatProp_Botson, 0.3);
            mat->addProperty(MxFunctions::MAT_Isotropy, MxFunctions::MatProp_Density, 7.85e-6);
            getCVM->getModelData()->addPhysicalFieldP(mat);
            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMaterialSig(true, mat->getMaterialId());
        }
        for (int id : matMgr.getAllMatID()) {
            auto mat = matMgr.getMaterialData(id);
            if (mat) {
                m_cmbMaterial->addItem(mat->getMaterialName(), id);
            }
        }
    }

    //【附着节点】拾取控件 → 全局节点 ID 集合
    std::set<int> mMotionAutoFlexPanel::attachNodeIdsFromWidget() const
    {
        std::set<int> ids;
        if (!m_attachPickWidget)
            return ids;
        const PickWidgetData& all = m_attachPickWidget->getAllData();
        for (auto cit = all.pickedData.begin(); cit != all.pickedData.end(); ++cit) {
            if (cit->first != MxFunctions::PickDataType::Node)
                continue;
            for (const auto& r : cit->second)
                if (r.result_id > 0)
                    ids.insert(r.result_id);
        }
        return ids;
    }

    //【附着节点】全局 ID 列表 → 拾取控件回显（同时同步兼容行编辑供旧序列化路径）
    void mMotionAutoFlexPanel::showAttachNodesInWidget(const std::vector<int>& globalIds)
    {
        if (!m_attachPickWidget)
            return;
        PickWidgetData data;
        std::vector<RenderPickResultData> items;
        items.reserve(globalIds.size());
#ifdef MX_MOTION_OCC_COMPAT
        std::map<int, int> nodeParts;
        std::set<int> parts;
        if (getCVM && m_cmbTargetBody->currentIndex() >= 0) {
            parts = mMotionRefMap::getSet(Physical_MotionBody,
                m_cmbTargetBody->currentData().toInt(), Physical_Part);
            auto* model = getCVM->getModelData();
            const auto* mesh = model ? model->getMeshData() : nullptr;
            if (mesh && !globalIds.empty()) {
                const std::set<int> selected(globalIds.begin(), globalIds.end());
                for (int part : parts) {
                    const auto ids = mesh->nodeIds().find(part);
                    if (ids == mesh->nodeIds().end()) continue;
                    for (int node : ids->second)
                        if (selected.count(node)) nodeParts.emplace(node, part);
                }
            }
        }
        if (parts != m_attachPickWidget->allowedParts())
            m_attachPickWidget->setAllowedParts(std::move(parts));
#endif
        for (int id : globalIds) {
            RenderPickResultData r;
            r.result_id = id;
#ifdef MX_MOTION_OCC_COMPAT
            if (const auto part = nodeParts.find(id); part != nodeParts.end())
                r.part_id = part->second;
#endif
            items.push_back(r);
        }
        data.pickedData[MxFunctions::PickDataType::Node] = std::move(items);
#ifdef MX_MOTION_OCC_COMPAT
        m_attachPickWidget->displayData(data, true);
#else
        m_attachPickWidget->displayData(data, false);
#endif
        if (m_edtAttachPoints) {
            QStringList lst;
            for (int id : globalIds)
                lst << QString::number(id);
            m_edtAttachPoints->setText(lst.join(','));
        }
    }

    //【自动附着·GUI 数据侧】ensureTetraMesh 兜底剖分 + 运动副 origin 容差收组，
    //全局 ID 平铺回显控件并落库（attachPoints）；导出/保存共用
    bool mMotionAutoFlexPanel::autoAttachFromJoints(std::vector<int>& outGlobalIds)
    {
        outGlobalIds.clear();
        if (!getCVM || !m_flexData)
            return false;
        auto* modelData = getCVM->getModelData();
        const int bodyId = m_cmbTargetBody->currentData().toInt();
        if (!modelData || m_cmbTargetBody->currentIndex()<0 || bodyId < 0)
            return false;

        auto& relMgr = modelData->getMapRelationManager();
        std::set<int> partIds = relMgr.getMapRelation<std::set<int>>({ Physical_MotionBody, bodyId }, Physical_Part, MxFunctions::MapRelation_None);
        std::vector<double> coords;
        std::vector<int> tets;
        std::map<int, int> g2l;
        auto log = [](const QString&) {};
        if (!mMotionFlexBridge::collectBodyTetraMesh(modelData, partIds, m_spbElementSize->value(), coords, tets, g2l, log))
            return false;

        const auto pts = mMotionFlexBridge::collectJointAttachPointsGui(modelData, bodyId);
        if (pts.empty())
            return false;

        //容差 = 连接半径>0 ? 半径 : 3×平均单元边长；每个副点收最近 16 节点
        double tol = m_spbConnectionRadius->value();
        if (tol <= 0) {
            double sum = 0; int cnt = 0;
            const int nT = (int)tets.size() / 4;
            for (int t = 0; t < nT && t < 200; ++t) {
                const int n0 = tets[4 * t];
                for (int k = 1; k < 4; ++k) {
                    const int nk = tets[4 * t + k];
                    const double dx = coords[3 * n0] - coords[3 * nk], dy = coords[3 * n0 + 1] - coords[3 * nk + 1], dz = coords[3 * n0 + 2] - coords[3 * nk + 2];
                    sum += std::sqrt(dx * dx + dy * dy + dz * dz);
                    ++cnt;
                }
            }
            tol = cnt > 0 ? 3.0 * sum / cnt : 0.0;
        }
        const int nNodes = (int)coords.size() / 3;
        std::set<int> selectedLocal;
        for (size_t a = 0; a + 2 < pts.size(); a += 3) {
            std::vector<int> group;
            for (int i = 0; i < nNodes; ++i) {
                const double dx = coords[3 * i] - pts[a], dy = coords[3 * i + 1] - pts[a + 1], dz = coords[3 * i + 2] - pts[a + 2];
                if (dx * dx + dy * dy + dz * dz <= tol * tol)
                    group.push_back(i);
            }
            if ((int)group.size() > 16) {
                std::sort(group.begin(), group.end(), [&](int lhs, int rhs) {
                    const double dl = (coords[3 * lhs] - pts[a]) * (coords[3 * lhs] - pts[a]) + (coords[3 * lhs + 1] - pts[a + 1]) * (coords[3 * lhs + 1] - pts[a + 1]) + (coords[3 * lhs + 2] - pts[a + 2]) * (coords[3 * lhs + 2] - pts[a + 2]);
                    const double dr = (coords[3 * rhs] - pts[a]) * (coords[3 * rhs] - pts[a]) + (coords[3 * rhs + 1] - pts[a + 1]) * (coords[3 * rhs + 1] - pts[a + 1]) + (coords[3 * rhs + 2] - pts[a + 2]) * (coords[3 * rhs + 2] - pts[a + 2]);
                    return dl < dr;
                });
                group.resize(16);
            }
            for (int l : group)
                selectedLocal.insert(l);
        }
        if (selectedLocal.empty())
            return false;

        //局部 → 全局（界面上拾取/显示的口径）
        std::map<int, int> l2g;
        for (const auto& kv : g2l)
            l2g[kv.second] = kv.first;
        for (int l : selectedLocal) {
            auto found = l2g.find(l);
            if (found != l2g.end())
                outGlobalIds.push_back(found->second);
        }
        std::sort(outGlobalIds.begin(), outGlobalIds.end());
        showAttachNodesInWidget(outGlobalIds);
        QVector<int> att;
        for (int v : outGlobalIds) att.push_back(v);
        m_flexData->setAttachPoints(att);
        mGlobalSignals::getInstance()->outputMessageSig(
            QString::fromUtf8("[自动柔性体] 自动附着：%1 节点（%2 个副点，容差 %3 mm）——已回显到附着节点列表，可增删")
                .arg((int)outGlobalIds.size()).arg(pts.size() / 3).arg(tol, 0, 'f', 1));
        return true;
    }

    //【MNF 离线缩减】当前条目 → .mx → 自动调结构求解器出 .mnf。
    // 导出取面板实时值（不要求先保存）：目标刚体/材料/模态数/网格参数先同步进数据对象；
    // 附着点手选优先，留空时按运动副作用点自动分组（与求解期自动附着同语义）
    void mMotionAutoFlexPanel::slot_exportMx()
    {
        if (!getCVM || !m_flexData)
            return;
        auto* modelData = getCVM->getModelData();
        if (!modelData)
            return;

        //面板实时值 → 数据对象（照 slot_save 的字段写法，最小集）
        const int bodyId = m_cmbTargetBody->currentData().toInt();
        if (m_cmbTargetBody->currentIndex()<0 || bodyId < 0) {
            mGlobalSignals::getInstance()->outputMessageSig(
                QString::fromUtf8("[MNF] 请先选择目标刚体"), true);
            return;
        }
        mMotionRefMap::set(Physical_MotionAutoFlex, m_flexData->getId(), Physical_MotionBody, MapRelation_None, bodyId);
        m_flexData->setMaterialId(m_cmbMaterial->currentData().toInt());
        //【材料直输】E/ν/ρ 回写所选材料对象（桥接按材料属性取值）
        {
            const int matId = m_flexData->getMaterialId();
            if (matId >= 0) {
                auto& matMgr = modelData->getManager<MechMaterialData::mMaterialManager>(Physical_Material);
                if (auto mat = matMgr.getMaterialData(matId)) {
                    mat->addProperty(MxFunctions::MAT_Isotropy, MxFunctions::MatProp_YoungsModulus, m_spbMatE->text().toDouble());
                    mat->addProperty(MxFunctions::MAT_Isotropy, MxFunctions::MatProp_Botson, m_spbMatNu->text().toDouble());
                    mat->addProperty(MxFunctions::MAT_Isotropy, MxFunctions::MatProp_Density, m_spbMatRho->text().toDouble());
                }
            }
        }
        m_flexData->setElementSize(m_spbElementSize->value());
        m_flexData->setModeCount(m_spbModeCount->value());
        m_flexData->setConnectionRadius(m_spbConnectionRadius->value());
        //附着节点：拾取控件优先；空则自动按运动副收组并回显控件（autoAttachFromJoints
        //内已落库 attachPoints），再进入收集
        {
            std::set<int> ids = attachNodeIdsFromWidget();
            if (ids.empty()) {
                std::vector<int> autoIds;
                if (!autoAttachFromJoints(autoIds)) {
                    mGlobalSignals::getInstance()->outputMessageSig(
                        QString::fromUtf8("[MNF] 未手选附着节点，且自动附着失败（目标刚体上无运动副或体网格不可用）"), true);
                    return;
                }
            }
            else {
                QVector<int> attachList;
                for (int v : ids) attachList.push_back(v);
                m_flexData->setAttachPoints(attachList);
            }
        }

        //collectFlexInputs 只收 enabled 条目：临时启用，导出后恢复原状态
        const bool wasEnabled = m_flexData->isEnabled();
        if (!wasEnabled)
            m_flexData->setEnabled(true);
        auto inputs = mMotionFlexBridge::collectFlexInputs(nullptr, modelData,
            [](const QString&) {});
        if (!wasEnabled)
            m_flexData->setEnabled(false);

        auto it = std::find_if(inputs.begin(), inputs.end(),
            [this](const mMotionFlexBridge::FlexInput& f) { return f.flexId == m_flexData->getId(); });
        if (it == inputs.end()) {
            mGlobalSignals::getInstance()->outputMessageSig(
                QString::fromUtf8("[MNF] 条目收集失败：目标刚体未设置或体网格不可用"), true);
            return;
        }
        if (!it->valid) {
            mGlobalSignals::getInstance()->outputMessageSig(
                QString::fromUtf8("[MNF] %1：%2").arg(it->name).arg(it->error), true);
            return;
        }

        //文件名 <条目名>.mx（路径非法字符替换），落模型工作目录
        QString name = m_flexData->getName();
        name.replace(QRegularExpression(QString::fromUtf8("[\\\\/:*?\"<>|\\s]")), "_");
        const QString fileName = name.isEmpty()
            ? QString("flex_%1.mx").arg(m_flexData->getId())
            : name + ".mx";
        const QString path = QDir(MAppConfiguration::mAppConfiguration::getInstance()->_workingDir)
                                 .filePath(fileName);

        QString err;
        if (mMotionFlexMxExport::exportMxForMnf(*it, path, &err)) {
            mGlobalSignals::getInstance()->outputMessageSig(
                QString::fromUtf8("[MNF] mx 已生成：%1（节点 %2 / 单元 %3 / 接口组 %4 / CB %5 阶），启动结构求解器...")
                    .arg(path).arg((int)it->nodeCoords.size() / 3).arg((int)it->tetIds.size() / 4)
                    .arg((int)it->attachGroups.size()).arg(it->modeCount));
            launchMnfSolver(path);
        } else {
            mGlobalSignals::getInstance()->outputMessageSig(
                QString::fromUtf8("[MNF] mx 生成失败：%1").arg(err), true);
        }
    }

    //【MNF 一键】拉起结构求解器（与结构求解任务中心同款调用：
    //mxlibi/MxSim.exe -i <mx> -t <核数> -nosplash）。stdout/stderr 逐段回显信息窗，
    //结束后在 mx 同目录按 <基名>.mnf / <基名>_mx.mnf / 目录最新 .mnf 定位结果并报告
    void mMotionAutoFlexPanel::launchMnfSolver(const QString& mxPath)
    {
        if (m_mnfProc) {
            mGlobalSignals::getInstance()->outputMessageSig(
                QString::fromUtf8("[MNF] 求解器已在运行（mx：%1），完成后才能再次启动").arg(m_mnfMxPath), true);
            return;
        }
        const QString solver = QDir(QCoreApplication::applicationDirPath() + "/mxlibi/MxSim.exe").absoluteFilePath(QString());
        if (!QFile::exists(solver)) {
            mGlobalSignals::getInstance()->outputMessageSig(
                QString::fromUtf8("[MNF] 未找到结构求解器：%1（mx 已生成，可手动求解生成 mnf）").arg(solver), true);
            return;
        }

        m_mnfProc = new QProcess(this);
        m_mnfMxPath = mxPath;
        const QDateTime startedAt = QDateTime::currentDateTime();

        //求解器 stdout 为 UTF-8（msg 带 BOM），GBK 输出兜底——与结构任务中心
        //toUnicodeText 同款先试 UTF-8/非法字符回退 GBK，避免信息窗乱码
        auto decodeSolverText = [](const QByteArray& raw) {
            QTextCodec::ConverterState state;
            QString text = QTextCodec::codecForName("UTF-8")->toUnicode(raw.constData(), raw.size(), &state);
            if (state.invalidChars > 0)
                text = QTextCodec::codecForName("GBK")->toUnicode(raw);
            return text;
        };
        connect(m_mnfProc, &QProcess::readyReadStandardOutput, this, [this, decodeSolverText]() {
            mGlobalSignals::getInstance()->outputMessageSig(
                QString::fromUtf8("[MNF] ") + decodeSolverText(m_mnfProc->readAllStandardOutput()));
        });
        connect(m_mnfProc, &QProcess::readyReadStandardError, this, [this, decodeSolverText]() {
            mGlobalSignals::getInstance()->outputMessageSig(
                QString::fromUtf8("[MNF] ") + decodeSolverText(m_mnfProc->readAllStandardError()));
        });
        connect(m_mnfProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, startedAt](int exitCode, QProcess::ExitStatus status) {
                const QString mxPath = m_mnfMxPath;
                m_mnfProc->deleteLater();
                m_mnfProc = nullptr;
                m_mnfMxPath.clear();
                if (exitCode != 0 || status != QProcess::NormalExit) {
                    mGlobalSignals::getInstance()->outputMessageSig(
                        QString::fromUtf8("[MNF] 结构求解器异常退出（exit=%1），mnf 未生成").arg(exitCode), true);
                    return;
                }
                //定位 mnf：优先基名约定，兜底目录内启动后新生成的最新 .mnf
                const QFileInfo fi(mxPath);
                const QString dir = fi.absolutePath();
                const QString base = fi.completeBaseName();
                QString mnf;
                for (const QString& cand : { base + ".mnf", base + "_mx.mnf", base + ".mx.mnf" }) {
                    const QString p = QDir(dir).filePath(cand);
                    if (QFile::exists(p) && QFileInfo(p).lastModified() >= startedAt) {
                        mnf = p;
                        break;
                    }
                }
                if (mnf.isEmpty()) {
                    QDir d(dir);
                    const auto entries = d.entryInfoList({ "*.mnf" }, QDir::Files, QDir::Time);
                    for (const auto& e : entries) {
                        if (e.lastModified() >= startedAt) { mnf = e.absoluteFilePath(); break; }
                    }
                }
                if (!mnf.isEmpty()) {
                    mGlobalSignals::getInstance()->outputMessageSig(
                        QString::fromUtf8("[MNF] mnf 生成完成：%1（可导入多体求解）").arg(mnf));
                } else {
                    mGlobalSignals::getInstance()->outputMessageSig(
                        QString::fromUtf8("[MNF] 求解器正常退出，但在 %1 未找到新生成的 .mnf（检查求解器输出日志）").arg(dir), true);
                }
            });
        connect(m_mnfProc, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart) {
                mGlobalSignals::getInstance()->outputMessageSig(
                    QString::fromUtf8("[MNF] 结构求解器启动失败"), true);
                m_mnfProc->deleteLater();
                m_mnfProc = nullptr;
                m_mnfMxPath.clear();
            }
        });

        m_mnfProc->setWorkingDirectory(QFileInfo(mxPath).absolutePath());
        const int cores = qBound(1, QThread::idealThreadCount(), 8);
        m_mnfProc->start(solver, { "-i", QDir::toNativeSeparators(mxPath),
                                   "-t", QString::number(cores), "-nosplash" });
        mGlobalSignals::getInstance()->outputMessageSig(
            QString::fromUtf8("[MNF] 结构求解器已启动（%1 核）：%2").arg(cores).arg(mxPath));
    }

    void mMotionAutoFlexPanel::slot_save()
    {
        if (!getCVM) return;

        bool isNew = (m_flexData == nullptr);
        int id;
        QString name;

        if (isNew) {
            id = getCVM->getModelData()->getManager<mMotionAutoFlexManager>(Physical_MotionAutoFlex).createID();
            name = QString("自动柔性体_%1").arg(id);
            int suffix = 1;
            while (getCVM->getModelData()->isExistName(Physical_MotionAutoFlex, name)) {
                name = QString("自动柔性体_%1_%2").arg(id).arg(suffix++);
            }
            m_flexData = new mMotionAutoFlex(id, name);
            getCVM->getModelData()->getManager<mMotionAutoFlexManager>(Physical_MotionAutoFlex).addData(m_flexData);
        }
        else {
            id = m_flexData->getId();
            name = m_flexData->getName();
        }

        // 基础信息
        mMotionRefMap::set(Physical_MotionAutoFlex, m_flexData->getId(), Physical_MotionBody, MapRelation_None, m_cmbTargetBody->currentData().toInt());
        m_flexData->setMaterialId(m_cmbMaterial->currentData().toInt());
        //【材料直输】E/ν/ρ 写回所选材料对象（材料自带持久化；桥接按材料属性取值）
        {
            const int matId = m_flexData->getMaterialId();
            if (matId >= 0 && getCVM) {
                auto& matMgr = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material);
                if (auto mat = matMgr.getMaterialData(matId)) {
                    mat->addProperty(MxFunctions::MAT_Isotropy, MxFunctions::MatProp_YoungsModulus, m_spbMatE->text().toDouble());
                    mat->addProperty(MxFunctions::MAT_Isotropy, MxFunctions::MatProp_Botson, m_spbMatNu->text().toDouble());
                    mat->addProperty(MxFunctions::MAT_Isotropy, MxFunctions::MatProp_Density, m_spbMatRho->text().toDouble());
                }
            }
        }

        //附着节点：拾取控件优先；空则自动按运动副收组并回显（保存时也生成自动连接，
        //求解/导出可直接复用落库结果）
        {
            std::set<int> ids = attachNodeIdsFromWidget();
            if (ids.empty()) {
                std::vector<int> autoIds;
                autoAttachFromJoints(autoIds);   // 失败静默：求解期仍有引擎侧自动附着兜底
                for (int v : autoIds)
                    ids.insert(v);
            }
            QVector<int> pts;
            for (int v : ids) pts.push_back(v);
            m_flexData->setAttachPoints(pts);
        }
        m_flexData->setEnabled(m_chkEnabled->isChecked());

        // 网格参数
        m_flexData->setElementSize(m_spbElementSize->value());
        m_flexData->setMeshType(static_cast<FlexMeshType>(m_cmbMeshType->currentData().toInt()));
        m_flexData->setMinElementSize(m_spbMinElementSize->value());
        m_flexData->setCurvatureRefinement(m_chkCurvatureRefinement->isChecked());

        // 模态分析参数
        m_flexData->setModeCount(m_spbModeCount->value());
        m_flexData->setFreqCutoff(m_spbFreqCutoff->value());
        m_flexData->setSolverType(static_cast<FlexSolverType>(m_cmbSolverType->currentData().toInt()));
        m_flexData->setFreqLower(m_spbFreqLower->value());
        m_flexData->setFreqUpper(m_spbFreqUpper->value());
        m_flexData->setRigidBodyModeFilter(m_chkRigidBodyModeFilter->isChecked());

        // 界面连接参数
        m_flexData->setConnectionType(static_cast<FlexConnectionType>(m_cmbConnectionType->currentData().toInt()));

        // 自由度字符串转位掩码
        QString dofStr = m_edtInterfaceDofs->text().trimmed().toUpper();
        int dofs = 0;
        if (dofStr.contains("X")) dofs |= 0x01;
        if (dofStr.contains("Y")) dofs |= 0x02;
        if (dofStr.contains("Z")) dofs |= 0x04;
        if (dofStr.contains("RX")) dofs |= 0x08;
        if (dofStr.contains("RY")) dofs |= 0x10;
        if (dofStr.contains("RZ")) dofs |= 0x20;
        m_flexData->setInterfaceDofs(dofs);
        m_flexData->setConnectionRadius(m_spbConnectionRadius->value());

        // 输出控制
        m_flexData->setOutputModeShapes(m_chkOutputModeShapes->isChecked());
        m_flexData->setOutputStressStrain(m_chkOutputStressStrain->isChecked());
        m_flexData->setOutputFormat(static_cast<FlexOutputFormat>(m_cmbOutputFormat->currentData().toInt()));

        // 阻尼参数
        m_flexData->setDampingType(static_cast<FlexDampingType>(m_cmbDampingType->currentData().toInt()));
        m_flexData->setModalDampingRatio(m_spbModalDampingRatio->value());
        m_flexData->setRayleighMass(m_spbRayleighMass->value());
        m_flexData->setRayleighStiffness(m_spbRayleighStiffness->value());

#ifdef MX_MOTION_OCC_COMPAT
        if(getCVM->commitAutoFlexAttachments) getCVM->commitAutoFlexAttachments(id,
            property("mxFlexAttachmentSelectionEdited").toBool());
        setProperty("mxFlexAttachmentSelectionEdited",false);
#endif
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionAutoFlexSig(isNew, id);
        //【刚柔耦合】保存即网格化（UG 式向导语义）：无体网格则按单元尺寸自动剖分，
        // 成功后立即预览网格。剖分日志吐消息栏。
        {
            const int savedBodyId = m_cmbTargetBody->currentData().toInt();
            std::set<int> savedPartIds = mMotionRefMap::getSet(Physical_MotionBody, savedBodyId, Physical_Part);
            const bool meshed = mMotionFlexBridge::ensureTetraMesh(
                getCVM ? getCVM->getModelData() : nullptr, savedPartIds,
                m_flexData->getElementSize(), [](const QString& m) {
                    mGlobalSignals::getInstance()->outputMessageSig(0, m, true);
                });
            if (meshed) {
                mMotionFlexBridge::previewFlexBodyMesh(getCVM ? getCVM->getModelData() : nullptr,
                                                       savedBodyId, QStringLiteral("__flex_preview__"));
                if (getCVM && getCVM->getPreRenderWidget())
                    getCVM->getPreRenderWidget()->updatePaint();
            }
        }

    }

    void mMotionAutoFlexPanel::slot_cancel()
    {
#ifdef MX_MOTION_OCC_COMPAT
        if(getCVM && getCVM->closePanel)getCVM->closePanel();
#else
        mInfoWindow::getInstance()->unEditInfoWindow();
#endif
    }

    void mMotionAutoFlexPanel::connectSig()
    {
        disconnectSig();
        // 拾取功能暂留接口，当前无需渲染连接
    }

    void mMotionAutoFlexPanel::disconnectSig()
    {
        if (conn_geo) disconnect(conn_geo);
    }

    void mMotionAutoFlexPanel::slot_finishedPick()
    {
        // 拾取功能预留
    }

    void mMotionAutoFlexPanel::slot_subWindowActivated(QMdiSubWindow* subWindow)
    {
    }

    //阻尼参数折叠区：阻尼类型行常驻；模态/瑞利参数行建好即 setParent(nullptr)
    //（不挂布局），由 slot_dampingTypeChanged 按类型动态 addRow 挂回。
    //瑞利 α（质量）/β（刚度）各占一行，不再挤同一行
    void mMotionAutoFlexPanel::setupDampingUI()
    {
        m_foldDamping = new FoldSection(QString::fromUtf8("阻尼参数"), this);
        m_mainLayout->addWidget(m_foldDamping);

        m_cmbDampingType = new QComboBox();
        m_cmbDampingType->addItem(QString::fromUtf8("无阻尼"), static_cast<int>(FlexDampingType::None));
        m_cmbDampingType->addItem(QString::fromUtf8("模态阻尼"), static_cast<int>(FlexDampingType::Modal));
        m_cmbDampingType->addItem(QString::fromUtf8("瑞利阻尼"), static_cast<int>(FlexDampingType::Rayleigh));
        m_foldDamping->addRow(new PanelLabel(QString::fromUtf8("阻尼类型:")), m_cmbDampingType);

        // 模态阻尼比（一行）
        m_lblModalDampingRatio = new QLabel(QString::fromUtf8("阻尼比:"));
        m_spbModalDampingRatio = new QDoubleSpinBox();
        m_spbModalDampingRatio->setRange(0.0, 1.0);
        m_spbModalDampingRatio->setDecimals(4);
        m_spbModalDampingRatio->setValue(0.02);
        m_spbModalDampingRatio->setButtonSymbols(QAbstractSpinBox::NoButtons);

        // 瑞利阻尼两行（默认=曲柄摇杆柔性连杆验证值：α=10/β=2e-4，16.5Hz 低阶模态 ~6% 阻尼比）
        m_lblRayleighMass = new QLabel(QString::fromUtf8("α (质量):"));
        m_spbRayleighMass = new QDoubleSpinBox();
        m_spbRayleighMass->setRange(0.0, 1e6);
        m_spbRayleighMass->setDecimals(4);
        m_spbRayleighMass->setValue(10.0);
        m_spbRayleighMass->setToolTip(QString::fromUtf8("质量比例阻尼 α：补低频（ζ=α/2ω）。机构耦合的低阶模态需要 ~5% 量级阻尼压制能量注入"));
        m_spbRayleighMass->setButtonSymbols(QAbstractSpinBox::NoButtons);

        m_lblRayleighStiff = new QLabel(QString::fromUtf8("β (刚度):"));
        m_spbRayleighStiffness = new QDoubleSpinBox();
        m_spbRayleighStiffness->setRange(0.0, 1e6);
        m_spbRayleighStiffness->setDecimals(6);
        m_spbRayleighStiffness->setValue(2.0e-4);
        m_spbRayleighStiffness->setToolTip(QString::fromUtf8("刚度比例阻尼 β：阻高频（ζ=βω/2）。默认 2e-4"));
        m_spbRayleighStiffness->setButtonSymbols(QAbstractSpinBox::NoButtons);

        //与运动副面板同款：建好即脱离父对象，挂载交给类型切换槽
        m_lblModalDampingRatio->setParent(nullptr);
        m_spbModalDampingRatio->setParent(nullptr);
        m_lblRayleighMass->setParent(nullptr);
        m_spbRayleighMass->setParent(nullptr);
        m_lblRayleighStiff->setParent(nullptr);
        m_spbRayleighStiffness->setParent(nullptr);
    }

    //阻尼类型切换：参数行整行动态摘挂（运动副面板 ensurePairInLayout 同款：
    //addRow 挂回 / setParent(nullptr) 摘下，QFormLayout 隐藏行仍占位故不用 setVisible）
    void mMotionAutoFlexPanel::slot_dampingTypeChanged(int index)
    {
        Q_UNUSED(index);
        auto ensurePairInLayout = [](QLabel* label, QWidget* field, bool shouldShow, FoldSection* section) {
            if (shouldShow) {
                if (label->parent() == nullptr || field->parent() == nullptr) {
                    section->addRow(label, field);
                }
            }
            else {
                if (label->parent() != nullptr) {
                    label->setParent(nullptr);
                }
                if (field->parent() != nullptr) {
                    field->setParent(nullptr);
                }
            }
            };

        const int type = m_cmbDampingType->currentData().toInt();
        const bool isModal = (type == static_cast<int>(FlexDampingType::Modal));
        const bool isRayleigh = (type == static_cast<int>(FlexDampingType::Rayleigh));
        ensurePairInLayout(m_lblModalDampingRatio, m_spbModalDampingRatio, isModal, m_foldDamping);
        //瑞利两行：α（质量）/β（刚度）
        ensurePairInLayout(m_lblRayleighMass, m_spbRayleighMass, isRayleigh, m_foldDamping);
        ensurePairInLayout(m_lblRayleighStiff, m_spbRayleighStiffness, isRayleigh, m_foldDamping);
    }

}
