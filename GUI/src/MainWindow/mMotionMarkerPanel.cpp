

#include "mMotionMarkerPanel.h"
#include "mMotionRefMap.h"
#include "mMotionPickerActivate.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"
#include "mMechGlobalSignals.h"
#include "mMotionPanelColors.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mGetSetTypeTemplate.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "PickCommonFunction.h"

//BasicData
#include "mMotionMarker.h"
#include "mMotionMarkerManager.h"
#include "mMotionBody.h"
#include "mMotionBodyManager.h"
#include "mModelData.h"

//Qt
#include <QComboBox>
#include <QMessageBox>

using namespace MechPreData;
using namespace MechMotionData;
using namespace MxFunctions;
using namespace mxr;

namespace MechMainWindow
{

    mMotionMarkerPanel::mMotionMarkerPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        m_motionMarkerData = nullptr;

        m_mainLayout = new QVBoxLayout(this);

        // 1. 附着运动体
        FoldSection* foldBody = new FoldSection("附着运动体", this);
        m_cmbBody = new QComboBox();
        foldBody->addRow(new QLabel("选择运动体:"), m_cmbBody);
        m_mainLayout->addWidget(foldBody);

        // 2. 局部坐标
        FoldSection* foldPos = new FoldSection("局部坐标", this);
        m_posPicker = new mPointPickerWidget(QStringLiteral("位置 (mm)"), this);
        m_posPicker->pickController()->setAllowedTypes(&MechCommon::pickGeoPointEdgeFaceSolidNodePart);
        foldPos->addRow(m_posPicker);
        m_mainLayout->addWidget(foldPos);

        // ==================== 交互控件的 Lambda 实时同步 ====================

        // 位置坐标实时同步
        connect(m_posPicker, &mPointPickerWidget::coordinateChanged, this, [this]() {
            if (m_motionMarkerData) {
                m_motionMarkerData->setLocalPosition(m_posPicker->coordinate());
                updateUIFromData();
            }
            });

