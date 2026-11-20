#include "mSetPanel.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "PickCommonFunction.h"

//通用拾取控件
#include "mGeneralPickUp.h"
#include "mPickWidgetController.h"
#include "IPickPersistenceStrategy.h"

//Functoin
#include "mPhysicalFieldEnum.h"

//BasicData
#include "mSetBase.h"
#include "mNode.h"
#include "mFrequencyPoint.h"
#include "mDOFNode.h"
#include "mElementSet.h"
#include "mElementSurface.h"
#include "mGeoSet.h"
#include "mPartSet.h"
#include "mModelData.h"
#include "mPanel.h"
#include "mMapRelationManager.h"
#include "mDataStatusManager.h"
#include "mNodeFace.h"
#include "mMeshData.h"
#include "mSurfaceAnalysis.h"

//Qt
#include <QComboBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QCompleter>
#include "mOtherFunction.h"


using namespace MechPreData;
using namespace MechSetData;
using namespace MxFunctions;
using namespace mxr;
using namespace MechMapRelation;

namespace MechMainWindow
{
    mSetPanel::mSetPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _setData = nullptr;

        //获取当前视口的数据
        _mainLayout = new QVBoxLayout(this);

        //属性组面板
        // 创建折叠面板
        FoldSection* defineFold = new FoldSection("属性组", this);

        /*
        // ====== 复选框示例开始 ======
        // 1. 显示标题栏左侧的复选框
        defineFold->setCheckable(true);

        // 2. 复选框状态变化时发送信号，外部连接修改存储数据
        connect(defineFold, &FoldWidget::enabledChanged, this, [this](bool checked) {
            //if (_setData)
                //_setData->enabled = checked;   // 示例：修改业务数据
            });
        */

        _hintLabel = new PanelLabel("类型");
        //类型
        _labelType = new QLabel("节点集");
        _comboLabelType = new QComboBox(); _comboLabelType->addItems({ "旋转解析平面","拉伸解析平面","解析线段" });
        defineFold->addRow(_hintLabel, _labelType);
        //是否排序
        _labelOrder = new PanelLabel("排序:");
        //_labelOrder->setAlignment(Qt::AlignLeft);
        _checkboxOrder = new QCheckBox();
        defineFold->addRow(_labelOrder, _checkboxOrder);
        // 范围：通用拾取控件（不可选择集合，仅存储高亮）
        _pickWidget = mGeneralPickUp::createSetPickWidget(CBBlue, defineFold);
        _pickWidget->setDefultTip("选择对象");
        defineFold->addRow(_pickWidget->widget());
        m_foldWidgets.append(defineFold);
        _mainLayout->addWidget(defineFold);

        //频率点集
        _labelFrequencyPoint = new PanelLabel("频率点");
        _frequencyPoint = new QLineEdit("");

        QRegularExpression regExp(R"(^[0-9,.\s]*$)");
        auto* validator = new QRegularExpressionValidator(regExp, this);
        _frequencyPoint->setValidator(validator);

        defineFold->addRow(_labelFrequencyPoint, _frequencyPoint);

        connect(_frequencyPoint, &QLineEdit::editingFinished,
            this, &mSetPanel::slot_lineEditChanged);

        //统计面板
        FoldSection* statsFold = new FoldSection("统计", this);
        //其他统计数据
        //_labelStates = new QLabel("无");
        //_labelStatesData = new QLabel("无");
        //statsFold->addRow(_labelStates, _labelStatesData, 1);

        initFrequencyPointTabel();
        statsFold->addRow(_widgetFrequencyPoint);
        statsFold->addRow(_tableFrequencyPoint);

        _dofLayout = new QHBoxLayout;
        _labelDofNode = new PanelLabel("节点");
        _lineEditDofNode = new QLineEdit("");
        QRegularExpression regExpDofLineEdit(R"(^[0-9,:\s]*$)");
        auto* validatorDof = new QRegularExpressionValidator(regExpDofLineEdit, this);
        _lineEditDofNode->setValidator(validatorDof);

        _btnHighLight = new QPushButton("高亮");
        _btnHighLight->setFixedWidth(30);
        _dofLayout->addWidget(_labelDofNode);
        _dofLayout->addWidget(_lineEditDofNode, 1);
        _dofLayout->addWidget(_btnHighLight);
        statsFold->addRow(_dofLayout);

        _labelDof = new PanelLabel("自由度");
        _comboDof = new QComboBox(); _comboDof->addItems({ "1", "2", "3", "4", "5", "6" });
        _comboDof->setCurrentIndex(1);
        statsFold->addRow(_labelDof, _comboDof);

        connect(_btnHighLight, &QPushButton::clicked, this, &mSetPanel::slot_highLightBtnClicked);
        connect(_comboDof, &QComboBox::currentTextChanged, this, &mSetPanel::slot_comboDofChanged);

        //面板集合
        _elementModel = new QStringListModel();

