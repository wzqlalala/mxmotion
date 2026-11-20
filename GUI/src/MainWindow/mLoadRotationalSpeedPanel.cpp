#include "mMechGlobalSignals.h"
#include <QStyledItemDelegate>
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mSetManager.h"
#include "mMapRelationEnum.h"
#include "mMapRelationManager.h"
#include "mUnitSystemManager.h"
#include "mInfoWindow.h"
#include "PickCommonFunction.h"
#include "mWorkingConditions.h"

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

#include "mLoadRotationalSpeedPanel.h"
#include "mDockLayoutManager.h"

using namespace MechPreData;
namespace MechMainWindow
{
    const QString mLoadRotationalSpeedPanel::s_dirArrowRenderKey = QStringLiteral("rotate_speed_dir_arrow");
    const QVector3D mLoadRotationalSpeedPanel::s_dirArrowColor = QVector3D(19.0f, 133.0f, 246.0f);

    mLoadRotationalSpeedPanel::mLoadRotationalSpeedPanel(QObject* parent)
    {
        mLoadRotationalSpeed_ = new mLoadRotationalSpeed();
        initInputs();
    }

    mLoadRotationalSpeedPanel::~mLoadRotationalSpeedPanel()
    {
        if (_tableWidget)
        {
            delete _tableWidget;
            _tableWidget = nullptr;
        }
    }

