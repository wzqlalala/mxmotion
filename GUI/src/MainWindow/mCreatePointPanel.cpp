
#include "mMeshPanel.h"
#include "mAppConfiguration.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mCalcLineEdit.h"
#include "mGlobalSignals.h"
#include "mOtherFunction.h"

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

#include "mInfoWindow.h"
#include "mCreatePointPanel.h"
#include "mDisplayMenuFunction.h"
using namespace MechPreData;
using namespace MeshPanelData;
using namespace MxFunctions;
//【镜像六件套摘除·连带】旧 mGeoModelData1.h 曾泄漏文件级 using namespace mxr;，
//镜像删除后本文件里 MeshSelectInfoData 等 mxr 类型失去可见性，就地补齐
using namespace mxr;
#include <set>
Q_DECLARE_METATYPE(std::set<int>)
Q_DECLARE_METATYPE(MeshParam)          // 头文件

namespace MechMainWindow
{

    mCreatePointPanel::mCreatePointPanel(QWidget* parent)
    {
        
        auto* mainLay = new QVBoxLayout(this);
        
        m_rangeFold = new FoldSection("创建方式", this);
        _meshControlLabel = new PanelLabel("方式");
        _meshControlBox = new QComboBox();
        _meshControlBox->addItems({ "输入坐标" , "基于所选对象平均","基于所选点偏移" });
        auto* rangeLay = new QHBoxLayout;
        m_rangeFold->addRow(_meshControlLabel, _meshControlBox);
        m_rangeFold->addRow(rangeLay);
        mainLay->addWidget(m_rangeFold);
        // ========== 参数折叠块 ==========
        m_ParamFold = new FoldSection("参数", this);
        QLabel* xlabel = new PanelLabel("X");
        QLabel* ylabel = new PanelLabel("Y");
        QLabel* zlabel = new PanelLabel("Z");
        xEdit = new QLineEdit("0");
        yEdit = new QLineEdit("0");
        zEdit = new QLineEdit("0");

        posPick.initWidget("对象");
        posPick.setState(false);
        posPick1.initWidget("选择点");
        posPick1.setState(false);
        vecPick.initWidget("偏移方向");
        QLabel* coorlabel= new PanelLabel("节点坐标");
        coorEdit = new QLineEdit("0,0,0"); coorEdit->setDisabled(true);

        QLabel* dislabel = new PanelLabel("偏移距离");
        disEdit = new QLineEdit();

        stackWidget = new QStackedWidget;
        QWidget* widget1 = new QWidget;
        QWidget* widget2 = new QWidget;
        QWidget* widget3 = new QWidget;
        stackWidget->addWidget(widget1);
        stackWidget->addWidget(widget2);
        stackWidget->addWidget(widget3);
        // 让 QStackedWidget 根据当前页面调整大小
        stackWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

        // 关键：安装事件过滤器，切换时调整
        connect(stackWidget, &QStackedWidget::currentChanged, this, [=](int index) {
            QWidget* current = stackWidget->widget(index);
            if (current) {
                stackWidget->setFixedHeight(current->sizeHint().height());
                // 或
                stackWidget->adjustSize();
                m_ParamFold->refreshLayout();
            }
            });
        QFormLayout* m_formLayout1 = new QFormLayout(widget1);
        QFormLayout* m_formLayout2 = new QFormLayout(widget2);
        QFormLayout* m_formLayout3 = new QFormLayout(widget3);
        m_formLayout1->setContentsMargins(0,0,0,0);
        m_formLayout2->setContentsMargins(0,0,0,0);
        m_formLayout3->setContentsMargins(0,0,0,0);
        m_formLayout1->setSpacing(10);
        m_formLayout2->setSpacing(10);
        m_formLayout3->setSpacing(10);

        m_formLayout1->addRow(xlabel, xEdit);
        m_formLayout1->addRow(ylabel, yEdit);
        m_formLayout1->addRow(zlabel, zEdit);
        m_formLayout2->addRow(posPick.propwid);
        m_formLayout2->addRow(coorlabel, coorEdit);
        m_formLayout3->addRow(posPick1.propwid);
        m_formLayout3->addRow(vecPick.propwid);
        m_formLayout3->addRow(dislabel, disEdit);
        m_ParamFold->addRow(stackWidget);
        m_foldWidgets.append(m_ParamFold);


        QFrame* horizontalLine = createHorizontalLine();
        QPushButton* btn = new QPushButton("创建");
        QPushButton* pickBtn = new QPushButton("获取节点坐标");
        pickBtn->setToolTip("将最新拾取的节点坐标填入X/Y/Z输入框");
        QHBoxLayout* h = new QHBoxLayout();
        h->addStretch(); h->addWidget(pickBtn); h->addWidget(btn);
        mainLay->addWidget(m_ParamFold);
        mainLay->addWidget(horizontalLine);
        mainLay->addLayout(h);
        mainLay->addStretch();
        stackWidget->setFixedHeight(widget1->sizeHint().height());
        // 或
        stackWidget->adjustSize();
        connect(_meshControlBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index)
            {
                if (index == 0)
                {
                    stackWidget->setCurrentIndex(0);
                }
                else if (index == 1)
                {
                    stackWidget->setCurrentIndex(1);
                }
                else if (index == 2)
                {
                    stackWidget->setCurrentIndex(2);
                }
            }
        );
        connect(posPick.btnok, &QToolButton::clicked, this, [=]() {
            // 获得拾取的网格单元ID             
            vector<MeshSelectInfoData> res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
            // 将MeshSelectInfoData转化为RenderPickResultData

            nodes = res;
            posPick.setState(true);
            posPick.labelpicked->setText(QString("已选择:%1个节点").arg(nodes.size()));
            updateNodePos();

            });
        connect(posPick.btnadd, &QToolButton::clicked, this, [=]() {
            // 获得拾取的网格单元ID             
            vector<MeshSelectInfoData> res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
            // 将MeshSelectInfoData转化为RenderPickResultData
            for (auto ele:res)
            {
                pushBackUnique(nodes, ele);
            }
            posPick.setState(true);
            posPick.labelpicked->setText(QString("已选择:%1个节点").arg(nodes.size()));
            updateNodePos();
            });
        connect(posPick.btnclear, &QToolButton::clicked, this, [=]() {
            coorEdit->setText("0,0,0");
            nodes.clear();
            posPick.setState(false);
            });
        connect(posPick1.btnok, &QToolButton::clicked, this, [=]() {
            // 获得拾取的网格单元ID             
            vector<MeshSelectInfoData> res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
            // 将MeshSelectInfoData转化为RenderPickResultData
            nodes = res;
            posPick1.setState(true);
            posPick1.labelpicked->setText(QString("已选择:%1个节点").arg(nodes.size()));
            
                      
            });
        connect(posPick1.btnadd, &QToolButton::clicked, this, [=]() {
            // 获得拾取的网格单元ID             
            vector<MeshSelectInfoData> res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
            // 将MeshSelectInfoData转化为RenderPickResultData
            for (auto ele : res)
            {
                pushBackUnique(nodes, ele);
            }
            posPick1.setState(true);
            posPick1.labelpicked->setText(QString("已选择:%1个节点").arg(nodes.size()));
            });
        connect(posPick1.btnclear, &QToolButton::clicked, this, [=]() {
            nodes.clear();
            posPick1.setState(false);
            });
        connect(btn, &QPushButton::clicked, this, &mCreatePointPanel::apply);

