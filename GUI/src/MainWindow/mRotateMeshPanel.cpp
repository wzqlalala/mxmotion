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
#include "MXMesh.h"
#include "mxr/view/mPreMeshHighLightRender.h"
#include "mxr/view/HighlightController.h"

#include "mxr/label/mSymbolController.h"
#include "mxr/label/SymbolDescription.h"
#include "PickCommonFunction.h"

// Mesh
#include "mMeshCreate.h"
#include "GlobalMeshMessage.h"

// Mech
#include "mMeshPanel.h"
#include "mRotateMeshPanel.h"

using namespace MechPreData;
using namespace MeshPanelData;
using namespace MxFunctions;
#include <set>
#include "mOtherFunction.h"
Q_DECLARE_METATYPE(std::set<int>)
Q_DECLARE_METATYPE(MeshParam)          // 头文件

namespace MechMainWindow
{

    mRotateMeshPanel::mRotateMeshPanel(QWidget* parent)
    {
        auto* mainLay = new QVBoxLayout(this);

        m_ObjectFold = new FoldSection("对象", this);
        posPick.initWidget("对象");
        posPick.setState(false);
        m_ObjectFold->addRow(posPick.propwid);
        mainLay->addWidget(m_ObjectFold);

        m_paramFold = new  FoldSection("参数", this);

        center = new PanelLabel("旋转中心");
        centerx = new mCalcLineEdit();
        centery = new mCalcLineEdit();
        centerz = new mCalcLineEdit();
        m_centerPickBtn = new QToolButton();
        m_centerPickBtn->setFixedWidth(26);
        m_centerPickBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        m_centerPickBtn->setToolTip("添加拾取节点坐标");

        QHBoxLayout* centerLayout = new QHBoxLayout();
        centerLayout->setMargin(0);
        centerLayout->setSpacing(10);
        centerLayout->addWidget(centerx);
        centerLayout->addWidget(centery);
        centerLayout->addWidget(centerz);
        centerLayout->addWidget(m_centerPickBtn);

        angle = new PanelLabel("角度");
        angleEdit = new QLineEdit("90");
        layer = new PanelLabel("层数");
        layerEdit = new QLineEdit("1");

        vecPick.initWidget("旋转轴", false, true);
        m_paramFold->addRow(center, centerLayout);
        m_paramFold->addRow(vecPick.propwid);
        m_paramFold->addRow(angle, angleEdit);
        m_paramFold->addRow(layer, layerEdit);
        mainLay->addWidget(m_paramFold);

        QFrame* horizontalLine = createHorizontalLine();
        mainLay->addWidget(horizontalLine);
        QPushButton* btn = new QPushButton("应用");
        mainLay->addWidget(btn);

        mainLay->addStretch();

        connect(centerx, &QLineEdit::textChanged, this, &mRotateMeshPanel::centerChanged);
        connect(centery, &QLineEdit::textChanged, this, &mRotateMeshPanel::centerChanged);
        connect(centerz, &QLineEdit::textChanged, this, &mRotateMeshPanel::centerChanged);
        connect(angleEdit, &QLineEdit::textChanged, this, [=]() {
            _meshOptPanelData->setStretchDis(angleEdit->text().toDouble());
            });
        connect(layerEdit, &QLineEdit::textChanged, this, [=]() {
            _meshOptPanelData->setStretchLayers(layerEdit->text().toDouble());
            });
        connect(m_centerPickBtn, &QToolButton::clicked, this, [=]() {
            auto pickMeshIDs = getCVM->getPreRenderWidget()
                ->getSelectionContainer()
                ->getMeshSelectData()
                ->getSelectDataByType(SelectionInfoType::MeshNode);

            if (pickMeshIDs.empty())
                return;

            QVector3D point = getCVM->getModelData()->getMeshData()->getVectorById(pickMeshIDs.back().data_id.node_id);
            centerx->setText(MxFunctions::mNumberToString(point.x()));
            centery->setText(MxFunctions::mNumberToString(point.y()));
            centerz->setText(MxFunctions::mNumberToString(point.z()));
            });
        connect(m_centerPickBtn, &QToolButton::clicked, this, [=]() {
            auto pickMeshIDs = getCVM->getPreRenderWidget()
                ->getSelectionContainer()
                ->getMeshSelectData()
                ->getSelectDataByType(SelectionInfoType::MeshNode);

            if (pickMeshIDs.empty())
                return;

            QVector3D point = getCVM->getModelData()->getMeshData()->getVectorById(pickMeshIDs.back().data_id.node_id);
            centerx->setText(MxFunctions::mNumberToString(point.x()));
            centery->setText(MxFunctions::mNumberToString(point.y()));
            centerz->setText(MxFunctions::mNumberToString(point.z()));
            });
        connect(posPick.btnok, &QToolButton::clicked, this, [=]() {
            // 获得拾取的网格单元ID
            MeshIDs.clear();
            std::vector<MeshSelectInfoData> pickMeshIDs = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);

            // 将MeshSelectInfoData转化为RenderPickResultData
            for (auto ele : pickMeshIDs)
            {
                RenderPickResultData rel = SelectData2ResultData(SelectionInfoType::MeshElement, ele);
                MeshIDs.push_back(rel);
            }
            if (!MeshIDs.empty())
            {
                posPick.setState(true);
                posPick.labelpicked->setText(QString("已选择:%1个单元").arg(MeshIDs.size()));
            }
            else
            {
                posPick.setState(false);
            }
            _meshOptPanelData->setMeshIDs(std::move(MeshIDs));
            });
        connect(posPick.btnclear, &QToolButton::clicked, this, [=]() {
            MeshIDs.clear();
            posPick.setState(false);
            _meshOptPanelData->setMeshIDs(std::move(MeshIDs));
            });
        connect(posPick.btnadd, &QToolButton::clicked, this, [=]() {
            // 获得拾取的网格单元ID，追加到MeshIDs
            std::vector<MeshSelectInfoData> pickMeshIDs = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);

            // 将MeshSelectInfoData转化为RenderPickResultData
            for (auto ele : pickMeshIDs)
            {
                RenderPickResultData rel = SelectData2ResultData(SelectionInfoType::MeshElement, ele);
                pushBackUnique(MeshIDs, rel);
            }
            if (!MeshIDs.empty())
            {
                posPick.setState(true);
                posPick.labelpicked->setText(QString("已选择:%1个单元").arg(MeshIDs.size()));
            }
            else
                posPick.setState(false);
            _meshOptPanelData->setMeshIDs(std::move(MeshIDs));
            });
        connect(vecPick.dirBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=]() {
            _meshOptPanelData->setDirAxial(vecPick.dirBox->currentText());
            // 三点确定展开/折叠后刷新参数折叠块布局，防止内容被挤压
            QTimer::singleShot(0, this, [=]() {
                vecPick.propwid->updateGeometry();
                vecPick.propwid->adjustSize();
                m_paramFold->refreshLayout();
                this->updateGeometry();
                this->adjustSize();
                qDebug() << "[mRotateMeshPanel] dirBox changed to" << vecPick.dirBox->currentText()
                         << "threePointWidget visible=" << vecPick.threePointWidget->isVisible()
                         << "propwid sizeHint=" << vecPick.propwid->sizeHint();
                });
            });
        connect(vecPick.vecEdit, &QLineEdit::textChanged, this, [=]() {
            _meshOptPanelData->setDirVec(vecPick.vecEdit->text());
            });
        connect(btn, &QPushButton::clicked, this, &mRotateMeshPanel::applyRotateStretch);
    }

    void mRotateMeshPanel::applyRotateStretch()
    {
        if (MeshIDs.empty())
        {
            QString msg = QString("未拾取任何单元，请重新拾取！");
            mGlobalSignals::getInstance()->outputMessageSig(WarningMessage, msg, true);
            return;
        }

        // 获得旋转基准点
        SPoint3 basePoint = SPoint3(centerx->text().toDouble(), centery->text().toDouble(), centerz->text().toDouble());
        // 定义方向向量
        SPoint3 dir;
        if (vecPick.dirBox->currentIndex() == 4 && vecPick._hasThreePoint)
        {
            QVector3D p1(vecPick.tpX1->text().toDouble(), vecPick.tpY1->text().toDouble(), vecPick.tpZ1->text().toDouble());
            QVector3D p2(vecPick.tpX2->text().toDouble(), vecPick.tpY2->text().toDouble(), vecPick.tpZ2->text().toDouble());
            QVector3D p3(vecPick.tpX3->text().toDouble(), vecPick.tpY3->text().toDouble(), vecPick.tpZ3->text().toDouble());
            QVector3D v1 = p2 - p1;
            QVector3D v2 = p3 - p1;
            QVector3D normal = QVector3D::crossProduct(v1, v2);
            if (normal.lengthSquared() > 1e-12)
            {
                normal.normalize();
                dir = SPoint3{ normal.x(), normal.y(), normal.z() };
            }
            else
            {
                dir = SPoint3{ 0,0,0 };
            }
        }
        else
        {
            QStringList Dir = vecPick.vecEdit->text().split(',');
            dir = SPoint3{ Dir[0].toDouble(),Dir[1].toDouble(),Dir[2].toDouble() };
        }
        // 对网格进行平移
        GlobalMeshMessage::getInstance()->setMeshData(getCVM->getModelData()->getMeshData());

        meshRotateStretch(MeshIDs, basePoint, dir, angleEdit->text().toDouble(), layerEdit->text().toInt(), true);

        QString partName = getCVM->getModelData()->getMeshData()->getRenamePartNameNoExist("meshPart");
        int partMaxId = getCVM->getModelData()->getMeshData()->getMaxPartID();
        vector<int> partIds = getCVM->getModelData()->getAllPhyID(Physical_Part);
        emit mGlobalSignals::getInstance()->createUpdatePartSig(true, set<int>{partIds.end() - 1, partIds.end()});


        // 清除拾取与高亮
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
    }
    mRotateMeshPanel::~mRotateMeshPanel()
    {

    }

    void mRotateMeshPanel::editMeshPanel(int meshId)
    {
        _meshOptPanelData = getCVM->getModelData()->getManager<mMeshOptPanelManager>(Physical_MeshOptPanel).getMeshPanelData(meshId);
        updatePanel();
    }
    
    void mRotateMeshPanel::updatePanel()
    {
        MeshIDs = _meshOptPanelData->getMeshIDs();
        if (MeshIDs.empty())
        {
            posPick.setState(false);
        }
        else
        {
            posPick.setState(true);
            posPick.labelpicked->setText(QString("已选择:%1个单元").arg(MeshIDs.size()));
        }
        vecPick.dirBox->setCurrentText(_meshOptPanelData->getDirAxial());
        vecPick.vecEdit->setText(_meshOptPanelData->getDirVec());
        //angleEdit->setExpression(MxFunctions::mNumberToString(_meshOptPanelData->getStretchDis()));
        //layerEdit->setExpression(MxFunctions::mNumberToString(_meshOptPanelData->getStretchLayers()));
    }

    void mRotateMeshPanel::centerChanged()
    {
        Point point;
        point.x = centerx->text().toDouble();
        point.y = centery->text().toDouble();
        point.z = centerz->text().toDouble();
        _meshOptPanelData->setStretchCenter(point);
    }
}
