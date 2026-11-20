#include "mCustomMaterialWindow.h"
#include "ui_mCustomMaterialWindow.h"
#include "mMechGlobalSignals.h"
#include "mUnitEnum.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mOtherFunction.h"

#include "mModelData.h"
#include "mMaterialBase.h"
#include "mPhysicalFieldEnum.h"

#include "mCustomMaterialProps.h"
#include "mPanelBase.h"
#include "mMaterialProps.h"

// Qt
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QStyledItemDelegate>
#include <QLineEdit>
#include <QKeyEvent>
#include <QMessageBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <memory>
#include <QGroupBox>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDir>
#include <QCoreApplication>
#include "mSectionWindow.h"
#include "mOMxFile.h"
//读文件
#include "mIMxFile.h"
#include "mIOFileManager.h"

#include "mInfoWindow.h"
#include "mIOFileManager.h"
using namespace MxFunctions;
using namespace MechMaterialData;
using namespace MechIOFile;
namespace MechMainWindow
{
    namespace
    {
        // 将 MaterialType 枚举映射为中文类型名（显示在 label_MatTypeEdit 中）
        QString materialTypeToName(MaterialType type)
        {
            switch (type)
            {
            case LinearElastic_Isotropic:                 return QStringLiteral("各向同性");
            case LinearElastic_Orthotropic:               return QStringLiteral("正交异性");
            case LinearElastic_Anisotropic:               return QStringLiteral("各向异性");
            case Plastic_Normal:                          return QStringLiteral("常规塑性");
                //case MAT_KinematicHardening:       return QStringLiteral("常规塑性-随动硬化");
                //case MAT_CompressiveHardening:     return QStringLiteral("常规塑性-组合硬化");
            case Plastic_Cyclic:                 return QStringLiteral("循环塑性");
            case Plastic_Damage:                 return QStringLiteral("变形塑性");
            case Plastic_Johnson:                return QStringLiteral("Johnson-Cook塑性");
            case Damage_Shear:                   return QStringLiteral("剪切损伤");
            case HyperElastic_NeoHookean:               return QStringLiteral("Neo-Hookean");
                //case MAT_Neohookean2:              return QStringLiteral("Neo-Hookean2");
            case HyperElastic_MooneyRivlin:             return QStringLiteral("Mooney-Rivlin");
            case HyperElastic_Ogden:                    return QStringLiteral("Ogden");
            case HyperElastic_ArrudaBoyce:              return QStringLiteral("Arruda-Boyce");
            case HyperElastic_Yeoh:                     return QStringLiteral("Yeoh");
            case HyperElastic_Polynomial:               return QStringLiteral("Polynomial");
            case Viscous_Viscoplastic:                  return QStringLiteral("粘塑性");
            case Viscous_Viscoelastic:                  return QStringLiteral("粘弹性");
            case Thermal_Base:                          return QStringLiteral("热学材料");
            case OtherMaterial_Foam:                    return QStringLiteral("超弹泡沫");
            case OtherMaterial_Brittle:                 return QStringLiteral("脆性材料");
            case OtherMaterial_Shim:                    return QStringLiteral("垫片材料");
            case OtherMaterial_Break:                   return QStringLiteral("断裂材料");
            case AcousticMaterial_Base:                 return QStringLiteral("声学材料");
            default:                                    return QStringLiteral("其它材料");
            }
        }

        // 判断该属性是否为曲线数据（包含温度相关、频率相关等）
        bool isCurveProperty(MaterialPropType propType)
        {
            return (propType >= MatProp_Curve_Density && propType <= MatProp_Curve_FailStress)
                || propType == MatProp_Curve_Fs_Ssr_Sr; //剪切损伤断裂曲线(枚举位于Curve区段之外)
        }

