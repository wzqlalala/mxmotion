
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
#include <QVector3D>
#include <QTimer>

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

#include "mInfoWindow.h"
#include "mStretchPanel.h"
#include "GlobalMeshMessage.h"
#include "mMeshCreate.h"
using namespace MechPreData;
using namespace MeshPanelData;
using namespace MxFunctions;
#include <set>
#include "mOtherFunction.h"
Q_DECLARE_METATYPE(std::set<int>)
Q_DECLARE_METATYPE(MeshParam)          // 头文件

namespace MechMainWindow
{

    mStretchPanel::mStretchPanel(QWidget* parent)
        : mMeshPanel(parent)
    {
        auto* mainLay = new QVBoxLayout(this);

        m_ObjectFold = new FoldSection("对象", this);
        posPick.initWidget("对象");
        posPick.setState(false);
        m_ObjectFold->addRow(posPick.propwid);
        mainLay->addWidget(m_ObjectFold);

        m_paramFold = new FoldSection("参数", this);

        QLabel* method = new PanelLabel("拉伸方式");
        methodBox = new QComboBox(); methodBox->addItems({ "指定方向和距离","沿曲线" });

        distance = new PanelLabel("拉伸距离");
        distanceEdit = new mCalcLineEdit("1");
        layers = new PanelLabel("拉伸层数");
        layersEdit = new mCalcLineEdit("1");
        vecPick.initWidget("拉伸方向", false, true);

        curvePick.initWidget("曲线");
        curvePick.setState(false);
        curvePick.btnok->hide();
        QLabel* discretization = new PanelLabel("离散化方法");
        discretizationBox = new QComboBox(); discretizationBox->addItems({ "均匀","双曲正切分布" });
        QLabel* startSize = new PanelLabel("起始尺寸");
        startSizeEdit = new mCalcLineEdit("2");

        m_paramFold->addRow(method, methodBox);
        m_paramFold->addRow(vecPick.propwid);
        m_paramFold->addRow(distance, distanceEdit);
        m_paramFold->addRow(layers, layersEdit);
        m_paramFold->addRow(curvePick.propwid);
        m_paramFold->addRow(discretization, discretizationBox);
        m_paramFold->addRow(startSize, startSizeEdit);
        mainLay->addWidget(m_paramFold);

        QFrame* horizontalLine = createHorizontalLine();
        mainLay->addWidget(horizontalLine);


        QPushButton* btn = new QPushButton("应用");
        mainLay->addWidget(btn);

        mainLay->addStretch();

        curvePick.propwid->hide();
        discretization->hide();
        discretizationBox->hide();
        startSize->hide();
        startSizeEdit->hide();

        // 辅助函数：根据当前方向类型更新拾取模式
        auto updatePickModeForStretchDir = [=]() {
            if (methodBox->currentIndex() != 0)
                return;
            if (vecPick.dirBox->currentIndex() == 4 && vecPick._hasThreePoint)
                MechCommon::mMechGlobalSignals::getInstance()->activePickSig(5); // 节点拾取，用于三点确定
            else
                MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8); // 默认网格单元拾取
            };

