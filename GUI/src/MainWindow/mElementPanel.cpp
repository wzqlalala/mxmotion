#include "mElementPanel.h"
#include "mAppConfiguration.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mInfoWindow.h"
#include "mCalcLineEdit.h"
#include "mElemeneParmTableWidget.h"

#include "mPythonAgent.h"
#include "mPyScriptWriter.h"
#include "mPyScriptReader.h"
#include "PickCommonFunction.h"

//通用拾取控件
#include "mGeneralPickUp.h"
#include "mPickWidgetController.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/view/CCSMPreRender.h"

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
#include <QCompleter>
#include <QTimer>
#include <QFormLayout>
#include <QRadioButton>
#include <QSet>
#include <QDateTime>

//BasicData
#include "mPropertyBase.h"
#include "mPropertyManager.h"
#include "mElementBase.h"
#include "mDataElement.h"
#include "mModelData.h"
#include "mMeshData.h"
#include "mSetBase.h"
#include "mElementSet.h"
#include "mOtherFunction.h"


using namespace MechPreData;
//using namespace MxBeam;
using namespace MechPropertyData;
using namespace MechElementData;
using namespace MxFunctions;
using namespace mxr;

namespace MechMainWindow
{
    static const QSet<QString> basicTypes = {
    "杆", "管", "三维实体", "三维实体壳", "平面应变",
    "平面应力", "剪切板", "低阶高精度", "轴对称",
    "绘图显示", "垫片", "膜", "间隙"
    };

    mElementPanel::mElementPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _currentViewportPrivate = nullptr;

        _elementParamTableWidget = new mElemeneParmTableWidget(this);
        _elementParamTableWidget->hide();

        _mainLayout = new QVBoxLayout(this);

        // ==========类型折叠块(0) ==========
        FoldSection* typeFold = new FoldSection("类型", this);
        // 部件选择
        _comboEleVariety = new QComboBox();
        _comboEleType = new QComboBox();
        _labelEleType = new QLabel();
        //_buttonParam = new QPushButton("修改单元数据");

        connect(_comboEleVariety, QOverload<const QString&>::of(&QComboBox::currentIndexChanged), this, &mElementPanel::slot_varietyComboBoxChanged);
        connect(_comboEleType, QOverload<const QString&>::of(&QComboBox::currentIndexChanged), this, &mElementPanel::slot_typeComboBoxChanged);
        //connect(_buttonParam, &QPushButton::clicked, this, &mElementPanel::slot_paramBtn);

        typeFold->addRow(new PanelLabel("单元种类"), _comboEleVariety);
        typeFold->addRow(new PanelLabel("单元类型"), _comboEleType);
        typeFold->addRow(new PanelLabel("描述"), _labelEleType);
        m_foldWidgets.append(typeFold);

        // ========== 赋予折叠块(1) ==========
        FoldSection* assignFold = new FoldSection("拾取单元列表", this);

        // 通用拾取控件（无集合选择、无临时高亮，仅拾取高亮），仅允许拾取单元
        _pickController = mGeneralPickUp::createSetPickWidget(CBBlue, assignFold);
        _pickController->setDefultTip("拾取单元");
        _pickController->setAllowedTypes([]() -> QSet<MxFunctions::PickDataType>
            {
                return { MxFunctions::PickDataType::Element };
            });

        //设置为此类型按钮
        QPushButton* buttonSet = new QPushButton("设置为此类型");

        assignFold->addRow(_pickController->widget());
        assignFold->addRow(buttonSet);
        m_foldWidgets.append(assignFold);

        connect(buttonSet, &QPushButton::clicked, this, &mElementPanel::slot_setTypeBtn);


        // ==========三维实体形状折叠块(2) ==========
        FoldSection* solidFold = new FoldSection("形状", this);

        _buttonGroup = new QButtonGroup();
        connect(_buttonGroup, QOverload<QAbstractButton*, bool>::of(&QButtonGroup::buttonToggled), this, &mElementPanel::slot_shapeChanged);

        m_foldWidgets.append(solidFold);

        for (int i = 0; i < m_foldWidgets.size(); i++)
        {
            _mainLayout->removeWidget(m_foldWidgets[i]); m_foldWidgets[i]->hide();
        }

