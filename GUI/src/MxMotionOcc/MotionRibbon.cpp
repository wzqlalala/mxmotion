#include "MotionWindow.h"

#include "MyTabBar.h"
#include "ThemeManager.h"
#include "UiMetrics.h"
#include "UiPalette.h"
#include "mRibbonButton.h"
#include "mRibbonWidget.h"
#include "mUiMenu.h"
#include "mCalcLineEdit.h"

#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QSignalBlocker>
#include <QSettings>
#include <QStackedWidget>
#include <QVariant>
#include <QVBoxLayout>
#include <initializer_list>
#include <functional>

using namespace MechMainWindow;
using namespace MxFunctions;

namespace MotionOcc {
namespace {
// Menu content comes from the user's original RebuildGUI-wzq_Mech upload,
// MainWindow/mMenuWidget.cpp. Only the receiver changes: the native backend
// implements the original action contract in dispatchOriginalMotionAction().
// Do not collapse independent commands into a generic object-type button.
struct OriginalEntry {
    const char* text;
    const char* icon;
    const char* action = "";
    QVariant argument;
    bool enabled = true;
};

QIcon resourceIcon(const char* path)
{
    return QIcon(QStringLiteral(":/mMainWidgets/Icon/") + QString::fromUtf8(path));
}

QString originalSettingsPath()
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/../Setting/Settings.ini");
}

QString hiddenMenuSettingsKey(const QString& mode)
{
    return QStringLiteral("menubar/hidden")
        + (mode == QLatin1String("Post") ? QStringLiteral("PostMenus")
            : mode == QLatin1String("Curve") ? QStringLiteral("CurveMenus")
            : QStringLiteral("ResidentMenus"));
}

void saveMenuVisibility(const QString& mode, const QMenu* menu)
{
    QSettings settings(originalSettingsPath(), QSettings::IniFormat);
    QStringList hidden;
    for (const auto* action : menu->actions())
        if (action->isCheckable() && !action->isChecked()) hidden.append(action->text());
    settings.setValue(hiddenMenuSettingsKey(mode), hidden);
}

mGroupWidget* group(RibbonPage* page, const char* text)
{
    auto* result = new mGroupWidget(QString::fromUtf8(text), page->contentWidget());
    auto* groups = page->contentLayout();
    groups->insertWidget(groups->count() - 1, result);
    page->appendGroup(result);
    return result;
}

void bindButton(QToolButton* button, QAction* action)
{
    button->setEnabled(action->isEnabled());
    button->setCheckable(action->isCheckable());
    button->setChecked(action->isChecked());
    button->setToolTip(action->text());
    button->setAccessibleName(action->text());
    QObject::connect(button, &QToolButton::clicked, action, &QAction::trigger);
    QObject::connect(action, &QAction::changed, button, [button, action] {
        button->setEnabled(action->isEnabled());
        button->setChecked(action->isChecked());
    });
}

void dispatch(MotionWindow& window, const char* action, const QVariant& argument = {})
{
    window.dispatchOriginalMotionAction(QString::fromLatin1(action), argument);
}

QToolButton* originalButton(mGroupWidget* group, MotionWindow& window,
    const OriginalEntry& entry, int row = -1, int col = -1, bool large = true, int rowSpan = 1)
{
    auto* button = group->addToolBtn(QString::fromUtf8(entry.text), QString(), row, col,
        rowSpan, 1, large ? QSize(30, 30) : QSize(20, 20));
    button->setIcon(resourceIcon(entry.icon));
    button->setText(QString::fromUtf8(entry.text));
    button->setAccessibleName(QString::fromUtf8(entry.text));
    button->setToolTip(QString::fromUtf8(entry.text));
    button->setToolButtonStyle(large ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonTextBesideIcon);
    button->setObjectName(large ? QStringLiteral("narrowButton") : QStringLiteral("wideButton"));
    button->setEnabled(entry.enabled);
    if (*entry.action) {
        button->setProperty("mxOriginalAction", QString::fromLatin1(entry.action));
        button->setProperty("mxOriginalArgument", entry.argument);
        QObject::connect(button, &QToolButton::clicked, &window,
            [&window, action = QByteArray(entry.action), argument = entry.argument] {
                dispatch(window, action.constData(), argument);
            });
    } else {
        // The uploaded GUI contains these display-only entries without a slot.
        button->setProperty("mxOriginalPlaceholder", true);
    }
    if (!entry.enabled) button->setToolTip(QStringLiteral("暂不支持"));
    return button;
}

QAction* originalAction(QMenu* menu, MotionWindow& window, const OriginalEntry& entry)
{
    auto* action = menu->addAction(resourceIcon(entry.icon), QString::fromUtf8(entry.text));
    action->setEnabled(entry.enabled);
    action->setProperty("mxOriginalAction", QString::fromLatin1(entry.action));
    action->setProperty("mxOriginalArgument", entry.argument);
    if (*entry.action) {
        QObject::connect(action, &QAction::triggered, &window,
            [&window, key = QByteArray(entry.action), argument = entry.argument] {
                dispatch(window, key.constData(), argument);
            });
    }
    return action;
}

QMenu* originalMenu(QToolButton* button, MotionWindow& window,
    std::initializer_list<OriginalEntry> entries, bool split = false)
{
    auto* menu = new mUiMenu(button);
    for (const auto& entry : entries) {
        auto* action = originalAction(menu, window, entry);
        if (split) {
            if (menu->actions().size() == 1) {
                button->setDefaultAction(action);
                button->setProperty("dropText", action->text());
            }
            QObject::connect(action, &QAction::triggered, button, [button, action] {
                button->setDefaultAction(action);
                button->setProperty("dropText", action->text());
                button->updateGeometry();
            });
        }
    }
    ThemeManager::applyMenuStyle(menu);
    button->setMenu(menu);
    button->setPopupMode(split ? QToolButton::MenuButtonPopup : QToolButton::InstantPopup);
    static_cast<mRibbonButton*>(button)->setHasArrow(true);
    return menu;
}

QToolButton* originalSplit(mGroupWidget* group, MotionWindow& window,
    std::initializer_list<OriginalEntry> entries, int row = -1, int col = -1, int rowSpan = 1)
{
    const auto& first = *entries.begin();
    OriginalEntry display{first.text, first.icon};
    auto* button = originalButton(group, window, display, row, col, true, rowSpan);
    button->setObjectName(QStringLiteral("comboButton"));
    button->setProperty("mxOriginalPlaceholder", false);
    originalMenu(button, window, entries, true);
    return button;
}

class OriginalRightClickFilter final : public QObject {
public:
    OriginalRightClickFilter(QWidget* parent, std::function<void()> callback)
        : QObject(parent), m_callback(std::move(callback)) { parent->installEventFilter(this); }
protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::MouseButtonPress
            && static_cast<QMouseEvent*>(event)->button() == Qt::RightButton
            && static_cast<QWidget*>(watched)->isEnabled()) {
            m_callback();
            return true;
        }
        return false;
    }
private:
    std::function<void()> m_callback;
};

QToolButton* actionButton(mGroupWidget* group, MotionWindow& window, Command id,
                          bool large = true, int row = -1, int col = -1, const char* image = "")
{
    auto* action = window.command(id);
    auto* button = group->addToolBtn(action->text(), QString(), row, col, large && row >= 0 ? 2 : 1, 1,
        large ? QSize(30, 30) : QSize(20, 20));
    button->setIcon(*image ? resourceIcon(image) : action->icon());
    button->setObjectName(large ? QStringLiteral("narrowButton") : QStringLiteral("wideButton"));
    bindButton(button, action);
    return button;
}

