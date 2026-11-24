#include "MotionWindow.h"
#include "NativeMnfBindingView.h"
#include "NativeMnfPreview.h"
#include "NativeModeInspector.h"
#include "compat/model/LegacyMotionContext.h"

#include <QBoxLayout>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QLabel>
#include <QProgressDialog>
#include <QPushButton>
#include <QSet>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QtConcurrentRun>

#include <algorithm>
#include <array>

namespace MotionOcc {
namespace {

struct LoadedMnf {
    RigidFlexResult preview;
    QString error;
};

using MnfPoint = std::array<double, 3>;

struct BindingRow {
    int jointId;
    QString name;
    MnfPoint pivot;
    QComboBox* node = nullptr;
    MnfInterfaceMatch match;
};

class MnfNodeComboBox final : public QComboBox {
public:
    using QComboBox::QComboBox;

    std::function<void()> popupClosed;

    void hidePopup() override
    {
        QComboBox::hidePopup();
        if (popupClosed) popupClosed();
    }
};

std::array<QDoubleSpinBox*, 3> addReferenceRow(QFormLayout& form, const QString& text,
    const QString& objectPrefix, const QJsonArray& initial, double maximum, int decimals)
{
    auto* row = new QWidget;
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    std::array<QDoubleSpinBox*, 3> controls{};
    for (int i = 0; i < 3; ++i) {
        auto* control = new QDoubleSpinBox(row);
        const QChar axis = QLatin1Char(static_cast<char>('X' + i));
        control->setObjectName(objectPrefix + axis);
        control->setDecimals(decimals);
        control->setRange(-maximum, maximum);
        control->setPrefix(axis + QStringLiteral("  "));
        control->setValue(initial.at(i).toDouble());
        control->setKeyboardTracking(false);
        layout->addWidget(control);
        controls[i] = control;
    }
    form.addRow(text, row);
    return controls;
}

MnfPoint referenceValues(const std::array<QDoubleSpinBox*, 3>& fields)
{
    return {fields[0]->value(), fields[1]->value(), fields[2]->value()};
}

QJsonArray pointJson(const MnfPoint& point)
{
    return {point[0], point[1], point[2]};
}

QString pointText(const MnfPoint& point)
{
    return QStringLiteral("(%1, %2, %3) mm").arg(point[0], 0, 'g', 8)
        .arg(point[1], 0, 'g', 8).arg(point[2], 0, 'g', 8);
}

} // namespace

void MotionWindow::importMnf(const QString& requestedPath, int bindingId)
{
    if (motionBusy() || m_jobPart >= 0 || findChild<QProgressDialog*>(QStringLiteral("mnfImportProgress")))
        return;
    const auto chosen = requestedPath.isEmpty()
        ? QFileDialog::getOpenFileName(this, tr("导入 MNF"), {}, tr("模态中性文件 (*.mnf)"))
        : requestedPath;
    if (chosen.isEmpty()) return;
    const auto path = QFileInfo(chosen).absoluteFilePath();
    const auto document = m_model.nativeDocument;
    const auto definition = document && bindingId >= 0
        ? document->entity("importedMnfBodies", bindingId) : QJsonObject{};
    const auto name = definition.value("name").toString(QFileInfo(path).fileName());
    const auto assetDirectory = document && bindingId >= 0 ? document->projectAssetDirectory : nullptr;
    auto* progress = new QProgressDialog(tr("正在读取 %1 的振型和表面…").arg(name),
                                         QString{}, 0, 0, this);
    progress->setObjectName(QStringLiteral("mnfImportProgress"));
    progress->setWindowTitle(tr("导入 MNF"));
    progress->setWindowModality(Qt::WindowModal);
    progress->setCancelButton(nullptr);
    progress->setMinimumDuration(0);
    progress->setWindowFlag(Qt::WindowCloseButtonHint, false);
    auto* watcher = new QFutureWatcher<std::shared_ptr<LoadedMnf>>(progress);
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, progress, path, name, bindingId] {
        auto imported = watcher->result();
        progress->close();
        progress->deleteLater();
        if (!imported->error.isEmpty()) {
            setNotice(tr("导入 MNF 失败。%1").arg(imported->error), true);
            return;
        }
        const auto& preview = imported->preview;
        std::function<void()> bind;
        if (m_model.nativeDocument && !m_model.nativeDocument->collection("bodies").isEmpty()) {
            bind = [this, path, imported, bindingId] {
                configureImportedMnf(path, imported->preview, bindingId);
            };
        }
        openImportedMnfModeInspector(this, preview, name, std::move(bind));
        setNotice(tr("已读入 %1 的 %2 个节点和 %3 阶弹性振型。")
            .arg(name).arg(preview.sourceNodeIds.size()).arg(preview.modeCount));
    });
    watcher->setFuture(QtConcurrent::run([path, assetDirectory] {
        Q_UNUSED(assetDirectory); // Keep the extracted project asset alive during parsing.
        auto imported = std::make_shared<LoadedMnf>();
        readMnfModePreview(path, imported->preview, imported->error);
        return imported;
    }));
    progress->show();
}