        // 根据属性类型与曲线维度生成与属性页编辑表格一致的表头
        QStringList defaultCurveHeaders(MaterialPropType propType, const MechMaterialData::Curves& curves,
            const QHash<MxFunctions::MaterialPropType, QVariant>& subProps)
        {
            QString first = curves.correlation == CurveType::FrequencyDependence
                ? QStringLiteral("频率")
                : QStringLiteral("温度");

            // 与温度/频率无关的曲线没有首列（如常规塑性未勾选"温度相关"的应力-塑性应变为2列）
            const bool noCorrelation = curves.correlation == CurveType::NoCorrelation;

            const int colCount = curves.curves.size();
            switch (propType)
            {
            case MatProp_Curve_ShimLoading:
                return { QStringLiteral("Pressure"),QStringLiteral("Closure") };
            case MatProp_Curve_ShimUnloading:
                return { QStringLiteral("Pressure"),QStringLiteral("Closure") ,QStringLiteral("Max.Closure") };
            case MatProp_Curve_TimeDomain:
                return  { QStringLiteral("g_i Prony  "),QStringLiteral("k_i Prony  "),QStringLiteral("tau_i Prony") };
            case MatProp_Curve_Fs_Ssr_Sr:
                return { QStringLiteral("断裂应变"),QStringLiteral("剪应力比"),QStringLiteral("应变率") };
            case MatProp_Curve_Density:
                return { first, QStringLiteral("密度") };
            case MatProp_Curve_CoefficientOfThermalExpansion:
                return { first, QStringLiteral("热膨胀系数") };
            case MatProp_Curve_Y_B:
                return { first, QStringLiteral("杨氏模量"), QStringLiteral("泊松比") };
            case MatProp_Curve_HeatConductivityCoefficient:
                return { first, QStringLiteral("热传导系数") };
            case MatProp_Curve_HeatCapacityDensity:
                return { first, QStringLiteral("比热容") };
            case MatProp_Curve_PlasticityProperties:
                if (noCorrelation)
                    return { QStringLiteral("应力"), QStringLiteral("塑性应变") };
                return { first, QStringLiteral("应力"), QStringLiteral("塑性应变") };
            case MatProp_Curve_CyclicHardeningWithTable:
                if (noCorrelation)
                    return { QStringLiteral("等效屈服应力"), QStringLiteral("等效塑性应变") };
                return { first, QStringLiteral("等效屈服应力"), QStringLiteral("等效塑性应变") };
            case MatProp_Curve_CreepBehavior:
                return { first, QStringLiteral("幂律乘数"), QStringLiteral("应力指数"), QStringLiteral("时间指数") };
            case MatProp_Curve_FailStrain:
            {
                // 失效应变表头随失效模型变化：0=各向同性（3列），1=二维正交各向异性（5列）
                int failType = 0;
                if (subProps.contains(MatProp_FailStrainType))
                    failType = subProps[MatProp_FailStrainType].toInt();
                if (failType == 1)
                    return { first, QStringLiteral("纵向拉伸应变极限"), QStringLiteral("纵向压缩应变极限"),
                             QStringLiteral("横向拉伸应变极限"), QStringLiteral("横向压缩应变极限"),
                             QStringLiteral("面内剪切应变极限") };
                return { first, QStringLiteral("拉伸应变极限"), QStringLiteral("压缩应变极限"),
                         QStringLiteral("剪切应变极限") };
            }
            case MatProp_Curve_Anisotropy_Gn:
                if (colCount == 7)
                    return { first, QStringLiteral("G11"), QStringLiteral("G12"), QStringLiteral("G13"),
                             QStringLiteral("G22"), QStringLiteral("G23"), QStringLiteral("G33") };
                if (colCount == 22)
                    return { first, QStringLiteral("G11"), QStringLiteral("G12"), QStringLiteral("G13"),
                             QStringLiteral("G14"), QStringLiteral("G15"), QStringLiteral("G16"),
                             QStringLiteral("G22"), QStringLiteral("G23"), QStringLiteral("G24"),
                             QStringLiteral("G25"), QStringLiteral("G26"), QStringLiteral("G33"),
                             QStringLiteral("G34"), QStringLiteral("G35"), QStringLiteral("G36"),
                             QStringLiteral("G44"), QStringLiteral("G45"), QStringLiteral("G46"),
                             QStringLiteral("G55"), QStringLiteral("G56"), QStringLiteral("G66") };
                break;
            case MatProp_Curve_Orthotropy:
            {
                int dim = 0;
                if (subProps.contains(MatProp_Dimensionality))
                    dim = subProps[MatProp_Dimensionality].toInt();
                if (dim == 0)
                    return { first, QStringLiteral("Ex"), QStringLiteral("Ey"), QStringLiteral("NUxy"),
                             QStringLiteral("Gxy"), QStringLiteral("Gxz"), QStringLiteral("Gyz") };
                if (dim == 1)
                    return { first, QStringLiteral("Ex"), QStringLiteral("Ey"), QStringLiteral("Ez"),
                             QStringLiteral("NUyz"), QStringLiteral("NUxz"), QStringLiteral("NUxy"),
                             QStringLiteral("Gyz"), QStringLiteral("Gxz"), QStringLiteral("Gxy") };
                if (dim == 2)
                    return { first, QStringLiteral("D1111"), QStringLiteral("D1122"), QStringLiteral("D2222"),
                             QStringLiteral("D1133"), QStringLiteral("D2233"), QStringLiteral("D3333"),
                             QStringLiteral("D1212"), QStringLiteral("D1313"), QStringLiteral("D2323") };
                break;
            }
            default:
                break;
            }
            return QStringList();
        }

        // 判断 QVariant 是否为 Curves 类型
        bool isCurveVariant(const QVariant& v)
        {
            return v.canConvert<MechMaterialData::Curves>();
        }

        // 与材料编辑界面（mMaterialPanel::m_SelectProps）一致的可勾选属性段。
        // 勾选状态通过材料属性持久化：props 中包含该属性段即视为勾选过。
        bool isOptionalPropClass(MxFunctions::MaterialClass matClass)
        {
            switch (matClass)
            {
            case MAT_Density:
            case MAT_MaterialDamping:
            case MAT_CoefficientOfThermalExpansion:
            case MAT_HeatConductivityCoefficient:
            case MAT_Heat:
            case MAT_YieldStress:
            case MAT_YieldStrain:
            case MAT_FailureHashin:
            case MAT_EnergyDamage:
            case MAT_CreepBehavior_Check:
            case MAT_HeatGenerationRate:
            case MAT_BulkModulus:
            case MAT_AcousticMaterialDamping:
            case MAT_RateDependent:
            case MAT_ShearDamage:
                return true;
            default:
                return false;
            }
        }

        // 找按钮同一输入行中已勾选的勾选框（温度相关等）。
        // 曲线表格按钮与勾选框由 singleCheckBox 创建在同一容器（propwid）内。
        QCheckBox* checkedSiblingCheckBox(QPushButton* btn)
        {
            QWidget* row = btn ? btn->parentWidget() : nullptr;
            if (!row)
                return nullptr;
            const QList<QCheckBox*> cbs = row->findChildren<QCheckBox*>();
            for (QCheckBox* cb : cbs)
            {
                if (cb->isChecked())
                    return cb;
            }
            return nullptr;
        }

        // 在目标表格中填充曲线数据，返回是否填充成功
        bool fillCurveTable(QTableWidget* table, const MechMaterialData::Curves& curves,
            const QStringList& headers = QStringList())
        {
            if (!table || curves.curves.isEmpty())
                return false;

            table->clear();
            table->setRowCount(0);
            table->setColumnCount(0);

            const int curveCount = curves.curves.size();
            int rowCount = 0;
            for (int i = 0; i < curveCount; ++i)
                rowCount = qMax(rowCount, curves.curves[i].size());
            if (rowCount <= 0)
                return false;

            table->setColumnCount(curveCount);
            table->setHorizontalHeaderLabels(headers);
            for (int i = 0; i < curveCount; ++i)
                table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);

            table->setRowCount(rowCount);
            for (int row = 0; row < rowCount; ++row)
            {
                for (int col = 0; col < curveCount; ++col)
                {
                    if (row < curves.curves[col].size())
                    {
                        QTableWidgetItem* item = new QTableWidgetItem(
                            MxFunctions::mNumberToString(curves.curves[col][row], 'g', 12));
                        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                        table->setItem(row, col, item);
                    }
                }
            }
            return true;
        }