void buildHome(RibbonPage* page, MotionWindow& window)
{
    auto* files = group(page, "文件");
    actionButton(files, window, Command::New, true, 0, 1, "homepage/xinjian3x.png");
    actionButton(files, window, Command::Open, true, 0, 2, "homepage/dakai3x.png");
    actionButton(files, window, Command::Save, false, 0, 4, "homepage/baocun3x.png");
    actionButton(files, window, Command::SaveAs, false, 1, 4, "homepage/lingcunwei3x.png");
    originalButton(files, window, {"导入几何模型", "homepage/daoru3x.png", "slot_importGeoFile"}, 0, 5, false);
    originalButton(files, window, {"导出几何模型", "homepage/daochu3x.png", "slot_exportGeoFile"}, 1, 5, false);
    auto* create = group(page, "创建");
    auto* unit = originalButton(create, window, {"单位", "homepage/danwei3x.png"}, 0, 2, true, 2);
    auto* unitMenu = new mUiMenu(unit);
    const auto unitSection = [&window, unitMenu](const char* title, const char* action,
        const QStringList& options, int selected) {
        auto* heading = unitMenu->addAction(QString::fromUtf8(title));
        heading->setEnabled(false);
        heading->setProperty("mxMenuSectionTitle", true);
        auto* actions = new QActionGroup(unitMenu);
        actions->setExclusive(true);
        for (int i = 0; i < options.size(); ++i) {
            auto* item = unitMenu->addAction(options[i]);
            item->setCheckable(true);
            item->setChecked(i == selected);
            item->setData(i);
            item->setProperty("mxOriginalAction", QString::fromLatin1(action));
            item->setProperty("mxOriginalArgument", i);
            actions->addAction(item);
        }
        QObject::connect(actions, &QActionGroup::triggered, &window,
            [&window, key = QByteArray(action)](QAction* selected) {
                dispatch(window, key.constData(), selected->data());
            });
        return actions;
    };
    auto* unitsGroup = unitSection("单位系统", "switchUnits", {
        "度量标准(m、kg、N、s、V、A)", "度量标准(cm、g、dyne、s、V、A)",
        "度量标准(mm、kg、N、s、mV、mA)", "度量标准(mm、t、N、s、mV、mA)",
        "度量标准(mm、dat、N、s、mV、mA)", "度量标准(μm、kg、μN、s、V、mA)",
        "美国惯用单位(ft、lbm、lbf、°F、s、V、A)", "美国惯用单位(in、lbm、lbf、°F、s、V、A)"}, 3);
    unitSection("角度", "switchAngle", {"度", "弧度"}, 0);
    unitSection("转速", "switchSpeed", {"rad", "RPM"}, 0);
    auto* temperatureGroup = unitSection("温度", "swicthTemp", {"摄氏度", "开尔文"}, 0);
    QObject::connect(unitsGroup, &QActionGroup::triggered, &window,
        [&window, temperatureGroup](QAction* selected) {
            const bool fahrenheit = selected->data().toInt() > 5;
            temperatureGroup->setEnabled(!fahrenheit);
            dispatch(window, "swicthTemp", fahrenheit ? 2
                : temperatureGroup->checkedAction()->data().toInt());
        });
    ThemeManager::applyMenuStyle(unitMenu);
    unit->setMenu(unitMenu);
    unit->setPopupMode(QToolButton::InstantPopup);
    static_cast<mRibbonButton*>(unit)->setHasArrow(true);
    auto* section = originalButton(create, window, {"剖面", "homepage/poumian3x.png"}, 0, 3, true, 2);
    originalMenu(section, window, {
        {"剖切面", "homepage/pouqiemian3x.png", "slot_createSection", 1},
        {"坐标系平面", "homepage/zuobiaoxipoumian3x.png", "slot_createSection", 2},
        {"几何平面", "homepage/jihepingmian3x.png", "slot_createSection", 3}});
    auto* layout = group(page, "布局");
    actionButton(layout, window, Command::ManageLayout, true, 0, 1, "homepage/guanlibuju3x.png");
    auto* reset = originalButton(layout, window, {"重置布局", "homepage/chongzhibuju3x.png"}, 0, 2);
    originalMenu(reset, window, {
        {"布局一", "homepage/bujuyi3x.png", "applyPreset1"},
        {"布局二", "homepage/bujuer3x.png", "applyPreset2"}});
    auto* tools = group(page, "工具");
    actionButton(tools, window, Command::QuickShot, false, 0, 1, "post/kuaisujieping.png");
    originalButton(tools, window, {"自定义截屏", "post/zidingyijieping.png", "clickCustomScreenshotSig"}, 1, 1, false);
}

