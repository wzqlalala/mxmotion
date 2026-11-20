
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
//#include "MXMeshAPI.h"
#include "mMeshData.h"
#include "MXMesh.h"
#include "mxr/view/mPreMeshHighLightRender.h"
#include "mxr/view/HighlightController.h"

#include "mxr/label/mSymbolController.h"
#include "mxr/label/SymbolDescription.h"
#include "PickCommonFunction.h"

#include "mInfoWindow.h"
#include "mBoundaryMeshPanel.h"
using namespace MechPreData;
using namespace MeshPanelData;
using namespace MxFunctions;
#include <set>
#include "mOtherFunction.h"
Q_DECLARE_METATYPE(std::set<int>)
Q_DECLARE_METATYPE(MeshParam)          // 头文件
namespace MechMainWindow
{
    mBoundaryMeshPanel::mBoundaryMeshPanel(QWidget* parent)
        : mMeshPanel(parent)
    {
        auto* mainLay = new QVBoxLayout(this);

        m_rangeFold = new FoldSection("网格定义", this);
        auto* vLay = new QVBoxLayout;

        auto* rangeLay2 = new QHBoxLayout;

        QLabel* wall = new PanelLabel("壁面几何面列表");
        QLabel* wallSet=new PanelLabel("几何面集合");
        QComboBox*wallSetBox = new QComboBox();
        wallSetBox->addItems({ "请选择几何面集合" });
        wallSetBox->setItemData(0, 0, Qt::UserRole - 1); // 禁止选中
        wallSetBox->setCurrentIndex(0);                 // 默认不选中任何项

        QListWidget* _wallListAssignPart = new QListWidget();
        _wallListAssignPart->setSelectionMode(QAbstractItemView::ExtendedSelection);

        QToolButton* wallAllBtn = new QToolButton(); wallAllBtn->setIcon(QIcon(":/mMainWidgets/Icon/xuanzequanbu.png")); wallAllBtn->setToolTip("全部几何面");
        QToolButton* wallAddBtn = new QToolButton(); wallAddBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png")); wallAddBtn->setToolTip("增加几何面");
        QToolButton* wallReduceBtn = new QToolButton(); wallReduceBtn->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png")); wallReduceBtn->setToolTip("减少几何面");
        QToolButton* wallDeleteBtn = new QToolButton(); wallDeleteBtn->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png")); wallDeleteBtn->setToolTip("清空几何面");

        QGridLayout* wallAssignLayout = new QGridLayout();
        wallAssignLayout->setSpacing(10);
        wallAssignLayout->addWidget(_wallListAssignPart, 0, 0, 8, 7);
        wallAssignLayout->addWidget(wallAllBtn, 0, 7);
        wallAssignLayout->addWidget(wallAddBtn, 1, 7);
        wallAssignLayout->addWidget(wallReduceBtn, 2, 7);
        wallAssignLayout->addWidget(wallDeleteBtn, 3, 7);

        m_rangeFold->addRow(wall);
        m_rangeFold->addRow(wallSet, wallSetBox);
        m_rangeFold->addRow(wallAssignLayout);

        QLabel* nonWall = new QLabel("非壁面几何面列表");
        QLabel* nonWallSet = new PanelLabel("几何面集合");
        QComboBox* nonWallSetBox = new QComboBox();
        nonWallSetBox->addItems({ "请选择几何面集合" });
        nonWallSetBox->setItemData(0, 0, Qt::UserRole - 1); // 禁止选中
        nonWallSetBox->setCurrentIndex(0);                 // 默认不选中任何项

        QListWidget* _nonWallListAssignPart = new QListWidget();
        _nonWallListAssignPart->setSelectionMode(QAbstractItemView::ExtendedSelection);

        QToolButton* nonWallAllBtn = new QToolButton(); nonWallAllBtn->setIcon(QIcon(":/mMainWidgets/Icon/xuanzequanbu.png")); nonWallAllBtn->setToolTip("全部几何面");
        QToolButton* nonWallAddBtn = new QToolButton(); nonWallAddBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png")); nonWallAddBtn->setToolTip("增加几何面");
        QToolButton* nonWallReduceBtn = new QToolButton(); nonWallReduceBtn->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png")); nonWallReduceBtn->setToolTip("减少几何面");
        QToolButton* nonWallDeleteBtn = new QToolButton(); nonWallDeleteBtn->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png")); nonWallDeleteBtn->setToolTip("清空几何面");

        QGridLayout* nonWallAssignLayout = new QGridLayout();
        nonWallAssignLayout->setSpacing(10);
        nonWallAssignLayout->addWidget(_nonWallListAssignPart, 0, 0, 8, 7);
        nonWallAssignLayout->addWidget(nonWallAllBtn, 0, 7);
        nonWallAssignLayout->addWidget(nonWallAddBtn, 1, 7);
        nonWallAssignLayout->addWidget(nonWallReduceBtn, 2, 7);
        nonWallAssignLayout->addWidget(nonWallDeleteBtn, 3, 7);


        m_rangeFold->addRow(nonWall);
        m_rangeFold->addRow(nonWallSet, nonWallSetBox);
        m_rangeFold->addRow(nonWallAssignLayout);

        FoldSection* m_paramFold = new FoldSection("网格参数设置");
        QLabel* thickness = new PanelLabel("初始层厚");
        mCalcLineEdit* thicknessEdit = new mCalcLineEdit("2");
        QLabel* growthRate = new PanelLabel("增长率");
        mCalcLineEdit* growthRateEdit = new mCalcLineEdit("2");
        QLabel* layer = new PanelLabel("层数");
        mCalcLineEdit* layerEdit = new mCalcLineEdit("2");
        m_paramFold->addRow(thickness,thicknessEdit);
        m_paramFold->addRow(growthRate, growthRateEdit);
        m_paramFold->addRow(layer, layerEdit);
        mainLay->addWidget(m_rangeFold);
        mainLay->addWidget(m_paramFold);
        mainLay->addStretch();


        connect(wallAllBtn, &QToolButton::clicked, [=]() {
            std::set<int> meshGeoId = getGEOServer.geoCalculate().getAllGeoID(2);
            QSet<QString> existSet;
            for (int i = 0; i < _wallListAssignPart->count(); ++i)
                existSet.insert(_wallListAssignPart->item(i)->text());
            for (int id : meshGeoId)                    // set 自动升序
            {
                QString txt = QString("几何面_%1").arg(id);
                if (!existSet.contains(txt))
                    _wallListAssignPart->addItem(txt);
            }
            _meshPanelData->_list.clear();
            for (int i = 0; i < _wallListAssignPart->count(); ++i)
                _meshPanelData->_list << _wallListAssignPart->item(i)->text();
            emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
            });
        connect(wallAddBtn, &QToolButton::clicked, [=]() {
            vector<int> faceids = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
            QSet<QString> existSet;
            for (int i = 0; i < _wallListAssignPart->count(); ++i)
                existSet.insert(_wallListAssignPart->item(i)->text());
            for (int id : faceids)                    // set 自动升序
            {
                QString txt = QString("几何面_%1").arg(id);
                if (!existSet.contains(txt))
                    _wallListAssignPart->addItem(txt);
            }
            _meshPanelData->_list.clear();
            for (int i = 0; i < _wallListAssignPart->count(); ++i)
                _meshPanelData->_list << _wallListAssignPart->item(i)->text();
            emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
            });
        connect(wallReduceBtn, &QToolButton::clicked, [=]() {
            QList<QListWidgetItem*> items = _wallListAssignPart->selectedItems();
            if (items.isEmpty())
                return;
            vector<int> ids;
            for (int i = items.count() - 1; i >= 0; --i)
            {
                ids.push_back(items[i]->text().split("_")[1].toInt());
                delete _wallListAssignPart->takeItem(_wallListAssignPart->row(items[i]));
            }
            _meshPanelData->_list.clear();
            for (int i = 0; i < _wallListAssignPart->count(); ++i)
            {
                _meshPanelData->_list << _wallListAssignPart->item(i)->text();
            }
            /*getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(mxr::SelectionOperate::CLEAR, mxr::SelectionInfoType::GeoFace, ids);
            MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
            getCVM->getPreRenderWidget()->updatePaint();*/
            emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
            });
        connect(wallDeleteBtn, &QToolButton::clicked, [=]() {
            /*  getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->ClearAll();
              getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightGeoModel();
              getCVM->getPreRenderWidget()->updatePaint();*/
            _wallListAssignPart->clear();
            _meshPanelData->_list.clear();
            emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();

            });
        connect(_wallListAssignPart, &QListWidget::itemClicked, [=](QListWidgetItem* item) {
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->ClearAll();
            getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightGeoModel();
            auto items = _wallListAssignPart->selectedItems();
            vector<int> ids;
            for (auto item : items)
            {
                QStringList strlist = item->text().split("_");
                int id = strlist[1].toInt();
                ids.push_back(id);
            }
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::GeoFace, ids);
            MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
            getCVM->getPreRenderWidget()->updatePaint();
            });
     
    
  
    }

    mBoundaryMeshPanel::~mBoundaryMeshPanel()
    {

    }
    void mBoundaryMeshPanel::newMeshPanel()
    {

    }
    void mBoundaryMeshPanel::editMeshPanel(int meshId)
    {
        _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(meshId);
        auto a = _meshPanelData->getMeshPanelType();
        updatePanel();

    }
    void mBoundaryMeshPanel::getMeshInfoById(const std::vector<int> ids)
    {
        
    }
    void mBoundaryMeshPanel::updatePanel()
    {
        ////网格形状
        //QString shape = _meshPanelData->getMeshShape();
        //_meshShapeBox->setCurrentText(shape);
        ////网格尺寸
        //double size = _meshPanelData->getMeshSize();
        //_meshSizeEdit->setText(MxFunctions::mNumberToString(size));
        ////实体集合
        //_wallListAssignPart->clear();
        //for (const QString& s : _meshPanelData->_list)
        //    _wallListAssignPart->addItem(s);          // 按 cache 顺序追加


        ////阶次
        //QString order = _meshPanelData->getMeshOrder();
        //_orderComboBox->setCurrentText(order);
    }
    void mBoundaryMeshPanel::setMeshSize()
    {

    }
    void mBoundaryMeshPanel::initSize()
    {

    }
}
