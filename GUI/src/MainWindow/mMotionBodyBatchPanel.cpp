#include "mMotionBodyBatchPanel.h"
#include "mMotionRefMap.h"
#include "mGlobalSignals.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mGetSetTypeTemplate.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "PickCommonFunction.h"

//BasicData
#include "mMotionBody.h"
#include "mMotionBodyManager.h"
#include "mModelData.h"
#include "mMeshData.h"

//Geo（质量属性自动计算引擎入口，与 mMotionBodyPanel::performAutoCalculation 同栈）
#include "mGeoEngineData.h"
#include "mGeoEngineHandle.h"

//Qt
#include <QEvent>
#include <QLabel>
#include <QGridLayout>
#include <QCompleter>
#include <QStringListModel>
#include <QPointer>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSet>
#include <functional>

using namespace MechPreData;
using namespace MechMotionData;
using namespace MxFunctions;
using namespace mxr;

namespace MechMainWindow
{
    mMotionBodyBatchPanel::mMotionBodyBatchPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        m_mainLayout = new QVBoxLayout(this);

        // ==================== 选择部件（mPropertyPanel/mMotionBodyPanel 同款：列表 + 右侧竖排 全部/增加/减少/清空） ====================
        FoldSection* foldParts = new FoldSection(QString::fromUtf8("选择部件"), this);
        QGridLayout* partsLayout = new QGridLayout();
        m_listPart = new QListWidget(this);
        m_listPart->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_btnAllPart = new QToolButton();
        m_btnAllPart->setIcon(QIcon(":/mMainWidgets/Icon/xuanzequanbu.png"));
        m_btnAllPart->setToolTip(QString::fromUtf8("全部部件"));
        m_btnAddPart = new QToolButton();
        m_btnAddPart->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        m_btnAddPart->setToolTip(QString::fromUtf8("增加部件"));
        m_btnReducePart = new QToolButton();
        m_btnReducePart->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        m_btnReducePart->setToolTip(QString::fromUtf8("减少部件"));
        m_btnClearPart = new QToolButton();
        m_btnClearPart->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png"));
        m_btnClearPart->setToolTip(QString::fromUtf8("清空部件"));

        partsLayout->addWidget(m_listPart, 0, 0, 8, 7);
        partsLayout->addWidget(m_btnAllPart, 0, 7);
        partsLayout->addWidget(m_btnAddPart, 1, 7);
        partsLayout->addWidget(m_btnReducePart, 2, 7);
        partsLayout->addWidget(m_btnClearPart, 3, 7);
        foldParts->addRow(partsLayout);
        m_mainLayout->addWidget(foldParts);

        connect(m_btnAllPart, &QToolButton::clicked, this, &mMotionBodyBatchPanel::slot_allPartBtn);
        connect(m_btnAddPart, &QToolButton::clicked, this, &mMotionBodyBatchPanel::slot_addPartBtn);
        connect(m_btnReducePart, &QToolButton::clicked, this, &mMotionBodyBatchPanel::slot_reducePartBtn);
        connect(m_btnClearPart, &QToolButton::clicked, this, &mMotionBodyBatchPanel::slot_clearPartBtn);

        // 列表的"激活"入口：点列表任意位置（含空白处，经 viewport 事件过滤）即接管拾取会话
        m_listPart->viewport()->installEventFilter(this);

        // ==================== 确定/取消 ====================
        QWidget* btnWidget = new QWidget(this);
        QHBoxLayout* btnLayout = new QHBoxLayout(btnWidget);
        btnLayout->setContentsMargins(0, 0, 0, 0);
        btnLayout->addStretch();
        m_btnOk = new QPushButton(QString::fromUtf8("确定"));
        m_btnCancel = new QPushButton(QString::fromUtf8("取消"));
        btnLayout->addWidget(m_btnOk);
        btnLayout->addWidget(m_btnCancel);
        m_mainLayout->addWidget(btnWidget);
        m_mainLayout->addStretch();

