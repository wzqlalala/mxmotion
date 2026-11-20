
#include "mMeshPanel.h"
#include "UiMetrics.h"
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
//#include "mGeometricFeatureAttribute.h" //【OCC迁移·链A续清配套】命名空间级 CalculateGeometricAttr 显式自含（原经 mGeoManager.h 传递） //【OCC摘除·S10/P0-1】调用已切 engine().geoCalculate() 同语法视图
//Mesh
//#include "MXMeshAPI.h"
#include "mMeshData.h"
#include "MXMesh.h"
#include "mxr/view/mPreMeshHighLightRender.h"
#include "mxr/view/HighlightController.h"

#include "mxr/label/mSymbolController.h"
#include "mxr/label/SymbolDescription.h"
#include "PickCommonFunction.h"

#include "mInfoWindow.h"
#include "mFaceMeshPanel.h"
//【OCC摘除·S9】旧栈 set 反查改 GN
#include "mGeoNeutralQuery.h"
using namespace MechPreData;
using namespace MeshPanelData;
using namespace MxFunctions;

namespace
{
    // 将 GBK/UTF-8 编码的 std::string 部件名转换为 QString
    // MXGeoEntity::getPartName() 在 Windows 中文环境下通常存储的是 GBK
    //（由 GeoDataModelSet::getPartName().toLocal8Bit().toStdString() 写入）
    QString partNameStringToQString(const std::string& name)
    {
        if (name.empty())
            return QString();
        QString localName = QString::fromLocal8Bit(name.c_str());
        if (!localName.isEmpty() && !localName.contains(QChar(0xFFFD)))
            return localName;
        return QString::fromUtf8(name.c_str());
    }

    // 根据面 ID 反查所属部件名
    // 优先使用 MXGeoFace::getPartID() + mMeshData::getPartNameByPartId()，
    // 与体剖分、模型树保持同一数据源，可正确显示中文；
    // 对于 toggleFace，通过其子面（原始几何面）的 partID 获取。
    QString getPartNameByFaceId(int faceId, mMeshData* meshData, MGeoData::GeoDataBase* geoDatabase, MeshEntity* meshEntity)
    {
        if (!meshData)
        {
            //【OCC摘除·S9】旧栈 set 反查退役（恒 null/空），无网格数据时改 GN 离散数据检索兜底
            auto* modelData0 = (getCVM != nullptr) ? getCVM->getModelData() : nullptr;
            auto* geoMgr0 = (modelData0 != nullptr) ? modelData0->getGeoManager() : nullptr;
            return MGeoNeutralQuery::partNameByGeoId(geoMgr0, 2, faceId);
        }

        auto getNameByPartId = [meshData](int partId) -> QString {
            if (!meshData || partId < 0)
                return QString();
            return meshData->getPartNameByPartId(partId);
        };

        // 1. 优先从 MXGeoFace 自身读取 partID，再通过 meshData 获取 QString 部件名
        if (meshEntity && meshData)
        {
            MXGeoFace* face = meshEntity->getGeoFaceById(faceId);
            if (face)
            {
                QString selfName = getNameByPartId(face->getPartID());
                if (!selfName.isEmpty())
                    return selfName;

                // 2. 若当前面是 toggleFace，从其原始子面读取 partID
                for (auto* subFace : meshEntity->getGeoFaces())
                {
                    if (subFace && subFace->toggleFace == face)
                    {
                        QString subName = getNameByPartId(subFace->getPartID());
                        if (!subName.isEmpty())
                            return subName;
                    }
                }

                // 3. 若当前面本身是子面，尝试其 toggleFace 的 partID（兼容处理）
                if (face->toggleFace)
                {
                    QString toggleName = getNameByPartId(face->toggleFace->getPartID());
                    if (!toggleName.isEmpty())
                        return toggleName;
                }
            }
        }

        // 4. 回退：通过 meshData 的 geoId -> partId -> partName 映射
        if (meshData)
        {
            int partId = meshData->getPartIdByGeoFace(faceId);
            if (partId >= 0)
            {
                QString partName = meshData->getPartNameByPartId(partId);
                if (!partName.isEmpty())
                    return partName;
            }

            if (meshEntity)
            {
                MXGeoFace* face = meshEntity->getGeoFaceById(faceId);
                if (face)
                {
                    for (auto* subFace : meshEntity->getGeoFaces())
                    {
                        if (subFace && subFace->toggleFace == face)
                        {
                            int subPartId = meshData->getPartIdByGeoFace(subFace->getGeoTag());
                            if (subPartId >= 0)
                            {
                                QString subPartName = meshData->getPartNameByPartId(subPartId);
                                if (!subPartName.isEmpty())
                                    return subPartName;
                            }
                        }
                    }
                }
            }
        }

        // 5. 回退：旧栈 set 反查已退役（恒 null/空），改 GN 离散数据检索【OCC摘除·S9】
        {
            auto* modelData0 = (getCVM != nullptr) ? getCVM->getModelData() : nullptr;
            auto* geoMgr0 = (modelData0 != nullptr) ? modelData0->getGeoManager() : nullptr;
            QString gnName = MGeoNeutralQuery::partNameByGeoId(geoMgr0, 2, faceId);
            if (!gnName.isEmpty())
                return gnName;
        }

        // 6. 最后回退：从几何面对象自身的 std::string partName 解析
        if (meshEntity)
        {
            MXGeoFace* face = meshEntity->getGeoFaceById(faceId);
            if (face)
            {
                QString selfPartName = partNameStringToQString(face->getPartName());
                if (!selfPartName.isEmpty())
                    return selfPartName;

                if (face->toggleFace)
                {
                    QString togglePartName = partNameStringToQString(face->toggleFace->getPartName());
                    if (!togglePartName.isEmpty())
                        return togglePartName;
                }
            }
        }

        return QString();
    }