        _mainLayout->addStretch();
	}

	mElementPanel::~mElementPanel()
	{
	}

    void mElementPanel::editElementPanel(QString type)
    {
        setEditing(true);
        connectSig();

        // 激活通用拾取控件（内部开启单元拾取模式），并清空上次编辑残留的拾取数据
        if (getCVM && getCVM->getPreRenderWidget())
        {
            _pickController->setPickActive(true);
        }
        _pickController->replacePickedDataAndClearSets({});

        for (int i = 0; i < m_foldWidgets.size(); i++)
        {
            _mainLayout->removeWidget(m_foldWidgets[i]); m_foldWidgets[i]->hide();
        }

        _variety = type;
        _buttonGroup->blockSignals(true);
        _comboEleVariety->blockSignals(true);
        _comboEleVariety->clear();

        bool isIn = false;
        auto fold = m_foldWidgets[2];
        while (fold->layout()->rowCount() > 0)
        {
            fold->removeRow(0);
        }

        if (_variety == "点单元")
        {
            _comboEleVariety->addItems({ "质量" });
        }
        else if (_variety == "线单元")
        {
            _comboEleVariety->addItems({ "杆", "梁", "管", "弹簧", "阻尼", "六向弹簧阻尼","间隙"});
        }
        else if (_variety == "面单元")
        {
            isIn = true;
            auto radio1 = new QRadioButton("三角形"); _buttonGroup->addButton(radio1); radio1->setChecked(true); radio1->setFixedWidth(90);
            auto radio2 = new QRadioButton("四边形"); _buttonGroup->addButton(radio2);

            fold->addRow(radio1, radio2);

            //_comboEleVariety->addItems({ "平面应力", "平面应变", "壳" ,"膜" , "轴对称", "剪切板" });
        }
        else if (_variety == "体单元")
        {
            isIn = true;
            auto radio1 = new QRadioButton("四面体"); _buttonGroup->addButton(radio1); radio1->setChecked(true); radio1->setFixedWidth(90);
            auto radio2 = new QRadioButton("六面体"); _buttonGroup->addButton(radio2);
            auto radio3 = new QRadioButton("三棱柱"); _buttonGroup->addButton(radio3); radio3->setFixedWidth(90);
            auto radio4 = new QRadioButton("四棱锥"); _buttonGroup->addButton(radio4);

            fold->addRow(radio1, radio2);
            fold->addRow(radio3, radio4);

            //_comboEleVariety->addItems({ "三维实体", "三维实体壳", "垫片单元" });
        }
        else if (_variety == "特殊单元")
        {
            _comboEleVariety->addItems({ "绘图显示" });
        }
        else if (_variety == "低阶高精度")
        {
            _comboEleVariety->addItems({ "低阶高精度" });
        }
        if (isIn)
        {
            for (auto button : _buttonGroup->buttons())
            {
                if (button->text() == _shape)
                {
                    button->setChecked(true);
                    break;
                }
            }

            _mainLayout->insertWidget(_mainLayout->count() - 1, fold); fold->show();
        }

        _buttonGroup->blockSignals(false);

        _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[0]); m_foldWidgets[0]->show();
        _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[1]); m_foldWidgets[1]->show();

        _mainLayout->invalidate();

        _comboEleVariety->blockSignals(false);
        //slot_varietyComboBoxChanged(_comboEleVariety->currentText());
        if (_buttonGroup->checkedButton())
        {
            slot_shapeChanged(_buttonGroup->checkedButton(), true);
        }
        else
        {
            slot_varietyComboBoxChanged(_comboEleVariety->currentText());
        }
    }
    
    void mElementPanel::uneditElementPanel()
    {
        if (!isEditing())
        {
            return;
        }
        //入口统一复位编辑态：后续各早退分支无需再单独清标志
        setEditing(false);
        //失活拾取控件：清理拾取数据与高亮，并恢复拾取工具栏
        deactivatePickWidget();

        disconnectSig();

        _elementParamTableWidget->unedit();
    }

    void mElementPanel::slot_varietyComboBoxChanged(const QString& text)
    {
        _type = text;
        if (getCVM == nullptr)
        {
            return;
        }
        _comboEleType->blockSignals(true);
        _comboEleType->clear();
        if (_type == "质量")
        {
            // 参数1是显示文本，参数2是隐藏的UserData
            _comboEleType->addItem("集中质量", "MASS1");
            _comboEleType->addItem("耦合质量", "CMASS2");
            _shape = "点";
        }
        else if (_type == "弹簧")
        {
            _comboEleType->addItem("ELAS1型弹簧", "ELAS1");
            _comboEleType->addItem("接地六自由度弹簧", "Spring1");
            _comboEleType->addItem("六自由度弹簧", "Spring2");
            _comboEleType->addItem("三自由度弹簧", "SpringA");
            _shape = "线";
        }
        else if (_type == "阻尼")
        {
            _comboEleType->addItem("单自由度阻尼", "ViscousDamp");
            _comboEleType->addItem("接地六自由度阻尼", "DASHPOT1");
            _comboEleType->addItem("六自由度阻尼", "DASHPOT2");
            _comboEleType->addItem("三自由度阻尼", "DASHPOTA");
            _shape = "线";
        }
        else if (_type == "六向弹簧阻尼")
        {
            _comboEleType->addItem("六向弹簧阻尼_O", "CBUSH");
            _comboEleType->addItem("六向弹簧阻尼_N", "CBUSH1");
            _shape = "线";
        }
        else if (_type == "杆")
        {
            _comboEleType->addItem("标准线性", "D3R2");
            _shape = "线";
        }
        else if (_type == "梁")
        {
            _comboEleType->addItem("标准线性", "D3B2");
            _comboEleType->addItem("线性 (31型)", "D3B31");
            _shape = "线";
        }
        else if (_type == "管")
        {
            _comboEleType->addItem("标准线性", "D3T2");
            _shape = "线";
        }
        else if (_type == "间隙")
        {
            _comboEleType->addItem("间隙单元", "GAPUNI");
            _shape = "线";
        }
        else if (_type == "三维实体")
        {
            if (_shape == "四面体")
            {
                _comboEleType->addItem("标准线性", "D3T4");
                _comboEleType->addItem("标准线性(增量格式)", "D3T4_N");
                _comboEleType->addItem("线性不可压缩(增量格式)", "D3T4H_N");
                _comboEleType->addItem("标准二阶", "D3T10");
                _comboEleType->addItem("二阶线性(增量格式)", "D3T10_N");
                _comboEleType->addItem("二阶不可压缩(增量格式)", "D3T10H_N");
                _comboEleType->addItem("二阶修正(增量格式)", "D3T10M_N");
                _comboEleType->addItem("二阶修正(增量格式v2)", "D3T10M_N2");
            }
            else if (_shape == "六面体")
            {
                //_comboEleType->addItem("标准线性", "D3H8");
                _comboEleType->addItem("BAR3列式", "D3H8B");
                _comboEleType->addItem("线性不可压缩", "D3H8H");
                _comboEleType->addItem("线性缩减积分", "D3H8R");
                _comboEleType->addItem("线性缩减积分(增量格式)", "D3H8R_N");
                _comboEleType->addItem("线性非协调", "D3H8I");
                _comboEleType->addItem("线性非协调(增量格式)", "D3H8I_N");
                _comboEleType->addItem("相场断裂单元", "PFCZMD3H8");
                _comboEleType->addItem("标准二阶", "D3H20");
                _comboEleType->addItem("标准二阶(增量格式)", "D3H20_N");
                _comboEleType->addItem("二阶不可压缩(增量格式)", "D3H20H_N");
                _comboEleType->addItem("二阶缩减积分", "D3H20R");
                _comboEleType->addItem("二阶缩减积分(增量格式)", "D3H20R_N");
            }
            else if (_shape == "三棱柱")
            {
                _comboEleType->addItem("标准线性", "D3P6");
                _comboEleType->addItem("标准线性(增量格式)", "D3P6_N");
                _comboEleType->addItem("线性不可压缩(增量格式)", "D3P6H_N");
                _comboEleType->addItem("标准二阶(增量格式)", "D3P15_N");
                _comboEleType->addItem("二阶不可压缩(增量格式)", "D3P15H_N");
            }
            else if (_shape == "四棱锥")
            {
                _comboEleType->addItem("标准线性", "D3P5");
                _comboEleType->addItem("标准二阶", "D3P13");
            }
        }
        else if (_type == "三维实体壳")
        {
            if (_shape == "三棱柱")
            {
                _comboEleType->addItem("缩减积分", "SP6R");
            }
            else if (_shape == "六面体")
            {
                _comboEleType->addItem("缩减积分", "SH8R");
            }
        }
        else if (_type == "垫片")
        {
            if (_shape == "三棱柱")
            {
                _comboEleType->addItem("全自由度垫片", "D3Gk6");
                _comboEleType->addItem("纯法向垫片", "D3Gk6N");
            }
            else if (_shape == "六面体")
            {
                _comboEleType->addItem("全自由度垫片", "D3Gk8");
                _comboEleType->addItem("纯法向垫片", "D3Gk8N");
            }
        }
        else if (_type == "平面应变")
        {
            if (_shape == "三角形")
            {
                _comboEleType->addItem("标准线性", "PST3");
                _comboEleType->addItem("标准二阶", "PST6");
            }
            else if (_shape == "四边形")
            {
                _comboEleType->addItem("标准线性", "PSQ4");
                _comboEleType->addItem("标准二阶", "PSQ8");
            }
        }
        else if (_type == "平面应力")
        {
            if (_shape == "三角形")
            {
                _comboEleType->addItem("标准线性", "PET3");
                _comboEleType->addItem("标准二阶", "PET6");
            }
            else if (_shape == "四边形")
            {
                _comboEleType->addItem("标准线性", "PEQ4");
                _comboEleType->addItem("线性非协调", "PEQ4I");
                _comboEleType->addItem("线性缩减积分", "PEQ4R");
                _comboEleType->addItem("标准二阶", "PEQ8");
                _comboEleType->addItem("二阶缩减积分", "PEQ8R");
            }
        }
        else if (_type == "壳")
        {
            if (_shape == "三角形")
            {
                _comboEleType->addItem("标准线性", "T3");
                _comboEleType->addItem("MITC列式", "MITC3");
                _comboEleType->addItem("MITC3D列式", "MITC3D");
                _comboEleType->addItem("DSG列式", "DSG");
                _comboEleType->addItem("CPDSG列式", "CPDSG");
            }
            else if (_shape == "四边形")
            {
                _comboEleType->addItem("标准线性", "Q4");
                _comboEleType->addItem("MITC列式", "MITC4");
                _comboEleType->addItem("MITC4D列式", "MITC4D");
                _comboEleType->addItem("GE4列式", "GE4");
            }
        }
        else if (_type == "膜")
        {
            if (_shape == "三角形")
            {
                _comboEleType->addItem("标准线性", "M3D3");
            }
            else if (_shape == "四边形")
            {
                _comboEleType->addItem("标准线性", "M3D4");
                _comboEleType->addItem("线性缩减积分", "M3D4R");
            }
        }
        else if (_type == "轴对称")
        {
            if (_shape == "三角形")
            {
                _comboEleType->addItem("标准线性", "CAXT3");
                _comboEleType->addItem("标准二阶", "CAXT6");
            }
            else if (_shape == "四边形")
            {
                _comboEleType->addItem("标准线性", "CAXQ4");
                _comboEleType->addItem("线性非协调", "CAXQ4I");
                _comboEleType->addItem("标准二阶", "CAXQ8");
                _comboEleType->addItem("二阶缩减积分", "CAXQ8R");
            }
        }
        else if (_type == "剪切板")
        {
            if (_shape == "四边形")
            {
                _comboEleType->addItem("标准线性", "SPQ4");
            }
        }
        else if (_type == "绘图显示")
        {
            _comboEleType->addItem("绘图线单元", "PLOTEL");
            _comboEleType->addItem("绘图三角形", "PLOTEL3");
            _comboEleType->addItem("绘图四边形", "PLOTEL4");
        }
        else if (_type == "低阶高精度")
        {
            _comboEleType->addItem("边光滑平面应变", "PST3-3");
            _comboEleType->addItem("边光滑平面应力", "PET3-3");
            _comboEleType->addItem("边光滑壳", "T3-3");
            _comboEleType->addItem("边光滑四面体", "D3T4-3");
            _comboEleType->addItem("梯度加权四面体", "D3T4-4");
        }
        _comboEleType->blockSignals(false);
        slot_typeComboBoxChanged(_comboEleType->currentText());
        

        //if (_type == "梁" || _type == "弹簧" || _type == "弹簧阻尼" || _type == "弹簧" || _type == "壳" || _type == "质量")
        //{
        //    _buttonParam->setVisible(true);
        //}
        //else
        //{
        //    _buttonParam->setVisible(false);
        //}
    }

    void mElementPanel::slot_typeComboBoxChanged(const QString& text)
    {
        if (getCVM == nullptr)
        {
            return;
        }

        // 找到当前文本对应的索引
        int index = _comboEleType->findText(text);
        if (index != -1)
        {
            // 提取之前绑定的 UserData，转换为 QString 并设置给 Label
            QString labelText = _comboEleType->itemData(index).toString();
            _labelEleType->setText(labelText);
        }
        if (text == "绘图线单元")
        {
            _shape = "线";
        }
        else if (text == "绘图三角形" || text == "边光滑平面应变"|| text == "边光滑平面应力" || text == "边光滑壳")
        {
            _shape = "三角形";
        }
        else if (text == "绘图四边形")
        {
            _shape = "四边形";
        }
        else if (text == "边光滑四面体" || text == "梯度加权四面体")
        {
            _shape = "四面体";
        }
        //类型变化后旧拾取数据失效：清空拾取数据与高亮
        _pickController->replacePickedDataAndClearSets({});
        //临时高亮当前单元类型的所有网格
        tempHighlightEle();
    }

    void mElementPanel::slot_setTypeBtn()
    {
        if (!getCVM || !getCVM->getPreRenderWidget())
        {
            return;
        }

        //从通用拾取控件获取拾取的单元
        auto eles = collectPickedElements();
        if (eles.empty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, QString("请先拾取单元！"));
            return;
        }

        auto checked = checkElement(eles);
        assignElement(checked);

        //清空拾取数据与高亮，便于继续拾取
        _pickController->replacePickedDataAndClearSets({});

        //临时高亮当前单元类型的所有网格
        tempHighlightEle();
    }

    void mElementPanel::slot_subWindowActivated(int viewportID)
    {
        Q_UNUSED(viewportID);
        if (_currentViewportPrivate == VMManager->getCurrentViewportPrivate())
        {
            return;
        }
        _currentViewportPrivate = VMManager->getCurrentViewportPrivate();

        //视口切换后旧视口的拾取数据失效：清空拾取数据，激活态下重新绑定新视口的拾取信号
        if (_pickController->isPickActive())
        {
            _pickController->setPickActive(false);
            _pickController->replacePickedDataAndClearSets({});
            _pickController->setPickActive(true);
        }
        else
        {
            _pickController->replacePickedDataAndClearSets({});
        }

        //在新视口重新临时高亮当前单元类型的所有网格
        tempHighlightEle();
    }
    void mElementPanel::slot_shapeChanged(QAbstractButton* button, bool isChecked)
    {
        if (button == nullptr)
        {
            return;
        }
        if (!isChecked)
        {
            return;
        }
        QString shapeText = button->text();
        _shape = shapeText;
        QString currentVariety = _comboEleVariety->currentText();
        _comboEleVariety->blockSignals(true);
        _comboEleVariety->clear();
        if (shapeText == "四面体")
        {
            _comboEleVariety->addItems({ "三维实体"});
        }
        else if (shapeText == "六面体")
        {
            _comboEleVariety->addItems({ "三维实体", "三维实体壳", "垫片" });
        }
        else if (shapeText == "三棱柱")
        {
            _comboEleVariety->addItems({ "三维实体", "三维实体壳", "垫片" });
        }
        else if (shapeText == "四棱锥")
        {
            _comboEleVariety->addItems({ "三维实体"});
        }
        else if (shapeText == "三角形")
        {
            _comboEleVariety->addItems({ "平面应力", "平面应变", "壳" ,"膜" /*, "轴对称"*/ });
        }
        else if (shapeText == "四边形")
        {
            _comboEleVariety->addItems({ "平面应力", "平面应变", "壳" ,"膜" , /*"轴对称",*/ "剪切板" });
        }
        _comboEleVariety->setCurrentText(currentVariety);
        _comboEleVariety->blockSignals(false);
        slot_varietyComboBoxChanged(_comboEleVariety->currentText());
    }
    void mElementPanel::slot_paramBtn()
    {
        //ElementType elementType = mElementFunction::getElementType(_labelEleType->text());
        //_elementParamTableWidget->edit({ 1,2 }, elementType);
    }
    void mElementPanel::connectSig()
    {
        disconnectSig();
        if (!getCVM  || !getCVM->getPreRenderWidget())
        {
            return;
        }
        //视口拾取由通用拾取控件自行连接处理，此处仅监听视口切换
        connect(MechPreData::mViewportModelManager::getInstance(), &MechPreData::mViewportModelManager::currentViewportChanged, this, &mElementPanel::slot_subWindowActivated, Qt::UniqueConnection);
    }
    void mElementPanel::disconnectSig()
    {
        disconnect(MechPreData::mViewportModelManager::getInstance(), &MechPreData::mViewportModelManager::currentViewportChanged, this, &mElementPanel::slot_subWindowActivated);
    }

    std::vector<mxr::MeshSelectInfoData> mElementPanel::checkElement(const std::vector<mxr::MeshSelectInfoData>& ids)
    {
        std::vector<mxr::MeshSelectInfoData> res;
        if (getCVM == nullptr)
        {
            return res;
        }
        auto meshData = getCVM->getModelData()->getMeshData();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        auto& propertyManager = getCVM->getModelData()->getManager<mPropertyManager>(Physical_Property);
        mDataElement* elementData = getCVM->getModelData()->getElementData();
        ElementType elementType = mElementFunction::getElementType(_labelEleType->text());
        if (elementType < 0)
        {
            return res;
        }
        MeshType meshType = mElementFunction::transformTypeStrToEnum(elementType);
        int noworder, lastorder;//当前单元的阶数和之前的阶数
        mElementFunction::getElementInfo(elementType, noworder);
        QStringList propertyeErrorMeshIds;
        QStringList shapeErrorMeshIds;
        QStringList orderErrorMeshIds;
        QStringList errorMeshIds;

        for (auto ele : ids)
        {
            if (meshData->getMeshTypeByEleId(ele.data_id.element_id) != meshType)
            {
                shapeErrorMeshIds.append(MxFunctions::mNumberToString(ele.data_id.element_id));
                continue;
            }
            RenderPickResultData data;
            data.result_id = ele.data_id.element_id;
            data.part_id = ele.part_id;
            lastorder = meshData->getOrder(data);
            if (noworder != lastorder)//判断阶数是否相同
            {
                orderErrorMeshIds.append(MxFunctions::mNumberToString(ele.data_id.element_id));
                continue;
            }
            else if (_type != "低阶高精度" && _type != "轴对称" && _type != "绘图显示" && _type != "质量")
            {
                int propertyId = mapRelation.getMapRelation<int>({ Physical_Part, ele.part_id }, Physical_Property);
                mPropertyBase* propertyBase = propertyManager.getPropertyData(propertyId);
                if (propertyBase == nullptr)
                {
                    propertyeErrorMeshIds.append(MxFunctions::mNumberToString(ele.data_id.element_id));
                    continue;
                }
                QString mainType = mElementFunction::getElementMainType(elementType);
                if (_type == "弹簧" || _type == "阻尼" || _type == "六向弹簧阻尼")//弹簧之间不能互相切换赋予
                {
                    if (mainType != propertyBase->getPropertyTypeStr())
                    {
                        errorMeshIds.append(MxFunctions::mNumberToString(ele.data_id.element_id));
                        continue;
                    }
                }
                else if (propertyBase->getPropertyType() == SolidProperty && mainType == "实体壳")//实体壳单元能够赋予到三维实体截面属性上去
                {
                    
                }
                else if (propertyBase->getPropertyType() == ShellProperty  && mainType == "膜")//膜单元能够赋予到壳截面属性上去
                {
                    
                }
                else if (propertyBase->getPropertyType() == PlyLaminateProperty && mainType == "壳")//壳单元能够赋予到铺层截面属性上去
                {
                   
                }
                else if(!propertyBase->getPropertyTypeStr().contains(mainType))
                {
                    errorMeshIds.append(MxFunctions::mNumberToString(ele.data_id.element_id));
                    continue;
                }
            }
            res.push_back(ele);
        }
        if (!propertyeErrorMeshIds.isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, QString("单元ID为%1的单元没有赋予截面属性！").arg(propertyeErrorMeshIds.join(",")));
        }
        if (!shapeErrorMeshIds.isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, QString("单元ID为%1的单元形状和当前类型不匹配！").arg(shapeErrorMeshIds.join(",")));
        }
        if (!orderErrorMeshIds.isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, QString("单元ID为%1的单元阶数和当前类型不匹配！").arg(orderErrorMeshIds.join(",")));
        }
        if (!errorMeshIds.isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, QString("单元ID为%1的属性和单元不匹配！").arg(errorMeshIds.join(",")));
        }
        return res;
    }
    void mElementPanel::assignElement(std::vector<mxr::MeshSelectInfoData> meshIDs)
    {
        if (getCVM == nullptr)
        {
            return;
        }
        mDataElement* elementData = getCVM->getModelData()->getElementData();
        ElementType elementType = mElementFunction::getElementType(_labelEleType->text());
        if (elementType < 0)
        {
            return;
        }
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        auto meshData = getCVM->getModelData()->getMeshData();
        if (basicTypes.contains(_type))
        {
            // 基本类型：直接分配 ElementType
            for (auto meshID : meshIDs)
            {
                elementData->assignElementTypeToMesh(meshID.data_id.element_id, elementType);
            }
        }
        else if (_type == "梁")
        {
            updateOrAssignComplexElement<mBeamElement>(meshIDs, elementData, elementType);
        }
        else if (_type == "弹簧" || _type == "阻尼")
        {
            updateOrAssignComplexElement<mElas1Element>(meshIDs, elementData, elementType);
        }
        else if (_type == "六向弹簧阻尼")
        {
            updateOrAssignComplexElement<mCbushElement>(meshIDs, elementData, elementType);
        }
        else if (_type == "壳")
        {
            updateOrAssignComplexElement<mShellElement>(meshIDs, elementData, elementType);
        }
        else if (_type == "质量")
        {
            if (elementType == Mass1)
            {
                updateOrAssignComplexElement<mMassElement>(meshIDs, elementData, elementType);
            }
            else if(elementType == CMASS2)
            {
                updateOrAssignComplexElement<mCmass2Element>(meshIDs, elementData, elementType);
            }
        }

        mGlobalSignals::getInstance()->outputMessageSig(0, QString("已将%1个单元设置为%2类型").arg(meshIDs.size()).arg(_labelEleType->text()));
    }
    std::vector<mxr::MeshSelectInfoData> mElementPanel::collectPickedElements()
    {
        std::vector<mxr::MeshSelectInfoData> eles;
        if (_pickController == nullptr)
        {
            return eles;
        }
        const auto& pickedData = _pickController->getAllData().pickedData;
        auto it = pickedData.find(MxFunctions::PickDataType::Element);
        if (it == pickedData.end())
        {
            return eles;
        }
        eles.reserve(it->second.size());
        for (const auto& data : it->second)
        {
            eles.push_back(PickData2SelectData(MxFunctions::PickDataType::Element, data));
        }
        return eles;
    }

    void mElementPanel::tempHighlightEle()
    {
        if (!getCVM || !getCVM->getPreRenderWidget())
        {
            return;
        }
        auto meshData = getCVM->getModelData()->getMeshData();
        auto elementData = getCVM->getModelData()->getElementData();
        ElementType elementType = mElementFunction::getElementType(_labelEleType->text());

        QList<int> ids = elementData->getElementIDs(elementType);

        std::vector<MeshSelectInfoData> res;
        for (auto id : ids)
        {
            res.push_back(PickData2SelectData(mxr::SelectionInfoType::MeshElement, meshData->getElementRPRDByID(id)));
        }

        auto widget_ptr = getCVM->getPreRenderWidget();
        auto highlight_controller = widget_ptr->getHighlightController();
        MeshSelectMemoryData* meshSlaveData = new MeshSelectMemoryData();
        meshSlaveData->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshElement, res);

        auto meshHighLightRender = highlight_controller->getExtraPreMeshHighLightRender("blue", false);
        meshHighLightRender->initElementHightLightRender();
        meshHighLightRender->setHighLightColor(0, 0, 255);
        meshHighLightRender->updateHighLightRender(meshSlaveData);
        delete meshSlaveData;

        //更新表格
        if (basicTypes.contains(_type) || elementType == SPRING1 || elementType == SPRING2 || elementType == SPRINGA || elementType == DASHPOTA)
        {
            _elementParamTableWidget->unedit();
        }
        else
        {
            _elementParamTableWidget->edit(ids, elementType);
        }
    }

    void mElementPanel::deactivatePickWidget()
    {
        if (_pickController == nullptr)
        {
            return;
        }

        //失活提交并清空视口拾取高亮
        _pickController->setKeepHighlightOnDeactivate(false);
        if (_pickController->isPickActive())
        {
            _pickController->setPickActive(false);
        }
        else
        {
            _pickController->clearAllHighlight();
        }
        _pickController->setKeepHighlightOnDeactivate(true);

        //清空拾取数据
        PickWidgetData emptyData;
        _pickController->displayData(emptyData);

        //切出拾取：恢复拾取工具栏全部按钮
        emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>(10, true));
    }
}