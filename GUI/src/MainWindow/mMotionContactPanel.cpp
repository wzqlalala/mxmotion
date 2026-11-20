#include "mMotionContactPanel.h"
#include "mMotionRefMap.h"
#include "mGlobalSignals.h"
#include "mMotionPanelColors.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "PickCommonFunction.h"

//BasicData
#include "mMotionBody.h"
#include "mMotionBodyManager.h"
#include "mMotionContact.h"
#include "mMotionContactManager.h"

//Geo
#include "mGeometricFeatureAttribute.h"

//Qt
#include <QComboBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
using namespace MechPreData;
using namespace MechMotionData;
using namespace mxr;

namespace MechMainWindow
{

    mMotionContactPanel::mMotionContactPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        m_motionContactData = nullptr;

        //获取当前视口的数据
        m_mainLayout = new QVBoxLayout(this);

        auto makePickBtn = [](const QString& iconPath, const QString& tip) {
            QToolButton* b = new QToolButton();
            b->setIcon(QIcon(iconPath));
            b->setToolTip(tip);
            b->setFixedWidth(30);
            return b;
            };

        auto makeSpinBox = [](double min, double max, int decimals) {
            QDoubleSpinBox* spb = new QDoubleSpinBox();
            spb->setRange(min, max);
            spb->setDecimals(decimals);
            spb->setButtonSymbols(QAbstractSpinBox::NoButtons);
            return spb;
            };

        // ==================== 1. 基础信息层 ====================
        FoldSection* foldBasic = new FoldSection("基础信息", this);foldBasic->setParent(nullptr);

        m_cmbContactType = new QComboBox();
        m_cmbContactType->addItems({ "CAD接触", "小面接触" });
        foldBasic->addRow(new QLabel("接触类型:"), m_cmbContactType);
        //m_mainLayout->addWidget(foldBasic);

        // ==================== 2. 接触对象层 ====================
        FoldSection* foldBodies = new FoldSection("接触对象", this);

        // QHBoxLayout* layAction = new QHBoxLayout();
        // layAction->setContentsMargins(0, 0, 0, 0);
        m_cmbActionBody = new QComboBox();
        // m_btnPickActionBody = makePickBtn(":/mMainWidgets/Icon/tihuan3x.png", "拾取动作体");
        // layAction->addWidget(m_cmbActionBody);
        // layAction->addWidget(m_btnPickActionBody);
        foldBodies->addRow(new QLabel("动作体:"), m_cmbActionBody);

        // QHBoxLayout* layBase = new QHBoxLayout();
        // layBase->setContentsMargins(0, 0, 0, 0);
        m_cmbBaseBody = new QComboBox();
        // m_btnPickBaseBody = makePickBtn(":/mMainWidgets/Icon/tihuan3x.png", "拾取基本体");
        // layBase->addWidget(m_cmbBaseBody);
        // layBase->addWidget(m_btnPickBaseBody);
        foldBodies->addRow(new QLabel("基本体:"), m_cmbBaseBody);

        // 接触几何处理：凸包高效稳定（默认）；精确网格用于平面/薄片等退化几何（凸包会失稳）
        m_cmbActionGeometry = new QComboBox();
        m_cmbActionGeometry->addItems({ "凸包", "精确网格" });
        foldBodies->addRow(new QLabel("动作体几何:"), m_cmbActionGeometry);

        m_cmbBaseGeometry = new QComboBox();
        m_cmbBaseGeometry->addItems({ "凸包", "精确网格" });
        foldBodies->addRow(new QLabel("基本体几何:"), m_cmbBaseGeometry);

        m_mainLayout->addWidget(foldBodies);

        // ==================== 3. 基本参数层 ====================
        FoldSection* foldParams = new FoldSection("基本参数", this);

