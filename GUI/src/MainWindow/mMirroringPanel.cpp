//Render
#include "PickCommonFunction.h"

// Mesh
#include "mMeshTransform.h"
#include "GlobalMeshMessage.h"
#include "mMechGlobalSignals.h"
// Mech
#include "mMeshPanel.h"
#include "mMirroringPanel.h"
#include "mOtherFunction.h"

using namespace MechPreData;
using namespace MechCommon;
using namespace MeshPanelData;

Q_DECLARE_METATYPE(std::set<int>)
Q_DECLARE_METATYPE(MeshParam)          // 头文件

namespace MechMainWindow
{

    mMirroringPanel::mMirroringPanel(QWidget* parent)
        : mMeshPanel(parent)
    {
        auto* mainLay = new QVBoxLayout(this);

        // 对象下拉框
        FoldSection* m_ObjectFold = new FoldSection("对象", this);
        posPick.initWidget("对象");
        posPick.setState(false);
        m_ObjectFold->addRow(posPick.propwid);

        // 镜像平面下拉框
        FoldSection* m_MirrorFold = new FoldSection("镜像平面", this);
        method_label = new PanelLabel("方式");
        method = new QComboBox;
        method->addItem("点-面法向");

        point = new PanelLabel("点");
        pointXEdit = new QLineEdit("0"); pointYEdit = new QLineEdit("0"); pointZEdit = new QLineEdit("0");
        QToolButton* point1Addbtn = new QToolButton(); point1Addbtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));

        m_MirrorFold->layout()->setVerticalSpacing(0);
        m_MirrorFold->layout()->setContentsMargins(12, 0, 12, 0);
        m_MirrorFold->layout()->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

        method->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        point->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        // 将点行包装为 QWidget，便于整体显示/隐藏，避免 QFormLayout 残留行间距
        auto createPointRow = [&](QLabel* label, QLineEdit* e1, QLineEdit* e2, QLineEdit* e3, QToolButton* btn) -> QWidget* {
            QHBoxLayout* h = new QHBoxLayout();
            h->setContentsMargins(0, 0, 0, 0);
            h->setSpacing(5);
            h->setAlignment(Qt::AlignVCenter);
            h->addWidget(label);
            h->addWidget(e1);
            h->addWidget(e2);
            h->addWidget(e3);
            h->addWidget(btn);
            QWidget* w = new QWidget();
            w->setLayout(h);
            w->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
            w->setFixedHeight(26);
            return w;
            };
        QWidget* point1Row = createPointRow(point, pointXEdit, pointYEdit, pointZEdit, point1Addbtn);

        vecPick.initWidget("面法向", false, true);

        // 方式行也作为整行添加，避免 QFormLayout label/field 默认策略带来额外间距
        QWidget* settingRow = new QWidget();
        QHBoxLayout* settingH = new QHBoxLayout(settingRow);
        settingH->setContentsMargins(0, 0, 0, 0);
        settingH->setSpacing(5);
        settingH->setAlignment(Qt::AlignVCenter);
        settingH->addWidget(method_label);
        settingH->addWidget(method);
        settingRow->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        settingRow->setFixedHeight(26);

        // 将所有镜像平面参数行放入一个垂直容器，完全由 QVBoxLayout 控制间距，避免 QFormLayout 行策略影响
        QWidget* planeWidget = new QWidget();
        QVBoxLayout* planeV = new QVBoxLayout(planeWidget);
        planeV->setContentsMargins(0, 0, 0, 0);
        planeV->setSpacing(10); // 与平移界面 QFormLayout 默认行距保持一致
        planeV->setAlignment(Qt::AlignTop);
        planeV->addWidget(settingRow);
        planeV->addWidget(point1Row);
        planeV->addWidget(vecPick.propwid);

        m_MirrorFold->addRow(planeWidget);

        vecPick.vecEdit->setEnabled(false);  vecPick.vecBtn->setEnabled(false);

        FoldSection* m_PartFold = new FoldSection("部件", this);
        newMeshPart = new QCheckBox("新建网格部件");
        m_PartFold->addRow(newMeshPart);
        QPushButton* btn = new QPushButton("应用");
        QFrame* horizontalLine = createHorizontalLine();

        mainLay->addWidget(m_ObjectFold);
        mainLay->addWidget(m_MirrorFold);
        mainLay->addWidget(m_PartFold);
        mainLay->addWidget(horizontalLine);
        mainLay->addWidget(btn);
        mainLay->addStretch();

        // 镜像方式修改
        connect(method, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=]()
            {
                _meshOptPanelData->setMirrorMethod(method->currentText());
            });

        // 点坐标修改
        connect(point1Addbtn, &QToolButton::clicked, this, [=]() {
            auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
            if (res.empty())
                return;
            QVector3D point = getCVM->getModelData()->getMeshData()->getVectorById(res.back().data_id.node_id);

            pointXEdit->setText(MxFunctions::mNumberToString(point.x()));
            pointYEdit->setText(MxFunctions::mNumberToString(point.y()));
            pointZEdit->setText(MxFunctions::mNumberToString(point.z()));
            });


        // 拾取单元
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
            {
                posPick.setState(false);
            }
                
            _meshOptPanelData->setMeshIDs(std::move(MeshIDs));
            });

        // 清除拾取单元
        connect(posPick.btnclear, &QToolButton::clicked, this, [=]() {
            MeshIDs.clear();
            posPick.setState(false);
            _meshOptPanelData->setMeshIDs(std::move(MeshIDs));
            });

        // 添加拾取单元
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

        connect(btn, &QPushButton::clicked, this, &mMirroringPanel::apply);
        connect(pointXEdit, &QLineEdit::textChanged, this, &mMirroringPanel::pointChanged);
        connect(pointYEdit, &QLineEdit::textChanged, this, &mMirroringPanel::pointChanged);
        connect(pointZEdit, &QLineEdit::textChanged, this, &mMirroringPanel::pointChanged);
        
        // 辅助函数：根据当前方向类型更新拾取模式
        auto updatePickModeForMirrorNormal = [=]() {
            if (vecPick.dirBox->currentIndex() == 4 && vecPick._hasThreePoint)
                MechCommon::mMechGlobalSignals::getInstance()->activePickSig(5); // 节点拾取，用于三点确定
            else
                MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8); // 默认网格单元拾取
            };

        connect(vecPick.dirBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=]() {
            _meshOptPanelData->setDirAxial(vecPick.dirBox->currentText());
            updatePickModeForMirrorNormal();
            // 三点确定展开/折叠后刷新参数折叠块布局，防止内容被挤压
            QTimer::singleShot(0, this, [=]() {
                vecPick.propwid->updateGeometry();
                vecPick.propwid->adjustSize();
                m_MirrorFold->refreshLayout();
                this->updateGeometry();
                this->adjustSize();
                });
            });
        connect(vecPick.vecEdit, &QLineEdit::textChanged, this, [=]() {
            _meshOptPanelData->setDirVec(vecPick.vecEdit->text());
            });
        connect(newMeshPart, &QCheckBox::stateChanged, this, [=] {
            _meshOptPanelData->setCreatePart(newMeshPart->checkState());
            });
    }

    void mMirroringPanel::apply()
    {
        if (MeshIDs.empty())
        {
            QString msg = QString("未拾取任何单元，请重新拾取！");
            mGlobalSignals::getInstance()->outputMessageSig(WarningMessage, msg, true);
            return;
        }
        // 获得拾取的网格单元ID
        std::vector<MeshSelectInfoData> pickMeshIDs = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);

        // 定义点
        SPoint3 point = SPoint3{ pointXEdit->text().toDouble(),pointYEdit->text().toDouble(),pointZEdit->text().toDouble() };

        // 定义面法向
        SPoint3 axis;
        if (vecPick.dirBox->currentIndex() == 4 && vecPick._hasThreePoint)
        {
            QVector3D p1(vecPick.tpX1->text().toDouble(), vecPick.tpY1->text().toDouble(), vecPick.tpZ1->text().toDouble());
            QVector3D p2(vecPick.tpX2->text().toDouble(), vecPick.tpY2->text().toDouble(), vecPick.tpZ2->text().toDouble());
            QVector3D p3(vecPick.tpX3->text().toDouble(), vecPick.tpY3->text().toDouble(), vecPick.tpZ3->text().toDouble());
            QVector3D v1 = p2 - p1;
            QVector3D v2 = p3 - p1;
            QVector3D normal = QVector3D::crossProduct(v1, v2);
            if (normal.lengthSquared() > 1e-12)
            {
                normal.normalize();
                axis = SPoint3{ normal.x(), normal.y(), normal.z() };
            }
            else
            {
                axis = SPoint3{ 0,0,0 };
            }
        }
        else
        {
            QStringList Axis = vecPick.vecEdit->text().split(',');
            axis = SPoint3{ Axis[0].toDouble(),Axis[1].toDouble(),Axis[2].toDouble() };
        }

        // 将meshdata传入网格数据
        GlobalMeshMessage::getInstance()->setMeshData(getCVM->getModelData()->getMeshData());
        if (newMeshPart->isChecked())
        {
            meshMirror(MeshIDs, axis, point, true);
            vector<int> partIds = getCVM->getModelData()->getAllPhyID(Physical_Part);
            emit mGlobalSignals::getInstance()->createUpdatePartSig(true, set<int>{partIds.end() - 1, partIds.end()});
        }
        else
        {
            meshMirror(MeshIDs, axis, point, false);
        }
        emit mGlobalSignals::getInstance()->updateModelTree();

        // 清除拾取与高亮
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
    }

    void mMirroringPanel::pointChanged()
    {
        Point point;
        point.x = pointXEdit->text().toDouble();
        point.y = pointYEdit->text().toDouble();
        point.z = pointZEdit->text().toDouble();
        _meshOptPanelData->setMirrorPoint(point);
    }
    
    mMirroringPanel::~mMirroringPanel()
    {

    }
    
    void mMirroringPanel::editMeshPanel(int meshId)
    {
        _meshOptPanelData = getCVM->getModelData()->getManager<mMeshOptPanelManager>(Physical_MeshOptPanel).getMeshPanelData(meshId);
        updatePanel();
    }

    void mMirroringPanel::updatePanel()
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
        vecPick.dirBox->setCurrentText(_meshOptPanelData->getDirAxial());
        vecPick.vecEdit->setText(_meshOptPanelData->getDirVec());
        method->setCurrentText(_meshOptPanelData->getMirrorMethod());

        Point point = _meshOptPanelData->getMirrorPoint();
        pointXEdit->setText(MxFunctions::mNumberToString(point.x));
        pointYEdit->setText(MxFunctions::mNumberToString(point.y));
        pointZEdit->setText(MxFunctions::mNumberToString(point.z));
        newMeshPart->setChecked(_meshOptPanelData->getCreatePart());
    }
}