void buildGeometry(RibbonPage* page, MotionWindow& window)
{
    auto* sketch = originalButton(group(page, "草图"), window, {"草图模式", "geometry/caotu.png"}, 0, 0);
    originalMenu(sketch, window, {
        {"XOY平面", "CenterMainWindow/qianshitu.png", "enterSketch", "XOY"},
        {"XOZ平面", "CenterMainWindow/xiashitu.png", "enterSketch", "XOZ"},
        {"YOZ平面", "CenterMainWindow/zuoshitu.png", "enterSketch", "YOZ"},
        {"手动选择平面", "choice/mian3x.png", "enterSketch", "CUSTOM"},
        {"三点确定平面", "choice/dingdian3x.png", "showThreePointPlanePanel"}});
    auto* solids = group(page, "实体");
    originalSplit(solids, window, {
        {"立方体", "geometry/立方体_大@3x.png", "createGeometry", "CUBE"},
        {"圆柱体", "geometry/圆柱体_大@3x.png", "createGeometry", "CYLINDER"},
        {"球体", "geometry/球体_大@3x.png", "createGeometry", "SPHERE"},
        {"圆锥", "geometry/圆锥_大@3x.png", "createGeometry", "CONOID"},
        {"圆环", "geometry/圆环_大@3x.png", "createGeometry", "TORUS"},
        {"圆台", "geometry/圆台_大@3x.png", "createGeometry", "FRUSTUM"}}, 0, 0);
    auto* features = group(page, "特征");
    originalSplit(features, window, {
        {"拉伸", "geometry/拉伸_大@3x.png", "createGeoFeature", "GeoStretch"},
        {"旋转", "geometry/旋转_大@3x.png", "createGeoFeature", "GeoRotation"}}, 0, 0);
    originalSplit(features, window, {
        {"扫掠", "geometry/扫掠_大@3x.png", "createGeoFeature", "GeoSweep"},
        {"放样", "geometry/放样_大@3x.png", "createGeoFeature", "GeoLoft"},
        {"拔模", "geometry/拔模_大@3x.png", "createGeoFeature", "GeoWithdrawingPattern"},
        {"抽壳", "geometry/抽壳_大@3x.png", "createGeoFeature", "GeoShellRemoval"}}, 0, 1);
    originalButton(features, window, {"倒角", "geometry/daojiao.png", "createGeoFeature", "GeoChamfering"}, 0, 2);
    auto* transforms = group(page, "变换");
    originalSplit(transforms, window, {
        {"移动", "geometry/移动_大@3x.png", "moveGeometryPart"},
        {"平移", "geometry/平移_大@3x.png", "createGeoTransform", "GeoOffset"},
        {"旋转", "geometry/旋转_大@3x-2.png", "createGeoTransform", "GeoRotate"},
        {"镜像", "geometry/镜像_大@3x.png", "createGeoTransform", "GeoMirrorimage"},
        {"缩放", "geometry/缩放_大@3x.png", "createGeoTransform", "GeoZoom"},
        {"定位", "geometry/定位_大@3x.png", "createGeoTransform", "GeoLocation"}}, 0, 0, 2);
    originalButton(transforms, window, {"线性阵列", "geometry/xianxingzhenglie.png", "createGeoTransform", "GeoLinearArray"}, 0, 1, false);
    originalButton(transforms, window, {"环形阵列", "geometry/huanxingzhenglie.png", "createGeoTransform", "GeoRingArray"}, 1, 1, false);
    originalSplit(group(page, "布尔操作"), window, {
        {"布尔求和", "geometry/布尔求和_大@3x.png", "createGeoBool", "GeoSummation"},
        {"布尔求差", "geometry/布尔求差_大@3x.png", "createGeoBool", "GeoSusubtrac"},
        {"布尔求交", "geometry/布尔求交_大@3x.png", "createGeoBool", "GeoInterse"}}, 0, 0);
    auto* extracts = group(page, "提取");
    originalSplit(extracts, window, {
        {"流体域", "geometry/流体域_大@3x.png", "createGeoExtract", "GeoFluid"},
        {"中线", "geometry/中线_大@3x.png", "createGeoExtract", "GeoMidLine"},
        {"边界面", "geometry/边界面_大@3x.png", "createGeoExtract", "GeoBoundaryFace"}}, 0, 0);
    originalButton(extracts, window, {"中面", "geometry/midSurface.png", "createGeoExtract", "GeoMidFace"}, 0, 2);
    auto* edits = group(page, "几何编辑");
    originalSplit(edits, window, {
        {"分割线", "geometry/分割线_大@3x.png", "createGeoEdit", "GeoSplitLine"},
        {"分割面", "geometry/分割面_大@3x.png", "createGeoEdit", "GeoSplitFace"},
        {"分割体", "geometry/分割体_大@3x.png", "createGeoEdit", "GeoSplitSolid"}});
    originalSplit(edits, window, {
        {"合并线", "geometry/合并线_大@3x.png", "createGeoEdit", "GeoMergeLine"},
        {"合并面", "geometry/合并面_大@3x.png", "createGeoEdit", "GeoMergeFace"},
        {"合并体", "geometry/合并体_大@3x.png", "createGeoEdit", "GeoMergeSolid"}});
    originalSplit(edits, window, {
        {"删除面", "geometry/删除面_大@3x.png", "createGeoEdit", "GeoDeleteFace"},
        {"删除体", "geometry/删除体_大@3x.png", "createGeoEdit", "GeoDeleteSolid"},
        {"删除线", "geometry/删除线_大@3x.png", "createGeoEdit", "GeoDeleteLine"}});
    originalSplit(edits, window, {
        {"短边", "geometry/短边_大@3x.png", "createGeoEdit", "GeoCheckShortEdge"},
        {"锐边", "geometry/锐边_大@3x.png", "createGeoEdit", "GeoCheckSharpEdge"},
        {"开放边", "geometry/检查开放边_大@3x.png", "createGeoEdit", "GeoCheckOpenEdge"},
        {"小型面", "geometry/小型面_大@3x.png", "createGeoEdit", "GeoCheckSmallFace"},
        {"微小实体", "geometry/检查微小实体_大@3x.png", "createGeoEdit", "GeoCheckTinySolid"},
        {"干涉检查", "geometry/几何干涉检查_大@3x.png", "createGeoEdit", "GeoInterferenceCheck"}});
    originalSplit(edits, window, {
        {"取消裁剪面", "geometry/取消裁剪面_大@3x.png", "createGeoEdit", "GeoCancelTrimFace"},
        {"线连接", "geometry/线连接_大@3x.png", "createGeoEdit", "GeoLineCombine"},
        {"线延长", "geometry/线延长_大@3x.png", "createGeoEdit", "GeoLineLengthen"},
        {"面延伸", "geometry/面延伸_大@3x.png", "createGeoEdit", "GeoFaceExtend"},
        {"面偏置", "geometry/面偏置_大@3x.png", "createGeoEdit", "GeoFaceOffset"},
        {"修补缝隙", "geometry/修补缝隙_大@3x.png", "createGeoEdit", "GeoRepairCracks"},
        {"填充曲面", "geometry/面延伸_大@3x.png", "createGeoEdit", "GeoFillSurface"}}, 0, 4, 2);
    originalButton(edits, window, {"特征投影", "geometry/project.png", "createGeoEdit", "GeoFeatureProjection"}, 0, 5, false);
    originalSplit(group(page, "几何修复"), window, {
        {"分割边", "geometry/修复分割边_大@3x.png", "createGeoRepair", "GeoRepairSplitEdge"},
        {"额外边", "geometry/额外边_大@3x.png", "createGeoRepair", "GeoRepairExtraEdge"},
        {"重复面", "geometry/重复面_大@3x.png", "createGeoRepair", "GeoRemoveRepeatFace"},
        {"硬点增删", "geometry/硬点删除_大@3x.png", "createGeoRepair", "GeoHardPoint"},
        {"重复曲线", "geometry/重复曲线_大@3x.png", "createGeoRepair", "GeoRemoveDuplicateLines"}}, 0, 0);
    auto* solidify = group(page, "几何实体化");
    originalButton(solidify, window, {"拼接", "geometry/pingjie.png", "createGeoSolidify", "GeoStitch"}, 0, 0, false);
    originalButton(solidify, window, {"间距", "geometry/jianju.png", "createGeoSolidify", "GeoGap"}, 0, 1, false);
    originalButton(solidify, window, {"缺失面", "geometry/queshimian.png", "createGeoSolidify", "GeoMissingFace"}, 1, 0, false);
    originalButton(group(page, "特征去除"), window, {"特征去除", "geometry/zidongquchu.png", "createGeoRemoval", "GeoAutoRemoval"});
    auto* share = group(page, "共享拓扑");
    originalButton(share, window, {"共享", "geometry/共享拓扑_大@3x.png", "createGeoSharedTopology", "GeoSharedTopology"});
    originalButton(share, window, {"自动共享", "geometry/自动共享_大@3x.png", "autoSharedTopologyOCC"}, 0, 1, true, 2);
}

