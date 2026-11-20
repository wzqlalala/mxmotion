
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
#include <QRadioButton>     

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
#include "mRenumberPanel.h"
#include "GlobalMeshMessage.h"
#include "mMeshTransform.h"
//#include <mGeoInfoCalculate.h> //【OCC摘除·S10/P0-1 终局2】死 include（零使用）
using namespace MechPreData;
using namespace MeshPanelData;
using namespace MxFunctions;
#include <set>
#include "mOtherFunction.h"
Q_DECLARE_METATYPE(std::set<int>)
Q_DECLARE_METATYPE(MeshParam)          // 头文件

namespace MechMainWindow
{

    mRenumberPanel::mRenumberPanel(QWidget* parent)
        : mMeshPanel(parent)
    {
        auto* mainLay = new QVBoxLayout(this);

        FoldSection* m_ObjectFold = new FoldSection("对象", this);
        QLabel* type = new PanelLabel("类型");
        typeBtn1 = new QRadioButton("节点");
        typeBtn2 = new QRadioButton("网格");
        m_ObjectFold->addRow(type, typeBtn1, typeBtn2);
        typeBtn1->setChecked(true);
        posPick.initWidget("范围");
        posPick.setState(false);
        m_ObjectFold->addRow(posPick.propwid);

        m_TranslationFold = new FoldSection("参数", this);
        startId = new PanelLabel("初始ID");
        startIdEdit = new mCalcLineEdit("1", mCalcLineEdit::CalcMode::StrictPositive, this);

        idGap = new PanelLabel("ID间隔");
        idGapEdit = new mCalcLineEdit("1", mCalcLineEdit::CalcMode::StrictPositive, this);
        m_TranslationFold->addRow(startId, startIdEdit);
        m_TranslationFold->addRow(idGap, idGapEdit);

        FoldSection* m_TotalFold = new FoldSection("统计", this);
        QLabel* maxId = new PanelLabel("最大ID");
        maxIdEdit = new QLabel();

        QLabel* minId = new PanelLabel("最小ID");
        minIdEdit = new QLabel();
        m_TotalFold->addRow(maxId, maxIdEdit);
        m_TotalFold->addRow(minId, minIdEdit);

        QPushButton* lockBtn = new QPushButton("锁定");
        QPushButton* unlockBtn = new QPushButton("解锁");
        QPushButton* btn = new QPushButton("重编号");
        QFrame* horizontalLine = createHorizontalLine();
        mainLay->addWidget(m_ObjectFold);
        mainLay->addWidget(m_TranslationFold);
        mainLay->addWidget(m_TotalFold);
        mainLay->addWidget(horizontalLine);
        QHBoxLayout* h = new QHBoxLayout;
        h->addWidget(lockBtn);
        h->addWidget(unlockBtn);
        h->addWidget(btn);
        mainLay->addLayout(h);
        mainLay->addStretch();
        _lockNumber.clear();
        connect(typeBtn1, &QRadioButton::toggled, this, [=](bool checked) {
            if (checked) {
                MechCommon::mMechGlobalSignals::getInstance()->activePickSig(5);
                posPick.setState(false);
                _lockNumber.clear();
                updateMinMaxId();
            }
            });

        connect(typeBtn2, &QRadioButton::toggled, this, [=](bool checked) {
            if (checked) {
                MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8);
                posPick.setState(false);
                _lockNumber.clear();
                updateMinMaxId();
            }
            });
        connect(posPick.btnok, &QToolButton::clicked, this, &mRenumberPanel::replaceObjects);
        connect(posPick.btnclear, &QToolButton::clicked, this, &mRenumberPanel::deleteObjects);
        connect(posPick.btnadd, &QToolButton::clicked, this, &mRenumberPanel::addObjects);
        connect(lockBtn, &QPushButton::clicked, this, &mRenumberPanel::slot_lock);
        connect(unlockBtn, &QPushButton::clicked, this, &mRenumberPanel::slot_unlock);
        connect(btn,&QPushButton::clicked,this,&mRenumberPanel::apply);
    }
    void mRenumberPanel::slot_lock()
    {
        vector<RenderPickResultData> data;
        QString str;
        if (typeBtn1->isChecked())
        {
            str = "节点已上锁！";
            //拾取节点
            std::vector<MeshSelectInfoData> pickMeshIDs;
            pickMeshIDs = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
            for (auto ele : pickMeshIDs)
            {
                RenderPickResultData rel = SelectData2ResultData(SelectionInfoType::MeshNode, ele);
                data.push_back(rel);
            }  
            
        }
        else
        {
            str = "单元已上锁！";
            //拾取单元
            std::vector<MeshSelectInfoData> pickMeshIDs;   
            pickMeshIDs = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
            for (auto ele : pickMeshIDs)
            {
                RenderPickResultData rel = SelectData2ResultData(SelectionInfoType::MeshElement, ele);
                data.push_back(rel);
            }
            
        }
        if (data.empty())
            return;

        for (auto id : data)
        {
            _lockNumber.insert(id.result_id);
        }

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(str);
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
    }
    void mRenumberPanel::slot_unlock()
    {
        vector<RenderPickResultData> data;
        QString str;
        if (typeBtn1->isChecked())
        {
            str = "节点已解锁！";
            //拾取节点
            std::vector<MeshSelectInfoData> pickMeshIDs;
            pickMeshIDs = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
            for (auto ele : pickMeshIDs)
            {
                RenderPickResultData rel = SelectData2ResultData(SelectionInfoType::MeshNode, ele);
                data.push_back(rel);
            }
        }
        else
        {
            str = "单元已解锁！";
            //拾取单元
            std::vector<MeshSelectInfoData> pickMeshIDs;
            pickMeshIDs = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
            for (auto ele : pickMeshIDs)
            {
                RenderPickResultData rel = SelectData2ResultData(SelectionInfoType::MeshElement, ele);
                data.push_back(rel);
            }
        }
        if (data.empty())
            return;

        for (auto id : data)
        {
            _lockNumber.erase(id.result_id);
        }

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(str);
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
    }
    void mRenumberPanel::apply()
    {
        if (MeshIDs.empty())
            return;

        // 校验"初始ID"与"ID间隔"：必须是大于 0 的整数
        auto isPositiveInt = [](const QString& text, int& value) -> bool {
            bool ok = false;
            value = text.toInt(&ok);
            return ok && value > 0;
            };

        int startIdValue = 0;
        if (!isPositiveInt(startIdEdit->text().trimmed(), startIdValue))
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, QStringLiteral("初始ID输入无效，请输入大于0的整数！"));
            return;
        }
        int idGapValue = 0;
        if (!isPositiveInt(idGapEdit->text().trimmed(), idGapValue))
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, QStringLiteral("ID间隔输入无效，请输入大于0的整数！"));
            return;
        }

        GlobalMeshMessage::getInstance()->setMeshData(getCVM->getModelData()->getMeshData());
        //节点
        if (typeBtn1->isChecked())
        {
            QHash<int, int> oldToNewNodes = getCVM->getModelData()->getMeshData()->nodeRenumber(startIdValue, idGapValue, _lockNumber, MeshIDs);
            if (oldToNewNodes.empty())
                return;

            this->updateNodeLabel(oldToNewNodes);
        }//单元
        else
        {
            QHash<int, QHash<int, int>> oldToNewElements = getCVM->getModelData()->getMeshData()->elementRenumber(startIdValue, idGapValue, _lockNumber, MeshIDs);
            if (oldToNewElements.empty())
                return;
            this->updateMeshLabel(oldToNewElements);
        }
        // 清除拾取与高亮
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
        posPick.btnclear->click();

        updateMinMaxId();
    }

    mRenumberPanel::~mRenumberPanel()
    {

    }
    void mRenumberPanel::newMeshPanel()
    {

    }
    void mRenumberPanel::editMeshPanel(int meshId)
    {
        _meshOptPanelData = getCVM->getModelData()->getManager<mMeshOptPanelManager>(Physical_MeshOptPanel).getMeshPanelData(meshId);

        updatePanel();
        updateMinMaxId();
    }
    void mRenumberPanel::getMeshInfoById(const std::vector<int> ids)
    {

    }
    void mRenumberPanel::updatePanel()
    {

    }

    void mRenumberPanel::updateMinMaxId()
    {
        if (!maxIdEdit || !minIdEdit)
            return;

        int maxId = -1;
        int minId = -1;
        auto modelData = getCVM->getModelData();
        if (!modelData || !modelData->getMeshData())
        {
            maxIdEdit->setText("");
            minIdEdit->setText("");
            return;
        }

        if (typeBtn1->isChecked())
        {
            std::set<int> nodeIds;
            QVector<QString> partNames = modelData->getMeshData()->getAllShowPartNames();
            for (const QString& partName : partNames)
            {
                std::set<int> partNodeIds = modelData->getMeshData()->getAllNodeIDsInPart(partName);
                nodeIds.insert(partNodeIds.begin(), partNodeIds.end());
            }
            if (!nodeIds.empty())
            {
                minId = *nodeIds.begin();
                maxId = *nodeIds.rbegin();
            }
        }
        else
        {
            std::vector<RenderPickResultData> elements = modelData->getMeshData()->getAllElements();
            if (!elements.empty())
            {
                minId = elements.front().result_id;
                maxId = elements.front().result_id;
                for (const auto& ele : elements)
                {
                    if (ele.result_id < minId)
                        minId = ele.result_id;
                    if (ele.result_id > maxId)
                        maxId = ele.result_id;
                }
            }
        }

        if (maxId < 0 || minId < 0)
        {
            maxIdEdit->setText("");
            minIdEdit->setText("");
        }
        else
        {
            maxIdEdit->setText(MxFunctions::mNumberToString(maxId));
            minIdEdit->setText(MxFunctions::mNumberToString(minId));
        }
    }

    void mRenumberPanel::setMeshSize()
    {

    }
    void mRenumberPanel::initSize()
    {

    }
  
    void mRenumberPanel::replaceObjects()
    {
  
        //拾取节点
        MeshIDs.clear();
        std::vector<MeshSelectInfoData> pickMeshIDs;
        if (typeBtn1->isChecked())
        {
            pickMeshIDs = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
            for (auto ele : pickMeshIDs)
            {
                RenderPickResultData rel = SelectData2ResultData(SelectionInfoType::MeshNode, ele);
                MeshIDs.push_back(rel);
            }
            if (!MeshIDs.empty())
            {
                posPick.setState(true);
                posPick.labelpicked->setText(QString("已选择:%1个节点").arg(MeshIDs.size()));
            }
            else
                posPick.setState(false);
        }
        else
        {
            pickMeshIDs = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
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
        }
        // 将MeshSelectInfoData转化为RenderPickResultData        
        _meshOptPanelData->setMeshIDs(std::move(MeshIDs));
        
    }
    void mRenumberPanel::addObjects()
    {
        // 根据当前类型分别拾取节点或单元，追加到MeshIDs
        std::vector<MeshSelectInfoData> pickMeshIDs;
        SelectionInfoType selType = typeBtn1->isChecked() ? SelectionInfoType::MeshNode : SelectionInfoType::MeshElement;
        pickMeshIDs = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(selType);

        // 将MeshSelectInfoData转化为RenderPickResultData
        for (auto ele : pickMeshIDs)
        {
            RenderPickResultData rel = SelectData2ResultData(selType, ele);
            pushBackUnique(MeshIDs, rel);
        }
        if (!MeshIDs.empty())
        {
            posPick.setState(true);
            QString typeStr = typeBtn1->isChecked() ? "节点" : "单元";
            posPick.labelpicked->setText(QString("已选择:%1个%2").arg(MeshIDs.size()).arg(typeStr));
        }
        else
            posPick.setState(false);
        _meshOptPanelData->setMeshIDs(std::move(MeshIDs));
    }
    void mRenumberPanel::deleteObjects()
    {
        MeshIDs.clear();
        posPick.setState(false);
        _meshOptPanelData->setMeshIDs(std::move(MeshIDs));    
    }
    void mRenumberPanel::updateNodeLabel(QHash<int, int> oldToNewNode)
    {
        QHash<int, QVector3D>& _nodeLabel = VMManager->getCurrentViewportPrivate()->_nodeIdVertexs;
        if (_nodeLabel.empty())
            return;

        for (auto it = oldToNewNode.begin(); it != oldToNewNode.end(); ++it)
        {
            if (_nodeLabel.constFind(it.key()) != _nodeLabel.cend())
            {
                _nodeLabel.insert(it.value(), _nodeLabel[it.key()]);
                _nodeLabel.remove(it.key());
            }
        }
        updateNodeNum();
    }
    void mRenumberPanel::updateMeshLabel(QHash<int, QHash<int, int>> oldToNewNode)
    {
        QHash<int, QVector3D>& _eleLabel = VMManager->getCurrentViewportPrivate()->_eleIdVertexs;
        if (_eleLabel.empty())
            return;
        MechModelData::mModelData* modelData = getCVM->getModelData();
        for (auto it = oldToNewNode.begin(); it != oldToNewNode.end(); ++it)
        {
            for (auto ID = it.value().begin(); ID != it.value().end(); ++ID)
            {
                if (_eleLabel.constFind(ID.key()) != _eleLabel.cend())
                {
                    RenderPickResultData data;
                    data.part_id = it.key();
                    data.result_id = ID.value();
                    _eleLabel[ID.value()] = modelData->getMeshData()->getCentroid(data);
                    _eleLabel.remove(ID.key());
                }
            }
        }

        updateEleNum();
    }
    void mRenumberPanel::updateNodeNum()
    {
        QHash<int, QVector3D> _nodeLabel = VMManager->getCurrentViewportPrivate()->_nodeIdVertexs;
        QVector<QVector3D> pos;
        QVector<QString> value;
        QHashIterator<int, QVector3D> iter(_nodeLabel);
        while (iter.hasNext())
        {
            iter.next();
            pos.append(iter.value());
            value.append(QString("%1").arg(iter.key()));
        }

        CCSMRendWidget* ccsmRendWidget = getCVM->getPreRenderWidget();
        if (ccsmRendWidget)
        {
            shared_ptr<mFontRender> fontRender = ccsmRendWidget->getFontRender();
            fontRender->appendCommonFont("NodeID", pos, value, QVector3D(41 / 255.0, 157 / 255.0, 1));
            ccsmRendWidget->update();
        }
    }
    void mRenumberPanel::updateEleNum()
    {
        QHash<int, QVector3D>& _eleLabel = VMManager->getCurrentViewportPrivate()->_eleIdVertexs;
        QVector<QVector3D> pos;
        QVector<QString> value;
        QHashIterator<int, QVector3D> iter(_eleLabel);
        while (iter.hasNext())
        {
            iter.next();
            pos.append(iter.value());
            value.append(QString("%1").arg(iter.key()));
        }

        CCSMRendWidget* ccsmRendWidget = getCVM->getPreRenderWidget();
        if (ccsmRendWidget)
        {
            shared_ptr<mFontRender> fontRender = ccsmRendWidget->getFontRender();
            fontRender->appendCommonFont("EleID", pos, value, QVector3D(157 / 255.0, 41 / 255.0, 1));
            ccsmRendWidget->update();
        }
    }
}
