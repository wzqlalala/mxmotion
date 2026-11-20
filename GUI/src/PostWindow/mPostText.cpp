#include "mPostText.h"

#include <QFormLayout>
#include <QColorDialog>
#include <algorithm>

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mDockLayoutManager.h"
#include "mViewTreeFunction.h"

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

using namespace MxFunctions;
using namespace MechCommon;
using namespace PostData;
using namespace MechPreData;
using namespace MIOFile;

namespace PostWindow
{
    mPostTextPanel::mPostTextPanel(QWidget* parent) : mPanelBase(parent)
    {
        _curPostRend = nullptr;

        _mainLayout = new QVBoxLayout(this);
        _mainLayout->setContentsMargins(2, 0, 2, 0);

        /********************样式********************/
        FoldSection* styleFold = new FoldSection("样式", this);
        _lineText = new QLineEdit();

        _btnColor = new QToolButton();
        _btnColor->setFixedSize(24, 24);
        _btnColor->setStyleSheet("QToolButton { background-color: rgb(0, 0, 0);}");

        _spinFontSize = new QSpinBox();
        _spinFontSize->setRange(8, 72);
        _spinFontSize->setValue(16);

        _sliderTransparency = new QSlider(Qt::Horizontal);
        _sliderTransparency->setRange(0, 100);

        _spinTransparency = new QSpinBox();
        _spinTransparency->setRange(0, 100);
        _spinTransparency->setSuffix("  %");

        styleFold->addRow(new QLabel("文本内容"), _lineText);
        styleFold->addRow(new QLabel("颜色"), _btnColor);
        styleFold->addRow(new QLabel("字号"), _spinFontSize);
        // styleFold->addRow(new QLabel("透明度"), _sliderTransparency, _spinTransparency);

        _currentStateButton = new QPushButton("编辑中...");
        styleFold->addRow(new QLabel("当前状态"), _currentStateButton);
        connect(_currentStateButton, &QToolButton::clicked, this, &mPostTextPanel::slot_changeType);

        styleFold->refreshLayout();
        m_foldWidgets.append(styleFold);
        _mainLayout->addWidget(styleFold);
        _mainLayout->addStretch();

        connect(_lineText, &QLineEdit::textChanged, this, &mPostTextPanel::slot_setText);
        connect(_lineText, &QLineEdit::editingFinished, this, &mPostTextPanel::slot_drawText);
        connect(_btnColor, &QToolButton::clicked, this, &mPostTextPanel::slot_setColor);
        connect(_spinFontSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &mPostTextPanel::slot_drawText);
        // connect(_sliderTransparency, &QSlider::valueChanged, this, &mPostTextPanel::slot_setSlider);
        // connect(_spinTransparency, QOverload<int>::of(&QSpinBox::valueChanged), this, &mPostTextPanel::slot_setSpinBox);
    }

    void mPostTextPanel::newTextPanel(int caseid)
    {
        //创建文本节点
        MechCommon::mMechGlobalSignals::getInstance()->createTextSig();

        auto* postWidget = getCVM->getPostWidget();
        if (postWidget == nullptr) return;
        const auto& mgr = postWidget->interaction()->getToolManager();
        mgr->setActive(mxr::InteractionToolType::CFDAnnotation);
        // 创建文字框
        const auto& tool = mgr->active<mxr::InteractionCFDAnnotationTool>();
        tool->createTextBox();

        QColor color = _btnColor->property("currentColor").value<QColor>();
        tool->setTextAnnotation(
            getCVM->getPostRenderWidget(),
            _lineText->text(),
            _spinFontSize->value(),
            "Times New Roman",
            { (float)color.red() / 255.0f, (float)color.green() / 255.0f, (float)color.blue() / 255.0f }
        );
    }

    void mPostTextPanel::setCurViewName(const QString& viewName)
    {
        _curViewName = viewName;

        //界面更新
        updatePanel();
    }

    void mPostTextPanel::updatePanel()
    {

    }

    void mPostTextPanel::slot_setText(QString text)
    {
        _lineText->setText(text);
    }

    void mPostTextPanel::slot_drawText()
    {
        auto* postWidget = getCVM->getPostWidget();
        if (postWidget == nullptr) return;
        const auto& mgr = postWidget->interaction()->getToolManager();
        mgr->setActive(mxr::InteractionToolType::CFDAnnotation);
        // 创建文本
        const auto& tool = mgr->active<mxr::InteractionCFDAnnotationTool>();

        QColor color = _btnColor->property("currentColor").value<QColor>();
        tool->setTextAnnotation(
            getCVM->getPostRenderWidget(),
            _lineText->text(),
            _spinFontSize->value(),
            "Times New Roman",
            { (float)color.red() / 255.0f, (float)color.green() / 255.0f, (float)color.blue() / 255.0f }
        );
    }

    void mPostTextPanel::slot_setColor()
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
        tool->setTextAnnotation(
            getCVM->getPostRenderWidget(),
            _lineText->text(),
            _spinFontSize->value(),
            "Times New Roman",
            { (float)color.red() / 255.0f, (float)color.green() / 255.0f, (float)color.blue() / 255.0f }
        );
    }

    void mPostTextPanel::slot_changeType()
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

    void mPostTextPanel::slot_setSlider(int value)
    {
        _spinTransparency->setValue(value);
    }

    void mPostTextPanel::slot_setSpinBox(int value)
    {
        _sliderTransparency->setValue(value);
    }

}