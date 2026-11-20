#include "mContactPanel.h"
#include "mAppConfiguration.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mCalcLineEdit.h"
#include "mGlobalSignals.h"
#include "mInfoWindow.h"
#include "mCalcLineEdit.h"
#include "mGeoDataBase.h"
#include "mTableDataWindow.h"
#include "mDockLayoutManager.h"

#include "mPythonAgent.h"
#include "mPyScriptWriter.h"
#include "mPyScriptReader.h"
#include "PickCommonFunction.h"
#include "mGeneralPickUp.h"
#include "mGetSetTypeTemplate.h"
#include "mGetSetFunction.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/view/CCSMPreRender.h"
#include "mxr/basic/CameraController.h"
#include "mxr/gizmo/GizmoManager.h"

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
#include <QCompleter>
#include <QTimer>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHash>
#include <QDateTime>

//BasicData
#include "mModelData.h"
#include "mMeshData.h"
#include "mContactManager.h"
#include "mContactBase.h"
#include "mFunctionManager.h"
#include "mSetBase.h"
#include "mPartSet.h"
#include "mMixSet.h"
#include "mGeoSet.h"
#include "mElementSurface.h"
#include "mNodeFace.h"

//Geo
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（拉 mOCCDocument→OCC 链；冻结岛保档）

#include "AllRenderSymbolManager.h"
#include "mDataStatusManager.h"
#include "mOtherFunction.h"
#include "mAPIServer.h"

