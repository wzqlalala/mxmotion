#include "mMeshPanel.h"
#include "mViewportModelManager.h"
#include "mMeshData.h"
#include "PickCommonFunction.h"
#include "mSharedNodePanel.h"
#include "GlobalMeshMessage.h"
#include "mMeshTransform.h"
//#include "mGeoInfoCalculate.h" //【OCC摘除·S10/P0-1 终局2】死 include（经 mGeoDataModelSet 拉 OCC 链）
#include "mMeshEdit.h"
using namespace MechPreData;
using namespace MeshPanelData;

namespace MechMainWindow
{
    mSharedNodePanel::mSharedNodePanel(QWidget* parent)
        : mMeshPanel(parent)
    {
        auto* mainLay = new QVBoxLayout(this);

        FoldSection* m_ObjectFold = new FoldSection("网格", this);
      
        posPick.initWidget("对象");
        posPick.setState(false);
        m_ObjectFold->addRow(posPick.propwid);

        m_paramFold = new FoldSection("参数", this);

        QLabel* tol = new PanelLabel("容差");
        tolEdit = new QLineEdit("0.01");
        m_paramFold->addRow(tol, tolEdit);

        QFrame* horizontalLine = createHorizontalLine();

        QHBoxLayout* h = new QHBoxLayout;
        h->addStretch();
        QPushButton* searchBtn = new QPushButton("查找");
        QPushButton* combineBtn = new QPushButton("合并");
        h->addWidget(searchBtn);
        h->addWidget(combineBtn);
        
        mainLay->addWidget(m_ObjectFold);
        mainLay->addWidget(m_paramFold);
        mainLay->addWidget(horizontalLine);
        mainLay->addLayout(h);
        mainLay->addStretch();

        connect(posPick.btnok, &QToolButton::clicked, this, [=]()
            {
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
            {
                posPick.setState(false);
            }
                
            _meshOptPanelData->setMeshIDs(std::move(MeshIDs));
            });

        // 检查
        connect(searchBtn, &QPushButton::clicked, this, &mSharedNodePanel::search);

        // 合并
        connect(combineBtn, &QPushButton::clicked, this, &mSharedNodePanel::combine);
    }

    mSharedNodePanel::~mSharedNodePanel()
    {

    }

    void mSharedNodePanel::editMeshPanel(int meshId)
    {
        _meshOptPanelData = getCVM->getModelData()->getManager<mMeshOptPanelManager>(Physical_MeshOptPanel).getMeshPanelData(meshId);
        updatePanel();
    }

    void mSharedNodePanel::updatePanel()
    {

    }

    void mSharedNodePanel::search()
    {
        // 拾取单元容器判空
        if (MeshIDs.empty())
        {
            QString message = QString("未拾取单元!");
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage, message, true);
            return;
        }

        // 获取网格数据
        auto meshdata = getCVM->getModelData()->getMeshData();
        GlobalMeshMessage::getInstance()->setMeshData(meshdata);

        // 进行共节点查找
        bool isDistance = true;
        double errorTol;
        nodeIds = meshdata->findElementMergeNodes(MeshIDs, tolEdit->text().toDouble(), isDistance, errorTol);

        // 如果未找到进行容差判断
        if (nodeIds.size() == 0)
        {
            if (!isDistance)
            {
                QString message = QString("最大容差不可超过 %1!").arg(errorTol);
                mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage, message, true);
                return;
            }
        }

        // 获取所有的共节点
        vector<RenderPickResultData> lightNodes;
        for (int i = 0; i < nodeIds.size(); i++)
        {
            for (int j = 0; j < nodeIds[i].size(); j++)
            {
                RenderPickResultData data;
                data.result_id = nodeIds[i][j];
                lightNodes.push_back(data);
            }
        }

        // 转换共节点数据格式
        std::vector<MeshSelectInfoData> SelectData;
        for (auto node : lightNodes)
        {
            SelectData.push_back(MechPreData::PickData2SelectData(SelectionInfoType::MeshNode, node));
        }

        // 将共节点进行高亮显示
        ClearPreWidgetMeshPick(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::MeshNode, SelectData);
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();

        // 输出提示信息
        QString str = QString("共节点查询完成!");
        mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::NormalMessage, str, true);
    }

    void mSharedNodePanel::combine()
    {
        // 共节点容器判空
        if (nodeIds.size() == 0)
        {
            QString message = QString("未查询到共节点!");
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage, message, true);
            return;
        }

        // 获取网格数据
        auto meshdata = getCVM->getModelData()->getMeshData();
        GlobalMeshMessage::getInstance()->setMeshData(meshdata);

        // 进行共节点合并
        meshdata->elementMergeNodes(nodeIds);

        // 清空高亮
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();

        // 输出提示信息
        QString message = QString("共节点合并完成!");
        mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::NormalMessage, message, true);
    }
}