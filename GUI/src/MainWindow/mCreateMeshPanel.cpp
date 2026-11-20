//Mech
#include "mMeshPanel.h"
#include "mGlobalSignals.h"
#include "mOtherFunction.h"
#include "PickCommonFunction.h"
#include "mCreateMeshPanel.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/view/HighlightController.h"

//BasicData
#include "mMeshData.h"
#include "mModelData.h"

using namespace MechPreData;
using namespace MeshPanelData;
using namespace MxFunctions;

namespace MechMainWindow
{
    // 形状下拉索引 -> 所需节点数 / 单元类型（与构造中 _meshShapeBox 的添加顺序一致）
    static const QMap<int, int>& shapeNodeCountMap()
    {
        static const QMap<int, int> map = {
            {0, 1},  // 点
            {1, 2},  // 线
            {2, 3},  // 三角形
            {3, 4},  // 四边形
            {4, 4},  // 四面体
            {5, 8},  // 六面体
            {6, 6}   // 三棱柱
        };
        return map;
    }
    static const QMap<int, ElementType>& shapeElementTypeMap()
    {
        static const QMap<int, ElementType> map = {
            {0, Mass1},
            {1, BS2},
            {2, CTRIA3},
            {3, Q4R},
            {4, D3T4},
            {5, D3H8},
            {6, D3P6}
        };
        return map;
    }

    mCreateMeshPanel::mCreateMeshPanel(QWidget* parent)
    {
        _imageMap[0] = ":/mMainWidgets/Icon/Mesh/01.png";
        _imageMap[1] = ":/mMainWidgets/Icon/Mesh/02.png";
        _imageMap[2] = ":/mMainWidgets/Icon/Mesh/03.png";
        _imageMap[3] = ":/mMainWidgets/Icon/Mesh/04.png";
        _imageMap[4] = ":/mMainWidgets/Icon/Mesh/04体.png";
        _imageMap[5] = ":/mMainWidgets/Icon/Mesh/08.png";
        _imageMap[6] = ":/mMainWidgets/Icon/Mesh/06.png";
        auto* mainLay = new QVBoxLayout(this);

        m_rangeFold = new FoldSection("网格", this);
        _meshControlLabel = new PanelLabel("所属部件");
        _meshControlBox = new QComboBox();
        _meshControlEdit = new QLineEdit();
        _meshControlBox->addItems({ "新建" , "部件名称" });

        QLabel *_meshShapeLabel = new PanelLabel("网格形状");
        _meshShapeBox = new QComboBox();
        _meshShapeBox->addItems({"点","线","三角形","四边形","四面体","六面体","三棱柱"});
        m_rangeFold->addRow(_meshControlLabel, _meshControlBox, _meshControlEdit);
        m_rangeFold->addRow(_meshShapeLabel, _meshShapeBox);

        mainLay->addWidget(m_rangeFold);
        // ========== 示意图折叠块 ==========
        m_GraphFold = new FoldSection("示意图", this);
        QLabel* _Graphlabel = new QLabel; _Graphlabel->setStyleSheet("border:1px solid #DBE5EE");
        QPixmap pixmap(_imageMap[0]);
        QPixmap scaledPixmap= pixmap.scaled(
            pixmap.width() / 2,
            pixmap.height() / 2,
            Qt::KeepAspectRatio,      // 保持宽高比
            Qt::SmoothTransformation  // 平滑缩放（高质量）
        );
        // 检查是否为空
        if (scaledPixmap.isNull()) {
            qDebug() << "图片加载失败！";
        }
        _Graphlabel->setPixmap(scaledPixmap);
        m_GraphFold->addRow(_Graphlabel);
        mainLay->addWidget(m_GraphFold);

        // ========== 操作按钮 ==========
        _undoBtn = new QPushButton("撤销上一点");
        _clearBtn = new QPushButton("清空已拾取节点");
        _undoElemBtn = new QPushButton("撤销上一单元");
        QFrame* horizontalLine = createHorizontalLine();
        QHBoxLayout* btnLay = new QHBoxLayout();
        btnLay->addWidget(_undoBtn);
        btnLay->addWidget(_clearBtn);
        btnLay->addWidget(_undoElemBtn);
        mainLay->addWidget(horizontalLine);
        mainLay->addLayout(btnLay);
        mainLay->addStretch();

        connect(_meshShapeBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [=](int index) {
                // 切换网格形状后，清空当前组拾取
                clearCurrentPick();

                QPixmap pixmap(_imageMap[index]);
                QPixmap scaledPixmap = pixmap.scaled(
                    pixmap.width() / 2,
                    pixmap.height() / 2,
                    Qt::KeepAspectRatio,      // 保持宽高比
                    Qt::SmoothTransformation  // 平滑缩放（高质量）
                );
                // 检查是否为空
                if (scaledPixmap.isNull()) {
                    qDebug() << "图片加载失败！";
                }
                _Graphlabel->setPixmap(scaledPixmap);
            }
        );

        connect(_clearBtn, &QPushButton::clicked, this, &mCreateMeshPanel::clearCurrentPick);
        connect(_undoBtn, &QPushButton::clicked, this, &mCreateMeshPanel::undoLastPick);
        connect(_undoElemBtn, &QPushButton::clicked, this, &mCreateMeshPanel::undoLastElement);
    }


