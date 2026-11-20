#include "mPostLog.h"

#include <QFormLayout>
#include <QColorDialog>
#include <QFileDialog>
#include <algorithm>

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mDockLayoutManager.h"
#include "mViewTreeFunction.h"
#include "mAppConfiguration.h"

//MxFunctions
#include "mGlobalSignals.h"

//BasicData
#include "mPhysicalFieldEnum.h"

//Post
#include "mPostDataBase.h"
#include "mIMxdbFile1.h"
#include "mMxdbThread.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include <InteractionCore/InteractionSystem.h>
#include <InteractionCore/Tool/CFDAnnotationTool.h>
//窗口
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "PostViewportWidget.h"
#include "mOtherFunction.h"

using namespace MxFunctions;
using namespace MechCommon;
using namespace PostData;
using namespace MechPreData;
using namespace MIOFile;

namespace PostWindow
{
    mPostLogPanel::mPostLogPanel(QWidget* parent) : mPanelBase(parent)
    {
        _curPostRend = nullptr;

        _mainLayout = new QVBoxLayout(this);
        _mainLayout->setContentsMargins(2, 0, 2, 0);

        /********************样式********************/
        FoldSection* styleFold = new FoldSection("样式", this);
        _lineFile = new QLineEdit();
        _lineFile->setReadOnly(true);

        _btnFile = new QPushButton("浏览");

        _btnColor = new QToolButton();
        _btnColor->setFixedSize(24, 24);
        _btnColor->setStyleSheet("QToolButton { background-color: rgb(0, 0, 0);}");

        _sliderTransparency = new QSlider(Qt::Horizontal);
        _sliderTransparency->setRange(0, 100);

        _spinTransparency = new QSpinBox();
        _spinTransparency->setRange(0, 100);
        _spinTransparency->setSuffix("  %");

        styleFold->addRow(new QLabel("文件"), _lineFile, _btnFile);
        styleFold->addRow(new QLabel("颜色"), _btnColor);
        // styleFold->addRow(new QLabel("透明度"), _sliderTransparency, _spinTransparency);

        _currentStateButton = new QPushButton("编辑中...");
        styleFold->addRow(new QLabel("当前状态"), _currentStateButton);
        connect(_currentStateButton, &QToolButton::clicked, this, &mPostLogPanel::slot_changeType);

        styleFold->refreshLayout();
        m_foldWidgets.append(styleFold);
        _mainLayout->addWidget(styleFold);
        _mainLayout->addStretch();


        connect(_btnFile, &QPushButton::clicked, this, &mPostLogPanel::slot_chooseFile);
        connect(_btnColor, &QToolButton::clicked, this, &mPostLogPanel::slot_setColor);
        // connect(_sliderTransparency, &QSlider::valueChanged, this, &mPostLogPanel::slot_setSlider);
        // connect(_spinTransparency, QOverload<int>::of(&QSpinBox::valueChanged), this, &mPostLogPanel::slot_setSpinBox);
    }

    void mPostLogPanel::newLogPanel(int caseid)
    {
        MechCommon::mMechGlobalSignals::getInstance()->createLogSig();

        // 创建文本
        auto* postWidget = getCVM->getPostWidget();
        if (postWidget == nullptr) return;
        const auto& mgr = postWidget->interaction()->getToolManager();
        mgr->setActive(mxr::InteractionToolType::CFDAnnotation);
        const auto& tool = mgr->active<mxr::InteractionCFDAnnotationTool>();

        const auto& data = getCVM->getGUIDataByPostRender(getCVM->getPostRenderWidget());
        QString postname = getCVM->getPostRenderWidget()->getPostDataName();
        mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);

        const auto& fileName = postDB->getName();
        int meshCount = 0;
        const auto& frames = postDB->getAllFrameData();
        for (auto itr = frames.begin(); itr != frames.end(); itr++)
        {
            if(postDB->getModelData(itr.key()))
            {
                meshCount = postDB->getModelData(itr.key())->getAllMeshIDs().size();
            }
        }

        QSize size = postWidget->size();
        int fontSize = 32;
        QColor color = _btnColor->property("currentColor").value<QColor>();
        QVector3D normalizedColor{
            (float)color.red() / 255.0f,
            (float)color.green() / 255.0f,
            (float)color.blue() / 255.0f
        };
        QString title0 = "网格数量:" + MxFunctions::mNumberToString(meshCount);
        QString title1 = "文件名称:" + fileName;
        tool->addTextAnnotation(
            getCVM->getPostRenderWidget(),
            {
                title0 + "\n"
                +
                title1 + "\n"
                +
                _curFileContent
            },
            { std::pair<int,int>{ size.width() - fontSize * std::max(title0.length(),title1.length()),0}},
            fontSize,
            "Times New Roman",
            normalizedColor
        );
    }

    void mPostLogPanel::setCurViewName(const QString& viewName)
    {
        _curViewName = viewName;

        //界面更新
        updatePanel();
    }

    void mPostLogPanel::updatePanel()
    {

    }

    void mPostLogPanel::slot_chooseFile()
    {
        QString dfdir = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
        QString filename;
        filename = QFileDialog::getOpenFileName(this, QString("打开文件"), dfdir, QString("Log File(*.log);"));


        if (filename.isEmpty())
        {
            return;
        }

        // 读取文件到_curFileContent
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug() << "无法打开文件:" << file.errorString();
            return;
        }

        QTextStream in(&file);
        in.setCodec("UTF-8");
        _curFileContent = in.readAll();
        file.close();
    }

    void mPostLogPanel::slot_changeType()
    {
        static bool isEdit = true;
        isEdit = !isEdit;
        auto* postWidget = getCVM->getPostWidget();
        if (postWidget == nullptr) return;
        if (isEdit)
        {
            const auto& mgr = postWidget->interaction()->getToolManager();
            mgr->setActive(mxr::InteractionToolType::CFDAnnotation);
            _currentStateButton->setText("编辑中...");
        }
        else
        {
            const auto& mgr = postWidget->interaction()->getToolManager();
            mgr->setActive(mxr::InteractionToolType::None);
            _currentStateButton->setText("编辑注释");
        }
    }

    void mPostLogPanel::slot_setColor()
    {
        //获取颜色
        QColor color;
        color = QColorDialog::getColor(Qt::black, this, QString("选择颜色"));

        if (!color.isValid()) return;

        _btnColor->setStyleSheet(QString("background-color: rgb(%1, %2, %3);")
            .arg(color.red()).arg(color.green()).arg(color.blue()));
        _btnColor->setProperty("currentColor", color);

        auto* postWidget = getCVM->getPostWidget();
        if (postWidget == nullptr) return;
        const auto& mgr = postWidget->interaction()->getToolManager();
        mgr->setActive(mxr::InteractionToolType::CFDAnnotation);
        const auto& tool = mgr->active<mxr::InteractionCFDAnnotationTool>();
        for (QLabel* label : tool->getQLabel())
        {
            if (label)
            {
                label->setStyleSheet(QString("color: rgb(%1, %2, %3);")
                    .arg(color.red()).arg(color.green()).arg(color.blue()));
            }
        }
    }

    void mPostLogPanel::slot_setSlider(int value)
    {
        _spinTransparency->setValue(value);
    }

    void mPostLogPanel::slot_setSpinBox(int value)
    {
        _sliderTransparency->setValue(value);
    }

}
