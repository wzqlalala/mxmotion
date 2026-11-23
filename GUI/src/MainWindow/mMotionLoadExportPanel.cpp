
#include "mMotionLoadExportPanel.h"
#include "mParameterPageLayout.h"
#include "mMotionRefMap.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"
#include "mMechGlobalSignals.h"
#include "mMotionResultQueryService.h"
#include "mMotionLoadCaseSelector.h"
#include "mMotionJointSymbolHelper.h"
#include "mMotionLoadCaseGenerator.h"
#include "mGeneralPickUp.h"
#include "mPickWidgetController.h"
#include "mGetSetTypeTemplate.h"

#include "PickCommonFunction.h"
#include "mxr/label/mSymbolController.h"

#include "mModelData.h"
#include "mMotionScenario.h"
#include "mMotionScenarioManager.h"
#include "mMotionJointManager.h"
#include "mMotionJointBase.h"
#include "mMotionJointBasic.h"
#include "mMotionBodyManager.h"
#include "mMotionBody.h"
#include "mMotionCouplingManager.h"
#include "mMotionCouplingBase.h"
#include "mMotionDriveManager.h"
#include "mMotionDriveBase.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QTextStream>
#ifdef MX_MOTION_OCC_COMPAT
#include "NativeMotionLoadExport.h"
#include "picking/LegacyPickContext.h"
#include "model/LegacyMotionContext.h"
#include <QSaveFile>
#include <QTimer>
#endif

using namespace MxFunctions;

namespace MechMainWindow
{
    // 把一个来源对象从结果里取出全时程反力曲线（运动副 I 端约束反力）
    static mMotionResultQueryService::LoadSeries queryJointSeries(
        MechMotionData::mMotionResultData* result, int jointId)
    {
        return mMotionResultQueryService::getLoadSeries(
            result, MechMotionData::CurveSourceType::Joint, jointId);
    }

    mMotionLoadExportPanel::mMotionLoadExportPanel(QWidget* parent) : mPanelBase(parent)
    {
        m_scenarioId = -1;

        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // ========== 顶部：导出模式切换 ==========
        FoldSection* modeSection = new FoldSection("导出模式", this);
        m_comboExportMode = new QComboBox();
        m_comboExportMode->addItems({ "手动提取", "特征工况导出", "连杆同步工况导出" });
        modeSection->addRow(new QLabel("模式"), m_comboExportMode);
        mainLayout->addWidget(modeSection);

        auto* exportParameters = new QWidget(this);
        m_modeStack = new ParameterPageLayout(exportParameters);

        // ==================================================
        // 模式一：手动提取（原功能）
        // ==================================================
        QWidget* manualPage = new QWidget(this);
        QVBoxLayout* manualLayout = new QVBoxLayout(manualPage);
        manualLayout->setContentsMargins(0, 0, 0, 0);
        manualLayout->setSpacing(4);

        FoldSection* basicSection = new FoldSection("基本设置", manualPage);
        m_editName = new QLineEdit();
        m_editName->setPlaceholderText("LoadCase_1");
        basicSection->addRow(new QLabel("载荷工况名称"), m_editName);

        m_comboSourceType = new QComboBox();
        m_comboSourceType->addItems({ "运动副", "耦合副", "驱动器" });
        basicSection->addRow(new QLabel("载荷来源类型"), m_comboSourceType);

        m_comboSourceId = new QComboBox();
        basicSection->addRow(new QLabel("载荷来源对象"), m_comboSourceId);

        m_btnGroupMarker = new QButtonGroup(manualPage);
        m_radioIMarker = new QRadioButton("I Marker");
        m_radioJMarker = new QRadioButton("J Marker");
        m_radioIMarker->setChecked(true);
        m_btnGroupMarker->addButton(m_radioIMarker, 0);
        m_btnGroupMarker->addButton(m_radioJMarker, 1);
        basicSection->addRow(new QLabel("作用端"), m_radioIMarker, m_radioJMarker);

        m_editNodeId = new QLineEdit();
        m_editNodeId->setPlaceholderText("3091");
#ifdef MX_MOTION_OCC_COMPAT
        m_editNodeId->setToolTip(QStringLiteral("手动输入仅指定输出节点编号，力矩关于所选 Marker；拾取节点则按该节点的随体位置逐帧转换力矩。拾取的节点必须属于唯一运动体。"));
        connect(m_editNodeId,&QLineEdit::textEdited,this,[this] {
            m_editNodeId->setProperty("mxPickedNodeId",QVariant{});
            m_editNodeId->setProperty("mxPickedNodePartId",QVariant{});
            m_editNodeId->setProperty("mxPickedNodePosition",QVariant{});
        });
#endif
        m_btnPickNode = new QPushButton("拾取");
        basicSection->addRow(new QLabel("节点编号"), m_editNodeId, m_btnPickNode);
#ifdef MX_MOTION_OCC_COMPAT
        // The original visible button keeps its layout. Its previously
        // unconnected action now uses the same original controller as the
        // other panels, resolving real mesh node IDs and coordinates.
        auto* nodePick=mGeneralPickUp::createOnePickWidget(CBRed,this);
        nodePick->setObjectName(QStringLiteral("originalLoadNodePickController"));
        nodePick->setAllowedTypes(&MechCommon::pickOnlyNode);
        nodePick->setKeepHighlightOnDeactivate(false);
        nodePick->widget()->hide();
        connect(m_btnPickNode,&QPushButton::clicked,this,[nodePick] {
            if(nodePick->isPickActive())nodePick->setPickActive(false);
            else { nodePick->displayData(PickWidgetData{});nodePick->setPickActive(true); }
        });
        connect(nodePick,&mPickWidgetController::dataChanged,this,[this,nodePick] {
            auto* context=nodePick->context();
            if(!context)return;
            const auto picks=context->resolve(nodePick->getAllData().pickedData);
            if(picks.isEmpty() || picks.back().kind!=MotionOcc::PickKind::Node)return;
            const auto& pick=picks.back();
            m_editNodeId->setText(QString::number(pick.nodeId));
            m_editNodeId->setProperty("mxPickedNodeId",pick.nodeId);
            m_editNodeId->setProperty("mxPickedNodePartId",pick.partId);
            m_editNodeId->setProperty("mxPickedNodePosition",QVariantList{pick.position[0],pick.position[1],pick.position[2]});
            QTimer::singleShot(0,nodePick,[nodePick] { nodePick->setPickActive(false); });
        });
#endif
        manualLayout->addWidget(basicSection);

        FoldSection* extractSection = new FoldSection("载荷提取", manualPage);
        m_btnGroupMode = new QButtonGroup(manualPage);
        m_radioMaxFrame = new QRadioButton("最大载荷帧");
        m_radioSingleFrame = new QRadioButton("指定帧");
        m_radioAllFrames = new QRadioButton("全部帧");
        m_radioMaxFrame->setChecked(true);
        m_btnGroupMode->addButton(m_radioMaxFrame, 0);
        m_btnGroupMode->addButton(m_radioSingleFrame, 1);
        m_btnGroupMode->addButton(m_radioAllFrames, 2);
        extractSection->addRow(new QLabel("提取模式"), m_radioMaxFrame, m_radioSingleFrame, m_radioAllFrames);

        m_spinFrameIndex = new QSpinBox();
        m_spinFrameIndex->setRange(0, 999999);
        m_spinFrameIndex->setEnabled(false);
        extractSection->addRow(new QLabel("帧编号"), m_spinFrameIndex);
        manualLayout->addWidget(extractSection);

        FoldSection* outputSection = new FoldSection("输出设置", manualPage);
        m_comboFormat = new QComboBox();
        m_comboFormat->addItems({ "CSV (通用)", "Nastran BDF (载荷卡片)" });
        outputSection->addRow(new QLabel("导出格式"), m_comboFormat);

        m_chkExportForce = new QCheckBox("输出力 (Fx/Fy/Fz)");
        m_chkExportForce->setChecked(true);
        outputSection->addRow(m_chkExportForce);

        m_chkExportTorque = new QCheckBox("输出力矩 (Mx/My/Mz)");
        m_chkExportTorque->setChecked(true);
        outputSection->addRow(m_chkExportTorque);

        m_chkFx = new QCheckBox("Fx");
        m_chkFy = new QCheckBox("Fy");
        m_chkFz = new QCheckBox("Fz");
        m_chkFx->setChecked(true);
        m_chkFy->setChecked(true);
        m_chkFz->setChecked(true);
        outputSection->addRow(new QLabel("力分量"), m_chkFx, m_chkFy, m_chkFz);

        m_chkMx = new QCheckBox("Mx");
        m_chkMy = new QCheckBox("My");
        m_chkMz = new QCheckBox("Mz");
        m_chkMx->setChecked(true);
        m_chkMy->setChecked(true);
        m_chkMz->setChecked(true);
        outputSection->addRow(new QLabel("力矩分量"), m_chkMx, m_chkMy, m_chkMz);

        m_chkUseGlobal = new QCheckBox("全局坐标系");
        m_chkUseGlobal->setChecked(true);
        m_chkUseGlobal->setEnabled(false);
        outputSection->addRow(m_chkUseGlobal);

        m_spinScale = new QDoubleSpinBox();
        m_spinScale->setRange(-1e6, 1e6);
        m_spinScale->setDecimals(4);
        m_spinScale->setValue(1.0);
        outputSection->addRow(new QLabel("缩放系数"), m_spinScale);
        manualLayout->addWidget(outputSection);

        m_btnExport = new QPushButton("导出载荷文件");
        manualLayout->addWidget(m_btnExport);
        manualLayout->addStretch();

        // ==================================================
        // 模式二/三：特征工况导出 / 连杆同步工况导出（共用建页）
        // ==================================================
        QWidget* casePage = buildCasePage(m_casePage, false);
        QWidget* syncPage = buildCasePage(m_syncPage, true);

        // ==================================================
        m_modeStack->addWidget(manualPage);
        m_modeStack->addWidget(casePage);
        m_modeStack->addWidget(syncPage);
        mainLayout->addWidget(exportParameters);
        mainLayout->addStretch();

        // 信号连接
        connect(m_comboExportMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &mMotionLoadExportPanel::onExportModeChanged);
        connect(m_comboSourceType, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &mMotionLoadExportPanel::onSourceTypeChanged);
        connect(m_btnGroupMode, QOverload<int>::of(&QButtonGroup::buttonClicked),
                this, &mMotionLoadExportPanel::onModeChanged);
        connect(m_btnExport, &QPushButton::clicked,
                this, &mMotionLoadExportPanel::onExportClicked);

        onExportModeChanged(0);
    }

