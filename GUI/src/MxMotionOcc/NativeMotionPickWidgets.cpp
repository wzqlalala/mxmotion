#include "NativeMotionPickWidgets.h"
#include "CadDocument.h"
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVariant>
#include <algorithm>
#include <cmath>

namespace MotionOcc {
namespace {
QJsonArray array(const std::array<double,3>& v) { return {v[0],v[1],v[2]}; }
std::array<double,3> vector(const QJsonArray& a)
{
    return a.size()==3 ? std::array<double,3>{a[0].toDouble(),a[1].toDouble(),a[2].toDouble()} : std::array<double,3>{};
}
QString safeName(QString text) { return text.replace('.', '_'); }
}

NativePickSession::NativePickSession(OccViewport* viewport, const CadDocument& cad,
    const QJsonObject& references, QObject* parent)
    : QObject(parent), m_viewport(viewport), m_cad(cad), m_references(references)
{
    connect(viewport, &OccViewport::pickSelectionChanged, this, [this](const QVector<PickResult>& picks) {
        if (!ownsSession() || !m_changed) return;
        record(m_key,picks);
        m_changed(picks);
    });
    connect(viewport, &OccViewport::pickCancelled, this, [this] {
        if (ownsSession()) finish(true);
    });
}

NativePickSession::~NativePickSession() { finish(); }

bool NativePickSession::ownsSession() const
{
    return m_viewport && m_viewport->property("nativePickOwner").value<quintptr>() == reinterpret_cast<quintptr>(this);
}

void NativePickSession::record(const QString& key, const QVector<PickResult>& picks)
{
    QJsonArray values;
    for (const auto& p:picks) {
        QJsonArray connectivity; for (int id:p.nodeIds) connectivity.append(id);
        values.append(QJsonObject{{"kind",int(p.kind)},{"partId",p.partId},
        {"index",p.index},{"nodeId",p.nodeId},{"position",array(p.position)},
        {"nodeIds",connectivity},{"hitPosition",array(p.hitPosition)},{"direction",array(p.direction)},{"hasDirection",p.hasDirection}});
    }
    m_references.insert(key,values);
}

QVector<PickResult> NativePickSession::saved(const QString& key) const
{
    QVector<PickResult> result;
    for (const auto& value:m_references.value(key).toArray()) {
        const auto o=value.toObject(); const int kind=o.value("kind").toInt(-1);
        if (kind<int(PickKind::Part) || kind>int(PickKind::Element)) continue;
        PickResult p; p.kind=PickKind(kind); p.partId=o.value("partId").toInt(-1);
        if (!m_cad.part(p.partId)) continue;
        p.index=o.value("index").toInt(-1); p.nodeId=o.value("nodeId").toInt(-1);
        for (const auto& id:o.value("nodeIds").toArray()) p.nodeIds.append(id.toInt());
        p.position=vector(o.value("position").toArray()); p.direction=vector(o.value("direction").toArray());
        p.hitPosition=vector(o.value("hitPosition").toArray());
        p.hasDirection=o.value("hasDirection").toBool();
        if (p.isValid()) result.append(p);
    }
    return result;
}

void NativePickSession::begin(const QString& key, QPushButton* button, PickKind kind, bool multiple,
    const std::set<int>& allowed, const QVector<PickResult>& initial, Changed changed, std::function<void()> cancelled)
{
    if (ownsSession() && m_button==button && m_viewport->isPicking()) { finish(); return; }
    finish();
    m_key=key; m_button=button; m_buttonText=button->text(); m_initial=initial; m_changed=std::move(changed); m_cancelled=std::move(cancelled);
    m_viewport->setProperty("nativePickOwner",QVariant::fromValue(reinterpret_cast<quintptr>(this)));
    button->setText(QStringLiteral("完成拾取")); button->setProperty("pickActive",true);
    button->setToolTip(QStringLiteral("点击视口选择；右键撤销最后一项；Esc 撤销本次拾取；再次点击完成。多选节点可 Ctrl+左键贯穿框选，包含框内被遮挡的节点。"));
    m_viewport->beginPick(kind,multiple,allowed,initial);
    m_viewport->setFocus(Qt::OtherFocusReason);
}

void NativePickSession::finish(bool cancel)
{
    if (!ownsSession()) return;
    if (cancel && m_changed) { record(m_key,m_initial); m_changed(m_initial); }
    if (cancel && m_cancelled) m_cancelled();
    m_viewport->setProperty("nativePickOwner",QVariant{});
    m_viewport->endPick();
    if (m_button) { m_button->setText(m_buttonText); m_button->setProperty("pickActive",false); }
    m_changed={}; m_cancelled={}; m_button=nullptr;
}

void NativePickSession::decorateVector(QFormLayout* form, QGroupBox* box, const QString& key, const QString& path)
{
    const QStringList positions{"origin","actionPoint","centerOfMass","localI","localJ","localTarget",
        "localOrigin","localPosition","referencePoint","localPos","position"};
    const QStringList directions{"axis","direction","fixedDirection","fixedAxis","actionSpin","actionPin",
        "baseSpin","basePin","actionVector","baseVector","axisDirection","gravity"};
    const bool isDirection=directions.contains(key);
    if (!isDirection && !positions.contains(key)) return;
    std::array<QLineEdit*,3> edits{};
    for (int i=0;i<3;++i) edits[i]=box->findChild<QLineEdit*>(QStringLiteral("native_")+QString(QChar('x'+i)));
    if (!edits[0] || !edits[1] || !edits[2]) return;
    auto read=[edits] { std::array<double,3> v{}; for (int i=0;i<3;++i) v[i]=edits[i]->text().toDouble(); return v; };
    auto write=[edits](const std::array<double,3>& v) { for (int i=0;i<3;++i) edits[i]->setText(QString::number(v[i],'g',17)); };
    auto* mode=new QComboBox(box); mode->setObjectName("native_pickMode_"+safeName(path));
    mode->addItem(QStringLiteral("直接输入"),"manual");
    if (isDirection) {
        mode->addItem(QStringLiteral("X 轴"),"x"); mode->addItem(QStringLiteral("Y 轴"),"y"); mode->addItem(QStringLiteral("Z 轴"),"z");
        mode->addItem(QStringLiteral("两顶点定向"),"twoVertices"); mode->addItem(QStringLiteral("两网格节点定向"),"twoNodes");
        mode->addItem(QStringLiteral("几何边方向 / 圆轴"),"edge"); mode->addItem(QStringLiteral("面法向 / 回转轴"),"face");
        mode->addItem(QStringLiteral("网格边方向"),"meshEdge"); mode->addItem(QStringLiteral("网格面法向"),"meshFace");
    } else {
        mode->addItem(QStringLiteral("几何顶点"),"vertex"); mode->addItem(QStringLiteral("边中点 / 圆心"),"edge");
        mode->addItem(QStringLiteral("面中心"),"face"); mode->addItem(QStringLiteral("实体质心"),"part");
        mode->addItem(QStringLiteral("网格节点"),"node"); mode->addItem(QStringLiteral("曲线上的点"),"edgeHit");
        mode->addItem(QStringLiteral("曲面上的点"),"faceHit");
        mode->addItem(QStringLiteral("网格边中点"),"meshEdge"); mode->addItem(QStringLiteral("网格面中心"),"meshFace");
        mode->addItem(QStringLiteral("单元中心"),"element");
    }
    auto* pick=new QPushButton(QStringLiteral("在视口拾取"),box); pick->setObjectName("native_pick_"+safeName(path));
    auto* status=new QLabel(box); status->setObjectName("native_pickStatus_"+safeName(path)); status->setWordWrap(true);
    form->addRow(QStringLiteral("取值方式"),mode); form->addRow(pick);
    if (isDirection) {
        auto* reverse=new QPushButton(QStringLiteral("反向"),box); reverse->setObjectName("native_reverse_"+safeName(path));
        form->addRow(reverse); connect(reverse,&QPushButton::clicked,box,[this,read,write,path] {
            finish();
            m_references.insert(path+".reversed",!m_references.value(path+".reversed").toBool());
            auto v=read(); for (auto& x:v) x=-x; write(v);
        });
    }
    form->addRow(status);
    for (auto* edit:edits) connect(edit,&QLineEdit::textEdited,box,[this,path,mode,status] {
        finish(); m_references.remove(path); m_references.remove(path+".mode"); m_references.remove(path+".reversed");
        const QSignalBlocker blocker(mode); mode->setCurrentIndex(mode->findData("manual"));
        status->setText(QStringLiteral("已改为手动输入，几何引用已解除。"));
    });
    auto previous=saved(path);
    if (!previous.isEmpty()) {
        const auto kind=previous.back().kind;
        const QString savedMode=m_references.value(path+".mode").toString();
        const int index=mode->findData(savedMode.isEmpty() ? (kind==PickKind::Node ? "node" : kind==PickKind::Edge ? "edge" : kind==PickKind::Face ? "face" :
            kind==PickKind::Part ? "part" : kind==PickKind::MeshEdge ? "meshEdge" : kind==PickKind::MeshFace ? "meshFace" : kind==PickKind::Element ? "element" : "vertex") : savedMode);
        if (index>=0) mode->setCurrentIndex(index);
        status->setText(QStringLiteral("已恢复 %1 项几何引用；激活后显示高亮。").arg(previous.size()));
    }
    connect(mode,qOverload<int>(&QComboBox::currentIndexChanged),box,[this,mode,read,write,isDirection,key,path] {
        finish(); const auto name=mode->currentData().toString(); m_references.insert(path+".mode",name);
        m_references.remove(path+".reversed");
        if (isDirection && (name=="x" || name=="y" || name=="z")) {
            const auto old=read(); const double mag=key=="gravity" ? std::sqrt(old[0]*old[0]+old[1]*old[1]+old[2]*old[2]) : 1.0;
            std::array<double,3> v{}; v[name=="x"?0:name=="y"?1:2]=mag; write(v);
        }
    });
    connect(pick,&QPushButton::clicked,box,[this,mode,pick,status,read,write,path,isDirection,key] {
        auto name=mode->currentData().toString();
        if (name=="manual" || name=="x" || name=="y" || name=="z") {
            const QSignalBlocker blocker(mode); mode->setCurrentIndex(mode->findData(isDirection ? "edge" : "vertex"));
            name=mode->currentData().toString();
        }
        m_references.insert(path+".mode",name);
        const PickKind kind=(name=="edge" || name=="edgeHit") ? PickKind::Edge : (name=="face" || name=="faceHit") ? PickKind::Face
            : (name=="node" || name=="twoNodes") ? PickKind::Node : name=="part" ? PickKind::Part :
            name=="meshEdge" ? PickKind::MeshEdge : name=="meshFace" ? PickKind::MeshFace : name=="element" ? PickKind::Element : PickKind::Vertex;
        const bool two=name=="twoVertices" || name=="twoNodes";
        const auto original=read(); const bool originalReversed=m_references.value(path+".reversed").toBool(); auto initial=saved(path);
        initial.erase(std::remove_if(initial.begin(),initial.end(),[kind](const auto& p) { return p.kind!=kind; }),initial.end());
        const double magnitude=key=="gravity" ? std::sqrt(original[0]*original[0]+original[1]*original[1]+original[2]*original[2]) : 1.0;
        begin(path,pick,kind,two,{},initial,[=](const QVector<PickResult>& picks) {
            if (picks.isEmpty()) { write(original); status->setText(QStringLiteral("尚未拾取")); return; }
            auto value=picks.back().position;
            if (isDirection) {
                if (two) {
                    if (picks.size()<2) { write(original); status->setText(QStringLiteral("请选择第二个点。")); return; }
                    for (int i=0;i<3;++i) value[i]=picks.back().position[i]-picks[picks.size()-2].position[i];
                } else {
                    if (!picks.back().hasDirection) { write(original); status->setText(QStringLiteral("所选几何没有可用方向，请选其他边或面。")); return; }
                    value=picks.back().direction;
                }
                const double length=std::sqrt(value[0]*value[0]+value[1]*value[1]+value[2]*value[2]);
                if (!(length>1e-12)) { write(original); status->setText(QStringLiteral("两点重合，不能确定方向。")); return; }
                const double sign=m_references.value(path+".reversed").toBool() ? -1.0 : 1.0;
                for (auto& x:value) x=x/length*magnitude*sign;
            } else if (name.endsWith("Hit")) value=picks.back().hitPosition;
            write(value);
            status->setText(QStringLiteral("部件 %1 · %2 %3").arg(picks.back().partId)
                .arg(kind==PickKind::Node ? QStringLiteral("节点") : QStringLiteral("拓扑"))
                .arg(kind==PickKind::Node ? picks.back().nodeId : picks.back().index));
        },[this,write,original,path,originalReversed] {
            write(original); m_references.insert(path+".reversed",originalReversed);
        });
    });
}
} // namespace MotionOcc
