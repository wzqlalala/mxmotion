
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
#include <QRadioButton>
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
#include "mCreateMetaMeshPanel.h"
using namespace MechPreData;
using namespace MeshPanelData;
using namespace MxFunctions;
#include <set>
#include "mOtherFunction.h"
Q_DECLARE_METATYPE(std::set<int>)
Q_DECLARE_METATYPE(MeshParam)          // 头文件

namespace MechMainWindow
{

    mCreateMetaMeshPanel::mCreateMetaMeshPanel(QWidget* parent)
    {

        // 提取为成员函数或lambda-设置界面的节点值
        auto setupNodePicker = [&](QToolButton* tool, QLineEdit* editx, QLineEdit* edity, QLineEdit* editz) {
            connect(tool, &QToolButton::clicked, [=]() {
                auto pickMeshIDs = getCVM->getPreRenderWidget()
                    ->getSelectionContainer()
                    ->getMeshSelectData()
                    ->getSelectDataByType(SelectionInfoType::MeshNode);

                if (pickMeshIDs.empty())
                    return;

                //根据节点ID获取对应的XYZ
                QVector3D point = getCVM->getModelData()->getMeshData()->getVectorById(pickMeshIDs.back().data_id.node_id);
                editx->setText(MxFunctions::mNumberToString(point.x()));
                edity->setText(MxFunctions::mNumberToString(point.y()));
                editz->setText(MxFunctions::mNumberToString(point.z()));
                
                });
            };
        auto* mainLay = new QVBoxLayout(this);
        
         m_rangeFold = new FoldSection("部件", this);
        _meshControlLabel = new PanelLabel("所属部件");
        _meshControlBox = new QComboBox();
        _meshControlEdit = new QLineEdit();
        _meshControlBox->addItems({ "新建" , "部件名称" });
        connect(_meshControlBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index)//如果不是新建，则禁用输入框
            {
                
                if (index == 0)
                {
                    _meshControlEdit->setDisabled(false);
                }
                else
                {
                    _meshControlEdit->setDisabled(true);
                }
            }
        );

        QLabel *_meshOrderLabel = new PanelLabel("网格阶次");
        QRadioButton* radio1 = new QRadioButton("一阶");
        QRadioButton* radio2 = new QRadioButton("二阶");
        radio1->setChecked(true);
        m_rangeFold->addRow(_meshControlLabel, _meshControlBox, _meshControlEdit);
        m_rangeFold->addRow(_meshOrderLabel, radio1, radio2);

        mainLay->addWidget(m_rangeFold);
        // ========== 拓扑信息折叠块 ==========
        m_TopologyFold = new FoldSection("拓扑信息", this);
        QLabel* _meshShape = new PanelLabel("拓扑形状");
        QComboBox* _meshShapeBox = new QComboBox();
        _meshShapeBox->addItems({ "四边形","圆面","球面","柱面","矩形面","长方体","球体","柱体","圆环体" });
        QLabel* _Graphlabel = new QLabel; _Graphlabel->setStyleSheet("border:1px solid #DBE5EE");
        QPixmap pixmap(":/mMainWidgets/Icon/Mesh/ti.png");
        // 检查是否为空
        if (pixmap.isNull()) {
            qDebug() << "图片加载失败！";
        }
        _Graphlabel->setPixmap(pixmap);
        //四边形
        stackWidget = new QStackedWidget;
        stackWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        QLabel* pointLabel1 = new PanelLabel("点1坐标");
        QLineEdit* xEdit = new QLineEdit;
        QLineEdit* yEdit = new QLineEdit;
        QLineEdit* zEdit = new QLineEdit;
        QToolButton* pBtn1 = new QToolButton; pBtn1->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        QLabel* pointLabel2 = new PanelLabel("点2坐标");
        QLineEdit* xEdit2 = new QLineEdit;
        QLineEdit* yEdit2 = new QLineEdit;
        QLineEdit* zEdit2 = new QLineEdit;
        QToolButton* pBtn2 = new QToolButton; pBtn2->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        QLabel* pointLabel3 = new PanelLabel("点3坐标");
        QLineEdit* xEdit3 = new QLineEdit;
        QLineEdit* yEdit3 = new QLineEdit;
        QLineEdit* zEdit3 = new QLineEdit;
        QToolButton* pBtn3 = new QToolButton; pBtn3->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        QWidget* widget1 = new QWidget;
        stackWidget->addWidget(widget1);
        QFormLayout* m_formLayout = new QFormLayout(widget1);
        m_formLayout->setContentsMargins(0, 0, 0, 0);
        m_formLayout->setSpacing(10);
        QHBoxLayout* h = new QHBoxLayout();
        h->addWidget(xEdit); h->addWidget(yEdit); h->addWidget(zEdit); h->addWidget(pBtn1);
        m_formLayout->addRow(pointLabel1, h);
        h = new QHBoxLayout();
        h->addWidget(xEdit2); h->addWidget(yEdit2); h->addWidget(zEdit2); h->addWidget(pBtn2);
        m_formLayout->addRow(pointLabel2, h);
        h = new QHBoxLayout();
        h->addWidget(xEdit3); h->addWidget(yEdit3); h->addWidget(zEdit3); h->addWidget(pBtn3);
        m_formLayout->addRow(pointLabel3, h);

