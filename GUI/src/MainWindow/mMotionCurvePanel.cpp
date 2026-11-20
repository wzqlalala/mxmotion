
#include "mMotionCurvePanel.h"
#include "mMotionResultQueryService.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"

#include "mViewportModelManager.h"

#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QColorDialog>
#include <QMessageBox>
#include "mViewportModel.h"
#include "mModelData.h"
#include "mMotionScenario.h"
#include "mMotionScenarioManager.h"
#include "mMotionMarkerManager.h"
#include "mMotionBodyManager.h"
#include "mMotionJointManager.h"
#include "mMotionCouplingManager.h"
#include "mMotionCouplingBase.h"
#include "mMotionDriveManager.h"
#include "mMotionMarker.h"
#include "mMotionBody.h"
#include "mMotionJointBase.h"
#include "mMotionDriveBase.h"
#include "mMotionSensorBase.h"
#include "mMotionDriveSensor.h"
#include "mMotionSensorManager.h"
#include "mMotionControllerManager.h"
#include "mMotionControllerBase.h"

#include "mCurveWindow.h"
#include "mPlotView.h"
#include "mPlotBasic.h"
#include "mPlotViewStyleWidget.h"
#include "mDockLayoutManager.h"

#include <QHeaderView>
#include <QCoreApplication>

#include <algorithm>
#include <limits>

using namespace MechMotionData;

namespace
{
    const MechMotionData::mMotionCurveRequest* findRequest(
        const MechMotionData::mMotionScenario* scenario, int requestId)
    {
        if (!scenario) return nullptr;
        for (const auto& item : scenario->getCurveRequests()) {
            if (item.m_requestId == requestId)
                return &item;
        }
        return nullptr;
    }

    const MechMotionData::mMotionSensorBase* findSensor(int sensorId)
    {
        if (!getCVM || !getCVM->getModelData() || sensorId < 0)
            return nullptr;
        return getCVM->getModelData()->getManager<MechMotionData::mMotionSensorManager>(
            Physical_MotionSensor).getData(sensorId);
    }

    const MechMotionData::mMotionControllerBase* findController(int controllerId)
    {
        if (!getCVM || !getCVM->getModelData() || controllerId < 0)
            return nullptr;
        return getCVM->getModelData()->getManager<MechMotionData::mMotionControllerManager>(
            Physical_MotionController).getData(controllerId);
    }

    // 标量型传感器：曲线端只有一个"传感器值"，分量选择无意义
    bool sensorIsScalar(const MechMotionData::mMotionSensorBase* sensor)
    {
        if (!sensor)
            return true;
        switch (sensor->getType()) {
        case MechMotionData::SensorType::Distance:
        case MechMotionData::SensorType::Angle:
            return true;
        case MechMotionData::SensorType::DriveActuation: {
            const auto var = static_cast<const MechMotionData::mMotionDriveSensor*>(sensor)->getVariable();
            return var == MechMotionData::DriveSensorVariable::ImposedValue
                || var == MechMotionData::DriveSensorVariable::ImposedSpeed
                || var == MechMotionData::DriveSensorVariable::ImposedAcc;
        }
        default:
            return false;
        }
    }

    // 在序列中取最接近 t 的采样值（各曲线同源于求解帧，最近采样即精确匹配）
    double valueAtTime(const QVector<double>& times, const QVector<double>& values, double t)
    {
        const int count = qMin(times.size(), values.size());
        if (count == 0) return 0.0;
        const auto it = std::lower_bound(times.begin(), times.end(), t);
        if (it == times.begin()) return values[0];
        if (it == times.end()) return values[count - 1];
        const int hi = static_cast<int>(it - times.begin());
        const int lo = hi - 1;
        return (t - times[lo] <= times[hi] - t) ? values[lo] : values[hi];
    }
}

namespace MechMainWindow
{
    mMotionCurvePanel::mMotionCurvePanel(QWidget* parent)
        : mPanelBase(parent), m_scenarioId(-1)
    {
        setupUI();
        populateSourceTypeCombo();
        onSourceTypeChanged(m_comboSourceType->currentIndex());
    }

    mMotionCurvePanel::~mMotionCurvePanel()
    {
        if (m_mainLayout != nullptr)
        {
            delete m_mainLayout;
            m_mainLayout = nullptr;
        }
    }

