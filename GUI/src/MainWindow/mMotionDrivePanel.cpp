
#include "mMotionDrivePanel.h"
#include "mMotionRefMap.h"
#include "motion/functions/Function_Expr.h"
#include "mMotionExprBuilderWidget.h"
#include "motion/functions/Function_Table.h"
#include "mCurveWindow.h"
#include "mMotionScenarioManager.h"
#include "mMotionScenario.h"

#include <QPushButton>
#include <QVector>
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"
#include "mMotionJointSymbolHelper.h"

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
#include "mMotionJointBase.h"
#include "mMotionJointBasic.h"
#include "mMotionJointScrew.h"
#include "mMotionJointUniversal.h"
#include "mMotionJointPerpendicular.h"
#include "mMotionJointConstantVelocity.h"
#include "mMotionJointManager.h"
#include "mMotionDriveManager.h"
#include "mMotionDriveBase.h"
#include "mMotionJointDrive.h"
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
using namespace MechPreData;
using namespace MechMotionData;
using namespace MxFunctions;
using namespace mxr;

namespace MechMainWindow
{

    mMotionDrivePanel::mMotionDrivePanel(QWidget* parent)
        : mPanelBase(parent)
    {
        m_motionDriveData = nullptr;
        m_lastTargetJointId = -1;

        //获取当前视口的数据
        m_mainLayout = new QVBoxLayout(this);


        auto makePickBtn = [](const QString& iconPath, const QString& tip) {
            QToolButton* b = new QToolButton();
            b->setIcon(QIcon(iconPath));
            b->setToolTip(tip);
            b->setFixedWidth(30);
            return b;
            };

        auto makeCoordLayout = [](QDoubleSpinBox*& x, QDoubleSpinBox*& y, QDoubleSpinBox*& z, QToolButton* btn) {
            QHBoxLayout* lay = new QHBoxLayout();
            lay->setContentsMargins(0, 0, 0, 0);
            x = new QDoubleSpinBox(); x->setRange(-1e9, 1e9); x->setDecimals(3); x->setButtonSymbols(QAbstractSpinBox::NoButtons);
            y = new QDoubleSpinBox(); y->setRange(-1e9, 1e9); y->setDecimals(3); y->setButtonSymbols(QAbstractSpinBox::NoButtons);
            z = new QDoubleSpinBox(); z->setRange(-1e9, 1e9); z->setDecimals(3); z->setButtonSymbols(QAbstractSpinBox::NoButtons);
            lay->addWidget(new QLabel("X:")); lay->addWidget(x);
            lay->addWidget(new QLabel("Y:")); lay->addWidget(y);
            lay->addWidget(new QLabel("Z:")); lay->addWidget(z);
            lay->addWidget(btn);
            return lay;
            };

        // ==================== 1. 驱动目标层 ====================
        FoldSection* foldTarget = new FoldSection("驱动目标", this);

        m_cmbTargetType = new QComboBox();
        m_cmbTargetType->addItems({ "依附于运动副", "依附于运动体" });
        foldTarget->addRow(new QLabel("目标类型:"), m_cmbTargetType);

        m_cmbTargetObject = new QComboBox();
        foldTarget->addRow(new QLabel("选择目标:"), m_cmbTargetObject);
        m_mainLayout->addWidget(foldTarget);

        // ==================== 2. 物理语义层 ====================
        FoldSection* foldSemantics = new FoldSection("驱动属性", this);

        m_cmbMotionType = new QComboBox();
        m_cmbMotionType->addItems({ "平动", "转动" });
        foldSemantics->addRow(new QLabel("运动方向:"), m_cmbMotionType);

        m_cmbControlMode = new QComboBox();
        m_cmbControlMode->addItems({ "位置", "速度" });  // 加速度模式暂不支持，隐藏（求解侧另有显式防护）
        foldSemantics->addRow(new QLabel("控制模式:"), m_cmbControlMode);
        m_mainLayout->addWidget(foldSemantics);

        FoldSection* foldExcitation = new FoldSection("驱动激励", this);

        m_cmbInputType = new QComboBox();
        m_cmbInputType->addItems({ "常数值", "函数表达式", "函数模板", "表格" });
        foldExcitation->addRow(new QLabel("输入模式:"), m_cmbInputType);

        // 利用 QStackedWidget 实现输入控件的无缝原地切换
        m_stkInputMode = new QStackedWidget();

        // --- 页 0：常数 ---
        m_spbConstant = new MxFunctions::mCalcLineEdit(
            MxFunctions::mCalcLineEdit::CalcMode::Standard, this);
        m_spbConstant->setExpression("0");
        m_stkInputMode->addWidget(m_spbConstant); // Index 0

        // --- 页 1：函数表达式（公用构造器控件：目录点选 + 运算符 + 校验 + 预览） ---
        m_exprBuilder = new mMotionExprBuilderWidget(this);
        m_stkInputMode->addWidget(m_exprBuilder); // Index 1

        // --- 页 2：函数模板（参数化生成表达式） ---
        {
            QWidget* pageTpl = new QWidget();
            QVBoxLayout* layTpl = new QVBoxLayout(pageTpl);
            layTpl->setContentsMargins(0, 0, 0, 0);
            layTpl->setSpacing(2);

            m_cmbTemplate = new QComboBox();
            m_cmbTemplate->addItems({ "正弦", "余弦", "斜坡", "阶跃", "多项式" });
            layTpl->addWidget(m_cmbTemplate);

            for (int i = 0; i < 4; ++i) {
                m_lblTplParam[i] = new QLabel("");
                m_tplParam[i] = new MxFunctions::mCalcLineEdit(
                    MxFunctions::mCalcLineEdit::CalcMode::Standard, this);
                m_tplParam[i]->setExpression("0");
                QHBoxLayout* row = new QHBoxLayout();
                row->setContentsMargins(0, 0, 0, 0);
                row->addWidget(m_lblTplParam[i]);
                row->addWidget(m_tplParam[i]);
                layTpl->addLayout(row);
            }

            m_lblTemplateExpr = new QLabel("");
            m_lblTemplateExpr->setStyleSheet("color:#606266;");
            m_lblTemplateExpr->setWordWrap(true);
            layTpl->addWidget(m_lblTemplateExpr);

            m_btnPreviewTpl = new QPushButton("预览曲线");
            layTpl->addWidget(m_btnPreviewTpl);

            m_stkInputMode->addWidget(pageTpl); // Index 2
        }

        // --- 页 3：表格驱动（时间-值两列编辑 + 预览） ---
        {
            QWidget* pageTbl = new QWidget();
            QVBoxLayout* layTbl = new QVBoxLayout(pageTbl);
            layTbl->setContentsMargins(0, 0, 0, 0);
            layTbl->setSpacing(2);

            m_tblDrive = new QTableWidget(0, 2);
            m_tblDrive->setHorizontalHeaderLabels(QStringList() << "时间 t (s)" << "激励值 (° 或 mm)");
            m_tblDrive->horizontalHeader()->setStretchLastSection(true);
            m_tblDrive->setMinimumHeight(120);
            layTbl->addWidget(m_tblDrive);

            m_btnTblAddRow = new QPushButton("添加行");
            m_btnTblDelRow = new QPushButton("删除行");
            m_btnTblPreview = new QPushButton("预览曲线");
            QHBoxLayout* layTblBtns = new QHBoxLayout();
            layTblBtns->addWidget(m_btnTblAddRow);
            layTblBtns->addWidget(m_btnTblDelRow);
            layTblBtns->addWidget(m_btnTblPreview);
            layTbl->addLayout(layTblBtns);

            m_lblTblHint = new QLabel("");
            m_lblTblHint->setStyleSheet("color:#606266;");
            m_lblTblHint->setWordWrap(true);
            layTbl->addWidget(m_lblTblHint);

            m_stkInputMode->addWidget(pageTbl); // Index 3
        }

        m_lblExcitationUnit = new QLabel("激励数值:");
        foldExcitation->addRow(m_lblExcitationUnit, m_stkInputMode);
        m_mainLayout->addWidget(foldExcitation);

        // 2. 构造函数中直接绑定 Lambda 信号 (纯即时生效逻辑)
            // ==================== Lambda 信号绑定 ====================

            // 1. 目标类型切换 (副 vs 体)
        connect(m_cmbTargetType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            auto newType = (index == 0) ? MxFunctions::DriveTargetType::Joint : MxFunctions::DriveTargetType::Body;
            handleTargetTypeChange(newType);
            });

