
#include "mMeshPanel.h"
#include "mAppConfiguration.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mCalcLineEdit.h"
#include "mGlobalSignals.h"

#include "mPythonAgent.h"
#include "mPyScriptWriter.h"
#include "mPyScriptReader.h"

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
#include <QHBoxLayout>
#include <QStyledItemDelegate>
#include <QMetaType>      // 元类型注册函数
#include <QSet>

//BasicData
#include "mCaseBase.h"
#include "mCaseManager.h"
#include "mCaseImplicitLinearStatic.h"
#include "mCaseImplicitModal.h"
#include "mCaseImplicitNonLinearStatic.h"
#include "mCaseImplicitLinearBuckling.h"
#include "mCaseImplicitTransient.h"
#include "mModelData.h"
#include "mMeshData.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/view/mFontRender.h"
//PreData
#include "mGeoManager.h"
//Mesh
//#include "MXMeshAPI.h"
#include "MXMesh.h"
#include "meshEdge.h"//偏置分布（meshBiasEdge）
#include "mxr/view/mPreMeshHighLightRender.h"
#include "mxr/view/HighlightController.h"

#include "mxr/label/mSymbolController.h"
#include "mxr/label/SymbolDescription.h"
#include "PickCommonFunction.h"

#include "mInfoWindow.h"
#include "mLocalSizePanel.h"
#include "GlobalMeshMessage.h"
using namespace MechPreData;
using namespace MeshPanelData;
using namespace MxFunctions;
#include <set>
#include "mOtherFunction.h"
Q_DECLARE_METATYPE(std::set<int>)
Q_DECLARE_METATYPE(MeshParam)          // 头文件

namespace MechMainWindow
{