        setupNodePicker(pBtn1, xEdit, yEdit, zEdit);//绑定信号和槽-添加点的xyz
        setupNodePicker(pBtn2, xEdit2, yEdit2, zEdit2);
        setupNodePicker(pBtn3, xEdit3, yEdit3, zEdit3);

        //圆面
        QLabel* center = new PanelLabel("圆心");
        QLineEdit* ctrEdit1 = new QLineEdit;
        QLineEdit* ctrEdit2 = new QLineEdit;
        QLineEdit* ctrEdit3 = new QLineEdit;
        QToolButton* ctrBtn = new QToolButton; ctrBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        vecPick.initWidget("法向");
        QLabel* radius = new PanelLabel("半径");
        QLineEdit* radiusEdit = new QLineEdit();
        QLabel* bias = new PanelLabel("偏置");
        QLineEdit* biasEdit = new QLineEdit();
        QLabel* angle = new PanelLabel("角度");
        QLineEdit* angleEdit = new QLineEdit();
        QWidget* widget2 = new QWidget;
        stackWidget->addWidget(widget2);
        m_formLayout = new QFormLayout(widget2);
        m_formLayout->setContentsMargins(0, 0, 0, 0);
        m_formLayout->setSpacing(10);
        h = new QHBoxLayout();
        h->addWidget(ctrEdit1); h->addWidget(ctrEdit2); h->addWidget(ctrEdit3); h->addWidget(ctrBtn);
        m_formLayout->addRow(center,h);
        m_formLayout->addRow(vecPick.propwid);
        m_formLayout->addRow(radius,radiusEdit);
        m_formLayout->addRow(bias, biasEdit);
        m_formLayout->addRow(angle, angleEdit);
        setupNodePicker(ctrBtn, ctrEdit1, ctrEdit2, ctrEdit3);//绑定信号和槽-添加点的xyz

        //球面
        QLabel* center_ = new PanelLabel("圆心");
        QLineEdit* ctrEdit1_ = new QLineEdit;
        QLineEdit* ctrEdit2_ = new QLineEdit;
        QLineEdit* ctrEdit3_ = new QLineEdit;
        QToolButton* ctrBtn_ = new QToolButton; ctrBtn_->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        QLabel* radius_ = new PanelLabel("半径");
        QLineEdit* radiusEdit_ = new QLineEdit();
        QWidget* widget3 = new QWidget;
        stackWidget->addWidget(widget3);
        m_formLayout = new QFormLayout(widget3);
        m_formLayout->setContentsMargins(0, 0, 0, 0);
        m_formLayout->setSpacing(10);
        h = new QHBoxLayout();
        h->addWidget(ctrEdit1_); h->addWidget(ctrEdit2_); h->addWidget(ctrEdit3_); h->addWidget(ctrBtn_);
        m_formLayout->addRow(center_, h);
        m_formLayout->addRow(radius_, radiusEdit_);
        setupNodePicker(ctrBtn_, ctrEdit1_, ctrEdit2_, ctrEdit3_);//绑定信号和槽-添加点的xyz

