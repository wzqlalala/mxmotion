
#include "mMotionAnimationPanel.h"
#include "mMotionRefMap.h"
#include "mMotionResultQueryService.h"
#ifndef MX_MOTION_OCC_COMPAT
#include "mMotionSolverThread.h"
#endif
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"

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
#include "mMotionResultData.h"
#ifndef MX_MOTION_OCC_COMPAT
#include "mxr/view/CCSMPreRender.h"
#include "mxr/view/mFlexBodyRender.h"
#endif
#include <QVector4D>
#include <cmath>
#include <fstream>
#include "mMotionJointBase.h"
#include "mMotionJointBasic.h"
#include "mMotionJointManager.h"
#include "mMotionDriveManager.h"
#include "mMotionDriveBase.h"
#include "mMotionJointDrive.h"
#include "mMotionScenario.h"
#include "mMotionScenarioManager.h"
#include "mMotionMarker.h"
#include "mMotionResultData.h"
#include "mModelData.h"
#include "mMeshData.h"
#include "mMotionSpringBase.h"
#include "mMotionBodyPointSpring.h"
#include "mMotionSpringManager.h"
#ifndef MX_MOTION_OCC_COMPAT
#include "mInfoWindow.h"
#endif
#include "mMotionSpringPanel.h"
#include "mMotionCurvePanel.h"
#include "mCurveWindow.h"
#include "mPlotView.h"

//Geo
#ifndef MX_MOTION_OCC_COMPAT
#include "mGeometricFeatureAttribute.h"
#endif

//Qt
#include <QComboBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QProgressDialog>
#include <QDateTime>
#include <QQuaternion>
#include <QVector3D>

//STL
#include <vector>
#include <map>
using namespace MechPreData;
using namespace MechMotionData;
using namespace MxFunctions;
using namespace mxr;

namespace
{
    // 动画当前时间在曲线上的标记线名
    const char* kCurveMarkerName = "动画当前帧";

    bool canPlayResult(const MechMotionData::mMotionResultData* result)
    {
        if (!result || result->getFrameCount() < 2)
            return false;
        const double first = result->m_frames.front().m_time;
        const double last = result->m_frames.back().m_time;
        return std::isfinite(first) && std::isfinite(last) && last > first;
    }
}

namespace MechMainWindow
{

    mMotionAnimationPanel::mMotionAnimationPanel(QWidget* parent) : mPanelBase(parent)
    {
        // 创建驱动动画的定时器
        m_playTimer = new QTimer(this);

        m_motionResultData = nullptr;

        //获取当前视口的数据
        m_mainLayout = new QVBoxLayout(this);

        // 1. 播放控制
        FoldSection* foldControl = new FoldSection("播放控制", this);
        m_btnPlay = new QPushButton("播放");
        m_btnPause = new QPushButton("暂停");
        m_btnStop = new QPushButton("停止");
        m_btnPlay->setEnabled(false); // 没有时间历程时不能启动播放
        m_btnPause->setEnabled(false); // 初始状态不能暂停
        foldControl->addRow(m_btnPlay, m_btnPause, m_btnStop, QString());
        m_mainLayout->addWidget(foldControl);

        // 2. 时间轴
        FoldSection* foldTimeline = new FoldSection("时间轴", this);
        m_sliderTimeline = new QSlider(Qt::Horizontal);
        m_sliderTimeline->setMinimum(0);
        m_sliderTimeline->setValue(0);
        m_sliderTimeline->setEnabled(false); // 没数据时置灰
        foldTimeline->addRow(m_sliderTimeline);
        m_mainLayout->addWidget(foldTimeline);

        // 3. 播放设置
        FoldSection* foldSettings = new FoldSection("播放设置", this);
        m_lblTimeInfo = new QLabel("时间: 0.000 s / 0.000 s");
        foldSettings->addRow(m_lblTimeInfo);

        m_cmbPlaySpeed = new QComboBox();
        m_cmbPlaySpeed->addItems({ "0.5x", "1.0x", "2.0x", "5.0x", "50.0x" });
        m_cmbPlaySpeed->setCurrentIndex(1); // 默认 1.0x
        m_cmbPlaySpeed->setFixedWidth(70);
        foldSettings->addRow(new QLabel("播放速度:"), m_cmbPlaySpeed);

        m_chkLoop = new QCheckBox("循环播放");
        m_chkLoop->setChecked(false);
        foldSettings->addRow(m_chkLoop);

        //【刚柔耦合】变形云图色标口径：默认不勾=全程最大变形定标（颜色随变形
        // 幅值演化，跨帧可比）；勾选=逐帧自适应（每帧归一化，小变形帧也满彩，
        // 但单模态主导的响应色分布会恒定——只有图例数字在变）
        m_chkFlexAdaptiveScale = new QCheckBox(QString::fromUtf8("云图色标逐帧自适应"));
        m_chkFlexAdaptiveScale->setObjectName(QStringLiteral("originalMotionFlexAdaptiveScale"));
        m_chkFlexAdaptiveScale->setChecked(false);
#ifdef MX_MOTION_OCC_COMPAT
        m_chkFlexAdaptiveScale->setToolTip(tr(
            "勾选后按当前帧节点的最小值和最大值着色，数值见右侧色标。\n"
            "如果只有变形幅值变化，颜色分布仍可能相近。取消勾选可用同一色标比较各帧。"));
#else
        m_chkFlexAdaptiveScale->setToolTip(QString::fromUtf8(
            "不勾（默认）：色标上限=全程最大弹性变形，颜色深浅随变形幅值演化，帧间可比\n"
            "勾选：色标上限=每帧实际最大变形，任何帧都有满色阶；单模态主导时分布恒定"));
#endif
        foldSettings->addRow(m_chkFlexAdaptiveScale);
        // 切换口径时强制下一帧重刷图例（范围从全程 max 切到帧 max 或反之）
        connect(m_chkFlexAdaptiveScale, &QCheckBox::toggled, this, [this]() {
            m_flexLegendShownMax = -1.0;
#ifdef MX_MOTION_OCC_COMPAT
            renderCurrentFrame();
#endif
        });

        m_cmbFlexContourRef = new QComboBox();
        m_cmbFlexContourRef->setObjectName(QStringLiteral("originalMotionFlexContour"));
#ifdef MX_MOTION_OCC_COMPAT
        m_cmbFlexContourRef->addItem(tr("变形参照: 整体位移"));
        m_cmbFlexContourRef->addItem(tr("变形参照: 弹性变形"));
        m_cmbFlexContourRef->setToolTip(tr(
            "整体位移：节点相对初始位置的位移，包含刚体平移和转动。\n"
            "弹性变形：去除刚体运动后，节点自身的变形。"));
#else
        m_cmbFlexContourRef->addItem(QString::fromUtf8("变形参照: 整体运动"));
        m_cmbFlexContourRef->addItem(QString::fromUtf8("变形参照: 弹性变形"));
        m_cmbFlexContourRef->setToolTip(QString::fromUtf8(
            "整体运动（默认，UG 口径）：扣质量加权刚体运动后的偏差，两端偏大中间偏小的对称分布\n"
            "弹性变形：相对接口焊接系的挠度，根部为 0、自由端最大"));
#endif
        foldSettings->addRow(m_cmbFlexContourRef);
        // 切换参照系：色标重定 + 图例标题随口径
        connect(m_cmbFlexContourRef, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
            m_flexLegendShownMax = -1.0;
#ifdef MX_MOTION_OCC_COMPAT
            renderCurrentFrame();
#else
            if (getCVM && getCVM->getPreRenderWidget())
                if (auto* ct = getCVM->getPreRenderWidget()->getColorTableRender())
                    ct->SetHeader(0, (m_cmbFlexContourRef->currentIndex() == 0
                        ? QString::fromUtf8("变形偏差 mm")
                        : QString::fromUtf8("弹性变形 mm")).toStdString());
#endif
        });