        // 递归禁用容器内所有可交互控件，使其只读。
        // 折叠块本身与折叠头按钮保持可点击，以便用户展开/收起查看内容；
        // 曲线表格按钮是否可点击由 showMaterialReadOnly 根据勾选框状态单独恢复。
        void disableAllInteractiveChildren(QWidget* parent)
        {
            if (!parent)
                return;

            for (QWidget* w : parent->findChildren<QWidget*>())
            {
                // 折叠块保持可用（其内部控件已被逐一禁用）
                if (FoldSection* fold = qobject_cast<FoldSection*>(w))
                {
                    fold->setEnabled(true);
                    continue;
                }

                if (QLineEdit* le = qobject_cast<QLineEdit*>(w))
                {
                    le->setReadOnly(true);
                    le->setFocusPolicy(Qt::NoFocus);
                }
                else if (QComboBox* cb = qobject_cast<QComboBox*>(w))
                {
                    cb->setEnabled(false);
                }
                else if (QCheckBox* ck = qobject_cast<QCheckBox*>(w))
                {
                    ck->setEnabled(false);
                }
                else if (QRadioButton* rb = qobject_cast<QRadioButton*>(w))
                {
                    rb->setEnabled(false);
                }
                else if (QPushButton* pb = qobject_cast<QPushButton*>(w))
                {
                    // 折叠头按钮位于 FoldWidget 的标题栏内，保留可点击
                    QWidget* header = pb->parentWidget();
                    if (header && header->parentWidget() && qobject_cast<FoldWidget*>(header->parentWidget()))
                        continue;
                    pb->setEnabled(false);
                }
                else if (QSpinBox* sb = qobject_cast<QSpinBox*>(w))
                {
                    sb->setReadOnly(true);
                    sb->setFocusPolicy(Qt::NoFocus);
                }
                else if (QDoubleSpinBox* dsb = qobject_cast<QDoubleSpinBox*>(w))
                {
                    dsb->setReadOnly(true);
                    dsb->setFocusPolicy(Qt::NoFocus);
                }
                else if (QTableWidget* tw = qobject_cast<QTableWidget*>(w))
                {
                    tw->setEditTriggers(QAbstractItemView::NoEditTriggers);
                    tw->setSelectionMode(QAbstractItemView::NoSelection);
                    tw->setFocusPolicy(Qt::NoFocus);
                }
                else if (QTreeWidget* trw = qobject_cast<QTreeWidget*>(w))
                {
                    trw->setEditTriggers(QAbstractItemView::NoEditTriggers);
                    trw->setSelectionMode(QAbstractItemView::NoSelection);
                    trw->setFocusPolicy(Qt::NoFocus);
                }
            }
        }
    }

    mCustomMaterialWindow::mCustomMaterialWindow(QWidget* parent)
        : QWidget(parent)
        , ui(new Ui::mCustomMaterialWindowClass())
        , m_customProps(new mCustomMaterialProps())
    {
        ui->setupUi(this);
        this->setWindowTitle("自定义材料库");
        this->setWindowIcon(QIcon(":/mMainWidgets/Icon/logo.png"));
        setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
        setWindowFlags(windowFlags() & ~Qt::WindowMinimizeButtonHint & ~Qt::WindowMaximizeButtonHint);

        initUI();
        initConnect();
        initMaterialTree();
        initPropertyTables();

        m_PanelConfig[LinearElastic_Isotropic] = { MAT_MaterialDefine, MAT_Isotropy,MAT_Density,MAT_MaterialDamping,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };
        m_PanelConfig[LinearElastic_Orthotropic] = { MAT_MaterialDefine,MAT_Orthotropy,MAT_Density,MAT_MaterialDamping,MAT_CoefficientOfThermalExpansion ,MAT_HeatConductivityCoefficient ,MAT_Heat /*,
        MAT_YieldStress*/ ,MAT_YieldStrain ,MAT_FailureHashin ,MAT_EnergyDamage,NONETYPE };
        m_PanelConfig[LinearElastic_Anisotropic] = { MAT_MaterialDefine, MAT_Anisotropy,MAT_Density,MAT_MaterialDamping,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };

        // 常规塑性不显示循环硬化面板，该面板仅循环塑性材料（Plastic_Cyclic）使用
        m_PanelConfig[Plastic_Normal] = { MAT_MaterialDefine, MAT_Flex,MAT_Plasticity,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,MAT_CreepBehavior_Check,NONETYPE };
        m_PanelConfig[Plastic_Cyclic] = { MAT_MaterialDefine, MAT_Flex,MAT_PlasticityWithOrder,MAT_CyclicHardening,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };
        m_PanelConfig[Plastic_Damage] = { MAT_MaterialDefine,MAT_Deformation,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };
        //与mMaterialPanel一致：率相关/剪切损伤/密度/热传导/比热容为可选折叠框
        m_PanelConfig[Plastic_Johnson] = { MAT_MaterialDefine, MAT_Flex, MAT_PlasticityJohnson, MAT_RateDependent, MAT_ShearDamage, MAT_Density, MAT_HeatConductivityCoefficient, MAT_Heat, NONETYPE };
        //旧版独立剪切损伤材料(存量文件)在库中的只读显示
        m_PanelConfig[Damage_Shear] = { MAT_MaterialDefine, MAT_ShearDamage, NONETYPE };

        m_PanelConfig[HyperElastic_MooneyRivlin] = { MAT_MaterialDefine, MAT_MooneyRivlin,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };
        m_PanelConfig[HyperElastic_NeoHookean] = { MAT_MaterialDefine, MAT_Neohookean2,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };
        m_PanelConfig[HyperElastic_Ogden] = { MAT_MaterialDefine, MAT_Ogden,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };
        m_PanelConfig[HyperElastic_Yeoh] = { MAT_MaterialDefine, MAT_Yeoh,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };
        m_PanelConfig[HyperElastic_ArrudaBoyce] = { MAT_MaterialDefine, MAT_Arrudaboyce,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };
        m_PanelConfig[HyperElastic_Polynomial] = { MAT_MaterialDefine, MAT_Polynomial,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };

        m_PanelConfig[Viscous_Viscoelastic] = { MAT_MaterialDefine, MAT_Viscoelasticity,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };
        m_PanelConfig[Viscous_Viscoplastic] = { MAT_MaterialDefine, MAT_Flex,MAT_CreepBehavior,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };

        m_PanelConfig[Thermal_Base] = { MAT_MaterialDefine,MAT_HeatConductivityCoefficient_UnCheck,MAT_Density,MAT_Heat,MAT_HeatGenerationRate,NONETYPE };

        m_PanelConfig[OtherMaterial_Shim] = { MAT_MaterialDefine, MAT_ThicknessBehavior,MAT_FilmBehavior,MAT_CutBehavior,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };
        m_PanelConfig[OtherMaterial_Brittle] = { MAT_MaterialDefine, MAT_HardBrittle,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };
        m_PanelConfig[OtherMaterial_Foam] = { MAT_MaterialDefine, MAT_Hyperhorm,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };
        m_PanelConfig[OtherMaterial_Break] = { MAT_MaterialDefine ,MAT_BreakParam, MAT_PhaseFieldParam,NONETYPE };

        m_PanelConfig[AcousticMaterial_Base] = { MAT_MaterialDefine ,MAT_SoundVelocity ,MAT_Density_UnCheck ,MAT_BulkModulus ,MAT_AcousticMaterialDamping,NONETYPE };

        m_PanelConfig[OtherMaterial_LIBRARY] = { MAT_LibCommon ,MAT_LibThermal,MAT_LibStructural };

    }

    mCustomMaterialWindow::~mCustomMaterialWindow()
    {
        delete ui;
    }

    void mCustomMaterialWindow::initUI()
    {
        ui->label_MatName->setText("材料名称：");
        ui->label_MatNameEdit->setText("--");
        ui->label_MatType->setText("材料类型：");
        ui->label_MatTypeEdit->setText("--");

        ui->tableWidget_Curve->hide();
        ui->widget_Curve->hide();

        // 替换 splitter_2 为可折叠风格的分割条
        QWidget* leftWidget = ui->splitter_2->widget(0);
        QWidget* rightWidget = ui->splitter_2->widget(1);
        if (leftWidget && rightWidget)
        {
            leftWidget->setParent(nullptr);
            rightWidget->setParent(nullptr);

            m_CustomSplitter = new CustomMatCollapsibleSplitter(ui->widget_2, 300);
            m_CustomSplitter->setObjectName("splitter_2");
            m_CustomSplitter->setHandleWidth(30);
            m_CustomSplitter->addWidget(leftWidget);
            m_CustomSplitter->addWidget(rightWidget);
            m_CustomSplitter->setCollapsible(0, false);

            QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(ui->widget_2->layout());
            if (layout)
            {
                // 原 splitter_2 是 verticalLayout_3 的第 1 个 item
                layout->removeWidget(ui->splitter_2);
                layout->insertWidget(1, m_CustomSplitter, 1);
            }

            delete ui->splitter_2;
            ui->splitter_2 = m_CustomSplitter;
        }

    }

    void mCustomMaterialWindow::initConnect()
    {
        connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::loadCustomDepotSig,
            this, &mCustomMaterialWindow::slot_loadCustomDepotSig, Qt::UniqueConnection);

        connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::addToMaterialDepot,
            this, &mCustomMaterialWindow::slot_addToMaterialDepot, Qt::UniqueConnection);

        connect(ui->treeWidget_Materials->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &mCustomMaterialWindow::slot_MaterialSelected);

        connect(ui->lineEdit_Filter, &QLineEdit::textChanged, this, [=](const QString& text) {
            for (int i = 0; i < ui->treeWidget_Materials->topLevelItemCount(); ++i)
            {
                QTreeWidgetItem* topItem = ui->treeWidget_Materials->topLevelItem(i);
                // 分类文本带数量后缀，匹配用 UserRole 中的纯分类名，避免搜数字误命中分类
                bool topMatch = topItem->data(0, Qt::UserRole).toString().contains(text, Qt::CaseInsensitive);
                bool childVisible = false;
                for (int j = 0; j < topItem->childCount(); ++j)
                {
                    QTreeWidgetItem* child = topItem->child(j);
                    bool match = child->text(0).contains(text, Qt::CaseInsensitive);
                    child->setHidden(!match && !text.isEmpty());
                    if (match)
                        childVisible = true;
                }
                topItem->setHidden(!topMatch && !childVisible && !text.isEmpty());
                if (!text.isEmpty() && (topMatch || childVisible))
                    topItem->setExpanded(true);
            }
            });
        connect(ui->pushButton_Remove, &QPushButton::clicked, this, &mCustomMaterialWindow::slot_removeMaterial);
        connect(ui->pushButton_Insert, &QPushButton::clicked, this, &mCustomMaterialWindow::slot_insertMaterial);
    }

    void mCustomMaterialWindow::initMaterialTree()
    {
        ui->treeWidget_Materials->setStyleSheet(R"(
    QTreeWidget
	{
	font-size:12px;
    background: #FFFFFF;
	}
	QTreeWidget::item {

	}
	QTreeWidget::item:hover
	{
	color:#303313;	
	background-color: #F0F3F6;	
	}
	QTreeWidget::item:selected
	{
	color:#303313;	
	background-color: #D0E7FD;	
	}
	QTreeWidget::branch:hover
	{	
	height:30px;	/*item高度*/
	margin:0px;		/*item间距*/
	border-left:0px;
	border-top:0px;
	margin-left:0px;		/*item间距*/

	margin-right:0px;		/*item间距*/
	margin-top:0px;		/*item间距*/
	margin-bottom:0px;		/*item间距*/
	background-color: transparent;
	}
	QTreeWidget::branch:selected
	{	
	height:30px;	/*item高度*/
	margin:0px;		/*item间距*/
	border-left:0px;
	border-top:0px;
	margin-left:0px;		/*item间距*/

	margin-right:0px;		/*item间距*/
	margin-top:0px;		/*item间距*/
	margin-bottom:0px;		/*item间距*/
	background-color: #FFFFFF;	
	}
    QTreeWidget::branch:open:has-children:hover {
        image: url(:/mMainWidgets/Icon/expand.png);
    }
    QTreeWidget::branch:open:has-children:selected {
        image: url(:/mMainWidgets/Icon/expand.png);
    }
    QTreeWidget::branch:closed:has-children:hover {
        image: url(:/mMainWidgets/Icon/fold.png);
    }
  QTreeWidget::branch:closed:has-children:selected {
        image: url(:/mMainWidgets/Icon/fold.png);
    }
)");
        ui->treeWidget_Materials->setFrameShape(QFrame::NoFrame);
        ui->treeWidget_Materials->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->treeWidget_Materials->setSelectionMode(QAbstractItemView::SingleSelection);
        ui->treeWidget_Materials->setHeaderHidden(true);
        ui->treeWidget_Materials->setRootIsDecorated(true);
        ui->treeWidget_Materials->setUniformRowHeights(true);
        ui->treeWidget_Materials->setIndentation(20);
        ui->treeWidget_Materials->header()->setSectionResizeMode(0, QHeaderView::Stretch);

        const QStringList categories = {
            "线弹性", "塑性", "超弹性", "粘性", "热学", "其它材料", "声学材料"
        };
        for (const QString& cat : categories)
        {
            QTreeWidgetItem* top = new QTreeWidgetItem(ui->treeWidget_Materials);
            // UserRole 保存纯分类名：显示文本携带数量后缀（如"塑性(3)"）后仍可按分类匹配
            top->setData(0, Qt::UserRole, cat);
            top->setText(0, QStringLiteral("%1（0）").arg(cat));
            ui->treeWidget_Materials->expandItem(top);
        }


    }

    void mCustomMaterialWindow::initPropertyTables()
    {
        QWidget* wid = ui->scrollArea->widget();
        wid->setStyleSheet("background:white;");
        QVBoxLayout* vbox = qobject_cast<QVBoxLayout*>(wid->layout());
        if (vbox)
        {
            vbox->setContentsMargins(0, 0, 0, 0);
            vbox->setSpacing(0);
        }
    }

    void mCustomMaterialWindow::showMaterialReadOnly(QString matName)
    {
        if (!ui || !ui->scrollArea)
            return;

        clearMaterialReadOnly();

        if (matName.isEmpty())
            return;

        auto matData = getMaterialDataByFile(matName);
        if (!matData)
            return;

        ui->label_MatNameEdit->setText(matData->getMaterialName());
        ui->label_MatTypeEdit->setText(materialTypeToName(matData->getMaterialType()));

        QWidget* container = ui->scrollArea->widget();
        QVBoxLayout* containerLayout = qobject_cast<QVBoxLayout*>(container->layout());
        if (!containerLayout)
            return;

        // 创建只读面板
        m_readOnlyPanel = new QWidget(container);
        m_readOnlyPanel->setObjectName("readOnlyMaterialPanel");
        QVBoxLayout* mainLayout = new QVBoxLayout(m_readOnlyPanel);
        mainLayout->setContentsMargins(12, 12, 12, 12);
        mainLayout->setSpacing(10);
        mainLayout->setAlignment(Qt::AlignTop);

        // 材料定义折叠块
        FoldSection* defFold = new FoldSection(QStringLiteral("材料定义"), m_readOnlyPanel);
        defFold->addRow(new QLabel(QStringLiteral("材料种类：")), new QLabel(materialTypeToCategory(static_cast<int>(matData->getMaterialType()))));
        defFold->addRow(new QLabel(QStringLiteral("材料类型：")), new QLabel(materialTypeToName(matData->getMaterialType())));
        mainLayout->addWidget(defFold);

        // 各材料属性折叠块：按 mMaterialPanel 的顺序展示
        auto props = matData->getAllProps();
        const MaterialType mainMatType = matData->getMaterialType();

        //旧版损伤演化独立大类迁移：并入剪切损伤大类(与mMaterialPanel::editMaterial一致)
        if ((mainMatType == Damage_Shear || mainMatType == Plastic_Johnson) && props.contains(MAT_DamageEvolution))
        {
            props[MAT_ShearDamage].unite(props.value(MAT_DamageEvolution));
            props.remove(MAT_DamageEvolution);
        }

        // 1. 主类型属性放最前
        // 2. 可选属性紧随其后（与 mMaterialPanel 的 allowprops 顺序一致）

        for (const MxFunctions::MaterialClass& matClass : m_PanelConfig[mainMatType])
        {
            // 只展示在编辑界面勾选过的属性段（可选属性段以 props 中是否保留该段为准）
            if (isOptionalPropClass(matClass) && !props.contains(matClass))
                continue;

            auto propFold = m_customProps->createPropByType(matClass, m_readOnlyPanel);
            if (!propFold)
                continue;
            propFold->setInputsValue(props[matClass]);
            mainLayout->addWidget(propFold);
            // 收集本属性页中的曲线数据，按属性类型建立映射（点击按钮时展示用）
            QHash<MxFunctions::MaterialPropType, Curves> tempCurveMap;
            for (auto pit = props[matClass].constBegin(); pit != props[matClass].constEnd(); ++pit)
            {
                if (!isCurveProperty(pit.key()))
                    continue;
                if (!isCurveVariant(pit.value()))
                    continue;
                Curves curves = pit.value().value<Curves>();
                if (curves.curves.isEmpty())
                    continue;
                tempCurveMap.insert(pit.key(), curves);
            }
            // 只读展示：先禁用全部控件，再按勾选状态恢复曲线按钮
            disableAllInteractiveChildren(propFold);
            QList<QPushButton*> btns = propFold->findChildren<QPushButton*>();
            for (QPushButton* btn : btns)
            {
                // 断开编辑界面的表格编辑连接，避免只读视图打开编辑表格
                disconnect(btn, &QPushButton::clicked, nullptr, nullptr);

                QVariant propTypeVar = btn->property("curvePropType");
                if (!propTypeVar.isValid())
                    continue;

                MxFunctions::MaterialPropType propType = static_cast<MxFunctions::MaterialPropType>(propTypeVar.toInt());

                // 同一行的勾选框（温度相关等）被勾选时按钮才可点击，其余保持禁用。
                // 特例：常规塑性的应力-塑性应变曲线与勾选框无关——未勾选时也有2列数据，
                // 勾选后为3列（含温度列），按钮始终可点击
                QCheckBox* siblingCb = checkedSiblingCheckBox(btn);
                if (propType != MatProp_Curve_PlasticityProperties && !siblingCb)
                    continue;

                Curves curves;
                auto it = tempCurveMap.find(propType);
                if (it != tempCurveMap.end())
                {
                    curves = it.value();
                }
                else if (siblingCb)
                {
                    // 曲线数据不在 props 中时，回退读取勾选框缓存的曲线
                    curves.correlation = CurveType::TemperatureCorrelation;
                    curves.curves = siblingCb->property("curve").value<QVector<QVector<double>>>();
                }

                btn->setEnabled(true);
                QStringList curveHeader = defaultCurveHeaders(propType, curves, props[matClass]);
                connect(btn, &QPushButton::clicked, this, [this, curves, curveHeader]() {
                    if (!fillCurveTable(ui->tableWidget_Curve, curves, curveHeader))
                        return;
                    ui->tableWidget_Curve->show();
                    ui->widget_Curve->hide();
                    if (m_CustomSplitter->isRightPanelCollapsed())
                        m_CustomSplitter->toggleRightPanel();
                    });
            }


        }



        mainLayout->addStretch(1);
        containerLayout->addWidget(m_readOnlyPanel, 1, Qt::AlignTop);

        // 收集温度相关曲线并显示在 tableWidget_Curve 中
        bool hasTempCurve = false;
        for (auto it = props.constBegin(); it != props.constEnd(); ++it)
        {
            const auto& subProps = it.value();
            for (auto pit = subProps.constBegin(); pit != subProps.constEnd(); ++pit)
            {
                if (!isCurveProperty(pit.key()))
                    continue;
                if (!isCurveVariant(pit.value()))
                    continue;
                Curves curves = pit.value().value<Curves>();
                QStringList curveHeader = defaultCurveHeaders(pit.key(), curves, subProps);
                if (fillCurveTable(ui->tableWidget_Curve, curves, curveHeader))
                    hasTempCurve = true;
            }
        }

        /*if (hasTempCurve)
        {
            ui->tableWidget_Curve->show();
            ui->widget_Curve->hide();
        }
        else
        {
            ui->tableWidget_Curve->hide();
            ui->widget_Curve->hide();
        }*/
    }

    void mCustomMaterialWindow::deleteMaterialsAndIndexLines(const QSet<QString>& reNames)
    {
        QString appDir = QCoreApplication::applicationDirPath();
        QString materialDir = appDir + "/CustomMaterials/";

        // --- 第一步：删除硬盘上的物理文件 ---
        for (const QString& name : reNames) {
            QString filePath = materialDir + name + ".mat";
            if (QFile::exists(filePath)) {
                if (QFile::remove(filePath)) {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString("材料删除成功。"));
                }
                else {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString("材料删除失败(权限/占用)。"));
                }
            }
            else {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("该材料文件不存在。"));
            }
        }

        // --- 第二步：清理索引文件中的对应行（过滤掉第一列匹配的行） ---
        QString indexFilePath = materialDir + "CustomMaterials.txt";
        QFile file(indexFilePath);
        if (!file.exists()) {
            mGlobalSignals::getInstance()->outputMessageSig(2, QString("索引文件不存在。"));
            return;
        }

        // 读取所有行
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            mGlobalSignals::getInstance()->outputMessageSig(2, QString("无法读取索引文件。"));
            return;
        }
        QStringList allLines;
        QTextStream in(&file);
        in.setCodec("UTF-8");
        while (!in.atEnd()) {
            allLines.append(in.readLine());
        }
        file.close();

        // 过滤：只保留第一列不在 reNames 中的行
        QStringList keptLines;
        for (const QString& line : allLines) {
            // 关键解析：取第一个 '|' 之前的内容，并去除首尾空格
            QString firstCol = line.section('|', 0, 0).trimmed();

            // 如果这一行的材料名不在删除名单里，就保留
            if (!reNames.contains(firstCol)) {
                keptLines.append(line);
            }

        }
        if (keptLines.isEmpty() && !file.exists()) {
            return;
        }
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            return;
        }
        QTextStream out(&file);
        out.setCodec("UTF-8");
        QStringList finalLines;
        for (const QString& line : keptLines) {
            if (!line.isEmpty()) {
                out << line << "\n";
            }
        }
        file.close();
    }

    void mCustomMaterialWindow::slot_removeMaterial()
    {
        QList<QTreeWidgetItem*> selecteditems = ui->treeWidget_Materials->selectedItems();
        QSet<QString> reNames;
        if (selecteditems.size() < 1)
        {
            return;
        }
        int parentcnt = 0;
        for (auto item : selecteditems)
        {
            QTreeWidgetItem* parent = item->parent();
            if (!parent)
            {
                ++parentcnt;
            }
        }
        if (parentcnt == selecteditems.size())
        {
            return;
        }
        QMessageBox box(QMessageBox::Question,
            tr("删除确认"),
            tr("确定要删除所选材料吗？"),
            QMessageBox::NoButton, this);

        QPushButton* yesBtn = box.addButton(tr("是"), QMessageBox::AcceptRole);
        QPushButton* noBtn = box.addButton(tr("否"), QMessageBox::RejectRole);
        box.setDefaultButton(noBtn);

        box.exec();
        if (box.clickedButton() != yesBtn)
            return;

        for (auto item : selecteditems)
        {
            QTreeWidgetItem* parent = item->parent();
            if (!parent)
            {
                continue;
            }
            QString name = item->data(0, Qt::UserRole).toString();
            delete item;
            item = nullptr;
            reNames.insert(name);
        }
        deleteMaterialsAndIndexLines(reNames);
        // 直接删除了树节点，同步刷新分类计数
        refreshCategoryCounts();


    }

    void mCustomMaterialWindow::slot_insertMaterial()
    {
        QList<QTreeWidgetItem*> selecteditems = ui->treeWidget_Materials->selectedItems();
        if (selecteditems.size() != 1)
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, QString("一次只能插入一个材料。"));
            return;
        }
        int parentcnt = 0;
        auto item = selecteditems.first();
        QString name = item->data(0, Qt::UserRole).toString();
        shared_ptr<MechMaterialData::mMaterialBase> mat = getMaterialDataByFile(name);
        int matID = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).createID();
        int nColor = PartColorLoop.size();
        QVector4D vector4d = PartColorLoop[(matID - 1) % nColor];
        QColor color = QColor(vector4d.x() * 255, vector4d.y() * 255, vector4d.z() * 255, vector4d.w() * 255);
        mat->setMaterialId(matID);
        mat->setMatColor(color);
        int id = 0;
        while (getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>
            (Physical_Material).ExistMaterialName(name))
        {
            ++id;
            name = name + "_" + MxFunctions::mNumberToString(id);
        }
        mat->setMaterialName(name);
        getCVM->getModelData()->addPhysicalFieldP(mat);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMaterialSig(true, mat->getMaterialId());
    }

    void mCustomMaterialWindow::slot_addToMaterialDepot(QVector<int> matIds)
    {
        if (matIds.size() == 0) return;
        auto materialTypeToCategory = [](int matType) -> QString {
            using namespace MxFunctions;
            switch (static_cast<MaterialType>(matType))
            {
            case LinearElastic_Isotropic:
            case LinearElastic_Orthotropic:
            case LinearElastic_Anisotropic:
                //case MAT_IsotropicThermalMaterial:
                return "线弹性";
                //case MAT_IsotropicHardening:
            case Plastic_Damage:
            case Plastic_Normal:
            case Plastic_Cyclic:
            case Plastic_Johnson:
                return "塑性";
            case HyperElastic_MooneyRivlin:
            case HyperElastic_NeoHookean:
            case HyperElastic_Ogden:
            case HyperElastic_Yeoh:
            case HyperElastic_ArrudaBoyce:
            case HyperElastic_Polynomial:
                //case MAT_Polynomial:
                //case MAT_Rpolynomial:
                //case MAT_Hyperhorm:
                return "超弹性";
            case Viscous_Viscoelastic:
            case Viscous_Viscoplastic:
                return "粘性";
            case Thermal_Base:
                return "热学";
            case AcousticMaterial_Base:
                return "声学材料";
            case OtherMaterial_Shim:
            case OtherMaterial_Brittle:
            case OtherMaterial_Foam:
            case OtherMaterial_Break:
                //case MAT_HardBrittle:
                //case MAT_Shim:
                //case MAT_Break:
                //case MAT_BreakParam:
                //case MAT_PhaseFieldParam:
            default:
                return "其它材料";
            }
        };

        QString appDir = QCoreApplication::applicationDirPath();
        QString customDir = appDir + "/CustomMaterials";
        QDir dir(customDir);
        if (!dir.exists())
        {
            if (!dir.mkpath("."))
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, "自定义材料目录创建失败。");
                return;
            }
        }

        QString summaryPath = customDir + "/CustomMaterials.txt";
        QSet<QString> existingNames;
        {
            QFile summaryFile(summaryPath);
            if (summaryFile.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                QTextStream ts(&summaryFile);
                ts.setCodec("UTF-8");
                while (!ts.atEnd())
                {
                    QString line = ts.readLine().trimmed();
                    if (line.isEmpty())
                        continue;
                    QString name = line.split("|").value(0).trimmed();
                    if (!name.isEmpty())
                        existingNames.insert(name);
                }
                summaryFile.close();
            }
        }

        for (auto& id : matIds)
        {
            std::shared_ptr<MechMaterialData::mMaterialBase> mat =
                getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).getMaterialData(id);
            if (mat == nullptr)
            {
                continue;
            }

            QString matName = mat->getMaterialName();
            QString filePath = customDir + "/" + matName + ".mat";
            if (QFile::exists(filePath) || existingNames.contains(matName))
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("自定义材料库中已存在名为 \"%1\" 的材料，请重命名后重新添加。").arg(matName));
                continue;
            }


            mIOFileManager::getInstance()->writeCustomMaterialFile(filePath, mat);

            QString category = materialTypeToCategory(static_cast<int>(mat->getMaterialType()));
            QFile summaryFile(summaryPath);
            if (summaryFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
            {
                QTextStream ts(&summaryFile);
                ts.setCodec("UTF-8");
                ts << matName << " | " << category << "|\n";
                summaryFile.close();
                existingNames.insert(matName);
            }
            else
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("更新 CustomMaterials.txt 失败。"));
            }
        }
        emit MechCommon::mMechGlobalSignals::getInstance()->loadCustomDepotSig();
    }

    void mCustomMaterialWindow::clearMaterialReadOnly()
    {
        // 先释放本次展示临时创建的可复用属性页实例
        for (auto* prop : m_readOnlyProps)
        {
            delete prop;
        }
        m_readOnlyProps.clear();

        if (m_readOnlyPanel)
        {
            QWidget* container = ui->scrollArea->widget();
            QVBoxLayout* containerLayout = qobject_cast<QVBoxLayout*>(container->layout());
            if (containerLayout)
                containerLayout->removeWidget(m_readOnlyPanel);
            delete m_readOnlyPanel;
            m_readOnlyPanel = nullptr;
        }

        ui->tableWidget_Curve->hide();
        ui->widget_Curve->hide();
        ui->tableWidget_Curve->clear();
        ui->tableWidget_Curve->setRowCount(0);
        ui->tableWidget_Curve->setColumnCount(0);

        ui->label_MatNameEdit->setText("--");
        ui->label_MatTypeEdit->setText("--");
    }

    void mCustomMaterialWindow::slot_MaterialSelected(const QItemSelection& selected, const QItemSelection& deselected)
    {
        Q_UNUSED(deselected)
            ui->label_MatName->show();
        ui->label_MatNameEdit->show();
        ui->label_MatType->show();
        ui->label_MatTypeEdit->show();
        if (!(m_CustomSplitter->isRightPanelCollapsed()))
            m_CustomSplitter->toggleRightPanel();

        clearMaterialReadOnly();

        QList<QTreeWidgetItem*> selecteditems = ui->treeWidget_Materials->selectedItems();
        ui->pushButton_Insert->setEnabled(selecteditems.size() == 1 && selecteditems.first()->parent());
        ui->pushButton_Remove->setEnabled(selecteditems.size() > 0 && selecteditems.first()->parent());

        if (selecteditems.size() != 1)
            return;

        QTreeWidgetItem* item = selecteditems.first();
        if (!item->parent())
        {
            if (ui->treeWidget_Materials->isItemExpanded(item))
                ui->treeWidget_Materials->collapseItem(item);
            else
                ui->treeWidget_Materials->expandItem(item);
            return;
        }

        QString matName = item->data(0, Qt::UserRole).toString();
        showMaterialReadOnly(matName);
    }

    bool mCustomMaterialWindow::eventFilter(QObject* obj, QEvent* ev)
    {
        QWidget* editor = qobject_cast<QWidget*>(obj);
        if (!editor) return false;

        if (ev->type() == QEvent::KeyPress)
        {
            auto* ke = static_cast<QKeyEvent*>(ev);
            if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter)
            {
                QMetaObject::invokeMethod(this, [this, editor]() {
                    ui->treeWidget_Materials->closePersistentEditor(
                        ui->treeWidget_Materials->currentItem(),
                        ui->treeWidget_Materials->currentColumn());
                    }, Qt::QueuedConnection);
                return true;
            }
        }
        else if (ev->type() == QEvent::FocusOut)
        {
            QMetaObject::invokeMethod(this, [this, editor]() {
                ui->treeWidget_Materials->closePersistentEditor(
                    ui->treeWidget_Materials->currentItem(),
                    ui->treeWidget_Materials->currentColumn());
                }, Qt::QueuedConnection);
        }
        return QObject::eventFilter(obj, ev);
    }

    QString mCustomMaterialWindow::materialTypeToCategory(int matType) const
    {
        const MaterialType type = static_cast<MaterialType>(matType);
        switch (static_cast<MaterialType>(matType))
        {
        case LinearElastic_Isotropic:
        case LinearElastic_Orthotropic:
        case LinearElastic_Anisotropic:
            //case MAT_IsotropicThermalMaterial:
            return "线弹性";
            //case MAT_IsotropicHardening:
        case Plastic_Damage:
        case Plastic_Normal:
        case Plastic_Cyclic:
        case Plastic_Johnson:
            return "塑性";
        case HyperElastic_MooneyRivlin:
        case HyperElastic_NeoHookean:
        case HyperElastic_Ogden:
        case HyperElastic_Yeoh:
        case HyperElastic_ArrudaBoyce:
        case HyperElastic_Polynomial:
            //case MAT_Polynomial:
            //case MAT_Rpolynomial:
            //case MAT_Hyperhorm:
            return "超弹性";
        case Viscous_Viscoelastic:
        case Viscous_Viscoplastic:
            return "粘性";
        case Thermal_Base:
            return "热学";
        case AcousticMaterial_Base:
            return "声学材料";
        case OtherMaterial_Shim:
        case OtherMaterial_Brittle:
        case OtherMaterial_Foam:
        case OtherMaterial_Break:
            //case MAT_HardBrittle:
            //case MAT_Shim:
            //case MAT_Break:
            //case MAT_BreakParam:
            //case MAT_PhaseFieldParam:
        default:
            return "其它材料";
        }
    }

    std::shared_ptr<mMaterialBase> mCustomMaterialWindow::getMaterialDataByFile(const QString& matName)
    {
        std::shared_ptr<mMaterialBase> mat;
        MechIOFile::mIOFileManager::getInstance()->readCustomMaterialFile(matName, mat);
        return mat;
    }

    void mCustomMaterialWindow::addMaterialFromModel(int srcMatId, int matType, const QString& name)
    {

        /*QString category = materialTypeToCategory(matType);
        if (category.isEmpty())
            return;

        QTreeWidgetItem* parentItem = nullptr;
        for (int i = 0; i < ui->treeWidget_Materials->topLevelItemCount(); ++i)
        {
            QTreeWidgetItem* top = ui->treeWidget_Materials->topLevelItem(i);
            if (top->text(0) == category)
            {
                parentItem = top;
                break;
            }
        }
        if (!parentItem)
            return;

        if (!getCVM || !getCVM->getModelData())
            return;

        auto& manager = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material);
        auto srcMat = manager.getMaterialData(srcMatId);
        if (!srcMat)
            return;

        int newMatId = manager.createID();
        QString matName = name;
        int matcnt = 1;
        while (manager.ExistMaterialName(matName))
        {
            matName = name + "_" + MxFunctions::mNumberToString(matcnt++);
        }

        auto matData = srcMat->copyMaterial(newMatId, matName);
        if (!matData)
            return;

        getCVM->getModelData()->addPhysicalFieldP(matData);

        QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
        item->setText(0, matName);
        item->setData(0, Qt::UserRole, newMatId);

        ui->treeWidget_Materials->setCurrentItem(item);
        ui->treeWidget_Materials->expandItem(parentItem);
        showMaterialReadOnly(newMatId);*/
    }

    void mCustomMaterialWindow::slot_loadCustomDepotSig()
    {
        QString appDir = QCoreApplication::applicationDirPath();
        QString customDir = appDir + "/CustomMaterials";
        QDir dir(customDir);
        if (!dir.exists())
        {
            if (!dir.mkpath("."))
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, "自定义材料目录不存在。");
                return;
            }
        }
        QString summaryPath = customDir + "/CustomMaterials.txt";
        QFile file(summaryPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return;
        }
        static QRegularExpression sep(R"(\s*\|\s*)");
        QTextStream ts(&file);
        ts.setCodec("UTF-8");
        for (int i = 0; i < ui->treeWidget_Materials->topLevelItemCount(); ++i)
        {
            QTreeWidgetItem* top = ui->treeWidget_Materials->topLevelItem(i);
            qDeleteAll(top->takeChildren());
        }
        while (!ts.atEnd())
        {
            QString line = ts.readLine().trimmed();
            QStringList parts = line.split(sep);

            QString name = parts[0].trimmed();
            QString category = parts[1];
            if (category.isEmpty())
                continue;

            QTreeWidgetItem* parentItem = nullptr;
            for (int i = 0; i < ui->treeWidget_Materials->topLevelItemCount(); ++i)
            {
                QTreeWidgetItem* top = ui->treeWidget_Materials->topLevelItem(i);
                if (top->data(0, Qt::UserRole).toString() == category)
                {
                    parentItem = top;
                    break;
                }
            }
            if (!parentItem)
                continue;
            QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
            item->setText(0, name);
            item->setData(0, Qt::UserRole, name);
        }

        refreshCategoryCounts();
    }

    void mCustomMaterialWindow::refreshCategoryCounts()
    {
        for (int i = 0; i < ui->treeWidget_Materials->topLevelItemCount(); ++i)
        {
            QTreeWidgetItem* top = ui->treeWidget_Materials->topLevelItem(i);
            QString category = top->data(0, Qt::UserRole).toString();
            if (!category.isEmpty())
                top->setText(0, QStringLiteral("%1（%2）").arg(category).arg(top->childCount()));
        }
    }
}
