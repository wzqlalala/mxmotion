#include "NativeMotionPickWidgets.h"
#include "CadDocument.h"
#include "picking/LegacyPickContext.h"
#include "mPointPickerWidget.h"
#include "mDirectionSelectorWidget.h"
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
    m_legacyContext=LegacyPickContext::forObject(parent);
    if (!m_legacyContext || m_legacyContext->viewport()!=viewport || &m_legacyContext->document()!=&cad) {
        m_legacyContext=new LegacyPickContext(qobject_cast<QWidget*>(parent),viewport,cad);
        if(!m_legacyContext->parent())m_legacyContext->setParent(this);
    }
    connect(m_legacyContext,&LegacyPickContext::allowedTypesChanged,this,[this] {
        if (MechMainWindow::mPickWidgetController::hasActivePickController() && m_button) finish();
    });
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
        if (kind<int(PickKind::Part) || kind>int(PickKind::Solid)) continue;
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
    if (auto* active=MechMainWindow::mPickWidgetController::activeController()) active->setPickActive(false);
    m_key=key; m_button=button; m_buttonText=button->text(); m_initial=initial; m_changed=std::move(changed); m_cancelled=std::move(cancelled);
    m_viewport->setProperty("nativePickOwner",QVariant::fromValue(reinterpret_cast<quintptr>(this)));
    button->setText(QStringLiteral("完成拾取")); button->setProperty("pickActive",true);
    button->setToolTip(kind == PickKind::Node && multiple
        ? QStringLiteral("左键加选，右键减选，点击和拖框均可用。Esc 撤销本次拾取，再次点击完成。")
        : QStringLiteral("点击视口选择；右键撤销最后一项；Esc 撤销本次拾取；再次点击完成。"));
    m_viewport->beginPick(kind,multiple,allowed,initial);
    m_viewport->setFocus(Qt::OtherFocusReason);
}

void NativePickSession::finish(bool cancel)
{
    if (ownsSession()) {
        if (cancel && m_changed) { record(m_key,m_initial); m_changed(m_initial); }
        if (cancel && m_cancelled) m_cancelled();
        m_viewport->setProperty("nativePickOwner",QVariant{});
        m_viewport->endPick();
    }
    if (m_button) { m_button->setText(m_buttonText); m_button->setProperty("pickActive",false); }
    m_changed={}; m_cancelled={}; m_button=nullptr;
}

