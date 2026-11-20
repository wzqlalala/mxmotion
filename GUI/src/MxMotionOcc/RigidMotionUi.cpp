#include "MotionWindow.h"
#include "OccViewport.h"
#include "RigidPlot.h"
#include "RigidProject.h"
#include "ThemeManager.h"
#include "mUiMenu.h"

#include <DockAreaWidget.h>
#include <DockManager.h>
#include <DockWidget.h>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMatrix4x4>
#include <QProgressBar>
#include <QPushButton>
#include <QQuaternion>
#include <QSignalBlocker>
#include <QSlider>
#include <QTimer>
#include <QTreeWidget>
#include <algorithm>
#include <cmath>
#include <exception>

namespace MotionOcc {
namespace {
constexpr int idRole = Qt::UserRole;
constexpr int kindRole = Qt::UserRole + 1;
constexpr int ticks = 10000;
using MechMainWindow::ThemeManager;
using MxFunctions::mUiMenu;

QIcon motionIcon(const char* file)
{
    return QIcon(QStringLiteral(":/mMainWidgets/Icon/Motion/") + QString::fromLatin1(file));
}

template<class T> const T* byId(const std::vector<T>& list, int id)
{
    const auto it = std::find_if(list.begin(), list.end(), [id](const auto& x) { return x.id == id; });
    return it == list.end() ? nullptr : &*it;
}

template<class T> int nextId(const std::vector<T>& list)
{
    for (int id = 1; id < 1000000; ++id) if (!byId(list, id)) return id;
    return 0;
}

template<class T> void replace(std::vector<T>& list, const T& item)
{
    const auto it = std::find_if(list.begin(), list.end(), [&](const T& x) { return x.id == item.id; });
    if (it == list.end()) list.push_back(item);
    else *it = item;
}

template<class T, class F> void eraseIf(std::vector<T>& list, F predicate)
{
    list.erase(std::remove_if(list.begin(), list.end(), predicate), list.end());
}

QString jointLabel(JointKind kind)
{
    switch (kind) {
    case JointKind::Revolute: return QStringLiteral("旋转副");
    case JointKind::Prismatic: return QStringLiteral("移动副");
    case JointKind::Fixed: return QStringLiteral("固定副");
    }
    return {};
}

QDoubleSpinBox* number(QFormLayout* form, const QString& name, double value,
                       double min = -1e8, double max = 1e8, int decimals = 6)
{
    auto* spin = new QDoubleSpinBox;
    spin->setDecimals(decimals);
    spin->setRange(min, max);
    spin->setKeyboardTracking(false);
    spin->setValue(value);
    form->addRow(name, spin);
    return spin;
}

std::array<QDoubleSpinBox*, 3> vectorFields(QFormLayout* form, const QString& name, const Vector3d& value)
{
    std::array<QDoubleSpinBox*, 3> fields{};
    for (int axis = 0; axis < 3; ++axis)
        fields[axis] = number(form, name + QLatin1Char(' ') + QLatin1Char('X' + axis), value[axis]);
    return fields;
}

Vector3d vectorValue(const std::array<QDoubleSpinBox*, 3>& fields)
{
    return {fields[0]->value(), fields[1]->value(), fields[2]->value()};
}

QVector3D vector3(const Vector3d& x) { return QVector3D(float(x[0]), float(x[1]), float(x[2])); }
QQuaternion quaternion(const Quaterniond& x) { return QQuaternion(float(x[0]), float(x[1]), float(x[2]), float(x[3])).normalized(); }

void selectData(QComboBox* combo, int id)
{
    const int index = combo->findData(id);
    if (index >= 0) combo->setCurrentIndex(index);
}

void finishForm(QFormLayout* form, MotionWindow* owner, const std::function<void()>& apply, const std::function<void()>& cancel)
{
    auto* row = new QWidget;
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* yes = new QPushButton(QStringLiteral("确定"), row);
    yes->setObjectName(QStringLiteral("motionApplyButton"));
    auto* no = new QPushButton(QStringLiteral("取消"), row);
    layout->addWidget(yes);
    layout->addWidget(no);
    form->addRow(row);
    QObject::connect(yes, &QPushButton::clicked, owner, apply);
    QObject::connect(no, &QPushButton::clicked, owner, cancel);
}

void openDock(ads::CDockWidget* dock)
{
    dock->toggleView(true);
    if (auto* area = dock->dockAreaWidget()) area->setCurrentDockWidget(dock);
}
}

void MotionWindow::createRigidCommands()
{
    auto add = [this](Command id, const QString& text, const char* image, const std::function<void()>& fn) {
        auto* action = new QAction(motionIcon(image), text, this);
        m_commands.emplace(id, action);
        connect(action, &QAction::triggered, this, fn);
    };
    add(Command::AddBody, tr("运动体"), "movingBody.png", [this] { editBody(); });
    add(Command::BatchBodies, tr("一键生成运动体"), "bodyBatch.png", [this] { batchBodies(); });
    add(Command::AddRevolute, tr("旋转副"), "joint_revolute.png", [this] { editJoint(JointKind::Revolute); });
    add(Command::AddPrismatic, tr("移动副"), "joint_prismatic.png", [this] { editJoint(JointKind::Prismatic); });
    add(Command::AddFixed, tr("固定副"), "joint_fixed.png", [this] { editJoint(JointKind::Fixed); });
    add(Command::AddSpeedDrive, tr("速度驱动"), "drive.png", [this] { editDrive(DriveKind::Speed); });
    add(Command::AddPositionDrive, tr("位移驱动"), "drive.png", [this] { editDrive(DriveKind::Displacement); });
    add(Command::NewScenario, tr("新建求解方案"), "drive.png", [this] { editScenario(); });
    add(Command::Solve, tr("求解"), "solve.png", [this] {
        int id = m_model.scenarios.empty() ? 0 : m_model.scenarios.back().id;
        if (m_selectionNode.kind >= ModelNode::Scenario) id = m_selectionNode.id;
        if (id) startRigidSolve(id, true);
        else editScenario();
    });
    add(Command::StopSolve, tr("停止求解"), "solve.png", [this] {
        if (m_running) m_running->stop.store(true, std::memory_order_relaxed);
        updateActions();
    });
    add(Command::LoadRigidExample, tr("刚体示例"), "movingBody.png", [this] { loadRigidExample(); });
}

void MotionWindow::createRigidDocks(ads::CDockAreaWidget* bottom)
{
    m_rigidPlot = new RigidPlot;
    m_curveDock = addPane("Curve", tr("函数与曲线"), m_rigidPlot);
    m_docks->addDockWidgetTabToArea(m_curveDock, bottom);
    m_curveDock->closeDockWidget();
    auto* controls = new QWidget;
    auto* layout = new QHBoxLayout(controls);
    m_playToggle = new QPushButton(tr("播放"), controls);
    auto* stop = new QPushButton(tr("停止"), controls);
    m_timeline = new QSlider(Qt::Horizontal, controls);
    m_timeline->setObjectName(QStringLiteral("motionTimeline"));
    m_timeline->setRange(0, ticks);
    m_timeLabel = new QLabel(QStringLiteral("0.000 s"), controls);
    m_loopPlayback = new QCheckBox(tr("循环"), controls);
    layout->addWidget(m_playToggle);
    layout->addWidget(stop);
    layout->addWidget(m_timeline, 1);
    layout->addWidget(m_timeLabel);
    layout->addWidget(m_loopPlayback);
    m_animationDock = addPane("MotionAnimation", tr("动画"), controls);
    m_docks->addDockWidget(ads::RightDockWidgetArea, m_animationDock, bottom);
    m_animationDock->closeDockWidget();
    connect(m_playToggle, &QPushButton::clicked, this, [this] { setPlayback(!m_playbackTimer->isActive()); });
    connect(stop, &QPushButton::clicked, this, [this] { setPlayback(false); seekRigidTime(0); });
    connect(m_timeline, &QSlider::valueChanged, this, [this](int position) {
        const auto run = m_results.find(m_resultScenario);
        if (run == m_results.end()) return;
        setPlayback(false);
        seekRigidTime(run->second->result.frames.back().seconds * position / ticks);
    });
    connect(m_rigidPlot, &RigidPlot::timeRequested, this, [this](double time) { setPlayback(false); seekRigidTime(time); });
}

void MotionWindow::createRigidControls()
{
    m_rigidPoll = new QTimer(this);
    m_rigidPoll->setInterval(50);
    connect(m_rigidPoll, &QTimer::timeout, this, &MotionWindow::pollRigidSolve);
    m_playbackTimer = new QTimer(this);
    m_playbackTimer->setInterval(16);
    connect(m_playbackTimer, &QTimer::timeout, this, [this] {
        const auto run = m_results.find(m_resultScenario);
        if (run == m_results.end()) { setPlayback(false); return; }
        const auto end = run->second->result.frames.back().seconds;
        double time = m_playbackStart + m_playbackClock.elapsed() / 1000.0;
        if (time >= end) {
            if (m_loopPlayback->isChecked() && end > 0) time = std::fmod(time, end);
            else setPlayback(false);
        }
        seekRigidTime(time);
    });
    connect(m_animationDock, &ads::CDockWidget::closed, this, [this] {
        setPlayback(false);
        seekRigidTime(0);
    });
}

bool MotionWindow::motionBusy() const { return bool(m_running); }

bool MotionWindow::canRemovePart(int id) const
{
    return std::none_of(m_model.bodies.begin(), m_model.bodies.end(), [id](const auto& b) { return b.partId == id; });
}

void MotionWindow::updateRigidActions()
{
    const bool idle = !motionBusy() && m_jobPart < 0;
    const bool usablePart = std::any_of(m_document.parts().begin(), m_document.parts().end(), [&](const auto& p) { return canRemovePart(p.id); });
    const bool moving = std::any_of(m_model.bodies.begin(), m_model.bodies.end(), [](const auto& b) { return !b.fixed; });
    const bool canDrive = std::any_of(m_model.joints.begin(), m_model.joints.end(), [&](const auto& j) {
        return j.kind != JointKind::Fixed && std::none_of(m_model.drives.begin(), m_model.drives.end(), [&](const auto& d) { return d.jointId == j.id; });
    });
    for (const auto id : {Command::AddBody, Command::BatchBodies}) command(id)->setEnabled(idle && usablePart);
    for (const auto id : {Command::AddRevolute, Command::AddPrismatic, Command::AddFixed, Command::Solve})
        command(id)->setEnabled(idle && moving);
    command(Command::AddSpeedDrive)->setEnabled(idle && canDrive);
    command(Command::AddPositionDrive)->setEnabled(idle && canDrive);
    command(Command::NewScenario)->setEnabled(idle);
    command(Command::LoadRigidExample)->setEnabled(idle);
    command(Command::StopSolve)->setEnabled(m_running && !m_running->stop.load());
}

void MotionWindow::appendRigidNodes(QTreeWidgetItem* root, const std::map<QString, QTreeWidgetItem*>& groups)
{
    auto insert = [this](QTreeWidgetItem* parent, const QString& label, ModelNode::Kind kind, int id, const char* image) {
        auto* item = new QTreeWidgetItem(parent, {label});
        item->setIcon(0, motionIcon(image));
        item->setData(0, idRole, id);
        item->setData(0, kindRole, int(kind));
        parent->setExpanded(true);
        if (m_selectionNode.kind == kind && m_selectionNode.id == id) m_tree->setCurrentItem(item);
        return item;
    };
    for (const auto& b : m_model.bodies) insert(groups.at(tr("运动体")), b.name, ModelNode::Body, b.id, "movingBody.png");
    for (const auto& j : m_model.joints) insert(groups.at(tr("运动副")), j.name, ModelNode::Joint, j.id,
        j.kind == JointKind::Revolute ? "joint_revolute.png" : j.kind == JointKind::Prismatic ? "joint_prismatic.png" : "joint_fixed.png");
    for (const auto& d : m_model.drives) insert(groups.at(tr("驱动")), d.name, ModelNode::Drive, d.id, "drive.png");
    for (const auto& sc : m_model.scenarios) {
        auto* item = insert(root, sc.name, ModelNode::Scenario, sc.id, "solve.png");
        const bool ready = m_results.count(sc.id) != 0;
        for (const auto& x : {std::pair{tr("动画"), ModelNode::Animation}, {tr("结果导出"), ModelNode::Export}, {tr("运动曲线"), ModelNode::Curves}}) {
            auto* child = insert(item, x.first, x.second, sc.id, "solve.png");
            if (!ready) child->setDisabled(true);
        }
    }
}

void MotionWindow::selectRigidNode(ModelNode node)
{
    m_selectionNode = node;
    m_partId = -1;
    m_viewport->selectPart(-1);
    closeEditor();
    m_meshForm->hide();
    updateParameters();
    updateActions();
    if (motionBusy() || m_jobPart >= 0) return;
    switch (node.kind) {
    case ModelNode::Body: editBody(node.id); break;
    case ModelNode::Joint: if (const auto* j = byId(m_model.joints, node.id)) editJoint(j->kind, j->id); break;
    case ModelNode::Drive: if (const auto* d = byId(m_model.drives, node.id)) editDrive(d->kind, d->id); break;
    case ModelNode::Scenario: editScenario(node.id); break;
    case ModelNode::Animation:
    case ModelNode::Curves: showRigidResult(node); break;
    default: break;
    }
}

void MotionWindow::showRigidMenu(ModelNode node, const QPoint& globalPosition)
{
    mUiMenu menu(this);
    if (node.kind >= ModelNode::Animation) {
        const auto text = node.kind == ModelNode::Animation ? tr("动画") : node.kind == ModelNode::Curves ? tr("运动曲线") : tr("结果导出");
        auto* action = menu.addAction(text);
        action->setEnabled(m_results.count(node.id) > 0);
        connect(action, &QAction::triggered, this, [this, node] { showRigidResult(node); });
    } else {
        const bool idle = !motionBusy() && m_jobPart < 0;
        auto* edit = menu.addAction(tr("编辑"));
        edit->setEnabled(idle);
        connect(edit, &QAction::triggered, this, [this, node] { selectRigidNode(node); });
        if (node.kind == ModelNode::Scenario) {
            auto* solve = menu.addAction(tr("求解"));
            solve->setEnabled(idle);
            connect(solve, &QAction::triggered, this, [this, node] { startRigidSolve(node.id, true); });
        }
        auto* remove = menu.addAction(tr("删除"));
        remove->setEnabled(idle);
        connect(remove, &QAction::triggered, this, [this, node] { removeRigidNode(node); });
    }
    ThemeManager::applyMenuStyle(&menu);
    menu.exec(globalPosition);
}

void MotionWindow::showEditor(QWidget* editor)
{
    closeEditor();
    m_meshForm->hide();
    m_parameterTitle->hide();
    m_parameterDetails->hide();
    m_editor = editor;
    m_parameterLayout->insertWidget(m_parameterLayout->count() - 1, m_editor);
    openDock(m_parameterDock);
    m_editor->show();
}

void MotionWindow::closeEditor()
{
    if (!m_editor) return;
    m_editor->hide();
    m_parameterLayout->removeWidget(m_editor);
    m_editor->deleteLater();
    m_editor = nullptr;
    m_parameterTitle->show();
    m_parameterDetails->show();
}

void MotionWindow::editBody(int id)
{
    if (motionBusy() || m_jobPart >= 0) return;
    const auto* current = byId(m_model.bodies, id);
    auto* panel = new QWidget;
    auto* form = new QFormLayout(panel);
    form->addRow(new QLabel(tr("运动体 · 质量和惯性矩")));
    const int key = current ? current->id : nextId(m_model.bodies);
    auto* name = new QLineEdit(current ? current->name : tr("运动体-%1").arg(key));
    name->setMaxLength(128);
    form->addRow(tr("名称"), name);
    auto* part = new QComboBox;
    for (const auto& p : m_document.parts()) if (canRemovePart(p.id) || (current && current->partId == p.id)) part->addItem(p.name, p.id);
    if (part->count() == 0) { delete panel; delete part; setNotice(tr("没有可分配的 CAD 实体。"), true); return; }
    selectData(part, current ? current->partId : m_partId);
    form->addRow(tr("部件"), part);
    auto* fixed = new QCheckBox(tr("固定参考体"));
    fixed->setChecked(current && current->fixed);
    form->addRow(fixed);
    auto* source = new QComboBox;
    source->addItems({tr("指定质量"), tr("按密度计算")});
    source->setCurrentIndex(current && current->densityKgM3 > 0 ? 1 : 0);
    form->addRow(tr("质量来源"), source);
    auto* mass = number(form, tr("质量 (kg)"), current ? current->massKg : 1.0, 1e-8, 1e8, 8);
    auto* density = number(form, tr("密度 (kg/m³)"), current && current->densityKgM3 > 0 ? current->densityKgM3 : 7800, 1e-5, 1e8);
    auto toggle = [=](int index) { mass->setEnabled(index == 0); density->setEnabled(index == 1); };
    connect(source, qOverload<int>(&QComboBox::currentIndexChanged), panel, toggle);
    toggle(source->currentIndex());
    finishForm(form, this, [=] {
        RigidModel model = m_model;
        RigidBody body{key, name->text().trimmed(), part->currentData().toInt(), fixed->isChecked(), mass->value(),
                       source->currentIndex() == 1 ? density->value() : 0.0};
        BodyProperties properties;
        QString error;
        if (!rigidBodyProperties(m_document, body, properties, error)) { setNotice(error, true); return; }
        replace(model.bodies, body);
        commitRigid(std::move(model), {ModelNode::Body, key});
    }, [this] { closeEditor(); updateParameters(); });
    showEditor(panel);
}

void MotionWindow::batchBodies()
{
    if (motionBusy() || m_jobPart >= 0) return;
    RigidModel next = m_model;
    int added = 0, skipped = 0;
    for (const auto& p : m_document.parts()) {
        if (!canRemovePart(p.id)) continue;
        RigidBody body{nextId(next.bodies), p.name.left(128), p.id, false, 1, 0};
        BodyProperties properties;
        QString error;
        if (rigidBodyProperties(m_document, body, properties, error)) { next.bodies.push_back(body); ++added; }
        else ++skipped;
    }
    if (added) commitRigid(std::move(next), {});
    setNotice(tr("生成 %1 个运动体，跳过 %2 个非实体或无有效惯量的部件。").arg(added).arg(skipped), added == 0);
}

void MotionWindow::editJoint(JointKind kind, int id)
{
    if (motionBusy() || m_jobPart >= 0) return;
    const auto* current = byId(m_model.joints, id);
    const RigidJoint initial = current ? *current : RigidJoint{nextId(m_model.joints), {}, kind};
    auto* panel = new QWidget;
    auto* form = new QFormLayout(panel);
    form->addRow(new QLabel(jointLabel(kind)));
    auto* name = new QLineEdit(initial.name.isEmpty() ? jointLabel(kind) + QStringLiteral("-%1").arg(initial.id) : initial.name);
    name->setMaxLength(128);
    form->addRow(tr("名称"), name);
    auto* moving = new QComboBox;
    auto* base = new QComboBox;
    base->addItem(tr("大地"), 0);
    for (const auto& b : m_model.bodies) {
        if (!b.fixed) moving->addItem(b.name, b.id);
        base->addItem(b.name, b.id);
    }
    selectData(moving, initial.movingBodyId);
    selectData(base, initial.baseBodyId);
    form->addRow(tr("动作体"), moving);
    form->addRow(tr("基本体"), base);
    const auto origin = vectorFields(form, tr("原点 (mm)"), initial.originMm);
    const auto axis = kind == JointKind::Fixed ? std::array<QDoubleSpinBox*, 3>{}
                                              : vectorFields(form, tr("轴向"), initial.axis);
    finishForm(form, this, [=] {
        RigidModel model = m_model;
        auto joint = initial;
        joint.name = name->text().trimmed();
        joint.movingBodyId = moving->currentData().toInt();
        joint.baseBodyId = base->currentData().toInt();
        joint.originMm = vectorValue(origin);
        if (kind != JointKind::Fixed) joint.axis = vectorValue(axis);
        replace(model.joints, joint);
        commitRigid(std::move(model), {ModelNode::Joint, joint.id});
    }, [this] { closeEditor(); updateParameters(); });
    showEditor(panel);
}

void MotionWindow::editDrive(DriveKind kind, int id)
{
    if (motionBusy() || m_jobPart >= 0) return;
    const auto* current = byId(m_model.drives, id);
    const RigidDrive initial = current ? *current : RigidDrive{nextId(m_model.drives), {}, 0, kind, 0};
    auto* panel = new QWidget;
    auto* form = new QFormLayout(panel);
    const auto label = kind == DriveKind::Speed ? tr("速度驱动") : tr("位移驱动");
    form->addRow(new QLabel(label));
    auto* name = new QLineEdit(initial.name.isEmpty() ? label + QStringLiteral("-%1").arg(initial.id) : initial.name);
    name->setMaxLength(128);
    form->addRow(tr("名称"), name);
    auto* joint = new QComboBox;
    for (const auto& j : m_model.joints) {
        if (j.kind == JointKind::Fixed) continue;
        if (std::any_of(m_model.drives.begin(), m_model.drives.end(), [&](const auto& d) { return d.id != id && d.jointId == j.id; })) continue;
        joint->addItem(j.name, j.id);
    }
    selectData(joint, initial.jointId);
    form->addRow(tr("运动副"), joint);
    auto* value = number(form, kind == DriveKind::Speed ? tr("恒定速度") : tr("末时刻位移"), initial.value);
    auto showUnit = [=] {
        const auto* target = byId(m_model.joints, joint->currentData().toInt());
        if (target) value->setSuffix(target->kind == JointKind::Revolute
            ? (kind == DriveKind::Speed ? QStringLiteral(" °/s") : QStringLiteral(" °"))
            : (kind == DriveKind::Speed ? QStringLiteral(" mm/s") : QStringLiteral(" mm")));
    };
    connect(joint, qOverload<int>(&QComboBox::currentIndexChanged), panel, showUnit);
    showUnit();
    finishForm(form, this, [=] {
        RigidModel model = m_model;
        RigidDrive drive{initial.id, name->text().trimmed(), joint->currentData().toInt(), kind, value->value()};
        replace(model.drives, drive);
        commitRigid(std::move(model), {ModelNode::Drive, drive.id});
    }, [this] { closeEditor(); updateParameters(); });
    showEditor(panel);
}

void MotionWindow::editScenario(int id)
{
    if (motionBusy() || m_jobPart >= 0) return;
    const auto* current = byId(m_model.scenarios, id);
    const RigidScenario initial = current ? *current : RigidScenario{nextId(m_model.scenarios)};
    auto* panel = new QWidget;
    auto* form = new QFormLayout(panel);
    form->addRow(new QLabel(tr("求解方案")));
    auto* name = new QLineEdit(initial.name.isEmpty() ? tr("求解方案-%1").arg(initial.id) : initial.name);
    name->setMaxLength(128);
    form->addRow(tr("名称"), name);
    auto* end = number(form, tr("终止时间 (s)"), initial.duration, 1e-6, 3600);
    auto* step = number(form, tr("步长 (s)"), initial.step, 1e-6, 10);
    const auto gravity = vectorFields(form, tr("重力 (m/s²)"), initial.gravityMS2);
    finishForm(form, this, [=] {
        RigidModel model = m_model;
        RigidScenario scenario{initial.id, name->text().trimmed(), end->value(), step->value(), vectorValue(gravity)};
        replace(model.scenarios, scenario);
        commitRigid(std::move(model), {ModelNode::Scenario, scenario.id});
    }, [this] { closeEditor(); updateParameters(); });
    showEditor(panel);
}

void MotionWindow::removeRigidNode(ModelNode node)
{
    if (motionBusy()) return;
    RigidModel next = m_model;
    if (node.kind == ModelNode::Body) {
        if (std::any_of(next.joints.begin(), next.joints.end(), [&](const auto& j) { return j.movingBodyId == node.id || j.baseBodyId == node.id; })) {
            setNotice(tr("此运动体已用于运动副，请先删除相关运动副。"), true);
            return;
        }
        eraseIf(next.bodies, [&](const auto& b) { return b.id == node.id; });
    } else if (node.kind == ModelNode::Joint) {
        eraseIf(next.drives, [&](const auto& d) { return d.jointId == node.id; });
        eraseIf(next.joints, [&](const auto& j) { return j.id == node.id; });
    } else if (node.kind == ModelNode::Drive) eraseIf(next.drives, [&](const auto& d) { return d.id == node.id; });
    else if (node.kind == ModelNode::Scenario) eraseIf(next.scenarios, [&](const auto& sc) { return sc.id == node.id; });
    else return;
    commitRigid(std::move(next), {});
}

void MotionWindow::commitRigid(RigidModel model, ModelNode selection)
{
    if (motionBusy() || m_jobPart >= 0) return;
    QString error;
    if (!validateRigidModel(m_document, model, error)) { setNotice(error, true); return; }
    closeEditor();
    clearResults();
    m_model = std::move(model);
    m_modified = true;
    m_selectionNode = selection;
    if (selection.kind != ModelNode::Part) { m_partId = -1; m_viewport->selectPart(-1); }
    updateTree();
    updateParameters();
    updateActions();
    setNotice(tr("已更新刚体模型。"));
}

void MotionWindow::clearResults()
{
    if (m_playbackTimer) setPlayback(false);
    m_rigidPlot->setResult(nullptr, nullptr);
    m_resultScenario = 0;
    m_playbackTime = 0.0;
    m_results.clear();
    m_viewport->clearPartPoses();
    m_animationDock->closeDockWidget();
    m_curveDock->closeDockWidget();
}

void MotionWindow::startRigidSolve(int scenarioId, bool showCurves)
{
    if (motionBusy() || m_jobPart >= 0) return;
    auto run = std::make_shared<RigidRun>();
    QString error;
    if (!prepareRigidModel(m_document, m_model, scenarioId, run->input, error)) { setNotice(error, true); return; }
    closeEditor();
    setPlayback(false);
    run->showCurves = showCurves;
    try {
        m_solverThread = std::thread([run] {
            try {
                run->status = solveRigid(run->input, run->result, run->error, {
                    [run] { return run->stop.load(std::memory_order_relaxed); },
                    [run](double value) { run->progress.store(value, std::memory_order_relaxed); }});
            } catch (const std::exception& e) { run->error = QString::fromUtf8(e.what()); }
            catch (...) { run->error = QStringLiteral("MotionEngine 求解异常。"); }
            run->done.store(true, std::memory_order_release);
        });
    } catch (const std::exception& e) { setNotice(QString::fromUtf8(e.what()), true); return; }
    m_running = std::move(run);
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_progress->show();
    m_rigidPoll->start();
    updateActions();
    setNotice(tr("MotionEngine 正在求解…"));
}

void MotionWindow::pollRigidSolve()
{
    auto run = m_running;
    if (!run) return;
    m_progress->setValue(int(std::clamp(run->progress.load(), 0.0, 1.0) * 100));
    if (!run->done.load(std::memory_order_acquire)) return;
    m_solverThread.join();
    m_running.reset();
    m_rigidPoll->stop();
    m_progress->hide();
    m_progress->setRange(0, 0);
    if (run->status == SolveStatus::Completed && !run->result.frames.empty()) {
        const int id = run->result.scenarioId;
        m_rigidPlot->setResult(nullptr, nullptr);
        m_results[id] = run;
        m_resultScenario = 0;
        m_selectionNode = {ModelNode::Scenario, id};
        updateTree();
        showRigidResult({run->showCurves ? ModelNode::Curves : ModelNode::Animation, id});
        setNotice(tr("MotionEngine 求解完成：%1 帧 / %2 s").arg(run->result.frames.size()).arg(run->result.frames.back().seconds));
    } else {
        setNotice(run->status == SolveStatus::Stopped ? tr("已停止求解。") : tr("求解失败：%1").arg(run->error), run->status != SolveStatus::Stopped);
    }
    updateParameters();
    updateActions();
}

void MotionWindow::showRigidResult(ModelNode node)
{
    const auto it = m_results.find(node.id);
    if (it == m_results.end()) return;
    const auto& run = *it->second;
    if (node.kind == ModelNode::Export) {
        auto path = QFileDialog::getSaveFileName(this, tr("导出求解结果"), run.input.scenario.name + ".csv", tr("CSV 表格 (*.csv)"));
        if (path.isEmpty()) return;
        if (QFileInfo(path).suffix().isEmpty()) path += ".csv";
        QString error;
        if (exportRigidCsv(path, run.input, run.result, error)) setNotice(tr("已导出结果：%1").arg(QFileInfo(path).fileName()));
        else setNotice(error, true);
        return;
    }
    if (m_resultScenario != node.id) {
        setPlayback(false);
        m_resultScenario = node.id;
        m_rigidPlot->setResult(&run.input, &run.result);
        seekRigidTime(0);
    }
    m_animationDock->toggleView(true);
    if (node.kind == ModelNode::Curves) openDock(m_curveDock);
    else openDock(m_animationDock);
}

void MotionWindow::seekRigidTime(double seconds)
{
    const auto it = m_results.find(m_resultScenario);
    if (it == m_results.end()) return;
    const auto& run = *it->second;
    const auto& frames = run.result.frames;
    seconds = std::clamp(seconds, 0.0, frames.back().seconds);
    const auto right = std::lower_bound(frames.begin(), frames.end(), seconds,
        [](const RigidFrame& frame, double time) { return frame.seconds < time; });
    const auto& b = right == frames.end() ? frames.back() : *right;
    const auto& a = right == frames.begin() || right == frames.end() ? b : *std::prev(right);
    const float ratio = b.seconds > a.seconds ? float((seconds - a.seconds) / (b.seconds - a.seconds)) : 0.0f;
    std::map<int, QMatrix4x4> poses;
    for (std::size_t i = 0; i < run.input.bodies.size(); ++i) {
        const auto& body = run.input.bodies[i];
        if (body.body.fixed) continue;
        const QVector3D pos = vector3(a.bodies[i].positionMm) * (1.0f - ratio) + vector3(b.bodies[i].positionMm) * ratio;
        QMatrix4x4 pose;
        pose.translate(pos);
        pose.rotate(QQuaternion::slerp(quaternion(a.bodies[i].rotation), quaternion(b.bodies[i].rotation), ratio));
        pose.translate(-vector3(body.properties.centerMm));
        poses.emplace(body.body.partId, pose);
    }
    m_viewport->setPartPoses(poses);
    m_playbackTime = seconds;
    const QSignalBlocker block(m_timeline);
    m_timeline->setValue(frames.back().seconds > 0 ? qRound(ticks * seconds / frames.back().seconds) : 0);
    m_timeLabel->setText(QStringLiteral("%1 / %2 s").arg(seconds, 0, 'f', 3).arg(frames.back().seconds, 0, 'f', 3));
    m_rigidPlot->setTime(seconds);
}

void MotionWindow::setPlayback(bool play)
{
    if (play) {
        const auto it = m_results.find(m_resultScenario);
        if (it == m_results.end()) return;
        m_playbackStart = m_playbackTime >= it->second->result.frames.back().seconds ? 0.0 : m_playbackTime;
        m_playbackClock.restart();
        m_playbackTimer->start();
    } else m_playbackTimer->stop();
    m_playToggle->setText(play ? tr("暂停") : tr("播放"));
}

void MotionWindow::saveProject(bool choosePath)
{
    QString path = m_projectPath;
    if (choosePath || path.isEmpty()) path = QFileDialog::getSaveFileName(this, tr("保存刚体项目"),
        path.isEmpty() ? QStringLiteral("model.mxmotion") : path, tr("MxMotion 刚体项目 (*.mxmotion)"));
    if (path.isEmpty()) return;
    if (!path.endsWith(QStringLiteral(".mxmotion"), Qt::CaseInsensitive)) path += ".mxmotion";
    QString error;
    if (!saveRigidProject(path, m_document, m_model, error)) { setNotice(error, true); return; }
    m_projectPath = path;
    m_modified = false;
    setNotice(tr("已保存项目：%1").arg(QFileInfo(path).fileName()));
}

bool MotionWindow::openRigidProject(const QString& path)
{
    if (!confirmReplace() || m_jobPart >= 0) return false;
    CadDocument cad;
    RigidModel model;
    QString error;
    if (!loadRigidProject(path, cad, model, error)) { setNotice(error, true); return false; }
    closeEditor();
    clearResults();
    m_document = std::move(cad);
    m_model = std::move(model);
    m_meshes.clear();
    m_hidden.clear();
    m_meshForm->hide();
    command(Command::ShadedEdges)->trigger();
    m_projectPath = path;
    m_modified = false;
    m_selectionNode = {};
    m_partId = -1;
    updateTree();
    syncViewport(true);
    selectPart(-1);
    setNotice(tr("已打开项目：%1").arg(QFileInfo(path).fileName()));
    return true;
}

void MotionWindow::loadRigidExample()
{
    if (!confirmReplace() || m_jobPart >= 0) return;
    CadDocument cad;
    int part = 0;
    QString error;
    if (!cad.addPrimitive(Primitive::Box, 80, 14, 12, part, error)) { setNotice(error, true); return; }
    closeEditor();
    clearResults();
    m_document = std::move(cad);
    m_meshes.clear();
    m_hidden.clear();
    m_meshForm->hide();
    command(Command::ShadedEdges)->trigger();
    m_projectPath.clear();
    RigidModel model;
    model.bodies.push_back({1, tr("摇臂"), part, false, 1, 0});
    model.joints.push_back({1, tr("摇臂铰链"), JointKind::Revolute, 1, 0, {0, 7, 6}, {0, 0, 1}});
    model.drives.push_back({1, tr("恒速驱动"), 1, DriveKind::Speed, 90});
    model.scenarios.push_back({1, tr("刚体示例 · 90°/s"), 1.005, 0.01, {0, 0, 0}});
    commitRigid(std::move(model), {ModelNode::Scenario, 1});
    syncViewport(true);
    setNotice(tr("已载入刚体示例。点击「求解」运行。"));
}
} // namespace MotionOcc
