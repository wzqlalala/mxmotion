
#include "mMotionLoadImportPanel.h"
#include "mGeneralPickUp.h"
#include "mPickWidgetController.h"
#include "mMotionLoadCaseGenerator.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mGeoEngineHandle.h"
#include "mMeshData.h"
#include "mMechGlobalSignals.h"
#include "mGlobalSignals.h"
#include "mGetSetTypeTemplate.h"
#include "mBrowserWindow.h"
#include "mTreeView.h"

#include "mModelData.h"
#include "mConnectorManager.h"
#include "mConnectorDistributedCoupling.h"
#include "mLoadBoundaryManager.h"
#include "mLoadForce.h"
#include "mLoadPressure.h"
#include "mCaseManager.h"
#include "mCaseImplicitLinearStatic.h"
#include "mMapRelationManager.h"
#include "mMixSet.h"

#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>

//#include <TopoDS_Face.hxx> //【OCC摘除·S10/P0-1】退役头死 include 清剿（冻结岛保档，同主线处理）
//#include <BRepAdaptor_Surface.hxx> //【OCC摘除·S10/P0-1】OCC 头随旧栈冻结岛退役
//#include <GeomAbs_SurfaceType.hxx> //【OCC摘除·S10/P0-1】OCC 头随旧栈冻结岛退役（产品集成清剿）

using namespace MxFunctions;
using namespace MechCommon;

namespace MechMainWindow
{
    mMotionLoadImportPanel::mMotionLoadImportPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        setTitle(QStringLiteral("多体载荷导入"));
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(4, 4, 4, 4);

        // ========== 文件 ==========
        FoldSection* fileSection = new FoldSection(QStringLiteral("工况文件"), this);
        m_editPath = new QLineEdit();
        m_editPath->setPlaceholderText(QStringLiteral("MxMotion 特征工况 JSON（.json）"));
        m_btnBrowse = new QPushButton(QStringLiteral("浏览…"));
        fileSection->addRow(new QLabel(QStringLiteral("文件")), m_editPath, m_btnBrowse);
        mainLayout->addWidget(fileSection);

        // ========== 目标体 ==========
        FoldSection* bodySection = new FoldSection(QStringLiteral("载荷目标"), this);
        m_comboBody = new QComboBox();
        bodySection->addRow(new QLabel(QStringLiteral("目标运动体")), m_comboBody);
        mainLayout->addWidget(bodySection);

        // ========== 运动副 + 几何面绑定 ==========
        m_jointSection = new FoldSection(QStringLiteral("运动副 → 几何面"), this);
        mainLayout->addWidget(m_jointSection);

        // ========== 生成 ==========
        m_btnGenerate = new QPushButton(QStringLiteral("生成静力工况"));
        m_btnGenerate->setMinimumHeight(32);
        mainLayout->addWidget(m_btnGenerate);
        mainLayout->addStretch();