    void mMotionCurvePanel::setupUI()
    {
        m_mainLayout = new QVBoxLayout(this);
        m_mainLayout->setSpacing(8);
        m_mainLayout->setContentsMargins(12, 12, 12, 12);

        // --- 1. 曲线配置 ---
        FoldSection* foldConfig = new FoldSection("曲线配置", this);

        m_comboSourceType = new QComboBox();
        m_comboSourceType->setMinimumWidth(180);
        foldConfig->addRow(new QLabel("源类型:"), m_comboSourceType);

        m_comboObject = new QComboBox();
        m_comboObject->setMinimumWidth(180);
        foldConfig->addRow(new QLabel("对象:"), m_comboObject);

        m_comboVarComp = new QComboBox();
        m_comboVarComp->setMinimumWidth(180);
        foldConfig->addRow(new QLabel("变量-分量:"), m_comboVarComp);

        m_editCurveName = new QLineEdit();
        m_editCurveName->setPlaceholderText("留空使用默认名称");
        foldConfig->addRow(new QLabel("曲线名称:"), m_editCurveName);

        m_btnAddCurve = new QPushButton("添加曲线");
        m_btnAddCurve->setStyleSheet("QPushButton { background-color: #409EFF; color: white; font-weight: bold; }");
        m_btnClearCurves = new QPushButton("清空");
        QHBoxLayout* addRow = new QHBoxLayout();
        addRow->addStretch();
        addRow->addWidget(m_btnClearCurves);
        addRow->addWidget(m_btnAddCurve);
        foldConfig->addRow(addRow);

        m_mainLayout->addWidget(foldConfig);

        // --- 2. 曲线列表 ---
        FoldSection* foldList = new FoldSection("曲线列表", this);
        m_listCurves = new QListWidget();
        m_listCurves->setSelectionMode(QAbstractItemView::NoSelection);
        foldList->addRow(m_listCurves);
        m_mainLayout->addWidget(foldList);

        // --- 3. 显示设置 ---
        FoldSection* foldDisplay = new FoldSection("显示设置", this);

        m_editTimeStart = new MxFunctions::mCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard);
        m_editTimeStart->setPlaceholderText("0");
        foldDisplay->addRow(new QLabel("起始时间:"), m_editTimeStart);