        connect(m_btnOk, &QPushButton::clicked, this, &mMotionBodyBatchPanel::slot_ok);
        connect(m_btnCancel, &QPushButton::clicked, this, &mMotionBodyBatchPanel::slot_cancel);
    }

    mMotionBodyBatchPanel::~mMotionBodyBatchPanel()
    {
        if (m_mainLayout != nullptr)
        {
            delete m_mainLayout;
            m_mainLayout = nullptr;
        }
    }

    void mMotionBodyBatchPanel::newBatchPanel()
    {
        setEditing(true);
        connectSig();

        m_listPart->clear();
        //列表空态即激活部件拾取：视口点选直接入列表（mMotionBodyPanel 同款会话语义）
        activatePartPick();
    }

    void mMotionBodyBatchPanel::uneditBatchPanel()
    {
        if (!isEditing())
            return;
        setEditing(false);
        disconnectSig();
        clearPartSelection();
    }

    int mMotionBodyBatchPanel::batchCreateMotionBodies(const std::vector<int>& partIds, QStringList* skippedNames)
    {
        if (!getCVM || !getCVM->getModelData())
            return 0;
        auto* modelData = getCVM->getModelData();
        auto& manager = modelData->getManager<mMotionBodyManager>(Physical_MotionBody);
        auto& mapRelation = modelData->getMapRelationManager();
        auto* meshData = modelData->getMeshData();
        if (!meshData)
            return 0;

        //一次收集：已归属其它运动体的部件集 + 已占用运动体名（批内新名也实时并入）
        QSet<int> ownedParts;
        QStringList usedNames;
        for (int bodyId : modelData->getAllPhyID(Physical_MotionBody))
        {
            usedNames << manager.getNameByID(bodyId);
            const auto owned = mMotionRefMap::getSet(Physical_MotionBody, bodyId, Physical_Part);
            for (int pid : owned)
                ownedParts.insert(pid);
        }

        int created = 0;
        QVector<int> createdIds;
        //第一阶段纯数据：建数据对象+映射+质量属性，不发任何 UI 信号
        //（逐体发信号会各触发一次树选中/滚动/面板刷新/信息表重建，是批量卡顿主源）
        for (int partId : partIds)
        {
            if (partId <= 0)
                continue;
            if (ownedParts.contains(partId))
            {
                if (skippedNames)
                    skippedNames->append(meshData->getPartNameByPartId(partId));
                continue;
            }

            //运动体名=部件名；与存量/批内重名追加 -2、-3（树复制命名同风格）
            const QString base = meshData->getPartNameByPartId(partId);
            QString name = base;
            int suffix = 1;
            while (usedNames.contains(name))
                name = QString("%1-%2").arg(base).arg(++suffix);
            usedNames << name;

            //与 mMotionBodyPanel::newMotionBodyPanel 同源副作用：数据 + 部件映射
            auto* body = new mMotionBody(manager.createID());
            body->setName(name);
            modelData->addPhysicalField<mMotionBody>(body);
            mapRelation.appendMapRelation({ Physical_MotionBody, body->getId() }, { Physical_Part, partId });

            //质量属性自动计算（面板 performAutoCalculation 同栈引擎调用，数据侧版）：
            //密度缺省 7.85e-6 kg/mm³（钢）；无几何/失败保留默认值不写垃圾
            const GeoNeutralData::GeoSolidMass geoProps =
                modelData->getGeoEngineDataMgr()->engine()
                .calcCalculatePartMass({ partId }, 7.85e-6);
            if (geoProps.mass > 1e-12)
            {
                body->setMass(geoProps.mass);
                body->setCenterOfMass(geoProps.centroid);
                body->setInertia({ geoProps.Ixx, geoProps.Iyy, geoProps.Izz,
                                   geoProps.Ixy, geoProps.Ixz, geoProps.Iyz });
            }

            createdIds.push_back(body->getId());
            ++created;
        }

        //第二阶段一次性 UI 刷新：树节点批量追加 + 末尾单次选中 + 状态/信息表批量
        if (!createdIds.isEmpty())
            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionBodyBatchSig(createdIds);
        return created;
    }

    void mMotionBodyBatchPanel::slot_allPartBtn()
    {
        if (!getCVM || !getCVM->getModelData() || !getCVM->getModelData()->getMeshData())
            return;
        m_listPart->clear();
        m_listPart->addItems(getCVM->getModelData()->getMeshData()->getAllPartNames());
        syncPickSetToList();
    }

    void mMotionBodyBatchPanel::slot_addPartBtn()
    {
        if (!getCVM || !getCVM->getModelData() || !getCVM->getModelData()->getMeshData())
            return;

        //行内编辑 + 名称自动补全选部件（mMotionBodyPanel::slot_addPartBtn 同款交互，
        //批量为纯列表操作：确定时才统一建体）
        QStringListModel* tempModel = new QStringListModel(this);
        tempModel->setStringList(getCVM->getModelData()->getMeshData()->getAllPartNames());

        QListWidgetItem* newItem = new QListWidgetItem();
        m_listPart->insertItem(0, newItem);

        QLineEdit* editor = new QLineEdit(m_listPart);
        m_listPart->setItemWidget(newItem, editor);
        editor->setFocus();

        QCompleter* completer = new QCompleter(tempModel, editor);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setFilterMode(Qt::MatchContains);
        completer->setCompletionMode(QCompleter::PopupCompletion);
        editor->setCompleter(completer);
        completer->complete();

        QPointer<QLineEdit> editorGuard(editor);
        auto finishFlag = std::make_shared<bool>(false);
        //批内去重：列表已有同名部件/名字无效则丢弃该行
        auto nameInList = [this](const QString& text) {
            for (int i = 0; i < m_listPart->count(); ++i)
            {
                QListWidgetItem* other = m_listPart->item(i);
                if (other != nullptr && other->text() == text && m_listPart->itemWidget(other) == nullptr)
                    return true;
            }
            return false;
            };

        auto finishEditing = [=]() {
            if (*finishFlag) return;
            *finishFlag = true;

            const QString text = editorGuard ? editorGuard->text().trimmed() : QString();
            const int row = m_listPart->row(newItem);
            const bool valid = !text.isEmpty()
                && getCVM && getCVM->getModelData()->getMeshData()
                && getCVM->getModelData()->getMeshData()->getPartIdByPartName(text) != -1
                && !nameInList(text);
            if (valid)
            {
                newItem->setText(text);
                m_listPart->removeItemWidget(newItem);
                syncPickSetToList();
            }
            else if (row >= 0)
            {
                m_listPart->takeItem(row);
                delete newItem;
            }
            if (editorGuard)
                editorGuard->deleteLater();
            tempModel->deleteLater();
            };
        connect(editor, &QLineEdit::editingFinished, this, finishEditing);
    }

    void mMotionBodyBatchPanel::slot_reducePartBtn()
    {
        for (QListWidgetItem* item : m_listPart->selectedItems())
        {
            m_listPart->takeItem(m_listPart->row(item));
            delete item;
        }
        syncPickSetToList();
    }

    void mMotionBodyBatchPanel::slot_clearPartBtn()
    {
        m_listPart->clear();
        syncPickSetToList();
    }

    void mMotionBodyBatchPanel::slot_ok()
    {
        if (!getCVM || !getCVM->getModelData())
            return;

        QStringList skipped;
        const int created = batchCreateMotionBodies(listPartIds(), &skipped);

        QString msg = QString::fromUtf8("批量创建 %1 个运动体（部件同名，重名自动加 -x 后缀）").arg(created);
        if (!skipped.isEmpty())
            msg += QString::fromUtf8("；跳过已归属部件：%1").arg(skipped.join(", "));
        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(msg);

        uneditBatchPanel();
    }

    void mMotionBodyBatchPanel::slot_cancel()
    {
        uneditBatchPanel();
    }

    std::vector<int> mMotionBodyBatchPanel::listPartIds() const
    {
        std::vector<int> ids;
        if (!getCVM || !getCVM->getModelData() || !getCVM->getModelData()->getMeshData())
            return ids;
        auto* meshData = getCVM->getModelData()->getMeshData();
        for (int i = 0; i < m_listPart->count(); ++i)
        {
            QListWidgetItem* item = m_listPart->item(i);
            //行内编辑未提交的行（itemWidget 非空）没有文本，跳过
            if (item == nullptr || m_listPart->itemWidget(item) != nullptr)
                continue;
            const int partId = meshData->getPartIdByPartName(item->text());
            if (partId != -1)
                ids.push_back(partId);
        }
        return ids;
    }

    //拾取选择集 ← 列表 差集同步（按钮改动列表后保持视口高亮一致；
    //断开容器信号再做增减，避免同步触发 slot_finishedPick 把改动刷掉）
    void mMotionBodyBatchPanel::syncPickSetToList()
    {
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;
        if (conn_geo)
            disconnect(conn_geo);

        MechCommon::mMechGlobalSignals::getInstance()->activePickSig(9);
        const auto current = MechPreData::getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
        const std::vector<int> want = listPartIds();
        QSet<int> currentSet;
        for (int id : current)
            currentSet.insert(id);
        QSet<int> wantSet;
        for (int id : want)
            wantSet.insert(id);

        std::vector<int> toRemove;
        for (int id : currentSet)
        {
            if (!wantSet.contains(id))
                toRemove.push_back(id);
        }
        std::vector<int> toAdd;
        for (int id : wantSet)
        {
            if (!currentSet.contains(id))
                toAdd.push_back(id);
        }
        if (!toRemove.empty())
            MechPreData::reducePreWidgetSelectPartID(getCVM->getPreRenderWidget(), toRemove);
        if (!toAdd.empty())
            MechPreData::addPreWidgetSelectPartID(getCVM->getPreRenderWidget(), toAdd);
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();

        conn_geo = connect(getCVM->getPreRenderWidget()->getSelectionContainer(),
            &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mMotionBodyBatchPanel::slot_finishedPick);
        m_lastPickPartData = MechPreData::getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
    }

    void mMotionBodyBatchPanel::slot_finishedPick()
    {
        if (!getCVM || !getCVM->getPreRenderWidget() || !getCVM->getModelData() || !isEditing())
            return;

        //纯部件会话：列表 = 拾取选择集（全量刷新；写列表不动选择容器，无回环）
        const auto partIds = MechPreData::getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
        auto* meshData = getCVM->getModelData()->getMeshData();
        if (!meshData)
            return;
        m_listPart->clear();
        for (int id : partIds)
        {
            const QString name = meshData->getPartNameByPartId(id);
            if (!name.isEmpty())
                m_listPart->addItem(name);
        }
        m_lastPickPartData = partIds;
    }

    void mMotionBodyBatchPanel::activatePartPick()
    {
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;
        //接管拾取会话：部件模式 + 视口高亮对齐当前列表
        syncPickSetToList();
        m_listPart->setToolTip(QString::fromUtf8("部件拾取已激活：视口点选加入部件，右键取消移除"));
    }

    void mMotionBodyBatchPanel::clearPartSelection()
    {
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;
        const auto selIds = MechPreData::getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
        if (selIds.empty())
        {
            m_lastPickPartData.clear();
            return;
        }
        if (conn_geo)
            disconnect(conn_geo);
        MechCommon::mMechGlobalSignals::getInstance()->activePickSig(9);
        MechPreData::reducePreWidgetSelectPartID(getCVM->getPreRenderWidget(), selIds);
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();
        m_lastPickPartData.clear();
    }

    void mMotionBodyBatchPanel::connectSig()
    {
        disconnectSig();
        connect(MechPreData::mViewportModelManager::getInstance(),
            &MechPreData::mViewportModelManager::currentViewportChanged,
            this, &mMotionBodyBatchPanel::slot_subWindowActivated);
    }

    void mMotionBodyBatchPanel::disconnectSig()
    {
        disconnect(MechPreData::mViewportModelManager::getInstance(),
            &MechPreData::mViewportModelManager::currentViewportChanged,
            this, &mMotionBodyBatchPanel::slot_subWindowActivated);
        if (conn_geo)
            disconnect(conn_geo);
    }

    bool mMotionBodyBatchPanel::eventFilter(QObject* obj, QEvent* ev)
    {
        // 列表 viewport 任意按下（含空白处，QListWidget::pressed 只在列表项上发射）即接管拾取会话
        if (obj == m_listPart->viewport() && ev->type() == QEvent::MouseButtonPress && isEditing())
            activatePartPick();
        return mPanelBase::eventFilter(obj, ev);
    }

    void mMotionBodyBatchPanel::slot_subWindowActivated(int viewportID)
    {
        Q_UNUSED(viewportID);
    }
}
