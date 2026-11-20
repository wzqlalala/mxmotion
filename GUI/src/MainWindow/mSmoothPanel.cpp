
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
#include "mSmoothPanel.h"
#include "GlobalMeshMessage.h"
#include "mMeshTransform.h"
#include "mMeshSmooth.h"
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

    mSmoothPanel::mSmoothPanel(QWidget* parent)
        : mMeshPanel(parent)
    {
        auto* mainLay = new QVBoxLayout(this);

        FoldSection* m_ObjectFold = new FoldSection("对象", this);
      
        posPick.initWidget("对象");
        posPick.setState(false);
        posPick.labelpicked->setText(QString("默认为全部"));

        MeshIDs.clear();
        MechModelData::mModelData* modelData = getCVM->getModelData();
        mMeshData* meshdata = modelData->getMeshData();
        std::vector<RenderPickResultData> allElements = meshdata->getAllElements();
        MeshIDs = allElements;

        m_ObjectFold->addRow(posPick.propwid);

        m_TranslationFold = new FoldSection("参数", this);
        QLabel* method = new PanelLabel("光滑方式");
        methodBox = new QComboBox; methodBox->addItems({"保持特征","不保持特征"});

        x = new PanelLabel("二面角");
        y = new PanelLabel("迭代次数");

        xEdit = new mCalcLineEdit("30");
        yEdit = new mCalcLineEdit("1.0");


        m_TranslationFold->addRow(method, methodBox);
        m_TranslationFold->addRow(x, xEdit);
        m_TranslationFold->addRow(y, yEdit);




        QPushButton* btn = new QPushButton("确定");
        QFrame* horizontalLine = createHorizontalLine();
        mainLay->addWidget(m_ObjectFold);
        mainLay->addWidget(m_TranslationFold);
        mainLay->addWidget(horizontalLine);
        QHBoxLayout* h = new QHBoxLayout;
        h->addStretch(); h->addWidget(btn);
        mainLay->addLayout(h);
        mainLay->addStretch();


        connect(methodBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=]() {
            if (methodBox->currentIndex() == 0)
            {
                x->show();
                xEdit->show();
            }
            else
            {
                x->hide();
                xEdit->hide();
            }
            _meshOptPanelData->setSmoothMethod(methodBox->currentText());
            });
        connect(xEdit, &QLineEdit::textChanged, this, [=]() {
            _meshOptPanelData->setSmoothAngle(xEdit->text().toDouble());
            });
        connect(yEdit, &QLineEdit::textChanged, this, [=]() {
            _meshOptPanelData->setSmoothNum(yEdit->text().toDouble());
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
        connect(btn, &QToolButton::clicked, this, &mSmoothPanel::apply);
    }

    void mSmoothPanel::apply()
    {

        if (MeshIDs.empty())
            return;
        // 获得拾取的网格单元ID
        std::vector<MeshSelectInfoData> pickMeshIDs = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);

        GlobalMeshMessage::getInstance()->setMeshData(getCVM->getModelData()->getMeshData());
        bool feature_preserve = false;
        if (methodBox->currentIndex() == 0)
            feature_preserve = true;
        else
            feature_preserve = false;
        meshSmooth(MeshIDs, feature_preserve, xEdit->text().toDouble(), yEdit->text().toDouble());



        // 清除拾取与高亮
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
        
    }

    mSmoothPanel::~mSmoothPanel()
    {

    }
    void mSmoothPanel::newMeshPanel()
    {

    }
    void mSmoothPanel::editMeshPanel(int meshId)
    {
        _meshOptPanelData = getCVM->getModelData()->getManager<mMeshOptPanelManager>(Physical_MeshOptPanel).getMeshPanelData(meshId);
        auto a = _meshOptPanelData->getMeshPanelType();
        updatePanel();

    }
    void mSmoothPanel::getMeshInfoById(const std::vector<int> ids)
    {

    }
    void mSmoothPanel::updatePanel()
    {
        if (_meshOptPanelData->getPickType() == "全部")
        {
            posPick.setState(false);
            posPick.labelpicked->setText(QString("默认为全部"));
            MeshIDs.clear();
            MechModelData::mModelData* modelData = getCVM->getModelData();
            mMeshData* meshdata = modelData->getMeshData();
            std::vector<RenderPickResultData> allElements = meshdata->getAllElements();
            MeshIDs = allElements;
            _meshOptPanelData->setMeshIDs(MeshIDs);
        }
        else
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
        }
        methodBox->setCurrentText(_meshOptPanelData->getSmoothMethod());
        xEdit->setExpression(MxFunctions::mNumberToString(_meshOptPanelData->getSmoothAngle()));
        yEdit->setExpression(MxFunctions::mNumberToString(_meshOptPanelData->getSmoothNum()));
    }

    void mSmoothPanel::setMeshSize()
    {

    }
    void mSmoothPanel::initSize()
    {

    }

}
