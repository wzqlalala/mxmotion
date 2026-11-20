
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
//#include "mGeoDataModelSet.h" //【OCC摘除·S10/P0-1 终局3】死 include（零使用；mGeoDataModelSet OCC 链）
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
//#include "mGeometricFeatureAttribute.h" //【OCC迁移·链A续清配套】CalculateGeometricAttr 显式自含 //【OCC摘除·S10/P0-1】调用已切 engine().geoCalculate() 同语法视图
//Mesh
//#include "MXMeshAPI.h"
#include "MXMesh.h"
#include "mxr/view/mPreMeshHighLightRender.h"
#include "mxr/view/HighlightController.h"

#include "mxr/label/mSymbolController.h"
#include "mxr/label/SymbolDescription.h"
#include "PickCommonFunction.h"

#include "mInfoWindow.h"
#include "mSolidMeshPanel.h"
//【OCC摘除·S9】旧栈 set 反查改 GN
#include "mGeoNeutralQuery.h"
using namespace MechPreData;
using namespace MeshPanelData;
using namespace MxFunctions;

namespace
{
    // 根据实体 ID 反查所属部件名
    QString getPartNameBySolidId(int solidId, MGeoData::GeoDataBase* geoDatabase)
    {
        //【OCC摘除·S9】旧栈 set 反查退役（恒 null/空），改 GN 离散数据检索
        auto* modelData0 = (getCVM != nullptr) ? getCVM->getModelData() : nullptr;
        auto* geoMgr0 = (modelData0 != nullptr) ? modelData0->getGeoManager() : nullptr;
        return MGeoNeutralQuery::partNameByGeoId(geoMgr0, 3, solidId);
    }

    // 获取某个部件名下 3D 实体的总数
    int getPartSolidCount(const QString& partName, MGeoData::GeoDataBase* geoDatabase)
    {
        //【OCC摘除·S9】旧栈 set 遍历退役，改 GN 离散表计数
        auto* mgr0 = (getCVM != nullptr && getCVM->getModelData() != nullptr) ? getCVM->getModelData()->getGeoManager() : nullptr;
        return (int)MGeoNeutralQuery::geoIdsByPartName(mgr0, 3, partName).size();
    }

    // 判断列表中是否已存在包含某个实体 ID 的项
    bool listContainsSolidId(QListWidget* list, int solidId)
    {
        for (int i = 0; i < list->count(); ++i)
        {
            QListWidgetItem* item = list->item(i);
            QVariantList varList = item->data(Qt::UserRole).toList();
            for (const QVariant& v : varList)
            {
                if (v.toInt() == solidId)
                    return true;
            }
        }
        return false;
    }

    // 向列表添加实体：
    // - 若实体所属部件只有一个 3D 实体，显示为“部件名”
    // - 若实体所属部件有多个 3D 实体，显示为“部件名-实体_ID”
    void addGroupedItemsToList(QListWidget* list, MGeoData::GeoDataBase* geoDatabase, const std::vector<int>& solidIds)
    {
        if (!geoDatabase || !list)
            return;

        for (int id : solidIds)
        {
            if (listContainsSolidId(list, id))
                continue;

            QString partName = getPartNameBySolidId(id, geoDatabase);
            if (partName.isEmpty())
                partName = QString("未命名部件");

            // 根据该部件下实体总数决定显示格式
            int solidCount = getPartSolidCount(partName, geoDatabase);
            QString displayText = (solidCount <= 1) ? partName : QString("%1-实体_%2").arg(partName).arg(id);

            QListWidgetItem* item = new QListWidgetItem(displayText);
            QVariantList varList;
            varList << id;
            item->setData(Qt::UserRole, varList);
            list->addItem(item);
        }
    }

    // 从单个列表项中读取所有实体 ID
    std::vector<int> getSolidIdsFromItem(QListWidgetItem* item)
    {
        std::vector<int> ids;
        if (!item)
            return ids;
        QVariantList varList = item->data(Qt::UserRole).toList();
        for (const QVariant& v : varList)
            ids.push_back(v.toInt());
        return ids;
    }

