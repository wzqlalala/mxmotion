//Mesh
//#include "mGeoInfoCalculate.h" //【OCC摘除·S10/P0-1】死 include（S9 已退役，零使用；dev 精简 include 后本就不含）
#include "mMeshCheck.h"
#include "GlobalMeshMessage.h"

//Mech
#include "mMeshPanel.h"
#include "PickCommonFunction.h"
#include "mFreeEdgePanel.h"

using namespace MechPreData;
using namespace MeshPanelData;

namespace MechMainWindow
{
    mFreeEdgePanel::mFreeEdgePanel(QWidget* parent)
        : mMeshPanel(parent)
    {
        // 主布局
        auto* mainLay = new QVBoxLayout(this);

        // 对象折叠部分
        FoldSection* m_ObjectFold = new FoldSection("对象", this);
        posPick.initWidget("对象");
        posPick.setState(false);
        posPick.labelpicked->setText(QString("默认为全部"));
        m_ObjectFold->addRow(posPick.propwid);

        // 参数折叠部分
        FoldSection* m_paraFold = new FoldSection("参数", this);
        QLabel* part = new PanelLabel("所属部件");
        QComboBox* partBox = new QComboBox();
        partBox->addItems({"新建","部件名称"});
        partEdit = new QLineEdit();
        m_paraFold->addRow(part, partBox,partEdit);

        // 水平分割线
        QFrame* horizontalLine = createHorizontalLine();

        // 查找与提取按钮
        QHBoxLayout* HLayout = new QHBoxLayout;
        QPushButton* searchBtn = new QPushButton("查找");
        QPushButton* extractBtn = new QPushButton("提取");
        HLayout->addStretch();
        HLayout->addWidget(searchBtn);
        HLayout->addWidget(extractBtn);

        // 子布局添加进主布局
        mainLay->addWidget(m_ObjectFold);
        mainLay->addWidget(m_paraFold);
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
        connect(posPick.btnclear, &QToolButton::clicked, this, [=]() {
            MeshIDs.clear();
            posPick.setState(false);
            _meshOptPanelData->setMeshIDs(std::move(MeshIDs));
            _meshOptPanelData->setPickType(QString("拾取"));
            });
        connect(searchBtn, &QPushButton::clicked, this, &mFreeEdgePanel::search);
        connect(extractBtn, &QPushButton::clicked, this, &mFreeEdgePanel::extract);
    }

    mFreeEdgePanel::~mFreeEdgePanel()
    {

    }

    void mFreeEdgePanel::editMeshPanel(int meshId)
    {
        _meshOptPanelData = getCVM->getModelData()->getManager<mMeshOptPanelManager>(Physical_MeshOptPanel).getMeshPanelData(meshId);

        updatePanel();
    }

    void mFreeEdgePanel::updatePanel()
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

    void mFreeEdgePanel::search()
    {
        if (MeshIDs.empty())
            return;
        auto meshdata = getCVM->getModelData()->getMeshData();
        GlobalMeshMessage::getInstance()->setMeshData(meshdata);
        auto FreeEdges = meshFreeEdge(MeshIDs);
        //获取零件名
        int dim = 2;
        if (dim == 1)
        {

        }
        else
        {
            //无自由边被发现
            if (FreeEdges.size() == 0)
            {
                QString msg = "无自由边被发现";
                mGlobalSignals::getInstance()->outputMessageSig(1, msg, true);
                return;
            }
            std::vector<MeshSelectInfoData> SelectData;
            for (auto edge : FreeEdges)
            {
                SelectData.push_back({ dim,0,std::min(edge.first,edge.second),std::max(edge.first,edge.second),0 });
            }
            ClearPreWidgetMeshPick(getCVM->getPreRenderWidget());
            getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::MeshLine, SelectData);
            MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
            getCVM->getPreRenderWidget()->updatePaint();
        }
    }

    void mFreeEdgePanel::extract()
    {
        if (MeshIDs.empty())
            return;

        auto meshdata = getCVM->getModelData()->getMeshData();
        GlobalMeshMessage::getInstance()->setMeshData(meshdata);
        auto FreeEdges = meshFreeEdge(MeshIDs);
        //获取零件名
        QString pname = partEdit->text().simplified();
        if (pname.isEmpty())
        {
            QString msg ="请输入零件名";
            mGlobalSignals::getInstance()->outputMessageSig(ErrorMessage, msg, true);
            return;
        }
        //判断零件存不存在
        if (getCVM->getModelData()->getMeshData()->havePart(pname))
        {
            QString msg = "该零件名已经存在";
            mGlobalSignals::getInstance()->outputMessageSig(1, msg, true);
            return;
        }
        int dim = 2;
        if (dim == 1)
        {

        }
        else
        {
            //无自由边被发现
            if (FreeEdges.size() == 0)
            {
                QString msg ="无自由边被发现";
                mGlobalSignals::getInstance()->outputMessageSig(1, msg, true);
                return;
            }

            QVector<int> index;
            for (auto it : FreeEdges)
            {
                index.push_back(it.first);
                index.push_back(it.second);
            }
            meshdata->createMeshToPart(D2R2, index, pname, 2);
            vector<int> partIds = getCVM->getModelData()->getAllPhyID(Physical_Part);
            emit mGlobalSignals::getInstance()->createUpdatePartSig(true, set<int>{partIds.end() - 1, partIds.end()});
            //mViewportManager::getInstance()->setAllPickLineData_Temp({});
        }
    }
}