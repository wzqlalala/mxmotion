#include "mRotationPanel.h"
#include "mMechGlobalSignals.h"
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
#include <QVector3D>
#include <QTimer>

//BasicData
#include "mMeshData.h"

//Mesh
#include "mMeshTransform.h"
//#include <mGeoInfoCalculate.h> //【OCC摘除·S10/P0-1 终局2】死 include（零使用）
#include "GlobalMeshMessage.h"
#include "PickCommonFunction.h"
#include "mOtherFunction.h"

using namespace MechPreData;
using namespace MeshPanelData;
using namespace MxFunctions;
using namespace MGeoManager;
#include <set>
#include "mOtherFunction.h"
Q_DECLARE_METATYPE(std::set<int>)
Q_DECLARE_METATYPE(MeshParam)          // 头文件

namespace MechMainWindow
{
    mRotationPanel::mRotationPanel(QWidget* parent)
        : mMeshPanel(parent)
    {
        auto* mainLay = new QVBoxLayout(this);

        FoldSection* m_ObjectFold = new FoldSection("对象", this);

        posPick.initWidget("对象");
        posPick.setState(false);
        m_ObjectFold->addRow(posPick.propwid);

        FoldSection* m_TranslationFold = new FoldSection("参数", this);
        x = new PanelLabel("旋转中心点");
        w = new PanelLabel("旋转角度");
        xEdit1 = new QLineEdit("0");
        xEdit2 = new QLineEdit("0");
        xEdit3 = new QLineEdit("0");
        QToolButton* addbtn = new QToolButton(); addbtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        vecPick.initWidget("旋转中心轴", false, true);
        wEdit = new mCalcLineEdit("0");
        m_TranslationFold->addRow(x, xEdit1, xEdit2, xEdit3, addbtn);
        m_TranslationFold->addRow(vecPick.propwid);
        m_TranslationFold->addRow(w, wEdit);

        FoldSection* m_PartFold = new FoldSection("部件", this);
        newMeshPart = new QCheckBox("新建网格部件");
        QPushButton* btnNeg = new QPushButton("旋转-");
        QPushButton* btn = new QPushButton("旋转+");
        QHBoxLayout* btnLay = new QHBoxLayout();
        btnLay->addWidget(btnNeg);
        btnLay->addWidget(btn);
        m_PartFold->addRow(newMeshPart);
        QFrame* horizontalLine = createHorizontalLine();

        mainLay->addWidget(m_ObjectFold);
        mainLay->addWidget(m_TranslationFold);
        mainLay->addWidget(m_PartFold);
        mainLay->addWidget(horizontalLine);
        mainLay->addLayout(btnLay);
        mainLay->addStretch();

        connect(addbtn, &QToolButton::clicked, this, [=]()
            {
                auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
                if (res.empty())
                    return;
                QVector3D point = getCVM->getModelData()->getMeshData()->getVectorById(res.back().data_id.node_id);

                xEdit1->setText(MxFunctions::mNumberToString(point.x()));
                xEdit2->setText(MxFunctions::mNumberToString(point.y()));
                xEdit3->setText(MxFunctions::mNumberToString(point.z()));
            });
        connect(xEdit1, &QLineEdit::textChanged, this, [=] {
            _meshOptPanelData->setRotationCtrX(xEdit1->text().toDouble());
            });
        connect(xEdit2, &QLineEdit::textChanged, this, [=] {
            _meshOptPanelData->setRotationCtrY(xEdit2->text().toDouble());
            });
        connect(xEdit3, &QLineEdit::textChanged, this, [=] {
            _meshOptPanelData->setRotationCtrZ(xEdit3->text().toDouble());
            });
        connect(wEdit, &QLineEdit::textChanged, this, [=] {
            _meshOptPanelData->setRotationAngle(wEdit->text().toDouble());
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
        connect(btn, &QPushButton::clicked, this, [=]() { apply(1.0); });
        connect(btnNeg, &QPushButton::clicked, this, [=]() { apply(-1.0); });
        // 辅助函数：根据当前方向类型更新拾取模式
        auto updatePickModeForRotationAxis = [=]() {
            if (vecPick.dirBox->currentIndex() == 4 && vecPick._hasThreePoint)
                MechCommon::mMechGlobalSignals::getInstance()->activePickSig(5); // 节点拾取，用于三点确定
            else
                MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8); // 默认网格单元拾取
            };

        connect(vecPick.dirBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=]() {
            _meshOptPanelData->setDirAxial(vecPick.dirBox->currentText());
            updatePickModeForRotationAxis();
            // 三点确定展开/折叠后刷新参数折叠块布局，防止内容被挤压
            QTimer::singleShot(0, this, [=]() {
                vecPick.propwid->updateGeometry();
                vecPick.propwid->adjustSize();
                m_TranslationFold->refreshLayout();
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

    void mRotationPanel::apply(double dirSign)
    {
        if (MeshIDs.empty())
            return;
        // 定义旋转中心点
        SPoint3 point = SPoint3{ xEdit1->text().toDouble(),xEdit2->text().toDouble(),xEdit3->text().toDouble() };
        // 定义旋转中心轴
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
            QStringList centerAxis = vecPick.vecEdit->text().split(',');
            if (centerAxis.size() != 3)
                return;
            axis = SPoint3{ centerAxis[0].toDouble(),centerAxis[1].toDouble(),centerAxis[2].toDouble() };
        }
        // 对网格进行旋转
        GlobalMeshMessage::getInstance()->setMeshData(getCVM->getModelData()->getMeshData());
        double angle = wEdit->text().toDouble() * dirSign;
        if (newMeshPart->isChecked())
        {
            meshRotate(MeshIDs, axis, point, angle, true);
            vector<int> partIds = getCVM->getModelData()->getAllPhyID(Physical_Part);
            emit mGlobalSignals::getInstance()->createUpdatePartSig(true, set<int>{partIds.end() - 1, partIds.end()});
        }
        else
            meshRotate(MeshIDs, axis, point, angle, false);

        // 清除拾取与高亮
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
    }
    mRotationPanel::~mRotationPanel()
    {

    }
    void mRotationPanel::newMeshPanel()
    {

    }
    void mRotationPanel::editMeshPanel(int meshId)
    {
        _meshOptPanelData = getCVM->getModelData()->getManager<mMeshOptPanelManager>(Physical_MeshOptPanel).getMeshPanelData(meshId);
        updatePanel();
    }
    void mRotationPanel::updatePanel()
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
        xEdit1->setText(MxFunctions::mNumberToString(_meshOptPanelData->getRotationCtrX()));
        xEdit2->setText(MxFunctions::mNumberToString(_meshOptPanelData->getRotationCtrY()));
        xEdit3->setText(MxFunctions::mNumberToString(_meshOptPanelData->getRotationCtrZ()));
        wEdit->setExpression(MxFunctions::mNumberToString(_meshOptPanelData->getRotationAngle()));
        newMeshPart->setChecked(_meshOptPanelData->getCreatePart());
    }
}
