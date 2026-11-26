#include "OriginalEngineExtensions.h"
#include "compat/model/LegacyMotionContext.h"
#include "NativeMotionTypedPanels.h"
#include "mFoldSection.h"
#include "mCalcLineEdit.h"
#include "mModelData.h"
#include "mMotionJointBase.h"
#include "mMotionJointManager.h"
#include "mMotionSpringBase.h"
#include "mMotionSpringManager.h"
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QDialog>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QPointer>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSignalBlocker>
#include <algorithm>
#include <cmath>

using MechMainWindow::FoldSection;
namespace MotionOcc {
namespace {
FoldSection* section(QWidget* panel,const QString& title) {
    auto* fold = new FoldSection(title, panel);
    if (auto* layout = qobject_cast<QVBoxLayout*>(panel->layout())) {
        layout->insertWidget(std::max(0, layout->count() - 1), fold);
        fold->refreshLayout();
    }
    return fold;
}
MxFunctions::mCalcLineEdit* number(FoldSection* fold,LegacyMotionContext* context,const QString& collection,int id,
                                const QString& key,const QString& label,double fallback,bool positive=false) {
    const double value=context->document().entity(collection,id).value(key).toDouble(fallback);
    auto* edit=new MxFunctions::mCalcLineEdit(QString::number(value,'g',17),positive ? MxFunctions::mCalcLineEdit::CalcMode::NoEmptyAndPositive : MxFunctions::mCalcLineEdit::CalcMode::Standard,fold);
    edit->setObjectName("engine_"+key);fold->addRow(label,edit);
    QObject::connect(edit,&QLineEdit::editingFinished,fold,[=] {
        if(edit->isValid())context->setExtensionFields(collection,id,{{key,edit->value()}});
    });return edit;
}
QComboBox* choice(FoldSection* fold,LegacyMotionContext* context,const QString& collection,int id,const QString& key,
                 const QString& title,const std::initializer_list<std::pair<const char*,const char*>>& items,const QString& fallback) {
    auto* combo=new QComboBox(fold);combo->setObjectName("engine_"+key);
    for(const auto& item:items)combo->addItem(QString::fromUtf8(item.first),QLatin1String(item.second));
    combo->setCurrentIndex(std::max(0,combo->findData(context->document().entity(collection,id).value(key).toString(fallback))));
    fold->addRow(title,combo);
    QObject::connect(combo,qOverload<int>(&QComboBox::currentIndexChanged),fold,[=] { context->setExtensionFields(collection,id,{{key,combo->currentData().toString()}}); });return combo;
}
void editInterfaceGroups(QWidget* parent,LegacyMotionContext* context,int id) {
    const auto flex=context->document().entity("autoFlexBodies",id);const int body=flex.value("targetBodyId").toInt(-1);
    QDialog dialog(parent);dialog.setWindowTitle(QStringLiteral("独立接口的运动副分组"));dialog.resize(480,360);
    auto* layout=new QVBoxLayout(&dialog);auto* info=new QLabel(QStringLiteral("为连接此柔性体的运动副指定接口组。同组运动副共用一个接口；不同组各自参与变形。"));info->setWordWrap(true);layout->addWidget(info);
    auto* table=new QTableWidget(0,2,&dialog);table->setHorizontalHeaderLabels({QStringLiteral("运动副"),QStringLiteral("接口组")});table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);layout->addWidget(table);
    QMap<int,int> groups;const auto existing=flex.value("interfaceJointGroups").toArray();
    for(int i=0;i<existing.size();++i)for(const auto& value:existing[i].toArray())groups[value.toInt()]=i+1;
    for(const auto& value:context->document().collection("joints")) {
        const auto joint=value.toObject();if(joint.value("actionBodyId").toInt(-1)!=body && joint.value("baseBodyId").toInt(-1)!=body)continue;
        const int row=table->rowCount(),jointId=joint.value("id").toInt();table->insertRow(row);
        auto* name=new QTableWidgetItem(joint.value("name").toString());name->setData(Qt::UserRole,jointId);name->setFlags(name->flags()&~Qt::ItemIsEditable);table->setItem(row,0,name);
        table->setItem(row,1,new QTableWidgetItem(QString::number(groups.value(jointId,row+1))));
    }
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);layout->addWidget(buttons);
    QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,[&] {
        QMap<int,QJsonArray> values;
        for(int row=0;row<table->rowCount();++row) {
            bool ok=false;const int group=table->item(row,1)->text().toInt(&ok);
            if(!ok || group<=0) { QMessageBox::warning(&dialog,QStringLiteral("接口组"),QStringLiteral("接口组需要正整数。"));return; }
            values[group].append(table->item(row,0)->data(Qt::UserRole).toInt());
        }
        QJsonArray result;for(const auto& ids:values)result.append(ids);
        context->setExtensionFields("autoFlexBodies",id,{{"interfaceJointGroups",result}});dialog.accept();
    });dialog.exec();
}
void editSpringLookupTable(QWidget* parent,LegacyMotionContext* context,int id) {
    context->flush();
    const auto row=context->document().entity("springs",id);
    const bool torsion=row.value("springType").toString()==QLatin1String("Torsional");
    const auto xArr=row.value("tableX").toArray(),yArr=row.value("tableY").toArray();

    QDialog dialog(parent);
    dialog.setObjectName(QStringLiteral("springLookupTableDialog"));
    dialog.setWindowTitle(QStringLiteral("编辑弹簧非线性刚度查表"));
    dialog.resize(460,380);
    auto* layout=new QVBoxLayout(&dialog);
    auto* info=new QLabel(QStringLiteral("配置至少 2 组严格递增的变形量 x = L - L₀ 与作用力 F 采样点（同引擎 F = -k·x 符号约定：压缩推开 > 0、拉伸拉回 < 0，正刚度恢复弹簧随 x 递增 F 递减）。"));
    info->setWordWrap(true);
    layout->addWidget(info);

    const int nRows=std::max(xArr.size(),yArr.size());
    auto* table=new QTableWidget(nRows,2,&dialog);
    table->setObjectName(QStringLiteral("springLookupTableWidget"));
    table->setHorizontalHeaderLabels({
        torsion ? QStringLiteral("扭转变形角 x (°)") : QStringLiteral("伸缩变形量 x (mm)"),
        torsion ? QStringLiteral("弹簧力矩 T (N·mm)") : QStringLiteral("弹簧力 F (N)")
    });
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    for(int r=0;r<nRows;++r) {
        table->setItem(r,0,new QTableWidgetItem(r<xArr.size()?QString::number(xArr[r].toDouble(),'g',15):QStringLiteral("0")));
        table->setItem(r,1,new QTableWidgetItem(r<yArr.size()?QString::number(yArr[r].toDouble(),'g',15):QStringLiteral("0")));
    }
    layout->addWidget(table,1);

    auto* rowBtns=new QHBoxLayout;
    auto* addBtn=new QPushButton(QStringLiteral("添加行"),&dialog);
    auto* delBtn=new QPushButton(QStringLiteral("删除选中行"),&dialog);
    rowBtns->addWidget(addBtn);
    rowBtns->addWidget(delBtn);
    rowBtns->addStretch();
    layout->addLayout(rowBtns);

    QObject::connect(addBtn,&QPushButton::clicked,&dialog,[table] {
        const int r=table->rowCount();
        const double nextX=r>0 && table->item(r-1,0) ? table->item(r-1,0)->text().toDouble()+10.0 : 0.0;
        table->insertRow(r);
        table->setItem(r,0,new QTableWidgetItem(QString::number(nextX,'g',12)));
        table->setItem(r,1,new QTableWidgetItem(QStringLiteral("0")));
    });
    QObject::connect(delBtn,&QPushButton::clicked,&dialog,[table] {
        auto sel=table->selectionModel()->selectedRows();
        std::sort(sel.begin(),sel.end(),[](const QModelIndex& a,const QModelIndex& b){ return a.row()>b.row(); });
        for(const auto& idx:sel) table->removeRow(idx.row());
    });

    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);
    layout->addWidget(buttons);
    QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,[&] {
        QVector<double> tx,ty;
        if(table->rowCount()==1) {
            QMessageBox::warning(&dialog,QStringLiteral("非线性刚度查表"),QStringLiteral("查表至少需要 2 个采样点，或清空全部行恢复线性刚度。"));
            return;
        }
        for(int r=0;r<table->rowCount();++r) {
            bool okX=false,okY=false;
            const double vx=table->item(r,0)?table->item(r,0)->text().toDouble(&okX):NAN;
            const double vy=table->item(r,1)?table->item(r,1)->text().toDouble(&okY):NAN;
            if(!okX || !okY || !std::isfinite(vx) || !std::isfinite(vy)) {
                QMessageBox::warning(&dialog,QStringLiteral("非线性刚度查表"),QStringLiteral("第 %1 行包含无效数值。").arg(r+1));
                return;
            }
            if(r>0 && vx<=tx.back()) {
                QMessageBox::warning(&dialog,QStringLiteral("非线性刚度查表"),QStringLiteral("变形量列必须严格递增（第 %1 行）。").arg(r+1));
                return;
            }
            tx.append(vx);
            ty.append(vy);
        }
        if(auto* spring=context->model()->getManager<MechMotionData::mMotionSpringManager>(MxFunctions::Physical_MotionSpring).getData(id)) {
            spring->setTable(tx,ty);
            context->flush();
        }
        dialog.accept();
    });
    dialog.exec();
}
std::array<QDoubleSpinBox*,3> makeVec3Row(QFormLayout* form,QWidget* parent,const QString& label,const QString& prefix,const QJsonObject& vecObj,double fallbackVal=0.0,double minVal=-1e9) {
    auto* wrap=new QWidget(parent);
    auto* h=new QHBoxLayout(wrap);
    h->setContentsMargins(0,0,0,0);
    h->setSpacing(4);
    std::array<QDoubleSpinBox*,3> out{};
    const char* keys[3]={"x","y","z"};
    for(int i=0;i<3;++i) {
        auto* sp=new QDoubleSpinBox(wrap);
        sp->setObjectName(prefix+QLatin1Char(static_cast<char>('X'+i)));
        sp->setDecimals(4);
        sp->setRange(minVal,1e9);
        sp->setKeyboardTracking(false);
        sp->setValue(vecObj.value(QLatin1String(keys[i])).toDouble(fallbackVal));
        h->addWidget(sp);
        out[i]=sp;
    }
    form->addRow(label,wrap);
    return out;
}
}
void appendOriginalEngineExtensions(QWidget* panel,LegacyMotionContext* context,const QString& collection,int id) {
    if(id<0)return;
    if(collection=="scenarios") {
        auto* fold=section(panel,QStringLiteral("静力平衡与预载"));
        auto* mode=choice(fold,context,collection,id,"analysisType",QStringLiteral("分析类型"),{{"动力学","Dynamic"},{"静力平衡","Static"},{"预载后动力学","PreloadedDynamics"}},"Dynamic");
        QList<QWidget*> fields;
        fields<<number(fold,context,collection,id,"staticMaxIterations",QStringLiteral("最大迭代数"),100,true);
        fields<<number(fold,context,collection,id,"staticTolerance",QStringLiteral("平衡残差容差"),1e-8,true);
        fields<<number(fold,context,collection,id,"staticConstraintTolerance",QStringLiteral("约束残差容差"),1e-7,true);
        fields<<number(fold,context,collection,id,"staticStepDamping",QStringLiteral("步长系数"),1,true);
        auto* help=new QLabel(QStringLiteral("预载后动力学先建立平衡构型，再从该构型开始积分；刚度和载荷仍由原弹簧、力元及驱动面板设置。"));help->setWordWrap(true);fold->addRow(help);
        const auto update=[mode,fields] { for(auto* field:fields)field->setEnabled(mode->currentData().toString()!="Dynamic"); };
        QObject::connect(mode,qOverload<int>(&QComboBox::currentIndexChanged),panel,update);update();

        auto* projFold=section(panel,QStringLiteral("位置投影与柔性体积分"));
        const auto scEntity=context->document().entity(collection,id);
        auto* projCheck=new QCheckBox(QStringLiteral("每步执行约束位置投影校正"),projFold);
        projCheck->setObjectName(QStringLiteral("engine_positionAssemblyPerStep"));
        projCheck->setChecked(scEntity.value("positionAssemblyPerStep").toBool(true));
        projFold->addRow(projCheck);
        auto* projIter=new QSpinBox(projFold);
        projIter->setObjectName(QStringLiteral("engine_positionAssemblyIterations"));
        projIter->setRange(1,50);
        projIter->setValue(std::clamp(scEntity.value("positionAssemblyIterations").toInt(50),1,50));
        projIter->setEnabled(projCheck->isChecked());
        projFold->addRow(QStringLiteral("位置投影最大迭代数:"),projIter);
        QObject::connect(projCheck,&QCheckBox::toggled,projFold,[=](bool checked) {
            projIter->setEnabled(checked);
            context->setExtensionFields(collection,id,{{"positionAssemblyPerStep",checked}});
        });
        QObject::connect(projIter,qOverload<int>(&QSpinBox::valueChanged),projFold,[=](int v) {
            context->setExtensionFields(collection,id,{{"positionAssemblyIterations",v}});
        });
        choice(projFold,context,collection,id,"mnfIntegratorType",QStringLiteral("MNF 柔性体积分器:"),
               {{"HHT 自适应积分","HHT"},{"线性化隐式欧拉 (闭环稳健)","EULER_IMPLICIT_LINEARIZED"}},"HHT");
        choice(projFold,context,collection,id,"independentPositionProjection",QStringLiteral("独立接口位置投影权重:"),
               {{"质量加权","MASS"},{"质量 + 刚度加权","MASS_STIFFNESS"}},"MASS");

        auto* hydFold=section(panel,QStringLiteral("机电液耦合初始化"));
        auto* steadyCheck=new QCheckBox(QStringLiteral("按初始静载平衡液压执行器腔压"),hydFold);
        steadyCheck->setObjectName(QStringLiteral("engine_hydraulicSteadyInit"));
        steadyCheck->setChecked(scEntity.value("hydraulicSteadyInit").toBool(false));
        hydFold->addRow(steadyCheck);
        auto* hydHelp=new QLabel(QStringLiteral("启用后在 t=0 自动反算平衡重力与外载所需的液压缸/液压马达 A、B 腔初始压力，并同步关联容腔初压。"),hydFold);
        hydHelp->setWordWrap(true);
        hydFold->addRow(hydHelp);
        QObject::connect(steadyCheck,&QCheckBox::toggled,hydFold,[=](bool checked) {
            context->setExtensionFields(collection,id,{{"hydraulicSteadyInit",checked}});
        });
    } else if(collection=="joints") {
        auto* fold=section(panel,QStringLiteral("黏性摩擦"));
        fold->setObjectName(QStringLiteral("engine_jointViscousFold"));
        const double initVisc=context->document().entity(collection,id).value("friction").toObject().value("viscousCoefficient").toDouble(0.0);
        auto* viscEdit=new MxFunctions::mCalcLineEdit(QString::number(initVisc,'g',15),MxFunctions::mCalcLineEdit::CalcMode::NonNegative,fold);
        viscEdit->setObjectName(QStringLiteral("engine_viscousCoefficient"));
        auto* unitLbl=new QLabel(fold);
        unitLbl->setWordWrap(true);
        fold->addRow(QStringLiteral("黏性摩擦系数:"),viscEdit);
        fold->addRow(unitLbl);
        QObject::connect(viscEdit,&QLineEdit::editingFinished,fold,[=] {
            if(!viscEdit->isValid())return;
            if(auto* j=context->model()->getManager<MechMotionData::mMotionJointManager>(MxFunctions::Physical_MotionJoint).getData(id)) {
                auto fdata=j->getFrictionData();
                fdata.viscousCoefficient=viscEdit->value();
                j->setFrictionData(fdata);
                context->flush();
            }
        });
        const auto syncJointVisc=[=] {
            const auto jObj=context->document().entity(collection,id);
            const QString jType=jObj.value("jointType").toString();
            const bool supported=jType==QLatin1String("Revolute")||jType==QLatin1String("Translational")||jType==QLatin1String("Cylindrical");
            fold->setVisible(supported);
            if(!supported)return;
            const auto fObj=jObj.value("friction").toObject();
            const bool fEnabled=fObj.value("enabled").toBool(false);
            viscEdit->setEnabled(fEnabled);
            const bool trans=jType==QLatin1String("Translational");
            viscEdit->setUnit(trans?QStringLiteral("N·s/mm"):QStringLiteral("N·mm·s/rad"));
            unitLbl->setText(fEnabled
                ? QStringLiteral("启用关节摩擦时生效，施加与相对速度成正比的黏性阻尼力/力矩（%1）。").arg(trans?QStringLiteral("N·s/mm"):QStringLiteral("N·mm·s/rad"))
                : QStringLiteral("请先在上方“摩擦”栏勾选“启用摩擦”，即可设置黏性摩擦系数。"));
            fold->refreshLayout();
        };
        QObject::connect(context,&LegacyMotionContext::documentChanged,panel,syncJointVisc);
        syncJointVisc();
    } else if(collection=="couplings") {
        auto* fold=section(panel,QStringLiteral("三维啮合与相位"));
        fold->setObjectName(QStringLiteral("engine_couplingMeshFold"));
        auto* alphaEdit=number(fold,context,collection,id,"pressureAngleDeg",QStringLiteral("压力角 α (°):"),0.0);
        auto* betaEdit=number(fold,context,collection,id,"helixAngleDeg",QStringLiteral("螺旋角 β (°):"),0.0);
        auto* phaseEdit=number(fold,context,collection,id,"phaseDeg",QStringLiteral("初始啮合相位 (°):"),0.0);
        auto* checkPhase=new QCheckBox(QStringLiteral("强制保持啮合相位（防跳齿漂移）"),fold);
        checkPhase->setObjectName(QStringLiteral("engine_checkPhase"));
        checkPhase->setChecked(context->document().entity(collection,id).value("checkPhase").toBool(false));
        fold->addRow(checkPhase);
        QObject::connect(checkPhase,&QCheckBox::toggled,fold,[=](bool checked) {
            context->setExtensionFields(collection,id,{{"checkPhase",checked}});
        });
        auto* meshHelp=new QLabel(QStringLiteral("非零压力角与螺旋角会在三维齿轮/齿条啮合点真实产生径向分力与轴向推力反力。"),fold);
        meshHelp->setWordWrap(true);
        fold->addRow(meshHelp);
        const auto syncCoupling=[=] {
            const QString cType=context->document().entity(collection,id).value("couplingType").toString();
            const bool gearLike=cType==QLatin1String("Gear")||cType==QLatin1String("GearRack");
            fold->setVisible(gearLike);
            alphaEdit->setEnabled(gearLike);
            betaEdit->setEnabled(gearLike);
            phaseEdit->setEnabled(gearLike);
            checkPhase->setEnabled(gearLike);
            if(gearLike)fold->refreshLayout();
        };
        QObject::connect(context,&LegacyMotionContext::documentChanged,panel,syncCoupling);
        syncCoupling();
    } else if(collection=="springs") {
        auto* fold=section(panel,QStringLiteral("非线性刚度查表"));
        fold->setObjectName(QStringLiteral("engine_springTableFold"));
        auto* summary=new QLabel(fold);
        summary->setObjectName(QStringLiteral("engine_springTableSummary"));
        summary->setWordWrap(true);
        fold->addRow(summary);
        auto* btnRow=new QWidget(fold);
        auto* btnLayout=new QHBoxLayout(btnRow);
        btnLayout->setContentsMargins(0,0,0,0);
        auto* editBtn=new QPushButton(QStringLiteral("编辑变形量—力查表…"),btnRow);
        editBtn->setObjectName(QStringLiteral("engine_editSpringTable"));
        auto* clearBtn=new QPushButton(QStringLiteral("清空查表"),btnRow);
        clearBtn->setObjectName(QStringLiteral("engine_clearSpringTable"));
        btnLayout->addWidget(editBtn);
        btnLayout->addWidget(clearBtn);
        fold->addRow(btnRow);

        const auto syncSpringTable=[=] {
            const auto sObj=context->document().entity(collection,id);
            const auto tx=sObj.value("tableX").toArray();
            const bool torsion=sObj.value("springType").toString()==QLatin1String("Torsional");
            clearBtn->setEnabled(!tx.isEmpty());
            summary->setText(tx.isEmpty()
                ? QStringLiteral("当前未配置查表，使用上方线性刚度或力函数表达式。")
                : QStringLiteral("已启用非线性查表：共 %1 组（%2）采样点，优先于常数刚度生效。")
                    .arg(tx.size()).arg(torsion?QStringLiteral("°, N·mm"):QStringLiteral("mm, N")));
            fold->refreshLayout();
        };
        QObject::connect(editBtn,&QPushButton::clicked,panel,[=] {
            editSpringLookupTable(panel,context,id);
            syncSpringTable();
        });
        QObject::connect(clearBtn,&QPushButton::clicked,panel,[=] {
            if(auto* spring=context->model()->getManager<MechMotionData::mMotionSpringManager>(MxFunctions::Physical_MotionSpring).getData(id)) {
                spring->setTable({},{});
                context->flush();
                syncSpringTable();
            }
        });
        QObject::connect(context,&LegacyMotionContext::documentChanged,panel,syncSpringTable);
        syncSpringTable();
    } else if(collection=="contacts") {
        auto* fold=section(panel,QStringLiteral("解析几何接触 (球 / 盒)"));
        fold->setObjectName(QStringLiteral("engine_analyticContactFold"));
        const auto cRow=context->document().entity(collection,id);
        const bool hasAnalytic=cRow.contains("analyticContact");
        const auto cfg=cRow.value("analyticContact").toObject();
        const auto actShape=cfg.value("actionShape").toObject();
        const auto basShape=cfg.value("baseShape").toObject();
        const auto actBodyCom=context->document().entity("bodies",cRow.value("actionBodyId").toInt(-1)).value("centerOfMass").toObject();
        const auto basBodyCom=context->document().entity("bodies",cRow.value("baseBodyId").toInt(-1)).value("centerOfMass").toObject();

        auto* enableChk=new QCheckBox(QStringLiteral("启用解析球/盒接触（高速精确 SMC 碰撞）"),fold);
        enableChk->setObjectName(QStringLiteral("engine_analyticContactEnabled"));
        enableChk->setChecked(hasAnalytic);
        fold->addRow(enableChk);

        auto* detail=new QWidget(fold);
        auto* form=new QFormLayout(detail);
        form->setContentsMargins(0,0,0,0);

        auto* actKind=new QComboBox(detail);
        actKind->setObjectName(QStringLiteral("engine_analyticActionKind"));
        actKind->addItem(QStringLiteral("球体 (Sphere)"),QStringLiteral("Sphere"));
        actKind->addItem(QStringLiteral("长方体 (Box)"),QStringLiteral("Box"));
        actKind->setCurrentIndex(std::max(0,actKind->findData(actShape.value("kind").toString("Sphere"))));
        form->addRow(QStringLiteral("动作体(1)形状:"),actKind);
        const auto actCenter=makeVec3Row(form,detail,QStringLiteral("动作体中心 (mm):"),QStringLiteral("engine_analyticActionCenter"),
                                         actShape.contains("center")?actShape.value("center").toObject():actBodyCom,0.0);
        auto* actRad=new QDoubleSpinBox(detail);
        actRad->setObjectName(QStringLiteral("engine_analyticActionRadius"));
        actRad->setDecimals(4);actRad->setRange(1e-3,1e8);actRad->setKeyboardTracking(false);
        actRad->setValue(actShape.value("radius").toDouble(25.0));
        auto* actRadLbl=new QLabel(QStringLiteral("动作体球半径 (mm):"),detail);
        form->addRow(actRadLbl,actRad);
        auto* actSizeLbl=new QLabel(QStringLiteral("动作体盒尺寸 (mm):"),detail);
        auto* actSizeWrap=new QWidget(detail);
        std::array<QDoubleSpinBox*,3> actSize{};
        {
            auto* h=new QHBoxLayout(actSizeWrap);h->setContentsMargins(0,0,0,0);h->setSpacing(4);
            const auto sz=actShape.value("size").toObject();
            const char* ks[3]={"x","y","z"};
            for(int i=0;i<3;++i) {
                auto* sp=new QDoubleSpinBox(actSizeWrap);
                sp->setObjectName(QStringLiteral("engine_analyticActionSize")+QLatin1Char(static_cast<char>('X'+i)));
                sp->setDecimals(4);sp->setRange(1e-3,1e8);sp->setKeyboardTracking(false);
                sp->setValue(sz.value(QLatin1String(ks[i])).toDouble(50.0));
                h->addWidget(sp);actSize[i]=sp;
            }
            form->addRow(actSizeLbl,actSizeWrap);
        }

        auto* basKind=new QComboBox(detail);
        basKind->setObjectName(QStringLiteral("engine_analyticBaseKind"));
        basKind->addItem(QStringLiteral("球体 (Sphere)"),QStringLiteral("Sphere"));
        basKind->addItem(QStringLiteral("长方体 (Box)"),QStringLiteral("Box"));
        basKind->setCurrentIndex(std::max(0,basKind->findData(basShape.value("kind").toString("Box"))));
        form->addRow(QStringLiteral("基本体(2)形状:"),basKind);
        const auto basCenter=makeVec3Row(form,detail,QStringLiteral("基本体中心 (mm):"),QStringLiteral("engine_analyticBaseCenter"),
                                         basShape.contains("center")?basShape.value("center").toObject():basBodyCom,0.0);
        auto* basRad=new QDoubleSpinBox(detail);
        basRad->setObjectName(QStringLiteral("engine_analyticBaseRadius"));
        basRad->setDecimals(4);basRad->setRange(1e-3,1e8);basRad->setKeyboardTracking(false);
        basRad->setValue(basShape.value("radius").toDouble(25.0));
        auto* basRadLbl=new QLabel(QStringLiteral("基本体球半径 (mm):"),detail);
        form->addRow(basRadLbl,basRad);
        auto* basSizeLbl=new QLabel(QStringLiteral("基本体盒尺寸 (mm):"),detail);
        auto* basSizeWrap=new QWidget(detail);
        std::array<QDoubleSpinBox*,3> basSize{};
        {
            auto* h=new QHBoxLayout(basSizeWrap);h->setContentsMargins(0,0,0,0);h->setSpacing(4);
            const auto sz=basShape.value("size").toObject();
            const char* ks[3]={"x","y","z"};
            for(int i=0;i<3;++i) {
                auto* sp=new QDoubleSpinBox(basSizeWrap);
                sp->setObjectName(QStringLiteral("engine_analyticBaseSize")+QLatin1Char(static_cast<char>('X'+i)));
                sp->setDecimals(4);sp->setRange(1e-3,1e8);sp->setKeyboardTracking(false);
                sp->setValue(sz.value(QLatin1String(ks[i])).toDouble(100.0));
                h->addWidget(sp);basSize[i]=sp;
            }
            form->addRow(basSizeLbl,basSizeWrap);
        }

        const auto makeParam=[&](const QString& name,const QString& title,double defVal) {
            auto* sp=new QDoubleSpinBox(detail);
            sp->setObjectName(QStringLiteral("engine_analytic_")+name);
            sp->setDecimals(4);sp->setRange(0.0,1e9);sp->setKeyboardTracking(false);
            sp->setValue(cfg.value(name).toDouble(defVal));
            form->addRow(title,sp);
            return sp;
        };
        auto* ktSpin=makeParam(QStringLiteral("tangentialStiffness"),QStringLiteral("切向接触刚度 (N/mm):"),50.0);
        auto* gnSpin=makeParam(QStringLiteral("normalDampingRate"),QStringLiteral("法向阻尼率 (1/s):"),20.0);
        auto* gtSpin=makeParam(QStringLiteral("tangentialDampingRate"),QStringLiteral("切向阻尼率 (1/s):"),20.0);
        auto* marginSpin=makeParam(QStringLiteral("collisionMargin"),QStringLiteral("碰撞包络裕度 (mm):"),1.0);
        fold->addRow(detail);

        const auto syncAnalyticVis=[=] {
            const bool on=enableChk->isChecked();
            detail->setVisible(on);
            const bool actSphere=actKind->currentData().toString()==QLatin1String("Sphere");
            actRadLbl->setVisible(actSphere);actRad->setVisible(actSphere);
            actSizeLbl->setVisible(!actSphere);actSizeWrap->setVisible(!actSphere);
            const bool basSphere=basKind->currentData().toString()==QLatin1String("Sphere");
            basRadLbl->setVisible(basSphere);basRad->setVisible(basSphere);
            basSizeLbl->setVisible(!basSphere);basSizeWrap->setVisible(!basSphere);
            fold->refreshLayout();
        };
        const auto commitAnalytic=[=] {
            syncAnalyticVis();
            if(!enableChk->isChecked()) {
                context->setExtensionFields(collection,id,{{"analyticContact",QJsonValue::Null}});
                return;
            }
            const auto buildShape=[](QComboBox* kind,const std::array<QDoubleSpinBox*,3>& ctr,QDoubleSpinBox* rad,const std::array<QDoubleSpinBox*,3>& sz) {
                QJsonObject s{{"kind",kind->currentData().toString()},
                              {"center",QJsonObject{{"x",ctr[0]->value()},{"y",ctr[1]->value()},{"z",ctr[2]->value()}}}};
                if(kind->currentData().toString()==QLatin1String("Sphere")) s.insert("radius",rad->value());
                else s.insert("size",QJsonObject{{"x",sz[0]->value()},{"y",sz[1]->value()},{"z",sz[2]->value()}});
                return s;
            };
            QJsonObject nextCfg{
                {"actionShape",buildShape(actKind,actCenter,actRad,actSize)},
                {"baseShape",buildShape(basKind,basCenter,basRad,basSize)},
                {"tangentialStiffness",ktSpin->value()},
                {"normalDampingRate",gnSpin->value()},
                {"tangentialDampingRate",gtSpin->value()},
                {"collisionMargin",marginSpin->value()}
            };
            context->setExtensionFields(collection,id,{{"analyticContact",nextCfg}});
        };
        QObject::connect(enableChk,&QCheckBox::toggled,fold,[=](bool){ commitAnalytic(); });
        QObject::connect(actKind,qOverload<int>(&QComboBox::currentIndexChanged),fold,[=](int){ commitAnalytic(); });
        QObject::connect(basKind,qOverload<int>(&QComboBox::currentIndexChanged),fold,[=](int){ commitAnalytic(); });
        for(auto* sp:{actCenter[0],actCenter[1],actCenter[2],actRad,actSize[0],actSize[1],actSize[2],
                      basCenter[0],basCenter[1],basCenter[2],basRad,basSize[0],basSize[1],basSize[2],
                      ktSpin,gnSpin,gtSpin,marginSpin}) {
            QObject::connect(sp,qOverload<double>(&QDoubleSpinBox::valueChanged),fold,[=](double){ commitAnalytic(); });
        }
        syncAnalyticVis();
    } else if(collection=="drives") {
        auto* fold=section(panel,QStringLiteral("执行器限制"));
        auto* effort=number(fold,context,collection,id,"effortLimit",QStringLiteral("额定力 / 力矩"),0);
        auto* power=number(fold,context,collection,id,"powerLimit",QStringLiteral("功率限制 (W)"),0);
        auto* speed=number(fold,context,collection,id,"noLoadSpeed",QStringLiteral("空载速度"),0);
        auto* units=new QLabel;units->setWordWrap(true);fold->addRow(units);
        const auto update=[=] {
            const auto drive=context->document().entity(collection,id);const auto mode=drive.value("controlMode").toString();const bool enabled=mode=="Force"||mode=="Torque";
            effort->setEnabled(enabled);power->setEnabled(enabled);speed->setEnabled(enabled);
            units->setText(enabled ? QStringLiteral("额定输出单位 %1，空载速度单位 %2；0 表示不限制。PID 连接此执行器时，输出为力或力矩。").arg(mode=="Force" ? "N" : "N·mm",mode=="Force" ? "mm/s" : "°/s") : QStringLiteral("在原控制模式中选择力或力矩后，可设置额定输出、功率和空载速度限制。"));
        };
        QObject::connect(context,&LegacyMotionContext::documentChanged,panel,update);update();
    } else if(collection=="autoFlexBodies") {
        auto* fold=section(panel,QStringLiteral("接口耦合与模态检查"));
        auto* mode=choice(fold,context,collection,id,"interfaceMode",QStringLiteral("接口方式"),{{"共用参考体","SharedBody"},{"独立接口","Independent"}},"SharedBody");
        auto* groups=new QPushButton(QStringLiteral("设置运动副接口分组…"));fold->addRow(groups);
        auto* inspect=new QPushButton(QStringLiteral("检查真实模态…"));inspect->setObjectName("nativeInspectModes");fold->addRow(inspect);
        auto* help=new QLabel;help->setWordWrap(true);fold->addRow(help);
        QObject::connect(groups,&QPushButton::clicked,panel,[=] { editInterfaceGroups(panel,context,id); });
        QObject::connect(inspect,&QPushButton::clicked,panel,[=] { context->flush();if(context->viewportModel()->inspectAutoFlex)context->viewportModel()->inspectAutoFlex(id); });
        const auto update=[=] {
            const bool independent=mode->currentData().toString()=="Independent";groups->setVisible(independent);
            help->setText(independent ? QStringLiteral("独立接口使用真实共回转体网格，各接口可相对变形。每片至少需要三个不共线节点；动力学保留全部节点自由度，模态数用于模态检查。") : QStringLiteral("共用参考体沿用原模态缩减流程。检查器显示真实特征频率与振型，振型预览幅值可单独调整。"));
        };
        QObject::connect(mode,qOverload<int>(&QComboBox::currentIndexChanged),panel,update);update();
    }
    if(QStringList{"scenarios","drives","sensors","controllers","autoFlexBodies"}.contains(collection)) {
        auto* fold=section(panel,QStringLiteral("配置检查"));auto* status=new QLabel;status->setWordWrap(true);fold->addRow(status);
        const auto update=[=] {
            const auto row=context->document().entity(collection,id);QString text=nativeTypedRecordIssue(row,collection,context->document());
            if(collection=="sensors" && text.isEmpty())text=QStringLiteral("测量单位：%1").arg(nativeSensorQuantityUnit(row,context->document()));
            if(collection=="autoFlexBodies" && row.value("interfaceMode").toString()=="Independent" && row.value("dampingType").toInt()==1)
                text=QStringLiteral("独立接口不能使用模态阻尼比，请在原阻尼设置中选择无阻尼或瑞利阻尼。");
            if(collection=="controllers" && text.isEmpty()) {
                const auto drive=context->document().entity("drives",row.value("targetDriveId").toInt(-1));const bool angular=drive.value("motionType").toString()=="Rotation";
                const QString feedback=row.value("feedbackSource").toString()=="Sensor" ? nativeSensorQuantityUnit(context->document().entity("sensors",row.value("feedbackSensorId").toInt(-1)),context->document())
                    : row.value("feedbackType").toString()=="Velocity" ? (angular ? "°/s" : "mm/s") : (angular ? "°" : "mm");
                text=QStringLiteral("目标与反馈：%1；控制输出：%2。").arg(feedback,nativeDriveCommandUnit(drive));
            }
            if (text.isEmpty()) text = QStringLiteral("配置参数通过当前检查。");
            if (status->text() != text) {
                status->setText(text);
                fold->refreshLayout();
            }
        };
        QObject::connect(context,&LegacyMotionContext::documentChanged,panel,update);update();
    }
}
}
