
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
#include "mAutoAdaptPanel.h"
//【OCC摘除·S9】旧栈 set 反查改 GN
#include "mGeoNeutralQuery.h"
using namespace MechPreData;
using namespace MeshPanelData;
using namespace MxFunctions;
#include <set>
#include <algorithm>
#include "mOtherFunction.h"

namespace
{
    // 根据几何特征下拉框索引获取维度
    int getDimByGeoFeatureIndex(int index)
    {
        if (index == 0) return 2;   // 几何面
        if (index == 1) return 1;   // 几何线
        return 3;                   // 几何体
    }

    // 根据维度获取列表前缀
    QString getGeoPrefixByDim(int dim)
    {
        if (dim == 1) return QString("几何线");
        if (dim == 2) return QString("几何面");
        return QString("实体");
    }

    // 根据维度获取渲染拾取类型
    mxr::SelectionInfoType getPickTypeByDim(int dim)
    {
        if (dim == 1) return mxr::SelectionInfoType::GeoEdge;
        if (dim == 2) return mxr::SelectionInfoType::GeoFace;
        return mxr::SelectionInfoType::GeoSolid;
    }

    // 解析后端列表字符串"前缀_ID"（几何线/几何面/实体）的维度，无法识别时返回 0
    int getDimByListString(const QString& s)
    {
        if (s.startsWith(QString("几何线"))) return 1;
        if (s.startsWith(QString("几何面"))) return 2;
        if (s.startsWith(QString("实体"))) return 3;
        return 0;
    }

    // 根据几何 ID 和维度反查所属部件名
    QString getPartNameByGeoId(int geoId, int dim, MGeoData::GeoDataBase* geoDatabase)
    {
        //【OCC摘除·S9】旧栈 set 反查退役（恒 null/空），改 GN 离散数据检索
        auto* modelData0 = (getCVM != nullptr) ? getCVM->getModelData() : nullptr;
        auto* geoMgr0 = (modelData0 != nullptr) ? modelData0->getGeoManager() : nullptr;
        return MGeoNeutralQuery::partNameByGeoId(geoMgr0, dim, geoId);
    }

    // 获取某个部件名下指定维度几何的总数
    int getPartGeoCount(const QString& partName, int dim, MGeoData::GeoDataBase* geoDatabase)
    {
        //【OCC摘除·S9】旧栈 set 反查退役（恒 null/空），改 GN 离散数据检索
        auto* modelData0 = (getCVM != nullptr) ? getCVM->getModelData() : nullptr;
        auto* geoMgr0 = (modelData0 != nullptr) ? modelData0->getGeoManager() : nullptr;
        return (int)MGeoNeutralQuery::geoIdsByPartName(geoMgr0, dim, partName).size();
    }

    // 判断列表中是否已存在包含某个几何 ID 的项
    bool listContainsGeoId(QListWidget* list, int geoId)
    {
        for (int i = 0; i < list->count(); ++i)
        {
            QListWidgetItem* item = list->item(i);
            QVariantList varList = item->data(Qt::UserRole).toList();
            for (const QVariant& v : varList)
            {
                if (v.toInt() == geoId)
                    return true;
            }
        }
        return false;
    }

    // 向列表添加几何：单个显示为“部件名”，多个显示为“部件名-几何_ ID”
    void addGroupedGeoItemsToList(QListWidget* list, MGeoData::GeoDataBase* geoDatabase, const std::vector<int>& geoIds, int dim)
    {
        if (!geoDatabase || !list)
            return;
        for (int id : geoIds)
        {
            if (listContainsGeoId(list, id))
                continue;
            QString partName = getPartNameByGeoId(id, dim, geoDatabase);
            if (partName.isEmpty())
                partName = QString("未命名部件");
            int geoCount = getPartGeoCount(partName, dim, geoDatabase);
            QString displayText = (geoCount <= 1) ? partName : QString("%1-%2_%3").arg(partName).arg(getGeoPrefixByDim(dim)).arg(id);
            QListWidgetItem* item = new QListWidgetItem(displayText);
            QVariantList varList;
            varList << id;
            item->setData(Qt::UserRole, varList);
            item->setData(Qt::UserRole + 1, dim);
            list->addItem(item);
        }
    }