    // 获取某个部件名下 2D 面的总数
    int getPartFaceCount(const QString& partName, MGeoData::GeoDataBase* geoDatabase)
    {
        //【OCC摘除·S9】旧栈 set 遍历退役，改 GN 离散表计数
        auto* mgr0 = (getCVM != nullptr && getCVM->getModelData() != nullptr) ? getCVM->getModelData()->getGeoManager() : nullptr;
        return (int)MGeoNeutralQuery::geoIdsByPartName(mgr0, 2, partName).size();
    }

    // 判断列表中是否已存在包含某个面 ID 的项
    bool listContainsFaceId(QListWidget* list, int faceId)
    {
        for (int i = 0; i < list->count(); ++i)
        {
            QListWidgetItem* item = list->item(i);
            QVariantList varList = item->data(Qt::UserRole).toList();
            for (const QVariant& v : varList)
            {
                if (v.toInt() == faceId)
                    return true;
            }
        }
        return false;
    }

    // 向列表添加面：
    // - checkExistence 为 true 时（模型树点击回显已保存的剖分对象），
    //   对象已不存在（m_idToGFace 中查不到，或虽在缓存中但部件归属全部失效）的显示为“未命名对象”；
    //   新增拾取路径必须传 false：首次添加时 m_idToGFace 尚未构建（size 为 0），
    //   其映射是在添加流程后续的默认尺寸计算中才建立的
    // - 若面所属部件只有一个 2D 面，显示为“部件名”
    // - 若面所属部件有多个 2D 面，显示为“部件名-几何面_ID”
    void addGroupedFaceItemsToList(QListWidget* list, mMeshData* meshData, MGeoData::GeoDataBase* geoDatabase, MeshEntity* meshEntity, const std::vector<int>& faceIds, bool checkExistence = false)
    {
        if (!list || (!meshData && !geoDatabase))
            return;

        for (int id : faceIds)
        {
            if (listContainsFaceId(list, id))
                continue;

            //不存在的对象本次仍显示“未命名对象”占位项：不携带 ID（数据引用已在 updatePanel 中删除），
            //下次进入面板该项不再出现
            if (checkExistence && !MechMainWindow::mFaceMeshPanel::isFaceMeshObjectAlive(id))
            {
                list->addItem(new QListWidgetItem(QString("未命名对象")));
                continue;
            }

            QString partName = getPartNameByFaceId(id, meshData, geoDatabase, meshEntity);
            if (partName.isEmpty())
                partName = QString("未命名部件");

            int faceCount = getPartFaceCount(partName, geoDatabase);
            QString displayText = (faceCount <= 1) ? partName : QString("%1-几何面_%2").arg(partName).arg(id);

            QListWidgetItem* item = new QListWidgetItem(displayText);
            QVariantList varList;
            varList << id;
            item->setData(Qt::UserRole, varList);
            list->addItem(item);
        }
    }

