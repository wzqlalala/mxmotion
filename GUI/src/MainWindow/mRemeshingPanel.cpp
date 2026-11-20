
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
//#include "MXMeshAPI.h"
#include "MXMesh.h"
#include "mxr/view/mPreMeshHighLightRender.h"
#include "mxr/view/HighlightController.h"

#include "mxr/label/mSymbolController.h"
#include "mxr/label/SymbolDescription.h"
#include "PickCommonFunction.h"

#include "mInfoWindow.h"
#include "mRemeshingPanel.h"
#include "GlobalMeshMessage.h"
#include "mMeshEdit.h"
//#include <mGeoInfoCalculate.h> //【OCC摘除·S10/P0-1 终局2】死 include（零使用）
#include "mMeshCheck.h"

using namespace MechPreData;
using namespace MeshPanelData;
using namespace MxFunctions;
#include <set>
#include "mOtherFunction.h"
Q_DECLARE_METATYPE(std::set<int>)
Q_DECLARE_METATYPE(MeshParam)          // 头文件

namespace MechMainWindow
{

    mRemeshingPanel::mRemeshingPanel(QWidget* parent)
        : mMeshPanel(parent)
    {
        auto* mainLay = new QVBoxLayout(this);

        FoldSection* m_ObjectFold = new FoldSection("网格", this);
      
        posPick.initWidget("对象");
        posPick.setState(false);

        // 初始不填充 MeshIDs，必须手动选择对象；
        // 只有用户点击确定/添加时才写入当前拾取的单元。
        MeshIDs.clear();

        m_ObjectFold->addRow(posPick.propwid);

        m_TranslationFold = new FoldSection("参数", this);
        QLabel* method = new PanelLabel("形状");
        methodBox = new QComboBox; methodBox->addItems({"三角形","四边形主导","四面体"});

        size = new PanelLabel("尺寸");
        sizeEdit = new mCalcLineEdit("2");

        // 增长率仅形状为四面体时生效；行样式同自适应面板的"几何邻近值"：
        // 勾选框 + 标签 + 输入框整行一个 widget，勾选框默认关闭且控制输入框启停
        growthRateBox = new QCheckBox;
        growthRateBox->setChecked(false);
        growthRate = new QLabel("增长率"); growthRate->setFixedWidth(80);
        growthRateEdit = new mCalcLineEdit("1.2");
        growthRateEdit->setEnabled(false);
        growthRateRow = new QWidget;
        {
            QHBoxLayout* rowLay = new QHBoxLayout;
            rowLay->setMargin(0);
            rowLay->setSpacing(0);
            rowLay->addWidget(growthRateBox);
            rowLay->addWidget(growthRate);
            rowLay->addWidget(growthRateEdit, 1);
            growthRateRow->setContentsMargins(0, 0, 0, 0);
            growthRateRow->setLayout(rowLay);
        }

        m_TranslationFold->addRow(method, methodBox);
        m_TranslationFold->addRow(size, sizeEdit);
        m_TranslationFold->addRow(growthRateRow);
        m_TranslationFold->setRowVisible(growthRateRow, methodBox->currentText() == "四面体");
  

        QPushButton* btn = new QPushButton("重剖分");
        QFrame* horizontalLine = createHorizontalLine();
        mainLay->addWidget(m_ObjectFold);
        mainLay->addWidget(m_TranslationFold);
        mainLay->addWidget(horizontalLine);
        QHBoxLayout* h = new QHBoxLayout;
        h->addStretch(); h->addWidget(btn);
        mainLay->addLayout(h);
        mainLay->addStretch();
        connect(methodBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=]() {
            _meshOptPanelData->setMeshType(methodBox->currentText());
            m_TranslationFold->setRowVisible(growthRateRow, methodBox->currentText() == "四面体");
            });
        connect(sizeEdit, &QLineEdit::textChanged, this, [=]() {_meshOptPanelData->setMeshSize(sizeEdit->text().toDouble()); });
        connect(growthRateEdit, &QLineEdit::textChanged, this, [=]() {_meshOptPanelData->setGrowthRate(growthRateEdit->text().toDouble()); });
        connect(growthRateBox, &QCheckBox::toggled, this, [=](bool checked) {
            growthRateEdit->setEnabled(checked);
            _meshOptPanelData->setGrowthRateChecked(checked);
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
            _meshOptPanelData->setPickType(QString("拾取"));
            });
        connect(posPick.btnclear, &QToolButton::clicked, this, [=]() {
            MeshIDs.clear();
            posPick.setState(false);
            _meshOptPanelData->setMeshIDs(std::move(MeshIDs));
            _meshOptPanelData->setPickType(QString("拾取"));
            });
        connect(posPick.btnadd, &QToolButton::clicked, this, [=]() {
            // 获得拾取的网格单元ID，追加到MeshIDs
            MeshIDs.clear();
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
            _meshOptPanelData->setPickType(QString("拾取"));
            });
        connect(btn,&QPushButton::clicked,this,&mRemeshingPanel::apply);
    }

    void mRemeshingPanel::apply()
    {
        if (MeshIDs.empty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage, "请选择需要重剖分的网格对象！", true);
            return;
        }
        // 当选择的形状为三角形或四边形主导时，判断拾取的网格类型是否为三角形或四边形
        if (methodBox->currentIndex() == 0 || methodBox->currentIndex() == 1)
        {
            mMeshData* meshData = getCVM->getModelData()->getMeshData();
            for (const auto& ele : MeshIDs)
            {
                MeshType meshType = meshData->getMeshTypeByEleId(ele.result_id);
                if (meshType != MeshTri && meshType != MeshQuad)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage, "选取网格类型错误：需选取三角形或四边形网格！", true);
                    getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
                    getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
                    getCVM->getPreRenderWidget()->updatePaint();
                    posPick.btnclear->click();
                    return;
                }
            }
            GlobalMeshMessage::getInstance()->setMeshData(meshData);
            auto FreeEdges = meshFreeEdge(MeshIDs);
            if (!FreeEdges.size())
            {
                mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage, "拾取区域为封闭区域，不支持网格重划分", true);
                getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
                getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
                getCVM->getPreRenderWidget()->updatePaint();
                posPick.btnclear->click();
                return;
            }
            // 对网格进行重剖分
            GlobalMeshMessage::getInstance()->setMeshData(getCVM->getModelData()->getMeshData());
            faceMeshRemesh(MeshIDs, methodBox->currentIndex(), sizeEdit->text().toDouble());
        }
        else if (methodBox->currentIndex() == 2)
        {
            mMeshData* meshData = getCVM->getModelData()->getMeshData();
            bool tetType = false;
            bool tetTri = false;
            for (const auto& ele : MeshIDs)
            {
                MeshType meshType = meshData->getMeshTypeByEleId(ele.result_id);
                if (meshType != MeshTet)
                {
                    if (meshType == MeshQuad)
                    {
                        mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage, "选取网格类型错误：需选取四面体或三角形！", true);
                        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
                        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
                        getCVM->getPreRenderWidget()->updatePaint();
                        posPick.btnclear->click();
                        return;
                    }
                    else if(meshType == MeshTri)
                    {
                        tetTri = true;
                    }
                }
                else
                {
                    tetType = true;
                } 
            }
            if (tetTri && tetType)
            {
                mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage, "选取网格类型不一致：需要单一选取四面体或三角形！", true);
            }
            // 增长率仅在勾选时传入；未勾选时传 -1，算法侧跳过 smoothness 设置保持引擎默认
            const double growthRateValue = growthRateBox->isChecked() ? growthRateEdit->text().toDouble() : 0.0;
            if (tetType)
            {
                tetMeshRemesh(MeshIDs, sizeEdit->text().toDouble(), growthRateValue);
            }
            else if (tetTri)
            {
                tetraMeshbySurTris(MeshIDs, sizeEdit->text().toDouble(), growthRateValue);
            }

        }

        //emit mGlobalSignals::getInstance()->updateModelTree();
        // 清除拾取与高亮
        MeshIDs.clear();
        posPick.setState(false);
        _meshOptPanelData->setMeshIDs(std::move(MeshIDs));
        _meshOptPanelData->setPickType(QString("拾取"));
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
        MeshIDs.clear();
    }

    mRemeshingPanel::~mRemeshingPanel()
    {

    }
    void mRemeshingPanel::newMeshPanel()
    {
        // 面板复用，每次新建时重置为未选择状态
        MeshIDs.clear();
        if (_meshOptPanelData)
        {
            _meshOptPanelData->setPickType(QString("拾取"));
            _meshOptPanelData->setMeshIDs({});
        }
        posPick.setState(false);

        // 清空视口中残留的拾取，避免第一次点击添加按钮把历史选择全部加入
        if (getCVM && getCVM->getPreRenderWidget())
        {
            getCVM->clearPick();
            getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
        }
    }
    void mRemeshingPanel::editMeshPanel(int meshId)
    {
        _meshOptPanelData = getCVM->getModelData()->getManager<mMeshOptPanelManager>(Physical_MeshOptPanel).getMeshPanelData(meshId);
        auto a = _meshOptPanelData->getMeshPanelType();
        updatePanel();

    }
    void mRemeshingPanel::getMeshInfoById(const std::vector<int> ids)
    {

    }
    void mRemeshingPanel::updatePanel()
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
        auto type = _meshOptPanelData->getMeshType();
        auto size = _meshOptPanelData->getMeshSize();
        methodBox->setCurrentText(type);
        sizeEdit->setText(MxFunctions::mNumberToString(size));
        growthRateEdit->setText(MxFunctions::mNumberToString(_meshOptPanelData->getGrowthRate()));
        growthRateBox->setChecked(_meshOptPanelData->getGrowthRateChecked());
        growthRateEdit->setEnabled(growthRateBox->isChecked());
        m_TranslationFold->setRowVisible(growthRateRow, type == "四面体");

    }

    void mRemeshingPanel::setMeshSize()
    {

    }
    void mRemeshingPanel::initSize()
    {

    }
}