        // 运动体下拉框切换
        connect(m_cmbBody, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (m_motionMarkerData) {
                int bodyId = m_cmbBody->itemData(index).toInt();
                mMotionRefMap::set(Physical_MotionMarker, m_motionMarkerData->getId(), Physical_MotionBody, MapRelation_None, bodyId);
                updateUIFromData();
                emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionMarkerSig(false, m_motionMarkerData->getId());
            }
            });

        m_mainLayout->addStretch();
    }

    mMotionMarkerPanel::~mMotionMarkerPanel()
    {
        if (m_mainLayout != nullptr)
        {
            delete m_mainLayout;
            m_mainLayout = nullptr;
        }
    }

    void mMotionMarkerPanel::newMotionMarkerPanel()
    {
        setEditing(true);
        connectSig();

        int markerID = getCVM->getModelData()->getManager<mMotionMarkerManager>(Physical_MotionMarker).createID();
        m_motionMarkerData = new mMotionMarker(markerID);
        m_motionMarkerData->setName(getCVM->getModelData()->createNameByType(Physical_MotionMarker, "标记点"));
        getCVM->getModelData()->addPhysicalField<mMotionMarker>(m_motionMarkerData);

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionMarkerSig(true, m_motionMarkerData->getId());
    }

    void mMotionMarkerPanel::editMotionMarkerPanel(int id)
    {
        setEditing(true);
        m_motionMarkerData = getCVM->getModelData()->getManager<mMotionMarkerManager>(Physical_MotionMarker).getData(id);
        if (!m_motionMarkerData)
            return;

        connectSig();
        updateUIFromData();

        //进入编辑激活位置拾取控件
        activateFirstVisiblePointPicker({ m_posPicker });
    }

    //数据对象即将被删除（树删除前调用）：置空编辑指针防悬垂（周期边界同款约定）
    void mMotionMarkerPanel::onMotionMarkerDataAboutToBeDestroyed(int id)
    {
        if (id >= 0)
        {
            if (m_motionMarkerData == nullptr || id != m_motionMarkerData->getId())
                return;
        }
        m_motionMarkerData = nullptr;
    }

    void mMotionMarkerPanel::uneditMotionMarkerPanel()
    {
        if (!isEditing())
            return;
        setEditing(false);
        m_motionMarkerData = nullptr;
        //退出编辑时失活拾取控件并清理高亮
        if (m_posPicker && m_posPicker->pickController())
        {
            m_posPicker->pickController()->setPickActive(false);
            m_posPicker->clearAllHighlight();
            //拾取数据是临时的（确定值已入数据类），退出编辑清空防残留高亮
            m_posPicker->pickController()->displayData(PickWidgetData());
        }
        disconnectSig();
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("markerPoint");
    }

    void mMotionMarkerPanel::refreshBodyComboBox()
    {
        const QSignalBlocker b(m_cmbBody);
        m_cmbBody->clear();
        m_cmbBody->addItem(" ", -1);

        auto& bodyManager = getCVM->getModelData()->getManager<mMotionBodyManager>(Physical_MotionBody);
        std::vector<int> allIds = bodyManager.getAllIDs();
        for (int id : allIds) {
            QString name = bodyManager.getNameByID(id);
            m_cmbBody->addItem(name, id);
        }
    }

    void mMotionMarkerPanel::updateUIFromData()
    {
        if (!getCVM || !m_motionMarkerData)
            return;

        refreshBodyComboBox();

        // 运动体
        int bodyId = mMotionRefMap::get(Physical_MotionMarker, m_motionMarkerData->getId(), Physical_MotionBody);
        int bodyIndex = m_cmbBody->findData(bodyId);
        const QSignalBlocker bBody(m_cmbBody);
        m_cmbBody->setCurrentIndex(bodyIndex != -1 ? bodyIndex : 0);

        // 高亮运动体
        auto motionBodyData = getCVM->getModelData()->getManager<mMotionBodyManager>(Physical_MotionBody).getData(bodyId);
        if (motionBodyData)
        {
            auto widget_ptr = getCVM->getPreRenderWidget();
            auto highlight_controller = widget_ptr->getHighlightController();
            auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("red"));
            geoHighLightRender->setHighLightColor(mMotionPanelColors::bodyA().red(),
                mMotionPanelColors::bodyA().green(), mMotionPanelColors::bodyA().blue());
            mGeoSelectData* geoData = new mGeoSelectData();
            const std::set<int> highlightPartIds = mMotionRefMap::getSet(Physical_MotionBody, motionBodyData->getId(), Physical_Part);
            geoData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoPart, std::vector<int>(highlightPartIds.begin(), highlightPartIds.end()));
            geoHighLightRender->updateHighLightRender(geoData, HighStyle::PartALLFeatures);
        }

        // 位置
        QVector3D localPos = m_motionMarkerData->getLocalPosition();
        m_posPicker->setCoordinate(localPos);

        // 渲染标记点位置（局部坐标直接显示，后续可结合 body 位姿做全局转换）
        getCVM->getPreRenderWidget()->getCommonShapeRender()->appendPoint(
            "markerPoint", { localPos }, mMotionPanelColors::vec(mMotionPanelColors::pointA()));

    }

    void mMotionMarkerPanel::connectSig()
    {
        disconnectSig();
        if (!getCVM || !getCVM->getPreRenderWidget())
        {
            return;
        }

        conn_geo = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mMotionMarkerPanel::slot_finishedPick);
    }

    void mMotionMarkerPanel::disconnectSig()
    {
        if (conn_geo)
            disconnect(conn_geo);
    }

    void mMotionMarkerPanel::slot_finishedPick()
    {
        if (!getCVM)
        {
            return;
        }
        int pickNum = 0;
        auto pickFilter = getCVM->getCurrentPickDataType(pickNum);
        if (pickFilter == PickDataType::None)
        {
            return;
        }
    }

    void mMotionMarkerPanel::slot_subWindowActivated(QMdiSubWindow* subWindow)
    {
        if (!getCVM)
        {
            return;
        }
    }

}