    QWidget* mMotionLoadExportPanel::buildCasePage(CasePageCtx& ctx, bool synchronized)
    {
        QWidget* page = new QWidget(this);
        QVBoxLayout* pageLayout = new QVBoxLayout(page);
        pageLayout->setContentsMargins(0, 0, 0, 0);
        pageLayout->setSpacing(4);

        FoldSection* selSection = new FoldSection("载荷来源", page);
        ctx.sourceSection = selSection;
        ctx.comboBody = new QComboBox();
        selSection->addRow(new QLabel("运动体"), ctx.comboBody);

        ctx.jointCheckArea = new QWidget(page);
        ctx.jointCheckLayout = new QVBoxLayout(ctx.jointCheckArea);
        ctx.jointCheckLayout->setContentsMargins(8, 2, 2, 2);
        ctx.jointCheckLayout->setSpacing(2);
        selSection->addRow(new QLabel("运动副"), ctx.jointCheckArea);
        pageLayout->addWidget(selSection);

        // 几何面绑定（免导入生成用）：行随运动副复选框同步重建（refreshJointChecks）
        ctx.faceSection = new FoldSection("运动副几何面", page);
        pageLayout->addWidget(ctx.faceSection);

        FoldSection* paramSection = new FoldSection("筛选参数", page);
        ctx.spinPeaks = new QSpinBox();
        ctx.spinPeaks->setRange(1, 32);
        ctx.spinPeaks->setValue(4);
        paramSection->addRow(new QLabel("每条曲线极值点数"), ctx.spinPeaks);

        ctx.spinMergeTol = new QDoubleSpinBox();
        ctx.spinMergeTol->setDecimals(9);
        ctx.spinMergeTol->setRange(0.0, 1.0);
        ctx.spinMergeTol->setValue(1e-9);
        ctx.spinMergeTol->setSingleStep(1e-9);
        QLabel* tolLabel = new QLabel("时刻合并容差");
        tolLabel->setToolTip(QStringLiteral("单位：秒"));
        paramSection->addRow(tolLabel, ctx.spinMergeTol);
        pageLayout->addWidget(paramSection);

        ctx.chkAutoGenerate = new QCheckBox(QStringLiteral("直接生成静力工况"));
        ctx.chkAutoGenerate->setToolTip(QStringLiteral(
            "勾选后导出时直接在当前模型生成隐式静力工况，需先为每个勾选运动副拾取几何面"));
        pageLayout->addWidget(ctx.chkAutoGenerate);

        ctx.chkExportJson = new QCheckBox(QStringLiteral("同时导出工况 JSON"));
        ctx.chkExportJson->setChecked(true);
        pageLayout->addWidget(ctx.chkExportJson);

        // 按钮文案随选择联动：只生成不导 JSON 时不再叫"导出 JSON"
        auto updateExportBtnText = [this, &ctx, synchronized]() {
            if (!ctx.btnExport)
                return;
            const bool genOnly = ctx.chkAutoGenerate && ctx.chkAutoGenerate->isChecked()
                && ctx.chkExportJson && !ctx.chkExportJson->isChecked();
            ctx.btnExport->setText(genOnly
                ? QStringLiteral("筛选并生成静力工况")
                : (synchronized ? QStringLiteral("筛选并导出同步工况 JSON")
                                : QStringLiteral("筛选并导出工况 JSON")));
        };
        connect(ctx.chkAutoGenerate, &QCheckBox::toggled, this, updateExportBtnText);
        connect(ctx.chkExportJson, &QCheckBox::toggled, this, updateExportBtnText);

        ctx.btnExport = new QPushButton(synchronized
            ? QStringLiteral("筛选并导出同步工况 JSON")
            : QStringLiteral("筛选并导出工况 JSON"));
        pageLayout->addWidget(ctx.btnExport);
        pageLayout->addStretch();

        connect(ctx.comboBody, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this, &ctx](int) { refreshJointChecks(ctx); });
        connect(ctx.btnExport, &QPushButton::clicked,
            this, &mMotionLoadExportPanel::onExportCaseClicked);
        return page;
    }

    mMotionLoadExportPanel::CasePageCtx* mMotionLoadExportPanel::currentCasePage()
    {
        return m_comboExportMode->currentIndex() == 2 ? &m_syncPage : &m_casePage;
    }

    void mMotionLoadExportPanel::highlightSelection()
    {
        if (!isEditing() || !getCVM || !getCVM->getPreRenderWidget())
            return;
        CasePageCtx* ctx = currentCasePage();
        if (!ctx)
            return;
        // 几何面拾取激活期间让位：避免部件选择集的整体重绘干扰正在进行的面拾取
        for (auto it = ctx->jointFacePicks.begin(); it != ctx->jointFacePicks.end(); ++it)
            if (it.value() && it.value()->isPickActive())
                return;
        auto* widget = getCVM->getPreRenderWidget();

        // 1) 体部件：拾取选择集覆盖式同步高亮（同运动体/截面属性面板的互通机制）
        std::vector<int> partIds;
        const int bodyId = ctx->comboBody->currentData().toInt();
        if (bodyId >= 0) {
            const auto partSet = mMotionRefMap::getSet(Physical_MotionBody, bodyId, Physical_Part);
            partIds.assign(partSet.begin(), partSet.end());
        }
        MechPreData::reducePreWidgetSelectPartID(widget, MechPreData::getPreWidgetSelectPartID(widget));
        if (!partIds.empty())
            MechPreData::addPreWidgetSelectPartID(widget, partIds);
        MechPreData::UpdatePreWidgetHighlight(widget);

        // 2) 运动副：勾选的确保符号存在并显示，未勾选的隐藏（借用模式，退出面板时统一隐藏）
        auto* symbolController = widget->GetSymbolController();
        if (symbolController) {
            auto& jointMgr = getCVM->getModelData()
                ->getManager<MechMotionData::mMotionJointManager>(Physical_MotionJoint);
            for (auto it = ctx->jointChecks.begin(); it != ctx->jointChecks.end(); ++it) {
                const QString symbolName = QString("motionJoint_%1").arg(it.key());
                if (it.value()->isChecked()) {
                    auto* joint = jointMgr.getData(it.key());
                    if (joint) {
                        mMotionJointSymbolHelper::ensureJointSymbolByData(symbolController, joint, widget);
                        symbolController->setSymbolShowState({ symbolName }, true);
                        m_shownJointSymbols.insert(symbolName);
                    }
                }
                else {
                    symbolController->setSymbolShowState({ symbolName }, false);
                    m_shownJointSymbols.remove(symbolName);
                }
            }
            symbolController->UpdateAllSymbol();
        }
        widget->updatePaint();
    }

    void mMotionLoadExportPanel::clearViewportHighlight()
    {
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;
        auto* widget = getCVM->getPreRenderWidget();

        // 清体部件拾取集高亮
        MechPreData::reducePreWidgetSelectPartID(widget, MechPreData::getPreWidgetSelectPartID(widget));
        MechPreData::UpdatePreWidgetHighlight(widget);

        // 隐藏本面板显示过的副符号（符号本体保留，树上点击可再显示）
        auto* symbolController = widget->GetSymbolController();
        if (symbolController && !m_shownJointSymbols.isEmpty()) {
            std::vector<QString> names(m_shownJointSymbols.begin(), m_shownJointSymbols.end());
            symbolController->setSymbolShowState(names, false);
            symbolController->UpdateAllSymbol();
        }
        m_shownJointSymbols.clear();
        widget->updatePaint();
    }

    mMotionLoadExportPanel::~mMotionLoadExportPanel()
    {
    }

    void mMotionLoadExportPanel::editLoadExportPanel(int id)
    {
        setEditing(true);
        m_scenarioId = id;

        if (!getCVM || !getCVM->getModelData()) {
            refreshSourceObjects();
            refreshResultState();
            refreshBodies();
            return;
        }

        auto* scenario = getCVM->getModelData()->getManager<MechMotionData::mMotionScenarioManager>(
            Physical_MotionScenario).getData(m_scenarioId);
        if (scenario && m_editName->text().trimmed().isEmpty()) {
            m_editName->setPlaceholderText(QString("%1_LoadCase").arg(scenario->getName()));
        }

        refreshSourceObjects();
        refreshResultState();
        refreshBodies();
    }

    void mMotionLoadExportPanel::uneditLoadExportPanel()
    {
        if (!isEditing())
            return;
        setEditing(false);
        m_scenarioId = -1;
#ifdef MX_MOTION_OCC_COMPAT
        if(auto* pick=findChild<mPickWidgetController*>(QStringLiteral("originalLoadNodePickController")))
            pick->setPickActive(false);
#endif
        // 失活并清空两页的几何面拾取控件：激活中的拾取全局互斥占用拾取会话，
        // 拾取数据是临时的，退出编辑清空防残留高亮与串到下个面板
        for (CasePageCtx* pageCtx : { &m_casePage, &m_syncPage }) {
            for (auto it = pageCtx->jointFacePicks.begin(); it != pageCtx->jointFacePicks.end(); ++it) {
                if (!it.value())
                    continue;
                it.value()->setPickActive(false);
                it.value()->displayData(PickWidgetData());
            }
        }
        clearViewportHighlight();  // 退出面板清掉体部件高亮与显示中的副符号
    }

    void mMotionLoadExportPanel::onExportModeChanged(int index)
    {
        m_modeStack->setCurrentIndex(index);
        // 切到工况页按当前页选择联动高亮；切回手动页则清掉联动高亮
        if (index == 1 || index == 2)
            highlightSelection();
        else
            clearViewportHighlight();
    }

    // ==================== 手动提取（原逻辑） ====================

    void mMotionLoadExportPanel::onSourceTypeChanged(int index)
    {
        Q_UNUSED(index)
        refreshSourceObjects();
    }

    void mMotionLoadExportPanel::onModeChanged(int id)
    {
        m_spinFrameIndex->setEnabled(id == 1);
    }

    void mMotionLoadExportPanel::onExportClicked()
    {
        if (!getCVM || !getCVM->getModelData() || m_scenarioId < 0)
            return;

        auto* scenario = getCVM->getModelData()->getManager<MechMotionData::mMotionScenarioManager>(
            Physical_MotionScenario).getData(m_scenarioId);
        if (!scenario || !scenario->hasResult()) {
            QMessageBox::warning(this, tr("提示"), tr("当前方案没有求解结果，无法导出载荷。"));
            return;
        }

        const int sourceId = m_comboSourceId->currentData().toInt();
        if (sourceId < 0) {
            QMessageBox::warning(this, tr("提示"), tr("请选择载荷来源对象。"));
            return;
        }

        if (!m_chkExportForce->isChecked() && !m_chkExportTorque->isChecked()) {
            QMessageBox::warning(this, tr("提示"), tr("请至少选择一种输出载荷类型。"));
            return;
        }

        const auto loadSeries = mMotionResultQueryService::getLoadSeries(
            scenario->getResult(), getCurrentSourceType(), sourceId);
        if (!loadSeries.isValid()) {
            QMessageBox::warning(this, tr("提示"), tr("求解结果中没有当前来源对象的载荷数据。"));
            return;
        }

        QVector<int> frameIndices;
        if (m_radioAllFrames->isChecked()) {
            frameIndices.reserve(loadSeries.times.size());
            for (int i = 0; i < loadSeries.times.size(); ++i)
                frameIndices.push_back(i);
        }
        else {
            const int frameIndex = m_radioMaxFrame->isChecked()
                ? getMaxLoadFrameIndex(loadSeries)
                : m_spinFrameIndex->value();
            if (frameIndex < 0 || frameIndex >= loadSeries.times.size()) {
                QMessageBox::warning(this, tr("提示"), tr("帧编号超出求解结果范围。"));
                return;
            }
            frameIndices.push_back(frameIndex);
        }

        const bool exportBdf = m_comboFormat->currentIndex() == 1;
        if (exportBdf) {
            bool nodeIdOk = false;
            const int nodeId = m_editNodeId->text().trimmed().toInt(&nodeIdOk);
            if (!nodeIdOk || nodeId <= 0) {
                QMessageBox::warning(this, tr("提示"),
                    tr("Nastran BDF 格式需要填写有效的有限元节点编号（正整数）。"));
                return;
            }
        }

        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_recentDir;
        if (defaultDir.isEmpty() || !QFileInfo(defaultDir).exists()) {
            defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
        }

        QString fileName = QFileDialog::getSaveFileName(
            this,
            tr("导出载荷数据"),
            defaultDir,
            exportBdf ? tr("Nastran BDF(*.bdf *.dat)") : tr("CSV File(*.csv)"));
        if (fileName.isEmpty())
            return;
        if (QFileInfo(fileName).suffix().isEmpty())
            fileName += exportBdf ? ".bdf" : ".csv";

#ifdef MX_MOTION_OCC_COMPAT
        if (!getCVM->exportMotionLoads) {
            QMessageBox::warning(this, tr("提示"), tr("当前方案的载荷结果服务未连接。"));
            return;
        }
        MotionOcc::NativeLoadExportOptions options;
        options.name = m_editName->text().trimmed().isEmpty()
            ? m_editName->placeholderText() : m_editName->text().trimmed();
        options.end = m_radioJMarker->isChecked() ? MotionOcc::NativeLoadEnd::J : MotionOcc::NativeLoadEnd::I;
        options.frames = m_radioAllFrames->isChecked() ? MotionOcc::NativeLoadFrames::All
            : m_radioSingleFrame->isChecked() ? MotionOcc::NativeLoadFrames::Specified : MotionOcc::NativeLoadFrames::Maximum;
        options.frameIndex = std::size_t(m_spinFrameIndex->value());
        options.format = exportBdf ? MotionOcc::NativeLoadFormat::NastranBdf : MotionOcc::NativeLoadFormat::Csv;
        options.nodeId = m_editNodeId->text().trimmed().toInt();
        options.scale = m_spinScale->value();
        const auto target=m_editNodeId->property("mxPickedNodePosition").toList();
        if(options.nodeId>0 && m_editNodeId->property("mxPickedNodeId").toInt()==options.nodeId && target.size()==3) {
            const int partId=m_editNodeId->property("mxPickedNodePartId").toInt();
            const auto* context=getCVM->getModelData()->context;
            std::set<int> owners;
            if(context) {
                const auto& bindings=context->document().bodyPartBindings;
                for(auto entry=bindings.begin();entry!=bindings.end();++entry)
                    if(entry.value().contains(partId))owners.insert(entry.key());
            }
            if(owners.size()!=1) {
                QMessageBox::warning(this,tr("提示"),tr("所拾取的目标节点没有唯一的运动体绑定，无法确定其逐帧位置。请检查部件与运动体的对应关系。"));
                return;
            }
            options.useTargetPoint=true;
            options.targetBodyId=*owners.begin();
            for(int component=0;component<3;++component)options.targetPointMm[component]=target[component].toDouble();
        }
        options.components = {{
            m_chkExportForce->isChecked() && m_chkFx->isChecked(),
            m_chkExportForce->isChecked() && m_chkFy->isChecked(),
            m_chkExportForce->isChecked() && m_chkFz->isChecked(),
            m_chkExportTorque->isChecked() && m_chkMx->isChecked(),
            m_chkExportTorque->isChecked() && m_chkMy->isChecked(),
            m_chkExportTorque->isChecked() && m_chkMz->isChecked()}};
        const QStringList collections{QStringLiteral("joints"),QStringLiteral("couplings"),QStringLiteral("drives")};
        const MotionOcc::NativeLoadSource source{collections.value(m_comboSourceType->currentIndex()),sourceId};
        QString error;
        if (!getCVM->exportMotionLoads(m_scenarioId,fileName,source,options,error)) {
            QMessageBox::warning(this,tr("提示"),error);
            return;
        }
        MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = QFileInfo(fileName).absolutePath();
#else
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QMessageBox::warning(this, tr("提示"), tr("无法写入载荷文件。"));
            return;
        }

        MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = QFileInfo(fileName).absolutePath();

        const double effectiveScale = m_spinScale->value() * (m_radioJMarker->isChecked() ? -1.0 : 1.0);

        QTextStream out(&file);
        out.setCodec("UTF-8");
        if (exportBdf)
            writeBdfContent(out, loadSeries, frameIndices, effectiveScale);
        else
            writeCsvContent(out, loadSeries, frameIndices, effectiveScale);
        file.close();
