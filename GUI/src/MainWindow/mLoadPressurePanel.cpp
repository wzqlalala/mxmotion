#include "mMechGlobalSignals.h"
#include "mDataStatusManager.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mSetManager.h"
#include "mMapRelationEnum.h"
#include "mMapRelationManager.h"
#include "mUnitSystemManager.h"
#include "mInfoWindow.h"
#include "PickCommonFunction.h"
#include "mWorkingConditions.h"
#include "mDockLayoutManager.h"
#include "mAppConfiguration.h"
#include "mGetSetFunction.h"
#include "mGeneralPickUp.h"
#include "mGetSetTypeTemplate.h"
#include "AllRenderSymbolManager.h"
#include "mDirectionSelectorWidget.h"

//Geo
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（拉 mOCCDocument→OCC 链；冻结岛保档）

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/view/CCSMPreRender.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/data/MeshElementExtract.h"
#include "mxr/label/mSymbolController.h"
#include "mxr/view/mShapeRender.h"

//Basic
#include<mMixSet.h>
#include<mMeshData.h>
#include<mGeoSet.h>
#include<mNode.h>
#include<mElementSurface.h>
#include<mElementSet.h>

#include "mLoadPressurePanel.h"
#include <QToolTip>
#include <QRegularExpression>
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;

namespace MechMainWindow
{
    mLoadPressurePanel::mLoadPressurePanel(QWidget* parent)
        :mLoadPanelBase(parent)
    {
        initInputs();
    }

    mLoadPressurePanel::~mLoadPressurePanel()
    {
        clearInputs();
    }

    void mLoadPressurePanel::connectUnitSystemResponse()
    {
        connect(getCVM->getModelData()->getUnitSystemManager(), &mUnitSystemManager::sg_unitChanged, this, &mLoadPressurePanel::slot_unitSystemResponse, Qt::UniqueConnection);
    }

    void mLoadPressurePanel::disconnectUnitSystemResponse()
    {
        disconnect(getCVM->getModelData()->getUnitSystemManager(), &mUnitSystemManager::sg_unitChanged, this, &mLoadPressurePanel::slot_unitSystemResponse);
    }

    void mLoadPressurePanel::slot_unitSystemResponse(UnitType type, QVector<int> idx)
    {
        Q_UNUSED(idx);
        if (!(type & UnitSystem)) return;
        if (_loadData)
            setInputsValue(_loadData);   //从载荷对象(基准值)重渲染，值+单位标签一次到位
    }

