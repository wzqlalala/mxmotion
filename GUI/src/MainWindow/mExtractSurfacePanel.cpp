//BasicData
#include "mMeshData.h"

//Mech
#include "mMeshPanel.h"
#include "PickCommonFunction.h"
#include "mExtractSurfacePanel.h"
#include "mMechGlobalSignals.h"

using namespace MechPreData;
using namespace MeshPanelData;

namespace MechMainWindow
{
    mExtractSurfacePanel::mExtractSurfacePanel(QWidget* parent)
    {
        auto* mainLay = new QVBoxLayout(this);
        
        m_rangeFold = new FoldSection("网格", this);
        _meshPartLabel = new PanelLabel("所属部件");
        _meshPartBox = new QComboBox();
        _meshPartBox->addItem("新建");
        _meshPartEdit = new QLineEdit();
        _meshPartEdit->setPlaceholderText("输入新部件名称");

        QLabel* shareNodeLabel = new PanelLabel("共节点");
        _shareNodeYes = new QRadioButton("是");
        _shareNodeNo = new QRadioButton("否");
        _shareNodeYes->setChecked(true);

        posPick.initWidget("表面网格");
        posPick.setState(false);

        m_rangeFold->addRow(posPick.propwid);
        m_rangeFold->addRow(_meshPartLabel, _meshPartBox, _meshPartEdit);
        m_rangeFold->addRow(shareNodeLabel, _shareNodeYes, _shareNodeNo);
        
        QFrame* horizontalLine = createHorizontalLine();

        QHBoxLayout* HLayout = new QHBoxLayout;
        QPushButton* btn_create = new QPushButton("创建");
        HLayout->addStretch();
        HLayout->addWidget(btn_create);

        mainLay->addWidget(m_rangeFold);
        mainLay->addWidget(horizontalLine);
        mainLay->addLayout(HLayout);
        mainLay->addStretch();

        connect(_meshPartBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
            // 新建部件时才需要输入名称，选已有部件时输入框置灰
            _meshPartEdit->setEnabled(index == 0);
            });