        //_labelInfo = new QLabel("单元集合");
        //statsFold->addRow(_labelInfo);
        _comboPanel = new QComboBox();
        _comboPanel->addItems({ "等效辐射功率面板集合", "单元/节点集合" });
        _comboPanel->setCurrentIndex(0);
        statsFold->addRow(_comboPanel);
        connect(_comboPanel, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int id) {
                QString type = _labelType->text();
                if (type == "面板集")
                {
                    auto set = this->toType<mPanel>(_setData);
                    if (set)
                    {
                        set->setPanelCollectionType(id);
                        enableCollectNode = (id == 1);
                        _listAssignElement->clear();

                        if (id == 1)
                            set->elenames().clear();
                        else if (id == 0)
                            set->_nodeNames.clear();
                    }


                    // 面板集合类型切换后刷新列表显示，仅显示当前选项对应的数据
                    refreshPanelListByCollectionType();

                    if (getCVM && _setData)
                    {
                        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Set, _setData->getSetId());
                    }
                }
            });

        QGridLayout* assignLayout = new QGridLayout();
        _listAssignElement = new QListWidget();
        _listAssignElement->setSelectionMode(QAbstractItemView::ExtendedSelection);
        _allEleBtn = new QToolButton();
        _allEleBtn->setIcon(QIcon(":/mMainWidgets/Icon/xuanzequanbu.png"));
        _allEleBtn->setToolTip("全部单元集");
        _addEleBtn = new QToolButton();
        _addEleBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        _addEleBtn->setToolTip("增加单元集");
        _reduceEleBtn = new QToolButton();
        _reduceEleBtn->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        _reduceEleBtn->setToolTip("减少单元集");
        _deleteEleBtn = new QToolButton();
        _deleteEleBtn->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png"));
        _deleteEleBtn->setToolTip("清空单元集");

        assignLayout->addWidget(_listAssignElement, 0, 0, 8, 7);
        assignLayout->addWidget(_allEleBtn, 0, 7);
        assignLayout->addWidget(_addEleBtn, 1, 7);
        assignLayout->addWidget(_reduceEleBtn, 2, 7);
        assignLayout->addWidget(_deleteEleBtn, 3, 7);

        statsFold->addRow(assignLayout);
        _listAssignElement->setVisible(false);

        connect(_listAssignElement, &QListWidget::itemClicked, this, &mSetPanel::slot_elementItemClicked);
        connect(_allEleBtn, &QPushButton::pressed, [this]() {
            //赋予全部部件为该属性
            });
        connect(_allEleBtn, &QToolButton::clicked, this, &mSetPanel::slot_allElementSetBtn);
        connect(_addEleBtn, &QToolButton::clicked, this, &mSetPanel::slot_addElementSetBtn);
        connect(_reduceEleBtn, &QToolButton::clicked, this, &mSetPanel::slot_reduceElementSetBtn);
        connect(_deleteEleBtn, &QToolButton::clicked, this, &mSetPanel::slot_clearElementSetBtn);

        m_foldWidgets.append(statsFold);
        _mainLayout->addWidget(statsFold);


        //通用拾取控件：数据变化即提交到集合对象；失活（确定语义）时刷新树与状态
        connect(_pickWidget, &mPickWidgetController::dataChanged, this, &mSetPanel::slot_pickDataChanged);
        connect(_pickWidget, &mPickWidgetController::pickActiveChanged, this, &mSetPanel::slot_pickActiveChanged);

        //=====圆柱局部=====
        FoldSection* Fold1 = new FoldSection("圆柱局部坐标系定义", this);
        _labelCoordinateA = new PanelLabel("旋转轴任意点A:");
        _labelCoordinateB = new PanelLabel("旋转轴任意点B:");
        _Xa = new QLineEdit("");
        _Ya = new QLineEdit("");
        _Za = new QLineEdit("");
        _Xb = new QLineEdit("");
        _Yb = new QLineEdit("");
        _Zb = new QLineEdit("");
        Fold1->addRow(_labelCoordinateA, _Xa, _Ya, _Za);
        Fold1->addRow(_labelCoordinateB, _Xb, _Yb, _Zb);
        m_foldWidgets.append(Fold1);
        _mainLayout->addWidget(Fold1);
        // 起点
        FoldSection* Fold2 = new FoldSection("起点坐标", this);
        _labelCoordinateRZ1 = new PanelLabel("rz局部坐标:");
        _r1 = new QLineEdit("");
        _z1 = new QLineEdit("");
        Fold2->addRow(_labelCoordinateRZ1, _r1, _z1);
        m_foldWidgets.append(Fold2);
        _mainLayout->addWidget(Fold2);
        // 终点
        FoldSection* Fold3 = new FoldSection("终点坐标", this);
        _labelCoordinateRZ2 = new PanelLabel("rz局部坐标:");
        _r2 = new QLineEdit("");
        _z2 = new QLineEdit("");
        Fold3->addRow(_labelCoordinateRZ2, _r2, _z2);
        m_foldWidgets.append(Fold3);
        _mainLayout->addWidget(Fold3);

        //=====拉伸解析平面=====
        FoldSection* Fold4 = new FoldSection("局部坐标系定义", this);
        _labelOrigin = new PanelLabel("原点:");
        _labelXAxis = new PanelLabel("局部坐标系x正轴:");
        _labelZNegAxis = new PanelLabel("局部坐标系z负轴:");
        _Ox = new QLineEdit("");
        _Oy = new QLineEdit("");
        _Oz = new QLineEdit("");
        _Xx = new QLineEdit("");
        _Xy = new QLineEdit("");
        _Xz = new QLineEdit("");
        _Zx = new QLineEdit("");
        _Zy = new QLineEdit("");
        _Zz = new QLineEdit("");
        Fold4->addRow(_labelOrigin, _Ox, _Oy, _Oz);
        Fold4->addRow(_labelXAxis, _Xx, _Xy, _Xz);
        Fold4->addRow(_labelZNegAxis, _Zx, _Zy, _Zz);
        m_foldWidgets.append(Fold4);
        _mainLayout->addWidget(Fold4);

        //起点坐标
        FoldSection* Fold5 = new FoldSection("起点坐标", this);
        _labelStartLocal = new PanelLabel("xy局部坐标:");
        _Sx = new QLineEdit("");
        _Sy = new QLineEdit("");
        Fold5->addRow(_labelStartLocal, _Sx, _Sy);
        m_foldWidgets.append(Fold5);
        _mainLayout->addWidget(Fold5);

        //终点坐标
        FoldSection* Fold6 = new FoldSection("终点坐标", this);
        _labelEndLocal = new PanelLabel("xy局部坐标:");
        _labelExtent = new PanelLabel("z轴拉伸长度:");
        _Ex = new QLineEdit("");
        _Ey = new QLineEdit("");
        _El = new QLineEdit("");
        Fold6->addRow(_labelEndLocal, _Ex, _Ey);
        Fold6->addRow(_labelExtent, _El);
        m_foldWidgets.append(Fold6);
        _mainLayout->addWidget(Fold6);

        //=====解析线段=====
        FoldSection* Fold7 = new FoldSection("局部坐标系定义", this);
        _labelSegOrigin = new PanelLabel("原点:");
        _labelSegXAxis = new PanelLabel("局部坐标系x正轴:");
        _labelSegZNegAxis = new PanelLabel("局部坐标系z负轴:");
        _segOx = new QLineEdit("");
        _segOy = new QLineEdit("");
        _segOz = new QLineEdit("");
        _segXx = new QLineEdit("");
        _segXy = new QLineEdit("");
        _segXz = new QLineEdit("");
        _segZx = new QLineEdit("");
        _segZy = new QLineEdit("");
        _segZz = new QLineEdit("");
        Fold7->addRow(_labelSegOrigin, _segOx, _segOy, _segOz);
        Fold7->addRow(_labelSegXAxis, _segXx, _segXy, _segXz);
        Fold7->addRow(_labelSegZNegAxis, _segZx, _segZy, _segZz);
        m_foldWidgets.append(Fold7);
        _mainLayout->addWidget(Fold7);
        //起点坐标
        FoldSection* Fold8 = new FoldSection("起点:", this);
        _labelSegStartLocal = new PanelLabel("xy局部坐标:");
        _segSx = new QLineEdit("");
        _segSy = new QLineEdit("");
        Fold8->addRow(_labelSegStartLocal, _segSx, _segSy);
        m_foldWidgets.append(Fold8);
        _mainLayout->addWidget(Fold8);

        FoldSection* Fold9 = new FoldSection("线段轮廓:", this);
        initSegContourTable();
        Fold9->addRow(_widgetContourBtn);
        Fold9->addRow(_tableSegContour);
        m_foldWidgets.append(Fold9);
        _mainLayout->addWidget(Fold9);

        _mainLayout->addStretch();
        // 解析表面所有输入框：编辑完成自动保存
        QList<QLineEdit*> analyticEdits = {
            _Xa, _Ya, _Za, _Xb, _Yb, _Zb, _r1, _z1, _r2, _z2,           // 旋转
            _Ox, _Oy, _Oz, _Xx, _Xy, _Xz, _Zx, _Zy, _Zz, _Sx, _Sy, _Ex, _Ey, _El,  // 拉伸
            _segOx, _segOy, _segOz, _segXx, _segXy, _segXz, _segZx, _segZy, _segZz, _segSx, _segSy  // 线段
        };
        for (auto* edit : analyticEdits)
        {
            connect(edit, &QLineEdit::editingFinished,
                this, &mSetPanel::slot_analyticalSurfaceDataChanged);
        }
        connect(_z2, &QLineEdit::editingFinished, this, &mSetPanel::createSymbol);
        connect(_El, &QLineEdit::editingFinished, this, &mSetPanel::createSymbol);

        // 解析表面相关折叠面板默认隐藏
        Fold1->setVisible(false);
        Fold2->setVisible(false);
        Fold3->setVisible(false);
        Fold4->setVisible(false);
        Fold5->setVisible(false);
        Fold6->setVisible(false);
        Fold7->setVisible(false);
        Fold8->setVisible(false);
        Fold9->setVisible(false);

        // 解析表面子类型切换：实时更新数据类子类型 + 切换显隐
        connect(_comboLabelType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                Q_UNUSED(index);
                if (_setData && _setData->getSetType() == MxFunctions::SetType::SurfaceAnalysis)
                {
                    auto sa = toType<mSurfaceAnalysis>(_setData);
                    if (sa)
                    {
                        sa->setSurfaceSubTypeByName(_comboLabelType->currentText());
                    }
                    updateAnalyticalSurfaceFolds();
                    loadAnalyticalSurfaceData();
                    emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateSetSig(false, _setData->getSetId());
                }
            });

        //newSetPanel();

        //connect(MechPreData::mViewportModelManager::getInstance(), &MechPreData::mViewportModelManager::currentViewportChanged, this, &mSetPanel::slot_subWindowActivated);
    }

    mSetPanel::~mSetPanel()
    {
        if (_mainLayout != nullptr)
        {
            delete _mainLayout;
            _mainLayout = nullptr;
        }

    }

    void mSetPanel::newSetPanel()
    {
        //切换编辑对象前先失活提交上一对象的拾取数据
        deactivatePickWidget();
        connectSig();
        _setData = nullptr;
        setEditing(true);
        if (!getCVM)
        {
            return;
        }
        clearSet();
        //重置面板，类型根据拾取模式来确定
        auto pickFilter = getCVM->getPreRenderWidget()->getPickFilter();
        QString labelType = getSetTypeStringByPickFilter(pickFilter);

        updateSetPanel(labelType);

       //立即创建集合对象并上树（拾取型集合数据由通用拾取控件提交）
       okSet();
       //绑定存储策略并激活拾取（激活即存储高亮）
       activatePickWidget();
    }

    void mSetPanel::newSetPanel(QString setName)
    {
        //切换编辑对象前先失活提交上一对象的拾取数据
        deactivatePickWidget();
        connectSig();
        _setData = nullptr;
        setEditing(true);
        if (!getCVM)
        {
            return;
        }
        clearSet();

        //重置面板，类型根据入参确定
        QString labelType = setName;

        updateSetPanel(labelType);

        //立即创建集合对象并上树（拾取型集合数据由通用拾取控件提交）
        okSet();
        //绑定存储策略并激活拾取（激活即存储高亮）
        activatePickWidget();
    }

    void mSetPanel::editSetPanel(int setID)
    {
        //切换编辑对象前先失活提交上一对象的拾取数据
        deactivatePickWidget();
        connectSig();
        clearSet();
        _setData = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setID);
        if (_setData == nullptr)
        {
            return;
        }
        setEditing(true);
        //根据集合类型和数据更新面板
        QString type = _setData->getSetTypeStr();
        bool order = false;

        auto fold = m_foldWidgets[0];
        QLayoutItem* item;
        while ((item = fold->layout()->takeAt(0)) != nullptr)
        {
            if (item->widget())
            {
                item->widget()->setParent(nullptr);  // 移除旧 widget
            }
            delete item;
        }
        auto statefold = m_foldWidgets[1];
        while ((item = statefold->layout()->takeAt(0)) != nullptr)
        {
            if (item->widget())
            {
                item->widget()->setParent(nullptr);  // 移除旧 widget
            }
            delete item;
        }
        fold->setVisible(true);
        if (type == "节点集")
        {
            _labelOrder->setParent(nullptr);
            _checkboxOrder->setParent(nullptr);

            fold->addRow(_hintLabel, _labelType);
            fold->addRow(_labelOrder, _checkboxOrder);
            fold->addRow(_pickWidget->widget());

            auto nodeSet = this->toType<mNode>(_setData);
            order = nodeSet->getOrder();
        }
        else if(type == "单元集")
        {
            _labelOrder->setParent(nullptr);
            _checkboxOrder->setParent(nullptr);


            fold->addRow(_hintLabel, _labelType);
            fold->addRow(_labelOrder, _checkboxOrder);
            fold->addRow(_pickWidget->widget());

            auto eleSet = this->toType<mElementSet>(_setData);
            order = eleSet->getOrder();
        }
        else if (type == "单元表面集")
        {
            _labelOrder->setParent(nullptr);
            _checkboxOrder->setParent(nullptr);

            fold->addRow(_hintLabel, _labelType);
            //fold->addRow(_labelOrder, _checkboxOrder);
            fold->addRow(_pickWidget->widget());
        }
        else if (type == "单元边集")
        {
            _labelOrder->setParent(nullptr);
            _checkboxOrder->setParent(nullptr);

            fold->addRow(_hintLabel, _labelType);
            //fold->addRow(_labelOrder, _checkboxOrder);
            fold->addRow(_pickWidget->widget());
        }
        else if (type == "解析表面集")
        {
            _labelType->setParent(nullptr);
            _labelOrder->setParent(nullptr);
            _checkboxOrder->setParent(nullptr);
            //解析表面集不做拾取：从面板移除通用拾取控件
            _pickWidget->widget()->setParent(nullptr);

            // 统一数据类，按子类型成员确定下拉框
            auto sa = toType<mSurfaceAnalysis>(_setData);
            QString subType = sa ? sa->surfaceSubTypeName() : "旋转解析平面";

            _comboLabelType->blockSignals(true);
            _comboLabelType->setCurrentText(subType);
            _comboLabelType->blockSignals(false);

            // 加载数据到界面
            loadAnalyticalSurfaceData();

            // 显示对应的折叠面板
            updateAnalyticalSurfaceFolds();
        }
        else if (type.contains("几何"))
        {
            _labelOrder->setParent(nullptr);
            _checkboxOrder->setParent(nullptr);

            fold->addRow(_hintLabel, _labelType);
            //fold->addRow(_labelOrder, _checkboxOrder);
            fold->addRow(_pickWidget->widget());
        }
        else if (type == "部件集")
        {

            _labelOrder->setParent(nullptr);
            _checkboxOrder->setParent(nullptr);

            fold->addRow(_hintLabel, _labelType);
            //fold->addRow(_labelOrder, _checkboxOrder);
            fold->addRow(_pickWidget->widget());
            }
        else if (type == "频率点集")
        {


            _labelFrequencyPoint->setParent(nullptr);
            _frequencyPoint->setParent(nullptr);
            _addBtn->setParent(nullptr);
            _reduceBtn->setParent(nullptr);
            _clearBtn->setParent(nullptr);
            _tableFrequencyPoint->setParent(nullptr);

            fold->addRow(_hintLabel, _labelType);
            fold->addRow(_labelFrequencyPoint, _frequencyPoint);

            auto widget = new QWidget;
            auto layout = new QHBoxLayout;
            layout->addWidget(_addBtn);
            layout->addWidget(_reduceBtn);
            layout->addWidget(_clearBtn);
            layout->addStretch();
            layout->setContentsMargins(0, 0, 0, 0);
            widget->setLayout(layout);

            statefold->addRow(widget);
            statefold->addRow(_tableFrequencyPoint);

            auto frequencyPointSet = this->toType<mFrequencyPoint>(_setData);
            if (frequencyPointSet)
            {
                // 使用数据类中的频率点回显界面
                QString frequencyText;
                for (int i = 0; i < frequencyPointSet->orderCount(); ++i)
                {
                    if (i > 0)
                        frequencyText += ", ";
                    frequencyText += MxFunctions::mNumberToString(frequencyPointSet->orderSecondValueAt(i));
                }
                _frequencyPoint->setText(frequencyText);
            }
            updateFrequencyTable(false);
        }
        else if (type == "自由度节点集")
        {
            _labelOrder->setParent(nullptr);
            _checkboxOrder->setParent(nullptr);

            _labelDofNode->setParent(nullptr);
            _lineEditDofNode->setParent(nullptr);
            _btnHighLight->setParent(nullptr);

            _labelDof->setParent(nullptr);
            _comboDof->setParent(nullptr);


            fold->addRow(_hintLabel, _labelType);
            fold->addRow(_pickWidget->widget());

            auto* dofLayout = new QHBoxLayout;
            dofLayout->addWidget(_labelDofNode);
            dofLayout->addWidget(_lineEditDofNode, 1);
            dofLayout->addWidget(_btnHighLight);
            statefold->addRow(dofLayout);
            statefold->addRow(_labelDof, _comboDof);
            //同面板集：激活布局确保子布局内控件完成重挂，再进入后续刷新
            statefold->layout()->activate();

            auto dofNodeSet = this->toType<mDOFNode>(_setData);
            auto nodeData = dofNodeSet->getNodeData();
            order = dofNodeSet->getOrder();
            QString nodeidstr = ""; int count = 0;
            for (auto node : nodeData)
            {
                nodeidstr += MxFunctions::mNumberToString(node.result_id);

                ++count;
                if (count != nodeData.size())
                    nodeidstr += ", ";

            }
            _lineEditDofNode->setText(nodeidstr);

            int dofIndex = dofNodeSet->dof();
            _comboDof->setCurrentIndex(dofIndex - 1);

        }
        else if (type == "面板集")
        {
            _labelFrequencyPoint->setParent(nullptr);
            _frequencyPoint->setParent(nullptr);
            _addBtn->setParent(nullptr);
            _reduceBtn->setParent(nullptr);
            _clearBtn->setParent(nullptr);
            _tableFrequencyPoint->setParent(nullptr);

            //新增下拉框
            _comboPanel->setParent(nullptr);

            //statefold->addRow(_labelInfo);
            statefold->addRow(_comboPanel);
            auto* assignLayout = new QGridLayout;
            assignLayout->addWidget(_listAssignElement, 0, 0, 8, 7);
            assignLayout->addWidget(_allEleBtn, 0, 7);
            assignLayout->addWidget(_addEleBtn, 1, 7);
            assignLayout->addWidget(_reduceEleBtn, 2, 7);
            assignLayout->addWidget(_deleteEleBtn, 3, 7);
            statefold->addRow(assignLayout);
            //子布局挂入后立即激活，确保其中的控件已重挂到折叠框；
            //若此时仍无父窗口，下方的 setVisible(true) 会把控件显示成独立浮窗
            statefold->layout()->activate();
            _listAssignElement->setVisible(true);
            _allEleBtn->setVisible(true);
            _addEleBtn->setVisible(true);
            _reduceEleBtn->setVisible(true);
            _deleteEleBtn->setVisible(true);

            fold->setVisible(false);

            _listAssignElement->clear();
            //从集合id的映射中更新单元集的内容
            int panelId = _setData->getSetId();
            auto set = this->toType<mPanel>(_setData);

            //获取所有单元集名称
            auto elementLists = getCVM->getModelData()
                ->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(Element);
            auto nodeSetLists = getCVM->getModelData()
                ->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(Node);

            //回显时使用数据中存储的面板集合类型，并屏蔽 currentIndexChanged 信号避免触发清空
            int collectionType = set ? set->getPanelCollectionType() : 0;
            _comboPanel->blockSignals(true);
            _comboPanel->setCurrentIndex(collectionType);
            enableCollectNode = (collectionType == 1);
            _comboPanel->blockSignals(false);

            // 根据当前面板集合类型，仅显示对应的数据
            refreshPanelListByCollectionType();
        }
        else if (type == "节点面集")
        {
            // 移除排序控件（节点面集无排序）
            _labelOrder->setParent(nullptr);
            _checkboxOrder->setParent(nullptr);

            // 面板布局：类型 + 拾取控件
            fold->addRow(_hintLabel, _labelType);
            fold->addRow(_pickWidget->widget());
        }

        updateSetPanel(type, order);

        //绑定存储策略、反显集合并激活拾取（激活即存储高亮）
        activatePickWidget();
    }

    void mSetPanel::uneditSetPanel()
    {
        if (!isEditing())
        {
            return;
        }
        //入口统一复位编辑态：后续各早退分支无需再单独清标志
        setEditing(false);
        //失活拾取控件：触发 commit 持久化并清理高亮
        deactivatePickWidget();
        _setData = nullptr;
        disconnectSig();
        MechCommon::mMechGlobalSignals::getInstance()->outputPermanentInfoSig("");
    }

    void mSetPanel::updateSetPanel(QString type, bool order)
    {
        MechCommon::mMechGlobalSignals::getInstance()->outputPermanentInfoSig("请在视图中拾取对象创建集合");
        //几何点类型
        if (!type.contains("集"))
        {
            type = type + "集";
        }
        _labelType->setText(type);

        // 解析表面集特殊处理
        if (type == "解析表面集")
        {
            // 从属性组移除普通类型标签和拾取控件，改用子类型下拉框（解析表面集不做拾取）
            _labelType->setParent(nullptr);
            _labelOrder->setParent(nullptr);
            _checkboxOrder->setParent(nullptr);
            deactivatePickWidget();
            _pickWidget->widget()->setParent(nullptr);

            auto fold = m_foldWidgets[0];
            _comboLabelType->setParent(nullptr);
            fold->addRow(_hintLabel, _comboLabelType);

            // 隐藏其他类型专属控件
            _labelFrequencyPoint->setParent(nullptr);
            _frequencyPoint->setParent(nullptr);
            _addBtn->setParent(nullptr);
            _reduceBtn->setParent(nullptr);
            _clearBtn->setParent(nullptr);
            _tableFrequencyPoint->setParent(nullptr);
            _listAssignElement->setParent(nullptr);
            _allEleBtn->setParent(nullptr);
            _addEleBtn->setParent(nullptr);
            _reduceEleBtn->setParent(nullptr);
            _deleteEleBtn->setParent(nullptr);
            _labelDofNode->setParent(nullptr);
            _lineEditDofNode->setParent(nullptr);
            _btnHighLight->setParent(nullptr);
            _labelDof->setParent(nullptr);
            _comboDof->setParent(nullptr);

            updateAnalyticalSurfaceFolds();
            m_foldWidgets[0]->refreshLayout();
            m_foldWidgets[1]->refreshLayout();
            return;
        }

        //隐藏排序
        if (type != "节点集" && type != "单元集")
        {
            //_labelOrder->setVisible(false);
            //_checkboxOrder->setVisible(false);
            _labelOrder->setParent(nullptr);
            _checkboxOrder->setParent(nullptr);
        }

        ////隐藏拾取对象
        if (type != "频率点集")
        {
            _labelFrequencyPoint->setParent(nullptr);
            _frequencyPoint->setParent(nullptr);
            _addBtn->setParent(nullptr);
            _reduceBtn->setParent(nullptr);
            _clearBtn->setParent(nullptr);
            _tableFrequencyPoint->setParent(nullptr);

        }

        if (type != "面板集")
        {
            _listAssignElement->setParent(nullptr);
            _allEleBtn->setParent(nullptr);
            _addEleBtn->setParent(nullptr);
            _reduceEleBtn->setParent(nullptr);
            _deleteEleBtn->setParent(nullptr);
        }

        //频率点集/面板集不使用拾取控件：失活并从面板移除
        if (type == "频率点集" || type == "面板集")
        {
            deactivatePickWidget();
            _pickWidget->widget()->setParent(nullptr);
        }

        if (type != "自由度节点集")
        {
            _labelDofNode->setParent(nullptr);
            _lineEditDofNode->setParent(nullptr);
            _btnHighLight->setParent(nullptr);

            _labelDof->setParent(nullptr);
            _comboDof->setParent(nullptr);
        }

        if (type == "节点集" || type == "单元集")
            _checkboxOrder->setChecked(order);

        //统计折叠框：仅对承载功能控件（频率点表格/自由度节点/面板集列表）的类型显示
        m_foldWidgets[1]->setVisible(type == "频率点集" || type == "自由度节点集" || type == "面板集");

        // 非解析表面集：隐藏解析表面相关折叠面板，移除子类型下拉框
        _comboLabelType->setParent(nullptr);
        m_foldWidgets[2]->setVisible(false);
        m_foldWidgets[3]->setVisible(false);
        m_foldWidgets[4]->setVisible(false);
        m_foldWidgets[5]->setVisible(false);
        m_foldWidgets[6]->setVisible(false);
        m_foldWidgets[7]->setVisible(false);
        m_foldWidgets[8]->setVisible(false);
        m_foldWidgets[9]->setVisible(false);
        m_foldWidgets[10]->setVisible(false);
        m_foldWidgets[0]->refreshLayout();
        m_foldWidgets[1]->refreshLayout();

    }

    QString mSetPanel::getSetTypeStringByPickFilter(mxr::PickFilter pickFilter)
    {
        QString labelType = "几何点集";
        switch (pickFilter)
        {
        case PickFilter::PickMeshNode:
            labelType = "节点集";
            break;
        case PickFilter::PickMeshElement:
            labelType = "单元集";
            break;
        case PickFilter::PickMeshLine:
            labelType = "单元边集";
            break;
        case PickFilter::PickMeshFace:
            labelType = "单元表面集";
            break;

        case PickFilter::PickGeoPoint:
            labelType = "几何点集";
            break;
        case PickFilter::PickGeoLine:
            labelType = "几何线集";
            break;
        case PickFilter::PickGeoFace:
            labelType = "几何面集";
            break;
        case PickFilter::PickGeoSolid:
            labelType = "几何实体集";
            break;
        case PickFilter::PickMeshPart | PickFilter::PickGeoPart:
            labelType = "部件集";
            break;
        default:
            break;
        }
        return labelType;
    }

    QString mSetPanel::getSetTypeStringByPickDataType(MxFunctions::PickDataType pickFilter)
    {
        QString labelType = "几何点集";
        switch (pickFilter)
        {
        case PickDataType::Node:
            labelType = "节点集";
            break;
        case PickDataType::Element:
            labelType = "单元集";
            break;
        case PickDataType::ElementEdge:
            labelType = "单元边集";
            break;
        case PickDataType::ElementFace:
            labelType = "单元表面集";
            break;
        case PickDataType::GeoSolid:
            labelType = "几何实体集";
            break;
        case PickDataType::GeoFace:
            labelType = "几何面集";
            break;
        case PickDataType::GeoEdge:
            labelType = "几何线集";
            break;
        case PickDataType::GeoPoint:
            labelType = "几何点集";
            break;
        case PickDataType::Part:
            labelType = "部件集";
            break;
        default:
            break;
        }
        return labelType;
    }

    QString mSetPanel::getEnglishNameByChineseName(QString chineseName)
    {
        QString name = "";
        if (chineseName == "几何点集")
            name = "PointSet";
        else if (chineseName == "几何线集")
            name = "LineSet";
        else if (chineseName == "几何面集")
            name = "SurfaceSet";
        else if (chineseName == "几何实体集")
            name = "BodySet";
        else if (chineseName == "几何体集")   //转换集合按钮的类型串
            name = "BodySet";
        else if (chineseName == "节点集")
            name = "NodeSet";
        else if (chineseName == "单元边集")
            name = "ElemEdgeSet";
        else if (chineseName == "单元表面集")
            name = "ElemFaceSet";
        else if (chineseName == "单元面集")   //转换集合按钮的类型串
            name = "ElemFaceSet";
        else if (chineseName == "单元集")
            name = "ElementSet";
        else if (chineseName == "部件集")
            name = "PartSet";
        else if (chineseName == "频率点集")
            name = "FreqSet";
        else if (chineseName == "自由度节点集")
            name = "DOFSet";
        else if (chineseName == "面板集")
            name = "PanelSet";
        else if (chineseName == "节点面集")    // 新增节点面集
            name = "NodeFaceSet";
        else if (chineseName == "解析表面集")
            name = "SurfaceSet";
        return name;
    }

    void mSetPanel::okSet()
    {
        QString type = _labelType->text();
        bool order = false;
        QString name = "";
        QString EnName = getEnglishNameByChineseName(type);
        bool isCreate = false;
        if (_setData == nullptr)//新建
        {
            name = getCVM->getModelData()->createNameByType(Physical_Set, EnName);
        }
        if (_setData != nullptr && _setData->getSetTypeStr() != type)
        {
            if (_setData->getIsUserRename())//用户重命名过
            {
                name = _setData->getSetName();
            }
            else//默认名称，类型更改后自动修改名称
            {
                QStringList list = _setData->getSetName().split("_");
                if (list.size() == 2)
                {
                    name = QString("%1_%2").arg(EnName).arg(list.at(1));
                }
                else//旧默认名不符合"前缀_编号"格式，按ID生成英文名，避免使用中文类型名
                {
                    name = QString("%1_%2").arg(EnName).arg(_setData->getSetId());
                }
                if (getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).ExistSetName(name))
                {
                    name = getCVM->getModelData()->createNameByType(Physical_Set, EnName);
                }
            }
        }
        if (type == "节点集")
        {
            if (_setData == nullptr)//新建
            {
                isCreate = true;
                int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
                auto nodeSet = new mNode(setID, Node, name);
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(nodeSet);
                _setData = nodeSet;
            }
            else if (_setData != nullptr && _setData->getSetTypeStr() != type)//更新且类型不同
            {
                auto nodeSet = new mNode(_setData->getSetId(), Node, name);
                getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).deleteSet(_setData->getSetId());
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(nodeSet);
                _setData = nodeSet;
            }
            auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
            //nodeSet->setNodeData(res);
            auto nodeSet = toType<mNode>(_setData);

            if (_checkboxOrder->isChecked())
            {
                order = true;
                nodeSet->setOrder(true);
                std::sort(res.begin(), res.end(), [](const MeshSelectInfoData& a, const MeshSelectInfoData& b) {
                    return a.data_id.element_id < b.data_id.element_id; // 升序排序 
                    });
            }
            else
            {
                nodeSet->setOrder(false);
            }
            std::vector<RenderPickResultData> nodes;
            for (auto node : res)
            {
                RenderPickResultData n = SelectData2ResultData(SelectionInfoType::MeshNode, node);
                nodes.push_back(n);
            }
            nodeSet->setNodeData(nodes);
        }
        else if (type == "单元集")
        {
            if (_setData == nullptr)//新建
            {
                isCreate = true;
                int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
                auto eleset = new mElementSet(setID, Element, name);
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(eleset);
                _setData = eleset;
            }
            else if (_setData != nullptr && _setData->getSetTypeStr() != type)//更新且类型不同
            {
                auto eleset = new mElementSet(_setData->getSetId(), Element, name);
                getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).deleteSet(_setData->getSetId());
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(eleset);
                _setData = eleset;
            }
            auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
            //nodeSet->setNodeData(res);
            auto eleSet = toType<mElementSet>(_setData);

            if (_checkboxOrder->isChecked())
            {
                order = true;
                eleSet->setOrder(true);
                std::sort(res.begin(), res.end(), [](const MeshSelectInfoData& a, const MeshSelectInfoData& b) {
                    return a.data_id.element_id < b.data_id.element_id; // 升序排序 
                    });
            }
            else
            {
                eleSet->setOrder(false);
            }
            std::vector<RenderPickResultData> eles;
            for (auto ele : res)
            {
                RenderPickResultData n = SelectData2ResultData(SelectionInfoType::MeshElement, ele);
                eles.push_back(n);
            }
            eleSet->setElementSetData(eles);
        }
        else if (type == "单元表面集")
        {
            if (_setData == nullptr)//新建
            {
                isCreate = true;
                int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
                auto elesurfaceset = new mElementSurface(setID, ElementSurface, name);
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(elesurfaceset);
                _setData = elesurfaceset;
            }
            else if (_setData != nullptr && _setData->getSetTypeStr() != type)//更新且类型不同
            {
                auto eleset = new mElementSurface(_setData->getSetId(), ElementSurface, name);
                getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).deleteSet(_setData->getSetId());
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(eleset);
                _setData = eleset;
            }
            auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshFace);
            //nodeSet->setNodeData(res);
            auto elesurfaceSet = toType<mElementSurface>(_setData);

            std::vector<RenderPickResultData> elesurfaces;
            for (auto elesurface : res)
            {
                RenderPickResultData n = SelectData2ResultData(SelectionInfoType::MeshFace, elesurface);
                elesurfaces.push_back(n);
            }
            elesurfaceSet->setElementSurfaceData(elesurfaces);
        }
        else if (type == "单元边集")
        {
            if (_setData == nullptr)//新建
            {
                isCreate = true;
                int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
                auto elesurfaceset = new mElementSurface(setID, ElementEdge, name);
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(elesurfaceset);
                _setData = elesurfaceset;
            }
            else if (_setData != nullptr && _setData->getSetTypeStr() != type)//更新且类型不同
            {
                auto eleset = new mElementSurface(_setData->getSetId(), ElementEdge, name);
                getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).deleteSet(_setData->getSetId());
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(eleset);
                _setData = eleset;
            }
            auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshLine);
            //nodeSet->setNodeData(res);
            auto elesurfaceSet = toType<mElementSurface>(_setData);

            std::vector<RenderPickResultData> elesurfaces;
            for (auto elesurface : res)
            {
                RenderPickResultData n = SelectData2ResultData(SelectionInfoType::MeshLine, elesurface);
                elesurfaces.push_back(n);
            }
            elesurfaceSet->setElementSurfaceData(elesurfaces);
            }
        else if (type.contains("几何"))
        {
            auto setType = SetType::GeoPoint;
            if (type == "几何线集")
            {
                setType = SetType::GeoEdge;
            }
            else if (type == "几何面集")
            {
                setType = SetType::GeoFace;
            }
            else if (type == "几何实体集")
            {
                setType = SetType::GeoSolid;
            }
            if (_setData == nullptr)//新建
            {
                isCreate = true;
                int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
                auto eleset = new mGeoSet(setID, setType, name);
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(eleset);
                _setData = eleset;
            }
            else if (_setData != nullptr && _setData->getSetTypeStr() != type)//更新且类型不同
            {
                auto eleset = new mGeoSet(_setData->getSetId(), setType, name);
                getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).deleteSet(_setData->getSetId());
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(eleset);
                _setData = eleset;
            }
            //获取拾取数据
            auto geoType = SelectionInfoType::GeoPoint;
            if (type == "几何线集")
            {
                geoType = SelectionInfoType::GeoEdge;
            }
            else if (type == "几何面集")
            {
                geoType = SelectionInfoType::GeoFace;
            }
            else if (type == "几何实体集")
            {
                geoType = SelectionInfoType::GeoSolid;
            }
            auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(geoType);
            auto geoSet = toType<mGeoSet>(_setData);
            geoSet->setGeoSetData(res);
        }
        else if (type == "部件集")
        {
            if (_setData == nullptr)//新建
            {
                isCreate = true;
                int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
                auto eleset = new mPartSet(setID, Part, name);
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(eleset);
                _setData = eleset;
            }
            else if (_setData != nullptr && _setData->getSetTypeStr() != _labelType->text())//更新且类型不同
            {
                int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
                auto eleset = new mPartSet(_setData->getSetId(), Part, name);
                getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).deleteSet(_setData->getSetId());
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(eleset);
                _setData = eleset;
            }
            auto res = getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
            auto geoSet = toType<mPartSet>(_setData);
            geoSet->setPartSetData(res);
        }
        else if (type == "频率点集")
        {
            if (_setData == nullptr) //新建
            {
                isCreate = true;
                int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
                auto frequencyPointSet = new mFrequencyPoint(setID, FrequencyPoint, name);
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(frequencyPointSet);
                _setData = frequencyPointSet;

                auto nodeSet = toType<mFrequencyPoint>(_setData);
                nodeSet->clearOrders();
                _frequencyPoint->setText("");
            }
            else if (_setData != nullptr && _setData->getSetTypeStr() != type)//更新且类型不同
            {
                auto frequencyPointSet = new mFrequencyPoint(_setData->getSetId(), FrequencyPoint, name);
                getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set)
                    .deleteSet(_setData->getSetId());
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(frequencyPointSet);
                _setData = frequencyPointSet;
            }

            updateFrequencyTable(false);

        }
        else if (type == "自由度节点集")
        {
            if (_setData == nullptr)//新建
            {
                isCreate = true;
                int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
                auto dofNodeSet = new mDOFNode(setID, DOFNode, name);
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(dofNodeSet);
                _setData = dofNodeSet;
            }
            else if (_setData != nullptr && _setData->getSetTypeStr() != type)//更新且类型不同
            {
                auto dofNodeSet = new mDOFNode(_setData->getSetId(), DOFNode, name);
                getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set)
                    .deleteSet(_setData->getSetId());
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(dofNodeSet);
                _setData = dofNodeSet;
            }
            auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()
                ->getSelectDataByType(SelectionInfoType::MeshNode);
            auto dofNodeSet = toType<mDOFNode>(_setData);

            QString nodeidstr = ""; int count = 0;
            std::vector<RenderPickResultData> nodes;
            for (auto node : res)
            {
                RenderPickResultData n = SelectData2ResultData(SelectionInfoType::MeshNode, node);
                nodes.push_back(n);

                nodeidstr += MxFunctions::mNumberToString(n.result_id);

                ++count;
                if (count != res.size())
                    nodeidstr += ", ";
            }
            dofNodeSet->setNodeData(nodes);

            _lineEditDofNode->setText(nodeidstr);

            int dofIndex = dofNodeSet->dof();
            _comboDof->setCurrentIndex(dofIndex - 1);
        }
        else if (type == "面板集")
        {
            if (_setData == nullptr)//新建
            {
                isCreate = true;
                int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
                auto panelSet = new mPanel(setID, Panel, name);
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(panelSet);
                _setData = panelSet;
            }
            else if (_setData != nullptr && _setData->getSetTypeStr() != type) //更新且类型不同
            {
                auto panelSet = new mPanel(_setData->getSetId(), Panel, name);
                getCVM->getModelData()
                    ->getManager<MechSetData::mSetManager>(Physical_Set).deleteSet(_setData->getSetId());
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(panelSet);
                _setData = panelSet;
            }

            // 面板集在确认时根据界面列表同步 mPanel 的单元集与节点集数据
            auto panelSet = this->toType<mPanel>(_setData);
            if (panelSet)
            {
                QStringList eleNames;
                QStringList nodeNames;
                auto elementLists = getCVM->getModelData()
                    ->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(Element);
                for (int i = 0; i < _listAssignElement->count(); ++i)
                {
                    QString text = _listAssignElement->item(i)->text();
                    if (elementLists.contains(text))
                    {
                        eleNames.append(text);
                    }
                    else
                    {
                        nodeNames.append(text);
                    }
                }
                panelSet->setElenames(eleNames);

                // 仅在“单元/节点集合”模式下保存节点集数据，避免等效辐射功率模式残留节点数据
                if (panelSet->getPanelCollectionType() == 1)
                {
                    panelSet->setNodeNames(nodeNames);
                }
                else
                {
                    panelSet->setNodeNames(QStringList());
                }
            }
        }
        else if (type == "节点面集")
        {
            if (_setData == nullptr) // 新建
            {
                isCreate = true;
                int setID = getCVM->getModelData()
                    ->getManager<MechSetData::mSetManager>(Physical_Set).createID();
                // 核心：复用 mNode 类，仅枚举类型传 NodeFace
                auto nodeFaceSet = new mNodeFace(setID, NodeSurface, name);
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(nodeFaceSet);
                _setData = nodeFaceSet;
            }
            else if (_setData != nullptr && _setData->getSetTypeStr() != type) // 类型变更
            {
                auto nodeFaceSet = new mNodeFace(_setData->getSetId(), NodeSurface, name);
                getCVM->getModelData()
                    ->getManager<MechSetData::mSetManager>(Physical_Set).deleteSet(_setData->getSetId());
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(nodeFaceSet);
                _setData = nodeFaceSet;
            }

            // 获取当前拾取的节点数据
            auto res = getCVM->getPreRenderWidget()->getSelectionContainer()
                ->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
            auto nodeFaceSet = toType<mNodeFace>(_setData);

            // 节点面集无排序功能，直接存储（不调用 setOrder）
            std::vector<RenderPickResultData> nodes;
            for (auto node : res)
            {
                RenderPickResultData n = SelectData2ResultData(SelectionInfoType::MeshNode, node);
                nodes.push_back(n);
            }
            nodeFaceSet->setNodeData(nodes);
        }
        else if (type == "解析表面集")
        {
            auto subType = _comboLabelType->currentText();
            if (_setData == nullptr)//新建
            {
                isCreate = true;
                int setID = getCVM->getModelData()
                    ->getManager<MechSetData::mSetManager>(Physical_Set).createID();
                auto sa = new mSurfaceAnalysis(setID, MxFunctions::SetType::SurfaceAnalysis, name);
                sa->setSurfaceSubTypeByName(subType);
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(sa);
                _setData = sa;
            }
            else
            {
                // 子类型切换：直接改成员变量，无需删除重建
                auto sa = toType<mSurfaceAnalysis>(_setData);
                if (sa)
                    sa->setSurfaceSubTypeByName(subType);
            }
        }

        //editSetPanel(_setData->getSetId());
        updateSetPanel(type, order);
        if(type != "自由度节点集")
            clearSet();

        //创建成功发送消息
        if (isCreate)
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建集合 %1").arg(_setData->getSetName()), true);
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("成功更新集合 %1").arg(_setData->getSetName()), true);
        }
        if (_setData)
        {
            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateSetSig(isCreate, _setData->getSetId());
        }

    }

    void mSetPanel::clearSet()
    {
        if (!getCVM)
        {
            return;
        }
        if (!getCVM || !getCVM->getPreRenderWidget())
        {
            return;
        }
        getCVM->clearPick();
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();

        //_lineEditDofNode->setText("");
    }

    void mSetPanel::connectSig()
    {
        disconnectSig();
        if (!getCVM || !getCVM->getPreRenderWidget())
        {
            return;
        }

        //视口拾取由通用拾取控件自行连接处理，此处仅监听拾取模式变化
        conn_pickFiter = connect(MxFunctions::mGlobalSignals::getInstance(), &MxFunctions::mGlobalSignals::pickFilterChangeSig, this, &mSetPanel::slot_pickFilterChange);
    }

    void mSetPanel::disconnectSig()
    {
        if (conn_pickFiter)
            disconnect(conn_pickFiter);
    }



    void mSetPanel::initFrequencyPointTabel()
    {
        _widgetFrequencyPoint = new QWidget;
        QHBoxLayout* layout = new QHBoxLayout;
        _addBtn = new QPushButton(); _addBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png")); _addBtn->setToolTip("增加");
        connect(_addBtn, &QPushButton::clicked, this, &mSetPanel::slot_addLine);
        _reduceBtn = new QPushButton(); _reduceBtn->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png")); _reduceBtn->setToolTip("减少");
        connect(_reduceBtn, &QPushButton::clicked, this, &mSetPanel::slot_deleteLine);
        _clearBtn = new QPushButton(); _clearBtn->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png")); _clearBtn->setToolTip("清空");
        connect(_clearBtn, &QPushButton::clicked, this, &mSetPanel::slot_clearTabel);
        layout->addWidget(_addBtn);
        layout->addWidget(_reduceBtn);
        layout->addWidget(_clearBtn);
        layout->addStretch();
        layout->setContentsMargins(0, 0, 0, 0);
        _widgetFrequencyPoint->setLayout(layout);

        _tableFrequencyPoint = new QTableWidget;
        _tableFrequencyPoint->setColumnCount(2);
        _tableFrequencyPoint->verticalHeader()->setHidden(true);
        _tableFrequencyPoint->resizeColumnsToContents();
        QStringList headerText;
        headerText << QString("序号") << QString("频率");
        _tableFrequencyPoint->setHorizontalHeaderLabels(headerText);
        _tableFrequencyPoint->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        _tableFrequencyPoint->horizontalHeader()->setEditTriggers(QAbstractItemView::NoEditTriggers);
        _tableFrequencyPoint->setSelectionMode(QAbstractItemView::ExtendedSelection);
        _tableFrequencyPoint->setSelectionBehavior(QAbstractItemView::SelectItems);
        _tableFrequencyPoint->setEditTriggers(QAbstractItemView::CurrentChanged | QAbstractItemView::SelectedClicked
            | QAbstractItemView::EditKeyPressed);
        _tableFrequencyPoint->setMinimumHeight(270);

        //设置交替换行
        _tableFrequencyPoint->setAlternatingRowColors(true);

        m_tableChangeConn = connect(_tableFrequencyPoint, &QTableWidget::itemChanged,
            this, &mSetPanel::slot_lineChanged);

        // 美化表格样式 - 浅蓝主题
        this->setStyleSheet(R"(
        QHeaderView::section {
            background-color: #87cefa;  /* 浅蓝色表头 */
            color: #2c3e50;
            font-weight: bold;
            padding: 6px;
            border: 1px solid #b0d4f1;
        }
        QTableWidget {
            gridline-color: #d0e6f7;
            alternate-background-color:#ECF5FF;
            selection-background-color: #add8e6; /* 选中浅蓝 */
            selection-color: black;
        }
        QTableWidget::item {
            background: transparent;
        }
        QTableWidget:disabled {
            background: #F2F2F2;
        }
        QTableWidget::item:disabled {
            color: #9A9A9A;
        }
        QHeaderView::section:horizontal
        {
            background: #F7F8FA;
            border: 1px solid #dcdcdc;
            border-left: 0;

	        padding:4px;
	        font-weight:normal;
        }
        QHeaderView::section:vertical
        {
            background: #F7F8FA;
            border: 1px solid #dcdcdc;
            border-top: 0;

	        padding:4px;
	        font-weight:normal;
        }
        QPushButton:disabled {
            background-color: #E6E6E6;
            color: #9A9A9A;
            border: 1px solid #C8C8C8;
        }
        QLineEdit:disabled, QComboBox:disabled {
            background-color: #F3F3F3;
            color: #888;
        }
            )");

    }

    void mSetPanel::updateFrequencyTable(bool deleteMap)
    {
        auto frequencyPointSet = this->toType<mFrequencyPoint>(_setData);
        if (deleteMap)
        {
            frequencyPointSet->clearOrders();
        }

        disconnectFrequencyPointTableSig();
        int iOrderSize = frequencyPointSet->orderCount();
        _tableFrequencyPoint->setRowCount(iOrderSize);
        for (int i = 0; i < iOrderSize; i++)
        {
            QTableWidgetItem* order = new QTableWidgetItem(MxFunctions::mNumberToString(i + 1));
            order->setFlags(order->flags() & ~Qt::ItemIsEditable);
            order->setTextAlignment(Qt::AlignCenter);
            QTableWidgetItem* weight = new QTableWidgetItem(MxFunctions::mNumberToString(frequencyPointSet->orderSecondValueAt(i)));
            weight->setTextAlignment(Qt::AlignCenter);

            _tableFrequencyPoint->setItem(i, 0, order);
            _tableFrequencyPoint->setItem(i, 1, weight);
        }

        //拼接同步显示到频率点输入框
        QString str = "";
        for (int i = 0; i < iOrderSize; i++)
        {
            str += MxFunctions::mNumberToString(frequencyPointSet->orderSecondValueAt(i));
            if (i != iOrderSize - 1)
            {
                str += ", ";
            }
        }
        _frequencyPoint->setText(str);
        connectFrequencyPointTableSig();
    }

    void mSetPanel::initSegContourTable()
    {
        _widgetContourBtn = new QWidget;
        QHBoxLayout* layout = new QHBoxLayout;

        _contourAddBtn = new QPushButton();
        _contourAddBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        _contourAddBtn->setToolTip("增加");
        connect(_contourAddBtn, &QPushButton::clicked, this, &mSetPanel::slot_contourAddLine);

        _contourReduceBtn = new QPushButton();
        _contourReduceBtn->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        _contourReduceBtn->setToolTip("减少");
        connect(_contourReduceBtn, &QPushButton::clicked, this, &mSetPanel::slot_contourDeleteLine);

        _contourClearBtn = new QPushButton();
        _contourClearBtn->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png"));
        _contourClearBtn->setToolTip("清空");
        connect(_contourClearBtn, &QPushButton::clicked, this, &mSetPanel::slot_contourClearTable);

        layout->addWidget(_contourAddBtn);
        layout->addWidget(_contourReduceBtn);
        layout->addWidget(_contourClearBtn);
        layout->addStretch();
        layout->setContentsMargins(0, 0, 0, 0);
        _widgetContourBtn->setLayout(layout);

        _tableSegContour = new QTableWidget;
        _tableSegContour->setColumnCount(5);
        _tableSegContour->verticalHeader()->setHidden(true);
        _tableSegContour->resizeColumnsToContents();

        QStringList headerText;
        headerText << QString("类型") << QString("x") << QString("y") << QString("x") << QString("y");
        _tableSegContour->setHorizontalHeaderLabels(headerText);
        _tableSegContour->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        _tableSegContour->horizontalHeader()->setEditTriggers(QAbstractItemView::NoEditTriggers);

        _tableSegContour->setSelectionMode(QAbstractItemView::ExtendedSelection);
        _tableSegContour->setSelectionBehavior(QAbstractItemView::SelectItems);
        _tableSegContour->setEditTriggers(QAbstractItemView::CurrentChanged | QAbstractItemView::SelectedClicked
            | QAbstractItemView::EditKeyPressed);

        _tableSegContour->setMinimumHeight(270);
        _tableSegContour->setAlternatingRowColors(true);

        m_contourTableChangeConn = connect(_tableSegContour, &QTableWidget::itemChanged,
            this, &mSetPanel::slot_contourLineChanged);

        // 样式复用浅蓝主题
        _tableSegContour->setStyleSheet(R"(
                QHeaderView::section {
                    background-color: #87cefa;
                    color: #2c3e50;
                    font-weight: bold;
                    padding: 6px;
                    border: 1px solid #b0d4f1;
                }
                QTableWidget {
                    gridline-color: #d0e6f7;
                    alternate-background-color:#ECF5FF;
                    selection-background-color: #add8e6;
                    selection-color: black;
                }
                QTableWidget::item {
                    background: transparent;
                }
                QTableWidget:disabled {
                    background: #F2F2F2;
                }
                QTableWidget::item:disabled {
                    color: #9A9A9A;
                }
                QHeaderView::section:horizontal
                {
                    background: #F7F8FA;
                    border: 1px solid #dcdcdc;
                    border-left: 0;
                    padding:4px;
                    font-weight:normal;
                }
                QHeaderView::section:vertical
                {
                    background: #F7F8FA;
                    border: 1px solid #dcdcdc;
                    border-top: 0;
                    padding:4px;
                    font-weight:normal;
                }
                QPushButton:disabled {
                    background-color: #E6E6E6;
                    color: #9A9A9A;
                    border: 1px solid #C8C8C8;
                }
                QLineEdit:disabled, QComboBox:disabled {
                    background-color: #F3F3F3;
                    color: #888;
                }
                )");

        //初始化3行默认数据 line、circl、circl
        _tableSegContour->setRowCount(0);
    }

    MxFunctions::SetType mSetPanel::getSetTypeByChineseName(const QString& chineseName)
    {
        if (chineseName == "几何点集") return MxFunctions::GeoPoint;
        if (chineseName == "几何线集") return MxFunctions::GeoEdge;
        if (chineseName == "几何面集") return MxFunctions::GeoFace;
        if (chineseName == "几何实体集") return MxFunctions::GeoSolid;
        if (chineseName == "节点集") return MxFunctions::Node;
        if (chineseName == "单元集") return MxFunctions::Element;
        if (chineseName == "单元边集") return MxFunctions::ElementEdge;
        if (chineseName == "单元表面集") return MxFunctions::ElementSurface;
        if (chineseName == "部件集") return MxFunctions::Part;
        if (chineseName == "自由度节点集") return MxFunctions::DOFNode;
        if (chineseName == "节点面集") return MxFunctions::NodeSurface;
        return MxFunctions::Mix;
    }

    MechSetData::mSetBase* mSetPanel::ensureSetData()
    {
        if (_setData != nullptr)
        {
            return _setData;
        }
        if (!getCVM)
        {
            return nullptr;
        }

        QString type = _labelType->text();
        QString EnName = getEnglishNameByChineseName(type);
        QString name = getCVM->getModelData()->createNameByType(Physical_Set, EnName);
        int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();

        MechSetData::mSetBase* newData = nullptr;
        if (type == "节点集")
        {
            newData = new mNode(setID, Node, name);
        }
        else if (type == "单元集")
        {
            newData = new mElementSet(setID, Element, name);
        }
        else if (type == "单元表面集")
        {
            newData = new mElementSurface(setID, ElementSurface, name);
        }
        else if (type == "单元边集")
        {
            newData = new mElementSurface(setID, ElementEdge, name);
        }
        else if (type.contains("几何"))
        {
            newData = new mGeoSet(setID, getSetTypeByChineseName(type), name);
        }
        else if (type == "部件集")
        {
            newData = new mPartSet(setID, Part, name);
        }
        else if (type == "自由度节点集")
        {
            newData = new mDOFNode(setID, DOFNode, name);
        }
        else if (type == "节点面集")
        {
            newData = new mNodeFace(setID, NodeSurface, name);
        }

        if (newData == nullptr)
        {
            return nullptr;
        }

        getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(newData);
        _setData = newData;

        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建集合 %1").arg(_setData->getSetName()), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateSetSig(true, _setData->getSetId());
        return _setData;
    }

    void mSetPanel::setupPickWidget()
    {
        if (!_pickWidget)
        {
            return;
        }

        //集合类型：优先取集合对象类型，拾取型集合未创建时按面板类型文本推断
        MxFunctions::SetType setType = _setData ? _setData->getSetType()
            : getSetTypeByChineseName(_labelType->text());
        MxFunctions::PickDataType pickType = MxFunctions::PickDataType::None;
        switch (setType)
        {
        case MxFunctions::DOFNode:
        case MxFunctions::NodeSurface:
            pickType = MxFunctions::PickDataType::Node;
            break;
        default:
            pickType = SetTypeToPickDataType(setType);
            break;
        }
        _pickWidget->setAllowedTypes([pickType]() -> QSet<MxFunctions::PickDataType> {
            return { pickType };
            });

        //存储策略：commit 时按集合类型将拾取数据写入集合对象（按ID现取，防指针失效）
        if (_setData)
        {
            int setId = _setData->getSetId();
            auto strategy = std::make_shared<SetDataPersistenceStrategy>([setId]() -> MechSetData::mSetBase* {
                if (!getCVM)
                {
                    return nullptr;
                }
                return getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setId);
                });
            _pickWidget->setPersistenceStrategy(strategy);
            _setStrategy = strategy;
        }
        _pickWidget->setPickType();
    }

    void mSetPanel::activatePickWidget()
    {
        if (!_pickWidget)
        {
            return;
        }

        //频率点集/面板集/解析表面集不使用拾取控件
        QString type = _labelType->text();
        if (type == "频率点集" || type == "面板集" || type == "解析表面集")
        {
            deactivatePickWidget();
            return;
        }

        setupPickWidget();
        //反显已有集合数据并激活（激活即按存储数据高亮，无临时高亮）
        _pickWidget->displayData(toPickWidgetData());
        _pickWidget->setPickActive(true);
    }

    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mSetPanel::captureViewportBySetType()
    {
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> data;
        if (!getCVM || !getCVM->getPreRenderWidget() || !_setData)
        {
            return data;
        }

        //集合类型 -> 拾取类型（与 setupPickWidget 一致）
        MxFunctions::SetType setType = _setData->getSetType();
        MxFunctions::PickDataType pickType = MxFunctions::PickDataType::None;
        switch (setType)
        {
        case MxFunctions::DOFNode:
        case MxFunctions::NodeSurface:
            pickType = MxFunctions::PickDataType::Node;
            break;
        default:
            pickType = SetTypeToPickDataType(setType);
            break;
        }
        if (pickType == MxFunctions::PickDataType::None)
        {
            return data;
        }

        if (pickType == MxFunctions::PickDataType::Part)
        {
            //部件拾取数据不在 SaveSelectMemoryData 读取范围内，单独读取
            auto partIds = getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
            if (!partIds.empty())
            {
                std::vector<RenderPickResultData> partVec;
                partVec.reserve(partIds.size());
                for (int partId : partIds)
                {
                    RenderPickResultData pickdata;
                    pickdata.result_id = partId;
                    pickdata.part_id = partId;
                    partVec.push_back(pickdata);
                }
                data[pickType] = std::move(partVec);
            }
            return data;
        }

        //按类型过滤读取，避免拾取其他类型的残留数据写入集合
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> all;
        SaveSelectMemoryData(getCVM->getPreRenderWidget(), all);
        auto it = all.find(pickType);
        if (it != all.end() && !it->second.empty())
        {
            data[pickType] = it->second;
        }
        return data;
    }

    void mSetPanel::deactivatePickWidget()
    {
        if (!_pickWidget)
        {
            return;
        }

        //集合可能已被删除（树上删除先释放对象、后经选中变化通知面板）：先校验防悬空
        validateSetData();

        //确定性兜底持久化：不依赖控件激活态/dirty标志，
        //优先取控件模型数据；模型无有效数据时从视口容器直接捕获
        //（与旧版"确定"的读取路径一致，容器是拾取数据的最终真源）
        auto pendingData = _pickWidget->getAllData().pickedData;
        bool hasPickedData = false;
        bool fromViewport = false;
        for (const auto& pair : pendingData)
        {
            if (!pair.second.empty())
            {
                hasPickedData = true;
                break;
            }
        }
        if (!hasPickedData)
        {
            pendingData = captureViewportBySetType();
            for (const auto& pair : pendingData)
            {
                if (!pair.second.empty())
                {
                    hasPickedData = true;
                    fromViewport = true;
                    break;
                }
            }
        }
        if (hasPickedData && _setData && _setStrategy)
        {
            _setStrategy->save(pendingData);
            if (fromViewport)
            {
                //捕获数据同步回控件模型：防止失活 commit 以空模型再次覆盖刚保存的集合数据
                PickWidgetData syncData;
                syncData.pickedData = pendingData;
                _pickWidget->displayData(syncData);
            }
        }

        //失活提交：保证已拾取数据写入集合对象；不保留高亮
        const bool renderValid = (getCVM && getCVM->getPreRenderWidget());
        _pickWidget->setKeepHighlightOnDeactivate(false);
        const bool wasActive = _pickWidget->isPickActive();
        if (wasActive)
        {
            _pickWidget->setPickActive(false);
        }
        //兜底：清理失活残留高亮
        if (renderValid && !wasActive)
        {
            _pickWidget->clearAllHighlight();
        }
        _pickWidget->setKeepHighlightOnDeactivate(true);

        //切出拾取：恢复拾取工具栏全部按钮
        emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>(10, true));
    }

    bool mSetPanel::validateSetData()
    {
        if (_setData == nullptr)
        {
            return false;
        }
        if (getCVM && !getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).containsSet(_setData))
        {
            //集合对象已被删除：释放引用，后续流程按无编辑对象处理
            _setData = nullptr;
            return false;
        }
        return true;
    }

    PickWidgetData mSetPanel::toPickWidgetData()
    {
        PickWidgetData data;
        if (!_setData)
        {
            return data;
        }

        //几何/部件集合以 ID 列表存储，转为拾取数据
        auto toResultList = [](const std::vector<int>& ids) {
            std::vector<RenderPickResultData> res;
            res.reserve(ids.size());
            for (int id : ids)
            {
                res.emplace_back(id, -1, -1);
            }
            return res;
        };

        switch (_setData->getSetType())
        {
        case MxFunctions::Node:
            if (auto* s = this->toType<mNode>(_setData)) data.pickedData[MxFunctions::PickDataType::Node] = s->getNodeData();
            break;
        case MxFunctions::DOFNode:
            if (auto* s = this->toType<mDOFNode>(_setData)) data.pickedData[MxFunctions::PickDataType::Node] = s->getNodeData();
            break;
        case MxFunctions::NodeSurface:
            if (auto* s = this->toType<mNodeFace>(_setData)) data.pickedData[MxFunctions::PickDataType::Node] = s->getNodeData();
            break;
        case MxFunctions::Element:
            if (auto* s = this->toType<mElementSet>(_setData)) data.pickedData[MxFunctions::PickDataType::Element] = s->getElementSetData();
            break;
        case MxFunctions::ElementSurface:
            if (auto* s = this->toType<mElementSurface>(_setData)) data.pickedData[MxFunctions::PickDataType::ElementFace] = s->getElementSurfaceData();
            break;
        case MxFunctions::ElementEdge:
            if (auto* s = this->toType<mElementSurface>(_setData)) data.pickedData[MxFunctions::PickDataType::ElementEdge] = s->getElementSurfaceData();
            break;
        case MxFunctions::GeoPoint:
        case MxFunctions::GeoEdge:
        case MxFunctions::GeoFace:
        case MxFunctions::GeoSolid:
            if (auto* s = this->toType<mGeoSet>(_setData))
                data.pickedData[SetTypeToPickDataType(_setData->getSetType())] = toResultList(s->getGeoSetData());
            break;
        case MxFunctions::Part:
            if (auto* s = this->toType<mPartSet>(_setData))
                data.pickedData[MxFunctions::PickDataType::Part] = toResultList(s->getPartSetData());
            break;
        default:
            break;
        }
        return data;
    }

    void mSetPanel::slot_pickDataChanged()
    {
        if (!_pickWidget)
        {
            return;
        }

        //集合可能已被删除（树上删除先释放对象）：校验防悬空，被删则走下方延迟创建逻辑
        validateSetData();

        //拾取型集合首次拾取时才创建集合对象（延迟创建，避免空集合导致完整性检查失败）
        if (_setData == nullptr)
        {
            ensureSetData();
            if (_setData == nullptr)
            {
                return;
            }
            //新对象已生成：绑定新集合的存储策略
            setupPickWidget();
        }

        //数据变化即提交：将拾取数据写入集合对象
        if (_pickWidget->isPickActive())
        {
            //激活态：经控件统一提交（含去重合并）
            mPickWidgetController::commitActivePickWidget();
        }
        else if (_setStrategy)
        {
            //未激活态的UI操作（清空/删除单条）也立即持久化，
            //避免挂起脏数据在失活 commit 时以空数据覆盖集合
            _setStrategy->save(_pickWidget->getAllData().pickedData);
        }

        const auto& allData = _pickWidget->getAllData();
        QString type = _labelType->text();

        //节点集/单元集：同步排序选项并按ID升序
        if (type == "节点集")
        {
            auto nodeSet = this->toType<mNode>(_setData);
            if (nodeSet)
            {
                nodeSet->setOrder(_checkboxOrder->isChecked());
                if (nodeSet->getOrder())
                {
                    auto nodes = nodeSet->getNodeData();
                    std::sort(nodes.begin(), nodes.end(), [](const RenderPickResultData& a, const RenderPickResultData& b) {
                        return a.result_id < b.result_id; // 升序排序
                        });
                    nodeSet->setNodeData(nodes);
                }
            }
        }
        else if (type == "单元集")
        {
            auto eleSet = this->toType<mElementSet>(_setData);
            if (eleSet)
            {
                eleSet->setOrder(_checkboxOrder->isChecked());
                if (eleSet->getOrder())
                {
                    auto eles = eleSet->getElementSetData();
                    std::sort(eles.begin(), eles.end(), [](const RenderPickResultData& a, const RenderPickResultData& b) {
                        return a.result_id < b.result_id; // 升序排序
                        });
                    eleSet->setElementSetData(eles);
                }
            }
        }
        else if (type == "自由度节点集")
        {
            //同步节点ID显示
            QString nodeidstr = "";
            auto it = allData.pickedData.find(MxFunctions::PickDataType::Node);
            if (it != allData.pickedData.end())
            {
                int idx = 0;
                for (const auto& node : it->second)
                {
                    nodeidstr += MxFunctions::mNumberToString(node.result_id);
                    ++idx;
                    if (idx != static_cast<int>(it->second.size()))
                        nodeidstr += ", ";
                }
            }
            _lineEditDofNode->setText(nodeidstr);
        }

        if (getCVM)
        {
            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Set, _setData->getSetId());
        }
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateSetSig(false, _setData->getSetId());
    }

    void mSetPanel::slot_pickActiveChanged(bool active)
    {
        if (active || !validateSetData())
        {
            return;
        }

        //失活时数据已 commit 到集合对象：刷新树与状态
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功更新集合 %1").arg(_setData->getSetName()), true);
        if (getCVM)
        {
            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Set, _setData->getSetId());
        }
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateSetSig(false, _setData->getSetId());
    }

    void mSetPanel::slot_pickFilterChange()
    {
        clearSet();
    }

    void mSetPanel::slot_lineEditChanged()
    {
        _tableFrequencyPoint->setRowCount(0);
        int x; double y;
        QString frequencyPointStr = _frequencyPoint->text();
        QStringList strList{};
        if (!frequencyPointStr.isEmpty())
            strList = frequencyPointStr.split(",");
        auto freqPointSet = this->toType<mFrequencyPoint>(_setData);
        freqPointSet->clearOrders();
        for (int i = 0; i < strList.size(); i++)
        {
            x = i + 1;
            QString str = strList.at(i);
            y = str.toDouble();
            freqPointSet->addOrder(x, y);
        }
        updateFrequencyTable(false);

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateSetSig(false, _setData->getSetId());
    }

    void mSetPanel::slot_lineChanged(QTableWidgetItem* item)
    {
        auto set = this->toType<mFrequencyPoint>(_setData);
        if (item->column() == 0)
        {
            //set->_order[item->row()].first = item->text().toDouble();
        }
        else if (item->column() == 1)
        {
            set->setOrderSecondValueAt(item->row(), item->text().toDouble());
        }

        //拼接同步显示到频率点输入框
        QString str = "";
        int iOrderSize = set->orderCount();
        for (int i = 0; i < iOrderSize; i++)
        {
            str += MxFunctions::mNumberToString(set->orderSecondValueAt(i));
            if (i != iOrderSize - 1)
            {
                str += ", ";
            }
        }
        _frequencyPoint->setText(str);

    }


    void mSetPanel::slot_addLine()
    {
        disconnectFrequencyPointTableSig();
        int row = _tableFrequencyPoint->currentRow() < 0 ? _tableFrequencyPoint->rowCount() : _tableFrequencyPoint->currentRow() + 1;
        _tableFrequencyPoint->insertRow(row);
        QTableWidgetItem* order = new QTableWidgetItem();
        order->setFlags(order->flags() & ~Qt::ItemIsEditable);
        order->setTextAlignment(Qt::AlignCenter);
        QTableWidgetItem* weight = new QTableWidgetItem();
        weight->setTextAlignment(Qt::AlignCenter);

        _tableFrequencyPoint->setItem(row, 0, order);
        _tableFrequencyPoint->setItem(row, 1, weight);

        auto set = this->toType<mFrequencyPoint>(_setData);

        set->insertOrder(row, row + 1, 0);

        updateFrequencyTable(false);
        connectFrequencyPointTableSig();

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateSetSig(false, _setData->getSetId());
    }

    void mSetPanel::slot_deleteLine()
    {
        disconnectFrequencyPointTableSig();
        QList<int> rows;
        for (const auto& idx : _tableFrequencyPoint->selectionModel()->selectedIndexes()) rows << idx.row();
        rows = rows.toSet().values();
        std::sort(rows.begin(), rows.end(), std::greater<int>());

        if (rows.isEmpty())
            rows.append(_tableFrequencyPoint->rowCount() - 1);

        auto set = this->toType<mFrequencyPoint>(_setData);
        for (auto row : rows)
        {
            if (row >= 0 && row < _tableFrequencyPoint->rowCount())
            {
                set->removeOrderAt(row);
                _tableFrequencyPoint->removeRow(row);
            }
        }

        updateFrequencyTable(false);

        connectFrequencyPointTableSig();

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateSetSig(false, _setData->getSetId());
    }

    void mSetPanel::slot_clearTabel()
    {
        disconnectFrequencyPointTableSig();
        _tableFrequencyPoint->setRowCount(0);
        auto frequencyPointSet = this->toType<mFrequencyPoint>(_setData);
        frequencyPointSet->clearOrders();
        _frequencyPoint->setText("");
        connectFrequencyPointTableSig();

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateSetSig(false, _setData->getSetId());
    }

    void mSetPanel::connectFrequencyPointTableSig()
    {
        disconnectFrequencyPointTableSig();
        m_tableChangeConn = connect(_tableFrequencyPoint, &QTableWidget::itemChanged,
            this, &mSetPanel::slot_lineChanged);
        //m_lineEditChangeConn = connect(_frequencyPoint, &QLineEdit::returnPressed,
         //   this, &mSetPanel::slot_lineEditChanged);
    }

    void mSetPanel::disconnectFrequencyPointTableSig()
    {
        if (m_tableChangeConn)
        {
            disconnect(m_tableChangeConn);
        }
        // if (m_lineEditChangeConn)
        /* {
             //disconnect(m_lineEditChangeConn);
         }*/
    }

    void mSetPanel::slot_highLightBtnClicked()
    {
        QString text = _lineEditDofNode->text();

        QSet<int> idSet; //去重
        QStringList parts = text.split(",", QString::SkipEmptyParts);
        for (const QString& part : parts)
        {
            QString trimmed = part.trimmed();

            // 匹配范围 (start-end 或 start-end:step)
            QRegularExpression re(R"((\d+)\s*-\s*(\d+)(?::(\d+))?)");
            QRegularExpressionMatch match = re.match(trimmed);

            if (match.hasMatch()) {
                int start = match.captured(1).toInt();
                int end = match.captured(2).toInt();
                int step = match.captured(3).isEmpty() ? 1 : match.captured(3).toInt();

                if (step <= 0) step = 1;
                if (start <= end) {
                    for (int i = start; i <= end; i += step) {
                        idSet.insert(i);
                    }
                }
                else //支持反向写法 20-15
                {
                    for (int i = start; i >= end; i -= step)
                    {
                        idSet.insert(i);
                    }
                }
            }
            else if (!trimmed.isEmpty()) {
                //单个数字
                bool ok = false;
                int num = trimmed.toInt(&ok);
                if (ok)
                {
                    idSet.insert(num);
                }
            }
        }

        //转成有序 QVector
        std::vector<int> ids;
        ids.clear();
        ids.reserve(idSet.size());
        for (int v : idSet)
        {
            ids.push_back(v);
        }

        setHighlight(SetType::DOFNode, ids, true);

        //同步拾取节点（自由度节点集可能尚未创建集合对象，手动输入也触发创建）
        if (_setData == nullptr)
        {
            ensureSetData();
            if (_setData)
            {
                setupPickWidget();
            }
        }
        auto set = this->toType<mDOFNode>(_setData);
        if (!set)
        {
            return;
        }
        std::vector<RenderPickResultData> nodes;
        int lastMaxNodeID = 0;
        auto modeldata = getCVM->getModelData();
        auto meshData = modeldata->getMeshData();
        //lastMaxNodeID = meshData->getMaxNodeId();

       // QHash<int, int> elementIDToPartID;
        int partID = -1;
        int phyID;
        for (auto id : ids)
        {
            //phyID = id + lastMaxNodeID;
           // if (elementIDToPartID.contains(phyID))
            //    partID = elementIDToPartID[phyID];
            phyID = id;
            RenderPickResultData data;
            data.result_id = phyID;
            data.part_id = partID;
            data.result_id2 = -1;
            nodes.push_back(data);
        }
        set->setNodeData(nodes);

        //同步拾取控件显示（不改变激活态与高亮）
        if (_pickWidget)
        {
            PickWidgetData pickData;
            pickData.pickedData[MxFunctions::PickDataType::Node] = nodes;
            _pickWidget->displayData(pickData);
        }
    }

    void mSetPanel::slot_comboDofChanged(const QString& text)
    {
        if (_setData == nullptr)
        {
            ensureSetData();
            if (_setData == nullptr)
            {
                return;
            }
        }
        auto dofNodeSet = toType<mDOFNode>(_setData);
        if (!dofNodeSet)
        {
            return;
        }
        dofNodeSet->setDof(text.toInt());
    }

    void mSetPanel::setHighlight(const MxFunctions::SetType& settype, const std::vector<int>& ids, bool on)
    {
        if (ids.size() == 0)
        {
            return;
        }
        if (getCVM->getPreRenderWidget() == nullptr)
            return;
        if (!getCVM)
        {
            return;
        }
        auto meshData = getCVM->getModelData()->getMeshData();
        MechCommon::mMechGlobalSignals::getInstance()->activePickSig(5);
        auto nodeIds = ids;
        std::vector<MeshSelectInfoData> nodes;
        for (auto node : nodeIds)
        {
            if (!meshData->NodeIsHaveByID(node))
            {
                continue;
            }
            MeshSelectInfoData n;
            n.data_id.element_id = node;
            //n.part_id = -1;
            nodes.push_back(n);
        }

        clearHighlight();
        getCVM->getPreRenderWidget()->getSelectionContainer()
            ->getMeshSelectData()
            ->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshNode, nodes);


        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();

    }

    void mSetPanel::clearHighlight()
    {
        if (!getCVM)
        {
            return;
        }
        ClearPreWidgetGeoPick(getCVM->getPreRenderWidget());
        ClearPreWidgetMeshPick(getCVM->getPreRenderWidget());
        getCVM->updateRender();
    }

    void mSetPanel::slot_elementItemClicked(QListWidgetItem* item)
    {
        QString text = item->text().trimmed();
        int elementId = getCVM->getModelData()
            ->getManager<MechSetData::mSetManager>(Physical_Set).getIDBySetName(text);

        //这里要区分节点集和单元集
        highlightElement(elementId);
    }

    void mSetPanel::highlightElement(int elementId)
    {
        MechSetData::mSetBase* setData = getCVM->getModelData()
            ->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(elementId);

        if (_setData == nullptr)
        {
            return;
        }
        //判断是节点还是单元
        auto elementLists = getCVM->getModelData()
            ->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(Element);
        auto nodeSetLists = getCVM->getModelData()
            ->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(Node);

        QString str = setData->getSetName();
        if (nodeSetLists.contains(str)) //是节点集
        {
            auto nodeSet = this->toType<mNode>(setData);
            if (nodeSet == nullptr) return;
            auto nodeData = nodeSet->getNodeData();
            std::vector<MeshSelectInfoData> nodes;
            for (auto node : nodeData)
            {
                MeshSelectInfoData n = PickData2SelectData(SelectionInfoType::MeshNode, node);
                nodes.push_back(n);
            }
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(5);
            getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshNode, {});
            getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshNode, nodes);

        }
        else
        {
            auto eleSet = this->toType<mElementSet>(setData);
            if (eleSet == nullptr) return;
            auto eleData = eleSet->getElementSetData();
            std::vector<MeshSelectInfoData> eles;
            for (auto ele : eleData)
            {
                MeshSelectInfoData n = PickData2SelectData(SelectionInfoType::MeshElement, ele);
                eles.push_back(n);
            }
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8);
            getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()
                ->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshElement, {});
            getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()
                ->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshElement, eles);
        }

        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mSetPanel::appendElementPanelMap(int elementId, int row)
    {
        int panelId = _setData->getSetId();
        getCVM->getModelData()->getMapRelationManager().appendTableMapRelation(
            { Physical_Set, panelId }, { Physical_Set, elementId }, row);
    }

    void mSetPanel::slot_allElementSetBtn()
    {
        if (getCVM == nullptr)
        {
            return;
        }

        auto set = this->toType<mPanel>(_setData);
        if (!set)
        {
            return;
        }

        //获取所有单元集名称
        auto elementLists = getCVM->getModelData()
            ->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(Element);

        _listAssignElement->clear();
        _listAssignElement->addItems(elementLists);

        //新增可选节点集合
        QStringList nodeSetLists;
        if (enableCollectNode)
        {
            nodeSetLists = getCVM->getModelData()
                ->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(Node);
            _listAssignElement->addItems(nodeSetLists);
        }

        // 仅保存当前选项对应的数据
        if (enableCollectNode)
        {
            QStringList allNames = nodeSetLists;
            allNames.append(elementLists);
            set->setElenames(elementLists);
            set->setNodeNames(allNames);
        }
        else
        {
            set->setElenames(elementLists);
            set->setNodeNames(QStringList());
        }

        //更新映射
        int panelId = _setData->getSetId();
        mMapRelationManager& mapManager = getCVM->getModelData()->getMapRelationManager();
        QList<int> iElementIds = mapManager.getMapRelation<QList<int>>({ Physical_Set, panelId }, Physical_Set);
        for (int row = 0; row < iElementIds.size(); row++)
        {
            int elementId = iElementIds.at(row);
            mapManager.deleteTabelMapRelation({ Physical_Set, panelId }, { Physical_Set, elementId }, row);
        }

        //QList<int> iElementIds22 = mapManager.getMapRelation<QList<int>>({ Physical_Set, panelId }, Physical_Set);

        for (int n = 0; n < elementLists.size(); n++)
        {
            QString eleText = elementLists.at(n);
            int id = getCVM->getModelData()->getPhyIDByName(Physical_Set, eleText);
            mapManager.appendTableMapRelation({ Physical_Set, panelId }, { Physical_Set, id }, n);
        }
        //增加节点集的映射
        if (enableCollectNode)
        {
            for (int n = 0; n < nodeSetLists.size(); n++)
            {
                QString nodeText = nodeSetLists.at(n);
                int id = getCVM->getModelData()->getPhyIDByName(Physical_Set, nodeText);
                mapManager.appendTableMapRelation({ Physical_Set, panelId }, { Physical_Set, id }, n);
            }
        }

        if (getCVM)
        {
            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Set, _setData->getSetId());
        }

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateSetSig(false, _setData->getSetId());
    }

    void mSetPanel::slot_addElementSetBtn()
    {
        if (getCVM == nullptr)
        {
            return;
        }
        QStringList eleList = getCVM->getModelData()
            ->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(Element);

        QStringList nodeSetList = getCVM->getModelData()
            ->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(Node);

        if (enableCollectNode)
        {
            QStringList t = eleList;
            t.append(nodeSetList);
            _elementModel->setStringList(t);
        }
        else
        {
            _elementModel->setStringList(eleList);
        }

        //  1) 插入item
        QListWidgetItem* newItem = new QListWidgetItem();
        _listAssignElement->insertItem(0, newItem);

        // 2) 创建编辑器并设置为item的widget
        QLineEdit* editor = new QLineEdit(_listAssignElement);
        _listAssignElement->setItemWidget(newItem, editor);
        editor->setFocus();

        // 3) 给每个editor单独创建model + completer (parent为editor)
        QCompleter* completer = new QCompleter(_elementModel, editor);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setFilterMode(Qt::MatchContains);
        completer->setCompletionMode(QCompleter::PopupCompletion);
        editor->setCompleter(completer);

        completer->complete();

        // 4) 安全引用
        // 用 QPointer管理editor的有效性
        QPointer<QLineEdit> editorGuard(editor);
        // finishFlag 防止 finishEditing 被重复执行
        auto finishFlag = std::make_shared<bool>(false);

        // helper: 安全地结束编辑
        std::function<void()> finishEditing = [=]() mutable
            {
                if (*finishFlag) return;    // 已经完成过一次,直接返回
                *finishFlag = true; // 标志为已完成

                // 如果 editor 已经被删除, 没必要继续
                if (!editorGuard)
                {
                    // 尝试把item从列表里移除 (如果仍存在)
                    int r = _listAssignElement->row(newItem);
                    if (r >= 0)
                    {
                        delete _listAssignElement->takeItem(r);
                    }
                    return;
                }

                //先查 item是否还在列表中
                int row = _listAssignElement->row(newItem);
                if (row < 0)
                {
                    // item不在列表(可能已被外部删除),直接清理editor
                    editorGuard->deleteLater();
                    return;
                }

                // 先把 editor widget从item中移除(但要不要delte item还没决定)
                _listAssignElement->removeItemWidget(newItem);

                QString text = editorGuard->text().trimmed();

                // 1) 空文本 -> 删除该行
                if (text.isEmpty())
                {
                    delete _listAssignElement->takeItem(row);
                    editorGuard->deleteLater();
                    return;
                }

                // 2) 非候选 -> 删除
                QStringList candidates = _elementModel->stringList();
                if (!candidates.contains(text))
                {
                    delete _listAssignElement->takeItem(row);
                    editorGuard->deleteLater();
                    return;
                }

                // 3) 去重：检查列表中是否存在相同文本（忽略newItem本身）
                bool exists = false;
                for (int i = 0; i < _listAssignElement->count(); ++i)
                {
                    QListWidgetItem* it = _listAssignElement->item(i);
                    if (!it) continue;
                    if (it == newItem) continue;
                    if (it->text() == text) { exists = true; break; }
                }
                if (exists)
                {
                    delete _listAssignElement->takeItem(row);
                    editorGuard->deleteLater();
                    return;
                }

                // 通过校验 -> 写入item文本,保留item
                newItem->setText(text);
                int elementId = getCVM->getModelData()->getPhyIDByName(Physical_Set, text);
                highlightElement(elementId);
                //从0开始
                int mapIndex = _listAssignElement->count() - 1;
                this->appendElementPanelMap(elementId, mapIndex);
                auto set = this->toType<mPanel>(_setData);

                // 获取所有单元集名称
                auto elementLists = getCVM->getModelData()
                    ->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(Element);
                if (elementLists.contains(text))
                {
                    set->appendEle(text);
                }
                else
                {
                    set->appendNodeName(text);
                }

                if (getCVM)
                {
                    getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Set, _setData->getSetId());
                }

                editorGuard->deleteLater();
            };
        // completer激活时（鼠标点击/回车选项） - 使用editor作为receiver（自动断开在editor销毁时）
        connect(completer, QOverload<const QString&>::of(&QCompleter::activated),
            editor,  // receiver
            [=](const QString& choice)
            {
                if (!editorGuard) return;
                editorGuard->setText(choice);
                finishEditing();
            }, Qt::UniqueConnection);

        connect(editor, &QLineEdit::editingFinished, editor, [=]() {
            finishEditing();
            }, Qt::UniqueConnection);

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateSetSig(false, _setData->getSetId());
    }

    void mSetPanel::slot_reduceElementSetBtn()
    {
        if (getCVM == nullptr)
        {
            return;
        }

        int panelId = _setData->getSetId();

        auto items = _listAssignElement->selectedItems();
        auto set = this->toType<mPanel>(_setData);
        for (QListWidgetItem* item : items)
        {
            if (!item) continue;

            QString text = item->text().trimmed();
            if (!text.isEmpty())
            {
                //从(面板)集合的映射中删除
                int row = _listAssignElement->row(item);
                int elementId = getCVM->getModelData()->getPhyIDByName(Physical_Set, text);
                getCVM->getModelData()->getMapRelationManager().deleteTabelMapRelation(
                    { Physical_Set, panelId }, { Physical_Set, elementId }, row);
            }
            //从列表中删除
            delete _listAssignElement->takeItem(_listAssignElement->row(item));

            auto elementLists = getCVM->getModelData()
                ->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(Element);
            if (set->getPanelCollectionType() == 0)
            {
                set->deleteEle(text);
            }
            else if (set->getPanelCollectionType() == 1)
            {
                set->removeNodeName(text);
            }

        }

        if (getCVM)
        {
            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Set, _setData->getSetId());
        }

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateSetSig(false, _setData->getSetId());
    }

    void mSetPanel::slot_clearElementSetBtn()
    {
        _listAssignElement->clear();
        std::vector<MeshSelectInfoData> eles;
        MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8);
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()
            ->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshElement, {});
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()
            ->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshElement, eles);
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();

        //删除映射
        auto set = this->toType<mPanel>(_setData);
        int panelId = _setData->getSetId();
        mMapRelationManager& mapManager = getCVM->getModelData()->getMapRelationManager();
        QList<int> iElementIds = mapManager.getMapRelation<QList<int>>({ Physical_Set, panelId }, Physical_Set);
        for (int row = 0; row < iElementIds.size(); row++)
        {
            int elementId = iElementIds.at(row);
            mapManager.deleteTabelMapRelation({ Physical_Set, panelId }, { Physical_Set, elementId }, row);
        }

        set->clearEles();
        set->clearNodeNames();

        if (getCVM)
        {
            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Set, _setData->getSetId());
        }

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateSetSig(false, _setData->getSetId());
    }

    void mSetPanel::refreshPanelListByCollectionType()
    {
        if (!getCVM || _setData == nullptr)
        {
            return;
        }

        auto set = this->toType<mPanel>(_setData);
        if (!set)
        {
            return;
        }

        _listAssignElement->clear();

        auto elementLists = getCVM->getModelData()
            ->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(Element);
        auto nodeSetLists = getCVM->getModelData()
            ->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(Node);

        int collectionType = set->getPanelCollectionType();
        if (collectionType == 0)
        {
            for (auto ele : set->elenames())
            {
                if (elementLists.contains(ele))
                    _listAssignElement->addItem(ele);
            }
        }
        else
        {
            for (auto name : set->nodeNames())
            {
                if (elementLists.contains(name) || nodeSetLists.contains(name))
                    _listAssignElement->addItem(name);
            }
        }
    }


    void mSetPanel::slot_analyticalSurfaceDataChanged()
    {
        if (!_setData || _setData->getSetType() != MxFunctions::SetType::SurfaceAnalysis)
            return;

        // 保存当前界面所有输入到数据类
        saveAnalyticalSurfaceData();

        // 通知树和管理器数据已更新
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateSetSig(false, _setData->getSetId());
    }

    void mSetPanel::slot_contourAddLine()
    {
        int row = _tableSegContour->rowCount();
        _tableSegContour->insertRow(row);

        // 第0列：类型下拉框（暂时只有line）
        QComboBox* typeCombo = new QComboBox(_tableSegContour);
        typeCombo->addItem("line");
        _tableSegContour->setCellWidget(row, 0, typeCombo);

        // 第1、2列：X1、Y1，可编辑
        _tableSegContour->setItem(row, 1, new QTableWidgetItem(""));
        _tableSegContour->setItem(row, 2, new QTableWidgetItem(""));

        // 第3、4列：X2、Y2，line类型不可编辑
        QTableWidgetItem* itemX2 = new QTableWidgetItem("");
        itemX2->setFlags(itemX2->flags() & ~Qt::ItemIsEditable);
        _tableSegContour->setItem(row, 3, itemX2);

        QTableWidgetItem* itemY2 = new QTableWidgetItem("");
        itemY2->setFlags(itemY2->flags() & ~Qt::ItemIsEditable);
        _tableSegContour->setItem(row, 4, itemY2);

        createSymbol();
    }

    void mSetPanel::slot_contourDeleteLine()
    {
        auto items = _tableSegContour->selectedItems();
        if (items.empty())
            return;
        QSet<int> rows;
        for (auto* item : items)
        {
            rows.insert(item->row());
        }
        if (rows.isEmpty())
            return;

        QList<int> rowList = rows.values();
        std::sort(rowList.begin(), rowList.end(), std::greater<int>());
        for (int r : rowList)
        {
            _tableSegContour->removeRow(r);
        }
    }

    void mSetPanel::slot_contourClearTable()
    {
        // 只清除3行以后，保留0、1、2初始行
        while (_tableSegContour->rowCount() > 3)
        {
            _tableSegContour->removeRow(_tableSegContour->rowCount() - 1);
        }
        // 初始三行数据重置回默认
        QStringList rowName = { "line", "circl", "circl" };
        for (int r = 0; r < 3; r++)
        {
            _tableSegContour->setItem(r, 0, new QTableWidgetItem(rowName[r]));
            _tableSegContour->setItem(r, 1, new QTableWidgetItem(""));
            _tableSegContour->setItem(r, 2, new QTableWidgetItem(""));
            _tableSegContour->setItem(r, 3, new QTableWidgetItem(""));
            _tableSegContour->setItem(r, 4, new QTableWidgetItem(""));

            auto* seqItem = _tableSegContour->item(r, 0);
            if (seqItem)
            {
                seqItem->setFlags(seqItem->flags() & (~Qt::ItemIsEditable));
            }

            if (r == 0)
            {
                auto* item3 = _tableSegContour->item(0, 3);
                auto* item4 = _tableSegContour->item(0, 4);
                if (item3)
                    item3->setFlags(item3->flags() & (~Qt::ItemIsEditable));
                if (item4)
                    item4->setFlags(item4->flags() & (~Qt::ItemIsEditable));
            }
        }
    }

    void mSetPanel::slot_contourLineChanged(QTableWidgetItem* item)
    {
        Q_UNUSED(item);
        if (!_setData || _setData->getSetType() != MxFunctions::SetType::SurfaceAnalysis)
            return;
        auto sa = toType<mSurfaceAnalysis>(_setData);
        if (!sa || sa->surfaceSubType() != MechSetData::SegmentSurface)
            return;

        saveAnalyticalSurfaceData();

        emit MechCommon::mMechGlobalSignals::getInstance()
            ->createUpdateSetSig(false, _setData->getSetId());
    }

    void mSetPanel::updateAnalyticalSurfaceFolds()
    {
        // 先全部隐藏
        m_foldWidgets[1]->setVisible(false);
        m_foldWidgets[2]->setVisible(false);
        m_foldWidgets[3]->setVisible(false);
        m_foldWidgets[4]->setVisible(false);
        m_foldWidgets[5]->setVisible(false);
        m_foldWidgets[6]->setVisible(false);
        m_foldWidgets[7]->setVisible(false);
        m_foldWidgets[8]->setVisible(false);
        m_foldWidgets[9]->setVisible(false);
        m_foldWidgets[10]->setVisible(false);

        QString subType = _comboLabelType->currentText();
        if (subType == "旋转解析平面")
        {
            m_foldWidgets[2]->setVisible(true);  // 圆柱局部坐标系定义
            m_foldWidgets[3]->setVisible(true);  // 起点坐标
            m_foldWidgets[4]->setVisible(true);  // 终点坐标
        }
        else if (subType == "拉伸解析平面")
        {
            m_foldWidgets[5]->setVisible(true);  // 局部坐标系定义
            m_foldWidgets[6]->setVisible(true);  // 起点坐标
            m_foldWidgets[7]->setVisible(true);  // 终点坐标
        }
        else if (subType == "解析线段")
        {
            m_foldWidgets[8]->setVisible(true);  // 局部坐标系定义
            m_foldWidgets[9]->setVisible(true);  // 起点
            m_foldWidgets[10]->setVisible(true);  // 线段轮廓
        }
    }

    void mSetPanel::createSymbol()
    {
        auto sa = toType<mSurfaceAnalysis>(_setData);
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (!sa) return;

        if (symbolController->isExistSymbol(m_symbolName))
            symbolController->deleteSymbol({ m_symbolName });
        if (sa->surfaceSubType() == MechSetData::RevolutionSurface)
        {
            // 唯一命名：集合名+类型后缀，避免重名冲突
            m_symbolName = QString("SA_Rev_%1").arg(_setData->getSetName());
            const auto& cylinderRW = symbolController->getOrCreateSymbolData<Cylinder_RW>(m_symbolName);

            // 1. 计算旋转轴全局参数
            QVector3D pointA(sa->xa(), sa->ya(), sa->za());
            QVector3D pointB(sa->xb(), sa->yb(), sa->zb());
            QVector3D axisDir = pointB - pointA;
            float axisLen = axisDir.length();

            // 合法性校验：轴向量为零、半径/长度无效则隐藏
            cylinderRW->_showState = true;
            if (axisLen < 1e-6f || sa->r1() <= 0.f)
            {
                cylinderRW->_showState = false;
            }
            QVector3D axisNormal = axisDir.normalized();

            // 2. 参数映射：解析平面参数 → Cylinder_RW 参数
            // 圆柱底面中心 = 轴上A点 + 轴向偏移z1
            QVector3D bottomCenter = pointA + axisNormal * sa->z1();
            cylinderRW->_pos = { bottomCenter.x(), bottomCenter.y(), bottomCenter.z() };
            // 轴向 = 旋转轴方向
            cylinderRW->_normal = { axisNormal.x(), axisNormal.y(), axisNormal.z() };
            // 半径 = 母线径向r1（圆柱等径，取r1即可）
            cylinderRW->_radius = sa->r1();
            // 圆柱长度 = 母线轴向长度 z2-z1
            cylinderRW->_L = sa->z2() - sa->z1();
            // 尺寸系数默认1.0
            cylinderRW->_size = 1.0f;
            cylinderRW->_isBottomTopFace = false;
            symbolController->createSymbol(cylinderRW);
        }
        else if (sa->surfaceSubType() == MechSetData::CylinderSurface)
        {
            m_symbolName = QString("SA_Cyl_%1").arg(_setData->getSetName());
            const auto& finiteRW = symbolController->getOrCreateSymbolData<Finite_RW>(m_symbolName);

            // 1. 局部坐标系基矢（全局坐标）
            QVector3D origin(sa->ox(), sa->oy(), sa->oz());
            QVector3D xAxis(sa->xx(), sa->xy(), sa->xz());
            QVector3D zAxis(sa->zx(), sa->zy(), sa->zz());
            xAxis.normalize();
            zAxis.normalize();
            QVector3D yAxis = QVector3D::crossProduct(zAxis, xAxis); // 补全正交y轴

            // 2. 计算母线两端点（局部xy平面 → 全局坐标）
            QVector3D pStart = origin + xAxis * sa->sx() + yAxis * sa->sy();
            QVector3D pEnd = origin + xAxis * sa->ex() + yAxis * sa->ey();
            QVector3D lineDir = pEnd - pStart;
            float lineLen = lineDir.length();

            // 3. 合法性校验：母线长度为0、拉伸长度≤0则隐藏
            finiteRW->_showState = true;
            //if (lineLen < 1e-6f || sa->el() <= 0.f)
            //{
            //    finiteRW->_showState = false;
            //}
            QVector3D lineDirNorm = lineDir.normalized();

            // 4. 参数映射：线段沿z轴拉伸 → 有限大刚性墙
            // 矩形中心 = 母线中点 + z轴方向偏移 el/2
            QVector3D center = (pStart + pEnd) * 0.5f + zAxis * (sa->el() * 0.5f);
            finiteRW->_pos = { center.x(), center.y(), center.z() };

            // 平面法向 = 母线方向 × 拉伸方向(z轴)，天然垂直于z轴，平面与z轴平行
            QVector3D normal = QVector3D::crossProduct(lineDirNorm, zAxis).normalized();
            finiteRW->_normal = { normal.x(), normal.y(), normal.z() };

            // 长度方向 = 母线方向；长度 = 母线长度；宽度 = 拉伸长度el
            finiteRW->_L_vec = { lineDirNorm.x(), lineDirNorm.y(), lineDirNorm.z() };
            finiteRW->_L = lineLen;
            finiteRW->_W = sa->el();

            // 通用参数
            finiteRW->_size = 1.0f;
            finiteRW->_isFixedPixel = false;
            symbolController->createSymbol(finiteRW);
        }
        else if (sa->surfaceSubType() == MechSetData::SegmentSurface)
        {
            m_symbolName = QString("SA_Seg_%1").arg(_setData->getSetName());
            const auto& univSym = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(m_symbolName);

            // 1. 局部坐标系基矢（全局）
            QVector3D origin(sa->ox(), sa->oy(), sa->oz());
            QVector3D xAxis(sa->xx(), sa->xy(), sa->xz());
            QVector3D zAxis(sa->zx(), sa->zy(), sa->zz());
            xAxis.normalize();
            zAxis.normalize();
            QVector3D yAxis = QVector3D::crossProduct(zAxis, xAxis);

            // 2. 构建顶点序列：起点 → 第1段终点 → 第2段终点 → ...
            //    每段LINE的(x1,y1)是该段终点，同时作为下一段的起点
            QVector<QVector3D> vertices;

            // 第0个顶点：轮廓总起点 START
            QVector3D startPt = origin + xAxis * sa->sx() + yAxis * sa->sy();
            vertices.push_back(startPt);

            // 依次追加每段LINE的终点
            const auto& contours = sa->contours();
            for (const auto& row : contours)
            {
                if (row.rowType == "line")
                {
                    QVector3D segEnd = origin + xAxis * row.x1 + yAxis * row.y1;
                    vertices.push_back(segEnd);
                }
            }
            // 3. 构建线索引：相邻顶点两两连接，形成连续折线
            //    索引序列：0,1, 1,2, 2,3, ...
            //    第1段：顶点0→顶点1，第2段：顶点1→顶点2（顶点1既是第1段终点也是第2段起点）
            std::vector<unsigned int> lineIndices;
            for (unsigned int i = 0; i + 1 < static_cast<unsigned int>(vertices.size()); ++i)
            {
                lineIndices.push_back(i);       // 本段起点
                lineIndices.push_back(i + 1);   // 本段终点 = 下一段起点
            }
            // 4. 赋值图元参数
            univSym->_vexter = vertices;
            univSym->_lineIndex = lineIndices;
            univSym->_size = 1.0f;
            univSym->_lineIsFixedPixel = false;
            univSym->_lineDepthState = false;
            univSym->_showState = true;
            symbolController->createSymbol(univSym);
        }
        symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mSetPanel::loadAnalyticalSurfaceData()
    {
        auto sa = toType<mSurfaceAnalysis>(_setData);
        if (!sa) return;

        auto toStr = [](double v) -> QString {
            return v == 0.0 ? "" : MxFunctions::mNumberToString(v);
            };

        if (sa->surfaceSubType() == MechSetData::RevolutionSurface)
        {
            _Xa->setText(toStr(sa->xa()));
            _Ya->setText(toStr(sa->ya()));
            _Za->setText(toStr(sa->za()));
            _Xb->setText(toStr(sa->xb()));
            _Yb->setText(toStr(sa->yb()));
            _Zb->setText(toStr(sa->zb()));
            _r1->setText(toStr(sa->r1()));
            _z1->setText(toStr(sa->z1()));
            _r2->setText(toStr(sa->r2()));
            _z2->setText(toStr(sa->z2()));
        }
        else if (sa->surfaceSubType() == MechSetData::CylinderSurface)
        {
            _Ox->setText(toStr(sa->ox()));
            _Oy->setText(toStr(sa->oy()));
            _Oz->setText(toStr(sa->oz()));
            _Xx->setText(toStr(sa->xx()));
            _Xy->setText(toStr(sa->xy()));
            _Xz->setText(toStr(sa->xz()));
            _Zx->setText(toStr(sa->zx()));
            _Zy->setText(toStr(sa->zy()));
            _Zz->setText(toStr(sa->zz()));
            _Sx->setText(toStr(sa->sx()));
            _Sy->setText(toStr(sa->sy()));
            _Ex->setText(toStr(sa->ex()));
            _Ey->setText(toStr(sa->ey()));
            _El->setText(toStr(sa->el()));
        }
        else if (sa->surfaceSubType() == MechSetData::SegmentSurface)
        {
            _segOx->setText(toStr(sa->ox()));
            _segOy->setText(toStr(sa->oy()));
            _segOz->setText(toStr(sa->oz()));
            _segXx->setText(toStr(sa->xx()));
            _segXy->setText(toStr(sa->xy()));
            _segXz->setText(toStr(sa->xz()));
            _segZx->setText(toStr(sa->zx()));
            _segZy->setText(toStr(sa->zy()));
            _segZz->setText(toStr(sa->zz()));
            _segSx->setText(toStr(sa->sx()));
            _segSy->setText(toStr(sa->sy()));

            // 解析线段轮廓表格加载
            disconnect(m_contourTableChangeConn);
            const auto& contours = sa->contours();
            _tableSegContour->setRowCount(contours.size());
            for (int i = 0; i < contours.size(); ++i)
            {
                const auto& row = contours[i];

                // 第0列：类型下拉框
                QComboBox* typeCombo = new QComboBox(_tableSegContour);
                typeCombo->addItem("line");
                typeCombo->setCurrentText(row.rowType);
                _tableSegContour->setCellWidget(i, 0, typeCombo);

                // 第1、2列：X1、Y1
                _tableSegContour->setItem(i, 1, new QTableWidgetItem(toStr(row.x1)));
                _tableSegContour->setItem(i, 2, new QTableWidgetItem(toStr(row.y1)));

                // 第3、4列：X2、Y2，line不可编辑
                QTableWidgetItem* itemX2 = new QTableWidgetItem("");
                itemX2->setFlags(itemX2->flags() & ~Qt::ItemIsEditable);
                _tableSegContour->setItem(i, 3, itemX2);

                QTableWidgetItem* itemY2 = new QTableWidgetItem("");
                itemY2->setFlags(itemY2->flags() & ~Qt::ItemIsEditable);
                _tableSegContour->setItem(i, 4, itemY2);
            }
            m_contourTableChangeConn = connect(_tableSegContour, &QTableWidget::itemChanged,
                this, &mSetPanel::slot_contourLineChanged);
        }
    }

    void mSetPanel::saveAnalyticalSurfaceData()
    {
        auto sa = toType<mSurfaceAnalysis>(_setData);
        if (!sa) return;

        if (sa->surfaceSubType() == MechSetData::RevolutionSurface)
        {
            sa->setXa(_Xa->text().toDouble());
            sa->setYa(_Ya->text().toDouble());
            sa->setZa(_Za->text().toDouble());
            sa->setXb(_Xb->text().toDouble());
            sa->setYb(_Yb->text().toDouble());
            sa->setZb(_Zb->text().toDouble());
            sa->setR1(_r1->text().toDouble());
            sa->setZ1(_z1->text().toDouble());
            sa->setR2(_r2->text().toDouble());
            sa->setZ2(_z2->text().toDouble());
        }
        else if (sa->surfaceSubType() == MechSetData::CylinderSurface)
        {
            sa->setOx(_Ox->text().toDouble());
            sa->setOy(_Oy->text().toDouble());
            sa->setOz(_Oz->text().toDouble());
            sa->setXx(_Xx->text().toDouble());
            sa->setXy(_Xy->text().toDouble());
            sa->setXz(_Xz->text().toDouble());
            sa->setZx(_Zx->text().toDouble());
            sa->setZy(_Zy->text().toDouble());
            sa->setZz(_Zz->text().toDouble());
            sa->setSx(_Sx->text().toDouble());
            sa->setSy(_Sy->text().toDouble());
            sa->setEx(_Ex->text().toDouble());
            sa->setEy(_Ey->text().toDouble());
            sa->setEl(_El->text().toDouble());
        }
        else if (sa->surfaceSubType() == MechSetData::SegmentSurface)
        {
            sa->setOx(_segOx->text().toDouble());
            sa->setOy(_segOy->text().toDouble());
            sa->setOz(_segOz->text().toDouble());
            sa->setXx(_segXx->text().toDouble());
            sa->setXy(_segXy->text().toDouble());
            sa->setXz(_segXz->text().toDouble());
            sa->setZx(_segZx->text().toDouble());
            sa->setZy(_segZy->text().toDouble());
            sa->setZz(_segZz->text().toDouble());
            sa->setSx(_segSx->text().toDouble());
            sa->setSy(_segSy->text().toDouble());

            QVector<SegmentContourRow> contours;
            for (int i = 0; i < _tableSegContour->rowCount(); ++i)
            {
                SegmentContourRow row;
                // 类型从下拉框取
                QComboBox* typeCombo = qobject_cast<QComboBox*>(_tableSegContour->cellWidget(i, 0));
                row.rowType = typeCombo ? typeCombo->currentText() : "line";

                // X1、Y1
                auto* item1 = _tableSegContour->item(i, 1);
                auto* item2 = _tableSegContour->item(i, 2);
                row.x1 = item1 ? item1->text().toDouble() : 0;
                row.y1 = item2 ? item2->text().toDouble() : 0;

                // line类型X2/Y2固定为0
                row.x2 = 0;
                row.y2 = 0;

                contours.append(row);
            }
            sa->setContours(contours);
        }
    }

}