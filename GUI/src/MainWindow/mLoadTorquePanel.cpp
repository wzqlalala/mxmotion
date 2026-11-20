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
#include "AllRenderSymbolManager.h"
#include "mGeneralPickUp.h"
#include "mGetSetTypeTemplate.h"
#include <QFileDialog>
#include "mAppConfiguration.h"
#include "mDockLayoutManager.h"
#include "mGetSetFunction.h"

//Geo
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（冻结岛保档，同主线处理）

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/label/mSymbolController.h"
#include "mxr/view/mShapeRender.h"

//Basic
#include<mMixSet.h>
#include<mMeshData.h>
#include<mGeoSet.h>
#include<mNode.h>
#include<mElementSurface.h>
#include<mElementSet.h>

#include "mLoadTorquePanel.h"
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;
namespace MechMainWindow
{
    mLoadTorquePanel::mLoadTorquePanel(QWidget* parent)
        :mLoadPanelBase(parent)
    {
        initInputs();
    }

    mLoadTorquePanel::~mLoadTorquePanel()
    {
        clearInputs();
    }

    void mLoadTorquePanel::connectUnitSystemResponse()
    {
        connect(getCVM->getModelData()->getUnitSystemManager(), &mUnitSystemManager::sg_unitChanged, this, &mLoadTorquePanel::slot_unitSystemResponse, Qt::UniqueConnection);
    }

    void mLoadTorquePanel::disconnectUnitSystemResponse()
    {
        disconnect(getCVM->getModelData()->getUnitSystemManager(), &mUnitSystemManager::sg_unitChanged, this, &mLoadTorquePanel::slot_unitSystemResponse);
    }

    void mLoadTorquePanel::slot_unitSystemResponse(UnitType type, QVector<int> idx)
    {
        Q_UNUSED(idx);
        if (!(type & UnitSystem)) return;
        if (_loadData)
            setInputsValue(_loadData);   //从载荷对象(基准值)重渲染，值+单位标签一次到位
    }

    // 刷新独立赋值表格
    void mLoadTorquePanel::updateOpenedTable()
    {
        if (!_currentLoad || !_pickWidgetController)
        {
            return;
        }

        // 独立赋值模式下，nodeForceMap_ 与离散数据控件的行数据保持严格一致：
        // 表格是数据的权威来源（含拾取追加与手动删除），此处按控件当前行重建 map，
        // 已填分量按节点 ID 保留。
        if (!_discreteControl)
        {
            return;
        }

        const auto rows = _discreteControl->rows();

        // 已填分量按节点 ID 保留；key 规范为 (result_id, -1, -1)（节点拾取 part_id/result_id2
        // 固定为 -1），保证 slot_discreteCellEdited 可用同一规范 key 直接索引。
        std::map<int, QVector<QVariant>> existingValues;
        for (const auto& entry : _currentLoad->nodeForceMap_)
        {
            const int nodeId = entry.first.result_id;
            if (existingValues.find(nodeId) == existingValues.end())
            {
                existingValues[nodeId] = entry.second;
            }
        }

        std::map<RenderPickResultData, QVector<QVariant>> newMap;
        for (const auto& row : rows)
        {
            const int nodeId = row.first.first;
            if (nodeId <= 0)
                continue;
            auto it = existingValues.find(nodeId);
            newMap[RenderPickResultData(nodeId, -1, -1)] =
                (it != existingValues.end()) ? it->second : QVector<QVariant>(3);
        }

        _currentLoad->nodeForceMap_ = std::move(newMap);
    }