        m_editTimeEnd = new MxFunctions::mCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard);
        m_editTimeEnd->setPlaceholderText("总时长");
        foldDisplay->addRow(new QLabel("结束时间:"), m_editTimeEnd);

        m_mainLayout->addWidget(foldDisplay);

        // --- 4. 导出设置 ---
        FoldSection* foldExport = new FoldSection("导出", this);
        m_spinExportInterval = new QSpinBox();
        m_spinExportInterval->setRange(0, 999999);
        m_spinExportInterval->setToolTip("0 表示全部导出；N 表示每隔 N 个点取一个");
        foldExport->addRow(new QLabel("间隔点:"), m_spinExportInterval);

        m_btnExport = new QPushButton("导出数据");
        QHBoxLayout* exportRow = new QHBoxLayout();
        exportRow->addStretch();
        exportRow->addWidget(m_btnExport);
        foldExport->addRow(exportRow);
        m_mainLayout->addWidget(foldExport);

        m_mainLayout->addStretch();

        // 信号只连接一次，保持始终连接
        connect(m_comboSourceType, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &mMotionCurvePanel::onSourceTypeChanged);
        connect(m_comboObject, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &mMotionCurvePanel::onUIChanged);
        // 传感器源的变量下拉随所选传感器实例变化（标量型/矢量型）
        connect(m_comboObject, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int) {
            if (static_cast<MechMotionData::CurveSourceType>(m_comboSourceType->currentData().toInt())
                == MechMotionData::CurveSourceType::Sensor) {
                const QSignalBlocker blockerVar(m_comboVarComp);
                populateVarCompCombo(MechMotionData::CurveSourceType::Sensor);
            }
        });
        connect(m_comboVarComp, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &mMotionCurvePanel::onUIChanged);
        connect(m_editCurveName, &QLineEdit::textChanged,
                this, &mMotionCurvePanel::onUIChanged);
        connect(m_btnAddCurve, &QPushButton::clicked, this, &mMotionCurvePanel::onAddCurve);
        connect(m_btnClearCurves, &QPushButton::clicked, this, &mMotionCurvePanel::onClearCurves);
        connect(m_btnExport, &QPushButton::clicked, this, &mMotionCurvePanel::onExportCurve);
        connect(m_editTimeStart, &QLineEdit::editingFinished, this, &mMotionCurvePanel::onTimeRangeChanged);
        connect(m_editTimeEnd, &QLineEdit::editingFinished, this, &mMotionCurvePanel::onTimeRangeChanged);
    }

    MechMotionData::mMotionScenario* mMotionCurvePanel::getScenario() const
    {
        if (!getCVM || !getCVM->getModelData() || m_scenarioId < 0)
            return nullptr;
        return getCVM->getModelData()->getManager<MechMotionData::mMotionScenarioManager>(
            Physical_MotionScenario).getData(m_scenarioId);
    }

    void mMotionCurvePanel::newMotionCurvePanel()
    {
        setEditing(true);
        m_scenarioId = -1;

        const QSignalBlocker blockerSrc(m_comboSourceType);
        const QSignalBlocker blockerObj(m_comboObject);
        const QSignalBlocker blockerVar(m_comboVarComp);
        const QSignalBlocker blockerName(m_editCurveName);

        m_comboSourceType->setCurrentIndex(0);
        populateObjectCombo(MechMotionData::CurveSourceType::Marker);
        populateVarCompCombo(MechMotionData::CurveSourceType::Marker);
        m_editCurveName->clear();

        m_listCurves->clear();
        m_editTimeStart->clear();
        m_editTimeEnd->clear();
    }

    void mMotionCurvePanel::editMotionCurvePanel(int scenarioId)
    {
        setEditing(true);
        m_scenarioId = scenarioId;

        auto* scenario = getScenario();
        if (!scenario) return;

        // 默认时间范围为结果全程
        if (scenario->hasResult() && scenario->getResult()->getFrameCount() > 0) {
            const QSignalBlocker blockerStart(m_editTimeStart);
            const QSignalBlocker blockerEnd(m_editTimeEnd);
            m_editTimeStart->setText("0");
            m_editTimeEnd->setText(QString::number(scenario->getResult()->getTotalTime(), 'g', 12));
        }

        if (scenario->hasCurrentCurveRequest()) {
            applyRequestToUI(scenario->getCurrentCurveRequest());
        } else {
            const QSignalBlocker blockerObj(m_comboObject);
            const QSignalBlocker blockerVar(m_comboVarComp);
            onSourceTypeChanged(m_comboSourceType->currentIndex());
        }

        rebuildCurveList();
        // 切换后自动显示该方案的曲线列表
        refreshPlotCurves();
    }

    void mMotionCurvePanel::uneditMotionCurvePanel()
    {
        if (!isEditing())
            return;
        setEditing(false);
        m_scenarioId = -1;
        m_listCurves->clear();

        // 清空轴标题（曲线窗内容保留，动画面板打开时会按方案重新显示）
        auto* plotView = mCurveWindow::getInstance()->getPlotView();
        if (plotView) {
            PlotView::AxisSettings xAxis = plotView->getAxisSettings(PlotView::X_Bottom);
            xAxis.title = QStringLiteral("");
            plotView->setAxisSettings(PlotView::X_Bottom, xAxis);

            PlotView::AxisSettings yAxis = plotView->getAxisSettings(PlotView::Y_Left);
            yAxis.title = QStringLiteral("");
            plotView->setAxisSettings(PlotView::Y_Left, yAxis);
        }
    }

    void mMotionCurvePanel::updateUIFromData()
    {
        auto* scenario = getScenario();
        if (!scenario) return;

        if (scenario->hasCurrentCurveRequest()) {
            const QSignalBlocker blockerSrc(m_comboSourceType);
            const QSignalBlocker blockerObj(m_comboObject);
            const QSignalBlocker blockerVar(m_comboVarComp);
            const QSignalBlocker blockerName(m_editCurveName);
            applyRequestToUI(scenario->getCurrentCurveRequest());
        }
    }

    void mMotionCurvePanel::populateSourceTypeCombo()
    {
        m_comboSourceType->clear();
        m_comboSourceType->addItem("标记点", static_cast<int>(MechMotionData::CurveSourceType::Marker));
        m_comboSourceType->addItem("运动体", static_cast<int>(MechMotionData::CurveSourceType::Body));
        m_comboSourceType->addItem("运动副", static_cast<int>(MechMotionData::CurveSourceType::Joint));
        m_comboSourceType->addItem("耦合副", static_cast<int>(MechMotionData::CurveSourceType::Coupling));
        m_comboSourceType->addItem("驱动",   static_cast<int>(MechMotionData::CurveSourceType::Drive));
        m_comboSourceType->addItem("传感器", static_cast<int>(MechMotionData::CurveSourceType::Sensor));
        m_comboSourceType->addItem("控制器", static_cast<int>(MechMotionData::CurveSourceType::Controller));
    }

    void mMotionCurvePanel::populateObjectCombo(MechMotionData::CurveSourceType type)
    {
        m_comboObject->clear();
        auto* modelData = getCVM->getModelData();

        switch (type) {
        case MechMotionData::CurveSourceType::Marker: {
            auto& mgr = modelData->getManager<MechMotionData::mMotionMarkerManager>(Physical_MotionMarker);
            for (int id : mgr.getAllIDs()) {
                auto* data = mgr.getData(id);
                if (data) m_comboObject->addItem(data->getName(), id);
            }
            break;
        }
        case MechMotionData::CurveSourceType::Body: {
            auto& mgr = modelData->getManager<MechMotionData::mMotionBodyManager>(Physical_MotionBody);
            for (int id : mgr.getAllIDs()) {
                auto* data = mgr.getData(id);
                if (data) m_comboObject->addItem(data->getName(), id);
            }
            break;
        }
        case MechMotionData::CurveSourceType::Joint: {
            auto& mgr = modelData->getManager<MechMotionData::mMotionJointManager>(Physical_MotionJoint);
            for (int id : mgr.getAllIDs()) {
                auto* data = mgr.getData(id);
                if (data) m_comboObject->addItem(data->getName(), id);
            }
            break;
        }
        case MechMotionData::CurveSourceType::Coupling: {
            auto& mgr = modelData->getManager<MechMotionData::mMotionCouplingManager>(Physical_MotionCoupling);
            for (int id : mgr.getAllIDs()) {
                auto* data = mgr.getData(id);
                if (data) m_comboObject->addItem(data->getName(), id);
            }
            break;
        }
        case MechMotionData::CurveSourceType::Drive: {
            auto& mgr = modelData->getManager<MechMotionData::mMotionDriveManager>(Physical_MotionDrive);
            for (int id : mgr.getAllIDs()) {
                auto* data = mgr.getData(id);
                if (data) m_comboObject->addItem(data->getName(), id);
            }
            break;
        }
        case MechMotionData::CurveSourceType::Sensor: {
            auto& mgr = modelData->getManager<MechMotionData::mMotionSensorManager>(Physical_MotionSensor);
            for (int id : mgr.getAllIDs()) {
                auto* data = mgr.getData(id);
                if (data && data->isEnabled()) m_comboObject->addItem(data->getName(), id);
            }
            break;
        }
        case MechMotionData::CurveSourceType::Controller: {
            auto& mgr = modelData->getManager<MechMotionData::mMotionControllerManager>(Physical_MotionController);
            for (int id : mgr.getAllIDs()) {
                auto* data = mgr.getData(id);
                if (data && data->isEnabled()) m_comboObject->addItem(data->getName(), id);
            }
            break;
        }
        }
    }

    void mMotionCurvePanel::populateVarCompCombo(MechMotionData::CurveSourceType type)
    {
        m_comboVarComp->clear();

        auto addVC = [this](MechMotionData::CurveVariable var, MechMotionData::CurveComponent comp, const QString& label) {
            QPair<int, int> pair(static_cast<int>(var), static_cast<int>(comp));
            m_comboVarComp->addItem(label, QVariant::fromValue(pair));
        };

        auto addVecVars = [&](const QString& base, MechMotionData::CurveVariable var) {
            addVC(var, MechMotionData::CurveComponent::X, base + " X");
            addVC(var, MechMotionData::CurveComponent::Y, base + " Y");
            addVC(var, MechMotionData::CurveComponent::Z, base + " Z");
            addVC(var, MechMotionData::CurveComponent::Magnitude, base + " 幅值");
        };

        switch (type) {
        case MechMotionData::CurveSourceType::Marker:
            addVecVars("位置", MechMotionData::CurveVariable::Position);
            addVecVars("速度", MechMotionData::CurveVariable::Velocity);
            addVecVars("加速度", MechMotionData::CurveVariable::Acceleration);
            addVecVars("角度", MechMotionData::CurveVariable::Rotation);
            addVecVars("角速度", MechMotionData::CurveVariable::AngularVelocity);
            addVecVars("角加速度", MechMotionData::CurveVariable::AngularAcceleration);
            break;

        case MechMotionData::CurveSourceType::Body:
            addVecVars("位置", MechMotionData::CurveVariable::Position);
            addVecVars("速度", MechMotionData::CurveVariable::Velocity);
            addVecVars("加速度", MechMotionData::CurveVariable::Acceleration);
            addVecVars("角度", MechMotionData::CurveVariable::Rotation);
            addVecVars("角速度", MechMotionData::CurveVariable::AngularVelocity);
            addVecVars("角加速度", MechMotionData::CurveVariable::AngularAcceleration);
            addVecVars("位移增量", MechMotionData::CurveVariable::Displacement);
            addVC(MechMotionData::CurveVariable::PotentialEnergy, MechMotionData::CurveComponent::Magnitude, "势能");
            addVC(MechMotionData::CurveVariable::KineticEnergy, MechMotionData::CurveComponent::Magnitude, "动能");
            break;

        case MechMotionData::CurveSourceType::Joint:
            addVecVars("节点I 位置", MechMotionData::CurveVariable::Position);
            addVecVars("节点I 速度", MechMotionData::CurveVariable::Velocity);
            addVecVars("节点I 加速度", MechMotionData::CurveVariable::Acceleration);
            addVecVars("节点I 角度", MechMotionData::CurveVariable::Rotation);
            addVecVars("节点I 角速度", MechMotionData::CurveVariable::AngularVelocity);
            addVecVars("节点I 角加速度", MechMotionData::CurveVariable::AngularAcceleration);
            addVecVars("约束反力", MechMotionData::CurveVariable::ReactionForce);
            addVecVars("约束反力矩", MechMotionData::CurveVariable::ReactionTorque);
            break;

        case MechMotionData::CurveSourceType::Coupling:
            addVecVars("节点I 位置", MechMotionData::CurveVariable::Position);
            addVecVars("节点I 速度", MechMotionData::CurveVariable::Velocity);
            addVecVars("节点I 加速度", MechMotionData::CurveVariable::Acceleration);
            addVecVars("节点I 角度", MechMotionData::CurveVariable::Rotation);
            addVecVars("节点I 角速度", MechMotionData::CurveVariable::AngularVelocity);
            addVecVars("节点I 角加速度", MechMotionData::CurveVariable::AngularAcceleration);
            addVecVars("约束反力", MechMotionData::CurveVariable::ReactionForce);
            addVecVars("约束反力矩", MechMotionData::CurveVariable::ReactionTorque);
            break;

        case MechMotionData::CurveSourceType::Drive:
            addVecVars("绝对位置", MechMotionData::CurveVariable::Position);
            addVecVars("绝对速度", MechMotionData::CurveVariable::Velocity);
            addVecVars("绝对加速度", MechMotionData::CurveVariable::Acceleration);
            addVecVars("绝对角度", MechMotionData::CurveVariable::Rotation);
            addVC(MechMotionData::CurveVariable::ImposedValue, MechMotionData::CurveComponent::Magnitude, "驱动位移");
            addVC(MechMotionData::CurveVariable::ImposedSpeed, MechMotionData::CurveComponent::Magnitude, "驱动速度");
            addVC(MechMotionData::CurveVariable::ImposedAcc, MechMotionData::CurveComponent::Magnitude, "驱动加速度");
            addVecVars("驱动力", MechMotionData::CurveVariable::ActuationForce);
            addVecVars("驱动力矩", MechMotionData::CurveVariable::ActuationTorque);
            addVecVars("约束反力", MechMotionData::CurveVariable::ReactionForce);
            addVecVars("约束反力矩", MechMotionData::CurveVariable::ReactionTorque);
            break;

        case MechMotionData::CurveSourceType::Sensor: {
            // 按当前所选传感器实例区分输出形态：标量型仅"传感器值"，矢量型提供分量
            if (sensorIsScalar(findSensor(m_comboObject->currentData().toInt()))) {
                addVC(MechMotionData::CurveVariable::SensorValue,
                    MechMotionData::CurveComponent::Magnitude, "传感器值");
            }
            else {
                addVC(MechMotionData::CurveVariable::SensorValue, MechMotionData::CurveComponent::X, "传感器值 X");
                addVC(MechMotionData::CurveVariable::SensorValue, MechMotionData::CurveComponent::Y, "传感器值 Y");
                addVC(MechMotionData::CurveVariable::SensorValue, MechMotionData::CurveComponent::Z, "传感器值 Z");
                addVC(MechMotionData::CurveVariable::SensorValue, MechMotionData::CurveComponent::Magnitude, "传感器值 幅值");
            }
            break;
        }
        case MechMotionData::CurveSourceType::Controller: {
            // PID 历史为标量序列：目标值/反馈值/误差/输出
            addVC(MechMotionData::CurveVariable::PidTarget,   MechMotionData::CurveComponent::Magnitude, "目标值");
            addVC(MechMotionData::CurveVariable::PidFeedback, MechMotionData::CurveComponent::Magnitude, "反馈值");
            addVC(MechMotionData::CurveVariable::PidError,    MechMotionData::CurveComponent::Magnitude, "误差");
            addVC(MechMotionData::CurveVariable::PidOutput,   MechMotionData::CurveComponent::Magnitude, "输出");
            break;
        }
        }
    }

    void mMotionCurvePanel::onSourceTypeChanged(int index)
    {
        if (index < 0) return;
        int typeInt = m_comboSourceType->currentData().toInt();
        auto type = static_cast<MechMotionData::CurveSourceType>(typeInt);
        {
            const QSignalBlocker blockerObj(m_comboObject);
            const QSignalBlocker blockerVar(m_comboVarComp);
            populateObjectCombo(type);
            populateVarCompCombo(type);
        }
        onUIChanged();
    }

    int mMotionCurvePanel::getSelectedObjectId() const
    {
        return m_comboObject->currentData().toInt();
    }

    QString mMotionCurvePanel::getSelectedObjectName() const
    {
        return m_comboObject->currentText();
    }

    mMotionCurvePanel::VarCompItem mMotionCurvePanel::getSelectedVarComp() const
    {
        QPair<int, int> pair = m_comboVarComp->currentData().value<QPair<int, int>>();
        VarCompItem vc;
        vc.variable = static_cast<MechMotionData::CurveVariable>(pair.first);
        vc.component = static_cast<MechMotionData::CurveComponent>(pair.second);
        vc.label = m_comboVarComp->currentText();
        return vc;
    }

    QString mMotionCurvePanel::requestTitle(const MechMotionData::mMotionCurveRequest& req)
    {
        return req.m_requestName.isEmpty() ? req.getFullLabel() : req.m_requestName;
    }

    QString mMotionCurvePanel::uniqueCurveTitle(const QString& base) const
    {
        auto* scenario = getScenario();
        if (!scenario) return base;

        auto titleUsed = [scenario](const QString& title) {
            for (const auto& item : scenario->getCurveRequests()) {
                if (requestTitle(item) == title)
                    return true;
            }
            return false;
        };

        QString title = base;
        int suffix = 2;
        while (titleUsed(title)) {
            title = QString("%1 (%2)").arg(base).arg(suffix);
            ++suffix;
        }
        return title;
    }

    QColor mMotionCurvePanel::autoColorFor(int index)
    {
        const auto& palette = PlotView::PlotColor;
        if (palette.isEmpty())
            return QColor(Qt::red);
        return palette.value(index % palette.size());
    }

    void mMotionCurvePanel::rebuildCurveList()
    {
        m_listCurves->clear();

        auto* scenario = getScenario();
        if (!scenario) return;

        const auto& requests = scenario->getCurveRequests();
        for (int i = 0; i < static_cast<int>(requests.size()); ++i) {
            auto* item = new QListWidgetItem(m_listCurves);
            item->setData(Qt::UserRole, requests[i].m_requestId);
            QWidget* row = createCurveRowWidget(requests[i], i);
            item->setSizeHint(row->sizeHint());
            m_listCurves->addItem(item);
            m_listCurves->setItemWidget(item, row);
        }
    }

    QWidget* mMotionCurvePanel::createCurveRowWidget(const MechMotionData::mMotionCurveRequest& req, int rowIndex)
    {
        QWidget* row = new QWidget(m_listCurves);
        QHBoxLayout* layout = new QHBoxLayout(row);
        layout->setContentsMargins(4, 2, 4, 2);
        layout->setSpacing(6);

        const int requestId = req.m_requestId;
        const QColor initColor = req.m_color.isValid() ? req.m_color : autoColorFor(rowIndex);

        // 色块按钮：打开取色对话框
        QPushButton* colorBtn = new QPushButton(row);
        colorBtn->setFixedSize(20, 20);
        auto styleColorBtn = [colorBtn](const QColor& c) {
            colorBtn->setStyleSheet(QString("QPushButton { background-color: %1; border: 1px solid #444; }")
                .arg(c.name()));
        };
        styleColorBtn(initColor);
        connect(colorBtn, &QPushButton::clicked, this, [this, requestId, styleColorBtn]() {
            auto* scenario = getScenario();
            const MechMotionData::mMotionCurveRequest* cur = findRequest(scenario, requestId);
            if (!scenario || !cur) return;

            const QColor picked = QColorDialog::getColor(
                cur->m_color.isValid() ? cur->m_color : QColor(Qt::white), this, tr("选择曲线颜色"));
            if (!picked.isValid()) return;

            MechMotionData::mMotionCurveRequest updated = *cur;
            updated.m_color = picked;
            scenario->updateCurveRequest(updated);
            styleColorBtn(picked);
            refreshPlotCurves();
        });

        // 曲线名称
        QLabel* nameLabel = new QLabel(requestTitle(req), row);
        nameLabel->setToolTip(requestTitle(req));

        // 可见性
        QCheckBox* visibleChk = new QCheckBox("显示", row);
        visibleChk->setChecked(req.m_visible);
        connect(visibleChk, &QCheckBox::toggled, this, [this, requestId](bool visible) {
            auto* scenario = getScenario();
            const MechMotionData::mMotionCurveRequest* cur = findRequest(scenario, requestId);
            if (!scenario || !cur) return;

            MechMotionData::mMotionCurveRequest updated = *cur;
            updated.m_visible = visible;
            scenario->updateCurveRequest(updated);
            refreshPlotCurves();
        });

        // 删除
        QPushButton* removeBtn = new QPushButton("删除", row);
        removeBtn->setFlat(true);
        connect(removeBtn, &QPushButton::clicked, this, [this, requestId]() {
            auto* scenario = getScenario();
            if (!scenario) return;
            scenario->removeCurveRequest(requestId);
            rebuildCurveList();
            refreshPlotCurves();
        });

        layout->addWidget(colorBtn);
        layout->addWidget(nameLabel, 1);
        layout->addWidget(visibleChk);
        layout->addWidget(removeBtn);
        return row;
    }

    bool mMotionCurvePanel::timeWindow(double& start, double& end) const
    {
        bool startOk = false, endOk = false;
        const double s = m_editTimeStart->text().trimmed().toDouble(&startOk);
        const double e = m_editTimeEnd->text().trimmed().toDouble(&endOk);
        if (!startOk && !endOk)
            return false;
        start = startOk ? s : 0.0;
        end = endOk ? e : std::numeric_limits<double>::infinity();
        return true;
    }

    mMotionCurvePanel::VisibleSeries mMotionCurvePanel::collectSeries(
        const MechMotionData::mMotionScenario* scenario)
    {
        VisibleSeries out;
        if (!scenario || !scenario->hasResult())
            return out;

        int colorIndex = 0;
        for (const auto& req : scenario->getCurveRequests()) {
            if (!req.m_visible)
                continue;

            mMotionResultQueryService::ResultSeries series;
            if (req.m_sourceType == MechMotionData::CurveSourceType::Sensor) {
                const MechMotionData::mMotionSensorBase* sensor = findSensor(req.m_sourceId);
                if (!sensor || !sensor->isEnabled())
                    continue;
                series = mMotionResultQueryService::getSensorSeries(
                    scenario->getResult(), sensor, req.m_component);
            }
            else if (req.m_sourceType == MechMotionData::CurveSourceType::Controller) {
                const MechMotionData::mMotionControllerBase* controller = findController(req.m_sourceId);
                if (!controller || !controller->isEnabled())
                    continue;
                series = mMotionResultQueryService::getControllerSeries(
                    scenario->getResult(), req.m_sourceId, req.m_variable);
            }
            else {
                series = mMotionResultQueryService::getResultSeries(scenario->getResult(), req);
            }
            if (!series.isValid())
                continue;

            QVector<double> times, values;
            times.reserve(series.times.size());
            values.reserve(series.values.size());
            for (int i = 0; i < qMin(series.times.size(), series.values.size()); ++i) {
                times.push_back(series.times[i]);
                values.push_back(series.values[i]);
            }
            if (times.isEmpty())
                continue;

            out.xs.push_back(times);
            out.ys.push_back(values);
            out.titles.push_back(requestTitle(req));
            out.colors.push_back(req.m_color.isValid() ? req.m_color : autoColorFor(colorIndex));
            ++colorIndex;
        }
        return out;
    }

    mMotionCurvePanel::VisibleSeries mMotionCurvePanel::collectVisibleSeries() const
    {
        VisibleSeries out = collectSeries(getScenario());
        if (!out.isValid())
            return out;

        double start = 0.0, end = 0.0;
        if (!timeWindow(start, end))
            return out;

        // 时间范围过滤
        VisibleSeries filtered;
        for (int c = 0; c < out.xs.size(); ++c) {
            QVector<double> times, values;
            for (int i = 0; i < out.xs[c].size(); ++i) {
                if (out.xs[c][i] < start || out.xs[c][i] > end)
                    continue;
                times.push_back(out.xs[c][i]);
                values.push_back(out.ys[c][i]);
            }
            if (times.isEmpty())
                continue;

            filtered.xs.push_back(times);
            filtered.ys.push_back(values);
            filtered.titles.push_back(out.titles[c]);
            filtered.colors.push_back(out.colors[c]);
        }
        return filtered;
    }

    bool mMotionCurvePanel::applySeriesToPlot(const mMotionCurvePanel::VisibleSeries& series)
    {
        if (!series.isValid())
            return false;

        auto* curveWin = mCurveWindow::getInstance();
        curveWin->updateMultiCurves(series.xs, series.ys, PlotView::LineChart, series.titles, true, true);

        auto* plotView = curveWin->getPlotView();
        if (plotView) {
            // 应用每条曲线的存储颜色
            for (int i = 0; i < series.titles.size(); ++i) {
                PlotView::CurveSettings settings = plotView->getCurveSettings(series.titles[i]);
                settings.lineColor = series.colors[i];
                settings.symbolColor = series.colors[i];
                plotView->setCurveStyle(series.titles[i], settings);
            }

            PlotView::AxisSettings xAxis = plotView->getAxisSettings(PlotView::X_Bottom);
            xAxis.title = QStringLiteral("时间(s)");
            plotView->setAxisSettings(PlotView::X_Bottom, xAxis);

            PlotView::AxisSettings yAxis = plotView->getAxisSettings(PlotView::Y_Left);
            yAxis.title = series.titles.size() == 1 ? series.titles.first() : QStringLiteral("数值");
            plotView->setAxisSettings(PlotView::Y_Left, yAxis);

            plotView->rescaleAxes();
        }

        mDockLayoutManager::getInstance()->activateDock("Curve");
        return true;
    }

    void mMotionCurvePanel::refreshPlotCurves()
    {
        auto* scenario = getScenario();
        const auto visible = collectVisibleSeries();

        if (!scenario || !scenario->hasResult() || !visible.isValid()) {
            mCurveWindow::getInstance()->clearAllPlot();
            return;
        }

        applySeriesToPlot(visible);
    }

    bool mMotionCurvePanel::displayScenarioCurves(int scenarioId)
    {
        if (!getCVM || !getCVM->getModelData())
            return false;

        auto* scenario = getCVM->getModelData()->getManager<MechMotionData::mMotionScenarioManager>(
            Physical_MotionScenario).getData(scenarioId);

        const auto series = collectSeries(scenario);
        if (!scenario || !scenario->hasResult() || !series.isValid()) {
            // 该方案没有可显示的曲线配置，清掉曲线窗避免残留其他方案的数据
            mCurveWindow::getInstance()->clearAllPlot();
            return false;
        }

        return applySeriesToPlot(series);
    }

    void mMotionCurvePanel::onAddCurve()
    {
        if (m_comboObject->count() == 0 || m_comboVarComp->count() == 0) return;

        auto* scenario = getScenario();
        if (!scenario) {
            //面板未正确进入某方案的编辑（如首次点击"曲线"节点时未完成编辑装配），
            //显式提示而不是静默返回，便于暴露装配类问题
            QMessageBox::warning(this, tr("提示"), tr("未找到当前求解方案，无法添加曲线。"));
            return;
        }

        if (!scenario->hasResult()) {
            QMessageBox::warning(this, tr("提示"), tr("当前方案没有求解结果，无法添加曲线。"));
            return;
        }

        MechMotionData::mMotionCurveRequest req = collectRequestFromUI();
        if (req.m_sourceId < 0) {
            QMessageBox::warning(this, tr("提示"), tr("请选择有效的曲线来源对象。"));
            return;
        }

        // 传感器源需解析到有效且启用的传感器定义（无传感器时下拉为空，sourceId 会取到 0）
        if (req.m_sourceType == MechMotionData::CurveSourceType::Sensor) {
            const MechMotionData::mMotionSensorBase* sensor = findSensor(req.m_sourceId);
            if (!sensor || !sensor->isEnabled()) {
                QMessageBox::warning(this, tr("提示"), tr("请选择有效的传感器。"));
                return;
            }
        }

        if (req.m_sourceType == MechMotionData::CurveSourceType::Controller) {
            const MechMotionData::mMotionControllerBase* controller = findController(req.m_sourceId);
            if (!controller || !controller->isEnabled()) {
                QMessageBox::warning(this, tr("提示"), tr("请选择有效的控制器。"));
                return;
            }
        }

        // 同源同变量同分量去重
        for (const auto& item : scenario->getCurveRequests()) {
            if (item.m_sourceType == req.m_sourceType && item.m_sourceId == req.m_sourceId &&
                item.m_variable == req.m_variable && item.m_component == req.m_component) {
                QMessageBox::information(this, tr("提示"), tr("该曲线已在列表中。"));
                return;
            }
        }

        const QString base = req.m_requestName.isEmpty() ? req.getFullLabel() : req.m_requestName;
        req.m_requestName = uniqueCurveTitle(base);
        req.m_visible = true;
        req.m_color = autoColorFor(static_cast<int>(scenario->getCurveRequests().size()));

        scenario->addCurveRequest(req);
        rebuildCurveList();
        refreshPlotCurves();
    }

    void mMotionCurvePanel::onClearCurves()
    {
        auto* scenario = getScenario();
        if (!scenario) return;

        scenario->clearCurveRequests();
        rebuildCurveList();
        refreshPlotCurves();
    }

    void mMotionCurvePanel::onExportCurve()
    {
        auto* scenario = getScenario();
        if (!scenario || !scenario->hasResult()) {
            return;
        }

        const auto visible = collectVisibleSeries();
        if (!visible.isValid()) {
            QMessageBox::warning(this, tr("提示"), tr("曲线列表为空或没有可见曲线，无法导出。"));
            return;
        }

        // 默认目录
        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_recentDir;
        QFileInfo info(defaultDir);
        if (defaultDir.isEmpty() || !info.exists())
        {
            defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
        }

        QString filesuffix = tr("CSV(*.csv);;TXT(*.txt)");
        QString fileName = QFileDialog::getSaveFileName(
            this,
            tr("导出曲线数据"),
            defaultDir,
            filesuffix
        );

        if (fileName.isEmpty()) return;
        MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = QFileInfo(fileName).absolutePath();

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return;

        QTextStream out(&file);
        out.setCodec("UTF-8");

        // 表头：时间 + 每条可见曲线一列
        out << QStringLiteral("时间(s)");
        for (const QString& title : visible.titles) {
            out << "," << title;
        }
        out << "\n";

        // 以第一条曲线的时间轴为主轴（各曲线同源于求解帧）
        const QVector<double>& master = visible.xs.first();
        const int curveCount = visible.xs.size();
        const int count = master.size();
        if (count > 0) {
            int interval = m_spinExportInterval->value();
            auto writeRow = [&](int i) {
                out << QString::number(master[i], 'g', 15);
                for (int c = 0; c < curveCount; ++c) {
                    const double value = valueAtTime(visible.xs[c], visible.ys[c], master[i]);
                    out << "," << QString::number(value, 'g', 15);
                }
                out << "\n";
            };

            if (interval <= 0) {
                for (int i = 0; i < count; ++i)
                    writeRow(i);
            }
            else {
                for (int i = 0; i < count; i += (interval + 1))
                    writeRow(i);
                const int lastIdx = count - 1;
                if ((lastIdx % (interval + 1)) != 0)
                    writeRow(lastIdx);
            }
        }

        file.close();
    }

    void mMotionCurvePanel::onTimeRangeChanged()
    {
        if (m_scenarioId < 0) return;
        refreshPlotCurves();
    }

    void mMotionCurvePanel::onUIChanged()
    {
        auto* scenario = getScenario();
        if (!scenario) return;

        scenario->setCurrentCurveRequest(collectRequestFromUI());
    }

    void mMotionCurvePanel::applyRequestToUI(const MechMotionData::mMotionCurveRequest& req)
    {
        const QSignalBlocker blockerSrc(m_comboSourceType);
        const QSignalBlocker blockerObj(m_comboObject);
        const QSignalBlocker blockerVar(m_comboVarComp);
        const QSignalBlocker blockerName(m_editCurveName);

        // 恢复源类型
        int typeIdx = m_comboSourceType->findData(static_cast<int>(req.m_sourceType));
        if (typeIdx >= 0) {
            m_comboSourceType->setCurrentIndex(typeIdx);
            onSourceTypeChanged(typeIdx);
        }

        // 恢复对象
        if (req.m_sourceId >= 0) {
            int objIdx = m_comboObject->findData(req.m_sourceId);
            if (objIdx >= 0)
                m_comboObject->setCurrentIndex(objIdx);
        }

        // 传感器源的变量下拉依赖所选传感器实例，恢复对象后重建
        if (req.m_sourceType == MechMotionData::CurveSourceType::Sensor) {
            populateVarCompCombo(MechMotionData::CurveSourceType::Sensor);
        }

        // 恢复变量-分量
        QPair<int, int> target(static_cast<int>(req.m_variable), static_cast<int>(req.m_component));
        for (int i = 0; i < m_comboVarComp->count(); ++i) {
            QPair<int, int> p = m_comboVarComp->itemData(i).value<QPair<int, int>>();
            if (p == target) {
                m_comboVarComp->setCurrentIndex(i);
                break;
            }
        }

        // 恢复曲线名称
        m_editCurveName->setText(req.m_requestName);
    }

    MechMotionData::mMotionCurveRequest mMotionCurvePanel::collectRequestFromUI() const
    {
        MechMotionData::mMotionCurveRequest req;
        req.m_sourceType = static_cast<MechMotionData::CurveSourceType>(m_comboSourceType->currentData().toInt());
        req.m_sourceId = getSelectedObjectId();
        req.m_sourceName = getSelectedObjectName();
        auto vc = getSelectedVarComp();
        req.m_variable = vc.variable;
        req.m_component = vc.component;
        req.m_requestName = m_editCurveName->text().trimmed();
        return req;
    }

}