void NativePickSession::decorateVector(QFormLayout* form, QGroupBox* box, const QString& key, const QString& path)
{
    using namespace MechMainWindow;
    const QStringList positions{"origin","actionPoint","centerOfMass","localI","localJ","localTarget",
        "localOrigin","localPosition","referencePoint","localPos","position"};
    const QStringList directions{"axis","direction","fixedDirection","fixedAxis","actionSpin","actionPin",
        "baseSpin","basePin","actionVector","baseVector","axisDirection","gravity"};
    const bool isDirection=directions.contains(key);
    if ((!isDirection && !positions.contains(key)) || !m_legacyContext) return;
    std::array<QLineEdit*,3> edits{};
    for(int i=0;i<3;++i)edits[i]=box->findChild<QLineEdit*>(QStringLiteral("native_")+QString(QChar('x'+i)));
    if(!edits[0]||!edits[1]||!edits[2])return;
    const auto initial=std::array<double,3>{edits[0]->text().toDouble(),edits[1]->text().toDouble(),edits[2]->text().toDouble()};
    const auto write=[edits](const QVector3D& value) {
        const double xyz[]={value.x(),value.y(),value.z()};
        for(int i=0;i<3;++i)edits[i]->setText(QString::number(xyz[i],'g',17));
    };
    // Keep the existing serialized fields alive for the new engine additions,
    // while the actual visible control is the uploaded original GUI source.
    for(int i=0;i<form->count();++i)if(auto* widget=form->itemAt(i)->widget())widget->hide();
    const QString title=box->title();
    box->setTitle({});box->setProperty("originalPickerHost",true);
    box->setStyleSheet(QStringLiteral("QGroupBox[originalPickerHost=\"true\"] { border:none; margin:0; padding:0; }"));
    form->setContentsMargins(0,0,0,0);
    const auto restored=[this,path] {
        PickWidgetData data;
        for(const auto& pick:saved(path)) {
            const auto value=m_legacyContext->toLegacy(pick,MxFunctions::PickDataType::None);
            if(value.first!=MxFunctions::PickDataType::None)data.pickedData[value.first].push_back(value.second);
        }
        return data;
    };
    if(!isDirection) {
        auto* point=new mPointPickerWidget(title,QStringLiteral("模型拾取"),box);
        point->setObjectName("originalPointPicker_"+safeName(path));
        point->setCoordinate(QVector3D(initial[0],initial[1],initial[2]));
        point->setPickData(restored());
        form->addRow(point);
        connect(point,&mPointPickerWidget::coordinateChanged,this,[this,point,path,write] {
            write(point->coordinate());
            record(path,m_legacyContext->resolve(point->pickData().pickedData));
        });
        return;
    }
    const bool gravity=key==QStringLiteral("gravity");
    auto* selector=new mDirectionSelectorWidget(title,gravity,QStringLiteral("大小"),gravity,
        gravity?QStringLiteral("mm/s²"):QString(),false,box,CBRed);
    selector->setObjectName("originalDirectionPicker_"+safeName(path));
    const double magnitude=std::hypot(initial[0],std::hypot(initial[1],initial[2]));
    QVector3D initialVector(initial[0],initial[1],initial[2]);
    if(gravity&&magnitude>0)initialVector/=magnitude;
    const auto state=m_references.value(path+".originalPicker").toObject();
    auto mode=mDirectionSelectorWidget::DirectInput;
    if(state.contains("mode")) {
        const int value=state.value("mode").toInt();
        if(value>=mDirectionSelectorWidget::XAxis&&value<=mDirectionSelectorWidget::DirectInput)
            mode=static_cast<mDirectionSelectorWidget::DirectionMode>(value);
    } else {
        const auto oldMode=m_references.value(path+".mode").toString();
        if(oldMode=="x")mode=mDirectionSelectorWidget::XAxis;
        else if(oldMode=="y")mode=mDirectionSelectorWidget::YAxis;
        else if(oldMode=="z")mode=mDirectionSelectorWidget::ZAxis;
        else if(!saved(path).isEmpty())mode=mDirectionSelectorWidget::SelectObject;
    }
    const bool reversed=state.value("reversed").toBool(m_references.value(path+".reversed").toBool());
    {
        const QSignalBlocker block(selector);
        selector->setDirectionMode(mode);selector->setReversed(reversed);
        if(mode==mDirectionSelectorWidget::CustomCoordinate&&state.value("start").isArray()&&state.value("end").isArray()) {
            const auto start=vector(state.value("start").toArray()),end=vector(state.value("end").toArray());
            selector->setCustomCoordinatePoints(QVector3D(start[0],start[1],start[2]),QVector3D(end[0],end[1],end[2]));
        } else if(mode==mDirectionSelectorWidget::SelectObject)selector->setObjectPickData(restored());
        else selector->setDirection(reversed?-initialVector:initialVector);
        if(gravity)selector->setMagnitudeValue(QString::number(magnitude,'g',17));
        for(auto* controller:{selector->startPickController(),selector->endPickController(),selector->objectPickController()})
            if(controller)controller->setPickActive(false);
    }
    form->addRow(selector);
    const auto update=[this,selector,path,write,gravity] {
        const auto direction=selector->getDirection();
        if(!direction.isValid())return;
        QVector3D value=direction.value<QVector3D>();
        if(gravity) {
            const float length=value.length();
            if(length>0)value=value/length*selector->magnitudeValue().toDouble();
        }
        write(value);
        const auto start=selector->startPoint(),end=selector->endPoint();
        m_references.insert(path+".originalPicker",QJsonObject{{"mode",int(selector->directionMode())},
            {"reversed",selector->isReversed()},{"start",QJsonArray{start.x(),start.y(),start.z()}},
            {"end",QJsonArray{end.x(),end.y(),end.z()}}});
        m_references.remove(path+".mode");m_references.remove(path+".reversed");
        record(path,m_legacyContext->resolve(selector->objectPickData().pickedData));
    };
    connect(selector,&mDirectionSelectorWidget::directionChanged,this,update);
    connect(selector,&mDirectionSelectorWidget::magnitudeChanged,this,update);
}
} // namespace MotionOcc