    // 对标温度载荷：保留节点，删除所有非节点拾取
    void mLoadTorquePanel::filterPicksToNodeOnly()
    {
        // 同步通用拾取控件最新数据到 picks，避免过滤时丢失拾取/集合
        auto picks = _pickWidgetController->getAllData().pickedData;

        auto it = picks.begin();
        while (it != picks.end())
        {
            if (it->first != MxFunctions::PickDataType::Node)
            {
                it = picks.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // 独立赋值/文件模式无需集合：把控件数据重设为"仅节点拾取、无集合"。
        if (_pickWidgetController)
        {
            _pickWidgetController->replacePickedDataAndClearSets(picks);
        }
        // 独立赋值已完全剥离通用拾取：清空并抑制本控件的一切拾取高亮渲染，
        // 拾取由离散数据控件直接驱动视口选择容器，视口中不再出现蓝色高亮。
        if (_pickWidgetController)
        {
            _pickWidgetController->setCloudChartMode(true);
        }

        updateOpenedTable();
    }

    void mLoadTorquePanel::initInputs()
    {
        // ===================== 新增：定义方式下拉框 =====================
        _defineTypeCombo.initWidget("定义方式");
        _defineTypeCombo.combobox->addItem("各向分量");
        _defineTypeCombo.combobox->addItem("独立赋值");
        _defineTypeCombo.combobox->addItem("文件");
        _defineTypeCombo.combobox->setMinimumWidth(100);
        _defineTypeCombo.pushbtn->hide();
        _defineTypeCombo.combobox->setCurrentIndex(0);
        connect(_defineTypeCombo.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadTorquePanel::slot_defineTypeChanged);

        // ===================== 修改：力矩分量名称（M1→X轴力矩） =====================
        QStringList inpnames = {
            "X轴力矩",
            "Y轴力矩",
            "Z轴力矩",
        };
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit = UnitsLibrary[(int)Unit::Torque][sysunitidxs[0]];
        for (int i = 0; i < 3; ++i)
        {
            InputWidgetSet inp;
            inp.initWidget(inpnames[i], false);
            inp.lineeditvalue->setUnit(unit);
            inp.labelname->setText(inpnames[i]);
            inp.lineeditvalue->setFixedHeight(30);
            inputWidgets.append(inp);
            connect(inp.lineeditvalue, &mCalcLineEdit::editingFinished,
                    [this]() {
                        updateLoadSymbolRealtime();
                        saveTorqueComponents();
                    });
        }

        // ===================== 独立赋值表格控件：通用离散数据控件（入口按钮 + 锚定弹窗表格） =====================
        _tableRowWidget = new QWidget();
        QHBoxLayout* tableRowLayout = new QHBoxLayout(_tableRowWidget);
        tableRowLayout->setContentsMargins(0, 0, 0, 0);
        tableRowLayout->setSpacing(8);

        _tableLabel = new QLabel("节点数据");
        _tableLabel->setMinimumWidth(87);
        tableRowLayout->addWidget(_tableLabel);

        mDiscreteDataControl::PickSpec nodeSpec;
        nodeSpec.typeName = QStringLiteral("节点");
        nodeSpec.idHeaders = QStringList{ QStringLiteral("节点编号") };
        nodeSpec.keyOf = [](const RenderPickResultData& d) { return qMakePair(d.result_id, -1); };
        nodeSpec.pickTypes = { MxFunctions::PickDataType::Node };
        nodeSpec.pickFilter = 5; // 视口拾取过滤器：仅节点（与 activePickSig 索引一致）
        // 拾取由离散数据控件直接驱动视口选择容器，不再经过通用拾取控件
        _discreteControl = new mDiscreteDataControl(
            QStringLiteral("节点数据"),
            { QStringLiteral("X轴力矩"), QStringLiteral("Y轴力矩"), QStringLiteral("Z轴力矩") },
            &nodeSpec, _tableRowWidget);
        // 力矩沿用既有实时符号渲染，不显示渲染云图按钮（默认隐藏）
        _discreteControl->setRenderButtonVisible(false);
        connect(_discreteControl, &mDiscreteDataControl::dataChanged,
            this, &mLoadTorquePanel::slot_discreteDataChanged);
        connect(_discreteControl, &mDiscreteDataControl::cellEdited,
            this, &mLoadTorquePanel::slot_discreteCellEdited);
        // 入口按钮从控件中剖分出来直接放入行布局，控件本体隐藏（弹窗为
        // 宿主主窗口子窗口不受影响），按钮可用状态与控件本体显隐/禁用解耦
        if (QPushButton* entry = _discreteControl->entryButton())
        {
            _discreteControl->detachEntryButton(_tableRowWidget);
            tableRowLayout->addWidget(entry, 1);
            _discreteControl->hide();
        }
        else
        {
            tableRowLayout->addWidget(_discreteControl);
        }

        // ===================== 文件选择控件 =====================
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
        connect(_fileBrowseBtn, &QPushButton::clicked,
            this, &mLoadTorquePanel::slot_browseFileClicked);
        fileRowLayout->addWidget(_fileBrowseBtn);

        // 通用拾取控件
        _pickWidgetController = mGeneralPickUp::createPickWidget(CBBlue);
        connect(_pickWidgetController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive)
            {
                emit mMechGlobalSignals::getInstance()->activePickSig(5);
            }
            });
        connect(_pickWidgetController, &mPickWidgetController::dataChanged, this, [this]() {
            // 独立赋值模式下通用拾取控制器不再接收拾取（由离散数据控件直驱视口），
            // 其 dataChanged 只在各向分量/文件模式下触发，无需按方式分支处理。
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

        // 函数
        _functable.clear();
        combofunc.initWidget("幅值曲线");
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
            this, &mLoadTorquePanel::slot_funcChanged);
        connect(combofunc.pushbtn, &QPushButton::clicked, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createFunction(2);

            combofunc.combobox->blockSignals(true);
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
            combofunc.combobox->blockSignals(false);
            saveFunc();
            });

        // 坐标系
        combocoor.initWidget("坐标系");
        combocoor.pushbtn->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/zuobiaoxi.png"));
        combocoor.pushbtn->setFixedSize(30, 30);
        combocoor.pushbtn->setIconSize(QSize(18, 18));
        combocoor.combobox->setFixedHeight(30);
        auto allCoor = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getAllCoordinateNames();
        for (auto coorname : allCoor)
        {
            combocoor.combobox->addItem(coorname);
        }
        connect(combocoor.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadTorquePanel::slot_coorChanged);
        connect(combocoor.pushbtn, &QPushButton::clicked, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createCoordinate();

            combocoor.combobox->blockSignals(true);
            combocoor.combobox->clear();
            auto allCoor = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getAllCoordinateNames();
            for (auto coorname : allCoor)
            {
                combocoor.combobox->addItem(coorname);
            }
            combocoor.combobox->setCurrentIndex(allCoor.size() > 0 ? allCoor.size() : 0);
            combocoor.combobox->blockSignals(false);
            saveCoor();
            });

