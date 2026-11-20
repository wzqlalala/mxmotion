#include "mChangeOrderPanel.h"
#include "mMeshPanel.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mGlobalSignals.h"

#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QCheckBox>
#include <QToolButton>
#include <QRadioButton>
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
#include "mxr/view/mPreMeshHighLightRender.h"
#include "mxr/view/HighlightController.h"
#include "PickCommonFunction.h"

using namespace MechPreData;
using namespace MeshPanelData;


namespace MechMainWindow
{
    mChangeOrderPanel::mChangeOrderPanel(QWidget* parent) : mMeshPanel(parent)
    {
        auto* mainLay = new QVBoxLayout(this);

        FoldSection* m_ObjectFold = new FoldSection("网格", this);

        posPick.initWidget("对象");
        posPick.setState(false);
        m_ObjectFold->addRow(posPick.propwid);

        connect(posPick.btnok, &QToolButton::clicked, this, [=]()
            {
                // 清空之前存储的网格部件
                MeshIDs.clear();
                // 获得拾取的网格部件
                std::vector<MeshSelectInfoData> pickMeshParts = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshPart);

                // 将MeshSelectInfoData转化为RenderPickResultData
                for (auto part : pickMeshParts)
                {
                    MeshIDs.push_back(SelectData2ResultData(SelectionInfoType::MeshElement, part));
                }

                // 更新拾取控件状态
                if (!MeshIDs.empty())
                {
                    posPick.setState(true);
                    posPick.labelpicked->setText(QString("已选择:%1个网格部件").arg(MeshIDs.size()));
                }
                else
                {
                    posPick.setState(false);
                }

                // 设置拾取的单元
                _meshOptPanelData->setMeshIDs(MeshIDs);
            });

        connect(posPick.btnadd, &QToolButton::clicked, this, [=]()
            {
                // 获得拾取的网格部件ID，追加到MeshPartIds
                std::vector<MeshSelectInfoData> pickMeshParts = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshPart);

                // 将MeshSelectInfoData转化为RenderPickResultData
                for (auto part : pickMeshParts)
                {
                    RenderPickResultData rel = SelectData2ResultData(SelectionInfoType::MeshElement, part);
                    pushBackUnique(MeshIDs, rel);
                }

                // 更新拾取控件状态
                if (!MeshIDs.empty())
                {
                    posPick.setState(true);
                    posPick.labelpicked->setText(QString("已选择:%1个网格部件").arg(MeshIDs.size()));
                }
                else
                {
                    posPick.setState(false);
                }

                _meshOptPanelData->setMeshIDs(MeshIDs);
            });

        connect(posPick.btnclear, &QToolButton::clicked, this, [=]()
            {
                MeshIDs.clear();
                posPick.setState(false);
                _meshOptPanelData->setMeshIDs(MeshIDs);
            });

        m_paramFold = new FoldSection("参数", this);

        QLabel* order = new PanelLabel("阶数");
        orderBox1 = new QRadioButton("一阶");
        orderBox1->setChecked(true);
        orderBox2 = new QRadioButton("二阶");
        m_paramFold->addRow(order, orderBox1, orderBox2);

        QPushButton* btnApply = new QPushButton("应用");
        connect(btnApply, &QPushButton::clicked, this, &mChangeOrderPanel::apply);
        QFrame* horizontalLine = createHorizontalLine();
        mainLay->addWidget(m_ObjectFold);

        mainLay->addWidget(m_paramFold);
        mainLay->addWidget(horizontalLine);
        mainLay->addWidget(btnApply);
        mainLay->addStretch();
    }

    mChangeOrderPanel::~mChangeOrderPanel()
    {

    }

    void mChangeOrderPanel::apply()
    {
        // 如果拾取部件为空，则直接跳过
        if (MeshIDs.empty())
        {
            return;
        }

        // 获取模型数据与网格数据
        MechModelData::mModelData* modelData = getCVM->getModelData();
        mMeshData* meshdata = modelData->getMeshData();

        // 获取拾取的网格部件名称
        set<QString> pickMeshPartNames;
        set<int> OldData, NewData;
        for (auto part : MeshIDs)
        {
            pickMeshPartNames.insert(meshdata->getPartNameByPartId(part.part_id));
        }

        // 一阶
        if (orderBox1->isChecked())
        {
            meshdata->setMeshFirst(pickMeshPartNames, OldData, NewData);
            mGlobalSignals::getInstance()->outputMessageSig(1, tr("一阶变换完成"), true);
        }
        // 二阶
        else if (orderBox2->isChecked())
        {
            meshdata->setMeshSecond(pickMeshPartNames, OldData, NewData);
            mGlobalSignals::getInstance()->outputMessageSig(1, tr("二阶变换完成"), true);
        }

        mGlobalSignals::getInstance()->MeshOrderChangeSig(OldData, NewData);
       
        // 清除拾取与高亮
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
    }
    void mChangeOrderPanel::newMeshPanel()
    {

    }
    void mChangeOrderPanel::editMeshPanel(int meshId)
    {
        _meshOptPanelData = getCVM->getModelData()->getManager<mMeshOptPanelManager>(Physical_MeshOptPanel).getMeshPanelData(meshId);
        updatePanel();
    }
    void mChangeOrderPanel::updatePanel()
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