void buildMotionPages(const QList<RibbonPage*>& pages, MotionWindow& window)
{
    auto* solve = group(pages[7], "求解");
    auto* bodies = group(pages[2], "体");
    auto* joints = group(pages[3], "基本运动副 ▼");
    auto* constraints = group(pages[3], "几何约束 ▼");
    auto* couplings = group(pages[3], "耦合副 ▼");
    auto* jointDrives = group(pages[4], "运动副驱动");
    auto* bodyDrives = group(pages[4], "运动体驱动");
    auto* flexible = group(pages[5], "柔性连接 ▼");
    auto* forces = group(pages[5], "加载力 ▼");
    auto* sensors = group(pages[6], "传感器");
    auto* controllers = group(pages[6], "控制器");
    auto* control = group(pages[6], "控制模块 ▼");
    auto* aircraft = group(pages[2], "飞机专用模块 ▼");
    auto* systems = group(pages[2], "系统建模工具 ▼");

    originalButton(solve, window, {"新建求解方案", "Motion/drive.png", "createMotionScenario"});
    actionButton(solve, window, Command::Solve);
    // These are previously requested additions; the original two commands keep
    // their positions and meanings.
    actionButton(solve, window, Command::StopSolve);

    originalButton(bodies, window, {"一键生成运动体", "xuanzequanbu.png", "createMotionBodyOneKey"});
    originalButton(bodies, window, {"批量定义运动体", "Motion/bodyBatch.png", "createMotionBodyBatch"});
    originalButton(bodies, window, {"运动体", "Motion/movingBody.png", "createMotionBody"});
    originalButton(bodies, window, {"自动柔性体", "Motion/mnf.png", "createMotionAutoFlex"});
    originalButton(bodies, window, {"导入 MNF…", "Motion/mnf.png", "importMotionMnf"});
    originalButton(bodies, window, {"标记点", "caotu/dian.png", "createMotionMarker"});

    originalButton(jointDrives, window, {"位置驱动", "Motion/drive.png", "createMotionDrive", "Position"});
    originalButton(jointDrives, window, {"速度驱动", "Motion/drive.png", "createMotionDrive", "Velocity"});
    originalButton(jointDrives, window, {"加速度驱动", "Motion/drive.png", "", {}, false});
#if defined(MX_HAS_BODY_DRIVES)
    originalSplit(bodyDrives, window, {
        {"平移位移", "Motion/drive.png", "createBodyDrive", "LinearPosition"},
        {"线速度", "Motion/drive.png", "createBodyDrive", "LinearSpeed"},
        {"转角", "Motion/drive.png", "createBodyDrive", "RotationAngle"},
        {"角速度", "Motion/drive.png", "createBodyDrive", "RotationSpeed"}});
    originalButton(bodyDrives, window, {"六自由度位姿", "Motion/drive.png", "createBodyDrive", "Pose"});
#else
    originalButton(bodyDrives, window, {"单轴驱动", "Motion/drive.png", "", {}, false});
    originalButton(bodyDrives, window, {"六自由度位姿", "Motion/drive.png", "", {}, false});
#endif

    const OriginalEntry commonJoints[] = {
        {"旋转副", "Motion/joint_revolute.png", "createMotionJoint", "Revolute"},
        {"移动副", "Motion/joint_prismatic.png", "createMotionJoint", "Translational"},
        {"柱面副", "Motion/joint_cylindrical.png", "createMotionJoint", "Cylindrical"},
        {"球面副", "Motion/joint_spherical.png", "createMotionJoint", "Spherical"}};
    for (int i = 0; i < 4; ++i) originalButton(joints, window, commonJoints[i], 0, i, true, 2);
    const OriginalEntry remainingJoints[] = {
        {"固定副", "Motion/joint_fixed.png", "createMotionJoint", "Fixed"},
        {"万向节", "Motion/joint_universal.png", "createMotionJoint", "Universal"},
        {"恒速副", "Motion/joint_cv.png", "createMotionJoint", "ConstantVelocity"},
        {"螺纹副", "Motion/joint_screw.png", "createMotionJoint", "Screw"},
        {"平面副", "Motion/joint_planar.png", "createMotionJoint", "Planar"},
        {"点线副", "Motion/joint_pointLine.png", "createMotionJoint", "PointLine"},
        {"点面副", "Motion/joint_pointPlane.png", "createMotionJoint", "PointPlane"}};
    for (int i = 0; i < 7; ++i) originalButton(joints, window, remainingJoints[i], i % 2, 4 + i / 2, false);
    const OriginalEntry geometricJoints[] = {
        {"点在面上副", "Motion/joint_pointOnSurface.png", "createMotionJoint", "PointOnSurface"},
        {"点在线上副", "Motion/joint_pointOnCurve.png", "createMotionJoint", "PointOnCurve"},
        {"线在线上副", "Motion/joint_curveOnCurve.png", "createMotionJoint", "CurveOnCurve"},
        {"平行约束", "Motion/joint_parallel.png", "createMotionJoint", "Parallel"},
        {"垂直约束", "Motion/joint_perpendicular.png", "createMotionJoint", "Perpendicular"},
        {"方向约束", "Motion/joint_direction.png", "createMotionJoint", "Direction"}};
    for (int i = 0; i < 6; ++i) originalButton(constraints, window, geometricJoints[i], i % 2, i / 2, false);
    originalButton(couplings, window, {"齿轮副", "Motion/coupling_gear.png", "createMotionCoupling", "Gear"}, 0, 0, true, 2);
    originalButton(couplings, window, {"齿轮齿条副", "Motion/coupling_gearRack.png", "createMotionCoupling", "GearRack"}, 0, 1, true, 2);
    originalButton(couplings, window, {"线缆副", "Motion/coupling_cable.png", "createMotionCoupling", "Cable"}, 0, 2, true, 2);

    originalButton(flexible, window, {"体点弹簧", "Motion/force_spring.png", "createMotionSpring", "BodyPoint"}, 0, 0, false);
    originalButton(flexible, window, {"拉压弹簧", "Motion/tcSpring.png", "createMotionSpring", "Translational"}, 0, 1, false);
    originalButton(flexible, window, {"梁单元", "Motion/beam.png", "", {}, false}, 1, 0, false);
    originalButton(flexible, window, {"扭转弹簧", "Motion/torsionSpring.png", "createMotionSpring", "Torsional"}, 0, 2, false);
    originalButton(flexible, window, {"轴承", "Motion/axletree.png", "", {}, false}, 1, 1, false);
    originalButton(flexible, window, {"止挡", "Motion/force_spring.png", "createMotionSoftStop"}, 0, 3, false);
    originalButton(flexible, window, {"只拉绳索", "Motion/coupling_cable.png", "createMotionTensionCable"}, 1, 3, false);

    const OriginalEntry loads[] = {
        {"矢量力", "Motion/force_vector.png", "createMotionVectorForce", "Force"},
        {"矢量力矩", "Motion/force_torque.png", "createMotionVectorForce", "Torque"},
        {"三点力", "Motion/force_tri.png", "createMotionVectorForce", "TriForce"},
        {"三点力矩", "Motion/force_tri_torque.png", "createMotionVectorForce", "TriTorque"},
        {"标量力", "Motion/force.png", "createMotionVectorForce", "ScalarForce"},
        {"标量力矩", "Motion/moment.png", "createMotionVectorForce", "ScalarTorque"},
        {"接触力", "Motion/contact.png", "createMotionContact", 0},
        {"衬套", "Motion/bush.png", "createMotionVectorForce", "Bushing"}};
    for (int i = 0; i < 8; ++i) originalButton(forces, window, loads[i], 0, i, false);
    const OriginalEntry originalDisabledLoads[] = {
        {"重力", "Motion/force_gravity.png", "", {}, false},
        {"力场", "Motion/force_field.png", "", {}, false},
        {"一般作用力", "Motion/force_general.png", "", {}, false},
        {"线性弹簧阻尼", "Motion/force_spring.png", "", {}, false},
        {"轮胎力", "Motion/force_tire.png", "", {}, false},
        {"电磁力", "Motion/electromagnetism.png", "", {}, false},
        {"自定义力", "Motion/custom.png", "", {}, false}};
    for (int i = 0; i < 7; ++i) originalButton(forces, window, originalDisabledLoads[i], 1, i, false);

    const OriginalEntry sensorItems[] = {
        {"体位移", "Motion/sensor_displacement.png", "createMotionSensor", "BodyDisplacement"},
        {"体速度", "Motion/sensor_velocity.png", "createMotionSensor", "BodyVelocity"},
        {"体加速度", "Motion/sensor_acceleration.png", "createMotionSensor", "BodyAcceleration"},
        {"关节反力", "Motion/sensor_jointReaction.png", "createMotionSensor", "JointReaction"},
        {"驱动输出", "Motion/sensor_driveActuation.png", "createMotionSensor", "DriveActuation"},
        {"距离", "Motion/sensor_distance.png", "createMotionSensor", "Distance"},
        {"角度", "Motion/sensor_angle.png", "createMotionSensor", "Angle"}};
    for (int i = 0; i < 7; ++i) originalButton(sensors, window, sensorItems[i], i / 4, i % 4, false);
    originalButton(controllers, window, {"控制器", "Motion/controlCalculation.png", "createMotionController"});
    originalButton(control, window, {"控制输入", "Motion/controlInput.png", "createControlInput"}, 0, 0, false);
    auto* electro = originalButton(control, window, {"机电耦合", "Motion/electromechanicalCoupling.png"}, 1, 0, false);
    auto* electroMenu = originalMenu(electro, window, {
        {"机电导入", "Motion/mech_electro_import.png", "createElectroImport"},
        {"机电导出", "Motion/mech_electro_export.png", "createElectroExport"}});
    electroMenu->addSeparator();
    originalAction(electroMenu, window, {"液压联合仿真", "Motion/mech_hydraulic.png", "createHydraulic"});
    originalButton(control, window, {"强化学习", "Motion/controlCalculation.png", "createRLPanel"}, 0, 1, false);
    originalButton(control, window, {"控制输出", "Motion/controlOutput.png"}, 1, 1, false);

    originalButton(aircraft, window, {"起落架", "Motion/undercart.png"}, 0, 0, false);
    originalButton(aircraft, window, {"轮胎", "Motion/force_tire.png"}, 1, 0, false);
    originalButton(aircraft, window, {"路面", "Motion/pavement.png"}, 0, 1, false);
    originalButton(aircraft, window, {"报告生成", "Motion/aircraftReport.png"}, 1, 1, false);
    originalButton(systems, window, {"机构", "Motion/rotaryJoint.png", "createMechanism"}, 0, 0, false);
    originalButton(systems, window, {"传动链", "Motion/GearPair.png", "createTransmission"}, 1, 0, false);
    originalButton(systems, window, {"机电液", "Motion/mech_elec_hyd.png", "createMechElecHyd"}, 0, 1, false);
}

