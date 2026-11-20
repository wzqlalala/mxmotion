
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
#include "MXMesh.h"
#include "mMeshTransform.h"
#include "GlobalMeshMessage.h"

//Mech
#include "PickCommonFunction.h"
#include "mZoomPanel.h"

//#include "mGeoInfoCalculate.h" //【OCC摘除·S10/P0-1 终局3】死 include（零使用；经 mGeoDataModelSet 拉 OCC 链）
using namespace MechPreData;
using namespace MeshPanelData;
using namespace MxFunctions;
#include <set>
#include "mOtherFunction.h"
Q_DECLARE_METATYPE(std::set<int>)
Q_DECLARE_METATYPE(MeshParam)          // 头文件

namespace MechMainWindow
{

    mZoomPanel::mZoomPanel(QWidget* parent)
        : mMeshPanel(parent)
    {
        // 主布局
        auto* mainLay = new QVBoxLayout(this);

        // 对象折叠部分
        FoldSection* m_ObjectFold = new FoldSection("对象", this);
        posPick.initWidget("对象");
        posPick.setState(false);
        m_ObjectFold->addRow(posPick.propwid);

        // 因子折叠部分
        FoldSection* m_FactorFold = new FoldSection("因子", this);
        QLabel* center = new PanelLabel("基准点");
        centerEdit1 = new mCalcLineEdit("0");
        centerEdit2 = new mCalcLineEdit("0");
        centerEdit3 = new mCalcLineEdit("0");
        QToolButton* centerAddbtn = new QToolButton(); centerAddbtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        QLabel* x = new PanelLabel("X轴");
        QLabel* y = new PanelLabel("Y轴");
        QLabel* z = new PanelLabel("Z轴");
        xEdit = new mCalcLineEdit("1.0");
        yEdit = new mCalcLineEdit("1.0");
        zEdit = new mCalcLineEdit("1.0");
        m_FactorFold->addRow(center, centerEdit1, centerEdit2, centerEdit3, centerAddbtn);
        m_FactorFold->addRow(x, xEdit);
        m_FactorFold->addRow(y, yEdit);
        m_FactorFold->addRow(z, zEdit);

        // 部件折叠部分
        FoldSection* m_PartFold = new FoldSection("部件", this);
        newMeshPart = new QCheckBox("新建网格部件");
        m_PartFold->addRow(newMeshPart);

        // 水平分割线
        QFrame* horizontalLine = createHorizontalLine();

        // 缩放按钮
        QPushButton* btnNeg = new QPushButton("缩放-");
        QPushButton* btn = new QPushButton("缩放+");
        QHBoxLayout* btnLay = new QHBoxLayout();
        btnLay->addWidget(btnNeg);
        btnLay->addWidget(btn);

        // 子布局添加进主布局
        mainLay->addWidget(m_ObjectFold);
        mainLay->addWidget(m_FactorFold);
        mainLay->addWidget(m_PartFold);
        mainLay->addWidget(horizontalLine);
        mainLay->addLayout(btnLay);
        mainLay->addStretch();

        // 连接信号与槽函数
        connect(centerEdit1, &QLineEdit::textChanged, this, &mZoomPanel::centerChanged);
        connect(centerEdit2, &QLineEdit::textChanged, this, &mZoomPanel::centerChanged);
        connect(centerEdit3, &QLineEdit::textChanged, this, &mZoomPanel::centerChanged);
        connect(xEdit, &QLineEdit::textChanged, this, [=]() {
            _meshOptPanelData->setZoomX(xEdit->text().toDouble());
            });
        connect(yEdit, &QLineEdit::textChanged, this, [=]() {
            _meshOptPanelData->setZoomY(yEdit->text().toDouble());
            });
        connect(zEdit, &QLineEdit::textChanged, this, [=]() {
            _meshOptPanelData->setZoomZ(zEdit->text().toDouble());
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
        connect(newMeshPart, &QCheckBox::stateChanged, this, [=] {
            _meshOptPanelData->setCreatePart(newMeshPart->checkState());
            });
    }

    mZoomPanel::~mZoomPanel()
    {

    }

    void mZoomPanel::apply(double dirSign)
    {
        if (MeshIDs.empty())
            return;

        // 缩放基准点
        SPoint3 point = SPoint3{ centerEdit1->text().toDouble(),centerEdit2->text().toDouble(),centerEdit3->text().toDouble() };

        // 缩放因子，缩放-时取倒数
        double sx = xEdit->text().toDouble();
        double sy = yEdit->text().toDouble();
        double sz = zEdit->text().toDouble();
        if (dirSign < 0)
        {
            if (sx == 0 || sy == 0 || sz == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig(1, tr("缩放因子不能为0"), true);
                return;
            }
            sx = 1.0 / sx;
            sy = 1.0 / sy;
            sz = 1.0 / sz;
        }

        // 对网格进行缩放
        GlobalMeshMessage::getInstance()->setMeshData(getCVM->getModelData()->getMeshData());

        if (newMeshPart->isChecked())
        {
            meshScale(MeshIDs, point, sx, sy, sz, true);
            vector<int> partIds = getCVM->getModelData()->getAllPhyID(Physical_Part);
            emit mGlobalSignals::getInstance()->createUpdatePartSig(true, set<int>{partIds.end() - 1, partIds.end()});
        }
        else
            meshScale(MeshIDs, point, sx, sy, sz, false);

        // 清除拾取与高亮
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
    }

    void mZoomPanel::centerChanged()
    {
        Point center;
        center.x = centerEdit1->text().toDouble();
        center.y = centerEdit2->text().toDouble();
        center.z = centerEdit3->text().toDouble();
        _meshOptPanelData->setZoomCenter(center);
    }

    void mZoomPanel::editMeshPanel(int meshId)
    {
        _meshOptPanelData = getCVM->getModelData()->getManager<mMeshOptPanelManager>(Physical_MeshOptPanel).getMeshPanelData(meshId);

        updatePanel();
    }

    void mZoomPanel::updatePanel()
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
        Point center = _meshOptPanelData->getZoomCenter();
        centerEdit1->setExpression(MxFunctions::mNumberToString(center.x));
        centerEdit2->setExpression(MxFunctions::mNumberToString(center.y));
        centerEdit3->setExpression(MxFunctions::mNumberToString(center.z));
        xEdit->setExpression(MxFunctions::mNumberToString(_meshOptPanelData->getZoomX()));
        yEdit->setExpression(MxFunctions::mNumberToString(_meshOptPanelData->getZoomY()));
        zEdit->setExpression(MxFunctions::mNumberToString(_meshOptPanelData->getZoomZ()));
        newMeshPart->setChecked(_meshOptPanelData->getCreatePart());
    }
}