    //"数量"列编辑代理：编辑器仅允许输入 >0 的正整数
    class NumPositiveIntDelegate : public QStyledItemDelegate
    {
    public:
        using QStyledItemDelegate::QStyledItemDelegate;
        QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override
        {
            QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);
            if (auto* edit = qobject_cast<QLineEdit*>(editor))
                edit->setValidator(new QRegularExpressionValidator(QRegularExpression("^(?:[1-9]|[1-9]\\d+)$"), edit));
            return editor;
        }
    };


    //偏置值钳制到 [-20, 20]：超出时 meshBiasEdge 的首段长度会 <=0，分布公式失效；
    //发生钳制时输出警告
    static double clampBiasValue(double bias)
    {
        if (bias > 20.0 || bias < -20.0)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1,
                QStringLiteral("偏置值的范围[-20,20]"), true);
        }
        if (bias > 20.0) return 20.0;
        if (bias < -20.0) return -20.0;
        return bias;
    }

    //"偏置"列编辑代理：Enter 提交时把超范围输入钳制到 [-20, 20] 再写入单元格
    class BiasFactorDelegate : public QStyledItemDelegate
    {
    public:
        using QStyledItemDelegate::QStyledItemDelegate;
        void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
        {
            if (auto* edit = qobject_cast<QLineEdit*>(editor))
            {
                double bias = clampBiasValue(edit->text().toDouble());
                model->setData(index, QString::number(bias), Qt::EditRole);
                return;
            }
            QStyledItemDelegate::setModelData(editor, model, index);
        }
    };

    mLocalSizePanel::mLocalSizePanel(QWidget* parent)
    {


        _factor = 0;
        auto* mainLay = new QVBoxLayout(this);


        _meshControlLabel = new PanelLabel("控制方法");
        _meshControlBox = new QComboBox();

        _meshControlBox->addItems({ "根据数量" , "根据尺寸" });//
        m_rangeFold = new FoldSection("尺寸控制", this);
        auto* rangeLay = new QHBoxLayout;
        _meshSizeLabel = new PanelLabel("网格尺寸");
        _meshSizeEdit = new QLineEdit("");
        auto* v = new QDoubleValidator(0.001, 1e11, 8, _meshSizeEdit);
        v->setNotation(QDoubleValidator::ScientificNotation);
        _meshSizeEdit->setValidator(v);
        _meshSizeButton = new QToolButton(); _meshSizeButton->setIcon(QIcon(":/mMainWidgets/Icon/post/reset.png")); _meshSizeButton->setToolTip("恢复默认尺寸");
        rangeLay->addWidget(_meshSizeLabel);
        rangeLay->addWidget(_meshSizeEdit);
        rangeLay->addWidget(_meshSizeButton);
        auto* rangeLay1 = new QHBoxLayout;
        _meshNumLabel = new PanelLabel("网格数量");
        _meshNumEdit = new QLineEdit("3");
        auto* v1 = new QRegularExpressionValidator(
            QRegularExpression("^(?:[1-9]|[1-9]\\d+)$"),   // >0 的正整数
            _meshNumEdit);
        _meshNumEdit->setValidator(v1);
        _meshNumButton = new QToolButton(); _meshNumButton->setIcon(QIcon(":/mMainWidgets/Icon/post/reset.png")); _meshNumButton->setToolTip("恢复默认尺寸");
        rangeLay1->addWidget(_meshNumLabel);
        rangeLay1->addWidget(_meshNumEdit);
        rangeLay1->addWidget(_meshNumButton);
        _meshSizeLabel->hide();
        _meshSizeEdit->hide();
        _meshSizeButton->hide();
        m_rangeFold->addRow(_meshControlLabel, _meshControlBox);
        m_rangeFold->addRow(rangeLay);
        m_rangeFold->addRow(rangeLay1);

        _meshthread = new mMeshThread(this);


        mainLay->addWidget(m_rangeFold);
        // ========== 几何体折叠块 ==========
        m_GeoFold = new FoldSection("几何线", this);
        auto* geoLay = new QHBoxLayout;
        //_geoLabel = new QLabel("部件集合");




        //已经赋予的部件列表
        QGridLayout* assignLayout = new QGridLayout();
        //_listAssignPart = new QListWidget();
        //_listAssignPart->setSelectionMode(QAbstractItemView::ExtendedSelection);
        stackWidget = new QStackedWidget;
        sizeTable = new QTableWidget;
        sizeTable->setColumnCount(3);
        sizeTable->setHorizontalHeaderLabels({ "ID","尺寸","偏置" });
        sizeTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        sizeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        sizeTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
        numTable = new QTableWidget;
        numTable->setColumnCount(3);
        numTable->setHorizontalHeaderLabels({ "ID","数量","偏置" });
        numTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        numTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        numTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
        numTable->setItemDelegateForColumn(1, new NumPositiveIntDelegate(numTable));
        //偏置列编辑器限制在 [-20, 20]
        sizeTable->setItemDelegateForColumn(2, new BiasFactorDelegate(sizeTable));
        numTable->setItemDelegateForColumn(2, new BiasFactorDelegate(numTable));
        stackWidget->addWidget(sizeTable);
        stackWidget->addWidget(numTable);
        stackWidget->setCurrentIndex(1);
        allBtn = new QToolButton(); allBtn->setIcon(QIcon(":/mMainWidgets/Icon/xuanzequanbu.png")); allBtn->setToolTip("全部几何线");
        /* allBtn->setStyleSheet(R"(
         QToolButton::hover
         {
         background-color: #299DFF;
         }
     )");*/
        addBtn = new QToolButton(); addBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png")); addBtn->setToolTip("增加几何线");
        reduceBtn = new QToolButton(); reduceBtn->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png")); reduceBtn->setToolTip("减少几何线");
        deleteBtn = new QToolButton(); deleteBtn->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png")); deleteBtn->setToolTip("清空几何线");

        assignLayout->addWidget(stackWidget, 0, 0, 8, 7);
        assignLayout->addWidget(allBtn, 0, 7);
        assignLayout->addWidget(addBtn, 1, 7);
        assignLayout->addWidget(reduceBtn, 2, 7);
        assignLayout->addWidget(deleteBtn, 3, 7);


        //m_GeoFold->addRow(new PanelLabel("几何线集合"), _geoComboBox);
        m_GeoFold->addRow(assignLayout);
        m_foldWidgets.append(m_GeoFold);

        mainLay->addWidget(m_GeoFold);
        // ========== 阶次折叠块 ==========
        // 偏移功能暂时屏蔽，仅注释不删除，后续恢复时取消注释即可
        //m_OrderFold = new FoldSection("偏移", this);
        //_orderComboBox = new QComboBox();
        //_orderComboBox->setItemData(0, 0, Qt::UserRole - 1); // 禁止选中
        //_orderComboBox->setCurrentIndex(0);                 // 默认不选中任何项
        //_orderComboBox->addItems({ "鼠标点击：左加右减" , "偏移因子" });//

        //_offsetDistanceLabel = new PanelLabel("偏移间隔");
        //_offsetDistanceEdit = new QLineEdit("1");

        //_offsetDistanceLabel1 = new PanelLabel("偏移因子");
        //_offsetDistanceEdit1 = new QLineEdit("0");
        //m_OrderFold->addRow(new PanelLabel("调整方式"), _orderComboBox);
        //m_OrderFold->addRow(_offsetDistanceLabel, _offsetDistanceEdit);
        //m_OrderFold->addRow(_offsetDistanceLabel1, _offsetDistanceEdit1);
        //_offsetDistanceLabel1->hide();
        //_offsetDistanceEdit1->hide();
        //_offsetDistanceEdit1->setValidator(new Max20Validator(this));

        //mainLay->addWidget(m_OrderFold);
        mainLay->addStretch();
        connect(_meshNumEdit, &QLineEdit::textChanged, this, [=]() {
            _meshPanelData->setMeshNum(_meshNumEdit->text().toInt());
            });
        connect(_meshNumEdit, &QLineEdit::editingFinished, this, [=]() {
            if (stackWidget->currentIndex() != 1)
                return;
            // 同步更新几何线面板 numTable 中所有行的数量
            QString newNumText = _meshNumEdit->text();
            int rowCount = numTable->rowCount();
            if (rowCount <= 0)
                return;
            for (int row = 0; row < rowCount; ++row)
            {
                QTableWidgetItem* item = numTable->item(row, 1);
                if (!item)
                    continue;
                item->setText(newNumText);
                // 同时更新 _meshPanelData 中对应几何线的映射
                QTableWidgetItem* idItem = numTable->item(row, 0);
                if (!idItem)
                    continue;
                QStringList list = idItem->text().split("_");
                if (list.size() < 2)
                    continue;
                int id = list[1].toInt();
                int num = newNumText.toInt();
                _meshPanelData->_ids2NumsMap[id] = num;
                auto geoManager = getCVM->getModelData()->getGeoManager();
                vector<int> res; res.push_back(id);
                auto lines = getGEOServer.geoCalculate().CalculateEdgeAttr(res);
                if (!lines.empty())
                {
                    double length = lines[0].m_length;
                    _meshPanelData->_idNums2SizesMap[id] = length / num;
                }
            }
            emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
            });
        connect(_meshSizeEdit, &QLineEdit::textChanged, this, [=]() {
            _meshPanelData->setMeshSize(_meshSizeEdit->text().toDouble());
            });
        connect(_meshSizeEdit, &QLineEdit::editingFinished, this, [=]() {
            if (stackWidget->currentIndex() != 0)
                return;
            // 同步更新几何线面板 sizeTable 中所有行的尺寸
            QString newSizeText = _meshSizeEdit->text();
            int rowCount = sizeTable->rowCount();
            if (rowCount <= 0)
                return;
            for (int row = 0; row < rowCount; ++row)
            {
                QTableWidgetItem* item = sizeTable->item(row, 1);
                if (!item)
                    continue;
                item->setText(newSizeText);
                // 同时更新 _meshPanelData 中对应几何线的映射
                QTableWidgetItem* idItem = sizeTable->item(row, 0);
                if (!idItem)
                    continue;
                QStringList list = idItem->text().split("_");
                if (list.size() < 2)
                    continue;
                int id = list[1].toInt();
                double size = newSizeText.toDouble();
                _meshPanelData->_ids2SizesMap[id] = size;
            }
            emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
            });
        connect(sizeTable, &QTableWidget::cellChanged,
            this, [=](int row, int column) {
                if (column == 1)
                {
                    QTableWidgetItem* item = sizeTable->item(row, column);
                    if (item) {
                        QString newValue = item->text();
                        QStringList list = sizeTable->item(row, 0)->text().split("_");
                        _meshPanelData->_ids2SizesMap[list[1].toInt()] = newValue.toDouble();
                    }

                }
                else if (column == 2)
                {
                    //偏置列：按几何线ID记录偏置值（提交前已经 BiasFactorDelegate 钳制）
                    QTableWidgetItem* item = sizeTable->item(row, column);
                    QTableWidgetItem* idItem = sizeTable->item(row, 0);
                    if (item && idItem) {
                        QStringList list = idItem->text().split("_");
                        if (list.size() < 2)
                            return;
                        //_meshPanelData 来自旧版 MeshPanelData.dll，不含 _ids2BiasMap，暂存本面板
                        _ids2BiasCache[list[1].toInt()] = item->text().toDouble();
                    }
                }

            });
        connect(numTable, &QTableWidget::cellChanged,
            this, [=](int row, int column) {
                if (column == 1)
                {
                    QTableWidgetItem* item = numTable->item(row, column);
                    QTableWidgetItem* idItem = numTable->item(row, 0);
                    if (item && idItem) {
                        QString newValue = item->text();
                        QStringList list = idItem->text().split("_");
                        if (list.size() < 2)
                            return;
                        int id = list[1].toInt();
                        int num = newValue.toInt();
                        if (num <= 0)
                        {
                            // 非法值（如编辑后被清空）：回退为该行最近的有效数量
                            auto it = _meshPanelData->_ids2NumsMap.find(id);
                            QString fallback = (it != _meshPanelData->_ids2NumsMap.end())
                                ? QString::number(it->second) : _meshNumEdit->text();
                            numTable->blockSignals(true);
                            item->setText(fallback);
                            numTable->blockSignals(false);
                            return;
                        }
                        _meshPanelData->_ids2NumsMap[id] = num;
                        vector<int> res; res.push_back(id);
                        auto geoManager = getCVM->getModelData()->getGeoManager();
                        auto lines = getGEOServer.geoCalculate().CalculateEdgeAttr(res);
                        if (!lines.empty())
                        {
                            double length = lines[0].m_length;
                            double size = length / num;
                            _meshPanelData->_idNums2SizesMap[id] = size;
                        }
                    }

                }
                else if (column == 2)
                {
                    //偏置列：按几何线ID记录偏置值（提交前已经 BiasFactorDelegate 钳制）
                    QTableWidgetItem* item = numTable->item(row, column);
                    QTableWidgetItem* idItem = numTable->item(row, 0);
                    if (item && idItem) {
                        QStringList list = idItem->text().split("_");
                        if (list.size() < 2)
                            return;
                        //_meshPanelData 来自旧版 MeshPanelData.dll，不含 _ids2BiasMap，暂存本面板
                        _ids2BiasCache[list[1].toInt()] = item->text().toDouble();
                    }
                }

            });

        // 表格行选择变化时，在视口高亮对应几何边
        auto highlightSelectedTableEdges = [=]() {
            CCSMRendWidget* preRend = getCVM->getPreRenderWidget();
            if (!preRend)
                return;
            QTableWidget* table = (stackWidget->currentIndex() == 0) ? sizeTable : numTable;
            QList<QTableWidgetItem*> selectedItems = table->selectedItems();
            std::set<int> selectedRows;
            for (auto* item : selectedItems)
                selectedRows.insert(item->row());

            std::vector<int> edgeIds;
            edgeIds.reserve(selectedRows.size());
            for (int row : selectedRows)
            {
                QTableWidgetItem* idItem = table->item(row, 0);
                if (!idItem)
                    continue;
                QStringList list = idItem->text().split("_");
                if (list.size() < 2)
                    continue;
                edgeIds.push_back(list[1].toInt());
            }

            ClearPreWidgetGeoPick(preRend);
            if (!edgeIds.empty())
            {
                preRend->getSelectionContainer()->getGeoSelectData()->UpdatePickData(
                    SelectionOperate::ADD, SelectionInfoType::GeoEdge, edgeIds);
            }
            MechPreData::UpdatePreWidgetHighlight(preRend);
            preRend->updatePaint();
            };
        connect(sizeTable, &QTableWidget::itemSelectionChanged, this, highlightSelectedTableEdges);
        connect(numTable, &QTableWidget::itemSelectionChanged, this, highlightSelectedTableEdges);
        // 切换数量/尺寸面板时清空遗留高亮，并在新面板根据当前选中项刷新高亮
        connect(stackWidget, &QStackedWidget::currentChanged, this, [=](int) {
            CCSMRendWidget* preRend = getCVM->getPreRenderWidget();
            if (preRend)
            {
                ClearPreWidgetGeoPick(preRend);
                MechPreData::UpdatePreWidgetHighlight(preRend);
                preRend->updatePaint();
            }
            highlightSelectedTableEdges();
            });

        connect(allBtn, &QToolButton::clicked, [=]() {
            //根据尺寸
            std::set<int> meshGeoId = getGEOServer.geoCalculate().getAllGeoID(1);
            QSet<QString> existSet;
            if (stackWidget->currentIndex() == 0)
            {
                for (int i = 0; i < sizeTable->rowCount(); ++i)
                    existSet.insert(sizeTable->item(i, 0)->text());
                for (int id : meshGeoId)                    // set 自动升序
                {
                    QString txt = QString("几何线_%1").arg(id);
                    if (!existSet.contains(txt))
                    {
                        int rowCount = sizeTable->rowCount();
                        sizeTable->insertRow(rowCount);
                        // 为新行的单元格设置数据
                        sizeTable->setItem(rowCount, 0, new QTableWidgetItem(txt));
                        auto item = sizeTable->item(rowCount, 0); item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                        sizeTable->setItem(rowCount, 1, new QTableWidgetItem(_meshSizeEdit->text()));
                        sizeTable->setItem(rowCount, 2, new QTableWidgetItem("0"));
                    }
                }
                _meshPanelData->_list.clear();
                _meshPanelData->_ids2SizesMap.clear();
                _ids2BiasCache.clear();
                for (int i = 0; i < sizeTable->rowCount(); ++i)
                {
                    QStringList list = sizeTable->item(i, 0)->text().split("_");
                    _meshPanelData->_list << sizeTable->item(i, 0)->text();
                    _meshPanelData->_ids2SizesMap[list[1].toInt()] = sizeTable->item(i, 1)->text().toDouble();
                    _ids2BiasCache[list[1].toInt()] = clampBiasValue(sizeTable->item(i, 2)->text().toDouble());
                }


            }
            //根据数量
            else if (stackWidget->currentIndex() == 1)
            {
                for (int i = 0; i < numTable->rowCount(); ++i)
                    existSet.insert(numTable->item(i, 0)->text());
                for (int id : meshGeoId)                    // set 自动升序
                {
                    QString txt = QString("几何线_%1").arg(id);
                    if (!existSet.contains(txt))
                    {
                        int rowCount = numTable->rowCount();
                        numTable->insertRow(rowCount);
                        // 为新行的单元格设置数据
                        numTable->setItem(rowCount, 0, new QTableWidgetItem(txt));
                        auto item = numTable->item(rowCount, 0); item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                        numTable->setItem(rowCount, 1, new QTableWidgetItem(_meshNumEdit->text()));
                        numTable->setItem(rowCount, 2, new QTableWidgetItem("0"));
                    }
                }
                _meshPanelData->_list.clear();
                _meshPanelData->_ids2NumsMap.clear();
                _meshPanelData->_idNums2SizesMap.clear();
                _ids2BiasCache.clear();

                auto geoManager = getCVM->getModelData()->getGeoManager();
                for (int i = 0; i < numTable->rowCount(); ++i)
                {
                    QStringList list = numTable->item(i, 0)->text().split("_");
                    int id = list[1].toInt();
                    vector<int> res; res.push_back(id);
                    auto lines = getGEOServer.geoCalculate().CalculateEdgeAttr(res);
                    double length = lines[0].m_length;
                    double size = length / numTable->item(i, 1)->text().toInt();
                    _meshPanelData->_list << numTable->item(i, 0)->text();
                    _meshPanelData->_ids2NumsMap[id] = numTable->item(i, 1)->text().toInt();
                    _meshPanelData->_idNums2SizesMap[id] = size;
                    _ids2BiasCache[id] = clampBiasValue(numTable->item(i, 2)->text().toDouble());
                }

            }
            emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
            });
        connect(addBtn, &QToolButton::clicked, [=]() {
            //根据尺寸
            vector<int> lineids = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
            QSet<QString> existSet;
            if (stackWidget->currentIndex() == 0)
            {
                for (int i = 0; i < sizeTable->rowCount(); ++i)
                    existSet.insert(sizeTable->item(i, 0)->text());
                for (int id : lineids)                    // set 自动升序
                {
                    QString txt = QString("几何线_%1").arg(id);
                    if (!existSet.contains(txt))
                    {
                        int rowCount = sizeTable->rowCount();
                        sizeTable->insertRow(rowCount);
                        // 为新行的单元格设置数据
                        sizeTable->setItem(rowCount, 0, new QTableWidgetItem(txt));
                        auto item = sizeTable->item(rowCount, 0); item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                        sizeTable->setItem(rowCount, 1, new QTableWidgetItem(_meshSizeEdit->text()));
                        sizeTable->setItem(rowCount, 2, new QTableWidgetItem("0"));
                    }
                }
                _meshPanelData->_list.clear();
                _meshPanelData->_ids2SizesMap.clear();
                _ids2BiasCache.clear();
                for (int i = 0; i < sizeTable->rowCount(); ++i)
                {
                    QStringList list = sizeTable->item(i, 0)->text().split("_");
                    _meshPanelData->_list << sizeTable->item(i, 0)->text();
                    _meshPanelData->_ids2SizesMap[list[1].toInt()] = sizeTable->item(i, 1)->text().toDouble();
                    _ids2BiasCache[list[1].toInt()] = clampBiasValue(sizeTable->item(i, 2)->text().toDouble());
                }

            }
            //根据数量
            else if (stackWidget->currentIndex() == 1)
            {
                for (int i = 0; i < numTable->rowCount(); ++i)
                    existSet.insert(numTable->item(i, 0)->text());
                for (int id : lineids)                    // set 自动升序
                {
                    QString txt = QString("几何线_%1").arg(id);
                    if (!existSet.contains(txt))
                    {
                        int rowCount = numTable->rowCount();
                        numTable->insertRow(rowCount);
                        // 为新行的单元格设置数据
                        numTable->setItem(rowCount, 0, new QTableWidgetItem(txt));
                        auto item = numTable->item(rowCount, 0); item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                        numTable->setItem(rowCount, 1, new QTableWidgetItem(_meshNumEdit->text()));
                        numTable->setItem(rowCount, 2, new QTableWidgetItem("0"));
                    }
                }
                _meshPanelData->_list.clear();
                _meshPanelData->_ids2NumsMap.clear();
                _meshPanelData->_idNums2SizesMap.clear();
                _ids2BiasCache.clear();
                auto geoManager = getCVM->getModelData()->getGeoManager();
                for (int i = 0; i < numTable->rowCount(); ++i)
                {
                    QStringList list = numTable->item(i, 0)->text().split("_");
                    int id = list[1].toInt();
                    vector<int> res; res.push_back(id);
                    auto lines = getGEOServer.geoCalculate().CalculateEdgeAttr(res);
                    double length = lines[0].m_length;
                    double size = length / numTable->item(i, 1)->text().toInt();
                    _meshPanelData->_list << numTable->item(i, 0)->text();
                    _meshPanelData->_ids2NumsMap[id] = numTable->item(i, 1)->text().toInt();
                    _meshPanelData->_idNums2SizesMap[id] = size;
                    _ids2BiasCache[id] = clampBiasValue(numTable->item(i, 2)->text().toDouble());
                }

            }
            emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
            });
        connect(reduceBtn, &QToolButton::clicked, [=]() {
            // 收集将被移除的几何边ID，用于同步清理剖分数据缓存
            QSet<int> removedIds;
            mMeshData* meshdata = getCVM->getModelData()->getMeshData();
            //根据尺寸
            if (stackWidget->currentIndex() == 0)
            {
                QSet<int> selectedRow;
                QModelIndexList selectedRows = sizeTable->selectionModel()->selectedRows();
                if (selectedRows.isEmpty())
                    return;
                for (int i = selectedRows.count() - 1; i >= 0; --i)
                {
                    int row = selectedRows[i].row();
                    QStringList list = sizeTable->item(row, 0)->text().split("_");
                    if (list.size() > 1)
                        removedIds.insert(list[1].toInt());
                    sizeTable->removeRow(row);
                }
                _meshPanelData->_list.clear();
                _meshPanelData->_ids2SizesMap.clear();
                _ids2BiasCache.clear();
                for (int i = 0; i < sizeTable->rowCount(); ++i)
                {
                    QStringList list = sizeTable->item(i, 0)->text().split("_");
                    _meshPanelData->_list << sizeTable->item(i, 0)->text();
                    _meshPanelData->_ids2SizesMap[list[1].toInt()] = sizeTable->item(i, 1)->text().toDouble();
                    _ids2BiasCache[list[1].toInt()] = clampBiasValue(sizeTable->item(i, 2)->text().toDouble());
                }
            }
            //根据数量
            else if (stackWidget->currentIndex() == 1)
            {
                QSet<int> selectedRow;
                QModelIndexList selectedRows = numTable->selectionModel()->selectedRows();
                if (selectedRows.isEmpty())
                    return;
                for (int i = selectedRows.count() - 1; i >= 0; --i)
                {
                    int row = selectedRows[i].row();
                    QStringList list = numTable->item(row, 0)->text().split("_");
                    if (list.size() > 1)
                        removedIds.insert(list[1].toInt());
                    numTable->removeRow(row);
                }
                _meshPanelData->_list.clear();
                _meshPanelData->_ids2NumsMap.clear();
                _meshPanelData->_idNums2SizesMap.clear();
                _ids2BiasCache.clear();
                auto geoManager = getCVM->getModelData()->getGeoManager();
                for (int i = 0; i < numTable->rowCount(); ++i)
                {
                    QStringList list = numTable->item(i, 0)->text().split("_");
                    int id = list[1].toInt();
                    vector<int> res; res.push_back(id);
                    auto lines = getGEOServer.geoCalculate().CalculateEdgeAttr(res);
                    double length = lines[0].m_length;
                    double size = length / numTable->item(i, 1)->text().toInt();
                    _meshPanelData->_list << numTable->item(i, 0)->text();
                    _meshPanelData->_ids2NumsMap[id] = numTable->item(i, 1)->text().toInt();
                    _meshPanelData->_idNums2SizesMap[id] = size;
                    _ids2BiasCache[id] = clampBiasValue(numTable->item(i, 2)->text().toDouble());
                }
            }
            // 从剖分数据缓存中清除被移除几何边的节点数与偏置值
            if (meshdata)
            {
                for (int gid : removedIds)
                {
                    meshdata->GEdgeIDTodynaNodeNum.remove(gid);
                    meshdata->GEdgeIDToBiasValue.remove(gid);
                }
            }
            emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
            });
        connect(deleteBtn, &QToolButton::clicked, [=]() {
            // 收集将被清空的几何边ID
            QSet<int> removedIds;
            mMeshData* meshdata = getCVM->getModelData()->getMeshData();
            if (stackWidget->currentIndex() == 0)
            {
                for (int i = 0; i < sizeTable->rowCount(); ++i)
                {
                    QStringList list = sizeTable->item(i, 0)->text().split("_");
                    if (list.size() > 1)
                        removedIds.insert(list[1].toInt());
                }
                sizeTable->setRowCount(0);
                _meshPanelData->_list.clear();
                _meshPanelData->_ids2SizesMap.clear();
                _ids2BiasCache.clear();
            }
            else if (stackWidget->currentIndex() == 1)
            {
                for (int i = 0; i < numTable->rowCount(); ++i)
                {
                    QStringList list = numTable->item(i, 0)->text().split("_");
                    if (list.size() > 1)
                        removedIds.insert(list[1].toInt());
                }
                numTable->setRowCount(0);
                _meshPanelData->_list.clear();
                _meshPanelData->_ids2NumsMap.clear();
                _meshPanelData->_idNums2SizesMap.clear();
                _ids2BiasCache.clear();
            }
            // 从剖分数据缓存中清除被删除几何边的节点数与偏置值
            if (meshdata)
            {
                for (int gid : removedIds)
                {
                    meshdata->GEdgeIDTodynaNodeNum.remove(gid);
                    meshdata->GEdgeIDToBiasValue.remove(gid);
                }
            }

            emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
            });

        connect(_meshSizeButton, &QToolButton::clicked, [=]() {_meshSizeEdit->setText("2"); });
        connect(_meshNumButton, &QToolButton::clicked, [=]() {_meshNumEdit->setText("2"); });

        connect(_meshthread, &mMeshThread::finishedAdjust, [=]() {
            cout << 1;
            });
        connect(_meshControlBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index)
            {
                _meshPanelData->setCtrlMethod(_meshControlBox->currentText());
                if (index == 1)
                {
                    _meshNumLabel->hide();
                    _meshNumEdit->hide();
                    _meshNumButton->hide();
                    _meshSizeLabel->show();
                    _meshSizeEdit->show();
                    _meshSizeButton->show();
                    //meshSizeChanged();
                    stackWidget->setCurrentIndex(0);
                }
                else if (index == 0)
                {
                    _meshNumLabel->show();
                    _meshNumEdit->show();
                    _meshNumButton->show();
                    _meshSizeLabel->hide();
                    _meshSizeEdit->hide();
                    _meshSizeButton->hide();
                    //meshNumChanged();
                    stackWidget->setCurrentIndex(1);
                }
            }
        );

        //偏移因子（偏移功能暂时屏蔽，相关连接一并注释，恢复时取消注释即可）
        //connect(_offsetDistanceEdit1, &QLineEdit::editingFinished, this, &mLocalSizePanel::meshBiasChanged);

        //connect(_orderComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index)
        //    {
        //        if (flag == true)
        //            _factor += _offsetDistanceEdit->text().toInt();
        //        else
        //            _factor -= _offsetDistanceEdit->text().toInt();
        //        _offsetDistanceEdit1->setText(MxFunctions::mNumberToString(_factor));
        //        meshBiasChanged();
        //    }
        //);
        //connect(_offsetDistanceEdit, &QLineEdit::editingFinished, [=]() {
        //    getCVM->getPreRenderWidget()->setPickFilter(PickFilter::PickBiasNum);

        //    });
        //左加右减
        /*connect(getCVM->getPreRenderWidget()->getStateMachineManager(), &StateMachineManager::signal_finishPickingBiasNumSig, this, [=](bool flag) {
            if (_meshPanelData == nullptr)
                return;
            if (_orderComboBox->currentIndex() == 0)
            {
                if (flag == true)
                    _factor += _offsetDistanceEdit->text().toInt();
                else
                    _factor -= _offsetDistanceEdit->text().toInt();
                _offsetDistanceEdit1->setText(QString::number(_factor));
                meshBiasChanged();
            }

            });*/
        //网格数量
        //connect(_meshNumEdit, &QLineEdit::editingFinished, this, &mLocalSizePanel::meshNumChanged);

        //网格尺寸
        //connect(_meshSizeEdit, &QLineEdit::editingFinished, this, &mLocalSizePanel::meshSizeChanged);

    }

    void mLocalSizePanel::meshBiasChanged()
    {
        //偏移功能暂时屏蔽，_offsetDistanceEdit1 未创建，偏置因子固定为 0
        _factor = 0;//_offsetDistanceEdit1->text().toDouble();
        if (_meshControlBox->currentIndex() == 1)
            meshSizeChanged();
        else
            meshNumChanged();
        /*auto solidids = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
        if (solidids.empty())
        {
            return;
        }*/
        int oneorall = 1;
        int LineId = 0;
        int _dim = 1;
        int num = 1;
        double factor = 0;//_offsetDistanceEdit1->text().toDouble();（偏移功能暂时屏蔽，偏置因子固定为 0）
        MechModelData::mModelData* modelData = getCVM->getModelData();
        mMeshData* meshdata = modelData->getMeshData();
        MeshEntity* entity = meshdata->getMeshEntity();
        std::shared_ptr<MXMesh>mesh = std::make_shared<MXMesh>();
        //RendTag(1)
        //渲染标签
        getCVM->getPreRenderWidget()->getFontRender()->clearCommonFont();

        QVector<QVector3D> pos1;

        if (num == 0)
        {
            QVector<int> txt;
            QHash<int, int> ids = meshdata->GEdgeIDTodynaNodeNum;
            for (auto it = ids.begin(); it != ids.end(); it++)
            {
                /*set<int>toggleID = meshdata->getToggleID();*/
                /*if (mMeshMain::getInstance()->_SuturelineId.find(it.key()) != mMeshMain::getInstance()->_SuturelineId.end())
                {
                    continue;
                }*/
                /* if (toggleID.find(it.key()) != toggleID.end())
                 {
                     continue;
                 }*/
                int NUM;
                pos1.push_back(meshdata->IDToCenterOfGEdge[it.key()]);
                txt.push_back(it.value());
            }
            //getCVM->getPreRenderWidget()->getFontRender()->appendCommonFont("adjustNum", pos1, txt);
            //meshdata->appendCommonFontInt("adjustNum", pos1, txt);
        }
        else if (num == 1)
        {
            QVector<double> txt;
            QHash<int, double> offset = meshdata->GEdgeIDToBiasValue;
            for (auto it = offset.begin(); it != offset.end(); it++)
            {
                //set<int>toggleID = MeshMessage::getInstance()->getToggleID();
                /*if (mMeshMain::getInstance()->_SuturelineId.find(it.key()) != mMeshMain::getInstance()->_SuturelineId.end())
                {
                    continue;
                }*/
                /*  if (toggleID.find(it.key()) != toggleID.end())
                  {
                      continue;
                  }*/
                int num;
                pos1.push_back(meshdata->IDToCenterOfGEdge[it.key()]);
                txt.push_back(factor);
            }

            getCVM->getPreRenderWidget()->getFontRender()->appendCommonFont("biastNum", pos1, txt);
        }
        //slot_biasFromFactor

        int temp = 0;
        if (_dim == 1)
        {
            applyEdgeBias(entity, meshdata);
        }

        //emit finishedAdjust(_dim, 1, temp);
         //slot_AdJustAndBiasNum(1,num,0)
        int dim = 1;
        int LineID = 0;
        int status = 0;
        set<int> pickline;
        if (dim == 1)
        {
            if (LineID == 0)
            {
                QHash<int, int> ids = meshdata->GEdgeIDTodynaNodeNum;
                for (auto it = ids.begin(); it != ids.end(); it++)
                {
                    pickline.insert(it.key());
                }
            }
            else
            {
                pickline.insert(LineID);
            }
            if (num == 0)
            {
                status = 0;

            }
            else if (num == 1)
            {
                status = 1;
            }
            //slot_finishedMesh(1);
            // 
            // this->RenderNode();
            set<MXMeshVertex*> Nodes;
            for (auto it1 : pickline)
            {
                set<MXMeshVertex*> Node;
                set<MXMeshVertex*>mvs;
                for (auto it : entity->getGeoEdges()) {
                    if (it->getGeoTag() == it1) {
                        mvs.insert(it->getBeginVertex()->meshGeo_vertexs[0]);
                        if (it->tempEdgeVertexs.size()) {
                            for (auto itv : it->tempEdgeVertexs) {
                                mvs.insert(itv);
                            }
                        }
                        else
                        {
                            if (true) {//原为 !MXMeshParameters::DYNAMIC，偏置提交后 tempEdgeVertexs 已清空，须无条件读取 meshGeo_vertexs
                                for (auto itv : it->meshGeo_vertexs) {
                                    mvs.insert(itv);
                                }
                            }
                        }
                        mvs.insert(it->getEndVertex()->meshGeo_vertexs[0]);

                    }
                    else if (it->getCombineEdge()) {
                        if (it->getCombineEdge()->getGeoTag() == it1) {
                            MXGeoEdge* combineGEdge = it->getCombineEdge();
                            for (auto itl : combineGEdge->_mLines) {
                                mvs.insert(itl->getVertex(0));
                                mvs.insert(itl->getVertex(1));
                            }

                        }
                    }
                }
                Node = mvs;
                for (auto id : Node)
                {
                    Nodes.insert(id);
                }
            }
            /*set<int>ids;
            for (auto it : Nodes)
            {
                ids.insert(it->getNum());
            }
            /*mMeshMain::getInstance()->setAllPickNodeData(MeshMessage::getInstance()->getRPRDByID(ids));*/

            PreMeshTempHighlightData highlight;
            highlight.nodeids = Nodes;
            highlight.stylePattern = NodeStylePattern::TRIANGLE;
            //mMeshMain::getInstance()->setAllPickNodeData_Temp(Nodes);
            getCVM->getPreRenderWidget()->getHighlightController()->updateTempMeshHighlightData(highlight);


            MXGeoEdge* dynamaicEdge = nullptr;
            for (auto it : meshdata->getALLGeoEdge()) {
                if (LineID == it->getGeoTag()) {
                    dynamaicEdge = it;
                }
                else if (it->getCombineEdge()) {
                    if (LineID == it->getCombineEdge()->getGeoTag()) {
                        dynamaicEdge = it;
                    }
                }
            }
            if (dynamaicEdge != nullptr)
            {
                for (auto& itk : dynamaicEdge->meshGeo_vertexs)
                {
                    delete itk;
                    itk = nullptr;
                }

                for (auto& itk : dynamaicEdge->_mLines)
                {
                    delete itk;
                    itk = nullptr;
                }
                dynamaicEdge->_mLines.clear();
                dynamaicEdge->meshGeo_vertexs = dynamaicEdge->tempEdgeVertexs;
                dynamaicEdge->tempEdgeVertexs.clear();
                for (int i = 0; i <= dynamaicEdge->meshGeo_vertexs.size(); i++) {
                    if (i == 0) {
                        MXMeshVertex* v1 = dynamaicEdge->getGeoPoint(0)->_mVertex;
                        MXMeshVertex* v2 = dynamaicEdge->meshGeo_vertexs[i];
                        MXMeshLine* line = new MXMeshLine(v1, v2);
                        line->setEntityTag(dynamaicEdge->getGeoTag());
                        dynamaicEdge->_mLines.push_back(line);

                    }
                    else if (i == dynamaicEdge->meshGeo_vertexs.size()) {
                        MXMeshVertex* v1 = dynamaicEdge->meshGeo_vertexs[i - 1];
                        MXMeshVertex* v2 = dynamaicEdge->getGeoPoint(1)->_mVertex;
                        MXMeshLine* line = new MXMeshLine(v1, v2);
                        line->setEntityTag(dynamaicEdge->getGeoTag());
                        dynamaicEdge->_mLines.push_back(line);
                    }
                    else
                    {
                        MXMeshVertex* v1 = dynamaicEdge->meshGeo_vertexs[i - 1];
                        MXMeshVertex* v2 = dynamaicEdge->meshGeo_vertexs[i];
                        MXMeshLine* line = new MXMeshLine(v1, v2);
                        line->setEntityTag(dynamaicEdge->getGeoTag());
                        dynamaicEdge->_mLines.push_back(line);
                    }
                }
            }
        }


    }

    void mLocalSizePanel::meshNumChanged()
    {
        MechModelData::mModelData* modelData = getCVM->getModelData();
        mMeshData* meshdata = modelData->getMeshData();

        _meshPanelData->setMeshSize(_meshSizeEdit->text().toDouble());
        double size = _meshPanelData->getMeshSize();


        meshdata->deleteAllMeshPart();
        getCVM->getPreRenderWidget()->getFontRender()->clearCommonFont();
        getCVM->getPreRenderWidget()->getHighlightController()->clearTempMeshHighlight();
        CCSMRendWidget* rend = getCVM->getPreRenderWidget();

        std::set<int> a = getGEOServer.geoCalculate().getAllGeoID(1);

        auto solidids = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
        int dim = 1;
        if (solidids.empty())
        {
            return;
        }
        // 获取对应几何维度的所有几何ID并进行一键剖分
        set<int> meshGeoId;
        for (auto mesh_geo_id : solidids)
        {
            meshGeoId.insert(mesh_geo_id);
        }

        MeshEntity* entity = meshdata->getMeshEntity();
        auto edges = entity->getGeoEdges();
        std::shared_ptr<MXMesh>mesh = std::make_shared<MXMesh>();

        std::vector<std::set<int>> vec;
        vec.reserve(meshGeoId.size());
        for (int v : meshGeoId) vec.emplace_back(std::set<int>{v});


        bool isMesh = true;
        if (isMesh)
        {

            // 执行一维剖分
            if (meshdata)
            {
                if (meshdata->getMaxNodeId() > MXMeshParameters::getInstance()->_nodeNum)
                {
                    MXMeshParameters::getInstance()->_nodeNum = meshdata->getMaxNodeId();
                }
                MXMeshParameters::getInstance()->_elementNum = meshdata->getMaxEleId();
            }
            else
            {
                MXMeshParameters::getInstance()->_nodeNum = 0;
                MXMeshParameters::getInstance()->_elementNum = 0;
            }
            entity->setMeshPattern(MeshEntity::MeshPattern::STANDRAD_MESHING);
            mesh->setMeshData(meshdata);
            int num = _meshNumEdit->text().toInt();
            for (int i = 0; i < meshGeoId.size(); ++i)
            {
                int id = *vec[i].begin();
                vector<int> res; res.push_back(id);
                auto geoManager = getCVM->getModelData()->getGeoManager();
                auto lines = getGEOServer.geoCalculate().CalculateEdgeAttr(res);
                double length = lines[0].m_length;
                double size = length / num;
                mesh->MXMeshInterface(size, size, false, false, false, 0.5, 0.3, 0.2, 0, dim, entity, vec[i], {});           // 真正剖分
            }
            //setMeshEntity
            meshdata->GEdgeIDToBiasValue.clear();
            if (dim == 1)
            {
                for (auto it : mesh->_meshEntity->getGeoEdges())
                {
                    auto its = meshGeoId.find(it->getGeoTag());
                    if (its != meshGeoId.end())
                    {
                        //偏置值按几何线取自表格，无记录时为0（偏移面板已屏蔽）
                        auto biasIt = _ids2BiasCache.find(it->getGeoTag());
                        double bias = (biasIt != _ids2BiasCache.end()) ? biasIt->second : 0.0;
                        meshdata->GEdgeIDToBiasValue.insert(it->getGeoTag(), bias);
                    }
                }
            }
            // 拿到几何边上的一维剖分数据（所有节点存入 Node_vec）
            auto mxedges = mesh->_meshEntity->getGeoEdges();

            //偏置值按几何线取自表格（偏移面板已屏蔽）
            applyEdgeBias(entity, meshdata);
            // 拿到不同几何边上的网格节点坐标
            set<MXMeshVertex*> Node_vec;
            // 遍历每一条边
            for (auto tag : meshGeoId)
            {
                //渲染节点
                set<MXMeshVertex*>mvs;
                for (auto it : entity->getGeoEdges()) {
                    if (it->getGeoTag() == tag) {
                        mvs.insert(it->getBeginVertex()->meshGeo_vertexs[0]);
                        if (it->tempEdgeVertexs.size()) {
                            for (auto itv : it->tempEdgeVertexs) {
                                mvs.insert(itv);
                            }
                        }
                        else
                        {
                            if (true) {//原为 !MXMeshParameters::DYNAMIC，偏置提交后 tempEdgeVertexs 已清空，须无条件读取 meshGeo_vertexs
                                for (auto itv : it->meshGeo_vertexs) {
                                    mvs.insert(itv);
                                }
                            }
                        }
                        mvs.insert(it->getEndVertex()->meshGeo_vertexs[0]);

                    }
                    else if (it->getCombineEdge()) {
                        if (it->getCombineEdge()->getGeoTag() == tag) {
                            MXGeoEdge* combineGEdge = it->getCombineEdge();
                            for (auto itl : combineGEdge->_mLines) {
                                mvs.insert(itl->getVertex(0));
                                mvs.insert(itl->getVertex(1));
                            }

                        }
                    }
                }
                for (auto vertex : mvs)
                {
                    Node_vec.insert(vertex);
                }
            }

            // 存储GEdgeIDTodynaNodeNum的值
            for (const auto& edge : mxedges) {
                if (edge->getCombineEdge()) {
                }
                else {
                    meshdata->GEdgeIDTodynaNodeNum.insert(edge->getGeoTag(),
                        edge->meshGeo_vertexs.size() + 2);
                }
            }
            //getNumOfVertexInEdge 获取几何边id -> 节点个数
            QHash<int, int> edge2idNum = meshdata->GEdgeIDTodynaNodeNum;
            //计算IDToCenterOfGEdge
            getIDToCenterOfGEdge(mesh, meshGeoId);

            //显示网格节点个数 
            QVector<QVector3D>pos_vec;
            QVector<double>num_string;
            for (auto it = edge2idNum.begin(); it != edge2idNum.end(); it++)
            {
                int NUM;
                pos_vec.push_back(meshdata->IDToCenterOfGEdge[it.key()]);
                num_string.push_back(_meshNumEdit->text().toDouble());
            }
            getCVM->getPreRenderWidget()->getFontRender()->appendCommonFont("adjustNum", pos_vec, num_string);

            PreMeshTempHighlightData highlight;
            highlight.nodeids = Node_vec;
            highlight.stylePattern = NodeStylePattern::TRIANGLE;
            getCVM->getPreRenderWidget()->getHighlightController()->updateTempMeshHighlightData(highlight);
            getCVM->getPreRenderWidget()->updatePaint();
        }
        return;




    }

    void mLocalSizePanel::meshSizeChanged()
    {

        _meshPanelData->setMeshSize(_meshSizeEdit->text().toDouble());
        double size = _meshPanelData->getMeshSize();

        MechModelData::mModelData* modelData = getCVM->getModelData();
        mMeshData* meshdata = modelData->getMeshData();

        meshdata->deleteAllMeshPart();
        getCVM->getPreRenderWidget()->getFontRender()->clearCommonFont();
        getCVM->getPreRenderWidget()->getHighlightController()->clearTempMeshHighlight();
        CCSMRendWidget* rend = getCVM->getPreRenderWidget();

        auto geoDatabase = getCVM->getModelData()->getGeoData();
        vector<int> solidids = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
        int dim = 2;
        if (solidids.empty())
        {
            solidids = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
            dim = 1;
        }
        if (solidids.empty())
        {
            return;
        }
        // 获取对应几何维度的所有几何ID并进行一键剖分

        set<int> meshGeoId;
        for (auto mesh_geo_id : solidids)
        {
            meshGeoId.insert(mesh_geo_id);
        }
        MeshEntity* entity = meshdata->getMeshEntity();

        if (meshdata)
        {
            if (meshdata->getMaxNodeId() > MXMeshParameters::getInstance()->_nodeNum)
            {
                MXMeshParameters::getInstance()->_nodeNum = meshdata->getMaxNodeId();
            }
            MXMeshParameters::getInstance()->_elementNum = meshdata->getMaxEleId();
        }
        else
        {
            MXMeshParameters::getInstance()->_nodeNum = 0;
            MXMeshParameters::getInstance()->_elementNum = 0;
        }
        std::shared_ptr<MXMesh>mesh = std::make_shared<MXMesh>();
        auto getAndSplitMeshIDsByDim = [&](int dim, bool isMesh)->std::set<int>
            {
                if (isMesh)
                {
                    entity->setMeshPattern(MeshEntity::MeshPattern::STANDRAD_MESHING);
                    mesh->setMeshData(meshdata);
                    mesh->MXMeshInterface(size, size, false, false, false, 0.5, 0.3, 0.2, 0, dim, entity, meshGeoId, {});           // 真正剖分
                    //meshdata->ensureMesh();//网格剖分后里存底层数据的接口
                }
                return meshGeoId;
            };

        /* 2. 存储待剖分id */
        getAndSplitMeshIDsByDim(dim, true);

        QVector<double> txt;
        QVector<QVector3D> pos1;
        mGlobalSignals::getInstance()->outputMessageSig(0, tr("剖分结束"), true);


        //setMeshEntity
        meshdata->GEdgeIDToBiasValue.clear();
        if (dim == 1)
        {
            for (auto it : mesh->_meshEntity->getGeoEdges())
            {
                auto its = meshGeoId.find(it->getGeoTag());
                if (its != meshGeoId.end())
                {
                    //偏置值按几何线取自表格，无记录时为0（偏移面板已屏蔽）
                    auto biasIt = _ids2BiasCache.find(it->getGeoTag());
                    double bias = (biasIt != _ids2BiasCache.end()) ? biasIt->second : 0.0;
                    meshdata->GEdgeIDToBiasValue.insert(it->getGeoTag(), bias);
                }
            }
        }
        //MXMesh1D
        //mesh->MXMesh1D();
        meshdata->GEdgeIDTodynaNodeNum.clear();
        meshdata->IDToCenterOfGEdge.clear();
        // 拿到几何边上的一维剖分数据（所有节点存入 Node_vec）
        auto mxedges = mesh->_meshEntity->getGeoEdges();
        //偏置值按几何线取自表格（偏移面板已屏蔽）
        applyEdgeBias(entity, meshdata);
        set<MXMeshVertex*> Node_vec;
        for (auto tag : meshGeoId)
        {
            //set<int>toggleID = meshdata->getToggleID();
            /*if (mMeshMain::getInstance()->_SuturelineId.find(it) != mMeshMain::getInstance()->_SuturelineId.end())
            {
                continue;
            }*/
            /* if (toggleID.find(it) != toggleID.end())
             {
                 continue;
             }*/
             //getVertesxInEdge
            set<MXMeshVertex*>mvs;
            for (auto it : entity->getGeoEdges()) {
                if (it->getGeoTag() == tag) {
                    /*if (it->getBeginVertex()->meshGeo_vertexs.empty())
                        continue;*/
                    mvs.insert(it->getBeginVertex()->meshGeo_vertexs[0]);
                    if (it->tempEdgeVertexs.size()) {
                        for (auto itv : it->tempEdgeVertexs) {
                            mvs.insert(itv);
                        }
                    }
                    else
                    {
                        if (true) {//原为 !MXMeshParameters::DYNAMIC，偏置提交后 tempEdgeVertexs 已清空，须无条件读取 meshGeo_vertexs
                            for (auto itv : it->meshGeo_vertexs) {
                                mvs.insert(itv);
                            }
                        }
                    }
                    /*if (it->getBeginVertex()->meshGeo_vertexs.empty())
                       continue;*/
                    mvs.insert(it->getEndVertex()->meshGeo_vertexs[0]);

                }
                else if (it->getCombineEdge()) {
                    if (it->getCombineEdge()->getGeoTag() == tag) {
                        MXGeoEdge* combineGEdge = it->getCombineEdge();
                        for (auto itl : combineGEdge->_mLines) {
                            mvs.insert(itl->getVertex(0));
                            mvs.insert(itl->getVertex(1));
                        }

                    }
                }
            }

            //渲染节点
            for (auto vertex : mvs)
            {
                Node_vec.insert(vertex);
            }
        }
        //存储GEdgeIDTodynaNodeNum的值
        for (const auto& edge : mxedges)
        {
            if (edge->getCombineEdge())
            {
            }
            else
            {
                meshdata->GEdgeIDTodynaNodeNum.insert(edge->getGeoTag(),
                    edge->meshGeo_vertexs.size() + 2);
            }

        }
        //getNumOfVertexInEdge
        QHash<int, int> edge2idNum = meshdata->GEdgeIDTodynaNodeNum;
        //计算IDToCenterOfEdge
        getIDToCenterOfGEdge(mesh, meshGeoId);
        //显示网格节点个数
        QVector<QVector3D> pos_vec;
        QVector<double> num_string;
        for (auto it = edge2idNum.begin(); it != edge2idNum.end(); it++)
        {
            int Num;
            pos_vec.push_back(meshdata->IDToCenterOfGEdge[it.key()]);
            num_string.push_back(it.value());
        }
        getCVM->getPreRenderWidget()->getFontRender()->appendCommonFont("adjustNum", pos_vec, num_string);
        //MXMeshParameters::getInstance()->DYNAMIC = false;
        PreMeshTempHighlightData highlight;
        highlight.nodeids = Node_vec;
        highlight.stylePattern = NodeStylePattern::TRIANGLE;
        //mMeshMain::getInstance()->setAllPickNodeData_Temp(Nodes);
        getCVM->getPreRenderWidget()->getHighlightController()->updateTempMeshHighlightData(highlight);
        getCVM->getPreRenderWidget()->updatePaint();
        return;
    }

    void mLocalSizePanel::applyEdgeBias(MeshEntity* entity, mMeshData* meshdata)
    {
        //偏置值在剖分前已按几何线写入 GEdgeIDToBiasValue，这里逐条应用
        for (auto it = meshdata->GEdgeIDToBiasValue.begin(); it != meshdata->GEdgeIDToBiasValue.end(); it++)
        {
            if (meshdata->GEdgeIDTodynaNodeNum[it.key()] == 2)
            {
                continue;
            }
            double biasValue = it.value();
            if (biasValue == 0.0)
            {
                continue;
            }
            //biasEdgeNode 当前为无效实现，直接对普通边调用底层偏置分布
            MXGeoEdge* gedge = entity->getGeoEdgeById(it.key());
            if (!gedge || gedge->getCombineEdge())
            {
                continue;
            }
            meshEdge mEdge;
            mEdge.meshBiasEdge(gedge, biasValue);
            if (gedge->tempEdgeVertexs.empty())
            {
                continue;
            }
            //偏置节点替换种子点：tempEdgeVertexs 提交为正式边离散并重建线段，
            //后续面/体剖分读取 meshGeo_vertexs/_mLines 时即可继承偏置分布
            for (auto& itk : gedge->meshGeo_vertexs)
            {
                delete itk;
                itk = nullptr;
            }
            for (auto& itk : gedge->_mLines)
            {
                delete itk;
                itk = nullptr;
            }
            gedge->_mLines.clear();
            gedge->meshGeo_vertexs = gedge->tempEdgeVertexs;
            gedge->tempEdgeVertexs.clear();
            gedge->setSplitNodeNum(gedge->meshGeo_vertexs.size() + 2);
            //标记为已剖分：面剖分入口的一维阶段按状态跳过该边（DiscreteSurface.cpp:4721），
            //否则 generateMXline 会按均匀分布重算，覆盖偏置节点
            gedge->setMeshStatic(MeshDone);
            for (int i = 0; i <= gedge->meshGeo_vertexs.size(); i++)
            {
                MXMeshVertex* v1 = nullptr;
                MXMeshVertex* v2 = nullptr;
                if (i == 0)
                {
                    v1 = gedge->getGeoPoint(0)->_mVertex;
                    v2 = gedge->meshGeo_vertexs[i];
                }
                else if (i == gedge->meshGeo_vertexs.size())
                {
                    v1 = gedge->meshGeo_vertexs[i - 1];
                    v2 = gedge->getGeoPoint(1)->_mVertex;
                }
                else
                {
                    v1 = gedge->meshGeo_vertexs[i - 1];
                    v2 = gedge->meshGeo_vertexs[i];
                }
                MXMeshLine* line = new MXMeshLine(v1, v2);
                line->setEntityTag(gedge->getGeoTag());
                gedge->_mLines.push_back(line);
            }
        }
    }

    void mLocalSizePanel::getIDToCenterOfGEdge(std::shared_ptr<MXMesh> mesh, set<int> meshGeoId)
    {
        MechModelData::mModelData* modelData = getCVM->getModelData();
        mMeshData* meshdata = modelData->getMeshData();
        MeshEntity* entity = meshdata->getMeshEntity();
        QVector<double> txt;
        QVector<QVector3D> pos1;
        mGlobalSignals::getInstance()->outputMessageSig(0, tr("剖分结束"), true);
        //setMeshEntity
        meshdata->GEdgeIDToBiasValue.clear();
        int dim = 1;
        if (dim == 1)
        {
            for (auto it : mesh->_meshEntity->getGeoEdges())
            {
                auto its = meshGeoId.find(it->getGeoTag());
                if (its != meshGeoId.end())
                {
                    //偏置值按几何线取自表格，无记录时为0（偏移面板已屏蔽）
                    auto biasIt = _ids2BiasCache.find(it->getGeoTag());
                    double bias = (biasIt != _ids2BiasCache.end()) ? biasIt->second : 0.0;
                    meshdata->GEdgeIDToBiasValue.insert(it->getGeoTag(), bias);
                }
            }
        }
        //MXMesh1D
        meshdata->IDToCenterOfGEdge.clear();
        auto mxedges = mesh->_meshEntity->getGeoEdges();


        //getNumOfVertexInEdge
        QHash<int, int> ids = meshdata->GEdgeIDTodynaNodeNum;
        for (auto it = ids.begin(); it != ids.end(); it++)
        {
            int NUM;
            QVector3D pos;
            for (auto it1 : entity->getGeoEdges())
            {
                int edgeTag = it.key();
                if (it1->getGeoTag() == edgeTag) {
                    //不是虚几何边
                    if (it1->tempEdgeVertexs.size()) {
                        NUM = it1->tempEdgeVertexs.size() + 2;
                        if (it1->tempEdgeVertexs.size() % 2 == 1) {
                            MXMeshVertex* v0 = it1->tempEdgeVertexs[(it1->tempEdgeVertexs.size() / 2)];
                            pos.setX(v0->vx());
                            pos.setY(v0->vy());
                            pos.setZ(v0->vz());
                        }
                        else
                        {
                            MXMeshVertex* v0 = it1->tempEdgeVertexs[it1->tempEdgeVertexs.size() / 2 - 1];
                            MXMeshVertex* v1 = it1->tempEdgeVertexs[it1->tempEdgeVertexs.size() / 2];
                            pos.setX((v0->vx() + v1->vx()) / 2);
                            pos.setY((v0->vy() + v1->vy()) / 2);
                            pos.setZ((v0->vz() + v1->vz()) / 2);

                        }
                        meshdata->IDToCenterOfGEdge.insert(edgeTag, pos);
                        break;
                    }
                    else
                    {
                        /*if (it->meshGeo_vertexs.size()) {
                            NUM = it->meshGeo_vertexs.size() + 2;
                            if (it->meshGeo_vertexs.size() % 2 == 1) {
                                MXMeshVertex*v0 = it->meshGeo_vertexs[it->meshGeo_vertexs.size() / 2];
                                Center.setX(v0->vx());
                                Center.setY(v0->vy());
                                Center.setZ(v0->vz());
                            }
                            else
                            {
                                MXMeshVertex*v0 = it->meshGeo_vertexs[it->meshGeo_vertexs.size() / 2 - 1];
                                MXMeshVertex*v1 = it->meshGeo_vertexs[it->meshGeo_vertexs.size() / 2];
                                Center.setX((v0->vx() + v1->vx()) / 2);
                                Center.setY((v0->vy() + v1->vy()) / 2);
                                Center.setZ((v0->vz() + v1->vz()) / 2);

                            }
                        }
                        else
                        {*/
                        if (true) {//原为 !MXMeshParameters::DYNAMIC，偏置提交后 tempEdgeVertexs 已清空，须无条件读取 meshGeo_vertexs
                            NUM = it1->meshGeo_vertexs.size() + 2;
                        }
                        else {
                            NUM = 2;
                        }

                        Range<double> par = it1->parBounds(0);
                        double t = (par.low() + par.high()) / 2;
                        //【OCC摘除·S10】旧 getPoint(gp_Pnt) 接口切中性 E-01 curvePoint
                        GeoNeutralData::GN_Point V = it1->curvePoint(t);
                        pos.setX(V.x());
                        pos.setY(V.y());
                        pos.setZ(V.z());
                        //}
                        meshdata->IDToCenterOfGEdge.insert(edgeTag, pos);
                        break;
                    }

                }
                else if (it1->getCombineEdge()) {
                    if (it1->getCombineEdge()->getGeoTag() == edgeTag) {
                        MXGeoEdge* combineGEdge = it1->getCombineEdge();
                        if (combineGEdge->getBeginVertex() == combineGEdge->getEndVertex())
                        {
                            NUM = combineGEdge->meshGeo_vertexs.size() + 1;
                        }
                        else
                        {
                            NUM = combineGEdge->meshGeo_vertexs.size() + 2;
                        }


                        if (NUM == 2) {
                            //获取所有的子几何边
                            double x = 0, y = 0, z = 0;
                            for (auto ite : it1->getCombineEdge()->subGEdges) {
                                Range<double> par = it1->parBounds(0);
                                double t = (par.low() + par.high()) / 2;
                                //【OCC摘除·S10】旧 getPoint(gp_Pnt) 接口切中性 E-01 curvePoint
                                GeoNeutralData::GN_Point V = it1->curvePoint(t);
                                x = x + V.x();
                                y = y + V.y();
                                z = z + V.z();
                            }

                            x = x / it1->getCombineEdge()->subGEdges.size();
                            y = y / it1->getCombineEdge()->subGEdges.size();
                            z = z / it1->getCombineEdge()->subGEdges.size();
                            pos.setX(x);
                            pos.setY(y);
                            pos.setZ(z);
                        }
                        else
                        {
                            if (combineGEdge->meshGeo_vertexs.size() % 2 == 1) {
                                MXMeshVertex* v0 = combineGEdge->meshGeo_vertexs[(combineGEdge->meshGeo_vertexs.size() / 2)];
                                pos.setX(v0->vx());
                                pos.setY(v0->vy());
                                pos.setZ(v0->vz());
                            }
                            else
                            {
                                MXMeshVertex* v0 = combineGEdge->meshGeo_vertexs[combineGEdge->meshGeo_vertexs.size() / 2 - 1];
                                MXMeshVertex* v1 = combineGEdge->meshGeo_vertexs[combineGEdge->meshGeo_vertexs.size() / 2];
                                pos.setX((v0->vx() + v1->vx()) / 2);
                                pos.setY((v0->vy() + v1->vy()) / 2);
                                pos.setZ((v0->vz() + v1->vz()) / 2);

                            }
                        }
                        meshdata->IDToCenterOfGEdge.insert(edgeTag, pos);
                        break;
                    }
                }

            }
            //break;
        }
    }

    mLocalSizePanel::~mLocalSizePanel()
    {
    }

    void mLocalSizePanel::newMeshPanel()
    {
    }
    void mLocalSizePanel::editMeshPanel(int meshId)
    {
        _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(meshId);
        auto a = _meshPanelData->getMeshPanelType();
        updatePanel();
    }
    void mLocalSizePanel::getMeshInfoById(const std::vector<int> ids)
    {

    }
    void mLocalSizePanel::updatePanel()
    {
        //网格尺寸
        double size = _meshPanelData->getMeshSize();
        _meshSizeEdit->setText(MxFunctions::mNumberToString(size));
        int num = _meshPanelData->getMeshNum();
        _meshNumEdit->setText(MxFunctions::mNumberToString(num));
        //实体集合
        //_listAssignPart->clear();
        //for (const QString& s : _meshPanelData->_list)
        //    _listAssignPart->addItem(s);          // 按 cache 顺序追加
        numTable->setRowCount(0);  // 设置行数为0，删除所有数据行
        for (auto it : _meshPanelData->_ids2NumsMap)                    // set 自动升序
        {
            QString txt = QString("几何线_%1").arg(it.first);

            int rowCount = numTable->rowCount();
            numTable->insertRow(rowCount);
            // 为新行的单元格设置数据
            numTable->setItem(rowCount, 0, new QTableWidgetItem(txt));
            auto item = numTable->item(rowCount, 0); item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            numTable->setItem(rowCount, 1, new QTableWidgetItem(MxFunctions::mNumberToString(it.second)));
            //偏置列回显，无记录时为0
            auto biasIt = _ids2BiasCache.find(it.first);
            double bias = (biasIt != _ids2BiasCache.end()) ? biasIt->second : 0.0;
            numTable->setItem(rowCount, 2, new QTableWidgetItem(MxFunctions::mNumberToString(bias)));

        }
        sizeTable->setRowCount(0);  // 设置行数为0，删除所有数据行
        for (auto it : _meshPanelData->_ids2SizesMap)                    // set 自动升序
        {
            QString txt = QString("几何线_%1").arg(it.first);

            int rowCount = sizeTable->rowCount();
            sizeTable->insertRow(rowCount);
            // 为新行的单元格设置数据
            sizeTable->setItem(rowCount, 0, new QTableWidgetItem(txt));
            auto item = sizeTable->item(rowCount, 0); item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            sizeTable->setItem(rowCount, 1, new QTableWidgetItem(MxFunctions::mNumberToString(it.second)));
            //偏置列回显，无记录时为0
            auto biasIt = _ids2BiasCache.find(it.first);
            double bias = (biasIt != _ids2BiasCache.end()) ? biasIt->second : 0.0;
            sizeTable->setItem(rowCount, 2, new QTableWidgetItem(MxFunctions::mNumberToString(bias)));

        }

        _meshControlBox->setCurrentText(_meshPanelData->getCtrlMethod());
        //偏移功能暂时屏蔽，控件未创建，跳过调整方式回显
        //_orderComboBox->setCurrentText(_meshPanelData->getAdjustMethod());


    }
    void mLocalSizePanel::setLocalNum()
    {
        MechModelData::mModelData* modelData = getCVM->getModelData();
        mMeshData* meshdata = modelData->getMeshData();

        _meshPanelData->setMeshSize(_meshSizeEdit->text().toDouble());

        //meshdata->deleteAllMeshPart();
        getCVM->getPreRenderWidget()->getFontRender()->clearCommonFont();
        getCVM->getPreRenderWidget()->getHighlightController()->clearTempMeshHighlight();
        CCSMRendWidget* rend = getCVM->getPreRenderWidget();



        vector<int> solidids;//表格中的实体
        map<int, int> ids2NumsMap;//实体的尺寸
        int dim = 1;
        QStringList list;
        for (int row = 0; row < numTable->rowCount(); ++row) {
            if (auto item = numTable->item(row, 0)) {  // C++11 auto + if初始化
                list = item->text().split("_");
                solidids.push_back(list[1].toInt());

                ids2NumsMap[list[1].toInt()] = numTable->item(row, 1)->text().toInt();
            }
        }
        if (solidids.empty())
        {
            return;
        }

        set<int> meshGeoId;
        MeshEntity* entity = meshdata->getMeshEntity();
        entity->buildGeoSolidIdMap();
        // 获取对应几何维度的所有几何ID并进行一键剖分
        for (auto mesh_geo_id : solidids)
        {
            auto split_node_num = ids2NumsMap[mesh_geo_id];
            if (split_node_num >= 1)
            {
                MXGeoEdge* gedge = entity->getGeoEdgeById(mesh_geo_id);
                gedge->setSplitNodeNum(split_node_num + 1);//节点数=网格段数+1
            }
            meshGeoId.insert(mesh_geo_id);
        }

        auto edges = entity->getGeoEdges();
        std::shared_ptr<MXMesh>mesh = std::make_shared<MXMesh>();

        std::vector<std::set<int>> vec;
        vec.reserve(meshGeoId.size());
        for (int v : meshGeoId) vec.emplace_back(std::set<int>{v});


        bool isMesh = true;
        if (isMesh)
        {

            // 执行一维剖分
            entity->setMeshPattern(MeshEntity::MeshPattern::STANDRAD_MESHING);
            mesh->setMeshData(meshdata);
            if (meshdata)
            {
                if (meshdata->getMaxNodeId() > MXMeshParameters::getInstance()->_nodeNum)
                {
                    MXMeshParameters::getInstance()->_nodeNum = meshdata->getMaxNodeId();
                }
                MXMeshParameters::getInstance()->_elementNum = meshdata->getMaxEleId();
            }
            else
            {
                MXMeshParameters::getInstance()->_nodeNum = 0;
                MXMeshParameters::getInstance()->_elementNum = 0;
            }
            for (int i = 0; i < meshGeoId.size(); ++i)
            {
                int id = *vec[i].begin();
                vector<int> res; res.push_back(id);
                auto geoManager = getCVM->getModelData()->getGeoManager();
                auto lines = getGEOServer.geoCalculate().CalculateEdgeAttr(res);
                double length = lines[0].m_length;
                double size = length / ids2NumsMap[id];
                mesh->MXMeshInterface(size, size, false, false, false, 0.5, 0.3, 0.2, 0, dim, entity, vec[i], {});           // 真正剖分
            }
            //setMeshEntity
            meshdata->GEdgeIDToBiasValue.clear();
            if (dim == 1)
            {
                for (auto it : mesh->_meshEntity->getGeoEdges())
                {
                    auto its = meshGeoId.find(it->getGeoTag());
                    if (its != meshGeoId.end())
                    {
                        //偏置值按几何线取自表格，无记录时为0（偏移面板已屏蔽）
                        auto biasIt = _ids2BiasCache.find(it->getGeoTag());
                        double bias = (biasIt != _ids2BiasCache.end()) ? biasIt->second : 0.0;
                        meshdata->GEdgeIDToBiasValue.insert(it->getGeoTag(), bias);
                    }
                }
            }
            meshdata->GEdgeIDTodynaNodeNum.clear();
            meshdata->IDToCenterOfGEdge.clear();
            // 拿到几何边上的一维剖分数据（所有节点存入 Node_vec）
            auto mxedges = mesh->_meshEntity->getGeoEdges();

            //偏置值按几何线取自表格（偏移面板已屏蔽）
            applyEdgeBias(entity, meshdata);
            // 拿到不同几何边上的网格节点坐标
            set<MXMeshVertex*> Node_vec;
            // 遍历每一条边
            for (auto tag : meshGeoId)
            {
                //渲染节点
                set<MXMeshVertex*>mvs;
                for (auto it : entity->getGeoEdges()) {
                    if (it->getGeoTag() == tag) {
                        mvs.insert(it->getBeginVertex()->meshGeo_vertexs[0]);
                        if (it->tempEdgeVertexs.size()) {
                            for (auto itv : it->tempEdgeVertexs) {
                                mvs.insert(itv);
                            }
                        }
                        else
                        {
                            if (true) {//原为 !MXMeshParameters::DYNAMIC，偏置提交后 tempEdgeVertexs 已清空，须无条件读取 meshGeo_vertexs
                                for (auto itv : it->meshGeo_vertexs) {
                                    mvs.insert(itv);
                                }
                            }
                        }
                        mvs.insert(it->getEndVertex()->meshGeo_vertexs[0]);

                    }
                    else if (it->getCombineEdge()) {
                        if (it->getCombineEdge()->getGeoTag() == tag) {
                            MXGeoEdge* combineGEdge = it->getCombineEdge();
                            for (auto itl : combineGEdge->_mLines) {
                                mvs.insert(itl->getVertex(0));
                                mvs.insert(itl->getVertex(1));
                            }

                        }
                    }
                }
                for (auto vertex : mvs)
                {
                    Node_vec.insert(vertex);
                }
            }

            // 存储GEdgeIDTodynaNodeNum的值
            for (const auto& edge : mxedges) {
                if (edge->getCombineEdge()) {
                }
                else {
                    meshdata->GEdgeIDTodynaNodeNum.insert(edge->getGeoTag(),
                        edge->meshGeo_vertexs.size() + 2);
                }
            }
            //getNumOfVertexInEdge 获取几何边id -> 节点个数
            QHash<int, int> edge2idNum = meshdata->GEdgeIDTodynaNodeNum;
            //计算IDToCenterOfGEdge
            getIDToCenterOfGEdge(mesh, meshGeoId);

            //显示网格节点个数 
            QVector<QVector3D>pos_vec;
            QVector<double>num_string;
            for (auto it = edge2idNum.begin(); it != edge2idNum.end(); it++)
            {
                auto a = it.value();
                pos_vec.push_back(meshdata->IDToCenterOfGEdge[it.key()]);
                num_string.push_back(it.value() - 1);
            }
            getCVM->getPreRenderWidget()->getFontRender()->appendCommonFont("adjustNum", pos_vec, num_string);

            PreMeshTempHighlightData highlight;
            highlight.nodeids = Node_vec;
            highlight.stylePattern = NodeStylePattern::TRIANGLE;
            getCVM->getPreRenderWidget()->getHighlightController()->updateTempMeshHighlightData(highlight);
            getCVM->getPreRenderWidget()->updatePaint();
        }
    }
    void mLocalSizePanel::slot_localSizePreview()
    {
        //根据尺寸
        if (stackWidget->currentIndex() == 0)
        {
            setLocalSize();
        }
        //根据数量
        else
        {
            setLocalNum();
        }
    }
    void mLocalSizePanel::setLocalSize()
    {
        {
            _meshPanelData->setMeshSize(_meshSizeEdit->text().toDouble());
            double size = _meshPanelData->getMeshSize();

            MechModelData::mModelData* modelData = getCVM->getModelData();
            mMeshData* meshdata = modelData->getMeshData();

            meshdata->deleteAllMeshPart();
            getCVM->getPreRenderWidget()->getFontRender()->clearCommonFont();
            getCVM->getPreRenderWidget()->getHighlightController()->clearTempMeshHighlight();
            CCSMRendWidget* rend = getCVM->getPreRenderWidget();


            vector<int> solidids;//表格中的实体
            map<int, double> ids2SizesMap;//实体的尺寸
            int dim = 1;
            QStringList list;
            for (int row = 0; row < sizeTable->rowCount(); ++row) {
                if (auto item = sizeTable->item(row, 0)) {  // C++11 auto + if初始化
                    list = item->text().split("_");
                    solidids.push_back(list[1].toInt());


                    ids2SizesMap[list[1].toInt()] = (sizeTable->item(row, 1)->text().toDouble());
                }
            }
            if (solidids.empty())
            {
                return;
            }

            // 获取对应几何维度的所有几何ID并进行一键剖分

            set<int> meshGeoId;
            for (auto mesh_geo_id : solidids)
            {
                meshGeoId.insert(mesh_geo_id);
            }
            std::vector<std::set<int>> vec;
            vec.reserve(meshGeoId.size());
            for (int v : meshGeoId)
            {
                vec.emplace_back(std::set<int>{v});
            }

            MeshEntity* entity = meshdata->getMeshEntity();

            for (int edgeId : meshGeoId)
            {
                MXGeoEdge* gedge = entity->getGeoEdgeById(edgeId);
                if (gedge)
                {
                    gedge->setSplitNodeNum(0);
                }
            }

            //剖分前登记网格规模上限（dev 侧原有逻辑，供剖分参数统计）
            if (meshdata)
            {
                if (meshdata->getMaxNodeId() > MXMeshParameters::getInstance()->_nodeNum)
                {
                    MXMeshParameters::getInstance()->_nodeNum = meshdata->getMaxNodeId();
                }
                MXMeshParameters::getInstance()->_elementNum = meshdata->getMaxEleId();
            }
            else
            {
                MXMeshParameters::getInstance()->_nodeNum = 0;
                MXMeshParameters::getInstance()->_elementNum = 0;
            }

std::shared_ptr<MXMesh>mesh = std::make_shared<MXMesh>();
            auto getAndSplitMeshIDsByDim = [&](int dim, bool isMesh)->std::set<int>
                {
                    if (isMesh)
                    {
                        entity->setMeshPattern(MeshEntity::MeshPattern::STANDRAD_MESHING);
                        mesh->setMeshData(meshdata);
                        for (int i = 0; i < meshGeoId.size(); ++i)
                        {
                            int id = *vec[i].begin();
                            vector<int> res; res.push_back(id);
                            double size = ids2SizesMap[id];
                            mesh->MXMeshInterface(size, size, false, false, false, 0.5, 0.3, 0.2, 0, dim, entity, vec[i], {});           // 真正剖分
                        }
                        //meshdata->ensureMesh();//网格剖分后里存底层数据的接口
                    }
                    return meshGeoId;
                };

            /* 2. 存储待剖分id */
            getAndSplitMeshIDsByDim(dim, true);

            QVector<double> txt;
            QVector<QVector3D> pos1;
            mGlobalSignals::getInstance()->outputMessageSig(0, tr("剖分结束"), true);


            //setMeshEntity
            meshdata->GEdgeIDToBiasValue.clear();
            if (dim == 1)
            {
                for (auto it : mesh->_meshEntity->getGeoEdges())
                {
                    auto its = meshGeoId.find(it->getGeoTag());
                    if (its != meshGeoId.end())
                    {
                        //偏置值按几何线取自表格，无记录时为0（偏移面板已屏蔽）
                        auto biasIt = _ids2BiasCache.find(it->getGeoTag());
                        double bias = (biasIt != _ids2BiasCache.end()) ? biasIt->second : 0.0;
                        meshdata->GEdgeIDToBiasValue.insert(it->getGeoTag(), bias);
                    }
                }
            }
            //MXMesh1D
            //mesh->MXMesh1D();
            meshdata->GEdgeIDTodynaNodeNum.clear();
            meshdata->IDToCenterOfGEdge.clear();
            // 拿到几何边上的一维剖分数据（所有节点存入 Node_vec）
            auto mxedges = mesh->_meshEntity->getGeoEdges();
            //偏置值按几何线取自表格（偏移面板已屏蔽）
            applyEdgeBias(entity, meshdata);
            set<MXMeshVertex*> Node_vec;
            for (auto tag : meshGeoId)
            {
                //set<int>toggleID = meshdata->getToggleID();
                /*if (mMeshMain::getInstance()->_SuturelineId.find(it) != mMeshMain::getInstance()->_SuturelineId.end())
                {
                continue;
                }*/
                /* if (toggleID.find(it) != toggleID.end())
                {
                continue;
                }*/
                //getVertesxInEdge
                set<MXMeshVertex*>mvs;
                for (auto it : entity->getGeoEdges()) {
                    if (it->getGeoTag() == tag) {
                        /*if (it->getBeginVertex()->meshGeo_vertexs.empty())
                        continue;*/
                        mvs.insert(it->getBeginVertex()->meshGeo_vertexs[0]);
                        if (it->tempEdgeVertexs.size()) {
                            for (auto itv : it->tempEdgeVertexs) {
                                mvs.insert(itv);
                            }
                        }
                        else
                        {
                            if (true) {//原为 !MXMeshParameters::DYNAMIC，偏置提交后 tempEdgeVertexs 已清空，须无条件读取 meshGeo_vertexs
                                for (auto itv : it->meshGeo_vertexs) {
                                    mvs.insert(itv);
                                }
                            }
                        }
                        /*if (it->getBeginVertex()->meshGeo_vertexs.empty())
                        continue;*/
                        mvs.insert(it->getEndVertex()->meshGeo_vertexs[0]);
                        it->setSplitNodeNum(it->meshGeo_vertexs.size() + 2);
                    }
                    else if (it->getCombineEdge()) {
                        if (it->getCombineEdge()->getGeoTag() == tag) {
                            MXGeoEdge* combineGEdge = it->getCombineEdge();
                            for (auto itl : combineGEdge->_mLines) {
                                mvs.insert(itl->getVertex(0));
                                mvs.insert(itl->getVertex(1));
                            }

                        }
                    }
                }

                //渲染节点
                for (auto vertex : mvs)
                {
                    Node_vec.insert(vertex);
                }
            }
            //存储GEdgeIDTodynaNodeNum的值
            for (const auto& edge : mxedges)
            {
                if (edge->getCombineEdge())
                {
                }
                else
                {
                    meshdata->GEdgeIDTodynaNodeNum.insert(edge->getGeoTag(),
                        edge->meshGeo_vertexs.size() + 2);
                }

            }
            //getNumOfVertexInEdge
            QHash<int, int> edge2idNum = meshdata->GEdgeIDTodynaNodeNum;
            //计算IDToCenterOfEdge
            getIDToCenterOfGEdge(mesh, meshGeoId);
            //显示网格节点个数
            QVector<QVector3D> pos_vec;
            QVector<double> num_string;
            for (auto it = edge2idNum.begin(); it != edge2idNum.end(); it++)
            {
                int Num;
                pos_vec.push_back(meshdata->IDToCenterOfGEdge[it.key()]);
                num_string.push_back(it.value() - 1);
            }
            getCVM->getPreRenderWidget()->getFontRender()->appendCommonFont("adjustNum", pos_vec, num_string);
            //MXMeshParameters::getInstance()->DYNAMIC = false;
            PreMeshTempHighlightData highlight;
            highlight.nodeids = Node_vec;
            highlight.stylePattern = NodeStylePattern::TRIANGLE;
            //mMeshMain::getInstance()->setAllPickNodeData_Temp(Nodes);
            getCVM->getPreRenderWidget()->getHighlightController()->updateTempMeshHighlightData(highlight);
            getCVM->getPreRenderWidget()->updatePaint();
            return;
        }
    }
}