        m_spbStiffness = new MxFunctions::mCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard, this);
        m_spbStiffness->setExpression("0");
        foldParams->addRow(new QLabel("刚度 (N/mm):"), m_spbStiffness);

        m_spbForceExponent = new MxFunctions::mCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard, this);
        m_spbForceExponent->setExpression("2.0");
        // IMPACT 力律指数（NX/Adams 语义）：法向力 F = K*δ^指数 - 阻尼项；
        // NX 常用 1.5，默认取数据类缺省 2.0
        m_spbForceExponent->setToolTip(QStringLiteral("IMPACT 法向力指数：F = K·δ^n，NX 常用 1.5"));
        foldParams->addRow(new QLabel("力指数:"), m_spbForceExponent);

        m_spbMaterialDamping = new MxFunctions::mCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard, this);
        m_spbMaterialDamping->setExpression("0");
        foldParams->addRow(new QLabel("材料阻尼 (N·s/mm):"), m_spbMaterialDamping);

        m_spbPenetrationDepth = new MxFunctions::mCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard, this);
        m_spbPenetrationDepth->setExpression("0");
        // 最大穿透深度（NX/Adams IMPACT 语义）：阻尼达到全值的穿透深度，
        // 不是零力死区，也不限制穿透量；未填时求解按引擎默认 0.0254
        m_spbPenetrationDepth->setToolTip(QStringLiteral(
            "阻尼达到全值的穿透深度（NX/Adams IMPACT 语义），非零力死区；未填时按引擎默认 0.0254"));
        foldParams->addRow(new QLabel("最大穿透深度 (mm):"), m_spbPenetrationDepth);

        m_mainLayout->addWidget(foldParams);

        // ==================== 4. 摩擦参数层 ====================
        FoldSection* foldFriction = new FoldSection("库仑摩擦", this);

        m_chkFrictionEnabled = new QCheckBox("启用摩擦");
        foldFriction->addRow(new QLabel("摩擦开关:"), m_chkFrictionEnabled);

        m_spbStaticFriction = new MxFunctions::mCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard, this);
        m_spbStaticFriction->setExpression("0");
        foldFriction->addRow(new QLabel("静摩擦系数:"), m_spbStaticFriction);

        m_spbDynamicFriction = new MxFunctions::mCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard, this);
        m_spbDynamicFriction->setExpression("0");
        foldFriction->addRow(new QLabel("动摩擦系数:"), m_spbDynamicFriction);

        m_mainLayout->addWidget(foldFriction);

        // ==================== 5. 高级参数层 ====================
        m_foldAdvanced = new FoldSection("高级参数", this);m_foldAdvanced->setParent(nullptr);

        m_cmbForceModel = new QComboBox();
        m_cmbForceModel->addItems({ "自动" });
        m_foldAdvanced->addRow(new QLabel("力模型:"), m_cmbForceModel);

        m_spbForceModelTol = new MxFunctions::mCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard, this);
        m_spbForceModelTol->setExpression("0");
        m_foldAdvanced->addRow(new QLabel("力模型公差:"), m_spbForceModelTol);

        //m_mainLayout->addWidget(m_foldAdvanced);

        // ==================== 6. 动作体小平面参数 ====================
        //【暂隐】小平面细分参数首通暂不开放（数据与同步逻辑保留，随时可恢复显示）
        m_foldActionFacet = new FoldSection("动作体小平面", this);m_foldActionFacet->setParent(nullptr);

        m_chkActionUseFacetFile = new QCheckBox("使用细分文件");
        m_foldActionFacet->addRow(new QLabel("细分文件:"), m_chkActionUseFacetFile);

        m_spbActionDistTol = new MxFunctions::mCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard, this);
        m_spbActionDistTol->setExpression("0");
        m_foldActionFacet->addRow(new QLabel("距离公差:"), m_spbActionDistTol);

        m_spbAngleTol = new MxFunctions::mCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard, this);
        m_spbAngleTol->setExpression("0");
        m_foldActionFacet->addRow(new QLabel("角度公差(°):"), m_spbAngleTol);

        //QHBoxLayout* layActionMax = new QHBoxLayout();
        //layActionMax->setContentsMargins(0, 0, 0, 0);
        m_chkActionUseMaxFacet = new QCheckBox("启用");
        m_spbActionMaxFacet = new MxFunctions::mCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard, this);
        m_spbActionMaxFacet->setExpression("0");
        //layActionMax->addWidget(m_chkActionUseMaxFacet);
        //layActionMax->addWidget(m_spbActionMaxFacet);
        m_foldActionFacet->addRow(new QLabel("最大小平面大小:"), m_chkActionUseMaxFacet, m_spbActionMaxFacet);

        //m_mainLayout->addWidget(m_foldActionFacet);

        // ==================== 7. 基本体小平面参数 ====================
        //【暂隐】同动作体小平面：首通暂不开放
        m_foldBaseFacet = new FoldSection("基本体小平面", this);m_foldBaseFacet->setParent(nullptr);

        m_chkBaseUseFacetFile = new QCheckBox("使用细分文件");
        m_foldBaseFacet->addRow(new QLabel("细分文件:"), m_chkBaseUseFacetFile);

        m_spbBaseDistTol = new MxFunctions::mCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard, this);
        m_spbBaseDistTol->setExpression("0");
        m_foldBaseFacet->addRow(new QLabel("距离公差:"), m_spbBaseDistTol);

        //QHBoxLayout* layBaseMax = new QHBoxLayout();
        //layBaseMax->setContentsMargins(0, 0, 0, 0);
        m_chkBaseUseMaxFacet = new QCheckBox("启用");
        m_spbBaseMaxFacet = new MxFunctions::mCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard, this);
        m_spbBaseMaxFacet->setExpression("0");
        //layBaseMax->addWidget(m_chkBaseUseMaxFacet);
        //layBaseMax->addWidget(m_spbBaseMaxFacet);
        m_foldBaseFacet->addRow(new QLabel("最大小平面大小:"), m_chkBaseUseMaxFacet, m_spbBaseMaxFacet);

        //m_mainLayout->addWidget(m_foldBaseFacet);

        m_mainLayout->addStretch();

        // ==================== Lambda 信号绑定 ====================

        // 1. 接触类型切换
        connect(m_cmbContactType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (m_motionContactData) m_motionContactData->setContactType(static_cast<MechMotionData::MotionContactType>(index));
            });

        // 2. 动作体切换
        connect(m_cmbActionBody, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!m_motionContactData) return;
            int bodyId = m_cmbActionBody->itemData(index).toInt();
            mMotionRefMap::set(Physical_MotionContact, m_motionContactData->getId(), Physical_MotionBody, MapRelation_MotionActionBody, bodyId);
            highlightBody(bodyId, std::string("contactBodyAction"), mMotionPanelColors::bodyA());
            // 写数据后刷新树状态，消除"未选择作用体"的过期警告
            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionContactSig(false, m_motionContactData->getId());
            });

        // 2.5 接触几何处理切换（0=凸包，1=精确网格）
        connect(m_cmbActionGeometry, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!m_motionContactData) return;
            m_motionContactData->setUseConvexHullAction(index == 0);
            });
        connect(m_cmbBaseGeometry, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!m_motionContactData) return;
            m_motionContactData->setUseConvexHullBase(index == 0);
            });

        // 3. 基本体切换
        connect(m_cmbBaseBody, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!m_motionContactData) return;
            int bodyId = m_cmbBaseBody->itemData(index).toInt();
            mMotionRefMap::set(Physical_MotionContact, m_motionContactData->getId(), Physical_MotionBody, MapRelation_MotionBaseBody, bodyId);
            highlightBody(bodyId, std::string("contactBodyBase"), mMotionPanelColors::bodyB());
            // 写数据后刷新树状态，消除"未选择基本体"的过期警告
            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionContactSig(false, m_motionContactData->getId());
            });

        // 4. 基本参数同步
        connect(m_spbStiffness, &QLineEdit::editingFinished, this, [this]() {
            double v = m_spbStiffness->text().toDouble();
            if (m_motionContactData) m_motionContactData->setStiffness(v);
            });
        connect(m_spbForceExponent, &QLineEdit::editingFinished, this, [this]() {
            double v = m_spbForceExponent->text().toDouble();
            if (m_motionContactData) m_motionContactData->setForceExponent(v);
            });
        connect(m_spbMaterialDamping, &QLineEdit::editingFinished, this, [this]() {
            double v = m_spbMaterialDamping->text().toDouble();
            if (m_motionContactData) m_motionContactData->setMaterialDamping(v);
            });
        connect(m_spbPenetrationDepth, &QLineEdit::editingFinished, this, [this]() {
            double v = m_spbPenetrationDepth->text().toDouble();
            if (m_motionContactData) m_motionContactData->setPenetrationDepth(v);
            });

        // 5. 摩擦参数同步
        connect(m_chkFrictionEnabled, &QCheckBox::toggled, this, [this](bool checked) {
            if (m_motionContactData) m_motionContactData->setFrictionEnabled(checked);
            updateFrictionVisibility();
            });
        connect(m_spbStaticFriction, &QLineEdit::editingFinished, this, [this]() {
            double v = m_spbStaticFriction->text().toDouble();
            if (m_motionContactData) m_motionContactData->setStaticFriction(v);
            });
        connect(m_spbDynamicFriction, &QLineEdit::editingFinished, this, [this]() {
            double v = m_spbDynamicFriction->text().toDouble();
            if (m_motionContactData) m_motionContactData->setDynamicFriction(v);
            });

        // 6. 高级参数同步
        connect(m_cmbForceModel, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (m_motionContactData) m_motionContactData->setForceModel(m_cmbForceModel->currentText());
            });
        connect(m_spbForceModelTol, &QLineEdit::editingFinished, this, [this]() {
            double v = m_spbForceModelTol->text().toDouble();
            if (m_motionContactData) m_motionContactData->setForceModelTolerance(v);
            });

        // 7. 动作体小平面同步
        connect(m_chkActionUseFacetFile, &QCheckBox::toggled, this, [this](bool checked) {
            if (m_motionContactData) m_motionContactData->setActionUseFacetFile(checked);
            });
        connect(m_spbActionDistTol, &QLineEdit::editingFinished, this, [this]() {
            double v = m_spbActionDistTol->text().toDouble();
            if (m_motionContactData) m_motionContactData->setActionDistanceTolerance(v);
            });
        connect(m_spbAngleTol, &QLineEdit::editingFinished, this, [this]() {
            double v = m_spbAngleTol->text().toDouble();
            if (m_motionContactData) m_motionContactData->setActionAngleTolerance(v);
            });
        connect(m_chkActionUseMaxFacet, &QCheckBox::toggled, this, [this](bool checked) {
            if (m_motionContactData) m_motionContactData->setActionUseMaxFacetSize(checked);
            updateActionFacetVisibility();
            });
        connect(m_spbActionMaxFacet, &QLineEdit::editingFinished, this, [this]() {
            double v = m_spbActionMaxFacet->text().toDouble();
            if (m_motionContactData) m_motionContactData->setActionMaxFacetSize(v);
            });

        // 8. 基本体小平面同步
        connect(m_chkBaseUseFacetFile, &QCheckBox::toggled, this, [this](bool checked) {
            if (m_motionContactData) m_motionContactData->setBaseUseFacetFile(checked);
            });
        connect(m_spbBaseDistTol, &QLineEdit::editingFinished, this, [this]() {
            double v = m_spbBaseDistTol->text().toDouble();
            if (m_motionContactData) m_motionContactData->setBaseDistanceTolerance(v);
            });
        connect(m_chkBaseUseMaxFacet, &QCheckBox::toggled, this, [this](bool checked) {
            if (m_motionContactData) m_motionContactData->setBaseUseMaxFacetSize(checked);
            updateBaseFacetVisibility();
            });
        connect(m_spbBaseMaxFacet, &QLineEdit::editingFinished, this, [this]() {
            double v = m_spbBaseMaxFacet->text().toDouble();
            if (m_motionContactData) m_motionContactData->setBaseMaxFacetSize(v);
            });
    }

    mMotionContactPanel::~mMotionContactPanel()
    {
    }

    void mMotionContactPanel::newMotionContactPanel(MechMotionData::MotionContactType type)
    {
        setEditing(true);
        disconnectSig();

        int newId = getCVM->getModelData()->getManager<mMotionContactManager>(Physical_MotionContact).createID();
        m_motionContactData = new mMotionContact(newId);
        m_motionContactData->setContactType(type);
        m_motionContactData->setName(getCVM->getModelData()->createNameByType(Physical_MotionContact, "接触"));
        getCVM->getModelData()->addPhysicalField<mMotionContact>(m_motionContactData);

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionContactSig(true, m_motionContactData->getId());
        connectSig();
    }

    void mMotionContactPanel::editMotionContactPanel(int id)
    {
        setEditing(true);
        disconnectSig();

        m_motionContactData = getCVM->getModelData()->getManager<mMotionContactManager>(Physical_MotionContact).getData(id);
        if (!m_motionContactData)
            return;

        refreshBodyComboBoxes();
        updateUIFromData();
        updateFrictionVisibility();
        updateActionFacetVisibility();
        updateBaseFacetVisibility();

        connectSig();
    }

    //数据对象即将被删除（树删除前调用）：置空编辑指针防悬垂（周期边界同款约定）
    void mMotionContactPanel::onMotionContactDataAboutToBeDestroyed(int id)
    {
        if (id >= 0)
        {
            if (m_motionContactData == nullptr || id != m_motionContactData->getId())
                return;
        }
        m_motionContactData = nullptr;
    }

    void mMotionContactPanel::uneditMotionContactPanel()
    {
        if (!isEditing())
            return;
        setEditing(false);
        disconnectSig();
        clearBodyHighlights();
        m_motionContactData = nullptr;
    }

    void mMotionContactPanel::refreshBodyComboBoxes()
    {
        if (!getCVM)
            return;

        const QSignalBlocker bAction(m_cmbActionBody);
        const QSignalBlocker bBase(m_cmbBaseBody);
        m_cmbActionBody->clear();
        m_cmbBaseBody->clear();

        m_cmbActionBody->addItem(" ", -1);
        m_cmbBaseBody->addItem(" ", -1);

        auto& bodyManager = getCVM->getModelData()->getManager<mMotionBodyManager>(Physical_MotionBody);
        for (int id : bodyManager.getAllIDs())
        {
            auto* body = bodyManager.getData(id);
            if (!body) continue;
            QString name = QString("%1").arg(body->getName()).arg(id);
            m_cmbActionBody->addItem(name, id);
            m_cmbBaseBody->addItem(name, id);
        }
    }

    void mMotionContactPanel::updateUIFromData()
    {
        if (!m_motionContactData) return;

        refreshBodyComboBoxes();

        const QSignalBlocker bType(m_cmbContactType);
        m_cmbContactType->setCurrentIndex(static_cast<int>(m_motionContactData->getContactType()));

        int actionIdx = m_cmbActionBody->findData(mMotionRefMap::get(Physical_MotionContact, m_motionContactData->getId(), Physical_MotionBody, MapRelation_MotionActionBody));
        int baseIdx = m_cmbBaseBody->findData(mMotionRefMap::get(Physical_MotionContact, m_motionContactData->getId(), Physical_MotionBody, MapRelation_MotionBaseBody));
        const QSignalBlocker bAction(m_cmbActionBody); m_cmbActionBody->setCurrentIndex(actionIdx >= 0 ? actionIdx : 0);
        const QSignalBlocker bBase(m_cmbBaseBody); m_cmbBaseBody->setCurrentIndex(baseIdx >= 0 ? baseIdx : 0);

        const QSignalBlocker bActionGeo(m_cmbActionGeometry); m_cmbActionGeometry->setCurrentIndex(m_motionContactData->getUseConvexHullAction() ? 0 : 1);
        const QSignalBlocker bBaseGeo(m_cmbBaseGeometry); m_cmbBaseGeometry->setCurrentIndex(m_motionContactData->getUseConvexHullBase() ? 0 : 1);

        m_spbStiffness->setExpression(QString::number(m_motionContactData->getStiffness()));
        m_spbForceExponent->setExpression(QString::number(m_motionContactData->getForceExponent()));
        m_spbMaterialDamping->setExpression(QString::number(m_motionContactData->getMaterialDamping()));
        m_spbPenetrationDepth->setExpression(QString::number(m_motionContactData->getPenetrationDepth()));

        m_chkFrictionEnabled->setChecked(m_motionContactData->getFrictionEnabled());
        m_spbStaticFriction->setExpression(QString::number(m_motionContactData->getStaticFriction()));
        m_spbDynamicFriction->setExpression(QString::number(m_motionContactData->getDynamicFriction()));

        int fmIdx = m_cmbForceModel->findText(m_motionContactData->getForceModel());
        if (fmIdx >= 0) m_cmbForceModel->setCurrentIndex(fmIdx);
        m_spbForceModelTol->setExpression(QString::number(m_motionContactData->getForceModelTolerance()));

        m_chkActionUseFacetFile->setChecked(m_motionContactData->getActionUseFacetFile());
        m_spbActionDistTol->setExpression(QString::number(m_motionContactData->getActionDistanceTolerance()));
        m_spbAngleTol->setExpression(QString::number(m_motionContactData->getActionAngleTolerance()));
        m_chkActionUseMaxFacet->setChecked(m_motionContactData->getActionUseMaxFacetSize());
        m_spbActionMaxFacet->setExpression(QString::number(m_motionContactData->getActionMaxFacetSize()));

        m_chkBaseUseFacetFile->setChecked(m_motionContactData->getBaseUseFacetFile());
        m_spbBaseDistTol->setExpression(QString::number(m_motionContactData->getBaseDistanceTolerance()));
        m_chkBaseUseMaxFacet->setChecked(m_motionContactData->getBaseUseMaxFacetSize());
        m_spbBaseMaxFacet->setExpression(QString::number(m_motionContactData->getBaseMaxFacetSize()));

        // 临时高亮动作体（黄）和基本体（洋红）
        clearBodyHighlights();
        highlightBody(mMotionRefMap::get(Physical_MotionContact, m_motionContactData->getId(), Physical_MotionBody, MapRelation_MotionActionBody), std::string("contactBodyAction"), QColor(255, 255, 0));
        highlightBody(mMotionRefMap::get(Physical_MotionContact, m_motionContactData->getId(), Physical_MotionBody, MapRelation_MotionBaseBody), std::string("contactBodyBase"), QColor(255, 0, 255));
    }

    void mMotionContactPanel::updateFrictionVisibility()
    {
        bool enabled = m_chkFrictionEnabled->isChecked();
        m_spbStaticFriction->setEnabled(enabled);
        m_spbDynamicFriction->setEnabled(enabled);
    }

    void mMotionContactPanel::updateActionFacetVisibility()
    {
        bool enabled = m_chkActionUseMaxFacet->isChecked();
        m_spbActionMaxFacet->setEnabled(enabled);
    }

    void mMotionContactPanel::updateBaseFacetVisibility()
    {
        bool enabled = m_chkBaseUseMaxFacet->isChecked();
        m_spbBaseMaxFacet->setEnabled(enabled);
    }

    void mMotionContactPanel::connectSig()
    {
        // 暂时不需要额外连接
    }

    void mMotionContactPanel::disconnectSig()
    {
        // 暂时不需要额外断开
    }

    void mMotionContactPanel::slot_subWindowActivated(QMdiSubWindow* subWindow)
    {
        // 视口切换时刷新体列表
        refreshBodyComboBoxes();
    }

    void mMotionContactPanel::slot_finishedPick()
    {
        // 拾取完成处理
    }

    void mMotionContactPanel::highlightBody(int bodyId, const std::string& key, const QColor& color)
    {
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        auto widget_ptr = getCVM->getPreRenderWidget();
        auto highlight_controller = widget_ptr->getHighlightController();
        if (!highlight_controller)
            return;

        auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(key);
        if (!geoHighLightRender)
            return;

        auto* motionBodyData = getCVM->getModelData()->getManager<mMotionBodyManager>(Physical_MotionBody).getData(bodyId);
        if (!motionBodyData)
        {
            // bodyId 无效时仅清空该 key 对应的高亮，不影响其他 key
            mGeoSelectData* geoData = new mGeoSelectData();
            geoData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoPart, std::vector<int>());
            geoHighLightRender->updateHighLightRender(geoData, HighStyle::PartALLFeatures);
            getCVM->getPreRenderWidget()->updatePaint();
            return;
        }

        geoHighLightRender->setHighLightColor(color.red(), color.green(), color.blue());
        mGeoSelectData* geoData = new mGeoSelectData();
        auto cSet = mMotionRefMap::getSet(Physical_MotionBody, motionBodyData->getId(), Physical_Part);
        geoData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoPart, std::vector<int>(cSet.begin(), cSet.end()));
        geoHighLightRender->updateHighLightRender(geoData, HighStyle::PartALLFeatures);
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mMotionContactPanel::clearBodyHighlights()
    {
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;
        auto highlight_controller = getCVM->getPreRenderWidget()->getHighlightController();
        if (!highlight_controller)
            return;

        auto actionRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("contactBodyAction"));
        if (actionRender)
        {
            mGeoSelectData* geoData = new mGeoSelectData();
            geoData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoPart, std::vector<int>());
            actionRender->updateHighLightRender(geoData, HighStyle::PartALLFeatures);
        }
        auto baseRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("contactBodyBase"));
        if (baseRender)
        {
            mGeoSelectData* geoData = new mGeoSelectData();
            geoData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoPart, std::vector<int>());
            baseRender->updateHighLightRender(geoData, HighStyle::PartALLFeatures);
        }
        getCVM->getPreRenderWidget()->updatePaint();
    }
}
