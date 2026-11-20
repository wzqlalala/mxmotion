#include "mCoordinatePanel.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"

//MxFunctions
#include "mGlobalSignals.h"
#include "mContainerOperation.h"
#include "PickCommonFunction.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
//label
#include "mxr/label/mSymbolController.h"
#include "mxr/view/mShapeRender.h"

//BasicData
#include "mPhysicalFieldEnum.h"
#include "mCoordinateManager.h"
#include "mCoordinateBase.h"
#include "mModelData.h"
#include "mMeshData.h"
#include "mMapRelationManager.h"
#include "mFieldManager.h"
#include "mDiscreteField.h"

//Geo
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（拉 mOCCDocument→OCC 链；冻结岛保档）

#include <QFormLayout>

using namespace MechCoordinateData;
using namespace MxFunctions;
using namespace mxr;
using namespace MechPreData;
using namespace MechDiscreteFieldData;
namespace MechMainWindow
{
    mCoordinatePanel::mCoordinatePanel(QWidget* parent) : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);
        _mainLayout->setContentsMargins(2, 0, 2, 0);

        _coorData = nullptr;
        _theta = QString::fromUtf8("\u03B8");

        // ========== 基本设置折叠块 ==========
        FoldSection* defineFold = new FoldSection("定义", this);

        // 类型
        _comboType = new QComboBox();
        _comboType->addItems({ "直角坐标系", "圆柱坐标系", "球坐标系", "离散坐标系" });
        defineFold->addRow(new PanelLabel("类型"), _comboType);

        // 定义方式
        _labelDefine = new PanelLabel("定义方式");
        _comboDefine = new QComboBox();
        _comboDefine->addItems({ "三点","基准轴" });
        defineFold->addRow(_labelDefine, _comboDefine);

        // 离散场
        _labelDiscreteField = new PanelLabel("离散场");
        _comboDiscreteField = new QComboBox();
        defineFold->addRow(_labelDiscreteField, _comboDiscreteField);
        m_foldWidgets.append(defineFold);
        _mainLayout->addWidget(defineFold);


        //原点折叠块（mPointPickerWidget）
        FoldSection* originFold = new FoldSection("原点", this, QColor(0, 204, 184));
        _originPicker = new mPointPickerWidget(QStringLiteral("原点位置"), this);
        connect(_originPicker, &mPointPickerWidget::coordinateChanged, this, [this]() {
            if (!_coorData) return;
            _coorData->setOrigin(_originPicker->coordinate());
            requestUpdate();
            });
        originFold->addRow(_originPicker);
        m_foldWidgets.append(originFold);
        _mainLayout->addWidget(originFold);


        // ========== 主轴折叠块 ==========
        FoldSection* mainAxisFold = new FoldSection("主轴", this);

        _mainAxis = new QComboBox(); _mainAxis->addItems({ "X", "Y", "Z" });
        _mainReverse = new QPushButton("反向");
        connect(_mainReverse, &QPushButton::pressed, this, [this]() {
            if (!_coorData)
            {
                return;
            }
            _coorData->setMainReverse();
            requestUpdate();
            });
        _mainRef = new QComboBox(); _mainRef->addItems({ "模型特征", "全局X", "全局Y", "全局Z" });

        // 方向控件（替换原"位置+应用"拾取控件）：方向变化时写入X线
        // 选择对象拾取颜色块：红色（主轴=X轴，与坐标系渲染X轴红色对应）
        _mainDirSelector = new mDirectionSelectorWidget(QStringLiteral("方向"), false, QString(), false, QString(), false, mainAxisFold->contentWidget(), CBRed);
        connect(_mainDirSelector, &mDirectionSelectorWidget::directionChanged, this, [this]() {
            if (!_coorData)
            {
                return;
            }
            QVariant res = _mainDirSelector->getDirection();
            if (!res.isValid())
            {
                return;
            }
            _coorData->setXLine(res.value<QVector3D>());
            requestUpdate();
            });
        connect(_mainDirSelector, &mDirectionSelectorWidget::layoutRefreshRequested, this, [mainAxisFold]() {
            mainAxisFold->refreshLayout();
            });
        connect(_mainRef, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, mainAxisFold](int index) {
            // "模型特征"时才有方向控件行；按移除/插入行处理，隐藏行不占表单布局空间，避免残留空白
            setFormRowPresent(mainAxisFold, _mainDirSelector, index == 0);
            if (index != 0)
            {
                // 行隐藏后失活内部拾取，避免隐藏控件仍占用拾取状态
                deactivateDirectionSelector(_mainDirSelector);
            }
            mainAxisFold->refreshLayout();
            if (!_coorData)
            {
                return;
            }
            _coorData->setMainBase(index);
            requestUpdate();
            });
        mainAxisFold->addRow(new PanelLabel("轴向"), _mainAxis, _mainReverse);
        mainAxisFold->addRow(new PanelLabel("基于"), _mainRef);
        mainAxisFold->addRow(_mainDirSelector);
        m_foldWidgets.append(mainAxisFold);
        _mainLayout->addWidget(mainAxisFold);


        // ========== 主轴朝向折叠块 ==========
        FoldSection* dirFold = new FoldSection("主轴朝向", this);

        _dirAxis = new QComboBox(); _dirAxis->addItems({ "Y", "Z" });
        connect(_dirAxis, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!_coorData)
            {
                return;
            }
            _coorData->setDirIndex(index);
            requestUpdate();
            });
        _dirReverse = new QPushButton("反向");
        connect(_dirReverse, &QPushButton::pressed, this, [this]() {
            if (!_coorData)
            {
                return;
            }
            _coorData->setDirReverse();
            requestUpdate();
            });
        _dirRef = new QComboBox(); _dirRef->addItems({ "模型特征", "全局X", "全局Y", "全局Z" });

        // 方向控件（替换原"位置+应用"拾取控件）：方向变化时写入Y线
        // 选择对象拾取颜色块：绿色（主轴朝向=Y轴，与坐标系渲染Y轴绿色对应，区别于主轴的红色）
        _dirDirSelector = new mDirectionSelectorWidget(QStringLiteral("方向"), false, QString(), false, QString(), false, dirFold->contentWidget(), CBGreen);
        connect(_dirDirSelector, &mDirectionSelectorWidget::directionChanged, this, [this]() {
            if (!_coorData)
            {
                return;
            }
            QVariant res = _dirDirSelector->getDirection();
            if (!res.isValid())
            {
                return;
            }
            _coorData->setYLine(res.value<QVector3D>());
            requestUpdate();
            });
        connect(_dirDirSelector, &mDirectionSelectorWidget::layoutRefreshRequested, this, [dirFold]() {
            dirFold->refreshLayout();
            });
        connect(_dirRef, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, dirFold](int index) {
            // "模型特征"时才有方向控件行；按移除/插入行处理，隐藏行不占表单布局空间，避免残留空白
            setFormRowPresent(dirFold, _dirDirSelector, index == 0);
            if (index != 0)
            {
                // 行隐藏后失活内部拾取，避免隐藏控件仍占用拾取状态
                deactivateDirectionSelector(_dirDirSelector);
            }
            dirFold->refreshLayout();
            if (!_coorData)
            {
                return;
            }
            _coorData->setDirBase(index);
            requestUpdate();
            });
        dirFold->addRow(new PanelLabel("轴向"), _dirAxis, _dirReverse);
        dirFold->addRow(new PanelLabel("基于"), _dirRef);
        dirFold->addRow(_dirDirSelector);
        m_foldWidgets.append(dirFold);
        _mainLayout->addWidget(dirFold);

        //轴点折叠块（mPointPickerWidget）
        FoldSection* pt1Fold = new FoldSection("轴点", this, QColor(54, 79, 107));

        // 轴选择 + 反向
        _pt1 = new QComboBox(); _pt1->addItems({ "X", "Y", "Z" });
        _pt1Reverse = new QPushButton("反向");
        connect(_pt1Reverse, &QPushButton::pressed, this, [this]() {
            if (!_coorData)
            {
                return;
            }
            _coorData->setXPointReverse();
            // 反向后同步更新输入框显示
            _pt1Picker->setCoordinate(_coorData->getXPoint());
            requestUpdate();
            });
        pt1Fold->addRow(new PanelLabel("轴上的点"), _pt1, _pt1Reverse);

        // 点坐标拾取控件（替换原位置按钮+三行输入）
        _pt1Picker = new mPointPickerWidget(QStringLiteral("位置"), this);
        connect(_pt1Picker, &mPointPickerWidget::coordinateChanged, this, [this]() {
            if (!_coorData) return;
            _coorData->setXPoint(_pt1Picker->coordinate());
            requestUpdate();
            });
        pt1Fold->addRow(_pt1Picker);
        m_foldWidgets.append(pt1Fold);
        _mainLayout->addWidget(pt1Fold);

        //平面点折叠块（PointPickerWidget）
        FoldSection* pt2Fold = new FoldSection("平面点", this, QColor(252, 81, 133));

        // 平面选择
        _pt2 = new QComboBox(); _pt2->addItems({ "X-Y", "X-Z" });
        connect(_pt2, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!_coorData)
            {
                return;
            }
            _coorData->setDirIndex(index);
            requestUpdate();
            });
        pt2Fold->addRow(new PanelLabel("平面上的点"), _pt2);

        // 点坐标拾取控件（替换原位置按钮+三行输入）
        _pt2Picker = new mPointPickerWidget(QStringLiteral("位置"), this);
        connect(_pt2Picker, &mPointPickerWidget::coordinateChanged, this, [this]() {
            if (!_coorData) return;
            _coorData->setXYPoint(_pt2Picker->coordinate());
            requestUpdate();
            });
        pt2Fold->addRow(_pt2Picker);
        m_foldWidgets.append(pt2Fold);
        _mainLayout->addWidget(pt2Fold);

        // ========== 方向矢量始终显示 ==========
        FoldSection* dirshowFold = new FoldSection("方向矢量", this, QColor("#909399"), false);
        QLabel* labelDirX = new PanelLabel("X 轴");
        _labelDirY = new PanelLabel("Y 轴");
        QLabel* labelDirZ = new PanelLabel("Z 轴");
        labelDirX->setAlignment(Qt::AlignVCenter);
        _labelDirY->setAlignment(Qt::AlignVCenter);
        labelDirZ->setAlignment(Qt::AlignVCenter);
        _dirX = new QLineEdit(); _dirY = new QLineEdit(); _dirZ = new QLineEdit();
        _dirX->setDisabled(true); _dirY->setDisabled(true); _dirZ->setDisabled(true);
        dirshowFold->addRow(labelDirX, _dirX);
        dirshowFold->addRow(_labelDirY, _dirY);
        dirshowFold->addRow(labelDirZ, _dirZ);
        m_foldWidgets.append(dirshowFold);
        _mainLayout->addWidget(dirshowFold);

        _mainLayout->addStretch();

        connect(MechPreData::mViewportModelManager::getInstance(), &MechPreData::mViewportModelManager::currentViewportChanged, this, &mCoordinatePanel::slot_subWindowActivated);

        // 绑定切换类型
        connect(_comboType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mCoordinatePanel::slot_onTypeChanged);

        // 绑定切换定义方式
        connect(_comboDefine, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mCoordinatePanel::slot_onDefineChanged);

        // 绑定离散场切换
        connect(_comboDiscreteField, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!_coorData || !getCVM)
                return;
            int coorID = _coorData->getCoordinateId();
            auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
            QString fieldName = _comboDiscreteField->currentText();
            if (!fieldName.isEmpty())
            {
                auto& manager = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
                int fieldID = manager.getIDByFieldName(fieldName);
                mapRelation.appendMapRelation({ Physical_Coordinate, coorID }, { Physical_Field, fieldID });
            }
            else
            {
                mapRelation.appendMapRelation({ Physical_Coordinate, coorID }, { Physical_Field, -1 });
            }
            });

        // 绑定切换主轴
        connect(_mainAxis, &QComboBox::currentTextChanged, this, &mCoordinatePanel::slot_onMainAxisChanged);

        // 绑定切换轴上的点
        connect(_pt1, &QComboBox::currentTextChanged, this, &mCoordinatePanel::slot_onAxisPointChanged);

        // 初始化：默认显示基准轴
        slot_onDefineChanged(1);
    }

    void mCoordinatePanel::newCoordinatePanel(MxFunctions::CoordinateType type, QVector3D origin)
    {
        if (!getCVM)
        {
            return;
        }
        //新建坐标系
        int coorID = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).createID();
        _coorData = new mCoordinateBase(coorID, type, getCVM->getModelData()->createNameByType(Physical_Coordinate, "System"));
        _coorData->setOrigin(origin);
        getCVM->getModelData()->addPhysicalField<mCoordinateBase>(_coorData);
        setEditing(true);

        //创建成功发送消息
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建坐标系 %1").arg(_coorData->getCoordinateName()), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateCoordinateSig(true, _coorData->getCoordinateId());
    }

    void mCoordinatePanel::editCoordinatePanel(int coordinateID)
    {
        setEditing(true);
        connectSig();

        _comboType->blockSignals(true);
        _comboDefine->blockSignals(true);
        _comboDiscreteField->blockSignals(true);
        _coorData = getCVM->getModelData()->getManager<mCoordinateManager>(Physical_Coordinate).getCoordinateData(coordinateID);

        /*****************************更新面板*************************/
        _comboType->setCurrentText(_coorData->getCoordinateTypeStr());//类型
        _comboDefine->setCurrentText(_coorData->getMethod());//定义方式

        // 坐标回填
        _originPicker->setCoordinate(_coorData->getOrigin());
        _pt1Picker->setCoordinate(_coorData->getXPoint());
        _pt2Picker->setCoordinate(_coorData->getXYPoint());

        _pt1->setCurrentIndex(_coorData->getMainIndex());//轴点
        _pt2->setCurrentIndex(_coorData->getDirIndex());//平面点

        _mainAxis->setCurrentIndex(_coorData->getMainIndex());//主轴
        _mainRef->setCurrentIndex(_coorData->getMainBase());
        _dirAxis->setCurrentIndex(_coorData->getDirIndex());//主轴朝向
        _dirRef->setCurrentIndex(_coorData->getDirBase());

        //判断是否为全局坐标系
        if (_coorData->getCoordinateId() == -1)
        {
            m_foldWidgets[2]->setVisible(false);
            m_foldWidgets[3]->setVisible(false);
            m_foldWidgets[4]->setVisible(false);
            m_foldWidgets[5]->setVisible(false);
            m_foldWidgets[0]->setEnabled(false);
            m_foldWidgets[1]->setEnabled(false);
        }
        else
        {
            bool isDiscrete = (_coorData->getCoordinateType() == MxFunctions::DiscreteCoordinate);
            if (isDiscrete)
                setDiscreteMode(true);
            else
            {
                setDiscreteMode(false);
                slot_onDefineChanged(_comboDefine->currentIndex());
            }
            m_foldWidgets[0]->setEnabled(true);
            m_foldWidgets[1]->setEnabled(!isDiscrete);
            m_foldWidgets[6]->setEnabled(!isDiscrete);
        }

        // 更新离散场下拉框
        if (_coorData->getCoordinateType() == MxFunctions::DiscreteCoordinate)
        {
            int discreteFieldID = -1;
            if (getCVM)
            {
                auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
                discreteFieldID = mapRelation.getMapRelation<int>({ Physical_Coordinate, _coorData->getCoordinateId() }, Physical_Field);
            }
            if (discreteFieldID >= 0)
            {
                auto& manager = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
                QString fieldName = manager.getFieldNameByID(discreteFieldID);
                _comboDiscreteField->setCurrentText(fieldName);
            }
            else
            {
                _comboDiscreteField->setCurrentIndex(-1);
            }
        }

        _comboType->blockSignals(false);
        _comboDefine->blockSignals(false);
        _comboDiscreteField->blockSignals(false);

        _lastSnapshot.first = false;
        requestUpdate();

        //进入编辑后激活第一个可见拾取控件
        if (_coorData&& _coorData->getCoordinateId() != -1)
        {
            activateFirstVisiblePicker();
        }
    }

    void mCoordinatePanel::uneditCoordinatePanel()
    {
        if (!isEditing())
        {
            return;
        }
        //入口统一复位编辑态：后续各早退分支无需再单独清标志
        setEditing(false);
        //退出编辑时失活所有拾取控件
        QList<mPointPickerWidget*> allPickers = { _originPicker, _pt1Picker, _pt2Picker };
        for (auto* picker : allPickers)
        {
            if (picker && picker->pickController())
            {
                picker->pickController()->setPickActive(false);
                picker->clearAllHighlight();
            }
        }

        //退出编辑时失活方向控件内部的拾取控件
        for (auto* selector : { _mainDirSelector, _dirDirSelector })
        {
            deactivateDirectionSelector(selector);
        }

        _coorData = nullptr;
        disconnectSig();

        //隐藏所有坐标系
        if (!getCVM || !getCVM->getPreRenderWidget())
        {
            return;
        }

        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("oPoint");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("xPoint");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("yPoint");
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mCoordinatePanel::slot_onTypeChanged(int index)
    {
        bool isDiscrete = (index == 3);
        setDiscreteMode(isDiscrete);

        if (!isDiscrete)
        {
            _mainAxis->clear();
            _pt1->clear();
            if (index == 0)
            {
                _mainAxis->addItems({ "X", "Y", "Z" });
                _pt1->addItems({ "X", "Y", "Z" });
                _labelDirY->setText("Y 轴");
            }
            else
            {
                _mainAxis->addItems({ "R", _theta, "Z" });
                _pt1->addItems({ "R", _theta, "Z" });
                _labelDirY->setText(_theta + "轴");
            }
        }
        if (_coorData == nullptr)
        {
            return;
        }
        _coorData->setCoordinateType(MxFunctions::CoordinateType(index));
        requestUpdate();

        //类型切换后刷新拾取激活状态
        activateFirstVisiblePicker();
    }

    void mCoordinatePanel::setDiscreteMode(bool discrete)
    {
        QFormLayout* formLayout = m_foldWidgets[0]->layout();

        // 查找"定义方式"所在行号
        int defineRow = -1;
        for (int i = 0; i < formLayout->rowCount(); ++i)
        {
            QLayoutItem* labelItem = formLayout->itemAt(i, QFormLayout::LabelRole);
            if (labelItem && labelItem->widget() == _labelDefine)
            {
                defineRow = i;
                break;
            }
        }

        if (discrete && defineRow >= 0)
        {
            QLayoutItem* labelItem = formLayout->itemAt(defineRow, QFormLayout::LabelRole);
            QLayoutItem* fieldItem = formLayout->itemAt(defineRow, QFormLayout::FieldRole);
            if (labelItem)
            {
                formLayout->removeItem(labelItem);
                delete labelItem;
            }
            if (fieldItem)
            {
                formLayout->removeItem(fieldItem);
                delete fieldItem;
            }
            _labelDefine->setParent(this);
            _comboDefine->setParent(this);
            _labelDefine->hide();
            _comboDefine->hide();
        }
        else if (!discrete && defineRow < 0)
        {
            formLayout->insertRow(1, _labelDefine, _comboDefine);
            _labelDefine->show();
            _comboDefine->show();
        }

        // 离散场标签和下拉框
        _labelDiscreteField->setVisible(discrete);
        _comboDiscreteField->setVisible(discrete);

        // 隐藏/显示相关折叠块
        m_foldWidgets[1]->setVisible(!discrete); // 原点
        m_foldWidgets[2]->setVisible(!discrete); // 主轴
        m_foldWidgets[3]->setVisible(!discrete); // 主轴朝向
        m_foldWidgets[4]->setVisible(!discrete); // 轴点
        m_foldWidgets[5]->setVisible(!discrete); // 平面点
        m_foldWidgets[6]->setVisible(!discrete); // 方向矢量

        // 刷新布局
        m_foldWidgets[0]->refreshLayout();

        if (discrete)
        {
            refreshDiscreteFieldCombo();
        }

        //离散模式切换后刷新拾取激活状态
        activateFirstVisiblePicker();
    }

    void mCoordinatePanel::refreshDiscreteFieldCombo()
    {
        _comboDiscreteField->blockSignals(true);
        _comboDiscreteField->clear();
        if (!getCVM)
        {
            _comboDiscreteField->blockSignals(false);
            return;
        }
        auto& manager = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
        QStringList names = manager.getAllFieldNames(MxFunctions::FieldType::Discrete);
        for (const QString& name : names)
        {
            int fieldID = manager.getIDByFieldName(name);
            auto* fieldData = manager.getDiscreteFieldData(fieldID);
            if (fieldData && fieldData->getSpaceType() == MxFunctions::DiscreteSpace::CoorSpace)
            {
                _comboDiscreteField->addItem(name);
            }
        }
        _comboDiscreteField->blockSignals(false);
    }

    void mCoordinatePanel::setFormRowPresent(FoldSection* fold, QWidget* label, QWidget* field, bool present)
    {
        if (!fold || !label || !field || !fold->layout())
        {
            return;
        }
        QFormLayout* formLayout = fold->layout();

        // 查找该行当前行号
        int row = -1;
        for (int i = 0; i < formLayout->rowCount(); ++i)
        {
            QLayoutItem* labelItem = formLayout->itemAt(i, QFormLayout::LabelRole);
            if (labelItem && labelItem->widget() == label)
            {
                row = i;
                break;
            }
        }

        if (!present && row >= 0)
        {
            // 从布局移除并删除布局项（不删控件），控件挂回面板保活，与"定义方式"行的处理一致
            QLayoutItem* labelItem = formLayout->itemAt(row, QFormLayout::LabelRole);
            QLayoutItem* fieldItem = formLayout->itemAt(row, QFormLayout::FieldRole);
            if (labelItem)
            {
                formLayout->removeItem(labelItem);
                delete labelItem;
            }
            if (fieldItem)
            {
                formLayout->removeItem(fieldItem);
                delete fieldItem;
            }
            label->setParent(this);
            field->setParent(this);
            label->hide();
            field->hide();
        }
        else if (present && row < 0)
        {
            // 位置行始终是末行，追加到表单末尾即可恢复原顺序
            formLayout->insertRow(formLayout->rowCount(), label, field);
            label->show();
            field->show();
        }
    }

    void mCoordinatePanel::setFormRowPresent(FoldSection* fold, QWidget* widget, bool present)
    {
        if (!fold || !widget || !fold->layout())
        {
            return;
        }
        QFormLayout* formLayout = fold->layout();

        // 查找该跨列整行控件当前行号
        int row = -1;
        for (int i = 0; i < formLayout->rowCount(); ++i)
        {
            QLayoutItem* item = formLayout->itemAt(i, QFormLayout::SpanningRole);
            if (item && item->widget() == widget)
            {
                row = i;
                break;
            }
        }

        if (!present && row >= 0)
        {
            // 从布局移除并删除布局项（不删控件），控件挂回面板保活，与"定义方式"行的处理一致
            QLayoutItem* item = formLayout->itemAt(row, QFormLayout::SpanningRole);
            if (item)
            {
                formLayout->removeItem(item);
                delete item;
            }
            widget->setParent(this);
            widget->hide();
        }
        else if (present && row < 0)
        {
            // 方向控件行始终是末行，追加到表单末尾即可恢复原顺序
            formLayout->insertRow(formLayout->rowCount(), widget);
            widget->show();
        }
    }

    void mCoordinatePanel::deactivateDirectionSelector(mDirectionSelectorWidget* selector)
    {
        if (!selector)
        {
            return;
        }
        // 仅失活拾取（不动控件可见性），后续激活仍由控件内部按模式自管理
        const QList<mPickWidgetController*> controllers = {
            selector->startPickController(), selector->endPickController(), selector->objectPickController() };
        for (auto* controller : controllers)
        {
            if (controller)
            {
                controller->setPickActive(false);
                controller->clearAllHighlight();
            }
        }
    }

    void mCoordinatePanel::slot_onDefineChanged(int index)
    {
        bool point2Axis = (index == 1); // 点+两轴
        m_foldWidgets[2]->setVisible(point2Axis);
        m_foldWidgets[3]->setVisible(point2Axis);

        m_foldWidgets[4]->setVisible(!point2Axis);
        m_foldWidgets[5]->setVisible(!point2Axis);

        // 刷新布局，避免间距错乱
        m_foldWidgets[0]->refreshLayout();
        if (_coorData == nullptr)
        {
            return;
        }
        _coorData->setMethod(_comboDefine->currentText());
        requestUpdate();

        // 定义方式切换后刷新拾取激活状态
        activateFirstVisiblePicker();
    }

    void mCoordinatePanel::slot_onMainAxisChanged(const QString& text)
    {
        auto dirAxisText = _dirAxis->currentText();
        _dirAxis->clear();
        if (text == "X")
        {
            _dirAxis->addItems({ "Y", "Z" });
        }
        else if (text == "Y")
        {
            _dirAxis->addItems({ "X", "Z" });
        }
        else if (text == "Z")
        {
            _dirAxis->addItems({ "X", "Y" });
        }
        else if (text == "R")
        {
            _dirAxis->addItems({ _theta, "Z" });
        }
        else if (text == _theta)
        {
            _dirAxis->addItems({ "R", "Z" });
        }
        else if (text == "Z")
        {
            _dirAxis->addItems({ "X", _theta });
        }
        if (!_coorData)
        {
            return;
        }
        _coorData->setMainIndex(_mainAxis->currentIndex());
        requestUpdate();
    }
    void mCoordinatePanel::slot_onAxisPointChanged(const QString& text)
    {
        auto plancePointText = _pt2->currentText();
        _pt2->clear();
        if (text == "X")
        {
            _pt2->addItems({ "X-Y", "X-Z" });
        }
        else if (text == "Y")
        {
            _pt2->addItems({ "X-Y", "Y-Z" });
        }
        else if (text == "Z" && _comboType->currentIndex() == 0)
        {
            _pt2->addItems({ "X-Z", "Y-Z" });
        }
        else if (text == "R")
        {
            _pt2->addItems({ QString("R-%1").arg(_theta), "R-Z" });
        }
        else if (text == _theta)
        {
            _pt2->addItems({ QString("R-%1").arg(_theta), QString("%1-Z").arg(_theta) });
        }
        else if (text == "Z")
        {
            _pt2->addItems({ "X-Z", QString("%1-Z").arg(_theta) });
        }
        if (!_coorData)
        {
            return;
        }
        _coorData->setMainIndex(_pt1->currentIndex());
        requestUpdate();
    }

    void mCoordinatePanel::slot_onPlanePointChanged(const QString& text)
    {

    }

    void mCoordinatePanel::connectSig()
    {
        disconnectSig();
        if (!getCVM->getPreRenderWidget())
        {
            return;
        }
        conn_geo = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mCoordinatePanel::slot_finishedPick);

        MechCommon::mMechGlobalSignals::getInstance()->activePickSig(0);

    }
    void mCoordinatePanel::disconnectSig()
    {
        if (conn_geo)
            disconnect(conn_geo);
        _lastSnapshot.first = false;
    }

    void mCoordinatePanel::requestUpdate()
    {
        if (!_coorData)
            return;

        CoordinateSnapshot now;

        bool success = false;
        QString msg = "";
        QVector3D origin;
        //坐标系方向矢量更新
        QVector<QVector3D> dirs = _coorData->getXYZAxis(success, msg, origin);

        if (!success)
        {
            return;
        }
        now.origin = origin;
        now.x = dirs.at(0);
        now.y = dirs.at(1);
        now.z = dirs.at(2);
        now.type = _coorData->getCoordinateType();
        now.method = _coorData->getMethod();
        now.coordinateId = _coorData->getCoordinateId();
        now.modelId = getCVM ? getCVM->getModelData()->getModelDataID() : -1;

        if (now == _lastSnapshot)
            return;   //第二次直接return

        _lastSnapshot = now;

        if (!_updateTimer)
        {
            _updateTimer = new QTimer(this);
            _updateTimer->setSingleShot(true);
            connect(_updateTimer, &QTimer::timeout,
                this, &mCoordinatePanel::updateCoordinateData);
        }
        _updateTimer->start(0);
    }

    void mCoordinatePanel::updateCoordinateData()
    {
        if (!_coorData)
        {
            return;
        }
        bool success = false;
        QString msg = "";
        QVector3D origin;
        //坐标系方向矢量更新
        QVector<QVector3D> dirs = _coorData->getXYZAxis(success, msg, origin);
        if (_coorData->getCoordinateId() != -1 || msg.isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(msg, true);
        }
        if (!success)
        {
            return;
        }
        _dirX->setText(QString("(%1)").arg(mContainerOperation::transformVector3DToString(dirs.at(0))));
        _dirY->setText(QString("(%1)").arg(mContainerOperation::transformVector3DToString(dirs.at(1))));
        _dirZ->setText(QString("(%1)").arg(mContainerOperation::transformVector3DToString(dirs.at(2))));

        //更新渲染
        int id = _coorData->getCoordinateId();
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        const auto& arrow3 = symbolController->getOrCreateSymbolData<Arrow_Font_Axis_Implicit>(QString("coordinate_%1").arg(_coorData->getCoordinateId()));
        arrow3->_color_x = { 255,0,0 };
        arrow3->_color_y = { 0,255,0 };
        arrow3->_color_z = { 0,0,255 };
        arrow3->_point_origin = origin;
        arrow3->_depthState = false;
        arrow3->_vec_x = dirs[0];
        arrow3->_vec_xy = dirs[1];
        arrow3->_type = LocalAxisType(_comboType->currentIndex());
        arrow3->_showState = true;
        symbolController->createSymbol(arrow3);
        symbolController->UpdateAllSymbol();
        clearPick();
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateCoordinateSig(false, _coorData->getCoordinateId());

        getCVM->getPreRenderWidget()->getCommonShapeRender()->appendPoint("oPoint", { _coorData->getOrigin() }, QVector3D(0, 204 / 255.0, 184 / 255.0));
        if (_comboDefine->currentText() == "三点")
        {
            getCVM->getPreRenderWidget()->getCommonShapeRender()->appendPoint("xPoint", { _coorData->getXPoint() }, QVector3D(54, 79, 107) / 255.0);
            getCVM->getPreRenderWidget()->getCommonShapeRender()->appendPoint("yPoint", { _coorData->getXYPoint() }, QVector3D(252, 81, 133) / 255.0);
        }
        else
        {
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("xPoint");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("yPoint");
        }

        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mCoordinatePanel::clearPick()
    {
        if (!getCVM)
        {
            return;
        }
        (getCVM->getPreRenderWidget());
        ClearPreWidgetMeshPick(getCVM->getPreRenderWidget());
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();
    }

    QVector3D mCoordinatePanel::getVec3DByLineEdits(QVector<mCalcLineEdit*> edits)
    {
        if (edits.size() != 3)
        {
            return QVector3D();
        }

        return QVector3D(edits[0]->text().toFloat(), edits[1]->text().toFloat(), edits[2]->text().toFloat());
    }

    void mCoordinatePanel::slot_finishedPick()
    {
        if (!getCVM)
        {
            return;
        }
    }

    void mCoordinatePanel::slot_subWindowActivated(int viewportID)
    {
        Q_UNUSED(viewportID);
    }

    //统一激活管理
    void mCoordinatePanel::activateFirstVisiblePicker()
    {
        QList<mPointPickerWidget*> allPickers = { _originPicker, _pt1Picker, _pt2Picker };

        //失活所有不可见/不可用的拾取控件（避免隐藏控件占用拾取状态）
        for (auto* picker : allPickers)
        {
            if (!picker || !picker->pickController())
                continue;
            bool shouldDeactivate = !picker->isVisible() || !picker->isEnabled();
            if (shouldDeactivate)
            {
                picker->pickController()->setPickActive(false);
                picker->clearAllHighlight();
            }
        }

        // 按顺序找到第一个可见且可用的拾取控件并激活
        // 拾取控件内部已实现全局互斥，激活新的会自动失活其他已激活的
        for (auto* picker : allPickers)
        {
            if (!picker || !picker->pickController())
                continue;
            if (picker->isVisible() && picker->isEnabled())
            {
                picker->pickController()->setPickActive(true);
                break;
            }
        }

        // 方向控件不可见/不可用时同步失活其内部拾取（激活由方向控件按模式自管理，此处只兜底）
        for (auto* selector : { _mainDirSelector, _dirDirSelector })
        {
            if (selector && (!selector->isVisible() || !selector->isEnabled()))
            {
                deactivateDirectionSelector(selector);
            }
        }
    }
}