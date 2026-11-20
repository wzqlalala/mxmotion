#pragma execution_character_set("utf-8")

#include "mPeriodicBoundaryPanel.h"
#include "mAppConfiguration.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mCalcLineEdit.h"
#include "mGlobalSignals.h"
#include "mInfoWindow.h"
#include "mDockLayoutManager.h"

#include "mPythonAgent.h"
#include "mPyScriptWriter.h"
#include "mPyScriptReader.h"
#include "PickCommonFunction.h"
#include "mGeneralPickUp.h"
#include "mGetSetTypeTemplate.h"
#include "mGetSetFunction.h"
#include "IPickPersistenceStrategy.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/view/CCSMPreRender.h"
#include "mxr/view/HighlightController.h"
#include "mxr/view/mPreMeshHighLightRender.h"
#include "mxr/view/mPreGeoHighLightRender.h"
#include "mxr/view/CCSMMeshHighLightRender.h"

//Qt
#include <QComboBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QToolButton>
#include <QTimer>
#include <QFormLayout>
#include <QDateTime>
#include <QSpacerItem>
#include <QMetaObject>

//BasicData
#include "mModelData.h"
#include "mMeshData.h"
#include "mPeriodicBoundaryManager.h"
#include "mPeriodicBoundary.h"
#include "mSetBase.h"
#include "mPartSet.h"
#include "mMixSet.h"
#include "mGeoSet.h"
#include "mElementSurface.h"

//Geo
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（冻结岛保档，同主线处理）

#include "AllRenderSymbolManager.h"
#include "mDataStatusManager.h"

#include <functional>

using namespace MechPreData;
using namespace MechSetData;
using namespace MechContactData;
using namespace MechElementData;
using namespace MxFunctions;
using namespace mxr;

