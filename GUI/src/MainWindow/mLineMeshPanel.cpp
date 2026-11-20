
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
//#include <mGeoDataModelSet.h> //【OCC摘除·S10/P0-1 终局2】死 include（零使用）
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QToolButton>
#include <QHBoxLayout>
#include <QMetaType>      // 元类型注册函数

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
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】死 include（零调用） //【OCC迁移·链A续清配套】命名空间级 CalculateGeometricAttr 显式自含（原经 mGeoManager.h 传递）
//Mesh
//#include "MXMeshAPI.h"
#include "MXMesh.h"
#include "mxr/view/mPreMeshHighLightRender.h"
#include "mxr/view/HighlightController.h"

#include "mxr/label/mSymbolController.h"
#include "mxr/label/SymbolDescription.h"
#include "PickCommonFunction.h"

#include "mInfoWindow.h"
#include "mLineMeshPanel.h"
//【OCC摘除·S9】旧栈 set 反查改 GN
#include "mGeoNeutralQuery.h"
using namespace MechPreData;
using namespace MeshPanelData;
using namespace MxFunctions;
#include <algorithm>

namespace
{
    // 根据线 ID 反查所属部件名
    QString getPartNameByLineId(int lineId, MGeoData::GeoDataBase* geoDatabase)
    {
        //【OCC摘除·S9】旧栈 set 反查退役（恒 null/空），改 GN 离散数据检索
        auto* modelData0 = (getCVM != nullptr) ? getCVM->getModelData() : nullptr;
        auto* geoMgr0 = (modelData0 != nullptr) ? modelData0->getGeoManager() : nullptr;
        return MGeoNeutralQuery::partNameByGeoId(geoMgr0, 1, lineId);
    }

    // 获取某个部件名下 1D 线的总数
    int getPartLineCount(const QString& partName, MGeoData::GeoDataBase* geoDatabase)
    {
        //【OCC摘除·S9】旧栈 set 遍历退役，改 GN 离散表计数
        auto* mgr0 = (getCVM != nullptr && getCVM->getModelData() != nullptr) ? getCVM->getModelData()->getGeoManager() : nullptr;
        return (int)MGeoNeutralQuery::geoIdsByPartName(mgr0, 1, partName).size();
    }

    // 判断列表中是否已存在包含某个线 ID 的项
    bool listContainsLineId(QListWidget* list, int lineId)
    {
        for (int i = 0; i < list->count(); ++i)
        {
            QListWidgetItem* item = list->item(i);
            QVariantList varList = item->data(Qt::UserRole).toList();
            for (const QVariant& v : varList)
            {
                if (v.toInt() == lineId)
                    return true;
            }
        }
        return false;
    }

    // 向列表添加线：
    // - 若线所属部件只有一个 1D 线，显示为“部件名”
    // - 若线所属部件有多个 1D 线，显示为“部件名-几何线_ID”
    void addGroupedLineItemsToList(QListWidget* list, MGeoData::GeoDataBase* geoDatabase, const std::vector<int>& lineIds)
    {
        if (!geoDatabase || !list)
            return;

        for (int id : lineIds)
        {
            if (listContainsLineId(list, id))
                continue;

            QString partName = getPartNameByLineId(id, geoDatabase);
            if (partName.isEmpty())
                partName = QString("未命名部件");

            int lineCount = getPartLineCount(partName, geoDatabase);
            QString displayText = (lineCount <= 1) ? partName : QString("%1-几何线_%2").arg(partName).arg(id);

            QListWidgetItem* item = new QListWidgetItem(displayText);
            QVariantList varList;
            varList << id;
            item->setData(Qt::UserRole, varList);
            list->addItem(item);
        }
    }

    // 从单个列表项中读取所有线 ID
    std::vector<int> getLineIdsFromItem(QListWidgetItem* item)
    {
        std::vector<int> ids;
        if (!item)
            return ids;
        QVariantList varList = item->data(Qt::UserRole).toList();
        for (const QVariant& v : varList)
            ids.push_back(v.toInt());
        return ids;
    }

    // 从整个列表中读取所有线 ID
    std::vector<int> getAllLineIdsFromList(QListWidget* list)
    {
        std::vector<int> allIds;
        if (!list)
            return allIds;
        for (int i = 0; i < list->count(); ++i)
        {
            std::vector<int> ids = getLineIdsFromItem(list->item(i));
            allIds.insert(allIds.end(), ids.begin(), ids.end());
        }
        return allIds;
    }

    // 将 QListWidget 中的项同步为后端可识别的 "几何线_ID" 列表
    void syncLineListToMeshPanelData(QListWidget* list, QStringList& outList)
    {
        outList.clear();
        std::vector<int> ids = getAllLineIdsFromList(list);
        for (int id : ids)
            outList << QString("几何线_%1").arg(id);
    }
}
#include <set>
#include "mOtherFunction.h"
Q_DECLARE_METATYPE(std::set<int>)
Q_DECLARE_METATYPE(MeshParam)          // 头文件