void MotionWindow::configureImportedMnf(const QString& path, const RigidFlexResult& preview, int bindingId)
{
    if (motionBusy() || m_jobPart >= 0 || !m_model.nativeDocument) return;
    if (m_original) m_original->flush();
    auto document = *m_model.nativeDocument;
    const auto previous = document.entity("importedMnfBodies", bindingId);
    if (bindingId >= 0 && (previous.isEmpty() || QFileInfo(document.assetPath(previous.value("mnfFile").toString()))
        .absoluteFilePath() != path)) {
        setNotice(tr("当前项目中的 MNF 绑定已变化，请从柔性体树重新打开。"), true);
        return;
    }
    const auto storedBindings = previous.value("jointBindings").toObject();
    const auto filename = previous.value("name").toString(QFileInfo(path).fileName());
    QSet<int> unavailable;
    QSet<int> otherMnfBodies;
    for (const auto& item : document.collection("autoFlexBodies")) {
        const auto flex = item.toObject();
        if (flex.value("enabled").toBool(true)) unavailable.insert(flex.value("targetBodyId").toInt(-1));
    }
    for (const auto& item : document.collection("importedMnfBodies")) {
        const auto flex = item.toObject();
        if (flex.value("id").toInt(-1) != bindingId) {
            const int target = flex.value("targetBodyId").toInt(-1);
            unavailable.insert(target);
            if (flex.value("enabled").toBool(true)) otherMnfBodies.insert(target);
        }
    }
    if (preview.modalBasis.attachmentGroups.empty()) {
        setNotice(tr("%1 没有可供刚柔连接的 MNF 接口节点。").arg(filename), true);
        return;
    }

    QDialog dialog(this);
    dialog.setObjectName(QStringLiteral("mnfBodyBinding"));
    dialog.setWindowTitle(tr("绑定 %1 到运动体").arg(filename));
    dialog.resize(1090, 610);
    auto* layout = new QVBoxLayout(&dialog);
    auto* content = new QHBoxLayout;
    layout->addLayout(content, 1);
    auto* settings = new QVBoxLayout;
    content->addLayout(settings, 4);
    auto* form = new QFormLayout;
    settings->addLayout(form);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    auto* body = new QComboBox(&dialog);
    body->setObjectName(QStringLiteral("mnfTargetBody"));
    for (const auto& item : document.collection("bodies")) {
        const auto row = item.toObject();
        const int bodyId = row.value("id").toInt(-1);
        if (!row.value("isFixed").toBool() && !unavailable.contains(bodyId))
            body->addItem(row.value("name").toString() + QStringLiteral(" [%1]").arg(bodyId), bodyId);
    }
    if (body->count() == 0) {
        setNotice(tr("没有可绑定的运动体。请选择尚未分配柔性体的活动运动体。"), true);
        return;
    }
    const int previousBodyId = previous.value("targetBodyId").toInt(-1);
    int preferredBody = previousBodyId;
    if (bindingId < 0 && m_selectionNode.kind == ModelNode::Native && m_selectionNode.collection == "bodies")
        preferredBody = m_selectionNode.id;
    if (const int index = body->findData(preferredBody); index >= 0) body->setCurrentIndex(index);
    form->addRow(tr("运动体"), body);
    const auto origin = addReferenceRow(*form, tr("参考原点 (mm)"),
        QStringLiteral("mnfReferenceTranslation"),
        previous.value("referenceTranslationMm").toArray(), 1e9, 6);
    const auto euler = addReferenceRow(*form, tr("参考姿态 (°)"),
        QStringLiteral("mnfReferenceEuler"),
        previous.value("referenceEuler321Deg").toArray(), 360, 5);
    for (auto* field : euler) field->setToolTip(tr("绕 X、Y、Z 的角度，按 Rz · Ry · Rx 组合。"));

    auto* joints = new QTableWidget(0, 3, &dialog);
    joints->setObjectName(QStringLiteral("mnfJointBindings"));
    joints->setHorizontalHeaderLabels({tr("运动副"), tr("MNF 接口节点"), tr("距离 (mm)")});
    joints->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    joints->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    joints->verticalHeader()->hide();
    joints->setEditTriggers(QAbstractItemView::NoEditTriggers);
    joints->setSelectionMode(QAbstractItemView::SingleSelection);
    joints->setSelectionBehavior(QAbstractItemView::SelectRows);
    settings->addWidget(joints, 1);
    auto* problem = new QLabel(&dialog);
    problem->setWordWrap(true);
    settings->addWidget(problem);

    auto* viewPanel = new QVBoxLayout;
    content->addLayout(viewPanel, 5);
    auto* view = new NativeMnfBindingView(preview, &dialog);
    viewPanel->addWidget(view, 1);
    auto* status = new QLabel(&dialog);
    status->setObjectName(QStringLiteral("mnfBindingStatus"));
    status->setWordWrap(true);
    status->setTextInteractionFlags(Qt::TextSelectableByMouse);
    viewPanel->addWidget(status);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    std::vector<BindingRow> rows;
    QHash<int, QHash<int, int>> explicitNodes;
    for (auto it = storedBindings.begin(); it != storedBindings.end(); ++it)
        explicitNodes[previousBodyId].insert(it.key().toInt(), it.value().toInt());

    const auto showJoint = [&](int row, int previewIndex = -1) {
        if (row < 0 || row >= int(rows.size())) {
            view->setActiveJoint({}, std::nullopt);
            status->setText(tr("选择运动副后，可在三维预览中点选接口节点。"));
            return;
        }
        const auto& joint = rows[row];
        if (joint.match.candidates.empty()) {
            view->setActiveJoint(joint.name, joint.pivot);
            status->setText(tr("运动副原点 %1").arg(pointText(joint.pivot)));
            return;
        }
        const int selectedIndex = joint.node->currentIndex();
        const auto& candidate = joint.match.candidates[std::max(0, (previewIndex > 0 ? previewIndex : selectedIndex) - 1)];
        view->setActiveJoint(joint.name, joint.pivot, candidate.nodeId);
        status->setText(tr("运动副原点 %1\n节点 %2  %3    距离 %4 mm")
            .arg(pointText(joint.pivot)).arg(candidate.nodeId).arg(pointText(candidate.worldMm))
            .arg(candidate.distanceMm, 0, 'g', 6));
    };

    const auto refreshFeedback = [&] {
        int pending = 0;
        bool unsupported = false;
        for (int row = 0; row < int(rows.size()); ++row) {
            const auto& joint = rows[row];
            if (!joint.node) {
                unsupported = true;
                continue;
            }
            const int index = joint.node->currentIndex();
            const bool chosen = index > 0;
            if (!chosen) ++pending;
            QString distance;
            if (!joint.match.candidates.empty()) {
                const auto& candidate = joint.match.candidates[chosen ? index - 1 : 0];
                distance = QString::number(candidate.distanceMm, 'g', 5);
                if (joint.match.confidence == MnfMatchConfidence::Ambiguous && !chosen)
                    joints->item(row, 2)->setToolTip(tr("有多个距离相近的节点，请在预览中确认。"));
                else if (!chosen)
                    joints->item(row, 2)->setToolTip(tr("最近节点与运动副之间仍有偏差，请检查参考位姿或确认节点。"));
                else
                    joints->item(row, 2)->setToolTip(pointText(candidate.worldMm));
            }
            joints->item(row, 2)->setText(distance);
        }
        buttons->button(QDialogButtonBox::Ok)->setEnabled(!rows.empty() && !unsupported && pending == 0);
        if (rows.empty())
            problem->setText(tr("此运动体没有相连的运动副。"));
        else if (unsupported)
            problem->setText(tr("MNF 支持与刚体或地面相连的无驱动、无摩擦、无限位转动副。"));
        else if (pending > 0)
            problem->setText(tr("%1 个运动副的接口待确认。可调整参考位姿，或在列表和三维预览中选择节点。").arg(pending));
        else
            problem->setText(tr("可在三维预览中核对位置，或直接点选别的接口节点。"));
        showJoint(joints->currentRow());
    };

    const auto selectNode = [&](int row, int index) {
        if (row < 0 || row >= int(rows.size()) || !rows[row].node || index < 0) return;
        auto& joint = rows[row];
        auto& choices = explicitNodes[body->currentData().toInt()];
        if (index > 0) choices.insert(joint.jointId, joint.node->itemData(index).toInt());
        else choices.remove(joint.jointId);
        if (joint.match.confidence == MnfMatchConfidence::Confident)
            joint.node->setItemText(1, tr("节点 %1").arg(joint.match.candidates.front().nodeId));
        joint.node->setCurrentIndex(index);
        joints->setCurrentCell(row, 0);
        refreshFeedback();
    };

    const auto refreshMatches = [&] {
        const auto translation = referenceValues(origin);
        const auto rotation = referenceValues(euler);
        QMatrix4x4 placement;
        placement.translate(translation[0], translation[1], translation[2]);
        placement.rotate(rotation[2], 0, 0, 1);
        placement.rotate(rotation[1], 0, 1, 0);
        placement.rotate(rotation[0], 1, 0, 0);
        view->setPlacement(placement);

        const auto choices = explicitNodes.value(body->currentData().toInt());
        for (auto& joint : rows) {
            if (!joint.node) continue;
            joint.match = matchMnfInterfaceNodes(preview, translation, rotation, joint.pivot);
            const QSignalBlocker block(joint.node);
            joint.node->clear();
            const auto& candidates = joint.match.candidates;
            joint.node->addItem(candidates.empty() ? tr("无可用接口节点")
                : tr("待确认 · 推荐 %1").arg(candidates.front().nodeId));
            for (const auto& candidate : candidates) {
                joint.node->addItem(tr("节点 %1").arg(candidate.nodeId), candidate.nodeId);
                joint.node->setItemData(joint.node->count() - 1,
                    tr("%1，距运动副 %2 mm").arg(pointText(candidate.worldMm)).arg(candidate.distanceMm, 0, 'g', 6),
                    Qt::ToolTipRole);
            }
            const int explicitIndex = joint.node->findData(choices.value(joint.jointId, -1));
            if (explicitIndex > 0) {
                joint.node->setCurrentIndex(explicitIndex);
                continue;
            }
            if (joint.match.confidence == MnfMatchConfidence::Confident) {
                joint.node->setItemText(1, tr("节点 %1 · 自动匹配").arg(candidates.front().nodeId));
                joint.node->setCurrentIndex(1);
            }
        }
        refreshFeedback();
    };

    QSet<int> drivenJoints;
    for (const auto& value : document.collection("drives"))
        drivenJoints.insert(value.toObject().value("targetJointId").toInt(-1));
    const auto refreshJoints = [&] {
        const QSignalBlocker block(joints);
        rows.clear();
        joints->setRowCount(0);
        const int selected = body->currentData().toInt();
        for (const auto& item : document.collection("joints")) {
            const auto joint = item.toObject();
            const int action = joint.value("actionBodyId").toInt(-1);
            const int base = joint.value("baseBodyId").toInt(-1);
            if (action != selected && base != selected) continue;
            const int id = joint.value("id").toInt();
            const int row = joints->rowCount();
            const auto name = joint.value("name").toString() + QStringLiteral(" [%1]").arg(id);
            const auto pivot = joint.value("origin").toObject();
            rows.push_back({id, name, {pivot.value("x").toDouble(), pivot.value("y").toDouble(), pivot.value("z").toDouble()}});
            joints->insertRow(row);
            auto* nameItem = new QTableWidgetItem(name);
            nameItem->setData(Qt::UserRole, id);
            joints->setItem(row, 0, nameItem);
            joints->setItem(row, 2, new QTableWidgetItem);
            if (joint.value("jointType").toString() != "Revolute" || action == base || drivenJoints.contains(id)
                || joint.value("friction").toObject().value("enabled").toBool()
                || joint.value("limit").toObject().value("enabled").toBool()
                || otherMnfBodies.contains(action == selected ? base : action)) {
                joints->setItem(row, 1, new QTableWidgetItem(tr("暂不支持")));
                continue;
            }
            auto* node = new MnfNodeComboBox(joints);
            rows.back().node = node;
            joints->setCellWidget(row, 1, node);
            node->popupClosed = [&, row] { if (joints->currentRow() == row) showJoint(row); };
            connect(node, qOverload<int>(&QComboBox::activated), &dialog, [&, row](int index) { selectNode(row, index); });
            connect(node, qOverload<int>(&QComboBox::highlighted), &dialog, [&, row](int index) {
                joints->setCurrentCell(row, 0);
                showJoint(row, index);
            });
        }
        if (!rows.empty()) joints->setCurrentCell(0, 0);
        refreshMatches();
    };

    connect(joints, &QTableWidget::currentCellChanged, &dialog, [&] { showJoint(joints->currentRow()); });
    connect(body, qOverload<int>(&QComboBox::currentIndexChanged), &dialog, refreshJoints);
    for (auto* field : origin) connect(field, qOverload<double>(&QDoubleSpinBox::valueChanged), &dialog, refreshMatches);
    for (auto* field : euler) connect(field, qOverload<double>(&QDoubleSpinBox::valueChanged), &dialog, refreshMatches);
    view->setNodePickedCallback([&](int nodeId) {
        const int row = joints->currentRow();
        if (row >= 0 && rows[row].node) selectNode(row, rows[row].node->findData(nodeId));
    });
    refreshJoints();

    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        QJsonObject mapping;
        for (int row = 0; row < joints->rowCount(); ++row) {
            const auto* node = qobject_cast<QComboBox*>(joints->cellWidget(row, 1));
            if (!node || !node->currentData().isValid()) {
                problem->setText(tr("请为每个转动副选择接口节点。"));
                return;
            }
            mapping.insert(QString::number(joints->item(row, 0)->data(Qt::UserRole).toInt()), node->currentData().toInt());
        }

        auto imports = document.collection("importedMnfBodies");
        int newId = bindingId;
        if (newId < 0 || previous.isEmpty()) {
            newId = 0;
            for (const auto& item : imports) newId = std::max(newId, item.toObject().value("id").toInt() + 1);
        }
        QString storedPath = previous.value("mnfFile").toString(path);
        if (previous.isEmpty() && !document.baseDirectory.isEmpty()) {
            const auto relative = QDir(document.baseDirectory).relativeFilePath(path);
            if (QDir::isRelativePath(relative) && !relative.startsWith(QStringLiteral("../"))
                && !document.projectAssetNames.contains(relative))
                storedPath = relative;
        }
        auto definition = previous;
        definition["id"] = newId;
        definition["name"] = filename;
        definition["targetBodyId"] = body->currentData().toInt();
        definition["mnfFile"] = storedPath;
        definition["jointBindings"] = mapping;
        definition["referenceTranslationMm"] = pointJson(referenceValues(origin));
        definition["referenceEuler321Deg"] = pointJson(referenceValues(euler));
        bool replaced = false;
        for (int i = 0; i < imports.size(); ++i) {
            if (imports.at(i).toObject().value("id").toInt(-1) != newId) continue;
            imports[i] = definition;
            replaced = true;
            break;
        }
        if (!replaced) imports.append(definition);
        auto next = document;
        auto data = next.root.value("motionData").toObject();
        data.insert("importedMnfBodies", imports);
        next.root.insert("motionData", data);
        if (commitNativeDocument(std::move(next), {ModelNode::Native, newId, QStringLiteral("importedMnfBodies")})) {
            dialog.accept();
            setNotice(tr("已绑定 %1。可在柔性体树中检查振型和修改接口。").arg(filename));
        } else {
            problem->setText(tr("无法保存绑定。请查看模型消息并修正后重试。"));
        }
    });
    dialog.exec();
}

} // namespace MotionOcc