    void mLoadRotationalSpeedPanel::initInputs()
    {
        // 方向选择控件：与旋转体力面板保持一致，构造时先挂到可用父窗口，
        // 避免无父窗口导致首次显示前短暂作为顶层窗口闪烁。
        _directionWidget = new mDirectionSelectorWidget(
            QStringLiteral("方向"), false, QString(), false, QString(), false, faval_ ? faval_->contentWidget() : nullptr);
        _directionWidget->setDirectionMode(mDirectionSelectorWidget::XAxis);
        _directionWidget->setDirectionCalculator(mDirectionSelectorWidget::DirectionCalculator());
        _directionWidget->setCoordinateCalculator(mDirectionSelectorWidget::CoordinateCalculator());

        connect(_directionWidget, &mDirectionSelectorWidget::directionChanged, this, [this]() {
            saveDirectionWidgetState();
            updateDirectionArrowRender();
            });
        connect(_directionWidget, &mDirectionSelectorWidget::layoutRefreshRequested, this, [this]() {
            if (faval_)
            {
                faval_->refreshLayout();
            }
            });

        // 旋转速度类型下拉框
        _speedCurveTypeCombo.initWidget("旋转速度");
        _speedCurveTypeCombo.combobox->addItem("常数");
        _speedCurveTypeCombo.combobox->addItem("曲线");
        _speedCurveTypeCombo.combobox->addItem("时间曲线");
        _speedCurveTypeCombo.combobox->setCurrentText("常数");
        connect(_speedCurveTypeCombo.combobox, static_cast<void(QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
            this, &mLoadRotationalSpeedPanel::slot_curveTypeChanged);

        // 表格按钮
        _tableBtn.initWidget("数据表格");
        _tableBtn.cb->setCheckable(false);
        _tableBtn.cb->setStyleSheet(R"(
                QCheckBox::indicator{
                    width:0px;
                    height:0px;
                }
            )");
        connect(_tableBtn.tablebtn, &QPushButton::clicked, this, &mLoadRotationalSpeedPanel::slot_roSpeedTable);

        initTable();

        singleComboBox combobox;
        combobox.initWidget("旋转部件");
        comboBoxs.append(combobox);

        if (getCVM == nullptr)
        {
            return;
        }
        QStringList partList = getCVM->getModelData()->getMeshData()->getAllPartNames();
        comboBoxs[0].combobox->addItem("全部部件");
        comboBoxs[0].combobox->addItems(partList);
        comboBoxs[0].combobox->setCurrentText("全部部件");

        //旋转部件
        connect(combobox.combobox, static_cast<void(QComboBox::*)(const QString &)>(&QComboBox::currentTextChanged),
            this, &mLoadRotationalSpeedPanel::slot_roPartChanged);
        combobox.combobox->blockSignals(true);
        combobox.combobox->setCurrentIndex(0);
        mLoadRotationalSpeed_->_rotationalPart = combobox.combobox->currentText();
        combobox.combobox->blockSignals(false);

        // 默认常数类型：初始化一份固定1行的表格数据缓存
        // 表格中不再输入起始点/方向点，起始点和方向点统一从面板上的拾取控件获取
        _constantTableHeaders = QStringList{ "旋转速度" };
        _constantTableData = { QStringList{ "" } };
        restoreTableDataByType(0);
        applyTableWindowStyleByType(0);
    }

    void mLoadRotationalSpeedPanel::initTable()
    {
        if (_tableWidget)
        {
            delete _tableWidget;
        }
        _tableWidget = new RotateTableWidget();
        connect(_tableWidget, &RotateTableWidget::dataChanged, this, [this]() {
            saveCurrentTableData();
            mLoadRotationalSpeed_->_tableData = _tableWidget->getTableData();
            mLoadRotationalSpeed_->_tableHeaders = _tableWidget->getColumnHeaders();
            notifyRotateAnalyseChanged();
            });
        connect(_tableWidget, &RotateTableWidget::rowAdded, this, [this](int row) {
            notifyRotateAnalyseChanged();
            });
        connect(_tableWidget, &RotateTableWidget::rowRemoved, this, [this](int row) {
            notifyRotateAnalyseChanged();
            });
        // 初始化表头以匹配当前曲线类型
        updateTableHeaders();
    }

    void mLoadRotationalSpeedPanel::updateTableHeaders()
    {
        if (!_tableWidget)
        {
            return;
        }
        int curveType = mLoadRotationalSpeed_->_speedCurveType;
        QStringList headers;
        if (curveType == 2)
        {
            headers = QStringList{ "时间", "旋转速度" };
        }
        else
        {
            headers = QStringList{ "旋转速度" };
        }
        _tableWidget->setColumnHeaders(headers);

        // 同步表头到当前类型缓存
        if (curveType == 0)
            _constantTableHeaders = headers;
        else if (curveType == 1)
            _curveTableHeaders = headers;
        else if (curveType == 2)
            _timeCurveTableHeaders = headers;
    }

    int mLoadRotationalSpeedPanel::curveTypeFromText(const QString& text) const
    {
        if (text == "曲线")
            return 1;
        if (text == "时间曲线")
            return 2;
        return 0; // 常数
    }

    QString mLoadRotationalSpeedPanel::textFromCurveType(int curveType) const
    {
        if (curveType == 1)
            return "曲线";
        if (curveType == 2)
            return "时间曲线";
        return "常数";
    }

    void mLoadRotationalSpeedPanel::saveTableDataByCurveType(int curveType)
    {
        if (curveType == 0)
        {
            saveDefaultTableData();
        }
        else
        {
            saveCurrentTableData();
        }
    }

    void mLoadRotationalSpeedPanel::restoreTableDataByCurveType(int curveType)
    {
        updateTableHeaders();
        if (curveType == 0)
        {
            restoreDefaultTableData();
        }
        else
            if (_tableWidget)
        {
            restoreTableDataByType(curveType);
        }
    }

    void mLoadRotationalSpeedPanel::saveCurrentTableData()
    {
        if (!_tableWidget)
            return;

        int curveType = mLoadRotationalSpeed_->_speedCurveType;
        QStringList data = _tableWidget->getColumnHeaders();
        if (curveType == 0)
        {
            _constantTableHeaders = data;
            _constantTableData = _tableWidget->getTableData();
        }
        else if (curveType == 1)
        {
            _curveTableHeaders = data;
            _curveTableData = _tableWidget->getTableData();
        }
        else if (curveType == 2)
        {
            _timeCurveTableHeaders = data;
            _timeCurveTableData = _tableWidget->getTableData();
        }
    }

    void mLoadRotationalSpeedPanel::applyTableWindowStyleByType(int curveType)
    {
        auto table = mTableDataWindow::getInstance()->getTableWidget();
        if (curveType == 0)
        {
            table->setSizeMode(mTableWidget::SizeMode::FixedSize);
            table->setFixedRow(1);
            // 表头由 restoreDefaultTableData 恢复，避免覆盖用户数据
            mTableDataWindow::getInstance()->setUseCurve(false);
        }
        else
        {
            table->setSizeMode(mTableWidget::SizeMode::DynamicSize);
            mTableDataWindow::getInstance()->setUseCurve(true);
        }
    }

    void mLoadRotationalSpeedPanel::saveDefaultTableData()
    {
        auto table = mTableDataWindow::getInstance()->getTableWidget();
        if (!table)
            return;

        // 仅保存处于常数模式时默认表格的数据，避免在曲线/时间曲线模式下清空缓存
        int rows = table->rowCount();
        int cols = table->columnCount();
        QList<QStringList> data;
        for (int r = 0; r < rows; ++r)
        {
            QStringList row;
            for (int c = 0; c < cols; ++c)
            {
                row << table->cellText(r, c);
            }
            data << row;
        }
        _constantTableData = data;

        QStringList headers = mTableDataWindow::getInstance()->getTableHeader();
        if (!headers.isEmpty())
        {
            _constantTableHeaders = headers;
        }
    }

    void mLoadRotationalSpeedPanel::restoreDefaultTableData()
    {
        auto table = mTableDataWindow::getInstance()->getTableWidget();
        if (!table)
            return;

        int cols = _constantTableHeaders.size();
        if (cols <= 0)
        {
            cols = 1;
            _constantTableHeaders = QStringList{ "旋转速度" };
        }

        table->clearAndReset(1, cols);
        table->setHorizontalHeaders(_constantTableHeaders);

        // 统一列宽到 220
        for (int c = 0; c < _constantTableHeaders.size(); ++c)
        {
            table->setColumnWidth(c, 220);
        }

        // 所有列均只允许输入实数
        for (int c = 0; c < _constantTableHeaders.size(); ++c)
        {
            table->setColumnValidation(c, mTableDataWindow::ColumnValidationType::Real);
        }

        if (!_constantTableData.isEmpty() && !_constantTableData[0].isEmpty())
        {
            table->blockSignals(true);
            for (int c = 0; c < _constantTableHeaders.size() && c < _constantTableData[0].size(); ++c)
            {
                table->setCellText(0, c, _constantTableData[0][c]);
            }
            table->blockSignals(false);
        }
    }

    void mLoadRotationalSpeedPanel::restoreTableDataByType(int curveType)
    {
        if (!_tableWidget)
            return;

        QStringList headers;
        QList<QStringList> data;
        if (curveType == 0)
        {
            headers = _constantTableHeaders.isEmpty() ? QStringList{ "旋转速度" } : _constantTableHeaders;
            data = _constantTableData.isEmpty() ? QList<QStringList>{ QStringList{ "" } } : _constantTableData;
        }
        else if (curveType == 1)
        {
            headers = _curveTableHeaders.isEmpty() ? QStringList{ "旋转速度" } : _curveTableHeaders;
            data = _curveTableData;
        }
        else if (curveType == 2)
        {
            headers = _timeCurveTableHeaders.isEmpty() ? QStringList{ "时间", "旋转速度" } : _timeCurveTableHeaders;
            data = _timeCurveTableData;
        }

        _tableWidget->setColumnHeaders(headers);
        _tableWidget->setTableData(data);
        // 所有列均只允许输入实数
        _tableWidget->setAllColumnsRealValidation();
    }

    void mLoadRotationalSpeedPanel::clearInputs()
    {
        if (_defaultTableConn)
        {
            disconnect(_defaultTableConn);
            _defaultTableConn = QMetaObject::Connection();
        }
        if (_tableWidget)
        {
            delete _tableWidget;
            _tableWidget = nullptr;
        }
        if (_directionWidget)
        {
            _directionWidget->clearAllHighlight();
            delete _directionWidget;
            _directionWidget = nullptr;
        }
        comboBoxs[0].clear();
        _speedCurveTypeCombo.clear();
    }

    void mLoadRotationalSpeedPanel::saveDirectionWidgetState()
    {
        if (!mLoadRotationalSpeed_ || !_directionWidget)
            return;

        QVariant dirVar = _directionWidget->getDirection();
        QVector3D dir = dirVar.isValid() && !dirVar.isNull() ? dirVar.value<QVector3D>() : QVector3D(1, 0, 0);
        if (dir.isNull())
            dir = QVector3D(1, 0, 0);

        // 自定义坐标模式下使用用户设置的实际起点；其余模式沿用原点作为旋转轴参考点
        if (_directionWidget->directionMode() == mDirectionSelectorWidget::CustomCoordinate)
        {
            mLoadRotationalSpeed_->_startPoint1 = _directionWidget->startPoint();
        }
        else
        {
            mLoadRotationalSpeed_->_startPoint1 = QVector3D(0, 0, 0);
        }
        mLoadRotationalSpeed_->_directPoint2 = mLoadRotationalSpeed_->_startPoint1 + dir;

        // 保存方向控件完整 UI 状态，用于后续反显
        auto& state = mLoadRotationalSpeed_->_dirPickState;
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
            state.directInputDir = dir;
        }
    }