    void mLoadPressurePanel::initInputs()
    {
        // 1. 压强类型下拉框
        _pressureTypeCombo.initWidget("压强类型");
        _pressureTypeCombo.combobox->addItem("常规压力");
        _pressureTypeCombo.combobox->addItem("静水压力");
        _pressureTypeCombo.combobox->setCurrentIndex(0);
        connect(_pressureTypeCombo.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadPressurePanel::slot_pressureTypeChanged);

        // 2. 定义方式下拉框
        _defineTypeCombo.initWidget("定义方式");
        _defineTypeCombo.combobox->addItem("统一赋值");
        _defineTypeCombo.combobox->addItem("独立赋值");
        _defineTypeCombo.combobox->addItem("文件赋值");
        _defineTypeCombo.combobox->addItem("分析场");
        _defineTypeCombo.combobox->setCurrentIndex(0);
        connect(_defineTypeCombo.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadPressurePanel::slot_defineTypeChanged);

        // 2.1 分析场方式：已有分析场下拉框（无新建按钮，仅选择已有分析场）
        _analysisFieldCombo.initWidget("分析场");
        _analysisFieldCombo.combobox->setFixedHeight(30);
        _analysisFieldCombo.combobox->addItem("");
        _analysisFieldTable[""] = -1;
        auto allAnalysisField = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field).getAllFieldID(MxFunctions::FieldType::Analysis);
        for (auto fieldid : allAnalysisField)
        {
            QString fieldname = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field).getFieldNameByID(fieldid);
            _analysisFieldCombo.combobox->addItem(fieldname);
            _analysisFieldTable[fieldname] = fieldid;
        }
        connect(_analysisFieldCombo.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadPressurePanel::slot_analysisFieldChanged);

        // 5. 压强输入框（带单位）
        _pressureUnits = QStringList{
            "Pa",                // 1  m-kg-N-s
            "dyne/cm²",          // 2  cm-g-dyne-s
            "MPa",               // 3  mm-kg-N-s
            "MPa",               // 4  mm-t-N-s
            "MPa",               // 5  mm-dat-N-s
            "kPa",               // 6  vm-kg-vN-s
            "lbf/ft²",           // 7  ft-lbm-lbf-s
            "psi"                // 8  in-lbm-lbf-s
        };
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit = _pressureUnits[sysunitidxs[0]];

        InputWidgetSet paInput;
        paInput.initWidget("压强", false);
        paInput.lineeditvalue->setUnit(unit);
        inputWidgets.append(paInput);
        connect(inputWidgets[0].lineeditvalue, &mCalcLineEdit::editingFinished,
            [this]() {
                updateLoadSymbolRealtime(); // 压强值符号变化实时刷新箭头方向
                savePressure();
            });

        // 6. 净水压力输入
        // 深度方向选择控件（延迟创建，首次进入净水压力模式时创建）
        _directionWidget = nullptr;

        _zeroDepthInput.initWidget("零压强深度", false);
        connect(_zeroDepthInput.lineeditvalue, &mCalcLineEdit::editingFinished,
            [this]() {
                saveDepthInfo();
            });

        _refDepthInput.initWidget("参考深度", false);
        connect(_refDepthInput.lineeditvalue, &mCalcLineEdit::editingFinished,
            [this]() {
                saveDepthInfo();
            });

        _refPressureInput.initWidget("参考压强值", false);
        _refPressureInput.lineeditvalue->setUnit(unit);
        connect(_refPressureInput.lineeditvalue, &mCalcLineEdit::editingFinished,
            [this]() {
                updateLoadSymbolRealtime(); // 参考压强值符号变化实时刷新箭头方向
                saveDepthInfo();
            });

        // 7. 文件选择行
        _fileRowWidget = new QWidget();
        QHBoxLayout* fileRowLayout = new QHBoxLayout(_fileRowWidget);
        fileRowLayout->setContentsMargins(0, 0, 0, 0);
        fileRowLayout->setSpacing(8);

        _fileLabel = new QLabel("文件");
        _fileLabel->setMinimumWidth(87);
        fileRowLayout->addWidget(_fileLabel);

        _filePathEdit = new QLineEdit();
        _filePathEdit->setReadOnly(true);
        _filePathEdit->setPlaceholderText("请选择文件...");
        fileRowLayout->addWidget(_filePathEdit);

        _fileBrowseBtn = new QPushButton("选择文件");
        connect(_fileBrowseBtn, &QPushButton::clicked, this, &mLoadPressurePanel::slot_browseFileClicked);
        fileRowLayout->addWidget(_fileBrowseBtn);

        // 8. 独立赋值表格行：通用离散数据控件（入口按钮 + 锚定弹窗表格）
        _tableRowWidget = new QWidget();
        QHBoxLayout* tableLayout = new QHBoxLayout(_tableRowWidget);
        tableLayout->setContentsMargins(0, 0, 0, 0);
        tableLayout->setSpacing(8);

        _tableLabel = new QLabel("单元面数据");
        _tableLabel->setMinimumWidth(87);
        tableLayout->addWidget(_tableLabel);

        mDiscreteDataControl::PickSpec faceSpec;
        faceSpec.typeName = QStringLiteral("单元面");
        faceSpec.idHeaders = QStringList{ QStringLiteral("单元编号"), QStringLiteral("面编号") };
        // 业务 key = (单元ID, 面序号)：part_id 不参与行 key，
        // 面板侧在重建/反显 map 时按 cellFaceMap_ 原有完整 key 回填 part_id
        faceSpec.keyOf = [](const RenderPickResultData& d) { return qMakePair(d.result_id, d.result_id2); };
        faceSpec.pickTypes = { MxFunctions::PickDataType::ElementFace };
        faceSpec.pickFilter = 7; // 视口拾取过滤器：仅单元面（与 activePickSig 索引一致）
        // 拾取由离散数据控件直接驱动视口选择容器，不再经过通用拾取控件
        _discreteControl = new mDiscreteDataControl(
            QStringLiteral("单元面数据"),
            { QStringLiteral("压强值") },
            &faceSpec, _tableRowWidget);
        // 压力面板需要渲染云图功能：显示渲染按钮，点击切换云图开关
        _discreteControl->setRenderButtonVisible(true, QStringLiteral("云图"));
        _discreteControl->setRenderHandler([this]() {
            _cloudOn = !_cloudOn;
            if (_cloudOn)
            {
                // 打开云图前先清空视口残留的拾取选中态/高亮（含通用拾取控件写入
                // 选择容器的数据），否则首次点击云图时旧拾取高亮会遮挡云图
                _discreteControl->clearPickingSelection();
            }
            setEleLoadCloudChart(_cloudOn);
            if (!_cloudOn)
            {
                // 关闭云图后恢复离散表格的拾取高亮（开启云图时已被清空）：
                // 云图按钮在弹窗内，此刻弹窗必然打开，回种即恢复"表格所见即高亮"
                _discreteControl->seedSelectionFromRows();
            }
            });
        connect(_discreteControl, &mDiscreteDataControl::dataChanged,
            this, &mLoadPressurePanel::slot_discreteDataChanged);
        connect(_discreteControl, &mDiscreteDataControl::cellEdited,
            this, &mLoadPressurePanel::slot_discreteCellEdited);
        // 入口按钮从控件中剖分出来直接放入行布局，控件本体隐藏（弹窗为
        // 宿主主窗口子窗口不受影响），按钮可用状态与控件本体显隐/禁用解耦
        if (QPushButton* entry = _discreteControl->entryButton())
        {
            _discreteControl->detachEntryButton(_tableRowWidget);
            tableLayout->addWidget(entry, 1);
            _discreteControl->hide();
        }
        else
        {
            tableLayout->addWidget(_discreteControl);
        }

        // 9. 通用拾取控件
        _pickWidgetController = mGeneralPickUp::createPickWidget(CBBlue);
        connect(_pickWidgetController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive)
            {
                emit mMechGlobalSignals::getInstance()->activePickSig(7);
            }
            });
        connect(_pickWidgetController, &mPickWidgetController::dataChanged, this, [this]() {
            // 独立赋值模式下通用拾取控制器不再接收拾取（由离散数据控件直驱视口），
            // 其 dataChanged 只在统一赋值/净水压力模式下触发，无需按方式分支重建 map。
            // 拾取/集合变化时实时刷新压力符号（统一赋值、净水压力均需）
            updateLoadSymbolRealtime();

            //判断拾取控件是否为空 以设置标志位
            if (_pickWidgetController->getAllData().isEmpty())
            {
                _currentLoad->setPickState(1);
            }
            else
            {
                _currentLoad->setPickState(2);
            }
            //发送更新信号
            notifyLoadChanged();
            });
        _pickWidgetController->setSetResolver(&resolveSetToTypedData);
        _pickWidgetController->setDefultTip("选择对象");

        // 10. 函数
        _functable.clear();
        combofunc.initWidget("函数");
        combofunc.pushbtn->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/hanshu.png"));
        combofunc.pushbtn->setFixedSize(30, 30);
        combofunc.pushbtn->setIconSize(QSize(18, 18));
        combofunc.combobox->setFixedHeight(30);
        auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
        _functable[""] = -1;
        combofunc.combobox->addItem("");
        for (auto funcid : allFunc)
        {
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
            combofunc.combobox->addItem(funcname);
            _functable[funcname] = funcid;
        }
        connect(combofunc.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadPressurePanel::slot_funcChanged);
        connect(combofunc.pushbtn, &QPushButton::clicked, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createFunction(2);

            combofunc.combobox->clear();
            _functable.clear();
            auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
            _functable[""] = -1;
            combofunc.combobox->addItem("");
            for (auto funcid : allFunc)
            {
                QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
                combofunc.combobox->addItem(funcname);
                _functable[funcname] = funcid;
            }
            combofunc.combobox->setCurrentIndex(allFunc.size() > 0 ? allFunc.size() : 0);
            saveFunc();
            });

        // 11. 缩放因子
        _scale.initWidget("缩放因子", false);
        _scale.labelname->setText("缩放因子");
        connect(_scale.lineeditvalue, &mCalcLineEdit::editingFinished, [this]() { saveScale(); });
    }

    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mLoadPressurePanel::getPickedData()
    {
        if (!_pickWidgetController)
            return std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>();
        return _pickWidgetController->getAllData().pickedData;
    }

    void mLoadPressurePanel::slot_pressureTypeChanged(int idx)
    {
        saveModeState();

        applyAllowedPickTypes(idx, _defineTypeCombo.combobox->currentIndex());

        // 通用拾取控件可见性随模式同步：常规压力的独立赋值/文件/分析场模式下隐藏
        // （独立赋值由离散数据控件直驱拾取，分析场无作用对象），其余模式恢复可见/激活与正常高亮
        const int defineType = _defineTypeCombo.combobox->currentIndex();
        if (_pickWidgetController)
        {
            if (idx == 0 && (defineType == 1 || defineType == 2 || defineType == 3))
            {
                _pickWidgetController->setPickWidgetVisible(false);
                if (defineType == 1)
                {
                    filterPicksToCellFaceOnly();
                    _pickWidgetController->setCloudChartMode(true);
                }
            }
            else
            {
                _pickWidgetController->setPickWidgetVisible(true);
                _pickWidgetController->setCloudChartMode(false);
                _pickWidgetController->setPickActive(true);
            }
        }
        rebuildValueLayout();
        // 压强类型切换后立即按新模式刷新符号（如切到静水压力按方向控件的深度方向渲染），
        // 否则残留上一模式的箭头，直到方向控件再次变化才刷新。
        updateLoadSymbolRealtime();
        emit sg_rebuildPanel();
    }

    void mLoadPressurePanel::slot_defineTypeChanged(int idx)
    {
        saveModeState();

        // 切换定义方式时关闭离散数据弹窗并结束拾取，避免旧模式弹窗残留
        if (_discreteControl)
        {
            _discreteControl->closePopup();
        }

        // 独立赋值：过滤拾取为仅单元面，并彻底关闭通用拾取控件的高亮渲染
        // （拾取由离散数据控件直接驱动视口选择容器）
        if (idx == 1)
        {
            filterPicksToCellFaceOnly();
            if (_pickWidgetController)
            {
                _pickWidgetController->setPickWidgetVisible(false);
                _pickWidgetController->setCloudChartMode(true);
            }
        }
        else if (idx == 2)
        {
            // 切换到文件赋值：清空所有拾取、高亮和渲染
            if (_pickWidgetController)
            {
                _pickWidgetController->setPickWidgetVisible(false);
                _pickWidgetController->clearAllHighlight();
                PickWidgetData empty;
                _pickWidgetController->replacePickedDataAndClearSets(empty.pickedData);
            }
        }
        else if (idx == 3)
        {
            // 分析场：无作用对象，隐藏拾取控件并清空拾取/高亮
            if (_pickWidgetController)
            {
                _pickWidgetController->setPickWidgetVisible(false);
                _pickWidgetController->setCloudChartMode(true);
                _pickWidgetController->clearAllHighlight();
                PickWidgetData empty;
                _pickWidgetController->replacePickedDataAndClearSets(empty.pickedData);
            }
        }
        else
        {
            // 统一赋值：恢复通用拾取控件可见/激活与正常高亮渲染
            if (_pickWidgetController)
            {
                _pickWidgetController->setPickWidgetVisible(true);
                _pickWidgetController->setCloudChartMode(false);
                _pickWidgetController->setPickActive(true);
            }
        }

        applyAllowedPickTypes(_pressureTypeCombo.combobox->currentIndex(), idx);
        rebuildValueLayout();
        // 定义方式切换后立即刷新符号（统一/独立按各自取值渲染，文件赋值清空箭头）
        updateLoadSymbolRealtime();
        emit sg_rebuildPanel();
    }

    void mLoadPressurePanel::ensureDirectionWidget()
    {
        if (_directionWidget)
            return;

        if (!faval_)
            return;

        _directionWidget = new mDirectionSelectorWidget(
            QStringLiteral("深度方向"), false, QString(), false, QString(), false, faval_ ? faval_->contentWidget() : nullptr);
        _directionWidget->setVisible(false);

        // 使用控件默认的方向/坐标计算器
        _directionWidget->setDirectionCalculator(
            mDirectionSelectorWidget::DirectionCalculator());
        _directionWidget->setCoordinateCalculator(
            mDirectionSelectorWidget::CoordinateCalculator());

        connect(_directionWidget, &mDirectionSelectorWidget::directionChanged, this, [this]() {
            updateLoadSymbolRealtime();
            saveDepthInfo();
            });
        connect(_directionWidget, &mDirectionSelectorWidget::layoutRefreshRequested, this, [this]() {
            if (faval_)
            {
                faval_->refreshLayout();
            }
            });
        connect(_directionWidget, &mDirectionSelectorWidget::currentWayNoPickWidget, this, [this]() {
            // 方向控件当前方式页无拾取控件时，激活压强面板中外层拾取控件
            if (_pickWidgetController)
            {
                _pickWidgetController->setPickActive(true);
            }
            });
    }

    void mLoadPressurePanel::rebuildValueLayout()
    {
        if (!faval_)
            return;

        int pressureType = _pressureTypeCombo.combobox->currentIndex();
        int defineType = _defineTypeCombo.combobox->currentIndex();

        faval_->removeAll();
        faval_->addRow(_pressureTypeCombo.propwid);
        _pressureTypeCombo.propwid->show();

        if (pressureType == 0)
        {
            // 常规压力
            faval_->addRow(_defineTypeCombo.propwid);
            _defineTypeCombo.propwid->show();

            if (defineType == 0)
            {
                // 统一赋值
                faval_->addRow(inputWidgets[0].propwid);
                inputWidgets[0].propwid->show();
                faval_->addRow(combofunc.propwid);
                combofunc.propwid->show();
            }
            else if (defineType == 1)
            {
                // 独立赋值（云图开关在离散数据弹窗内，不再单独占行）
                faval_->addRow(_tableRowWidget);
                _tableRowWidget->show();
                faval_->addRow(combofunc.propwid);
                combofunc.propwid->show();
            }
            else if (defineType == 2)
            {
                // 文件赋值
                faval_->addRow(_fileRowWidget);
                _fileRowWidget->show();
                faval_->addRow(combofunc.propwid);
                combofunc.propwid->show();
            }
            else if (defineType == 3)
            {
                // 分析场：分析场下拉框 + 函数（无作用对象、无大小）
                faval_->addRow(_analysisFieldCombo.propwid);
                _analysisFieldCombo.propwid->show();
                faval_->addRow(combofunc.propwid);
                combofunc.propwid->show();
            }
        }
        else
        {
            // 净水压力
            ensureDirectionWidget();
            if (_directionWidget)
            {
                _directionWidget->hide();
                faval_->addRow(_directionWidget);
                _directionWidget->show();
                _directionWidget->setPickWidgetVisible(true);
            }
            faval_->addRow(_zeroDepthInput.propwid);
            _zeroDepthInput.propwid->show();
            faval_->addRow(_refDepthInput.propwid);
            _refDepthInput.propwid->show();
            faval_->addRow(_refPressureInput.propwid);
            _refPressureInput.propwid->show();
            faval_->addRow(combofunc.propwid);
            combofunc.propwid->show();
        }

        // 离开独立赋值（常规压力）模式时，若云图仍开启则关闭
        if (!(pressureType == 0 && defineType == 1) && _cloudOn)
        {
            _cloudOn = false;
            setEleLoadCloudChart(false);
        }

        faval_->refreshLayout();

        // 独立赋值/文件/分析场模式：隐藏集合选择按钮，强制仅单元面
        if (pressureType == 0 && (defineType == 1 || defineType == 2 || defineType == 3))
        {
            if (_pickWidgetController)
            {
                _pickWidgetController->setSelectSetBtnVisible(false);
            }
        }
        else
        {
            if (_pickWidgetController)
            {
                _pickWidgetController->setSelectSetBtnVisible(true);
            }
        }
    }

    void mLoadPressurePanel::filterPicksToCellFaceOnly()
    {
        // 同步通用拾取控件最新数据到 picks，避免过滤时丢失拾取/集合
        auto picks = _pickWidgetController->getAllData().pickedData;

        auto it = picks.begin();
        while (it != picks.end())
        {
            if (it->first != MxFunctions::PickDataType::ElementFace)
                it = picks.erase(it);
            else
                ++it;
        }

        if (_pickWidgetController)
        {
            _pickWidgetController->replacePickedDataAndClearSets(picks);
        }

        updateOpenedTable();
    }

    void mLoadPressurePanel::setEleLoadCloudChart(bool flag)
    {
        if (getCVM->getPreRenderWidget() && getCVM->getPreRenderWidget()->getPreRender())
        {
            // cellFaceMap_: key=单元面拾取数据 value=压强值（QVector<QVariant>，压强值在第0列），转换为渲染侧需要的(压强值, 拾取数据)列表
            // 关闭云图时按接口约定传空数据（渲染侧 flag=false 时会清空全部载荷云图数据）
            std::vector<std::pair<double, RenderPickResultData>> meshIDs;
            if (flag && _currentLoad)
            {
                meshIDs.reserve(_currentLoad->cellFaceMap_.size());
                for (const auto& entry : _currentLoad->cellFaceMap_)
                {
                    const QVariant v = entry.second.value(0);
                    if (!v.isValid() || v.isNull())
                        continue;
                    meshIDs.emplace_back(v.toDouble(), entry.first);
                }
            }
            getCVM->getPreRenderWidget()->getPreRender()->enableMeshEleLoadCloudChart(flag, mxr::pickRefsOf(meshIDs));
            // 统一赋值下进入/退出云图状态时切换拾取控件云图模式（清空并抑制拾取高亮，数据保留 / 恢复高亮）。
            // 独立赋值（常规压力）下通用拾取控件常驻抑制态，云图开关不改变其模式，
            // 否则退出云图时会按控件模型里反显的离散数据画失活临时高亮（蓝色残留）
            if (_pickWidgetController)
            {
                const bool discreteMode = (_currentLoad && _currentLoad->pressureType_ == 0 && _currentLoad->way_ == 1);
                _pickWidgetController->setCloudChartMode(discreteMode ? true : flag);
            }
            // 显示云图时隐藏该载荷的符号渲染，关闭云图时恢复符号
            if (_currentLoad)
            {
                auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
                if (symbolController)
                {
                    std::vector<QString> symbolNames = { QString("load_%1").arg(_currentLoad->getLoadBoundaryId()) };
                    symbolController->setSymbolShowState(symbolNames, !flag);
                    symbolController->UpdateAllSymbol();
                }
            }
            getCVM->getPreRenderWidget()->updatePaint();
        }
    }

    void mLoadPressurePanel::applyAllowedPickTypes(int pressureType, int defineType)
    {
        if (!_pickWidgetController) return;

        if (pressureType == 1)
        {
            // 净水压力：可拾取几何面 + 单元面
            _pickWidgetController->setAllowedTypes(&pickGeoFaceAndElementSurface);
            _pickWidgetController->setPickType();
            return;
        }

        if (defineType == 2 || defineType == 3)
        {
            // 文件赋值/分析场：禁用全部拾取
            _pickWidgetController->setAllPickTypesDisabled(true);
        }
        else if (defineType == 1)
        {
            // 独立赋值：仅设置白名单记录，不切换视口过滤器
            // （拾取过滤器由离散数据控件在开始拾取时自行激活/恢复）
            _pickWidgetController->setAllowedTypes(&pickOnlyElementSurface);
        }
        else
        {
            // 统一赋值：几何面 + 单元面
            _pickWidgetController->setAllowedTypes(&pickGeoFaceAndElementSurface);
            _pickWidgetController->setPickType();
        }
    }

    void mLoadPressurePanel::updateOpenedTable()
    {
        if (!_currentLoad || !_discreteControl)
            return;

        // 独立赋值模式下，cellFaceMap_ 与离散数据控件的行数据保持严格一致：
        // 表格是数据的权威来源（含拾取追加与手动删除），此处按控件当前行重建 map。
        // 行 key 为业务 key（单元ID, 面序号），重建时优先保留 cellFaceMap_ 中原有
        // 完整 key（含 part_id）与已填值；新行按单元 ID 反查 part_id 构造完整 key，
        // 与 mIMxFile 读入键规范 (cellId, partId, faceId) 保持一致。
        const auto rows = _discreteControl->rows();

        std::map<QPair<int, int>, RenderPickResultData> existingKeys;
        std::map<QPair<int, int>, QVector<QVariant>> existingValues;
        for (const auto& entry : _currentLoad->cellFaceMap_)
        {
            QPair<int, int> bizKey(entry.first.result_id, entry.first.result_id2);
            if (existingKeys.find(bizKey) == existingKeys.end())
            {
                existingKeys[bizKey] = entry.first;
                existingValues[bizKey] = entry.second;
            }
        }

        auto meshData = getCVM->getModelData()->getMeshData();
        QHash<int, int> partIdByCellId;
        std::map<RenderPickResultData, QVector<QVariant>> newMap;
        for (const auto& row : rows)
        {
            const int cellId = row.first.first;
            const int faceId = row.first.second;
            if (cellId <= 0 || faceId <= 0)
                continue;

            const QPair<int, int> bizKey(cellId, faceId);
            auto keyIt = existingKeys.find(bizKey);
            if (keyIt != existingKeys.end())
            {
                // 保留原有完整 key（含 part_id）与已填值（含空值占位）
                newMap[keyIt->second] = existingValues[bizKey];
                continue;
            }

            // 新增行：按单元 ID 反查 part_id 构造完整 key，空值占位
            auto partIt = partIdByCellId.constFind(cellId);
            if (partIt == partIdByCellId.constEnd())
            {
                const int partId = meshData ? meshData->getPartIdByEleId(cellId) : -1;
                partIt = partIdByCellId.insert(cellId, partId);
            }
            newMap[RenderPickResultData(cellId, partIt.value(), faceId)] = { QVariant() };
        }

        _currentLoad->cellFaceMap_ = std::move(newMap);
    }

    void mLoadPressurePanel::slot_discreteDataChanged()
    {
        // 离散数据控件行变化（拾取追加/删除/清空）：
        // 按控件当前行重建 cellFaceMap_，随后刷新实时符号并通知外部。
        // 拾取会话进行中跳过：确认拾取（完成拾取）后控件会补发一次 dataChanged，
        // 此时才执行重建/渲染/云图刷新，避免拾取界面期间反复重绘。
        if (!_currentLoad || (_discreteControl && _discreteControl->isPicking()))
            return;
        updateOpenedTable();
        updateLoadSymbolRealtime();
        // 云图处于开启状态时，表格数据变化后同步刷新云图
        if (_cloudOn)
        {
            setEleLoadCloudChart(true);
        }
        notifyLoadChanged();
    }

    void mLoadPressurePanel::slot_discreteCellEdited(const QPair<int, int>& key, int valueIndex, const QString& text)
    {
        if (!_currentLoad || key.first <= 0)
            return;

        // 行 key 只有（单元ID, 面序号）两个槽位，按业务 key 在 cellFaceMap_ 中查找
        // 已有完整 key（保留原 part_id）；不存在时按单元 ID 反查 part_id 构造完整 key，
        // 与 mIMxFile 读入键规范 (cellId, partId, faceId) 保持一致。
        auto it = _currentLoad->cellFaceMap_.begin();
        for (; it != _currentLoad->cellFaceMap_.end(); ++it)
        {
            if (it->first.result_id == key.first && it->first.result_id2 == key.second)
                break;
        }

        RenderPickResultData loadKey;
        if (it != _currentLoad->cellFaceMap_.end())
        {
            loadKey = it->first;
        }
        else
        {
            auto meshData = getCVM->getModelData()->getMeshData();
            const int partId = meshData ? meshData->getPartIdByEleId(key.first) : -1;
            loadKey = RenderPickResultData(key.first, partId, key.second);
        }

        bool ok = false;
        const double v = text.toDouble(&ok);
        const QVariant value = (ok && !text.isEmpty()) ? QVariant(v) : QVariant();
        // 清空压强值时保留 key，值置空 QVariant（行仍被记录，写文件时过滤空值）
        QVector<QVariant>& values = _currentLoad->cellFaceMap_[loadKey];
        if (values.isEmpty())
            values.resize(1);
        if (valueIndex == 0)
            values[0] = value;

        updateLoadSymbolRealtime();
        // 云图处于开启状态时，表格数值变化后同步刷新云图
        if (_cloudOn)
        {
            setEleLoadCloudChart(true);
        }
        notifyLoadChanged();
    }

    void mLoadPressurePanel::slot_browseFileClicked()
    {
        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
        QFileInfo info(defaultDir);
        if (defaultDir.isEmpty() || !info.exists())
        {
            defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
        }
        QString filePath = QFileDialog::getOpenFileName(
            nullptr, "选择载荷文件", defaultDir, "Text Files (*.txt);;All Files (*)");
        if (!filePath.isEmpty())
        {
            QFileInfo fi(filePath);
            QString fileName = fi.fileName();
            _filePathEdit->setText(fileName);
            _filePathEdit->setToolTip(filePath);
            if (_currentLoad)
            {
                _currentLoad->path_ = filePath;
                notifyLoadChanged();
            }
        }
    }

    void mLoadPressurePanel::slot_funcChanged(int idx)
    {
        Q_UNUSED(idx)
        saveFunc();
    }

    void mLoadPressurePanel::slot_analysisFieldChanged(int idx)
    {
        Q_UNUSED(idx)
        saveAnalysisField();
    }

    void mLoadPressurePanel::unEditInputs()
    {
        // 退出压强界面时删除云图显示
        if (_cloudOn)
        {
            _cloudOn = false;
            setEleLoadCloudChart(false);
        }
    }

    void mLoadPressurePanel::clearInputs()
    {
        for (auto inp : inputWidgets)
        {
            inp.clear();
        }
        inputWidgets.clear();

        if (_pickWidgetController)
        {
            _pickWidgetController->setPickActive(false);
            delete _pickWidgetController;
            _pickWidgetController = nullptr;
        }

        _currentLoad.reset();

        _pressureTypeCombo.clear();
        _defineTypeCombo.clear();
        _analysisFieldCombo.clear();
        _analysisFieldTable.clear();

        if (_directionWidget)
        {
            _directionWidget->hide();
            delete _directionWidget;
            _directionWidget = nullptr;
        }

        _zeroDepthInput.clear();
        _refDepthInput.clear();
        _refPressureInput.clear();

        if (_tableRowWidget)
        {
            delete _tableRowWidget;
            _tableRowWidget = nullptr;
        }
        _tableLabel = nullptr;
        _discreteControl = nullptr; // 随 _tableRowWidget 一并删除
        _cloudOn = false;

        if (_fileRowWidget)
        {
            delete _fileRowWidget;
            _fileRowWidget = nullptr;
        }
        _fileLabel = nullptr;
        _filePathEdit = nullptr;
        _fileBrowseBtn = nullptr;

        combofunc.clear();
        _scale.clear();
        _functable.clear();
    }

    void mLoadPressurePanel::setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase>loadData)
    {
        // 设置系统单位
        QVector<int> unitto = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double pressureRatio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, { 3,0,0,0 }, unitto);
        QString unit = _pressureUnits[unitto[0]];

        _currentLoad = std::dynamic_pointer_cast<MechCaseData::mLoadPressure>(loadData);
        if (!_currentLoad)
            return;

        // 压强类型
        _pressureTypeCombo.combobox->blockSignals(true);
        _pressureTypeCombo.combobox->setCurrentIndex(_currentLoad->pressureType_);
        _pressureTypeCombo.combobox->blockSignals(false);

        // 定义方式（分析场方式反显为组合框第3项，way_ 保持统一赋值语义）
        _defineTypeCombo.combobox->blockSignals(true);
        _defineTypeCombo.combobox->setCurrentIndex(_currentLoad->isAnalysisFieldMode_ ? 3 : _currentLoad->way_);
        _defineTypeCombo.combobox->blockSignals(false);

        // 压强值
        inputWidgets[0].lineeditvalue->setExpression(_currentLoad->pa_.isNull() ? "" : MxFunctions::mNumberToString(_currentLoad->pa_.toDouble() * pressureRatio));
        inputWidgets[0].lineeditvalue->setUnit(unit);

        // 文件路径
        if (!_currentLoad->path_.isEmpty())
        {
            QFileInfo fi(_currentLoad->path_);
            _filePathEdit->setText(fi.fileName());
            _filePathEdit->setToolTip(_currentLoad->path_);
        }
        else
        {
            _filePathEdit->clear();
            _filePathEdit->setToolTip("");
        }

        // 净水压力
        if (_currentLoad->pressureType_ == 1)
        {
            ensureDirectionWidget();
            if (_directionWidget)
            {
                restoreDirectionWidgetFromLoad(_currentLoad);
            }
            _zeroDepthInput.lineeditvalue->setExpression(_currentLoad->zeroDepth_.isNull() ? "" : MxFunctions::mNumberToString(_currentLoad->zeroDepth_.toDouble()));
            _refDepthInput.lineeditvalue->setExpression(_currentLoad->refDepth_.isNull() ? "" : MxFunctions::mNumberToString(_currentLoad->refDepth_.toDouble()));
            _refPressureInput.lineeditvalue->setExpression(_currentLoad->refPressure_.isNull() ? "" : MxFunctions::mNumberToString(_currentLoad->refPressure_.toDouble() * pressureRatio));
            _refPressureInput.lineeditvalue->setUnit(unit);
        }

        // 函数下拉刷新
        combofunc.combobox->blockSignals(true);

        combofunc.combobox->clear();
        _functable.clear();
        _functable[""] = -1;
        combofunc.combobox->addItem("");
        auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
        for (auto funcid : allFunc)
        {
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
            combofunc.combobox->addItem(funcname);
            _functable[funcname] = funcid;
        }
        int funcid = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_Function);
        if (funcid != -1)
        {
            combofunc.combobox->setCurrentText(getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid));
        }
        combofunc.combobox->blockSignals(false);

        // 分析场下拉刷新与反显
        _analysisFieldCombo.combobox->blockSignals(true);
        _analysisFieldCombo.combobox->clear();
        _analysisFieldTable.clear();
        _analysisFieldCombo.combobox->addItem("");
        _analysisFieldTable[""] = -1;
        auto& analysisFieldMgr = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
        auto allAnalysisField = analysisFieldMgr.getAllFieldID(MxFunctions::FieldType::Analysis);
        for (auto fieldid : allAnalysisField)
        {
            QString fieldname = analysisFieldMgr.getFieldNameByID(fieldid);
            _analysisFieldCombo.combobox->addItem(fieldname);
            _analysisFieldTable[fieldname] = fieldid;
        }
        int analysisFieldId = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_LoadBoundary, loadData->getLoadBoundaryId() }, Physical_Field, MapRelationType::MapRelation_AnalysisField);
        if (analysisFieldId != -1)
        {
            _analysisFieldCombo.combobox->setCurrentText(analysisFieldMgr.getFieldNameByID(analysisFieldId));
        }
        _analysisFieldCombo.combobox->blockSignals(false);

        // 通用拾取控件持久化策略与集合列表
        if (_pickWidgetController)
        {
            auto strategy = std::make_shared<MapRelationPersistenceStrategy>(
                MxFunctions::Physical_LoadBoundary,
                _loadData->getLoadBoundaryId());
            _pickWidgetController->setPersistenceStrategy(strategy);
            _pickWidgetController->setSetResolver(&resolveSetToTypedData);

            QHash<QString, QString> setToType = getSetNamesList({ SetType::ElementSurface, SetType::GeoFace });
            _pickWidgetController->setSelectSetList(setToType);

            applyAllowedPickTypes(_currentLoad->pressureType_, _currentLoad->isAnalysisFieldMode_ ? 3 : _currentLoad->way_);
        }

        // 拾取数据反显
        // 单例面板复用：无论新载荷定义方式如何，先清空离散表格残留行，
        // 避免旧载荷独立赋值数据在用户切回独立赋值时复现并污染新载荷。
        // setRows 不发 dataChanged，不会反向污染新载荷，内部还会清掉视口残留选中态。
        if (_discreteControl)
        {
            _discreteControl->setRows({});
        }
        PickWidgetData displayData;
        // 独立赋值：直接从 cellFaceMap_ 生成拾取数据，不依赖集合映射
        if (_currentLoad->pressureType_ == 0 && _currentLoad->way_ == 1)
        {
            std::vector<RenderPickResultData> facePicks;
            for (const auto& entry : _currentLoad->cellFaceMap_)
            {
                facePicks.push_back(entry.first);
            }
            if (!facePicks.empty())
            {
                displayData.pickedData[MxFunctions::PickDataType::ElementFace] = std::move(facePicks);
            }

            // 反显到离散数据控件（单元面 + 已填压强值），行 key 为业务 key（单元ID, 面序号）
            if (_discreteControl)
            {
                QVector<mDiscreteDataControl::Row> rows;
                rows.reserve(static_cast<int>(_currentLoad->cellFaceMap_.size()));
                for (const auto& entry : _currentLoad->cellFaceMap_)
                {
                    QStringList values;
                    const QVariant v = !entry.second.isEmpty() ? entry.second.at(0) : QVariant();
                    values << ((v.isValid() && !v.isNull()) ? QString::number(v.toDouble()) : QString());
                    rows.append(qMakePair(qMakePair(entry.first.result_id, entry.first.result_id2), values));
                }
                _discreteControl->setRows(rows);
            }
        }
        else
        {
            int setid = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_Set);
            if (setid != -1)
            {
                mSetBase* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setid);
                if (set && set->getInternal())
                {
                    auto mixset = dynamic_cast<mMixSet*>(set);
                    if (mixset) displayData.pickedData = mixset->getMixSetData();
                }
                else if (set)
                {
                    displayData.selectedSetNames = { set->getSetName() };
                }
            }
        }

        if (_pickWidgetController)
        {
            if (_currentLoad->pressureType_ == 0 && _currentLoad->isAnalysisFieldMode_)
            {
                // 分析场：无作用对象，全程保持失活，不激活、不渲染任何拾取高亮
                _pickWidgetController->setPickWidgetVisible(false);
                _pickWidgetController->setCloudChartMode(true);
                _pickWidgetController->displayData(displayData);
            }
            else if (_currentLoad->pressureType_ == 0 && _currentLoad->way_ == 1)
            {
                // 独立赋值已剥离通用拾取：仅写入数据（供 pickState/持久化一致性），
                // 全程保持失活，不激活、不渲染任何拾取高亮
                _pickWidgetController->setPickWidgetVisible(false);
                _pickWidgetController->setCloudChartMode(true);
                _pickWidgetController->displayData(displayData);
            }
            else
            {
                // displayData 默认不重画高亮；若控件此前已处于激活态，setPickActive(true) 会直接返回，
                // 该情况下需显式重画一次高亮以匹配反显数据。整体保证进入面板只做一次全量高亮。
                // 旧载荷在独立赋值/文件模式下曾 setPickWidgetVisible(false)，
                // setPickActive 在隐藏态会被静默拦截，需先恢复可见性
                _pickWidgetController->setPickWidgetVisible(true);
                const bool wasActive = _pickWidgetController->isPickActive();
                _pickWidgetController->displayData(displayData);
                _pickWidgetController->setPickActive(true);
                if (wasActive)
                {
                    _pickWidgetController->refreshHighlight();
                }
            }
        }

        // 同步集合按钮显隐
        if (_pickWidgetController)
        {
            _pickWidgetController->setSelectSetBtnVisible(!(_currentLoad->pressureType_ == 0 && (_currentLoad->way_ == 1 || _currentLoad->way_ == 2 || _currentLoad->isAnalysisFieldMode_)));
        }

        // 缩放因子回填
        double scale = getCVM->getModelData()->getMapRelationManager().getScaleByCase({ MxFunctions::Physical_LoadBoundary,loadData->getLoadBoundaryId() }, _caseId);
        _scale.lineeditvalue->setExpression(MxFunctions::mNumberToString(scale));

        //判断拾取控件是否为空 以设置标志位
        if (_pickWidgetController->getAllData().isEmpty())
        {
            _currentLoad->setPickState(1);
        }
        else
        {
            _currentLoad->setPickState(2);
        }
        //发送更新信号
        notifyLoadChanged();
        // 延迟刷新符号：等折叠块布局完成后（控件尺寸/渲染上下文就绪）再渲染，
        // 避免 setInputsValue 阶段 _pickWidgetController 尚未显示导致符号位置计算失败。
        QMetaObject::invokeMethod(this, &mLoadPressurePanel::updateLoadSymbolRealtime, Qt::QueuedConnection);
    }

    QVector<QWidget*> mLoadPressurePanel::getRefStateInputs(FoldSection* fathertar)
    {
        refstate_ = fathertar;
        QVector<QWidget*>wds;
        // 分析场方式无大小概念，不需要缩放因子（折叠框整体隐藏）
        if (_currentLoad && _currentLoad->pressureType_ == 0 && _currentLoad->isAnalysisFieldMode_)
            return wds;
        wds.append(_scale.propwid);
        return wds;
    }

    QVector<QWidget*> mLoadPressurePanel::getTargetInputs(FoldSection* fathertar)
    {
        fatar_ = fathertar;
        QVector<QWidget*>wds;
        // 分析场方式无作用对象（折叠框整体隐藏）
        if (_currentLoad && _currentLoad->pressureType_ == 0 && _currentLoad->isAnalysisFieldMode_)
            return wds;
        if (_pickWidgetController)
        {
            wds.append(_pickWidgetController->widget());
        }
        return wds;
    }

    QVector<QWidget*> mLoadPressurePanel::getValueInputs(FoldSection* fatherval)
    {
        faval_ = fatherval;
        QVector<QWidget*>wds;

        int pressureType = _pressureTypeCombo.combobox->currentIndex();
        int defineType = _defineTypeCombo.combobox->currentIndex();

        wds.append(_pressureTypeCombo.propwid);

        if (pressureType == 0)
        {
            wds.append(_defineTypeCombo.propwid);
            if (defineType == 0)
            {
                wds.append(inputWidgets[0].propwid);
                wds.append(combofunc.propwid);
            }
            else if (defineType == 1)
            {
                wds.append(_tableRowWidget);
                wds.append(combofunc.propwid);
            }
            else if (defineType == 2)
            {
                wds.append(_fileRowWidget);
                wds.append(combofunc.propwid);
            }
            else if (defineType == 3)
            {
                // 分析场：分析场下拉框 + 函数
                wds.append(_analysisFieldCombo.propwid);
                wds.append(combofunc.propwid);
            }
        }
        else
        {
            ensureDirectionWidget();
            if (_directionWidget)
            {
                wds.append(_directionWidget);
            }
            wds.append(_zeroDepthInput.propwid);
            wds.append(_refDepthInput.propwid);
            wds.append(_refPressureInput.propwid);
            wds.append(combofunc.propwid);
        }

        return wds;
    }

    void mLoadPressurePanel::savePressure()
    {
        if (!_currentLoad)
            return;

        QVector<int> curidx = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double pressureRatio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, curidx, { 3,0,0,0 });

        QString pastr = inputWidgets[0].lineeditvalue->text().trimmed();
        _currentLoad->pa_ = pastr.isEmpty() ? QVariant() : QVariant::fromValue<double>(pastr.toDouble() * pressureRatio);

        notifyLoadChanged();
    }

    void mLoadPressurePanel::saveDepthInfo()
    {
        if (!_currentLoad)
            return;

        QVector<int> curidx = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double pressureRatio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, curidx, { 3,0,0,0 });
        double lengthRatio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Length, curidx, { 3,0,0,0 });

        // 从方向控件获取深度方向并保存 UI 状态
        applyDirectionWidgetToLoad(_currentLoad);

        QString zeroStr = _zeroDepthInput.lineeditvalue->text().trimmed();
        _currentLoad->zeroDepth_ = zeroStr.isEmpty() ? QVariant() : QVariant::fromValue<double>(zeroStr.toDouble());

        QString refStr = _refDepthInput.lineeditvalue->text().trimmed();
        _currentLoad->refDepth_ = refStr.isEmpty() ? QVariant() : QVariant::fromValue<double>(refStr.toDouble());

        QString refPaStr = _refPressureInput.lineeditvalue->text().trimmed();
        _currentLoad->refPressure_ = refPaStr.isEmpty() ? QVariant() : QVariant::fromValue<double>(refPaStr.toDouble() * pressureRatio);

        notifyLoadChanged();
    }

    void mLoadPressurePanel::saveFunc()
    {
        if (!_currentLoad)
            return;

        int funcid = getFunc();
        getCVM->getModelData()->getMapRelationManager().appendMapRelation(
            { Physical_LoadBoundary, _loadData->getLoadBoundaryId() },
            { Physical_Function, funcid });

        notifyLoadChanged();
    }

    void mLoadPressurePanel::saveAnalysisField()
    {
        if (!_currentLoad)
            return;

        int fieldid = -1;
        QString fieldname = _analysisFieldCombo.combobox->currentText();
        if (_analysisFieldTable.find(fieldname) != _analysisFieldTable.end())
        {
            fieldid = _analysisFieldTable[fieldname];
        }
        getCVM->getModelData()->getMapRelationManager().appendMapRelation(
            { Physical_LoadBoundary, _loadData->getLoadBoundaryId() },
            { Physical_Field, fieldid }, MapRelationType::MapRelation_AnalysisField);

        notifyLoadChanged();
    }

    void mLoadPressurePanel::saveScale()
    {
        if (!_currentLoad)
            return;

        double scale = _scale.lineeditvalue->text().trimmed().toDouble();
        getCVM->getModelData()->getMapRelationManager().setScaleByCase(
            { MxFunctions::Physical_LoadBoundary, _loadData->getLoadBoundaryId() }, _caseId, scale);

        notifyLoadChanged();
    }

    void mLoadPressurePanel::notifyLoadChanged()
    {
        if (!_loadData)
            return;

        // 仅通知外部刷新树/渲染，不再调用 storeLoadValue 全量保存
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, _loadData->getLoadBoundaryId());
    }

    void mLoadPressurePanel::saveModeState()
    {
        if (!_currentLoad)
            return;

        const int pressureType = _pressureTypeCombo.combobox->currentIndex();
        _currentLoad->pressureType_ = pressureType;

        if (pressureType == 0)
        {
            const int defineType = _defineTypeCombo.combobox->currentIndex();
            // 分析场（组合框第3项）：way_ 保持统一赋值语义，独立标志记录分析场方式
            _currentLoad->isAnalysisFieldMode_ = (defineType == 3);
            _currentLoad->way_ = (defineType == 3) ? 0 : defineType;

            // 从净水压力切换回常规压力时，清理净水压力相关字段，避免交叉污染
            _currentLoad->depthDirection_ = QVector3D();
            _currentLoad->depthDirPickState_ = DirectionPickState();
            _currentLoad->depthDirPickState_.mode = 1; // mDirectionSelectorWidget::XAxis
            _currentLoad->zeroDepth_ = QVariant();
            _currentLoad->refDepth_ = QVariant();
            _currentLoad->refPressure_ = QVariant();

            const int way = _currentLoad->way_;
            if (way == 0)
            {
                _currentLoad->path_ = "";
            }
            else if (way == 1)
            {
                _currentLoad->pa_ = QVariant();
                _currentLoad->path_ = "";
            }
            else if (way == 2)
            {
                _currentLoad->pa_ = QVariant();
                _currentLoad->path_ = _filePathEdit->toolTip();
            }

            // 分析场：无大小/文件/独立数据
            if (_currentLoad->isAnalysisFieldMode_)
            {
                _currentLoad->pa_ = QVariant();
                _currentLoad->path_ = "";
                _currentLoad->cellFaceMap_.clear();
            }
        }
        else
        {
            // 净水压力：固定为统一赋值，清理常规压力字段
            _currentLoad->way_ = 0;
            _currentLoad->isAnalysisFieldMode_ = false;
            _currentLoad->pa_ = QVariant();
            _currentLoad->path_ = "";
            _currentLoad->cellFaceMap_.clear();
        }

        notifyLoadChanged();
    }

    void mLoadPressurePanel::applyDirectionWidgetToLoad(std::shared_ptr<MechCaseData::mLoadPressure> load)
    {
        if (!load || !_directionWidget)
        {
            return;
        }

        // 保存方向矢量
        QVariant dirVar = _directionWidget->getDirection();
        if (dirVar.isValid() && !dirVar.isNull())
        {
            QVector3D dir = dirVar.value<QVector3D>();
            if (!dir.isNull())
            {
                dir.normalize();
                load->depthDirection_ = dir;
            }
            else
            {
                load->depthDirection_ = QVector3D(1, 0, 0);
            }
        }
        else
        {
            load->depthDirection_ = QVector3D(1, 0, 0);
        }

        // 保存方向控件完整 UI 状态
        auto& state = load->depthDirPickState_;
        state.mode = static_cast<int>(_directionWidget->directionMode());
        state.reversed = _directionWidget->isReversed();

        if (state.mode == mDirectionSelectorWidget::CustomCoordinate)
        {
            state.startPoint = QVariant::fromValue(_directionWidget->startPoint());
            state.endPoint = QVariant::fromValue(_directionWidget->endPoint());
        }
        else if (state.mode == mDirectionSelectorWidget::SelectObject)
        {
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
            state.directInputDir = dirVar.value<QVector3D>();
        }
    }

    void mLoadPressurePanel::restoreDirectionWidgetFromLoad(std::shared_ptr<MechCaseData::mLoadPressure> load)
    {
        if (!load || !_directionWidget)
        {
            return;
        }

        const auto& state = load->depthDirPickState_;

        // 先阻断 directionChanged 信号，避免反显过程中触发保存
        bool oldSignalsBlocked = _directionWidget->blockSignals(true);

        _directionWidget->resetToDefault();

        // 如果 state.mode 为 0（DefaultDirection，未设置过），则根据 depthDirection_ 推断一个合理的方向控件模式，
        // 避免数据方向与控件显示不一致导致渲染错误。
        int mode = state.mode;
        bool reversed = state.reversed;
        QVector3D dir = load->depthDirection_;
        if (mode == mDirectionSelectorWidget::DefaultDirection && !dir.isNull())
        {
            dir.normalize();
            QVector3D absDir(qAbs(dir.x()), qAbs(dir.y()), qAbs(dir.z()));
            if (absDir.x() >= absDir.y() && absDir.x() >= absDir.z())
            {
                mode = mDirectionSelectorWidget::XAxis;
                reversed = (dir.x() < 0);
            }
            else if (absDir.y() >= absDir.x() && absDir.y() >= absDir.z())
            {
                mode = mDirectionSelectorWidget::YAxis;
                reversed = (dir.y() < 0);
            }
            else if (absDir.z() >= absDir.x() && absDir.z() >= absDir.y())
            {
                mode = mDirectionSelectorWidget::ZAxis;
                reversed = (dir.z() < 0);
            }
            else
            {
                mode = mDirectionSelectorWidget::DirectInput;
            }
        }

        _directionWidget->setDirectionMode(static_cast<mDirectionSelectorWidget::DirectionMode>(mode));
        _directionWidget->setReversed(reversed);

        if (mode == mDirectionSelectorWidget::CustomCoordinate &&
            state.startPoint.isValid() && !state.startPoint.isNull() &&
            state.endPoint.isValid() && !state.endPoint.isNull())
        {
            _directionWidget->setCustomCoordinatePoints(
                state.startPoint.value<QVector3D>(),
                state.endPoint.value<QVector3D>());
        }
        else if (mode == mDirectionSelectorWidget::SelectObject)
        {
            PickWidgetData displayData;
            if (state.selectedObject.second.result_id > 0)
            {
                displayData.pickedData[state.selectedObject.first].push_back(state.selectedObject.second);
            }
            _directionWidget->objectPickController()->displayData(displayData);
        }
        else if (mode == mDirectionSelectorWidget::DirectInput)
        {
            if (!state.directInputDir.isNull())
                _directionWidget->setDirection(state.directInputDir);
            else if (!load->depthDirection_.isNull())
                _directionWidget->setDirection(load->depthDirection_);
            else
                _directionWidget->setDirection(QVector3D(1, 0, 0));
        }

        _directionWidget->blockSignals(oldSignalsBlocked);
    }

    int mLoadPressurePanel::getFunc()
    {
        int funcid = -1;
        QString funcname = combofunc.combobox->currentText();
        if (_functable.find(funcname) != _functable.end())
        {
            funcid = _functable[funcname];
        }
        return funcid;
    }

    void mLoadPressurePanel::deactivatePickAndCommit()
    {
        // 面板为单例复用，切换载荷/面板时不会走析构/clearInputs，
        // 由统一退出出口（unEditLoad）调用，失活并提交当前拾取数据。
        const bool renderValid = (getCVM && getCVM->getPreRenderWidget());

        // 离散数据控件：结束拾取并关闭弹窗
        if (_discreteControl)
        {
            _discreteControl->closePopup();
        }

        if (_pickWidgetController)
        {
            _pickWidgetController->setKeepHighlightOnDeactivate(false);
            const bool wasActive = _pickWidgetController->isPickActive();
            if (wasActive) _pickWidgetController->setPickActive(false);
            if (renderValid && !wasActive)
            {
                _pickWidgetController->clearAllHighlight();
            }
            _pickWidgetController->setKeepHighlightOnDeactivate(true);
        }

        // 方向选择控件内部包含多个子拾取控件，切出面板时同样要失活并清理高亮
        if (_directionWidget)
        {
            auto deactivateChildPick = [](mPickWidgetController* ctrl) {
                if (!ctrl) return;
                const bool wasActive = ctrl->isPickActive();
                if (!wasActive) return;
                ctrl->setKeepHighlightOnDeactivate(false);
                ctrl->setPickActive(false);
                ctrl->setKeepHighlightOnDeactivate(true);
            };
            deactivateChildPick(_directionWidget->startPickController());
            deactivateChildPick(_directionWidget->endPickController());
            deactivateChildPick(_directionWidget->objectPickController());
            _directionWidget->clearAllHighlight();
        }
    }

    void mLoadPressurePanel::updateLoadSymbolRealtime()
    {
        if (!_pickWidgetController || !_currentLoad)
            return;
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        const int pressureType = _pressureTypeCombo.combobox->currentIndex();
        const int defineType = _defineTypeCombo.combobox->currentIndex();

        // 分析场方式无作用对象，无需渲染压强符号（清除残留箭头）
        if (pressureType == 0 && defineType == 3)
        {
            AllRenderSymbolManager::getInstance()->assignLoad(
                _currentLoad, {}, "Load_Pressure", QVector3D(50, 133, 255));
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController) symbolController->UpdateAllSymbol();
            getCVM->getPreRenderWidget()->updatePaint();
            return;
        }

        // 独立赋值：先按离散数据控件当前行同步 cellFaceMap_（数据源为表格，
        // 通用拾取控制器已不再接收视口拾取，不能再以它为准），再合成方向。
        if (pressureType == 0 && defineType == 1)
        {
            updateOpenedTable();
        }

        // 1. 汇总当前数据：拾取(pickedData) + 集合(selectedSetNames 按名称展开），合并为按类型分组的 map
        const PickWidgetData allData = _pickWidgetController->getAllData();
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> merged;
        if (!(pressureType == 0 && defineType == 1))
        {
            merged = allData.pickedData;
        }

        auto& setManager = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set);

        // 工具lambda：仅用于int类型ID数组 -> RenderPickResultData转换
        auto appendIds = [](MxFunctions::PickDataType type, const std::vector<int>& ids,
            std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& dst) {
                auto& vec = dst[type];
                for (int id : ids)
                    vec.emplace_back(id, -1, -1);
            };

        for (const QString& setName : allData.selectedSetNames)
        {
            mSetBase* setBase = setManager.getSetData(setName);
            if (!setBase) continue;

            if (auto mixSet = dynamic_cast<MechSetData::mMixSet*>(setBase))
            {
                // 混合集合：元素已经是RenderPickResultData，直接插入
                for (const auto& inner : mixSet->getMixSetData())
                {
                    auto& vec = merged[inner.first];
                    vec.insert(vec.end(), inner.second.begin(), inner.second.end());
                }
                continue;
            }

            switch (setBase->getSetType())
            {
            case MxFunctions::GeoFace:
                // 几何面集合：存储int ID，用appendIds转换
                if (auto s = dynamic_cast<mGeoSet*>(setBase))
                    appendIds(SetTypeToPickDataType(setBase->getSetType()), s->getGeoSetData(), merged);
                break;

            case MxFunctions::ElementSurface:
                // 单元面集合：存储RenderPickResultData，直接插入（修正类型不匹配）
                if (auto s = dynamic_cast<MechSetData::mElementSurface*>(setBase))
                {
                    auto& vec = merged[SetTypeToPickDataType(setBase->getSetType())];
                    const auto& surfaceData = s->getElementSurfaceData();
                    vec.insert(vec.end(), surfaceData.begin(), surfaceData.end());
                }
                break;

            default:
                break;
            }
        }

        // 2. 计算每个作用对象的位置与压力方向
        auto meshData = getCVM->getModelData()->getMeshData();
        auto geoManager = getCVM->getModelData()->getGeoManager();
        auto geoData = getCVM->getModelData()->getGeoData();

        QVector<std::pair<QVector3D, QVector3D>> pos_dir;

        // 常规压力：渲染方向与压强值符号关联，负值时方向取反；无值/非数字默认正方向（不翻转）。
        // 统一赋值读输入框实时值；独立赋值逐面查 cellFaceMap_；文件赋值无值默认正方向。
        bool negativeUniform = false;
        if (pressureType == 0 && defineType == 0)
        {
            bool paOk = false;
            double pa = inputWidgets[0].lineeditvalue->text().trimmed().toDouble(&paOk);
            negativeUniform = (paOk && pa < 0);
        }

        // 净水压力方向计算
        QVector3D hydrostaticDir;
        if (pressureType == 1)
        {
            if (_directionWidget)
            {
                QVariant dirVar = _directionWidget->getDirection();
                if (dirVar.isValid() && !dirVar.isNull())
                {
                    hydrostaticDir = dirVar.value<QVector3D>();
                }
            }
            if (hydrostaticDir.isNull() && !_currentLoad->depthDirection_.isNull())
            {
                hydrostaticDir = _currentLoad->depthDirection_;
            }
            if (hydrostaticDir.isNull())
            {
                hydrostaticDir = QVector3D(0, 0, 1);
            }
            // 静水压力：渲染方向与参考压强值符号关联，负值时方向取反；无值默认正方向
            bool refPaOk = false;
            double refPa = _refPressureInput.lineeditvalue->text().trimmed().toDouble(&refPaOk);
            if (refPaOk && refPa < 0)
            {
                hydrostaticDir = -hydrostaticDir;
            }
        }

        // 独立赋值：已剥离通用拾取，merged 为空；
        // 符号位置/方向直接遍历 cellFaceMap_（表格为权威数据源）
        if (pressureType == 0 && defineType == 1)
        {
            for (const auto& entry : _currentLoad->cellFaceMap_)
            {
                QVector3D centerPos;
                QVector3D normal;
                meshData->GetOutVectorByMeshFaceID(entry.first, normal, centerPos);
                if (centerPos.isNull()) continue;

                // 独立赋值：该面压强为负时方向取反，无值默认正方向
                bool negative = false;
                if (!entry.second.isEmpty() && entry.second[0].isValid() && !entry.second[0].isNull())
                {
                    bool ok = false;
                    const double v = entry.second[0].toDouble(&ok);
                    negative = (ok && v < 0);
                }
                pos_dir.push_back({ centerPos, negative ? -normal : normal });
            }
        }
        else
        {
            for (const auto& pair : merged)
            {
                if (pair.second.empty()) continue;

                switch (pair.first)
                {
                case MxFunctions::PickDataType::ElementFace:
                {
                    for (const auto& d : pair.second)
                    {
                        QVector3D centerPos;
                        QVector3D normal;
                        // 直接传入RenderPickResultData，无需重复构造
                        meshData->GetOutVectorByMeshFaceID(d, normal, centerPos);
                        if (centerPos.isNull()) continue;

                        if (pressureType == 1)
                        {
                            pos_dir.push_back({ centerPos, hydrostaticDir });
                        }
                        else
                        {
                            pos_dir.push_back({ centerPos, negativeUniform ? -normal : normal });
                        }
                    }
                    break;
                }
                case MxFunctions::PickDataType::GeoFace:
                {
                    std::vector<int> ids;
                    ids.reserve(pair.second.size());
                    for (const auto& d : pair.second)
                        ids.push_back(d.result_id);

                    auto points = geoManager->CalculateFaceAttr(ids);
                    auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(
                        2, QVector<int>::fromStdVector(ids)); //【引擎化】去 geoData 尾参

                    // 与基准接口逻辑对齐：所有顶点复用第一个面的法向
                    QVector3D faceDir = points.empty() ? QVector3D() : points[0].m_dir;
                    for (const auto& coord : coords)
                    {
                        if (pressureType == 1)
                        {
                            pos_dir.push_back({ coord, hydrostaticDir });
                        }
                        else
                        {
                            // 几何面仅在统一赋值下可选：负值取反，无值默认正方向
                            pos_dir.push_back({ coord, negativeUniform ? -faceDir : faceDir });
                        }
                    }
                    break;
                }
                default:
                    break;
                }
            }
        }

        // 3. 重画该载荷的压力符号
        AllRenderSymbolManager::getInstance()->assignLoad(
            _currentLoad, pos_dir, "Load_Pressure", QVector3D(50, 133, 255));

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (symbolController) symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }
}