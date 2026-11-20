

#include "mMotionBodyPanel.h"
#include "mMotionRefMap.h"
#include "mMotionPickerActivate.h"
#include "mCalcLineEdit.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"
#include "mMotionPanelColors.h"

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

//Geo
#include "mGeoEngineData.h"
#include "mGeoEngineHandle.h"

//Qt
#include <QComboBox>
#include <QEvent>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QGridLayout>
#include <QCompleter>
#include <QStringListModel>
#include <QPointer>
#include <functional>
using namespace MechPreData;
using namespace MechMotionData;
using namespace MxFunctions;
using namespace mxr;

namespace MechMainWindow
{
    mMotionBodyPanel::mMotionBodyPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        m_motionBodyData = nullptr;

        //获取当前视口的数据
        m_mainLayout = new QVBoxLayout(this);

        // ==================== 1. 运动体对象：已分配部件列表（截面属性面板 mPropertyPanel 同款） ====================
        // 列表多选 + 右侧竖排 全部/增加/减少/清空；"增加"为行内编辑 + 名称自动补全选部件
        FoldSection* foldObject = new FoldSection("运动体对象", this);
        QGridLayout* assignLayout = new QGridLayout();
        m_listAssignPart = new QListWidget(this);
        m_listAssignPart->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_btnAllPart = new QToolButton();
        m_btnAllPart->setIcon(QIcon(":/mMainWidgets/Icon/xuanzequanbu.png"));
        m_btnAllPart->setToolTip("全部部件");
        m_btnAddPart = new QToolButton();
        m_btnAddPart->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        m_btnAddPart->setToolTip("增加部件");
        m_btnReducePart = new QToolButton();
        m_btnReducePart->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        m_btnReducePart->setToolTip("减少部件");
        m_btnClearPart = new QToolButton();
        m_btnClearPart->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png"));
        m_btnClearPart->setToolTip("清空部件");

        assignLayout->addWidget(m_listAssignPart, 0, 0, 8, 7);
        assignLayout->addWidget(m_btnAllPart, 0, 7);
        assignLayout->addWidget(m_btnAddPart, 1, 7);
        assignLayout->addWidget(m_btnReducePart, 2, 7);
        assignLayout->addWidget(m_btnClearPart, 3, 7);
        foldObject->addRow(assignLayout);
        m_mainLayout->addWidget(foldObject);

        connect(m_btnAllPart, &QToolButton::clicked, this, &mMotionBodyPanel::slot_allPartBtn);
        connect(m_btnAddPart, &QToolButton::clicked, this, &mMotionBodyPanel::slot_addPartBtn);
        connect(m_btnReducePart, &QToolButton::clicked, this, &mMotionBodyPanel::slot_reducePartBtn);
        connect(m_btnClearPart, &QToolButton::clicked, this, &mMotionBodyPanel::slot_clearPartBtn);

        // 列表的"激活"入口：点列表任意位置（含空白处，经 viewport 事件过滤）即接管拾取会话
        // （质心拾取让位），之后视口拾取/右键取消作用于部件
        m_listAssignPart->viewport()->installEventFilter(this);


        // ==================== 2. 质量属性选项与数值 ====================
        FoldSection* foldMass = new FoldSection("质量和惯性矩", this);

        // 下拉框：自动 / 自定义
        m_cmbMassOption = new QComboBox();
        m_cmbMassOption->addItem("由几何体自动计算");
        m_cmbMassOption->addItem("用户自定义");
        foldMass->addRow(new QLabel("属性选项:"), m_cmbMassOption);

        auto makeCalcLineEdit = [](bool nonNegative) {
            MxFunctions::mCalcLineEdit* le = new MxFunctions::mCalcLineEdit();
            le->setExpression("0");
            le->setEnabled(false); // 默认禁用，因为默认是"自动计算"
            le->setNonNegative(nonNegative);
            return le;
            };

        m_leMass = makeCalcLineEdit(true); // 质量非负
        m_leIxx = makeCalcLineEdit(false);
        m_leIyy = makeCalcLineEdit(false);
        m_leIzz = makeCalcLineEdit(false);
        m_leIxy = makeCalcLineEdit(false);
        m_leIxz = makeCalcLineEdit(false);
        m_leIyz = makeCalcLineEdit(false);

        // 使用通用点坐标拾取控件作为“质心坐标”输入
        m_cmPicker = new mPointPickerWidget(QStringLiteral("质心坐标 (mm)"), this);
        m_cmPicker->pickController()->setAllowedTypes(&MechCommon::pickGeoPointEdgeFaceSolidNodePart);
        m_cmPicker->setWidgetEnabled(false); // 默认禁用，因为默认是”自动计算”
        foldMass->addRow(m_cmPicker);

        // 质心控件一旦被激活（含用户点它的拾取按钮），部件列表让位：
        // 可视挂起 + 部件从激活选择集高亮降级为独立层临时高亮（保留归属可见性）
        connect(m_cmPicker, &mPointPickerWidget::pickControllerActivated, this, [this](mPickWidgetController*) {
            setPartPickActiveVisual(false);
            clearPartSelection();
            renderPartTempHighlight();
            });