        connect(posPick.btnok, &QToolButton::clicked, this, [=]()
            {
                //拾取单元面
                MeshIDs.clear();
                std::vector<MeshSelectInfoData> pickMeshIDs = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshFace);

                for (auto ele : pickMeshIDs)
                {
                    RenderPickResultData rel = SelectData2ResultData(SelectionInfoType::MeshFace, ele);
                    MeshIDs.push_back(rel);
                }
                if (!MeshIDs.empty())
                {
                    posPick.setState(true);
                    posPick.labelpicked->setText(QString("已选择:%1个单元面").arg(MeshIDs.size()));
                }
                else
                {
                    posPick.setState(false);
                }
                    
                if (_meshOptPanelData != nullptr)
                {
                    _meshOptPanelData->setMeshIDs(std::move(MeshIDs));
                }
                    
            });
        connect(posPick.btnclear, &QToolButton::clicked, this, [=]()
            {
                MeshIDs.clear();
                posPick.setState(false);
                if (_meshOptPanelData != nullptr)
                {
                    _meshOptPanelData->setMeshIDs(std::move(MeshIDs));
                }
            });
        connect(posPick.btnadd, &QToolButton::clicked, this, [=]()
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
                    posPick.labelpicked->setText(QString("已选择:%1个单元面").arg(MeshIDs.size()));
                }
                else
                {
                    posPick.setState(false);
                }
                    
                if (_meshOptPanelData != nullptr)
                {
                    _meshOptPanelData->setMeshIDs(std::move(MeshIDs));
                }
            });

        connect(_shareNodeYes, &QRadioButton::clicked, this, [=]()
            {
                _meshOptPanelData->setCoNodes(true);
            });

        connect(_shareNodeNo, &QRadioButton::clicked, this, [=]()
            {
                _meshOptPanelData->setCoNodes(false);
            });

        connect(btn_create,&QPushButton::clicked,this,&mExtractSurfacePanel::apply);
    }

    mExtractSurfacePanel::~mExtractSurfacePanel()
    {

    }

    void mExtractSurfacePanel::newMeshPanel()
    {
        // 面板是全局单例，每次新建都要重置上次的拾取状态
        MeshIDs.clear();
        posPick.setState(false);
        _meshOptPanelData = nullptr;
        refreshPartCombo();
        // 进入面板自动切换为拾取单元面（7 = PickMeshFace）
        emit MechCommon::mMechGlobalSignals::getInstance()->activePickSig(7);
    }
    void mExtractSurfacePanel::editMeshPanel(int meshId)
    {
        _meshOptPanelData = getCVM->getModelData()->getManager<mMeshOptPanelManager>(Physical_MeshOptPanel).getMeshPanelData(meshId);
        // 进入面板自动切换为拾取单元面（7 = PickMeshFace）
        emit MechCommon::mMechGlobalSignals::getInstance()->activePickSig(7);
        updatePanel();
    }
    void mExtractSurfacePanel::updatePanel()
    {
        // 刷新部件下拉框
        refreshPartCombo();

        // 切回面板时重置拾取显示，避免沿用上次的状态
        MeshIDs = _meshOptPanelData->getMeshIDs();
        if (MeshIDs.empty())
        {
            posPick.setState(false);
        }
        else
        {
            posPick.setState(true);
            posPick.labelpicked->setText(QString("已选择:%1个单元面").arg(MeshIDs.size()));
        }

        bool coNodes = _meshOptPanelData->getCoNodes();
        if (coNodes == true)
        {
            _shareNodeYes->setChecked(true);
            _shareNodeNo->setChecked(false);
        }
        else
        {
            _shareNodeYes->setChecked(false);
            _shareNodeNo->setChecked(true);
        }
    }

    void mExtractSurfacePanel::apply()
    {
        //获取零件名
        QString pname;
        if (_meshPartBox->currentIndex() == 0)
        {
            pname = _meshPartEdit->text().simplified();
            if (pname.isEmpty())
            {
                QString msg = ("请输入零件名");
                mGlobalSignals::getInstance()->outputMessageSig(ErrorMessage, msg, true);
                return;
            }
            //判断零件存不存在
            if (getCVM->getModelData()->getMeshData()->havePart(pname))
            {
                QString msg = ("该零件名已经存在");
                mGlobalSignals::getInstance()->outputMessageSig(1, msg, true);
                return;
            }
        }
        else
        {
            //选择已有部件，直接追加
            pname = _meshPartBox->currentText();
        }
        //获取高亮的网格表面
        std::vector<RenderPickResultData>ids = MeshIDs;
        if (ids.size() == 0)
        {
            QString msg =("请拾取或选择网格表面");
            mGlobalSignals::getInstance()->outputMessageSig(1, msg, true);
            return;
        }

        //是否共节点（是：与原体网格共享节点；否：复制独立节点）
        bool shareNode = _shareNodeYes->isChecked();

        getCVM->getModelData()->getMeshData()->createMeshBySurface(ids, pname, shareNode);
        // 新建部件才通知模型树创建节点；追加到已有部件时节点已存在，无需再加
        if (_meshPartBox->currentIndex() == 0)
        {
            vector<int> partIds = getCVM->getModelData()->getAllPhyID(Physical_Part);
            emit mGlobalSignals::getInstance()->createUpdatePartSig(true, set<int>{partIds.end() - 1, partIds.end()});
        }
        mGlobalSignals::getInstance()->updatePartInfoSig();

        //清空高亮与拾取
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
        refreshPartCombo();

        //输出信息
        QString msg = "成功提取表面网格";
        mGlobalSignals::getInstance()->outputMessageSig(msg, true);
    }

    void mExtractSurfacePanel::refreshPartCombo()
    {
        _meshPartBox->clear();
        _meshPartBox->addItem("新建");
        QStringList partNames = getCVM->getModelData()->getMeshData()->getAllPartNames();
        _meshPartBox->addItems(partNames);
        _meshPartBox->setCurrentIndex(0);
        _meshPartEdit->setEnabled(true);
        _meshPartEdit->clear();
    }
}