        // 将最新拾取的节点坐标填入X/Y/Z输入框
        connect(pickBtn, &QPushButton::clicked, this, [=]() {
            const auto& res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
            if (res.empty())
            {
                mGlobalSignals::getInstance()->outputMessageSig(WarningMessage, tr("请先在视口中拾取一个节点"), true);
                return;
            }
            QVector3D coord = getCVM->getModelData()->getMeshData()->getVectorById(res.back().data_id.node_id);
            xEdit->setText(MxFunctions::mNumberToString(coord.x()));
            yEdit->setText(MxFunctions::mNumberToString(coord.y()));
            zEdit->setText(MxFunctions::mNumberToString(coord.z()));
            });

    }

    mCreatePointPanel::~mCreatePointPanel()
    {
    }

    void mCreatePointPanel::newMeshPanel()
    {
    }
    void mCreatePointPanel::editMeshPanel(int meshId)
    {
        _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(meshId);
        //auto a = _meshPanelData->getMeshPanelType();
        updatePanel();
    }
    void mCreatePointPanel::getMeshInfoById(const std::vector<int> ids)
    {

    }
    void mCreatePointPanel::updatePanel()
    {



    }
    QVector3D calculateOffsetPoint(
        const QVector3D& basePoint,      // 基准点 (x0, y0, z0)
        const SPoint3& direction,      // 偏移方向 (x, y, z)
        double distance)                 // 偏移距离 d
    {
        // 1. 计算方向向量的长度
        double len = std::sqrt(direction.x() * direction.x() +
            direction.y() * direction.y() +
            direction.z() * direction.z());

        // 2. 避免除零
        if (len < 1e-10) {
            return basePoint;  // 方向为零向量，返回原
        }

        // 3. 单位化方向向量并乘以距离
        QVector3D offset(
            direction.x() / len * distance,
            direction.y() / len * distance,
            direction.z() / len * distance
        );

        // 4. 计算新点
        return basePoint + offset;
    }


    void mCreatePointPanel::apply()
    {
        if (getCVM == nullptr)
        {
            return;
        }
        std::vector<QVector3D> coordVec;
        if (_meshControlBox->currentIndex() == 0)
        {
            coordVec.push_back({ xEdit->text().toFloat(), yEdit->text().toFloat(), zEdit->text().toFloat() }) ;
        }
        else if (_meshControlBox->currentIndex() == 1)
        {
            // 按逗号分割
            QStringList vec = coorEdit->text().split(',');

            if (vec.size() != 3) {
                return;
            }

            double x = vec[0].toDouble();
            double y = vec[1].toDouble();
            double z = vec[2].toDouble();

            coordVec.push_back( QVector3D(x, y, z));
        }
        else
        {
            if (nodes.empty())
            {
                return;
            }
            QVector3D vertex;
            auto meshData = getCVM->getModelData()->getMeshData();
            auto elementData = getCVM->getModelData()->getElementData();
            //偏移方向
            QStringList dirstr = vecPick.vecEdit->text().split(',');
            if (dirstr.size() != 3)
                return;
            SPoint3 dir = SPoint3{ dirstr[0].toDouble(),dirstr[1].toDouble(),dirstr[2].toDouble() };
            if (disEdit->text().isEmpty())
                return;
            double d = disEdit->text().toDouble();                       // 偏移距离50

            auto getNodeVertexByID = [&](mMeshData* meshData, int id, QVector3D& vertex3D) {
                double* vertex = meshData->_nodedata.xyz + meshData->_nodedata.nodeidmap[id] * 3;
                vertex3D = QVector3D(vertex[0], vertex[1], vertex[2]);
                };
            for (int i=0;i<nodes.size();i++)
            {
                getNodeVertexByID(meshData, nodes[i].data_id.element_id, vertex);
                coordVec.push_back(calculateOffsetPoint(vertex, dir, d));
            }
         
        }
        //创建节点
        for (auto coord : coordVec)
        {
            getCVM->getModelData()->getMeshData()->createNode(coord);
        }

        mGlobalSignals::getInstance()->outputMessageSig("成功创建节点！");
        getCVM->getPreRenderWidget()->updatePaint();
        getCVM->updateAllModelOperate(AppendOnePart);

        if (getCVM->getPreRenderWidget())
        {
            auto meshData = getCVM->getModelData()->getMeshData();
            auto tempNodes = meshData->getTempNodes();
            auto currentViewportPrivate = dynamic_cast<MechPreData::mPreViewportPrivate*>(VMManager->getCurrentViewportPrivate());
            currentViewportPrivate->_showFreeNode.clear();
            currentViewportPrivate->_showFreeNode.insert(tempNodes.begin(), tempNodes.end());
            //mDisplayMenuFunction::getInstance()->updateFreeNodeShow(currentViewportPrivate);
            if (currentViewportPrivate == nullptr)
            {
                return;
            }
            if (currentViewportPrivate->getViewportType() == Viewport3D)
            {
                auto meshData = getCVM->getModelData()->getMeshData();
                QVector<QVector3D> vertexs;
                for (auto id : currentViewportPrivate->_showFreeNode)
                {
                    vertexs.append(meshData->getVectorById(id));
                }
                currentViewportPrivate->getRendWidget()->getCommonShapeRender()->appendPoint("freeNode", vertexs, QVector3D(108 / 255.0, 181 / 255.0, 1.0));
                currentViewportPrivate->getRendWidget()->updatePaint();
            }

        }

    }

    void mCreatePointPanel::updateNodePos()
    {
        QVector3D coord;
        if (nodes.empty())
        {
            coord = {0,0,0};
        }
        vector<QVector3D> vertexVec;
        vertexVec.resize(nodes.size(), QVector3D(0, 0, 0));  // 明确初始值
        auto meshData = getCVM->getModelData()->getMeshData();
        auto elementData = getCVM->getModelData()->getElementData();
        auto getNodeVertexByID = [&](mMeshData* meshData, int id, QVector3D& vertex3D) {
            double* vertex = meshData->_nodedata.xyz + meshData->_nodedata.nodeidmap[id] * 3;
            vertex3D = QVector3D(vertex[0], vertex[1], vertex[2]);
            };
        for (int i=0;i< nodes.size();i++)
        {
            getNodeVertexByID(meshData, nodes[i].data_id.element_id, vertexVec[i]);
        }

        // 自定义累加器
        auto sum = std::accumulate(
            vertexVec.begin(),
            vertexVec.end(),
            QVector3D(0, 0, 0),  // 初始值
            [](const QVector3D& a, const QVector3D& b) {
                return QVector3D(a.x() + b.x(), a.y() + b.y(), a.z() + b.z());
            }
        );

        float count = static_cast<float>(vertexVec.size());

        QVector3D result = QVector3D(sum.x() / count, sum.y() / count, sum.z() / count);
        QString average= QString("%1,%2,%3")
            .arg(result.x(), 0, 'f', 2)  // 6位小数
            .arg(result.y(), 0, 'f', 2)
            .arg(result.z(), 0, 'f', 2);
        if (vertexVec.empty())//未拾取节点
        {
            average = "0,0,0";
        }
        
        //偏移方向
        coorEdit->setText(average);

    }

}