    // 从列表项读取几何 ID
    std::vector<int> getGeoIdsFromItem(QListWidgetItem* item)
    {
        std::vector<int> ids;
        if (!item)
            return ids;
        QVariantList varList = item->data(Qt::UserRole).toList();
        for (const QVariant& v : varList)
            ids.push_back(v.toInt());
        return ids;
    }

    // 从整个列表读取所有几何 ID
    std::vector<int> getAllGeoIdsFromList(QListWidget* list)
    {
        std::vector<int> allIds;
        if (!list)
            return allIds;
        for (int i = 0; i < list->count(); ++i)
        {
            std::vector<int> ids = getGeoIdsFromItem(list->item(i));
            allIds.insert(allIds.end(), ids.begin(), ids.end());
        }
        return allIds;
    }

    // 同步为后端可识别的列表字符串
    void syncGeoListToMeshPanelData(QListWidget* list, QStringList& outList, int dim)
    {
        outList.clear();
        std::vector<int> ids = getAllGeoIdsFromList(list);
        QString prefix = getGeoPrefixByDim(dim);
        for (int id : ids)
            outList << QString("%1_%2").arg(prefix).arg(id);
    }
}
Q_DECLARE_METATYPE(std::set<int>)
Q_DECLARE_METATYPE(MeshParam)          // 头文件

namespace MechMainWindow
{
    mAutoAdaptPanel::mAutoAdaptPanel(QWidget* parent)
    {
        auto* mainLay = new QVBoxLayout(this);


        m_paramFold = new FoldSection("参数");
        checkBox1 = nullptr;
        checkBox2 = new QCheckBox;
        checkBox3 = new QCheckBox;
        dimSmoothnessRatio = new QLabel("尺寸光滑率"); dimSmoothnessRatio->setFixedWidth(80);
        curvatureEigenValue = new QLabel("曲率特征值"); curvatureEigenValue->setFixedWidth(80);
        geoProximityValue = new QLabel("几何邻近值"); geoProximityValue->setFixedWidth(80);
        maxSize = new PanelLabel("最大尺寸");
        minSize = new PanelLabel("最小尺寸");
        geoFeature = new PanelLabel("几何特征");
        geoFeatureBox = new QComboBox; geoFeatureBox->addItems({ "几何面","几何线","几何体" });
        dimSmoothnessRatioEdit = new QLineEdit;
        curvatureEigenValueEdit = new QLineEdit;
        geoProximityValueEdit = new QLineEdit;
        QDoubleValidator* vdim = new QDoubleValidator(0, 10000, 2, dimSmoothnessRatioEdit);
        vdim->setNotation(QDoubleValidator::StandardNotation);   // 不接受科学计数法
        dimSmoothnessRatioEdit->setValidator(vdim);
        QDoubleValidator* vcur = new QDoubleValidator(0, 10000, 2, curvatureEigenValueEdit);
        vcur->setNotation(QDoubleValidator::StandardNotation);   // 不接受科学计数法
        curvatureEigenValueEdit->setValidator(vcur);
        QDoubleValidator* vgeo = new QDoubleValidator(0, 10000, 2, geoProximityValueEdit);
        vgeo->setNotation(QDoubleValidator::StandardNotation);   // 不接受科学计数法
        geoProximityValueEdit->setValidator(vgeo);
        maxSizeEdit = new QLineEdit;
        auto* v = new QDoubleValidator(0.001, 1e11, 8, maxSizeEdit);
        v->setNotation(QDoubleValidator::ScientificNotation);
        maxSizeEdit->setValidator(v);
        maxSizeButton = new QToolButton(); maxSizeButton->setIcon(QIcon(":/mMainWidgets/Icon/post/reset.png")); maxSizeButton->setToolTip("恢复默认尺寸");
        minSizeEdit = new QLineEdit;
        auto* v1 = new QDoubleValidator(0.001, 1e11, 8, minSizeEdit);
        v1->setNotation(QDoubleValidator::ScientificNotation);
        minSizeEdit->setValidator(v1);
        minSizeButton = new QToolButton(); minSizeButton->setIcon(QIcon(":/mMainWidgets/Icon/post/reset.png")); minSizeButton->setToolTip("恢复默认尺寸");
        checkBox2->setChecked(true);
        checkBox3->setChecked(true);

        inputWidgets[0].initWidget(nullptr, dimSmoothnessRatio, dimSmoothnessRatioEdit, "尺寸光滑率", 1.2);
        inputWidgets[1].initWidget(checkBox2, curvatureEigenValue, curvatureEigenValueEdit, "曲率特征值", 15);
        inputWidgets[2].initWidget(checkBox3, geoProximityValue, geoProximityValueEdit, "几何邻近值", 2);
        connect(checkBox2, &QCheckBox::toggled, [=](bool checked)
            {curvatureEigenValueEdit->setEnabled(checked);
        _meshPanelData->setCurvatureChecked(checked);  });
        connect(checkBox3, &QCheckBox::toggled, [=](bool checked)
            {geoProximityValueEdit->setEnabled(checked);
        _meshPanelData->setGeoProimityChecked(checked); });
        connect(geoFeatureBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index)
            {
                if (_meshPanelData)
                    _meshPanelData->_list.clear();
                _listAssignPart->clear();
                if (index == 0)
                    MechCommon::mMechGlobalSignals::getInstance()->activePickSig(3);
                else if (index == 1)
                    MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
                else if (index == 2)
                    MechCommon::mMechGlobalSignals::getInstance()->activePickSig(4);
                refreshGeoComboBox();
                emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
            });
        m_GeoFold = new FoldSection("几何特征");
        _geoComboBox = new QComboBox;
        _geoComboBox->addItem("请选择剖分部件");
        _geoComboBox->setItemData(0, 0, Qt::UserRole - 1); // 禁止选中
        _geoComboBox->setCurrentIndex(0);
        _geoLabel = new PanelLabel("几何特征集合");