    void mLoadRotationalSpeedPanel::updateDirectionArrowRender()
    {
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (!symbolController)
            return;

        symbolController->deleteSymbol({ s_dirArrowRenderKey });

        if (!mLoadRotationalSpeed_ || !_directionWidget)
            return;

        QVariant dirVar = _directionWidget->getDirection();
        QVector3D dir = dirVar.isValid() && !dirVar.isNull() ? dirVar.value<QVector3D>() : QVector3D(1, 0, 0);
        if (dir.lengthSquared() < 1e-12f)
            return;

        // 方向箭头：尾部位于起始点，头部指向当前方向。
        // 起点使用持久化的旋转轴起始点 _startPoint1；
        // 方向直接从方向控件获取，避免 saveDirectionWidgetState 更新 _directPoint2 之前出现延迟一帧的问题。
        QVector3D startPt = mLoadRotationalSpeed_->_startPoint1;
        QVector<std::pair<QVector3D, QVector3D>> pos_dir;
        pos_dir.push_back({ startPt, dir });

        const auto& arrow = symbolController->getOrCreateSymbolData<mxr::Arrow_Font_Load_Implicit>(s_dirArrowRenderKey);
        arrow->_pos_dir = pos_dir;
        arrow->_txt = "";
        arrow->_color = s_dirArrowColor;
        arrow->_depthState = false;
        arrow->_isFixedPixel = false;
        arrow->_size = 30.0;
        symbolController->createSymbol(arrow);

        symbolController->symbolSelfAdaptiveSize(getCVM->getPreRenderWidget(), { s_dirArrowRenderKey }, 0.05);

        if (symbolController) symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mLoadRotationalSpeedPanel::syncCurveTypeFromData()
    {
        QString curveText = textFromCurveType(mLoadRotationalSpeed_->_speedCurveType);
        _speedCurveTypeCombo.combobox->blockSignals(true);
        if (_speedCurveTypeCombo.combobox->findText(curveText) >= 0)
            _speedCurveTypeCombo.combobox->setCurrentText(curveText);
        else
            _speedCurveTypeCombo.combobox->setCurrentText("常数");
        _speedCurveTypeCombo.combobox->blockSignals(false);
    }

    void mLoadRotationalSpeedPanel::trimLegacyTableCache(QStringList& headers, QList<QStringList>& data, int keepCols)
    {
        // 旧格式表头含有“起始点/方向点”列，新表格只保留前 keepCols 列数据
        if (headers.size() > keepCols)
        {
            headers = headers.mid(0, keepCols);
        }
        for (QStringList& row : data)
        {
            if (row.size() > keepCols)
            {
                row = row.mid(0, keepCols);
            }
        }
    }

    void mLoadRotationalSpeedPanel::setInputsValue(std::shared_ptr<MechCaseData::mRoatateAnalyseBase> rotateData)
    {
        // 在重新加载数据前，先保存当前模式下的表格数据，防止覆盖外部数据
        int prevType = mLoadRotationalSpeed_->_speedCurveType;
        saveTableDataByCurveType(prevType);

        //设置延迟时间数据
        std::shared_ptr<MechCaseData::mLoadRotationalSpeed> rotSpeedPtr
            = std::dynamic_pointer_cast<MechCaseData::mLoadRotationalSpeed>(rotateData);
        if (!rotSpeedPtr)
        {
            return;
        }

        // 先把外部数据同步到面板内部对象，否则后续 getValueInputs 会基于旧值覆盖显示
        mLoadRotationalSpeed_->_startPoint1 = rotSpeedPtr->_startPoint1;
        mLoadRotationalSpeed_->_directPoint2 = rotSpeedPtr->_directPoint2;
        mLoadRotationalSpeed_->_rotationalPart = rotSpeedPtr->_rotationalPart;
        mLoadRotationalSpeed_->way_ = rotSpeedPtr->way_;
        mLoadRotationalSpeed_->_speedCurveType = rotSpeedPtr->_speedCurveType;
        mLoadRotationalSpeed_->_tableHeaders = rotSpeedPtr->_tableHeaders;
        mLoadRotationalSpeed_->_tableData = rotSpeedPtr->_tableData;
        mLoadRotationalSpeed_->_dirPickState = rotSpeedPtr->_dirPickState;

        // 加载各类型独立缓存
        _constantTableHeaders = rotSpeedPtr->_constantTableHeaders;
        _constantTableData = rotSpeedPtr->_constantTableData;
        _curveTableHeaders = rotSpeedPtr->_curveTableHeaders;
        _curveTableData = rotSpeedPtr->_curveTableData;
        _timeCurveTableHeaders = rotSpeedPtr->_timeCurveTableHeaders;
        _timeCurveTableData = rotSpeedPtr->_timeCurveTableData;

        // 兼容旧数据：裁剪旧格式中的“起始点/方向点”列，仅保留数据列
        trimLegacyTableCache(_constantTableHeaders, _constantTableData, 1);
        trimLegacyTableCache(_curveTableHeaders, _curveTableData, 1);
        trimLegacyTableCache(_timeCurveTableHeaders, _timeCurveTableData, 2);
        trimLegacyTableCache(mLoadRotationalSpeed_->_tableHeaders, mLoadRotationalSpeed_->_tableData,
            mLoadRotationalSpeed_->_speedCurveType == 2 ? 2 : 1);

        // 兼容旧数据：如果新缓存为空但当前 _tableData 有数据，则按当前类型填充对应缓存
        int loadType = mLoadRotationalSpeed_->_speedCurveType;
        if (loadType == 0 && _constantTableData.isEmpty())
        {
            _constantTableHeaders = mLoadRotationalSpeed_->_tableHeaders;
            _constantTableData = mLoadRotationalSpeed_->_tableData;
        }
        else if (loadType == 1 && _curveTableData.isEmpty())
        {
            _curveTableHeaders = mLoadRotationalSpeed_->_tableHeaders;
            _curveTableData = mLoadRotationalSpeed_->_tableData;
        }
        else if (loadType == 2 && _timeCurveTableData.isEmpty())
        {
            _timeCurveTableHeaders = mLoadRotationalSpeed_->_tableHeaders;
            _timeCurveTableData = mLoadRotationalSpeed_->_tableData;
        }

        // 设置旋转速度类型下拉框（根据 _speedCurveType 回显）
        syncCurveTypeFromData();

        //slot_curveTypeChanged(_speedCurveTypeCombo.combobox->currentText());
        restoreTableDataByCurveType(loadType);
        applyTableWindowStyleByType(loadType);
        slot_roSpeedTable();

        // 数据反显完成后刷新一次方向箭头渲染
        // 直接调用时面板尚未完成布局/显示，渲染上下文可能未就绪，
        // 使用 QueuedConnection 延迟到事件循环下一阶段执行，确保首次进入面板也能正确渲染。
        QMetaObject::invokeMethod(this, &mLoadRotationalSpeedPanel::updateDirectionArrowRender, Qt::QueuedConnection);

        // 方向控件反显
        if (_directionWidget)
        {
            bool oldSignalsBlocked = _directionWidget->blockSignals(true);
            _directionWidget->resetToDefault();

            const auto& state = mLoadRotationalSpeed_->_dirPickState;
            _directionWidget->setDirectionMode(
                static_cast<mDirectionSelectorWidget::DirectionMode>(state.mode));
            _directionWidget->setReversed(state.reversed);

            if (state.mode == mDirectionSelectorWidget::CustomCoordinate)
            {
                // 优先用持久化的 _startPoint1/_directPoint2 反推起点终点，
                // 避免 state 中的 QVariant 失效后落到兜底分支被重置为 X 轴。
                QVector3D startPt = mLoadRotationalSpeed_->_startPoint1;
                QVector3D endPt = mLoadRotationalSpeed_->_directPoint2;
                if (state.startPoint.isValid() && !state.startPoint.isNull())
                    startPt = state.startPoint.value<QVector3D>();
                if (state.endPoint.isValid() && !state.endPoint.isNull())
                    endPt = state.endPoint.value<QVector3D>();
                _directionWidget->setCustomCoordinatePoints(startPt, endPt);
            }
            else if (state.mode == mDirectionSelectorWidget::SelectObject)
            {
                PickWidgetData displayData;
                if (state.selectedObject.second.result_id > 0)
                {
                    displayData.pickedData[state.selectedObject.first].push_back(state.selectedObject.second);
                }
                _directionWidget->objectPickController()->displayData(displayData);
            }
            else if (state.mode == mDirectionSelectorWidget::DirectInput)
            {
                _directionWidget->setDirection(state.directInputDir);
            }
            // 坐标轴/默认方向：兜底，用分解后的方向点推断
            else if (!mLoadRotationalSpeed_->_directPoint2.isNull())
            {
                QVector3D dir = mLoadRotationalSpeed_->_directPoint2;
                if (!dir.isNull())
                {
                    dir.normalize();
                    if (state.mode == mDirectionSelectorWidget::XAxis ||
                        state.mode == mDirectionSelectorWidget::YAxis ||
                        state.mode == mDirectionSelectorWidget::ZAxis)
                    {
                        _directionWidget->setReversed(dir[static_cast<int>(state.mode) - 1] < 0);
                    }
                    else
                    {
                        _directionWidget->setDirectionMode(mDirectionSelectorWidget::XAxis);
                        _directionWidget->setReversed(dir.x() < 0);
                    }
                }
            }
            else
            {
                _directionWidget->setDirectionMode(mDirectionSelectorWidget::XAxis);
            }

            _directionWidget->blockSignals(oldSignalsBlocked);
        }

        // 设置旋转部件下拉框
        if (getCVM == nullptr)
        {
            return;
        }
        QStringList partList = getCVM->getModelData()->getMeshData()->getAllPartNames();
        comboBoxs[0].combobox->blockSignals(true);
        comboBoxs[0].combobox->clear();
        comboBoxs[0].combobox->addItem("全部部件");
        comboBoxs[0].combobox->addItems(partList);
        if (partList.contains(mLoadRotationalSpeed_->_rotationalPart))
            comboBoxs[0].combobox->setCurrentText(mLoadRotationalSpeed_->_rotationalPart);
        else
            comboBoxs[0].combobox->setCurrentText("全部部件");
        comboBoxs[0].combobox->blockSignals(false);

        // 方向状态保存与箭头渲染统一通过 QueuedConnection 延迟执行，
        // 与旋转体力面板保持一致，避免在面板尚未完成布局/显示时直接渲染。
    }

    QVector<QWidget*> mLoadRotationalSpeedPanel::getValueInputs(FoldSection* fatherval)
    {
        faval_ = fatherval;
        // 首次获得折叠块时，将方向控件父窗口切到折叠内容页，避免无父窗口或旧父被释放后短暂顶层显示
        if (_directionWidget && _directionWidget->parentWidget() != faval_->contentWidget())
        {
            _directionWidget->setParent(faval_->contentWidget());
        }

        QVector<QWidget*> wds;
        wds.append(_directionWidget);
        
        //wds.append(_tableBtn.propwid);
        wds.append(comboBoxs[0].propwid);
        wds.append(_speedCurveTypeCombo.propwid);

        // 同步当前类型下拉框（根据 _speedCurveType 回显）
        syncCurveTypeFromData();

        int curveType = mLoadRotationalSpeed_->_speedCurveType;
        //slot_curveTypeChanged(_speedCurveTypeCombo.combobox->currentText());
        restoreTableDataByCurveType(curveType);
        applyTableWindowStyleByType(curveType);
        slot_roSpeedTable();

        QStringList partList = getCVM->getModelData()->getMeshData()->getAllPartNames();
        comboBoxs[0].combobox->blockSignals(true);
        comboBoxs[0].combobox->clear();
        comboBoxs[0].combobox->addItem("全部部件");
        comboBoxs[0].combobox->addItems(partList);
        
        if (partList.contains(mLoadRotationalSpeed_->_rotationalPart))
            comboBoxs[0].combobox->setCurrentText(mLoadRotationalSpeed_->_rotationalPart);
        else
            comboBoxs[0].combobox->setCurrentText("全部部件");
        comboBoxs[0].combobox->blockSignals(false);

        saveDirectionWidgetState();
        // 延迟到事件循环下一阶段再渲染，确保方向箭头在面板布局完成后才刷新
        QMetaObject::invokeMethod(this, &mLoadRotationalSpeedPanel::updateDirectionArrowRender, Qt::QueuedConnection);

        return wds;
    }

    QVector<QWidget*> mLoadRotationalSpeedPanel::getTargetInputs(FoldSection* fathertar)
    {
        fatar_ = fathertar;
        QVector<QWidget*>wds;
        return wds;
    }

    void mLoadRotationalSpeedPanel::storeLoadValue(std::shared_ptr<MechCaseData::mLoadRotationalSpeed> load)
    {
        int curveType = mLoadRotationalSpeed_->_speedCurveType;

        // 按当前类型保存：常数模式用默认 mTableWidget，其它用自定义表格
        if (curveType == 0)
        {
            saveDefaultTableData();
        }
        else
        {
            saveCurrentTableData();
        }

        // 从方向控件同步起点/方向点到数据对象
        saveDirectionWidgetState();

        load->_directPoint2 = mLoadRotationalSpeed_->_directPoint2;
        load->_startPoint1 = mLoadRotationalSpeed_->_startPoint1;
        load->_rotationalPart = mLoadRotationalSpeed_->_rotationalPart;
        load->_speedCurveType = curveType;

        // 保存当前类型的表格数据到旧字段，保持兼容性
        if (curveType == 0)
        {
            load->_tableHeaders = _constantTableHeaders;
            load->_tableData = _constantTableData;
        }
        else if (curveType == 1)
        {
            load->_tableHeaders = _curveTableHeaders;
            load->_tableData = _curveTableData;
        }
        else if (curveType == 2)
        {
            load->_tableHeaders = _timeCurveTableHeaders;
            load->_tableData = _timeCurveTableData;
        }

        // 保存各类型独立缓存
        load->_constantTableHeaders = _constantTableHeaders;
        load->_constantTableData = _constantTableData;
        load->_curveTableHeaders = _curveTableHeaders;
        load->_curveTableData = _curveTableData;
        load->_timeCurveTableHeaders = _timeCurveTableHeaders;
        load->_timeCurveTableData = _timeCurveTableData;

        // 保存方向控件 UI 状态
        load->_dirPickState = mLoadRotationalSpeed_->_dirPickState;
    }

    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mLoadRotationalSpeedPanel::getPickedData()
    {
        return _picks;
    }

    void mLoadRotationalSpeedPanel::slot_roPartChanged(QString partName)
    {
        mLoadRotationalSpeed_->_rotationalPart = partName;
        notifyRotateAnalyseChanged();
    }

    void mLoadRotationalSpeedPanel::slot_curveTypeChanged(QString text)
    {
        int oldType = mLoadRotationalSpeed_->_speedCurveType;
        int newType = curveTypeFromText(text);


        // 1. 保存旧类型的表格数据
        saveTableDataByCurveType(oldType);

        // 2. 更新当前类型
        mLoadRotationalSpeed_->_speedCurveType = newType;

        // 3. 恢复新类型的表格数据
        restoreTableDataByCurveType(newType);

        // 4. 根据类型设置表格窗口风格
        applyTableWindowStyleByType(newType);

        // 5. 同步当前公共字段
        if (newType == 0)
        {
            mLoadRotationalSpeed_->_tableHeaders = _constantTableHeaders;
            mLoadRotationalSpeed_->_tableData = _constantTableData;
        }
        else
        {
            mLoadRotationalSpeed_->_tableHeaders = _tableWidget->getColumnHeaders();
            mLoadRotationalSpeed_->_tableData = _tableWidget->getTableData();
        }


        slot_roSpeedTable();
        notifyRotateAnalyseChanged();
    }

    void mLoadRotationalSpeedPanel::slot_roSpeedTable(bool connFlag)
    {
        int curveType = mLoadRotationalSpeed_->_speedCurveType;
        applyTableWindowStyleByType(curveType);

        auto tableWin = mTableDataWindow::getInstance();
        if (curveType == 0)
        {
            // 常数模式：使用固定 1 行的默认 mTableWidget
            tableWin->resetInitialTable();

            tableWin->getTableWidget()->setFixedRow(1);
            tableWin->getTableWidget()->setSizeMode(mTableWidget::SizeMode::FixedSize);
            mTableDataWindow::getInstance()->setUseCurve(false);

            restoreDefaultTableData();

            if (_defaultTableConn)
            {
                disconnect(_defaultTableConn);
            }
            if (connFlag == true)
            {
                _defaultTableConn = connect(tableWin->getTableWidget(), &mTableWidget::cellDataUpdated,
                    this, [this](int row, int col, const QString& text) {
                        saveDefaultTableData();
                        mLoadRotationalSpeed_->_tableHeaders = _constantTableHeaders;
                        mLoadRotationalSpeed_->_tableData = _constantTableData;
                        notifyRotateAnalyseChanged();
                    });
            }
        }
        else
        {
            // 曲线/时间曲线模式：使用自定义 RotateTableWidget
            if (_defaultTableConn)
            {
                disconnect(_defaultTableConn);
            }
            tableWin->resetInitialTable();
            tableWin->setCustomWidget(_tableWidget);
        }

        mDockLayoutManager::getInstance()->activateDock("Table");
    }

    void mLoadRotationalSpeedPanel::notifyRotateAnalyseChanged()
    {
        if (!mLoadRotationalSpeed_)
        {
            return;
        }
        // 旋转分析数据由面板内部对象实时维护，这里仅通知外部刷新树/渲染
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateRotateAnalyseSig(false, mLoadRotationalSpeed_->getRotateAnalyseId());
    }

    int mLoadRotationalSpeedPanel::getLoadBoundId()
    {
        int loadboundid = -1;
        QString loadboundname = comboBoxs[0].combobox->currentText();
        if (_loadboundtable.find(loadboundname) != _loadboundtable.end())
        {
            loadboundid = _loadboundtable[loadboundname];
        }
        return loadboundid;
    }
}