        m_mainLayout->addWidget(foldSettings);

        // 3.5 曲线联动（曲线面板负责配置画哪些曲线，这里负责动画与曲线的联动）
        FoldSection* foldCurveLink = new FoldSection("曲线联动", this);

        m_chkFollowCurveMark = new QCheckBox("曲线上跟随动画帧标记");
        m_chkFollowCurveMark->setChecked(true);
        foldCurveLink->addRow(m_chkFollowCurveMark);

        m_chkClickJump = new QCheckBox("点击曲线跳转动画帧");
        m_chkClickJump->setChecked(true);
        foldCurveLink->addRow(m_chkClickJump);

        m_mainLayout->addWidget(foldCurveLink);

        connect(m_chkFollowCurveMark, &QCheckBox::toggled, this, [this](bool on) {
            if (!on)
                removeCurveMarker();
        });

        // 4. Marker 追踪
        m_foldMarkerTrace = new FoldSection("Marker 追踪", this);
        m_btnClearMarkerTraces = new QPushButton("清除追踪");
        m_foldMarkerTrace->addRow(m_btnClearMarkerTraces);
        m_mainLayout->addWidget(m_foldMarkerTrace);

        connect(m_btnClearMarkerTraces, &QPushButton::clicked, this, [this]() {
            if (!getCVM || !getCVM->getModelData())
                return;

            auto scenario = getCVM->getModelData()->getManager<MechMotionData::mMotionScenarioManager>(Physical_MotionScenario).getData(m_id);
            if (!scenario)
                return;

            scenario->clearTrackedMarkers();
            refreshMarkerTraceList();
            rebuildMarkerTraces();
        });

        m_mainLayout->addStretch();

        connect(m_btnPlay, &QPushButton::clicked, this, &mMotionAnimationPanel::onPlayClicked);
        connect(m_btnPause, &QPushButton::clicked, this, &mMotionAnimationPanel::onPauseClicked);
        connect(m_btnStop, &QPushButton::clicked, this, &mMotionAnimationPanel::onStopClicked);

        connect(m_sliderTimeline, &QSlider::valueChanged, this, &mMotionAnimationPanel::onSliderMoved);

        connect(m_playTimer, &QTimer::timeout, this, &mMotionAnimationPanel::onTimerTick);

