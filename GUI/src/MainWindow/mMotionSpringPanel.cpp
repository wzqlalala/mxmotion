
#include "mMotionSpringPanel.h"
#include "mMotionRefMap.h"
#include "mMotionPickerActivate.h"
#include "mMotionExprBuilderWidget.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"
#include "mMotionPanelColors.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mxr/label/mSymbolController.h"
#include "mxr/data/SpaceTree.h"
#include "mMechGlobalSignals.h"
#include "mGetSetTypeTemplate.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "PickCommonFunction.h"

//BasicData
#include "mMotionJointBase.h"
#include "mMotionJointBasic.h"
#include "mMotionJointManager.h"
#include "mMotionSpringBase.h"
#include "mMotionTranslationalSpring.h"
#include "mMotionTorsionalSpring.h"
#include "mMotionBodyPointSpring.h"
#include "mMotionSpringManager.h"
#include "mMotionBody.h"
#include "mMotionBodyManager.h"
#include "mModelData.h"

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
#include <QLineEdit>
using namespace MechPreData;
using namespace MechMotionData;
using namespace MxFunctions;
using namespace mxr;

namespace MechMainWindow
{

    mMotionSpringPanel::mMotionSpringPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        m_motionSpringData = nullptr;

        m_mainLayout = new QVBoxLayout(this);

        auto makeCalcLineEdit = [](MxFunctions::mCalcLineEdit::CalcMode mode) {
            auto* edit = new MxFunctions::mCalcLineEdit("0", mode);
            return edit;
        };

        auto makeDirectionCombo = []() {
            QComboBox* cmb = new QComboBox();
            cmb->addItems({ "双向", "仅正方向", "仅反方向" });
            return cmb;
        };

        // 1. 基本属性
        FoldSection* foldBasic = new FoldSection("基本属性", this);

        m_cmbSpringType = new QComboBox();
        m_cmbSpringType->addItems({ "拉压弹簧", "扭转弹簧", "体点弹簧" });
        foldBasic->addRow(new QLabel("弹簧类型:"), m_cmbSpringType);

        m_lblJoint = new QLabel("关联运动副:");
        m_cmbJoint = new QComboBox();
        foldBasic->addRow(m_lblJoint, m_cmbJoint);
        m_mainLayout->addWidget(foldBasic);
        m_foldWidgets.append(foldBasic);

        // 1.5 动作 - 体点弹簧端点 I
        m_foldActionBodyPoint = new FoldSection("动作", this);
        m_cmbActionBody = new QComboBox();
        m_foldActionBodyPoint->addRow(new QLabel("运动体:"), m_cmbActionBody);

        m_actionPointPicker = new mPointPickerWidget(QStringLiteral("点坐标"), this);
        m_actionPointPicker->pickController()->setAllowedTypes(&MechCommon::pickGeoPointEdgeFaceSolidNodePart);
        m_foldActionBodyPoint->addRow(m_actionPointPicker);
        m_mainLayout->addWidget(m_foldActionBodyPoint);
        m_foldWidgets.append(m_foldActionBodyPoint);

        // 1.5 基本 - 体点弹簧端点 J
        m_foldBaseBodyPoint = new FoldSection("基本", this);
        m_cmbBaseBody = new QComboBox();
        m_foldBaseBodyPoint->addRow(new QLabel("运动体:"), m_cmbBaseBody);

        m_basePointPicker = new mPointPickerWidget(QStringLiteral("点坐标"), this);
        m_basePointPicker->pickController()->setAllowedTypes(&MechCommon::pickGeoPointEdgeFaceSolidNodePart);
        m_foldBaseBodyPoint->addRow(m_basePointPicker);
        m_mainLayout->addWidget(m_foldBaseBodyPoint);
        m_foldWidgets.append(m_foldBaseBodyPoint);

        // 2. 弹簧参数
        FoldSection* foldSpring = new FoldSection("弹簧参数", this);

