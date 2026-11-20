
#include "mGlobalDimPanel.h"

#include "mAppConfiguration.h"

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
#include <QToolButton>

#include "mMeshPanelManager.h"
#include "mViewportModel.h"
#include "mViewportModelManager.h"
#include "mModelData.h"
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/view/HighlightController.h"
#include "mGeoManager.h"
#include "MXMesh.h"
#include "mxr/view/mFontRender.h"
#include "mxr/view/mFontRender.h"
//
#include <mMechGlobalSignals.h>
#include "GlobalMeshMessage.h"
#include "mOtherFunction.h"
using namespace MeshPanelData;
using namespace MechPreData;

namespace MechMainWindow
{
	mGlobalDimPanel::mGlobalDimPanel(QWidget* parent)
		: mPanelBase(parent)
	{
		auto* mainLay = new QVBoxLayout(this);


		m_rangeFold = new FoldSection("尺寸控制", this);
		auto* rangeLay = new QHBoxLayout;
		_GlobalSizeLabel = new PanelLabel("全局尺寸");
		_GlobalSizeEdit = new QLineEdit("");
		_GlobalSizeEdit->setText("2");
        auto* v = new QDoubleValidator(0.001, 1e11, 8, _GlobalSizeEdit);
        v->setNotation(QDoubleValidator::ScientificNotation);
        _GlobalSizeEdit->setValidator(v);
		_GlobalSizeButton = new QToolButton();
		_GlobalSizeButton->setIcon(QIcon(":/mMainWidgets/Icon/shuaxin3x.png"));
		rangeLay->addWidget(_GlobalSizeLabel);
		rangeLay->addWidget(_GlobalSizeEdit);
		rangeLay->addWidget(_GlobalSizeButton);

		m_rangeFold->addRow(rangeLay);
		auto* lay2 = new QHBoxLayout;

		lay2->addStretch();
		//lay2->addWidget(_comfirmButton);
		m_rangeFold->addRow(lay2);

		mainLay->addWidget(m_rangeFold);
		mainLay->addStretch();

        connect(_GlobalSizeButton, &QToolButton::clicked, this, &mGlobalDimPanel::onSizeButtonClicked);
        connect(_GlobalSizeEdit, &QLineEdit::editingFinished, this, &mGlobalDimPanel::apply);
	}