        //柱面
        QLabel* downCtr = new PanelLabel("底部圆心");
        QLineEdit* downCtrEdit1 = new QLineEdit;
        QLineEdit* downCtrEdit2 = new QLineEdit;
        QLineEdit* downCtrEdit3 = new QLineEdit;
        QToolButton* downCtrBtn = new QToolButton; downCtrBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        vecPick1.initWidget("轴");
        QLabel* downR = new PanelLabel("底部半径");
        QLineEdit* downREdit = new QLineEdit;
        QCheckBox* downBox = new QCheckBox("底盖");
        QLabel* upR = new PanelLabel("顶部半径");
        QLineEdit* upREdit = new QLineEdit;
        QCheckBox* upBox = new QCheckBox("顶盖");
        QLabel* height = new PanelLabel("高度");
        QLineEdit* heightEdit = new QLineEdit;
        QWidget* widget4 = new QWidget;
        stackWidget->addWidget(widget4);
        m_formLayout = new QFormLayout(widget4);
        m_formLayout->setContentsMargins(0, 0, 0, 0);
        m_formLayout->setSpacing(10);
        h = new QHBoxLayout();
        h->addWidget(downCtrEdit1); h->addWidget(downCtrEdit2); h->addWidget(downCtrEdit3); h->addWidget(downCtrBtn);
        m_formLayout->addRow(downCtr, h);
        m_formLayout->addRow(vecPick1.propwid);
        h = new QHBoxLayout();
        h->addWidget(downREdit); h->addWidget(downBox);
        m_formLayout->addRow(downR,h);
        h = new QHBoxLayout();
        h->addWidget(upREdit); h->addWidget(upBox);
        m_formLayout->addRow(upR, h);
        m_formLayout->addRow(height, heightEdit);
        setupNodePicker(downCtrBtn, downCtrEdit1, downCtrEdit2, downCtrEdit3);//绑定信号和槽-添加点的xyz

        //矩形面
        QLabel* coor = new PanelLabel("参考系");
        QComboBox* coorBox = new QComboBox();
        coorBox->addItems({"全局坐标系"});
        QLabel* xRange = new PanelLabel("X范围");
        QLineEdit* xRangeEdit1 = new QLineEdit();
        QLabel* xRange1 = new PanelLabel("-"); xRange1->setFixedWidth(30); xRange1->setAlignment(Qt::AlignHCenter);
        QLineEdit* xRangeEdit2 = new QLineEdit();
        QLabel* yRange = new PanelLabel("Y范围");
        QLineEdit* yRangeEdit1 = new QLineEdit();
        QLabel* yRange1 = new PanelLabel("-"); yRange1->setFixedWidth(30); yRange1->setAlignment(Qt::AlignHCenter);
        QLineEdit* yRangeEdit2 = new QLineEdit();
        QLabel* zRange = new PanelLabel("Z范围");
        QLineEdit* zRangeEdit1 = new QLineEdit();
        QLabel* zRange1 = new PanelLabel("-"); zRange1->setFixedWidth(30); zRange1->setAlignment(Qt::AlignHCenter);
        QLineEdit* zRangeEdit2 = new QLineEdit();
        QLabel* openFace = new PanelLabel("开口面");
        QComboBox* openFaceBox = new QComboBox();
        openFaceBox->addItems({"无"});
        QWidget* widget5 = new QWidget;
        stackWidget->addWidget(widget5);
        m_formLayout = new QFormLayout(widget5);
        m_formLayout->setContentsMargins(0, 0, 0, 0);
        m_formLayout->setSpacing(10);
        m_formLayout->addRow(coor, coorBox);
        h = new QHBoxLayout(); h->addWidget(xRangeEdit1); h->addWidget(xRange1); h->addWidget(xRangeEdit2);
        m_formLayout->addRow(xRange, h);
        h = new QHBoxLayout(); h->addWidget(yRangeEdit1); h->addWidget(yRange1); h->addWidget(yRangeEdit2);
        m_formLayout->addRow(yRange, h);
        h = new QHBoxLayout(); h->addWidget(zRangeEdit1); h->addWidget(zRange1); h->addWidget(zRangeEdit2);
        m_formLayout->addRow(zRange, h);
        m_formLayout->addRow(openFace, openFaceBox);