    // 从单个列表项中读取所有面 ID
    std::vector<int> getFaceIdsFromItem(QListWidgetItem* item)
    {
        std::vector<int> ids;
        if (!item)
            return ids;
        QVariantList varList = item->data(Qt::UserRole).toList();
        for (const QVariant& v : varList)
            ids.push_back(v.toInt());
        return ids;
    }

    // 从整个列表中读取所有面 ID
    std::vector<int> getAllFaceIdsFromList(QListWidget* list)
    {
        std::vector<int> allIds;
        if (!list)
            return allIds;
        for (int i = 0; i < list->count(); ++i)
        {
            std::vector<int> ids = getFaceIdsFromItem(list->item(i));
            allIds.insert(allIds.end(), ids.begin(), ids.end());
        }
        return allIds;
    }

    // 将 QListWidget 中的项同步为后端可识别的 "几何面_ID" 列表
    void syncFaceListToMeshPanelData(QListWidget* list, QStringList& outList)
    {
        outList.clear();
        std::vector<int> ids = getAllFaceIdsFromList(list);
        for (int id : ids)
            outList << QString("几何面_%1").arg(id);
    }

    // 清理已删除几何的残留面：从 MeshEntity 的 mFaces/m_idToGFace 中移除，
    // 并从视口拾取数据中剔除对应 ID，避免加号按钮把这些面重新加入剖分列表。
    // 不 delete 面指针：对象归属在数据层（deleteGeoPart 等负责释放），避免双重释放
    void purgeDeadGeoFaces(const std::vector<int>& deadFaceIds)
    {
        if (deadFaceIds.empty())
            return;

        MechModelData::mModelData* modelData = getCVM->getModelData();
        if (!modelData)
            return;
        mMeshData* meshdata = modelData->getMeshData();
        MeshEntity* meshEntity = meshdata ? meshdata->getMeshEntity() : nullptr;
        if (!meshEntity)
            return;

        for (int id : deadFaceIds)
        {
            MXGeoFace* face = meshEntity->getGeoFaceById(id);
            if (face)
                meshEntity->RemoveGeoFace(face);
            meshEntity->m_idToGFace.erase(id);
        }

        auto* selectData = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData();
        selectData->UpdatePickData(mxr::SelectionOperate::REDUCE, mxr::SelectionInfoType::GeoFace, deadFaceIds);
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();
    }

    // 计算默认尺寸时，将列表中的 toggleFace ID 还原为其子面（原始几何面）ID，
    // 避免 toggleFace 自身几何属性缺失导致尺寸计算为 0
    std::vector<int> getRealFaceIdsForDefaultSize(MeshEntity* meshEntity, const std::vector<int>& faceIds)
    {
        std::vector<int> realFaceIds;
        if (!meshEntity)
        {
            realFaceIds = faceIds;
            return realFaceIds;
        }

        QSet<int> uniqueIds;
        for (int faceId : faceIds)
        {
            MXGeoFace* targetFace = meshEntity->getGeoFaceById(faceId);
            if (!targetFace)
            {
                uniqueIds.insert(faceId);
                continue;
            }

            bool isToggleFace = false;
            for (auto* subFace : meshEntity->getGeoFaces())
            {
                if (subFace && subFace->toggleFace == targetFace)
                {
                    uniqueIds.insert(subFace->getGeoTag());
                    isToggleFace = true;
                }
            }

            if (!isToggleFace)
            {
                uniqueIds.insert(faceId);
            }
        }

        for (int id : uniqueIds)
            realFaceIds.push_back(id);
        return realFaceIds;
    }
}
#include <set>
#include "mOtherFunction.h"
Q_DECLARE_METATYPE(std::set<int>)
Q_DECLARE_METATYPE(MeshParam)          // 头文件
namespace MechMainWindow
{
    bool mFaceMeshPanel::isFaceMeshObjectAlive(int faceId)
    {
        MechModelData::mModelData* modelData = getCVM->getModelData();
        if (!modelData)
            return false;

        mMeshData* meshdata = modelData->getMeshData();
        MGeoData::GeoDataBase* geoDatabase = modelData->getGeoData();
        MeshEntity* meshEntity = meshdata ? meshdata->getMeshEntity() : nullptr;
        if (!meshEntity)
            return false;

        //m_idToGFace 中查不到：几何已被删除（getGeoFaceById 内部按需建立映射后查询）
        if (!meshEntity->getGeoFaceById(faceId))
            return false;

        //面仍在缓存中，但所有部件归属来源均已失效：几何删除后的残留面，同样视为不存在
        return !getPartNameByFaceId(faceId, meshdata, geoDatabase, meshEntity).isEmpty();
    }

