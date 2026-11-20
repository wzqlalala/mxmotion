//Mesh
#include "GlobalMeshMessage.h"
#include "mMeshTransform.h"
//#include <mGeoInfoCalculate.h> //【OCC摘除·S10/P0-1 终局2】死 include（零使用）
#include "mMeshCheck.h"
//Mech
#include "PickCommonFunction.h"
#include "mMeshInterferePanel.h"

using namespace MechPreData;
using namespace MeshPanelData;

namespace MechMainWindow
{
    mMeshInterferePanel::mMeshInterferePanel(QWidget* parent)
        : mMeshPanel(parent)
    {
        //主部件
        auto* mainLay = new QVBoxLayout(this);
        FoldSection* m_ObjectFold = new FoldSection("对象", this);
        posPick.initWidget("对象");
        posPick.setState(false);
        posPick.labelpicked->setText(QString("默认为全部"));
        m_ObjectFold->addRow(posPick.propwid);
        
        //参数折叠部分
        FoldSection* paramFold = new FoldSection("参数",this);
        QLabel* overlap = new PanelLabel("干涉阈值");
        overlapEdit = new QLineEdit();
        paramFold->addRow(overlap, overlapEdit);

        //清除与检查按钮
        QFrame* horizontalLine = createHorizontalLine();
        QHBoxLayout* HLayout = new QHBoxLayout;
        QPushButton* clearBtn = new QPushButton("清除");
        QPushButton* checkBtn = new QPushButton("检查");
        HLayout->addStretch();
        HLayout->addWidget(clearBtn);
        HLayout->addWidget(checkBtn);

        //将子部件添加到主部件
        mainLay->addWidget(m_ObjectFold);
        mainLay->addWidget(paramFold);
        mainLay->addWidget(horizontalLine);
        mainLay->addLayout(HLayout);
        mainLay->addStretch();

        //连接信号和槽
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
        connect(checkBtn, &QPushButton::clicked, this, &mMeshInterferePanel::apply);
        connect(clearBtn, &QPushButton::clicked, this, &mMeshInterferePanel::clear);
    }

    mMeshInterferePanel::~mMeshInterferePanel()
    {

    }

    void mMeshInterferePanel::apply()
    {
        if (MeshIDs.empty())
            return;

        // 干涉阈值防御：空输入或非正值会导致零厚度膨胀产生退化三角形，求交阶段崩溃
        bool ok = false;
        double threshold = overlapEdit->text().toDouble(&ok);
        if (!ok || threshold <= 0.0)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("请输入大于0的干涉阈值！"), true);
            return;
        }

        // 对网格进行平移
        GlobalMeshMessage::getInstance()->setMeshData(getCVM->getModelData()->getMeshData());
        std::vector<RenderPickResultData> IDs = meshInterference(MeshIDs, threshold);
        // 清除拾取与高亮
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
        std::vector<MeshSelectInfoData> SelectData;
        for (auto ele : IDs)
        {
            MeshSelectInfoData rel = PickData2SelectData(SelectionInfoType::MeshElement, ele);
            SelectData.push_back(rel);
        }
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::MeshElement, SelectData);
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();
    }
    
    void mMeshInterferePanel::clear()
    {
        // 清除拾取与高亮
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mMeshInterferePanel::editMeshPanel(int meshId)
    {
        _meshOptPanelData = getCVM->getModelData()->getManager<mMeshOptPanelManager>(Physical_MeshOptPanel).getMeshPanelData(meshId);

        updatePanel();

    }

    void mMeshInterferePanel::updatePanel()
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