        //长方体
        QLabel* coor_ = new PanelLabel("参考系");
        QComboBox* coorBox_ = new QComboBox();
        coorBox_->addItems({ "全局坐标系" });
        QLabel* xRange_ = new PanelLabel("X范围");
        QLineEdit* xRangeEdit1_ = new QLineEdit();
        QLabel* xRange1_ = new PanelLabel("-"); xRange1_->setFixedWidth(30); xRange1_->setAlignment(Qt::AlignHCenter);
        QLineEdit* xRangeEdit2_ = new QLineEdit();
        QLabel* yRange_ = new PanelLabel("Y范围");
        QLineEdit* yRangeEdit1_ = new QLineEdit();
        QLabel* yRange1_ = new PanelLabel("-"); yRange1_->setFixedWidth(30); yRange1_->setAlignment(Qt::AlignHCenter);
        QLineEdit* yRangeEdit2_ = new QLineEdit();
        QLabel* zRange_ = new PanelLabel("Z范围");
        QLineEdit* zRangeEdit1_ = new QLineEdit();
        QLabel* zRange1_ = new PanelLabel("-"); zRange1_->setFixedWidth(30); zRange1_->setAlignment(Qt::AlignHCenter);
        QLineEdit* zRangeEdit2_ = new QLineEdit();
        QWidget* widget6 = new QWidget;
        stackWidget->addWidget(widget6);
        m_formLayout = new QFormLayout(widget6);
        m_formLayout->setContentsMargins(0, 0, 0, 0);
        m_formLayout->setSpacing(10);
        m_formLayout->addRow(coor_, coorBox_);
        h = new QHBoxLayout(); h->addWidget(xRangeEdit1_); h->addWidget(xRange1_); h->addWidget(xRangeEdit2_);
        m_formLayout->addRow(xRange_, h);
        h = new QHBoxLayout(); h->addWidget(yRangeEdit1_); h->addWidget(yRange1_); h->addWidget(yRangeEdit2_);
        m_formLayout->addRow(yRange_, h);
        h = new QHBoxLayout(); h->addWidget(zRangeEdit1_); h->addWidget(zRange1_); h->addWidget(zRangeEdit2_);
        m_formLayout->addRow(zRange_, h);

        //球体
        QLabel* ballCtr = new PanelLabel("球心");
        QLineEdit* ballCtrEdit1 = new QLineEdit;
        QLineEdit* ballCtrEdit2 = new QLineEdit;
        QLineEdit* ballCtrEdit3 = new QLineEdit;
        QToolButton* ballCtrBtn = new QToolButton; ballCtrBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        QLabel* ballR = new PanelLabel("半径");
        QLineEdit* ballREdit= new QLineEdit;
        QWidget* widget7 = new QWidget;
        stackWidget->addWidget(widget7);
        m_formLayout = new QFormLayout(widget7);
        m_formLayout->setContentsMargins(0, 0, 0, 0);
        m_formLayout->setSpacing(10);
        h = new QHBoxLayout();
        h->addWidget(ballCtrEdit1); h->addWidget(ballCtrEdit2); h->addWidget(ballCtrEdit3); h->addWidget(ballCtrBtn);
        m_formLayout->addRow(ballCtr, h);
        m_formLayout->addRow(ballR, ballREdit);
        setupNodePicker(ballCtrBtn, ballCtrEdit1, ballCtrEdit2, ballCtrEdit3);//绑定信号和槽-添加点的xyz

