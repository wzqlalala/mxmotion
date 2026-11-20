//Mech
#include "PickCommonFunction.h"
#include "mTranslationPanel.h"
#include "mMechGlobalSignals.h"

//Mesh
#include "mMeshTransform.h"
#include "GlobalMeshMessage.h"

using namespace MechPreData;
using namespace MeshPanelData;
using namespace MxFunctions;
#include <set>
#include "mOtherFunction.h"
Q_DECLARE_METATYPE(std::set<int>)
Q_DECLARE_METATYPE(MeshParam)          // 头文件

namespace MechMainWindow
{
    mTranslationPanel::mTranslationPanel(QWidget* parent)
        : mMeshPanel(parent)
    {
        //主布局
        auto* mainLay = new QVBoxLayout(this);

        //对象折叠部分
        FoldSection* m_ObjectFold = new FoldSection("对象", this);
        posPick.initWidget("对象");
        posPick.setState(false);
        m_ObjectFold->addRow(posPick.propwid);

        //距离折叠部分
        //分量方式
        FoldSection* m_ParaFold = new FoldSection("参数", this);
        QLabel* method = new PanelLabel("方式");
        methodBox = new QComboBox(); methodBox->addItems({ "分量","矢量" });

        //分量容器
        QWidget* compWidget = new QWidget(this);
        QFormLayout* compLay = new QFormLayout(compWidget);
        compLay->setContentsMargins(0, 0, 0, 0);
        compLay->setVerticalSpacing(10);
        compLay->setHorizontalSpacing(10);
        QLabel* x = new PanelLabel("X方向");
        QLabel* y = new PanelLabel("Y方向");
        QLabel* z = new PanelLabel("Z方向");
        xEdit = new mCalcLineEdit("0");
        yEdit = new mCalcLineEdit("0");
        zEdit = new mCalcLineEdit("0");
        compLay->addRow(x, xEdit);
        compLay->addRow(y, yEdit);
        compLay->addRow(z, zEdit);

        //矢量容器
        QWidget* vecWidget = new QWidget(this);
        QFormLayout* vecLay = new QFormLayout(vecWidget);
        vecLay->setContentsMargins(0, 0, 0, 0);
        vecLay->setVerticalSpacing(10);
        vecLay->setHorizontalSpacing(10);
        QLabel* distance = new QLabel("距离");
        distanceEdit = new mCalcLineEdit("10");
        vecPick.initWidget("方向", false, true);
        // 矢量方向不需要参考系行，直接隐藏并去掉该行占位高度
        vecPick.coor->hide();
        vecPick.coorBox->hide();
        vecPick.coorBtn->hide();
        // 方向行：标签 + 下拉框 + 反向按钮（包成 HBox）
        QHBoxLayout* dirRow = new QHBoxLayout();
        dirRow->setContentsMargins(0, 0, 0, 0);
        dirRow->addWidget(vecPick.dirBox);
        dirRow->addWidget(vecPick.dirBtn);
        vecLay->addRow(vecPick.dir, dirRow);
        // 向量行：用 vecPick 内部的 vecLayout（包含编辑框+拾取按钮），由 VecPickWidget 自己控制显隐
        vecLay->addRow(vecPick.vec, vecPick.vecLayout);
        // 三点确定行：整行加入，初始隐藏
        vecLay->addRow(vecPick.threePointWidget);
        vecPick.threePointWidget->setVisible(false);
        // 距离行
        vecLay->addRow(distance, distanceEdit);
        vecWidget->hide();

        m_ParaFold->addRow(method, methodBox);
        m_ParaFold->addRow(compWidget);
        m_ParaFold->addRow(vecWidget);
        m_ParaFold->refreshLayout();

        //部件折叠部分
        FoldSection* m_PartFold = new FoldSection("部件", this);
        newMeshPart = new QCheckBox("新建网格部件");
        QFrame* horizontalLine = createHorizontalLine();
        QPushButton* btnNeg = new QPushButton("平移-");
        QPushButton* btnPos = new QPushButton("平移+");
        QHBoxLayout* btnLay = new QHBoxLayout();
        btnLay->addWidget(btnNeg);
        btnLay->addWidget(btnPos);
        m_PartFold->addRow(newMeshPart);

        //将子部件添加进主部件
        mainLay->addWidget(m_ObjectFold);
        mainLay->addWidget(m_ParaFold);
        mainLay->addWidget(m_PartFold);
        mainLay->addWidget(horizontalLine);
        mainLay->addLayout(btnLay);
        mainLay->addStretch();

        //连接信号和槽
        connect(xEdit, &QLineEdit::textChanged, this, [=]() {_meshOptPanelData->setTranslateX(xEdit->text().toDouble()); });
        connect(yEdit, &QLineEdit::textChanged, this, [=]() {_meshOptPanelData->setTranslateY(yEdit->text().toDouble()); });
        connect(zEdit, &QLineEdit::textChanged, this, [=]() {_meshOptPanelData->setTranslateZ(zEdit->text().toDouble()); });
        connect(distanceEdit, &QLineEdit::textChanged, this, [=]() {_meshOptPanelData->setTranslateDis(distanceEdit->text().toDouble()); });
        // 分量/矢量切换：hide/show 容器，避免 QFormLayout 行操作导致的崩溃和空白
        connect(methodBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=]()
            {
                if (methodBox->currentIndex() == 0)
                {
                    compWidget->show();
                    vecWidget->hide();
                    _meshOptPanelData->setTranslateMethod("分量");
                }
                else if (methodBox->currentIndex() == 1)
                {
                    compWidget->hide();
                    vecWidget->show();
                    _meshOptPanelData->setTranslateMethod("矢量");
                }
                m_ParaFold->refreshLayout();
            });
        // 矢量方向切换：控制三点确定展开/折叠、拾取模式切换、布局刷新
        connect(vecPick.dirBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=]() {
            _meshOptPanelData->setDirAxial(vecPick.dirBox->currentText());
            if (vecPick.dirBox->currentIndex() == 4 && vecPick._hasThreePoint)
                MechCommon::mMechGlobalSignals::getInstance()->activePickSig(5); // 节点拾取，用于三点确定
            else
                MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8); // 默认网格单元拾取
            // 三点确定展开/折叠后刷新布局，防止内容被挤压
            QTimer::singleShot(0, this, [=]() {
                vecWidget->updateGeometry();
                vecWidget->adjustSize();
                m_ParaFold->refreshLayout();
                this->updateGeometry();
                this->adjustSize();
                });
            });
        connect(vecPick.vecEdit, &QLineEdit::textChanged, this, [=]() {
            _meshOptPanelData->setDirVec(vecPick.vecEdit->text());
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
            });
        connect(btnPos, &QPushButton::clicked, this, [=]() { apply(1.0); });
        connect(btnNeg, &QPushButton::clicked, this, [=]() { apply(-1.0); });
        connect(newMeshPart, &QCheckBox::stateChanged, this, [=] {
            _meshOptPanelData->setCreatePart(newMeshPart->checkState());
            });
        connect(vecPick.dirBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=]() {
            _meshOptPanelData->setDirAxial(vecPick.dirBox->currentText());
            if (vecPick.dirBox->currentIndex() == 4 && vecPick._hasThreePoint)
                MechCommon::mMechGlobalSignals::getInstance()->activePickSig(5); // 节点拾取，用于三点确定
            else
                MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8); // 默认网格单元拾取
            // 三点确定展开/折叠后刷新参数折叠块布局，防止内容被挤压
            QTimer::singleShot(0, this, [=]() {
                vecPick.propwid->updateGeometry();
                vecPick.propwid->adjustSize();
                m_ParaFold->refreshLayout();  //【集成】dev 版折叠块名（src 的 m_TranslationFold 成员未随合并保留）
                this->updateGeometry();
                this->adjustSize();
                });
            });
        connect(vecPick.vecEdit, &QLineEdit::textChanged, this, [=]() {
            _meshOptPanelData->setDirVec(vecPick.vecEdit->text());
            });
    }

    mTranslationPanel::~mTranslationPanel()
    {

    }

    void mTranslationPanel::apply(double dirSign)
    {
        if (MeshIDs.empty())
            return;
        // 定义方向向量
        SPoint3 dir;
        if (methodBox->currentIndex() == 1)
        {
            // 矢量方式：距离沿设定方向平移
            SPoint3 axis;
            if (vecPick.dirBox->currentIndex() == 4 && vecPick._hasThreePoint)
            {
                QVector3D p1(vecPick.tpX1->text().toDouble(), vecPick.tpY1->text().toDouble(), vecPick.tpZ1->text().toDouble());
                QVector3D p2(vecPick.tpX2->text().toDouble(), vecPick.tpY2->text().toDouble(), vecPick.tpZ2->text().toDouble());
                QVector3D p3(vecPick.tpX3->text().toDouble(), vecPick.tpY3->text().toDouble(), vecPick.tpZ3->text().toDouble());
                QVector3D normal = QVector3D::crossProduct(p2 - p1, p3 - p1);
                if (normal.lengthSquared() > 1e-12)
                {
                    normal.normalize();
                    axis = SPoint3{ normal.x(), normal.y(), normal.z() };
                }
                else
                    return;
            }
            else
            {
                QStringList vec = vecPick.vecEdit->text().split(',');
                if (vec.size() != 3)
                    return;
                QVector3D v(vec[0].toDouble(), vec[1].toDouble(), vec[2].toDouble());
                if (v.lengthSquared() < 1e-12)
                    return;
                v.normalize();
                axis = SPoint3{ v.x(), v.y(), v.z() };
            }
            double dis = distanceEdit->text().toDouble() * dirSign;
            dir = SPoint3{ axis.x() * dis, axis.y() * dis, axis.z() * dis };
        }
        else
        {
            // 分量方式：整体乘以方向符号
            dir = SPoint3{ xEdit->text().toDouble() * dirSign,
                           yEdit->text().toDouble() * dirSign,
                           zEdit->text().toDouble() * dirSign };
        }
        // 对网格进行平移
        GlobalMeshMessage::getInstance()->setMeshData(getCVM->getModelData()->getMeshData());
        if (newMeshPart->isChecked())
        {
            meshMove(MeshIDs, dir, true);
            vector<int> partIds = getCVM->getModelData()->getAllPhyID(Physical_Part);
            emit mGlobalSignals::getInstance()->createUpdatePartSig(true, set<int>{partIds.end() - 1, partIds.end()});

        }
        else
            meshMove(MeshIDs, dir, false);
        // 清除拾取与高亮
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();

    }

    void mTranslationPanel::newMeshPanel()
    {

    }
    void mTranslationPanel::editMeshPanel(int meshId)
    {
        _meshOptPanelData = getCVM->getModelData()->getManager<mMeshOptPanelManager>(Physical_MeshOptPanel).getMeshPanelData(meshId);
        updatePanel();
    }
    void mTranslationPanel::updatePanel()
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
        auto method = _meshOptPanelData->getTranslateMethod();
        methodBox->setCurrentText(method);
        xEdit->setText(QString::number(_meshOptPanelData->getTranslateX()));
        yEdit->setText(QString::number(_meshOptPanelData->getTranslateY()));
        zEdit->setText(QString::number(_meshOptPanelData->getTranslateZ()));
        distanceEdit->setText(QString::number(_meshOptPanelData->getTranslateDis()));
        QString axial = _meshOptPanelData->getDirAxial();
        if (!axial.isEmpty())
            vecPick.dirBox->setCurrentText(axial);
        QString dirVec = _meshOptPanelData->getDirVec();
        if (!dirVec.isEmpty())
            vecPick.vecEdit->setText(dirVec);
        newMeshPart->setChecked(_meshOptPanelData->getCreatePart());
        vecPick.dirBox->setCurrentText(_meshOptPanelData->getDirAxial());
        vecPick.vecEdit->setText(_meshOptPanelData->getDirVec());
    }
}
