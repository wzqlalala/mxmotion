
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
#include <QButtonGroup>      // 元类型注册函数

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
#include "PreViewportWidget.h"
#include "InteractionCore/Controller/InteractionToolManager.h"
#include <InteractionCore/Tool/EditMeshTool.h>

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
#include "mMeshEditPanel.h"
#include "mMeshEdit.h"
#include "GlobalMeshMessage.h"

using namespace MechPreData;
using namespace MeshPanelData;
using namespace MxFunctions;
using namespace MechCommon;
#include <set>
Q_DECLARE_METATYPE(std::set<int>)
Q_DECLARE_METATYPE(MeshParam)          // 头文件

namespace MechMainWindow
{

    mMeshEditPanel::mMeshEditPanel(QWidget* parent)
    {
        // 创建布局
        auto* mainLay = new QVBoxLayout(this);

        // 创建折叠区域
        m_rangeFold = new FoldSection("网格", this);

        // 创建按钮组
        btnPlaceNode = new QToolButton();
        btnSwapEdge = new QToolButton();
        btnCollpaseEdge = new QToolButton();
        btnSplitEdge = new QToolButton();
        btnCombineTri = new QToolButton();
        btnSplitQuad = new QToolButton();

        // 添加按钮标签
        btnPlaceNode->setText("移动点");
        btnSwapEdge->setText("交换边");
        btnCollpaseEdge->setText("合并边");
        btnSplitEdge->setText("分割边");
        btnCombineTri->setText("合并三角形");
        btnSplitQuad->setText("分割四边形");

        // 将按钮添加进容器
        buttons.push_back(btnPlaceNode);
        buttons.push_back(btnSwapEdge);
        buttons.push_back(btnCollpaseEdge);
        buttons.push_back(btnSplitEdge);
        buttons.push_back(btnCombineTri);
        buttons.push_back(btnSplitQuad);

        // 设置按钮初始状态
        for (auto btn : buttons)
        {
            btn->setFixedWidth(80);
            btn->setCheckable(true);
            btn->setChecked(false);
        }

        m_rangeFold->addRow(btnPlaceNode, btnCollpaseEdge, btnCombineTri);
        m_rangeFold->addRow(btnSwapEdge, btnSplitEdge, btnSplitQuad);
        mainLay->addWidget(m_rangeFold);

        // 连接按钮信号
        connect(btnPlaceNode, SIGNAL(clicked()), this, SLOT(slot_placenode()));		    //移动节点
        connect(btnSwapEdge, SIGNAL(clicked()), this, SLOT(slot_swapedge()));			//交换边
        connect(btnCollpaseEdge, SIGNAL(clicked()), this, SLOT(slot_collapseedge()));	//合并边
        connect(btnSplitEdge, SIGNAL(clicked()), this, SLOT(slot_splitedge()));		    //分割边
        connect(btnCombineTri, SIGNAL(clicked()), this, SLOT(slot_combinetri()));		//三角形单元合并
        connect(btnSplitQuad, SIGNAL(clicked()), this, SLOT(slot_splitquad()));		    //四边形单元分割

        QFrame* horizontalLine = createHorizontalLine();
        mainLay->addWidget(horizontalLine);
        mainLay->addStretch();
    }


    mMeshEditPanel::~mMeshEditPanel()
    {

    }

    void mMeshEditPanel::newMeshPanel()
    {

    }

    void mMeshEditPanel::editMeshPanel(int meshId)
    {
        _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(meshId);

        updatePanel();
    }

    void mMeshEditPanel::getMeshInfoById(const std::vector<int> ids)
    {

    }