        //柱体和柱面一样
        //圆环体
        QLabel* AnnR = new PanelLabel("圆环半径");
        QLineEdit* AnnREdit = new QLineEdit();
        QLabel* secR = new PanelLabel("截面半径");
        QLineEdit* secREdit = new QLineEdit();
        QWidget* widget8 = new QWidget;
        stackWidget->addWidget(widget8);
        m_formLayout = new QFormLayout(widget8);
        m_formLayout->setContentsMargins(0, 0, 0, 0);
        m_formLayout->setSpacing(10);
        m_formLayout->addRow(AnnR, AnnREdit);
        m_formLayout->addRow(secR, secREdit);

        m_TopologyFold->addRow(_meshShape, _meshShapeBox);
        m_TopologyFold->addRow(_Graphlabel);
        m_TopologyFold->addRow(stackWidget);
        mainLay->addWidget(m_TopologyFold);

        // ========== 网格数量折叠块 ==========
        m_ParamFold = new FoldSection("网格数量", this);
        QLabel* edge1 = new PanelLabel("边1数量");
        QLabel* edge2 = new PanelLabel("边2数量");
        QLineEdit* edgeEdit1 = new QLineEdit;
        QLineEdit* edgeEdit2 = new QLineEdit;
        QLabel* xDir = new  PanelLabel("X向数量");
        QLabel* yDir = new  PanelLabel("Y向数量");
        QLabel* zDir = new  PanelLabel("Z向数量");
        QLineEdit* xDirEdit= new QLineEdit;
        QLineEdit* yDirEdit = new QLineEdit;
        QLineEdit* zDirEdit = new QLineEdit;
        m_ParamFold->addRow(edge1, edgeEdit1);
        m_ParamFold->addRow(edge2, edgeEdit2);
        m_ParamFold->addRow(xDir, xDirEdit);
        m_ParamFold->addRow(yDir, yDirEdit);
        m_ParamFold->addRow(zDir, zDirEdit);
        mainLay->addWidget(m_ParamFold);

        // ========== 网格尺寸折叠块 ==========
        m_SizeFold = new FoldSection("网格尺寸", this);
        QLabel* size = new PanelLabel("尺寸");
        QLineEdit* sizeEdit = new QLineEdit();
        m_SizeFold->addRow(size, sizeEdit);

        QFrame* horizontalLine = createHorizontalLine();
        QPushButton* applyBtn = new QPushButton("创建");

        m_foldWidgets.append(m_ParamFold);
        mainLay->addWidget(m_SizeFold);
        mainLay->addWidget(horizontalLine);
        h = new QHBoxLayout();
        h->addStretch(); h->addWidget(applyBtn);
        mainLay->addLayout(h);
        mainLay->addStretch();

        connect(_meshShapeBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index)
            {
                if(index==7)
                    stackWidget->setCurrentIndex(3);
                else if(index==8)
                    stackWidget->setCurrentIndex(7);
                else
                    stackWidget->setCurrentIndex(index);             
            }
        );
    
        // 关键：安装事件过滤器，切换时调整
        connect(stackWidget, &QStackedWidget::currentChanged, this, [=](int index) {
            QWidget* current = stackWidget->widget(index);
            if (current) {
                stackWidget->setFixedHeight(current->sizeHint().height());
                // 或
                stackWidget->adjustSize();
            }
            m_TopologyFold->refreshLayout();
            });

        stackWidget->setFixedHeight(widget1->sizeHint().height());
        // 或
        stackWidget->adjustSize();

    }






    mCreateMetaMeshPanel::~mCreateMetaMeshPanel()
    {
    }

    void mCreateMetaMeshPanel::newMeshPanel()
    {
    }
    void mCreateMetaMeshPanel::editMeshPanel(int meshId)
    {
        _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(meshId);

        updatePanel();
    }
    void mCreateMetaMeshPanel::getMeshInfoById(const std::vector<int> ids)
    {

    }
    void mCreateMetaMeshPanel::updatePanel()
    {



    }



}