        // 速度切换
        connect(m_cmbPlaySpeed, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (m_isPlaying) {
                onPlayClicked(); // 重新计算定时器间隔并启动
            }
            });
    }

    mMotionAnimationPanel::~mMotionAnimationPanel()
    {
        if (m_mainLayout != nullptr)
        {
            delete m_mainLayout;
            m_mainLayout = nullptr;
        }

    }

    void mMotionAnimationPanel::editAnimationPanel(int id)
    {
        setEditing(true);
        m_id = id;
        // 1. 从管理器中获取该方案的 Result 数据
        auto scenario = getCVM->getModelData()->getManager<MechMotionData::mMotionScenarioManager>(Physical_MotionScenario).getData(m_id);
        if (scenario && scenario->hasResult()) {
            m_motionResultData = scenario->getResult();
        }
        else {
            m_motionResultData = nullptr;
        }

        // 重置播放状态相关字段，确保下次进入从第 0 帧暂停态开始
        m_currentFrameIndex = 0;
        m_currentAnimTime = 0.0;
        m_isPlaying = false;
        const bool playable = canPlayResult(m_motionResultData);
        m_btnPlay->setEnabled(playable);
        m_btnPause->setEnabled(false);
        m_cmbPlaySpeed->setEnabled(playable);
        m_chkLoop->setEnabled(playable);
        if (m_playTimer && m_playTimer->isActive()) {
            m_playTimer->stop();
        }
        m_lblTimeInfo->setText("时间: 0.000 s / 0.000 s");

        // 【模拟数据加载成功】
        if (m_motionResultData && m_motionResultData->getFrameCount() > 0) {
            // 与拖动进度条到索引 0、点击停止保持一致：使用第一帧的实际时间
            m_currentAnimTime = m_motionResultData->m_frames.front().m_time;
            m_sliderTimeline->setEnabled(playable);
            m_sliderTimeline->setMaximum(m_motionResultData->getFrameCount() - 1);
            m_sliderTimeline->setValue(0);

            refreshMarkerTraceList();
            rebuildMarkerTraces();
            renderCurrentFrame(); // 先把第0帧的画面刷出来静止显示

            // 打开动画即按方案曲线配置刷新曲线窗显示，并接管曲线联动
            mMotionCurvePanel::displayScenarioCurves(m_id);
            connectCurvePlotSignal();
            updateCurveMarker(m_currentAnimTime);
        }
        else {
            m_sliderTimeline->setEnabled(false);
            m_sliderTimeline->setValue(0);
            m_sliderTimeline->setMaximum(0);
            refreshMarkerTraceList();
            clearMarkerTraces();

            disconnectCurvePlotSignal();
            removeCurveMarker();
        }
    }

    //方案（及其结果对象）即将被删除：置空结果指针防悬垂（方案删除连带 delete 结果）
    void mMotionAnimationPanel::onScenarioDataAboutToBeDestroyed(int id)
    {
        Q_UNUSED(id);  //编辑会话与方案一一对应，方案删除即整体失效
        m_motionResultData = nullptr;
    }

    void mMotionAnimationPanel::uneditAnimationPanel()
    {
        if (!isEditing())
            return;
        setEditing(false);
        // 停止播放并回到第 0 帧
        onStopClicked();

        // 退出面板时必须把所有运动体渲染矩阵重置为单位矩阵，恢复模型初始位姿
        resetBodyTransformsToIdentity();
#ifndef MX_MOTION_OCC_COMPAT
        //【刚柔耦合】恢复被柔体替换部件的显隐原状，收掉变形云图图例与柔体网格
        if (getCVM && getCVM->getPreRenderWidget()) {
            if (auto* flexRend = getCVM->getPreRenderWidget()->getPreRender()->getFlexBodyRender())
                flexRend->clear();
            restoreFlexReplacedParts();
            hideFlexDeformLegend();
            m_flexBuiltForResult = nullptr;
            m_flexMaxElastic.clear();
            m_flexFreeModes.clear();
            m_flexNodeWeights.clear();
                m_flexFreeModes.clear();
            m_flexLegendShownMax = -1.0;
            getCVM->getPreRenderWidget()->updatePaint();
        }
#endif
        clearMarkerTraces();

        m_id = -1;
        m_motionResultData = nullptr;
        m_sliderTimeline->setEnabled(false);

        // 退出动画即解除曲线联动
        disconnectCurvePlotSignal();
        removeCurveMarker();
    }

    void mMotionAnimationPanel::connectSig()
    {
        disconnectSig();
        if (!getCVM || !getCVM->getPreRenderWidget())
        {
            return;
        }

        conn_geo = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mMotionAnimationPanel::slot_finishedPick);
    }

    void mMotionAnimationPanel::disconnectSig()
    {
        if (conn_geo)
            disconnect(conn_geo);
    }

    void mMotionAnimationPanel::slot_finishedPick()
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

    void mMotionAnimationPanel::slot_subWindowActivated(QMdiSubWindow* subWindow)
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

    // ==================== 播放控制逻辑 ====================

    void mMotionAnimationPanel::onPlayClicked()
    {
        if (!canPlayResult(m_motionResultData)) return;

        double totalTime = m_motionResultData->getTotalTime();
        if (m_currentAnimTime >= totalTime) {
            // 与停止/进入面板保持一致：从第一帧实际时间开始
            m_currentAnimTime = m_motionResultData->m_frames.front().m_time;
        }

        m_isPlaying = true;
        m_btnPlay->setEnabled(false);
        m_btnPause->setEnabled(true);

        // 固定约60fps刷新，实际推进量在onTimerTick中用dt*speedMult计算
        m_lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
        m_playTimer->start(16);
    }

    void mMotionAnimationPanel::onPauseClicked()
    {
        m_isPlaying = false;
        m_playTimer->stop();
        m_btnPlay->setEnabled(canPlayResult(m_motionResultData));
        m_btnPause->setEnabled(false);
    }


    void mMotionAnimationPanel::onStopClicked()
    {
        onPauseClicked();

        // 点击停止按钮回到第一帧（与拖动进度条到索引 0 保持一致：使用第一帧实际时间）
        m_currentFrameIndex = 0;
        if (m_motionResultData && !m_motionResultData->m_frames.empty()) {
            m_currentAnimTime = m_motionResultData->m_frames.front().m_time;
        }
        else {
            m_currentAnimTime = 0.0;
        }
        const QSignalBlocker blocker(m_sliderTimeline);
        m_sliderTimeline->setValue(0);
        renderCurrentFrame();
        // 停止后标记线与时间轴一起回到第一帧
        updateCurveMarker(m_currentAnimTime);
    }

    void mMotionAnimationPanel::onTimerTick()
    {
        if (!canPlayResult(m_motionResultData)) {
            onPauseClicked();
            return;
        }

        // 获取当前速度倍率
        double speedMult = 1.0;
        if (m_cmbPlaySpeed->currentIndex() == 0) speedMult = 0.5;
        else if (m_cmbPlaySpeed->currentIndex() == 1) speedMult = 1.0;
        else if (m_cmbPlaySpeed->currentIndex() == 2) speedMult = 2.0;
        else if (m_cmbPlaySpeed->currentIndex() == 3) speedMult = 5.0;
        else if (m_cmbPlaySpeed->currentIndex() == 4) speedMult = 50.0;

        // 计算真实世界流逝时间
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        double dt = (currentTime - m_lastUpdateTime) / 1000.0;
        m_lastUpdateTime = currentTime;

        // 推进动画时间
        m_currentAnimTime += dt * speedMult;

        double totalTime = m_motionResultData->getTotalTime();
        if (m_currentAnimTime >= totalTime) {
            if (m_chkLoop && m_chkLoop->isChecked()) {
                // 循环播放：回到第一帧实际时间，继续播放
                m_currentAnimTime = m_motionResultData->m_frames.front().m_time;
                m_currentFrameIndex = 0;
                m_lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
                const QSignalBlocker blocker(m_sliderTimeline);
                m_sliderTimeline->setValue(0);
                updateCurveMarker(m_currentAnimTime);
                renderCurrentFrame();
                return;
            }
            else {
                onStopClicked();
                return;
            }
        }

        // 根据当前动画时间反推帧索引，用于更新滑块
        const auto& frames = m_motionResultData->m_frames;
        auto it = std::lower_bound(frames.begin(), frames.end(), m_currentAnimTime,
            [](const MechMotionData::mMotionFrame& f, double val) {
                return f.m_time < val;
            });
        m_currentFrameIndex = static_cast<int>(std::distance(frames.begin(), it));
        if (m_currentFrameIndex >= static_cast<int>(frames.size())) {
            m_currentFrameIndex = static_cast<int>(frames.size()) - 1;
        }

        // 阻塞进度条自身信号，防止触发 onSliderMoved
        const QSignalBlocker blocker(m_sliderTimeline);
        m_sliderTimeline->setValue(m_currentFrameIndex);
        updateCurveMarker(m_currentAnimTime);

        // 渲染当前时间点（含插值）
        renderCurrentFrame();
    }

    void mMotionAnimationPanel::onSliderMoved(int frameIndex)
    {
        if (!m_motionResultData || frameIndex < 0 || frameIndex >= m_motionResultData->getFrameCount()) return;

        // 将动画时间同步为该索引对应的物理帧时间
        m_currentAnimTime = m_motionResultData->m_frames[frameIndex].m_time;
        m_currentFrameIndex = frameIndex;
        updateCurveMarker(m_currentAnimTime);

        renderCurrentFrame();
    }

    void mMotionAnimationPanel::setCurrentTime(double time)
    {
        if (!m_motionResultData || m_motionResultData->m_frames.empty()) return;

        m_currentAnimTime = qBound(m_motionResultData->m_frames.front().m_time, time,
            m_motionResultData->getTotalTime());

        // 与 onTimerTick 相同：按时间反推帧索引并同步滑块
        const auto& frames = m_motionResultData->m_frames;
        auto it = std::lower_bound(frames.begin(), frames.end(), m_currentAnimTime,
            [](const MechMotionData::mMotionFrame& f, double val) {
                return f.m_time < val;
            });
        m_currentFrameIndex = static_cast<int>(std::distance(frames.begin(), it));
        if (m_currentFrameIndex >= static_cast<int>(frames.size())) {
            m_currentFrameIndex = static_cast<int>(frames.size()) - 1;
        }

        const QSignalBlocker blocker(m_sliderTimeline);
        m_sliderTimeline->setValue(m_currentFrameIndex);

        renderCurrentFrame();
        updateCurveMarker(m_currentAnimTime);
    }

    void mMotionAnimationPanel::updateCurveMarker(double time)
    {
        if (!m_motionResultData || !m_chkFollowCurveMark || !m_chkFollowCurveMark->isChecked())
            return;

        auto* plotView = mCurveWindow::getInstance()->getPlotView();
        if (!plotView) return;

        auto* line = plotView->getMarkerLine(kCurveMarkerName);
        if (!line) {
            plotView->addMarkerLine(kCurveMarkerName, PlotView::mPlotMarkerLine::Vertical, time);
        }
        else {
            line->setValue(time);
        }
    }

    void mMotionAnimationPanel::removeCurveMarker()
    {
        auto* plotView = mCurveWindow::getInstance()->getPlotView();
        if (plotView && plotView->getMarkerLine(kCurveMarkerName)) {
            plotView->removeMarkerLine(kCurveMarkerName);
        }
    }

    void mMotionAnimationPanel::connectCurvePlotSignal()
    {
        auto* plotView = mCurveWindow::getInstance()->getPlotView();
        if (!plotView) return;

        disconnectCurvePlotSignal();
        connect(plotView, &PlotView::mPlotView::signalTimeClicked, this, [this](double time) {
            if (m_chkClickJump && m_chkClickJump->isChecked() && m_motionResultData)
                setCurrentTime(time);
        });
    }

    void mMotionAnimationPanel::disconnectCurvePlotSignal()
    {
        auto* plotView = mCurveWindow::getInstance()->getPlotView();
        if (!plotView) return;
        disconnect(plotView, &PlotView::mPlotView::signalTimeClicked, this, nullptr);
    }

    void mMotionAnimationPanel::resetBodyTransformsToIdentity()
    {
#ifdef MX_MOTION_OCC_COMPAT
        if (getCVM && getCVM->resetMotionDisplay)
            getCVM->resetMotionDisplay();
#else
        if (!getCVM || !getCVM->getModelData() || !getCVM->getPreRenderWidget())
            return;

        auto preRender = getCVM->getPreRenderWidget()->getPreRender();
        if (!preRender)
            return;

        auto* symbolController = getCVM->getPreRenderWidget()->GetSymbolController();

        auto& bodyManager = getCVM->getModelData()->getManager<MechMotionData::mMotionBodyManager>(Physical_MotionBody);
        for (int bodyId : bodyManager.getAllIDs())
        {
            auto* body = bodyManager.getData(bodyId);
            if (!body)
                continue;

            for (int partId : mMotionRefMap::getSet(Physical_MotionBody, bodyId, Physical_Part))
            {
                preRender->setModelMat(partId, glm::mat4(1.0f));
            }
        }

        // 将动画过程中更新的弹簧符号也恢复为初始位姿下的显示
        if (symbolController)
        {
            auto& springManager = getCVM->getModelData()->getManager<MechMotionData::mMotionSpringManager>(Physical_MotionSpring);
            for (int springId : springManager.getAllIDs())
            {
                MechMotionData::mMotionSpringBase* spring = springManager.getData(springId);
                if (!spring)
                    continue;

                SpringType springType = spring->getType();
                if (springType == SpringType::BodyPoint)
                {
                    auto* bpSpring = dynamic_cast<MechMotionData::mMotionBodyPointSpring*>(spring);
                    if (!bpSpring)
                        continue;

                    mMotionSpringPanel::createOrUpdateSpringSymbolByBodyTransforms(
                        symbolController,
                        bpSpring->getId(),
                        springType,
                        bpSpring->isDampingEnabled(),
                        bpSpring->getName(),
                        QMatrix4x4(),
                        QMatrix4x4(),
                        bpSpring->getLocalI(),
                        bpSpring->getLocalJ());
                    continue;
                }

                int jointId = mMotionRefMap::get(Physical_MotionSpring, spring->getId(), Physical_MotionJoint);
                if (jointId < 0)
                    continue;

                auto joint = dynamic_cast<MechMotionData::mMotionJointBasic*>(
                    getCVM->getModelData()->getManager<MechMotionData::mMotionJointManager>(Physical_MotionJoint).getData(jointId));
                if (!joint)
                    continue;

                mMotionSpringPanel::createOrUpdateSpringSymbol(
                    symbolController,
                    spring->getId(),
                    springType,
                    spring->isDampingEnabled(),
                    spring->getName(),
                    joint->getOrigin(),
                    joint->getAxis());
            }
        }

        preRender->updateModelMatrix();
        getCVM->getPreRenderWidget()->updatePaint();
#endif
    }

    // ==================== 核心：将数据喂给 OpenGL 渲染引擎 ====================
    void mMotionAnimationPanel::renderCurrentFrame()
    {
        if (!m_motionResultData || m_motionResultData->getFrameCount() == 0) return;

        double totalTime = m_motionResultData->getTotalTime();
        if (m_currentAnimTime < m_motionResultData->m_frames.front().m_time)
            m_currentAnimTime = m_motionResultData->m_frames.front().m_time;
        if (m_currentAnimTime > totalTime) m_currentAnimTime = totalTime;

        // 更新 UI 时间文字
        m_lblTimeInfo->setText(QString("时间: %1 s / %2 s")
            .arg(m_currentAnimTime, 0, 'f', 3)
            .arg(totalTime, 0, 'f', 3));

#ifdef MX_MOTION_OCC_COMPAT
        if (!getCVM || !getCVM->displayMotionTime)
            return;
        getCVM->displayMotionTime(m_id, m_currentAnimTime,
            m_cmbFlexContourRef->currentIndex(), m_chkFlexAdaptiveScale->isChecked());
        updateMarkerTraceCurrentPoints();
        if (getCVM->getPreRenderWidget())
            getCVM->getPreRenderWidget()->updatePaint();
#else

        // ==========================================
        // 1. 二分查找：当前时间卡在哪两个物理帧之间？
        // ==========================================
        const auto frameSpan = mMotionResultQueryService::locateTime(
            m_motionResultData, m_currentAnimTime);
        if (!frameSpan.isValid())
            return;

        const int frame1_idx = frameSpan.firstIndex;
        const int frame2_idx = frameSpan.secondIndex;
        const double fraction = frameSpan.ratio;
        const auto& frames = m_motionResultData->m_frames;

        // ==========================================
        // 2. 提取矩阵并进行平滑插值渲染
        // ==========================================
        auto& bodyManager = getCVM->getModelData()->getManager<MechMotionData::mMotionBodyManager>(Physical_MotionBody);
        // 缓存每个运动体在当前动画时刻的世界矩阵，供后续弹簧符号计算使用
        std::map<int, QMatrix4x4> bodyWorldMatrices;

        for (int bodyId : bodyManager.getAllIDs())
        {
            auto* body = bodyManager.getData(bodyId);
            if (!body) continue;

            //【刚柔耦合】被柔性体替换的刚体：接口体已入模态装配体，系统顶层无此体 →
            // 无每帧 body 矩阵。必须在矩阵查询 continue 之前处理，否则 part 永远
            // 停在单位矩阵（原始几何原地不动）。几何通道收缩到质心微点隐藏，
            // 网格通道由数据 mask 隐藏（见 renderFlexBodies/hideFlexReplacedParts）。
            bool replacedByFlex = false;
            if (m_motionResultData) {
                for (const auto& fb : m_motionResultData->m_flexBodies)
                    if (fb.m_bodyId == bodyId) { replacedByFlex = true; break; }
            }
            if (replacedByFlex) {
                const QVector3D com = body->getCenterOfMass();
                glm::mat4 shrink(1e-6f);   // 均匀缩到微点（不能全零——顶点坍到世界原点拉黑带）
                shrink[3] = glm::vec4(com.x(), com.y(), com.z(), 1.0f);
                for (int part_id : mMotionRefMap::getSet(Physical_MotionBody, bodyId, Physical_Part))
                    getCVM->getPreRenderWidget()->getPreRender()->setModelMat(part_id, shrink);
                continue;
            }

            QMatrix4x4 finalMat;
            if (!mMotionResultQueryService::getBodyMatrix(
                m_motionResultData, bodyId, body->getCenterOfMass(), m_currentAnimTime, finalMat))
                continue;

            bodyWorldMatrices[bodyId] = finalMat;

            // 显式元素拷贝：QMatrix4x4 → glm::mat4
            // 项目中其他代码（如 mPickToolClass::qMat4ToOsgMat4）使用转置映射：
            //   rel[col][row] = mat(row, col)
            // 因此此处也需转置以保持与渲染层一致
            glm::mat4 glmMat(1.0f);
            for (int row = 0; row < 4; ++row)
                for (int col = 0; col < 4; ++col)
                    glmMat[col][row] = finalMat(row, col);

            for (int part_id : mMotionRefMap::getSet(Physical_MotionBody, bodyId, Physical_Part))
                getCVM->getPreRenderWidget()->getPreRender()->setModelMat(part_id, glmMat);
        }

        // ==========================================
        // 3. 更新弹簧符号：根据关节两 marker 的世界位姿
        // ==========================================
        if (getCVM && getCVM->getPreRenderWidget())
        {
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController)
            {
                auto& springManager = getCVM->getModelData()->getManager<MechMotionData::mMotionSpringManager>(Physical_MotionSpring);

                const auto& jointFrames1 = frames[frame1_idx].m_jointFrames;
                const auto& jointFrames2 = frames[frame2_idx].m_jointFrames;

                std::vector<int> springIds = springManager.getAllIDs();
                for (int springId : springIds)
                {
                    MechMotionData::mMotionSpringBase* spring = springManager.getData(springId);
                    if (!spring) continue;

                    SpringType springType = spring->getType();

                    // 体-点弹簧不依赖运动副，直接从 body 世界矩阵计算两端点
                    if (springType == SpringType::BodyPoint)
                    {
                        MechMotionData::mMotionBodyPointSpring* bpSpring = dynamic_cast<MechMotionData::mMotionBodyPointSpring*>(spring);
                        if (!bpSpring) continue;

                        auto itMatI = bodyWorldMatrices.find(mMotionRefMap::get(Physical_MotionSpring, bpSpring->getId(), Physical_MotionBody, MapRelation_MotionBodyI));
                        auto itMatJ = bodyWorldMatrices.find(mMotionRefMap::get(Physical_MotionSpring, bpSpring->getId(), Physical_MotionBody, MapRelation_MotionBodyJ));
                        if (itMatI == bodyWorldMatrices.end() || itMatJ == bodyWorldMatrices.end())
                            continue;

                        mMotionSpringPanel::createOrUpdateSpringSymbolByBodyTransforms(
                            symbolController,
                            bpSpring->getId(),
                            springType,
                            bpSpring->isDampingEnabled(),
                            bpSpring->getName(),
                            itMatI->second,
                            itMatJ->second,
                            bpSpring->getLocalI(),
                            bpSpring->getLocalJ());
                        continue;
                    }

                    int jointId = mMotionRefMap::get(Physical_MotionSpring, spring->getId(), Physical_MotionJoint);
                    if (jointId < 0) continue;

                    auto itJF1 = jointFrames1.find(jointId);
                    auto itJF2 = jointFrames2.find(jointId);
                    if (itJF1 == jointFrames1.end() || itJF2 == jointFrames2.end())
                        continue;

                    const auto& jf1 = itJF1->second;
                    const auto& jf2 = itJF2->second;

                    QVector3D posI, posJ;
                    QQuaternion rotI, rotJ;

                    if (frame1_idx == frame2_idx || fraction <= 0.001) {
                        posI = jf1.m_nodeI.m_worldPosition;
                        posJ = jf1.m_nodeJ.m_worldPosition;
                        rotI = jf1.m_nodeI.m_worldRotation;
                        rotJ = jf1.m_nodeJ.m_worldRotation;
                    }
                    else if (fraction >= 0.999) {
                        posI = jf2.m_nodeI.m_worldPosition;
                        posJ = jf2.m_nodeJ.m_worldPosition;
                        rotI = jf2.m_nodeI.m_worldRotation;
                        rotJ = jf2.m_nodeJ.m_worldRotation;
                    }
                    else {
                        posI = jf1.m_nodeI.m_worldPosition +
                            (jf2.m_nodeI.m_worldPosition - jf1.m_nodeI.m_worldPosition) * static_cast<float>(fraction);
                        posJ = jf1.m_nodeJ.m_worldPosition +
                            (jf2.m_nodeJ.m_worldPosition - jf1.m_nodeJ.m_worldPosition) * static_cast<float>(fraction);
                        rotI = QQuaternion::slerp(jf1.m_nodeI.m_worldRotation, jf2.m_nodeI.m_worldRotation, static_cast<float>(fraction));
                        rotJ = QQuaternion::slerp(jf1.m_nodeJ.m_worldRotation, jf2.m_nodeJ.m_worldRotation, static_cast<float>(fraction));
                    }

                    mMotionSpringPanel::createOrUpdateSpringSymbolByMarkers(
                        symbolController,
                        spring->getId(),
                        springType,
                        spring->isDampingEnabled(),
                        spring->getName(),
                        posI,
                        posJ,
                        rotI,
                        rotJ);
                }
            }
        }

        renderFlexBodies();

        getCVM->getPreRenderWidget()->getPreRender()->updateModelMatrix();
        updateMarkerTraceCurrentPoints();
        getCVM->getPreRenderWidget()->updatePaint();
