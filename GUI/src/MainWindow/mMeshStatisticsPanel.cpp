//Model
#include <mBasicEnum.h>
#include <mMeshData.h>
#include "mDataElement.h"

//Mech
#include "PickCommonFunction.h"
#include "mMeshStatisticsPanel.h"
#include "mOtherFunction.h"

using namespace MechPreData;
using namespace MeshPanelData;

namespace MechMainWindow
{
    mMeshStatisticsPanel::mMeshStatisticsPanel(QWidget* parent)
        : mMeshPanel(parent)
    {
        //主布局
        auto* mainLay = new QVBoxLayout(this);

        //对象折叠部分
        FoldSection* m_ObjectFold = new FoldSection("对象", this);
        posPick.initWidget("对象");
        posPick.setState(false);
        m_ObjectFold->addRow(posPick.propwid);

        //这里整段疑似都没有用上
        QPushButton* combineBtn = new QPushButton("合并");
        QHBoxLayout* HLayout = new QHBoxLayout;
        HLayout->addStretch();
        HLayout->addWidget(combineBtn);
        QFrame* horizontalLine = createHorizontalLine();

        //统计功能折叠部分
        FoldSection* statisticsFold = new FoldSection("统计",this);
        auto* hut = new QWidget;
        hut->setFixedHeight(350);
        statisticsFold->layout()->addRow(hut);   // 标签空着，整行只放 table

        // 创建表格
        tab = new QTableWidget();
        // 设置为8行2列
        tab->setRowCount(8);
        tab->setColumnCount(2);

        // 设置列标题
        QStringList headers;
        headers << "类型" << "数量";
        tab->setHorizontalHeaderLabels(headers);

        // 设置行标题（垂直表头）
        QStringList types;
        types << "节点"
            << "线"
            << "三角形"
            << "四边形"
            << "四面体"
            << "六面体"
            << "三棱柱"
            << "四棱锥";
        for (int i = 0; i < types.size(); i++) {
            // 第一列：类型
            QTableWidgetItem* typeItem = new QTableWidgetItem(types[i]);
            typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);  // 设置类型列不可编辑
            typeItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter); // 左对齐
            tab->setItem(i, 0, typeItem);