void buildDisplay(RibbonPage* page, MotionWindow& window)
{
    auto* views = group(page, "视图");
    auto* step = originalButton(views, window, {"步骤", "display/buzhou.png", "slot_changeViewList", false}, 0, 0, true, 2);
    step->setToolTip(QStringLiteral("左键：上一个视角\n右键：下一个视角"));
    new OriginalRightClickFilter(step, [&window] { dispatch(window, "slot_changeViewList", true); });
    const auto spin = [views](const QString& name, double low, double high, int decimals, double value, int width) {
        auto* edit = new QDoubleSpinBox(views);
        edit->setObjectName(name);
        edit->setButtonSymbols(QAbstractSpinBox::NoButtons);
        edit->setRange(low, high);
        edit->setDecimals(decimals);
        edit->setValue(value);
        edit->setMinimumWidth(width);
        edit->setFixedHeight(UiMetrics::menuItemHeight());
        return edit;
    };
    const auto addSpin = [views](QDoubleSpinBox* edit, const char* unit, int col) {
        auto* row = new QHBoxLayout;
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(edit);
        auto* label = new QLabel(QString::fromUtf8(unit));
        label->setFixedWidth(15);
        row->addWidget(label);
        auto* box = new QVBoxLayout;
        box->setContentsMargins(0, 0, 0, 0);
        box->addStretch();
        box->addLayout(row);
        views->addBoxLayout(box, 0, col, 2, 1);
    };
    auto* angle = spin(QStringLiteral("originalViewAngle"), 0.1, 359, 1, 30, 46);
    const char* axes[] = {"旋转Sx", "旋转Sy", "旋转Sz"};
    const char* icons[] = {"display/xuanzhuanSx.png", "display/xuanzhuanSy.png", "display/xuanzhuanSz.png"};
    for (int i = 0; i < 3; ++i) {
        auto* button = originalButton(views, window, {axes[i], icons[i]}, 0, 1 + i, true, 2);
        button->setProperty("mxOriginalPlaceholder", false);
        button->setProperty("mxOriginalAction", QStringLiteral("slot_angleRotation"));
        const QString axis = QStringLiteral("S%1").arg(QChar('x' + i));
        const auto rotate = [&window, angle, axis](bool reverse) {
            dispatch(window, "slot_angleRotation", QVariantList{(reverse ? QStringLiteral("旋转-") : QStringLiteral("旋转+")) + axis, angle->value()});
        };
        QObject::connect(button, &QToolButton::clicked, &window, [rotate] { rotate(false); });
        new OriginalRightClickFilter(button, [rotate] { rotate(true); });
        button->setToolTip(QStringLiteral("左键：绕屏幕%1轴正向旋转\n右键：绕屏幕%1轴反向旋转").arg(QChar('x' + i)));
    }
    addSpin(angle, "度", 4);
    auto* pan = spin(QStringLiteral("originalViewPanPercent"), 1, 100, 0, 10, 40);
    const auto panButton = [&window, views, pan](const char* title, const char* icon, const char* direction, int row, int col) {
        auto* button = originalButton(views, window, {title, icon}, row, col, false);
        button->setProperty("mxOriginalPlaceholder", false);
        button->setProperty("mxOriginalAction", QStringLiteral("slot_directionPan"));
        QObject::connect(button, &QToolButton::clicked, &window, [&window, pan, direction] {
            dispatch(window, "slot_directionPan", QVariantList{QString::fromLatin1(direction), pan->value()});
        });
    };
    panButton("向上", "display/xiangshang.png", "up", 0, 6);
    panButton("向下", "display/xiangxia.png", "down", 1, 6);
    panButton("向左", "display/xiangzuo.png", "left", 1, 5);
    panButton("向右", "display/xiangyou.png", "right", 1, 7);
    addSpin(pan, "%", 8);
    auto* zoom = spin(QStringLiteral("originalViewZoomFactor"), 0.1, 100, 1, 2, 42);
    auto* zoomButton = originalButton(views, window, {"缩放", "display/suofang.png"}, 0, 9, true, 2);
    zoomButton->setProperty("mxOriginalPlaceholder", false);
    zoomButton->setProperty("mxOriginalAction", QStringLiteral("slot_valueZoom"));
    zoomButton->setToolTip(QStringLiteral("左键：放大\n右键：缩小"));
    QObject::connect(zoomButton, &QToolButton::clicked, &window, [&window, zoom] {
        dispatch(window, "slot_valueZoom", QVariantList{true, zoom->value()});
    });
    new OriginalRightClickFilter(zoomButton, [&window, zoom] {
        dispatch(window, "slot_valueZoom", QVariantList{false, zoom->value()});
    });
    addSpin(zoom, "倍", 10);
    auto* rotate = originalButton(views, window, {"自动旋转", "display/zidongxuanzhuang.png"}, 0, 11, true, 2);
    rotate->setObjectName(QStringLiteral("comboButton"));
    rotate->setProperty("mxOriginalPlaceholder", false);
    rotate->setProperty("mxOriginalAction", QStringLiteral("slot_autoRotation"));
    rotate->setProperty("dropText", QStringLiteral("+Sx"));
    rotate->setCheckable(true);
    auto* rotateMenu = new mUiMenu(rotate);
    auto* rotateAxes = new QActionGroup(rotateMenu);
    rotateAxes->setExclusive(true);
    for (const QString& axis : {QStringLiteral("+Sx"), QStringLiteral("-Sx"), QStringLiteral("+Sy"), QStringLiteral("-Sy"), QStringLiteral("+Sz"), QStringLiteral("-Sz")}) {
        auto* item = rotateMenu->addAction(axis);
        item->setCheckable(true);
        item->setChecked(axis == QStringLiteral("+Sx"));
        rotateAxes->addAction(item);
        QObject::connect(item, &QAction::triggered, &window, [&window, rotate, axis] {
            rotate->setProperty("dropText", axis);
            rotate->update();
            if (rotate->isChecked()) dispatch(window, "slot_autoRotation", QVariantList{axis, true});
        });
    }
    rotate->setMenu(rotateMenu);
    rotate->setPopupMode(QToolButton::MenuButtonPopup);
    static_cast<mRibbonButton*>(rotate)->setHasArrow(true);
    ThemeManager::applyMenuStyle(rotateMenu);
    QObject::connect(rotate, &QToolButton::toggled, &window, [&window, rotate](bool active) {
        dispatch(window, "slot_autoRotation", QVariantList{rotate->property("dropText").toString(), active});
    });

    // The original Motion product hides the FEM entity-representation group.
    auto* numbers = group(page, "编号");
    originalButton(numbers, window, {"显示编号", "display/xianshiID.png", "slot_showNodeEleLabel"}, 0, 1, true, 2);
    originalButton(numbers, window, {"隐藏编号", "display/yincangID.png", "slot_hideNodeEleLabel"}, 0, 2, false);
    originalButton(numbers, window, {"隐藏全部编号", "display/yincangquanbu.png", "slot_hideAllNodeEleLabel"}, 1, 2, false);
    auto* free = group(page, "自由节点");
    originalButton(free, window, {"显示自由节点", "display/xianshiziyoujiedian.png", "slot_showFreeNode"}, 0, 0, false);
    originalButton(free, window, {"隐藏自由节点", "display/yincangziyoujiedian.png", "slot_hideFreeNode"}, 1, 0, false);
    originalButton(free, window, {"显示全部自由节点", "display/quanbuziyoujiedian.png", "slot_showAllFreeNode"}, 0, 1, false);
    originalButton(free, window, {"隐藏全部自由节点", "display/yincangquanbuziyoujiedian.png", "slot_hideAllFreeNode"}, 1, 1, false);
    originalButton(free, window, {"删除全部自由节点", "display/yincangquanbuziyoujiedian.png", "slot_deleteAllFreeNode"}, 0, 2, false);
    originalButton(free, window, {"删除自由节点", "display/yincangziyoujiedian.png", "slot_deleteFreeNode"}, 1, 2, false);
    auto* models = group(page, "模型");
    originalButton(models, window, {"隐藏当前", "display/yincangdangqian.png", "slot_maskModel"}, 0, 1, false);
    originalButton(models, window, {"隐藏其他", "display/yincangqita.png", "slot_maskOtherModel"}, 1, 1, false);
    originalButton(models, window, {"反转显示", "display/fanzhuanxianshi.png", "slot_reverseMaskModel"}, 0, 2, false);
    originalButton(models, window, {"显示全部", "display/xianshiquanbu.png", "slot_showAllModel"}, 1, 2, false);
}

