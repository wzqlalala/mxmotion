#include "mPostCurve.h"

#include <QFormLayout>
#include <QTableWidget>
#include <QFileDialog>
#include <QTextCodec>
#include <cmath>

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mDockLayoutManager.h"
#include "mAppConfiguration.h"

#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "ZsTempRender.h"

//PlotView
#include "mCurveWindow.h"
#include "mPlotView.h"
#include "mPlotViewStyleWidget.h"

//MxFunctions
#include "mGlobalSignals.h"

//BasicData
#include "mPhysicalFieldEnum.h"

//post
#include "mIMxdbFile1.h"
#include "mMxdbThread.h"
#include "mPostCurveData.h"
#include "mPostDataTable.h"

//窗口
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mOtherFunction.h"

using namespace MxFunctions;
using namespace MechCommon;
using namespace PostData;
using namespace MechPreData;
using namespace MIOFile;

namespace PostWindow
{
    mPostCurvePanel::mPostCurvePanel(QWidget* parent) : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);
        _mainLayout->setContentsMargins(2, 0, 2, 0);

        /********************类型********************/
        _typeFold = new FoldSection("类型", this);

		//类型
		_comboCurveType = new QComboBox();
		_comboCurveType->addItems({ "时间-变量曲线" , "导入", "导入CSV文件" , "路径"});

        //拾取对象
        _widgetPick = new QWidget();
        QHBoxLayout* rowH = new QHBoxLayout(_widgetPick);
        rowH->setContentsMargins(0, 0, 0, 0);
        rowH->setSpacing(4);

        _labelObject = new QLabel("节点");
        rowH->addWidget(_labelObject);
        _labelSelectStatu = new QLabel("当前未选择");
        _labelSelectStatu->setStyleSheet("color: red; font-style: italic;");
        _labelSelectStatu->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        rowH->addWidget(_labelSelectStatu);

        _btnAdd = new QPushButton(); _btnAdd->setToolTip("添加");
        _btnReplace = new QPushButton(); _btnReplace->setToolTip("替换");
        _btnClear = new QPushButton(); _btnClear->setToolTip("清空");
        _btnAdd->setIcon(QIcon(":/mMainWidgets/Icon/post/tianjia.png"));
        _btnReplace->setIcon(QIcon(":/mMainWidgets/Icon/post/tihuan.png"));
        _btnClear->setIcon(QIcon(":/mMainWidgets/Icon/post/qingkong.png"));
        _btnAdd->setFixedWidth(30);
        _btnReplace->setFixedWidth(30);
        _btnClear->setFixedWidth(30);

        rowH->addWidget(_btnAdd);
        rowH->addWidget(_btnReplace);
        rowH->addWidget(_btnClear);

        //文件
        _labelFile = new QLabel("文件");
        _lineEditFile = new QLineEdit();
        _lineEditFile->setReadOnly(true);
        _lineEditFile->setStyleSheet(R"(
			QLineEdit[readOnly="true"] {
				background: white;      /* 背景色 */
				color: black;           /* 文字色 */
			}
		)");

        _btnFile = new QPushButton();
        _btnFile->setToolTip("选择文件");
        _btnFile->setIcon(QIcon(":/mMainWidgets/Icon/homepage/dakai3x"));
        _btnFile->setFixedWidth(30);

        //路径
        _labelPath = new QLabel("路径");
        _comboPath = new QComboBox();
        connect(_comboPath, QOverload<const QString&>::of(&QComboBox::currentIndexChanged),
            [=](const QString& text) {
                PostData::mPostCurveData* curveData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getCurveData(_curViewName);
                if (curveData == nullptr)
                    return;
                curveData->_curvePath = text;
                updateItemStatus();
                QVector<int> nodes = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getIdsByPathName(text);
                highlightPath(nodes);
            });

        _btnPath = new QPushButton(); _btnPath->setToolTip("创建路径");
        _btnPath->setIcon(QIcon(":/mMainWidgets/Icon/post/svg/lujing.svg"));
        _btnPath->setFixedWidth(30);
        connect(_btnPath, &QPushButton::clicked, this, [=]() {
            emit MechCommon::mMechGlobalSignals::getInstance()->clickPathSig();
            });

        _checkPath = new QCheckBox("包含相交点");
        connect(_checkPath, &QCheckBox::toggled, this, [=](bool checked) {
            PostData::mPostCurveData* curveData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getCurveData(_curViewName);
            if (curveData == nullptr)
                return;
            curveData->_curveIntersection = checked;
            updateItemStatus();
            });

        _typeFold->refreshLayout();
        m_foldWidgets.append(_typeFold);
        _mainLayout->addWidget(_typeFold);

        /********************X轴********************/
        _xFold = new FoldSection("X轴", this);

        //时间-变量曲线
        _labelData = new QLabel("数据");
        _labelDataType = new QLabel("时间");
        _labelDataType->setAlignment(Qt::AlignCenter);

        //路径
        _widgetPath = new QWidget();
        QList<QRadioButton*> radios;
        QStringList titles = { "X距离", "X坐标", "Y距离", "Y坐标", "Z距离", "Z坐标", "真实距离", "归一化距离" };
        for (int i = 0; i < 8; ++i) {
            radios << new QRadioButton(titles.value(i), _widgetPath);
        }
        _btnGroupPath = new QButtonGroup(_widgetPath);
        for (int i = 0; i < radios.size(); i++)
        {
            QRadioButton* btn = radios[i];
            _btnGroupPath->addButton(btn, i + 1);
        }
        _btnGroupPath->button(1)->setChecked(true);
        connect(_btnGroupPath, QOverload<int>::of(&QButtonGroup::buttonClicked),
            this, [=](int id) {
                PostData::mPostCurveData* curveData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getCurveData(_curViewName);
                if (curveData == nullptr)
                    return;
                curveData->_curveXID = id;
                updateItemStatus();
            });

        QGridLayout* layout = new QGridLayout(_widgetPath);
        int idx = 0;
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 2; ++col) {
                layout->addWidget(radios[idx++], row, col);
            }
        }
        _widgetPath->setLayout(layout);

        _xFold->refreshLayout();
        m_foldWidgets.append(_xFold);
        _mainLayout->addWidget(_xFold);

        /********************y轴********************/
        _yFold = new FoldSection("Y轴", this);
        _comboCase = new QComboBox();
        _comboFrame = new QComboBox();
        _comboVariable = new QComboBox();
        _btnAllCompo = new QPushButton("全选");
        _btnNoneCompo = new QPushButton("全不选");
        _btnReverseCompo = new QPushButton("反选");
        _btnAllCompo->setFixedWidth(50);
        _btnNoneCompo->setFixedWidth(50);
        _btnReverseCompo->setFixedWidth(50);
        _listComponent = new QListWidget();
        _listComponent->setFixedHeight(120);
        _listComponent->setSelectionMode(QAbstractItemView::MultiSelection);
        _btnAllLayer = new QPushButton("全选");
        _btnNoneLayer = new QPushButton("全不选");
        _btnReverseLayer = new QPushButton("反选");
        _btnAllLayer->setFixedWidth(50);
        _btnNoneLayer->setFixedWidth(50);
        _btnReverseLayer->setFixedWidth(50);
        _listLayer = new QListWidget();
        _listLayer->setFixedHeight(120);
        _listLayer->setSelectionMode(QAbstractItemView::MultiSelection);
        _checkReal = new QCheckBox("实部");
        _checkImag = new QCheckBox("虚部");
        _checkMag = new QCheckBox("幅值");
        _checkMag->setChecked(true);
        _checkPhase = new QCheckBox("相位");
        _boxList.append(_checkReal);
        _boxList.append(_checkImag);
        _boxList.append(_checkMag);
        _boxList.append(_checkPhase);

        _labelCase = new QLabel("工况");
        _labelFrame = new QLabel("帧");
        _yFold->refreshLayout();
        m_foldWidgets.append(_yFold);
        _mainLayout->addWidget(_yFold);

        _btnGenerate = new QPushButton("生成");
        _btnExport = new QPushButton("导出");
        QHBoxLayout* btnGE = new QHBoxLayout(this);
        btnGE->addWidget(_btnExport);
        btnGE->addWidget(_btnGenerate);
        QFrame* line = new QFrame(this);
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken); // 可选，也可以用 QFrame::Plain
        line->setFixedHeight(10); // 线的粗细（像素）
        _mainLayout->addWidget(line);
        _mainLayout->addLayout(btnGE);

        _mainLayout->addStretch();

        connect(_comboCurveType, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slot_chooseCurveType(const QString&)));
        connect(_comboCase, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slot_changeCaseName(const QString&)));
        connect(_comboFrame, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slot_chooseFrame(const QString&)));
        connect(_comboVariable, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slot_chooseVariable(const QString&)));
        connect(_btnAllCompo,&QPushButton::clicked,this,&mPostCurvePanel::slot_selectAll);
        connect(_btnAllLayer, &QPushButton::clicked, this, &mPostCurvePanel::slot_selectAll);
        connect(_btnNoneCompo, &QPushButton::clicked, this, &mPostCurvePanel::slot_selectNone);
        connect(_btnNoneLayer, &QPushButton::clicked, this, &mPostCurvePanel::slot_selectNone);
        connect(_btnReverseCompo, &QPushButton::clicked, this, &mPostCurvePanel::slot_selectReverse);
        connect(_btnReverseLayer, &QPushButton::clicked, this, &mPostCurvePanel::slot_selectReverse);
        connect(_listComponent, &QListWidget::itemSelectionChanged, this, &mPostCurvePanel::slot_selectedCompoChanged);
        connect(_listLayer, &QListWidget::itemSelectionChanged, this, &mPostCurvePanel::slot_selectedLayerChanged);
        connect(_btnAdd, &QPushButton::clicked, this, &mPostCurvePanel::slot_addPick);
        connect(_btnReplace, &QPushButton::clicked, this, &mPostCurvePanel::slot_addPick);
        connect(_btnClear, &QPushButton::clicked, this, &mPostCurvePanel::slot_clearPick);
        connect(_btnGenerate, &QPushButton::clicked, this, &mPostCurvePanel::slot_generateCurve);
        connect(_btnExport, &QPushButton::clicked, this, &mPostCurvePanel::slot_exportCurve);
        connect(_checkReal, &QCheckBox::toggled, this, &mPostCurvePanel::slot_checkedChange);
        connect(_checkImag, &QCheckBox::toggled, this, &mPostCurvePanel::slot_checkedChange);
        connect(_checkMag, &QCheckBox::toggled, this, &mPostCurvePanel::slot_checkedChange);
        connect(_checkPhase, &QCheckBox::toggled, this, &mPostCurvePanel::slot_checkedChange);
        connect(_btnFile, &QPushButton::clicked, this, &mPostCurvePanel::slot_chooseCurveFile);
    }

    void mPostCurvePanel::editCurveForViewport(int caseid)
    {
        //视口绑定打开（用户定案 2026-11-04）：曲线不按次生成实例——数据键=当前激活
        //后处理视口名：同视口重复打开沿用既有配置，切换视口后打开自动跟随。
        //条目登记按视口名幂等（mPostCurveData 挂在 CurveInfo 条目上，无条目则
        //addCurveData 静默无效，故首开该视口时登记并初始化数据）
        auto* curPriv = VMManager->getCurrentViewportPrivate();
        if (curPriv == nullptr || curPriv->getViewportType() != MxFunctions::ViewportPost3D)
        {
            return;
        }
        const QString viewName = curPriv->getViewportName();
        if (viewName.isEmpty())
        {
            return;
        }

        initUI();

        _curViewName = viewName;

        auto& mgr = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post);
        if (!mgr.hasItemByViewName(ItemType::CurveItem, viewName))
        {
            int curveID = mgr.getItemId(ItemType::CurveItem);
            CurveInfo info;
            info.caseId = caseid;
            info.curveId = curveID;
            info.itemName = QString("曲线_%1").arg(curveID);
            info.viewName = viewName;
            mgr.addItem(ItemType::CurveItem, info);

            updateCurvePanel(caseid);
        }
        else
        {
            updateCurveGUI();
        }
    }

    void mPostCurvePanel::updateCurvePanel(int caseid)
    {
        _postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(caseid);
        if (_postDB == nullptr) return;

        QPointer<PostData::mPostCurveData> curveData = new PostData::mPostCurveData();
        getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).addCurveData(_curViewName, curveData);

        //获取所有变量
        QStringList vnames = _postDB->getAllVariableNames();
        _comboVariable->clear();
        _comboVariable->addItems(vnames);
        curveData->_curveVarList = vnames;
    }

    QStringList mPostCurvePanel::getCheckedComplex()
    {
        QStringList complexs{};
        if (_isComplex)
        {
            if (_checkReal->isChecked()) complexs.append("实部");
            if (_checkImag->isChecked()) complexs.append("虚部");
            if (_checkMag->isChecked()) complexs.append("幅值");
            if (_checkPhase->isChecked()) complexs.append("相位");
        }
        return complexs;
    }

    void mPostCurvePanel::getCurveDatas(QVector<double>& xValues, QVector<QVector<double>>& yValues, QStringList& headers)
    {
        if (_XValueD.size() > 0)
        {
            xValues = _XValueD;
        }
        else
        {
            for (float value : _XValueF)
            {
                xValues.append(static_cast<double>(value));
            }
        }

        QHash<QString, QVector<double>>	yval;
        if (_YValuesD.size() > 0)
        {
            yval = _YValuesD;
        }
        else
        {
            // 遍历源哈希表并进行转换
            for (auto it = _YValuesF.begin(); it != _YValuesF.end(); ++it) {
                const QString& key = it.key();
                const QVector<float>& floatVector = it.value();

                QVector<double> doubleVector;
                // 将 floatVector 中的元素逐个转换为 double 类型
                for (float value : floatVector) {
                    doubleVector.append(static_cast<double>(value));
                }
                yval.insert(key, doubleVector);
            }
        }

        for (int i = 0; i < _curveNames.size(); i++)
        {
            QString name = _curveNames[i];
            if (headers.contains(name)) continue;
            headers.append(name);
            yValues.append(yval.value(name));
        }

    }

    void mPostCurvePanel::setCurViewName(const QString& viewName)
    {
        _curViewName = viewName;
    }

    void mPostCurvePanel::updateCurveGUI()
    {
        PostData::mPostCurveData* curveData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getCurveData(_curViewName);
        if (curveData == nullptr)
            return;

        QStringList paths = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getAllPathNames();

        _comboCurveType->blockSignals(true);
        _comboPath->blockSignals(true);
        _comboVariable->blockSignals(true);
        _listComponent->blockSignals(true);
        _listLayer->blockSignals(true);
        _checkReal->blockSignals(true);
        _checkImag->blockSignals(true);
        _checkMag->blockSignals(true);
        _checkPhase->blockSignals(true);
        _lineEditFile->blockSignals(true);

        _comboCurveType->clear();
        _comboCurveType->addItems(curveData->_curveTypeList);
        _comboCurveType->setCurrentText(curveData->_curveType);

        _lineEditFile->clear();

        if (curveData->_curveType == "时间-变量曲线")
        {
            showTimeVarWidget();
            _labelObject->setText(curveData->_curveObject);
            _pickIds = curveData->_curvePickIds;
            if (curveData->_curvePickIds.size() > 0)
            {
                _labelSelectStatu->setText(QString("已选择%1个%2").arg(curveData->_curvePickIds.size()).arg(curveData->_curveObject));
                _labelSelectStatu->setStyleSheet("color: black; font-weight: bold;");
            }
            else
            {
                _labelSelectStatu->setText("当前未选择");
                _labelSelectStatu->setStyleSheet("color: red; font-style: italic;");
            }
            _labelDataType->setText(curveData->_curveDataType);

		}
		else if (curveData->_curveType == "导入")
		{
			showImportWidget();
			_lineEditFile->setText(curveData->_curveFilePath);
		}
        else if (curveData->_curveType == "导入CSV文件")
        {
            showImportWidget();
            _lineEditFile->setText(curveData->_curveFilePath);
        }
		else if (curveData->_curveType == "路径")
		{
			showPathWidget();
			_comboPath->clear();
			_comboPath->addItems(paths);

            if (paths.size() > 0)
            {
                if (curveData->_curvePath.isEmpty())
                    curveData->_curvePath = paths[0];
                _comboPath->setCurrentText(curveData->_curvePath);
                QVector<int> nodes = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getIdsByPathName(paths[0]);
                highlightPath(nodes);
            }

            _checkPath->setChecked(curveData->_curveIntersection);
            _btnGroupPath->button(curveData->_curveXID)->setChecked(true);
        }


        if (curveData->_curveType == "时间-变量曲线" || curveData->_curveType == "路径")
        {
            _comboVariable->clear();
            _comboVariable->addItems(curveData->_curveVarList);
            _comboVariable->setCurrentText(curveData->_curveVar);
            _listComponent->clear();
            _listComponent->addItems(curveData->_curveComponentList);
            for (int i = 0; i < _listComponent->count(); ++i)
            {
                QListWidgetItem* item = _listComponent->item(i);
                if (item && curveData->_curveComponent.contains(item->text()))
                {
                    item->setSelected(true);
                }
            }

            if (curveData->_curveHaveLayer)
            {
                _listLayer->clear();
                _listLayer->addItems(curveData->_curveLayerList);
                for (int i = 0; i < _listLayer->count(); ++i)
                {
                    QListWidgetItem* item = _listLayer->item(i);
                    if (item && curveData->_curveLayer.contains(item->text()))
                    {
                        item->setSelected(true);
                    }
                }
            }
            else
            {
                _listLayer->setEnabled(false);
                _btnAllLayer->setEnabled(false);
                _btnNoneLayer->setEnabled(false);
                _btnReverseLayer->setEnabled(false);
            }

            if (curveData->_curveHaveComplex)
            {
                _checkReal->setEnabled(true);
                _checkImag->setEnabled(true);
                _checkMag->setEnabled(true);
                _checkPhase->setEnabled(true);

                for (QCheckBox* cb : _boxList)
                {
                    if (curveData->_curveComplexList.contains(cb->text()))
                        cb->setChecked(true);
                }
            }
            else
            {
                _checkReal->setEnabled(false);
                _checkImag->setEnabled(false);
                _checkMag->setEnabled(false);
                _checkPhase->setEnabled(false);
            }
        }

        //更新曲线
        if (curveData->_isCurveGenerated)
        {
            //显示曲线
            mCurveWindow::getInstance()->updateMultiCurves(curveData->_xValues, curveData->_yValues, PlotView::LineChart, curveData->_curveHeaders, true, true);
        }

        _comboCurveType->blockSignals(false);
        _comboPath->blockSignals(false);
        _comboVariable->blockSignals(false);
        _listComponent->blockSignals(false);
        _listLayer->blockSignals(false);
        _checkReal->blockSignals(false);
        _checkImag->blockSignals(false);
        _checkMag->blockSignals(false);
        _checkPhase->blockSignals(false);
        _checkMag->blockSignals(false);
        _lineEditFile->blockSignals(false);
    }

    void mPostCurvePanel::initUI()
    {
        //类型
        _comboCurveType->blockSignals(true);
        _comboCurveType->setCurrentText("时间-变量曲线");
        _comboCurveType->blockSignals(false);
        showTimeVarWidget();

        QStringList paths = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getAllPathNames();
        _comboPath->clear();
        _comboPath->addItems(paths);
        _comboPath->setCurrentIndex(0);

        _checkPath->setChecked(false);
        _btnGroupPath->button(1)->setChecked(true);

        //导入文件
        _lineEditFile->setText("");

        //复数默认选择
        blockCheckSig(true);
        for (auto bx : _boxList)
        {
            bx->setChecked(false);
            bx->setEnabled(false);
        }
        _checkMag->setChecked(true);
        blockCheckSig(false);

        //拾取
        _pickIds.clear();
        _labelSelectStatu->setText("当前未选择");
        _labelSelectStatu->setStyleSheet("color: red; font-style: italic;");
    }

    void mPostCurvePanel::blockCheckSig(bool open)
    {
        for (QCheckBox* cb : _boxList)
        {
            cb->blockSignals(open);
        }
    }

    void mPostCurvePanel::updateDisplayCurve()
    {
        PostData::mPostCurveData* curveData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getCurveData(_curViewName);
        if (curveData == nullptr)
            return;

        if (curveData->_curveList.size() == 0) return;
        QVector<QVector<double>> yvals;
        QStringList headers;
        for (int i = 0; i < curveData->_curveList.size(); i++)
        {
            if (curveData->_curveList[i].visible)
            {
                yvals.push_back(curveData->_yValues[i]);
                headers.push_back(curveData->_curveHeaders[i]);
            }
        }

        mCurveWindow::getInstance()->updateMultiCurves(curveData->_xValues, yvals, PlotView::LineChart, headers, true, true);
    }

    void mPostCurvePanel::showTableData()
    {
        PostData::mPostCurveData* curveData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getCurveData(_curViewName);
        if (curveData == nullptr)
            return;

        QList<int> visibleIdxs;
        QStringList headers;
        for (int i = 0; i < curveData->_curveList.size(); ++i) {
            if (curveData->_curveList[i].visible) {
                visibleIdxs.append(i);
                headers << curveData->_curveList[i].name;
            }
        }

        if (headers.size() == 0) return;

        int rowCount = curveData->_xValues.size() + 1;        // 第一行为表头
        int colCount = visibleIdxs.size() + 1;  // 第一列为x值，后面每列是visible曲线

        mDockLayoutManager::getInstance()->activateDock("Table");
        mTableDataWindow::getInstance()->setUseCurve(false);
        _tableWidget = mTableDataWindow::getInstance()->getTableWidget();

        _tableWidget->clear();
        _tableWidget->setRowCount(rowCount);
        _tableWidget->setColumnCount(colCount);

        // 设置表头
        _tableWidget->setItem(0, 0, new QTableWidgetItem("")); // 左上角空
        for (int j = 0; j < headers.size(); ++j) {
            _tableWidget->setItem(0, j + 1, new QTableWidgetItem(headers[j]));
        }

		// 填充x和visible y数据
		for (int i = 0; i < curveData->_xValues.size(); ++i) {
			_tableWidget->setItem(i + 1, 0, new QTableWidgetItem(MxFunctions::mNumberToString(curveData->_xValues[i])));
			for (int j = 0; j < visibleIdxs.size(); ++j) {
				int srcIdx = visibleIdxs[j];
				double y = (srcIdx < curveData->_yValues.size() && i < curveData->_yValues[srcIdx].size()) ? curveData->_yValues[srcIdx][i] : 0;
				_tableWidget->setItem(i + 1, j + 1, new QTableWidgetItem(MxFunctions::mNumberToString(y)));
			}
		}

        // 可选：自适应列宽
        _tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        _tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        _tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }

    void mPostCurvePanel::updateItemStatus()
    {
        PostData::mPostCurveData* curveData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getCurveData(_curViewName);
        if (curveData == nullptr)
            return;

        int phyID, caseID;
        getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getCurveInfoByViewName(_curViewName, phyID, caseID);

        //更新模型树状态
        BrowserTreeStatus statu = curveData->getNodeStatu();
        mMechGlobalSignals::getInstance()->updatePostBrowserTreeStatus(CurveChildItem, phyID, caseID, statu);
    }

    void mPostCurvePanel::showTimeVarWidget()
    {
        _typeFold->removeAll();
        _typeFold->addRow(new QLabel("类型"), _comboCurveType);
        _comboCurveType->show();
        _typeFold->addRow(_widgetPick);
        _widgetPick->show();

        _xFold->show();
        _xFold->removeAll();
        _xFold->addRow(_labelData, _labelDataType);
        _labelData->show();
        _labelDataType->show();

        _yFold->show();
        _yFold->removeAll();
        _yFold->addRow(new QLabel("变量"), _comboVariable);
        _comboVariable->show();
        _yFold->addRow(new QLabel("分量"), _btnAllCompo, _btnNoneCompo, _btnReverseCompo);
        _btnAllCompo->show();
        _btnNoneCompo->show();
        _btnReverseCompo->show();
        _yFold->addRow(_listComponent);
        _listComponent->show();
        _yFold->addRow(new QLabel("层"), _btnAllLayer, _btnNoneLayer, _btnReverseLayer);
        _btnAllLayer->show();
        _btnNoneLayer->show();
        _btnReverseLayer->show();
        _yFold->addRow(_listLayer);
        _listLayer->show();
        _yFold->addRow(new QLabel("复数"), _checkReal, _checkImag, _checkMag, _checkPhase);
        _checkReal->show();
        _checkImag->show();
        _checkMag->show();
        _checkPhase->show();

        _typeFold->refreshLayout();
        _xFold->refreshLayout();
        _yFold->refreshLayout();
    }

    void mPostCurvePanel::showImportWidget()
    {
        _typeFold->removeAll();
        _typeFold->addRow(new QLabel("类型"), _comboCurveType);
        _comboCurveType->show();
        _typeFold->addRow(_labelFile, _lineEditFile, _btnFile);
        _labelFile->show();
        _lineEditFile->show();
        _btnFile->show();
        _labelPath->hide();
        _comboPath->hide();
        _btnPath->hide();

        _xFold->hide();
        _yFold->hide();

        _typeFold->refreshLayout();
        _xFold->refreshLayout();
        _yFold->refreshLayout();
    }

    void mPostCurvePanel::showPathWidget()
    {
        _typeFold->removeAll();
        _typeFold->addRow(new QLabel("类型"), _comboCurveType);
        _comboCurveType->show();
        _typeFold->addRow(_labelPath, _comboPath, _btnPath);
        _labelPath->show();
        _comboPath->show();
        _btnPath->show();
        _lineEditFile->hide();
        _btnFile->hide();
        _typeFold->addRow("",_checkPath);
        _checkPath->show();

        _xFold->removeAll();
        _xFold->show();
        _xFold->addRow(_widgetPath);
        _widgetPath->show();

        _yFold->removeAll();
        _yFold->show();
        _yFold->addRow(_labelCase, _comboCase);
        _labelCase->show();
        _comboCase->show();
        _yFold->addRow(_labelFrame, _comboFrame);
        _labelFrame->show();
        _comboFrame->show();
        _yFold->addRow(new QLabel("变量"), _comboVariable);
        _comboVariable->show();
        _yFold->addRow(new QLabel("分量"), _btnAllCompo, _btnNoneCompo, _btnReverseCompo);
        _btnAllCompo->show();
        _btnNoneCompo->show();
        _btnReverseCompo->show();
        _yFold->addRow(_listComponent);
        _listComponent->show();
        _yFold->addRow(new QLabel("层"), _btnAllLayer, _btnNoneLayer, _btnReverseLayer);
        _btnAllLayer->show();
        _btnNoneLayer->show();
        _btnReverseLayer->show();
        _yFold->addRow(_listLayer);
        _listLayer->show();
        _yFold->addRow(new QLabel("复数"), _checkReal, _checkImag, _checkMag, _checkPhase);
        _checkReal->show();
        _checkImag->show();
        _checkMag->show();
        _checkPhase->show();

        _typeFold->refreshLayout();
        _xFold->refreshLayout();
        _yFold->refreshLayout();
    }

    QVector<double> mPostCurvePanel::calPathCurveXvalues(QVector<QVector3D> vertexs, QString& type)
    {
        QVector<double> res;
        int size = vertexs.size();
        if (size == 1 || size == 0)
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, QString("无法提取路径数据"), true);
        }

        int id = _btnGroupPath->checkedId();
        type = _btnGroupPath->checkedButton()->text();
        switch (id)
        {
        case 1:
        {
            res.push_back(0);
            //每小段路径x距离总和
            double pathXTotalDis = 0;
            for (int i = 1; i < size; i++)
            {
                double dis = abs(vertexs[i].x() - vertexs[i - 1].x());
                pathXTotalDis += dis;
                res.push_back(pathXTotalDis);
            }
            break;
        }
        case 2:
        {
            for (int i = 0; i < size; i++)
            {
                res.push_back(vertexs[i].x());
            }
            break;
        }
        case 3:
        {
            res.push_back(0);
            //每小段路径y距离总和
            double pathYTotalDis = 0;
            for (int i = 1; i < size; i++)
            {
                double dis = abs(vertexs[i].y() - vertexs[i - 1].y());
                pathYTotalDis += dis;
                res.push_back(pathYTotalDis);
            }
        }
        break;
        case 4:
        {
            for (int i = 0; i < size; i++)
            {
                res.push_back(vertexs[i].y());
            }
        }
        break;
        case 5:
        {
            res.push_back(0);
            //每小段路径Z距离总和
            double pathZTotalDis = 0;
            for (int i = 1; i < size; i++)
            {
                double dis = abs(vertexs[i].z() - vertexs[i - 1].z());
                pathZTotalDis += dis;
                res.push_back(pathZTotalDis);
            }
        }
        break;
        case 6:
        {
            for (int i = 0; i < size; i++)
            {
                res.push_back(vertexs[i].z());
            }
        }
        break;
        case 7:
        {
            res.push_back(0);
            //每小段路径距离总和
            double pathTotalDis = 0;
            for (int i = 1; i < size; i++)
            {
                double distance = vertexs[i].distanceToPoint(vertexs[i - 1]);
                pathTotalDis += distance;
                res.push_back(pathTotalDis);
            }
        }
        break;
        case 8:
        {
            res.push_back(0);
            //每小段路径距离之和
            QVector<double> pathTotalDistance;
            //计算总距离
            double totalDis = 0;
            for (int i = 1; i < size; i++)
            {
                double distance = vertexs[i].distanceToPoint(vertexs[i - 1]);
                totalDis += distance;
                pathTotalDistance.push_back(totalDis);
            }

            for (int j = 0; j < pathTotalDistance.size(); j++)
            {
                double ratio = pathTotalDistance[j] / totalDis;
                res.push_back(ratio);
            }
        }
        break;
        default:
            break;
        }
        return res;
    }

    void mPostCurvePanel::getPathCurveYvalues(QVector<int> ids)
    {
        //获取工况名称
        QString caseName = _comboCase->currentText();

        //获取变量、层、分量、复数
        QString name = _comboVariable->currentText();
        QStringList layers = getSelectedList(_listLayer);
        QStringList compos = getSelectedList(_listComponent);
        QStringList complexs = getCheckedComplex();

		// 标量变量没有分量，使用空分量占位以生成一条曲线
		if (compos.isEmpty() && _listComponent->count() == 0)
			compos << "";

		if (_listComponent->count() > 0 && compos.size() == 0)
		{
			mGlobalSignals::getInstance()->outputMessageSig(2, QString("没有选择分量!"), true);
			return;
		}

        if (_listLayer->count() > 0 && layers.size() == 0)
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, QString("没有选择层!"), true);
            return;
        }

        if (_isComplex && complexs.size() == 0)
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, QString("没有选择复数输出类型!"), true);
            return;
        }

        //获取帧
        QString fName;
        QString framestr = _comboFrame->currentText();
        if (framestr.contains(":"))
            fName = framestr.split(":").at(0);
        set<int> frameIds;
        frameIds.insert(fName.toInt());
        //将用户输入的帧阶数转换为帧号方便查找
        frameIds = _postDB->getCaseFrameNumByFrameName(caseName, frameIds);

        QHash<QString, QHash<int, double>> tempContainerD;
        QHash<QString, QHash<int, float>> tempContainerF;

        CurveKey curveKey;
        QList<CurveKey> idCurves = curveKey.generateCurveKeys(compos, layers, complexs);

        // 读取曲线数据
        QString postPathName = _postDB->getFile();
        if (_postDB->getDataFlag())
        {
            mIMxdbFile1* mxdbReader = new mIMxdbFile1;
            mxdbReader->readIDVarCurve(postPathName, name, idCurves, ids, frameIds, _curveNames, tempContainerD, caseName);
            delete mxdbReader;
            mxdbReader = nullptr;
        }
        else
        {
            mMxdbThread* mxdbReader = new mMxdbThread(postPathName, _postDB, this);
            mxdbReader->readIDVarCurve(postPathName, name, idCurves, ids, frameIds, _curveNames, tempContainerD, tempContainerF, caseName);
            delete mxdbReader;
            mxdbReader = nullptr;
        }

        //生成Y值
        if (tempContainerD.size() > 0)
        {
            for (QString cname : _curveNames)
            {
                QVector<double> yval;
                for (auto it = ids.begin(); it != ids.end(); ++it)
                {
                    yval.append(tempContainerD.value(cname).value(*it));
                }
                _YValuesD.insert(cname, yval);
            }
        }

        if (tempContainerF.size() > 0)
        {
            for (QString cname : _curveNames)
            {
                QVector<float> yval;
                for (auto it = ids.begin(); it != ids.end(); ++it)
                {
                    yval.append(tempContainerF.value(cname).value(*it));
                }
                _YValuesF.insert(cname, yval);
            }
        }
    }

    void mPostCurvePanel::highlightPath(QVector<int> pathIDs, bool isIntersect)
    {
        if (!_postDB) return;
        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;

        auto postRender = postRend->getPostRender();

        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return;

        mPostModelData* modelData = _postDB->getModelData(guidata->_cloudFrame);
        if (modelData == nullptr)
            return;

        std::vector<QVector3D> poss;
        std::vector<std::pair<int, int>> lines;
        for (int i = 0; i < pathIDs.size() - 1; i++)
        {
            int startPointId = pathIDs[i];
            int endPointId = pathIDs[i + 1];

            int cSId = modelData->_nodedata.nodeidmap.value(startPointId);
            QVector3D sVertex = modelData->getCoordinates(cSId);
            int cEId = modelData->_nodedata.nodeidmap.value(endPointId);
            QVector3D eVertex = modelData->getCoordinates(cEId);
            double totalDistance = sVertex.distanceToPoint(eVertex);

            poss.push_back(sVertex);
            poss.push_back(eVertex);

            std::pair<int, int> linePair = std::make_pair(startPointId, endPointId);
            lines.push_back(linePair);
        }
        // S5 新门面：临时标注=G5 TempMark 单实例整面替换（重复 addObject=整面替换）
        mxr::TempMarkParams tempMark;
        tempMark.nodes = QVector<QVector3D>::fromStdVector(poss);
        tempMark.lines.reserve(int(lines.size()));
        for (const auto& l : lines)
            tempMark.lines.push_back(QPair<int, int>(l.first, l.second));
        postRender->addObject(tempMark);
        postRender->updateHighLightRender();

        //显示ID
        QVector<QVector3D> vertexs;
        const QHash<int, QVector3D> dis = _postDB->getDisplacementDataByFrameID(guidata->_cloudFrame);
        QVector3D deformationScale = guidata->_deformFactor;
        for (int id : pathIDs)
        {
            int cid = modelData->_nodedata.nodeidmap.value(id);
            auto nodecoordinate = modelData->getVector(cid);
            QVector3D vertex = nodecoordinate + deformationScale * dis.value(cid);
            vertexs.push_back(vertex);
        }

        QVector<QString> ids;
        auto fontRender = postRend->getFontRender();
        for (int i = 0; i < pathIDs.size(); i++)
        {
            ids.append(QString("%1").arg(pathIDs[i]));
        }
        fontRender->appendCommonFont("PathID", vertexs, ids, QVector3D(0, 0, 0));

        postRend->updatePaint();
    }

    void mPostCurvePanel::clearhighlight()
    {
        if (!getCVM) return;
        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;

        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        // S5 新门面：清空临时标注=G5 TempMark 整面替换为空（缺省空 nodes/lines，空集重置）
        mxr::TempMarkParams tempMark;
        postRender->addObject(tempMark);
        postRender->updateHighLightRender();

        auto fontRender = postRend->getFontRender();
        fontRender->clearCommonFont("PathID");
        postRend->updatePaint();
    }

    void mPostCurvePanel::slot_chooseCurveType(const QString& type)
    {
        PostData::mPostCurveData* curveData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getCurveData(_curViewName);
        if (curveData == nullptr)
            return;
        curveData->_curveType = type;

        if (type == "时间-变量曲线")
        {
            showTimeVarWidget();
            //获取所有变量
            QStringList vnames = _postDB->getAllVariableNames();
            _comboVariable->blockSignals(true);
            _comboVariable->clear();
            _comboVariable->blockSignals(false);
            _comboVariable->addItems(vnames);
            curveData->_curveVarList = vnames;
            clearhighlight();
        }
        else if (type == "导入")
        {
            showImportWidget();
            clearhighlight();
		}
        else if (type == "导入CSV文件")
        {
            showImportWidget();
            clearhighlight();
        }
		else if (type == "路径")
		{
			showPathWidget();
			QStringList paths = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getAllPathNames();
            _comboPath->blockSignals(true);
            _comboPath->clear();
            _comboPath->blockSignals(false);
            _comboPath->addItems(paths);

            QString pathName = _comboPath->currentText();
            QVector<int> nodes = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getIdsByPathName(pathName);
            highlightPath(nodes);

            //工况
            QStringList cases = _postDB->getAllCaseNames();
            _comboCase->blockSignals(true);
            _comboCase->clear();
            _comboCase->blockSignals(false);
            _comboCase->addItems(cases);
            curveData->_curveCaseList = cases;
        }
        updateItemStatus();
    }

    void mPostCurvePanel::slot_changeCaseName(const QString& caseName)
    {
        PostData::mPostCurveData* curveData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getCurveData(_curViewName);
        if (curveData == nullptr)
            return;
        curveData->_curveCase = caseName;

        if (_postDB == nullptr) return;

        QStringList frameinfos = _postDB->getCaseFrameNames(caseName);
        _comboFrame->blockSignals(true);
        _comboFrame->clear();
        _comboFrame->blockSignals(false);
        _comboFrame->addItems(frameinfos);
        curveData->_curveFrameList = frameinfos;
    }

    void mPostCurvePanel::slot_chooseFrame(const QString& frame)
    {
        PostData::mPostCurveData* curveData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getCurveData(_curViewName);
        if (curveData == nullptr)
            return;
        curveData->_curveFrame = frame;

        //获取该帧的变量
        QString fName;
        if (frame.contains(":"))
            fName = frame.split(":").at(0);
        int fi = _postDB->getFrameNumByCaseAndFrameName(_comboCase->currentText(), fName);
        QStringList vnames = _postDB->getVariableByFrameID(fi);

        QStringList varlist;
        //判断是否为节点变量
        for (auto var : vnames)
        {
            int type = _postDB->getVariableType(var);
            if (type == 0)
            {
                varlist.append(var);
            }
        }

        if (varlist.size() != 0)
        {
            _comboVariable->blockSignals(true);
            _comboVariable->clear();
            _comboVariable->blockSignals(false);
            _comboVariable->addItems(varlist);
            _comboVariable->setEnabled(true);
        }
        else
        {
            _comboVariable->clear();
            _comboVariable->setEnabled(false);

            curveData->_curveHaveComponent = false;
            _listComponent->setEnabled(false);
            _btnAllCompo->setEnabled(false);
            _btnNoneCompo->setEnabled(false);
            _btnReverseCompo->setEnabled(false);

            curveData->_curveHaveLayer = false;
            _listLayer->setEnabled(false);
            _btnAllLayer->setEnabled(false);
            _btnNoneLayer->setEnabled(false);
            _btnReverseLayer->setEnabled(false);

            curveData->_curveHaveComplex = false;
            _isComplex = false;
            _checkReal->setEnabled(false);
            _checkImag->setEnabled(false);
            _checkMag->setEnabled(false);
            _checkPhase->setEnabled(false);
        }
    }

    void mPostCurvePanel::slot_chooseVariable(const QString& var)
    {
        //存储界面数据
        PostData::mPostCurveData* curveData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getCurveData(_curViewName);
        if (curveData == nullptr)
            return;
        curveData->_curveVar = var;

		if (_postDB == nullptr) return;
		QStringList cnames = _postDB->getVariableCompos(var);
		QStringList lnames = _postDB->getVariableLayer(var);
		curveData->_curveComponentList = cnames;
		curveData->_curveLayerList = lnames;
		curveData->_curveHaveComponent = cnames.size() > 0;

        _listComponent->clear();
        _listComponent->addItems(cnames);
        _listLayer->clear();
        _listLayer->addItems(lnames);

		if (cnames.size() == 0)
		{
			curveData->_curveComponent.clear();
			_listComponent->setEnabled(false);
			_btnAllCompo->setEnabled(false);
			_btnNoneCompo->setEnabled(false);
			_btnReverseCompo->setEnabled(false);
		}
		else
		{
			_listComponent->setEnabled(true);
			_btnAllCompo->setEnabled(true);
			_btnNoneCompo->setEnabled(true);
			_btnReverseCompo->setEnabled(true);
		}

		if (lnames.size() == 0)
		{
			curveData->_curveHaveLayer = false;
			_listLayer->setEnabled(false);
			_btnAllLayer->setEnabled(false);
			_btnNoneLayer->setEnabled(false);
			_btnReverseLayer->setEnabled(false);
		}
		else
		{
			curveData->_curveHaveLayer = true;
			_listLayer->setEnabled(true);
			_btnAllLayer->setEnabled(true);
			_btnNoneLayer->setEnabled(true);
			_btnReverseLayer->setEnabled(true);
		}

        int vartype = _postDB->getVariableType(var);
        if (vartype == 0)
        {
            _varType = 0;
            _labelObject->setText("节点");
            curveData->_curveObject = "节点";
            //设置拾取
        }
        else
        {
            _varType = 1;
            _labelObject->setText("单元");
            curveData->_curveObject = "单元";
            //设置拾取
        }
    }

    void mPostCurvePanel::slot_selectedCompoChanged()
    {
        PostData::mPostCurveData* curveData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getCurveData(_curViewName);
        if (curveData == nullptr)
            return;

        if (_postDB == nullptr) return;
        QString var = _comboVariable->currentText();

        //获取选择的分量
        QStringList componames = getSelectedList(_listComponent);
        curveData->_curveComponent = componames;

        updateItemStatus();
        for (QString cname : componames)
        {
            //获取该分量是否为复数
            bool iscomplex = _postDB->isVarCompoComplex(var, cname);
            if (iscomplex)
            {
                _isComplex = true;
                curveData->_curveHaveComplex = true;
                _checkReal->setEnabled(true);
                _checkImag->setEnabled(true);
                _checkMag->setEnabled(true);
                _checkPhase->setEnabled(true);
                return;
            }
        }
        curveData->_curveHaveComplex = false;
        _isComplex = false;
        _checkReal->setEnabled(false);
        _checkImag->setEnabled(false);
        _checkMag->setEnabled(false);
        _checkPhase->setEnabled(false);

    }

    void mPostCurvePanel::slot_selectedLayerChanged()
    {
        PostData::mPostCurveData* curveData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getCurveData(_curViewName);
        if (curveData == nullptr)
            return;

        //获取选择的层
        QStringList layers = getSelectedList(_listLayer);
        curveData->_curveLayer = layers;

        updateItemStatus();
    }

    void mPostCurvePanel::slot_selectAll()
    {
        QPushButton* pb = qobject_cast<QPushButton*>(sender());
        if (pb == nullptr) return;

        if (pb == _btnAllCompo)
        {
            for (int i = 0; i < _listComponent->count(); ++i)
            {
                _listComponent->item(i)->setSelected(true);
            }
        }
        if (pb == _btnAllLayer)
        {
            for (int i = 0; i < _listLayer->count(); ++i)
            {
                _listLayer->item(i)->setSelected(true);
            }
        }
    }

    void mPostCurvePanel::slot_selectNone()
    {
        QPushButton* pb = qobject_cast<QPushButton*>(sender());
        if (pb == nullptr) return;

        if (pb == _btnNoneCompo)
        {
            for (int i = 0; i < _listComponent->count(); ++i)
            {
                _listComponent->item(i)->setSelected(false);
            }
        }
        if (pb == _btnNoneLayer)
        {
            for (int i = 0; i < _listLayer->count(); ++i)
            {
                _listLayer->item(i)->setSelected(false);
            }
        }
    }

    void mPostCurvePanel::slot_selectReverse()
    {
        QPushButton* pb = qobject_cast<QPushButton*>(sender());
        if (pb == nullptr) return;

        if (pb == _btnReverseCompo)
        {
            for (int i = 0; i < _listComponent->count(); ++i)
            {
                QListWidgetItem* item = _listComponent->item(i);
                item->setSelected(!item->isSelected());
            }
        }
        if (pb == _btnReverseLayer)
        {
            for (int i = 0; i < _listLayer->count(); ++i)
            {
                QListWidgetItem* item = _listLayer->item(i);
                item->setSelected(!item->isSelected());
            }
        }
    }

    void mPostCurvePanel::slot_addPick()
    {
        PostData::mPostCurveData* curveData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getCurveData(_curViewName);
        if (curveData == nullptr)
            return;

        QPushButton* pb = qobject_cast<QPushButton*>(sender());
        if (pb == nullptr) return;

        if (pb == _btnReplace)
            _pickIds.clear();

        std::vector<MeshSelectInfoData> res;
        int num = 0;
        //获取拾取模式
        PickDataType pickDataType = getCVM->getPostPickDataType(num);
        if (pickDataType != PickDataType::Node && pickDataType != PickDataType::Element)
        {
            return;
        }

        if (num == 0) return;

        int count = 0;
        QString type;
        if (_varType == 0)
        {
            type = "节点";
            res = getCVM->getPostRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
            count = res.size() + _pickIds.size();

        }
        else
        {
            type = "单元";
            res = getCVM->getPostRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
            count = res.size() + _pickIds.size();
        }

        getCVM->clearPick();
        if (count == 0)
        {
            _labelSelectStatu->setText("当前未选择");
            _labelSelectStatu->setStyleSheet("color: red; font-style: italic;");
        }
        else
        {
            _labelSelectStatu->setText(QString("已选择%1个%2").arg(count).arg(type));
            _labelSelectStatu->setStyleSheet("color: black; font-weight: bold;");
        }

        for (auto r : res)
        {
            _pickIds.push_back(r.data_id.element_id);
        }

        curveData->_curvePickIds = _pickIds;
        updateItemStatus();
    }

    void mPostCurvePanel::slot_clearPick()
    {
        PostData::mPostCurveData* curveData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getCurveData(_curViewName);
        if (curveData == nullptr)
            return;

        _pickIds.clear();
        _labelSelectStatu->setText("当前未选择");
        _labelSelectStatu->setStyleSheet("color: red; font-style: italic;");
        getCVM->clearPick();

        curveData->_curvePickIds = _pickIds;
        updateItemStatus();
    }

    void mPostCurvePanel::slot_checkedChange()
    {
        PostData::mPostCurveData* curveData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getCurveData(_curViewName);
        if (curveData == nullptr)
            return;

        QStringList complexs = getCheckedComplex();
        curveData->_curveComplexList = complexs;
        updateItemStatus();
    }

    void mPostCurvePanel::slot_chooseCurveFile()
    {
        PostData::mPostCurveData* curveData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getCurveData(_curViewName);
        if (curveData == nullptr)
            return;

        QString dfdir = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
        // 导入CSV文件类型默认选中 CSV 过滤器（过滤器列表首个为默认选中项）
        QString filter = (curveData->_curveType == "导入CSV文件")
            ? QString("CSV File(*.csv);;Text File(*.txt);;MxH File(*.mxh)")
            : QString("Text File(*.txt);;CSV File(*.csv);;MxH File(*.mxh)");
        QString filename = QFileDialog::getOpenFileName(this, QString("打开文件"), dfdir, filter);

		if (filename.isEmpty())	return;
		_lineEditFile->setText(filename);
		curveData->_curveFilePath = filename;
		updateItemStatus();
	}

    bool mPostCurvePanel::loadCurveFile(const QString& filePath, QString& errMsg)
    {
        PostData::mPostCurveData* curveData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getCurveData(_curViewName);
        QString canonicalPath = QFileInfo(filePath).canonicalFilePath();
        if (canonicalPath.isEmpty())
        {
            errMsg = QString("导入CSV文件失败: 文件不存在或路径无效，请重新选择文件!");
            return false;
        }

        if (curveData->_curveDataCache.contains(canonicalPath))
            return true;

        QFile file(canonicalPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            errMsg = QString("导入CSV文件失败: 无法打开文件，请检查文件权限!");
            return false;
        }

        QTextStream in(&file);
        QString headerLine = in.readLine();
        if (headerLine.isEmpty())
        {
            errMsg = QString("导入CSV文件失败: 文件内容为空!");
            return false;
        }

        // 自动检测分隔符
        QStringList headers;
        int tabCount = headerLine.count('\t');
        int commaCount = headerLine.count(',');
        if (tabCount >= commaCount)
            headers = headerLine.split('\t');
        else
            headers = headerLine.split(',');

        // 清理列名
        QStringList colNames;
        for (const QString& h : headers) {
            QString trimmed = h.trimmed();
            if (!trimmed.isEmpty())
                colNames << trimmed;
        }
        int colCount = colNames.size();
        if (colCount == 0)
        {
            errMsg = QString("导入CSV文件失败: 文件头格式错误，未识别到有效的列名!");
            return false;
        }

        // 初始化数据容器
        QHash<QString, QVector<double>> columnsData;
        for (const QString& name : colNames)
            columnsData[name] = QVector<double>();

        // 解析行
        int lineNumber = 1;
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            lineNumber++;
            if (line.isEmpty()) continue;

            QStringList parts = (tabCount >= commaCount) ? line.split('\t') : line.split(',');
            if (parts.size() < colCount) {
                qWarning() << "Line" << lineNumber << "column mismatch";
                continue;
            }

            for (int i = 0; i < colCount; ++i) {
                bool ok = false;
                double val = parts[i].trimmed().toDouble(&ok);
                columnsData[colNames[i]].append(ok ? val : std::numeric_limits<double>::quiet_NaN());
            }
        }

        // 检查是否解析到有效数值（全是非数字内容视为文件格式错误）
        bool hasData = false;
        for (const auto& vec : columnsData) {
            for (double v : vec) {
                if (!std::isnan(v)) { hasData = true; break; }
            }
            if (hasData) break;
        }
        if (!hasData)
        {
            errMsg = QString("导入CSV文件失败: 未解析到有效的数值数据，请检查文件格式(应为带表头的数值型表格数据)!");
            return false;
        }

        curveData->_curveDataCache[canonicalPath] = columnsData;

        return true;
    }

	void mPostCurvePanel::slot_readCurveFile(QString filename, QStringList& curveNames, QVector<double>& xValue, QVector<QVector<double>>& yValue)
	{
		//获取曲线名称
		QFile txtfile(filename);
		QStringList list = filename.split('/');
		if (list.size() <= 0)
			return;

        QString fileText = list[list.size() - 1];
        QTextCodec* codeC = QTextCodec::codecForName("UTF-8");
        if (txtfile.exists())
        {
            txtfile.open(QIODevice::ReadOnly | QIODevice::Text);

            //临时变量
            QString linestr;
            QStringList linelist;

            //读取曲线名
            linestr = codeC->toUnicode(txtfile.readLine().simplified());
            if (linestr.contains("*Type1", Qt::CaseInsensitive))
            {
                if (linestr.contains("*Type1", Qt::CaseInsensitive))
                {
                    linestr = codeC->toUnicode(txtfile.readLine().simplified());

                }

                while (!txtfile.atEnd())
                {
                    if (linestr.contains("Title", Qt::CaseInsensitive))
                    {
                        //曲线标题
                        QString title = linestr.split("=").at(1).simplified();
                        linestr = codeC->toUnicode(txtfile.readLine().simplified());
                    }
                    else
                    {
                        linelist = linestr.split(",");

                        for (QString str : linelist)
                        {
                            if (str.trimmed() == "")
                                continue;
                            curveNames.append(str.trimmed());
                        }

                        //读取坐标轴标题
                        linestr = codeC->toUnicode(txtfile.readLine().simplified());
                        linelist = linestr.split(",");
                        QString xTitle;
                        QStringList yTitles{};
                        for (int i = 0; i < linelist.size(); ++i)
                        {
                            if (i == 0)
                            {
                                xTitle = linelist.at(0);
                            }
                            else
                            {
                                yTitles.append(linelist.at(i).simplified());
                            }
                        }

                        int ncurve = curveNames.size();
                        QVector<QVector<double>> tempVectors;
                        QVector<double> xValues{};
                        int flag = 0;
                        while (!txtfile.atEnd())
                        {
                            linestr = codeC->toUnicode(txtfile.readLine().simplified());
                            if (linestr.isEmpty())
                            {
                                continue;
                            }
                            linelist = linestr.split(",");

                            //存储X数据
                            double xval = 0;
                            QString valstr = linelist.at(0).simplified();
                            if (!valstr.isEmpty())
                            {
                                xval = valstr.toDouble();
                            }
                            xValues.append(xval);

                            //存储Y数据
                            int n = linelist.size();
                            for (int i = 1; i < n; ++i)
                            {
                                double yval = 0;
                                QString str = linelist.at(i).simplified();
                                if (!str.isEmpty())
                                {
                                    yval = str.toDouble();
                                }

                                QVector<double> yvalvec;
                                if (flag > 0)
                                {
                                    yvalvec = tempVectors.at(i - 1);
                                    yvalvec.append(yval);
                                    tempVectors.replace(i - 1, yvalvec);
                                }
                                else
                                {
                                    yvalvec.append(yval);
                                    tempVectors.append(yvalvec);
                                }
                            }
                            //若该行数据缺失，则补0
                            for (int i = n; i < ncurve + 1; ++i)
                            {
                                QVector<double> yvalvec;
                                if (flag > 0)
                                {
                                    yvalvec = tempVectors.at(i - n);
                                    yvalvec.append(0);
                                    tempVectors.replace(i - n, yvalvec);
                                }
                                else
                                {
                                    yvalvec.append(0);
                                    tempVectors.append(yvalvec);
                                }
                            }

                            //标识+1
                            ++flag;
                        }
                        //存储数据
                        QVector<QVector<double>> yValues{};
                        for (int i = 0; i < ncurve; ++i)
                        {
                            yValues.append(tempVectors.at(i));
                        }
                        xValue = xValues;
                        yValue = yValues;
                    }
                }
            }
            else
            {
                QString title = linestr;
                linestr = codeC->toUnicode(txtfile.readLine().simplified());
                while (!txtfile.atEnd())
                {
                    linelist = linestr.split(" ");

                    QStringList yTitles{};
                    int index = 0;
                    for (QString str : linelist)
                    {
                        if (str.trimmed() == "")
                            continue;
                        if (index == 0)
                        {
                            QString xTitle = str;
                        }
                        else
                        {
                            curveNames.append(str.trimmed());
                            yTitles.append(str.simplified());
                        }
                        ++index;
                    }
                    const int ncurve = curveNames.size();
                    QVector<QVector<double>> tempVectors;
                    QVector<double> xValues{};
                    int flag = 0;
                    while (!txtfile.atEnd())
                    {
                        linestr = codeC->toUnicode(txtfile.readLine().simplified());
                        if (linestr.isEmpty())
                        {
                            continue;
                        }
                        linelist = linestr.split(" ");

                        //存储X数据
                        double xval = 0;
                        QString valstr = linelist.at(0).simplified();
                        if (!valstr.isEmpty())
                        {
                            xval = valstr.toDouble();
                        }
                        xValues.append(xval);

                        //存储Y数据
                        int n = linelist.size();
                        for (int i = 1; i < n; ++i)
                        {
                            double yval = 0;
                            QString str = linelist.at(i).simplified();
                            if (!str.isEmpty())
                            {
                                yval = str.toDouble();
                            }

                            QVector<double> yvalvec;
                            if (flag > 0)
                            {
                                yvalvec = tempVectors.at(i - 1);
                                yvalvec.append(yval);
                                tempVectors.replace(i - 1, yvalvec);
                            }
                            else
                            {
                                yvalvec.append(yval);
                                tempVectors.append(yvalvec);
                            }
                        }
                        //若该行数据缺失，则补0
                        for (int i = n; i < ncurve + 1; ++i)
                        {
                            QVector<double> yvalvec;
                            if (flag > 0)
                            {
                                yvalvec = tempVectors.at(i - n);
                                yvalvec.append(0);
                                tempVectors.replace(i - n, yvalvec);
                            }
                            else
                            {
                                yvalvec.append(0);
                                tempVectors.append(yvalvec);
                            }
                        }

                        //标识+1
                        ++flag;
                    }
                    //存储数据
                    QVector<QVector<double>> yValues{};
                    for (int i = 0; i < ncurve; ++i)
                    {
                        yValues.append(tempVectors.at(i));
                    }
                    xValue = xValues;
                    yValue = yValues;
                }
            }
        }
    }

    void mPostCurvePanel::slot_generateCurve()
    {
        if (_postDB == nullptr) return;

        PostData::mPostCurveData* curveData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getCurveData(_curViewName);
        if (curveData == nullptr)
            return;

        auto clearGeneratedCurve = [=]()
            {
                mCurveWindow::getInstance()->clearAllPlot();
                curveData->_isCurveGenerated = false;
                curveData->_xValues.clear();
                curveData->_yValues.clear();
                curveData->_curveHeaders.clear();
                curveData->_curveList.clear();
                curveData->_lastCurveType.clear();
                updateItemStatus();
            };

        QStringList headers;
        QVector<double> xs;
        QVector<QVector<double>> ys;
        QString ctype = _comboCurveType->currentText();

        //获取变量、层、分量、复数
        QString var = _comboVariable->currentText();
        QStringList layers = getSelectedList(_listLayer);
        QStringList compos = getSelectedList(_listComponent);
        QStringList complexs = getCheckedComplex();

		// 标量变量没有分量，使用空分量占位以生成一条曲线
		if (compos.isEmpty() && _listComponent->count() == 0)
			compos << "";

		if (ctype == "时间-变量曲线")
		{
			if (_listComponent->count() > 0 && compos.size() == 0)
			{
				mGlobalSignals::getInstance()->outputMessageSig(2, QString("没有选择分量!"), true);
				clearGeneratedCurve();
				return;
			}

            if (_listLayer->count() > 0 && layers.size() == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("没有选择层!"), true);
                clearGeneratedCurve();
                return;
            }

            if (_isComplex && complexs.size() == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("没有选择复数输出类型!"), true);
                clearGeneratedCurve();
                return;
            }

            if (_pickIds.size() == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("没有拾取到节点或单元!"), true);
                clearGeneratedCurve();
                return;
            }

            //读取曲线数据
            _curveNames.clear();
            _XValueD.clear();
            _XValueF.clear();
            _YValuesD.clear();
            _YValuesF.clear();

		CurveKey curveKey;
		QList<CurveKey> timeCurves = curveKey.generateCurveKeys(compos, layers, complexs);

		QString postPathName = _postDB->getFile();
			if (_postDB->getDataFlag())
			{
				mIMxdbFile1* mxdbReader = new mIMxdbFile1;
				mxdbReader->readTimeVarCurve(postPathName, var, timeCurves, _pickIds, _curveNames, _XValueD, _YValuesD);
				delete mxdbReader;
				mxdbReader = nullptr;
			}
			else
			{
				mMxdbThread* mxdbReader = new mMxdbThread(postPathName, _postDB, this);
				// 读取曲线数据
				mxdbReader->readTimeVarCurve(postPathName, var, timeCurves, _pickIds, _curveNames, _XValueD, _YValuesD, _YValuesF);
				delete mxdbReader;
				mxdbReader = nullptr;
			}

            curveData->addTimeVarParam(_pickIds, var, compos, layers, complexs);
            curveData->_curveComplexList = complexs;

            int ncurve = _curveNames.size();
            if (ncurve == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("曲线生成失败!"), true);
                clearGeneratedCurve();
                return;
            }

			// 处理曲线数据
			getCurveDatas(xs, ys, headers);
		}
		else if (ctype == "导入")
		{
			QString filename = _lineEditFile->text();

			if (filename.isEmpty() || !QFileInfo(filename).exists())
			{
				mGlobalSignals::getInstance()->outputMessageSig(2, QString("导入失败: 文件不存在或路径无效，请重新选择文件!"), true);
				clearGeneratedCurve();
				return;
			}

			slot_readCurveFile(filename, headers, xs, ys);
			if (headers.size() == 0)
			{
				mGlobalSignals::getInstance()->outputMessageSig(2, QString("导入失败: 文件格式错误，无法解析出曲线数据，请检查文件内容是否符合要求的曲线文件格式!"), true);
				clearGeneratedCurve();
				return;
			}
			//存储导入参数
			curveData->addImportParam(filename);
		}
        else if (ctype == "导入CSV文件")
        {
            QString filename = _lineEditFile->text();
            // 调用解析/缓存函数
            QString loadErrMsg;
            if (!loadCurveFile(filename, loadErrMsg))
            {
                QString msg = loadErrMsg.isEmpty() ? QString("曲线生成失败!") : loadErrMsg;
                mGlobalSignals::getInstance()->outputMessageSig(2, msg, true);
                clearGeneratedCurve();
                return;
            }
            // 使用规范化路径作为缓存键
            QString canonicalPath = QFileInfo(filename).canonicalFilePath();
            // 获取缓存的数据列
            const QHash<QString, QVector<double>>& columns = curveData->_curveDataCache[canonicalPath];
            // 取出 X 轴数据
            xs = columns["time"];
            // 填充 Y 数据和曲线名
            headers.clear();
            ys.clear();
            //单条
            /*headers << "ExternalWork";
            ys.append(columns["ExternalWork"]);*/
            //多条
            for (auto it = columns.begin(); it != columns.end(); ++it)
            {
                if (it.key() == "time") continue;   // time 不作为 Y 轴
                headers << it.key();
                ys << it.value();
            }
            // 获取 mPlotView*
            mPlotView* plotView = mCurveWindow::getInstance()->getPlotView();
            if (!plotView) return;

            // 设置 X 轴
            AxisSettings xSettings = plotView->getAxisSettings(X_Bottom);
            xSettings.title = "Time";
            plotView->updateAxisAppearance(X_Bottom, xSettings);
            // 设置 Y 轴
            AxisSettings ySettings = plotView->getAxisSettings(Y_Left);
            //ySettings.title = headers.value(0);  // 第一条曲线名
            ySettings.title = "Unit";
            plotView->updateAxisAppearance(Y_Left, ySettings);
            curveData->addImportParam(filename);
        }
		else if (ctype == "路径")
		{
			if (_listComponent->count() > 0 && compos.size() == 0)
			{
				mGlobalSignals::getInstance()->outputMessageSig(2, QString("没有选择分量!"), true);
				clearGeneratedCurve();
				return;
			}

            if (_listLayer->count() > 0 && layers.size() == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("没有选择层!"), true);
                clearGeneratedCurve();
                return;
            }

            if (_isComplex && complexs.size() == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("没有选择复数输出类型!"), true);
                clearGeneratedCurve();
                return;
            }

            //读取曲线数据
            _curveNames.clear();
            _XValueD.clear();
            _XValueF.clear();
            _YValuesD.clear();
            _YValuesF.clear();

            QString pathName = _comboPath->currentText();

            QVector<int> nodes = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getIdsByPathName(pathName);
            if (nodes.size() == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig(0, QString("路径节点获取失败！"), true);
                clearGeneratedCurve();
                return;
            }

            QString caseName = _comboCase->currentText();
            //获取该帧的变量
            QString fName = _comboFrame->currentText();
            if (fName.contains(":"))
                fName = fName.split(":").at(0);
            int fi = _postDB->getFrameNumByCaseAndFrameName(_comboCase->currentText(), fName);

            mPostModelData* modeldata = _postDB->getModelData(fi);
            if (modeldata == nullptr)
            {
                clearGeneratedCurve();
                return;
            }

            QVector<QVector3D> nodeVerter;
            for (auto iter = nodes.begin(); iter != nodes.end(); ++iter)
            {
                int cId = modeldata->_nodedata.nodeidmap.value(*iter);
                QVector3D vertex = modeldata->getCoordinates(cId);
                nodeVerter.push_back(vertex);
            }

            //不包含交点
            if (!_checkPath->isChecked())
            {
                QString xtype;
                //获取X值
                QVector<double> XValues = calPathCurveXvalues(nodeVerter, xtype);
                if (XValues.size() == 0)
                {
                    clearGeneratedCurve();
                    return;
                }
                _XValueD = XValues;

                //获取Y值
                getPathCurveYvalues(nodes);
            }
            //包含交点
            else
            {
                mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
                if (postRend == nullptr)
                {
                    clearGeneratedCurve();
                    return;
                }

                auto postRender = postRend->getPostRender();
                if (postRender == nullptr)
                {
                    clearGeneratedCurve();
                    return;
                }

                // S5 门面直接替换：getCurFrameDataProxy 消亡→frameProxy(currentFrame()) 现取（F8：换帧/重装载后重取）
                mxr::PostFrameDataProxy* curFrameProxy = postRender->frameProxy(postRender->currentFrame());
                mxr::IPostRendQueryDataProxy* query_proxy = dynamic_cast<mxr::IPostRendQueryDataProxy*>(curFrameProxy ? curFrameProxy->getRendQueryDataProxy() : nullptr);
                mxr::IPostModelRenderDataProxy* model_query_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(curFrameProxy ? curFrameProxy->getModelRenderDataProxy() : nullptr);
                if (query_proxy == nullptr || model_query_proxy == nullptr)
                {
                    clearGeneratedCurve();
                    return;
                }

                //部件对应的单元
                std::vector<std::pair<int, int>> eleIds{};
                ZsTempRender* tempRender = new ZsTempRender();

                std::vector<vector<QVector3D>> pathIntersectPoint{};
                //获取交点 包含所有点（包括路径点和交点）
                QVector<QVector3D> allIntersectPoint;
                for (int i = 0; i < nodes.size() - 1; i++)
                {
                    int startPointId = nodes[i];
                    int endPointId = nodes[i + 1];
                    //交点坐标
                    std::vector<QVector3D> pointPos;
                    //获取交点
                    tempRender->chooseNode(startPointId, endPointId, fi, modeldata, _postDB, query_proxy, model_query_proxy, eleIds, pointPos);
                    pathIntersectPoint.push_back(pointPos);

                    for (const auto& vec3D : pointPos)
                    {
                        if (!allIntersectPoint.contains(vec3D))
                        {
                            allIntersectPoint.append(vec3D);
                        }
                    }
                }

                QString xtype;
                //获取X值
                QVector<double> XValues = calPathCurveXvalues(allIntersectPoint, xtype);
                if (XValues.size() == 0)
                {
                    clearGeneratedCurve();
                    return;
                }
                _XValueD = XValues;

                //获取Y值
                getPathCurveYvalues(nodes);
                //处理Y值
                QHash<QString, QVector<double>>	YValuesD;
                if (_YValuesD.size() > 0)
                {
                    YValuesD = _YValuesD;
                }
                else
                {
                    // 遍历源哈希表并进行转换
                    for (auto it = _YValuesF.begin(); it != _YValuesF.end(); ++it) {
                        const QString& key = it.key();
                        const QVector<float>& floatVector = it.value();

                        QVector<double> doubleVector;
                        // 将 floatVector 中的元素逐个转换为 double 类型
                        for (float value : floatVector) {
                            doubleVector.append(static_cast<double>(value));
                        }
                        YValuesD.insert(key, doubleVector);
                    }
                }


                QHash<QString, QVector<double>>	intersectYVals;
                for (auto it = YValuesD.begin(); it != YValuesD.end(); ++it)
                {
                    QVector<double> tempVector;

                    QString key = it.key();
                    QVector<double> curveYval = it.value();

                    for (int i = 0; i < nodes.size() - 1; i++)
                    {
                        int startPointId = nodes[i];
                        int endPointId = nodes[i + 1];
                        if (i == 0)
                        {
                            tempVector.push_back(curveYval[i]);
                        }

                        //计算起点、终点距离
                        int cSId = modeldata->_nodedata.nodeidmap.value(startPointId);
                        QVector3D sVertex = modeldata->getCoordinates(cSId);
                        int cEId = modeldata->_nodedata.nodeidmap.value(endPointId);
                        QVector3D eVertex = modeldata->getCoordinates(cEId);
                        double totalDistance = sVertex.distanceToPoint(eVertex);

                        //根据交点距离占比计算结果值
                        for (int j = 1; j < pathIntersectPoint[i].size() - 1; j++)
                        {
                            double distance = sVertex.distanceToPoint(pathIntersectPoint[i][j]);
                            double ratio = distance / totalDistance;
                            double value = ratio * curveYval[i] + (1 - ratio) * curveYval[i + 1];
                            tempVector.push_back(value);
                        }
                        tempVector.push_back(curveYval[i + 1]);
                    }
                    intersectYVals.insert(key, tempVector);
                }
                _YValuesD = intersectYVals;
            }

            curveData->addPathParam(pathName, _checkPath->isChecked(), _btnGroupPath->checkedId(), var, compos, layers, complexs);
            curveData->_curveComplexList = complexs;

            int ncurve = _curveNames.size();
            if (ncurve == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("曲线生成失败!"), true);
                clearGeneratedCurve();
                return;
            }

            // 处理曲线数据
            getCurveDatas(xs, ys, headers);
        }

		//显示曲线
		mDockLayoutManager::getInstance()->activateDock("Curve");
		mCurveWindow::getInstance()->updateMultiCurves(xs, ys, PlotView::LineChart, headers, true, true);
      
		//更新节点状态为正确
		curveData->_isCurveGenerated = true;
		curveData->_lastCurveType = ctype;
		updateItemStatus();
		curveData->addCurve(xs, ys, headers);
		return;
	}

    void mPostCurvePanel::slot_exportCurve()
    {
        if (_postDB == nullptr) return;

        PostData::mPostCurveData* curveData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getCurveData(_curViewName);
        if (curveData == nullptr)
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, QString("曲线数据为空，曲线导出失败!"), true);
            return;
        }
            

        if (_dataTable == nullptr)
            _dataTable = new mPostDataTable(this);


        _dataTable->setTableData(curveData->_curveHeaders, "x轴", curveData->_xValues, curveData->_yValues);

        _dataTable->show();

    }

    PostData::mPostCurveData* mPostCurvePanel::getCurveData()
    {
        return getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getCurveData(_curViewName);
    }

    void mPostCurvePanel::hideEvent(QHideEvent* event)
    {
        clearhighlight();
    }

}
