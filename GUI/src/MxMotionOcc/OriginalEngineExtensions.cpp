#include "OriginalEngineExtensions.h"
#include "compat/model/LegacyMotionContext.h"
#include "NativeMotionTypedPanels.h"
#include "mFoldSection.h"
#include "mCalcLineEdit.h"
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialog>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QPointer>
#include <cmath>

using MechMainWindow::FoldSection;
namespace MotionOcc {
namespace {
FoldSection* section(QWidget* panel,const QString& title) {
    auto* fold=new FoldSection(title,panel);auto* layout=qobject_cast<QVBoxLayout*>(panel->layout());
    if(layout)layout->insertWidget(std::max(0,layout->count()-1),fold);return fold;
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
            status->setText(text.isEmpty() ? QStringLiteral("配置参数通过当前检查。") : text);
        };
        QObject::connect(context,&LegacyMotionContext::documentChanged,panel,update);update();
    }
}
}