void buildSelection(RibbonPage* page, MotionWindow& window)
{
    auto* extend = group(page, "扩展");
    auto* features = group(page, "特征");
    auto* position = group(page, "位置尺寸");
    auto* mesh = group(page, "网格");
    auto* conversion = group(page, "转换");
    auto* batch = group(page, "批量操作");
    originalButton(extend, window, {"相邻", "choice/xianglin3x.png", "slot_adjacentFaceLine", false});
    originalButton(extend, window, {"限值", "choice/xianzhi3x.png", "slot_adjacentFaceLine", true}, 0, 0, false);
    originalButton(extend, window, {"连接", "choice/lianjie3x.png", "slot_extendConnectFace"}, 1, 0, false);
    originalButton(features, window, {"公共边", "choice/gonggongbian3x.png", "slot_faceCommonEdge"}, 0, 0, false);
    originalButton(features, window, {"圆柱面", "choice/yuanzhumian3x.png", "slot_cylinderFaceSelect"}, 1, 0, false);
    originalButton(features, window, {"凹陷", "choice/aoxian.png", "slot_sunkenRaisedFaceSelect", true}, 0, 1, false);
    originalButton(features, window, {"凸起", "choice/tuqi.png", "slot_sunkenRaisedFaceSelect", false}, 1, 1, false);

    auto* locations = originalButton(position, window, {"位置", "choice/weizhi3x.png"});
    originalMenu(locations, window, {
        {"选择X位置相同的所有实体", "choice/Xshiti3x.png", "slot_sameLocationGeoSelect", 0},
        {"选择Y位置相同的所有实体", "choice/Yshiti3x.png", "slot_sameLocationGeoSelect", 1},
        {"选择Z位置相同的所有实体", "choice/Zshiti3x.png", "slot_sameLocationGeoSelect", 2},
        {"选择具有相同X位置的所有共面实体", "choice/XGMshiti3x.png", "slot_sameLocationFaceGeoSelect", 0},
        {"选择具有相同Y位置的所有共面实体", "choice/YGMshiti3x.png", "slot_sameLocationFaceGeoSelect", 1},
        {"选择具有相同Z位置的所有共面实体", "choice/ZGMshiti3x.png", "slot_sameLocationFaceGeoSelect", 2}});

    auto* fromSelected = originalButton(position, window, {"基于已选选择实体", "choice/JYYXZST3x.png"}, 0, 0, false);
    fromSelected->setProperty("mxOriginalPlaceholder", false);
    fromSelected->setProperty("mxOriginalAction", QStringLiteral("slot_sizeGeoSelect"));
    auto* relative = position->addCombobox(QStringLiteral("select1"), 0, 1);
    relative->addItems({"大于", "等于", "小于"});
    relative->setMinimumWidth(64);
    relative->setFixedHeight(UiMetrics::menuItemHeight());
    relative->setCurrentIndex(1);
    QObject::connect(fromSelected, &QToolButton::clicked, &window, [&window, relative] {
        dispatch(window, "slot_sizeGeoSelect", relative->currentIndex());
    });
    auto* bySize = originalButton(position, window, {"基于尺寸选择实体", "choice/JYCCXZST3x.png"}, 1, 0, false);
    bySize->setProperty("mxOriginalPlaceholder", false);
    bySize->setProperty("mxOriginalAction", QStringLiteral("slot_sizeGeoSelect"));
    auto* kind = position->addCombobox(QStringLiteral("select2"), 1, 1);
    kind->addItems({"几何线", "几何面", "几何体"});
    kind->setMinimumWidth(76);
    kind->setFixedHeight(UiMetrics::menuItemHeight());
    QObject::connect(kind, qOverload<int>(&QComboBox::currentIndexChanged), &window,
        [&window](int value) { dispatch(window, "activePickSig", value + 2); });
    auto* lower = new mCalcLineEdit;
    position->addLineEdit(lower, 1, 2);
    lower->setMinimumWidth(56);
    lower->setFixedHeight(UiMetrics::menuItemHeight());
    auto* relation = position->addCombobox(QStringLiteral("select3"), 1, 3);
    relation->addItems({"大于", "等于", "小于", "之间"});
    relation->setCurrentIndex(3);
    relation->setMinimumWidth(64);
    relation->setFixedHeight(UiMetrics::menuItemHeight());
    QObject::connect(relation, qOverload<int>(&QComboBox::currentIndexChanged), lower,
        [lower](int index) { lower->setEnabled(index == 3); });
    auto* upper = new mCalcLineEdit;
    position->addLineEdit(upper, 1, 4);
    upper->setMinimumWidth(56);
    upper->setFixedHeight(UiMetrics::menuItemHeight());
    QObject::connect(bySize, &QToolButton::clicked, &window, [&window, kind, relation, lower, upper] {
        dispatch(window, "slot_sizeGeoSelect", QVariantList{kind->currentText(), relation->currentIndex(),
            lower->text().toDouble(), upper->text().toDouble()});
    });

    originalButton(mesh, window, {"增长单元", "choice/zengzhangdanyuan3x.png", "slot_growEles"});
    auto* nodes = originalButton(mesh, window, {"按照Id选择节点", "choice/IdChoicePoint3x.png"}, 0, 0, false);
    auto* elements = originalButton(mesh, window, {"按照Id选择单元", "choice/IdChoiceUnit3x.png"}, 1, 0, false);
    auto* nodeIds = new mCalcLineEdit;
    auto* elementIds = new mCalcLineEdit;
    nodeIds->setObjectName(QStringLiteral("originalNodeIdInput"));
    elementIds->setObjectName(QStringLiteral("originalElementIdInput"));
    mesh->addLineEdit(nodeIds, 0, 1);
    mesh->addLineEdit(elementIds, 1, 1);
    for (auto* edit : {nodeIds, elementIds}) {
        edit->setMinimumWidth(110);
        edit->setFixedHeight(UiMetrics::menuItemHeight());
    }
    nodes->setProperty("mxOriginalPlaceholder", false);
    nodes->setProperty("mxOriginalAction", QStringLiteral("slot_nodeIDSelect"));
    elements->setProperty("mxOriginalPlaceholder", false);
    elements->setProperty("mxOriginalAction", QStringLiteral("slot_eleIDSelect"));
    QObject::connect(nodes, &QToolButton::clicked, &window, [&window, nodeIds] { dispatch(window, "slot_nodeIDSelect", nodeIds->text()); });
    QObject::connect(elements, &QToolButton::clicked, &window, [&window, elementIds] { dispatch(window, "slot_eleIDSelect", elementIds->text()); });

    auto* edgeAngle = mesh->addDoubleSpinBox(QStringLiteral("通过边角度"), 0, 3, 1, 1);
    auto* faceAngle = mesh->addDoubleSpinBox(QStringLiteral("通过面角度"), 1, 3, 1, 1);
    for (auto* edit : {edgeAngle, faceAngle}) {
        edit->setRange(0, 180);
        edit->setValue(60);
        edit->setMinimumWidth(80);
        edit->setFixedHeight(UiMetrics::menuItemHeight());
    }
    QObject::connect(edgeAngle, qOverload<double>(&QDoubleSpinBox::valueChanged), &window,
        [&window](double value) { dispatch(window, "setPickAssistLineAngle", value); });
    QObject::connect(faceAngle, qOverload<double>(&QDoubleSpinBox::valueChanged), &window,
        [&window](double value) { dispatch(window, "setPickAssistFaceAngle", value); });
    auto* byEdge = originalButton(mesh, window, {"通过边角度", "choice/bianjiaodu.png"}, 0, 2, false);
    auto* byFace = originalButton(mesh, window, {"通过面角度", "choice/mianjiaodu.png"}, 1, 2, false);
    auto* path = originalButton(mesh, window, {"路径", "choice/lujing.png"}, 0, 4, true, 2);
    const QList<QToolButton*> assists{byEdge, byFace, path};
    const QStringList modes{QStringLiteral("LineAngle"), QStringLiteral("FaceAngle"), QStringLiteral("Path")};
    for (int i = 0; i < assists.size(); ++i) {
        auto* button = assists[i];
        const auto mode = modes[i];
        button->setCheckable(true);
        button->setProperty("mxOriginalPlaceholder", false);
        button->setProperty("mxOriginalAction", QStringLiteral("setPickAssistMode"));
        button->setProperty("mxOriginalArgument", mode);
        QObject::connect(button, &QToolButton::toggled, &window,
            [&window, mesh, assists, button, mode, edgeAngle, faceAngle](bool on) {
                if (on) {
                    mesh->setProperty("assistMode", mode);
                    for (auto* other : assists) if (other != button) {
                        QSignalBlocker block(other);
                        other->setChecked(false);
                    }
                    if (mode == QStringLiteral("LineAngle")) dispatch(window, "setPickAssistLineAngle", edgeAngle->value());
                    if (mode == QStringLiteral("FaceAngle")) dispatch(window, "setPickAssistFaceAngle", faceAngle->value());
                    dispatch(window, "setPickAssistMode", mode);
                    dispatch(window, "applyPickAssist");
                } else if (mesh->property("assistMode").toString() == mode) {
                    mesh->setProperty("assistMode", QStringLiteral("None"));
                    dispatch(window, "setPickAssistMode", QStringLiteral("None"));
                }
            });
    }

    originalButton(conversion, window, {"顶点", "choice/zhuan_jihedian.png", "slot_mappingGeoPoint"}, 1, 1, false);
    originalButton(conversion, window, {"边", "choice/zhuan_jihexian.png", "slot_mappingGeoEdge"}, 0, 1, false);
    originalButton(conversion, window, {"面", "choice/zhuan_jihemian.png", "slot_mappingGeoFace"}, 1, 0, false);
    originalButton(conversion, window, {"几何体", "choice/zhuan_jiheti.png", "slot_mappingGeoSolid"}, 0, 0, false);
    originalButton(conversion, window, {"节点", "choice/zhuan_jiedian.png", "slot_mappingNode"}, 1, 3, false);
    originalButton(conversion, window, {"单元边", "choice/zhuan_danyuanbian.png", "slot_mappingMeshLine"}, 0, 3, false);
    originalButton(conversion, window, {"单元面", "choice/zhuan_danyuanmian.png", "slot_mappingMeshFace"}, 1, 2, false);
    originalButton(conversion, window, {"单元", "choice/zhuan_danyuan.png", "slot_mappingEle"}, 0, 2, false);
    originalButton(conversion, window, {"部件", "choice/zhuan_bujian.png", "slot_mappingPart"}, 0, 4, false);
    originalButton(batch, window, {"反向", "choice/fanxiang3x.png", "slot_reverse"}, 0, 0, true, 2);
    originalButton(batch, window, {"清空", "choice/clear.png", "clearPick"}, 0, 1, true, 2);
}
} // namespace