        connect(_geoComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
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
            int dim = getDimByGeoFeatureIndex(geoFeatureBox->currentIndex());
            std::vector<int> geoIds = MGeoNeutralQuery::geoIdsByPartId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), dim, partId);

            // 点击下拉框项仅高亮该部件下全部几何，不加入列表；后续通过“+”按钮添加
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->ClearAll();
            getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightGeoModel();
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(mxr::SelectionOperate::ADD, getPickTypeByDim(dim), geoIds);
            MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
            getCVM->getPreRenderWidget()->updatePaint();

            _geoComboBox->setCurrentIndex(0);
            });
        QGridLayout* assignLayout = new QGridLayout();
        _listAssignPart = new QListWidget();
        _listAssignPart->setSelectionMode(QAbstractItemView::ExtendedSelection);
        allBtn = new QToolButton(); allBtn->setIcon(QIcon(":/mMainWidgets/Icon/xuanzequanbu.png")); allBtn->setToolTip("全部几何特征");
        addBtn = new QToolButton(); addBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png")); addBtn->setToolTip("增加几何特征");
        reduceBtn = new QToolButton(); reduceBtn->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png")); reduceBtn->setToolTip("减少几何特征");
        deleteBtn = new QToolButton(); deleteBtn->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png")); deleteBtn->setToolTip("清空几何特征");
        connect(allBtn, &QToolButton::clicked, [=]() {
            auto* modelData = getCVM->getModelData();
            if (!modelData) return;
            auto* geoDatabase = modelData->getGeoData();
            if (!geoDatabase) return;

            int dim = getDimByGeoFeatureIndex(geoFeatureBox->currentIndex());
            std::set<int> meshGeoId = getGEOServer.geoCalculate().getAllGeoID(dim);
            std::vector<int> allIds(meshGeoId.begin(), meshGeoId.end());
            addGroupedGeoItemsToList(_listAssignPart, geoDatabase, allIds, dim);
            syncGeoListToMeshPanelData(_listAssignPart, _meshPanelData->_list, dim);

            emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
            });
        connect(addBtn, &QToolButton::clicked, [=]() {
            auto* modelData = getCVM->getModelData();
            if (!modelData) return;
            auto* geoDatabase = modelData->getGeoData();
            if (!geoDatabase) return;

            int dim = getDimByGeoFeatureIndex(geoFeatureBox->currentIndex());
            mxr::SelectionInfoType pickType = getPickTypeByDim(dim);
            vector<int> geoIds = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(pickType);
            addGroupedGeoItemsToList(_listAssignPart, geoDatabase, geoIds, dim);
            syncGeoListToMeshPanelData(_listAssignPart, _meshPanelData->_list, dim);

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

            int dim = getDimByGeoFeatureIndex(geoFeatureBox->currentIndex());
            syncGeoListToMeshPanelData(_listAssignPart, _meshPanelData->_list, dim);

            emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
            });
        connect(deleteBtn, &QToolButton::clicked, [=]() {
            _listAssignPart->clear();
            _meshPanelData->_list.clear();
            emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
            });
        connect(_listAssignPart, &QListWidget::itemClicked, [=](QListWidgetItem* item) {
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->ClearAll();
            getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightGeoModel();
            auto items = _listAssignPart->selectedItems();
            // 显示文本带部件名前缀（如"部件A-几何面_12"），不能按文本拆分判断类型，
            // 必须从 Qt::UserRole 读几何 ID、Qt::UserRole+1 读维度
            int curDim = getDimByGeoFeatureIndex(geoFeatureBox->currentIndex());
            vector<int> lineIds;
            vector<int> faceIds;
            vector<int> solidIds;
            for (auto selectedItem : items)
            {
                int dim = selectedItem->data(Qt::UserRole + 1).toInt();
                if (dim != 1 && dim != 2 && dim != 3)
                    dim = curDim;
                for (int id : getGeoIdsFromItem(selectedItem))
                {
                    if (dim == 1)
                        lineIds.push_back(id);
                    else if (dim == 2)
                        faceIds.push_back(id);
                    else
                        solidIds.push_back(id);
                }
            }
            if (!lineIds.empty())
                getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::GeoEdge, lineIds);
            if (!faceIds.empty())
                getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::GeoFace, faceIds);
            if (!solidIds.empty())
                getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::GeoSolid, solidIds);
            MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
            getCVM->getPreRenderWidget()->updatePaint();
            });

        assignLayout->addWidget(_listAssignPart, 0, 0, 8, 7);
        assignLayout->addWidget(allBtn, 0, 7);
        assignLayout->addWidget(addBtn, 1, 7);
        assignLayout->addWidget(reduceBtn, 2, 7);
        assignLayout->addWidget(deleteBtn, 3, 7);

        m_paramFold->addRow(inputWidgets[0].widget);
        m_paramFold->addRow(inputWidgets[1].widget);
        m_paramFold->addRow(inputWidgets[2].widget);
        m_paramFold->addRow(maxSize, maxSizeEdit, maxSizeButton);
        m_paramFold->addRow(minSize, minSizeEdit, minSizeButton);
        m_paramFold->addRow(geoFeature, geoFeatureBox);

        m_GeoFold->addRow(_geoLabel, _geoComboBox);
        m_GeoFold->addRow(assignLayout);

        mainLay->addWidget(m_paramFold);
        mainLay->addWidget(m_GeoFold);
        mainLay->addStretch();

        connect(dimSmoothnessRatioEdit, &QLineEdit::editingFinished, [=]() {
            double value = dimSmoothnessRatioEdit->text().toDouble();
            if (value < 1.2)
            {
                mGlobalSignals::getInstance()->outputMessageSig(1, tr("尺寸光滑率输入数值 %1 超过允许的输入范围 [1.2, 1.6]").arg(value), true);
                dimSmoothnessRatioEdit->setText("1.2");
            }
            if (value > 1.6)
            {
                mGlobalSignals::getInstance()->outputMessageSig(1, tr("尺寸光滑率输入数值 %1 超过允许的输入范围 [1.2, 1.6]").arg(value), true);
                dimSmoothnessRatioEdit->setText("1.6");
            }
            _meshPanelData->setDimSmoothnessRatio(dimSmoothnessRatioEdit->text().toDouble());
            });
        connect(curvatureEigenValueEdit, &QLineEdit::editingFinished, [=]() {
            if (curvatureEigenValueEdit->text().toDouble() < 6)
                curvatureEigenValueEdit->setText("6");
            if (curvatureEigenValueEdit->text().toDouble() > 300)
                curvatureEigenValueEdit->setText("300");
            _meshPanelData->setCurvatureEigenValue(curvatureEigenValueEdit->text().toDouble());
            });
        connect(geoProximityValueEdit, &QLineEdit::editingFinished, [=]() {
            if (geoProximityValueEdit->text().toDouble() < 0.5)
                geoProximityValueEdit->setText("0.5");
            if (geoProximityValueEdit->text().toDouble() > 2.0)
                geoProximityValueEdit->setText("2.0");
            _meshPanelData->setGeoProximityValue(geoProximityValueEdit->text().toDouble());
            });
        connect(maxSizeEdit, &QLineEdit::textChanged, [=]() {
            if (maxSizeEdit->text().isEmpty())
                return;
            double maxVal = maxSizeEdit->text().toDouble();
            double minVal = minSizeEdit->text().toDouble();
            if (maxVal < minVal)
                minSizeEdit->setText(MxFunctions::mNumberToString(maxVal));
            _meshPanelData->setMaxSize(maxVal); });
        connect(minSizeEdit, &QLineEdit::textChanged, [=]() {
            if (minSizeEdit->text().isEmpty())
                return;
            double maxVal = maxSizeEdit->text().toDouble();
            double minVal = minSizeEdit->text().toDouble();
            if (minVal > maxVal)
                maxSizeEdit->setText(MxFunctions::mNumberToString(minVal));
            _meshPanelData->setMinSize(minVal); });
        connect(geoFeatureBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
            _meshPanelData->setMeshGeo(index);
            });
        connect(maxSizeButton, &QToolButton::clicked, this, &mAutoAdaptPanel::setMaxSize);
        connect(minSizeButton, &QToolButton::clicked, this, &mAutoAdaptPanel::setMinSize);
    }
    mAutoAdaptPanel::~mAutoAdaptPanel()
    {
    }
    void mAutoAdaptPanel::newMeshPanel()
    {
    }
    void mAutoAdaptPanel::newAutoAdaptPanel()
    {
    }
    void mAutoAdaptPanel::editMeshPanel(int meshId)
    {
        _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(meshId);
        auto a = _meshPanelData->getMeshPanelType();
        updatePanel();
    }
    void mAutoAdaptPanel::getMeshInfoById(const std::vector<int> ids)
    {
        if (ids.size() <= 0)
        {
            return;
        }
        if (ids.size() == 1)
        {
            return;
        }
        _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(ids[ids.size() - 1]);
        bool firstSmoothChecked = _meshPanelData->getSmoothChecked();
        bool firstCurvatureChecked = _meshPanelData->getCurvatureChecked();
        bool firstGeoProimityChecked = _meshPanelData->getGeoProimityChecked();
        double firstSmoothnessRatio = _meshPanelData->getDimSmoothnessRatio();
        double firstCurvatureEigenValue = _meshPanelData->getCurvatureEigenValue();
        double firstGeoProximityValue = _meshPanelData->getGeoProximityValue();
        double firstMaxSize = _meshPanelData->getMaxSize();
        double firstMinSize = _meshPanelData->getMinSize();

        checkBox2->setChecked(firstCurvatureChecked);
        checkBox3->setChecked(firstGeoProimityChecked);
        dimSmoothnessRatioEdit->setText(MxFunctions::mNumberToString(firstSmoothnessRatio));
        curvatureEigenValueEdit->setText(MxFunctions::mNumberToString(firstCurvatureEigenValue));
        geoProximityValueEdit->setText(MxFunctions::mNumberToString(firstGeoProximityValue));
        maxSizeEdit->setText(MxFunctions::mNumberToString(firstMaxSize));
        minSizeEdit->setText(MxFunctions::mNumberToString(firstMinSize));

        //网格形状
        for (auto id : ids)
        {
            _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
            if (_meshPanelData->getMaxSize() != firstMaxSize)
            {
                maxSizeEdit->setText("");
            }
            if (_meshPanelData->getMinSize() != firstMinSize)
            {
                minSizeEdit->setText("");
            }
            _meshPanelData->setSmoothChecked(firstSmoothChecked);
            _meshPanelData->setCurvatureChecked(firstCurvatureChecked);
            _meshPanelData->setGeoProimityChecked(firstGeoProimityChecked);
        }

        _geoComboBox->setDisabled(true);
        _listAssignPart->setDisabled(true);
        allBtn->setDisabled(true);
        addBtn->setDisabled(true);
        reduceBtn->setDisabled(true);
        deleteBtn->setDisabled(true);
        connect(checkBox2, &QCheckBox::toggled, [=](bool checked)
            {curvatureEigenValueEdit->setEnabled(checked);
        for (auto id : ids)
        {
            _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
            _meshPanelData->setCurvatureChecked(checked);
        }
            });
        connect(checkBox3, &QCheckBox::toggled, [=](bool checked)
            {geoProximityValueEdit->setEnabled(checked);
        for (auto id : ids)
        {
            _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
            _meshPanelData->setGeoProimityChecked(checked);
        }
            });
        connect(dimSmoothnessRatioEdit, &QLineEdit::editingFinished, [=]() {
            for (auto id : ids)
            {
                _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                _meshPanelData->setDimSmoothnessRatio(dimSmoothnessRatioEdit->text().toDouble());
            }
            });
        connect(curvatureEigenValueEdit, &QLineEdit::editingFinished, [=]() {
            for (auto id : ids)
            {
                _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                _meshPanelData->setCurvatureEigenValue(curvatureEigenValueEdit->text().toDouble());
            }
            });
        connect(geoProximityValueEdit, &QLineEdit::editingFinished, [=]() {
            for (auto id : ids)
            {
                _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                _meshPanelData->setGeoProximityValue(geoProximityValueEdit->text().toDouble());
            }
            });
        connect(maxSizeEdit, &QLineEdit::textChanged, [=]() {
            if (maxSizeEdit->text().isEmpty())
                return;
            double maxVal = maxSizeEdit->text().toDouble();
            double minVal = minSizeEdit->text().toDouble();
            if (maxVal < minVal)
                minSizeEdit->setText(MxFunctions::mNumberToString(maxVal));
            for (auto id : ids)
            {
                _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                _meshPanelData->setMaxSize(maxVal);
            }
            });
        connect(minSizeEdit, &QLineEdit::textChanged, [=]() {
            if (minSizeEdit->text().isEmpty())
                return;
            double maxVal = maxSizeEdit->text().toDouble();
            double minVal = minSizeEdit->text().toDouble();
            if (minVal > maxVal)
                maxSizeEdit->setText(MxFunctions::mNumberToString(minVal));
            for (auto id : ids)
            {
                _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                _meshPanelData->setMinSize(minVal);
            }
            });

    }
    void mAutoAdaptPanel::updatePanel()
    {
        dimSmoothnessRatioEdit->setEnabled(true);
        checkBox2->setChecked(_meshPanelData->getCurvatureChecked()); curvatureEigenValueEdit->setEnabled(_meshPanelData->getCurvatureChecked());
        checkBox3->setChecked(_meshPanelData->getGeoProimityChecked()); geoProximityValueEdit->setEnabled(_meshPanelData->getGeoProimityChecked());
        dimSmoothnessRatioEdit->setText(MxFunctions::mNumberToString(_meshPanelData->getDimSmoothnessRatio()));
        curvatureEigenValueEdit->setText(MxFunctions::mNumberToString(_meshPanelData->getCurvatureEigenValue()));
        geoProximityValueEdit->setText(MxFunctions::mNumberToString(_meshPanelData->getGeoProximityValue()));
        maxSizeEdit->setText(MxFunctions::mNumberToString(_meshPanelData->getMaxSize()));
        minSizeEdit->setText(MxFunctions::mNumberToString(_meshPanelData->getMinSize()));
        _listAssignPart->clear();
        QStringList tempList = _meshPanelData->_list;
        geoFeatureBox->setCurrentIndex(_meshPanelData->getMeshGeo());
        refreshGeoComboBox();
        for (const QString& s : tempList)
        {
            // 恢复的项补写 UserRole 几何 ID 与维度，保证点击高亮与新添加的项行为一致
            QListWidgetItem* item = new QListWidgetItem(s);
            QVariantList varList;
            varList << s.section('_', -1).toInt();
            item->setData(Qt::UserRole, varList);
            item->setData(Qt::UserRole + 1, getDimByListString(s));
            _listAssignPart->addItem(item);   // 按 cache 顺序追加
        }
        _meshPanelData->_list = tempList;
        maxSizeButton->click();
        minSizeButton->click();


    }
    void mAutoAdaptPanel::slot_createMeshAutoAdapt()
    {
        set<int> lineIds;
        set<int> faceIds;
        set<int> solidIds;
        for (auto str : _meshPanelData->_list)
        {
            QStringList strlist = str.split("_");
            int id = strlist[1].toInt();
            QString type = strlist[0];
            if (type == "几何线")
                lineIds.insert(id);
            else if (type == "几何面")
                faceIds.insert(id);
            else
                solidIds.insert(id);
        }
        MeshParam mesh_param(_meshPanelData->getMaxSize(), _meshPanelData->getMinSize(),
            true, _meshPanelData->getDimSmoothnessRatio(),
            checkBox3->isChecked(), _meshPanelData->getGeoProximityValue(),
            checkBox2->isChecked(), _meshPanelData->getCurvatureEigenValue()

        );


        emit startSlot_createMeshAutoAdapt(1, lineIds, mesh_param);
        emit startSlot_createMeshAutoAdapt(2, faceIds, mesh_param);
        emit startSlot_createMeshAutoAdapt(3, solidIds, mesh_param);
    }



    void mAutoAdaptPanel::setMaxSize()
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
            QString str = MxFunctions::mNumberToString(sum * 5);
            maxSizeEdit->setText(str);
        }
        else
        {
            maxSizeEdit->setText("0");
        }
    }

    void mAutoAdaptPanel::setMinSize()
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

        // 找到最小尺寸
        auto min_size = findMinDouble(size_pair);

        // 获取平均尺寸
        double sum = std::accumulate(size_pair.begin(), size_pair.end(), 0.0,
            [](double acc, const std::pair<int, double>& pair) {
                return acc + pair.second;
            });
        sum /= size_pair.size();

        if (min_size > 0)  // 避免除零错误
        {
            QString str = MxFunctions::mNumberToString(sum / 5);
            minSizeEdit->setText(str);
        }
        else
        {
            minSizeEdit->setText("0");
        }
    }
    void mAutoAdaptPanel::initSize()
    {
        maxSizeButton->click();
        minSizeButton->click();
    }

    void mAutoAdaptPanel::refreshGeoComboBox()
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

        int dim = getDimByGeoFeatureIndex(geoFeatureBox->currentIndex());
        QSet<QString> addedItems;
        std::vector<int> partIDs = meshData->getAllPartIds();
        for (int partId : partIDs)
        {
            QString partName = meshData->getPartNameByPartId(partId);
            if (partName.isEmpty())
                continue;

            std::vector<int> geoIds = MGeoNeutralQuery::geoIdsByPartId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), dim, partId);
            if (geoIds.empty())
                continue;

            if (!addedItems.contains(partName))
            {
                _geoComboBox->addItem(partName);
                addedItems.insert(partName);
            }
        }
        _geoComboBox->setCurrentIndex(0);
    }
}