namespace MechMainWindow
{

    mLineMeshPanel::mLineMeshPanel(QWidget* parent)
        : mMeshPanel(parent)
    {
        auto* mainLay = new QVBoxLayout(this);

        m_rangeFold = new FoldSection("尺寸", this);
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
        m_rangeFold->addRow(rangeLay);

        mainLay->addWidget(m_rangeFold);
        // ========== 几何体折叠块 ==========
        m_GeoFold = new FoldSection("几何线", this);
        auto* geoLay = new QHBoxLayout;
        //_geoLabel = new QLabel("部件集合");
        _geoComboBox = new QComboBox();
        _geoComboBox->addItem("请选择剖分部件");
        _geoComboBox->setItemData(0, 0, Qt::UserRole - 1); // 禁止选中
        _geoComboBox->setCurrentIndex(0);                 // 默认不选中任何项
        //程序化更新文本时不视为用户手动输入；用户手动输入后标记，后续添加/删减剖分几何不再自动更新尺寸
        connect(_meshSizeEdit, &QLineEdit::textChanged, [=]() {
            if (_programmaticSizeUpdate)
                return;
            if (!_meshSizeEdit->text().isEmpty())
                _userSizeManuallyEdited = true;
            });
        connect(_meshSizeEdit, &QLineEdit::textChanged, [=]() {
            if (_meshSizeEdit->text().isEmpty())
                return;
            _meshPanelData->setMeshSize(_meshSizeEdit->text().toDouble());  });
        connect(_meshSizeButton, &QToolButton::clicked, this, &mLineMeshPanel::setMeshSize);
        connect(_geoComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
            if (index <= 0)
                return;
            QString partName = _geoComboBox->currentText();

            MechModelData::mModelData* modelData = getCVM->getModelData();
            if (!modelData)
            {
                _geoComboBox->setCurrentIndex(0);
                return;
            }
            mMeshData* meshdata = modelData->getMeshData();
            if (!meshdata)
            {
                _geoComboBox->setCurrentIndex(0);
                return;
            }

            int partId = -1;
            for (int pid : meshdata->getAllPartIds())
            {
                if (meshdata->getPartNameByPartId(pid) == partName)
                {
                    partId = pid;
                    break;
                }
            }
            if (partId < 0)
            {
                _geoComboBox->setCurrentIndex(0);
                return;
            }

            auto* geoDatabase = modelData->getGeoData();
            if (!geoDatabase)
            {
                _geoComboBox->setCurrentIndex(0);
                return;
            }
            vector<int> lineids = MGeoNeutralQuery::geoIdsByPartId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 1, partId);

            // 点击下拉框项仅高亮该部件下全部几何线，不加入剖分列表；后续通过“+”按钮添加
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->ClearAll();
            getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightGeoModel();
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::GeoEdge, lineids);
            MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
            getCVM->getPreRenderWidget()->updatePaint();