    mFaceMeshPanel::mFaceMeshPanel(QWidget* parent)
        : mMeshPanel(parent)
    {
        auto* mainLay = new QVBoxLayout(this);

        m_rangeFold = new FoldSection("网格定义", this);
        auto* vLay = new QVBoxLayout;

        auto* rangeLay2 = new QHBoxLayout;

        _meshShapeLabel = new PanelLabel("网格形状");
        _meshShapeBox = new QComboBox();
        _meshShapeBox->addItems({ "三角形" , "四边形主导","全四边形"});

        _meshSizeLabel = new PanelLabel("网格尺寸"); 
        _meshSizeEdit = new QLineEdit("");
        auto* v = new QDoubleValidator(0.001, 1e11, 8, _meshSizeEdit);
        v->setNotation(QDoubleValidator::ScientificNotation);
        _meshSizeEdit->setValidator(v);
        _meshSizeButton = new QToolButton(); _meshSizeButton->setIcon(QIcon(":/mMainWidgets/Icon/post/reset.png")); _meshSizeButton->setToolTip("恢复默认尺寸");


        rangeLay2->addWidget(_meshSizeLabel);
        rangeLay2->addWidget(_meshSizeEdit);
        rangeLay2->addWidget(_meshSizeButton);

        
        vLay->addLayout(rangeLay2);
        m_rangeFold->addRow(_meshShapeLabel, _meshShapeBox);
        m_rangeFold->addRow(vLay);

        mainLay->addWidget(m_rangeFold);
        // ========== 几何面折叠块 ==========
        m_GeoFold = new FoldSection("几何面", this);
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
            vector<int> faceids = MGeoNeutralQuery::geoIdsByPartId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 2, partId);

