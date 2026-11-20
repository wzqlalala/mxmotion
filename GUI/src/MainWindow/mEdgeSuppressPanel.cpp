
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
//Mesh
#include "MXMesh.h"
#include "mxr/view/mPreMeshHighLightRender.h"
#include "mxr/view/HighlightController.h"

#include "mxr/label/mSymbolController.h"
#include "mxr/label/SymbolDescription.h"
#include "PickCommonFunction.h"

#include "mInfoWindow.h"
#include "mEdgeSuppressPanel.h"
#include"mViewportModelManager.h"
using namespace MechPreData;
using namespace MeshPanelData;
using namespace MxFunctions;
#include <set>

namespace MechMainWindow
{
    mEdgeSuppressPanel::mEdgeSuppressPanel(QWidget* parent)
        : mMeshPanel(parent)
    {
        auto* mainLay = new QVBoxLayout(this);

        // ========== 几何线折叠块 ==========
        m_GeoFold = new FoldSection("几何线", this);
        auto* geoLay = new QHBoxLayout;
        _geoComboBox = new QComboBox();
        _geoComboBox->addItem("请选择几何线集合");
        _geoComboBox->setItemData(0, 0, Qt::UserRole - 1); // 禁止选中
        _geoComboBox->setCurrentIndex(0);                 // 默认不选中任何项
        connect(_geoComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
            QString partSetName = _geoComboBox->currentText();
            _geoComboBox->setCurrentIndex(0);
            });

        //已经赋予的部件列表
        QGridLayout* assignLayout = new QGridLayout();
        _listAssignPart = new QListWidget();
        _listAssignPart->setSelectionMode(QAbstractItemView::ExtendedSelection);
        addBtn = new QToolButton(); addBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png")); addBtn->setToolTip("增加几何线");
        reduceBtn = new QToolButton(); reduceBtn->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png")); reduceBtn->setToolTip("减少几何线");
        deleteBtn = new QToolButton(); deleteBtn->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png")); deleteBtn->setToolTip("清空几何线");
        connect(addBtn, &QToolButton::clicked, [=]() {
            vector<int> lineids = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
            QSet<QString> existSet;
            for (int i = 0; i < _listAssignPart->count(); ++i)
                existSet.insert(_listAssignPart->item(i)->text());
            for (int id : lineids)                    // set 自动升序
            {
                QString txt = QString("几何线_%1").arg(id);
                if (!existSet.contains(txt))
                    _listAssignPart->addItem(txt);
            }
            _meshPanelData->_list.clear();
            for (int i = 0; i < _listAssignPart->count(); ++i)
                _meshPanelData->_list << _listAssignPart->item(i)->text();
            emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
            });
        connect(reduceBtn, &QToolButton::clicked, [=]() {
            QList<QListWidgetItem*> items = _listAssignPart->selectedItems();
            if (items.isEmpty())
                return;
            vector<int> ids;
            for (int i = items.count() - 1; i >= 0; --i)
            {
                ids.push_back(items[i]->text().split("_")[1].toInt());
                delete _listAssignPart->takeItem(_listAssignPart->row(items[i]));
            }
            _meshPanelData->_list.clear();
            for (int i = 0; i < _listAssignPart->count(); ++i)
            {
                _meshPanelData->_list << _listAssignPart->item(i)->text();
            }
            //update父节点状态
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
            vector<int> ids;
            for (auto item : items)
            {
                QStringList strlist = item->text().split("_");
                int id = strlist[1].toInt();
                ids.push_back(id);
            }
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::GeoEdge, ids);
            MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
            getCVM->getPreRenderWidget()->updatePaint();
            });
        assignLayout->addWidget(_listAssignPart, 0, 0, 8, 7);
        assignLayout->addWidget(addBtn, 0, 7);
        assignLayout->addWidget(reduceBtn, 1, 7);
        assignLayout->addWidget(deleteBtn, 2, 7);

        m_GeoFold->addRow(new PanelLabel("几何线集合"), _geoComboBox);
        m_GeoFold->addRow(assignLayout);
        m_foldWidgets.append(m_GeoFold);

        mainLay->addWidget(m_GeoFold);
        mainLay->addStretch();
    }

    mEdgeSuppressPanel::~mEdgeSuppressPanel()
    {
    }

    void mEdgeSuppressPanel::newMeshPanel()
    {
    }

    void mEdgeSuppressPanel::editMeshPanel(int meshId)
    {
        _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(meshId);
        auto a = _meshPanelData->getMeshPanelType();
        updatePanel();
    }

    void mEdgeSuppressPanel::getMeshInfoById(const std::vector<int> ids)
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
        //禁用编辑
        _geoComboBox->setDisabled(true);
        _listAssignPart->setDisabled(true);
        addBtn->setDisabled(true);
        reduceBtn->setDisabled(true);
        deleteBtn->setDisabled(true);
    }

    void mEdgeSuppressPanel::updatePanel()
    {
        _listAssignPart->clear();
        for (const QString& s : _meshPanelData->_list)
            _listAssignPart->addItem(s);          // 按 cache 顺序追加
    }

    void mEdgeSuppressPanel::initSize()
    {
    }
}