namespace MechMainWindow
{
    mPeriodicBoundaryPanel::mPeriodicBoundaryPanel(QWidget* parent) : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);
        _mainLayout->setContentsMargins(2, 0, 2, 0);

        _pbData = nullptr;
        _currentViewportPrivate = nullptr;

        initUI();

        for (int i = 0; i < m_foldWidgets.size(); ++i)
        {
            _mainLayout->removeWidget(m_foldWidgets[i]);
            m_foldWidgets[i]->hide();
        }

        _mainLayout->addStretch();
    }

    mPeriodicBoundaryPanel::~mPeriodicBoundaryPanel()
    {
    }

    void mPeriodicBoundaryPanel::initUI()
    {
        // === 0. 类型 ===
        {
            FoldSection* f = new FoldSection("类型", this);
            cmb_type = new QComboBox();
            cmb_type->addItems({ "平移边界", "旋转边界" });
            f->addRow(new PanelLabel("类型"), cmb_type);
            m_foldWidgets.append(f);

            connect(cmb_type, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mPeriodicBoundaryPanel::slot_typeComboBoxChanged);
        }

        // ================== 1. 边界面 ==================
        {
            FoldSection* f = new FoldSection("边界面", this);

            // 总体容器
            QWidget* pairContainer = new QWidget();
            QHBoxLayout* mainHLayout = new QHBoxLayout(pairContainer);
            mainHLayout->setContentsMargins(0, 0, 0, 0);
            mainHLayout->setSpacing(5);

            // 左侧垂直布局 (主面行 + 从面行)
            QWidget* leftWidget = new QWidget();
            QVBoxLayout* leftVLayout = new QVBoxLayout(leftWidget);
            leftVLayout->setContentsMargins(0, 0, 0, 0);
            leftVLayout->setSpacing(5);

            auto createPickRow = [&](mPickWidgetController*& picker, mColorBlock colorBlock, const QString& tip) -> QWidget*
                {
                    QWidget* rowW = new QWidget();
                    QHBoxLayout* rowH = new QHBoxLayout(rowW);
                    rowH->setContentsMargins(0, 0, 0, 0);
                    rowH->setSpacing(4);

                    picker = mGeneralPickUp::createPickWidgetNoTempHight(colorBlock, rowW);
                    picker->setDefultTip(tip);
                    picker->setAllowedTypes(&pickGeoFaceAndElementSurface);

                    connect(picker, &mPickWidgetController::pickActiveChanged, this, [=](bool isActive) {
                        if (isActive)
                        {
                            emit mMechGlobalSignals::getInstance()->activePickSig(7);
                        }
                        else
                        {
                            // 取消激活设置为0
                            if (tip == "主面")
                            {
                                _pbData->setPickStateMaster(0);
                            }
                            else if (tip == "从面")
                            {
                                _pbData->setPickStateSlave(0);
                            }
                        }
                        });

                    if (tip == "主面")
                    {
                        connect(picker, &mPickWidgetController::dataChanged, this, &mPeriodicBoundaryPanel::slot_commitMasterPeriodicBoundaryData);
                    }
                    else if (tip == "从面")
                    {
                        connect(picker, &mPickWidgetController::dataChanged, this, &mPeriodicBoundaryPanel::slot_commitSlavePeriodicBoundaryData);
                    }

                    rowH->addWidget(picker->widget(), 1);

                    return rowW;
                };

            QWidget* masterRow = createPickRow(pick_master, CBRed, "主面");
            leftVLayout->addWidget(masterRow);

            QWidget* slaveRow = createPickRow(pick_slave, CBBlue, "从面");
            leftVLayout->addWidget(slaveRow);

            // 右侧交换按钮
            btn_swap = new QToolButton();
            btn_swap->setIcon(QIcon(":/mMainWidgets/Icon/jiaohuan3x.png"));
            btn_swap->setFixedWidth(30);
            btn_swap->setFixedHeight(30);
            btn_swap->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            btn_swap->setToolTip("交换");

            mainHLayout->addWidget(leftWidget, 1);
            mainHLayout->addWidget(btn_swap, 0);

            f->addRow(pairContainer);
            m_foldWidgets.append(f);

            // 交换按钮：先提交控件数据到底层集合，再交换底层集合id映射，最后从底层反显并刷新渲染
            connect(btn_swap, &QPushButton::clicked, this, [=]() {
                if (!_pbData) return;

                mPickWidgetController::commitActivePickWidget();

                auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
                int masterSetId = mapRelationManager.getMapRelation<int>(
                    { PhysicalFieldType::Physical_PeriodicBoundary, _pbData->getPeriodicBoundaryId() }, Physical_Set, MapRelation_Master);
                int slaveSetId = mapRelationManager.getMapRelation<int>(
                    { PhysicalFieldType::Physical_PeriodicBoundary, _pbData->getPeriodicBoundaryId() }, Physical_Set, MapRelation_Slave);

                if (masterSetId != -1 || slaveSetId != -1)
                {
                    mapRelationManager.appendMapRelation(
                        { PhysicalFieldType::Physical_PeriodicBoundary, _pbData->getPeriodicBoundaryId() },
                        { Physical_Set, slaveSetId }, MapRelation_Master);
                    mapRelationManager.appendMapRelation(
                        { PhysicalFieldType::Physical_PeriodicBoundary, _pbData->getPeriodicBoundaryId() },
                        { Physical_Set, masterSetId }, MapRelation_Slave);
                }

                PickWidgetData masterData, slaveData;
                loadPeriodicBoundaryDataFromSet(MapRelation_Master, masterData);
                loadPeriodicBoundaryDataFromSet(MapRelation_Slave, slaveData);

                if (pick_master) pick_master->displayData(masterData, true);
                if (pick_slave) pick_slave->displayData(slaveData, true);

                slot_commitMasterPeriodicBoundaryData();
                slot_commitSlavePeriodicBoundaryData();
                });
        }

        // ================== 2. 方向/轴 ==================
        {
            FoldSection* f = new FoldSection("方向/轴", this);

            _directionWidget = new mDirectionSelectorWidget(
                QStringLiteral("方向"), true, QStringLiteral("平移距离"), false, QString(), false, this);
            _directionWidget->setMagnitudeVisible(true);
            _directionWidget->setDirectionMode(mDirectionSelectorWidget::XAxis);
            _directionWidget->setCustomCoordinatePoints(QVector3D(0, 0, 0), QVector3D(1, 0, 0));

            connect(_directionWidget, &mDirectionSelectorWidget::directionChanged, this, [this]() {
                if (!_pbData) return;

                saveDirectionWidgetToPbData();
                updateDirectionArrowRender();
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_PeriodicBoundary, _pbData->getPeriodicBoundaryId());

                // 强制刷新视口，确保箭头实时更新
                if (getCVM && getCVM->getPreRenderWidget())
                {
                    auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
                    if (symbolController)
                        symbolController->UpdateAllSymbol();
                    getCVM->getPreRenderWidget()->updatePaint();
                }
                });

            connect(_directionWidget, &mDirectionSelectorWidget::magnitudeChanged, this, [this]() {
                if (!_pbData) return;
                bool isTranslation = (_pbData->getPeriodicBoundaryType() == PeriodicBoundary_Translation);
                QString valStr = _directionWidget->magnitudeValue();
                bool ok;
                double val = valStr.toDouble(&ok);

                if (isTranslation)
                {
                    // 平移距离：非负数，不合法修正为0
                    if (!ok || val < 0)
                    {
                        val = 0.0;
                        QString fixedStr = QString::number(val);
                        _directionWidget->setMagnitudeValue(fixedStr);
                        _pbData->value = fixedStr;
                    }
                    else
                    {
                        _pbData->value = valStr;
                    }
                }
                else
                {
                    // 周向对称数量：正整数，至少1，不合法修正为1
                    if (!ok || val < 1)
                    {
                        val = 1.0;
                        QString fixedStr = QString::number(static_cast<int>(val));
                        _directionWidget->setMagnitudeValue(fixedStr);
                        _pbData->value = fixedStr;
                    }
                    else
                    {
                        // 强制取整
                        int intVal = static_cast<int>(val);
                        QString fixedStr = QString::number(intVal);
                        if (fixedStr != valStr) {
                            _directionWidget->setMagnitudeValue(fixedStr);
                        }
                        _pbData->value = fixedStr;
                    }
                }
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_PeriodicBoundary, _pbData->getPeriodicBoundaryId());
                });

            connect(_directionWidget, &mDirectionSelectorWidget::layoutRefreshRequested, this, [this]() {
                // 方向控件内部页面高度变化（如从 X/Y/Z 轴切换到自定义坐标）时，
                // 通知外层折叠块刷新布局，避免内容被挤压或留白。
                if (m_foldWidgets.size() > 2)
                {
                    m_foldWidgets[2]->refreshLayout();
                }
                });

            f->addRow(_directionWidget);
            m_foldWidgets.append(f);
        }

        // ================== 3. 属性 ==================
        {
            FoldSection* f = new FoldSection("属性", this);
            QButtonGroup* btnGroup = new QButtonGroup(this);

            // 第一行：自动计算（双列布局，右列占位，与第二行单选按钮左对齐）
            rb_autoCalc = new QRadioButton("自动计算", this);
            rb_autoCalc->setFixedWidth(90);
            btnGroup->addButton(rb_autoCalc, 0);
            QWidget* emptyWidget = new QWidget(this); // 右列占位控件
            f->addRow(rb_autoCalc, emptyWidget);

            // 第二行：手动指定 + 输入框（标准双列布局，与上方控件对齐）
            rb_manualSpecify = new QRadioButton("手动指定", this);
            rb_manualSpecify->setFixedWidth(90);
            btnGroup->addButton(rb_manualSpecify, 1);
            le_manualValue = new QLineEdit(this);
            le_manualValue->setPlaceholderText("请输入正数");
            le_manualValue->setEnabled(false);
            le_manualValue->setFixedHeight(30);
            f->addRow(rb_manualSpecify, le_manualValue);

            // 第三行：调整复选框
            cb_adjust = new QCheckBox("调整", this);
            f->addRow(cb_adjust);

            m_foldWidgets.append(f);

            // 信号连接（同上，不变）
            connect(rb_autoCalc, &QRadioButton::toggled, this, &mPeriodicBoundaryPanel::slot_autoCalcToggled);
            connect(rb_manualSpecify, &QRadioButton::toggled, this, [=](bool checked) {
                le_manualValue->setEnabled(checked);
                if (checked && _pbData) {
                    le_manualValue->setText(QString::number(_pbData->getManualValue()));
                }
                });
            connect(le_manualValue, &QLineEdit::editingFinished, this, &mPeriodicBoundaryPanel::slot_manualValueEditingFinished);
            connect(cb_adjust, &QCheckBox::toggled, this, &mPeriodicBoundaryPanel::slot_adjustToggled);
        }
    }

    void mPeriodicBoundaryPanel::newPeriodicBoundaryPanel(MxFunctions::PeriodicBoundaryType type)
    {
        if (!getCVM) return;
        if (getCVM->getPreRenderWidget() == nullptr) return;

        uneditPeriodicBoundaryPanel();

        auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
        auto& setManager = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set);

        int pbID = getCVM->getModelData()->getManager<MechContactData::mPeriodicBoundaryManager>(Physical_PeriodicBoundary).createID();
        _pbData = new MechContactData::mPeriodicBoundary(pbID, type);

        QString nameBase = (type == PeriodicBoundary_Translation) ? "TranslateBoundary" : "RotateBoundary";
        QString name = getCVM->getModelData()->createNameByType(Physical_PeriodicBoundary, nameBase);
        _pbData->setPeriodicBoundaryName(name, false);

        getCVM->getModelData()->addPhysicalField<MechContactData::mPeriodicBoundary>(_pbData);
        setEditing(true);

        // 创建主面/从面内部混合集合，并添加映射
        int masterSetId = setManager.createID();
        mMixSet* masterSet = new mMixSet(masterSetId, SetType::Mix, QString("%1_%2").arg("主面").arg(_pbData->getPeriodicBoundaryName()), true);
        getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(masterSet);
        mapRelationManager.appendMapRelation({ Physical_PeriodicBoundary, pbID }, { Physical_Set, masterSetId }, MapRelation_Master);

        int slaveSetId = setManager.createID();
        mMixSet* slaveSet = new mMixSet(slaveSetId, SetType::Mix, QString("%1_%2").arg("从面").arg(_pbData->getPeriodicBoundaryName()), true);
        getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(slaveSet);
        mapRelationManager.appendMapRelation({ Physical_PeriodicBoundary, pbID }, { Physical_Set, slaveSetId }, MapRelation_Slave);

        loadDataToUI();

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdatePeriodicBoundarySig(true, pbID);
    }

    void mPeriodicBoundaryPanel::editPeriodicBoundaryPanel(int pbID)
    {
        if (!getCVM) return;
        if (pbID <= 0) return;

        _pbData = getCVM->getModelData()->getManager<MechContactData::mPeriodicBoundaryManager>(Physical_PeriodicBoundary).getPeriodicBoundaryData(pbID);
        if (!_pbData) return;
        setEditing(true);

        loadDataToUI();
    }

    void mPeriodicBoundaryPanel::onPeriodicBoundaryDataAboutToBeDestroyed(int pbID)
    {
        //必须在数据对象 delete 之前调用：此时 _pbData 仍有效，安全丢弃悬垂指针
        if (pbID >= 0)
        {
            if (_pbData == nullptr || pbID != _pbData->getPeriodicBoundaryId())
                return;
        }
        _pbData = nullptr;
    }

    void mPeriodicBoundaryPanel::uneditPeriodicBoundaryPanel()
    {
        if (!isEditing())
        {
            return;
        }
        //入口统一复位编辑态：后续各早退分支（数据为空/已删除）无需再单独清标志
        setEditing(false);
        if (!getCVM || !getCVM->getPreRenderWidget())
        {
            return;
        }
        //_pbData 为裸指针，数据被删除时由 onPeriodicBoundaryDataAboutToBeDestroyed 提前置空，
        //此处 nullptr 即表示数据已删，直接退出不走切出收尾
        if (_pbData == nullptr)
        {
            return;
        }
        // 切出/取消编辑面板：失活不保留临时高亮，清空高亮
        for (auto* ctrl : { pick_master, pick_slave })
        {
            if (!ctrl) continue;
            ctrl->setKeepHighlightOnDeactivate(false);
            const bool wasActive = ctrl->isPickActive();
            ctrl->setPickActive(false);
            if (!wasActive)
            {
                ctrl->clearAllHighlight();
            }
            ctrl->setKeepHighlightOnDeactivate(true);
        }

        // 恢复拾取工具栏全部按钮
        emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>(10, true));

        _pbData = nullptr;
        clearFaceHighlight();

        // 清除方向箭头渲染
        if (getCVM && getCVM->getPreRenderWidget())
        {
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController)
                symbolController->deleteSymbol({ s_dirArrowRenderKey });
        }
    }

    void mPeriodicBoundaryPanel::loadDataToUI()
    {
        if (!_pbData) return;

        // 类型
        cmb_type->blockSignals(true);
        cmb_type->setCurrentIndex(_pbData->getPeriodicBoundaryType() == PeriodicBoundary_Translation ? 0 : 1);
        cmb_type->blockSignals(false);

        // 主面
        if (pick_master)
        {
            PickWidgetData masterData;
            loadPeriodicBoundaryDataFromSet(MapRelation_Master, masterData);
            auto strategy = std::make_shared<MapRelationPersistenceStrategy>(
                MxFunctions::Physical_PeriodicBoundary,
                _pbData->getPeriodicBoundaryId(), MapRelation_Master);
            pick_master->setPersistenceStrategy(strategy);
            pick_master->setSetResolver(&resolveSetToTypedData);
            QHash<QString, QString> setToType = getSetNamesList({ SetType::GeoFace, SetType::ElementSurface });
            pick_master->setSelectSetList(setToType);
            pick_master->displayData(masterData);
            //判断拾取控件是否为空 以设置标志位
            _pbData->setPickStateMaster(pick_master->getAllData().isEmpty() ? 1 : 2);
        }

        // 从面
        if (pick_slave)
        {
            PickWidgetData slaveData;
            loadPeriodicBoundaryDataFromSet(MapRelation_Slave, slaveData);
            auto strategy = std::make_shared<MapRelationPersistenceStrategy>(
                MxFunctions::Physical_PeriodicBoundary,
                _pbData->getPeriodicBoundaryId(), MapRelation_Slave);
            pick_slave->setPersistenceStrategy(strategy);
            pick_slave->setSetResolver(&resolveSetToTypedData);
            QHash<QString, QString> setToType = getSetNamesList({ SetType::GeoFace, SetType::ElementSurface });
            pick_slave->setSelectSetList(setToType);
            pick_slave->displayData(slaveData);
            //判断拾取控件是否为空 以设置标志位
            _pbData->setPickStateSlave(pick_slave->getAllData().isEmpty() ? 1 : 2);
        }

        // 反显方向控件完整 UI 状态
        restoreDirectionWidgetFromPbData();

        updateDirectionArrowRender();
        updateFoldVisibility();

        // 默认激活主面拾取
        if (pick_master && !pick_master->isPickActive() && !pick_slave->isPickActive())
        {
            pick_master->setPickActive(true);
        }

        // 加载属性设置到界面
        if (rb_autoCalc && rb_manualSpecify && le_manualValue && cb_adjust)
        {
            rb_autoCalc->blockSignals(true);
            rb_manualSpecify->blockSignals(true);
            le_manualValue->blockSignals(true);
            cb_adjust->blockSignals(true);

            bool autoCalc = _pbData->isAutoCalc();
            rb_autoCalc->setChecked(autoCalc);
            rb_manualSpecify->setChecked(!autoCalc);
            le_manualValue->setEnabled(!autoCalc);
            le_manualValue->setText(QString::number(_pbData->getManualValue()));
            cb_adjust->setChecked(_pbData->isAdjust());

            rb_autoCalc->blockSignals(false);
            rb_manualSpecify->blockSignals(false);
            le_manualValue->blockSignals(false);
            cb_adjust->blockSignals(false);
        }

        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_PeriodicBoundary, _pbData->getPeriodicBoundaryId());

        // 数据反显完成后刷新一次方向箭头渲染。
        // 直接调用时面板尚未完成布局/显示，渲染上下文可能未就绪，
        // 使用 QueuedConnection 延迟到事件循环下一阶段执行，确保首次进入面板也能正确渲染。
        QMetaObject::invokeMethod(this, [this]() {
            updateDirectionArrowRender();
            if (getCVM && getCVM->getPreRenderWidget())
            {
                auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
                if (symbolController)
                    symbolController->UpdateAllSymbol();
                getCVM->getPreRenderWidget()->updatePaint();
            }
            }, Qt::QueuedConnection);

    }

    void mPeriodicBoundaryPanel::updatePanel()
    {
        loadDataToUI();
    }

    void mPeriodicBoundaryPanel::updateFoldVisibility()
    {
        // 先全部移除/隐藏
        for (auto* f : m_foldWidgets)
        {
            _mainLayout->removeWidget(f);
            f->hide();
        }

        // 周期边界面板所有折叠块始终显示，按顺序插入到 stretch 之前
        for (auto* f : m_foldWidgets)
        {
            f->show();
            _mainLayout->insertWidget(_mainLayout->count() - 1, f);
        }

        updateRowVisibility();
    }

    void mPeriodicBoundaryPanel::updateRowVisibility()
    {
        bool isTranslation = (_pbData && _pbData->getPeriodicBoundaryType() == PeriodicBoundary_Translation);
        if (_directionWidget)
        {
            _directionWidget->setMagnitudeLabel(isTranslation ? QStringLiteral("平移距离") : QStringLiteral("周向对称数量"));
        }
    }

    void mPeriodicBoundaryPanel::loadPeriodicBoundaryDataFromSet(MxFunctions::MapRelationType mapType, PickWidgetData& outData)
    {
        outData.clear();
        if (!_pbData) return;

        auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
        int setId = mapRelationManager.getMapRelation<int>(
            { PhysicalFieldType::Physical_PeriodicBoundary, _pbData->getPeriodicBoundaryId() }, Physical_Set, mapType);
        if (setId == -1) return;

        mSetBase* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setId);
        if (!set) return;

        if (set->getSetType() == Mix)
        {
            auto mixset = dynamic_cast<mMixSet*>(set);
            if (mixset)
            {
                outData.pickedData = mixset->getMixSetData();
            }
        }
        else if (set->getSetType() == GeoFace || set->getSetType() == ElementSurface)
        {
            outData.selectedSetNames.insert(set->getSetName());
        }
    }

    void mPeriodicBoundaryPanel::clearFaceHighlight()
    {
        if (!getCVM)
            return;
        auto widget_ptr = getCVM->getPreRenderWidget();
        if (!widget_ptr)
            return;
        auto highlight_controller = widget_ptr->getHighlightController();
        if (!highlight_controller)
            return;

        // 清除主面红色高亮
        auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("red"), true, -2.0f);
        if (geoHighLightRender)
        {
            mGeoSelectData* geoData = new mGeoSelectData();
            geoData->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoFace, {});
            geoHighLightRender->updateHighLightRender(geoData);
        }

        auto meshHighLightRender = highlight_controller->getExtraPreMeshHighLightRender(std::string("red"));
        if (meshHighLightRender)
        {
            MeshSelectMemoryData* meshData = new MeshSelectMemoryData();
            meshData->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshFace, {});
            meshHighLightRender->updateHighLightRender(meshData);
        }

        // 清除从面蓝色高亮
        geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("blue"), true, -2.0f);
        if (geoHighLightRender)
        {
            mGeoSelectData* geoData = new mGeoSelectData();
            geoData->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoFace, {});
            geoHighLightRender->updateHighLightRender(geoData);
        }

        meshHighLightRender = highlight_controller->getExtraPreMeshHighLightRender(std::string("blue"));
        if (meshHighLightRender)
        {
            MeshSelectMemoryData* meshData = new MeshSelectMemoryData();
            meshData->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshFace, {});
            meshHighLightRender->updateHighLightRender(meshData);
        }
    }

    void mPeriodicBoundaryPanel::slot_commitMasterPeriodicBoundaryData()
    {
        if (!getCVM)
        {
            return;
        }
        auto widget_ptr = getCVM->getPreRenderWidget();
        if (!widget_ptr) return;
        auto highlight_controller = widget_ptr->getHighlightController();
        if (!highlight_controller) return;

        auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("red"), true, -2.0f);
        geoHighLightRender->setHighLightColor(255, 0, 0);
        mGeoSelectData* geoData = new mGeoSelectData();
        geoData->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoFace, {});

        auto meshHighLightRender = highlight_controller->getExtraPreMeshHighLightRender(std::string("red"));
        meshHighLightRender->setHighLightColor(255, 0, 0);
        MeshSelectMemoryData* meshData = new MeshSelectMemoryData();
        meshData->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshFace, {});

        std::vector<RenderPickResultData> meshRenderData;
        std::vector<int> geoRenderData;
        AllRenderSymbolManager::getInstance()->rendContact(pick_master->getAllData(), meshRenderData, geoRenderData);

        if (geoHighLightRender && geoRenderData.size() > 0)
        {
            geoData->UpdatePickData(mxr::SelectionOperate::ADD, SelectionInfoType::GeoFace, geoRenderData);
        }

        if (meshHighLightRender && meshRenderData.size() > 0)
        {
            std::vector<mxr::MeshSelectInfoData> meshIds;
            meshIds.reserve(meshRenderData.size());
            for (const auto& d : meshRenderData)
            {
                mxr::MeshSelectInfoData n;
                n.data_id.element_id = d.result_id;
                n.data_index.line_index = d.result_id2;
                n.part_id = d.part_id;
                meshIds.push_back(n);
            }
            meshData->UpdateSelectDataByType(mxr::SelectionOperate::ADD, SelectionInfoType::MeshFace, meshIds);
        }

        geoHighLightRender->updateHighLightRender(geoData);
        meshHighLightRender->updateHighLightRender(meshData);
        delete geoData;
        delete meshData;

        //判断拾取控件是否为空 以设置标志位
        _pbData->setPickStateMaster(pick_master->getAllData().isEmpty() ? 1 : 2);
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_PeriodicBoundary, _pbData->getPeriodicBoundaryId());
    }

    void mPeriodicBoundaryPanel::slot_commitSlavePeriodicBoundaryData()
    {
        if (!getCVM)
        {
            return;
        }
        auto widget_ptr = getCVM->getPreRenderWidget();
        if (!widget_ptr) return;
        auto highlight_controller = widget_ptr->getHighlightController();
        if (!highlight_controller) return;

        auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("blue"), true, -2.0f);
        geoHighLightRender->setHighLightColor(0, 0, 255);
        mGeoSelectData* geoData = new mGeoSelectData();
        geoData->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoFace, {});

        auto meshHighLightRender = highlight_controller->getExtraPreMeshHighLightRender(std::string("blue"));
        meshHighLightRender->setHighLightColor(0, 0, 255);
        MeshSelectMemoryData* meshData = new MeshSelectMemoryData();
        meshData->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshFace, {});

        std::vector<RenderPickResultData> meshRenderData;
        std::vector<int> geoRenderData;
        AllRenderSymbolManager::getInstance()->rendContact(pick_slave->getAllData(), meshRenderData, geoRenderData);

        if (geoHighLightRender && geoRenderData.size() > 0)
        {
            geoData->UpdatePickData(mxr::SelectionOperate::ADD, SelectionInfoType::GeoFace, geoRenderData);
        }

        if (meshHighLightRender && meshRenderData.size() > 0)
        {
            std::vector<mxr::MeshSelectInfoData> meshIds;
            meshIds.reserve(meshRenderData.size());
            for (const auto& d : meshRenderData)
            {
                mxr::MeshSelectInfoData n;
                n.data_id.element_id = d.result_id;
                n.data_index.line_index = d.result_id2;
                n.part_id = d.part_id;
                meshIds.push_back(n);
            }
            meshData->UpdateSelectDataByType(mxr::SelectionOperate::ADD, SelectionInfoType::MeshFace, meshIds);
        }

        geoHighLightRender->updateHighLightRender(geoData);
        meshHighLightRender->updateHighLightRender(meshData);
        delete geoData;
        delete meshData;
        //判断拾取控件是否为空 以设置标志位
        _pbData->setPickStateSlave(pick_slave->getAllData().isEmpty() ? 1 : 2);
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_PeriodicBoundary, _pbData->getPeriodicBoundaryId());
    }

    void mPeriodicBoundaryPanel::slot_typeComboBoxChanged(int index)
    {
        if (!_pbData) return;

        MxFunctions::PeriodicBoundaryType newType = (index == 0) ? PeriodicBoundary_Translation : PeriodicBoundary_Rotation;
        if (newType == _pbData->getPeriodicBoundaryType()) return;

        // 自动重命名（如果用户未手动重命名）
        if (!_pbData->getIsUserRename())
        {
            QString nameBase = (newType == PeriodicBoundary_Translation) ? "TranslateBoundary" : "RotateBoundary";
            QString name = getCVM->getModelData()->createNameByType(Physical_PeriodicBoundary, nameBase);
            _pbData->setPeriodicBoundaryName(name, false);
        }

        _pbData->setPeriodicBoundaryType(newType);
        // 切换类型后校验数值合法性
        bool ok;
        double val = _pbData->value.toDouble(&ok);
        if (newType == PeriodicBoundary_Translation)
        {
            if (!ok || val < 0) _pbData->value = "0";
        }
        else
        {
            if (!ok || val < 1)
                _pbData->value = "1";
            else
                _pbData->value = QString::number(static_cast<int>(val));
        }
        // 反显修正后的值
        _directionWidget->setMagnitudeValue(_pbData->value);

        updateRowVisibility();

        // 平移/旋转切换后刷新方向箭头，确保箭头长度/样式同步更新
        updateDirectionArrowRender();
        if (getCVM && getCVM->getPreRenderWidget())
        {
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController)
                symbolController->UpdateAllSymbol();
            getCVM->getPreRenderWidget()->updatePaint();
        }

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdatePeriodicBoundarySig(false, _pbData->getPeriodicBoundaryId());
    }

    const QVector3D mPeriodicBoundaryPanel::s_dirArrowColor = QVector3D(19.0f, 133.0f, 246.0f);

    void mPeriodicBoundaryPanel::saveDirectionWidgetToPbData()
    {
        if (!_pbData || !_directionWidget)
            return;

        // 保存方向控件完整 UI 状态
        DirectionPickState& state = _pbData->dirPickState;
        state.mode = static_cast<int>(_directionWidget->directionMode());
        state.reversed = _directionWidget->isReversed();

        // 统一计算起点/终点：所有模式都分解为两个点
        QVector3D startPt(0, 0, 0);
        QVector3D endPt(1, 0, 0);

        if (state.mode == mDirectionSelectorWidget::CustomCoordinate)
        {
            startPt = _directionWidget->startPoint();
            endPt = _directionWidget->endPoint();
        }
        else if (state.mode == mDirectionSelectorWidget::SelectObject)
        {
            // 选择对象模式：起点为原点，终点为方向向量
            QVariant dirVar = _directionWidget->getDirection();
            if (dirVar.isValid() && !dirVar.isNull())
            {
                endPt = dirVar.value<QVector3D>();
            }

            PickWidgetData pickData = _directionWidget->objectPickController()->getAllData();
            if (!pickData.pickedData.empty())
            {
                auto it = pickData.pickedData.begin();
                state.selectedObject.first = it->first;
                if (!it->second.empty())
                {
                    state.selectedObject.second = it->second.front();
                }
            }
        }
        else if (state.mode == mDirectionSelectorWidget::DirectInput)
        {
            QVariant dirVar = _directionWidget->getDirection();
            if (dirVar.isValid() && !dirVar.isNull())
            {
                endPt = dirVar.value<QVector3D>();
            }
            state.directInputDir = endPt;
        }
        else if (state.mode == mDirectionSelectorWidget::XAxis)
        {
            endPt = QVector3D(1, 0, 0);
        }
        else if (state.mode == mDirectionSelectorWidget::YAxis)
        {
            endPt = QVector3D(0, 1, 0);
        }
        else if (state.mode == mDirectionSelectorWidget::ZAxis)
        {
            endPt = QVector3D(0, 0, 1);
        }

        // 处理反向：非自定义坐标模式下，方向向量取反
        if (state.reversed && state.mode != mDirectionSelectorWidget::CustomCoordinate)
        {
            endPt = -endPt;
        }

        // 兜底：保证至少有一个有效方向点
        if (startPt.lengthSquared() < 1e-12f && endPt.lengthSquared() < 1e-12f)
        {
            endPt = QVector3D(1, 0, 0);
        }

        _pbData->startPoint = startPt;
        _pbData->endPoint = endPt;

        // 自定义坐标模式下同时保存起点/终点到 state，便于反显
        if (state.mode == mDirectionSelectorWidget::CustomCoordinate)
        {
            state.startPoint = QVariant::fromValue(startPt);
            state.endPoint = QVariant::fromValue(endPt);
        }
    }

    void mPeriodicBoundaryPanel::restoreDirectionWidgetFromPbData()
    {
        if (!_pbData || !_directionWidget)
            return;

        const DirectionPickState& state = _pbData->dirPickState;

        // 阻断信号，避免反显过程中触发保存
        bool oldSignalsBlocked = _directionWidget->blockSignals(true);

        _directionWidget->setDirectionMode(
            static_cast<mDirectionSelectorWidget::DirectionMode>(state.mode));
        _directionWidget->setReversed(state.reversed);

        // 自定义坐标反显
        if (state.mode == mDirectionSelectorWidget::CustomCoordinate &&
            state.startPoint.isValid() && !state.startPoint.isNull() &&
            state.endPoint.isValid() && !state.endPoint.isNull())
        {
            _directionWidget->setCustomCoordinatePoints(
                state.startPoint.value<QVector3D>(),
                state.endPoint.value<QVector3D>());
        }
        // 选择对象反显
        else if (state.mode == mDirectionSelectorWidget::SelectObject)
        {
            PickWidgetData displayData;
            if (state.selectedObject.second.result_id > 0)
            {
                displayData.pickedData[state.selectedObject.first].push_back(state.selectedObject.second);
            }
            _directionWidget->objectPickController()->displayData(displayData);
        }
        // 直接输入反显
        else if (state.mode == mDirectionSelectorWidget::DirectInput)
        {
            _directionWidget->setDirection(state.directInputDir);
        }
        // 坐标轴模式：根据已保存的方向点推导方向
        else
        {
            QVector3D startPt = _pbData->startPoint;
            QVector3D endPt = _pbData->endPoint;
            if (startPt.lengthSquared() < 1e-12f && endPt.lengthSquared() < 1e-12f)
            {
                endPt = QVector3D(1, 0, 0);
            }
            _directionWidget->setCustomCoordinatePoints(startPt, endPt);
        }

        _directionWidget->setMagnitudeValue(_pbData->value);

        _directionWidget->blockSignals(oldSignalsBlocked);
    }

    void mPeriodicBoundaryPanel::updateDirectionArrowRender()
    {
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (!symbolController)
            return;

        // 清理旧箭头
        symbolController->deleteSymbol({ s_dirArrowRenderKey });

        if (!_pbData)
            return;

        QVector3D startPt = _pbData->startPoint;
        QVector3D endPt = _pbData->endPoint;
        if (startPt.lengthSquared() < 1e-12f && endPt.lengthSquared() < 1e-12f)
        {
            endPt = QVector3D(1, 0, 0);
        }

        QVector3D dir = endPt - startPt;
        if (dir.lengthSquared() < 1e-12f)
            return;

        QVector<std::pair<QVector3D, QVector3D>> pos_dir;
        pos_dir.push_back({ startPt, dir });

        const auto& arrow = symbolController->getOrCreateSymbolData<Arrow_Font_Load_Implicit>(s_dirArrowRenderKey);
        arrow->_pos_dir = pos_dir;
        arrow->_txt = "";
        arrow->_color = s_dirArrowColor;
        arrow->_depthState = false;
        arrow->_isFixedPixel = false;

        bool isTranslation = (_pbData->getPeriodicBoundaryType() == PeriodicBoundary_Translation);
        if (isTranslation)
        {
            // 平移边界：箭头长度等于两点实际距离
            arrow->_size = dir.length();
        }
        else
        {
            // 旋转边界：固定长度
            arrow->_size = 30.0;
        }

        symbolController->createSymbol(arrow);

        if (!isTranslation)
        {
            symbolController->symbolSelfAdaptiveSize(getCVM->getPreRenderWidget(), { s_dirArrowRenderKey }, 0.05);
        }
    }

    void mPeriodicBoundaryPanel::slot_autoCalcToggled(bool checked)
    {
        if (!_pbData) return;
        _pbData->setAutoCalc(checked);
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_PeriodicBoundary, _pbData->getPeriodicBoundaryId());
    }

    void mPeriodicBoundaryPanel::slot_manualValueEditingFinished()
    {
        if (!_pbData) return;
        bool ok;
        double val = le_manualValue->text().toDouble(&ok);
        // 限制正数，输入不合法修正为1.0
        if (!ok || val <= 0) {
            val = 1.0;
            le_manualValue->setText(QString::number(val));
        }
        _pbData->setManualValue(val);
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_PeriodicBoundary, _pbData->getPeriodicBoundaryId());
    }

    void mPeriodicBoundaryPanel::slot_adjustToggled(bool checked)
    {
        if (!_pbData) return;
        _pbData->setAdjust(checked);
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_PeriodicBoundary, _pbData->getPeriodicBoundaryId());
    }
}