    // 从整个列表中读取所有实体 ID
    std::vector<int> getAllSolidIdsFromList(QListWidget* list)
    {
        std::vector<int> allIds;
        if (!list)
            return allIds;
        for (int i = 0; i < list->count(); ++i)
        {
            std::vector<int> ids = getSolidIdsFromItem(list->item(i));
            allIds.insert(allIds.end(), ids.begin(), ids.end());
        }
        return allIds;
    }

    // 将 QListWidget 中的项同步为后端可识别的 "实体_ID" 列表
    void syncListToMeshPanelData(QListWidget* list, QStringList& outList)
    {
        outList.clear();
        std::vector<int> ids = getAllSolidIdsFromList(list);
        for (int id : ids)
            outList << QString("实体_%1").arg(id);
    }
}
#include <set>
#include "mOtherFunction.h"
Q_DECLARE_METATYPE(std::set<int>)
Q_DECLARE_METATYPE(MeshParam)          // 头文件

namespace MechMainWindow
{
    mSolidMeshPanel::mSolidMeshPanel(QWidget* parent)
        : mMeshPanel(parent)
    {
        auto* mainLay = new QVBoxLayout(this);

        m_rangeFold = new FoldSection("网格定义", this);

        auto* rangeLay = new QHBoxLayout;

        _meshShapeLabel = new PanelLabel("网格形状");
        _meshShapeBox = new QComboBox();
        _meshShapeBox->addItems({ "四面体" , "六面体" });//

        _meshSizeLabel = new PanelLabel("网格尺寸");
        _meshSizeEdit = new QLineEdit("");
        auto* v = new QDoubleValidator(0.001, 1e11, 8, _meshSizeEdit);
        v->setNotation(QDoubleValidator::ScientificNotation);
        _meshSizeEdit->setValidator(v);
        _meshSizeButton = new QToolButton(); _meshSizeButton->setIcon(QIcon(":/mMainWidgets/Icon/post/reset.png")); _meshSizeButton->setToolTip("恢复默认尺寸");
        rangeLay->addWidget(_meshSizeLabel);
        rangeLay->addWidget(_meshSizeEdit);
        rangeLay->addWidget(_meshSizeButton);

        m_rangeFold->addRow(_meshShapeLabel, _meshShapeBox);
        m_rangeFold->addRow(rangeLay);

        mainLay->addWidget(m_rangeFold);
        // ========== 几何体折叠块 ==========
        m_GeoFold = new FoldSection("几何体", this);
        auto* geoLay = new QHBoxLayout;
        //_geoLabel = new QLabel("部件集合");
        _geoComboBox = new QComboBox();
        _geoComboBox->addItem("请选择剖分部件");
        _geoComboBox->setItemData(0, 0, Qt::UserRole - 1); // 禁止选中
        _geoComboBox->setCurrentIndex(0);                 // 默认不选中任何项

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
            vector<int> solidids = MGeoNeutralQuery::geoIdsByPartId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 3, partId);