    void mMeshEditPanel::updatePanel()
    {
        connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mMeshEditPanel::slot_dealPickedFinished);
    }

    void mMeshEditPanel::slot_dealPickedFinished()
    {
        auto pickEdgeIDs = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshLine);
        auto pickEleIDs = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);

        GlobalMeshMessage::getInstance()->setMeshData(getCVM->getModelData()->getMeshData());

        if (function_flag == 0)
        {
            return;
        }
        else if (function_flag == 2)//交换边     
        {
            // 获取拾取的网格边
            if (pickEdgeIDs.size() == 1)
            {
                // 执行翻转
                int num = MeshEditToSwapEdge(pickEdgeIDs[0].data_id.node_id, pickEdgeIDs[0].data_index.line_index, pickEdgeIDs[0].part_id);
                // 翻转错误进行提示
                if (num == 1)
                {
                    QString msg = "不能翻转，该边为自由边！";
                    mGlobalSignals::getInstance()->outputMessageSig(msg, true);
                }
                else if (num == 2)
                {
                    QString msg = "不能翻转，该边为非流形边！";
                    mGlobalSignals::getInstance()->outputMessageSig(msg, true);
                }
                else if (num == 3)
                {
                    QString msg = "不能翻转，翻转后单元会重叠！";
                    mGlobalSignals::getInstance()->outputMessageSig(msg, true);
                }
                else if (num == 4)
                {
                    QString msg = "不能翻转，不兼容的单元法线导致此边无法进行交换！";
                    mGlobalSignals::getInstance()->outputMessageSig(msg, true);
                }
                else if (num == 5)
                {
                    QString msg = "不能翻转，该边连接的两个单元均为四边形单元！";
                    mGlobalSignals::getInstance()->outputMessageSig(msg, true);
                }
            }
        }
        else if (function_flag == 3)//合并边
        {
            // 获取拾取的网格边
            if (pickEdgeIDs.size() == 1)
            {
                // 执行翻转
                int num = MeshEditToCollapseEdge(pickEdgeIDs[0].data_id.node_id, pickEdgeIDs[0].data_index.line_index, pickEdgeIDs[0].part_id);
            }
        }
        else if (function_flag == 4)//分割边
        {
            // 获取拾取的网格边
            if (pickEdgeIDs.size() == 1)
            {
                // 执行翻转
                int num = MeshEditToSplitEdge(pickEdgeIDs[0].data_id.node_id, pickEdgeIDs[0].data_index.line_index, pickEdgeIDs[0].part_id);

                // 分割错误进行提示
                if (num == 1)
                {
                    QString msg = "不能分割，该边为非流形边！";
                    mGlobalSignals::getInstance()->outputMessageSig(msg, true);
                }
            }
        }
        else if (function_flag == 5)//合并三角形
        {
            // 获取拾取的网格边
            if (pickEdgeIDs.size() == 1)
            {
                // 执行翻转
                int num = MeshEditToCombineTriangle(pickEdgeIDs[0].data_id.node_id, pickEdgeIDs[0].data_index.line_index, pickEdgeIDs[0].part_id);

                // 合并错误进行提示
                if (num == 1)
                {
                    QString msg = "不能合并，该边为自由边！";
                    mGlobalSignals::getInstance()->outputMessageSig(msg, true);
                }
                else if (num == 2)
                {
                    QString msg = "不能合并，该边为非流形边！";
                    mGlobalSignals::getInstance()->outputMessageSig(msg, true);
                }
                else if (num == 3)
                {
                    QString msg = "不能合并，该边连接的两个单元并不都是三角形单元！";
                    mGlobalSignals::getInstance()->outputMessageSig(msg, true);
                }
                else if (num == 4)
                {
                    QString msg = "不能合并，合并后会产生退化的四边形单元！";
                    mGlobalSignals::getInstance()->outputMessageSig(msg, true);
                }
                else if (num == 5)
                {
                    QString msg = "不能合并，不兼容的单元法线导致此边无法进行合并！";
                    mGlobalSignals::getInstance()->outputMessageSig(msg, true);
                }

            }
        }
        else if (function_flag == 6)//分割四边形
        {
            // 获取拾取的网格边
            if (pickEleIDs.size() == 1)
            {
                // 执行翻转
                int num = MeshEditToSplitQuadrangle(pickEleIDs[0].data_id.element_id, pickEleIDs[0].part_id);

            }
        }

        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    // 退出网格编辑面板，清除按钮状态
    void mMeshEditPanel::clearCheck()
    {
        for (auto btn : buttons)
        {
            btn->setChecked(false);
        }

        function_flag = 0;
        if(getCVM->getPreRenderWidget())
            disconnect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mMeshEditPanel::slot_dealPickedFinished);
        
    }

    void mMeshEditPanel::slot_placenode()
    {
        if (function_flag != 1)
        {
            if (function_flag != 0)
            {
                buttons[function_flag - 1]->setChecked(false);
            }

            function_flag = 1;
            btnPlaceNode->setChecked(true);

            getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(mxr::InteractionToolType::MeshEdit);

            mMechGlobalSignals::getInstance()->activePickSig(5);
            QString msg = "当前选中节点移动功能！";
            mGlobalSignals::getInstance()->outputMessageSig(msg, true);
        }
        else
        {
            getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(mxr::InteractionToolType::None);

            buttons[function_flag - 1]->setChecked(false);
            function_flag = 0;
            QString msg = "已退出节点移动功能！";
            mGlobalSignals::getInstance()->outputMessageSig(msg, true);
        }
    }

    void mMeshEditPanel::slot_swapedge()
    {
        if (function_flag != 2)
        {
            if (function_flag == 1)
            {
                getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(mxr::InteractionToolType::None);
            }

            if (function_flag != 0)
            {
                buttons[function_flag - 1]->setChecked(false);
            }
            
            function_flag = 2;
            btnSwapEdge->setChecked(true);

            mMechGlobalSignals::getInstance()->activePickSig(6);
            QString msg = "当前选中交换边功能！";
            mGlobalSignals::getInstance()->outputMessageSig(msg, true);
        }
        else
        {
            buttons[function_flag - 1]->setChecked(false);
            function_flag = 0;
            QString msg = "已退出交换边功能！";
            mGlobalSignals::getInstance()->outputMessageSig(msg, true);
        }
    }

    void mMeshEditPanel::slot_collapseedge()
    {
        if (function_flag != 3)
        {
            if (function_flag == 1)
            {
                getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(mxr::InteractionToolType::None);
            }

            if (function_flag != 0)
            {
                buttons[function_flag - 1]->setChecked(false);
            }

            function_flag = 3;
            btnCollpaseEdge->setChecked(true);

            mMechGlobalSignals::getInstance()->activePickSig(6);
            QString msg = "当前选中合并边功能！";
            mGlobalSignals::getInstance()->outputMessageSig(msg, true);
        }
        else
        {
            buttons[function_flag - 1]->setChecked(false);
            function_flag = 0;
            QString msg = "已退出合并边功能！";
            mGlobalSignals::getInstance()->outputMessageSig(msg, true);
        }
    }

    void mMeshEditPanel::slot_splitedge()
    {
        if (function_flag != 4)
        {
            if (function_flag == 1)
            {
                getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(mxr::InteractionToolType::None);
            }

            if (function_flag != 0)
            {
                buttons[function_flag - 1]->setChecked(false);
            }

            function_flag = 4;
            btnSplitEdge->setChecked(true);

            mMechGlobalSignals::getInstance()->activePickSig(6);
            QString msg = "当前选中分割边功能！";
            mGlobalSignals::getInstance()->outputMessageSig(msg, true);
        }
        else
        {
            buttons[function_flag - 1]->setChecked(false);
            function_flag = 0;
            QString msg = "已退出分割边功能！";
            mGlobalSignals::getInstance()->outputMessageSig(msg, true);
        }
    }

    void mMeshEditPanel::slot_combinetri()
    {
        if (function_flag != 5)
        {
            if (function_flag == 1)
            {
                getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(mxr::InteractionToolType::None);
            }

            if (function_flag != 0)
            {
                buttons[function_flag - 1]->setChecked(false);
            }

            function_flag = 5;
            btnCombineTri->setChecked(true);

            mMechGlobalSignals::getInstance()->activePickSig(6);
            QString msg = "当前选中合并三角形单元功能！";
            mGlobalSignals::getInstance()->outputMessageSig(msg, true);
        }
        else
        {
            buttons[function_flag - 1]->setChecked(false);
            function_flag = 0;
            QString msg = "已退出合并三角形单元功能！";
            mGlobalSignals::getInstance()->outputMessageSig(msg, true);
        }
    }

    void mMeshEditPanel::slot_splitquad()
    {
        if (function_flag != 6)
        {
            if (function_flag == 1)
            {
                getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(mxr::InteractionToolType::None);
            }

            if (function_flag != 0)
            {
                buttons[function_flag - 1]->setChecked(false);
            }

            function_flag = 6;
            btnSplitQuad->setChecked(true);

            mMechGlobalSignals::getInstance()->activePickSig(8);

            QString msg = "当前选中分割四边形单元功能！";
            mGlobalSignals::getInstance()->outputMessageSig(msg, true);
        }
        else
        {
            buttons[function_flag - 1]->setChecked(false);
            function_flag = 0;
            QString msg = "已退出分割四边形单元功能！";
            mGlobalSignals::getInstance()->outputMessageSig(msg, true);
        }
    }
}