#endif
    }

#ifndef MX_MOTION_OCC_COMPAT
    // 四元数 [x,y,z,w] → 旋转向量（axis*angle），用于接口体 6 维增量中的转动部分
    static void quatToRotationVector(double qx, double qy, double qz, double qw, double& wx, double& wy, double& wz)
    {
        const double n = std::sqrt(qx * qx + qy * qy + qz * qz);
        const double angle = 2.0 * std::atan2(n, qw);
        if (n < 1e-12) { wx = wy = wz = 0.0; return; }
        wx = qx / n * angle;
        wy = qy / n * angle;
        wz = qz / n * angle;
    }

    //【刚柔耦合】动画期间隐藏被柔体替换的部件：几何+网格双通道走数据 mask
    //（与树节点右键隐藏同机制——updateAllGeoModelOperate/updateAllMeshModelOperate
    // 会按 mask 全量重打包两条渲染通道），并记录原值供恢复。mask 极性：true=隐藏。
    void mMotionAnimationPanel::hideFlexReplacedParts(const MechMotionData::mMotionResultData* result)
    {
        if (!getCVM || !getCVM->getModelData())
            return;
        auto* meshData = getCVM->getModelData()->getMeshData();
        if (!meshData || !result)
            return;

        std::set<int> partIds;
        for (const auto& fb : result->m_flexBodies)
            for (int partId : mMotionRefMap::getSet(Physical_MotionBody, fb.m_bodyId, Physical_Part))
                partIds.insert(partId);
        if (partIds.empty())
            return;

        for (int partId : partIds) {
            if (m_flexSavedPartMasks.count(partId))
                continue;  // 已记录过（结果集切换中途），不覆盖首次的原值
            m_flexSavedPartMasks[partId] = { meshData->getGeoPartMask(partId), meshData->getMeshPartMask(partId) };
            meshData->setGeoPartMask(partId, true);
            meshData->setMeshPartMask(partId, true);
        }
        getCVM->updateAllGeoModelOperate(mxr::HideOnePart, partIds);
        getCVM->updateAllMeshModelOperate(mxr::HideOnePart);
    }

    //【刚柔耦合】退出动画/结果切换时恢复部件显隐原状（用户自己隐藏的部件保持隐藏）
    void mMotionAnimationPanel::restoreFlexReplacedParts()
    {
        if (m_flexSavedPartMasks.empty())
            return;
        if (getCVM && getCVM->getModelData() && getCVM->getModelData()->getMeshData()) {
            auto* meshData = getCVM->getModelData()->getMeshData();
            std::set<int> partIds;
            for (const auto& kv : m_flexSavedPartMasks) {
                meshData->setGeoPartMask(kv.first, kv.second.first);
                meshData->setMeshPartMask(kv.first, kv.second.second);
                partIds.insert(kv.first);
            }
            getCVM->updateAllGeoModelOperate(mxr::ShowOnePart, partIds);
            getCVM->updateAllMeshModelOperate(mxr::ShowOnePart);
        }
        m_flexSavedPartMasks.clear();
    }

    //【刚柔耦合】全程最大弹性变形（只依赖模态列·q 时程，接口刚体运动不影响弹性量）
    double mMotionAnimationPanel::computeMaxElastic(const MechMotionData::mMotionFlexBodyData& fb)
    {
        const int N = fb.m_nodeX0.size() / 3;
        const int m = fb.m_nModes;
        const int K = fb.m_nInterfaces;
        const int cols = 6 * K + m;
        if (N <= 0 || m <= 0 || fb.m_psi.size() != (qint64)3 * N * cols)
            return 0.0;
        const double* psi = fb.m_psi.constData();
        const int modStart = 6 * K;
        double maxE = 0.0;
        for (const auto& frame : fb.m_frames) {
            if (frame.m_modalQ.size() < m)
                continue;
            for (int r = 0; r < N; ++r) {
                const double* rowX = psi + (size_t)(3 * r) * cols + modStart;
                const double* rowY = rowX + cols;   // 行距=整行宽（同列块跨行）；
                const double* rowZ = rowY + cols;   // 此前误用 cols-modStart → 落到边界静力列
                double ex = 0, ey = 0, ez = 0;
                for (int c = 0; c < m; ++c) {
                    ex += rowX[c] * frame.m_modalQ[c];
                    ey += rowY[c] * frame.m_modalQ[c];
                    ez += rowZ[c] * frame.m_modalQ[c];
                }
                maxE = std::max(maxE, std::sqrt(ex * ex + ey * ey + ez * ez));
            }
        }
        return maxE;
    }

    //【刚柔耦合】变形云图图例：复用前处理视口色带（网格质量云图同款窗口）。
    // 默认渐变红(max)→蓝(min)，与顶点着色 ramp 一致，数值等差 8 段。
    void mMotionAnimationPanel::showFlexDeformLegend(double maxElastic)
    {
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;
        auto* colorTable = getCVM->getPreRenderWidget()->getColorTableRender();
        if (!colorTable)
            return;
        const float maxV = (float)std::max(maxElastic, 1e-9);
        colorTable->setValueCountAndMinMax(0, 8, 0.0f, maxV, true)->setFontColor({ 0, 0, 0 });
        // 变形常为 μm~mm 级：大值定点 4 位，小值（<0.01mm）切科学计数 3 位，避免色标全 0
        colorTable->SetValueStyle(maxElastic < 0.01 ? 1 : 0, maxElastic < 0.01 ? 3 : 4);
        // 标题随云图参照系（NX 口径=偏差 / 弹性口径=挠度），无括号单位
        colorTable->SetHeader(0, ((!m_cmbFlexContourRef || m_cmbFlexContourRef->currentIndex() == 0)
            ? QString::fromUtf8("变形偏差 mm")
            : QString::fromUtf8("弹性变形 mm")).toStdString());
        colorTable->showOrHidleHeaderLabel(true);
        colorTable->setVisiable(true);
    }

    void mMotionAnimationPanel::hideFlexDeformLegend()
    {
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;
        if (auto* colorTable = getCVM->getPreRenderWidget()->getColorTableRender())
            colorTable->setVisiable(false);
    }

    //【刚柔耦合】柔体变形网格回放：一次性建网格（结果集切换时），每帧 CPU 展开
    // x = x0 + Psi * [db_1..db_K, q] 并刷 Positions；节点按弹性变形量着色（蓝→红云图）
    void mMotionAnimationPanel::renderFlexBodies()
    {
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;
        mxr::mFlexBodyRender* flexRend = getCVM->getPreRenderWidget()->getPreRender()->getFlexBodyRender();
        if (!flexRend)
            return;

        const MechMotionData::mMotionResultData* result = m_motionResultData;
        if (!result || result->m_flexBodies.isEmpty()) {
            if (m_flexBuiltForResult != nullptr) {
                flexRend->clear();
                m_flexBuiltForResult = nullptr;
                restoreFlexReplacedParts();   // 恢复原部件显隐 + 收掉云图图例
                hideFlexDeformLegend();
                m_flexMaxElastic.clear();
            m_flexFreeModes.clear();
                m_flexNodeWeights.clear();
                m_flexFreeModes.clear();
                m_flexLegendShownMax = -1.0;
                getCVM->getPreRenderWidget()->updatePaint();
            }
            return;
        }

        // 结果集切换 → 重建柔体网格（原刚体 part 由 mask 隐藏，见 hideFlexReplacedParts）
        if (m_flexBuiltForResult != result) {
            flexRend->clear();
            flexRend->deleteFlexBody(QStringLiteral("__flex_preview__"));  // 清掉保存时的青色预览（静态不跟随，回放中是残影）
            restoreFlexReplacedParts();   // 先恢复上一结果集的部件（幂等）
            m_flexMaxElastic.clear();
            m_flexFreeModes.clear();
            m_flexNodeWeights.clear();
                m_flexFreeModes.clear();
            double globalMaxE = 0.0;
            for (const auto& fb : result->m_flexBodies) {
                const double maxE = computeMaxElastic(fb);
                m_flexMaxElastic[fb.m_bodyId] = maxE;
                globalMaxE = std::max(globalMaxE, maxE);
            }
            hideFlexReplacedParts(result);
            showFlexDeformLegend(globalMaxE);   // 配好色带/格式/初值（此后 GetColorData 字节就绪）
            m_flexLegendShownMax = -1.0;        // 强制首帧按当前口径刷新图例色标与 GPU 值域

            //【GPU 云图】图例色带字节 → 值通道网格（片元采样期归一+查表，
            // NEAREST 离散带与图例逐像素一致；口径切换只动 uniform）
            auto* colorTable = getCVM->getPreRenderWidget()->getColorTableRender();
            const unsigned char* legendRGB = colorTable ? colorTable->GetColorData() : nullptr;
            const int legendCount = colorTable ? colorTable->GetColorCount() : 0;
            for (const auto& fb : result->m_flexBodies) {
                const int nNodes = fb.m_nodeX0.size() / 3;
                QVector<QVector3D> nodes;
                nodes.reserve(nNodes);
                for (int i = 0; i < nNodes; ++i)
                    nodes.push_back(QVector3D(fb.m_nodeX0[3 * i], fb.m_nodeX0[3 * i + 1], fb.m_nodeX0[3 * i + 2]));
                QVector<uint32_t> tris;
                for (int t : fb.m_surfaceTris)
                    tris.push_back((uint32_t)t);
                const QString key = QString::number(fb.m_bodyId);
                if (legendRGB && legendCount >= 2) {
                    flexRend->appendFlexBodyValue(key, nodes, tris, legendRGB, legendCount);
                    flexRend->setFlexValueRange(key, 0.0, std::max(globalMaxE, 1e-9));
                } else {
                    // 图例不可用应急兜底：通用着色路径（静态青色，不随帧变形）
                    flexRend->appendFlexBody(key, nodes, tris, QVector4D(0.35f, 0.65f, 0.95f, 1.0f));
                }
                //【NX 口径】节点集总体积权（均匀材料=质量加权；四面体体积均摊到 4 角点，
                // 零体积/无四面体节点由拟合内均匀权兜底），归一化均值 1 防数值悬殊
                {
                    QVector<double> w(nNodes, 0.0);
                    const int nT = fb.m_tetIds.size() / 4;
                    for (int t = 0; t < nT; ++t) {
                        const int ia = fb.m_tetIds[4 * t], ib = fb.m_tetIds[4 * t + 1];
                        const int ic = fb.m_tetIds[4 * t + 2], id = fb.m_tetIds[4 * t + 3];
                        if (ia < 0 || ia >= nNodes || ib < 0 || ib >= nNodes ||
                            ic < 0 || ic >= nNodes || id < 0 || id >= nNodes)
                            continue;
                        const QVector3D pa(fb.m_nodeX0[3 * ia], fb.m_nodeX0[3 * ia + 1], fb.m_nodeX0[3 * ia + 2]);
                        const QVector3D pb(fb.m_nodeX0[3 * ib], fb.m_nodeX0[3 * ib + 1], fb.m_nodeX0[3 * ib + 2]);
                        const QVector3D pc(fb.m_nodeX0[3 * ic], fb.m_nodeX0[3 * ic + 1], fb.m_nodeX0[3 * ic + 2]);
                        const QVector3D pd(fb.m_nodeX0[3 * id], fb.m_nodeX0[3 * id + 1], fb.m_nodeX0[3 * id + 2]);
                        const double vol = std::abs(QVector3D::dotProduct(pb - pa, QVector3D::crossProduct(pc - pa, pd - pa))) / 6.0;
                        w[ia] += vol / 4.0; w[ib] += vol / 4.0; w[ic] += vol / 4.0; w[id] += vol / 4.0;
                    }
                    double wSum = 0.0;
                    for (double v : w) wSum += v;
                    if (wSum > 0.0 && nNodes > 0)
                        for (double& v : w) v /= (wSum / nNodes);
                    m_flexNodeWeights[fb.m_bodyId] = w;
                }
                //【UG 口径】自由基第一弯曲模态形状（渲染端计算，不影响求解）
                m_flexFreeModes[fb.m_bodyId] = mMotionFlexBridge::computeFreeMode(
                    fb.m_nodeX0, fb.m_tetIds, 7.85e-9);
            }
            m_flexBuiltForResult = result;
        }

        //【GPU 云图·逐帧】第一遍：展开各柔体本帧节点位置+云图值（参照系由面板下拉
        // 决定：NX 偏差口径默认 / 弹性口径）直灌 GPU（归一/查表在片元采样期）；
        // frameMaxE 供逐帧自适应口径的图例/值域
        // 参照系：整体运动口径默认（index 0=NX 加权拟合偏差；1=弹性接口口径）
        const bool nxRef = !m_cmbFlexContourRef || m_cmbFlexContourRef->currentIndex() == 0;
        double frameMaxE = 0.0;
        for (const auto& fb : result->m_flexBodies) {
            if (fb.m_frames.isEmpty())
                continue;
            // 取当前时间最近帧（首版不插值）
            int fi = 0;
            double bestAbs = 1e300;
            for (int i = 0; i < fb.m_frames.size(); ++i) {
                const double a = std::abs(fb.m_frames[i].m_time - m_currentAnimTime);
                if (a < bestAbs) { bestAbs = a; fi = i; }
            }
            const auto& frame = fb.m_frames[fi];

            const int N = fb.m_nodeX0.size() / 3;
            const int K = fb.m_nInterfaces;
            const int m = fb.m_nModes;
            const int cols = 6 * K + m;
            if (fb.m_psi.size() != (qint64)3 * N * cols || frame.m_interfacePose.size() < 7 * K)  // 3N行×cols列
                continue;

            // 增量列向量 [db; q]
            QVector<double> red(cols, 0.0);
            for (int k = 0; k < K; ++k) {
                const double* pT = frame.m_interfacePose.constData() + 7 * k;        // 当前位姿
                const double* p0 = fb.m_interfacePose0.constData() + 7 * k;           // 初始位姿
                // 平移增量（近似：当前 q(t)·q0^-1 作用于初始位置处省略小量，首版直接差）
                red[6 * k + 0] = pT[0] - p0[0];
                red[6 * k + 1] = pT[1] - p0[1];
                red[6 * k + 2] = pT[2] - p0[2];
                // 旋转增量：dq = q(t) * q0^-1（共轭），转旋转向量
                // Chrono 四元数 data() = [w,x,y,z]（e0=标量），此前错读为 [x,y,z,w]
                const double qw = pT[3], qx = pT[4], qy = pT[5], qz = pT[6];
                const double mw0 = p0[3], mx0 = p0[4], my0 = p0[5], mz0 = p0[6];
                // q0 归一化共轭
                const double n0 = std::sqrt(mx0 * mx0 + my0 * my0 + mz0 * mz0 + mw0 * mw0);
                const double cx = -mx0 / n0, cy = -my0 / n0, cz = -mz0 / n0, cw = mw0 / n0;
                // dq = q * q0^-1
                const double dqx = qw * cx + qx * cw + qy * cz - qz * cy;
                const double dqy = qw * cy - qx * cz + qy * cw + qz * cx;
                const double dqz = qw * cz + qx * cy - qy * cx + qz * cw;
                const double dqw = qw * cw - qx * cx - qy * cy - qz * cz;
                double rx, ry, rz;
                quatToRotationVector(dqx, dqy, dqz, dqw, rx, ry, rz);
                red[6 * k + 3] = rx;
                red[6 * k + 4] = ry;
                red[6 * k + 5] = rz;
            }
            for (int i = 0; i < m && i < frame.m_modalQ.size(); ++i)
                red[6 * K + i] = frame.m_modalQ[i];

            // 保存接口体位姿供刚体变换用
            const double* p0_list = fb.m_interfacePose0.constData();
            const double* pT_list = frame.m_interfacePose.constData();
            const double mw0_val = p0_list[3], mx0_val = p0_list[4], my0_val = p0_list[5], mz0_val = p0_list[6];

            // 展开：刚体变换（直接用接口体位姿，不通过 Psi 边界列——静力凝聚列
            // 对远距离旋转映射不准）+ 纯模态弹性（只取 Psi 的模态列）
            QVector<QVector3D> nodes(N);
            QVector<double> elastic(N);         // 本帧每节点原始弹性变形量（GPU 采样期归一）
            const double* psi = fb.m_psi.constData();
            // 刚体变换参数：当前接口体位姿 vs 初始
            QVector3D bodyPos0(p0_list[0], p0_list[1], p0_list[2]);
            QVector3D bodyPosT(pT_list[0], pT_list[1], pT_list[2]);
            QQuaternion bodyQuat0 = QQuaternion(mw0_val, mx0_val, my0_val, mz0_val); // (w,x,y,z)
            QQuaternion bodyQuatT = QQuaternion(pT_list[3], pT_list[4], pT_list[5], pT_list[6]); // (w,x,y,z)
            QQuaternion dq = bodyQuatT * bodyQuat0.conjugated();
            const int modStart = 6 * K;
            for (int r = 0; r < N; ++r) {
                QVector3D x0(fb.m_nodeX0[3*r], fb.m_nodeX0[3*r+1], fb.m_nodeX0[3*r+2]);
                // 刚体：绕接口体初始位置旋转 + 平移到当前位置
                QVector3D rigid = dq.rotatedVector(x0 - bodyPos0) + bodyPosT;
                // 弹性：只取模态列（跳过前 6K 列边界列）
                double ex = 0, ey = 0, ez = 0;
                const double* rowX = psi + (size_t)(3 * r) * cols + modStart;
                const double* rowY = rowX + cols;   // 行距=整行宽（同列块跨行）；
                const double* rowZ = rowY + cols;   // 此前误用 cols-modStart → y/z 读到边界静力列（云图分布错）
                for (int c = 0; c < m; ++c) {
                    ex += rowX[c] * red[modStart + c];
                    ey += rowY[c] * red[modStart + c];
                    ez += rowZ[c] * red[modStart + c];
                }
                nodes[r] = rigid + dq.rotatedVector(QVector3D(ex, ey, ez));  // 弹性在体坐标系，随接口体姿态旋转到世界系
                elastic[r] = std::sqrt(ex * ex + ey * ey + ez * ez);
            }
            //【云图参照系】默认 NX/UG 口径：自由基第一弯曲模态贡献量（两个节点→双蓝块，
            // 与 UG 的 MNF 自由基云图同分布形态）；弹性口径=相对接口焊接系的 |Ψ·q|
            QVector<double> contourValues = elastic;
            if (nxRef) {
                auto fmIt = m_flexFreeModes.find(fb.m_bodyId);
                if (fmIt != m_flexFreeModes.end() && fmIt->second.modeMass > 0) {
                    // 弹性变形向量（体坐标系，不旋转——投影在体系做）
                    QVector<double> elasticFlat(3 * N);
                    for (int r = 0; r < N; ++r) {
                        double ex2=0, ey2=0, ez2=0;
                        const double* rX = psi + (size_t)(3 * r) * cols + modStart;
                        const double* rY = rX + cols;
                        const double* rZ = rY + cols;
                        for (int c = 0; c < m; ++c) {
                            ex2 += rX[c] * red[modStart + c];
                            ey2 += rY[c] * red[modStart + c];
                            ez2 += rZ[c] * red[modStart + c];
                        }
                        elasticFlat[3*r] = ex2; elasticFlat[3*r+1] = ey2; elasticFlat[3*r+2] = ez2;
                    }
                    contourValues = mMotionFlexBridge::freeModeContour(fmIt->second, elasticFlat);
                } else {
                    // 自由基不可用兜底：NX 拟合偏差（接口体已知旋转）
                    auto wIt = m_flexNodeWeights.find(fb.m_bodyId);
                    contourValues = mMotionFlexBridge::nxContourValues(
                        fb.m_nodeX0, nodes,
                        wIt != m_flexNodeWeights.end() ? wIt->second : QVector<double>(),
                        &dq);
                }
            }
            for (double v : contourValues)
                frameMaxE = std::max(frameMaxE, v);
            // 位置+值一次直灌（角点展开在渲染器内完成；线层同步）
            flexRend->updateFlexValues(QString::number(fb.m_bodyId), nodes, contourValues);
        }

        // 第二遍：图例 + GPU 值域同步（只动 uniform/纹理，零数据重灌）
        // 口径：全程定标（默认，颜色随变形幅值演化）/ 逐帧自适应（面板开关）
        {
            double fullMaxE = 1e-9;
            for (const auto& kv : m_flexMaxElastic)
                fullMaxE = std::max(fullMaxE, kv.second);
            const bool adaptive = m_chkFlexAdaptiveScale && m_chkFlexAdaptiveScale->isChecked();
            const double targetMax = adaptive ? frameMaxE : fullMaxE;
            auto* colorTable = getCVM->getPreRenderWidget()->getColorTableRender();
            if (colorTable) {
                // 色标变化才重刷（图例数值纹理 + GPU 归一值域；口径切换由
                // toggled 槽置 -1 强制刷新）
                if (std::abs(targetMax - m_flexLegendShownMax) > 1e-15) {
                    m_flexLegendShownMax = targetMax;
                    colorTable->setValueCountAndMinMax(0, 8, 0.0f, (float)std::max(targetMax, 1e-9), true);
                    const double rangeMax = std::max(targetMax, 1e-9);
                    for (const auto& fb : result->m_flexBodies)
                        flexRend->setFlexValueRange(QString::number(fb.m_bodyId), 0.0, rangeMax);
                }
                // 图例色带字节可能被用户编辑 → 刷 GPU 色表纹理（内部 diff，未变 no-op）
                const unsigned char* legendRGB = colorTable->GetColorData();
                const int legendCount = colorTable->GetColorCount();
                if (legendRGB && legendCount >= 2) {
                    for (const auto& fb : result->m_flexBodies)
                        flexRend->updateFlexColorTable(QString::number(fb.m_bodyId), legendRGB, legendCount);
                }
            }
        }
    }