        m_spbStiffness = makeCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::NonNegative);
        m_lblStiffness = new QLabel("刚度 (K):");
        foldSpring->addRow(m_lblStiffness, m_spbStiffness);

        m_spbFreeValue = makeCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard);
        m_lblFreeValue = new QLabel("自由长度/角度:");
        foldSpring->addRow(m_lblFreeValue, m_spbFreeValue);

        m_spbInstallValue = makeCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard);
        m_lblInstallValue = new QLabel("安装长度/角度:");
        foldSpring->addRow(m_lblInstallValue, m_spbInstallValue);

        m_spbPreload = makeCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard);
        m_lblPreload = new QLabel("预紧力/力矩:");
        foldSpring->addRow(m_lblPreload, m_spbPreload);

        m_cmbSpringDirection = makeDirectionCombo();
        foldSpring->addRow(new QLabel("方向:"), m_cmbSpringDirection);
        m_mainLayout->addWidget(foldSpring);
        m_foldWidgets.append(foldSpring);

        // 3. 阻尼参数
        FoldSection* foldDamping = new FoldSection("阻尼参数", this);

        m_chkEnableDamping = new QCheckBox("启用阻尼");
        foldDamping->addRow(m_chkEnableDamping);

        m_spbDamping = makeCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::NonNegative);
        m_lblDamping = new QLabel("阻尼 (C):");
        foldDamping->addRow(m_lblDamping, m_spbDamping);

        m_cmbDampingDirection = makeDirectionCombo();
        foldDamping->addRow(new QLabel("方向:"), m_cmbDampingDirection);
        m_mainLayout->addWidget(foldDamping);
        m_foldWidgets.append(foldDamping);

        m_spbDamping->setEnabled(false);
        m_cmbDampingDirection->setEnabled(false);

        // 4. 高级函数
        FoldSection* foldAdvanced = new FoldSection("高级", this);

        m_chkUseForceFunction = new QCheckBox("使用力函数");
        m_forceFuncBuilder = new mMotionExprBuilderWidget(this);
        m_forceFuncBuilder->setEnabled(false);
        foldAdvanced->addRow(m_chkUseForceFunction);
        foldAdvanced->addRow(new QLabel("力函数:"), m_forceFuncBuilder);

        m_chkUseDampingFunction = new QCheckBox("使用阻尼函数");
        m_dampFuncBuilder = new mMotionExprBuilderWidget(this);
        m_dampFuncBuilder->setEnabled(false);
        foldAdvanced->addRow(m_chkUseDampingFunction);
        foldAdvanced->addRow(new QLabel("阻尼函数:"), m_dampFuncBuilder);
        m_mainLayout->addWidget(foldAdvanced);
        m_foldWidgets.append(foldAdvanced);

        // ==================== Lambda 信号绑定 ====================

        // 弹簧类型切换
        connect(m_cmbSpringType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            handleSpringTypeChange(static_cast<SpringType>(index));
            });

        // 体-点弹簧端点同步
        auto syncBodyPointParams = [this]() {
            if (!m_motionSpringData || m_motionSpringData->getType() != SpringType::BodyPoint)
                return;
            auto* bp = dynamic_cast<mMotionBodyPointSpring*>(m_motionSpringData);
            if (!bp) return;

            mMotionRefMap::set(Physical_MotionSpring, m_motionSpringData->getId(), Physical_MotionBody, MapRelation_MotionBodyI, m_cmbActionBody->currentData().toInt());
            mMotionRefMap::set(Physical_MotionSpring, m_motionSpringData->getId(), Physical_MotionBody, MapRelation_MotionBodyJ, m_cmbBaseBody->currentData().toInt());

            updateInstallValueFromBodyPoints();
            updateUIFromBodyPointData();
            triggerHighlight();
            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionSpringSig(false, m_motionSpringData->getId());
            };
        connect(m_cmbActionBody, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [syncBodyPointParams](int) { syncBodyPointParams(); });
        connect(m_cmbBaseBody, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [syncBodyPointParams](int) { syncBodyPointParams(); });

        // 体-点坐标拾取控件数值变化回调（写入端点I/J并联动）
        connect(m_actionPointPicker, &mPointPickerWidget::coordinateChanged, this, [this]() {
            const QVector3D pt = m_actionPointPicker->coordinate();
            if (m_motionSpringData && m_motionSpringData->getType() == SpringType::BodyPoint) {
                if (auto* bp = dynamic_cast<mMotionBodyPointSpring*>(m_motionSpringData))
                    bp->setLocalI(pt);
            }
            updateInstallValueFromBodyPoints();
            updatePointRender("springActionPoint", pt, QVector3D(1, 0, 0));
            triggerHighlight();
            if (m_motionSpringData)
                emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionSpringSig(false, m_motionSpringData->getId());
            });
        connect(m_basePointPicker, &mPointPickerWidget::coordinateChanged, this, [this]() {
            const QVector3D pt = m_basePointPicker->coordinate();
            if (m_motionSpringData && m_motionSpringData->getType() == SpringType::BodyPoint) {
                if (auto* bp = dynamic_cast<mMotionBodyPointSpring*>(m_motionSpringData))
                    bp->setLocalJ(pt);
            }
            updateInstallValueFromBodyPoints();
            updatePointRender("springBasePoint", pt, QVector3D(0, 0, 1));
            triggerHighlight();
            if (m_motionSpringData)
                emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionSpringSig(false, m_motionSpringData->getId());
            });

        // 关联运动副切换
        connect(m_cmbJoint, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!m_motionSpringData) return;
            int jointId = m_cmbJoint->itemData(index).toInt();
            mMotionRefMap::set(Physical_MotionSpring, m_motionSpringData->getId(), Physical_MotionJoint, MapRelation_None, jointId);
            triggerHighlight();
            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionSpringSig(false, m_motionSpringData->getId());
            });

        // 弹簧参数实时同步
        auto syncSpringParams = [this]() {
            if (!m_motionSpringData) return;
            m_motionSpringData->setStiffness(m_spbStiffness->text().toDouble());
            m_motionSpringData->setFreeValue(m_spbFreeValue->text().toDouble());
            m_motionSpringData->setInstallValue(m_spbInstallValue->text().toDouble());
            m_motionSpringData->setPreload(m_spbPreload->text().toDouble());
            m_motionSpringData->setSpringDirectionMode(static_cast<SpringDirectionMode>(m_cmbSpringDirection->currentIndex()));
            triggerHighlight();
            };
        connect(m_spbStiffness, &QLineEdit::editingFinished, this, [syncSpringParams]() { syncSpringParams(); });
        connect(m_spbFreeValue, &QLineEdit::editingFinished, this, [syncSpringParams]() { syncSpringParams(); });
        connect(m_spbInstallValue, &QLineEdit::editingFinished, this, [syncSpringParams]() { syncSpringParams(); });
        connect(m_spbPreload, &QLineEdit::editingFinished, this, [syncSpringParams]() { syncSpringParams(); });
        connect(m_cmbSpringDirection, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [syncSpringParams](int) { syncSpringParams(); });

        // 启用阻尼切换
        auto updateDampingEnabled = [this]() {
            bool enabled = m_chkEnableDamping->isChecked();
            m_spbDamping->setEnabled(enabled);
            m_cmbDampingDirection->setEnabled(enabled);
            if (m_motionSpringData) {
                m_motionSpringData->setDampingEnabled(enabled);
                if (!enabled) {
                    m_motionSpringData->setDamping(0.0);
                    const QSignalBlocker b(m_spbDamping); m_spbDamping->setExpression("0");
                }
                triggerHighlight();
            }
            };
        connect(m_chkEnableDamping, &QCheckBox::toggled, this, [updateDampingEnabled](bool) { updateDampingEnabled(); });

        // 阻尼参数实时同步
        auto syncDampingParams = [this]() {
            if (!m_motionSpringData) return;
            m_motionSpringData->setDamping(m_spbDamping->text().toDouble());
            m_motionSpringData->setDampingDirectionMode(static_cast<SpringDirectionMode>(m_cmbDampingDirection->currentIndex()));
            };
        connect(m_spbDamping, &QLineEdit::editingFinished, this, [syncDampingParams]() { syncDampingParams(); });
        connect(m_cmbDampingDirection, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [syncDampingParams](int) { syncDampingParams(); });

        // 函数启用/编辑同步
        auto syncForceFunction = [this]() {
            if (!m_motionSpringData) return;
            bool use = m_chkUseForceFunction->isChecked();
            m_forceFuncBuilder->setEnabled(use);
            // 仅合法表达式入库存；非法/未完成编辑时置空（求解侧回退线性参数）
            m_motionSpringData->setForceFunction(
                (use && m_forceFuncBuilder->isValid()) ? m_forceFuncBuilder->expression() : QString());
            };
        connect(m_chkUseForceFunction, &QCheckBox::toggled, this, [syncForceFunction](bool) { syncForceFunction(); });
        connect(m_forceFuncBuilder, &mMotionExprBuilderWidget::expressionAccepted, this, [syncForceFunction](const QString&) { syncForceFunction(); });

        auto syncDampingFunction = [this]() {
            if (!m_motionSpringData) return;
            bool use = m_chkUseDampingFunction->isChecked();
            m_dampFuncBuilder->setEnabled(use);
            m_motionSpringData->setDampingFunction(
                (use && m_dampFuncBuilder->isValid()) ? m_dampFuncBuilder->expression() : QString());
            };
        connect(m_chkUseDampingFunction, &QCheckBox::toggled, this, [syncDampingFunction](bool) { syncDampingFunction(); });
        connect(m_dampFuncBuilder, &mMotionExprBuilderWidget::expressionAccepted, this, [syncDampingFunction](const QString&) { syncDampingFunction(); });

        m_mainLayout->addStretch();
    }

    mMotionSpringPanel::~mMotionSpringPanel()
    {
        if (m_mainLayout != nullptr)
        {
            delete m_mainLayout;
            m_mainLayout = nullptr;
        }
    }

    void mMotionSpringPanel::newMotionSpringPanel(SpringType type)
    {
        setEditing(true);
        connectSig();

        int springID = getCVM->getModelData()->getManager<mMotionSpringManager>(Physical_MotionSpring).createID();
        if (type == SpringType::Translational)
            m_motionSpringData = new mMotionTranslationalSpring(springID);
        else if (type == SpringType::Torsional)
            m_motionSpringData = new mMotionTorsionalSpring(springID);
        else
            m_motionSpringData = new mMotionBodyPointSpring(springID);

        m_motionSpringData->setName(getCVM->getModelData()->createNameByType(Physical_MotionSpring, m_motionSpringData->getTypeName()), false);
        getCVM->getModelData()->addPhysicalField<mMotionSpringBase>(m_motionSpringData);

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionSpringSig(true, m_motionSpringData->getId());
    }

    void mMotionSpringPanel::editMotionSpringPanel(int id)
    {
        setEditing(true);
        m_motionSpringData = dynamic_cast<mMotionSpringBase*>(getCVM->getModelData()->getManager<mMotionSpringManager>(Physical_MotionSpring).getData(id));
        if (!m_motionSpringData)
            return;

        connectSig();

        updateUIFromData();

        //进入编辑激活端点拾取控件（仅体点弹簧类型可见）
        activateFirstVisiblePointPicker({ m_actionPointPicker, m_basePointPicker });

        //恢复拾取会话：延迟到事件循环下一拍（避开树点击链路后续清理），详见Joint面板注释
        QTimer::singleShot(0, this, [this]() {
            if (!m_motionSpringData)
                return;
            restorePickSessionsOnEntry({ m_actionPointPicker, m_basePointPicker });
            });
    }

    //数据对象即将被删除（树删除前调用）：置空编辑指针防悬垂（周期边界同款约定）
    void mMotionSpringPanel::onMotionSpringDataAboutToBeDestroyed(int id)
    {
        if (id >= 0)
        {
            if (m_motionSpringData == nullptr || id != m_motionSpringData->getId())
                return;
        }
        m_motionSpringData = nullptr;
    }

    void mMotionSpringPanel::uneditMotionSpringPanel()
    {
        if (!isEditing())
            return;
        setEditing(false);
        //clearSpringSymbol();
        m_motionSpringData = nullptr;
        //退出编辑时失活拾取控件并清理高亮
        for (mPointPickerWidget* picker : { m_actionPointPicker, m_basePointPicker })
        {
            if (picker && picker->pickController())
            {
                picker->pickController()->setPickActive(false);
                picker->clearAllHighlight();
                //拾取数据是临时的（确定值已入数据类），退出编辑清空防残留高亮
                picker->pickController()->displayData(PickWidgetData());
            }
        }
        disconnectSig();

        clearBodyHighlights();
        if (getCVM && getCVM->getPreRenderWidget() && getCVM->getPreRenderWidget()->getCommonShapeRender())
        {
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("springActionPoint");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("springBasePoint");
            getCVM->getPreRenderWidget()->updatePaint();
        }
    }

    void mMotionSpringPanel::refreshJointList(SpringType type)
    {
        const QSignalBlocker b(m_cmbJoint);
        m_cmbJoint->clear();
        m_cmbJoint->addItem(" ", -1);

        if (type == SpringType::BodyPoint)
            return;

        auto& jointManager = getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint);
        QVector<JointType> allowedTypes;
        if (type == SpringType::Translational)
        {
            allowedTypes = { JointType::Translational };
        }
        else
        {
            allowedTypes = { JointType::Revolute, JointType::Cylindrical };
        }

        QStringList names = jointManager.getAllNamesByTypes(allowedTypes);
        for (const QString& name : names)
        {
            m_cmbJoint->addItem(name, jointManager.getIDByName(name));
        }
    }

    void mMotionSpringPanel::refreshBodyComboBoxes()
    {
        const QSignalBlocker bAction(m_cmbActionBody);
        const QSignalBlocker bBase(m_cmbBaseBody);
        m_cmbActionBody->clear();
        m_cmbBaseBody->clear();
        m_cmbActionBody->addItem(" ", -1);
        m_cmbBaseBody->addItem(" ", -1);

        if (!getCVM)
            return;

        auto& bodyManager = getCVM->getModelData()->getManager<mMotionBodyManager>(Physical_MotionBody);
        for (int id : bodyManager.getAllIDs())
        {
            auto* body = bodyManager.getData(id);
            if (!body) continue;
            m_cmbActionBody->addItem(body->getName(), id);
            m_cmbBaseBody->addItem(body->getName(), id);
        }
    }

    void mMotionSpringPanel::updateInstallValueFromBodyPoints()
    {
        if (!m_motionSpringData || m_motionSpringData->getType() != SpringType::BodyPoint)
            return;
        auto* bp = dynamic_cast<mMotionBodyPointSpring*>(m_motionSpringData);
        if (!bp) return;

        // 初始位姿下，假设运动体局部坐标系与世界坐标系重合，
        // 安装长度 = 两个局部点的世界距离
        QVector3D diff = bp->getLocalJ() - bp->getLocalI();
        double installLength = static_cast<double>(diff.length());
        m_motionSpringData->setInstallValue(installLength);

        const QSignalBlocker b(m_spbInstallValue);
        m_spbInstallValue->setExpression(QString::number(installLength));
    }

    void mMotionSpringPanel::setBodyPointUIVisible(bool visible)
    {
        if (m_lblJoint) m_lblJoint->setVisible(!visible);
        if (m_cmbJoint) m_cmbJoint->setVisible(!visible);
        if (m_foldActionBodyPoint) m_foldActionBodyPoint->setVisible(visible);
        if (m_foldBaseBodyPoint) m_foldBaseBodyPoint->setVisible(visible);
        m_spbInstallValue->setReadOnly(visible);
        m_spbInstallValue->setEnabled(!visible);

        QTimer::singleShot(0, m_foldWidgets[0], [=]() { m_foldWidgets[0]->refreshLayout(); });
    }

    void mMotionSpringPanel::updateUIFromData()
    {
        if (!getCVM)
            return;

        if (!m_motionSpringData)
            return;

        // 还原弹簧类型
        const QSignalBlocker bType(m_cmbSpringType);
        m_cmbSpringType->setCurrentIndex(static_cast<int>(m_motionSpringData->getType()));

        refreshJointList(m_motionSpringData->getType());

        // 参数单位标签随弹簧类型切换：拉压/体点=长度·力（mm·N·mm），扭簧=角度·度
        //（扭簧的刚度 N·mm/°、自由/安装角 °、阻尼 N·mm/(°/s)，求解导出处统一换算为弧度）
        if (m_lblStiffness && m_lblFreeValue && m_lblInstallValue && m_lblPreload && m_lblDamping) {
            const bool torsion = (m_motionSpringData->getType() == SpringType::Torsional);
            m_lblStiffness->setText(torsion ? "刚度 (N·mm/°):" : "刚度 (N/mm):");
            m_lblFreeValue->setText(torsion ? "自由角度 (°):" : "自由长度 (mm):");
            m_lblInstallValue->setText(torsion ? "安装角度 (°):" : "安装长度 (mm):");
            m_lblPreload->setText(torsion ? "预紧力矩 (N·mm):" : "预紧力 (N):");
            m_lblDamping->setText(torsion ? "阻尼 (N·mm/(°/s)):" : "阻尼 (N/(mm/s)):");
        }

        // 还原关联运动副
        int jointId = mMotionRefMap::get(Physical_MotionSpring, m_motionSpringData->getId(), Physical_MotionJoint);
        int jointIndex = m_cmbJoint->findData(jointId);
        const QSignalBlocker bJoint(m_cmbJoint);
        m_cmbJoint->setCurrentIndex(jointIndex != -1 ? jointIndex : 0);

        // 还原弹簧参数
        const QSignalBlocker bStiff(m_spbStiffness); m_spbStiffness->setExpression(QString::number(m_motionSpringData->getStiffness()));
        const QSignalBlocker bFree(m_spbFreeValue); m_spbFreeValue->setExpression(QString::number(m_motionSpringData->getFreeValue()));
        const QSignalBlocker bInst(m_spbInstallValue); m_spbInstallValue->setExpression(QString::number(m_motionSpringData->getInstallValue()));
        const QSignalBlocker bPre(m_spbPreload); m_spbPreload->setExpression(QString::number(m_motionSpringData->getPreload()));
        const QSignalBlocker bDir(m_cmbSpringDirection); m_cmbSpringDirection->setCurrentIndex(static_cast<int>(m_motionSpringData->getSpringDirectionMode()));

        // 还原阻尼启用状态
        bool enabled = m_motionSpringData->isDampingEnabled();
        const QSignalBlocker bChkEnableDamp(m_chkEnableDamping); m_chkEnableDamping->setChecked(enabled);
        m_spbDamping->setEnabled(enabled);
        m_cmbDampingDirection->setEnabled(enabled);

        // 还原阻尼参数
        const QSignalBlocker bDamp(m_spbDamping); m_spbDamping->setExpression(QString::number(m_motionSpringData->getDamping()));
        const QSignalBlocker bDampDir(m_cmbDampingDirection); m_cmbDampingDirection->setCurrentIndex(static_cast<int>(m_motionSpringData->getDampingDirectionMode()));

        // 还原函数
        QString forceFunc = m_motionSpringData->getForceFunction();
        bool useForce = !forceFunc.isEmpty();
        const QSignalBlocker bChkForce(m_chkUseForceFunction); m_chkUseForceFunction->setChecked(useForce);
        m_forceFuncBuilder->setExpression(forceFunc); m_forceFuncBuilder->setEnabled(useForce);

        QString dampFunc = m_motionSpringData->getDampingFunction();
        bool useDamp = !dampFunc.isEmpty();
        const QSignalBlocker bChkDamp(m_chkUseDampingFunction); m_chkUseDampingFunction->setChecked(useDamp);
        m_dampFuncBuilder->setExpression(dampFunc); m_dampFuncBuilder->setEnabled(useDamp);

        // 按类型切换显示
        if (m_motionSpringData->getType() == SpringType::BodyPoint)
        {
            updateUIFromBodyPointData();
            setBodyPointUIVisible(true);
        }
        else
        {
            setBodyPointUIVisible(false);
            clearBodyHighlights();
            if (getCVM && getCVM->getPreRenderWidget() && getCVM->getPreRenderWidget()->getCommonShapeRender())
            {
                getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("springActionPoint");
                getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("springBasePoint");
                getCVM->getPreRenderWidget()->updatePaint();
            }
        }

        triggerHighlight();
    }

    void mMotionSpringPanel::updateUIFromBodyPointData()
    {
        if (!m_motionSpringData || m_motionSpringData->getType() != SpringType::BodyPoint)
            return;
        auto* bp = dynamic_cast<mMotionBodyPointSpring*>(m_motionSpringData);
        if (!bp) return;

        refreshBodyComboBoxes();

        int bodyIIndex = m_cmbActionBody->findData(mMotionRefMap::get(Physical_MotionSpring, m_motionSpringData->getId(), Physical_MotionBody, MapRelation_MotionBodyI));
        int bodyJIndex = m_cmbBaseBody->findData(mMotionRefMap::get(Physical_MotionSpring, m_motionSpringData->getId(), Physical_MotionBody, MapRelation_MotionBodyJ));
        const QSignalBlocker bAction(m_cmbActionBody); m_cmbActionBody->setCurrentIndex(bodyIIndex != -1 ? bodyIIndex : 0);
        const QSignalBlocker bBase(m_cmbBaseBody); m_cmbBaseBody->setCurrentIndex(bodyJIndex != -1 ? bodyJIndex : 0);

        // 端点坐标反显到拾取控件（旧 setObject 直绑数据成员，改由 setCoordinate 同步）
        m_actionPointPicker->setCoordinate(bp->getLocalI());
        m_basePointPicker->setCoordinate(bp->getLocalJ());

        updatePointRender("springActionPoint", bp->getLocalI(),
            mMotionPanelColors::vec(mMotionPanelColors::pointA()));
        updatePointRender("springBasePoint", bp->getLocalJ(),
            mMotionPanelColors::vec(mMotionPanelColors::pointB()));

        updateInstallValueFromBodyPoints();

        // 高亮作用体（红）和基本体（蓝），端点为橙/品红，体与点色系正交
        highlightBody(mMotionRefMap::get(Physical_MotionSpring, m_motionSpringData->getId(), Physical_MotionBody, MapRelation_MotionBodyI), std::string("springBodyI"), mMotionPanelColors::bodyA());
        highlightBody(mMotionRefMap::get(Physical_MotionSpring, m_motionSpringData->getId(), Physical_MotionBody, MapRelation_MotionBodyJ), std::string("springBodyJ"), mMotionPanelColors::bodyB());
    }

    void mMotionSpringPanel::ensureSpringSymbolByData(mxr::mSymbolController* symbolController,
        MechMotionData::mMotionSpringBase* springData, mxr::CCSMRendWidget* rendWidget)
    {
        if (!symbolController || !springData || !getCVM)
            return;
        //已存在的符号不动（保留面板编辑后的最新状态；显隐由树统一控制）
        QString symbolName = QString("motionSpring_%1").arg(springData->getId());
        if (symbolController->isExistSymbol(symbolName))
            return;
        Q_UNUSED(rendWidget);

        if (springData->getType() == SpringType::BodyPoint)
        {
            auto* bp = dynamic_cast<mMotionBodyPointSpring*>(springData);
            if (!bp) return;
            // 初始位姿下假设运动体局部坐标系与世界坐标系重合
            buildTranslationalSpringSymbolByMarkersImpl(symbolController, bp->getId(),
                bp->isDampingEnabled(), bp->getName(), bp->getLocalI(), bp->getLocalJ());
            return;
        }

        // 关联副弹簧：从映射取关联运动副的位姿
        int jointId = mMotionRefMap::get(Physical_MotionSpring, springData->getId(), Physical_MotionJoint);
        auto* joint = dynamic_cast<mMotionJointBasic*>(
            getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint).getData(jointId));
        if (!joint)
            return;
        QVector3D axis = joint->getAxis();
        if (axis.lengthSquared() < 1e-12)
            return;
        axis.normalize();
        if (springData->getType() == SpringType::Torsional)
            buildTorsionalSpringSymbolImpl(symbolController, springData->getId(),
                springData->isDampingEnabled(), springData->getName(), joint->getOrigin(), axis);
        else
            buildTranslationalSpringSymbolImpl(symbolController, springData->getId(),
                springData->isDampingEnabled(), springData->getName(), joint->getOrigin(), axis);
    }

    void mMotionSpringPanel::triggerHighlight()
    {
        clearSpringSymbol();
        if (!m_motionSpringData || !getCVM)
            return;

        if (m_motionSpringData->getType() == SpringType::BodyPoint)
        {
            auto* bp = dynamic_cast<mMotionBodyPointSpring*>(m_motionSpringData);
            if (!bp) return;
            // 初始位姿下假设运动体局部坐标系与世界坐标系重合
            buildTranslationalSpringSymbolByMarkersImpl(
                getCVM->getPreRenderWidget()->GetSymbolController(),
                bp->getId(),
                bp->isDampingEnabled(),
                bp->getName(),
                bp->getLocalI(),
                bp->getLocalJ());
            return;
        }

        int jointId = mMotionRefMap::get(Physical_MotionSpring, m_motionSpringData->getId(), Physical_MotionJoint);
        if (jointId < 0)
            return;

        auto joint = dynamic_cast<mMotionJointBasic*>(getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint).getData(jointId));
        if (!joint)
            return;

        QVector3D origin = joint->getOrigin();
        QVector3D axis = joint->getAxis();
        if (axis.lengthSquared() < 1e-12)
            return;
        axis.normalize();

        if (m_motionSpringData->getType() == SpringType::Translational)
            buildTranslationalSpringSymbol(origin, axis);
        else
            buildTorsionalSpringSymbol(origin, axis);
    }

    void mMotionSpringPanel::updateSpringSymbolForAnimation(const QVector3D& origin, const QVector3D& axis)
    {
        if (!m_motionSpringData)
            return;

        if (m_motionSpringData->getType() == SpringType::Translational)
            buildTranslationalSpringSymbol(origin, axis);
        else if (m_motionSpringData->getType() == SpringType::Torsional)
            buildTorsionalSpringSymbol(origin, axis);
    }

    void mMotionSpringPanel::createOrUpdateSpringSymbol(mxr::mSymbolController* symbolController,
        int springId,
        SpringType type,
        bool dampingEnabled,
        const QString& name,
        const QVector3D& origin,
        const QVector3D& axis)
    {
        if (!symbolController)
            return;

        QVector3D normalizedAxis = axis;
        if (normalizedAxis.lengthSquared() < 1e-12)
            return;
        normalizedAxis.normalize();

        if (type == SpringType::Translational)
            buildTranslationalSpringSymbolImpl(symbolController, springId, dampingEnabled, name, origin, normalizedAxis);
        else
            buildTorsionalSpringSymbolImpl(symbolController, springId, dampingEnabled, name, origin, normalizedAxis);
    }

    void mMotionSpringPanel::createOrUpdateSpringSymbolByMarkers(mxr::mSymbolController* symbolController,
        int springId,
        SpringType type,
        bool dampingEnabled,
        const QString& name,
        const QVector3D& posI,
        const QVector3D& posJ,
        const QQuaternion& rotI,
        const QQuaternion& /*rotJ*/)
    {
        if (!symbolController)
            return;

        if (type == SpringType::Translational || type == SpringType::BodyPoint) {
            buildTranslationalSpringSymbolByMarkersImpl(symbolController, springId, dampingEnabled, name, posI, posJ);
        }
        else {
            // 扭转弹簧：中心点取两 marker 中点，轴向由 marker I 的局部 Z 轴在世界中的方向决定
            QVector3D origin = (posI + posJ) / 2.0f;
            QVector3D axis = QVector3D(0, 0, 1);
            if (!rotI.isIdentity())
                axis = rotI.rotatedVector(QVector3D(0, 0, 1));
            else if ((posJ - posI).lengthSquared() > 1e-12)
                axis = (posJ - posI).normalized();

            if (axis.lengthSquared() < 1e-12)
                return;
            axis.normalize();

            buildTorsionalSpringSymbolImpl(symbolController, springId, dampingEnabled, name, origin, axis);
        }
    }

    void mMotionSpringPanel::createOrUpdateSpringSymbolByBodyTransforms(mxr::mSymbolController* symbolController,
        int springId,
        SpringType type,
        bool dampingEnabled,
        const QString& name,
        const QMatrix4x4& bodyTransformI,
        const QMatrix4x4& bodyTransformJ,
        const QVector3D& localI,
        const QVector3D& localJ)
    {
        if (!symbolController || type != SpringType::BodyPoint)
            return;

        QVector3D posI = bodyTransformI * localI;
        QVector3D posJ = bodyTransformJ * localJ;
        buildTranslationalSpringSymbolByMarkersImpl(symbolController, springId, dampingEnabled, name, posI, posJ);
    }

    void mMotionSpringPanel::clearSpringSymbol()
    {
        if (!m_motionSpringData)
            return;
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (!symbolController)
            return;
        clearSpringSymbol(symbolController, m_motionSpringData->getId());
    }

    void mMotionSpringPanel::clearSpringSymbol(mxr::mSymbolController* symbolController, int springId)
    {
        if (!symbolController)
            return;
        QString newsymbol = QString("motionSpring_%1").arg(springId);
        symbolController->deleteSymbol({ newsymbol });
        symbolController->UpdateAllSymbol();
    }

    void mMotionSpringPanel::buildTranslationalSpringSymbol(const QVector3D& origin, const QVector3D& axis)
    {
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (!symbolController || !m_motionSpringData)
            return;

        buildTranslationalSpringSymbolImpl(symbolController, m_motionSpringData->getId(),
            m_motionSpringData->isDampingEnabled(), m_motionSpringData->getName(),
            origin, axis);
    }

    void mMotionSpringPanel::buildTranslationalSpringSymbolByMarkersImpl(mxr::mSymbolController* symbolController,
        int springId, bool dampingEnabled, const QString& name,
        const QVector3D& posI, const QVector3D& posJ)
    {
        if (!symbolController)
            return;

        QVector3D forward = posJ - posI;
        float length = forward.length();
        if (length < 1e-6f)
            return;
        forward /= length;

        QVector3D origin = (posI + posJ) / 2.0f;

        // 根据视图包围盒反算世界尺度
        float sceneScale = 0.3f;
        if (getCVM && getCVM->getPreRenderWidget())
        {
            mxr::AABB viewAABB = getCVM->getPreRenderWidget()->getAABBOfView();
            QVector3D diag = viewAABB.maxEdge - viewAABB.minEdge;
            float maxLen = std::max({ diag.x(), diag.y(), diag.z() });
            if (maxLen > 1e-6f)
                sceneScale = maxLen * 0.05f;
        }

        // 构建局部坐标：forward 为 I->J 方向，u/v 使其垂直平面始终朝向相机
        QVector3D viewDir;
        if (getCVM && getCVM->getPreRenderWidget())
        {
            if (auto camCtrl = getCVM->getPreRenderWidget()->getCameraController())
            {
                mxr::CameraState camState = camCtrl->getCameraState();
                QVector3D camPos(camState.eye.x, camState.eye.y, camState.eye.z);
                viewDir = camPos - origin;
                if (viewDir.lengthSquared() > 1e-12)
                    viewDir.normalize();
            }
        }
        if (viewDir.lengthSquared() < 1e-12)
            viewDir = QVector3D(0, 0, 1);

        QVector3D u = QVector3D::crossProduct(forward, viewDir);
        if (u.lengthSquared() < 1e-12)
            u = QVector3D::crossProduct(forward, QVector3D(0, 1, 0));
        if (u.lengthSquared() < 1e-12)
            u = QVector3D::crossProduct(forward, QVector3D(1, 0, 0));
        u.normalize();
        QVector3D v = QVector3D::crossProduct(forward, u);
        v.normalize();

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIdx;
        std::vector<unsigned int> faceIdx;

        float L = length / 2.0f;
        float leadLen = std::min(sceneScale * 0.4f, L * 0.25f);
        float zigWidth = L - leadLen;
        float springAmp = sceneScale * 0.25f;
        int numZigZags = 6;
        float step = (2.0f * zigWidth) / numZigZags;

        int pBase = points.size();

        // 左侧引线段：从 marker I 到弹簧螺旋起点
        points.append(posI);
        points.append(posI + forward * leadLen);

        // 锯齿段：从 I 侧螺旋起点到 J 侧螺旋终点
        for (int i = 1; i < numZigZags; ++i)
        {
            float x = -zigWidth + i * step; // 相对中心 origin
            float y = (i % 2 == 1) ? -springAmp : springAmp;
            points.append(origin + forward * x + u * y);
        }

        // 右侧引线段：从弹簧螺旋终点到 marker J
        points.append(posJ - forward * leadLen);
        points.append(posJ);

        // 连接弹簧线段索引
        for (int i = 0; i < points.size() - 1; ++i)
        {
            lineIdx.push_back(pBase + i);
            lineIdx.push_back(pBase + i + 1);
        }

        // 阻尼器：沿 I->J 方向居中放置，长度随实际间距缩放
        if (dampingEnabled)
        {
            float c_base_x = -L * 0.4f;
            float c_open_x = L * 0.4f;
            float piston_x = 0.0f;
            float cyl_w = sceneScale * 0.4f;
            float piston_w = sceneScale * 0.25f;

            // 左侧连杆：从 marker I 到缸体底部
            int dLeft = points.size();
            points.append(posI);
            points.append(origin + forward * c_base_x);
            lineIdx.push_back(dLeft); lineIdx.push_back(dLeft + 1);

            // C型缸体底板（垂直于 forward）
            int cBaseIdx = points.size();
            points.append(origin + forward * c_base_x + u * cyl_w);
            points.append(origin + forward * c_base_x - u * cyl_w);
            lineIdx.push_back(cBaseIdx); lineIdx.push_back(cBaseIdx + 1);

            // C型缸体上臂（沿 forward）
            int cTopIdx = points.size();
            points.append(origin + forward * c_base_x + u * cyl_w);
            points.append(origin + forward * c_open_x + u * cyl_w);
            lineIdx.push_back(cTopIdx); lineIdx.push_back(cTopIdx + 1);

            // C型缸体下臂（沿 forward）
            int cBotIdx = points.size();
            points.append(origin + forward * c_base_x - u * cyl_w);
            points.append(origin + forward * c_open_x - u * cyl_w);
            lineIdx.push_back(cBotIdx); lineIdx.push_back(cBotIdx + 1);

            // 右侧连杆：从 marker J 到活塞头
            int dRight = points.size();
            points.append(posJ);
            points.append(origin + forward * piston_x);
            lineIdx.push_back(dRight); lineIdx.push_back(dRight + 1);

            // T型活塞头（垂直于 forward）
            int pistonIdx = points.size();
            points.append(origin + forward * piston_x + u * piston_w);
            points.append(origin + forward * piston_x - u * piston_w);
            lineIdx.push_back(pistonIdx); lineIdx.push_back(pistonIdx + 1);
        }

        QString newsymbol = QString("motionSpring_%1").arg(springId);
        auto sym = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(newsymbol);
        sym->_vexter = points;
        sym->_lineIndex = lineIdx;
        sym->_faceIndex = faceIdx;
        sym->_lineColor = QVector3D(0, 128, 255);
        sym->_faceColor = QVector3D(0, 128, 255);
        sym->_lineIsFixedPixel = false;
        sym->_lineDepthState = false;
        sym->_faceIsFixedPixel = false;
        sym->_faceDepthState = false;
        sym->_showState = true;
        sym->_txt_pos = { { name, origin } };
        sym->_textColor = QVector3D(0, 0, 0);

        symbolController->createSymbol(sym);
        symbolController->UpdateAllSymbol();
    }

    void mMotionSpringPanel::buildTranslationalSpringSymbolImpl(mxr::mSymbolController* symbolController,
        int springId, bool dampingEnabled, const QString& name,
        const QVector3D& origin, const QVector3D& axis)
    {
        if (!symbolController)
            return;

        QVector3D normalizedAxis = axis;
        if (normalizedAxis.lengthSquared() < 1e-12)
            return;
        normalizedAxis.normalize();

        // 根据视图包围盒反算世界尺度
        float sceneScale = 0.3f;
        if (getCVM && getCVM->getPreRenderWidget())
        {
            mxr::AABB viewAABB = getCVM->getPreRenderWidget()->getAABBOfView();
            QVector3D diag = viewAABB.maxEdge - viewAABB.minEdge;
            float maxLen = std::max({ diag.x(), diag.y(), diag.z() });
            if (maxLen > 1e-6f)
                sceneScale = maxLen * 0.05f;
        }

        // 构建局部坐标：axis 为弹簧主方向（沿运动/拉伸方向），u/v 为其垂直平面
        QVector3D forward = normalizedAxis;
        QVector3D u = QVector3D::crossProduct(forward, QVector3D(1, 0, 0));
        if (u.lengthSquared() < 1e-12)
            u = QVector3D::crossProduct(forward, QVector3D(0, 1, 0));
        u.normalize();
        QVector3D v = QVector3D::crossProduct(forward, u);
        v.normalize();

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIdx;
        std::vector<unsigned int> faceIdx;

        float L = sceneScale * 1.5f;          // 图标总半长

        // ==========================================
        // 1. 绘制弹簧 (折线图) - 沿 axis 方向
        // ==========================================
        float zigWidth = sceneScale * 0.6f;   // 弹簧折线区域沿 axis 的半长
        float springAmp = sceneScale * 0.25f; // 弹簧折线振幅（垂直于 axis）

        int pBase = points.size();

        // 左侧直线段
        points.append(origin - forward * L);
        points.append(origin - forward * zigWidth);

        // 锯齿段 (6 个半波)
        int numZigZags = 6;
        float step = (2.0f * zigWidth) / numZigZags;
        for (int i = 1; i < numZigZags; ++i)
        {
            float x = -zigWidth + i * step;   // 沿 axis 方向
            float y = (i % 2 == 1) ? -springAmp : springAmp; // 垂直于 axis 方向
            points.append(origin + forward * x + u * y);
        }

        // 右侧直线段
        points.append(origin + forward * zigWidth);
        points.append(origin + forward * L);

        // 连接弹簧线段索引
        for (int i = 0; i < points.size() - 1; ++i)
        {
            lineIdx.push_back(pBase + i);
            lineIdx.push_back(pBase + i + 1);
        }

        // ==========================================
        // 2. 绘制阻尼器 (C-T 型套筒) - 与弹簧同轴重叠
        // ==========================================
        if (dampingEnabled)
        {
            // 严格按照你提供的图片比例构建 C-T 缸体
            float c_base_x = -sceneScale * 0.4f;   // C型缸体底部 (沿 axis 左侧)
            float c_open_x = sceneScale * 0.4f;   // C型缸体开口端 (沿 axis 右侧延伸)
            float piston_x = 0.0f;                // T型活塞头位置 (居中)

            // 关键视觉控制：缸体要比弹簧宽，活塞头要和弹簧差不多宽
            float cyl_w = sceneScale * 0.4f;   // C型缸体半宽（垂直于 axis）
            float piston_w = sceneScale * 0.25f;  // T型活塞半宽（垂直于 axis）

            // 左侧连杆 (连接到缸体底部)
            int dLeft = points.size();
            points.append(origin - forward * L);
            points.append(origin + forward * c_base_x);
            lineIdx.push_back(dLeft); lineIdx.push_back(dLeft + 1);

            // C型缸体底板 (垂直于 axis)
            int cBaseIdx = points.size();
            points.append(origin + forward * c_base_x + u * cyl_w);
            points.append(origin + forward * c_base_x - u * cyl_w);
            lineIdx.push_back(cBaseIdx); lineIdx.push_back(cBaseIdx + 1);

            // C型缸体上臂 (沿 axis 水平线)
            int cTopIdx = points.size();
            points.append(origin + forward * c_base_x + u * cyl_w);
            points.append(origin + forward * c_open_x + u * cyl_w);
            lineIdx.push_back(cTopIdx); lineIdx.push_back(cTopIdx + 1);

            // C型缸体下臂 (沿 axis 水平线)
            int cBotIdx = points.size();
            points.append(origin + forward * c_base_x - u * cyl_w);
            points.append(origin + forward * c_open_x - u * cyl_w);
            lineIdx.push_back(cBotIdx); lineIdx.push_back(cBotIdx + 1);

            // 右侧连杆 (穿过缸体开口，连到活塞头)
            int dRight = points.size();
            points.append(origin + forward * L);
            points.append(origin + forward * piston_x);
            lineIdx.push_back(dRight); lineIdx.push_back(dRight + 1);

            // T型活塞头 (垂直于 axis，悬浮在缸体内)
            int pistonIdx = points.size();
            points.append(origin + forward * piston_x + u * piston_w);
            points.append(origin + forward * piston_x - u * piston_w);
            lineIdx.push_back(pistonIdx); lineIdx.push_back(pistonIdx + 1);
        }
        QString newsymbol = QString("motionSpring_%1").arg(springId);
        auto sym = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(newsymbol);
        sym->_vexter = points;
        sym->_lineIndex = lineIdx;
        sym->_faceIndex = faceIdx;
        sym->_lineColor = QVector3D(0, 128, 255);
        sym->_faceColor = QVector3D(0, 128, 255);
        sym->_lineIsFixedPixel = false;
        sym->_lineDepthState = false;
        sym->_faceIsFixedPixel = false;
        sym->_faceDepthState = false;
        sym->_showState = true;
        sym->_txt_pos = { { name, origin } };
        sym->_textColor = QVector3D(0, 0, 0);

        symbolController->createSymbol(sym);
        symbolController->UpdateAllSymbol();
    }

    void mMotionSpringPanel::buildTorsionalSpringSymbol(const QVector3D& origin, const QVector3D& axis)
    {
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (!symbolController || !m_motionSpringData)
            return;

        buildTorsionalSpringSymbolImpl(symbolController, m_motionSpringData->getId(),
            m_motionSpringData->isDampingEnabled(), m_motionSpringData->getName(),
            origin, axis);
    }

    void mMotionSpringPanel::buildTorsionalSpringSymbolImpl(mxr::mSymbolController* symbolController,
        int springId, bool dampingEnabled, const QString& name,
        const QVector3D& origin, const QVector3D& axis)
    {
        if (!symbolController)
            return;

        QVector3D normalizedAxis = axis;
        if (normalizedAxis.lengthSquared() < 1e-12)
            return;
        normalizedAxis.normalize();

        // 根据视图包围盒反算世界尺度
        float sceneScale = 0.3f;
        if (getCVM && getCVM->getPreRenderWidget())
        {
            mxr::AABB viewAABB = getCVM->getPreRenderWidget()->getAABBOfView();
            QVector3D diag = viewAABB.maxEdge - viewAABB.minEdge;
            float maxLen = std::max({ diag.x(), diag.y(), diag.z() });
            if (maxLen > 1e-6f)
                sceneScale = maxLen * 0.05f;
        }

        // 构建局部坐标：任意垂直于 axis 的 u/v
        QVector3D u = QVector3D::crossProduct(normalizedAxis, QVector3D(1, 0, 0));
        if (u.lengthSquared() < 1e-12)
            u = QVector3D::crossProduct(normalizedAxis, QVector3D(0, 1, 0));
        u.normalize();
        QVector3D v = QVector3D::crossProduct(normalizedAxis, u);

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIdx;
        std::vector<unsigned int> faceIdx;

        // --- 图例尺度定义 ---
        float H = sceneScale * 0.8f;  // 回路半高 (现在沿 v 方向，完全垂直于 axis)
        float W = sceneScale * 1.0f;  // 回路半宽 (沿 u 方向)

        // ==========================================
        // 1. 中线 (现在位于 u-v 平面内，垂直于真实的物理 axis)
        // ==========================================
        int cBase = points.size();
        points.append(origin - v * H);
        points.append(origin + v * H);
        lineIdx.push_back(cBase); lineIdx.push_back(cBase + 1);

        // ==========================================
        // 2. 绘制弹簧分支 (右侧, +u 方向) - 必定存在
        // ==========================================
        QVector3D S_center = origin + u * W; // 右侧分支中心

        // 顶部和底部横臂 (连接中线和弹簧)
        int sArmBase = points.size();
        points.append(origin + v * H);
        points.append(S_center + v * H);
        points.append(origin - v * H);
        points.append(S_center - v * H);
        lineIdx.push_back(sArmBase); lineIdx.push_back(sArmBase + 1);
        lineIdx.push_back(sArmBase + 2); lineIdx.push_back(sArmBase + 3);

        // 弹簧上下直线段
        float springZEnd = H * 0.6f;
        int sLineBase = points.size();
        points.append(S_center + v * H);
        points.append(S_center + v * springZEnd);
        points.append(S_center - v * H);
        points.append(S_center - v * springZEnd);
        lineIdx.push_back(sLineBase); lineIdx.push_back(sLineBase + 1);
        lineIdx.push_back(sLineBase + 2); lineIdx.push_back(sLineBase + 3);

        // 弹簧折线段 (沿 v 方向的锯齿，振幅在 u 方向)
        int numZigZags = 6;
        float step = (2.0f * springZEnd) / numZigZags;
        float Amp = W * 0.25f;
        int zigBase = points.size();

        for (int i = 0; i <= numZigZags; ++i) {
            float y = springZEnd - i * step; // 从上往下走
            float x = 0.0f;
            if (i > 0 && i < numZigZags) {
                x = (i % 2 == 1) ? Amp : -Amp; // 左右折叠
            }
            points.append(S_center + v * y + u * x);
            if (i > 0) {
                lineIdx.push_back(zigBase + i - 1);
                lineIdx.push_back(zigBase + i);
            }
        }

        // ==========================================
        // 3. 绘制阻尼器分支 (左侧, -u 方向) - 按需叠加
        // ==========================================
        if (dampingEnabled)
        {
            QVector3D D_center = origin - u * W; // 左侧分支中心

            // 顶部和底部横臂 (连接中线和阻尼器)
            int dArmBase = points.size();
            points.append(origin + v * H);
            points.append(D_center + v * H);
            points.append(origin - v * H);
            points.append(D_center - v * H);
            lineIdx.push_back(dArmBase); lineIdx.push_back(dArmBase + 1);
            lineIdx.push_back(dArmBase + 2); lineIdx.push_back(dArmBase + 3);

            float c_half_h = H * 0.3f; // 缸体半高
            float c_half_w = W * 0.25f; // 缸体半宽
            float p_half_w = W * 0.15f; // 活塞头半宽

            // 底部连杆 (从底臂连接到缸底)
            int dBotLine = points.size();
            points.append(D_center - v * H);
            points.append(D_center - v * c_half_h);
            lineIdx.push_back(dBotLine); lineIdx.push_back(dBotLine + 1);

            // C型缸体 (安装在底部，开口朝上)
            int cBaseIdx = points.size();
            // 底板
            points.append(D_center - v * c_half_h - u * c_half_w);
            points.append(D_center - v * c_half_h + u * c_half_w);
            lineIdx.push_back(cBaseIdx); lineIdx.push_back(cBaseIdx + 1);
            // 左侧壁
            int cLeftIdx = points.size();
            points.append(D_center - v * c_half_h - u * c_half_w);
            points.append(D_center + v * c_half_h - u * c_half_w);
            lineIdx.push_back(cLeftIdx); lineIdx.push_back(cLeftIdx + 1);
            // 右侧壁
            int cRightIdx = points.size();
            points.append(D_center - v * c_half_h + u * c_half_w);
            points.append(D_center + v * c_half_h + u * c_half_w);
            lineIdx.push_back(cRightIdx); lineIdx.push_back(cRightIdx + 1);

            // 顶部连杆 (从顶臂一直插进缸体中心)
            int dTopLine = points.size();
            points.append(D_center + v * H);
            points.append(D_center); // 活塞中心点 (悬浮在缸体内)
            lineIdx.push_back(dTopLine); lineIdx.push_back(dTopLine + 1);

            // T型活塞头 (水平横线)
            int pHeadIdx = points.size();
            points.append(D_center - u * p_half_w);
            points.append(D_center + u * p_half_w);
            lineIdx.push_back(pHeadIdx); lineIdx.push_back(pHeadIdx + 1);
        }

        QString newsymbol = QString("motionSpring_%1").arg(springId);
        auto sym = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(newsymbol);
        sym->_vexter = points;
        sym->_lineIndex = lineIdx;
        sym->_faceIndex = faceIdx;
        sym->_lineColor = QVector3D(255, 128, 0);
        sym->_faceColor = QVector3D(255, 128, 0);
        sym->_lineIsFixedPixel = false;
        sym->_lineDepthState = false;
        sym->_faceIsFixedPixel = false;
        sym->_faceDepthState = false;
        sym->_showState = true;
        sym->_txt_pos = { { name, origin } };
        sym->_textColor = QVector3D(0, 0, 0);

        symbolController->createSymbol(sym);
        symbolController->UpdateAllSymbol();
    }

    void mMotionSpringPanel::handleSpringTypeChange(SpringType newType)
    {
        if (!m_motionSpringData)
        {
            refreshJointList(newType);
            return;
        }

        if (m_motionSpringData->getType() == newType)
            return;

        QString name = "";
        if (m_motionSpringData->getIsUserRename())//用户重命名过
        {
            name = m_motionSpringData->getName();
        }
        else//默认名称，类型更改后自动修改名称
        {
            QStringList list = m_motionSpringData->getName().split("_");
            if (list.size() >= 1)
            {
                name = m_motionSpringData->getTypeName(newType);
                if (list.size() == 2)
                {
                    name = QString("%1_%2").arg(name).arg(list.at(1));
                }
            }
            else
            {
                name = QString("%1_%2").arg(m_motionSpringData->getTypeName(newType)).arg(m_motionSpringData->getId());
            }
            if (getCVM->getModelData()->getManager<MechMotionData::mMotionJointManager>(Physical_MotionSpring).existName(name))
            {
                name = getCVM->getModelData()->createNameByType(Physical_MotionSpring, m_motionSpringData->getTypeName(newType));
            }
        }

        // 保存通用字段
        int id = m_motionSpringData->getId();
        double stiffness = m_motionSpringData->getStiffness();
        double damping = m_motionSpringData->getDamping();
        double preload = m_motionSpringData->getPreload();
        SpringDirectionMode springDir = m_motionSpringData->getSpringDirectionMode();
        SpringDirectionMode dampDir = m_motionSpringData->getDampingDirectionMode();
        double freeValue = m_motionSpringData->getFreeValue();
        double installValue = m_motionSpringData->getInstallValue();
        QVector<double> tableX = m_motionSpringData->getTableX();
        QVector<double> tableY = m_motionSpringData->getTableY();
        QString forceFunc = m_motionSpringData->getForceFunction();
        QString dampFunc = m_motionSpringData->getDampingFunction();
        //int jointId = mMotionRefMap::get(Physical_MotionSpring, m_motionSpringData->getId(), Physical_MotionJoint);

        // 保存体-点字段（若当前是体-点弹簧）
        int bodyIId = -1, bodyJId = -1;
        QVector3D localI, localJ;
        QQuaternion localRotI, localRotJ;
        if (m_motionSpringData->getType() == SpringType::BodyPoint)
        {
            auto* bp = dynamic_cast<mMotionBodyPointSpring*>(m_motionSpringData);
            if (bp)
            {
                bodyIId = mMotionRefMap::get(Physical_MotionSpring, m_motionSpringData->getId(), Physical_MotionBody, MapRelation_MotionBodyI);
                bodyJId = mMotionRefMap::get(Physical_MotionSpring, m_motionSpringData->getId(), Physical_MotionBody, MapRelation_MotionBodyJ);
                localI = bp->getLocalI();
                localJ = bp->getLocalJ();
                localRotI = bp->getLocalRotI();
                localRotJ = bp->getLocalRotJ();
            }
        }

        // 替换数据对象
        mMotionSpringBase* newSpring = nullptr;
        if (newType == SpringType::Translational)
            newSpring = new mMotionTranslationalSpring(id);
        else if (newType == SpringType::Torsional)
            newSpring = new mMotionTorsionalSpring(id);
        else
            newSpring = new mMotionBodyPointSpring(id);

        newSpring->setName(name, m_motionSpringData->getIsUserRename());
        newSpring->setStiffness(stiffness);
        newSpring->setDamping(damping);
        newSpring->setPreload(preload);
        newSpring->setSpringDirectionMode(springDir);
        newSpring->setDampingDirectionMode(dampDir);
        newSpring->setFreeValue(freeValue);
        newSpring->setInstallValue(installValue);
        if (!tableX.isEmpty() && !tableY.isEmpty())
            newSpring->setTable(tableX, tableY);
        newSpring->setForceFunction(forceFunc);
        newSpring->setDampingFunction(dampFunc);
        newSpring->setDampingEnabled(m_motionSpringData->isDampingEnabled());

        // 迁移体-点字段
        if (newType == SpringType::BodyPoint)
        {
            auto* bp = dynamic_cast<mMotionBodyPointSpring*>(newSpring);
            if (bp)
            {
                mMotionRefMap::set(Physical_MotionSpring, newSpring->getId(), Physical_MotionBody, MapRelation_MotionBodyI, bodyIId);
                mMotionRefMap::set(Physical_MotionSpring, newSpring->getId(), Physical_MotionBody, MapRelation_MotionBodyJ, bodyJId);
                bp->setLocalI(localI);
                bp->setLocalJ(localJ);
                bp->setLocalRotI(localRotI);
                bp->setLocalRotJ(localRotJ);
            }
        }

        // 更新 manager
        getCVM->getModelData()->getManager<mMotionSpringManager>(Physical_MotionSpring).deleteData(id);
        getCVM->getModelData()->addPhysicalField<mMotionSpringBase>(newSpring);
        m_motionSpringData = newSpring;

        refreshJointList(newType);
        updateUIFromData();

        //类型切换后刷新拾取激活状态
        activateFirstVisiblePointPicker({ m_actionPointPicker, m_basePointPicker });

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionSpringSig(false, m_motionSpringData->getId());
    }

    void mMotionSpringPanel::updatePointRender(const QString& key, const QVector3D& point, const QVector3D& color)
    {
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;
        auto commonShapeRender = getCVM->getPreRenderWidget()->getCommonShapeRender();
        if (!commonShapeRender)
            return;
        //注意：此处严禁 ClearPreWidget*Pick/UpdatePreWidgetHighlight——本函数在拾取信号
        //处理链内被调用，清空+ReadyToReadContainer 会让激活中的拾取控件立刻空捕获并
        //清掉刚拾取的数据与高亮（表现为拾取的线高亮闪没、但点应用又有效）
        commonShapeRender->appendPoint(key, { point }, color);
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mMotionSpringPanel::highlightBody(int bodyId, const std::string& key, const QColor& color)
    {
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;
        auto* motionBodyData = getCVM->getModelData()->getManager<mMotionBodyManager>(Physical_MotionBody).getData(bodyId);
        if (!motionBodyData)
            return;

        auto widget_ptr = getCVM->getPreRenderWidget();
        auto highlight_controller = widget_ptr->getHighlightController();
        if (!highlight_controller)
            return;

        auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(key);
        if (!geoHighLightRender)
            return;
        geoHighLightRender->setHighLightColor(color.red(), color.green(), color.blue());
        mGeoSelectData* geoData = new mGeoSelectData();
        const std::set<int> highlightPartIds = mMotionRefMap::getSet(Physical_MotionBody, motionBodyData->getId(), Physical_Part);
        geoData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoPart, std::vector<int>(highlightPartIds.begin(), highlightPartIds.end()));
        geoHighLightRender->updateHighLightRender(geoData, HighStyle::PartALLFeatures);
    }

    void mMotionSpringPanel::clearBodyHighlights()
    {
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;
        auto highlight_controller = getCVM->getPreRenderWidget()->getHighlightController();
        if (!highlight_controller)
            return;

        auto bodyIRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("springBodyI"));
        if (bodyIRender)
        {
            mGeoSelectData* geoData = new mGeoSelectData();
            geoData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoPart, std::vector<int>());
            bodyIRender->updateHighLightRender(geoData, HighStyle::PartALLFeatures);
        }
        auto bodyJRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("springBodyJ"));
        if (bodyJRender)
        {
            mGeoSelectData* geoData = new mGeoSelectData();
            geoData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoPart, std::vector<int>());
            bodyJRender->updateHighLightRender(geoData, HighStyle::PartALLFeatures);
        }
    }

    void mMotionSpringPanel::connectSig()
    {
    }

    void mMotionSpringPanel::disconnectSig()
    {
    }

    void mMotionSpringPanel::slot_subWindowActivated(QMdiSubWindow* subWindow)
    {
    }

    void mMotionSpringPanel::slot_finishedPick()
    {
    }

}
