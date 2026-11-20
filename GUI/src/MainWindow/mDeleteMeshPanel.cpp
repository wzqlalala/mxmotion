//Mech
#include <mMeshData.h>
#include "mDeleteMeshPanel.h"
#include "mMeshPanel.h"
#include "mMechGlobalSignals.h"
#include "PickCommonFunction.h"

using namespace MechPreData;
using namespace MeshPanelData;


namespace MechMainWindow
{
    mDeleteMeshPanel::mDeleteMeshPanel(QWidget* parent) : mMeshPanel(parent)
    {
        //主布局
        auto* mainLay = new QVBoxLayout(this);

        //对象折叠部分
        FoldSection* m_ObjectFold = new FoldSection("对象", this);
        posPick.initWidget("对象");
        posPick.setState(false);
        posPick.labelpicked->setText(QString("当前未选择"));
        m_ObjectFold->addRow(posPick.propwid);

        // 参数折叠部分
        FoldSection* m_paraFold = new FoldSection("参数", this);
        QLabel* part = new PanelLabel("选择对象类型");
        partBox = new QComboBox();
        partBox->addItems({ "单元","部件" });
        m_paraFold->addRow(part, partBox);

        // 水平分割线
        QFrame* horizontalLine = createHorizontalLine();

        //删除按钮
        QHBoxLayout* HLayout = new QHBoxLayout;
        QPushButton* deleteBtn = new QPushButton("删除");
        HLayout->addStretch();
        HLayout->addWidget(deleteBtn);

        // 子布局添加进主布局
        mainLay->addWidget(m_ObjectFold);
        mainLay->addWidget(m_paraFold);
        mainLay->addWidget(horizontalLine);
        mainLay->addLayout(HLayout);
        mainLay->addStretch();

        //连接信号与槽函数
        connect(posPick.btnok, &QToolButton::clicked, this, [=]() {
            if (partBox->currentIndex() == 0)//对象类型为单元
            {
                // 获得拾取的网格单元ID
                MeshIDs.clear();
                std::vector<MeshSelectInfoData> pickMeshIDs = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
                // 将MeshSelectInfoData转化为RenderPickResultData
                for (auto mesh : pickMeshIDs)
                {
                    RenderPickResultData rel = SelectData2ResultData(SelectionInfoType::MeshElement, mesh);
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
            }
            else if (partBox->currentIndex() == 1)//对象类型为部件
            {
                // 获得拾取的网格部件ID
                PartIDs.clear();
                std::vector<MeshSelectInfoData> pickPartIDs = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshPart);
                // 将MeshSelectInfoData转化为RenderPickResultData
                for (auto part : pickPartIDs)
                {
                    RenderPickResultData rel = SelectData2ResultData(SelectionInfoType::MeshPart, part);
                    PartIDs.push_back(rel);
                }
                if (!PartIDs.empty())
                {
                    posPick.setState(true);
                    posPick.labelpicked->setText(QString("已选择:%1个部件").arg(pickPartIDs.size()));
                }
                else
                    posPick.setState(false);
                _meshOptPanelData->setMeshIDs(std::move(PartIDs));
            }
            });
        connect(posPick.btnadd, &QToolButton::clicked, this, [=]() {
            if (partBox->currentIndex() == 0)
            {
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
                _meshOptPanelData->setMeshIDs(MeshIDs);
            }
            else if (partBox->currentIndex() == 1)
            {
                // 获得拾取的网格部件ID，追加到PartIDs
                std::vector<MeshSelectInfoData> pickPartIDs = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshPart);
                // 将MeshSelectInfoData转化为RenderPickResultData
                for (auto ele : pickPartIDs)
                {
                    RenderPickResultData rel = SelectData2ResultData(SelectionInfoType::MeshPart, ele);
                    pushBackUnique(PartIDs, rel);
                }
                if (!PartIDs.empty())
                {
                    posPick.setState(true);
                    posPick.labelpicked->setText(QString("已选择:%1个部件").arg(PartIDs.size()));
                }
                else
                    posPick.setState(false);
                _meshOptPanelData->setMeshIDs(PartIDs);
            }
            });
        connect(posPick.btnclear, &QToolButton::clicked, this, [=]() {
            MeshIDs.clear();
            PartIDs.clear();
            posPick.setState(false);
            _meshOptPanelData->setMeshIDs(std::move(MeshIDs));
            });
        connect(partBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
            if (index == 0)
            {
                MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8);
                /*PartIDs.clear();
                posPick.setState(false);*/
                posPick.setState(false);//切换选择对象时清掉所选状态,但不能清除留存数据避免影响模型树切换
                _meshOptPanelData->setPickType(partBox->currentText());
            }
            else if (index == 1)
            {
                MechCommon::mMechGlobalSignals::getInstance()->activePickSig(9);
                /*MeshIDs.clear();*/
                posPick.setState(false);//切换选择对象时清掉所选状态，但不能清除留存数据避免影响模型树切换
                _meshOptPanelData->setPickType(partBox->currentText());
            }
            });
        connect(deleteBtn,&QPushButton::clicked,this,&mDeleteMeshPanel::deletemesh);
    }

    mDeleteMeshPanel::~mDeleteMeshPanel()
    {

    }

    void mDeleteMeshPanel::deletemesh()
    {
        // 获取模型数据与网格数据
        MechModelData::mModelData* modelData = getCVM->getModelData();
        if (!modelData)
        {
            return;
        }
        mMeshData* meshdata = modelData->getMeshData();
        if (!meshdata)
        {
            return;
        }
        if (partBox->currentIndex() == 0)
        {
            if (MeshIDs.empty()) return;
            //过滤已失效的单元ID，避免deleteMesh内部计数为负导致崩溃
            std::vector<RenderPickResultData> validMeshIDs;
            for (const auto& mesh : MeshIDs)
            {
                mPartData* part = meshdata->getPartData(mesh.part_id);//根据部件id获取部件数据
                if (part && part->_parteledata.elementidmap.contains(mesh.result_id))
                    validMeshIDs.push_back(mesh);
            }
            if (validMeshIDs.empty()) return; 
            //还得增加一种情况，即所选的单元刚好覆盖了整个部件，应执行的是删除网格部件

            //QString partname = meshdata->getSingleRPRDByPartName(validMeshIDs[0]);//通过RenderPickResultData获取部件名
            ////auto part = meshdata->getSingleRPRDByPartName(partname);//通过部件名获取部件id RenderPickResultData
            //std::set<QString> names;
            //names.insert(partname);
            //vector<RenderPickResultData> partInfo = meshdata->getRPRDbyPartName(names);//通过部件名获取RenderPickResultData

            ////然后如果部件中的每个单元都能在MeshIDs中找到，说明就是要用删除部件的方式去删除,判断一下其是否为真
            //bool is_find = true;
            //for (int i = 0; i < partInfo.size(); ++i)
            //{
            //    if (is_find == false) break;
            //    for (int j = 0; j < validMeshIDs.size(); ++j)
            //    {
            //        if (partInfo[i] == validMeshIDs[j])
            //        {
            //            is_find = true;
            //            break;
            //        }
            //        else is_find = false;
            //    }
            //}
            //if (is_find ==false) meshdata->deleteMesh(validMeshIDs);
            //else if (is_find==true) meshdata->deleteMeshPart(partname);
            
            std::map<QString, std::vector<RenderPickResultData>> groupByPart;
            for (const auto& mesh : validMeshIDs)
            {
                QString partname = meshdata->getSingleRPRDByPartName(mesh);//通过RenderPickResultData获取部件名
                groupByPart[partname].push_back(mesh);
            }
            for (auto it = groupByPart.begin(); it != groupByPart.end(); ++it)
            {
                const QString& partname = it->first;
                std::set<QString> names{ partname };//std::set<QString> names;names.insert(partname);相当于这两行的合并
                std::vector<RenderPickResultData> partInfo = meshdata->getRPRDbyPartName(names);//通过部件名获取该部件全部单元

                //该部件的每个单元都在所选列表中才算覆盖
                bool isFullPart = std::all_of(partInfo.begin(), partInfo.end(),
                    [&eleIDs = it->second](const RenderPickResultData& pi) {
                        return std::find(eleIDs.begin(), eleIDs.end(), pi) != eleIDs.end();
                    });//后两行是一个lambda表达式

                if (isFullPart) meshdata->deleteMeshPart(partname);
                else meshdata->deleteMesh(it->second);
            }
        }
        else if (partBox->currentIndex() == 1)
        {
            if (PartIDs.empty()) return;
            for (auto part : PartIDs)
            {
                QString partname = meshdata->getPartNameByPartId(part.part_id);//根据部件ID获取部件名称
                meshdata->deleteMeshPart(partname);
            }
        }
        //getCVM->clearPick();
        //按 slot_deleteMeshAuto 的流程清理残留，避免下次拾取/树刷新访问失效ID
        getCVM->clearPick();
        getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightGeoModel();
        //同步清空面板数据和界面状态，防止 updatePanel 把已删除的ID灌回来
        MeshIDs.clear();
        PartIDs.clear();
        _meshOptPanelData->setMeshIDs({});
        posPick.setState(false);
    }

    void mDeleteMeshPanel::editMeshPanel(int meshId)
    {
        _meshOptPanelData = getCVM->getModelData()->getManager<mMeshOptPanelManager>(Physical_MeshOptPanel).getMeshPanelData(meshId);

        updatePanel();
    }

    void mDeleteMeshPanel::updatePanel()
    {
        if (_meshOptPanelData->getPickType() == "单元" || _meshOptPanelData->getPickType() == "全部")
        {
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8);

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

            partBox->setCurrentIndex(0);
            
        }
        else if(_meshOptPanelData->getPickType() == "部件" || _meshOptPanelData->getPickType() == "全部")
        {
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(9);

            PartIDs = _meshOptPanelData->getMeshIDs();
            if (PartIDs.empty())
            {
                posPick.setState(false);
            }
            else
            {
                posPick.setState(true);
                posPick.labelpicked->setText(QString("已选择:%1个部件").arg(PartIDs.size()));
            }

            partBox->setCurrentIndex(1);
        }
    }
}