            // 点击下拉框项仅高亮该部件下全部实体，不加入剖分列表；后续通过“+”按钮添加
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->ClearAll();
            getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightGeoModel();
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::GeoSolid, solidids);
            MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
            getCVM->getPreRenderWidget()->updatePaint();

            _geoComboBox->setCurrentIndex(0);
            });
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
            double resetSize = _meshSizeEdit->text().toDouble();
            if (resetSize < (0.01 * _meshPanelData->getPreDefineMeshSize()))
            {
                if (resetSize > 0.0)
                {
                    _programmaticSizeUpdate = true;
                    _meshSizeEdit->setText(QString::number(_meshPanelData->getPreDefineMeshSize()));
                    _programmaticSizeUpdate = false;
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, QString("%1").arg(QString("尺寸设置过小")), true);
                    return;
                }
            }
            _meshPanelData->setMeshSize(_meshSizeEdit->text().toDouble()); });
        connect(_meshShapeBox, &QComboBox::currentTextChanged, [=]() {_meshPanelData->setMeshShape(_meshShapeBox->currentText()); });
        connect(_meshSizeButton, &QToolButton::clicked, this, &mSolidMeshPanel::setMeshSize);
        //已经赋予的部件列表
        QGridLayout* assignLayout = new QGridLayout();
        _listAssignPart = new QListWidget();
        _listAssignPart->setSelectionMode(QAbstractItemView::ExtendedSelection);
        allBtn = new QToolButton(); allBtn->setIcon(QIcon(":/mMainWidgets/Icon/xuanzequanbu.png")); allBtn->setToolTip("全部几何体");
        addBtn = new QToolButton(); addBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png")); addBtn->setToolTip("增加几何体");
        reduceBtn = new QToolButton(); reduceBtn->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png")); reduceBtn->setToolTip("减少几何体");
        deleteBtn = new QToolButton(); deleteBtn->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png")); deleteBtn->setToolTip("清空几何体");
        connect(allBtn, &QToolButton::clicked, [=]() {
            auto* modelData = getCVM->getModelData();
            if (!modelData) return;
            auto* geoDatabase = modelData->getGeoData();
            if (!geoDatabase) return;

            std::set<int> meshGeoId = getGEOServer.geoCalculate().getAllGeoID(3);
            std::vector<int> allIds(meshGeoId.begin(), meshGeoId.end());
            addGroupedItemsToList(_listAssignPart, geoDatabase, allIds);

            syncListToMeshPanelData(_listAssignPart, _meshPanelData->_list);

            mMeshData* meshdata = modelData->getMeshData();
            if (!meshdata) return;

            auto* meshEntity = meshdata->getMeshEntity();
            if (!meshEntity) return;
            //用户手动输入过尺寸值后，添加剖分几何不再自动更新尺寸
            if (_userSizeManuallyEdited)
            {
                emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
                return;
            }
            vector<double>defaultsizes;
            bool sizeDiff = false;
            std::vector<int> solidids = getAllSolidIdsFromList(_listAssignPart);
            double defaultmeshSize = meshEntity->calculateDefaultMeshsizeBaseAttributeForSolidMesh(solidids, defaultsizes, sizeDiff);
            //推荐尺寸值四舍五入保留小数点后5位
            QString sizeStr = QString::number(defaultmeshSize, 'f', 5);
            defaultmeshSize = sizeStr.toDouble();
            if (sizeDiff)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, QString("%1").arg(QString("实体剖分选取的剖分对象大小差距较大，造成计算的预定义尺寸不同")), true);
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, QString("%1").arg(QString("选取最小实体的计算剖分尺寸作为当前体剖分的预定义尺寸")), true);
            }
            _meshPanelData->setMeshSize(defaultmeshSize);
            _meshPanelData->setPreDefineMeshSize(defaultmeshSize);
            _programmaticSizeUpdate = true;
            _meshSizeEdit->setText(sizeStr);
            _programmaticSizeUpdate = false;
            
            emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
            });
        connect(addBtn, &QToolButton::clicked, [=]() {
            auto* modelData = getCVM->getModelData();
            if (!modelData) return;
            auto* geoDatabase = modelData->getGeoData();
            if (!geoDatabase) return;

            vector<int> solidids = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoSolid);
            addGroupedItemsToList(_listAssignPart, geoDatabase, solidids);
            syncListToMeshPanelData(_listAssignPart, _meshPanelData->_list);

            mMeshData* meshdata = modelData->getMeshData();
            if (!meshdata) return;

            auto* meshEntity = meshdata->getMeshEntity();
            if (!meshEntity) return;
            //用户手动输入过尺寸值后，添加剖分几何不再自动更新尺寸
            if (_userSizeManuallyEdited)
            {
                emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
                return;
            }
            vector<double>defaultsizes;
            bool sizeDiff = false;
            std::vector<int> currentIds = getAllSolidIdsFromList(_listAssignPart);
            double defaultmeshSize = meshEntity->calculateDefaultMeshsizeBaseAttributeForSolidMesh(currentIds, defaultsizes, sizeDiff);
            //推荐尺寸值四舍五入保留小数点后5位
            QString sizeStr = QString::number(defaultmeshSize, 'f', 5);
            defaultmeshSize = sizeStr.toDouble();
            if (sizeDiff)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, QString("%1").arg(QString("实体剖分选取的剖分对象大小差距较大，造成计算的预定义尺寸不同")), true);
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, QString("%1").arg(QString("选取最小实体的计算剖分尺寸作为当前体剖分的预定义尺寸")), true);
            }
            _meshPanelData->setMeshSize(defaultmeshSize);
            _meshPanelData->setPreDefineMeshSize(defaultmeshSize);
            _programmaticSizeUpdate = true;
            _meshSizeEdit->setText(sizeStr);
            _programmaticSizeUpdate = false;

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
            
            syncListToMeshPanelData(_listAssignPart, _meshPanelData->_list);
            
            MechModelData::mModelData* modelData = getCVM->getModelData();            if (!modelData) return;

            mMeshData* meshdata = modelData->getMeshData();
            if (!meshdata) return;

            auto* meshEntity = meshdata->getMeshEntity();
            if (!meshEntity) return;
            //用户手动输入过尺寸值后，减少剖分几何不再自动更新尺寸
            if (_userSizeManuallyEdited)
            {
                emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
                return;
            }
            vector<double>defaultsizes;
            bool sizeDiff = false;
            std::vector<int> solidids = getAllSolidIdsFromList(_listAssignPart);
            double defaultmeshSize = meshEntity->calculateDefaultMeshsizeBaseAttributeForSolidMesh(solidids, defaultsizes, sizeDiff);
            //推荐尺寸值四舍五入保留小数点后5位
            QString sizeStr = QString::number(defaultmeshSize, 'f', 5);
            defaultmeshSize = sizeStr.toDouble();
            if (sizeDiff)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, QString("%1").arg(QString("实体剖分选取的剖分对象大小差距较大，造成计算的预定义尺寸不同")), true);
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, QString("%1").arg(QString("选取最小实体的计算剖分尺寸作为当前体剖分的预定义尺寸")), true);
            }
            _meshPanelData->setMeshSize(defaultmeshSize);
            _meshPanelData->setPreDefineMeshSize(defaultmeshSize);
            _programmaticSizeUpdate = true;
            _meshSizeEdit->setText(sizeStr);
            _programmaticSizeUpdate = false;

            emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
            });
        connect(deleteBtn, &QToolButton::clicked, [=]() {
            /*getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->ClearAll();
            getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightGeoModel();
            getCVM->getPreRenderWidget()->updatePaint();*/
            _listAssignPart->clear();
            _meshPanelData->_list.clear();
            _meshPanelData->setMeshSize(2.0);
            _meshPanelData->setPreDefineMeshSize(0.0);
            _userSizeManuallyEdited = false;   //清空几何体后恢复自动计算
            _programmaticSizeUpdate = true;
            _meshSizeEdit->setText(QString::number(2.0));
            _programmaticSizeUpdate = false;
            emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
            });
        connect(_listAssignPart, &QListWidget::itemClicked, [=](QListWidgetItem* item) {
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->ClearAll();
            getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightGeoModel();
            auto items = _listAssignPart->selectedItems();
            vector<int> ids;
            for (auto selectedItem : items)
            {
                std::vector<int> itemIds = getSolidIdsFromItem(selectedItem);
                ids.insert(ids.end(), itemIds.begin(), itemIds.end());
            }
            if (!ids.empty())
            {
                getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::GeoSolid, ids);
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

    mSolidMeshPanel::~mSolidMeshPanel()
    {

    }
    void mSolidMeshPanel::newMeshPanel()
    {
        //新建节点时重置手动输入标志，恢复自动计算默认尺寸
        _userSizeManuallyEdited = false;
    }
    void mSolidMeshPanel::editMeshPanel(int meshId)
    {
        _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(meshId);
        auto a = _meshPanelData->getMeshPanelType();
        updatePanel();

    }
    void mSolidMeshPanel::getMeshInfoById(const std::vector<int> ids)
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
        QString firstShape = _meshPanelData->getMeshShape();
        double firstSize = _meshPanelData->getMeshSize();
        QString firstOrder = _meshPanelData->getMeshOrder();
        _meshShapeBox->setCurrentText(firstShape);
        _programmaticSizeUpdate = true;
        _meshSizeEdit->setText(QString::number(firstSize));
        _programmaticSizeUpdate = false;
        _orderComboBox->setCurrentText(firstOrder);
        //网格形状
        for (auto id : ids)
        {
            _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
            if (_meshPanelData->getMeshShape() != firstShape)
            {
                _meshShapeBox->setCurrentIndex(-1);   // 不选中任何项
            }
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

        connect(_meshShapeBox, &QComboBox::currentTextChanged, [=]() {
            for (auto id : ids)
            {
                _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                _meshPanelData->setMeshShape(_meshShapeBox->currentText());
            }
            });
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
    void mSolidMeshPanel::refreshGeoComboBox()
    {
        if (!_geoComboBox)
            return;

        // 保留当前选择，清空除提示项外的旧项
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

            std::vector<int> solidids = MGeoNeutralQuery::geoIdsByPartId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 3, partId);
            if (solidids.empty())
                continue;

            // 下拉框仅显示部件名，不显示子实体项
            if (!addedItems.contains(partName))
            {
                _geoComboBox->addItem(partName);
                addedItems.insert(partName);
            }
        }
        _geoComboBox->setCurrentIndex(0);
    }

    void mSolidMeshPanel::updatePanel()
    {
        // 刷新剖分部件下拉框：加载模型树中所有部件名
        refreshGeoComboBox();

        //网格形状
        QString shape = _meshPanelData->getMeshShape();
        if (shape.isEmpty())
            shape = "四面体";
        _meshShapeBox->setCurrentText(shape);
        //网格尺寸
        double size = _meshPanelData->getMeshSize();
        _programmaticSizeUpdate = true;
        _meshSizeEdit->setText(QString::number(size));
        _programmaticSizeUpdate = false;
        //实体集合
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
                    addGroupedItemsToList(_listAssignPart, geoDatabase, cachedIds);
                }
            }
        }

        //阶次
        QString order = _meshPanelData->getMeshOrder();
        if (order.isEmpty())
            order = "一阶";   //空值兜底，避免下拉框残留上一个节点的显示状态
        _orderComboBox->setCurrentText(order);


    }
    void mSolidMeshPanel::setMeshSize()
    {
        MechModelData::mModelData* modelData = getCVM->getModelData();
        if (!modelData) return;

        mMeshData* meshdata = modelData->getMeshData();
        if (!meshdata) return;

        auto* meshEntity = meshdata->getMeshEntity();
        if (!meshEntity) return;
        if (_meshPanelData->getPreDefineMeshSize() > 0.0)
        {
            //恢复默认尺寸时同样保留小数点后5位
            QString str = QString::number(_meshPanelData->getPreDefineMeshSize(), 'f', 5);
            _userSizeManuallyEdited = false;   //恢复默认尺寸后重新允许自动计算
            _programmaticSizeUpdate = true;
            _meshSizeEdit->setText(str);
            _programmaticSizeUpdate = false;
            _meshPanelData->setMeshSize(_meshPanelData->getPreDefineMeshSize());
        }
        //// 获取预剖分尺寸
        //const auto& size_pair = meshEntity->getAndSetPreMeshSize(3, false);
        //if (size_pair.empty()) return;

        //// 获取平均尺寸
        //double sum = std::accumulate(size_pair.begin(), size_pair.end(), 0.0,
        //    [](double acc, const std::pair<int, double>& pair) {
        //        return acc + pair.second;
        //    });
        //sum /= size_pair.size();

        //// 找到最小尺寸
        //auto min_size = findMinDouble(size_pair);

        //if (min_size > 0)  // 避免除零错误
        //{
        //    QString str = MxFunctions::mNumberToString(sum);
        //    _meshSizeEdit->setText(str);
        //}
        //else
        //{
        //    _meshSizeEdit->setText("0");
        //}
    }
    void mSolidMeshPanel::initSize()
    {
        _meshSizeButton->click();
    }

}