#endif

    void mMotionAnimationPanel::refreshMarkerTraceList()
    {
        if (!m_foldMarkerTrace)
            return;

        qDeleteAll(m_markerTraceCheckBoxes);
        m_markerTraceCheckBoxes.clear();

        if (!getCVM || !getCVM->getModelData()) {
            m_foldMarkerTrace->refreshLayout();
            return;
        }

        auto scenario = getCVM->getModelData()->getManager<MechMotionData::mMotionScenarioManager>(Physical_MotionScenario).getData(m_id);
        if (!scenario) {
            m_foldMarkerTrace->refreshLayout();
            return;
        }

        int row = 0;
        auto& markerManager = getCVM->getModelData()->getManager<MechMotionData::mMotionMarkerManager>(Physical_MotionMarker);
        for (int markerId : markerManager.getAllIDs()) {
            auto* marker = markerManager.getData(markerId);
            if (!marker)
                continue;

            QCheckBox* checkBox = new QCheckBox(marker->getName(), m_foldMarkerTrace);
            checkBox->setChecked(scenario->isTrackingMarker(markerId));
            m_foldMarkerTrace->insertRow(row, checkBox);
            m_markerTraceCheckBoxes.push_back(checkBox);

            connect(checkBox, &QCheckBox::toggled, this, [this, scenario, markerId](bool checked) {
                if (checked)
                    scenario->addTrackedMarkerId(markerId);
                else
                    scenario->removeTrackedMarkerId(markerId);

                rebuildMarkerTraces();
                });
            ++row;
        }

        // 清除按钮保持最后一行：新复选框始终插在它前面。
        m_foldMarkerTrace->refreshLayout();
    }

    void mMotionAnimationPanel::rebuildMarkerTraces()
    {
        clearMarkerTraces();
        if (!getCVM || !getCVM->getModelData() || !getCVM->getPreRenderWidget() ||
            !m_motionResultData || m_motionResultData->m_frames.empty())
            return;

        auto scenario = getCVM->getModelData()->getManager<MechMotionData::mMotionScenarioManager>(Physical_MotionScenario).getData(m_id);
        if (!scenario)
            return;

        auto commonShapeRender = getCVM->getPreRenderWidget()->getCommonShapeRender();
        m_renderedTraceMarkerIds.clear();
        for (int markerId : scenario->getTrackedMarkerIds()) {
            QVector<QVector3D> points;
            if (mMotionResultQueryService::getMarkerPositions(
                m_motionResultData, markerId, points)) {
                commonShapeRender->appendPoint(
                    QString("markerTrace_%1").arg(markerId),
                    points,
                    QVector3D(255, 128, 0) / 255.0f,
                    3.0f);
                m_renderedTraceMarkerIds.insert(markerId);
            }
        }

        updateMarkerTraceCurrentPoints();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mMotionAnimationPanel::updateMarkerTraceCurrentPoints()
    {
        auto commonShapeRender = getCVM && getCVM->getPreRenderWidget()
            ? getCVM->getPreRenderWidget()->getCommonShapeRender()
            : nullptr;
        if (!commonShapeRender || !m_motionResultData || m_motionResultData->m_frames.empty())
            return;

        std::set<int> currentIds = m_renderedCurrentMarkerIds;
        m_renderedCurrentMarkerIds.clear();
        for (int markerId : currentIds) {
            commonShapeRender->deletePoint(QString("markerTraceCurrent_%1").arg(markerId));
        }

        if (!getCVM || !getCVM->getModelData())
            return;

        auto scenario = getCVM->getModelData()->getManager<MechMotionData::mMotionScenarioManager>(Physical_MotionScenario).getData(m_id);
        if (!scenario)
            return;

        for (int markerId : scenario->getTrackedMarkerIds()) {
            mMotionResultQueryService::MarkerPosition currentPosition;
            if (!mMotionResultQueryService::getMarkerPosition(
                m_motionResultData, markerId, m_currentAnimTime, currentPosition))
                continue;

            const QVector3D position = currentPosition.position;
            commonShapeRender->appendPoint(
                QString("markerTraceCurrent_%1").arg(markerId),
                { position },
                QVector3D(255, 213, 79) / 255.0f,
                8.0f);
            m_renderedCurrentMarkerIds.insert(markerId);
        }
    }

    void mMotionAnimationPanel::clearMarkerTraces()
    {
        if (!getCVM || !getCVM->getModelData() || !getCVM->getPreRenderWidget() ||
            !getCVM->getPreRenderWidget()->getCommonShapeRender())
            return;

        auto commonShapeRender = getCVM->getPreRenderWidget()->getCommonShapeRender();
        auto& markerManager = getCVM->getModelData()->getManager<MechMotionData::mMotionMarkerManager>(Physical_MotionMarker);
        std::set<int> traceIds = m_renderedTraceMarkerIds;
        m_renderedTraceMarkerIds.clear();
        for (int markerId : markerManager.getAllIDs()) {
            traceIds.insert(markerId);
        }

        auto scenario = getCVM->getModelData()->getManager<MechMotionData::mMotionScenarioManager>(Physical_MotionScenario).getData(m_id);
        if (scenario) {
            for (int markerId : scenario->getTrackedMarkerIds()) {
                traceIds.insert(markerId);
            }
        }

        for (int markerId : traceIds) {
            commonShapeRender->deletePoint(QString("markerTrace_%1").arg(markerId));
            commonShapeRender->deletePoint(QString("markerTraceCurrent_%1").arg(markerId));
        }

        getCVM->getPreRenderWidget()->updatePaint();
    }
}