#endif

        QMessageBox::information(this, tr("提示"),
            tr("载荷导出完成：已写入 %1 帧到\n%2")
                .arg(frameIndices.size())
                .arg(QDir::toNativeSeparators(fileName)));
    }

    void mMotionLoadExportPanel::writeCsvContent(QTextStream& out,
        const mMotionResultQueryService::LoadSeries& series,
        const QVector<int>& frameIndices, double effectiveScale) const
    {
        const QString loadCase = m_editName->text().trimmed().isEmpty()
            ? m_editName->placeholderText()
            : m_editName->text().trimmed();
        const QString endLabel = m_radioJMarker->isChecked() ? "J" : "I";
        const QString nodeIdText = m_editNodeId->text().trimmed();

        QStringList headers = {
            "LoadCase", "SourceType", "SourceName", "FrameIndex", "Time", "NodeID", "End"
        };
        if (m_chkExportForce->isChecked()) {
            if (m_chkFx->isChecked()) headers << "Fx";
            if (m_chkFy->isChecked()) headers << "Fy";
            if (m_chkFz->isChecked()) headers << "Fz";
        }
        if (m_chkExportTorque->isChecked()) {
            if (m_chkMx->isChecked()) headers << "Mx";
            if (m_chkMy->isChecked()) headers << "My";
            if (m_chkMz->isChecked()) headers << "Mz";
        }
        out << headers.join(",") << "\n";

        for (int index : frameIndices) {
            const QVector3D force = series.forces[index] * static_cast<float>(effectiveScale);
            const QVector3D torque = series.torques[index] * static_cast<float>(effectiveScale);

            QStringList values = {
                loadCase,
                m_comboSourceType->currentText(),
                m_comboSourceId->currentText(),
                QString::number(index),
                QString::number(series.times[index], 'g', 15),
                nodeIdText,
                endLabel
            };
            if (m_chkExportForce->isChecked()) {
                if (m_chkFx->isChecked()) values << QString::number(force.x(), 'g', 15);
                if (m_chkFy->isChecked()) values << QString::number(force.y(), 'g', 15);
                if (m_chkFz->isChecked()) values << QString::number(force.z(), 'g', 15);
            }
            if (m_chkExportTorque->isChecked()) {
                if (m_chkMx->isChecked()) values << QString::number(torque.x(), 'g', 15);
                if (m_chkMy->isChecked()) values << QString::number(torque.y(), 'g', 15);
                if (m_chkMz->isChecked()) values << QString::number(torque.z(), 'g', 15);
            }
            out << values.join(",") << "\n";
        }
    }

    void mMotionLoadExportPanel::writeBdfContent(QTextStream& out,
        const mMotionResultQueryService::LoadSeries& series,
        const QVector<int>& frameIndices, double effectiveScale) const
    {
        const QString loadCase = m_editName->text().trimmed().isEmpty()
            ? m_editName->placeholderText()
            : m_editName->text().trimmed();
        const int nodeId = m_editNodeId->text().trimmed().toInt();
        const int sourceId = m_comboSourceId->currentData().toInt();

        auto bdfNumber = [](double value) {
            QString text = QString::number(value, 'g', 12);
            if (text.contains(QLatin1Char('e')))
                text.replace(QLatin1Char('e'), QLatin1Char('E'));
            return text;
        };

        out << "$ MxSim Motion Load Export\n";
        out << "$ LoadCase   : " << loadCase << "\n";
        out << "$ Source     : " << m_comboSourceId->currentText()
            << " (" << m_comboSourceType->currentText() << ", ID " << sourceId << ")\n";
        out << "$ End        : " << (m_radioJMarker->isChecked() ? "J Marker" : "I Marker") << "\n";
        out << "$ Node       : " << nodeId << "\n";
        out << "$ Frames     : " << frameIndices.size() << "\n";
        out << "$ Coordinate : Global (CID = 0)\n";
        out << "$ Units      : 与求解模型单位制一致，未做换算\n";
        out << "$ Scale      : " << QString::number(m_spinScale->value(), 'g', 12) << "\n";
        out << "$ LoadSet    : SID = FrameIndex + 1，每帧一个载荷集\n";

        for (int index : frameIndices) {
            const QVector3D force = series.forces[index] * static_cast<float>(effectiveScale);
            const QVector3D torque = series.torques[index] * static_cast<float>(effectiveScale);
            const int sid = index + 1;

            out << "$ Frame " << index
                << "  Time " << QString::number(series.times[index], 'g', 12)
                << "  SID " << sid << "\n";

            if (m_chkExportForce->isChecked()) {
                out << "FORCE," << sid << "," << nodeId << ",0,1.0,"
                    << bdfNumber(m_chkFx->isChecked() ? force.x() : 0.0) << ","
                    << bdfNumber(m_chkFy->isChecked() ? force.y() : 0.0) << ","
                    << bdfNumber(m_chkFz->isChecked() ? force.z() : 0.0) << "\n";
            }
            if (m_chkExportTorque->isChecked()) {
                out << "MOMENT," << sid << "," << nodeId << ",0,1.0,"
                    << bdfNumber(m_chkMx->isChecked() ? torque.x() : 0.0) << ","
                    << bdfNumber(m_chkMy->isChecked() ? torque.y() : 0.0) << ","
                    << bdfNumber(m_chkMz->isChecked() ? torque.z() : 0.0) << "\n";
            }
        }
        out << "ENDDATA\n";
    }

    void mMotionLoadExportPanel::refreshSourceObjects()
    {
        if (!m_comboSourceId)
            return;

        m_comboSourceId->clear();
        if (!getCVM || !getCVM->getModelData())
            return;

        auto* modelData = getCVM->getModelData();
        switch (getCurrentSourceType()) {
        case MechMotionData::CurveSourceType::Joint: {
            auto& manager = modelData->getManager<MechMotionData::mMotionJointManager>(Physical_MotionJoint);
            for (int id : manager.getAllIDs()) {
                auto* data = manager.getData(id);
                if (data) m_comboSourceId->addItem(data->getName(), id);
            }
            break;
        }
        case MechMotionData::CurveSourceType::Coupling: {
            auto& manager = modelData->getManager<MechMotionData::mMotionCouplingManager>(Physical_MotionCoupling);
            for (int id : manager.getAllIDs()) {
                auto* data = manager.getData(id);
                if (data) m_comboSourceId->addItem(data->getName(), id);
            }
            break;
        }
        case MechMotionData::CurveSourceType::Drive: {
            auto& manager = modelData->getManager<MechMotionData::mMotionDriveManager>(Physical_MotionDrive);
            for (int id : manager.getAllIDs()) {
                auto* data = manager.getData(id);
                if (data) m_comboSourceId->addItem(data->getName(), id);
            }
            break;
        }
        default:
            break;
        }
    }

    void mMotionLoadExportPanel::refreshResultState()
    {
        const bool hasScenario = getCVM && getCVM->getModelData() && m_scenarioId >= 0;
        int maxFrame = 0;
        bool hasResult = false;
        if (hasScenario) {
            auto* scenario = getCVM->getModelData()->getManager<MechMotionData::mMotionScenarioManager>(
                Physical_MotionScenario).getData(m_scenarioId);
            hasResult = scenario && scenario->hasResult()
                && scenario->getResult()->getFrameCount() > 0;
            if (hasResult)
                maxFrame = qMax(0, scenario->getResult()->getFrameCount() - 1);
        }
        m_spinFrameIndex->setRange(0, maxFrame);
        // A converged static equilibrium is one accepted frame (index 0),
        // and is a valid load history for manual CSV/BDF export. The case
        // selector reports if that history contains no internal extrema.
        m_btnExport->setEnabled(hasResult);
        m_casePage.btnExport->setEnabled(hasResult);
        m_syncPage.btnExport->setEnabled(hasResult);
    }

    MechMotionData::CurveSourceType mMotionLoadExportPanel::getCurrentSourceType() const
    {
        switch (m_comboSourceType->currentIndex()) {
        case 1:
            return MechMotionData::CurveSourceType::Coupling;
        case 2:
            return MechMotionData::CurveSourceType::Drive;
        default:
            return MechMotionData::CurveSourceType::Joint;
        }
    }

    int mMotionLoadExportPanel::getMaxLoadFrameIndex(
        const mMotionResultQueryService::LoadSeries& series) const
    {
        int maxIndex = 0;
        double maxScore = -1.0;
        double maxForceMagnitude = 0.0;
        double maxTorqueMagnitude = 0.0;

        for (int i = 0; i < series.times.size(); ++i) {
            maxForceMagnitude = qMax(maxForceMagnitude, static_cast<double>(series.forces[i].length()));
            maxTorqueMagnitude = qMax(maxTorqueMagnitude, static_cast<double>(series.torques[i].length()));
        }

        for (int i = 0; i < series.times.size(); ++i) {
            double score = 0.0;
            if (m_chkExportForce->isChecked() && maxForceMagnitude > 0.0)
                score += series.forces[i].length() / maxForceMagnitude;
            if (m_chkExportTorque->isChecked() && maxTorqueMagnitude > 0.0)
                score += series.torques[i].length() / maxTorqueMagnitude;

            if (score > maxScore) {
                maxScore = score;
                maxIndex = i;
            }
        }
        return maxIndex;
    }

    // ==================== 特征工况导出 ====================

    void mMotionLoadExportPanel::refreshBodies()
    {
        auto fill = [this](CasePageCtx& ctx) {
            const QSignalBlocker blocker(ctx.comboBody);
            ctx.comboBody->clear();
            if (getCVM && getCVM->getModelData()) {
                auto& bodyMgr = getCVM->getModelData()
                    ->getManager<MechMotionData::mMotionBodyManager>(Physical_MotionBody);
                for (int id : bodyMgr.getAllIDs()) {
                    auto* body = bodyMgr.getData(id);
                    if (body)
                        ctx.comboBody->addItem(body->getName(), id);
                }
            }
            refreshJointChecks(ctx);
        };
        fill(m_casePage);
        fill(m_syncPage);
    }

    void mMotionLoadExportPanel::refreshJointChecks(CasePageCtx& ctx)
    {
        // 清空旧复选框
        QLayoutItem* item;
        while ((item = ctx.jointCheckLayout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        ctx.jointChecks.clear();

        // 清空旧几何面拾取行（切换运动体后副集合变化，拾取行随之重建；
        // 清理方式与导入面板 refreshJointRows 同款：removeAll 只移除布局项，控件显式 deleteLater）
        if (ctx.faceSection) {
            ctx.faceSection->removeAll();
            for (auto it = ctx.jointFacePicks.begin(); it != ctx.jointFacePicks.end(); ++it) {
                if (it.value() && it.value()->widget())
                    it.value()->widget()->deleteLater();
            }
            ctx.jointFacePicks.clear();
        }

        const int bodyId = ctx.comboBody->currentData().toInt();
        if (bodyId >= 0 && getCVM && getCVM->getModelData())
        {
            auto& jointMgr = getCVM->getModelData()
                ->getManager<MechMotionData::mMotionJointManager>(Physical_MotionJoint);
            for (int jid : jointMgr.getAllIDs()) {
                auto* joint = jointMgr.getData(jid);
                if (!joint)
                    continue;
                if (mMotionRefMap::get(Physical_MotionJoint, jid, Physical_MotionBody, MapRelation_MotionActionBody) != bodyId
                    && mMotionRefMap::get(Physical_MotionJoint, jid, Physical_MotionBody, MapRelation_MotionBaseBody) != bodyId)
                    continue;
                QCheckBox* chk = new QCheckBox(
                    QString("%1 [%2]").arg(joint->getName(), joint->getTypeName()),
                    ctx.jointCheckArea);
                chk->setChecked(true);
                connect(chk, &QCheckBox::toggled, this, [this](bool) { highlightSelection(); });
                ctx.jointCheckLayout->addWidget(chk);
                ctx.jointChecks.insert(jid, chk);

                // 同一副一行的几何面拾取器（免导入生成用，同导入面板控件）
                if (ctx.faceSection) {
                    auto* facePick = mGeneralPickUp::createMorePickWidget(CBRed);
                    facePick->setDefultTip(QStringLiteral("几何面（可多选）"));
                    facePick->setAllowedTypes(&MechCommon::pickGeoPointEdgeFaceSolidNodePart);
                    ctx.faceSection->addRow(new QLabel(joint->getName()), facePick->widget());
                    ctx.jointFacePicks.insert(jid, facePick);
                }
            }
        }
        ctx.jointCheckLayout->addStretch();

        // 运动副数量随所选运动体增减，折叠区内容高度需要重算，否则截断或留白
        if (ctx.sourceSection)
            ctx.sourceSection->refreshLayout();
        if (ctx.faceSection)
            ctx.faceSection->refreshLayout();

        // 体/勾选变化后联动视口高亮（体部件 + 勾选副符号）
        highlightSelection();
    }

    bool mMotionLoadExportPanel::collectCaseJoints(const CasePageCtx& ctx, QVector<JointSel>& out, QString& error)
    {
        out.clear();
        for (auto it = ctx.jointChecks.begin(); it != ctx.jointChecks.end(); ++it) {
            if (!it.value()->isChecked())
                continue;
            const int jid = it.key();
            auto& jointMgr = getCVM->getModelData()
                ->getManager<MechMotionData::mMotionJointManager>(Physical_MotionJoint);
            auto* joint = jointMgr.getData(jid);
            if (!joint)
                continue;
            JointSel sel;
            sel.jointId = jid;
            sel.jointName = joint->getName();
            sel.jointType = joint->getTypeName();
            sel.actionBodyId = mMotionRefMap::get(Physical_MotionJoint, jid, Physical_MotionBody, MapRelation_MotionActionBody);
            sel.baseBodyId = mMotionRefMap::get(Physical_MotionJoint, jid, Physical_MotionBody, MapRelation_MotionBaseBody);
            out.push_back(sel);
        }
        if (out.isEmpty()) {
            error = QStringLiteral("请至少勾选一个运动副。");
            return false;
        }
        return true;
    }

    void mMotionLoadExportPanel::onExportCaseClicked()
    {
        if (!getCVM || !getCVM->getModelData() || m_scenarioId < 0)
            return;
        const bool synchronized = (m_comboExportMode->currentIndex() == 2);
        CasePageCtx* ctx = currentCasePage();
        if (!ctx)
            return;
        auto* scenario = getCVM->getModelData()->getManager<MechMotionData::mMotionScenarioManager>(
            Physical_MotionScenario).getData(m_scenarioId);
        if (!scenario || !scenario->hasResult()) {
            QMessageBox::warning(this, tr("提示"), tr("当前方案没有求解结果，无法导出工况。"));
            return;
        }

        const bool autoGenerate = ctx->chkAutoGenerate && ctx->chkAutoGenerate->isChecked();
        // JSON 是否落盘由用户选择：直接生成时可通过复选框关掉 JSON 导出（连文件对话框都不弹）
        const bool writeJson = !autoGenerate || !ctx->chkExportJson || ctx->chkExportJson->isChecked();

        QString fileName;
        if (writeJson) {
            QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_recentDir;
            if (defaultDir.isEmpty() || !QFileInfo(defaultDir).exists())
                defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();

            const QString defaultName = QString("%1/%2%3_loadcases.json")
                .arg(defaultDir, scenario->getName(), synchronized ? "_sync" : "");
            fileName = QFileDialog::getSaveFileName(
                this, synchronized ? tr("导出连杆同步工况") : tr("导出特征工况"),
                defaultName, tr("LoadCase JSON(*.json)"));
            if (fileName.isEmpty())
                return;
            if (!fileName.contains(QLatin1Char('.')))
                fileName += ".json";
        }

        QString error;
        int nGenCases = 0, nGenLoads = 0, nGenRbe3 = 0;
        if (!exportLoadCaseJson(fileName, synchronized, autoGenerate, writeJson, error,
                &nGenCases, &nGenLoads, &nGenRbe3)) {
            QMessageBox::warning(this, tr("提示"), error);
            return;
        }

        // 生成统计打到信息栏（与导入面板同通道同文案），不弹窗打断
        if (autoGenerate) {
            mGlobalSignals::getInstance()->outputMessageSig(
                QStringLiteral("[多体载荷] 已生成 %1 个静力工况、%2 个载荷、%3 个 RBE3 耦合。")
                    .arg(nGenCases).arg(nGenLoads).arg(nGenRbe3));
        }

        if (!writeJson)
            return;   // 纯生成：无文件路径可报，结果走信息栏

        MAppConfiguration::mAppConfiguration::getInstance()->_recentDir =
            QFileInfo(fileName).absolutePath();

        // 上报平台（平台模式下把工况 JSON 挂到 results 通道）
        reportToPlatform(QFileInfo(fileName).absoluteFilePath());

        QMessageBox::information(this, tr("提示"),
            tr("%1导出完成：\n%2")
                .arg(synchronized ? tr("连杆同步工况") : tr("特征工况"))
                .arg(QDir::toNativeSeparators(fileName)));
    }

    bool mMotionLoadExportPanel::exportLoadCaseJson(const QString& fileName, bool synchronized,
        bool autoGenerate, bool writeJson, QString& error,
        int* genCases, int* genLoads, int* genRbe3)
    {
#ifdef MX_MOTION_OCC_COMPAT
        if (!getCVM || !getCVM->buildMotionLoadCases) {
            error = QStringLiteral("当前方案的载荷工况服务未连接。");
            return false;
        }
        // No partial JSON or fictitious static cases when the separate
        // structural-model backend has not been installed in this host.
        if (autoGenerate && !getCVM->generateMotionStaticCases) {
            error = QStringLiteral("隐式静力模型生成尚未连接：此流程需要结构工况、载荷、RBE3、集合及几何面到网格的真实数据接口。当前没有创建任何静力工况或写入文件。");
            return false;
        }
        CasePageCtx* ctx = synchronized ? &m_syncPage : &m_casePage;
        QVector<JointSel> selected;
        if (!collectCaseJoints(*ctx,selected,error)) return false;
        MotionOcc::NativeLoadCaseOptions options;
        options.analysisBodyId = ctx->comboBody->currentData().toInt();
        options.peaksPerSignal = ctx->spinPeaks->value();
        options.mergeTolerance = ctx->spinMergeTol->value();
        options.synchronized = synchronized;
        QMap<int,QVector<int>> bindings;
        for (const auto& joint:selected) {
            options.jointIds.push_back(joint.jointId);
            if (!autoGenerate) continue;
            QVector<int> faces;
            if (auto* picker=ctx->jointFacePicks.value(joint.jointId,nullptr)) {
                const auto data=picker->getAllData();
                for (const auto& entry:data.pickedData) if (entry.first==MxFunctions::PickDataType::GeoFace)
                    for (const auto& face:entry.second) if (face.result_id>0 && !faces.contains(face.result_id))
                        faces.push_back(face.result_id);
            }
            if (faces.isEmpty()) {
                error=QStringLiteral("运动副「%1」还没有拾取几何面。").arg(joint.jointName);
                return false;
            }
            bindings.insert(joint.jointId,faces);
        }
        QJsonObject data;
        if (!getCVM->buildMotionLoadCases(m_scenarioId,options,data,error)) return false;
        if (autoGenerate && !getCVM->generateMotionStaticCases(data,bindings,error,genCases,genLoads,genRbe3)) return false;
        if (writeJson) {
            QSaveFile file(fileName);
            if (!file.open(QIODevice::WriteOnly|QIODevice::Text)
                || file.write(QJsonDocument(data).toJson(QJsonDocument::Indented))<0 || !file.commit()) {
                error=QStringLiteral("无法写入工况文件：%1").arg(file.errorString());
                return false;
            }
        }
        return true;
#else
        CasePageCtx* ctx = synchronized ? &m_syncPage : &m_casePage;
        auto* scenario = getCVM->getModelData()->getManager<MechMotionData::mMotionScenarioManager>(
            Physical_MotionScenario).getData(m_scenarioId);
        MechMotionData::mMotionResultData* result = scenario->getResult();

        // 1) 收集选中的运动副
        QVector<JointSel> sels;
        if (!collectCaseJoints(*ctx, sels, error))
            return false;

        // 2) 组装筛选器输入（含全时程反力）
        QVector<mMotionLoadCaseSelector::JointInput> inputs;
        for (const JointSel& s : sels) {
            mMotionLoadCaseSelector::JointInput in;
            in.jointId = s.jointId;
            in.jointName = s.jointName;
            in.jointType = s.jointType;
            in.actionBodyId = s.actionBodyId;
            in.baseBodyId = s.baseBodyId;
            in.series = queryJointSeries(result, s.jointId);
            if (!in.series.isValid())
                continue;
            // 作用点用运动副初始原点（固定模型/初始姿态下的几何位置），
            // 与隐式软件的静止几何一致。
            {
                auto& jointMgr2 = getCVM->getModelData()
                    ->getManager<MechMotionData::mMotionJointManager>(Physical_MotionJoint);
                auto* jointObj = jointMgr2.getData(s.jointId);
                auto* basic = dynamic_cast<MechMotionData::mMotionJointBasic*>(jointObj);
                if (basic)
                    in.actionPoint = basic->getOrigin();
            }
            inputs.push_back(in);
        }
        if (inputs.isEmpty()) {
            error = QStringLiteral("选中的运动副没有可用的反力结果数据。");
            return false;
        }

        // 2.5) 免导入生成：前置收集每副几何面绑定（任一勾选副面为空则整体失败，
        //      不写文件不留半成品；读法与导入面板一致——GeoFace 且 result_id>0 去重）
        QMap<int, QVector<int>> faceBindings;
        if (autoGenerate) {
            for (const auto& in : inputs) {
                QVector<int> faceIds;
                if (auto* pick = ctx->jointFacePicks.value(in.jointId, nullptr)) {
                    const auto data = pick->getAllData();
                    for (const auto& pair : data.pickedData) {
                        if (pair.first != MxFunctions::PickDataType::GeoFace) continue;
                        for (const auto& d : pair.second) {
                            if (d.result_id > 0 && !faceIds.contains(d.result_id))
                                faceIds.push_back(d.result_id);
                        }
                    }
                }
                if (faceIds.isEmpty()) {
                    error = QStringLiteral("运动副「%1」还没有拾取几何面。").arg(in.jointName);
                    return false;
                }
                faceBindings.insert(in.jointId, faceIds);
            }
        }

        // 3) 筛选工况
        mMotionLoadCaseSelector::Params params;
        params.peaksPerSignal = ctx->spinPeaks->value();
        params.mergeTolerance = ctx->spinMergeTol->value();
        params.synchronizedAllJoints = synchronized;
        QVector<mMotionLoadCaseSelector::LoadCase> cases;
        if (!mMotionLoadCaseSelector::select(inputs, params, cases, error))
            return false;

        // 4) 组装 JSON
        QJsonObject infoSrc;
        infoSrc["scenarioId"] = m_scenarioId;
        infoSrc["scenarioName"] = scenario->getName();
        infoSrc["analysisBodyId"] = ctx->comboBody->currentData().toInt();
        infoSrc["analysisBodyName"] = ctx->comboBody->currentText();
        infoSrc["peaksPerSignal"] = params.peaksPerSignal;
        infoSrc["mergeTolerance"] = params.mergeTolerance;
        infoSrc["exportedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QJsonObject info;
        info["format"] = "MxMotionLoadCases";
        info["version"] = 1;
        info["method"] = synchronized ? "synchronizedLinkLoads" : "extremeJointsOnly";
        info["description"] = synchronized
            ? QStringLiteral(
                "机械臂连杆同步载荷导出。按时刻组织静力工况：cases[] 每个元素是一个特征时刻，"
                "包含分析连杆全部关联运动副在该时刻的同步载荷（供单连杆静力分析）。")
            : QStringLiteral(
                "多体动力学特征工况导出。按时刻组织静力工况：cases[] 每个元素是一个特征时刻，"
                "含该时刻所有取到极值的运动副载荷。");
        info["units"] = QStringLiteral(
            "与求解模型单位制一致（力=N, 力矩=N*m），未做换算");
        info["source"] = infoSrc;
        info["coordinateSystem"] = synchronized
            ? QStringLiteral(
                "bodyInitial：力/力矩统一用分析体（连杆）在该时刻相对初始姿态的旋转 R(t) "
                "反变换回初始姿态坐标系；分析体位于运动副 base(J) 端时该副载荷已整体取反"
                "（action(I) 端保持原值），隐式软件在固定模型上按全局系直接施加即可。")
            : QStringLiteral(
                "bodyInitial：力/力矩已用分析体在该时刻相对初始姿态的旋转 R(t) 反变换回"
                "初始姿态坐标系，隐式软件在固定模型上按全局系直接施加即可。");

        // 运动体 id -> geoEntityIds / isFixed（供 joints 与 bodies 表查询）
        auto& bodyMgr = getCVM->getModelData()
            ->getManager<MechMotionData::mMotionBodyManager>(Physical_MotionBody);
        auto bodyGeoIds = [&](int bodyId) {
            QJsonArray arr;
            if (auto* body = bodyMgr.getData(bodyId))
                for (int g : mMotionRefMap::getSet(Physical_MotionBody, body->getId(), Physical_Part))
                    arr.push_back(g);
            return arr;
        };
        auto bodyIsFixed = [&](int bodyId) {
            if (auto* body = bodyMgr.getData(bodyId))
                return body->getIsFixed();
            return false;
        };

        QJsonArray jointsArr;
        for (const auto& in : inputs) {
            QJsonObject j;
            j["id"] = in.jointId;
            j["name"] = in.jointName;
            j["type"] = in.jointType;
            j["actionBodyId"] = in.actionBodyId;
            j["baseBodyId"] = in.baseBodyId;
            j["actionBodyPartIds"] = bodyGeoIds(in.actionBodyId);
            j["baseBodyPartIds"] = bodyGeoIds(in.baseBodyId);
            j["actionPoint"] = QJsonArray{
                in.actionPoint.x(), in.actionPoint.y(), in.actionPoint.z() };
            jointsArr.push_back(j);
        }

        // bodies[]：本次导出涉及的运动体（选中副的 action/base 体 + 分析体），
        // 隐式软件据此建立 运动体 id -> 几何部件 id 的映射。
        const int analysisBodyId = ctx->comboBody->currentData().toInt();
        QSet<int> bodyIds;
        bodyIds.insert(analysisBodyId);
        for (const auto& in : inputs) {
            bodyIds.insert(in.actionBodyId);
            bodyIds.insert(in.baseBodyId);
        }
        QJsonArray bodiesArr;
        for (int bid : bodyIds) {
            if (bid < 0) continue;
            auto* body = bodyMgr.getData(bid);
            if (!body) continue;
            QJsonObject b;
            b["id"] = bid;
            b["name"] = body->getName();
            b["partIds"] = bodyGeoIds(bid);
            b["isFixed"] = bodyIsFixed(bid);
            bodiesArr.push_back(b);
        }

        // jointId -> action/base 体（坐标旋转与端别符号的依据）与名称（JSON loads 用）
        QMap<int, int> jointActionBody;   // jointId -> actionBodyId
        QMap<int, int> jointBaseBody;     // jointId -> baseBodyId
        QMap<int, QString> jointNames;    // jointId -> name
        for (const auto& in : inputs) {
            jointActionBody.insert(in.jointId, in.actionBodyId);
            jointBaseBody.insert(in.jointId, in.baseBodyId);
            jointNames.insert(in.jointId, in.jointName);
        }

        // 先统一计算最终载荷矢量（R(t)^T 反变换 + 同步端别符号）——
        // JSON 写出与免导入生成共用同一份结果，保证两条路径数值完全一致
        QVector<mMotionLoadCaseGenerator::GenCase> finalCases;
        for (const auto& lc : cases) {
            mMotionLoadCaseGenerator::GenCase gc;
            gc.name = lc.name;
            gc.timeSeconds = lc.timeSeconds;

            // 连杆同步模式：整工况统一用分析体（连杆）自己的 R(t)^T，一次算好复用
            QQuaternion analysisInv;   // 默认单位四元数 = 不旋转
            if (synchronized && analysisBodyId >= 0) {
                QMatrix4x4 analysisMat;
                auto* analysisBody = bodyMgr.getData(analysisBodyId);
                if (analysisBody &&
                    mMotionResultQueryService::getBodyMatrix(result, analysisBodyId,
                        analysisBody->getCenterOfMass(), lc.timeSeconds, analysisMat))
                    analysisInv = QQuaternion::fromRotationMatrix(analysisMat.normalMatrix()).conjugated();
            }

            for (const auto& load : lc.loads) {
                // 多体里体在动，反力是 t 时刻全局系值；隐式是固定模型（初始姿态），
                // 把力/力矩反变换回初始姿态系：
                //   常规模式——按该副 action 体的 R(t)^T（历史行为保持不变）；
                //   同步模式——统一按分析体（连杆）的 R(t)^T。
                QQuaternion inv;   // 默认单位四元数 = 不旋转
                if (synchronized) {
                    inv = analysisInv;
                }
                else {
                    const int bodyId = jointActionBody.value(load.jointId, -1);
                    QMatrix4x4 bodyMat;
                    auto* loadBody = bodyId >= 0 ? bodyMgr.getData(bodyId) : nullptr;
                    if (loadBody &&
                        mMotionResultQueryService::getBodyMatrix(result, bodyId,
                            loadBody->getCenterOfMass(), lc.timeSeconds, bodyMat)) {
                        inv = QQuaternion::fromRotationMatrix(bodyMat.normalMatrix()).conjugated();
                    }
                }
                QVector3D f = inv.rotatedVector(load.force);
                QVector3D tq = inv.rotatedVector(load.torque);

                // 同步模式端别符号：分析体为该副 action(I) 端保持原值，
                // 为 base(J) 端时力与力矩整体取反（沿用手动提取 J Marker 端 ×-1 约定）
                if (synchronized) {
                    double sign = 1.0;
                    if (jointActionBody.value(load.jointId, -1) == analysisBodyId)
                        sign = 1.0;
                    else if (jointBaseBody.value(load.jointId, -2) == analysisBodyId)
                        sign = -1.0;
                    f *= sign;
                    tq *= sign;
                }

                mMotionLoadCaseGenerator::GenLoad gl;
                gl.jointId = load.jointId;
                gl.force = f;
                gl.torque = tq;
                gc.loads.push_back(gl);
            }
            finalCases.push_back(gc);
        }

        QJsonArray casesArr;
        for (const auto& gc : finalCases) {
            QJsonObject c;
            c["name"] = gc.name;
            // 数值写出：导入侧 toDouble() 读旧下划线字符串（"0_5"）恒得 0
            c["timeSeconds"] = gc.timeSeconds;
            QJsonArray loadsArr;
            for (const auto& load : gc.loads) {
                QJsonObject comp;
                comp["FX"] = load.force.x();
                comp["FY"] = load.force.y();
                comp["FZ"] = load.force.z();
                comp["FM"] = load.force.length();
                comp["TX"] = load.torque.x();
                comp["TY"] = load.torque.y();
                comp["TZ"] = load.torque.z();
                comp["TM"] = load.torque.length();
                QJsonObject lo;
                lo["jointId"] = load.jointId;
                lo["jointName"] = jointNames.value(load.jointId);
                lo["components"] = comp;
                loadsArr.push_back(lo);
            }
            c["loads"] = loadsArr;
            casesArr.push_back(c);
        }

        QJsonObject root;
        root["info"] = info;
        root["bodies"] = bodiesArr;
        root["joints"] = jointsArr;
        root["cases"] = casesArr;

        // 5) 写文件（纯生成模式 writeJson=false 时跳过，不落盘）
        if (writeJson) {
            QFile file(fileName);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                error = QStringLiteral("无法写入工况文件：%1").arg(fileName);
                return false;
            }
            file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
            file.close();
        }

        // 6) 免导入生成：直接在当前模型生成隐式静力工况（载荷=finalCases 最终值，
        //    与 JSON 完全一致；面绑定已在前置校验收集）
        if (autoGenerate) {
            QVector<mMotionLoadCaseGenerator::GenJoint> genJoints;
            for (const auto& in : inputs) {
                mMotionLoadCaseGenerator::GenJoint gj;
                gj.jointId = in.jointId;
                gj.name = in.jointName;
                gj.actionPoint = in.actionPoint;
                genJoints.push_back(gj);
            }
            if (!mMotionLoadCaseGenerator::generate(genJoints, finalCases, faceBindings, error,
                    genCases, genLoads, genRbe3))
                return false;   // JSON（如写出）已落盘，生成失败原因在 error
        }
        return true;
#endif
    }

    void mMotionLoadExportPanel::reportToPlatform(const QString& filePath)
    {
        // 仅平台模式下桥接才真正发消息；独立运行时桥接内部 no-op。
        emit MechCommon::mMechGlobalSignals::getInstance()
            ->mpcommLoadCaseExported(filePath);
    }
}