	mGlobalDimPanel::~mGlobalDimPanel()
	{
	
	}
    static double findMinDouble(const std::vector<std::pair<int, double>>& vec)
    {
        if (vec.empty()) {
            throw std::invalid_argument("Vector is empty");
        }

        auto min_it = std::min_element(vec.begin(), vec.end(),
            [](const std::pair<int, double>& a, const std::pair<int, double>& b) {
                return a.second < b.second;
            });

        return min_it->second;
    }
    void MechMainWindow::mGlobalDimPanel::setDefaultSize()
    {
        MechModelData::mModelData* modelData = getCVM->getModelData();
        if (!modelData) return;

        mMeshData* meshdata = modelData->getMeshData();
        if (!meshdata) return;

        auto* meshEntity = meshdata->getMeshEntity();
        if (!meshEntity) return;

        // 获取预剖分尺寸
        const auto& size_pair = meshEntity->getAndSetPreMeshSize(3, false);
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
            QString str = MxFunctions::mNumberToString(sum);
            _GlobalSizeEdit->setText(str);
        }
        else
        {
            _GlobalSizeEdit->setText("0");
        }
        _meshPanelData->setPreDefineMeshSize(sum);
        // 仅计算并填入默认尺寸，不剖分、不显示白点；白点预览由点击按钮触发
    }

    void MechMainWindow::mGlobalDimPanel::onSizeButtonClicked()
    {
        // 输入框为空时先计算默认尺寸，否则直接使用输入的尺寸
        if (_GlobalSizeEdit->text().trimmed().isEmpty())
            setDefaultSize();
        apply();
    }

	void mGlobalDimPanel::newGlobalDimPanel()
	{
       
        _meshPanelData = new mMeshPanelBase("全局尺寸");
        getCVM->getModelData()->addPhysicalField<mMeshPanelBase>(_meshPanelData);
        auto size = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(_meshPanelData->getMeshPanelId());//->getMeshSize();
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建网格 %1").arg("全局尺寸"), true);

        //updatePanel();

        //emit MechCommon::mMechGlobalSignals::getInstance()->createUpdatePropertySig(true, _propertyData->getPropertyId());

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateGlobalDimSig(true);
        initSize();
	}

	void mGlobalDimPanel::updatePanel()
	{


	}

	void mGlobalDimPanel::editGlobalDimPanel(int meshID)
	{

		updatePanel();

	}
    void mGlobalDimPanel::initSize()
    {
        setDefaultSize();
    }
    void mGlobalDimPanel::apply()
    {

        MechModelData::mModelData* modelData = getCVM->getModelData();
        mMeshData* meshdata = modelData->getMeshData();
        if (!meshdata) return;

        _globalSize = _GlobalSizeEdit->text().toDouble();
        if (_globalSize < (0.001 * _meshPanelData->getPreDefineMeshSize()))
        {
            _GlobalSizeEdit->setText(MxFunctions::mNumberToString(_meshPanelData->getPreDefineMeshSize()));
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, QString("%1").arg(QString("尺寸设置过小")), true);
            return;
        }

        getCVM->getPreRenderWidget()->getFontRender()->clearCommonFont();
        getCVM->getModelData()->getManager<MeshPanelData::mMeshPanelManager>(Physical_MeshPanel)._globalSize = _globalSize;
        ////

        double size = _globalSize;

        getCVM->getPreRenderWidget()->getHighlightController()->clearTempHighlight();

        auto geoDatabase = getCVM->getModelData()->getGeoData();

        int dim = 1;
        std::set<int> meshGeoId = getGEOServer.geoCalculate().getAllGeoID(1);

        MeshEntity* entity = meshdata->getMeshEntity();
        if (!entity) return;
        std::shared_ptr<MXMesh>mesh = std::make_shared<MXMesh>();

        // 收集尚未剖分的边，只对未剖分边做剖分，已剖分边的节点直接复用
        bool needMesh = false;
        for (auto* edge : entity->getGeoEdges())
        {
            if (meshGeoId.find(edge->getGeoTag()) != meshGeoId.end() &&
                edge->getMeshStatic() != MeshDone)
            {
                needMesh = true;
                break;
            }
        }

        if (needMesh)
        {
            //一键删除
            meshdata->deleteAllMeshPart();
            entity->resetPreMeshSize(dim, MAX_LC);   // 重置尺寸！！！！
            entity->setMeshPattern(MeshEntity::MeshPattern::STANDRAD_MESHING);
            mesh->setMeshData(meshdata);
            //void MXMeshInterface(double lcMax, double lcMin, bool smoothBool, bool curvatureBool, bool proximityBool, double smoothFactor, double curvatureFac,
            // double proximityFac,int meshtype, int meshDim, MeshEntity* meshE, set<int>meshGeoId, set<int>ToggleID);
            mesh->MXMeshInterface(size, size, false, false, false, 0.5, 0.3, 0.2, 0, dim, entity, meshGeoId, {});           // 真正剖分
            mGlobalSignals::getInstance()->outputMessageSig(0, tr("剖分结束"), true);
        }

        meshdata->GEdgeIDToBiasValue.clear();
        if (dim == 1)
        {
            for (auto it : entity->getGeoEdges())
            {
                auto its = meshGeoId.find(it->getGeoTag());
                if (its != meshGeoId.end())
                {

                    meshdata->GEdgeIDToBiasValue.insert(it->getGeoTag(), 0/*_offsetDistanceEdit1->text().toDouble()*/);
                }
            }
        }
        // meshdata->GEdgeIDTodynaNodeNum.clear();
         //meshdata->IDToCenterOfGEdge.clear();
         //MXMesh1D
        auto mxedges = entity->getGeoEdges();
        /*    vector<MXGeoEdge*> meshedges;*/
           /* for (auto it : mxedges)
            {
                if (it->getGeoEntity() && !it->isDegenerated())
                {
                    meshedges.push_back(it);
                }
            }*/
            // 存储GEdgeIDTodynaNodeNum的值
        for (const auto& edge : mxedges) {
            if (edge->getCombineEdge()) {
            }
            else {
                meshdata->GEdgeIDTodynaNodeNum.insert(edge->getGeoTag(),
                    edge->meshGeo_vertexs.size() + 2);
            }
        }
        //全部边
        {
            QHash<int, double> ids = meshdata->GEdgeIDToBiasValue;
            for (auto it = ids.begin(); it != ids.end(); it++)
            {
                if (meshdata->GEdgeIDTodynaNodeNum[it.key()] == 2)
                {
                    continue;
                }
                try
                {
                    meshdata->GEdgeIDToBiasValue[it.key()] = 0;
                    mesh->biasEdgeNode(it.key(), 0);
                }
                catch (...)
                {

                }

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
        //偏置因子显示

        QVector<double> txt;
        QVector<QVector3D> pos1;
        if (dim == 1)
        {
            QHash<int, int> ids = meshdata->GEdgeIDTodynaNodeNum;
            for (auto it = ids.begin(); it != ids.end(); it++)
            {

                int NUM;
                pos1.push_back(meshdata->IDToCenterOfGEdge[it.key()]);
                txt.push_back(it.value());
            }

            //getCVM->getPreRenderWidget()->getFontRender()->appendCommonFont("adjustNum", pos1, txt);
        }

        //下面是偏置节点高亮
        //RenderNode

        //QHash<int, int> ids = meshdata->GEdgeIDTodynaNodeNum;

        set<MXMeshVertex*> Nodes;
        for (auto it1 : meshGeoId)
        {

            set<MXMeshVertex*>mvs;
            for (auto it : entity->getGeoEdges()) {
                if (it->getGeoTag() == it1) {
                    /*if (it->getBeginVertex()->meshGeo_vertexs.empty())
                        continue;*/
                    if (!it->getBeginVertex()->meshGeo_vertexs.empty())
                        mvs.insert(it->getBeginVertex()->meshGeo_vertexs[0]);
                    if (it->tempEdgeVertexs.size()) {
                        for (auto itv : it->tempEdgeVertexs) {
                            mvs.insert(itv);
                        }
                    }
                    else
                    {
                        if (!MXMeshParameters::getInstance()->DYNAMIC) {
                            for (auto itv : it->meshGeo_vertexs) {
                                mvs.insert(itv);
                            }
                        }
                    }
                    if (!it->getEndVertex()->meshGeo_vertexs.empty())
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

            //渲染节点
            set<MXMeshVertex*> Node = mvs;
            for (auto id : Node)
            {
                Nodes.insert(id);
            }
        }


        //MXMeshParameters::getInstance()->DYNAMIC = false;
        PreMeshTempHighlightData highlight;
        highlight.nodeids = Nodes;
        highlight.stylePattern = NodeStylePattern::CIRCLE;
        highlight.stylePatternColor = { 1.0f,1.0f,1.0f,1.0f };
        //mMeshMain::getInstance()->setAllPickNodeData_Temp(Nodes);
        getCVM->getPreRenderWidget()->getHighlightController()->updateTempMeshHighlightData(highlight);
        getCVM->getPreRenderWidget()->updatePaint();

        //mMeshMain::getInstance()->updatePickRender();
        ////mMeshMain::getInstance()->appendCommonFontdouble("biastNum", pos1, txt);

        ////
    }
}
