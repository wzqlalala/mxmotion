//Qt 
#include <QColorDialog>     
//BasicData
#include "mMeshData.h"
#include "mGlobalSignals.h"
//Mech
#include "mMeshPanel.h"
#include "mMoveMeshPanel.h"
#include "PickCommonFunction.h"

using namespace MechPreData;
using namespace MeshPanelData;
using namespace MxFunctions;

namespace MechMainWindow
{

    mMoveMeshPanel::mMoveMeshPanel(QWidget* parent)
        : mMeshPanel(parent)
    {
        auto* mainLay = new QVBoxLayout(this);

        FoldSection* m_ObjectFold = new FoldSection("对象", this);
        posPick.initWidget("对象");
        posPick.setState(false);
        m_ObjectFold->addRow(posPick.propwid);

        m_PartFold = new FoldSection("部件", this);
        QLabel* part = new PanelLabel("部件");
        m_partBox = new QComboBox;
        m_partBox->addItem("新建");
        m_partEdit = new QLineEdit;
        m_partEdit->setPlaceholderText("输入新部件名称");
        m_PartFold->addRow(part, m_partBox, m_partEdit);
        QLabel* arrowColor = new PanelLabel("颜色");
        QToolButton* arrowColorBtn = new QToolButton();
        m_PartFold->addRow(arrowColor, arrowColorBtn);

        QFrame* horizontalLine = createHorizontalLine();

        QPushButton* copyBtn = new QPushButton("复制");
        QPushButton* moveBtn = new QPushButton("移动");
        QHBoxLayout* HLayout = new QHBoxLayout;
        HLayout->addStretch();
        HLayout->addWidget(copyBtn);
        HLayout->addWidget(moveBtn);

        mainLay->addWidget(m_ObjectFold);
        mainLay->addWidget(m_PartFold);
        mainLay->addWidget(horizontalLine);
        mainLay->addLayout(HLayout);
        mainLay->addStretch();

        connect(arrowColorBtn, &QToolButton::clicked, [=]() {
            QColor _color = QColorDialog::getColor(QColor(curColor.x(), curColor.y(), curColor.z()), nullptr, "获取颜色");
            //pre_colorBtn->setText(MxFunctions::mNumberToString(_color.red()) + "," + MxFunctions::mNumberToString(_color.green()) + "," + MxFunctions::mNumberToString(_color.blue()));
            curColor = QVector3D(_color.red(), _color.green(), _color.blue());
            arrowColorBtn->setStyleSheet(QString("background:%1").arg(_color.name()));
            });

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

            // 拾取判断
            if (!MeshIDs.empty())
            {
                posPick.setState(true);
                posPick.labelpicked->setText(QString("已选择:%1个单元").arg(MeshIDs.size()));
            }
            else
            {
                posPick.setState(false);
            }

            // 设置拾取单元数据
            _meshOptPanelData->setMeshIDs(std::move(MeshIDs));
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
            {
                posPick.setState(false);
            }
                
            _meshOptPanelData->setMeshIDs(std::move(MeshIDs));
            });
        connect(posPick.btnclear, &QToolButton::clicked, this, [=]() {
            MeshIDs.clear();
            posPick.setState(false);
            _meshOptPanelData->setMeshIDs(std::move(MeshIDs));
            });
        connect(m_partBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
            m_partEdit->setEnabled(index == 0);
            });
        m_partBox->setCurrentIndex(0);
        m_partEdit->setEnabled(true);
        connect(arrowColorBtn, &QToolButton::clicked, [=]() {
            QColor _color = QColorDialog::getColor(QColor(curColor.x(), curColor.y(), curColor.z()), nullptr, "获取颜色");
            if (!_color.isValid())
            {
                mGlobalSignals::getInstance()->outputMessageSig(1, "获取颜色无效，已取消颜色选择");
                return;
            }
            curColor = QVector3D(_color.red(), _color.green(), _color.blue());
            arrowColorBtn->setStyleSheet(QString("background:%1").arg(_color.name()));
            });
        // 移动：选中单元摘除到目标部件
        connect(moveBtn, &QPushButton::clicked, this, [=]() {
            if (MeshIDs.empty())
            {
                mGlobalSignals::getInstance()->outputMessageSig(1, "请先拾取要移动的网格单元");
                return;
            }
            QString targetPart = getTargetPartName();
            if (targetPart.isEmpty())
            {
                mGlobalSignals::getInstance()->outputMessageSig(1, "请输入部件名称！");
                return;
            }
                
            int dstPartId = getCVM->getModelData()->getMeshData()->moveElementsToPart(targetPart, MeshIDs);
            if (dstPartId >= 0)
            {
                applyColorToPart(getCVM->getModelData()->getMeshData()->getPartNameByPartId(dstPartId), QColor(curColor.x(), curColor.y(), curColor.z()));
            }
                
            finishOperate();
            });
        // 复制：选中单元副本追加到目标部件（共享节点）
        connect(copyBtn, &QPushButton::clicked, this, [=]() {
            if (MeshIDs.empty())
            {
                mGlobalSignals::getInstance()->outputMessageSig(1, "请先拾取要复制的网格单元");
                return;
            }

            QString targetPart = getTargetPartName();
            if (targetPart.isEmpty())
            {
                mGlobalSignals::getInstance()->outputMessageSig(1, "请输入部件名称！");
                return;
            }

            int dstPartId = getCVM->getModelData()->getMeshData()->copyElementsToPart(targetPart, MeshIDs);
            if (dstPartId >= 0)
            {
                applyColorToPart(getCVM->getModelData()->getMeshData()->getPartNameByPartId(dstPartId), QColor(curColor.x(), curColor.y(), curColor.z()));
            }

            finishOperate();
            });
    }

    void mMoveMeshPanel::refreshPartCombo()
    {
        m_partBox->clear();
        m_partBox->addItem("新建");
        QStringList partNames = getCVM->getModelData()->getMeshData()->getAllPartNames();
        m_partBox->addItems(partNames);
        m_partBox->setCurrentIndex(0);
        m_partEdit->clear();
        // 复位颜色按钮
        curColor = QVector3D(255, 255, 255);
        if (QToolButton* colorBtn = m_PartFold->findChild<QToolButton*>())
        {
            colorBtn->setStyleSheet(QString());
        }
            
    }

    QString mMoveMeshPanel::getTargetPartName()
    {
        auto* meshData = getCVM->getModelData()->getMeshData();
        if (m_partBox->currentIndex() == 0)
        {
            QString name = m_partEdit->text().trimmed();
            if (name.isEmpty())
            {
                emit mGlobalSignals::getInstance()->outputMessageSig(1, "请输入新部件名称");
                return QString();
            }
            if (meshData->havePart(name))
            {
                emit mGlobalSignals::getInstance()->outputMessageSig(1, QString("部件[%1]已存在，请更换名称或直接在下拉框中选择").arg(name));
                return QString();
            }
            return name;
        }
        return m_partBox->currentText();
    }

    void mMoveMeshPanel::applyColorToPart(const QString& partName, const QColor& color)
    {
        auto* meshData = getCVM->getModelData()->getMeshData();
        mPartData* part = meshData->getPartData(partName);
        if (part == nullptr)
            return;
        //保留原透明度，仅替换RGB
        QRgba64 rgba64 = color.rgba64();
        QRgba64 oldRgba64 = part->getPartInnerColor().rgba64();
        QColor newColor;
        newColor.setRgba64(qRgba64(rgba64.red(), rgba64.green(), rgba64.blue(), oldRgba64.alpha()));
        part->setPartInnerColor(newColor);
        getCVM->updatePartShowColor(PartColorType::partColor, { (int)part->getPartID() });
    }

    void mMoveMeshPanel::finishOperate()
    {
        // 清除拾取与高亮
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
        MeshIDs.clear();
        posPick.setState(false);
        _meshOptPanelData->setMeshIDs(MeshIDs);
        refreshPartCombo();
        // 更新模型树
        vector<int> partIds = getCVM->getModelData()->getAllPhyID(Physical_Part);
        emit mGlobalSignals::getInstance()->createUpdatePartSig(true, set<int>{partIds.end() - 1, partIds.end()});
    }

    mMoveMeshPanel::~mMoveMeshPanel()
    {

    }
    void mMoveMeshPanel::newMeshPanel()
    {
        refreshPartCombo();
    }
    void mMoveMeshPanel::editMeshPanel(int meshId)
    {
        _meshOptPanelData = getCVM->getModelData()->getManager<mMeshOptPanelManager>(Physical_MeshOptPanel).getMeshPanelData(meshId);

        updatePanel();

    }
    void mMoveMeshPanel::updatePanel()
    {
        refreshPartCombo();
    }
}