QWidget* createMotionRibbon(MotionWindow& window, QWidget* parent)
{
    auto* ribbon = new QWidget(parent);
    ribbon->setObjectName(QStringLiteral("mMenuWidget"));
    auto* rows = new QVBoxLayout(ribbon);
    rows->setContentsMargins(0, 0, 0, 0);
    rows->setSpacing(0);
    auto* top = new QWidget(ribbon);
    auto* buttons = new QHBoxLayout(top);
    buttons->setSpacing(0);
    const int inset = UiMetrics::windowInset();
    buttons->setContentsMargins(inset, 0, inset, 0);
    rows->addWidget(top);
    auto* stack = new QStackedWidget(ribbon);
    stack->setObjectName(QStringLiteral("RibbonMenuStackeWidget"));
    stack->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
    rows->addWidget(stack, 1);

    auto addSeparator = [buttons, inset] {
        auto* sep = new QFrame;
        sep->setObjectName(QStringLiteral("menuBarSeparator"));
        sep->setFrameShape(QFrame::VLine);
        sep->setFixedHeight(UiMetrics::separatorLineHeight());
        buttons->addSpacing(inset);
        buttons->addWidget(sep);
        buttons->addSpacing(inset);
    };

    auto* file = new mRibbonButton(mRibbonButton::Mode::Small, top);
    file->setObjectName(QStringLiteral("FileButton"));
    file->setText(QStringLiteral("文件"));
    file->setIcon(resourceIcon("file.png"));
    auto* fileMenu = new mUiMenu(file);
    fileMenu->addAction(window.command(Command::New));
    fileMenu->addAction(window.command(Command::Open));
    auto* recent = new mUiMenu(QStringLiteral("最近打开"), fileMenu);
    recent->setIcon(resourceIcon("homepage/zuijindakai.png"));
    fileMenu->addMenu(recent);
    QObject::connect(recent, &QMenu::aboutToShow, &window, [recent, &window] {
        recent->clear();
        QSettings settings(originalSettingsPath(), QSettings::IniFormat);
        settings.beginGroup(QStringLiteral("recent"));
        const auto keys = settings.childKeys();
        int count = 0;
        for (auto it = keys.crbegin(); it != keys.crend() && count < 5; ++it) {
            const QString path = settings.value(*it).toString();
            if (path.isEmpty()) continue;
            auto* item = recent->addAction(path);
            item->setProperty("mxOriginalAction", QStringLiteral("slot_openRecentFile"));
            item->setProperty("mxOriginalArgument", path);
            QObject::connect(item, &QAction::triggered, &window, [&window, path] { dispatch(window, "slot_openRecentFile", path); });
            ++count;
        }
        originalAction(recent, window, {"查看所有最近打开的模型", "homepage/zuijindakai.png", "showRecentModels"});
    });
    ThemeManager::applyMenuStyle(recent);
    fileMenu->addSeparator();
    fileMenu->addAction(window.command(Command::Save));
    fileMenu->addAction(window.command(Command::SaveAs));
    fileMenu->addSeparator();
    originalAction(fileMenu, window, {"打开会话(.mxses)", "homepage/dakai.png", "slot_openMxsesFile"});
    originalAction(fileMenu, window, {"保存会话(.mxses)", "homepage/baocun.png", "slot_saveMxsesFile"});
    fileMenu->addSeparator();
    auto* importMenu = new mUiMenu(QStringLiteral("导入"), fileMenu);
    importMenu->setIcon(resourceIcon("homepage/daoru3x.png"));
    originalAction(importMenu, window, {"几何模型", "homepage/daoru3x.png", "slot_importGeoFile"});
    originalAction(importMenu, window, {"运动数据", "homepage/daoru3x.png", "slot_importMotionJsonFile"});
    fileMenu->addMenu(importMenu);
    ThemeManager::applyMenuStyle(importMenu);
    auto* exportMenu = new mUiMenu(QStringLiteral("导出"), fileMenu);
    exportMenu->setIcon(resourceIcon("homepage/daochu3x.png"));
    originalAction(exportMenu, window, {"几何模型", "homepage/daochu3x.png", "slot_exportGeoFile"});
    originalAction(exportMenu, window, {"运动数据", "homepage/daochu3x.png", "slot_exportMotionJsonFile"});
    fileMenu->addMenu(exportMenu);
    ThemeManager::applyMenuStyle(exportMenu);
    fileMenu->addSeparator();
    originalAction(fileMenu, window, {"工作目录", "homepage/dakai.png", "createWorkingDir"});
    fileMenu->addSeparator();
    auto* cases = originalAction(fileMenu, window, {"案例库", "homepage/anliku.png", "showCaseLibrary"});
    cases->setVisible(window.property("caseLibraryAvailable").toBool());
    fileMenu->addSeparator();
    originalAction(fileMenu, window, {"首选项", "homepage/shouxuanxiang.png", "showPreferences"});
    originalAction(fileMenu, window, {"许可证", "homepage/xukezheng.png", "showLicense"});
    fileMenu->addSeparator();
    originalAction(fileMenu, window, {"帮助文档", "homepage/bangzhuwendang.png"});
    auto* updates = new mUiMenu(QStringLiteral("更新产品及组件"), fileMenu);
    updates->setIcon(resourceIcon("homepage/gengxin.png"));
    updates->addAction(QStringLiteral("更新产品"));
    updates->addAction(QStringLiteral("更新案例库"));
    updates->addAction(QStringLiteral("更新材料库"));
    fileMenu->addMenu(updates);
    ThemeManager::applyMenuStyle(updates);
    fileMenu->addSeparator();
    fileMenu->addAction(window.command(Command::About));
    fileMenu->addSeparator();
    fileMenu->addAction(window.command(Command::Exit));
    ThemeManager::applyMenuStyle(fileMenu);
    file->setMenu(fileMenu);
    file->setHasArrow(false);
    file->setPopupMode(QToolButton::InstantPopup);
    buttons->addWidget(file);
    addSeparator();

    auto* quick = new QHBoxLayout;
    quick->setObjectName(QStringLiteral("quickButtonLayout"));
    quick->setContentsMargins(0, 0, 0, 0);
    quick->setSpacing(UiMetrics::quickGap());
    auto quickButton = [quick, top](const char* name, const char* image) {
        auto* button = new mRibbonButton(mRibbonButton::Mode::Small, top);
        button->setObjectName(QString::fromLatin1(name));
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setIcon(resourceIcon(image));
        button->setIconSize(UiMetrics::quickIconSize());
        button->setFixedSize(UiMetrics::quickButtonSize(), UiMetrics::quickButtonSize());
        quick->addWidget(button);
        return button;
    };
    auto* views = quickButton("ViewButton", "菜单栏设置.svg");
    views->setToolTip(QStringLiteral("视图"));
    views->setAccessibleName(QStringLiteral("视图"));
    views->setPopupMode(QToolButton::InstantPopup);
    auto* viewMenu = new mUiMenu(views);
    views->setMenu(viewMenu);
    bindButton(quickButton("QuickNewBtn", "homepage/xinjian.png"), window.command(Command::New));
    bindButton(quickButton("QuickSaveBtn", "homepage/baocun.png"), window.command(Command::Save));
    bindButton(quickButton("QuickOpenBtn", "homepage/dakai.png"), window.command(Command::Open));
    buttons->addLayout(quick);
    addSeparator();

    auto* tabs = new MyTabBar(top);
    tabs->setObjectName(QStringLiteral("MenuTabBar"));
    tabs->setDocumentMode(true);
    tabs->setExpanding(false);
    tabs->setUsesScrollButtons(true);
    tabs->setFixedHeight(UiMetrics::tabBarHeight());
    QFont tabFont(UiMetrics::uiFontFamily());
    tabFont.setPixelSize(UiMetrics::tabFontSize());
    tabs->setFont(tabFont);
    tabs->setAttribute(Qt::WA_StyledBackground, false);
    buttons->addWidget(tabs);
    buttons->addStretch(0);
    const QStringList names = { QStringLiteral("主页"), QStringLiteral("几何"), QStringLiteral("体定义"),
        QStringLiteral("运动副与约束"), QStringLiteral("驱动"), QStringLiteral("连接与力元"),
        QStringLiteral("传感与控制"), QStringLiteral("求解"), QStringLiteral("显示"), QStringLiteral("选择") };
    tabs->setResidentTabNames(names);
    tabs->setSeparatorGroup({ QStringLiteral("显示"), QStringLiteral("选择") });
    auto* visibleTabs = new mUiMenu(QStringLiteral("前处理常驻菜单栏"), viewMenu);
    viewMenu->addMenu(visibleTabs);
    const auto otherModeMenu = [&window, viewMenu](const QString& title, const QString& mode,
        const QStringList& names, const QStringList& checked) {
        auto* menu = new mUiMenu(title, viewMenu);
        QSettings settings(originalSettingsPath(), QSettings::IniFormat);
        const auto key = hiddenMenuSettingsKey(mode);
        const auto hidden = settings.value(key).toStringList();
        const bool saved = settings.contains(key);
        for (const auto& name : names) {
            auto* action = menu->addAction(name);
            action->setCheckable(true);
            action->setChecked(saved ? !hidden.contains(name) : checked.contains(name));
            action->setProperty("mxOriginalAction", QStringLiteral("setModeMenuVisible"));
            action->setProperty("mxOriginalArgument", QVariantList{mode, name});
            QObject::connect(action, &QAction::toggled, &window, [&window, menu, mode, name](bool visible) {
                saveMenuVisibility(mode, menu);
                dispatch(window, "setModeMenuVisible", QVariantList{mode, name, visible});
            });
        }
        viewMenu->addMenu(menu);
        ThemeManager::applyMenuStyle(menu);
    };
    otherModeMenu(QStringLiteral("后处理常驻菜单栏"), QStringLiteral("Post"),
        {"主页", "模型", "结果", "云图", "动画", "路径", "流线图", "切面图", "等值面图", "矢量图",
         "分区显示", "后处理-集合", "线性叠加", "分析报告", "显示", "选择"},
        {"主页", "模型", "结果", "云图", "显示", "选择"});
    otherModeMenu(QStringLiteral("曲线常驻菜单栏"), QStringLiteral("Curve"),
        {"主页", "曲线", "报告", "3D曲线"}, {"主页", "曲线", "报告", "3D曲线"});
    viewMenu->addSeparator();
    auto* treeLink = new mUiMenu(QStringLiteral("树菜单栏联动"), viewMenu);
    auto* treeLinkActions = new QActionGroup(treeLink);
    treeLinkActions->setExclusive(true);
    QSettings settings(originalSettingsPath(), QSettings::IniFormat);
    const int savedLinkMode = qBound(0, settings.value(QStringLiteral("menubar/treeMenuLinkMode"), 0).toInt(), 2);
    window.setProperty("treeMenuLinkMode", savedLinkMode);
    const QStringList treeLinkModes{QStringLiteral("切换至关联菜单"), QStringLiteral("仅切换动态菜单"), QStringLiteral("不自动切换")};
    for (int i = 0; i < treeLinkModes.size(); ++i) {
        auto* action = treeLink->addAction(treeLinkModes[i]);
        action->setCheckable(true);
        action->setChecked(i == savedLinkMode);
        treeLinkActions->addAction(action);
        action->setProperty("mxOriginalAction", QStringLiteral("setTreeMenuLinkMode"));
        action->setProperty("mxOriginalArgument", i);
        QObject::connect(action, &QAction::triggered, &window, [&window, i] {
            QSettings settings(originalSettingsPath(), QSettings::IniFormat);
            settings.setValue(QStringLiteral("menubar/treeMenuLinkMode"), i);
            window.setProperty("treeMenuLinkMode", i);
            dispatch(window, "setTreeMenuLinkMode", i);
        });
    }
    viewMenu->addMenu(treeLink);
    ThemeManager::applyMenuStyle(treeLink);
    viewMenu->addSeparator();
    auto* navigationMenu = new mUiMenu(QStringLiteral("鼠标导航"), viewMenu);
    navigationMenu->setObjectName(QStringLiteral("mouseNavigationMenu"));
    navigationMenu->addAction(window.command(Command::NavigationClassic));
    navigationMenu->addAction(window.command(Command::NavigationNx));
    viewMenu->addMenu(navigationMenu);
    QList<RibbonPage*> pages;
    const auto hiddenResident = settings.value(hiddenMenuSettingsKey(QStringLiteral("Pre"))).toStringList();
    for (const auto& name : names) {
        auto* page = new RibbonPage(stack);
        pages.push_back(page);
        stack->addWidget(page);
        const int index = tabs->addTab(name);
        auto* show = visibleTabs->addAction(name);
        show->setCheckable(true);
        show->setChecked(!hiddenResident.contains(name));
        tabs->setTabVisible(index, show->isChecked());
        QObject::connect(show, &QAction::toggled, tabs, [tabs, visibleTabs, index, &window, name](bool on) {
            tabs->setTabVisible(index, on);
            saveMenuVisibility(QStringLiteral("Pre"), visibleTabs);
            dispatch(window, "setModeMenuVisible", QVariantList{QStringLiteral("Pre"), name, on});
        });
    }
    ThemeManager::applyMenuStyle(viewMenu);
    QObject::connect(tabs, &QTabBar::currentChanged, stack, &QStackedWidget::setCurrentIndex);
    stack->setCurrentIndex(tabs->currentIndex());
    buildHome(pages[0], window);
    buildGeometry(pages[1], window);
    buildMotionPages(pages, window);
    buildDisplay(pages[8], window);
    buildSelection(pages[9], window);
    for (auto* page : pages) {
        for (auto* item : page->groups()) item->updateLayout();
        page->updateMenuGeometry();
    }
    return ribbon;
}
} // namespace MotionOcc