            // 第二列：数量（初始为空或0，不可编辑）
            QTableWidgetItem* countItem = new QTableWidgetItem("0");
            countItem->setFlags(countItem->flags() & ~Qt::ItemIsEditable);  // 设置数量列不可编辑
            countItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter); // 数字右对齐
            tab->setItem(i, 1, countItem);
        }

        // 可选：设置表格样式
        tab->horizontalHeader()->setStretchLastSection(true);  // 最后一列自动拉伸
        tab->verticalHeader()->setVisible(true);  // 显示垂直表头（默认就是显示的）

        // 可选：设置列宽
        tab->setColumnWidth(0, 150);  // 类型列宽度
        tab->setColumnWidth(1, 100);  // 数量列宽度

        tab->setParent(hut);
        auto* vbox = new QVBoxLayout(hut);
        vbox->setContentsMargins(0, 0, 0, 0);
        vbox->addWidget(tab);

        //将子布局添加进主布局
        mainLay->addWidget(m_ObjectFold);
        mainLay->addWidget(statisticsFold);
        mainLay->addStretch();

        //连接信号和槽
        connect(posPick.btnok, &QToolButton::clicked, this, [=]() {
            StatisticsUpdate();
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
            StatisticsClear();
            });
        connect(posPick.btnadd, &QToolButton::clicked, this, [=]() {
            
            StatisticsUpdate();

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
    }

    mMeshStatisticsPanel::~mMeshStatisticsPanel()
    {

    }

    void mMeshStatisticsPanel::editMeshPanel(int meshId)
    {
        _meshOptPanelData = getCVM->getModelData()->getManager<mMeshOptPanelManager>(Physical_MeshOptPanel).getMeshPanelData(meshId);

        updatePanel();

    }

    void mMeshStatisticsPanel::StatisticsClear()
    {
        tab->item(0, 1)->setText("0");
        tab->item(1, 1)->setText("0");
        tab->item(2, 1)->setText("0");
        tab->item(3, 1)->setText("0");
        tab->item(4, 1)->setText("0");
        tab->item(5, 1)->setText("0");
        tab->item(6, 1)->setText("0");
        tab->item(7, 1)->setText("0");
    }
    void mMeshStatisticsPanel::StatisticsUpdate()
    {
        StatisticsClear();
        int num = 0;
        PickDataType pickDataType = getCVM->getCurrentPickDataType(num);
        QString type = "节点";//假设为节点
        std::vector<MeshSelectInfoData> res;
        auto meshData = getCVM->getModelData()->getMeshData();
        auto elementData = getCVM->getModelData()->getElementData();
        //拾取对象涉及的节点集合（去重）
        std::set<int> nodeSet;
        switch (pickDataType)
        {
        case PickDataType::Node:
        {
            res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
            type = "节点";
            break;
        }
        case PickDataType::ElementEdge:
        {
            res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshLine);
            type = "单元线";
            break;
        }
        case PickDataType::ElementFace:
        {
            res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshFace);
            type = "单元面";
            break;
        }
        case PickDataType::Element:
        {
            res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
            type = "单元";
            break;
        }
        default:
            break;
        }
        if (type == "节点")
        {
            tab->item(0, 1)->setText(MxFunctions::mNumberToString(res.size()));
        }
        else if (type == "单元线")
        {
            // 统计单元线的节点（每个单元线为两个端点，去重）
            for (auto& r : res)
            {
                mPartData* part = meshData->getPartData(r.part_id);
                if (part == nullptr)
                    continue;
                std::pair<int, int> edgeNodes = part->getEdgeNodeByElementIdAndBN(r.data_id.element_id, r.data_index.line_index);
                nodeSet.insert(edgeNodes.first);
                nodeSet.insert(edgeNodes.second);
            }
            tab->item(0, 1)->setText(QString::number(nodeSet.size()));
            tab->item(1, 1)->setText(QString::number(res.size()));
        }
        else if (type == "单元面")
        {
            // 定义单元面结构体
            struct ElementFace {
                int eleID;      // 单元ID
                int order; //单元面在单元的第几个面
                ElementType type;//单元类型
            };
            QVector<ElementFace> elefaces;
            QHash<MeshType, QVector<int>> hash;
            for (auto r : res)
            {
                int meshID = r.data_id.element_id;
                ElementType elementType = elementData->getElementTypeByMeshID(meshID);
                auto type = meshData->getMeshTypeByEleId(meshID);
                hash[type].push_back(meshID);
                if (elementType == NoneElement)
                {
                    elementType = mElementFunction::getDefaultElementType(ImplicitAnalysis, meshData->getMeshTypeByEleId(meshID));
                }
                elefaces.append({ meshID,  r.data_index.face_index, elementType });

                // 统计单元面的节点（去重）
                RenderPickResultData faceRprd;
                faceRprd.part_id = r.part_id;
                faceRprd.result_id = r.data_id.element_id;
                faceRprd.result_id2 = r.data_index.face_index;
                QVector<int> faceNodes = meshData->getNodeIndexBySurface(faceRprd);
                nodeSet.insert(faceNodes.begin(), faceNodes.end());
            }
            tab->item(0, 1)->setText(QString::number(nodeSet.size()));
            tab->item(1, 1)->setText(QString::number(hash[MeshBeam].size()));
            tab->item(2, 1)->setText(QString::number(hash[MeshTri].size()));
            tab->item(3, 1)->setText(QString::number(hash[MeshQuad].size()));
            tab->item(4, 1)->setText(QString::number(hash[MeshTet].size()));
            tab->item(5, 1)->setText(QString::number(hash[MeshHex].size()));
            tab->item(6, 1)->setText(QString::number(hash[MeshWedge].size()));
            tab->item(7, 1)->setText(QString::number(hash[MeshPyramid].size()));
        }
        else if (type == "单元")
        {
            // 定义单元结构体
            struct Element {
                int eleID;      // 单元ID
                ElementType type;//单元类型
            };

            // 示例数据（4个单元面）
            //QVector<Element> elefaces = {
            //	{ 1, D3T4},
            //	{ 2, D3T4},
            //	{ 3, D3T4},
            //	{ 4, D3T4}
            //};
            QVector<Element> elefaces;
            QHash<MeshType, QVector<int>> hash;
            for (auto r : res)
            {
                int meshID = r.data_id.element_id;
                ElementType elementType = elementData->getElementTypeByMeshID(meshID);
                auto type = meshData->getMeshTypeByEleId(meshID);
                hash[type].push_back(meshID);
                elefaces.append({ meshID, elementType });

                // 统计单元的节点（去重）
                mPartData* part = meshData->getPartData(r.part_id);
                if (part == nullptr)
                    continue;
                std::vector<int> eleNodes = part->getNodesByElementId(meshID);
                nodeSet.insert(eleNodes.begin(), eleNodes.end());
            }

            tab->item(0, 1)->setText(QString::number(nodeSet.size()));
            tab->item(1, 1)->setText(QString::number(hash[MeshBeam].size()));
            tab->item(2, 1)->setText(QString::number(hash[MeshTri].size()));
            tab->item(3, 1)->setText(QString::number(hash[MeshQuad].size()));
            tab->item(4, 1)->setText(QString::number(hash[MeshTet].size()));
            tab->item(5, 1)->setText(QString::number(hash[MeshHex].size()));
            tab->item(6, 1)->setText(QString::number(hash[MeshWedge].size()));
            tab->item(7, 1)->setText(QString::number(hash[MeshPyramid].size()));
        }
    }
}
