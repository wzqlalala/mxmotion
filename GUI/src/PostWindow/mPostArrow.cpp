#include "mPostArrow.h"

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
    mPostArrowPanel::mPostArrowPanel(QWidget* parent) : mPanelBase(parent)
    {
        _curPostRend = nullptr;

        _mainLayout = new QVBoxLayout(this);
        _mainLayout->setContentsMargins(2, 0, 2, 0);

        /********************样式********************/
        FoldSection* styleFold = new FoldSection("样式", this);
        _btnColor = new QToolButton();
        _btnColor->setFixedSize(24, 24);
        _btnColor->setStyleSheet("QToolButton { background-color: rgb(72, 116, 203);}");

        _sliderTransparency = new QSlider(Qt::Horizontal);
        _sliderTransparency->setRange(0, 100);

        _spinTransparency = new QSpinBox();
        _spinTransparency->setRange(0, 100);
        _spinTransparency->setSuffix("  %");

        _spinWidth = new QSpinBox();
        _spinWidth->setRange(1, 10);

        _comboType = new QComboBox();

        styleFold->addRow(new QLabel("颜色"), _btnColor);
        // styleFold->addRow(new QLabel("透明度"), _sliderTransparency, _spinTransparency);
        styleFold->addRow(new QLabel("宽度"), _spinWidth);
        styleFold->addRow(new QLabel("复合类型"), _comboType);

        _currentStateButton = new QPushButton("编辑中...");
        styleFold->addRow(new QLabel("当前状态"), _currentStateButton);
        connect(_currentStateButton, &QToolButton::clicked, this, &mPostArrowPanel::slot_changeType);


        styleFold->refreshLayout();
        m_foldWidgets.append(styleFold);
        _mainLayout->addWidget(styleFold);
        _mainLayout->addStretch();

        connect(_btnColor, &QToolButton::clicked, this, &mPostArrowPanel::slot_setColor);
        // connect(_sliderTransparency, &QSlider::valueChanged, this, &mPostArrowPanel::slot_setSlider);
        // connect(_spinTransparency, QOverload<int>::of(&QSpinBox::valueChanged), this, &mPostArrowPanel::slot_setSpinBox);
        connect(_spinWidth, QOverload<int>::of(&QSpinBox::valueChanged), this, &mPostArrowPanel::slot_setWidth);
        connect(_comboType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mPostArrowPanel::slot_setType);
    }

    void mPostArrowPanel::newArrowPanel(int caseid)
    {
        //创建箭头节点
        MechCommon::mMechGlobalSignals::getInstance()->createArrowSig();

        //创建箭头
        auto* postWidget = getCVM->getPostWidget();
        if (postWidget == nullptr) return;
        const auto& mgr = postWidget->interaction()->getToolManager();
        mgr->setActive(mxr::InteractionToolType::CFDAnnotation);
        const auto& tool = mgr->active<mxr::InteractionCFDAnnotationTool>();

        QColor color = _btnColor->property("currentColor").value<QColor>();
        tool->setArrowAnnotation(
            getCVM->getPostRenderWidget()->getSymbolController(),
            { (float)color.red() / 255.0f, (float)color.green() / 255.0f, (float)color.blue() / 255.0f },
            _spinWidth->value()
        );
    }

    void mPostArrowPanel::setCurViewName(const QString& viewName)
    {
        _curViewName = viewName;

        //界面更新
        updatePanel();
    }

    void mPostArrowPanel::updatePanel()
    {

    }

    void mPostArrowPanel::slot_setColor()
    {
        //获取颜色
        QColor color;
        color = QColorDialog::getColor(Qt::black, this, QString("选择颜色"));

        if (!color.isValid()) return;

        _btnColor->setStyleSheet(QString("background-color: rgb(%1, %2, %3);")
            .arg(color.red()).arg(color.green()).arg(color.blue()));
        _btnColor->setProperty("currentColor", color);

        auto* postWidget = getCVM->getPostWidget();
        if (postWidget)
        {
            const auto& mgr = postWidget->interaction()->getToolManager();
            mgr->setActive(mxr::InteractionToolType::CFDAnnotation);
            const auto& tool = mgr->active<mxr::InteractionCFDAnnotationTool>();
            tool->setArrowAnnotation(
                getCVM->getPostRenderWidget()->getSymbolController(),
                { (float)color.red() / 255.0f, (float)color.green() / 255.0f, (float)color.blue() / 255.0f },
                _spinWidth->value()
            );
        }
    }

    void mPostArrowPanel::slot_changeType()
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

    void mPostArrowPanel::slot_setSlider(int value)
    {
        _spinTransparency->setValue(value);
    }

    void mPostArrowPanel::slot_setSpinBox(int value)
    {
        _sliderTransparency->setValue(value);
    }

    void mPostArrowPanel::slot_setWidth(int value)
    {
        auto* postWidget = getCVM->getPostWidget();
        if (postWidget)
        {
            QColor color = _btnColor->property("currentColor").value<QColor>();
            const auto& mgr = postWidget->interaction()->getToolManager();
            mgr->setActive(mxr::InteractionToolType::CFDAnnotation);
            const auto& tool = mgr->active<mxr::InteractionCFDAnnotationTool>();
            tool->setArrowAnnotation(
                getCVM->getPostRenderWidget()->getSymbolController(),
                { (float)color.red() / 255.0f, (float)color.green() / 255.0f, (float)color.blue() / 255.0f },
                _spinWidth->value()
            );
        }
    }

    void mPostArrowPanel::slot_setType(int index)
    {
    }

}