#include "MotionWindow.h"

#include "MyTabBar.h"
#include "ThemeManager.h"
#include "UiMetrics.h"
#include "UiPalette.h"
#include "mRibbonButton.h"
#include "mRibbonWidget.h"
#include "mUiMenu.h"

#include <QAction>
#include <QFrame>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <initializer_list>

using namespace MechMainWindow;
using namespace MxFunctions;

namespace MotionOcc {
namespace {
const QString unavailable = QStringLiteral("此功能需要完整 MxMotion SDK，OCC 构建暂不支持。");

QIcon resourceIcon(const char* path)
{
    return QIcon(QStringLiteral(":/mMainWidgets/Icon/") + QString::fromUtf8(path));
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

struct UnavailableButton { const char* text; const char* icon; };

void disabledButtons(mGroupWidget* group, std::initializer_list<UnavailableButton> buttons, bool large = true)
{
    int position = 0;
    for (const auto& item : buttons) {
        auto* button = group->addToolBtn(QString::fromUtf8(item.text), QString(),
            large ? -1 : position % 2, large ? -1 : position / 2, 1, 1,
            large ? QSize(30, 30) : QSize(20, 20));
        button->setIcon(resourceIcon(item.icon));
        button->setAccessibleName(QString::fromUtf8(item.text));
        button->setToolTip(unavailable);
        button->setEnabled(false);
        ++position;
    }
}

void buildHome(RibbonPage* page, MotionWindow& window)
{
    auto* files = group(page, "文件");
    actionButton(files, window, Command::New, true, 0, 0);
    actionButton(files, window, Command::Open, true, 0, 1);
    actionButton(files, window, Command::Save, false, 0, 2);
    actionButton(files, window, Command::SaveAs, false, 1, 2);
    actionButton(files, window, Command::Import, false, 0, 3);
    actionButton(files, window, Command::Export, false, 1, 3);
    actionButton(files, window, Command::ImportMotionJson, false, 0, 4);
    actionButton(files, window, Command::ExportMotionJson, false, 1, 4);
    disabledButtons(group(page, "工具"), {
        { "分析", "homepage/fenxi3x.png" }, { "单位", "homepage/danwei3x.png" },
        { "剖面", "geometry/中面_大@3x.png" }
    });
    auto* layout = group(page, "布局");
    actionButton(layout, window, Command::ManageLayout);
    actionButton(layout, window, Command::ResetLayout);
    auto* screenshot = group(page, "截屏");
    actionButton(screenshot, window, Command::QuickShot, false);
    disabledButtons(screenshot, { { "自定义截屏", "post/zidingyijieping.png" } }, false);
}

void buildGeometry(RibbonPage* page, MotionWindow& window)
{
    disabledButtons(group(page, "草图"), { { "草图模式", "geometry/caotu.png" } });
    auto* solids = group(page, "实体");
    auto* box = actionButton(solids, window, Command::Box);
    auto* menu = new mUiMenu(box);
    for (const Command id : { Command::Box, Command::Cylinder, Command::Sphere })
        menu->addAction(window.command(id));
    for (const auto& item : { UnavailableButton{ "圆锥", "geometry/圆锥_大@3x.png" },
                              UnavailableButton{ "圆环", "geometry/圆环_大@3x.png" },
                              UnavailableButton{ "圆台", "geometry/圆台_大@3x.png" } }) {
        auto* action = menu->addAction(resourceIcon(item.icon), QString::fromUtf8(item.text));
        action->setEnabled(false);
        action->setToolTip(unavailable);
    }
    ThemeManager::applyMenuStyle(menu);
    box->setMenu(menu);
    box->setPopupMode(QToolButton::MenuButtonPopup);
    static_cast<mRibbonButton*>(box)->setHasArrow(true);
    disabledButtons(group(page, "特征"), { { "拉伸", "geometry/拉伸_大@3x.png" },
        { "扫掠", "geometry/扫掠_大@3x.png" }, { "倒角", "geometry/倒角_大@3x.png" } });
    disabledButtons(group(page, "变换"), { { "线性阵列", "geometry/xianxingzhenglie.png" },
        { "环形阵列", "geometry/huanxingzhenglie.png" } }, false);
    disabledButtons(group(page, "布尔操作"), { { "布尔求和", "geometry/布尔求和_大@3x.png" } });
    disabledButtons(group(page, "提取"), { { "中面", "geometry/中面_大@3x.png" } });
    disabledButtons(group(page, "几何编辑"), { { "特征投影", "geometry/project.png" } });
    disabledButtons(group(page, "几何修复"), { { "修补缝隙", "geometry/xiubufengxi.png" } });
    disabledButtons(group(page, "几何实体化"), { { "拼接", "geometry/pingjie.png" },
        { "间距", "geometry/jianju.png" }, { "缺失面", "geometry/queshimian.png" } }, false);
    disabledButtons(group(page, "特征去除"), { { "特征去除", "geometry/zidongquchu.png" } });
    disabledButtons(group(page, "共享拓扑"), { { "共享", "geometry/共享拓扑_小@3x.png" } });
}

void buildMotionPages(const QList<RibbonPage*>& pages, MotionWindow& window)
{
    // Keep the original Motion pages and group order. Unsupported SDK commands stay disabled.
    auto* bodies = group(pages[2], "体");
    actionButton(bodies, window, Command::BatchBodies);
    actionButton(bodies, window, Command::AddBody);
    actionButton(bodies, window, Command::AddAutoFlex);
    actionButton(bodies, window, Command::AddMarker);
    disabledButtons(group(pages[2], "飞机专用模块 ▼"), { { "飞机报告", "Motion/aircraftReport.png" } });
    disabledButtons(group(pages[2], "系统建模工具 ▼"), { { "汽车报告", "Motion/automobileReport.png" } });
    auto* joints = group(pages[3], "基本运动副 ▼");
    for (const auto id : {Command::AddRevolute, Command::AddPrismatic, Command::AddFixed}) actionButton(joints, window, id);
    actionButton(joints, window, Command::AddNativeJoint);
    actionButton(group(pages[3], "耦合副 ▼"), window, Command::AddCoupling);
    auto* drives = group(pages[4], "运动副驱动");
    actionButton(drives, window, Command::AddPositionDrive);
    actionButton(drives, window, Command::AddSpeedDrive);
    disabledButtons(drives, { { "加速度驱动", "Motion/drive.png" } });
    disabledButtons(group(pages[4], "运动体驱动"), {
        { "平动驱动", "Motion/drive.png" }, { "转动驱动", "Motion/drive.png" }
    });
    auto* flexible = group(pages[5], "柔性连接 ▼");
    actionButton(flexible, window, Command::AddSpring);
    actionButton(flexible, window, Command::AddSoftStop);
    actionButton(flexible, window, Command::AddTensionCable);
    auto* forces = group(pages[5], "加载力 ▼");
    actionButton(forces, window, Command::AddVectorForce);
    auto* scalar = actionButton(forces, window, Command::AddScalarForce);
    auto* forceMenu = new mUiMenu(scalar);
    for (const auto id : {Command::AddScalarForce, Command::AddScalarTorque, Command::AddThreePointForce, Command::AddThreePointTorque})
        forceMenu->addAction(window.command(id));
    ThemeManager::applyMenuStyle(forceMenu); scalar->setMenu(forceMenu); scalar->setPopupMode(QToolButton::MenuButtonPopup);
    static_cast<mRibbonButton*>(scalar)->setHasArrow(true);
    actionButton(forces, window, Command::AddContact);
    actionButton(group(pages[6], "传感器"), window, Command::AddSensor);
    actionButton(group(pages[6], "控制器"), window, Command::AddController);
    actionButton(group(pages[6], "控制输入"), window, Command::AddControlInput);
    auto* solve = group(pages[7], "求解");
    for (const auto id : {Command::NewScenario, Command::Solve, Command::StopSolve, Command::LoadRigidExample})
        actionButton(solve, window, id);
}

void buildDisplay(RibbonPage* page, MotionWindow& window)
{
    auto* orientation = group(page, "方向");
    for (const auto id : { Command::Front, Command::Top, Command::Right, Command::Isometric, Command::Fit })
        actionButton(orientation, window, id);
    auto* display = group(page, "显示");
    actionButton(display, window, Command::ShadedEdges);
    actionButton(display, window, Command::Shaded);
    actionButton(display, window, Command::Wireframe);
    actionButton(display, window, Command::MeshDisplay);
    auto* models = group(page, "模型显隐");
    actionButton(models, window, Command::HideSelected, false, 0, 0);
    actionButton(models, window, Command::HideOthers, false, 1, 0);
    actionButton(models, window, Command::InvertVisibility, false, 0, 1);
    actionButton(models, window, Command::ShowAll, false, 1, 1);
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
    for (const auto id : { Command::New, Command::Open, Command::Save, Command::SaveAs })
        fileMenu->addAction(window.command(id));
    fileMenu->addSeparator();
    for (const auto& entry : { std::pair{ "导入", Command::Import }, std::pair{ "导出", Command::Export } }) {
        auto* submenu = new mUiMenu(QString::fromUtf8(entry.first), fileMenu);
        submenu->addAction(window.command(entry.second));
        submenu->addAction(window.command(entry.second == Command::Import ? Command::ImportMotionJson : Command::ExportMotionJson));
        fileMenu->addMenu(submenu);
    }
    fileMenu->addSeparator();
    fileMenu->addAction(window.command(Command::About));
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
    buttons->addWidget(tabs, 1);
    const QStringList names = { QStringLiteral("主页"), QStringLiteral("几何"), QStringLiteral("体定义"),
        QStringLiteral("运动副与约束"), QStringLiteral("驱动"), QStringLiteral("连接与力元"),
        QStringLiteral("传感与控制"), QStringLiteral("求解"), QStringLiteral("显示"), QStringLiteral("选择") };
    tabs->setResidentTabNames(names);
    tabs->setSeparatorGroup({ QStringLiteral("显示"), QStringLiteral("选择") });
    auto* visibleTabs = new mUiMenu(QStringLiteral("前处理常驻菜单栏"), viewMenu);
    viewMenu->addMenu(visibleTabs);
    viewMenu->addAction(window.command(Command::ManageLayout));
    viewMenu->addAction(window.command(Command::ResetLayout));
    auto* navigationMenu = new mUiMenu(QStringLiteral("鼠标导航"), viewMenu);
    navigationMenu->setObjectName(QStringLiteral("mouseNavigationMenu"));
    navigationMenu->addAction(window.command(Command::NavigationClassic));
    navigationMenu->addAction(window.command(Command::NavigationNx));
    viewMenu->addMenu(navigationMenu);
    QList<RibbonPage*> pages;
    for (const auto& name : names) {
        auto* page = new RibbonPage(stack);
        pages.push_back(page);
        stack->addWidget(page);
        const int index = tabs->addTab(name);
        auto* show = visibleTabs->addAction(name);
        show->setCheckable(true);
        show->setChecked(true);
        show->setEnabled(index > 0);
        QObject::connect(show, &QAction::toggled, tabs, [tabs, index](bool on) { tabs->setTabVisible(index, on); });
    }
    ThemeManager::applyMenuStyle(viewMenu);
    QObject::connect(tabs, &QTabBar::currentChanged, stack, &QStackedWidget::setCurrentIndex);
    buildHome(pages[0], window);
    buildGeometry(pages[1], window);
    buildMotionPages(pages, window);
    buildDisplay(pages[8], window);
    auto* selection = group(pages[9], "选择");
    actionButton(selection, window, Command::PickPart);
    actionButton(selection, window, Command::ClearSelection);
    disabledButtons(group(pages[9], "扩展"), { { "相邻", "choice/xianglin3x.png" },
        { "限值", "choice/xianzhi3x.png" }, { "连接", "choice/lianjie3x.png" } });
    disabledButtons(group(pages[9], "特征"), { { "公共边", "choice/gonggongbian3x.png" },
        { "圆柱面", "choice/yuanzhumian3x.png" } }, false);
    for (auto* page : pages) {
        for (auto* item : page->groups()) item->updateLayout();
        page->updateMenuGeometry();
    }
    return ribbon;
}
} // namespace MotionOcc