            // 点击下拉框项仅高亮该部件下全部几何面，不加入剖分列表；后续通过“+”按钮添加
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->ClearAll();
            getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightGeoModel();
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::GeoFace, faceids);
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
        connect(_meshSizeButton, &QToolButton::clicked, this, &mFaceMeshPanel::setMeshSize);
        //已经赋予的部件列表
        QGridLayout* assignLayout = new QGridLayout();
        _listAssignPart = new QListWidget();
        _listAssignPart->setSelectionMode(QAbstractItemView::ExtendedSelection);
        // 设置支持中文的字体，避免中文字符显示为黑方块
        QFont listFont = _listAssignPart->font();
        listFont.setFamily(MxFunctions::UiMetrics::uiFontFamily());
        _listAssignPart->setFont(listFont);
        allBtn = new QToolButton(); allBtn->setIcon(QIcon(":/mMainWidgets/Icon/xuanzequanbu.png")); allBtn->setToolTip("全部几何面");
        addBtn = new QToolButton(); addBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png")); addBtn->setToolTip("增加几何面");
        reduceBtn = new QToolButton(); reduceBtn->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png")); reduceBtn->setToolTip("减少几何面");
        deleteBtn = new QToolButton(); deleteBtn->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png")); deleteBtn->setToolTip("清空几何面");
        connect(allBtn, &QToolButton::clicked, [=]() {
            auto* modelData = getCVM->getModelData();
            if (!modelData) return;
            auto* geoDatabase = modelData->getGeoData();
            if (!geoDatabase) return;
            std::set<int> meshGeoId = getGEOServer.geoCalculate().getAllGeoID(2);
            mMeshData* meshdata = modelData->getMeshData();
            if (!meshdata) return;
            auto* meshEntity = meshdata->getMeshEntity();
            if (!meshEntity) return;

            std::vector<int> allIds(meshGeoId.begin(), meshGeoId.end());
            addGroupedFaceItemsToList(_listAssignPart, meshdata, geoDatabase, meshEntity, allIds);
            syncFaceListToMeshPanelData(_listAssignPart, _meshPanelData->_list);

            //用户手动输入过尺寸值后，添加剖分几何不再自动更新尺寸
            if (_userSizeManuallyEdited)
            {
                emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
                return;
            }
                double defaultsizes = 2.0;
            std::vector<int> faceids = getAllFaceIdsFromList(_listAssignPart);
            std::vector<int> realFaceIds = getRealFaceIdsForDefaultSize(meshEntity, faceids);
            double defaultmeshSize = meshEntity->calculateDefaultMeshsizeBaseAttributeForFaceMesh(realFaceIds, defaultsizes);
            //推荐尺寸值四舍五入保留小数点后5位
            QString sizeStr = QString::number(defaultmeshSize, 'f', 5);
            defaultmeshSize = sizeStr.toDouble();
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
            mMeshData* meshdata = modelData->getMeshData();
            if (!meshdata) return;
            auto* meshEntity = meshdata->getMeshEntity();
            if (!meshEntity) return;

            vector<int> selectedFaceIds = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
            // 处理 toggleFace 映射：若选中的是被合并面的原始面，则替换为 toggle 目标面
            std::vector<int> mappedFaceIds;
            mappedFaceIds.reserve(selectedFaceIds.size());
            for (int id : selectedFaceIds)
            {
                int finalId = id;
                for (auto ita : meshEntity->getGeoFaces())
                {
                    if (ita->toggleFace && ita->getGeoTag() == id)
                    {
                        finalId = ita->toggleFace->getGeoTag();
                        break;
                    }
                }
                mappedFaceIds.push_back(finalId);
            }

            addGroupedFaceItemsToList(_listAssignPart, meshdata, geoDatabase, meshEntity, mappedFaceIds);
            syncFaceListToMeshPanelData(_listAssignPart, _meshPanelData->_list);
            //用户手动输入过尺寸值后，添加剖分几何不再自动更新尺寸
            if (_userSizeManuallyEdited)
            {
                emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
                return;
            }
            double defaultsizes = 2.0;
            std::vector<int> faceids = getAllFaceIdsFromList(_listAssignPart);
            std::vector<int> realFaceIds = getRealFaceIdsForDefaultSize(meshEntity, faceids);
            double defaultmeshSize = meshEntity->calculateDefaultMeshsizeBaseAttributeForFaceMesh(realFaceIds, defaultsizes);
            //推荐尺寸值四舍五入保留小数点后5位
            QString sizeStr = QString::number(defaultmeshSize, 'f', 5);
            defaultmeshSize = sizeStr.toDouble();
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

            syncFaceListToMeshPanelData(_listAssignPart, _meshPanelData->_list);

            auto* modelData = getCVM->getModelData();
                if (!modelData) return;
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
                double defaultsizes = 2.0;
            std::vector<int> faceids = getAllFaceIdsFromList(_listAssignPart);
            std::vector<int> realFaceIds = getRealFaceIdsForDefaultSize(meshEntity, faceids);
            double defaultmeshSize = meshEntity->calculateDefaultMeshsizeBaseAttributeForFaceMesh(realFaceIds, defaultsizes);
            //推荐尺寸值四舍五入保留小数点后5位
            QString sizeStr = QString::number(defaultmeshSize, 'f', 5);
            defaultmeshSize = sizeStr.toDouble();
                _meshPanelData->setMeshSize(defaultmeshSize);
                _meshPanelData->setPreDefineMeshSize(defaultmeshSize);
            _programmaticSizeUpdate = true;
            _meshSizeEdit->setText(sizeStr);
            _programmaticSizeUpdate = false;
            /*getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(mxr::SelectionOperate::CLEAR, mxr::SelectionInfoType::GeoFace, ids);
            MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
            getCVM->getPreRenderWidget()->updatePaint();*/
            emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
            });
        connect(deleteBtn, &QToolButton::clicked, [=]() {
          /*  getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->ClearAll();
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
                std::vector<int> itemIds = getFaceIdsFromItem(selectedItem);
                ids.insert(ids.end(), itemIds.begin(), itemIds.end());
            }
            if (!ids.empty())
            {
                getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::GeoFace, ids);
                MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
                getCVM->getPreRenderWidget()->updatePaint();
            }
            });
        assignLayout->setSpacing(10);
        assignLayout->addWidget(_listAssignPart, 0, 0, 8, 7);
        assignLayout->addWidget(allBtn, 0, 7);
        assignLayout->addWidget(addBtn, 1, 7);
        assignLayout->addWidget(reduceBtn, 2, 7);
        assignLayout->addWidget(deleteBtn, 3, 7);

        auto label = new PanelLabel("剖分部件");
        m_GeoFold->addRow(label, _geoComboBox);
        m_GeoFold->addRow(assignLayout);
        m_foldWidgets.append(m_GeoFold);


        mainLay->addWidget(m_GeoFold);
        // ========== 阶次折叠块 ==========
        m_OrderFold = new FoldSection("阶次", this);
        _orderComboBox = new QComboBox();
        _orderComboBox->setItemData(0, 0, Qt::UserRole - 1); // 禁止选中
        _orderComboBox->setCurrentIndex(0);                 // 默认不选中任何项
        _orderComboBox->addItems({ "一阶" , "二阶" });//
        label = new PanelLabel("网格阶次");
        m_OrderFold->addRow(label, _orderComboBox);

        mainLay->addWidget(m_OrderFold);
        mainLay->addStretch();
        connect(_orderComboBox, &QComboBox::currentTextChanged, [=]() {_meshPanelData->setMeshOrder(_orderComboBox->currentText()); });
    }

    mFaceMeshPanel::~mFaceMeshPanel()
    {

    }
    void mFaceMeshPanel::newMeshPanel()
    {
        //新建节点时重置手动输入标志，恢复自动计算默认尺寸
        _userSizeManuallyEdited = false;
    }
    void mFaceMeshPanel::editMeshPanel(int meshId)
    {
        _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(meshId);
        auto a = _meshPanelData->getMeshPanelType();
        updatePanel();

    }
    void mFaceMeshPanel::getMeshInfoById(const std::vector<int> ids)
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
    void mFaceMeshPanel::refreshGeoComboBox()
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

            std::vector<int> faceids = MGeoNeutralQuery::geoIdsByPartId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 2, partId);
            if (faceids.empty())
                continue;

            if (!addedItems.contains(partName))
            {
                _geoComboBox->addItem(partName);
                addedItems.insert(partName);
            }
        }
        _geoComboBox->setCurrentIndex(0);
    }

    void mFaceMeshPanel::updatePanel()
    {
        // 刷新剖分部件下拉框：加载模型树中所有部件名
        refreshGeoComboBox();

        //网格形状
        QString shape = _meshPanelData->getMeshShape();
        if (shape.isEmpty())
            shape = "三角形";   //空值兜底，避免下拉框残留上一个节点的显示状态
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
                mMeshData* meshdata = modelData->getMeshData();
                MeshEntity* meshEntity = meshdata ? meshdata->getMeshEntity() : nullptr;
                if (geoDatabase && meshEntity)
                {
                    std::vector<int> cachedIds;
                    std::vector<int> deadIds;
                    QStringList aliveList;
                    for (const QString& s : _meshPanelData->_list)
                    {
                        QStringList strlist = s.split("_");
                        if (strlist.size() >= 2 && strlist[0] == "几何面")
                        {
                            int faceId = strlist[1].toInt();
                            cachedIds.push_back(faceId);
                            if (isFaceMeshObjectAlive(faceId))
                                aliveList << s;
                            else
                                deadIds.push_back(faceId);
                        }
                        else
                        {
                            aliveList << s;
                        }
                    }
                    if (!deadIds.empty())
                    {
                        //已不存在的剖分对象：从面板存储中删除引用（本次面板仍显示“未命名对象”占位项），
                        //并清理 MeshEntity/拾取数据中的残留面，防止加号按钮再次拾取加入
                        _meshPanelData->_list = aliveList;
                        purgeDeadGeoFaces(deadIds);
                    }
                    addGroupedFaceItemsToList(_listAssignPart, meshdata, geoDatabase, meshEntity, cachedIds, true);
                }
            }
        }

        //阶次
        QString order = _meshPanelData->getMeshOrder();
        if (order.isEmpty())
            order = "一阶";   //空值兜底，避免下拉框残留上一个节点的显示状态
        _orderComboBox->setCurrentText(order);
    }
    void mFaceMeshPanel::setMeshSize()
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
        //const auto& size_pair = meshEntity->getAndSetPreMeshSize(2, false);
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
    void mFaceMeshPanel::initSize()
    {
        _meshSizeButton->click();
    }
}