        // 2. 目标对象切换 (具体绑定哪个副/体)
        connect(m_cmbTargetObject, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!m_motionDriveData) return;
            int targetId = m_cmbTargetObject->itemData(index).toInt();

            if (m_motionDriveData->getTargetType() == MxFunctions::DriveTargetType::Joint) {
                auto* jointDrive = dynamic_cast<MechMotionData::mMotionJointDrive*>(m_motionDriveData);
                mMotionRefMap::set(Physical_MotionDrive, m_motionDriveData->getId(), Physical_MotionJoint, MapRelation_None, targetId);

                // 智能联动：如果选了特定的运动副，尝试自动锁定平动/转动
                autoLockMotionType(targetId);
            }
            else {
                //auto* bodyDrive = dynamic_cast<mMotionBodyDrive*>(m_motionDriveData);
                //if (bodyDrive) bodyDrive->setTargetBodyId(targetId);
            }

            triggerHighlight();

            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionDriveSig(false, m_motionDriveData->getId());
            });

        // 3. 物理语义修改
        connect(m_cmbMotionType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (m_motionDriveData) m_motionDriveData->setMotionType(static_cast<MechMotionData::DriveMotionType>(index));
            //激励数值单位随"运动方向×控制模式"联动刷新
            refreshExcitationUnitLabel();
            });

        connect(m_cmbControlMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (m_motionDriveData) m_motionDriveData->setControlMode(static_cast<MechMotionData::DriveControlMode>(index));
            refreshExcitationUnitLabel();
            });

        // 4. 激励模式切换 (常数 / 表达式 / 模板)
        connect(m_cmbInputType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            m_stkInputMode->setCurrentIndex(index);

            // 切换时，主动把当前显示框里的值写入底层数据
            if (index == 0) {
                if (m_motionDriveData)
                    m_motionDriveData->setExpression(m_spbConstant->text());
            }
            else if (index == 2) {
                refreshTemplateParams();
                applyTemplate();
            }
            else if (index == 3) {
                syncDriveTableFromWidget();
            }
            // 表达式模式：控件内部实时校验并经 expressionAccepted 写数据
            });

        // 5. 常数值完成编辑后同步
        connect(m_spbConstant, &QLineEdit::editingFinished, this, [this]() {
            if (m_motionDriveData)
                m_motionDriveData->setExpression(m_spbConstant->text());
            });

        // 6. 表达式构造器：合法时写数据；预览请求转采样绘制
        connect(m_exprBuilder, &mMotionExprBuilderWidget::expressionAccepted, this, [this](const QString& expr) {
            if (m_motionDriveData)
                m_motionDriveData->setExpression(expr);
            });
        connect(m_exprBuilder, &mMotionExprBuilderWidget::previewRequested, this, [this](const QString& expr) {
            previewExpressionCurve(expr);
            });

        // 7. 模板：类型切换 / 参数编辑 → 生成表达式写入数据
        connect(m_cmbTemplate, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
            refreshTemplateParams();
            applyTemplate();
            });
        for (int i = 0; i < 4; ++i) {
            connect(m_tplParam[i], &QLineEdit::editingFinished, this, [this]() {
                applyTemplate();
                });
        }

        // 9. 表格驱动：行编辑/增删即同步，预览独立按钮
        connect(m_btnTblAddRow, &QPushButton::clicked, this, [this]() {
            const int row = m_tblDrive->rowCount();
            m_tblDrive->insertRow(row);
            m_tblDrive->setItem(row, 0, new QTableWidgetItem("0.0"));
            m_tblDrive->setItem(row, 1, new QTableWidgetItem("0.0"));
            });
        connect(m_btnTblDelRow, &QPushButton::clicked, this, [this]() {
            const int row = m_tblDrive->currentRow();
            if (row >= 0) {
                m_tblDrive->removeRow(row);
                syncDriveTableFromWidget();
            }
            });
        connect(m_tblDrive, &QTableWidget::cellChanged, this, [this](int, int) {
            syncDriveTableFromWidget();
            });
        connect(m_btnTblPreview, &QPushButton::clicked, this, [this]() {
            previewDriveTable();
            });

        // 8. 模板页预览（表达式页预览在构造器控件内部触发）
        connect(m_btnPreviewTpl, &QPushButton::clicked, this, [this]() {
            previewExpressionCurve(m_lblTemplateExpr->text());
            });

        m_mainLayout->addStretch();
    }

    mMotionDrivePanel::~mMotionDrivePanel()
    {
        if (m_mainLayout != nullptr)
        {
            delete m_mainLayout;
            m_mainLayout = nullptr;
        }

    }

    void mMotionDrivePanel::newMotionDrivePanel(MxFunctions::DriveTargetType type, MechMotionData::DriveControlMode mode)
    {
        setEditing(true);
        connectSig();

        int motionDriveID = getCVM->getModelData()->getManager<mMotionDriveManager>(Physical_MotionDrive).createID();
        if (type == DriveTargetType::Joint)
        {
            m_motionDriveData = new mMotionJointDrive(motionDriveID);
        }
        else
        {
            //运动体驱动暂不支持（面板仅有运动副驱动编辑器），防御性返回
            return;
        }

        m_motionDriveData->setName(getCVM->getModelData()->createNameByType(Physical_MotionDrive, "驱动"));
        getCVM->getModelData()->addPhysicalField<mMotionDriveBase>(m_motionDriveData);

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionDriveSig(true, m_motionDriveData->getId());

        //菜单按"目标×控制模式"入口创建（位置/速度）：显式同步数据与下拉框。
        //加速度模式面板暂不支持（下拉仅两项），菜单侧入口已禁用，不会走到
        if (m_motionDriveData && static_cast<int>(mode) < m_cmbControlMode->count())
        {
            m_motionDriveData->setControlMode(mode);
            const QSignalBlocker blocker(m_cmbControlMode);
            m_cmbControlMode->setCurrentIndex(static_cast<int>(mode));
            refreshExcitationUnitLabel();
        }
    }

    void mMotionDrivePanel::editMotionDrivePanel(int id)
    {
        setEditing(true);
        m_motionDriveData = dynamic_cast<mMotionDriveBase*>(getCVM->getModelData()->getManager<mMotionDriveManager>(Physical_MotionDrive).getData(id));
        if (!m_motionDriveData)
            return;

        //emit MechCommon::mMechGlobalSignals::getInstance()->activePickSig(9);

        connectSig();

        updateUIFromData();
    }

    //数据对象即将被删除（树删除前调用）：置空编辑指针防悬垂（周期边界同款约定）
    void mMotionDrivePanel::onMotionDriveDataAboutToBeDestroyed(int id)
    {
        if (id >= 0)
        {
            if (m_motionDriveData == nullptr || id != m_motionDriveData->getId())
                return;
        }
        m_motionDriveData = nullptr;
    }

    void mMotionDrivePanel::uneditMotionDrivePanel()
    {
        if (!isEditing())
            return;
        setEditing(false);
        if (m_lastTargetJointId >= 0 && getCVM && getCVM->getPreRenderWidget()) {
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController) {
                QString symbolName = QString("motionJoint_%1").arg(m_lastTargetJointId);
                if (symbolController->isExistSymbol(symbolName)) {
                    symbolController->setSymbolShowState({ symbolName }, false);
                    symbolController->UpdateAllSymbol();
                    getCVM->getPreRenderWidget()->updatePaint();
                }
            }
        }
        m_lastTargetJointId = -1;
        m_motionDriveData = nullptr;
        disconnectSig();
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("jointPoint");
    }

    void mMotionDrivePanel::refreshTargetList(MxFunctions::DriveTargetType type)
    {
        const QSignalBlocker b(m_cmbTargetObject);
        m_cmbTargetObject->clear();
        m_cmbTargetObject->addItem(" ", -1); // 默认空白项

        if (type == MxFunctions::DriveTargetType::Joint) {
            // 伪代码：获取所有运动副并填入
             auto &driveManager = getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint);
             auto names = driveManager.getAllNamesByTypes({ MxFunctions::JointType::Revolute, MxFunctions::JointType::Cylindrical, MxFunctions::JointType::Translational, MxFunctions::JointType::Screw });
             for(QString name : names) {
                 m_cmbTargetObject->addItem(name, driveManager.getIDByName(name));
             }
             //球面副不能当做驱动

        }
        else {
            // 伪代码：获取所有运动体并填入
        }
    }

    // 激励数值单位随"运动方向 × 控制模式"刷新：转动=角度制（度），平动=长度制（mm）。
    // 与求解语义一致：转动驱动的数值/表达式/表格 y 轴按 °（°/s）输入，导出时统一换算弧度。
    void mMotionDrivePanel::refreshExcitationUnitLabel()
    {
        if (!m_lblExcitationUnit)
            return;

        const bool rotation = (m_cmbMotionType && m_cmbMotionType->currentIndex() == 1);
        const bool velocity = (m_cmbControlMode && m_cmbControlMode->currentIndex() == 1);

        if (rotation)
            m_lblExcitationUnit->setText(velocity ? "激励数值 (°/s):" : "激励数值 (°):");
        else
            m_lblExcitationUnit->setText(velocity ? "激励数值 (mm/s):" : "激励数值 (mm):");
    }

    void mMotionDrivePanel::updateUIFromData()
    {
        if (!getCVM)
        {
            return;
        }

        if (!m_motionDriveData)
            return;

        // 1. 还原目标类型
        auto targetType = m_motionDriveData->getTargetType();
        const QSignalBlocker bType(m_cmbTargetType);
        m_cmbTargetType->setCurrentIndex(targetType == DriveTargetType::Joint ? 0 : 1);

        // 确保列表数据是对的
        refreshTargetList(targetType);

            // 2. 还原目标ID
            int targetId = -1;
        if (targetType == DriveTargetType::Joint) {
            auto* jd = dynamic_cast<mMotionJointDrive*>(m_motionDriveData);
            targetId = mMotionRefMap::get(Physical_MotionDrive, m_motionDriveData->getId(), Physical_MotionJoint);
        }
        else {
            //auto* bd = dynamic_cast<mMotionBodyDrive*>(m_motionDriveData);
            //if (bd) targetId = bd->getTargetBodyId();
        }
        int objIndex = m_cmbTargetObject->findData(targetId);
        const QSignalBlocker bObj(m_cmbTargetObject);
        m_cmbTargetObject->setCurrentIndex(objIndex != -1 ? objIndex : 0);

        // 3. 还原物理语义
        const QSignalBlocker bMotion(m_cmbMotionType);
        m_cmbMotionType->setCurrentIndex(static_cast<int>(m_motionDriveData->getMotionType()));

        const QSignalBlocker bCtrl(m_cmbControlMode);
        m_cmbControlMode->setCurrentIndex(static_cast<int>(m_motionDriveData->getControlMode()));

        //单位标签须在两个语义下拉还原到数据值之后再刷新（此前刷新读到的是上一会话的旧索引，
        //从模型树 edit 进入时单位不跟随）
        refreshExcitationUnitLabel();

        // 4. 还原激励 (表格 > 表达式 > 常数)
        const QSignalBlocker bInputType(m_cmbInputType);
        if (m_motionDriveData->hasDriveTable()) {
            m_cmbInputType->setCurrentIndex(3);
            m_stkInputMode->setCurrentIndex(3);
            restoreTableWidget();
        }
        else {
        QString expr = m_motionDriveData->getExpression();
        bool isNumeric;
        double constVal = expr.toDouble(&isNumeric);

        if (isNumeric || expr.isEmpty()) {
            m_cmbInputType->setCurrentIndex(0);
            m_stkInputMode->setCurrentIndex(0);
            const QSignalBlocker bConst(m_spbConstant);
            m_spbConstant->setExpression(QString::number(isNumeric ? constVal : 0.0));
        }
        else {
            m_cmbInputType->setCurrentIndex(1);
            m_stkInputMode->setCurrentIndex(1);
            m_exprBuilder->setExpression(expr);
        }
        }

        triggerHighlight();
    }

    void mMotionDrivePanel::refreshTemplateParams()
    {
        // 各模板的参数标签与数量（多余的参数行隐藏）
        struct TplDef { const char* labels[4]; int count; };
        static const TplDef defs[] = {
            { { "幅值 A:", "频率 f(Hz):", "相位 p(rad):", "" }, 3 },      // 正弦
            { { "幅值 A:", "频率 f(Hz):", "相位 p(rad):", "" }, 3 },      // 余弦
            { { "斜率 k:", "截距 b:", "", "" }, 2 },                       // 斜坡
            { { "幅值 A:", "时刻 t0(s):", "过渡时长(s):", "" }, 3 },        // 阶跃（step 五参）
            { { "常数项 a:", "一次项 b:", "二次项 c:", "三次项 d:" }, 4 },  // 多项式
        };
        const int idx = m_cmbTemplate->currentIndex();
        if (idx < 0 || idx > 4) return;
        for (int i = 0; i < 4; ++i) {
            m_lblTplParam[i]->setText(defs[idx].labels[i]);
            m_lblTplParam[i]->setVisible(i < defs[idx].count);
            m_tplParam[i]->setVisible(i < defs[idx].count);
        }
    }

    void mMotionDrivePanel::applyTemplate()
    {
        auto fmt = [](MxFunctions::mCalcLineEdit* edit) -> QString {
            bool ok = false;
            const double v = edit->text().toDouble(&ok);
            return QString::number(ok ? v : 0.0, 'g', 12);
        };
        const QString A = fmt(m_tplParam[0]);
        const QString B = fmt(m_tplParam[1]);
        const QString C = fmt(m_tplParam[2]);
        const QString D = fmt(m_tplParam[3]);

        QString expr;
        switch (m_cmbTemplate->currentIndex()) {
        case 0: expr = QStringLiteral("%1*sin(2*pi*%2*t + %3)").arg(A, B, C); break;
        case 1: expr = QStringLiteral("%1*cos(2*pi*%2*t + %3)").arg(A, B, C); break;
        case 2: expr = QStringLiteral("%1*t + %2").arg(A, B); break;
        case 3: expr = QStringLiteral("step(t, %2, 0, %2 + %3, %1)").arg(A, B, C); break;
        case 4: expr = QStringLiteral("%1 + %2*t + %3*t^2 + %4*t^3").arg(A, B, C, D); break;
        default: return;
        }

        m_lblTemplateExpr->setText(expr);

        // 生成的表达式同样过校验（模板参数恒为数字，正常恒通过）
        motion::MotionFunction_Expr f(expr.toStdString());
        if (f.IsValid() && m_motionDriveData)
            m_motionDriveData->setExpression(expr);
    }

    void mMotionDrivePanel::syncDriveTableFromWidget()
    {
        if (!m_motionDriveData)
            return;

        // 收集合法行（两列均为数字）
        QVector<QPair<double, double>> rows;
        for (int r = 0; r < m_tblDrive->rowCount(); ++r) {
            auto* it = m_tblDrive->item(r, 0);
            auto* iv = m_tblDrive->item(r, 1);
            if (!it || !iv) continue;
            bool okT = false, okV = false;
            const double t = it->text().trimmed().toDouble(&okT);
            const double v = iv->text().trimmed().toDouble(&okV);
            if (okT && okV)
                rows.append({ t, v });
        }

        const int total = m_tblDrive->rowCount();
        if (rows.size() != total) {
            m_lblTblHint->setStyleSheet("color:#FF4D4F;");
            m_lblTblHint->setText(QString("有 %1 行含非法数值，已忽略").arg(total - rows.size()));
        }

        // 按时间排序并检查严格递增
        std::sort(rows.begin(), rows.end(),
            [](const QPair<double, double>& a, const QPair<double, double>& b) { return a.first < b.first; });
        bool monotonic = true;
        for (int i = 1; i < rows.size(); ++i) {
            if (!(rows[i].first > rows[i - 1].first)) {
                monotonic = false;
                break;
            }
        }

        if (rows.size() < 2 || !monotonic) {
            m_lblTblHint->setStyleSheet("color:#FF4D4F;");
            m_lblTblHint->setText(monotonic ? QStringLiteral("至少需要 2 行有效数据")
                                            : QStringLiteral("存在重复时间戳（需严格递增），表格未生效"));
            m_motionDriveData->setDriveTable({}, {});
            return;
        }

        QVector<double> xs, ys;
        xs.reserve(rows.size());
        ys.reserve(rows.size());
        for (const auto& p2 : rows) {
            xs.append(p2.first);
            ys.append(p2.second);
        }
        m_motionDriveData->setDriveTable(xs, ys);
        if (rows.size() == total) {
            m_lblTblHint->setStyleSheet("color:#67C23A;");
            m_lblTblHint->setText(QString("表格有效：%1 行，区间 [%2, %3] s")
                .arg(rows.size()).arg(xs.front()).arg(xs.back()));
        }
    }

    void mMotionDrivePanel::previewDriveTable()
    {
        if (!getCVM)
            return;
        syncDriveTableFromWidget();
        const QVector<double> xs = m_motionDriveData ? m_motionDriveData->getDriveTableX() : QVector<double>();
        const QVector<double> ys = m_motionDriveData ? m_motionDriveData->getDriveTableY() : QVector<double>();
        if (xs.size() < 2)
            return;

        // 预览时长：表格区间，或方案总时长（取较大者）
        double T = xs.back();
        auto& scenarioMgr = getCVM->getModelData()->getManager<mMotionScenarioManager>(Physical_MotionScenario);
        const auto ids = scenarioMgr.getAllIDs();
        if (!ids.empty()) {
            auto* scenario = scenarioMgr.getData(ids.front());
            if (scenario && scenario->getEndTime() > T)
                T = scenario->getEndTime();
        }

        motion::MotionFunction_Table f(xs.toStdVector(), ys.toStdVector());
        const int N = 400;
        QVector<double> pxs, pys;
        pxs.reserve(N);
        pys.reserve(N);
        for (int i = 0; i <= N; ++i) {
            const double t = T * i / N;
            pxs.append(t);
            pys.append(f.Get_y(t));
        }

        auto* curveWindow = mCurveWindow::getInstance();
        curveWindow->setPlotTitle(QStringLiteral("驱动表格预览"));
        curveWindow->SetAxisTitle(QStringLiteral("时间 t (s)"), true);
        curveWindow->updateCurve(pxs, pys, PlotView::LineChart,
            QStringLiteral("表格驱动（%1 行）").arg(xs.size()), true, true);
    }

    void mMotionDrivePanel::restoreTableWidget()
    {
        const QVector<double> xs = m_motionDriveData ? m_motionDriveData->getDriveTableX() : QVector<double>();
        const QVector<double> ys = m_motionDriveData ? m_motionDriveData->getDriveTableY() : QVector<double>();

        // 阻塞 cellChanged，避免还原期触发同步
        const QSignalBlocker b(m_tblDrive);
        m_tblDrive->setRowCount(0);
        for (int i = 0; i < qMin(xs.size(), ys.size()); ++i) {
            const int row = m_tblDrive->rowCount();
            m_tblDrive->insertRow(row);
            m_tblDrive->setItem(row, 0, new QTableWidgetItem(QString::number(xs[i], 'g', 12)));
            m_tblDrive->setItem(row, 1, new QTableWidgetItem(QString::number(ys[i], 'g', 12)));
        }
        syncDriveTableFromWidget();
    }

    void mMotionDrivePanel::previewExpressionCurve(const QString& expr)
    {
        motion::MotionFunction_Expr f(expr.toStdString());
        if (!f.IsValid() || !getCVM)
            return;

        // 预览时长：取第一个求解方案的总时间，无方案时 10 秒
        double T = 10.0;
        auto& scenarioMgr = getCVM->getModelData()->getManager<mMotionScenarioManager>(Physical_MotionScenario);
        const auto ids = scenarioMgr.getAllIDs();
        if (!ids.empty()) {
            auto* scenario = scenarioMgr.getData(ids.front());
            if (scenario && scenario->getEndTime() > 0.0)
                T = scenario->getEndTime();
        }

        const int N = 400;
        QVector<double> xs, ys;
        xs.reserve(N);
        ys.reserve(N);
        for (int i = 0; i <= N; ++i) {
            const double t = T * i / N;
            xs.push_back(t);
            ys.push_back(f.Get_y(t));
        }

        auto* curveWindow = mCurveWindow::getInstance();
        curveWindow->setPlotTitle(QStringLiteral("驱动激励预览"));
        curveWindow->SetAxisTitle(QStringLiteral("时间 t (s)"), true);
        curveWindow->updateCurve(xs, ys, PlotView::LineChart, expr, true, true);
    }

    void mMotionDrivePanel::triggerHighlight()
    {
        if (!m_motionDriveData)
            return;

        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (!symbolController)
            return;

        int targetId = -1;
        auto targetType = m_motionDriveData->getTargetType();
        if (targetType == MxFunctions::DriveTargetType::Joint) {
            auto* jd = dynamic_cast<mMotionJointDrive*>(m_motionDriveData);
            if (jd)
                targetId = mMotionRefMap::get(Physical_MotionDrive, m_motionDriveData->getId(), Physical_MotionJoint);

            // 如果目标运动副改变，先隐藏旧符号（不删除，因为符号属于运动副面板）
            if (m_lastTargetJointId >= 0 && m_lastTargetJointId != targetId) {
                QString oldSymbol = QString("motionJoint_%1").arg(m_lastTargetJointId);
                if (symbolController->isExistSymbol(oldSymbol)) {
                    symbolController->setSymbolShowState({ oldSymbol }, false);
                }
            }
            m_lastTargetJointId = targetId;

            if (targetId == -1) {
                //旧符号的隐藏状态需 UpdateAllSymbol 推送到渲染才生效（选空目标时清掉运动副符号）
                symbolController->UpdateAllSymbol();
                getCVM->getPreRenderWidget()->updatePaint();
                return;
            }

            auto* basicJoint = dynamic_cast<mMotionJointBasic*>(getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint).getData(targetId));
            auto* screwJoint = dynamic_cast<mMotionJointScrew*>(getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint).getData(targetId));
            auto* universalJoint = dynamic_cast<mMotionJointUniversal*>(getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint).getData(targetId));
            auto* perpendicularJoint = dynamic_cast<mMotionJointPerpendicular*>(getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint).getData(targetId));
            auto* constantVelocityJoint = dynamic_cast<mMotionJointConstantVelocity*>(getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint).getData(targetId));

            QString symbolName = QString("motionJoint_%1").arg(targetId);
            bool symbolHandled = false;

            if (basicJoint) {
                if (symbolController->isExistSymbol(symbolName)) {
                    symbolController->setSymbolShowState({ symbolName }, true);
                }
                else {
                    mMotionJointSymbolHelper::createOrUpdateJointSymbol(symbolController,
                        targetId,
                        basicJoint->getType(),
                        basicJoint->getName(),
                        basicJoint->getOrigin(),
                        basicJoint->getAxis(),
                        getCVM->getPreRenderWidget());
                }
                symbolHandled = true;
            }
            else if (screwJoint) {
                if (symbolController->isExistSymbol(symbolName)) {
                    symbolController->setSymbolShowState({ symbolName }, true);
                }
                else {
                    mMotionJointSymbolHelper::createOrUpdateJointSymbol(symbolController,
                        targetId,
                        screwJoint->getType(),
                        screwJoint->getName(),
                        screwJoint->getOrigin(),
                        screwJoint->getAxis(),
                        getCVM->getPreRenderWidget());
                }
                symbolHandled = true;
            }
            else if (universalJoint) {
                if (symbolController->isExistSymbol(symbolName)) {
                    symbolController->setSymbolShowState({ symbolName }, true);
                }
                else {
                    mMotionJointSymbolHelper::createOrUpdateUniversalSymbol(symbolController,
                        targetId,
                        universalJoint->getName(),
                        universalJoint->getOrigin(),
                        universalJoint->getActionBodySpinVector(),
                        universalJoint->getActionBodyPinVector(),
                        universalJoint->getBaseBodySpinVector(),
                        universalJoint->getBaseBodyPinVector(),
                        getCVM->getPreRenderWidget());
                }
                symbolHandled = true;
            }
            else if (perpendicularJoint) {
                if (symbolController->isExistSymbol(symbolName)) {
                    symbolController->setSymbolShowState({ symbolName }, true);
                }
                else {
                    mMotionJointSymbolHelper::createOrUpdatePerpendicularSymbol(symbolController,
                        targetId,
                        perpendicularJoint->getName(),
                        perpendicularJoint->getOrigin(),
                        perpendicularJoint->getActionBodyVector(),
                        perpendicularJoint->getBaseBodyVector(),
                        getCVM->getPreRenderWidget());
                }
                symbolHandled = true;
            }
            else if (constantVelocityJoint) {
                if (symbolController->isExistSymbol(symbolName)) {
                    symbolController->setSymbolShowState({ symbolName }, true);
                }
                else {
                    mMotionJointSymbolHelper::createOrUpdateConstantVelocitySymbol(symbolController,
                        targetId,
                        constantVelocityJoint->getName(),
                        constantVelocityJoint->getOrigin(),
                        constantVelocityJoint->getActionBodyVector(),
                        constantVelocityJoint->getBaseBodyVector(),
                        getCVM->getPreRenderWidget());
                }
                symbolHandled = true;
            }

            if (symbolHandled) {
                symbolController->UpdateAllSymbol();
                getCVM->getPreRenderWidget()->updatePaint();
            }
        }
        else {
            //目标类型切到运动体：清理残留的目标运动副符号显示
            if (m_lastTargetJointId >= 0) {
                QString oldSymbol = QString("motionJoint_%1").arg(m_lastTargetJointId);
                if (symbolController->isExistSymbol(oldSymbol)) {
                    symbolController->setSymbolShowState({ oldSymbol }, false);
                }
                symbolController->UpdateAllSymbol();
                getCVM->getPreRenderWidget()->updatePaint();
                m_lastTargetJointId = -1;
            }
        }
    }

    void mMotionDrivePanel::handleTargetTypeChange(DriveTargetType newType)
    {
        if (!m_motionDriveData || m_motionDriveData->getTargetType() == newType) return;

        // 异构数据迁移
        int id = m_motionDriveData->getId();
        QString name = m_motionDriveData->getName();
        auto motionType = m_motionDriveData->getMotionType();
        auto ctrlMode = m_motionDriveData->getControlMode();
        QString expr = m_motionDriveData->getExpression();

        mMotionDriveBase* newDriveObj = nullptr;
        if (newType == DriveTargetType::Joint) {
            newDriveObj = new mMotionJointDrive(id, name);
        }
        else {
            //newDriveObj = new mMotionBodyDrive(id, name);
        }

        if (newDriveObj) {
            newDriveObj->setMotionType(motionType);
            newDriveObj->setControlMode(ctrlMode);
            newDriveObj->setExpression(expr);

            // 伪代码：通知全局 Manager 替换旧指针
            // getCVM->getModelData()->getManager<mMotionDriveManager>()->replaceData(newDriveObj);

            m_motionDriveData = newDriveObj;

            // 目标类型变了，下拉框里的列表必须刷新
            refreshTargetList(newType);
            updateUIFromData();
        }

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionDriveSig(false, m_motionDriveData->getId());
    }

    void mMotionDrivePanel::autoLockMotionType(int targetJointId)
    {
        auto &jointManager = getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint);
        auto joint = jointManager.getData(targetJointId);
        if (joint)
        {
            const QSignalBlocker b(m_cmbMotionType);
            if (joint->getType() == JointType::Revolute) {
                m_cmbMotionType->setCurrentIndex(1); // 转动
                m_cmbMotionType->setEnabled(false);  // 锁定不让改
                if (m_motionDriveData) m_motionDriveData->setMotionType(DriveMotionType::Rotation);
            }
            else if (joint->getType() == JointType::Translational) {
                m_cmbMotionType->setCurrentIndex(0); // 平动
                m_cmbMotionType->setEnabled(false);
                if (m_motionDriveData) m_motionDriveData->setMotionType(DriveMotionType::Translation);
            }
            else
            {
                m_cmbMotionType->setEnabled(true);// 其他类型放开限制
            }
        }
    }

    void mMotionDrivePanel::connectSig()
    {
        disconnectSig();
        if (!getCVM || !getCVM->getPreRenderWidget())
        {
            return;
        }

        conn_geo = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mMotionDrivePanel::slot_finishedPick);
    }

    void mMotionDrivePanel::disconnectSig()
    {
        if (conn_geo)
            disconnect(conn_geo);
    }

    void mMotionDrivePanel::slot_finishedPick()
    {
        if (!getCVM)
        {
            return;
        }
        //重置面板，类型根据拾取模式来确定
        int pickNum = 0;
        auto pickFilter = getCVM->getCurrentPickDataType(pickNum);
        if (pickFilter == PickDataType::None)
        {
            return;
        }
    }

    void mMotionDrivePanel::slot_subWindowActivated(QMdiSubWindow* subWindow)
    {
        if (!getCVM)
        {
            return;
        }
        //if (getCVM == _currentViewportModel)
        //{
        //    return;
        //}

        ////获取当前视口的数据
        //_currentViewportModel = getCVM;
    }
}
