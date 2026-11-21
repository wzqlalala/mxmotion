#include "NativeModeInspector.h"
#include "CadDocument.h"
#include "NativeMotionDocument.h"
#include "NativeMotionSymbols.h"
#include "OccViewport.h"
#include "FemMesh.h"
#include "RigidMotion.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QSplitter>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <memory>
#include <set>

namespace MotionOcc {
namespace {
QString basisName(const QString& kind)
{
    if(kind=="FixedInterface")return QStringLiteral("固定接口模态基（附着区夹持）");
    if(kind=="FreeBody")return QStringLiteral("自由体模态基（含可能的刚体模态）");
    return kind;
}
struct Inspection {
    CadDocument cad;
    NativeMotionDocument document;
    std::vector<RigidFlexResult> results;
    OccViewport* viewport=nullptr;
    QComboBox* body=nullptr;
    QTableWidget* frequencies=nullptr;
    QTableWidget* patches=nullptr;
    QLabel* description=nullptr;
    QLabel* status=nullptr;
    QDoubleSpinBox* amplitude=nullptr;
    QDoubleSpinBox* visualSpeed=nullptr;
    QCheckBox* attachments=nullptr;
    QSlider* phase=nullptr;
    QPushButton* play=nullptr;
    QTimer* timer=nullptr;
    QElapsedTimer clock;
    int part=-1;
    double markerSize=1;