            _geoComboBox->setCurrentIndex(0);
            });

        //已经赋予的部件列表
        QGridLayout* assignLayout = new QGridLayout();
        _listAssignPart = new QListWidget();
        _listAssignPart->setSelectionMode(QAbstractItemView::ExtendedSelection);
        allBtn = new QToolButton(); allBtn->setIcon(QIcon(":/mMainWidgets/Icon/xuanzequanbu.png")); allBtn->setToolTip("全部几何线");
        addBtn = new QToolButton(); addBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png")); addBtn->setToolTip("增加几何线");
        reduceBtn = new QToolButton(); reduceBtn->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png")); reduceBtn->setToolTip("减少几何线");
        deleteBtn = new QToolButton(); deleteBtn->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png")); deleteBtn->setToolTip("清空几何线");
        connect(allBtn, &QToolButton::clicked, [=]() {
            //【GUI适配】新 getAllGeoID(1) 未推送（下方既有静态链等价）
            std::set<int> meshGeoId = getGEOServer.geoCalculate().getAllGeoID(1);
            std::vector<int> allIds(meshGeoId.begin(), meshGeoId.end());
            auto* modelData = getCVM->getModelData();
            if (!modelData) return;
            auto* geoDatabase = modelData->getGeoData();
            if (!geoDatabase) return;
            addGroupedLineItemsToList(_listAssignPart, geoDatabase, allIds);
            syncLineListToMeshPanelData(_listAssignPart, _meshPanelData->_list);

            //update父节点状态
            emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
            });
        connect(addBtn, &QToolButton::clicked, [=]() {
            auto* modelData = getCVM->getModelData();
            if (!modelData) return;
            auto* geoDatabase = modelData->getGeoData();
            if (!geoDatabase) return;

            vector<int> lineids = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
            addGroupedLineItemsToList(_listAssignPart, geoDatabase, lineids);
            syncLineListToMeshPanelData(_listAssignPart, _meshPanelData->_list);

            // 添加完成后清空渲染拾取高亮
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->ClearAll();
            getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightGeoModel();
            getCVM->getPreRenderWidget()->updatePaint();

            emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
            });
        connect(reduceBtn, &QToolButton::clicked, [=]() {
            QList<QListWidgetItem*> items = _listAssignPart->selectedItems();
            if (items.isEmpty())
                return;
            for (int i = items.count() - 1; i >= 0; --i)
                delete _listAssignPart->takeItem(_listAssignPart->row(items[i]));

            syncLineListToMeshPanelData(_listAssignPart, _meshPanelData->_list);

            emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
            });
        connect(deleteBtn, &QToolButton::clicked, [=]() {
            /* getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->ClearAll();
             getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightGeoModel();
             getCVM->getPreRenderWidget()->updatePaint();*/
            _listAssignPart->clear();
            _meshPanelData->_list.clear();
            emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
            });
        connect(_listAssignPart, &QListWidget::itemClicked, [=](QListWidgetItem* item) {
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->ClearAll();
            getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightGeoModel();
            auto items = _listAssignPart->selectedItems();
            vector<int> ids;
            for (auto selectedItem : items)
            {
                std::vector<int> itemIds = getLineIdsFromItem(selectedItem);
                ids.insert(ids.end(), itemIds.begin(), itemIds.end());
            }
            if (!ids.empty())
            {
                getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::GeoEdge, ids);
                MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
                getCVM->getPreRenderWidget()->updatePaint();
            }
            });

        assignLayout->addWidget(_listAssignPart, 0, 0, 8, 7);
        assignLayout->addWidget(allBtn, 0, 7);
        assignLayout->addWidget(addBtn, 1, 7);
        assignLayout->addWidget(reduceBtn, 2, 7);
        assignLayout->addWidget(deleteBtn, 3, 7);


        m_GeoFold->addRow(new PanelLabel("剖分部件"), _geoComboBox);
        m_GeoFold->addRow(assignLayout);
        m_foldWidgets.append(m_GeoFold);


        mainLay->addWidget(m_GeoFold);
        // ========== 阶次折叠块 ==========
        m_OrderFold = new FoldSection("阶次", this);
        _orderComboBox = new QComboBox();
        _orderComboBox->setItemData(0, 0, Qt::UserRole - 1); // 禁止选中
        _orderComboBox->setCurrentIndex(0);                 // 默认不选中任何项
        _orderComboBox->addItems({ "一阶" , "二阶" });//
        m_OrderFold->addRow(new PanelLabel("网格阶次"), _orderComboBox);

        mainLay->addWidget(m_OrderFold);
        mainLay->addStretch();
        connect(_orderComboBox, &QComboBox::currentTextChanged, [=]() {_meshPanelData->setMeshOrder(_orderComboBox->currentText()); });
    }

    mLineMeshPanel::~mLineMeshPanel()
    {

    }
    void mLineMeshPanel::newMeshPanel()
    {
        //新建节点时重置手动输入标志，恢复自动计算默认尺寸
        _userSizeManuallyEdited = false;
    }
    void mLineMeshPanel::editMeshPanel(int meshId)
    {
        _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(meshId);
        auto a = _meshPanelData->getMeshPanelType();
        updatePanel();

    }
    void mLineMeshPanel::getMeshInfoById(const std::vector<int> ids)
    {
        if (ids.size() <= 0)
        {
            return;
        }
        if (ids.size() == 1)
        {
            return;
        }
        _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(ids[0]);
        double firstSize = _meshPanelData->getMeshSize();
        QString firstOrder = _meshPanelData->getMeshOrder();
        _programmaticSizeUpdate = true;
        _meshSizeEdit->setText(MxFunctions::mNumberToString(firstSize));
        _programmaticSizeUpdate = false;
        _orderComboBox->setCurrentText(firstOrder);
        //网格形状
        for (auto id : ids)
        {
            _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
            if (_meshPanelData->getMeshSize() != firstSize)
            {
                _programmaticSizeUpdate = true;
                _meshSizeEdit->setText("");
                _programmaticSizeUpdate = false;
            }
            if (_meshPanelData->getMeshOrder() != firstOrder)
            {
                _orderComboBox->setCurrentIndex(-1);   // 不选中任何项
            }
        }

        _geoComboBox->setDisabled(true);
        _listAssignPart->setDisabled(true);
        allBtn->setDisabled(true);
        addBtn->setDisabled(true);
        reduceBtn->setDisabled(true);
        deleteBtn->setDisabled(true);

        connect(_meshSizeEdit, &QLineEdit::textChanged, [=]() {
            for (auto id : ids)
            {
                _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                _meshPanelData->setMeshSize(_meshSizeEdit->text().toDouble());
            }
            });
        connect(_orderComboBox, &QComboBox::currentTextChanged, [=]() {
            for (auto id : ids)
            {
                _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                _meshPanelData->setMeshOrder(_orderComboBox->currentText());
            }
            });
    }
    void mLineMeshPanel::refreshGeoComboBox()
    {
        if (!_geoComboBox)
            return;

        QString oldTip = _geoComboBox->itemText(0);
        _geoComboBox->clear();
        _geoComboBox->addItem(oldTip.isEmpty() ? QStringLiteral("请选择剖分部件") : oldTip);
        _geoComboBox->setItemData(0, 0, Qt::UserRole - 1); // 禁止选中

        MechModelData::mModelData* modelData = getCVM->getModelData();
        if (!modelData)
        {
            _geoComboBox->setCurrentIndex(0);
            return;
        }

        mMeshData* meshData = modelData->getMeshData();
        if (!meshData)
        {
            _geoComboBox->setCurrentIndex(0);
            return;
        }

        MGeoData::GeoDataBase* geoDatabase = modelData->getGeoData();
        if (!geoDatabase)
        {
            _geoComboBox->setCurrentIndex(0);
            return;
        }

        QSet<QString> addedItems;
        std::vector<int> partIDs = meshData->getAllPartIds();
        for (int partId : partIDs)
        {
            QString partName = meshData->getPartNameByPartId(partId);
            if (partName.isEmpty())
                continue;

            std::vector<int> lineids = MGeoNeutralQuery::geoIdsByPartId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 1, partId);
            if (lineids.empty())
                continue;

            if (!addedItems.contains(partName))
            {
                _geoComboBox->addItem(partName);
                addedItems.insert(partName);
            }
        }
        _geoComboBox->setCurrentIndex(0);
    }

    void mLineMeshPanel::updatePanel()
    {
        // 刷新剖分部件下拉框：加载模型树中所有部件名
        refreshGeoComboBox();

        //网格尺寸
        double size = _meshPanelData->getMeshSize();
        _programmaticSizeUpdate = true;
        _meshSizeEdit->setText(MxFunctions::mNumberToString(size));
        _programmaticSizeUpdate = false;
        //几何线列表
        _listAssignPart->clear();
        {
            MechModelData::mModelData* modelData = getCVM->getModelData();
            if (modelData)
            {
                MGeoData::GeoDataBase* geoDatabase = modelData->getGeoData();
                if (geoDatabase)
                {
                    std::vector<int> cachedIds;
                    for (const QString& s : _meshPanelData->_list)
                    {
                        QStringList strlist = s.split("_");
                        if (strlist.size() >= 2)
                            cachedIds.push_back(strlist[1].toInt());
                    }
                    addGroupedLineItemsToList(_listAssignPart, geoDatabase, cachedIds);
                }
            }
        }

        //阶次
        QString order = _meshPanelData->getMeshOrder();
        if (order.isEmpty())
            order = "一阶";   //空值兜底，避免下拉框残留上一个节点的显示状态
        _orderComboBox->setCurrentText(order);
    }

    void mLineMeshPanel::setMeshSize()
    {
        MechModelData::mModelData* modelData = getCVM->getModelData();
        if (!modelData) return;

        mMeshData* meshdata = modelData->getMeshData();
        if (!meshdata) return;

        auto* meshEntity = meshdata->getMeshEntity();
        if (!meshEntity) return;

        // 获取预剖分尺寸
        const auto& size_pair = meshEntity->getAndSetPreMeshSize(1, false);
        if (size_pair.empty()) return;

        // 获取平均尺寸
        double sum = std::accumulate(size_pair.begin(), size_pair.end(), 0.0,
            [](double acc, const std::pair<int, double>& pair) {
                return acc + pair.second;
            });
        sum /= size_pair.size();

        // 找到最小尺寸
        auto min_size = findMinDouble(size_pair);

        if (min_size > 0)  // 避免除零错误
        {
            //推荐尺寸值（src 侧 mNumberToString 默认精度 15，替代原保留 5 位）
            QString str = MxFunctions::mNumberToString(sum);
            _userSizeManuallyEdited = false;   //恢复默认尺寸后重新允许自动计算
            _programmaticSizeUpdate = true;
            _meshSizeEdit->setText(str);
            _programmaticSizeUpdate = false;
        }
        else
        {
            _userSizeManuallyEdited = false;   //恢复默认尺寸后重新允许自动计算
            _programmaticSizeUpdate = true;
            _meshSizeEdit->setText("0");
            _programmaticSizeUpdate = false;
        }
    }
    void mLineMeshPanel::initSize()
    {
        _meshSizeButton->click();
    }
}
