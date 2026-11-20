//Mesh
#include "MXMesh.h"
#include "mMeshTransform.h"
#include "GlobalMeshMessage.h"

//Mech
#include "mMeshPanel.h"
#include "PickCommonFunction.h"
#include "mMatrixPanel.h"
#include "mMechGlobalSignals.h"
#include "mOtherFunction.h"

using namespace MechPreData;
using namespace MeshPanelData;

namespace MechMainWindow
{

    mMatrixPanel::mMatrixPanel(QWidget* parent)
        : mMeshPanel(parent)
    {
        // 主布局
        auto* mainLay = new QVBoxLayout(this);

        // 对象折叠部分
        FoldSection* m_ObjectFold = new FoldSection("对象", this);
        posPick.initWidget("对象");
        posPick.setState(false);
        m_ObjectFold->addRow(posPick.propwid);

        // 参数折叠部分
        FoldSection* m_ParaFold = new FoldSection("参数", this);
        QLabel* method = new PanelLabel("方式");
        QLabel* center = new PanelLabel("圆心");
        QLabel* spacing = new PanelLabel("间距");
        QLabel* num = new PanelLabel("个数");
        methodBox = new QComboBox; methodBox->addItems({ "圆周阵列","线性阵列" });
        centerEdit1 = new mCalcLineEdit("0");
        centerEdit2 = new mCalcLineEdit("0");
        centerEdit3 = new mCalcLineEdit("0");
        QToolButton* centerAddbtn = new QToolButton(); centerAddbtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        spacingEdit = new mCalcLineEdit("0");
        numEdit = new mCalcLineEdit("1");
        vecPick.initWidget("方向", false, true);
        m_ParaFold->addRow(method, methodBox);
        m_ParaFold->addRow(center, centerEdit1, centerEdit2, centerEdit3, centerAddbtn);
        m_ParaFold->addRow(vecPick.propwid);
        vecPick.vecEdit->setDisabled(true); vecPick.vecBtn->setDisabled(true);
        m_ParaFold->addRow(spacing, spacingEdit);
        m_ParaFold->addRow(num, numEdit);

        // 水平分割线
        QFrame* horizontalLine = createHorizontalLine();

        // 阵列按钮
        QPushButton* btnNeg = new QPushButton("阵列-");
        QPushButton* btn = new QPushButton("阵列+");
        QHBoxLayout* btnLay = new QHBoxLayout();
        btnLay->addWidget(btnNeg);
        btnLay->addWidget(btn);

        // 子布局添加进主布局
        mainLay->addWidget(m_ObjectFold);
        mainLay->addWidget(m_ParaFold);
        mainLay->addWidget(horizontalLine);
        mainLay->addLayout(btnLay);
        mainLay->addStretch();

        // 连接信号与槽函数
        connect(methodBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=]()
            {
                m_ParaFold->refreshLayout();
                _meshOptPanelData->setMatrixMethod(methodBox->currentText());
                if (methodBox->currentIndex() == 0)
                {
                    center->show(); centerEdit1->show(); centerEdit2->show(); centerEdit3->show(); centerAddbtn->show();
                }
                else if (methodBox->currentIndex() == 1)
                {
                    center->hide(); centerEdit1->hide(); centerEdit2->hide(); centerEdit3->hide(); centerAddbtn->hide();
                }
            });
        connect(centerAddbtn, &QToolButton::clicked, this, [=]() {
            auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
            if (res.empty())
                return;
            QVector3D point = getCVM->getModelData()->getMeshData()->getVectorById(res.back().data_id.node_id);

            centerEdit1->setText(MxFunctions::mNumberToString(point.x()));
            centerEdit2->setText(MxFunctions::mNumberToString(point.y()));
            centerEdit3->setText(MxFunctions::mNumberToString(point.z()));
            });
        connect(centerEdit1, &QLineEdit::textChanged, this, &mMatrixPanel::centerPointChanged);
        connect(centerEdit2, &QLineEdit::textChanged, this, &mMatrixPanel::centerPointChanged);
        connect(centerEdit3, &QLineEdit::textChanged, this, &mMatrixPanel::centerPointChanged);
        connect(spacingEdit, &QLineEdit::textChanged, this, [=]() {
            _meshOptPanelData->setMatrixDis(spacingEdit->text().toDouble());
            });
        connect(numEdit, &QLineEdit::textChanged, this, [=]() {
            _meshOptPanelData->setMatrixNum(numEdit->text().toInt());
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

        // 辅助函数：根据当前方向类型更新拾取模式
        auto updatePickModeForMatrixDir = [=]() {
            if (vecPick.dirBox->currentIndex() == 4 && vecPick._hasThreePoint)
                MechCommon::mMechGlobalSignals::getInstance()->activePickSig(5); // 节点拾取，用于三点确定
            else
                MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8); // 默认网格单元拾取
            };

        connect(btn, &QPushButton::clicked, this, [=]() { apply(1.0); });
        connect(btnNeg, &QPushButton::clicked, this, [=]() { apply(-1.0); });
        connect(vecPick.dirBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=]() {
            _meshOptPanelData->setDirAxial(vecPick.dirBox->currentText());
            updatePickModeForMatrixDir();
            // 三点确定展开/折叠后刷新参数折叠块布局，防止内容被挤压
            QTimer::singleShot(0, this, [=]() {
                vecPick.propwid->updateGeometry();
                vecPick.propwid->adjustSize();
                m_ParaFold->refreshLayout();
                this->updateGeometry();
                this->adjustSize();
                });
            });
        connect(vecPick.vecEdit, &QLineEdit::textChanged, this, [=]() {
            _meshOptPanelData->setDirVec(vecPick.vecEdit->text());
            });
    }

    mMatrixPanel::~mMatrixPanel()
    {

    }

    void mMatrixPanel::apply(double dirSign)
    {
        if (MeshIDs.empty())
            return;

        // 定义方向，阵列-时取反
        SPoint3 dir;
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
                dir = SPoint3{ normal.x(), normal.y(), normal.z() };
            }
            else
            {
                dir = SPoint3{ 0,0,0 };
            }
        }
        else
        {
            QStringList Dir = vecPick.vecEdit->text().split(',');
            dir = SPoint3{ Dir[0].toDouble(),Dir[1].toDouble(),Dir[2].toDouble() };
        }
        if (dirSign < 0)
            dir = SPoint3{ -dir.x(),-dir.y(),-dir.z() };

        // 对网格进行阵列（阵列副本始终生成为新网格部件）
        GlobalMeshMessage::getInstance()->setMeshData(getCVM->getModelData()->getMeshData());
        if (methodBox->currentIndex() == 0)
        {
            // 圆周阵列
            SPoint3 center = SPoint3{ centerEdit1->text().toDouble(),centerEdit2->text().toDouble(),centerEdit3->text().toDouble() };
            meshArray(MeshIDs, dir, center, spacingEdit->text().toDouble(), numEdit->text().toInt());
        }
        else
        {
            // 线性阵列
            meshArray(MeshIDs, dir, spacingEdit->text().toDouble(), numEdit->text().toInt());
        }
        vector<int> partIds = getCVM->getModelData()->getAllPhyID(Physical_Part);
        emit mGlobalSignals::getInstance()->createUpdatePartSig(true, set<int>{partIds.end() - numEdit->text().toInt(), partIds.end()});

        // 清除拾取与高亮
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
    }

    void mMatrixPanel::centerPointChanged()
    {
        Point center;
        center.x = centerEdit1->text().toDouble();
        center.y = centerEdit2->text().toDouble();
        center.z = centerEdit3->text().toDouble();
        _meshOptPanelData->setMatrixCenter(center);
    }

    void mMatrixPanel::editMeshPanel(int meshId)
    {
        _meshOptPanelData = getCVM->getModelData()->getManager<mMeshOptPanelManager>(Physical_MeshOptPanel).getMeshPanelData(meshId);

        updatePanel();
    }

    void mMatrixPanel::updatePanel()
    {
        vecPick.dirBox->setCurrentText(_meshOptPanelData->getDirAxial());
        vecPick.vecEdit->setText(_meshOptPanelData->getDirVec());
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
        methodBox->setCurrentText(_meshOptPanelData->getMatrixMethod());
        Point center = _meshOptPanelData->getMatrixCenter();
        centerEdit1->setExpression(MxFunctions::mNumberToString(center.x));
        centerEdit2->setExpression(MxFunctions::mNumberToString(center.y));
        centerEdit3->setExpression(MxFunctions::mNumberToString(center.z));
        spacingEdit->setExpression(MxFunctions::mNumberToString(_meshOptPanelData->getMatrixDis()));
        numEdit->setExpression(MxFunctions::mNumberToString(_meshOptPanelData->getMatrixNum()));
    }
}