        foldMass->addRow(new QLabel("质量 (kg):"), m_leMass);
        QLabel* inertiaTip = new QLabel("惯性矩 (kg·mm²):");
        inertiaTip->setToolTip(QStringLiteral("关于质心的惯性张量分量，kg·mm²"));
        foldMass->addRow(inertiaTip, m_leIxx);
        foldMass->addRow(new QLabel("Iyy:"), m_leIyy);
        foldMass->addRow(new QLabel("Izz:"), m_leIzz);
        foldMass->addRow(new QLabel("Ixy:"), m_leIxy);
        foldMass->addRow(new QLabel("Ixz:"), m_leIxz);
        foldMass->addRow(new QLabel("Iyz:"), m_leIyz);

        m_mainLayout->addWidget(foldMass);


        // ==================== 3. 设置 ====================
        FoldSection* foldSettings = new FoldSection("设置", this);
        m_chkIsFixed = new QCheckBox("不使用运动副而固定运动体");
        foldSettings->addRow(m_chkIsFixed);

        // 初始线速度（全局坐标系），用于给运动体赋初速（如滑块滑动算例）
        auto makeVelEdit = []() {
            auto* edit = new MxFunctions::mCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard);
            edit->setExpression("0");
            return edit;
        };
        m_leInitVelX = makeVelEdit();
        m_leInitVelY = makeVelEdit();
        m_leInitVelZ = makeVelEdit();
        foldSettings->addRow(new QLabel("初速度X (mm/s):"), m_leInitVelX);
        foldSettings->addRow(new QLabel("初速度Y (mm/s):"), m_leInitVelY);
        foldSettings->addRow(new QLabel("初速度Z (mm/s):"), m_leInitVelZ);

        // 初始角速度（全局坐标系，°/s）
        m_leInitOmegaX = makeVelEdit();
        m_leInitOmegaY = makeVelEdit();
        m_leInitOmegaZ = makeVelEdit();
        foldSettings->addRow(new QLabel("初角速度X (°/s):"), m_leInitOmegaX);
        foldSettings->addRow(new QLabel("初角速度Y (°/s):"), m_leInitOmegaY);
        foldSettings->addRow(new QLabel("初角速度Z (°/s):"), m_leInitOmegaZ);
        m_mainLayout->addWidget(foldSettings);

        m_mainLayout->addStretch();


        //连接信号
        // 2. 固定选项修改
        connect(m_chkIsFixed, &QCheckBox::toggled, this, [this](bool checked) {
            if (m_motionBodyData)
                m_motionBodyData->setIsFixed(checked);

            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionBodySig(false, m_motionBodyData->getId());
            });

        // 2.5 初始线速度（editingFinished 写回）
        auto velChanged = [this](QLineEdit* edit, int axis) {
            if (!m_motionBodyData) return;
            QVector3D vel = m_motionBodyData->getInitialVelocity();
            const double v = edit->text().toDouble();
            if (axis == 0) vel.setX(v); else if (axis == 1) vel.setY(v); else vel.setZ(v);
            m_motionBodyData->setInitialVelocity(vel);
            };
        connect(m_leInitVelX, &QLineEdit::editingFinished, this, [this, velChanged]() { velChanged(m_leInitVelX, 0); });
        connect(m_leInitVelY, &QLineEdit::editingFinished, this, [this, velChanged]() { velChanged(m_leInitVelY, 1); });
        connect(m_leInitVelZ, &QLineEdit::editingFinished, this, [this, velChanged]() { velChanged(m_leInitVelZ, 2); });

        // 初始角速度（editingFinished 写回，°/s 全局系）
        auto omegaChanged = [this](QLineEdit* edit, int axis) {
            if (!m_motionBodyData) return;
            QVector3D omega = m_motionBodyData->getInitialAngularVelocity();
            const double w = edit->text().toDouble();
            if (axis == 0) omega.setX(w); else if (axis == 1) omega.setY(w); else omega.setZ(w);
            m_motionBodyData->setInitialAngularVelocity(omega);
            };
        connect(m_leInitOmegaX, &QLineEdit::editingFinished, this, [this, omegaChanged]() { omegaChanged(m_leInitOmegaX, 0); });
        connect(m_leInitOmegaY, &QLineEdit::editingFinished, this, [this, omegaChanged]() { omegaChanged(m_leInitOmegaY, 1); });
        connect(m_leInitOmegaZ, &QLineEdit::editingFinished, this, [this, omegaChanged]() { omegaChanged(m_leInitOmegaZ, 2); });

        // 3. 质量选项下拉框切换
        connect(m_cmbMassOption, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            bool isCustom = (index == 1);

            // --- 新增：将模式状态写入数据类 ---
            if (m_motionBodyData) {
                m_motionBodyData->setIsCustomMassProps(isCustom);
            }

            // 控制控件的亮/灰显
            m_leMass->setEnabled(isCustom);
            m_cmPicker->setWidgetEnabled(isCustom);
            m_leIxx->setEnabled(isCustom); m_leIyy->setEnabled(isCustom); m_leIzz->setEnabled(isCustom);
            m_leIxy->setEnabled(isCustom); m_leIxz->setEnabled(isCustom); m_leIyz->setEnabled(isCustom);

            // 如果切回了"自动计算"，且当前有选中的几何体，立刻触发计算并同步
            if (!isCustom && m_motionBodyData && !mMotionRefMap::getSet(Physical_MotionBody, m_motionBodyData->getId(), Physical_Part).empty()) {
                performAutoCalculation();
            }

            // 3. 将数据反向刷新到界面上让用户看到
            updateUIFromData();

            // 自定义模式激活质心拾取，自动计算模式下控件禁用自动失活
            activateFirstVisiblePointPicker({ m_cmPicker });

            // 切回"自动计算"后把拾取会话交还部件列表（质心控件已因禁用失活）
            if (!isCustom)
                activatePartPick();
            });

        // 4. 质量数值实时同步
        connect(m_leMass, &QLineEdit::editingFinished, this, [this]() {
            if (m_motionBodyData) m_motionBodyData->setMass(m_leMass->text().toDouble());
            });

        // 5. 质心坐标实时同步 (通用点坐标拾取控件)
        connect(m_cmPicker, &mPointPickerWidget::coordinateChanged, this, [this]() {
            if (m_motionBodyData) {
                const QVector3D pt = m_cmPicker->coordinate();
                m_motionBodyData->setCenterOfMass(pt);
                if (getCVM && getCVM->getPreRenderWidget())
                {
                    auto commonShapeRender = getCVM->getPreRenderWidget()->getCommonShapeRender();
                    if (commonShapeRender)
                    {
                        //注意：拾取信号链内严禁 ClearPreWidget*Pick/UpdatePreWidgetHighlight，
                        //否则激活中的拾取控件会被空捕获清掉刚拾取的数据与高亮
                        commonShapeRender->appendPoint("massPoint", { pt },
                            mMotionPanelColors::vec(mMotionPanelColors::pointA()));
                        getCVM->getPreRenderWidget()->updatePaint();
                    }
                }
                emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionBodySig(false, m_motionBodyData->getId());
            }
            });

        // 6. 惯性矩实时同步
        auto updateInertiaLogic = [this]() {
            if (m_motionBodyData) {
                m_motionBodyData->setInertia({
                m_leIxx->text().toDouble(), m_leIyy->text().toDouble(), m_leIzz->text().toDouble(),
                m_leIxy->text().toDouble(), m_leIxz->text().toDouble(), m_leIyz->text().toDouble()
                    });
            }
            };

        connect(m_leIxx, &QLineEdit::editingFinished, this, [updateInertiaLogic]() { updateInertiaLogic(); });
        connect(m_leIyy, &QLineEdit::editingFinished, this, [updateInertiaLogic]() { updateInertiaLogic(); });
        connect(m_leIzz, &QLineEdit::editingFinished, this, [updateInertiaLogic]() { updateInertiaLogic(); });
        connect(m_leIxy, &QLineEdit::editingFinished, this, [updateInertiaLogic]() { updateInertiaLogic(); });
        connect(m_leIxz, &QLineEdit::editingFinished, this, [updateInertiaLogic]() { updateInertiaLogic(); });
        connect(m_leIyz, &QLineEdit::editingFinished, this, [updateInertiaLogic]() { updateInertiaLogic(); });
    }

    mMotionBodyPanel::~mMotionBodyPanel()
    {
        if (m_mainLayout != nullptr)
        {
            delete m_mainLayout;
            m_mainLayout = nullptr;
        }

    }

    void mMotionBodyPanel::newMotionBodyPanel()
    {
        setEditing(true);
        connectSig();

        int motionBodyID = getCVM->getModelData()->getManager<mMotionBodyManager>(Physical_MotionBody).createID();
        m_motionBodyData = new mMotionBody(motionBodyID);

        m_motionBodyData->setName(getCVM->getModelData()->createNameByType(Physical_MotionBody, "运动体"));
        getCVM->getModelData()->addPhysicalField<mMotionBody>(m_motionBodyData);

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionBodySig(true, m_motionBodyData->getId());
    }

    void mMotionBodyPanel::editMotionBodyPanel(int id)
    {
        setEditing(true);
        m_motionBodyData = getCVM->getModelData()->getManager<mMotionBodyManager>(Physical_MotionBody).getData(id);
        if (!m_motionBodyData)
            return;

        emit MechCommon::mMechGlobalSignals::getInstance()->activePickSig(9);

        connectSig();

        updateUIFromData();

        //进入编辑时把已分配部件灌入拾取选择集（视口高亮 ↔ 列表初始一致，同截面属性面板）
        const auto assignedSet = mMotionRefMap::getSet(Physical_MotionBody, m_motionBodyData->getId(), Physical_Part);
        highlightPart(std::vector<int>(assignedSet.begin(), assignedSet.end()));

        //进入编辑激活质心拾取控件（自动计算模式下控件禁用，不会被激活）
        activateFirstVisiblePointPicker({ m_cmPicker });

        //按最终会话归属初始化列表激活态可视反馈
        setPartPickActiveVisual(!(m_cmPicker && m_cmPicker->pickController() && m_cmPicker->pickController()->isPickActive()));
    }

    //数据对象即将被删除（树删除前调用）：置空编辑指针防悬垂（周期边界同款约定）
    void mMotionBodyPanel::onMotionBodyDataAboutToBeDestroyed(int id)
    {
        if (id >= 0)
        {
            if (m_motionBodyData == nullptr || id != m_motionBodyData->getId())
                return;
        }
        m_motionBodyData = nullptr;
    }

    void mMotionBodyPanel::uneditMotionBodyPanel()
    {
        if (!isEditing())
            return;
        setEditing(false);
        m_motionBodyData = nullptr;
        //退出编辑时失活拾取控件并清理高亮
        if (m_cmPicker && m_cmPicker->pickController())
        {
            m_cmPicker->pickController()->setPickActive(false);
            m_cmPicker->clearAllHighlight();
            //拾取数据是临时的（确定值已入数据类），退出编辑清空防残留高亮
            m_cmPicker->pickController()->displayData(PickWidgetData());
        }
        disconnectSig();
        clearPartSelection();  // 退出编辑清空视口拾取选择集，防残留高亮串到下个面板
        clearPartTempHighlight();  // 挂起临时层同样清理（自定义质心模式下存在）
        setPartPickActiveVisual(false);
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("massPoint");
    }

    void mMotionBodyPanel::performAutoCalculation() {
        if (!m_motionBodyData || mMotionRefMap::getSet(Physical_MotionBody, m_motionBodyData->getId(), Physical_Part).empty()) return;

        auto partSet = mMotionRefMap::getSet(Physical_MotionBody, m_motionBodyData->getId(), Physical_Part);
        //【OCC摘除·引擎化】旧栈 MGeoManager::CalculateGeometricAttr::CalculatePartMass 走 OCC
        // getSetByPartId（OCC 摘除后恒空壳），返回全 -1 哨兵；与求解线程同栈切几何引擎
        // calcCalculatePartMass，密度缺省沿用 7.85e-6 kg/mm³（钢）
        const GeoNeutralData::GeoSolidMass geoProps =
            getCVM->getModelData()->getGeoEngineDataMgr()->engine()
            .calcCalculatePartMass(std::vector<int>(partSet.begin(), partSet.end()), 7.85e-6);
        if (geoProps.mass <= 1e-12)
            return; // 无几何或计算失败（-1 哨兵）：保留现有值，不写入垃圾数据

        m_motionBodyData->setMass(geoProps.mass);
        m_motionBodyData->setCenterOfMass(geoProps.centroid);
        m_motionBodyData->setInertia({ geoProps.Ixx, geoProps.Iyy, geoProps.Izz,
                                       geoProps.Ixy, geoProps.Ixz, geoProps.Iyz });
    }

    // 部件变更后的统一收尾：自动模式重算质量属性 + 全量反显 + 通知树/视口刷新
    void mMotionBodyPanel::finishPartAssignmentChanged()
    {
        if (m_cmbMassOption->currentIndex() == 0) {
            performAutoCalculation();
        }

        updateUIFromData();

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionBodySig(false, m_motionBodyData->getId());
    }

    void mMotionBodyPanel::slot_allPartBtn()
    {
        if (!m_motionBodyData || !getCVM || !getCVM->getModelData())
            return;

        auto* meshData = getCVM->getModelData()->getMeshData();
        const QStringList partList = meshData->getAllPartNames();

        // 列表整体替换为全部部件；映射关系同步重置（清掉已删部件的残留）
        m_listAssignPart->clear();
        m_listAssignPart->addItems(partList);

        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        mapRelation.clearMotionRole(Physical_MotionBody, m_motionBodyData->getId(), Physical_Part, MapRelation_None);
        std::vector<int> allIds;
        for (const QString& name : partList) {
            const int partId = meshData->getPartIdByPartName(name);
            if (partId == -1)
                continue;
            allIds.push_back(partId);
            mapRelation.appendMapRelation(
                { Physical_MotionBody, m_motionBodyData->getId() }, { Physical_Part, partId });
        }

        highlightPart(allIds);  // 视口拾取选择集同步高亮（截面属性面板同款互通）
        finishPartAssignmentChanged();
    }

    void mMotionBodyPanel::slot_addPartBtn()
    {
        if (!m_motionBodyData || !getCVM || !getCVM->getModelData())
            return;

        // 候选 = 当前模型全部部件名
        QStringListModel* tempModel = new QStringListModel(this);
        tempModel->setStringList(getCVM->getModelData()->getMeshData()->getAllPartNames());

        QListWidgetItem* newItem = new QListWidgetItem();
        m_listAssignPart->insertItem(0, newItem);

        QLineEdit* editor = new QLineEdit(m_listAssignPart);
        m_listAssignPart->setItemWidget(newItem, editor);
        editor->setFocus();

        // 自动补全（大小写不敏感、包含匹配）
        QCompleter* completer = new QCompleter(tempModel, editor);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setFilterMode(Qt::MatchContains);
        completer->setCompletionMode(QCompleter::PopupCompletion);
        editor->setCompleter(completer);
        completer->complete();

        QPointer<QLineEdit> editorGuard(editor);
        auto finishFlag = std::make_shared<bool>(false);

        std::function<void()> finishEditing = [=]() mutable {
            if (*finishFlag) return;
            *finishFlag = true;

            // 统一清理
            auto cleanUp = [&]() {
                editorGuard->deleteLater();
                tempModel->deleteLater();
                };

            // 编辑器已被删除：该行废弃
            if (!editorGuard) {
                int r = m_listAssignPart->row(newItem);
                if (r >= 0) delete m_listAssignPart->takeItem(r);
                return;
            }

            // 获取当前行号
            int row = m_listAssignPart->row(newItem);
            if (row < 0) {
                cleanUp();
                return;
            }

            m_listAssignPart->removeItemWidget(newItem);
            QString text = editorGuard->text().trimmed();

            // 空值 / 不在候选列表 / 与已选重复：该行废弃
            if (text.isEmpty() || !tempModel->stringList().contains(text)) {
                delete m_listAssignPart->takeItem(row);
                cleanUp();
                return;
            }
            for (int i = 0; i < m_listAssignPart->count(); ++i)
            {
                QListWidgetItem* it = m_listAssignPart->item(i);
                if (it == newItem) continue;
                if (it && it->text() == text) {
                    delete m_listAssignPart->takeItem(row);
                    cleanUp();
                    return;
                }
            }

            // 名称反查部件ID
            const int partId = getCVM->getModelData()->getMeshData()->getPartIdByPartName(text);
            if (partId == -1 || !m_motionBodyData) {
                delete m_listAssignPart->takeItem(row);
                cleanUp();
                return;
            }

            newItem->setText(text);
            getCVM->getModelData()->getMapRelationManager().appendMapRelation(
                { Physical_MotionBody, m_motionBodyData->getId() }, { Physical_Part, partId });

            cleanUp();
            highlightPart({ partId });  // 视口拾取选择集同步高亮
            finishPartAssignmentChanged();
            };

        connect(completer, QOverload<const QString&>::of(&QCompleter::activated),
            editor, [=](const QString& choice)
            {
                if (editorGuard)
                {
                    editorGuard->setText(choice);
                    finishEditing();
                }
            }, Qt::UniqueConnection);

        connect(editor, &QLineEdit::editingFinished, editor, finishEditing, Qt::UniqueConnection);
    }

    void mMotionBodyPanel::slot_reducePartBtn()
    {
        if (!m_motionBodyData || !getCVM || !getCVM->getModelData())
            return;

        const auto selectedItems = m_listAssignPart->selectedItems();
        if (selectedItems.isEmpty()) return;

        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        auto* meshData = getCVM->getModelData()->getMeshData();
        std::vector<int> removedIds;

        // 遍历删除每一个选中行（按部件名反查ID删单条映射）
        for (QListWidgetItem* item : selectedItems)
        {
            if (!item) continue;

            const int row = m_listAssignPart->row(item);
            if (row < 0) continue;

            const int partId = meshData->getPartIdByPartName(item->text().trimmed());
            if (partId != -1) {
                removedIds.push_back(partId);
                mapRelation.deleteMapRelation(
                    { Physical_MotionBody, m_motionBodyData->getId() },
                    { Physical_Part, partId });
            }

            delete m_listAssignPart->takeItem(row);
        }

        unHighlightPart(removedIds);  // 视口拾取选择集同步去高亮
        finishPartAssignmentChanged();
    }

    void mMotionBodyPanel::slot_clearPartBtn()
    {
        if (!m_motionBodyData)
            return;
        getCVM->getModelData()->getMapRelationManager().clearMotionRole(Physical_MotionBody, m_motionBodyData->getId(), Physical_Part, MapRelation_None);
        m_listAssignPart->clear();

        clearPartSelection();  // 视口拾取选择集同步清空
        finishPartAssignmentChanged();
    }

    void mMotionBodyPanel::updateUIFromData()
    {
        if (!getCVM)
        {
            return;
        }

        if (!m_motionBodyData)
            return;


        // 【极其关键】：使用 QSignalBlocker 阻塞信号！
        // 因为我们马上要用代码修改 SpinBox 和 LineEdit 的值，
        // 如果不阻塞，修改 UI 就会触发我们上面写的 Lambda 信号，导致死循环或者多余的数据覆盖。

        const QSignalBlocker blockerFixed(m_chkIsFixed);
        m_chkIsFixed->setChecked(m_motionBodyData->getIsFixed());

        const QVector3D initVel = m_motionBodyData->getInitialVelocity();
        const QSignalBlocker blockerVX(m_leInitVelX); m_leInitVelX->setExpression(QString::number(initVel.x()));
        const QSignalBlocker blockerVY(m_leInitVelY); m_leInitVelY->setExpression(QString::number(initVel.y()));
        const QSignalBlocker blockerVZ(m_leInitVelZ); m_leInitVelZ->setExpression(QString::number(initVel.z()));

        const QVector3D initOmega = m_motionBodyData->getInitialAngularVelocity();
        const QSignalBlocker blockerWX(m_leInitOmegaX); m_leInitOmegaX->setExpression(QString::number(initOmega.x()));
        const QSignalBlocker blockerWY(m_leInitOmegaY); m_leInitOmegaY->setExpression(QString::number(initOmega.y()));
        const QSignalBlocker blockerWZ(m_leInitOmegaZ); m_leInitOmegaZ->setExpression(QString::number(initOmega.z()));

        // 刷新已分配部件列表
        m_listAssignPart->clear();
        for (int geoId : mMotionRefMap::getSet(Physical_MotionBody, m_motionBodyData->getId(), Physical_Part)) {
            m_listAssignPart->addItem(getCVM->getModelData()->getMeshData()->getPartNameByPartId(geoId));
        }

        // 拾取比对基准与面板数据对齐（面板内变更不通知拾取会话，此处保持一致）
        const auto assignedNow = mMotionRefMap::getSet(Physical_MotionBody, m_motionBodyData->getId(), Physical_Part);
        m_lastPickPartData.assign(assignedNow.begin(), assignedNow.end());

        // 部件高亮统一由拾取选择集负责（editMotionBodyPanel/highlightPart 一族），
        // 不直写共享 "red" 渲染——该渲染被接触面板/符号管理器共用，直写会互相覆盖产生残留

        //还原下拉框状态与控件亮显状态
        const QSignalBlocker blockerMassOpt(m_cmbMassOption);
        bool isCustom = m_motionBodyData->getIsCustomMassProps();
        m_cmbMassOption->setCurrentIndex(isCustom ? 1 : 0);

        m_leMass->setEnabled(isCustom);
        m_cmPicker->setWidgetEnabled(isCustom);
        m_leIxx->setEnabled(isCustom); m_leIyy->setEnabled(isCustom); m_leIzz->setEnabled(isCustom);
        m_leIxy->setEnabled(isCustom); m_leIxz->setEnabled(isCustom); m_leIyz->setEnabled(isCustom);

        // 刷新质量
        const QSignalBlocker blockerMass(m_leMass);
        m_leMass->setExpression(QString::number(m_motionBodyData->getMass(), 'g', 17));

        // 刷新质心：同步到通用点坐标拾取控件
        const QVector3D cmPos = m_motionBodyData->getCenterOfMass();
        m_cmPicker->setCoordinate(cmPos);
        if (getCVM && getCVM->getPreRenderWidget())
        {
            auto commonShapeRender = getCVM->getPreRenderWidget()->getCommonShapeRender();
            if (commonShapeRender)
            {
                //注意：本函数会被拾取信号链间接触发，清理拾取会误伤激活中的拾取会话
                commonShapeRender->appendPoint("massPoint", { cmPos },
                    mMotionPanelColors::vec(mMotionPanelColors::pointA()));
                getCVM->getPreRenderWidget()->updatePaint();
            }
        }

        // 刷新惯性矩
        QVector<double> inertia = m_motionBodyData->getInertia();
        if (inertia.size() == 6) {
            const QSignalBlocker blockerIxx(m_leIxx); m_leIxx->setExpression(QString::number(inertia[0], 'g', 17));
            const QSignalBlocker blockerIyy(m_leIyy); m_leIyy->setExpression(QString::number(inertia[1], 'g', 17));
            const QSignalBlocker blockerIzz(m_leIzz); m_leIzz->setExpression(QString::number(inertia[2], 'g', 17));
            const QSignalBlocker blockerIxy(m_leIxy); m_leIxy->setExpression(QString::number(inertia[3], 'g', 17));
            const QSignalBlocker blockerIxz(m_leIxz); m_leIxz->setExpression(QString::number(inertia[4], 'g', 17));
            const QSignalBlocker blockerIyz(m_leIyz); m_leIyz->setExpression(QString::number(inertia[5], 'g', 17));
        }
    }

    void mMotionBodyPanel::connectSig()
    {
        disconnectSig();
        if (!getCVM || !getCVM->getPreRenderWidget())
        {
            return;
        }

        conn_geo = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mMotionBodyPanel::slot_finishedPick);
    }

    void mMotionBodyPanel::disconnectSig()
    {
        if (conn_geo)
            disconnect(conn_geo);
    }

        void mMotionBodyPanel::slot_finishedPick()
        {
            if (!getCVM || !m_motionBodyData)
            {
                return;
            }
            //质心拾取会话激活期间让位：点拾取会整体替换共享选择容器（部件集被清空），
            //此时做部件同步等于拾取一个质心点就清空部件列表；列表会话经 activatePartPick() 接管
            if (m_cmPicker && m_cmPicker->pickController() && m_cmPicker->pickController()->isPickActive())
            {
                return;
            }
            //注意：不能用 getCurrentPickDataType==None 做早退——该类型靠选择容器存量计数
            //判定，右键取消拾取清空部件后计数为 0 会判成 None，最后一次取消就被吞掉，
            //映射/列表不同步（截面属性面板 mPropertyPanel 同款无守卫，靠增量比对天然 inert）
            auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
            auto partIds = MechPreData::getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
            const int bodyId = m_motionBodyData->getId();

            if (partIds.size() > m_lastPickPartData.size())
            {
                // 拾取数据比当前多 → 新增的部分
                std::vector<int> addedIds;
                QSet<int> currentSet;
                for (auto id : m_lastPickPartData)
                {
                    currentSet.insert(id);
                }
                for (auto& e : partIds)
                {
                    if (!currentSet.contains(e))
                    {
                        addedIds.push_back(e);
                        mapRelation.appendMapRelation({ Physical_MotionBody, bodyId }, { Physical_Part, e });
                    }
                }
                if (!addedIds.empty())
                {
                    finishPartAssignmentChanged();
                }
            }
            else
            {
                // 拾取数据比当前少 → 减少的部分
                std::vector<int> reducedIds;
                QSet<int> selectedSet;
                for (auto& e : partIds)
                    selectedSet.insert(e);

                for (int id : m_lastPickPartData)
                {
                    if (!selectedSet.contains(id))
                    {
                        reducedIds.push_back(id);
                        mapRelation.deleteMapRelation({ Physical_MotionBody, bodyId }, { Physical_Part, id });
                    }
                }
                if (!reducedIds.empty())
                {
                    finishPartAssignmentChanged();
                }
            }

            m_lastPickPartData = partIds;
        }

        // 拾取会话选择集同步：先断开拾取信号防重入，操作选择集并刷新高亮，最后重连并更新基准
        void mMotionBodyPanel::highlightPart(std::vector<int> ids)
        {
            if (!getCVM || !getCVM->getPreRenderWidget() || ids.empty())
                return;
            if (conn_geo)
                disconnect(conn_geo);
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(9);
            MechPreData::addPreWidgetSelectPartID(getCVM->getPreRenderWidget(), ids);
            MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
            getCVM->getPreRenderWidget()->updatePaint();

            conn_geo = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mMotionBodyPanel::slot_finishedPick);
            m_lastPickPartData = MechPreData::getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
        }

        void mMotionBodyPanel::unHighlightPart(std::vector<int> ids)
        {
            if (!getCVM || !getCVM->getPreRenderWidget() || ids.empty())
                return;
            if (conn_geo)
                disconnect(conn_geo);
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(9);
            MechPreData::reducePreWidgetSelectPartID(getCVM->getPreRenderWidget(), ids);
            MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
            getCVM->getPreRenderWidget()->updatePaint();

            conn_geo = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mMotionBodyPanel::slot_finishedPick);
            m_lastPickPartData = MechPreData::getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
        }

        void mMotionBodyPanel::clearPartSelection()
        {
            if (!getCVM || !getCVM->getPreRenderWidget())
                return;
            // 仅拾取选择集非空时才需要断开重连（清空会触发 ReadyToReadContainer 重入）
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

            conn_geo = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mMotionBodyPanel::slot_finishedPick);
            m_lastPickPartData.clear();
        }


        void mMotionBodyPanel::activatePartPick()
        {
            if (!getCVM || !getCVM->getPreRenderWidget() || !m_motionBodyData)
                return;

            // 质心拾取控件让位：失活但保留已拾取值与临时高亮（同全局互斥"被顶下"语义）
            if (m_cmPicker && m_cmPicker->pickController() && m_cmPicker->pickController()->isPickActive())
                m_cmPicker->pickController()->setPickActive(false);

            // 恢复部件拾取模式，并按当前数据重建选择集与比对基准
            // （质心点拾取可能已把共享容器里的部件集清空/替换）
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(9);
            clearPartSelection();
            clearPartTempHighlight();  // 接管后由激活选择集高亮接管，清掉挂起临时层
            const auto assigned = mMotionRefMap::getSet(Physical_MotionBody, m_motionBodyData->getId(), Physical_Part);
            if (!assigned.empty())
                highlightPart(std::vector<int>(assigned.begin(), assigned.end()));
            setPartPickActiveVisual(true);
        }

        void mMotionBodyPanel::setPartPickActiveVisual(bool active)
        {
            if (!m_listAssignPart)
                return;
            if (active)
            {
                const QColor c = mMotionPanelColors::bodyA();
                m_listAssignPart->setStyleSheet(QString(
                    "QListWidget { border: 2px solid rgba(%1,%2,%3,200);"
                    "background: rgba(%1,%2,%3,26); }"
                    "QListWidget::item:selected { background: rgba(%1,%2,%3,90); }")
                    .arg(c.red()).arg(c.green()).arg(c.blue()));
                m_listAssignPart->setToolTip(QStringLiteral("部件拾取已激活：视口点选加入部件，右键取消移除"));
            }
            else
            {
                m_listAssignPart->setStyleSheet(QString());
                m_listAssignPart->setToolTip(QStringLiteral("部件拾取挂起中：点击列表可接管（质心拾取激活期间让位）"));
            }
        }

        void mMotionBodyPanel::renderPartTempHighlight()
        {
            if (!getCVM || !getCVM->getPreRenderWidget() || !m_motionBodyData)
                return;
            auto* widget = getCVM->getPreRenderWidget();
            auto* highlightController = widget->getHighlightController();
            if (!highlightController)
                return;

            // 几何/网格部件分流（判定同 PickCommonFunction::setPreWidgetSelectPartID）
            const auto& modeldata = widget->getModelData();
            auto* meshData = mxr::refData<mMeshData>(modeldata._meshData);
            std::vector<int> geoPartIds;
            std::vector<mxr::MeshSelectInfoData> meshPartInfos;
            for (int id : mMotionRefMap::getSet(Physical_MotionBody, m_motionBodyData->getId(), Physical_Part))
            {
                if (meshData && meshData->havePart(meshData->getPartNameByPartId(id)))
                {
                    mxr::MeshSelectInfoData info;
                    info.part_id = id;
                    meshPartInfos.push_back(info);
                }
                if (modeldata.getGeoPartMgr() != nullptr && modeldata.getGeoPartMgr()->containsPart(id))
                    geoPartIds.push_back(id);
            }

            // 独占键的额外渲染层（浅红=挂起临时态）；勿改用共享 "red" 键——会与接触面板/符号管理器互相覆盖
            const QColor tempColor(255, 150, 150);
            auto geoRender = highlightController->getExtraPreGeoHighLightRender("MotionBodyPartTemp");
            geoRender->setHighLightCover(true, -3.0f);
            geoRender->setHighLightColor(tempColor.red(), tempColor.green(), tempColor.blue());
            auto meshRender = highlightController->getExtraPreMeshHighLightRender("MotionBodyPartTemp");
            meshRender->setHighLightCover(true, -3.0f);
            meshRender->setHighLightColor(tempColor.red(), tempColor.green(), tempColor.blue());

            if (!geoPartIds.empty())
            {
                mGeoSelectData* geoData = new mGeoSelectData();
                geoData->UpdatePickData(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::GeoPart, geoPartIds);
                geoRender->updateHighLightRender(geoData);
                delete geoData;
            }
            if (!meshPartInfos.empty())
            {
                MeshSelectMemoryData* meshSel = new MeshSelectMemoryData();
                meshSel->UpdateSelectDataByType(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::MeshPart, meshPartInfos);
                meshRender->updateHighLightRender(meshSel);
                delete meshSel;
            }
            widget->updatePaint();
        }

        void mMotionBodyPanel::clearPartTempHighlight()
        {
            if (!getCVM || !getCVM->getPreRenderWidget())
                return;
            auto* widget = getCVM->getPreRenderWidget();
            auto* highlightController = widget->getHighlightController();
            if (!highlightController)
                return;

            auto geoRender = highlightController->getExtraPreGeoHighLightRender("MotionBodyPartTemp");
            if (geoRender)
            {
                mGeoSelectData* emptyGeo = new mGeoSelectData();
                for (mxr::SelectionInfoType t : { mxr::SelectionInfoType::GeoPoint, mxr::SelectionInfoType::GeoEdge,
                    mxr::SelectionInfoType::GeoFace, mxr::SelectionInfoType::GeoSolid, mxr::SelectionInfoType::GeoPart })
                    emptyGeo->UpdatePickData(mxr::SelectionOperate::CLEAR, t, {});
                geoRender->updateHighLightRender(emptyGeo);
                delete emptyGeo;
            }

            auto meshRender = highlightController->getExtraPreMeshHighLightRender("MotionBodyPartTemp");
            if (meshRender)
            {
                MeshSelectMemoryData* emptyMesh = new MeshSelectMemoryData();
                emptyMesh->UpdateSelectDataByType(mxr::SelectionOperate::CLEAR, mxr::SelectionInfoType::MeshPart, {});
                meshRender->updateHighLightRender(emptyMesh);
                delete emptyMesh;
            }
            widget->updatePaint();
        }

        bool mMotionBodyPanel::eventFilter(QObject* obj, QEvent* ev)
        {
            // 列表 viewport 任意按下（含空白处，QListWidget::pressed 只在列表项上发射）即接管拾取会话
            if (obj == m_listAssignPart->viewport() && ev->type() == QEvent::MouseButtonPress && isEditing())
                activatePartPick();
            return mPanelBase::eventFilter(obj, ev);
        }

    void mMotionBodyPanel::slot_subWindowActivated(QMdiSubWindow* subWindow)
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