        // 缩放因子
        _scale.initWidget("缩放因子", false);
        _scale.labelname->setText("缩放因子");
        connect(_scale.lineeditvalue, &mCalcLineEdit::editingFinished,
            [this]() {
                saveScale();
            });
    }

    void mLoadTorquePanel::saveTorqueComponents()
    {
        if (!_currentLoad)
        {
            return;
        }

        QVector<int> curidx = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Force, curidx, { 3,0,0,0 });

        // 读取输入文本并校验数字转换结果
        QString xStr = inputWidgets[0].lineeditvalue->text().trimmed();
        QString yStr = inputWidgets[1].lineeditvalue->text().trimmed();
        QString zStr = inputWidgets[2].lineeditvalue->text().trimmed();

        bool xOk = false, yOk = false, zOk = false;
        double fx = xStr.toDouble(&xOk);
        double fy = yStr.toDouble(&yOk);
        double fz = zStr.toDouble(&zOk);

        // 分量独立赋值：转换成功则计算单位比例后赋值，失败则设为无效空值
        if (xOk) {
            _currentLoad->force_x = fx * ratio;
        }
        else {
            _currentLoad->force_x.clear();
        }

        if (yOk) {
            _currentLoad->force_y = fy * ratio;
        }
        else {
            _currentLoad->force_y.clear();
        }

        if (zOk) {
            _currentLoad->force_z = fz * ratio;
        }
        else {
            _currentLoad->force_z.clear();
        }

        // 方向向量：仅当三个分量全部转换成功时才计算方向，否则方向置为无效
        if (xOk && yOk && zOk) {
            QVector3D dir(fx, fy, fz);
            if (!dir.isNull()) {
                dir.normalize();
                _currentLoad->dir_ = dir;
            }
            else {
                _currentLoad->dir_ = QVariant();
            }
        }
        else {
            _currentLoad->dir_ = QVariant();
        }

        notifyLoadChanged();
    }

    void mLoadTorquePanel::saveWayAndClearModeFields()
    {
        if (!_currentLoad)
        {
            return;
        }
        _currentLoad->way_ = _defineTypeCombo.combobox->currentIndex();
        // 重置所有模式字段，避免交叉污染
        _currentLoad->force_ = QVariant();
        _currentLoad->force_x = QVariant();
        _currentLoad->force_y = QVariant();
        _currentLoad->force_z = QVariant();
        _currentLoad->dir_ = QVariant();
        _currentLoad->path_ = "";
        _currentLoad->dirPickState_ = DirectionPickState();
    }

    void mLoadTorquePanel::saveFilePath()
    {
        if (!_currentLoad)
        {
            return;
        }
        _currentLoad->path_ = _filePathEdit->toolTip();
        notifyLoadChanged();
    }

    void mLoadTorquePanel::saveScale()
    {
        if (!_currentLoad)
        {
            return;
        }
        double scale = _scale.lineeditvalue->text().trimmed().toDouble();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        mapload.setScaleByCase({ Physical_LoadBoundary ,_loadData->getLoadBoundaryId() }, _caseId, scale);
        notifyLoadChanged();
    }

    void mLoadTorquePanel::saveFunc()
    {
        if (!_currentLoad)
        {
            return;
        }
        if (_currentLoad->way_ == 2) // 文件模式无函数
        {
            return;
        }
        int funcid = getFunc();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        mapload.appendMapRelation({ Physical_LoadBoundary ,_loadData->getLoadBoundaryId() }, { Physical_Function,funcid });
        notifyLoadChanged();
    }

    void mLoadTorquePanel::saveCoor()
    {
        if (!_currentLoad)
        {
            return;
        }
        if (_currentLoad->way_ != 0 && _currentLoad->way_ != 1) // 仅各向分量/独立赋值保存坐标系
        {
            return;
        }
        int coorid = getCoor();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        mapload.appendMapRelation({ Physical_LoadBoundary ,_loadData->getLoadBoundaryId() }, { Physical_Coordinate,coorid });
        notifyLoadChanged();
    }

    void mLoadTorquePanel::notifyLoadChanged()
    {
        if (!_loadData)
        {
            return;
        }
        // 仅通知外部刷新树/渲染
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, _loadData->getLoadBoundaryId());
    }

    void mLoadTorquePanel::slot_defineTypeChanged(int idx)
    {
        saveWayAndClearModeFields();

        // 切换定义方式时关闭离散数据弹窗并结束拾取，避免旧模式弹窗残留
        if (_discreteControl)
        {
            _discreteControl->closePopup();
        }

        // 同步拾取类型白名单
        applyAllowedPickTypes(idx);

        // ===================== 核心：照搬集中力 → 清空布局，重新添加控件 =====================
        faval_->removeAll();
        // 先添加定义方式下拉框
        faval_->addRow(_defineTypeCombo.propwid);
        _defineTypeCombo.propwid->show();

        // 文件模式：完全隐藏拾取控件；独立赋值模式：拾取由离散数据控件直接驱动，
        // 通用拾取控件不再激活。其余模式恢复正常显示与激活。
        if (_pickWidgetController)
        {
            if (idx == 1 || idx == 2)
            {
                _pickWidgetController->setPickWidgetVisible(false);
            }
            else
            {
                _pickWidgetController->setPickWidgetVisible(true);
                _pickWidgetController->setPickActive(true);
            }
        }

        if (idx == 0) // 各向分量：显示力矩输入框 + 函数 + 坐标系
        {
            faval_->addRow(inputWidgets[0].propwid);
            inputWidgets[0].propwid->show();
            faval_->addRow(inputWidgets[1].propwid);
            inputWidgets[1].propwid->show();
            faval_->addRow(inputWidgets[2].propwid);
            inputWidgets[2].propwid->show();
            faval_->addRow(combofunc.propwid);
            combofunc.propwid->show();
            faval_->addRow(combocoor.propwid);
            combocoor.propwid->show();
        }
        else if (idx == 1) // 独立赋值：显示表格 + 函数 + 坐标系
        {
            faval_->addRow(_tableRowWidget);
            _tableRowWidget->show();
            faval_->addRow(combofunc.propwid);
            combofunc.propwid->show();
            faval_->addRow(combocoor.propwid);
            combocoor.propwid->show();
        }
        else // 文件模式（idx=2）：仅显示文件选择
        {
            saveFilePath();
            faval_->addRow(_fileRowWidget);
            _fileRowWidget->show();
        }

        // 刷新布局（集中力标准写法）
        faval_->refreshLayout();

        // 独立赋值：隐藏集合选择按钮，强制仅节点（filterPicksToNodeOnly 内同时
        // 开启云图抑制模式，彻底关闭通用拾取控件的高亮渲染，拾取高亮完全剥离）
        if (idx == 1)
        {
            filterPicksToNodeOnly();
            if (_pickWidgetController)
            {
                _pickWidgetController->setSelectSetBtnVisible(false);
            }
        }
        else if (idx == 2) // 文件：隐藏集合选择按钮
        {
            filterPicksToNodeOnly();
            if (_pickWidgetController)
            {
                _pickWidgetController->setSelectSetBtnVisible(false);
            }
        }
        else // 各向分量：显示集合选择按钮，恢复正常高亮渲染
        {
            if (_pickWidgetController)
            {
                _pickWidgetController->setSelectSetBtnVisible(true);
                _pickWidgetController->setCloudChartMode(false);
            }
            updateLoadSymbolRealtime();
        }

        // 定义方式切换属于结构性变化，仍通过 sg_rebuildPanel 由外部槽统一整体保存
        emit sg_rebuildPanel();
    }

    void mLoadTorquePanel::slot_discreteDataChanged()
    {
        // 离散数据控件行变化（拾取追加/删除/清空/分量增减）：
        // 按控件当前行重建 nodeForceMap_，随后刷新实时符号并通知外部。
        // 拾取会话进行中跳过：确认拾取（完成拾取）后控件会补发一次 dataChanged，
        // 此时才执行重建与渲染，避免拾取界面期间反复重绘。
        if (!_currentLoad || (_discreteControl && _discreteControl->isPicking()))
        {
            return;
        }
        updateOpenedTable();
        updateLoadSymbolRealtime();
        notifyLoadChanged();
    }

    void mLoadTorquePanel::slot_discreteCellEdited(const QPair<int, int>& key, int valueIndex, const QString& text)
    {
        if (!_currentLoad || key.first <= 0)
        {
            return;
        }
        // 单点写回：仅更新/清除该节点的该分量，节点本身始终保留。
        // 力矩固定 3 个分量（X/Y/Z 轴力矩），节点拾取 key 规范为 (result_id, -1, -1)。
        const RenderPickResultData loadKey(key.first, -1, -1);
        QVector<QVariant>& values = _currentLoad->nodeForceMap_[loadKey];
        if (values.size() < 3)
        {
            values.resize(3);
        }
        if (valueIndex >= 0 && valueIndex < 3)
        {
            bool ok = false;
            const double v = text.toDouble(&ok);
            values[valueIndex] = (ok && !text.isEmpty()) ? QVariant(v) : QVariant();
        }
        updateLoadSymbolRealtime();
        notifyLoadChanged();
    }

    void mLoadTorquePanel::slot_browseFileClicked()
    {
        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
        QFileInfo info(defaultDir);
        if (defaultDir.isEmpty() || !info.exists())
        {
            defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
        }
        QString filePath = QFileDialog::getOpenFileName(
            nullptr,
            "选择文件",
            defaultDir,
            "Text Files (*.txt);;All Files (*)");
        if (!filePath.isEmpty())
        {
            QFileInfo fi(filePath);
            QString fileName = fi.fileName();
            _filePathEdit->setText(fileName);
            _filePathEdit->setToolTip(filePath);
            saveFilePath();
        }
    }

    void mLoadTorquePanel::slot_funcChanged(int idx)
    {
        Q_UNUSED(idx)
        saveFunc();
    }

    void mLoadTorquePanel::slot_coorChanged(int idx)
    {
        Q_UNUSED(idx)
        saveCoor();
    }

    void mLoadTorquePanel::clearInputs()
    {
        for (auto& inp : inputWidgets) inp.clear();
        inputWidgets.clear();

        if (_pickWidgetController)
        {
            _pickWidgetController->setPickActive(false);
            delete _pickWidgetController;
            _pickWidgetController = nullptr;
        }

        for (auto& combo : comboBoxs) combo.clear();
        comboBoxs.clear();
        _defineTypeCombo.clear();
        combofunc.clear();
        combocoor.clear();
        _functable.clear();
        _currentLoad.reset();

        if (_tableRowWidget) { delete _tableRowWidget; _tableRowWidget = nullptr; }
        _tableLabel = nullptr;
        _discreteControl = nullptr; // 随 _tableRowWidget 一并删除
        if (_fileRowWidget) { delete _fileRowWidget; _fileRowWidget = nullptr; }
        _fileLabel = nullptr;
        _filePathEdit = nullptr;
        _fileBrowseBtn = nullptr;
    }

    void mLoadTorquePanel::setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase>loadData)
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Torque, { 3,0,0,0 }, sysunitidxs);
        QString unit = UnitsLibrary[(int)Unit::Torque][sysunitidxs[0]];
        std::shared_ptr<MechCaseData::mLoadForce> torquePtr = std::dynamic_pointer_cast<MechCaseData::mLoadForce>(loadData);
        if (!torquePtr) return;
        _currentLoad = torquePtr;

        // 持久化策略绑定
        if (_pickWidgetController)
        {
            auto strategy = std::make_shared<MapRelationPersistenceStrategy>(
                MxFunctions::Physical_LoadBoundary,
                _loadData->getLoadBoundaryId());
            _pickWidgetController->setPersistenceStrategy(strategy);
            _pickWidgetController->setSetResolver(&resolveSetToTypedData);

            QHash<QString, QString> setToType = getSetNamesList({ SetType::GeoPoint,SetType::GeoEdge, SetType::GeoFace,SetType::GeoSolid,SetType::Node });
            _pickWidgetController->setSelectSetList(setToType);

            applyAllowedPickTypes(torquePtr->way_);
        }

        // 阻塞信号
        _defineTypeCombo.combobox->blockSignals(true);
        combofunc.combobox->blockSignals(true);
        combocoor.combobox->blockSignals(true);

        // 回显定义方式 (way_直接存下拉框索引：0=各向分量,1=独立赋值,2=文件)
        int defineType = torquePtr->way_;
        _defineTypeCombo.combobox->blockSignals(true);
        _defineTypeCombo.combobox->setCurrentIndex(defineType);
        _defineTypeCombo.combobox->blockSignals(false);

        // 力矩分量回显
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setUnit(unit);
            inp.lineeditvalue->setExpression("");
            inputWidgets[1].lineeditvalue->clear();// 清空界面显示文字
        }
        QString m1str = torquePtr->force_x.isValid() ? MxFunctions::mNumberToString(torquePtr->force_x.toDouble() * ratio) : "";
        QString m2str = torquePtr->force_y.isValid() ? MxFunctions::mNumberToString(torquePtr->force_y.toDouble() * ratio) : "";
        QString m3str = torquePtr->force_z.isValid() ? MxFunctions::mNumberToString(torquePtr->force_z.toDouble() * ratio) : "";
        inputWidgets[0].lineeditvalue->setExpression(m1str);
        inputWidgets[1].lineeditvalue->setExpression(m2str);
        inputWidgets[2].lineeditvalue->setExpression(m3str);

        // 函数
        _functable.clear();
        combofunc.combobox->clear();
        combofunc.combobox->addItem("");
        _functable[""] = -1;
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
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
            combofunc.combobox->setCurrentText(funcname);
        }

        // 坐标系
        combocoor.combobox->clear();
        auto allCoor = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getAllCoordinateNames();
        for (auto coorname : allCoor) combocoor.combobox->addItem(coorname);
        int coorid = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_Coordinate);
        if (coorid != -1)
        {
            QString coorname = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getCoordinateNameByID(coorid);
            combocoor.combobox->setCurrentText(coorname);
        }

        // 作用对象反显
        // 单例面板复用：无论新载荷定义方式如何，先清空离散表格残留行，
        // 避免旧载荷独立赋值数据在用户切回独立赋值时复现并污染新载荷
        if (_discreteControl)
        {
            _discreteControl->setRows({});
        }
        PickWidgetData displayData;
        int setid = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_Set);
        if (setid != -1 && (torquePtr->way_ == 0 || torquePtr->way_ == 1))
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
        else if (torquePtr->way_ == 1)
        {
            // 独立赋值：优先保留 nodeForceMap_ 中的完整 key（含原始 part_id），
            // 避免用 result_id2=0 的临时 pickData 破坏 key 完整性。
            for (auto it = torquePtr->nodeForceMap_.begin(); it != torquePtr->nodeForceMap_.end(); ++it)
            {
                const RenderPickResultData& key = it->first;
                displayData.pickedData[PickDataType::Node].push_back(key);
            }

            // 反显到离散数据控件（节点 + 已填分量）
            if (_discreteControl)
            {
                QVector<mDiscreteDataControl::Row> rows;
                rows.reserve(static_cast<int>(torquePtr->nodeForceMap_.size()));
                for (const auto& entry : torquePtr->nodeForceMap_)
                {
                    QStringList values;
                    for (int i = 0; i < 3; ++i)
                    {
                        const QVariant v = i < entry.second.size() ? entry.second.at(i) : QVariant();
                        values << ((v.isValid() && !v.isNull()) ? QString::number(v.toDouble()) : QString());
                    }
                    rows.append(qMakePair(qMakePair(entry.first.result_id, -1), values));
                }
                _discreteControl->setRows(rows);
            }
        }

        if (_pickWidgetController)
        {
            if (torquePtr->way_ == 1)
            {
                // 独立赋值已剥离通用拾取：仅写入数据（供 pickState/持久化一致性），
                // 全程保持失活，不激活、不渲染任何拾取高亮。
                // 入口同步隐藏控件（对齐 slot_defineTypeChanged），保证 _isVisible 与实际一致，
                // applyHighlight 的可见性守卫才能拦住高亮渲染
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
            _pickWidgetController->setSelectSetBtnVisible(torquePtr->way_ != 1 && torquePtr->way_ != 2);
        }

        if (torquePtr->way_ == 1 || torquePtr->way_ == 2)
        {
            filterPicksToNodeOnly();
        }

        // 文件路径回显
        if (_filePathEdit)
        {
            QFileInfo fi(torquePtr->path_);
            _filePathEdit->setText(fi.fileName());
            _filePathEdit->setToolTip(torquePtr->path_);
        }

        //缩放因子
        double scale = getCVM->getModelData()->getMapRelationManager().getScaleByCase({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, _caseId);
        _scale.lineeditvalue->setExpression(MxFunctions::mNumberToString(scale));

        // 解除信号阻塞
        _defineTypeCombo.combobox->blockSignals(false);
        combofunc.combobox->blockSignals(false);
        combocoor.combobox->blockSignals(false);

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
        QMetaObject::invokeMethod(this, &mLoadTorquePanel::updateLoadSymbolRealtime, Qt::QueuedConnection);
    }

    QVector<QWidget*> mLoadTorquePanel::getRefStateInputs(FoldSection* fathertar)
    {
        refstate_ = fathertar;
        QVector<QWidget*>wds;
        wds.append(_scale.propwid);
        return wds;
    }

    QVector<QWidget*> mLoadTorquePanel::getTargetInputs(FoldSection* fathertar)
    {
        fatar_ = fathertar;
        QVector<QWidget*>wds;
        if (_pickWidgetController)
        {
            wds.append(_pickWidgetController->widget());
        }
        return wds;
    }

    QVector<QWidget*> mLoadTorquePanel::getValueInputs(FoldSection* fatherval)
    {
        faval_ = fatherval;
        QVector<QWidget*>wds;

        // 定义方式
        wds.append(_defineTypeCombo.propwid);
        int defineType = _defineTypeCombo.combobox->currentIndex();

        if (defineType == 0) // 各向分量
        {
            wds.append(inputWidgets[0].propwid);
            wds.append(inputWidgets[1].propwid);
            wds.append(inputWidgets[2].propwid);
            wds.append(combofunc.propwid);
            wds.append(combocoor.propwid);
        }
        else if (defineType == 1) // 独立赋值
        {
            wds.append(_tableRowWidget);
            wds.append(combofunc.propwid);
            wds.append(combocoor.propwid);
        }
        else // 文件模式 (defineType == 2)
        {
            wds.append(_fileRowWidget);
        }
        return wds;
    }


    void mLoadTorquePanel::applyAllowedPickTypes(int way)
    {
        if (!_pickWidgetController) return;
        if (way == 2) // 文件定义：禁用全部拾取
        {
            _pickWidgetController->setAllPickTypesDisabled(true);
        }
        else if (way == 1) // 独立赋值：仅设置白名单记录，不切换视口过滤器
                           // （拾取过滤器由离散数据控件在开始拾取时自行激活/恢复）
        {
            _pickWidgetController->setAllowedTypes(&pickOnlyNode);
        }
        else // 各向分量(0)：几何+节点全支持
        {
            _pickWidgetController->setAllowedTypes(&pickGeoPointEdgeFaceSolidNodePart);
            _pickWidgetController->setPickType();
        }
    }

    void mLoadTorquePanel::deactivatePickAndCommit()
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
    }

    void mLoadTorquePanel::updateLoadSymbolRealtime()
    {
        if (!_pickWidgetController || !_currentLoad)
            return;
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        // 定义方式：各向分量(0)用集合展开+坐标计算；独立赋值(1)按表格数据逐节点取坐标；文件(2)无符号。
        const int way = _defineTypeCombo.combobox->currentIndex();

        // 独立赋值：先按离散数据控件当前行同步 nodeForceMap_（数据源为表格，
        // 通用拾取控制器已不再接收视口拾取，不能再以它为准），再合成方向。
        if (way == 1)
        {
            updateOpenedTable();
        }

        // 1. 汇总当前数据：拾取(pickedData) + 集合(selectedSetNames 按名称展开），合并为按类型分组的 map。
        const PickWidgetData allData = _pickWidgetController->getAllData();
        auto meshData = getCVM->getModelData()->getMeshData();
        QVector<QVector3D> objs;

        // 集合展开与作用对象坐标计算仅"各向分量(0)"需要；
        // 独立赋值(1)按表格数据逐节点取坐标、文件(2)无符号，跳过该段全量计算。
        if (way == 0)
        {
            std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> merged = allData.pickedData;

            auto& setManager = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set);
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
                    for (const auto& inner : mixSet->getMixSetData())
                    {
                        auto& vec = merged[inner.first];
                        vec.insert(vec.end(), inner.second.begin(), inner.second.end());
                    }
                    continue;
                }
                switch (setBase->getSetType())
                {
                case MxFunctions::Node:
                    if (auto s = dynamic_cast<mNode*>(setBase)) { auto v = s->getNodeData(); auto& d = merged[PickDataType::Node]; d.insert(d.end(), v.begin(), v.end()); }
                    break;
                case MxFunctions::Element:
                    if (auto s = dynamic_cast<mElementSet*>(setBase)) { auto v = s->getElementSetData(); auto& d = merged[PickDataType::Element]; d.insert(d.end(), v.begin(), v.end()); }
                    break;
                case MxFunctions::ElementSurface:
                    if (auto s = dynamic_cast<mElementSurface*>(setBase)) { auto v = s->getElementSurfaceData(); auto& d = merged[PickDataType::ElementFace]; d.insert(d.end(), v.begin(), v.end()); }
                    break;
                case MxFunctions::GeoPoint:
                case MxFunctions::GeoEdge:
                case MxFunctions::GeoFace:
                case MxFunctions::GeoSolid:
                    if (auto s = dynamic_cast<mGeoSet*>(setBase)) appendIds(SetTypeToPickDataType(setBase->getSetType()), s->getGeoSetData(), merged);
                    break;
                default:
                    break;
                }
            }

            // 2. 按类型计算作用对象坐标。
            auto geoManager = getCVM->getModelData()->getGeoManager();
            auto geoData = getCVM->getModelData()->getGeoData();
            for (const auto& pair : merged)
            {
                std::vector<int> ids;
                ids.reserve(pair.second.size());
                for (const auto& d : pair.second)
                    ids.push_back(d.result_id);
                if (ids.empty()) continue;

                switch (pair.first)
                {
                case PickDataType::Node:
                case PickDataType::Element:
                case PickDataType::ElementFace:
                    for (int id : ids)
                    {
                        QVector3D pos = meshData->getVectorById(id);
                        if (!pos.isNull())
                            objs.push_back(pos);
                    }
                    break;
                case PickDataType::GeoPoint:
                {
                    auto points = geoManager->CalculateVertexAttr(ids);
                    for (auto p : points) objs.push_back(p.m_coor);
                    break;
                }
                case PickDataType::GeoEdge:
                {
                    QVector<int> pids; for (int id : ids) pids.append(id);
                    auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(1, pids);
                    for (auto c : coords) objs.push_back(c);
                    break;
                }
                case PickDataType::GeoFace:
                {
                    QVector<int> pids; for (int id : ids) pids.append(id);
                    auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                    for (auto c : coords) objs.push_back(c);
                    break;
                }
                case PickDataType::GeoSolid:
                {
                    QVector<int> pids; for (int id : ids) pids.append(id);
                    auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, pids);
                    for (auto c : coords) objs.push_back(c);
                    break;
                }
                default:
                    break;
                }
            }
        }

        // 3. 方向：根据当前定义方式从输入/数据实时求有效方向。
        //    各向分量(0)：从输入框合成方向；独立赋值(1)：从 nodeForceMap_ 各节点分量合成；
        //    文件(2)：无有效方向。
        QVector<std::pair<QVector3D, QVector3D>> pos_dir;

        if (way == 1) // 独立赋值：逐节点合成方向
        {
            // 独立赋值已剥离通用拾取：方向合成直接以离散数据控件行（nodeForceMap_ 已同步）为准，
            // 不再读取 _pickWidgetController->getAllData()（控制器不再接收视口拾取，数据已停滞）。
            for (const auto& entry : _currentLoad->nodeForceMap_)
            {
                const QVector<QVariant>& values = entry.second;
                QVector3D nodeDir(0, 0, 0);
                if (values.size() > 0 && values[0].isValid() && !values[0].isNull()) nodeDir.setX(values[0].toDouble());
                if (values.size() > 1 && values[1].isValid() && !values[1].isNull()) nodeDir.setY(values[1].toDouble());
                if (values.size() > 2 && values[2].isValid() && !values[2].isNull()) nodeDir.setZ(values[2].toDouble());
                if (nodeDir.isNull()) continue;
                nodeDir.normalize();
                QVector3D pos = meshData->getVectorById(entry.first.result_id);
                if (pos.isNull()) continue;
                pos_dir.push_back({ pos, nodeDir });
            }
        }
        else if (way == 0) // 各向分量：从输入框合成方向
        {
            double fx = inputWidgets[0].lineeditvalue->text().toDouble();
            double fy = inputWidgets[1].lineeditvalue->text().toDouble();
            double fz = inputWidgets[2].lineeditvalue->text().toDouble();
            QVector3D dir(fx, fy, fz);
            if (!dir.isNull() && !objs.isEmpty())
            {
                dir.normalize();
                pos_dir.reserve(objs.size());
                for (const auto& o : objs)
                    pos_dir.push_back({ o, dir });
            }
        }
        // way==2 文件：pos_dir为空，清除箭头

        // 4. 重画该载荷的力矩符号（key=load_<id>，同名覆盖）。
        AllRenderSymbolManager::getInstance()->assignLoad(
            _currentLoad, pos_dir, "Load_Torque", QVector3D(255.0f, 165.0f, 0.0f));

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (symbolController) symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    int mLoadTorquePanel::getFunc()
    {
        int funcid = -1;
        QString funcname = combofunc.combobox->currentText();
        if (_functable.find(funcname) != _functable.end())
        {
            funcid = _functable[funcname];
        }
        return funcid;
    }

    int mLoadTorquePanel::getCoor()
    {
        return getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getIDByCoordinateName(combocoor.combobox->currentText());
    }
}