        connect(distanceEdit, &QLineEdit::textChanged, this, [=]() {
            _meshOptPanelData->setStretchDis(distanceEdit->text().toDouble());
            });
        connect(layersEdit, &QLineEdit::textChanged, this, [=]() {
            _meshOptPanelData->setStretchLayers(layersEdit->text().toDouble());
            });
        connect(startSizeEdit, &QLineEdit::textChanged, this, [=]() {
            _meshOptPanelData->setStretchStartSize(startSizeEdit->text().toDouble());
            });
        connect(discretizationBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=]() {
            _meshOptPanelData->setStretchDiscMethod(discretizationBox->currentText());
            if (discretizationBox->currentIndex() == 0)
            {
                startSize->hide();
                startSizeEdit->hide();
            }
            else
            {
                startSize->show();
                startSizeEdit->show();
            }
            m_paramFold->refreshLayout();
            });
        connect(methodBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=]() {
            _meshOptPanelData->setStretchMethod(methodBox->currentText());
            if (methodBox->currentIndex() == 0)
            {
                updatePickModeForStretchDir();
                vecPick.propwid->show();
                distance->show(); distanceEdit->show();
                curvePick.propwid->hide();
                discretization->hide();
                discretizationBox->hide();
                startSize->hide();
                startSizeEdit->hide();
            }
            else
            {
                MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
                if (discretizationBox->currentIndex() == 0)
                {
                    startSize->hide();
                    startSizeEdit->hide();
                }
                else
                {
                    startSize->show();
                    startSizeEdit->show();
                }
                vecPick.propwid->hide();
                distance->hide(); distanceEdit->hide();
                curvePick.propwid->show();
                discretization->show();
                discretizationBox->show();
            }
            m_paramFold->refreshLayout();
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
                posPick.setState(false);
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
        connect(curvePick.btnok, &QToolButton::clicked, this, [=]() {
            edgeIds = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
            if (edgeIds.size() != 1)
                return;
            if (!edgeIds.empty())
            {
                curvePick.setState(true);
                curvePick.labelpicked->setText(QString("已选择:几何线%1").arg(edgeIds[0]));
            }
            else
                curvePick.setState(false);
            _meshOptPanelData->setStretchCurveIDs(edgeIds);
            });
        connect(curvePick.btnadd, &QPushButton::clicked, this, [=]() {
            auto newEdgeIds = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
            for (int eid : newEdgeIds)
            {
                if (std::find(edgeIds.begin(), edgeIds.end(), eid) == edgeIds.end())
                    edgeIds.push_back(eid);
            }
            if (!edgeIds.empty())
            {
                curvePick.setState(true);
                curvePick.labelpicked->setText(QString("已选择:几何线:%1").arg(edgeIds.size()));
            }
            else
            {
                curvePick.setState(false);
            }
            _meshOptPanelData->setStretchCurveIDs(edgeIds);
            });
        connect(curvePick.btnclear, &QToolButton::clicked, this, [=]() {
            edgeIds.clear();
            curvePick.setState(false);
            });
        connect(btn, &QPushButton::clicked, this, &mStretchPanel::applyStretch);
        connect(vecPick.dirBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=]() {
            _meshOptPanelData->setDirAxial(vecPick.dirBox->currentText());
            updatePickModeForStretchDir();
            // 三点确定展开/折叠后刷新参数折叠块布局，防止内容被挤压
            QTimer::singleShot(0, this, [=]() {
                vecPick.propwid->updateGeometry();
                vecPick.propwid->adjustSize();
                m_paramFold->refreshLayout();
                this->updateGeometry();
                this->adjustSize();
                });
            });
        connect(vecPick.vecEdit, &QLineEdit::textChanged, this, [=]() {
            _meshOptPanelData->setDirVec(vecPick.vecEdit->text());
            });
    }

    void mStretchPanel::applyStretch()
    {
        {
            if (MeshIDs.empty())
                return;
            // 获得拾取的网格单元ID
            std::vector<MeshSelectInfoData> pickMeshIDs = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);

            //方向和距离
            if (methodBox->currentIndex() == 0)
            {
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

                meshStretch(MeshIDs, dir, distanceEdit->text().toDouble(), layersEdit->text().toInt(), true);
            }
            //曲线
            else if (methodBox->currentIndex() == 1)
            {
                if (edgeIds.empty())
                    return;
                //曲线Id
                int edgeId = edgeIds[0];
                //拉伸层数
                int layers = layersEdit->text().toInt();
                //类型
                QString type = discretizationBox->currentText();
                //起始尺寸
                double startSize = startSizeEdit->text().toDouble();

                if (type == "均匀")
                {
                    meshStretchAlongCurve(MeshIDs, edgeIds, StretchEdgeDiscreteMode::UNIFORM, startSize, layersEdit->text().toInt(), true);
                }
                else if (type == "双曲正切分布")
                {
                    meshStretchAlongCurve(MeshIDs, edgeIds, StretchEdgeDiscreteMode::TANH, startSize, layersEdit->text().toInt(), true);
                }
            }
            QString partName = getCVM->getModelData()->getMeshData()->getRenamePartNameNoExist("meshPart");
            int partMaxId = getCVM->getModelData()->getMeshData()->getMaxPartID();
            vector<int> partIds = getCVM->getModelData()->getAllPhyID(Physical_Part);
            emit mGlobalSignals::getInstance()->createUpdatePartSig(true, set<int>{partIds.end() - 1, partIds.end()});


            // 清除拾取与高亮
            getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
            getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
        }
    }

    mStretchPanel::~mStretchPanel()
    {

    }
    void mStretchPanel::newMeshPanel()
    {

    }
    void mStretchPanel::editMeshPanel(int meshId)
    {
        _meshOptPanelData = getCVM->getModelData()->getManager<mMeshOptPanelManager>(Physical_MeshOptPanel).getMeshPanelData(meshId);
        auto a = _meshOptPanelData->getMeshPanelType();
        updatePanel();
    }
    void mStretchPanel::getMeshInfoById(const std::vector<int> ids)
    {

    }
    void mStretchPanel::updatePanel()
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
        edgeIds = _meshOptPanelData->getStretchCurveIDs();
        if (edgeIds.empty())
        {
            curvePick.setState(false);
        }
        else
        {
            curvePick.setState(true);
            curvePick.labelpicked->setText(QString("已选择:曲线:%1").arg(edgeIds[0]));
        }
        vecPick.dirBox->setCurrentText(_meshOptPanelData->getDirAxial());
        vecPick.vecEdit->setText(_meshOptPanelData->getDirVec());
        methodBox->setCurrentText(_meshOptPanelData->getStretchMethod());
        distanceEdit->setExpression(MxFunctions::mNumberToString(_meshOptPanelData->getStretchDis()));
        layersEdit->setExpression(MxFunctions::mNumberToString(_meshOptPanelData->getStretchLayers()));
        startSizeEdit->setExpression(MxFunctions::mNumberToString(_meshOptPanelData->getStretchStartSize()));
        discretizationBox->setCurrentText(_meshOptPanelData->getStretchDiscMethod());
    }

    void mStretchPanel::setMeshSize()
    {

    }
    void mStretchPanel::initSize()
    {

    }
}
