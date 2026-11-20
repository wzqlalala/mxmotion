//Mesh
#include "GlobalMeshMessage.h"
#include "mMeshTransform.h"
//#include <mGeoInfoCalculate.h> //【OCC摘除·S10/P0-1 终局2】死 include（零使用）
#include "mMeshCheck.h"
//Mech
#include "PickCommonFunction.h"
#include "mMeshIntersectPanel.h"

using namespace MechPreData;
using namespace MeshPanelData;

namespace MechMainWindow
{
    mMeshIntersectPanel::mMeshIntersectPanel(QWidget* parent)
        : mMeshPanel(parent)
    {
        //主布局
        auto* mainLay = new QVBoxLayout(this);

        //折叠对象部分
        FoldSection* m_ObjectFold = new FoldSection("对象", this);
        posPick.initWidget("对象");
        posPick.setState(false);
        posPick.labelpicked->setText(QString("默认为全部"));
        m_ObjectFold->addRow(posPick.propwid);

        //水平分割线
        QFrame* horizontalLine = createHorizontalLine();

        //清除与提取按钮
        QHBoxLayout* HLayout = new QHBoxLayout;
        QPushButton* clearBtn = new QPushButton("清除");
        QPushButton* combineBtn = new QPushButton("提取");
        HLayout->addStretch();
        HLayout->addWidget(clearBtn);
        HLayout->addWidget(combineBtn);

        //将子布局添加到主布局
        mainLay->addWidget(m_ObjectFold);
        mainLay->addWidget(horizontalLine);
        mainLay->addLayout(HLayout);
        mainLay->addStretch();

        // 连接信号与槽函数
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
            _meshOptPanelData->setPickType(QString("拾取"));
            });
        connect(posPick.btnclear, &QToolButton::clicked, this, [=]() {
            MeshIDs.clear();
            posPick.setState(false);
            _meshOptPanelData->setMeshIDs(std::move(MeshIDs));
            _meshOptPanelData->setPickType(QString("拾取"));
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
            _meshOptPanelData->setPickType(QString("拾取"));
            });
        connect(combineBtn,&QPushButton::clicked,this,&mMeshIntersectPanel::apply);
        connect(clearBtn,&QPushButton::clicked,this,&mMeshIntersectPanel::clear);
    }

    mMeshIntersectPanel::~mMeshIntersectPanel()
    {

    }

    void mMeshIntersectPanel::apply()  
    {

        if (MeshIDs.empty())
            return;

        // 对网格进行平移
        GlobalMeshMessage::getInstance()->setMeshData(getCVM->getModelData()->getMeshData());
        std::vector<RenderPickResultData> interSecIDs = meshIntersection(MeshIDs);
        // 清除拾取与高亮
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
        std::vector<MeshSelectInfoData> SelectData; 
        for (auto ele : interSecIDs)
        {
            MeshSelectInfoData rel = PickData2SelectData(SelectionInfoType::MeshElement, ele);
            SelectData.push_back(rel);
        }
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::MeshElement, SelectData);
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint(); 
    }

    void mMeshIntersectPanel::clear()
    {
        // 清除拾取与高亮
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mMeshIntersectPanel::editMeshPanel(int meshId)
    {
        _meshOptPanelData = getCVM->getModelData()->getManager<mMeshOptPanelManager>(Physical_MeshOptPanel).getMeshPanelData(meshId);

        updatePanel();

    }

    void mMeshIntersectPanel::updatePanel()
    {
        if (_meshOptPanelData->getPickType() == "全部")
        {
            MeshIDs.clear();
            MechModelData::mModelData* modelData = getCVM->getModelData();
            mMeshData* meshdata = modelData->getMeshData();
            std::vector<RenderPickResultData> allElements = meshdata->getAllElements();
            MeshIDs = allElements;
            _meshOptPanelData->setMeshIDs(MeshIDs);
        }
        else
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
        }
    }
}