    const RigidFlexResult* selected()const{
        const int index=body->currentIndex();
        return index>=0&&std::size_t(index)<results.size()?&results[std::size_t(index)]:nullptr;
    }
    void draw(){
        const auto* result=selected();if(!result||part<1)return;
        FlexModeSample sample;QString error;
        if(!sampleFlexMode(*result,frequencies->currentRow(),amplitude->value(),phase->value()/1000.0,sample,error)){
            status->setText(error);return;
        }
        FemMesh mesh;
        for(std::size_t node=0;node<sample.positions.size()/3;++node)
            mesh.nodes.push_back({sample.positions[3*node],sample.positions[3*node+1],sample.positions[3*node+2]});
        for(std::size_t face=0;face+2<result->surfaceTriangles.size();face+=3)
            mesh.surface.push_back({std::uint32_t(result->surfaceTriangles[face]),std::uint32_t(result->surfaceTriangles[face+1]),std::uint32_t(result->surfaceTriangles[face+2])});
        viewport->setDeformedMeshes({{part,mesh}},{});
        viewport->setMeshScalars({{part,sample.magnitudes}},0,1,QStringLiteral("振型幅值（归一化）"));
        QVector<MotionSymbol> symbols;
        if(attachments->isChecked())for(std::size_t group=0;group<result->modalBasis.attachmentGroups.size();++group){
            MotionSymbol symbol;symbol.collection="modalAttachment";symbol.id=int(group);
            symbol.label=QStringLiteral("接口 %1").arg(group+1);
            symbol.color=QColor::fromHsv(int(group*83)%360,230,195);
            symbol.emphasized=patches->currentRow()==int(group);
            const auto& nodes=result->modalBasis.attachmentGroups[group];
            for(int node:nodes){
                const auto p=mesh.nodes[std::size_t(node)];
                for(int component=0;component<3;++component){
                    auto first=p,last=p;first[component]-=markerSize;last[component]+=markerSize;
                    symbol.linePoints.push_back(first);symbol.linePoints.push_back(last);
                    symbol.origin[component]+=p[component]/nodes.size();
                }
            }
            symbols.append(std::move(symbol));
        }
        viewport->setMotionSymbols(symbols);
        status->setText(QStringLiteral("第 %1 阶 · %2 Hz · 显示峰值 %3 mm · 振型原始最大范数 %4")
            .arg(frequencies->currentRow()+1).arg(sample.frequencyHz,0,'g',9)
            .arg(amplitude->value(),0,'g',6).arg(sample.rawMaximum,0,'g',9));
    }
    void selectBody(){
        timer->stop();play->setText(QStringLiteral("播放振型"));
        part=-1;frequencies->setRowCount(0);patches->setRowCount(0);description->clear();
        std::set<int> allHidden;for(const auto& item:cad.parts())allHidden.insert(item.id);
        viewport->setScene(cad,{},allHidden);viewport->setMotionSymbols({});
        const auto* result=selected();if(!result)return;
        QString error;
        const bool valid=validateFlexModes(*result,error);
        for(auto* field:std::initializer_list<QWidget*>{play,amplitude,phase,attachments,visualSpeed})field->setEnabled(valid);
        if(!valid){status->setText(error);description->setText(basisName(result->modalBasis.kind)+"\n"+result->modalBasis.diagnostics.join("\n"));return;}
        const auto bound=document.bodyPartBindings.value(result->bodyId);
        if(bound.isEmpty() || !cad.part(bound.front())){status->setText(QStringLiteral("柔性体缺少 CAD 显示绑定。"));return;}
        part=bound.front();std::set<int> hidden;
        for(const auto& item:cad.parts())if(item.id!=part)hidden.insert(item.id);
        viewport->setScene(cad,{},hidden);
        double span=0;
        for(int component=0;component<3;++component){
            double first=result->referenceNodes[component],last=first;
            for(std::size_t node=0;node<result->referenceNodes.size()/3;++node){
                first=std::min(first,result->referenceNodes[3*node+component]);
                last=std::max(last,result->referenceNodes[3*node+component]);
            }
            span=std::max(span,last-first);
        }
        markerSize=std::max(span*.005,1e-6);
        {QSignalBlocker blocker(amplitude);amplitude->setMaximum(std::max(1.0,span));amplitude->setValue(std::max(1e-6,span*.05));}
        {QSignalBlocker blocker(phase);phase->setValue(1000);}
        const auto& basis=result->modalBasis;
        description->setText(QStringLiteral("%1\n请求 %2 阶，实际保留 %3 阶；截止频率 %4。\n%5\n振型按最大节点位移归一化，仅显示幅值可调；这不是工况变形或整机固有模态。")
            .arg(basisName(basis.kind)).arg(basis.requestedCount).arg(basis.frequenciesHz.size())
            .arg(basis.cutoffHz>0?QStringLiteral("%1 Hz").arg(basis.cutoffHz,0,'g',9):QStringLiteral("不截断"))
            .arg(result->reducedDynamics?QStringLiteral("动力学表示：模态缩减。"):
                QStringLiteral("动力学表示：独立接口共回转全有限元；此处的夹持模态仅用于检查。")));
        if(!basis.diagnostics.isEmpty())description->setText(description->text()+"\n"+basis.diagnostics.join("\n"));
        {
            QSignalBlocker blocker(frequencies);frequencies->setRowCount(int(basis.frequenciesHz.size()));
            for(int mode=0;mode<frequencies->rowCount();++mode){
                const double f=basis.frequenciesHz[std::size_t(mode)];
                frequencies->setItem(mode,0,new QTableWidgetItem(QString::number(mode+1)));
                frequencies->setItem(mode,1,new QTableWidgetItem(QString::number(f,'g',12)));
                frequencies->setItem(mode,2,new QTableWidgetItem(f>1e-12?QString::number(1/f,'g',8):QStringLiteral("—")));
            }
            frequencies->selectRow(0);
        }
        {
            QSignalBlocker blocker(patches);patches->setRowCount(int(basis.attachmentGroups.size()));
            for(int group=0;group<patches->rowCount();++group){
                QStringList ids;for(int id:basis.attachmentGroups[std::size_t(group)])ids.append(QString::number(id));
                patches->setItem(group,0,new QTableWidgetItem(QString::number(group+1)));
                patches->setItem(group,1,new QTableWidgetItem(QString::number(ids.size())));
                auto* item=new QTableWidgetItem(ids.join(", "));item->setToolTip(item->text());patches->setItem(group,2,item);
            }
        }
        draw();viewport->viewIsometric();viewport->fitAll();
    }
};
}

QDialog* openNativeModeInspector(QWidget* parent,const CadDocument& cad,const NativeMotionDocument& document,
                                 const std::vector<RigidFlexResult>& results)
{
    auto state=std::make_shared<Inspection>();state->cad=cad;state->document=document;
    for(const auto& source:results){
        // Retain only the basis/reference mesh. A potentially large transient
        // history is not duplicated just to display a mode.
        RigidFlexResult copy;copy.flexId=source.flexId;copy.bodyId=source.bodyId;
        copy.referenceNodes=source.referenceNodes;copy.surfaceTriangles=source.surfaceTriangles;
        copy.modalBasis=source.modalBasis;copy.reducedDynamics=source.reducedDynamics;
        state->results.push_back(std::move(copy));
    }
    auto* dialog=new QDialog(parent);dialog->setObjectName("nativeModeInspector");
    dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->setWindowTitle(QStringLiteral("真实模态检查器"));dialog->resize(1120,760);
    auto* layout=new QVBoxLayout(dialog);auto* splitter=new QSplitter(dialog);layout->addWidget(splitter,1);
    auto* controls=new QWidget;auto* form=new QVBoxLayout(controls);
    state->body=new QComboBox;state->body->setObjectName("nativeModeBody");
    for(const auto& result:state->results){
        const auto entity=document.entity("autoFlexBodies",result.flexId);
        state->body->addItem(QStringLiteral("%1 [%2]").arg(entity.value("name").toString(QStringLiteral("柔性体"))).arg(result.flexId));
    }
    form->addWidget(state->body);state->description=new QLabel;state->description->setWordWrap(true);
    state->description->setObjectName("nativeModeDescription");form->addWidget(state->description);
    state->frequencies=new QTableWidget(0,3);state->frequencies->setObjectName("nativeModeFrequencies");
    state->frequencies->setHorizontalHeaderLabels({QStringLiteral("阶次"),QStringLiteral("频率 (Hz)"),QStringLiteral("周期 (s)")});
    state->frequencies->setSelectionBehavior(QAbstractItemView::SelectRows);state->frequencies->setSelectionMode(QAbstractItemView::SingleSelection);
    state->frequencies->setEditTriggers(QAbstractItemView::NoEditTriggers);state->frequencies->verticalHeader()->hide();
    state->frequencies->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    state->frequencies->horizontalHeader()->setStretchLastSection(true);form->addWidget(state->frequencies,2);
    auto* settings=new QFormLayout;
    state->amplitude=new QDoubleSpinBox;state->amplitude->setObjectName("nativeModeAmplitude");state->amplitude->setDecimals(6);state->amplitude->setMinimum(0);state->amplitude->setSuffix(" mm");
    settings->addRow(QStringLiteral("显示峰值幅值"),state->amplitude);
    state->visualSpeed=new QDoubleSpinBox;state->visualSpeed->setObjectName("nativeModeVisualSpeed");state->visualSpeed->setRange(.05,5);state->visualSpeed->setValue(.5);state->visualSpeed->setSuffix(" Hz");
    state->visualSpeed->setToolTip(QStringLiteral("放慢后的观察速度，与求解得到的真实频率分开。"));settings->addRow(QStringLiteral("观察速度"),state->visualSpeed);
    state->phase=new QSlider(Qt::Horizontal);state->phase->setObjectName("nativeModePhase");state->phase->setRange(-1000,1000);settings->addRow(QStringLiteral("振型相位"),state->phase);
    form->addLayout(settings);state->play=new QPushButton(QStringLiteral("播放振型"));state->play->setObjectName("nativeModePlay");form->addWidget(state->play);
    state->attachments=new QCheckBox(QStringLiteral("显示实际附着节点"));state->attachments->setChecked(true);state->attachments->setObjectName("nativeModeAttachments");form->addWidget(state->attachments);
    state->patches=new QTableWidget(0,3);state->patches->setObjectName("nativeModePatches");
    state->patches->setHorizontalHeaderLabels({QStringLiteral("接口"),QStringLiteral("节点数"),QStringLiteral("节点索引（从 0 起）")});
    state->patches->setEditTriggers(QAbstractItemView::NoEditTriggers);state->patches->setSelectionBehavior(QAbstractItemView::SelectRows);
    state->patches->verticalHeader()->hide();state->patches->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);state->patches->horizontalHeader()->setStretchLastSection(true);form->addWidget(state->patches,1);
    splitter->addWidget(controls);state->viewport=new OccViewport;state->viewport->setObjectName("nativeModeViewport");state->viewport->setPickEnabled(false);splitter->addWidget(state->viewport);splitter->setStretchFactor(1,2);splitter->setSizes({380,720});
    state->status=new QLabel;state->status->setObjectName("nativeModeStatus");state->status->setWordWrap(true);layout->addWidget(state->status);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Close);auto* csv=buttons->addButton(QStringLiteral("导出频率与振型 CSV…"),QDialogButtonBox::ActionRole);csv->setObjectName("nativeModeExport");
    // The application's legacy button style has a fixed width; retain enough
    // space for the actual export action in this dialog.
    csv->setMinimumWidth(220);buttons->button(QDialogButtonBox::Close)->setText(QStringLiteral("关闭"));layout->addWidget(buttons);
    state->timer=new QTimer(dialog);state->timer->setInterval(30);
    QObject::connect(state->body,qOverload<int>(&QComboBox::currentIndexChanged),dialog,[state]{state->selectBody();});
    QObject::connect(state->frequencies,&QTableWidget::currentCellChanged,dialog,[state]{state->draw();});
    QObject::connect(state->patches,&QTableWidget::currentCellChanged,dialog,[state]{state->draw();});
    QObject::connect(state->amplitude,qOverload<double>(&QDoubleSpinBox::valueChanged),dialog,[state]{state->draw();});
    QObject::connect(state->phase,&QSlider::valueChanged,dialog,[state]{state->draw();});
    QObject::connect(state->attachments,&QCheckBox::toggled,dialog,[state]{state->draw();});
    QObject::connect(state->play,&QPushButton::clicked,dialog,[state]{
        if(state->timer->isActive()){state->timer->stop();state->play->setText(QStringLiteral("播放振型"));}
        else{state->clock.restart();state->timer->start();state->play->setText(QStringLiteral("暂停振型"));}
    });
    QObject::connect(state->timer,&QTimer::timeout,dialog,[state]{
        constexpr double pi=3.14159265358979323846;
        state->phase->setValue(int(1000*std::cos(2*pi*state->visualSpeed->value()*state->clock.elapsed()/1000.0)));
    });
    QObject::connect(csv,&QPushButton::clicked,dialog,[state,dialog]{
        const auto* result=state->selected();if(!result)return;
        auto path=QFileDialog::getSaveFileName(dialog,QStringLiteral("导出真实模态"),QStringLiteral("flex-%1-modes.csv").arg(result->flexId),QStringLiteral("CSV (*.csv)"));
        if(path.isEmpty())return;if(QFileInfo(path).suffix().isEmpty())path+=".csv";
        QString error;state->status->setText(exportFlexModesCsv(path,*result,error)?QStringLiteral("已导出：%1").arg(path):error);
    });
    QObject::connect(buttons,&QDialogButtonBox::rejected,dialog,&QDialog::close);
    state->selectBody();dialog->show();return dialog;
}

} // namespace MotionOcc