using namespace MechPreData;
using namespace MechSetData;
using namespace MechContactData;
using namespace MechElementData;
using namespace MxFunctions;
using namespace mxr;
using namespace MechCommon;
namespace MechMainWindow
{
    mContactPanel::mContactPanel(QWidget* parent): mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);

        _mainLayout->setContentsMargins(2, 0, 2, 0);

        _contactData = nullptr;

        initUI();

        for (int i = 0; i < m_foldWidgets.size(); i++)
        {
            _mainLayout->removeWidget(m_foldWidgets[i]); m_foldWidgets[i]->hide();
        }

        _mainLayout->addStretch();
    }

    void mContactPanel::initUI()
    {
        // === 0. 类型 ===
        {
            FoldSection* f = new FoldSection("类型", this);
            cmb_type = new QComboBox();
            cmb_type->addItems({ "面面接触", "点面接触", "线面接触", "通用接触", "自接触", "二维接触"});
            f->addRow(new PanelLabel("类型"), cmb_type);
            m_foldWidgets.append(f);

            connect(cmb_type, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int idx) {
                if (!_contactData) return;

                mPickWidgetController::commitActivePickWidget();

                // 只调 API，由 API 内部写数据，后续 updatePanel() 重新 loadDataToUI 回显
                commitContactField(QStringLiteral("type"), cmb_type->currentText());

                QScrollArea* parentScrollArea = nullptr;
                QWidget* p = this->parentWidget();
                while (p) {
                    parentScrollArea = qobject_cast<QScrollArea*>(p);
                    if (parentScrollArea) break;
                    p = p->parentWidget();
                }

                // 2. 刷新界面
                updatePanel();

                // 3. 强制回到顶部
                if (parentScrollArea) {
                    QTimer::singleShot(0, parentScrollArea, [=]() {
                        parentScrollArea->verticalScrollBar()->setValue(0);
                        });
                }

                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Contact, _contactData->getContactId());
                });
        }

        // ================== 1. 接触对 ==================
        {
            FoldSection* f = new FoldSection("接触对", this);

            chk_transparent = new QCheckBox("部件透明");

            // 总体容器 Widget
            wd_pairContainer = new QWidget();
            QHBoxLayout* mainHLayout = new QHBoxLayout(wd_pairContainer);
            mainHLayout->setContentsMargins(0, 0, 0, 0);
            mainHLayout->setSpacing(5);

            // 左侧垂直布局 (放主面行 + 从面行)
            QWidget* leftWidget = new QWidget();
            QVBoxLayout* leftVLayout = new QVBoxLayout(leftWidget);
            leftVLayout->setContentsMargins(0, 0, 0, 0);
            leftVLayout->setSpacing(5);

            // helper: 创建一行通用拾取控件（颜色块区分；prependWidget 拾取控件前附加控件，如从面类型下拉框）
            auto createPickRow = [&](mPickWidgetController*& picker, mColorBlock colorBlock, const QString& tip, QWidget* prependWidget = nullptr) -> QWidget*
                {
                    QWidget* rowW = new QWidget();
                    QHBoxLayout* rowH = new QHBoxLayout(rowW);
                    rowH->setContentsMargins(0, 0, 0, 0);
                    rowH->setSpacing(4);

                    if (prependWidget)
                    {
                        rowH->addWidget(prependWidget, 0);
                    }

                    picker = mGeneralPickUp::createPickWidgetNoTempHight(colorBlock, rowW);
                    picker->setDefultTip(tip);
                    picker->setAllowedTypes([this]() {
                        return _contactData && _contactData->getContactType() == MxFunctions::TwoDimensionalContact
                            ? pickGeoEdgeFaceSolidAndElement()
                            : pickGeoFaceAndElementSurface();
                        });

                    // 激活时失活同面板其它拾取控件，并切换为面拾取模式；
                    // 点面接触从面为节点表面模式时切入节点拾取（5=节点，7=单元面）
                    connect(picker, &mPickWidgetController::pickActiveChanged, this, [=](bool isActive) {
                        if (isActive)
                        {
                            const bool is2D = _contactData && _contactData->getContactType() == MxFunctions::TwoDimensionalContact;
                            const int pickFilter = is2D ? 6 : ((tip == "从面" && slavePickIsNodeMode()) ? 5 : 7);
                            emit mMechGlobalSignals::getInstance()->activePickSig(pickFilter);

                            // 激活流程 applyHighlight 会把控件已有数据写回共享容器，防抖刷新
                            // 重建主拾取高亮点图元后已拾取节点会变回拾取色：延迟到防抖之后
                            // 重建一次本控件颜色的点图元（见 deferActivateNodePointTopUp）
                            deferActivateNodePointTopUp(picker);
                        }
                        else
                        {
                            // 取消激活设置为0
                            if (tip == "主面" || tip == "接触面")
                            {
                                _contactData->setPickStateMaster(0);
                            }
                            else if (tip == "从面")
                            {
                                _contactData->setPickStateSlave(0);
                            }
                        }
                        });

                    // 数据变化实时提交并刷新渲染
                    if (tip == "主面")
                    {
                        connect(picker, &mPickWidgetController::dataChanged, this, &mContactPanel::slot_commitMasterContactData);
                    }
                    else   if (tip == "从面")
                    {
                        connect(picker, &mPickWidgetController::dataChanged, this, &mContactPanel::slot_commitSlaveContactData);
                    }
                    else
                    {
                        connect(picker, &mPickWidgetController::dataChanged, this, &mContactPanel::slot_commitSelfContactData);
                    }

                    rowH->addWidget(picker->widget(), 1);

                    return rowW;
                };

            QWidget* masterRow = createPickRow(pick_master, CBRed, "主面");
            leftVLayout->addWidget(masterRow);

            // 从面类型下拉框（仅点面/线面接触显示）：单元面/节点表面切换从面拾取控件的拾取类型与集合选择类型
            cmb_slaveRegion = new QComboBox();
            cmb_slaveRegion->addItems({ QStringLiteral("单元面"), QStringLiteral("节点面") });
            cmb_slaveRegion->setToolTip(QStringLiteral("从面类型"));
            cmb_slaveRegion->setFixedWidth(88);
            // 窄下拉框保留箭头槽，避免首次动态显示时退化为无边框弹框。
            cmb_slaveRegion->setStyleSheet(QStringLiteral(
                "QComboBox::drop-down { width:18px; border:none; }"
                "QComboBox QAbstractItemView { outline:none; }"));
            cmb_slaveRegion->hide();

            QWidget* slaveRow = createPickRow(pick_slave, CBBlue, "从面", cmb_slaveRegion);
            leftVLayout->addWidget(slaveRow);

            // 从面拾取类型动态分流：点面/线面接触节点表面模式可拾取几何对象、节点和部件，
            // 其余保持几何面+单元面。激活时按当前接触类型+slave_region 实时求值，避免模式串扰
            pick_slave->setAllowedTypes([this]() {
                if (_contactData && _contactData->getContactType() == MxFunctions::TwoDimensionalContact)
                    return pickGeoEdgeFaceSolidAndElement();
                return slavePickIsNodeMode() ? pickGeoPointEdgeFaceSolidNodePart() : pickGeoFaceAndElementSurface();
                });

            // 切换从面类型：清空互不兼容的已拾取数据，激活从面拾取使新类型即刻生效
            connect(cmb_slaveRegion, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int idx) {
                if (!_contactData) return;
                // 仅点面/线面接触生效（控件共用区分：其它接触类型下拉框不显示，此处再兜一层）
                const auto type = _contactData->getContactType();
                if (type != MxFunctions::ContactType::NodeToSurfaceContact
                    && type != MxFunctions::ContactType::LineToSurfaceContact) return;

                // 区域类型通过 contact API 写入并记录命令，随工程文件持久化
                commitContactField(QStringLiteral("slaveRegion"), QString::number(idx));

                // 节点表面与面数据互不兼容：切换时清空从面已拾取数据，交由失活提交同步清底层数据
                if (pick_slave)
                {
                    pick_slave->replacePickedDataAndClearSets({});

                    // 立即激活从面拾取：从面未激活时拾取工具栏屏蔽/视口拾取模式仍跟随当前
                    // 激活控件（面板默认激活主面），切换看上去不生效；激活后按新模式重设屏蔽
                    pick_slave->setPickActive(true);
                }

                applySlaveRegionPickTypes();

                // 节点表面模式下交换无意义（节点面集不能换作主面），隐藏交换按钮
                if (btn_swap)
                {
                    btn_swap->setVisible(!slavePickIsNodeMode());
                }
                });

            // 右侧交换按钮
            btn_swap = new QToolButton();
            btn_swap->setIcon(QIcon(":/mMainWidgets/Icon/jiaohuan3x.png"));
            btn_swap->setFixedWidth(30);
            btn_swap->setFixedHeight(30);
            btn_swap->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            btn_swap->setToolTip("交换");

            // 创建 自接触面行
            wd_contactRow = createPickRow(pick_face, CBRed, "接触面");

            mainHLayout->addWidget(leftWidget, 1);
            mainHLayout->addWidget(btn_swap, 0);

            wd_pairContainer->setParent(nullptr);
            wd_contactRow->setParent(nullptr);
            f->addRow(chk_transparent);
            m_foldWidgets.append(f);

            // 是否透明
            connect(chk_transparent, &QCheckBox::toggled, this, [=](bool c) {
                if (!_contactData) return;
                if (c)
                {
                    transparentPart();
                }
                else
                {
                    resetPartAlpha(false);
                }
                });

            // 交换按钮：先提交控件数据（走持久化策略API），再走 contact API 交换底层主/从集合映射，
            // 最后从底层反显并刷新渲染
            connect(btn_swap, &QPushButton::clicked, this, [=]() {
                if (!_contactData) return;

                mPickWidgetController::commitActivePickWidget();

                // 交换主/从集合映射（contact API swap 键）并记录命令
                commitContactField(QStringLiteral("swap"), QStringLiteral("true"));

                // 从底层数据重新加载主/从 PickWidgetData 并反显（带高亮刷新）
                PickWidgetData masterData, slaveData;
                loadContactDataFromSet(MapRelation_Master, masterData);
                loadContactDataFromSet(MapRelation_Slave, slaveData);
                if (pick_master) pick_master->displayData(masterData, true);
                if (pick_slave) pick_slave->displayData(slaveData, true);

                // 同步更新主/从渲染
                slot_commitMasterContactData();
                slot_commitSlaveContactData();
                });
        }


        // === 2. 接触参数 ===
        {
            FoldSection* f = new FoldSection("接触参数", this);
            lb_smallSlip = new PanelLabel("小滑移");
            cmb_smallSlip = new QComboBox(); cmb_smallSlip->addItems({ "关闭", "开启" });

            f->addRow(lb_smallSlip, cmb_smallSlip);
            m_foldWidgets.append(f);

            // 信号槽 (简化写法，仅示例)
            connect(cmb_smallSlip, &QComboBox::currentTextChanged, this, [=](QString v) {
                if (_contactData)
                {
                    commitContactField(QStringLiteral("smallSlip"), (v == "开启") ? QStringLiteral("true") : QStringLiteral("false"));
                    // 点面/线面接触不支持接触间隙，其余类型在开启小滑移时隐藏
                    const auto type = _contactData->getContactType();
                    const bool noGap = type == MxFunctions::ContactType::NodeToSurfaceContact
                        || type == MxFunctions::ContactType::LineToSurfaceContact;
                    m_foldWidgets[6]->setVisible(!noGap && v != "开启");
                }
                });
        }

        // === 3. 法向 (点面) ===
        {
            FoldSection* f = new FoldSection("法向接触行为", this);
            lb_nodeOver = new PanelLabel("压力过盈");
            cmb_nodeOver = new QComboBox();
            cmb_nodeOver->addItems({ "硬接触", "完全绑定接触", "法向不分离接触", "线性接触"});

            lb_nodeConst = new PanelLabel("约束方法");lb_nodeConst->setParent(nullptr);
            cmb_nodeConst = new QComboBox();cmb_nodeConst->setParent(nullptr);

            lb_nodeScale = new PanelLabel("缩放系数");lb_nodeScale->setParent(nullptr);
            le_nodeScale = new mCalcLineEdit(this, true, false);le_nodeScale->setParent(nullptr);

            lb_nodePenalty = new PanelLabel("罚系数");lb_nodePenalty->setParent(nullptr);
            le_nodePenalty = new mCalcLineEdit(this, true, false);le_nodePenalty->setParent(nullptr);

            f->addRow(lb_nodeOver, cmb_nodeOver);
            //f->addRow(lb_nodeConst, cmb_nodeConst);
            //f->addRow(lb_nodeScale, le_nodeScale);
            //f->addRow(lb_nodePenalty, le_nodePenalty);
            m_foldWidgets.append(f);

            // 联动逻辑：压力过盈改变 -> 触发约束方法改变 -> 触发UI刷新
            // 信号连接
            connect(cmb_nodeOver, &QComboBox::currentTextChanged, this, [=](const QString& text) {
                if (!_contactData) return;
                commitContactField(QStringLiteral("overclosure"), text);
                updateRowVisibility(); // 关键！选项改变导致下面两行显隐变化
                });

            connect(cmb_nodeConst, &QComboBox::currentTextChanged, this, [=](const QString& text) {
                if (!_contactData) return;
                // 因为ComboBox可能会被清空重建，需要判空
                if (!text.isEmpty()) { commitContactField(QStringLiteral("constraint"), text); }
                updateRowVisibility(); // 关键！选项改变导致下面两行显隐变化
                });

            connect(le_nodeScale, &QLineEdit::editingFinished, this, [=]() {
                if (_contactData) { commitContactField(QStringLiteral("scaleFactor"), le_nodeScale->text()); }
                });

            connect(le_nodePenalty, &QLineEdit::editingFinished, this, [=]() {
                if (_contactData) { commitContactField(QStringLiteral("penaltyFactor"), le_nodePenalty->text()); }
                });

        }

        // === 4. 切向行为 ===
        {
            FoldSection* f = new FoldSection("切向行为", this);f->setCheckable(true);//f->setChecked(false);
            //chk_tangent = new QCheckBox("定义切向行为");

            lb_tanFric = new PanelLabel("摩擦公式");
            cmb_tanFric = new QComboBox(); cmb_tanFric->addItems({ "无摩擦", "通用接触摩擦", "粗糙" });

            lb_tanCoef = new PanelLabel("静摩擦系数");
            le_tanCoef = new mCalcLineEdit(this, true, false);

            //f->addRow(chk_tangent);
            f->addRow(lb_tanFric, cmb_tanFric);
            f->addRow(lb_tanCoef, le_tanCoef); lb_tanCoef->hide(); le_tanCoef->hide();
            m_foldWidgets.append(f);

            connect(f, &FoldWidget::enabledChanged, this, [=](bool c) {
                if (!_contactData) return;
                commitContactField(QStringLiteral("tangentialDefine"), c ? QStringLiteral("true") : QStringLiteral("false"));
                //updateRowVisibility();
                });
            connect(cmb_tanFric, &QComboBox::currentTextChanged, this, [=](QString v) {
                if (!_contactData) return;
                commitContactField(QStringLiteral("tangentialFriction"), v);
                updateRowVisibility();
                });

            connect(le_tanCoef, &QLineEdit::editingFinished, this, [=]() {
                if (_contactData) { commitContactField(QStringLiteral("tangentialStaticCoef"), le_tanCoef->text()); }
                });
        }

        // === 5. 热接触 ===
        {
            FoldSection* f = new FoldSection("热接触间隙传导", this);f->setCheckable(true);
            lb_heat = new QLabel("热接触间隙传导");
            lb_heat->setFixedWidth(90);
            btn_heat = new QToolButton();QIcon icon(":/mMainWidgets/Icon/table.png");btn_heat->setIcon(icon);
            btn_heat->setFixedSize(30,30);
            btn_heat->setIconSize(QSize(30,30));
            connect(f, &FoldWidget::enabledChanged, this, [=](bool checked) {
                if (!_contactData) return;
                commitContactField(QStringLiteral("heatDefine"), checked ? QStringLiteral("true") : QStringLiteral("false"));
                if (!checked)
                {
                    mTableDataWindow::getInstance()->resetInitialTable();
                    mTableDataWindow::getInstance()->clearAndReset(0, 0);
                }
                else
                {
                    btn_heat->clicked();
                }
                });
            connect(btn_heat, &QToolButton::clicked, [=]() {
                if (!_contactData) return;
                mTableDataWindow::getInstance()->clearAndReset(2, 2);
                mTableDataWindow::getInstance()->resetInitialTable();
                mDockLayoutManager::getInstance()->activateDock("Table");
                QVector<QVector<double>> xydatas;xydatas.resize(2);
                for (const QPair<double, double>& data : _contactData->heat_value)
                {
                    xydatas[0].append(data.first);
                    xydatas[1].append(data.second);
                }
                mTableDataWindow::getInstance()->setTableData(xydatas);
                mTableDataWindow::getInstance()->setHorizontalHeaders({ "导热系数", "间隙值" });
                });
            //tableBtn->cb->setChecked(_contactData->checked);
            f->addRow(lb_heat, btn_heat);
            m_foldWidgets.append(f);
        }

        // === 6. 接触间隙 ===
        {
            FoldSection* f = new FoldSection("接触间隙", this);f->setCheckable(true);//f->setChecked(false);
            //chk_gap = new QCheckBox("定义接触间隙");
            lb_gapVal = new PanelLabel("过盈量"); le_gapVal = new mCalcLineEdit(this, true, false);

            //f->addRow(chk_gap);
            f->addRow(lb_gapVal, le_gapVal);
            m_foldWidgets.append(f);

            connect(f, &FoldWidget::enabledChanged, this, [=](bool c) {
                if (_contactData) { commitContactField(QStringLiteral("gapDefine"), c ? QStringLiteral("true") : QStringLiteral("false")); }
                //updateRowVisibility();
                });

            connect(le_gapVal, &QLineEdit::editingFinished, this, [=]() {
                if (_contactData) { commitContactField(QStringLiteral("gapOverclosure"), le_gapVal->text()); }
                });
        }

        // === 7. 几何调整 ===
        {
            FoldSection* f = new FoldSection("几何调整", this);f->setCheckable(true);//f->setChecked(false);
            //chk_geom = new QCheckBox("定义几何调整");
            lb_geomVal = new PanelLabel("调整值"); le_geomVal = new mCalcLineEdit(this, true, false);

            //f->addRow(chk_geom);
            f->addRow(lb_geomVal, le_geomVal);
            m_foldWidgets.append(f);

            connect(f, &FoldWidget::enabledChanged, this, [=](bool c) {
                if (_contactData) { commitContactField(QStringLiteral("geomDefine"), c ? QStringLiteral("true") : QStringLiteral("false")); }
                //updateRowVisibility();
                });

            connect(le_geomVal, &QLineEdit::editingFinished, this, [=]() {
                if (_contactData) { commitContactField(QStringLiteral("geomValue"), le_geomVal->text()); }
                });
        }

        // === 8. 作用对象 ===
        {
            FoldSection* f = new FoldSection("作用对象", this);
            cmb_obj = new QComboBox();
            f->addRow(new PanelLabel("接触对"), cmb_obj);
            m_foldWidgets.append(f);

            connect(cmb_obj, &QComboBox::currentTextChanged, this, [=](QString text) {
                if (!_contactData) return;
                // 走 contact API 写入接触对映射并记录命令
                commitContactField(QStringLiteral("contactPair"), text);

                //设置接触对显示状态
                auto data = getCVM->getModelData()->getManager<MechContactData::mContactManager>(Physical_Contact).getContactData(text);
                updateContactPairUI(data);
                });
        }
        // === 9. 定义 ===
        {
            FoldSection* f = new FoldSection("定义", this);
            lb_masterd = new QLabel("");
            lb_slaved = new QLabel("");
            f->addRow(new PanelLabel("主面"), lb_masterd);
            f->addRow(new PanelLabel("从面"), lb_slaved);
            m_foldWidgets.append(f);
        }
        // === 10. 参数定义 ===
        {
            FoldSection* f = new FoldSection("参数定义", this);
            cmb_paradefine = new QComboBox();
            cmb_paradefine->addItems({ "零间隙配合","逐步消除间隙配合量" });
            f->addRow(new PanelLabel("间隙配合方式"), cmb_paradefine);

            lb_paraSize = new PanelLabel("大小");
            le_paraSize = new mCalcLineEdit(this, true, false);
            f->addRow(lb_paraSize, le_paraSize);

            //函数
            combofunc = new comboBoxWithPushBtn();
            combofunc->initWidget("函数");
            combofunc->labelname->setFixedWidth(95);
            combofunc->pushbtn->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/hanshu.png"));

            f->addRow(combofunc->propwid);

            m_foldWidgets.append(f);


            //函数切换添加映射（走 contact API 写入映射并记录命令）
            connect(combofunc->combobox, &QComboBox::currentTextChanged, this, [=](QString text) {
                if (!_contactData) return;
                commitContactField(QStringLiteral("function"), text);
                });

            //新建函数，添加映射
            connect(combofunc->pushbtn, &QPushButton::clicked, this, [this]() {
                if (!_contactData) return;
                int contactId = _contactData->getContactId();  // 先保存 ID，因为 unEditInfoWindow 会清空 _contactData
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createFunction(2);

                //重新填充函数下拉框，把新建的放到第一个
                combofunc->combobox->blockSignals(true);
                combofunc->combobox->clear();
                auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
                for (auto funcid : allFunc)
                {
                    QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
                    combofunc->combobox->addItem(funcname);
                }
                combofunc->combobox->setCurrentIndex(allFunc.size() - 1);
                combofunc->combobox->blockSignals(false);
                //获取当前下拉框显示，走 contact API 写入映射并记录命令
                commitContactField(QStringLiteral("function"), combofunc->combobox->currentText());
                });

            connect(cmb_paradefine, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int idx) {
                if (_contactData)
                {
                    commitContactField(QStringLiteral("clrFitType"), QString::number(idx));
                }
                // 关键！选项改变导致下面两行显隐变化
                updateRowVisibility();
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Contact, _contactData->getContactId());
                });

            connect(le_paraSize, &QLineEdit::editingFinished, this, [=]() {
                if (_contactData)
                {
                    commitContactField(QStringLiteral("size"), le_paraSize->text());
                }
                });

        }

        // === 11. 类型定义 ===
        {
            FoldSection* f = new FoldSection("接触定义", this);
            f->addRow(new PanelLabel("接触类型："), new PanelLabel("过盈接触"));
            m_foldWidgets.append(f);
        }

        // === 12. 接触压力均匀性评估指标 ===
        {
            FoldSection* f = new FoldSection("接触压力均匀性评估指标", this); f->setCheckable(true);
            connect(f, &FoldWidget::enabledChanged, this, [=](bool c) {
                if (_contactData) { commitContactField(QStringLiteral("evaluation"), c ? QStringLiteral("true") : QStringLiteral("false")); }
                });
            chk_mp = new QCheckBox("平均压力");
            chk_sd = new QCheckBox("标准差");
            chk_cv = new QCheckBox("变异系数");
            chk_pc = new QCheckBox("极差");
            chk_r = new QCheckBox("峰值集中度");
            f->addRow(chk_mp);
            f->addRow(chk_sd);
            f->addRow(chk_cv);
            f->addRow(chk_pc);
            f->addRow(chk_r);
            m_foldWidgets.append(f);
            connect(chk_mp, &QCheckBox::toggled, this, [=](bool c) {
                if (_contactData)
                {
                    commitContactField(QStringLiteral("evaMP"), c ? QStringLiteral("true") : QStringLiteral("false"));
                } });
            connect(chk_sd, &QCheckBox::toggled, this, [=](bool c) {
                if (_contactData)
                {
                    commitContactField(QStringLiteral("evaSD"), c ? QStringLiteral("true") : QStringLiteral("false"));
                } });
            connect(chk_cv, &QCheckBox::toggled, this, [=](bool c) {
                if (_contactData)
                {
                    commitContactField(QStringLiteral("evaCV"), c ? QStringLiteral("true") : QStringLiteral("false"));
                } });
            connect(chk_pc, &QCheckBox::toggled, this, [=](bool c) {
                if (_contactData)
                {
                    commitContactField(QStringLiteral("evaPC"), c ? QStringLiteral("true") : QStringLiteral("false"));
                } });
            connect(chk_r, &QCheckBox::toggled, this, [=](bool c) {
                if (_contactData)
                {
                    commitContactField(QStringLiteral("evaR"), c ? QStringLiteral("true") : QStringLiteral("false"));
                } });
        }
    }

    void mContactPanel::loadDataToUI()
    {
        if (!_contactData) return;

        // 统一 block/unblock 信号，避免反显时触发各控件槽
        // 注意：FoldSection 是 QWidget 子类，setChecked 会触发 enabledChanged，必须屏蔽
        QObject* const signalTargets[] = {
            cmb_type, cmb_smallSlip, cmb_nodeOver, cmb_nodeConst,
            le_nodeScale, le_nodePenalty, cmb_tanFric, le_tanCoef,
            le_gapVal, le_geomVal, cmb_obj, combofunc->combobox,
            cmb_paradefine, le_paraSize,  // 过盈接触：间隙配合方式、大小
            m_foldWidgets.at(4),  // 切向行为 FoldSection
            m_foldWidgets.at(5),  // 热接触 FoldSection
            m_foldWidgets.at(6),  // 接触间隙 FoldSection
            m_foldWidgets.at(7),  // 几何调整 FoldSection
            m_foldWidgets.at(12), // 评估指标 FoldSection
            chk_mp, chk_sd, chk_cv, chk_pc, chk_r  // 评估指标复选框
        };
        const auto blockAll = [&]() {
            for (auto* w : signalTargets) if (w) w->blockSignals(true);
            };
        const auto unblockAll = [&]() {
            for (auto* w : signalTargets) if (w) w->blockSignals(false);
            };
        blockAll();

        auto masterFunction = [this]() {
            if (pick_master)
            {
                PickWidgetData masterData;
                loadContactDataFromSet(MapRelation_Master, masterData);
                auto strategy = std::make_shared<APIPersistenceStrategy>(
                    MxFunctions::Physical_Contact,
                    _contactData->getContactId(), MapRelation_Master);
                pick_master->setPersistenceStrategy(strategy);

                // 集合类型化解析器（高亮用）
                pick_master->setSetResolver(&resolveSetToTypedData);

                // 集合列表可能随模型变化，每次设值时刷新。
                // 下拉列表从根上不出现非法类型集合，与工具栏按钮置灰同源。
                const bool is2D = _contactData && _contactData->getContactType() == MxFunctions::TwoDimensionalContact;
                QHash<QString, QString> setToType = is2D
                    ? getSetNamesList({ SetType::GeoEdge, SetType::GeoFace, SetType::ElementEdge })
                    : getSetNamesList({ SetType::GeoFace, SetType::ElementSurface });
                pick_master->setSelectSetList(setToType);
                pick_master->displayData(masterData);
                //判断拾取控件是否为空 以设置标志位
                _contactData->setPickStateMaster(pick_master->getAllData().isEmpty() ? 1 : 2);
            }
            };

        auto slaveFunction = [this]() {
            if (pick_slave)
            {
                PickWidgetData slaveData;
                loadContactDataFromSet(MapRelation_Slave, slaveData);
                auto strategy = std::make_shared<APIPersistenceStrategy>(
                    MxFunctions::Physical_Contact,
                    _contactData->getContactId(), MapRelation_Slave);
                pick_slave->setPersistenceStrategy(strategy);

                // 集合类型化解析器（高亮用）
                pick_slave->setSetResolver(&resolveSetToTypedData);

                // 集合列表可能随模型变化，每次设值时刷新。
                // 下拉列表从根上不出现非法类型集合，与工具栏按钮置灰同源。
                // 点面/线面接触节点表面模式可选几何集合、节点面集和部件集，其余保持几何面+单元面集合
                const bool is2D = _contactData && _contactData->getContactType() == MxFunctions::TwoDimensionalContact;
                QHash<QString, QString> setToType = slavePickIsNodeMode()
                    ? getSetNamesList({ SetType::GeoPoint, SetType::GeoEdge, SetType::GeoFace, SetType::GeoSolid, SetType::NodeSurface, SetType::Part })
                    : (is2D ? getSetNamesList({ SetType::GeoEdge, SetType::GeoFace, SetType::ElementEdge })
                        : getSetNamesList({ SetType::GeoFace,SetType::ElementSurface }));
                pick_slave->setSelectSetList(setToType);
                pick_slave->displayData(slaveData);
                _contactData->setPickStateSlave(pick_slave->getAllData().isEmpty() ? 1 : 2);
            }
            };

        auto faceFunction = [this]() {
            if (pick_face)
            {
                PickWidgetData faceData;
                loadContactDataFromSet(MapRelation_Master, faceData);
                auto strategy = std::make_shared<APIPersistenceStrategy>(
                    MxFunctions::Physical_Contact,
                    _contactData->getContactId(), MapRelation_Master);
                pick_face->setPersistenceStrategy(strategy);

                // 集合类型化解析器（高亮用）
                pick_face->setSetResolver(&resolveSetToTypedData);

                // 集合列表可能随模型变化，每次设值时刷新。
                // 下拉列表从根上不出现非法类型集合，与工具栏按钮置灰同源。
                QHash<QString, QString> setToType = getSetNamesList({ SetType::GeoFace,SetType::ElementSurface });
                pick_face->setSelectSetList(setToType);
                pick_face->displayData(faceData);
                _contactData->setPickStateMaster(pick_face->getAllData().isEmpty() ? 1 : 2);
            }
            };

        // type combo
        if (_contactData->getContactType() == MxFunctions::ContactType::SurfaceToSurfaceContact
            || _contactData->getContactType() == MxFunctions::ContactType::TwoDimensionalContact)
        {
            cmb_type->setCurrentText(_contactData->getContactType() == MxFunctions::ContactType::TwoDimensionalContact
                ? QStringLiteral("二维接触") : QStringLiteral("面面接触"));
            cmb_slaveRegion->hide();
            btn_swap->setVisible(true);
            pick_face->setPickWidgetVisible(false);
            pick_master->setPickWidgetVisible(true);
            pick_slave->setPickWidgetVisible(true);
            masterFunction();
            slaveFunction();
            // 从面仍处于激活态时（如节点拾取中直接切类型），按新类型刷新拾取过滤
            if (pick_slave && pick_slave->isPickActive())
            {
                pick_slave->setPickType();
            }
            if (!pick_master->isPickActive() && !pick_slave->isPickActive())
            {
                pick_master->setPickActive(true);
            }
            if (pick_master->isPickActive() || pick_slave->isPickActive())
            {
                const bool is2D = _contactData->getContactType() == MxFunctions::TwoDimensionalContact;
                emit mMechGlobalSignals::getInstance()->activePickSig(is2D ? 6 : 7);
                (pick_master->isPickActive() ? pick_master : pick_slave)->setPickType();
            }
            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Contact, _contactData->getContactId());
        }
        else if (_contactData->getContactType() == MxFunctions::ContactType::NodeToSurfaceContact
            || _contactData->getContactType() == MxFunctions::ContactType::LineToSurfaceContact)
        {
            cmb_type->setCurrentText(_contactData->getContactTypeStr());
            // 下拉框反显 slave_region（屏蔽信号，避免触发切换槽清空数据），从面刷新依赖该标记
            syncSlaveRegionCombo();
            cmb_slaveRegion->show();
            // 节点表面模式隐藏交换按钮（节点面集不可换作主面）
            btn_swap->setVisible(!slavePickIsNodeMode());
            pick_face->setPickWidgetVisible(false);
            pick_master->setPickWidgetVisible(true);
            pick_slave->setPickWidgetVisible(true);
            masterFunction();
            slaveFunction();
            // 从面仍处于激活态时（如其它拾取中直接切类型），按新类型刷新拾取过滤
            if (pick_slave && pick_slave->isPickActive())
            {
                pick_slave->setPickType();
            }
            if (!pick_master->isPickActive() && !pick_slave->isPickActive())
            {
                pick_master->setPickActive(true);
            }
            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Contact, _contactData->getContactId());
        }
        else if (_contactData->getContactType() == MxFunctions::ContactType::CommonContact)
        {
            cmb_type->setCurrentIndex(3);
            cmb_slaveRegion->hide();
            pick_face->setPickWidgetVisible(false);
            pick_master->setPickWidgetVisible(false);
            pick_slave->setPickWidgetVisible(false);
        }
        else if (_contactData->getContactType() == MxFunctions::ContactType::SelfContact)
        {
            cmb_type->setCurrentIndex(4);
            pick_master->setPickWidgetVisible(false);
            pick_slave->setPickWidgetVisible(false);
            pick_face->setPickWidgetVisible(true);
            faceFunction();
            pick_face->setPickActive(true);
            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Contact, _contactData->getContactId());
        }
        else if (_contactData->getContactType() == MxFunctions::ContactType::InterferenceContact)
        {
            //作用对象下拉框反写
            cmb_obj->clear();
            //获取面面接触、点面接触和二维接触
            QStringList nameList1 = getCVM->getModelData()->getManager<MechContactData::mContactManager>(Physical_Contact).getAllContactNamesByTypes(SurfaceToSurfaceContact);
            QStringList nameList2 = getCVM->getModelData()->getManager<MechContactData::mContactManager>(Physical_Contact).getAllContactNamesByTypes(NodeToSurfaceContact);
            QStringList nameList3 = getCVM->getModelData()->getManager<MechContactData::mContactManager>(Physical_Contact).getAllContactNamesByTypes(TwoDimensionalContact);
            QStringList nameList = nameList1 + nameList2 + nameList3;
            // 过滤被抑制的接触
            QStringList filteredNameList;
            for (const QString& contactName : nameList)
            {
                auto contactData = getCVM->getModelData()->getManager<MechContactData::mContactManager>(Physical_Contact).getContactData(contactName);
                if (contactData && contactData->isSuppressed())
                    continue;
                filteredNameList.append(contactName);
            }
            cmb_obj->addItems(filteredNameList);
            int id = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_Contact,_contactData->getContactId() }, Physical_Contact);
            if (id != -1)
            {
                QString name = getCVM->getModelData()->getManager<MechContactData::mContactManager>(Physical_Contact).getContactNameByID(id);
                cmb_obj->setCurrentText(name);
                //设置接触对显示状态
                auto data = getCVM->getModelData()->getManager<MechContactData::mContactManager>(Physical_Contact).getContactData(name);
                resetPartAlpha(false);
                updateContactPairUI(data);
            }
            else //无映射默认选第一个需添加映射，否则出现下拉框有东西，但映射没有添加上的bug
            {
                cmb_obj->setCurrentIndex(0);
                QString name = cmb_obj->currentText();
                // 走 contact API 写入接触对映射
                commitContactField(QStringLiteral("contactPair"), name);

                //设置接触对显示状态
                auto data = getCVM->getModelData()->getManager<MechContactData::mContactManager>(Physical_Contact).getContactData(name);
                resetPartAlpha(false);
                updateContactPairUI(data);
            }

            //间隙配合方式下拉框反写
            cmb_paradefine->setCurrentIndex(_contactData->clr_fit_typ);
            //间隙配合方式大小反写
            le_paraSize->setExpression(_contactData->size);
            //函数下拉框反写
            combofunc->combobox->clear();
            auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
            for (auto funcid : allFunc)
            {
                QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
                combofunc->combobox->addItem(funcname);
            }
            //获取映射
            int funid = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_Contact,_contactData->getContactId() }, Physical_Function);
            if (funid != -1)
            {
                QString name = getCVM->getModelData()->getManager< MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funid);
                combofunc->combobox->setCurrentText(name);
            }
            else //无映射默认选第一个需添加映射，否则出现下拉框有东西，但映射没有添加上的bug
            {
                // 走 contact API 写入函数映射（信号已屏蔽，手动提交）
                commitContactField(QStringLiteral("function"), combofunc->combobox->currentText());
            }
            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Contact, _contactData->getContactId());

            unblockAll();
            return;
        }

        // contact params
        cmb_smallSlip->setCurrentText(_contactData->smallSlip ? "开启" : "关闭");

        // normal
        cmb_nodeOver->setCurrentText(_contactData->overclosure);
        cmb_nodeConst->setCurrentText(_contactData->constraint);
        le_nodeScale->setExpression(_contactData->scale_factor);
        le_nodePenalty->setExpression(_contactData->penalty_factor);

        // tangential
        m_foldWidgets.at(4)->setChecked(_contactData->tangential_define);
        cmb_tanFric->setCurrentText(_contactData->tangential_friction);
        le_tanCoef->setExpression(QString::number(_contactData->tangential_static_coef));
        //cmb_tanFric->setEnabled(_contactData->tangential_define);
        //le_tanCoef->setEnabled(_contactData->tangential_define);

        // heat
        m_foldWidgets.at(5)->setChecked(_contactData->heat_define);
        //le_heatConst->setExpression(QString::number(_contactData->heat_constants));
        //le_heatCoef->setExpression(QString::number(_contactData->heat_pressure));
        //le_heatConst->setEnabled(_contactData->heat_define);
        //le_heatCoef->setEnabled(_contactData->heat_define);

        // gap
        m_foldWidgets.at(6)->setChecked(_contactData->gap_define);
        le_gapVal->setExpression(QString::number(_contactData->gap_overclosure));
        //le_gapVal->setEnabled(_contactData->gap_define);

        // geom
        m_foldWidgets.at(7)->setChecked(_contactData->geom_define);
        le_geomVal->setExpression(QString::number(_contactData->geom_value));
        //le_geomVal->setEnabled(_contactData->geom_define);

        // Eva
        m_foldWidgets.at(12)->setChecked(_contactData->evaluation);
        chk_mp->setChecked(_contactData->eva_MP);
        chk_sd->setChecked(_contactData->eva_SD);
        chk_cv->setChecked(_contactData->eva_CV);
        chk_pc->setChecked(_contactData->eva_PC);
        chk_r->setChecked(_contactData->eva_R);

        unblockAll();
    }

    void mContactPanel::updatePanel()
    {
        if (!_contactData) return;

        // 1. 加载数据到控件 (blockSignals防止循环触发)
        loadDataToUI();

        if (_contactData->getContactType() != CommonContact)
        {
            if (chk_transparent->isChecked())
            {
                transparentPart();
            }
        }
        else
        {
            resetPartAlpha(false);
        }

        if (_contactData->getContactType() != InterferenceContact)
        {
            updateContactPairUI();
        }

        // 2. 根据类型控制【折叠块】的显隐
        updateFoldVisibility();

        // 3. 根据具体参数值控制折叠块【内部行】的显隐
        updateRowVisibility();
        //_mainLayout->invalidate();
        //_mainLayout->activate();
        //this->updateGeometry();

    }

    void mContactPanel::updateFoldVisibility() {
        // 先全部移除/隐藏
        for (auto* f : m_foldWidgets) {
            _mainLayout->removeWidget(f);
            f->hide();
        }

        MxFunctions::ContactType type = _contactData->getContactType();

        // 辅助lambda：添加指定索引的折叠块
        auto addFold = [&](int index) {
            m_foldWidgets[index]->show();
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[index]);
            };

        // 接触间隙(6)是否显示：由接触类型决定（点面/线面接触不支持间隙），
        // 叠加小滑移状态（开启小滑移时隐藏间隙）。统一在此处决定，不再单独补一刀
        auto showGap = [&]() -> bool {
            return type != MxFunctions::ContactType::NodeToSurfaceContact
                && type != MxFunctions::ContactType::LineToSurfaceContact
                && !_contactData->smallSlip;
            };

        if (type != MxFunctions::ContactType::InterferenceContact) //点 面 通用 自接触
        {
            // 始终显示“类型” (Index 0)
            m_foldWidgets[0]->show();
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[0]);

            if (type == MxFunctions::ContactType::TwoDimensionalContact) {
                addFold(1); addFold(2); addFold(3); addFold(4);
                if (showGap()) addFold(6);
                addFold(7);
            }
            else if (type == MxFunctions::ContactType::SurfaceToSurfaceContact) {
                // 面面：接触对(1), 参数(2), 法向(3), 切向(4), 热(5), 间隙(6), 几何(7), 压评(12)
                addFold(1); addFold(2); addFold(3); addFold(4); addFold(5);
                if (showGap()) addFold(6);
                addFold(7); addFold(12);
            }
            else if (type == MxFunctions::ContactType::NodeToSurfaceContact
                || type == MxFunctions::ContactType::LineToSurfaceContact) {
                // 点面/线面：接触对(1), 参数(2), 法向(3), 切向(4), 热(5), 几何(7)；不用间隙(6)
                addFold(1); addFold(2); addFold(3); addFold(4); addFold(5); addFold(7);
            }
            else if (type == MxFunctions::ContactType::CommonContact) {
                // 通用：参数(2), 法向(3), 切向(4), 热(5), 间隙(6), 几何(7)
                // 通用接触不用接触对(1)
                addFold(2); addFold(3); addFold(4); addFold(5);
                if (showGap()) addFold(6);
                addFold(7);
            }
            else if (type == MxFunctions::ContactType::SelfContact) {
                // 自接触：接触对(1), 法向(3), 切向(4), 热(5), 间隙(6), 几何(7)
                addFold(1); addFold(3); addFold(4); addFold(5);
                if (showGap()) addFold(6);
                addFold(7);
            }
        }
        else //过盈接触面板与以上面板不同
        {
            addFold(11);
            addFold(8);
            addFold(9);
            addFold(10);
        }
    }

    void mContactPanel::updateRowVisibility() {

        if (!_contactData) return;

        // 冻结全局绘制
        this->setUpdatesEnabled(false);

        MxFunctions::ContactType type = _contactData->getContactType();
        {
            auto fold = m_foldWidgets[1];
            if (type == MxFunctions::ContactType::SelfContact)
            {
                fold->insertRow(1, wd_contactRow);
                wd_pairContainer->setParent(nullptr);
            }
            else
            {
                fold->insertRow(1, wd_pairContainer);
                wd_contactRow->setParent(nullptr);
            }
            // 面面切换点面/线面时，从面行由单控件变为下拉框+拾取控件；
            // 延迟刷新接触对内容高度，避免动态显示后仍沿用面面的行高。
            fold->refreshLayout();
        }


        // ========================================================
        // 接触参数
        // ========================================================
        bool isNode = (type == MxFunctions::ContactType::NodeToSurfaceContact);
        // 如果显隐发生了变化，立马刷新这一个折叠块
        //if (setRowVisible(lb_selfContact, cmb_selfContact, !isNode)) {
        //    m_foldWidgets[2]->refreshLayout();
        //}

        // ========================================================
        // 法向细节 (Index 3)
        // ========================================================
        {
            auto fold = m_foldWidgets[3];
            bool sectionChanged = false; // 标记该板块是否需要刷新

            QString over = cmb_nodeOver->currentText();
            const bool is2D = type == MxFunctions::ContactType::TwoDimensionalContact;
            const QStringList allowedOver = is2D
                ? QStringList{ QStringLiteral("硬接触"), QStringLiteral("线性接触") }
                : QStringList{ QStringLiteral("硬接触"), QStringLiteral("完全绑定接触"), QStringLiteral("法向不分离接触"), QStringLiteral("线性接触") };
            QStringList currentOverItems;
            for (int i = 0; i < cmb_nodeOver->count(); ++i) currentOverItems << cmb_nodeOver->itemText(i);
            if (!allowedOver.contains(_contactData->overclosure))
            {
                commitContactField(QStringLiteral("overclosure"), QStringLiteral("硬接触"));
            }
            cmb_nodeOver->blockSignals(true);
            const QString currentOver = _contactData->overclosure;
            if (currentOverItems != allowedOver)
            {
                cmb_nodeOver->clear();
                cmb_nodeOver->addItems(allowedOver);
            }
            cmb_nodeOver->setCurrentText(currentOver);
            cmb_nodeOver->blockSignals(false);
            over = cmb_nodeOver->currentText();

            // 动态填充 ComboBox 逻辑
            int oldCount = cmb_nodeConst->count();

            cmb_nodeConst->blockSignals(true);
            QString currentConst = cmb_nodeConst->currentText();
            cmb_nodeConst->clear();
            if (over == "硬接触") {
                cmb_nodeConst->addItems({ "罚函数法", "增广-摄动拉格朗日乘子法" });
            }
            else if (over == "完全绑定接触") {
                //if(isNode)
                //    cmb_nodeConst->addItems({ "多点约束法" });
                //else
                cmb_nodeConst->addItems({ "罚函数法","多点约束法" });
            }
            else if (over == "法向不分离接触") {
                //if (isNode)
                //    cmb_nodeConst->addItems({ "罚函数法", "多点约束法" });
                //else
                cmb_nodeConst->addItems({ "罚函数法","增广-摄动拉格朗日乘子法",  "多点约束法" });
            }
            cmb_nodeConst->setCurrentText(currentConst);

            //在新的列表里找不到
            if (cmb_nodeConst->currentIndex() == -1 && cmb_nodeConst->count() > 0)
            {
                // 强制选中第一个作为默认值
                cmb_nodeConst->setCurrentIndex(0);
            }

            cmb_nodeConst->blockSignals(false);
            if (_contactData->constraint != cmb_nodeConst->currentText())
            {
                commitContactField(QStringLiteral("constraint"), cmb_nodeConst->currentText());
            }

            if (cmb_nodeConst->count() != oldCount)
                sectionChanged = true;

            // 显隐逻辑
            bool showConst = !(over == "线性接触");
            if (cmb_nodeConst->count() == 0)
                showConst = false;

            if (over == "线性接触")
            {
                fold->insertRow(1, lb_nodePenalty, le_nodePenalty);
                lb_nodeConst->setParent(nullptr);cmb_nodeConst->setParent(nullptr);
                lb_nodeScale->setParent(nullptr);le_nodeScale->setParent(nullptr);
            }
            else
            {
                fold->insertRow(1, lb_nodeConst, cmb_nodeConst);
                // 从 API 写回后的数据中读取 constraint 判断显隐
                if (_contactData->constraint == "罚函数法")
                {
                    fold->insertRow(2, lb_nodeScale, le_nodeScale);
                }
                else
                {
                    lb_nodeScale->setParent(nullptr);le_nodeScale->setParent(nullptr);
                }
                lb_nodePenalty->setParent(nullptr);le_nodePenalty->setParent(nullptr);
            }

            fold->refreshLayout();

        }

        // ========================================================
        // 切向细节 (Index 5)
        // ========================================================
        {
            bool sectionChanged = false;
            auto fold = m_foldWidgets[4];

            const QStringList allowedFric = type == MxFunctions::ContactType::TwoDimensionalContact
                ? QStringList{ QStringLiteral("无摩擦"), QStringLiteral("通用接触摩擦") }
                : QStringList{ QStringLiteral("无摩擦"), QStringLiteral("通用接触摩擦"), QStringLiteral("粗糙") };
            QStringList currentFricItems;
            for (int i = 0; i < cmb_tanFric->count(); ++i) currentFricItems << cmb_tanFric->itemText(i);
            if (!allowedFric.contains(_contactData->tangential_friction))
            {
                commitContactField(QStringLiteral("tangentialFriction"), QStringLiteral("无摩擦"));
            }
            cmb_tanFric->blockSignals(true);
            if (currentFricItems != allowedFric)
            {
                cmb_tanFric->clear();
                cmb_tanFric->addItems(allowedFric);
            }
            cmb_tanFric->setCurrentText(_contactData->tangential_friction);
            cmb_tanFric->blockSignals(false);

            QString fric = cmb_tanFric->currentText();
            bool showCoef = (fric == "通用接触摩擦");

            if (showCoef) {
                if (lb_tanCoef->parent() == nullptr) {
                    fold->insertRow(1, lb_tanCoef, le_tanCoef);
                    lb_tanCoef->show();
                    le_tanCoef->show();
                    sectionChanged = true;
                }
                else if (!lb_tanCoef->isVisible()) {
                    lb_tanCoef->show();
                    le_tanCoef->show();
                    sectionChanged = true;
                }
            }
            else {
                if (lb_tanCoef->parent() != nullptr) {
                    lb_tanCoef->setParent(nullptr);
                    le_tanCoef->setParent(nullptr);
                    sectionChanged = true;
                }
            }

            if (sectionChanged) {
                fold->refreshLayout();
            }
        }

        // ========================================================
        // 6/7/8
        // ========================================================
        //bool heatDef = chk_heat->cb->isChecked();
        //le_heatConst->setEnabled(heatDef);
        //le_heatCoef->setEnabled(heatDef);

        //bool gapDef = chk_gap->isChecked();
        //le_gapVal->setEnabled(gapDef);

        //bool geomDef = chk_geom->isChecked();
        //le_geomVal->setEnabled(geomDef);

        //间隙配合方式
        {
            auto fold = m_foldWidgets[10];
            QString fric = cmb_paradefine->currentText();

            if (fric == "零间隙配合") //移除大小和函数
            {
                lb_paraSize->setParent(nullptr);
                le_paraSize->setParent(nullptr);
                combofunc->propwid->setParent(nullptr);
            }
            else if (fric == "逐步消除间隙配合量")
            {
                fold->insertRow(1, lb_paraSize, le_paraSize);
                fold->insertRow(2, combofunc->propwid);
            }
            m_foldWidgets[10]->refreshLayout();
        }
        // 解冻全局绘制
        this->setUpdatesEnabled(true);
    }

    void mContactPanel::updateContactPairUI(MechContactData::mContactBase* contactData)
    {
        if (lb_masterd) lb_masterd->setText("当前未选择");
        if (lb_slaved) lb_slaved->setText("当前未选择");

        if (!contactData) return;

        // 清空面高亮并重新渲染
        clearFaceHighlight();
        auto render = AllRenderSymbolManager::getInstance();
        render->rendContact(contactData);

        // 主/从面是否已选择由通用拾取控件自行显示，这里仅保留定义折叠块的文本提示
        auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
        auto hasSetData = [&](MxFunctions::MapRelationType mapType) -> bool {
            int setId = mapRelationManager.getMapRelation<int>(
                { PhysicalFieldType::Physical_Contact, contactData->getContactId() }, Physical_Set, mapType);
            if (setId == -1) return false;
            mSetBase* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setId);
            if (!set) return false;
            if (set->getSetType() == Mix)
            {
                auto mixset = dynamic_cast<mMixSet*>(set);
                if (!mixset) return false;
                for (const auto& pair : mixset->getMixSetData())
                {
                    if (!pair.second.empty()) return true;
                }
            }
            else if (set->getSetType() == GeoFace || set->getSetType() == GeoEdge)
            {
                auto geoset = dynamic_cast<mGeoSet*>(set);
                if (geoset && !geoset->getGeoSetData().empty()) return true;
            }
            else if (set->getSetType() == ElementSurface || set->getSetType() == ElementEdge)
            {
                auto eleset = dynamic_cast<mElementSurface*>(set);
                if (eleset && !eleset->getElementSurfaceData().empty()) return true;
            }
            else if (set->getSetType() == MxFunctions::Node)
            {
                auto nodeset = dynamic_cast<mNode*>(set);
                if (nodeset && !nodeset->getNodeData().empty()) return true;
            }
            else if (set->getSetType() == MxFunctions::NodeSurface)
            {
                auto nodeFaceSet = dynamic_cast<mNodeFace*>(set);
                if (nodeFaceSet && !nodeFaceSet->getNodeData().empty()) return true;
            }
            return false;
            };

        if (lb_masterd && hasSetData(MapRelation_Master))
            lb_masterd->setText("已选择对象");
        if (lb_slaved && hasSetData(MapRelation_Slave))
            lb_slaved->setText("已选择对象");
    }

    void mContactPanel::updateContactPairUI()
    {
        if (!_contactData) return;

        // 清空拾取与面高亮，重新按当前映射渲染
        //getCVM->clearPick();
        clearFaceHighlight();

        auto render = AllRenderSymbolManager::getInstance();
        render->rendContact(_contactData);
    }

    void mContactPanel::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (!_contactData)
        {
            return;
        }
        if (!_contactData->heat_define)
        {
            return;
        }

        if (x.size() < 1 || y.size() < 1)
        {
            return;
        }

        // 从输入参数组装热传导表，直接调 API（不直接写 _contactData）
        auto xdata = x.first();
        auto ydata = y.first();
        QStringList pairList;
        for (int i = 0; i < xdata.size(); ++i)
        {
            pairList << QStringLiteral("%1:%2").arg(xdata.at(i)).arg(ydata.at(i));
        }
        commitContactField(QStringLiteral("heatValue"), pairList);

    }

    void mContactPanel::slot_commitMasterContactData()
    {
        if (!getCVM)
        {
            return;
        }
        auto widget_ptr = getCVM->getPreRenderWidget();
        auto highlight_controller = widget_ptr->getHighlightController();

        auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("red"), true, -2.0f);
        geoHighLightRender->setHighLightColor(255, 0, 0);
        mGeoSelectData* geoData = new mGeoSelectData();
        geoData->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoFace, {});
        geoData->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoEdge, {});

        auto meshHighLightRender = highlight_controller->getExtraPreMeshHighLightRender(std::string("red"));
        meshHighLightRender->setHighLightColor(255, 0, 0);
        MeshSelectMemoryData* meshData = new MeshSelectMemoryData();
        meshData->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshFace, {});
        meshData->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshLine, {});
        meshData->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshNode, {});

        std::vector<RenderPickResultData> meshRenderData, meshEdgeRenderData;
        std::vector<int> geoRenderData, geoEdgeRenderData;
        std::vector<mxr::MeshSelectInfoData> nodeRenderData;
        AllRenderSymbolManager::getInstance()->rendContact(pick_master->getAllData(), meshRenderData, geoRenderData, &nodeRenderData, &meshEdgeRenderData, &geoEdgeRenderData);

        // 一次性更新几何高亮
        if (geoHighLightRender && (geoRenderData.size() > 0 || geoEdgeRenderData.size() > 0))
        {
            geoData->UpdatePickData(mxr::SelectionOperate::ADD, SelectionInfoType::GeoFace, geoRenderData);
            geoData->UpdatePickData(mxr::SelectionOperate::ADD, SelectionInfoType::GeoEdge, geoEdgeRenderData);
        }

        // 一次性更新网格高亮
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

        if (meshHighLightRender && meshEdgeRenderData.size() > 0)
        {
            std::vector<mxr::MeshSelectInfoData> meshIds;
            meshIds.reserve(meshEdgeRenderData.size());
            for (const auto& d : meshEdgeRenderData)
            {
                mxr::MeshSelectInfoData n;
                n.data_id.element_id = d.result_id;
                n.data_index.line_index = d.result_id2;
                n.part_id = d.part_id;
                meshIds.push_back(n);
            }
            meshData->UpdateSelectDataByType(mxr::SelectionOperate::ADD, SelectionInfoType::MeshLine, meshIds);
        }

        // 一次性更新节点高亮（交换后主面可能持有节点数据）
        if (meshHighLightRender && nodeRenderData.size() > 0)
        {
            meshData->UpdateSelectDataByType(mxr::SelectionOperate::ADD, SelectionInfoType::MeshNode, nodeRenderData);
        }

        geoHighLightRender->updateHighLightRender(geoData);
        meshHighLightRender->updateHighLightRender(meshData);
        //显式请求重绘：拾取过程中高亮数据已就绪，须在本次交互内立即上色
        widget_ptr->updatePaint();

        //同从面：延迟一拍重建点图元，避免被防抖重建的主拾取高亮点盖住（交换后主面可能持节点数据）
        deferExtraMeshHighlightRebuild(widget_ptr, "red", 255, 0, 0, meshData, 1);
        delete geoData;
        //判断拾取控件是否为空 以设置标志位
        _contactData->setPickStateMaster(pick_master->getAllData().isEmpty() ? 1 : 2);
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Contact, _contactData->getContactId());
    }

    void mContactPanel::slot_commitSlaveContactData()
    {
        if (!getCVM)
        {
            return;
        }
        auto widget_ptr = getCVM->getPreRenderWidget();
        auto highlight_controller = widget_ptr->getHighlightController();

        auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("blue"), true, -2.0f);
        geoHighLightRender->setHighLightColor(0, 0, 255);
        mGeoSelectData* geoData = new mGeoSelectData();
        geoData->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoFace, {});
        geoData->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoEdge, {});

        auto meshHighLightRender = highlight_controller->getExtraPreMeshHighLightRender(std::string("blue"));
        meshHighLightRender->setHighLightColor(0, 0, 255);
        MeshSelectMemoryData* meshData = new MeshSelectMemoryData();
        meshData->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshFace, {});
        meshData->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshLine, {});
        meshData->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshNode, {});

        std::vector<RenderPickResultData> meshRenderData, meshEdgeRenderData;
        std::vector<int> geoRenderData, geoEdgeRenderData;
        std::vector<mxr::MeshSelectInfoData> nodeRenderData;
        AllRenderSymbolManager::getInstance()->rendContact(pick_slave->getAllData(), meshRenderData, geoRenderData, &nodeRenderData, &meshEdgeRenderData, &geoEdgeRenderData);

        // 一次性更新几何高亮
        if (geoHighLightRender && (geoRenderData.size() > 0 || geoEdgeRenderData.size() > 0))
        {
            geoData->UpdatePickData(mxr::SelectionOperate::ADD, SelectionInfoType::GeoFace, geoRenderData);
            geoData->UpdatePickData(mxr::SelectionOperate::ADD, SelectionInfoType::GeoEdge, geoEdgeRenderData);
        }

        // 一次性更新网格高亮
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

        if (meshHighLightRender && meshEdgeRenderData.size() > 0)
        {
            std::vector<mxr::MeshSelectInfoData> meshIds;
            meshIds.reserve(meshEdgeRenderData.size());
            for (const auto& d : meshEdgeRenderData)
            {
                mxr::MeshSelectInfoData n;
                n.data_id.element_id = d.result_id;
                n.data_index.line_index = d.result_id2;
                n.part_id = d.part_id;
                meshIds.push_back(n);
            }
            meshData->UpdateSelectDataByType(mxr::SelectionOperate::ADD, SelectionInfoType::MeshLine, meshIds);
        }

        // 一次性更新节点高亮：点面接触从面为节点模式时，拾取的节点与几何面/单元面
        // 一样临时高亮成从面颜色块的颜色（蓝色）
        if (meshHighLightRender && nodeRenderData.size() > 0)
        {
            meshData->UpdateSelectDataByType(mxr::SelectionOperate::ADD, SelectionInfoType::MeshNode, nodeRenderData);
        }

        geoHighLightRender->updateHighLightRender(geoData);
        meshHighLightRender->updateHighLightRender(meshData);
        //显式请求重绘：拾取过程中高亮数据已就绪，须在本次交互内立即上色（含节点变蓝），
        //不能依赖共享高亮服务防抖定时器的补绘时机
        widget_ptr->updatePaint();

        //节点点图元与主拾取高亮的点图元同挂 ui_root，且每次更新都会重建点图元（追加到
        // ui_root 末尾、最后绘制）：本槽同步更新后，拾取控件自身 applyHighlight 启动的
        // 0ms 防抖会用共享容器重建主渲染器点图元，标准拾取色点将盖住本渲染器的蓝点。
        // 故延迟一拍（排在防抖之后）重建本渲染器点图元，保证拾取完毕立即以从面蓝色显示
        deferExtraMeshHighlightRebuild(widget_ptr, "blue", 0, 0, 255, meshData, 1);
        delete geoData;
        //判断拾取控件是否为空 以设置标志位
        _contactData->setPickStateSlave(pick_slave->getAllData().isEmpty() ? 1 : 2);
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Contact, _contactData->getContactId());
    }

    void mContactPanel::slot_commitSelfContactData()
    {
        if (!getCVM)
        {
            return;
        }
        auto widget_ptr = getCVM->getPreRenderWidget();
        auto highlight_controller = widget_ptr->getHighlightController();

        auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("red"), true, -2.0f);
        geoHighLightRender->setHighLightColor(255, 0, 0);
        mGeoSelectData* geoData = new mGeoSelectData();
        geoData->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoFace, {});
        geoData->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoEdge, {});

        auto meshHighLightRender = highlight_controller->getExtraPreMeshHighLightRender(std::string("red"));
        meshHighLightRender->setHighLightColor(255, 0, 0);
        MeshSelectMemoryData* meshData = new MeshSelectMemoryData();
        meshData->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshFace, {});
        meshData->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshLine, {});
        meshData->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshNode, {});

        std::vector<RenderPickResultData> meshRenderData; std::vector<int> geoRenderData;
        std::vector<mxr::MeshSelectInfoData> nodeRenderData;
        AllRenderSymbolManager::getInstance()->rendContact(pick_face->getAllData(), meshRenderData, geoRenderData, &nodeRenderData);

        // 一次性更新几何高亮
        if (geoHighLightRender && geoRenderData.size() > 0)
        {
            geoData->UpdatePickData(mxr::SelectionOperate::ADD, SelectionInfoType::GeoFace, geoRenderData);
        }

        // 一次性更新网格高亮
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

        // 一次性更新节点高亮
        if (meshHighLightRender && nodeRenderData.size() > 0)
        {
            meshData->UpdateSelectDataByType(mxr::SelectionOperate::ADD, SelectionInfoType::MeshNode, nodeRenderData);
        }

        geoHighLightRender->updateHighLightRender(geoData);
        meshHighLightRender->updateHighLightRender(meshData);
        //显式请求重绘：拾取后立即上色
        widget_ptr->updatePaint();
        delete geoData;
        delete meshData;
        //判断拾取控件是否为空 以设置标志位
        _contactData->setPickStateMaster(pick_face->getAllData().isEmpty() ? 1 : 2);
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Contact, _contactData->getContactId());
    }

    mContactPanel::~mContactPanel()
    {
        if (_mainLayout != nullptr)
        {
            delete _mainLayout;
            _mainLayout = nullptr;
        }

    }

    void mContactPanel::newContactPanel(ContactType type)
    {
        // 接触类型枚举 → 中文字符串（与 contact API 的 type 键对应）
        auto typeToStr = [](ContactType t) -> QString {
            switch (t)
            {
            case SurfaceToSurfaceContact: return QStringLiteral("面面接触");
            case TwoDimensionalContact:   return QStringLiteral("二维接触");
            case NodeToSurfaceContact:    return QStringLiteral("点面接触");
            case LineToSurfaceContact:    return QStringLiteral("线面接触");
            case CommonContact:           return QStringLiteral("通用接触");
            case SelfContact:             return QStringLiteral("自接触");
            case InterferenceContact:     return QStringLiteral("过盈接触");
            }
            return QStringLiteral("面面接触");
        };

        // ① 生成创建命令（名称由 createNameByType 预生成，与 API 创建后一致）
        const QString contactName = getCVM->getModelData()->createNameByType(Physical_Contact, "Contact");
        const QString typeStr = typeToStr(type);
        const QString cmd = QStringLiteral("Contact(\"%1\", type=\"%2\").create()").arg(contactName, typeStr);

        // ② 先落盘到日志文件：即使后续 API 执行中闪退，肇事命令也已记录
        MechPreData::mPythonAgent::getInstance()->logCommandToFile(cmd);

        // ③ 走 contact API 创建数据（isCreate=true）
        APIServer::APIParams params;
        params.insert(QStringLiteral("isCreate"), QStringList{ QStringLiteral("true") });
        params.insert(QStringLiteral("name"), QStringList{ contactName });
        params.insert(QStringLiteral("type"), QStringList{ typeStr });
        const auto apiResult = getAPISchedulingServer->executeAPI(QStringLiteral("contact"), params);
        if (!apiResult._success)
            return;

        // ④ 成功才入命令历史
        MechPreData::mPythonAgent::getInstance()->addCommandToHistory(cmd);

        // API 创建后取回数据对象进入编辑（API 内部已发 createUpdateContactSig 建树）
        _contactData = getCVM->getModelData()->getManager<mContactManager>(Physical_Contact).getContactData(contactName);
        if (!_contactData) return;
        setEditing(true);
    }

    void mContactPanel::editContactPanel(int contactID)
    {
        _contactData = getCVM->getModelData()->getManager<mContactManager>(Physical_Contact).getContactData(contactID);
        if (_contactData == nullptr)
        {
            return;
        }
        setEditing(true);

        connectSig();
        updatePanel();
    }

    void mContactPanel::onContactDataAboutToBeDestroyed(int contactID)
    {
        //必须在数据对象 delete 之前调用：此时 _contactData 仍有效，安全丢弃悬垂指针
        if (contactID >= 0)
        {
            if (_contactData == nullptr || contactID != _contactData->getContactId())
                return;
        }
        _contactData = nullptr;
    }

    void mContactPanel::uneditContactPanel()
    {
        if (!isEditing())
        {
            return;
        }
        //入口统一复位编辑态：后续各早退分支（数据为空/已删除）无需再单独清标志
        setEditing(false);
        if (!getCVM)
            return;
        if (!getCVM->getPreRenderWidget())
            return;
        //_contactData 为裸指针，数据被删除时由 onContactDataAboutToBeDestroyed 提前置空，
        //此处 nullptr 即表示数据已删，直接退出不走切出收尾
        if (_contactData == nullptr)
        {
            return;
        }
        deactivatePickAndCommit();
        _contactData = nullptr;
        disconnectSig();
        auto viewportPrivate = VMManager->getCurrentViewportPrivate();
        MechPreData::mPreViewportPrivate* preViewport = nullptr;
        if (viewportPrivate != nullptr)
        {
            preViewport = dynamic_cast<MechPreData::mPreViewportPrivate*>(viewportPrivate);
            if (preViewport == nullptr)
            {
                return;
            }
        }
        resetPartAlpha();
        clearFaceHighlight();
    }

    void mContactPanel::slot_matComboBoxChanged(const QString& text)
    {
        if (getCVM == nullptr)
        {
            return;
        }
        if (_contactData == nullptr)
        {
            return;
        }
        auto& materialManager = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material);
        auto mat = materialManager.getMaterialData(text);
        if (mat == nullptr)
        {
            return;
        }
    }

    void mContactPanel::resetPartAlpha(bool isClear)
    {
        if (!getCVM)
        {
            return;
        }
        std::vector<int> partIds = getCVM->getModelData()->getMeshData()->getAllPartIds();
        std::set<int> sId;
        for (auto id : partIds)
        {
            sId.insert(id);
        }
        if (getCVM->getPreRenderWidget())
        {
            getCVM->getPreRenderWidget()->getPreRender()->setTransparent(true, 1.0);
            getCVM->updateAllModelOperate(ColorOnePart, sId);
            getCVM->updateRender();
        }
    }

    void mContactPanel::transparentPart()
    {
        if (!getCVM)
        {
            return;
        }
        //透明所有部件
        //获取部件颜色
        std::vector<int> partIds = getCVM->getModelData()->getMeshData()->getAllPartIds();
        std::set<int> sId;
        for (auto id : partIds)
        {
            sId.insert(id);
        }
        if (!sId.empty())
        {
            if (getCVM->getPreRenderWidget())
            {
                getCVM->getPreRenderWidget()->getPreRender()->setTransparent(true, 0.02f);
                //getCVM->updateRender();
                //reducePreWidgetSelectPartID(getCVM->getPreRenderWidget(), partIds);
                getCVM->updateAllModelOperate(ColorOnePart, sId);
                getCVM->updateRender();
            }
        }
    }

    void mContactPanel::clearFaceHighlight()
    {
        if (!getCVM)
        {
            return;
        }
        auto widget_ptr = getCVM->getPreRenderWidget();
        auto highlight_controller = widget_ptr->getHighlightController();

        auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("red"), true, -2.0f);
        mGeoSelectData* geoData = new mGeoSelectData();
        geoData->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoFace, {});
        geoData->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoEdge, {});
        geoHighLightRender->updateHighLightRender(geoData);

        auto meshHighLightRender = highlight_controller->getExtraPreMeshHighLightRender(std::string("red"));
        MeshSelectMemoryData* meshData = new MeshSelectMemoryData();
        meshData->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshFace, {});
        meshData->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshLine, {});
        meshData->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshNode, {});
        meshHighLightRender->updateHighLightRender(meshData);

        geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("blue"), true, -2.0f);
        geoData->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoFace, {});
        geoData->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoEdge, {});
        geoHighLightRender->updateHighLightRender(geoData);

        meshHighLightRender = highlight_controller->getExtraPreMeshHighLightRender(std::string("blue"));
        meshData->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshFace, {});
        meshData->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshLine, {});
        meshData->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshNode, {});
        meshHighLightRender->updateHighLightRender(meshData);
    }

    void mContactPanel::connectSig()
    {
        disconnectSig();
        connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mContactPanel::slot_connTable);
    }

    void mContactPanel::disconnectSig()
    {
        disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mContactPanel::slot_connTable);
    }

    void mContactPanel::deactivatePickAndCommit()
    {
        const bool renderValid = (getCVM && getCVM->getPreRenderWidget());
        for (auto* ctrl : { pick_master,pick_slave ,pick_face })
        {
            if (!ctrl) continue;
            // 切出/取消编辑面板：失活不保留临时高亮（置标志 false），失活提交即清空高亮。
            ctrl->setKeepHighlightOnDeactivate(false);
            // 失活提交：激活态时执行完整 commit 流程 + 清空分支 clearAllHighlight 清空高亮；
            // 已失活时幂等返回（不重复 commit、不清高亮）。
            const bool wasActive = ctrl->isPickActive();
            ctrl->setPickActive(false);
            // 兜底：仅对"此前已被同面板另一控件抢活而处于失活临时高亮态"的控件清空残留。
            // 激活控件已由 setPickActive(false) 清空分支清过，此处跳过避免重复清空。
            if (renderValid && !wasActive)
            {
                ctrl->clearAllHighlight();
            }
            // 恢复默认（保留），供下次进入面板/同面板多控件切换使用。
            ctrl->setKeepHighlightOnDeactivate(true);
        }

        // 切出面板：恢复拾取工具栏全部按钮，避免常驻置灰影响其它面板
        emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>(10, true));
    }

    QString mContactPanel::contactCmdPrefix() const
    {
        if (!_contactData) return QString();
        return QStringLiteral("Contact(\"%1\")").arg(_contactData->getContactName());
    }

    void mContactPanel::commitContactField(const QString& fieldKey, const QString& fieldValue)
    {
        commitContactField(fieldKey, QStringList{ fieldValue });
    }

    void mContactPanel::commitContactField(const QString& fieldKey, const QStringList& fieldValues)
    {
        if (!_contactData) return;

        // ① 生成对应 Python 命令
        QString cmd;
        const QString prefix = contactCmdPrefix();
        const QString v = fieldValues.isEmpty() ? QString() : fieldValues.first();

        if (fieldKey == QStringLiteral("smallSlip"))
            cmd = QStringLiteral("%1.set_small_slip(%2).update()").arg(prefix, v == QStringLiteral("true") ? QStringLiteral("True") : QStringLiteral("False"));
        else if (fieldKey == QStringLiteral("type"))
            cmd = QStringLiteral("%1.set_type(\"%2\").update()").arg(prefix, v);
        else if (fieldKey == QStringLiteral("slaveRegion"))
            cmd = QStringLiteral("%1.set_slave_region(%2).update()").arg(prefix, v);
        else if (fieldKey == QStringLiteral("selfContactSearch"))
            cmd = QStringLiteral("%1.set_self_contact_search(%2).update()").arg(prefix, v == QStringLiteral("true") ? QStringLiteral("True") : QStringLiteral("False"));
        else if (fieldKey == QStringLiteral("overclosure"))
            cmd = QStringLiteral("%1.set_pressure_overclosure(\"%2\").update()").arg(prefix, v);
        else if (fieldKey == QStringLiteral("constraint"))
            cmd = QStringLiteral("%1.set_constraint(\"%2\").update()").arg(prefix, v);
        else if (fieldKey == QStringLiteral("scaleFactor"))
            cmd = QStringLiteral("%1.set_scale_factor(\"%2\").update()").arg(prefix, v);
        else if (fieldKey == QStringLiteral("penaltyFactor"))
            cmd = QStringLiteral("%1.set_penalty_factor(\"%2\").update()").arg(prefix, v);
        else if (fieldKey == QStringLiteral("tangentialDefine"))
            cmd = QStringLiteral("%1.set_tangential(%2, \"%3\", \"%4\").update()").arg(prefix,
                v == QStringLiteral("true") ? QStringLiteral("True") : QStringLiteral("False"),
                _contactData->tangential_friction, QString::number(_contactData->tangential_static_coef));
        else if (fieldKey == QStringLiteral("tangentialFriction"))
            cmd = QStringLiteral("%1.set_tangential(%2, \"%3\", \"%4\").update()").arg(prefix,
                _contactData->tangential_define ? QStringLiteral("True") : QStringLiteral("False"),
                v, QString::number(_contactData->tangential_static_coef));
        else if (fieldKey == QStringLiteral("tangentialStaticCoef"))
            cmd = QStringLiteral("%1.set_tangential(%2, \"%3\", \"%4\").update()").arg(prefix,
                _contactData->tangential_define ? QStringLiteral("True") : QStringLiteral("False"),
                _contactData->tangential_friction, v);
        else if (fieldKey == QStringLiteral("heatDefine"))
            cmd = QStringLiteral("%1.set_heat_conductance(%2).update()").arg(prefix, v == QStringLiteral("true") ? QStringLiteral("True") : QStringLiteral("False"));
        else if (fieldKey == QStringLiteral("heatValue"))
        {
            QStringList quoted;
            for (const QString& item : fieldValues)
                quoted << QStringLiteral("\"%1\"").arg(item);
            cmd = QStringLiteral("%1.set_heat_conductance(True, [%2]).update()").arg(prefix, quoted.join(QStringLiteral(",")));
        }
        else if (fieldKey == QStringLiteral("gapDefine"))
            cmd = QStringLiteral("%1.set_gap(%2, \"%3\").update()").arg(prefix,
                v == QStringLiteral("true") ? QStringLiteral("True") : QStringLiteral("False"),
                QString::number(_contactData->gap_overclosure));
        else if (fieldKey == QStringLiteral("gapOverclosure"))
            cmd = QStringLiteral("%1.set_gap(%2, \"%3\").update()").arg(prefix,
                _contactData->gap_define ? QStringLiteral("True") : QStringLiteral("False"), v);
        else if (fieldKey == QStringLiteral("geomDefine"))
            cmd = QStringLiteral("%1.set_geom_adjust(%2, \"%3\").update()").arg(prefix,
                v == QStringLiteral("true") ? QStringLiteral("True") : QStringLiteral("False"),
                QString::number(_contactData->geom_value));
        else if (fieldKey == QStringLiteral("geomValue"))
            cmd = QStringLiteral("%1.set_geom_adjust(%2, \"%3\").update()").arg(prefix,
                _contactData->geom_define ? QStringLiteral("True") : QStringLiteral("False"), v);
        else if (fieldKey == QStringLiteral("clrFitType"))
            cmd = QStringLiteral("%1.set_clearance_fit(\"%2\", \"%3\").update()").arg(prefix, v, _contactData->size);
        else if (fieldKey == QStringLiteral("size"))
            cmd = QStringLiteral("%1.set_clearance_fit(\"%2\", \"%3\").update()").arg(prefix,
                QString::number(_contactData->clr_fit_typ), v);
        else if (fieldKey == QStringLiteral("master"))
            cmd = QStringLiteral("%1.set_master(\"%2\").update()").arg(prefix, fieldValues.join(QStringLiteral("/")));
        else if (fieldKey == QStringLiteral("slave"))
            cmd = QStringLiteral("%1.set_slave(\"%2\").update()").arg(prefix, fieldValues.join(QStringLiteral("/")));
        else if (fieldKey == QStringLiteral("contactPair"))
            cmd = QStringLiteral("%1.set_contact_pair(\"%2\").update()").arg(prefix, v);
        else if (fieldKey == QStringLiteral("function"))
            cmd = QStringLiteral("%1.set_function(\"%2\").update()").arg(prefix, v);
        else if (fieldKey == QStringLiteral("analysisStep"))
            cmd = QStringLiteral("%1.set_analysis_step(\"%2\").update()").arg(prefix, v);
        else if (fieldKey == QStringLiteral("swap"))
            cmd = QStringLiteral("%1.swap().update()").arg(prefix);
        else if (fieldKey == QStringLiteral("evaluation"))
            cmd = QStringLiteral("%1.set_evaluation(%2).update()").arg(prefix, v == QStringLiteral("true") ? QStringLiteral("True") : QStringLiteral("False"));
        else if (fieldKey == QStringLiteral("evaMP"))
            cmd = QStringLiteral("%1.set_evaluation(True, mp=%2).update()").arg(prefix, v == QStringLiteral("true") ? QStringLiteral("True") : QStringLiteral("False"));
        else if (fieldKey == QStringLiteral("evaSD"))
            cmd = QStringLiteral("%1.set_evaluation(True, sd=%2).update()").arg(prefix, v == QStringLiteral("true") ? QStringLiteral("True") : QStringLiteral("False"));
        else if (fieldKey == QStringLiteral("evaCV"))
            cmd = QStringLiteral("%1.set_evaluation(True, cv=%2).update()").arg(prefix, v == QStringLiteral("true") ? QStringLiteral("True") : QStringLiteral("False"));
        else if (fieldKey == QStringLiteral("evaPC"))
            cmd = QStringLiteral("%1.set_evaluation(True, pc=%2).update()").arg(prefix, v == QStringLiteral("true") ? QStringLiteral("True") : QStringLiteral("False"));
        else if (fieldKey == QStringLiteral("evaR"))
            cmd = QStringLiteral("%1.set_evaluation(True, r=%2).update()").arg(prefix, v == QStringLiteral("true") ? QStringLiteral("True") : QStringLiteral("False"));

        if (cmd.isEmpty()) return;

        // ② 先落盘到日志文件：即使后续 API 执行中闪退，导致崩溃的命令也已记录
        MechPreData::mPythonAgent::getInstance()->logCommandToFile(cmd);

        // ③ 增量调用 contact API（isCreate=false），只带变更字段
        APIServer::APIParams params;
        params.insert(QStringLiteral("isCreate"), QStringList{ QStringLiteral("false") });
        params.insert(QStringLiteral("name"), QStringList{ _contactData->getContactName() });
        params.insert(fieldKey, fieldValues);
        const auto apiResult = getAPISchedulingServer->executeAPI(QStringLiteral("contact"), params);

        // ④ API 执行成功才入命令历史（供脚本导出回放；失败命令不改变数据，不入历史）
        if (apiResult._success)
            MechPreData::mPythonAgent::getInstance()->addCommandToHistory(cmd);
    }

    void mContactPanel::loadContactDataFromSet(MxFunctions::MapRelationType mapType, PickWidgetData& outData)
    {
        outData.clear();
        if (!_contactData) return;

        auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
        int setId = mapRelationManager.getMapRelation<int>(
            { PhysicalFieldType::Physical_Contact, _contactData->getContactId() }, Physical_Set, mapType);
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
        else if (set->getSetType() == GeoFace || set->getSetType() == ElementSurface
            || set->getSetType() == GeoEdge || set->getSetType() == ElementEdge
            || set->getSetType() == GeoPoint || set->getSetType() == GeoSolid
            || set->getSetType() == Part
            || set->getSetType() == MxFunctions::Node || set->getSetType() == MxFunctions::NodeSurface)
        {
            outData.selectedSetNames.insert(set->getSetName());
        }
    }

    bool mContactPanel::slavePickIsNodeMode() const
    {
        // 从面节点表面/单元面统一以数据对象的 slave_region 为唯一判据（下拉框仅作显示）。
        // 从面拾取控件为多类型共用：点面/线面接触且 slave_region=1 为节点表面模式，
        // 其余场景一律按面拾取，避免模式串扰
        const auto type = _contactData ? _contactData->getContactType() : MxFunctions::ContactType::SurfaceToSurfaceContact;
        return _contactData
            && (type == MxFunctions::ContactType::NodeToSurfaceContact
                || type == MxFunctions::ContactType::LineToSurfaceContact)
            && _contactData->slave_region == 1;
    }

    void mContactPanel::syncSlaveRegionCombo()
    {
        if (!cmb_slaveRegion || !_contactData) return;

        // 统一以数据对象的 slave_region 反显（0=单元面，1=节点表面），不再按集合数据反推；
        // 屏蔽信号避免触发切换槽（该槽会清空从面数据）
        cmb_slaveRegion->blockSignals(true);
        cmb_slaveRegion->setCurrentIndex(_contactData->slave_region == 1 ? 1 : 0);
        cmb_slaveRegion->blockSignals(false);
    }

    void mContactPanel::applySlaveRegionPickTypes()
    {
        if (!pick_slave) return;

        // 集合选择类型跟随模式：节点表面模式可选几何集合、节点面集和部件集，面模式保持几何面+单元面集合
        QHash<QString, QString> setToType;
        if (_contactData && _contactData->getContactType() == MxFunctions::TwoDimensionalContact)
        {
            setToType = getSetNamesList({ SetType::GeoEdge, SetType::GeoFace, SetType::ElementEdge });
        }
        else
        {
            setToType = slavePickIsNodeMode()
                ? getSetNamesList({ SetType::GeoPoint, SetType::GeoEdge, SetType::GeoFace, SetType::GeoSolid, SetType::NodeSurface, SetType::Part })
                : getSetNamesList({ SetType::GeoFace, SetType::ElementSurface });
        }
        pick_slave->setSelectSetList(setToType);

        // 从面处于激活态时立即重设拾取工具栏类型过滤（节点/面互斥）
        if (pick_slave->isPickActive())
        {
            pick_slave->setPickType();
        }
    }

    void mContactPanel::deferExtraMeshHighlightRebuild(mxr::CCSMRendWidget* widgetPtr, const char* colorKey,
        int r, int g, int b, MeshSelectMemoryData* meshData, int hops)
    {
        QTimer::singleShot(0, this, [this, widgetPtr, colorKey, r, g, b, meshData, hops]() {
            // 视口已切换/销毁：丢弃数据防悬垂
            if (!getCVM || !getCVM->getPreRenderWidget() || getCVM->getPreRenderWidget() != widgetPtr)
            {
                delete meshData;
                return;
            }
            // 尚未排到共享高亮服务防抖之后：继续跳
            if (hops > 1)
            {
                deferExtraMeshHighlightRebuild(widgetPtr, colorKey, r, g, b, meshData, hops - 1);
                return;
            }
            auto meshRender = widgetPtr->getHighlightController()->getExtraPreMeshHighLightRender(std::string(colorKey));
            if (meshRender)
            {
                meshRender->setHighLightColor(r, g, b);
                meshRender->updateHighLightRender(meshData);
            }
            widgetPtr->updatePaint();
            delete meshData;
            });
    }

    void mContactPanel::deferActivateNodePointTopUp(mPickWidgetController* picker)
    {
        if (!picker || !getCVM || !getCVM->getPreRenderWidget()) return;

        // 仅有节点数据时受 ui_root 点图元顺序影响（面图元挂各自渲染器私有 geode，顺序稳定）
        std::vector<RenderPickResultData> meshRenderData; std::vector<int> geoRenderData;
        std::vector<mxr::MeshSelectInfoData> nodeRenderData;
        AllRenderSymbolManager::getInstance()->rendContact(picker->getAllData(), meshRenderData, geoRenderData, &nodeRenderData);
        if (nodeRenderData.empty()) return;

        auto* meshData = new MeshSelectMemoryData();
        if (!meshRenderData.empty())
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

        meshData->UpdateSelectDataByType(mxr::SelectionOperate::ADD, SelectionInfoType::MeshNode, nodeRenderData);

        // 主面/自接触面红色，从面蓝色；两跳：排到激活流程 applyHighlight 启动的防抖之后
        if (picker == pick_slave)
        {
            deferExtraMeshHighlightRebuild(getCVM->getPreRenderWidget(), "blue", 0, 0, 255, meshData, 2);
        }
        else
        {
            deferExtraMeshHighlightRebuild(getCVM->getPreRenderWidget(), "red", 255, 0, 0, meshData, 2);
        }
    }
}