
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
#include <QRegularExpression>
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
#include "mMeshData.h"
#include "mxr/view/mPreMeshHighLightRender.h"
#include "mxr/view/HighlightController.h"

#include "mxr/label/mSymbolController.h"
#include "mxr/label/SymbolDescription.h"
#include "PickCommonFunction.h"

#include "mInfoWindow.h"
#include "mCustomEncryptionPanel.h"
using namespace MechPreData;
using namespace MeshPanelData;
using namespace MxFunctions;
#include <set>
#include "mOtherFunction.h"
#include "MeshEntity.h" //【OCC摘除·S10/P0-1 终局2】本地自含（迭代 MeshEntity 完整类型；原经 mMeshThread.h 传递）
Q_DECLARE_METATYPE(std::set<int>)
Q_DECLARE_METATYPE(MeshParam)          // 头文件

namespace MechMainWindow
{
    // 函数：从 QString 中提取数字和字母
    void extractNumberAndText(const QString& input, QString& number, QString& text) {
        // 使用正则表达式匹配数字部分和字母部分
        QRegularExpression re("^(\\d+)([a-zA-Z]+)$");
        QRegularExpressionMatch match = re.match(input);

        if (match.hasMatch()) {
            number = match.captured(1); // 数字部分
            text = match.captured(2);  // 字母部分
        }
        else {
            number.clear(); // 如果没有匹配，清空结果
            text.clear();
        }
    }
    mCustomEncryptionPanel::mCustomEncryptionPanel(QWidget* parent)
    {
        auto* mainLay = new QVBoxLayout(this);

        m_boundingBoxFold = new FoldSection("定义包围盒");
        shape = new PanelLabel("形状");
        method = new PanelLabel("方式");
        num = new PanelLabel("数量");
        thickness = new PanelLabel("过渡厚度");
        shapeBox = new QComboBox;  shapeBox->addItems({ "箱形", "球形" , "圆柱形","非规则" });//
        methodBox = new QComboBox; methodBox->addItems({ "基于几何","输入参数" });

        QStackedWidget* stackWidget = new QStackedWidget();
        QWidget* widget1 = new QWidget;
        QWidget* widget2 = new QWidget;
        QWidget* widget3 = new QWidget;
        QWidget* widget4 = new QWidget;
        stackWidget->addWidget(widget1);
        stackWidget->addWidget(widget2);
        stackWidget->addWidget(widget3);
        stackWidget->addWidget(widget4);
        stackWidget->setCurrentIndex(3);
        //箱形
        vertex1 = new PanelLabel("角点1");
        vertex2 = new PanelLabel("角点2");
        vertex1Edit = new QLineEdit; 
        vertex2Edit = new QLineEdit;  
        //球形
        center = new PanelLabel("球心");
        radius = new PanelLabel("半径");
        centerEdit = new QLineEdit;  
        radiusEdit = new mCalcLineEdit;   radiusEdit->setUnit("mm");
        //底面圆心
        downCenter = new PanelLabel("圆心");
        downRadius = new PanelLabel("半径");
        height = new PanelLabel("高度");
        axial = new PanelLabel("轴向");
        downCenterEdit = new QLineEdit;  
        downRadiusEdit = new mCalcLineEdit;  downRadiusEdit->setUnit("mm");
        heightEdit = new mCalcLineEdit;   heightEdit->setUnit("mm");
        axialEdit = new QLineEdit;   



        numBox = new QComboBox; numBox->addItems({ "单个","多个" });
        thicknessEdit = new mCalcLineEdit("1"); thicknessEdit->setUnit("mm");
        auto* thick = new QDoubleValidator(0.001, 1e11, 8, thicknessEdit);
        thick->setNotation(QDoubleValidator::ScientificNotation);
        thicknessEdit->setValidator(thick);
        m_boundingBoxFold->addRow(shape, shapeBox);
        m_boundingBoxFold->addRow(method, methodBox);
        m_boundingBoxFold->addRow(num, numBox);
        m_boundingBoxFold->addRow(thickness, thicknessEdit);

        m_boundingBoxFold->addRow(stackWidget);


        QFormLayout* m_formLayout = new QFormLayout(widget1);
        m_formLayout->setContentsMargins(0, 0, 0, 0);
        m_formLayout->setSpacing(10);
        QHBoxLayout* h = new QHBoxLayout();
        h->addWidget(vertex1Edit);
        m_formLayout->addRow(vertex1, h);
        h = new QHBoxLayout();
        h->addWidget(vertex2Edit);
        m_formLayout->addRow(vertex2, h);

        m_formLayout = new QFormLayout(widget2);
        m_formLayout->setContentsMargins(0, 0, 0, 0);
        m_formLayout->setSpacing(10);
        h = new QHBoxLayout();
        h->addWidget(centerEdit); 
        m_formLayout->addRow(center, h);
        h = new QHBoxLayout();
        h->addWidget(radiusEdit); 
        m_formLayout->addRow(radius, h);

        m_formLayout = new QFormLayout(widget3);
        m_formLayout->setContentsMargins(0, 0, 0, 0);
        m_formLayout->setSpacing(10);
        h = new QHBoxLayout();
        h->addWidget(downCenterEdit);
        m_formLayout->addRow(downCenter, h);
        h = new QHBoxLayout();
        h->addWidget(downRadiusEdit);
        m_formLayout->addRow(downRadius, h);
        h = new QHBoxLayout();
        h->addWidget(heightEdit); 
        m_formLayout->addRow(height, h);
        h = new QHBoxLayout();
        h->addWidget(axialEdit);
        m_formLayout->addRow(axial, h);



        m_meshSizeFold = new FoldSection("网格尺寸");
        inside = new PanelLabel("内部");
        outside = new PanelLabel("外部");
        insideEdit = new mCalcLineEdit(); insideEdit->setUnit("mm");   
        auto* v = new QDoubleValidator(0.001, 1e11, 8, insideEdit);
        v->setNotation(QDoubleValidator::ScientificNotation);
        insideEdit->setValidator(v);
        outsideEdit = new mCalcLineEdit; outsideEdit->setUnit("mm"); 
        auto* v1 = new QDoubleValidator(0.001, 1e11, 8, outsideEdit);
        v1->setNotation(QDoubleValidator::ScientificNotation);
        outsideEdit->setValidator(v1);
        m_meshSizeFold->addRow(inside, insideEdit);
        m_meshSizeFold->addRow(outside, outsideEdit);

        m_offsetFold = new FoldSection("偏移");
        x = new PanelLabel("x方向");
        y = new PanelLabel("y方向");
        z = new PanelLabel("z方向");
        xEdit = new mCalcLineEdit("0"); xEdit->setUnit("mm"); 
        yEdit = new mCalcLineEdit("0"); yEdit->setUnit("mm");   
        zEdit = new mCalcLineEdit("0"); zEdit->setUnit("mm");
        m_offsetFold->addRow(x, xEdit);
        m_offsetFold->addRow(y, yEdit);
        m_offsetFold->addRow(z, zEdit);

        m_stretchFold = new FoldSection("拉伸");
        xup = new PanelLabel("x+方向");
        xdown = new PanelLabel("x-方向");
        yup = new PanelLabel("y+方向");
        ydown = new PanelLabel("y-方向");
        zup = new PanelLabel("z+方向");
        zdown = new PanelLabel("z-方向");
        xupEdit = new mCalcLineEdit("0"); 
        xdownEdit = new mCalcLineEdit("0");  
        yupEdit = new mCalcLineEdit("0");  
        ydownEdit = new mCalcLineEdit("0");    
        zupEdit = new mCalcLineEdit("0");    
        zdownEdit = new mCalcLineEdit("0");    

        m_stretchFold->addRow(xup, xupEdit);
        m_stretchFold->addRow(xdown, xdownEdit);
        m_stretchFold->addRow(yup, yupEdit);
        m_stretchFold->addRow(ydown, ydownEdit);
        m_stretchFold->addRow(zup, zupEdit);
        m_stretchFold->addRow(zdown, zdownEdit);

        mainLay->addWidget(m_boundingBoxFold);
        mainLay->addWidget(m_meshSizeFold);
        mainLay->addWidget(m_offsetFold);
        mainLay->addWidget(m_stretchFold);
        mainLay->addStretch();

        stackWidget->setFixedHeight(widget4->sizeHint().height());
        // 或
        stackWidget->adjustSize();
        connect(shapeBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() {
            _meshPanelData->setShape(shapeBox->currentText());
            if (_meshPanelData->getShape() == "非规则")
            {
                methodBox->clear();
                methodBox->addItems({ "基于几何体" });
                m_stretchFold->hide();
            }
            else if (_meshPanelData->getShape() == "箱形")
            {
                methodBox->clear();
                methodBox->addItems({ "基于几何","输入参数" });
                if (_meshPanelData->getMethod() == "输入参数")
                {
                    methodBox->setCurrentIndex(1);
                }
                m_stretchFold->show();
            }
            else//球形、圆柱形
            {
                methodBox->clear();
                methodBox->addItems({"输入参数" });
                if (_meshPanelData->getMethod() == "输入参数")
                {
                    methodBox->setCurrentIndex(0);
                }
                m_stretchFold->hide();
            }
            });
        connect(methodBox, QOverload<const QString&>::of(&QComboBox::currentTextChanged), [=]() {
            //hideAllParam();
            _meshPanelData->setMethod(methodBox->currentText());
            if (_meshPanelData->getMethod() == "基于几何")
            {
                numBox->clear();
                numBox->addItems({ "单个","多个" });
                if (_meshPanelData->getNum() == "多个")
                {
                    numBox->setCurrentIndex(1);
                }
                stackWidget->setCurrentIndex(3);
            }
            else if (_meshPanelData->getMethod() == "基于几何体")
            {
                numBox->clear();
                numBox->addItems({ "单个" });
            }
            else if (_meshPanelData->getMethod() == "输入参数")//输入参数
            {
                numBox->clear();
                numBox->addItems({ "单个" });
                if (_meshPanelData->getShape() == "箱形")
                {
                    stackWidget->setCurrentIndex(0);
                }
                else if (_meshPanelData->getShape() == "球形")
                {
                    stackWidget->setCurrentIndex(1);
                }
                else
                {
                    stackWidget->setCurrentIndex(2);
                }
            }
            m_boundingBoxFold->refreshLayout();
            });
        // 关键：安装事件过滤器，切换时调整
        connect(stackWidget, &QStackedWidget::currentChanged, this, [=](int index) {
            QWidget* current = stackWidget->widget(index);
            if (current) {
                stackWidget->setFixedHeight(current->sizeHint().height());
                // 或
                stackWidget->adjustSize();
                //m_ParamFold->refreshLayout();
            }
            });
        connect(numBox, &QComboBox::currentTextChanged, this, [=]() {_meshPanelData->setNum(numBox->currentText()); });
        connect(insideEdit, &QLineEdit::textChanged, [=]() { if (insideEdit->text().isEmpty()) return;  _meshPanelData->setInsideSize(insideEdit->text().toDouble()); });
        connect(outsideEdit, &QLineEdit::textChanged, [=]() {if (outsideEdit->text().isEmpty()) return; _meshPanelData->setOutsideSize(outsideEdit->text().toDouble()); });
        connect(xEdit, &QLineEdit::textChanged, this, &mCustomEncryptionPanel::offsetChanged);
        connect(yEdit, &QLineEdit::textChanged, this, &mCustomEncryptionPanel::offsetChanged);
        connect(zEdit, &QLineEdit::textChanged, this, &mCustomEncryptionPanel::offsetChanged);
        connect(xupEdit, &QLineEdit::textChanged, this, &mCustomEncryptionPanel::stretchChanged);
        connect(xdownEdit, &QLineEdit::textChanged, this, &mCustomEncryptionPanel::stretchChanged);
        connect(yupEdit, &QLineEdit::textChanged, this, &mCustomEncryptionPanel::stretchChanged);
        connect(ydownEdit, &QLineEdit::textChanged, this, &mCustomEncryptionPanel::stretchChanged);
        connect(zupEdit, &QLineEdit::textChanged, this, &mCustomEncryptionPanel::stretchChanged);
        connect(zdownEdit, &QLineEdit::textChanged, this, &mCustomEncryptionPanel::stretchChanged);
        connect(vertex1Edit, &QLineEdit::textChanged, this, [=]() {if (vertex1Edit->text().isEmpty()) return; _meshPanelData->setVertex1(vertex1Edit->text()); });
        connect(vertex2Edit, &QLineEdit::textChanged, this, [=]() {if (vertex2Edit->text().isEmpty()) return; _meshPanelData->setVertex2(vertex2Edit->text()); });
        connect(centerEdit, &QLineEdit::textChanged, this, [=]() {if (centerEdit->text().isEmpty()) return; _meshPanelData->setCenter(centerEdit->text()); });
        connect(radiusEdit, &QLineEdit::textChanged, this, [=]() {if (radiusEdit->text().isEmpty()) return; _meshPanelData->setRadius(radiusEdit->text().toDouble()); });
        connect(downCenterEdit, &QLineEdit::textChanged, this, [=]() {if (downCenterEdit->text().isEmpty()) return; _meshPanelData->setDownCenter(downCenterEdit->text()); });
        connect(downRadiusEdit, &QLineEdit::textChanged, this, [=]() {if (downRadiusEdit->text().isEmpty()) return; _meshPanelData->setDownRadius(downRadiusEdit->text().toDouble()); });
        connect(heightEdit, &QLineEdit::textChanged, this, [=]() {if (heightEdit->text().isEmpty()) return; _meshPanelData->setHeight(heightEdit->text().toDouble()); });
        connect(axialEdit, &QLineEdit::textChanged, this, [=]() {if (axialEdit->text().isEmpty()) return; _meshPanelData->setAxial(axialEdit->text()); });
        connect(thicknessEdit, &QLineEdit::textChanged, this, [=]() {
            if (thicknessEdit->text().isEmpty()) return;
            _meshPanelData->setThickness(thicknessEdit->text().toDouble());
            boxChange();
            });




    }
    mCustomEncryptionPanel::~mCustomEncryptionPanel()
    {
    }
    void mCustomEncryptionPanel::newMeshPanel()
    {
    }
    void mCustomEncryptionPanel::newCustomEncryptionPanel()
    {
    }
    void mCustomEncryptionPanel::editMeshPanel(int meshId)
    {
        _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(meshId);
        auto a = _meshPanelData->getMeshPanelType();
        currentId = meshId;
        updatePanel();
    }
    void mCustomEncryptionPanel::getMeshInfoById(const std::vector<int> ids)
    {
        if (ids.size() <= 0)
        {
            return;
        }
        if (ids.size() == 1)
        {
            return;
        }
        _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(ids[0]);
        QString firstShape = _meshPanelData->getShape();
        QString firstMethod = _meshPanelData->getMethod();
        QString firstNum = _meshPanelData->getNum();
        double firstThickness = _meshPanelData->getThickness();
        double firstInsideSize = _meshPanelData->getInsideSize();
        double firstOutsideSize = _meshPanelData->getOutsideSize();
        double firstX = _meshPanelData->getOffset().x;
        double firstY = _meshPanelData->getOffset().y;
        double firstZ = _meshPanelData->getOffset().z;
        double firstXup = _meshPanelData->getStretch().xup;
        double firstXdown = _meshPanelData->getStretch().xdown;
        double firstYup = _meshPanelData->getStretch().yup;
        double firstYdown = _meshPanelData->getStretch().ydown;
        double firstZup = _meshPanelData->getStretch().zup;
        double firstZdown = _meshPanelData->getStretch().zdown;
        QString firstVertex1 = _meshPanelData->getVertex1();
        QString firstVertex2 = _meshPanelData->getVertex2();
        QString firstCenter = _meshPanelData->getCenter();
        double firstRadius = _meshPanelData->getRadius();
        QString firstDownCenter = _meshPanelData->getDownCenter();
        double firstDownRadius = _meshPanelData->getDownRadius();
        double firstHeight = _meshPanelData->getHeight();
        QString firstAxial = _meshPanelData->getAxial();

        shapeBox->setCurrentText(firstShape);
        methodBox->setCurrentText(firstMethod);
        numBox->setCurrentText(firstNum);
        thicknessEdit->setExpression(MxFunctions::mNumberToString(firstThickness));
        insideEdit->setExpression(MxFunctions::mNumberToString(firstInsideSize));
        outsideEdit->setExpression(MxFunctions::mNumberToString(firstOutsideSize));
        xEdit->setExpression(MxFunctions::mNumberToString(firstX));
        yEdit->setExpression(MxFunctions::mNumberToString(firstY));
        zEdit->setExpression(MxFunctions::mNumberToString(firstZ));
        xupEdit->setExpression(MxFunctions::mNumberToString(firstXup));
        xdownEdit->setExpression(MxFunctions::mNumberToString(firstXdown));
        yupEdit->setExpression(MxFunctions::mNumberToString(firstYup));
        ydownEdit->setExpression(MxFunctions::mNumberToString(firstYdown));
        zupEdit->setExpression(MxFunctions::mNumberToString(firstZup));
        zdownEdit->setExpression(MxFunctions::mNumberToString(firstZdown));
        vertex1Edit->setText(firstVertex1);
        vertex2Edit->setText(firstVertex2);
        centerEdit->setText(firstCenter);
        radiusEdit->setExpression(MxFunctions::mNumberToString(firstRadius));
        downCenterEdit->setText(firstDownCenter);
        downRadiusEdit->setExpression(MxFunctions::mNumberToString(firstDownRadius));
        heightEdit->setExpression(MxFunctions::mNumberToString(firstHeight));
        axialEdit->setText(firstAxial);


        //网格形状
        for (auto id : ids)
        {
            _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
            if (_meshPanelData->getShape() != firstShape)
            {
                shapeBox->setCurrentIndex(-1);
                methodBox->setCurrentIndex(-1);
                numBox->setCurrentIndex(-1);
            }
            if (_meshPanelData->getMethod() != firstMethod)
            {
                methodBox->setCurrentIndex(-1);
                numBox->setCurrentIndex(-1);
            }
            if (_meshPanelData->getNum() != firstNum)
            {
                numBox->setCurrentIndex(-1);
            }
            if (_meshPanelData->getThickness() != firstThickness)
            {
                thicknessEdit->setText("");
            }
            if (_meshPanelData->getInsideSize() != firstInsideSize)
            {
                insideEdit->setText("");
            }
            if (_meshPanelData->getOutsideSize() != firstOutsideSize)
            {
                outsideEdit->setText("");
            }
            if (_meshPanelData->getOffset().x != firstX)
            {
                xEdit->setText("");
            }
            if (_meshPanelData->getOffset().y != firstY)
            {
                yEdit->setText("");
            }
            if (_meshPanelData->getOffset().z != firstZ)
            {
                zEdit->setText("");
            }
            if (_meshPanelData->getStretch().xup != firstXup)
            {
                xupEdit->setText("");
            }
            if (_meshPanelData->getStretch().xdown != firstXdown)
            {
                xdownEdit->setText("");
            }
            if (_meshPanelData->getStretch().yup != firstYup)
            {
                yupEdit->setText("");
            }
            if (_meshPanelData->getStretch().ydown != firstYdown)
            {
                ydownEdit->setText("");
            }
            if (_meshPanelData->getStretch().zup != firstZup)
            {
                zupEdit->setText("");
            }
            if (_meshPanelData->getStretch().zdown != firstZdown)
            {
                zdownEdit->setText("");
            }
            if (_meshPanelData->getVertex1() != firstVertex1)
                vertex1Edit->setText("");
            if (_meshPanelData->getVertex2() != firstVertex2)
                vertex2Edit->setText("");
            if (_meshPanelData->getCenter() != firstCenter)
                centerEdit->setText("");
            if (_meshPanelData->getRadius() != firstRadius)
                radiusEdit->setText("");
            if (_meshPanelData->getDownCenter() != firstDownCenter)
                downCenterEdit->setText("");
            if (_meshPanelData->getDownRadius() != firstDownRadius)
                downRadiusEdit->setText("");
            if (_meshPanelData->getHeight() != firstHeight)
                heightEdit->setText("");
            if (_meshPanelData->getAxial() != firstAxial)
                axialEdit->setText("");
        }
        connect(thicknessEdit, &QLineEdit::textChanged, [=]() {
            for (auto id : ids)
            {
                _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                _meshPanelData->setThickness(thicknessEdit->text().toDouble());
            }});
        connect(insideEdit, &QLineEdit::textChanged, [=]() {
            for (auto id : ids)
            {
                _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                _meshPanelData->setInsideSize(insideEdit->text().toDouble());
            }});
        connect(outsideEdit, &QLineEdit::textChanged, [=]() {
            for (auto id : ids)
            {
                _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                _meshPanelData->setOutsideSize(outsideEdit->text().toDouble());
            } });
        connect(xEdit, &QLineEdit::textChanged, [=]() {
            for (auto id : ids)
            {
                _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                Offset offset; offset.x = xEdit->text().toDouble(); offset.y = _meshPanelData->getOffset().y; offset.z = _meshPanelData->getOffset().z;
                _meshPanelData->setOffset(offset);
            } });
        connect(yEdit, &QLineEdit::textChanged, [=]() {
            for (auto id : ids)
            {
                _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                Offset offset; offset.x = _meshPanelData->getOffset().x; offset.y = yEdit->text().toDouble(); offset.z = _meshPanelData->getOffset().z;
                _meshPanelData->setOffset(offset);
            } });
        connect(zEdit, &QLineEdit::textChanged, [=]() {
            for (auto id : ids)
            {
                _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                Offset offset; offset.x = _meshPanelData->getOffset().x; offset.y = _meshPanelData->getOffset().y; offset.z = zEdit->text().toDouble();
                _meshPanelData->setOffset(offset);
            } });
        connect(xupEdit, &QLineEdit::textChanged, [=]() {
            for (auto id : ids)
            {
                _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                _meshPanelData->setStretch({ xupEdit->text().toDouble(), _meshPanelData->getStretch().xdown,
                    _meshPanelData->getStretch().yup, _meshPanelData->getStretch().ydown,
                    _meshPanelData->getStretch().zup, _meshPanelData->getStretch().zdown });
            } });
        connect(xdownEdit, &QLineEdit::textChanged, [=]() {
            for (auto id : ids)
            {
                _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                _meshPanelData->setStretch({ _meshPanelData->getStretch().xup, xdownEdit->text().toDouble(),
                        _meshPanelData->getStretch().yup, _meshPanelData->getStretch().ydown,
                        _meshPanelData->getStretch().zup, _meshPanelData->getStretch().zdown });
            } });
        connect(yupEdit, &QLineEdit::textChanged, [=]() {
            for (auto id : ids)
            {
                _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                _meshPanelData->setStretch({ _meshPanelData->getStretch().xup, _meshPanelData->getStretch().xdown,
                        yupEdit->text().toDouble(), _meshPanelData->getStretch().ydown,
                        _meshPanelData->getStretch().zup, _meshPanelData->getStretch().zdown });
            } });
        connect(ydownEdit, &QLineEdit::textChanged, [=]() {
            for (auto id : ids)
            {
                _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                _meshPanelData->setStretch({ _meshPanelData->getStretch().xup, _meshPanelData->getStretch().xdown,
                    _meshPanelData->getStretch().yup, ydownEdit->text().toDouble(),
                    _meshPanelData->getStretch().zup, _meshPanelData->getStretch().zdown });
            } });
        connect(zupEdit, &QLineEdit::textChanged, [=]() {
            for (auto id : ids)
            {
                _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                _meshPanelData->setStretch({ _meshPanelData->getStretch().xup, _meshPanelData->getStretch().xdown,
                    _meshPanelData->getStretch().yup,  _meshPanelData->getStretch().ydown,
                    zupEdit->text().toDouble(), _meshPanelData->getStretch().zdown });
            } });
        connect(zdownEdit, &QLineEdit::textChanged, [=]() {
            for (auto id : ids)
            {
                _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                _meshPanelData->setStretch({ _meshPanelData->getStretch().xup, _meshPanelData->getStretch().xdown,
                    _meshPanelData->getStretch().yup,  _meshPanelData->getStretch().ydown,
                    _meshPanelData->getStretch().zup, zdownEdit->text().toDouble() });
            }
            });
        connect(vertex1Edit, &QLineEdit::textChanged, [=]() {
            for (auto id : ids)
            {
                _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                _meshPanelData->setVertex1(vertex1Edit->text());
            }
            });
        connect(vertex2Edit, &QLineEdit::textChanged, [=]() {
            for (auto id : ids)
            {
                _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                _meshPanelData->setVertex2(vertex2Edit->text());
            }
            });
        connect(centerEdit, &QLineEdit::textChanged, [=]() {
            for (auto id : ids)
            {
                _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                _meshPanelData->setCenter(centerEdit->text());
            }
            });
        connect(radiusEdit, &QLineEdit::textChanged, [=]() {
            for (auto id : ids)
            {
                _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                _meshPanelData->setRadius(radiusEdit->text().toDouble());
            }
            });
        connect(downCenterEdit, &QLineEdit::textChanged, [=]()
            {
                for (auto id : ids)
                {
                    _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                    _meshPanelData->setDownCenter(downCenterEdit->text());
                }
            }
        );
        connect(downRadiusEdit, &QLineEdit::textChanged, [=]()
            {
                for (auto id : ids)
                {
                    _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                    _meshPanelData->setDownRadius(downRadiusEdit->text().toDouble());
                }
            }
        );
        connect(heightEdit, &QLineEdit::textChanged, [=]()
            {
                for (auto id : ids)
                {
                    _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                    _meshPanelData->setHeight(heightEdit->text().toDouble());
                }
            }
        );
        connect(axialEdit, &QLineEdit::textChanged, [=]()
            {
                for (auto id : ids)
                {
                    _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id);
                    _meshPanelData->setAxial(axialEdit->text());
                }
            }
        );
    }
    void mCustomEncryptionPanel::updatePanel()
    {
        slot_boxHide();
        vector<QString> showSymbol;
        showSymbol.insert(showSymbol.end(), _meshPanelData->_Renderlabel.begin(), _meshPanelData->_Renderlabel.end());       
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        symbolController->setSymbolShowState(showSymbol, true);
        symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
        auto strs = _meshPanelData->_PreviewGeos;
        vector<int> ids; QString type;
        for (auto str : strs)
        {
            QStringList strlist = str.split("_");
            type = strlist[0];
            int id = strlist[1].toInt();
            ids.push_back(id);
        }
        if (type == "体")
        {
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(4);
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::GeoSolid, ids);

        }
        else if (type == "面")
        {
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(3);
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::GeoFace, ids);

        }
        else
        {
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::GeoEdge, ids);

        }    
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();


        Offset offset = _meshPanelData->getOffset();
        xEdit->setExpression(MxFunctions::mNumberToString(offset.x));
        yEdit->setExpression(MxFunctions::mNumberToString(offset.y));
        zEdit->setExpression(MxFunctions::mNumberToString(offset.z));
        Stretch stretch = _meshPanelData->getStretch();
        xupEdit->setExpression(MxFunctions::mNumberToString(stretch.xup));
        xdownEdit->setExpression(MxFunctions::mNumberToString(stretch.xdown));
        yupEdit->setExpression(MxFunctions::mNumberToString(stretch.yup));
        ydownEdit->setExpression(MxFunctions::mNumberToString(stretch.ydown));
        zupEdit->setExpression(MxFunctions::mNumberToString(stretch.zup));
        zdownEdit->setExpression(MxFunctions::mNumberToString(stretch.zdown));
        insideEdit->setExpression(MxFunctions::mNumberToString(_meshPanelData->getInsideSize()));
        outsideEdit->setExpression(MxFunctions::mNumberToString(_meshPanelData->getOutsideSize()));
        vertex1Edit->setText(_meshPanelData->getVertex1());
        vertex2Edit->setText(_meshPanelData->getVertex2());
        centerEdit->setText(_meshPanelData->getCenter());
        radiusEdit->setExpression(MxFunctions::mNumberToString(_meshPanelData->getRadius()));
        downCenterEdit->setText(_meshPanelData->getDownCenter());
        downRadiusEdit->setExpression(MxFunctions::mNumberToString(_meshPanelData->getDownRadius()));
        heightEdit->setExpression(MxFunctions::mNumberToString(_meshPanelData->getHeight()));
        axialEdit->setText(_meshPanelData->getAxial());
        thicknessEdit->setExpression(MxFunctions::mNumberToString(_meshPanelData->getThickness()));
        shapeBox->setCurrentText(_meshPanelData->getShape());
        QString a = _meshPanelData->getMethod();
        methodBox->setCurrentText(_meshPanelData->getMethod());
        numBox->setCurrentText(_meshPanelData->getNum());
    }

    void mCustomEncryptionPanel::boxChangeByParam(MeshEntity* Entity)
    {

        FieldManager* manager = Entity->getFields();
        QString name = _meshPanelData->_PreviewRenderlabel[0];
        QString number;
        QString type;
        extractNumberAndText(name, number, type);
        int id = number.toInt();

        if (_meshPanelData->getShape() == "箱形")
        {
            // 创建自定义Box尺寸场  在创建newid之前先判断当前所选的listitem是不是已经为背景网格
             //创建自定义尺寸场
            if (type != "box")
                return;
            SizeField* BoxField = manager->get(id);
            if (BoxField == NULL) {
                BoxField = manager->Create(id, "Box");
            }
            //将尺寸场设置为背景尺寸场
            setValueBoxBySetting(BoxField);

;
            double min[3]{ 0.0 };
            double max[3]{ 0.0 };
            QStringList v1 = _meshPanelData->getVertex1().split(',');
            QStringList v2 = _meshPanelData->getVertex2().split(',');
            if (v1.size() != 3 || v2.size() != 3)
                return;
            min[0] = v1[0].toDouble();
            min[1] = v1[1].toDouble();
            min[2] = v1[2].toDouble();
            max[0] = v2[0].toDouble();
            max[1] = v2[1].toDouble();
            max[2] = v2[2].toDouble();
            double thick = _meshPanelData->getThickness();
            QVector3D normalx = QVector3D(1, 0, 0);
            QVector3D normaly = QVector3D(0, 1, 0);
            QVector3D normalz = QVector3D(0, 0, 1);
            //与尺寸场一致：先拉伸(以中心为基准)再平移偏移
            applyStretchOffset(min, max);
            QVector3D pos = QVector3D((min[0] + max[0]) / 2, (min[1] + max[1]) / 2, (min[2] + max[2]) / 2);
            float lx = max[0] - min[0];
            float ly = max[1] - min[1];
            float lz = max[2] - min[2];
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            for (auto name : _meshPanelData->_PreviewRenderlabel)
            {
                QString number;
                QString type;
                extractNumberAndText(name,number,type);
                if (type == "box")
                {  
                    auto box = symbolController->getOrCreateSymbolData<Box_Explicit>(name);
                    box->_size = _meshPanelData->getThickness();
                    box->_pos = pos;
                    box->_normalx = normalx;
                    box->_normaly = normaly;
                    box->_normalz = normalz;
                    box->_L_x = lx;
                    box->_L_y = ly;
                    box->_L_z = lz;
                    box->_color = QVector3D(94, 203, 102);
                }
                else
                {
                    auto thickbox = symbolController->getOrCreateSymbolData<Box_Explicit>(name);
                    thickbox->_size = _meshPanelData->getThickness();
                    thickbox->_pos = pos;
                    thickbox->_normalx = normalx;
                    thickbox->_normaly = normaly;
                    thickbox->_normalz = normalz;
                    thickbox->_L_x = lx + 2 * thick;
                    thickbox->_L_y = ly + 2 * thick;
                    thickbox->_L_z = lz + 2 * thick;
                    thickbox->_color = QVector3D(94, 203, 102);
                }

            } 
            symbolController->UpdateAllSymbol();
            getCVM->getPreRenderWidget()->updatePaint();
            //mViewportManager::getInstance()->renderBoundingBox(str + "-thickbox", min, max);  //加了过渡层之后的包围盒
            QString str = MxFunctions::mNumberToString(BoxField->id);
    
        }
        else if (_meshPanelData->getShape() == "球形")
        {
            if (type != "ball")
                return;
            // 创建自定义Ball尺寸场 
            FieldManager* manger = Entity->getFields();

            SizeField* BallField = manager->get(id);
            if (BallField == NULL) {
                BallField = manager->Create(id, "Ball");
            }
            // 将尺寸场设置为背景尺寸场
            setValueBallBySetting(BallField);
            //渲染规则加密区域包围盒
            QString str = MxFunctions::mNumberToString(BallField->id);
            QStringList center = _meshPanelData->getCenter().split(',');
            if (center.size() != 3)
                return;
            double xcenter = center[0].toDouble();
            double ycenter = center[1].toDouble();
            double zcenter = center[2].toDouble();
            double radius = _meshPanelData->getRadius();
            double Thickness = _meshPanelData->getThickness();
            //偏移
            double offsetX = _meshPanelData->getOffset().x;
            double offsetY = _meshPanelData->getOffset().y;
            double offsetZ = _meshPanelData->getOffset().z;
            QVector3D _center(xcenter + offsetX, ycenter + offsetY, zcenter + offsetZ);
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();

            for (auto name : _meshPanelData->_PreviewRenderlabel)
            {
                QString number;
                QString type;
                extractNumberAndText(name, number, type);
                if (type == "ball")
                {
                    auto ball = symbolController->getOrCreateSymbolData<Sphere_RW>(name);
                    //mViewportManager::getInstance()->renderSphere(str + "-ball", center, radius);
                    ball->_pos = _center;
                    ball->_radius = radius;
                    ball->_color = QVector3D(94, 203, 102);
                }
                else if(type=="thickball")
                {
                    //mViewportManager::getInstance()->renderSphere(str + "-thickball", center, radius);
                    auto thickball = symbolController->getOrCreateSymbolData<Sphere_RW>(name);
                    //mViewportManager::getInstance()->renderSphere(str + "-ball", center, radius);
                    thickball->_pos = _center;
                    thickball->_radius = radius + Thickness;;
                    thickball->_color = QVector3D(94, 203, 102);
                }
            }

   
            symbolController->UpdateAllSymbol();
            getCVM->getPreRenderWidget()->updatePaint();
        }
        else if (_meshPanelData->getShape() == "圆柱形")
        {
            if (type != "cylinder")
                return;
            if (_meshPanelData->getDownCenter().split(',').size() != 3 ||
                _meshPanelData->getAxial().split(',').size() != 3)
            {
                return;
            }
            // 创建自定义Cylinder尺寸场
            FieldManager* manger = Entity->getFields();
            SizeField* CylinderField = manager->get(id);
            if (CylinderField == NULL) {
                CylinderField = manager->Create(id, "Cylinder");
            }
            setValueCylinderBySetting(CylinderField);
            //渲染规则加密区域包围盒
            QStringList downCenter = _meshPanelData->getDownCenter().split(',');
            QStringList normal = _meshPanelData->getAxial().split(',');
            if (downCenter.size() != 3 || normal.size() != 3)
                return;
            auto xcenter = downCenter[0].toDouble();
            auto ycenter = downCenter[1].toDouble();
            auto zcenter = downCenter[2].toDouble();
            auto radius = _meshPanelData->getDownRadius();
            /*      auto xaxis = options["XAxis"]->numericalValue();
            auto yaxis = options["YAxis"]->numericalValue();
            auto zaxis = options["ZAxis"]->numericalValue();*/
            auto l = _meshPanelData->getHeight();
            auto Thickness = _meshPanelData->getThickness();
            double xnormal = normal[0].toDouble();
            double ynormal = normal[1].toDouble();
            double znormal = normal[2].toDouble();
            //偏移
            double offsetX = _meshPanelData->getOffset().x;
            double offsetY = _meshPanelData->getOffset().y;
            double offsetZ = _meshPanelData->getOffset().z;
            QVector3D _normal(xnormal, ynormal, znormal);
            QVector3D _center(xcenter + offsetX, ycenter + offsetY, zcenter + offsetZ);
            //QVector3D xyz_axis(xaxis, yaxis, zaxis);
            //mViewportManager::getInstance()->renderCylinder(str + "-cylinder", center, xyz_axis, radius);
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            for (auto name : _meshPanelData->_PreviewRenderlabel)
            {
                QString number;
                QString type;
                extractNumberAndText(name, number, type);
                if (type == "cylinder")
                {
                    auto cylinder = symbolController->getOrCreateSymbolData<Cylinder_RW>(name);
                    //mViewportManager::getInstance()->renderSphere(str + "-ball", center, radius);
                    cylinder->_pos = _center;
                    cylinder->_normal = _normal;
                    cylinder->_radius = radius;
                    cylinder->_L = l;
                    cylinder->_color = QVector3D(94, 203, 102);
                }
                else
                {
                    auto thickcylinder = symbolController->getOrCreateSymbolData<Cylinder_RW>(name);
                    //mViewportManager::getInstance()->renderSphere(str + "-ball", center, radius);
                    thickcylinder->_pos = _center;
                    thickcylinder->_normal = _normal;
                    thickcylinder->_radius = radius+ Thickness;
                    thickcylinder->_L = l;
                    thickcylinder->_color = QVector3D(94, 203, 102);
                }
            }    
            symbolController->UpdateAllSymbol();
            getCVM->getPreRenderWidget()->updatePaint();
            //mViewportManager::getInstance()->renderCylinder(str + "-thickcylinder", center, xyz_axis, radius);
            QString str = MxFunctions::mNumberToString(CylinderField->id);

        }
    }

    void mCustomEncryptionPanel::createEncryptionByGeo(MeshEntity* Entity)
    {
        //_meshPanelData->_PreviewGeos当前拾取的几何线/面/体名称
        _meshPanelData->_PreviewGeos.clear();
        QVector<SBoundingBox3d>BoundingBoxVec;
        auto pickDilter = getCVM->getPreRenderWidget()->getPickFilter();
        //拾取体
        if (pickDilter == PickFilter::PickGeoSolid)
        {
            vector<int> solidIds = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoSolid);
            for (int id : solidIds)
            {
                QString geoName = QString("体_%1").arg(id);
                _meshPanelData->_PreviewGeos.push_back(geoName);
            }

            if (solidIds.empty())
                return;
            QString str = "体加密对象选择成功！";
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::NormalMessage, str, true);
            if (_meshPanelData->getNum() == "单个") {
                SBoundingBox3d bd;
                if (solidIds.size()) {
                    for (auto it : Entity->getGeoSolids())
                    {
                        int tag = it->getGeoTag();
                        for (auto tagSet : solidIds)
                        {
                            if (tag == tagSet)
                            {
                                QVector<MXGeoFace*> faces = it->getface();
                                for (auto face : faces)
                                {
                                    bd += face->getBound();
                                }
                            }
                        }
                    }
                }
                BoundingBoxVec.push_back(bd);
            }
            else if (_meshPanelData->getNum() == "多个") {//连续创建即为每个拾取的部件都创建一个BoxField
                if (solidIds.size()) {
                    for (auto it : Entity->getGeoSolids())
                    {
                        int tag = it->getGeoTag();
                        for (auto tagSet : solidIds)
                        {
                            if (tag == tagSet)
                            {
                                SBoundingBox3d bd;
                                QVector<MXGeoFace*> faces = it->getface();
                                for (auto face : faces)
                                {
                                    bd += face->getBound();
                                }
                                BoundingBoxVec.push_back(bd);
                            }
                        }
                    }
                }
            }
        }
        //拾取面
        else if (pickDilter == PickFilter::PickGeoFace)
        {
            vector<int> faceIds = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
            for (int id : faceIds)
            {
                QString geoName = QString("面_%1").arg(id);
                _meshPanelData->_PreviewGeos.push_back(geoName);
            }
            set<int> _faceIDs(faceIds.begin(), faceIds.end());
            if (_faceIDs.empty())
                return;
            QString str = "面加密对象选择成功！";
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::NormalMessage, str, true);
            if (_meshPanelData->getNum() == "单个") {
                SBoundingBox3d bd;
                if (_faceIDs.size()) {
                    for (auto it : Entity->getGeoFaces())
                    {
                        int tag = it->getGeoTag();
                        for (auto tagSet : _faceIDs)
                        {
                            if (tag == tagSet)
                            {
                                bd += it->getBound();
                            }
                        }
                    }
                }
                BoundingBoxVec.push_back(bd);
            }
            else if (_meshPanelData->getNum() == "多个") {//连续创建即为每个拾取的部件都创建一个BoxField
                if (_faceIDs.size()) {
                    for (auto it : Entity->getGeoFaces())
                    {
                        int tag = it->getGeoTag();
                        for (auto tagSet : _faceIDs)
                        {
                            if (tag == tagSet)
                            {
                                SBoundingBox3d bd;
                                bd += it->getBound();
                                BoundingBoxVec.push_back(bd);
                            }
                        }
                    }
                }
            }
        }
        //拾取线
        else if (pickDilter == PickFilter::PickGeoLine)
        {
            vector<int> lineIds = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
            for (int id : lineIds)
            {
                QString geoName = QString("线_%1").arg(id);
                _meshPanelData->_PreviewGeos.push_back(geoName);
            }
            if (lineIds.empty())
            {
                QString str = "没有选中几何，线加密对象选择失败！";
                mGlobalSignals::getInstance()->outputMessageSig(1, str, true);
                return;
            }
            else {
                QString str = "线加密对象选择成功！";
                mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::NormalMessage, str, true);
            }
            if (_meshPanelData->getNum() == "单个") {
                SBoundingBox3d bd;
                if (lineIds.size()) {
                    for (auto it : Entity->getGeoEdges())
                    {
                        int tag = it->getGeoTag();
                        for (auto tagSet : lineIds)
                        {
                            if (tag == tagSet)
                            {
                                bd += it->getBound();
                            }
                        }
                    }
                }
                BoundingBoxVec.push_back(bd);
            }
            else if (_meshPanelData->getNum() == "多个") {
                //连续创建即为每个拾取的几何线都创建一个BoxField
                if (lineIds.size()) {
                    for (auto it : Entity->getGeoEdges())
                    {
                        int tag = it->getGeoTag();
                        for (auto tagSet : lineIds)
                        {
                            if (tag == tagSet)
                            {
                                SBoundingBox3d bd;
                                bd += it->getBound();
                                BoundingBoxVec.push_back(bd);
                            }
                        }
                    }
                }
            }
        }
        //清除几何的拾取
        getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightGeoModel();

        FieldManager* manger = Entity->getFields();
        //删除自定义尺寸场
        if (manger->size())
        {
            for (const auto& field : _meshPanelData->_Fields) {
                manger->deleteSizeField(field);
                manger->deleteBackgroundParaFieldById(field);
                manger->deleteBackgroundFieldById(field);
            }
            //manger->resetSizeField();
            QString str = "实体加密对象删除成功！";
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::NormalMessage, str, true);
        }

        if (!getCVM->getPreRenderWidget())
            return;
        //删除渲染包围盒
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        symbolController->deleteSymbol(_meshPanelData->_Renderlabel);
        _meshPanelData->_Renderlabel.clear();
        _meshPanelData->_Fields.clear();
        symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();

        for (auto bd : BoundingBoxVec) {
#pragma region
            //创建自定义尺寸场
            double min[3]{ 0.0 };
            double max[3]{ 0.0 };

           
            int newId = manger->newId();
            SizeField* BoxField = manger->Create(newId, "Box");
            manger->addBackgroundFieldById(newId);
            // 将尺寸场设置为背景尺寸场
            manger->setBackgroundField(BoxField);
            MeshPanelData::meshBox meshbox1;
            MeshPanelData::meshBox meshbox2;
            meshbox1.boxMax[0] = bd.max().x();
            meshbox1.boxMax[1] = bd.max().y();
            meshbox1.boxMax[2] = bd.max().z();
            meshbox1.boxMin[0] = bd.min().x();
            meshbox1.boxMin[1] = bd.min().y();
            meshbox1.boxMin[2] = bd.min().z();
            meshbox2.boxMax[0] = bd.max().x() + _meshPanelData->getThickness();
            meshbox2.boxMax[1] = bd.max().y() + _meshPanelData->getThickness();
            meshbox2.boxMax[2] = bd.max().z() + _meshPanelData->getThickness();
            meshbox2.boxMin[0] = bd.min().x() - _meshPanelData->getThickness();
            meshbox2.boxMin[1] = bd.min().y() - _meshPanelData->getThickness();
            meshbox2.boxMin[2] = bd.min().z() - _meshPanelData->getThickness();
#pragma endregion
            setValueBox(bd, min, max, BoxField);
            createRenderBox(bd, min, max, BoxField);
            QString str = MxFunctions::mNumberToString(BoxField->id);

        


            //保存包围盒信息
            _meshPanelData->_Fields.insert(newId);
            _meshPanelData->_Renderlabel.push_back(str + "box");
            _meshPanelData->_Renderlabel.push_back(str + "thickbox");

            _meshPanelData->_PreviewFields.insert(newId);
            
            _meshPanelData->_PreviewRenderlabel.push_back(str + "box");
            _meshPanelData->_PreviewRenderlabel.push_back(str + "thickbox");
            _meshPanelData->_meshBoxMap[str + "box"] = meshbox1;
            _meshPanelData->_meshBoxMap[str + "thickbox"] = meshbox2;



            
        }
    }
    //几何拾取已清空时，基于已保存的原始包围盒重建加密区域（套用当前偏移/拉伸）
    void mCustomEncryptionPanel::recreateBySavedBox(MeshEntity* Entity)
    {
        //先取出已保存的原始盒（meshBoxMap中非thickbox的项），删除阶段会清空map
        std::vector<MeshPanelData::meshBox> savedBoxes;
        for (auto it = _meshPanelData->_meshBoxMap.begin(); it != _meshPanelData->_meshBoxMap.end(); ++it)
        {
            if (!it->first.endsWith("thickbox"))
                savedBoxes.push_back(it->second);
        }
        if (savedBoxes.empty())
            return;

        //删除旧尺寸场与旧渲染（与createEncryptionByGeo的清理一致）
        FieldManager* manger = Entity->getFields();
        if (!manger) {
            manger = new FieldManager();
            Entity->AddSizeField(manger);
        }
        for (const auto& field : _meshPanelData->_Fields) {
            manger->deleteSizeField(field);
            manger->deleteBackgroundParaFieldById(field);
            manger->deleteBackgroundFieldById(field);
        }
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        symbolController->deleteSymbol(_meshPanelData->_Renderlabel);
        _meshPanelData->_Renderlabel.clear();
        _meshPanelData->_Fields.clear();
        _meshPanelData->_PreviewRenderlabel.clear();
        _meshPanelData->_meshBoxMap.clear();

        //逐个原始盒重建：登记原始盒信息后，套用当前拉伸/偏移写入尺寸场并渲染
        for (auto& saved : savedBoxes) {
            double min[3]{ 0.0 };
            double max[3]{ 0.0 };
            SBoundingBox3d bd(saved.boxMin[0], saved.boxMin[1], saved.boxMin[2],
                saved.boxMax[0], saved.boxMax[1], saved.boxMax[2]);

            int newId = manger->newId();
            SizeField* BoxField = manger->Create(newId, "Box");
            manger->addBackgroundFieldById(newId);
            manger->setBackgroundField(BoxField);
            MeshPanelData::meshBox meshbox1;
            MeshPanelData::meshBox meshbox2;
            meshbox1.boxMax[0] = bd.max().x();
            meshbox1.boxMax[1] = bd.max().y();
            meshbox1.boxMax[2] = bd.max().z();
            meshbox1.boxMin[0] = bd.min().x();
            meshbox1.boxMin[1] = bd.min().y();
            meshbox1.boxMin[2] = bd.min().z();
            meshbox2.boxMax[0] = bd.max().x() + _meshPanelData->getThickness();
            meshbox2.boxMax[1] = bd.max().y() + _meshPanelData->getThickness();
            meshbox2.boxMax[2] = bd.max().z() + _meshPanelData->getThickness();
            meshbox2.boxMin[0] = bd.min().x() - _meshPanelData->getThickness();
            meshbox2.boxMin[1] = bd.min().y() - _meshPanelData->getThickness();
            meshbox2.boxMin[2] = bd.min().z() - _meshPanelData->getThickness();

            //setValueBox内部会按拉伸修改bd，登记原始盒须在其之前完成
            setValueBox(bd, min, max, BoxField);
            createRenderBox(bd, min, max, BoxField);
            QString str = QString::number(BoxField->id);

            _meshPanelData->_Fields.insert(newId);
            _meshPanelData->_Renderlabel.push_back(str + "box");
            _meshPanelData->_Renderlabel.push_back(str + "thickbox");
            _meshPanelData->_PreviewFields.insert(newId);
            _meshPanelData->_PreviewRenderlabel.push_back(str + "box");
            _meshPanelData->_PreviewRenderlabel.push_back(str + "thickbox");
            _meshPanelData->_meshBoxMap[str + "box"] = meshbox1;
            _meshPanelData->_meshBoxMap[str + "thickbox"] = meshbox2;
        }
        symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
        QString str = "基于已保存包围盒重建加密区域成功！";
        mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::NormalMessage, str, true);
    }
    void mCustomEncryptionPanel::createEncryptionByParam(MeshEntity* Entity)
    {
        FieldManager* manager = Entity->getFields();
        if (_meshPanelData->getShape() == "箱形")
        {
            // 创建自定义Box尺寸场  在创建newid之前先判断当前所选的listitem是不是已经为背景网格
             //创建自定义尺寸场
            FieldManager* manger = Entity->getFields();
            int id = manger->newId();
            SizeField* BoxField = manager->get(id);
            if (BoxField == NULL) {
                BoxField = manager->Create(id, "Box");
            }
            manager->addBackgroundParaFieldById(id);
            //将尺寸场设置为背景尺寸场
            setValueBoxBySetting(BoxField);
            createRenderBoxBySetting(BoxField);
            QString str = MxFunctions::mNumberToString(BoxField->id);

            //保存包围盒信息
            _meshPanelData->_Fields.insert(id);
            _meshPanelData->_Renderlabel.push_back(str + "box");
            _meshPanelData->_Renderlabel.push_back(str + "thickbox");

            _meshPanelData->_PreviewFields.insert(id);
            _meshPanelData->_PreviewRenderlabel.push_back(str + "box");
            _meshPanelData->_PreviewRenderlabel.push_back(str + "thickbox");

            
        }
        else if (_meshPanelData->getShape() == "球形")
        {
            // 创建自定义Ball尺寸场 
            FieldManager* manger = Entity->getFields();
            int id = manger->newId();

            SizeField* BallField = manager->get(id);
            if (BallField == NULL) {
                BallField = manager->Create(id, "Ball");
            }
            manager->addBackgroundParaFieldById(id);
            // 将尺寸场设置为背景尺寸场
            setValueBallBySetting(BallField);
            createRenderBallBySetting(BallField);
            QString str = MxFunctions::mNumberToString(BallField->id);

            //保存包围盒信息
            _meshPanelData->_Fields.insert(id);
            _meshPanelData->_Renderlabel.push_back(str + "ball");
            _meshPanelData->_Renderlabel.push_back(str + "thickball");   
            _meshPanelData->_PreviewFields.insert(id);
            _meshPanelData->_PreviewRenderlabel.push_back(str + "ball");
            _meshPanelData->_PreviewRenderlabel.push_back(str + "thickball");
            
        }
        else if (_meshPanelData->getShape() == "圆柱形")
        {
            if (_meshPanelData->getDownCenter().split(',').size() != 3 ||
                _meshPanelData->getAxial().split(',').size() != 3)
            {
                return;
            }
            // 创建自定义Cylinder尺寸场
            FieldManager* manger = Entity->getFields();
            int id = manger->newId();
            SizeField* CylinderField = manager->get(id);
            if (CylinderField == NULL) {
                CylinderField = manager->Create(id, "Cylinder");
            }
            manager->addBackgroundParaFieldById(id);
            setValueCylinderBySetting(CylinderField);
            createRenderCylinderBySetting(CylinderField);
            QString str = MxFunctions::mNumberToString(CylinderField->id);

             //保存包围盒信息
            _meshPanelData->_PreviewFields.insert(id);
            _meshPanelData->_PreviewRenderlabel.push_back(str + "cylinder");
            _meshPanelData->_PreviewRenderlabel.push_back(str + "thickcylinder");

            _meshPanelData->_Fields.insert(id);
            _meshPanelData->_Renderlabel.push_back(str + "cylinder");
            _meshPanelData->_Renderlabel.push_back(str + "thickcylinder");
            
        }
    }
    void mCustomEncryptionPanel::createEncryptionByIrregular(MeshEntity* Entity)
    {
        //创建自定义尺寸场
        FieldManager* manger = Entity->getFields();
        vector<int> solidIds = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoSolid);
        //获取拾取到的几何体ID
        if (solidIds.empty())
        {
            QString str = "没有选中几何，体加密对象选择失败！";
            mGlobalSignals::getInstance()->outputMessageSig(1, str, true);
            return;
        }
        else {
            QString str = "体加密对象选择成功！";
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::NormalMessage, str, true);
        }
        //清除几何的拾取
        getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightGeoModel();
        for (auto tag : solidIds) {
            //创建SolidField
            int newId = manger->newId();
            SizeField* SolidField = manger->Create(newId, "Solid");
            manger->addBackgroundFieldById(newId); _meshPanelData->_Fields.insert(newId);
            //在Solid尺寸场中存储拾取到的几何体ID
            std::map<std::string, SizeFieldOption*> options = SolidField->options;
            //获取Solid的option并赋值
            auto Vin = options["VIn"];
            auto Vout = options["VOut"];
            auto Thickness = options["Thickness"];
            auto Tag = options["SolidTag"];
            Vin->numericalValue(_meshPanelData->getInsideSize());
            Vout->numericalValue(_meshPanelData->getOutsideSize());
            Thickness->numericalValue(_meshPanelData->getThickness());
            Tag->numericalValue(tag);
        }
    }


    void mCustomEncryptionPanel::createRenderBox(SBoundingBox3d& bd, double  min[3], double  max[3], SizeField* BoxField)
    {
        QString str = MxFunctions::mNumberToString(BoxField->id);
        double thick = _meshPanelData->getThickness();
        //min/max已包含拉伸和偏移，直接取中心，避免偏移重复叠加
        QVector3D pos = QVector3D((min[0] + max[0]) / 2, (min[1] + max[1]) / 2, (min[2] + max[2]) / 2);
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        auto box = symbolController->getOrCreateSymbolData<Box_Explicit>(str + "box");
        auto thickbox = symbolController->getOrCreateSymbolData<Box_Explicit>(str + "thickbox");
        QVector3D normalx = QVector3D(1, 0, 0);
        QVector3D normaly = QVector3D(0, 1, 0);
        QVector3D normalz = QVector3D(0, 0, 1);
        float lx = max[0] - min[0];
        float ly = max[1] - min[1];
        float lz = max[2] - min[2];
        box->_size = _meshPanelData->getThickness();
        box->_name = str + "box";
        box->_pos = pos;
        box->_normalx = normalx;
        box->_normaly = normaly;
        box->_normalz = normalz;
        box->_L_x = lx;
        box->_L_y = ly;
        box->_L_z = lz;
        box->_color = QVector3D(94, 203, 102);
        thickbox->_size = _meshPanelData->getThickness();
        thickbox->_name = str + "thickbox";
        thickbox->_pos = pos;
        thickbox->_normalx = normalx;
        thickbox->_normaly = normaly;
        thickbox->_normalz = normalz;
        thickbox->_L_x = lx + 2 * thick;
        thickbox->_L_y = ly + 2 * thick;
        thickbox->_L_z = lz + 2 * thick;
        thickbox->_color = QVector3D(94, 203, 102);
        symbolController->createSymbol(thickbox);
        symbolController->createSymbol(box);
        symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }
    void mCustomEncryptionPanel::createRenderBoxBySetting(SizeField* BoxField)
    {
        QString str = MxFunctions::mNumberToString(BoxField->id);
        double min[3]{ 0.0 };
        double max[3]{ 0.0 };
        QStringList v1 = _meshPanelData->getVertex1().split(',');
        QStringList v2 = _meshPanelData->getVertex2().split(',');
        if (v1.size() != 3 || v2.size() != 3)
            return;
        min[0] = v1[0].toDouble();
        min[1] = v1[1].toDouble();
        min[2] = v1[2].toDouble();
        max[0] = v2[0].toDouble();
        max[1] = v2[1].toDouble();
        max[2] = v2[2].toDouble();
        double thick = _meshPanelData->getThickness();
        QVector3D normalx = QVector3D(1, 0, 0);
        QVector3D normaly = QVector3D(0, 1, 0);
        QVector3D normalz = QVector3D(0, 0, 1);
        //与尺寸场一致：先拉伸(以中心为基准)再平移偏移
        applyStretchOffset(min, max);
        QVector3D pos = QVector3D((min[0] + max[0]) / 2, (min[1] + max[1]) / 2, (min[2] + max[2]) / 2);
        float lx = max[0] - min[0];
        float ly = max[1] - min[1];
        float lz = max[2] - min[2];
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        auto box = symbolController->getOrCreateSymbolData<Box_Explicit>(str + "box");
        box->_size = _meshPanelData->getThickness();
        box->_name = str + "box";
        box->_pos = pos;
        box->_normalx = normalx;
        box->_normaly = normaly;
        box->_normalz = normalz;
        box->_L_x = lx;
        box->_L_y = ly;
        box->_L_z = lz;
        box->_color = QVector3D(94, 203, 102);

        auto thickbox = symbolController->getOrCreateSymbolData<Box_Explicit>(str + "thickbox");
        thickbox->_size = _meshPanelData->getThickness();
        thickbox->_name = str + "thickbox";
        thickbox->_pos = pos;
        thickbox->_normalx = normalx;
        thickbox->_normaly = normaly;
        thickbox->_normalz = normalz;
        thickbox->_L_x = lx + 2 * thick;
        thickbox->_L_y = ly + 2 * thick;
        thickbox->_L_z = lz + 2 * thick;
        thickbox->_color = QVector3D(94, 203, 102);
        symbolController->createSymbol(box);
        symbolController->createSymbol(thickbox);
        symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
        //mViewportManager::getInstance()->renderBoundingBox(str + "-thickbox", min, max);  //加了过渡层之后的包围盒
    }
    void mCustomEncryptionPanel::createRenderBallBySetting(SizeField* BallField)
    {
        ;
        //渲染规则加密区域包围盒
        QString str = MxFunctions::mNumberToString(BallField->id);
        QStringList center = _meshPanelData->getCenter().split(',');
        if (center.size() != 3)
            return;
        double xcenter = center[0].toDouble();
        double ycenter = center[1].toDouble();
        double zcenter = center[2].toDouble();
        double radius = _meshPanelData->getRadius();
        double Thickness = _meshPanelData->getThickness();
        //偏移
        double offsetX = _meshPanelData->getOffset().x;
        double offsetY = _meshPanelData->getOffset().y;
        double offsetZ = _meshPanelData->getOffset().z;
        QVector3D _center(xcenter + offsetX, ycenter + offsetY, zcenter + offsetZ);
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        auto ball = symbolController->getOrCreateSymbolData<Sphere_RW>(str + "ball");
        //mViewportManager::getInstance()->renderSphere(str + "-ball", center, radius);
        ball->_name = str + "ball";
        ball->_pos = _center;
        ball->_radius = radius;
        ball->_color = QVector3D(94, 203, 102);
        radius = radius + Thickness;
        //mViewportManager::getInstance()->renderSphere(str + "-thickball", center, radius);
        auto thickball = symbolController->getOrCreateSymbolData<Sphere_RW>(str + "thickball");
        //mViewportManager::getInstance()->renderSphere(str + "-ball", center, radius);
        thickball->_name = str + "thickball";
        thickball->_pos = _center;
        thickball->_radius = radius;
        thickball->_color = QVector3D(94, 203, 102);
        symbolController->createSymbol(ball);

        symbolController->createSymbol(thickball);
        symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();

    }
    void mCustomEncryptionPanel::createRenderCylinderBySetting(SizeField* CylinderField)
    {
        //渲染规则加密区域包围盒
        QString str = MxFunctions::mNumberToString(CylinderField->id);
        QStringList downCenter = _meshPanelData->getDownCenter().split(',');
        QStringList normal = _meshPanelData->getAxial().split(',');
        if (downCenter.size() != 3 || normal.size() != 3)
            return;
        auto xcenter = downCenter[0].toDouble();
        auto ycenter = downCenter[1].toDouble();
        auto zcenter = downCenter[2].toDouble();
        auto radius = _meshPanelData->getDownRadius();
        /*      auto xaxis = options["XAxis"]->numericalValue();
        auto yaxis = options["YAxis"]->numericalValue();
        auto zaxis = options["ZAxis"]->numericalValue();*/
        auto l = _meshPanelData->getHeight();
        auto Thickness = _meshPanelData->getThickness();
        double xnormal = normal[0].toDouble();
        double ynormal = normal[1].toDouble();
        double znormal = normal[2].toDouble();
        //偏移
        double offsetX = _meshPanelData->getOffset().x;
        double offsetY = _meshPanelData->getOffset().y;
        double offsetZ = _meshPanelData->getOffset().z;
        QVector3D _normal(xnormal, ynormal, znormal);
        QVector3D _center(xcenter + offsetX, ycenter + offsetY, zcenter + offsetZ);
        //QVector3D xyz_axis(xaxis, yaxis, zaxis);
        //mViewportManager::getInstance()->renderCylinder(str + "-cylinder", center, xyz_axis, radius);
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        auto cylinder = symbolController->getOrCreateSymbolData<Cylinder_RW>(str + "cylinder");
        //mViewportManager::getInstance()->renderSphere(str + "-ball", center, radius);
        cylinder->_pos = _center;
        cylinder->_name = str + "cylinder";
        cylinder->_normal = -_normal;
        cylinder->_radius = radius;
        cylinder->_L = l;
        cylinder->_color = QVector3D(94, 203, 102);
        radius = radius + Thickness;
        auto thickcylinder = symbolController->getOrCreateSymbolData<Cylinder_RW>(str + "thickcylinder");
        //mViewportManager::getInstance()->renderSphere(str + "-ball", center, radius);
        thickcylinder->_pos = _center;
        thickcylinder->_name = str + "thickcylinder";
        thickcylinder->_normal = -_normal;
        thickcylinder->_radius = radius;
        thickcylinder->_L = l;
        thickcylinder->_color = QVector3D(94, 203, 102);
        symbolController->createSymbol(cylinder);
        symbolController->createSymbol(thickcylinder);
        symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
        //mViewportManager::getInstance()->renderCylinder(str + "-thickcylinder", center, xyz_axis, radius);

    }
    void mCustomEncryptionPanel::setValueBox(SBoundingBox3d& bd, double  min[3], double  max[3], SizeField* BoxField)
    {
        //bd.scale(_meshPanelData->getThickness()); //等比例扩大box
        bd.scale(_meshPanelData->getStretch().xup, _meshPanelData->getStretch().xdown, _meshPanelData->getStretch().yup,
            _meshPanelData->getStretch().ydown, _meshPanelData->getStretch().zup, _meshPanelData->getStretch().zdown);
        //偏移叠加到包围盒上，保证尺寸场与预览位置一致
        double offsetX = _meshPanelData->getOffset().x;
        double offsetY = _meshPanelData->getOffset().y;
        double offsetZ = _meshPanelData->getOffset().z;
        min[0] = bd.min().x() + offsetX, min[1] = bd.min().y() + offsetY, min[2] = bd.min().z() + offsetZ;
        max[0] = bd.max().x() + offsetX, max[1] = bd.max().y() + offsetY, max[2] = bd.max().z() + offsetZ;
        // Box尺寸场赋值
        std::map<std::string, SizeFieldOption*> options = BoxField->options;
        auto Vin = options["VIn"];    Vin->numericalValue(_meshPanelData->getInsideSize());
        auto Vout = options["VOut"];  Vout->numericalValue(_meshPanelData->getOutsideSize());
        auto Xmax = options["XMax"];  Xmax->numericalValue(max[0]);
        auto Ymax = options["YMax"];  Ymax->numericalValue(max[1]);
        auto Zmax = options["ZMax"];  Zmax->numericalValue(max[2]);
        auto Xmin = options["XMin"];  Xmin->numericalValue(min[0]);
        auto Ymin = options["YMin"];  Ymin->numericalValue(min[1]);
        auto Zmin = options["ZMin"];  Zmin->numericalValue(min[2]);
        auto Thickness = options["Thickness"]; Thickness->numericalValue(_meshPanelData->getThickness());
    }
    //对箱形包围盒施加拉伸(以中心为基准，上下边分别缩放)与偏移
    void mCustomEncryptionPanel::applyStretchOffset(double min[3], double max[3])
    {
        Stretch s = _meshPanelData->getStretch();
        Offset o = _meshPanelData->getOffset();
        double cx = (min[0] + max[0]) / 2;
        double cy = (min[1] + max[1]) / 2;
        double cz = (min[2] + max[2]) / 2;
        double nmin[3], nmax[3];
        nmax[0] = cx + (max[0] - cx) * s.xup;
        nmax[1] = cy + (max[1] - cy) * s.yup;
        nmax[2] = cz + (max[2] - cz) * s.zup;
        nmin[0] = cx + (min[0] - cx) * s.xdown;
        nmin[1] = cy + (min[1] - cy) * s.ydown;
        nmin[2] = cz + (min[2] - cz) * s.zdown;
        min[0] = nmin[0] + o.x; min[1] = nmin[1] + o.y; min[2] = nmin[2] + o.z;
        max[0] = nmax[0] + o.x; max[1] = nmax[1] + o.y; max[2] = nmax[2] + o.z;
    }
    void mCustomEncryptionPanel::setValueBoxBySetting(SizeField* BoxField)
    {
        double min[3]{ 0.0 };
        double max[3]{ 0.0 };
        QStringList v1 = _meshPanelData->getVertex1().split(',');
        QStringList v2 = _meshPanelData->getVertex2().split(',');
        if (v1.size() != 3 || v2.size() != 3)
            return;
        min[0] = v1[0].toDouble();
        min[1] = v1[1].toDouble();
        min[2] = v1[2].toDouble();
        max[0] = v2[0].toDouble();
        max[1] = v2[1].toDouble();
        max[2] = v2[2].toDouble();
        //拉伸(以中心为基准)与偏移叠加到写入尺寸场的坐标上，保证加密区域与预览位置一致
        applyStretchOffset(min, max);
        std::map<std::string, SizeFieldOption*> options = BoxField->options;
        auto Vin = options["VIn"]; Vin->numericalValue(_meshPanelData->getInsideSize());
        auto Vout = options["VOut"]; Vout->numericalValue(_meshPanelData->getOutsideSize());
        auto Xmax = options["XMax"]; Xmax->numericalValue(std::max(min[0], max[0]));
        auto Ymax = options["YMax"]; Ymax->numericalValue(std::max(min[1], max[1]));
        auto Zmax = options["ZMax"]; Zmax->numericalValue(std::max(min[2], max[2]));
        auto Xmin = options["XMin"]; Xmin->numericalValue(std::min(min[0], max[0]));
        auto Ymin = options["YMin"]; Ymin->numericalValue(std::min(min[1], max[1]));
        auto Zmin = options["ZMin"]; Zmin->numericalValue(std::min(min[2], max[2]));
        auto Thickness = options["Thickness"]; Thickness->numericalValue(_meshPanelData->getThickness());
    }
    void mCustomEncryptionPanel::setValueBallBySetting(SizeField* BallField)
    {
        std::map<std::string, SizeFieldOption*> options = BallField->options;
        QStringList center = _meshPanelData->getCenter().split(',');
        if (center.size() != 3)
            return;
        double xcenter = center[0].toDouble();
        double ycenter = center[1].toDouble();
        double zcenter = center[2].toDouble();
        //偏移叠加到球心上，保证尺寸场与预览位置一致
        xcenter += _meshPanelData->getOffset().x;
        ycenter += _meshPanelData->getOffset().y;
        zcenter += _meshPanelData->getOffset().z;
        // 获取Ball的option
        auto Vin = options["VIn"]; Vin->numericalValue(_meshPanelData->getInsideSize());
        auto Vout = options["VOut"]; Vout->numericalValue(_meshPanelData->getOutsideSize());
        auto Xcenter = options["XCenter"]; Xcenter->numericalValue(xcenter);
        auto Ycenter = options["YCenter"]; Ycenter->numericalValue(ycenter);
        auto Zcenter = options["ZCenter"]; Zcenter->numericalValue(zcenter);
        auto Radius = options["Radius"]; Radius->numericalValue(_meshPanelData->getRadius());
        auto Thickness = options["Thickness"]; Thickness->numericalValue(_meshPanelData->getThickness());
    }
    void mCustomEncryptionPanel::setValueCylinderBySetting(SizeField* CylinderField)
    {
        std::map<std::string, SizeFieldOption*> options = CylinderField->options;
        QStringList downCenter = _meshPanelData->getDownCenter().split(',');//底面圆心        
        QStringList normal = _meshPanelData->getAxial().split(',');
        if (downCenter.size() != 3 || normal.size() != 3)
            return;
        double height = _meshPanelData->getHeight();
        auto xcenter = downCenter[0].toDouble();
        auto ycenter = downCenter[1].toDouble();
        auto zcenter = downCenter[2].toDouble();
        //偏移叠加到底面圆心上，保证尺寸场与预览位置一致
        QVector3D offset(_meshPanelData->getOffset().x, _meshPanelData->getOffset().y, _meshPanelData->getOffset().z);
        QVector3D dCtr(downCenter[0].toDouble(), downCenter[1].toDouble(), downCenter[2].toDouble());
        dCtr += offset;
        QVector3D axis(normal[0].toDouble(), normal[1].toDouble(), normal[2].toDouble());
        QVector3D unitAxis = axis.normalized();
        QVector3D center = dCtr + ((height / 2) * unitAxis);//中心圆坐标
        auto Vin = options["VIn"]; Vin->numericalValue(_meshPanelData->getInsideSize());
        auto Vout = options["VOut"]; Vout->numericalValue(_meshPanelData->getOutsideSize());
        auto Xcenter = options["XCenter"]; Xcenter->numericalValue(center.x());
        auto Ycenter = options["YCenter"]; Ycenter->numericalValue(center.y());
        auto Zcenter = options["ZCenter"]; Zcenter->numericalValue(center.z());
        auto Xaxis = options["XAxis"]; Xaxis->numericalValue(unitAxis.x() * (height / 2));
        auto Yaxis = options["YAxis"]; Yaxis->numericalValue(unitAxis.y() * (height / 2));
        auto Zaxis = options["ZAxis"]; Zaxis->numericalValue(unitAxis.z() * (height / 2));
        auto Thickness = options["Thickness"]; Thickness->numericalValue(_meshPanelData->getThickness());
        auto Radius = options["Radius"]; Radius->numericalValue(_meshPanelData->getDownRadius());
    }

    void mCustomEncryptionPanel::slot_boxDelete()
    {
        cancelPreview();
        MechModelData::mModelData* modelData = getCVM->getModelData();
        mMeshData* meshdata = modelData->getMeshData();
        MeshEntity* Entity = meshdata->getMeshEntity();
        //删除自定义尺寸场
        FieldManager* manger = Entity->getFields();
        if (!manger) {
            manger = new FieldManager();
            Entity->AddSizeField(manger);
        }
        if (manger->size())
        {
            for (const auto& field : _meshPanelData->_Fields) {
                manger->deleteSizeField(field);
                manger->deleteBackgroundParaFieldById(field);
                manger->deleteBackgroundFieldById(field);
            }
            //manger->resetSizeField();
            QString str = "实体加密对象删除成功！";
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::NormalMessage, str, true);
        }

        if (!getCVM->getPreRenderWidget())
            return;
        //删除渲染包围盒
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        symbolController->deleteSymbol(_meshPanelData->_Renderlabel);
        _meshPanelData->_Renderlabel.clear();
        _meshPanelData->_Fields.clear();
        _meshPanelData->_PreviewGeos.clear();

        symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
        getCVM->getPreRenderWidget()->getHighlightController()->clearTempHighlight();
        getCVM->getPreRenderWidget()->getHighlightController()->clearTempMeshHighlight();
    }
    void mCustomEncryptionPanel::slot_boxShow()
    {
        vector<QString> showSymbol;
        auto meshDatas = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getAllMeshPanelDataByType(CustomEncryption);
        for (auto meshData : meshDatas)
        {
            showSymbol.insert(showSymbol.end(), meshData->_Renderlabel.begin(), meshData->_Renderlabel.end());      
        }
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        symbolController->setSymbolShowState(showSymbol, true);     
        symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }
    void mCustomEncryptionPanel::slot_boxHide()
    {
        cancelPreview();
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        vector<QString> showSymbol;
        auto meshDatas = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getAllMeshPanelDataByType(CustomEncryption);
        for (auto meshData : meshDatas)
        {
            showSymbol.insert(showSymbol.end(), meshData->_Renderlabel.begin(), meshData->_Renderlabel.end());
        }
        symbolController->setSymbolShowState(showSymbol, false);
        symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }
    void mCustomEncryptionPanel::slot_boxPreview()
    {
        //删除预览的包围盒
        cancelPreview();
        //覆盖之前保存的包围盒
        MechModelData::mModelData* modelData = getCVM->getModelData();
        mMeshData* meshdata = modelData->getMeshData();
        MeshEntity* Entity = meshdata->getMeshEntity();
        //删除自定义尺寸场
        FieldManager* manger = Entity->getFields();
        if (!manger) {
            manger = new FieldManager();
            Entity->AddSizeField(manger);
        }
        if (manger->size())
        {
            for (const auto& field : _meshPanelData->_Fields) {
                manger->deleteSizeField(field);
                manger->deleteBackgroundParaFieldById(field);
                manger->deleteBackgroundFieldById(field);
            }
            //manger->resetSizeField();
            QString str = "实体加密对象删除成功！";
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::NormalMessage, str, true);
        }

        if (!getCVM->getPreRenderWidget())
            return;
        //删除渲染包围盒
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        symbolController->deleteSymbol(_meshPanelData->_Renderlabel);
        _meshPanelData->_Renderlabel.clear();
        _meshPanelData->_Fields.clear();

        symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
        getCVM->getPreRenderWidget()->getHighlightController()->clearTempHighlight();
        getCVM->getPreRenderWidget()->getHighlightController()->clearTempMeshHighlight();

        //保存当前包围盒
        if (_meshPanelData->getMethod() == "基于几何") {
            //首次保存后几何拾取已被清除（createEncryptionByGeo尾部ClearAll），
            //再次保存时若拾取为空但存在已保存的原始包围盒，直接基于它重建（套用当前偏移/拉伸），
            //不再要求重新拾取，也避免旧场旧盒先被删掉后重建失败导致包围盒消失
            auto geoSelect = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData();
            bool hasPick = !geoSelect->getSelectDataByType(SelectionInfoType::GeoSolid).empty()
                || !geoSelect->getSelectDataByType(SelectionInfoType::GeoFace).empty()
                || !geoSelect->getSelectDataByType(SelectionInfoType::GeoEdge).empty();
            if (!hasPick && !_meshPanelData->_meshBoxMap.empty())
            {
                recreateBySavedBox(Entity);
                return;
            }
            createEncryptionByGeo(Entity);
        }
        else if (_meshPanelData->getMethod() == "输入参数")
        {
            createEncryptionByParam(Entity);
        }
        else if (_meshPanelData->getMethod() == "基于几何体")
        {
            createEncryptionByIrregular(Entity);
        }
    }
    void mCustomEncryptionPanel::cancelPreview()
    {
        //删除自定义尺寸场
        _meshPanelData->_PreviewFields.clear();
        if (!getCVM->getPreRenderWidget())
            return;
        _meshPanelData->_PreviewRenderlabel.clear();
    }
    void mCustomEncryptionPanel::offsetChanged()
    {
        if (xEdit->text().isEmpty() || yEdit->text().isEmpty() || zEdit->text().isEmpty()) return;
        Offset offset = { xEdit->text().toDouble(),yEdit->text().toDouble(),zEdit->text().toDouble() };
        _meshPanelData->setOffset(offset);
        //slot_boxDelete(currentId);
        boxChange();
    }
    void mCustomEncryptionPanel::stretchChanged()
    {
        if (xupEdit->text().isEmpty() || xdownEdit->text().isEmpty() || yupEdit->text().isEmpty()
            || ydownEdit->text().isEmpty() || zupEdit->text().isEmpty() || zdownEdit->text().isEmpty()) return;
        Stretch stretch = { xupEdit->text().toDouble(),xdownEdit->text().toDouble(),yupEdit->text().toDouble(),ydownEdit->text().toDouble(),
        zupEdit->text().toDouble() ,zdownEdit->text().toDouble() };
        _meshPanelData->setStretch(stretch);
        //slot_boxDelete(currentId);
        boxChange();
    }

    void mCustomEncryptionPanel::boxChange()
    {
        //保存预览
        MechModelData::mModelData* modelData = getCVM->getModelData();
        mMeshData* meshdata = modelData->getMeshData();
        MeshEntity* Entity = meshdata->getMeshEntity();
        FieldManager* manger = Entity->getFields();
        if (!manger) {
            manger = new FieldManager();
            Entity->AddSizeField(manger);
        }       
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (_meshPanelData->_PreviewRenderlabel.empty())
            return;
        if (_meshPanelData->getMethod() == "基于几何") {   
            QVector<SBoundingBox3d>BoundingBoxVec;
            auto pickDilter = getCVM->getPreRenderWidget()->getPickFilter();       
            //非连续创建即为多个部件创建一个BoxField
            for (auto name : _meshPanelData->_PreviewRenderlabel)
            {
                QString number;
                QString type;
                extractNumberAndText(name, number, type);
                if (type == "box"|| type=="thickbox")
                {
                    auto box = symbolController->getOrCreateSymbolData<Box_Explicit>(name);
                    auto boxMap = _meshPanelData->_meshBoxMap[name];
                    double offsetX = _meshPanelData->getOffset().x;
                    double offsetY = _meshPanelData->getOffset().y;
                    double offsetZ = _meshPanelData->getOffset().z;                            
                    boxMap.boxMax[0] = (boxMap.boxMax[0] - (boxMap.boxMin[0] + boxMap.boxMax[0]) * .5) * _meshPanelData->getStretch().xup + (boxMap.boxMin[0] + boxMap.boxMax[0]) * .5;
                    boxMap.boxMax[1] = (boxMap.boxMax[1] - (boxMap.boxMin[1] + boxMap.boxMax[1]) * .5) * _meshPanelData->getStretch().yup + (boxMap.boxMin[1] + boxMap.boxMax[1]) * .5;
                    boxMap.boxMax[2] = (boxMap.boxMax[2] - (boxMap.boxMin[2] + boxMap.boxMax[2]) * .5) * _meshPanelData->getStretch().zup + (boxMap.boxMin[2] + boxMap.boxMax[2]) * .5;
                    boxMap.boxMin[0] = (boxMap.boxMin[0] - (boxMap.boxMin[0] + boxMap.boxMax[0]) * .5) * _meshPanelData->getStretch().xdown + (boxMap.boxMin[0] + boxMap.boxMax[0]) * .5;
                    boxMap.boxMin[1] = (boxMap.boxMin[1] - (boxMap.boxMin[1] + boxMap.boxMax[1]) * .5) * _meshPanelData->getStretch().ydown + (boxMap.boxMin[1] + boxMap.boxMax[1]) * .5;
                    boxMap.boxMin[2] = (boxMap.boxMin[2] - (boxMap.boxMin[2] + boxMap.boxMax[2]) * .5) * _meshPanelData->getStretch().zdown + (boxMap.boxMin[2] + boxMap.boxMax[2]) * .5;
                    QVector3D pos = QVector3D((boxMap.boxMin[0] + boxMap.boxMax[0]) / 2 + offsetX, (boxMap.boxMin[1] + boxMap.boxMax[1]) / 2 + offsetY, (boxMap.boxMin[2] + boxMap.boxMax[2]) / 2 + offsetZ);
                    float lx = boxMap.boxMax[0] - boxMap.boxMin[0];
                    float ly = boxMap.boxMax[1] - boxMap.boxMin[1];
                    float lz = boxMap.boxMax[2] - boxMap.boxMin[2];
                    box->_pos = pos;
                    box->_L_x = lx;
                    box->_L_y = ly;
                    box->_L_z = lz;
                    symbolController->UpdateAllSymbol();
                    getCVM->getPreRenderWidget()->updatePaint();
                }            
                SBoundingBox3d bd;
            }                
        }
        else if (_meshPanelData->getMethod() == "输入参数")
        {
            boxChangeByParam(Entity);
        }
        else if (_meshPanelData->getMethod() == "基于几何体")
        {
            createEncryptionByIrregular(Entity);
        }
    }

}