    mCreateMeshPanel::~mCreateMeshPanel()
    {
    }

    void mCreateMeshPanel::newMeshPanel()
    {
        _lockedPartName.clear();
        _pickedNodeIds.clear();
        lockPickToNode(true);
        connectPick();
    }
    void mCreateMeshPanel::editMeshPanel(int meshId)
    {
        _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(meshId);

        updatePanel();
    }
    void mCreateMeshPanel::getMeshInfoById(const std::vector<int> ids)
    {

    }
    void mCreateMeshPanel::updatePanel()
    {
        // 面板每次被激活/重新进入时调用：清空状态并连接拾取信号
        _lockedPartName.clear();
        _pickedNodeIds.clear();
        lockPickToNode(true);
        connectPick();
    }

    int mCreateMeshPanel::currentNeedCount() const
    {
        return shapeNodeCountMap().value(_meshShapeBox->currentIndex(), 0);
    }

    ElementType mCreateMeshPanel::currentElementType() const
    {
        return shapeElementTypeMap().value(_meshShapeBox->currentIndex(), Mass1);
    }

    void mCreateMeshPanel::connectPick()
    {
        if (getCVM->getPreRenderWidget())
        {
            connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer,
                this, &mCreateMeshPanel::slot_onNodePicked, Qt::UniqueConnection);
        }
    }

    void mCreateMeshPanel::disconnectPick()
    {
        if (getCVM->getPreRenderWidget())
        {
            disconnect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer,
                this, &mCreateMeshPanel::slot_onNodePicked);
        }
    }

    void mCreateMeshPanel::clearCurrentPick()
    {
        _pickedNodeIds.clear();
        if (getCVM->getPreRenderWidget())
        {
            getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
            getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
            getCVM->getPreRenderWidget()->updatePaint();
        }
    }

    void mCreateMeshPanel::undoLastPick()
    {
        if (_pickedNodeIds.isEmpty())
            return;
        _pickedNodeIds.removeLast();

        // 以内部列表为准重建视口选择容器
        auto render = getCVM->getPreRenderWidget();
        if (render)
        {
            auto meshSelectData = render->getSelectionContainer()->getMeshSelectData();
            meshSelectData->ClearAll();
            if (!_pickedNodeIds.isEmpty())
            {
                std::vector<MeshSelectInfoData> selectArray;
                for (int id : _pickedNodeIds)
                {
                    MeshSelectInfoData data;
                    data.data_id.node_id = id;
                    selectArray.push_back(data);
                }
                meshSelectData->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshNode, selectArray);
            }
            render->getHighlightController()->clearHighlightMesh();
            render->updatePaint();
        }
    }

    void mCreateMeshPanel::slot_onNodePicked()
    {
        auto render = getCVM->getPreRenderWidget();
        if (render == nullptr)
        {
            return;
        }
        auto meshSelectData = render->getSelectionContainer()->getMeshSelectData();
        const auto& containerNodes = meshSelectData->getSelectDataByType(SelectionInfoType::MeshNode);

        // 始终以容器为准重建内部列表：同时覆盖增选、Ctrl减选、以及一次信号内既有减又有增的情况
        _pickedNodeIds.clear();
        for (const auto& data : containerNodes)
        {
            _pickedNodeIds.append(data.data_id.node_id);
        }

        // 拾取高亮（默认橙色圆点）由渲染层选择容器链路自动维护，面板无需额外预览

        if (_pickedNodeIds.size() >= currentNeedCount())
        {
            if (createElementFromNodes())
            {
                // 创建成功：走统一 clearPick 链路彻底清空拾取与高亮，继续拾取下一组
                _pickedNodeIds.clear();
                getCVM->clearPick();
            }
        }
    }

    bool mCreateMeshPanel::createElementFromNodes()
    {
        QString partname = _meshControlEdit->text().simplified();
        QString method = _meshControlBox->currentText();
        bool isCreateNewPart = false;
        if (method == tr("新建"))
        {
            if (partname.isEmpty())
            {
                mGlobalSignals::getInstance()->outputMessageSig(WarningMessage, tr("请输入新建部件名称"), true);
                return false;
            }
            if (getCVM->getModelData()->getMeshData()->havePart(partname))
            {
                mGlobalSignals::getInstance()->outputMessageSig(WarningMessage, tr("该零件名已存在"), true);
                return false;
            }
            isCreateNewPart = true;
        }
        else
        {
            if (partname.isEmpty())
            {
                mGlobalSignals::getInstance()->outputMessageSig(WarningMessage, tr("请选择部件"), true);
                return false;
            }
        }

        auto meshdata = getCVM->getModelData()->getMeshData();
        for (int id : _pickedNodeIds)
        {
            if (!meshdata->NodeIsHaveByID(id))
            {
                mGlobalSignals::getInstance()->outputMessageSig(WarningMessage, tr("节点%1不存在").arg(id), true);
                return false;
            }
        }

        int elementId = meshdata->createMeshToPart(currentElementType(), _pickedNodeIds, partname);
        if (elementId < 0)
        {
            mGlobalSignals::getInstance()->outputMessageSig(WarningMessage, tr("创建单元失败"), true);
            return false;
        }
        _createdElementIds.append(elementId);
        mGlobalSignals::getInstance()->outputMessageSig(NormalMessage, tr("已创建单元 ID=%1").arg(elementId), true);

        if (isCreateNewPart)
        {
            // 新建部件首个单元创建成功：刷新树，并锁定后续单元加入同一部件
            vector<int> partIds = getCVM->getModelData()->getAllPhyID(Physical_Part);
            if (!partIds.empty())
            {
                emit mGlobalSignals::getInstance()->createUpdatePartSig(true, set<int>{partIds.back()});
            }
            switchToExistingPart(partname);
        }
        // 显式刷新渲染（createMeshToPart 发出的刷新信号在拾取高亮场景下会被后到的清选刷新覆盖）
        if (getCVM->getPreRenderWidget())
        {
            getCVM->getPreRenderWidget()->updatePaint();
        }
        return true;
    }

    void mCreateMeshPanel::switchToExistingPart(const QString& partName)
    {
        _lockedPartName = partName;
        _meshControlBox->setCurrentIndex(1);
        _meshControlEdit->setText(partName);
        // 锁定部件行，连续创建期间不允许再改
        _meshControlBox->setEnabled(false);
        _meshControlEdit->setEnabled(false);
    }

    void mCreateMeshPanel::undoLastElement()
    {
        if (_createdElementIds.isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(WarningMessage, tr("没有可撤销的单元"), true);
            return;
        }
        auto meshdata = getCVM->getModelData()->getMeshData();
        int elementId = _createdElementIds.last();
        int partId = meshdata->getPartIdByEleId(elementId);
        if (partId < 0)
        {
            // 单元已被其他操作删除，移除记录即可
            _createdElementIds.removeLast();
            mGlobalSignals::getInstance()->outputMessageSig(WarningMessage, tr("单元%1已不存在").arg(elementId), true);
            return;
        }
        QString partName = meshdata->getPartNameByPartId(partId);
        meshdata->deleteMesh({ RenderPickResultData(elementId, partId) });
        _createdElementIds.removeLast();
        mGlobalSignals::getInstance()->outputMessageSig(NormalMessage, tr("已撤销单元 ID=%1").arg(elementId), true);

        // 若该部件是本次新建且单元已全部撤销，deleteMesh 内部已将其从 _partdata 移除；
        // 只需发信号更新模型树，并解除部件锁定恢复"新建"模式
        if (!_lockedPartName.isEmpty() && partName == _lockedPartName && !meshdata->havePart(partName))
        {
            emit mGlobalSignals::getInstance()->deletePartSig(set<int>{ partId });
            mGlobalSignals::getInstance()->outputMessageSig(NormalMessage, tr("部件%1已空，已删除").arg(partName), true);
            _lockedPartName.clear();
            _meshControlBox->setCurrentIndex(0);
            _meshControlEdit->clear();
            _meshControlBox->setEnabled(true);
            _meshControlEdit->setEnabled(true);
        }
        if (getCVM->getPreRenderWidget())
            getCVM->getPreRenderWidget()->updatePaint();
    }

    void mCreateMeshPanel::apply()
    {
        // 兼容接口：当前面板拾取凑满即自动创建，这里手动触发一次（当前组凑满时）
        if (_pickedNodeIds.size() == currentNeedCount() && createElementFromNodes())
        {
            _pickedNodeIds.clear();
            if (getCVM->getPreRenderWidget())
            {
                getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
                getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
                getCVM->getPreRenderWidget()->updatePaint();
            }
        }
    }

    void mCreateMeshPanel::hideEvent(QHideEvent* event)
    {
        // 面板隐藏（切换面板/关闭Dock等任何路径）时断开拾取并清理现场
        disconnectPick();
        clearCurrentPick();
        lockPickToNode(false);
        // 解锁部件行，下次进入面板恢复可选
        _meshControlBox->setEnabled(true);
        _meshControlEdit->setEnabled(true);
        _lockedPartName.clear();
        mMeshPanel::hideEvent(event);
    }

    void mCreateMeshPanel::lockPickToNode(bool lock)
    {
        // 面板激活期间仅允许拾取网格节点（索引5对应 MeshNode），退出时恢复全部可用
        QVector<bool> enabled(10, true);
        if (lock)
        {
            enabled.fill(false);
            enabled[5] = true;
        }
        emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(enabled);
    }

}
