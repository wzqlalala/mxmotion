#include "MotionWindow.h"
#include "RigidProject.h"
#include "NativeMotionLoadExport.h"
#include "NativeMotionPickWidgets.h"
#include "NativeMotionGeometry.h"
#include "OccViewport.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>
#include <cmath>

namespace MotionOcc {
void MotionWindow::editNativeLoadExport(int scenarioId)
{
    const auto found=m_results.find(scenarioId);
    if (found==m_results.end() || !found->second->input.nativeInput) return;
    const auto run=found->second;
    const auto& document=run->input.nativeInput->document;
    auto* panel=new QWidget; panel->setObjectName("nativeLoadExportPanel");
    auto* layout=new QVBoxLayout(panel);
    auto* heading=new QLabel(tr("多体结果与载荷工况导出")); heading->setStyleSheet("font-weight:600"); layout->addWidget(heading);
    auto* csv=new QPushButton(tr("导出全部求解结果 CSV")); csv->setObjectName("nativeExportAllResults"); layout->addWidget(csv);
    const auto destination=[this](QString name,const QString& filter,const QString& suffix) {
        auto path=QFileDialog::getSaveFileName(this,tr("导出载荷与工况"),name,filter);
        if (!path.isEmpty() && QFileInfo(path).suffix().isEmpty()) path+=suffix;
        return path;
    };
    connect(csv,&QPushButton::clicked,panel,[this,run,destination] {
        const auto path=destination(run->input.scenario.name+".csv",tr("CSV 表格 (*.csv)"),".csv"); if (path.isEmpty()) return;
        QString error; if (!exportRigidCsv(path,run->input,run->result,error)) setNotice(error,true); else setNotice(tr("全部结果已导出。"));
    });
    auto* manual=new QGroupBox(tr("手动提取载荷")); auto* form=new QFormLayout(manual); form->setRowWrapPolicy(QFormLayout::WrapLongRows); layout->addWidget(manual);
    auto* source=new QComboBox; source->setObjectName("nativeLoadSource");
    for (const auto& collection:{"joints","couplings","drives"}) for (const auto& value:document.collection(collection)) {
        const auto row=value.toObject(); const int id=row.value("id").toInt();
        source->addItem(QStringLiteral("%1 [%2 / %3]").arg(row.value("name").toString(),QString::fromLatin1(collection)).arg(id),QStringLiteral("%1:%2").arg(collection).arg(id));
    }
    form->addRow(tr("载荷来源"),source);
    auto* end=new QComboBox; end->setObjectName("nativeLoadEnd"); end->addItem(tr("作用于 I 端"),0); end->addItem(tr("作用于 J 端"),1); form->addRow(tr("受力端"),end);
    auto* mode=new QComboBox; mode->setObjectName("nativeLoadFrames"); mode->addItems({tr("最大载荷帧"),tr("指定帧"),tr("全部帧")}); form->addRow(tr("提取时刻"),mode);
    auto* frame=new QSpinBox; frame->setObjectName("nativeLoadFrame"); frame->setRange(0,int(run->result.frames.size())-1); form->addRow(tr("帧序号 (从 0 开始)"),frame);
    auto* time=new QLabel; form->addRow(tr("指定帧时间"),time);
    const auto updateTime=[run,frame,time] { time->setText(QStringLiteral("%1 s").arg(run->result.frames.at(frame->value()).seconds,0,'g',12)); };
    connect(frame,qOverload<int>(&QSpinBox::valueChanged),panel,[updateTime](int) { updateTime(); }); updateTime();
    auto* scale=new QDoubleSpinBox; scale->setObjectName("nativeLoadScale"); scale->setRange(-1e12,1e12); scale->setDecimals(6); scale->setValue(1); form->addRow(tr("载荷系数"),scale);
    auto* format=new QComboBox; format->setObjectName("nativeLoadFormat"); format->addItems({"CSV","Nastran BDF"}); form->addRow(tr("文件格式"),format);
    auto* grid=new QSpinBox; grid->setObjectName("nativeLoadNodeId"); grid->setRange(0,2147483647); form->addRow(tr("BDF 目标节点 ID"),grid);
    auto* components=new QWidget; auto* compLayout=new QHBoxLayout(components); compLayout->setContentsMargins(0,0,0,0);
    std::array<QCheckBox*,6> masks{}; const QStringList names{"Fx","Fy","Fz","Tx","Ty","Tz"};
    for (int i=0;i<6;++i) { masks[i]=new QCheckBox(names[i]); masks[i]->setChecked(true); compLayout->addWidget(masks[i]); }
    form->addRow(components);
    auto* target=new QCheckBox(tr("将力矩转移到目标点 (r × F)")); target->setObjectName("nativeLoadUseTargetPoint"); form->addRow(target);
    auto* point=new QGroupBox(tr("目标点 · 全局 mm")); auto* pointForm=new QFormLayout(point); std::array<QLineEdit*,3> xyz{};
    for (int i=0;i<3;++i) { xyz[i]=new QLineEdit("0"); xyz[i]->setObjectName(QStringLiteral("native_")+QChar('x'+i)); pointForm->addRow(QString(QChar('X'+i)),xyz[i]); }
    form->addRow(point);
    auto* picker=new NativePickSession(m_viewport,m_document,{},panel); picker->decorateVector(pointForm,point,"actionPoint","loadTargetPoint");
    auto* node=new QPushButton(tr("拾取目标网格节点")); node->setObjectName("nativeLoadPickNode"); form->addRow(node);
    connect(node,&QPushButton::clicked,panel,[picker,node,grid,target,xyz] {
        const int previousNodeId=grid->value(); const bool previousTransfer=target->isChecked();
        std::array<QString,3> previousCoordinates;
        for (int i=0;i<3;++i) previousCoordinates[i]=xyz[i]->text();
        picker->begin("loadTargetNode",node,PickKind::Node,false,{}, {},[=](const auto& picks) {
            if (picks.isEmpty()) return;
            grid->setValue(picks.back().nodeId); target->setChecked(true);
            for (int i=0;i<3;++i) xyz[i]->setText(QString::number(picks.back().position[i],'g',17));
        },[grid,target,xyz,previousNodeId,previousTransfer,previousCoordinates] {
            grid->setValue(previousNodeId); target->setChecked(previousTransfer);
            for (int i=0;i<3;++i) xyz[i]->setText(previousCoordinates[i]);
        });
    });
    auto* help=new QLabel(tr("导出物理受力端的全局力和力矩，单位 N、N·mm。BDF 写入 FORCE / MOMENT 卡片；目标节点需存在于接收侧网格。")); help->setWordWrap(true); form->addRow(help);
    auto* exportLoad=new QPushButton(tr("导出提取载荷…")); exportLoad->setObjectName("nativeExportLoad"); form->addRow(exportLoad);
    connect(exportLoad,&QPushButton::clicked,panel,[this,run,source,end,mode,frame,scale,format,grid,masks,target,xyz,destination,picker] {
        picker->finish(); const auto key=source->currentData().toString().split(':'); if (key.size()!=2) { setNotice(tr("没有可导出的载荷来源。"),true); return; }
        NativeLoadExportOptions options; options.end=end->currentIndex()==0 ? NativeLoadEnd::I : NativeLoadEnd::J;
        options.frames=NativeLoadFrames(mode->currentIndex()); options.frameIndex=frame->value(); options.scale=scale->value(); options.nodeId=grid->value();
        options.format=format->currentIndex()==0 ? NativeLoadFormat::Csv : NativeLoadFormat::NastranBdf;
        options.useTargetPoint=target->isChecked(); for (int i=0;i<6;++i) options.components[i]=masks[i]->isChecked();
        for (int i=0;i<3;++i) { bool ok=false; options.targetPointMm[i]=xyz[i]->text().toDouble(&ok);
            if (options.useTargetPoint && (!ok || !std::isfinite(options.targetPointMm[i]))) { setNotice(tr("目标点坐标无效。"),true); return; } }
        const bool bdf=options.format==NativeLoadFormat::NastranBdf;
        const auto path=destination(bdf ? "loads.bdf" : "loads.csv",bdf ? tr("Nastran BDF (*.bdf)") : tr("CSV 表格 (*.csv)"),bdf ? ".bdf" : ".csv");
        if (path.isEmpty()) return; QString error;
        if (!exportNativeLoads(path,*run->input.nativeInput,run->result,{key[0],key[1].toInt()},options,error)) setNotice(error,true);
        else setNotice(tr("载荷已导出：%1").arg(QFileInfo(path).fileName()));
    });
    auto* cases=new QGroupBox(tr("特征工况 / 连杆同步工况")); auto* caseForm=new QFormLayout(cases); caseForm->setRowWrapPolicy(QFormLayout::WrapLongRows); layout->addWidget(cases);
    auto* body=new QComboBox; body->setObjectName("nativeLoadAnalysisBody");
    for (const auto& value:document.collection("bodies")) { const auto row=value.toObject(); body->addItem(row.value("name").toString(),row.value("id").toInt()); }
    caseForm->addRow(tr("分析运动体"),body);
    auto* joints=new QListWidget; joints->setObjectName("nativeLoadCaseJoints"); joints->setMaximumHeight(160); caseForm->addRow(tr("关联运动副"),joints);
    const auto refresh=[body,joints,document] {
        joints->clear(); const int id=body->currentData().isValid() ? body->currentData().toInt() : -1;
        for (const auto& value:document.collection("joints")) { const auto row=value.toObject();
            if (row.value("actionBodyId").toInt(-1)!=id && row.value("baseBodyId").toInt(-1)!=id) continue;
            auto* item=new QListWidgetItem(QStringLiteral("%1 [%2]").arg(row.value("name").toString()).arg(row.value("id").toInt()),joints);
            item->setData(Qt::UserRole,row.value("id").toInt()); item->setFlags(item->flags()|Qt::ItemIsUserCheckable); item->setCheckState(Qt::Checked);
        }
    };
    connect(body,qOverload<int>(&QComboBox::currentIndexChanged),panel,[refresh](int) { refresh(); }); refresh();
    auto* peaks=new QSpinBox; peaks->setObjectName("nativeLoadCasePeaks"); peaks->setRange(1,32); peaks->setValue(4); caseForm->addRow(tr("每条曲线极值数"),peaks);
    auto* tolerance=new QDoubleSpinBox; tolerance->setObjectName("nativeLoadCaseTolerance"); tolerance->setDecimals(12); tolerance->setRange(0,1e6); tolerance->setValue(1e-9); caseForm->addRow(tr("时刻合并容差 (s)"),tolerance);
    auto* synchronized=new QCheckBox(tr("每个工况包含全部所选副的同步载荷")); synchronized->setChecked(true); synchronized->setObjectName("nativeLoadCaseSynchronized"); caseForm->addRow(synchronized);
    auto* caseHelp=new QLabel(tr("按真实结果曲线筛选极值并合并时刻；载荷符号按分析体所在端处理，力、力矩及作用点统一变换到分析体初始姿态。")); caseHelp->setWordWrap(true); caseForm->addRow(caseHelp);
    auto* exportCases=new QPushButton(tr("导出工况 JSON…")); exportCases->setObjectName("nativeExportLoadCases"); caseForm->addRow(exportCases);
    connect(exportCases,&QPushButton::clicked,panel,[this,run,body,joints,peaks,tolerance,synchronized,destination] {
        NativeLoadCaseOptions options; options.analysisBodyId=body->currentData().toInt(); options.peaksPerSignal=peaks->value(); options.mergeTolerance=tolerance->value(); options.synchronized=synchronized->isChecked();
        for (int i=0;i<joints->count();++i) if (joints->item(i)->checkState()==Qt::Checked) options.jointIds.push_back(joints->item(i)->data(Qt::UserRole).toInt());
        QJsonObject preview; QString error;
        if (!buildNativeLoadCases(*run->input.nativeInput,run->result,options,preview,error)) { setNotice(error,true); return; }
        const auto path=destination("motion-load-cases.json",tr("载荷工况 JSON (*.json)"),".json"); if (path.isEmpty()) return;
        if (!exportNativeLoadCases(path,*run->input.nativeInput,run->result,options,error)) setNotice(error,true); else setNotice(tr("载荷工况已导出：%1").arg(QFileInfo(path).fileName()));
    });
    auto* scroll=new QScrollArea; scroll->setWidgetResizable(true); scroll->setWidget(panel); showEditor(scroll);
}
} // namespace MotionOcc