        connect(m_btnBrowse, &QPushButton::clicked, this, &mMotionLoadImportPanel::onBrowse);
        connect(m_comboBody, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &mMotionLoadImportPanel::onBodyChanged);
        connect(m_btnGenerate, &QPushButton::clicked, this, &mMotionLoadImportPanel::onGenerate);
    }

    mMotionLoadImportPanel::~mMotionLoadImportPanel()
    {
        // 行控件（含无父对象的拾取控制器）由 deactivatePickAndCommit 后的
        // refreshJointRows/此处统一释放，这里直接 delete 收尾
        for (auto& row : m_jointRows) {
            if (row.chk) delete row.chk;
            if (row.facePick) delete row.facePick;
        }
        m_jointRows.clear();
    }

    bool mMotionLoadImportPanel::isEditing() const
    {
        for (const auto& row : m_jointRows) {
            if (!row.facePick) continue;
            if (row.facePick->isPickActive() || !row.facePick->getAllData().isEmpty())
                return true;
        }
        return false;
    }

    void mMotionLoadImportPanel::deactivatePickAndCommit()
    {
        for (const auto& row : m_jointRows) {
            if (!row.facePick) continue;
            //未激活且无拾取数据：无编辑态可收尾，跳过（unEditInfoWindow 高频触发）
            if (!row.facePick->isPickActive() && row.facePick->getAllData().isEmpty())
                continue;
            const bool wasActive = row.facePick->isPickActive();
            // 激活控件：失活提交且清空高亮；未激活控件：仅清掉临时高亮。
            // keep 标志用完即复位（对齐 mPickWidgetController 视口切换联动写法）
            row.facePick->setKeepHighlightOnDeactivate(false);
            if (wasActive)
                row.facePick->setPickActive(false);
            row.facePick->setKeepHighlightOnDeactivate(true);
            if (!wasActive)
                row.facePick->clearAllHighlight();
        }
    }

    void mMotionLoadImportPanel::openWithFile(const QString& path)
    {
        if (!path.isEmpty()) {
            m_editPath->setText(path);
            QString err;
            if (!parseJson(path, err)) {
                mGlobalSignals::getInstance()->outputMessageSig(1,
                    QStringLiteral("[多体载荷] 解析失败：%1").arg(err));
                return;
            }
            mGlobalSignals::getInstance()->outputMessageSig(
                QStringLiteral("[多体载荷] 已加载 %1：%2 个体 / %3 个副 / %4 个工况")
                    .arg(path).arg(m_bodies.size()).arg(m_joints.size()).arg(m_cases.size()));
            refreshBodyCombo();
        }
    }

    void mMotionLoadImportPanel::onBrowse()
    {
        const QString path = QFileDialog::getOpenFileName(
            this, QStringLiteral("选择多体工况 JSON"), m_editPath->text(),
            QStringLiteral("LoadCase JSON(*.json)"));
        if (path.isEmpty())
            return;
        m_editPath->setText(path);
        QString err;
        if (!parseJson(path, err)) {
            QMessageBox::warning(this, QStringLiteral("提示"), err);
            return;
        }
        mGlobalSignals::getInstance()->outputMessageSig(
            QStringLiteral("[多体载荷] 已加载 %1：%2 个体 / %3 个副 / %4 个工况")
                .arg(path).arg(m_bodies.size()).arg(m_joints.size()).arg(m_cases.size()));
        refreshBodyCombo();
    }

    // ==================== 解析 ====================

    bool mMotionLoadImportPanel::parseJson(const QString& path, QString& error)
    {
        m_bodies.clear(); m_joints.clear(); m_cases.clear();
        m_analysisBodyId = -1; m_analysisBodyName.clear();
        m_filePath.clear();

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            error = QStringLiteral("无法打开文件：%1").arg(path);
            return false;
        }
        QJsonParseError pe;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &pe);
        if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
            error = QStringLiteral("JSON 解析失败：%1").arg(pe.errorString());
            return false;
        }
        QJsonObject root = doc.object();
        if (root.value("info").toObject().value("format").toString() != "MxMotionLoadCases") {
            error = QStringLiteral("不是多体工况文件（format 不是 MxMotionLoadCases）。");
            return false;
        }

        // info.source
        const QJsonObject src = root.value("info").toObject().value("source").toObject();
        m_analysisBodyId = src.value("analysisBodyId").toInt(-1);
        m_analysisBodyName = src.value("analysisBodyName").toString();

        auto readIntArray = [](const QJsonArray& a) {
            QVector<int> out; for (const auto& v : a) out.push_back(v.toInt()); return out;
        };

        for (const auto& bv : root.value("bodies").toArray()) {
            const QJsonObject b = bv.toObject();
            BodyInfo bi;
            bi.id = b.value("id").toInt(-1);
            bi.name = b.value("name").toString();
            bi.partIds = readIntArray(b.value("partIds").toArray());
            bi.isFixed = b.value("isFixed").toBool(false);
            m_bodies.push_back(bi);
        }
        for (const auto& jv : root.value("joints").toArray()) {
            const QJsonObject j = jv.toObject();
            JointInfo ji;
            ji.id = j.value("id").toInt(-1);
            ji.name = j.value("name").toString();
            ji.type = j.value("type").toString();
            ji.actionBodyId = j.value("actionBodyId").toInt(-1);
            ji.baseBodyId = j.value("baseBodyId").toInt(-1);
            ji.actionPartIds = readIntArray(j.value("actionBodyPartIds").toArray());
            ji.basePartIds = readIntArray(j.value("baseBodyPartIds").toArray());
            const QJsonArray ap = j.value("actionPoint").toArray();
            if (ap.size() == 3)
                ji.actionPoint = QVector3D(ap[0].toDouble(), ap[1].toDouble(), ap[2].toDouble());
            m_joints.push_back(ji);
        }
        for (const auto& cv : root.value("cases").toArray()) {
            const QJsonObject c = cv.toObject();
            LoadCase lc;
            lc.name = c.value("name").toString();
            lc.timeSeconds = c.value("timeSeconds").toDouble();
            for (const auto& lv : c.value("loads").toArray()) {
                const QJsonObject l = lv.toObject();
                CaseLoad cl;
                cl.jointId = l.value("jointId").toInt(-1);
                cl.jointName = l.value("jointName").toString();
                const QJsonObject comp = l.value("components").toObject();
                cl.comp.fx = comp.value("FX").toDouble();
                cl.comp.fy = comp.value("FY").toDouble();
                cl.comp.fz = comp.value("FZ").toDouble();
                cl.comp.fm = comp.value("FM").toDouble();
                cl.comp.tx = comp.value("TX").toDouble();
                cl.comp.ty = comp.value("TY").toDouble();
                cl.comp.tz = comp.value("TZ").toDouble();
                cl.comp.tm = comp.value("TM").toDouble();
                lc.loads.push_back(cl);
            }
            if (!lc.loads.isEmpty())
                m_cases.push_back(lc);
        }

        if (m_bodies.isEmpty() || m_joints.isEmpty() || m_cases.isEmpty()) {
            error = QStringLiteral("文件内容不完整（bodies/joints/cases 有空）。");
            return false;
        }
        m_filePath = path;
        return true;
    }

    const mMotionLoadImportPanel::JointInfo* mMotionLoadImportPanel::findJoint(int jointId) const
    {
        for (const auto& j : m_joints) if (j.id == jointId) return &j;
        return nullptr;
    }
    const mMotionLoadImportPanel::BodyInfo* mMotionLoadImportPanel::findBody(int bodyId) const
    {
        for (const auto& b : m_bodies) if (b.id == bodyId) return &b;
        return nullptr;
    }

    // ==================== UI 刷新 ====================

    void mMotionLoadImportPanel::refreshBodyCombo()
    {
        m_comboBody->blockSignals(true);
        m_comboBody->clear();
        int selectIdx = 0;
        for (int i = 0; i < m_bodies.size(); ++i) {
            const auto& b = m_bodies[i];
            QString label = b.name + (b.isFixed ? QStringLiteral("（机架）") : QString());
            m_comboBody->addItem(label, b.id);
            if (b.id == m_analysisBodyId)
                selectIdx = i;
        }
        m_comboBody->setCurrentIndex(selectIdx);
        m_comboBody->blockSignals(false);
        // 目标体由多体导出时的 analysisBodyId 唯一确定（力已变换到该体初始姿态系），
        // 不允许用户改选其它体（改了坐标系对不上）。
        m_comboBody->setEnabled(false);
        refreshJointRows();
    }

    void mMotionLoadImportPanel::onBodyChanged(int index)
    {
        Q_UNUSED(index)
        refreshJointRows();
    }

    void mMotionLoadImportPanel::refreshJointRows()
    {
        // 清旧行：先结束拾取编辑（旧行控制器可能处于激活态），再从折叠块布局移除
        deactivatePickAndCommit();
        m_jointSection->removeAll();
        for (auto& row : m_jointRows) {
            if (row.chk) row.chk->deleteLater();
            if (row.facePick) {
                if (row.facePick->widget()) row.facePick->widget()->deleteLater();
                row.facePick->deleteLater();  // 控制器无父对象，须随行一起释放
            }
        }
        m_jointRows.clear();

        const int bodyId = m_comboBody->currentData().toInt();
        if (bodyId < 0) { m_jointSection->refreshLayout(); return; }

        for (const auto& j : m_joints) {
            if (j.actionBodyId != bodyId && j.baseBodyId != bodyId)
                continue;
            JointRow row;
            row.jointId = j.id;
            row.chk = new QCheckBox(QStringLiteral("%1 [%2]").arg(j.name, j.type));
            row.chk->setChecked(true);

            row.facePick = mGeneralPickUp::createMorePickWidget(CBRed);
            row.facePick->setDefultTip(QStringLiteral("几何面（可多选）"));
            row.facePick->setAllowedTypes(&pickGeoPointEdgeFaceSolidNodePart);

            m_jointSection->addRow(row.chk, row.facePick->widget());
            m_jointRows.push_back(row);
        }
        m_jointSection->refreshLayout();
    }

    // ==================== 生成 ====================

    QVector<QPair<QString, mMotionLoadImportPanel::LoadComp>>
        mMotionLoadImportPanel::loadsForJoint(int jointId) const
    {
        QVector<QPair<QString, LoadComp>> out;
        for (const auto& lc : m_cases)
            for (const auto& l : lc.loads)
                if (l.jointId == jointId)
                    out.push_back({ lc.name, l.comp });
        return out;
    }

    void mMotionLoadImportPanel::onGenerate()
    {
        QString err;
        if (!generateCases(err))
            QMessageBox::warning(this, QStringLiteral("提示"), err);
    }

    bool mMotionLoadImportPanel::generateCases(QString& error)
    {
        if (m_filePath.isEmpty() || m_cases.isEmpty()) {
            error = QStringLiteral("请先加载有效的多体工况文件。");
            return false;
        }
        if (!getCVM || !getCVM->getModelData()) {
            error = QStringLiteral("当前没有打开的模型。");
            return false;
        }

        // 收集选中副 + 几何面（一个副可选多个面，压力会铺到所有选中面上）
        QMap<int, QVector<int>> faceBindings;
        for (const auto& row : m_jointRows) {
            if (!row.chk->isChecked()) continue;
            const JointInfo* j = findJoint(row.jointId);
            if (!j) continue;
            // 收集该副拾取的全部几何面（可多个）
            QVector<int> faceIds;
            if (row.facePick) {
                const auto data = row.facePick->getAllData();
                for (const auto& pair : data.pickedData) {
                    if (pair.first != PickDataType::GeoFace) continue;
                    for (const auto& d : pair.second) {
                        if (d.result_id > 0 && !faceIds.contains(d.result_id))
                            faceIds.push_back(d.result_id);
                    }
                }
            }
            if (faceIds.isEmpty()) {
                error = QStringLiteral("运动副「%1」还没有拾取几何面。").arg(j->name);
                return false;
            }
            faceBindings.insert(j->id, faceIds);
        }
        if (faceBindings.isEmpty()) {
            error = QStringLiteral("请至少勾选一个运动副。");
            return false;
        }

        // 组装共用生成内核的输入（载荷为 JSON 原值：分析体初始姿态系最终值）
        QVector<mMotionLoadCaseGenerator::GenJoint> genJoints;
        for (auto it = faceBindings.begin(); it != faceBindings.end(); ++it) {
            const JointInfo* j = findJoint(it.key());
            if (!j) continue;
            mMotionLoadCaseGenerator::GenJoint gj;
            gj.jointId = j->id;
            gj.name = j->name;
            gj.actionPoint = j->actionPoint;
            genJoints.push_back(gj);
        }
        QVector<mMotionLoadCaseGenerator::GenCase> genCases;
        for (const auto& lc : m_cases) {
            mMotionLoadCaseGenerator::GenCase gc;
            gc.name = lc.name;
            gc.timeSeconds = lc.timeSeconds;
            for (const auto& load : lc.loads) {
                mMotionLoadCaseGenerator::GenLoad gl;
                gl.jointId = load.jointId;
                gl.force = QVector3D(load.comp.fx, load.comp.fy, load.comp.fz);
                gl.torque = QVector3D(load.comp.tx, load.comp.ty, load.comp.tz);
                gc.loads.push_back(gl);
            }
            genCases.push_back(gc);
        }

        int nCases = 0, nLoads = 0, nRbe3 = 0;
        if (!mMotionLoadCaseGenerator::generate(genJoints, genCases, faceBindings, error,
                &nCases, &nLoads, &nRbe3))
            return false;

        mGlobalSignals::getInstance()->outputMessageSig(
            QStringLiteral("[多体载荷] 已生成 %1 个静力工况、%2 个载荷、%3 个 RBE3 耦合。")
                .arg(nCases).arg(nLoads).arg(nRbe3), true);
        return true;
    }
}
