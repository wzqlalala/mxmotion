#include "mPostParrayMimage.h"
#include "mCommonFunctions.h"
#include "mGlobalSignals.h"
#include "mAppConfiguration.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mxr/view/REBUILD_PostModelRender.h"

#include <glm/gtc/matrix_transform.hpp>

#include <QRegExp>
#include <QRegExpValidator>
#include <QIntValidator>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QHideEvent>
#include <QLineEdit>
#include <QPushButton>

#include "mComboActionRow.h"
#include "mChoiceGroup.h"
#include "mJoinedPairRow.h"
#include "mPrimaryButton.h"
#include "mGroupBox.h"
#include "UiMetrics.h"
#include "UiPalette.h"

#include "mPostGeneralPickUp.h"
#include "mPostPartScopeField.h"
#include "mPostDataManager.h"
#include "mPostDataBase.h"
#include "mPhysicalFieldEnum.h"


namespace PostWindow
{
    namespace
    {
        // 当前后处理数据库全部部件号（与 mPostPartScopeField 弹窗部件列表
        // 同源同一 ID 空间；无库/无数据返回空）
        QVector<int> currentAllPartIds()
        {
            auto* model = getCVM;
            if (model == nullptr) return {};
            auto* data = model->getModelData();
            if (data == nullptr) return {};
            auto db = data->getManager<PostData::mPostDataManager>(MxFunctions::Physical_Post).getPostDatabase();
            if (db.isNull()) return {};
            return db->getAllPartId();
        }

        // 阵列四槽共用就绪守卫（功能阵列接回 2026-11-06，契约 FR-22 §5①）：
        // frameProxy(currentFrame()) 现取不缓存（F8 契约：换帧/重装载/LRU 淘汰后重取）——
        // 空即未绑定/未就绪，无可作用实例；渲染侧 fail-soft（B-12）已防崩，此处只
        // 避免无效下发并提示（替代 HEAD getOneFrameRender 空指针静默 return）
        bool parrayReadyGuard(mxr::REBUILD_PostRender* postRender)
        {
            if (postRender->frameProxy(postRender->currentFrame()) != nullptr)
                return true;
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString::fromUtf8("请先显示云图/结果帧！"), true);
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString::fromUtf8("Please display a contour/result frame first!"), true);
            }
            return false;
        }
    }

    PanelLabel* mPostParrayMimagePanel::regLabel(const QString& zh, const QString& en)
    {
        RegLabel entry;
        entry.widget = new PanelLabel(zh);
        entry.zh = zh;
        entry.en = en;
        _regLabels.append(entry);
        return entry.widget;
    }

    /*
    * 选择部件行（mPost 通用拾取条，云图/向量图"对象选择"同款）：点击条体激活拾取/
    * 再点停止（开关语义）、详情弹窗（分组+逐项删/按类型整删）、清空钮（空集=默认
    * "所有部件"）。本面板仅部件类型（无类型下拉）。CBBlue 颜色块=云图同款视觉标识。
    */
    mPostPickWidgetController* mPostParrayMimagePanel::buildPickRow(const QString& objectName)
    {
        mPostPickWidgetController* controller = mPostGeneralPickUp::createPickWidget(CBBlue, this);
        controller->widget()->setObjectName(objectName);             //ui-shot 沿用旧名
        controller->setSelectSetBtnVisible(false);                   //后处理无集合语义
        controller->setKeepHighlightOnDeactivate(false);             //解除拾取即清视口高亮
        controller->setAllowedTypes([]() {
            QSet<MxFunctions::PickDataType> types;
            types.insert(MxFunctions::PickDataType::Part);           //本面板仅部件
            return types;
            });
        if (_partScope != nullptr)
            controller->setDefultTip(_partScope->scopeText());       //构造期 scopeTextChanged 未连接，显式设默认文案

        // 激活/失活收口（云图/向量图 slot_objectPickActiveChanged 同口径；勿用
        // activePickSig——该信号只同步前处理工具栏按钮组，对后处理视口不生效）：
        // 激活=校验后处理上下文（无效立即失活回退）+视口设部件拾取过滤器；失活=
        // 清过滤器与拾取高亮+恢复工具栏全部拾取类型。收割（ReadyToReadContainer
        // 连接）、高亮与重入回显（激活时历史数据写回选择容器）由拾取栈控制器自管
        connect(controller, &mPostPickWidgetController::pickActiveChanged, this,
            [this, controller](bool active) {
                mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
                if (active)
                {
                    if (postRend == nullptr || postRend->getPostRender() == nullptr)
                    {
                        controller->setPickActive(false);
                        return;
                    }
                    postRend->setPickFilter(mxr::PickFilter::PickMeshPart);
                }
                else
                {
                    if (postRend != nullptr)
                        postRend->setPickFilter(mxr::PickFilter::PickNothing);
                    emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>());
                }
            });
        return controller;
    }

    /*
    * 操作选择与顺序（V1.3 固定面板，非折叠）：1px 边框容器 + 标题条（header 底）
    * + 三行（勾选框+名称 | 上移/下移）。行顺序即执行顺序；勾选即启用对应动作
    * 分组；行序变化经 syncGroupOrder 同步分组顺序（原型 MutationObserver 语义）。
    */
    void mPostParrayMimagePanel::buildOrderPanel()
    {
        _orderPanel = new QFrame(this);
        _orderPanel->setObjectName(QString::fromUtf8("symmetryOrderPanel"));
        _orderPanel->setStyleSheet(QString::fromUtf8(
            "QFrame#symmetryOrderPanel { border: 1px solid %1; border-radius: %2px; background: %3; }")
            .arg(MxFunctions::UiPalette::borderHex())
            .arg(MxFunctions::UiMetrics::zoneRadius())
            .arg(MxFunctions::UiPalette::surfaceHex()));

        QVBoxLayout* panelLayout = new QVBoxLayout(_orderPanel);
        panelLayout->setContentsMargins(0, 0, 0, 0);
        panelLayout->setSpacing(0);

        _orderHeading = new QLabel(QString::fromUtf8("操作选择与顺序"), _orderPanel);
        _orderHeading->setObjectName(QString::fromUtf8("symmetryOrderHeading"));
        _orderHeading->setFixedHeight(MxFunctions::UiMetrics::controlHeight());
        _orderHeading->setStyleSheet(QString::fromUtf8(
            "QLabel#symmetryOrderHeading { background: %1; color: %2; font-size: %3px; padding: 0 %4px; border-top-left-radius: %5px; border-top-right-radius: %5px; }")
            .arg(MxFunctions::UiPalette::headerHex())
            .arg(MxFunctions::UiPalette::textHex())
            .arg(MxFunctions::UiMetrics::bodyFontSize())
            .arg(8)
            .arg(MxFunctions::UiMetrics::zoneRadius()));
        panelLayout->addWidget(_orderHeading);

        QWidget* listHost = new QWidget(_orderPanel);
        _orderListLayout = new QVBoxLayout(listHost);
        _orderListLayout->setContentsMargins(8, 6, 8, 6);
        _orderListLayout->setSpacing(MxFunctions::UiMetrics::rowGap());
        panelLayout->addWidget(listHost);

        // opId 为稳定标识（不随语言切换变化），行显示文本由语言切换单独维护；
        // 默认顺序：平面镜像、平面阵列、圆周阵列，对应 MIRROR_PATH_POLAR
        auto makeOrderRow = [this](const QString& opId, const QString& displayName, QCheckBox*& checkBox) {
            QWidget* row = new QWidget();
            row->setProperty("opName", opId);
            row->setObjectName(QString::fromUtf8("orderRow"));
            row->setFixedHeight(MxFunctions::UiMetrics::controlHeight());
            QHBoxLayout* rowLayout = new QHBoxLayout(row);
            rowLayout->setContentsMargins(0, 0, 0, 0);
            rowLayout->setSpacing(MxFunctions::UiMetrics::rowGap());
            checkBox = new QCheckBox(displayName, row);
            checkBox->setObjectName(opId == QString::fromUtf8("mirror") ? QString::fromUtf8("opMirror")
                : opId == QString::fromUtf8("rect") ? QString::fromUtf8("opPlanar")
                : QString::fromUtf8("opCircular"));	//ui-shot 状态驱动（opMirror.setChecked(true) 等）
            QPushButton* up = new QPushButton(QString::fromUtf8("上移"), row);
            up->setObjectName(QString::fromUtf8("pushButton_orderUp"));
            QPushButton* down = new QPushButton(QString::fromUtf8("下移"), row);
            down->setObjectName(QString::fromUtf8("pushButton_orderDown"));
            const QSize btnSize(40, MxFunctions::UiMetrics::controlHeight());
            up->setFixedSize(btnSize);
            down->setFixedSize(btnSize);
            rowLayout->addWidget(checkBox);
            rowLayout->addStretch();
            rowLayout->addWidget(up);
            rowLayout->addWidget(down);
            _orderListLayout->addWidget(row);
            return row;
            };

        QWidget* row_mirror = makeOrderRow(QString::fromUtf8("mirror"), QString::fromUtf8("平面镜像"), _checkBox_orderMirror);
        QWidget* row_rect = makeOrderRow(QString::fromUtf8("rect"), QString::fromUtf8("平面阵列"), _checkBox_orderRect);
        QWidget* row_circular = makeOrderRow(QString::fromUtf8("circular"), QString::fromUtf8("圆周阵列"), _checkBox_orderCircular);

        // 上移/下移：重排操作表行 + 同步分组顺序（行顺序即执行顺序，slot_apply 直接读取）
        auto moveOrderRow = [this](QWidget* row, int dir) {
            int idx = _orderListLayout->indexOf(row);
            int to = idx + dir;
            if (idx < 0 || to < 0 || to >= _orderListLayout->count())
                return;
            _orderListLayout->removeWidget(row);
            _orderListLayout->insertWidget(to, row);
            refreshOrderButtons();
            syncGroupOrder();
            };

        auto connectOrderRow = [&](QWidget* row) {
            QPushButton* up = row->findChild<QPushButton*>(QString::fromUtf8("pushButton_orderUp"));
            QPushButton* down = row->findChild<QPushButton*>(QString::fromUtf8("pushButton_orderDown"));
            connect(up, &QPushButton::clicked, this, [row, moveOrderRow]() { moveOrderRow(row, -1); });
            connect(down, &QPushButton::clicked, this, [row, moveOrderRow]() { moveOrderRow(row, 1); });
            };
        connectOrderRow(row_mirror);
        connectOrderRow(row_rect);
        connectOrderRow(row_circular);
    }

    /*
    * 动作分组（mGroupBox 无勾选图标细线框）：默认收起（未启用）；启用/收起由
    * 操作表勾选驱动（原型 legend 复选已隐藏，启用只由前置表格控制——V1.3 运行时
    * 重排脚本终态）。标题可点切换为等效交互（与表格勾选双向同步，不失步）。
    */
    MxFunctions::mGroupBox* mPostParrayMimagePanel::buildActionGroup(const QString& objectName)
    {
        auto* box = new MxFunctions::mGroupBox(QString(), QPixmap(), QPixmap(), _mainFold);
        box->setObjectName(objectName);
        connect(box, &QGroupBox::toggled, this, [this](bool) { _mainFold->refreshLayout(); });
        return box;
    }

    void mPostParrayMimagePanel::buildMirrorRows(MxFunctions::mGroupBox* box)
    {
        auto* grid = box->grid();

        // 坐标系：下拉伸展 + "＋"待接入置灰（变形控制同款）
        _coordMirror = new MxFunctions::mComboActionRow(box);
        _coordMirror->setObjectName(QString::fromUtf8("coordMirror"));
        _coordMirror->combo()->addItem(QString::fromUtf8("全局坐标系"));
        _coordMirror->actionButton()->setToolTip(QString::fromUtf8("快捷创建坐标系（待接入）"));
        _coordMirror->actionButton()->setEnabled(false);
        grid->addWidget(regLabel(QString::fromUtf8("坐标系"), QString("Coordinate System")), 0, 0);
        grid->addWidget(_coordMirror, 0, 1, 1, 4);

        // 镜像平面：三连单选格 X-Y | Y-Z | Z-X（三面互斥，默认 X-Y；选中格 selected 底蓝字）
        _mirrorPlanes = new MxFunctions::mChoiceGroup({ QString::fromUtf8("X-Y"), QString::fromUtf8("Y-Z"), QString::fromUtf8("Z-X") }, box);
        _mirrorPlanes->setObjectName(QString::fromUtf8("mirrorPlanes"));
        _mirrorPlanes->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        _mirrorPlanes->setCurrentIndex(0);
        grid->addWidget(regLabel(QString::fromUtf8("镜像平面"), QString("Mirror Plane")), 1, 0);
        grid->addWidget(_mirrorPlanes, 1, 1, 1, 4);
    }

    void mPostParrayMimagePanel::buildPlanarRows(MxFunctions::mGroupBox* box)
    {
        auto* grid = box->grid();

        _coordPlanar = new MxFunctions::mComboActionRow(box);
        _coordPlanar->setObjectName(QString::fromUtf8("coordPlanar"));
        _coordPlanar->combo()->addItem(QString::fromUtf8("全局坐标系"));
        _coordPlanar->actionButton()->setToolTip(QString::fromUtf8("快捷创建坐标系（待接入）"));
        _coordPlanar->actionButton()->setEnabled(false);
        grid->addWidget(regLabel(QString::fromUtf8("坐标系"), QString("Coordinate System")), 0, 0);
        grid->addWidget(_coordPlanar, 0, 1, 1, 4);

        // "数量|距离"列头：11px muted 居中，1:2 与拼接行列同宽
        QWidget* head = new QWidget(box);
        head->setObjectName(QString::fromUtf8("planarColumnHead"));
        QHBoxLayout* headLay = new QHBoxLayout(head);
        headLay->setContentsMargins(0, 0, 0, 0);
        headLay->setSpacing(0);
        const QString headStyle = QString::fromUtf8(
            "QLabel { color: %1; font-size: %2px; }")
            .arg(MxFunctions::UiPalette::mutedHex())
            .arg(MxFunctions::UiMetrics::helperFontSize());
        _headCount = new QLabel(QString::fromUtf8("数量"), head);
        _headDistance = new QLabel(QString::fromUtf8("距离"), head);
        _headCount->setStyleSheet(headStyle);
        _headDistance->setStyleSheet(headStyle);
        _headCount->setAlignment(Qt::AlignCenter);
        _headDistance->setAlignment(Qt::AlignCenter);
        headLay->addWidget(_headCount, 1);
        headLay->addWidget(_headDistance, 2);
        grid->addWidget(head, 1, 1, 1, 4);

        // X/Y/Z 方向阵列：数量|距离 1:2 拼接行
        auto addPairRow = [this, box, grid](int row, const QString& zh, const QString& en,
            MxFunctions::mJoinedPairRow*& pair, const QString& objectName) {
            pair = new MxFunctions::mJoinedPairRow(box);
            pair->setObjectName(objectName);
            pair->leftEdit()->setText(QString::fromUtf8("1"));
            pair->rightEdit()->setText(QString::fromUtf8("0"));
            grid->addWidget(regLabel(zh, en), row, 0);
            grid->addWidget(pair, row, 1, 1, 4);
            };
        addPairRow(2, QString::fromUtf8("X方向阵列"), QString("X Direction Array"), _pairX, QString::fromUtf8("planarXPair"));
        addPairRow(3, QString::fromUtf8("Y方向阵列"), QString("Y Direction Array"), _pairY, QString::fromUtf8("planarYPair"));
        addPairRow(4, QString::fromUtf8("Z方向阵列"), QString("Z Direction Array"), _pairZ, QString::fromUtf8("planarZPair"));
    }

    void mPostParrayMimagePanel::buildCircularRows(MxFunctions::mGroupBox* box)
    {
        auto* grid = box->grid();

        _coordCircular = new MxFunctions::mComboActionRow(box);
        _coordCircular->setObjectName(QString::fromUtf8("coordCircular"));
        _coordCircular->combo()->addItem(QString::fromUtf8("全局坐标系"));
        _coordCircular->actionButton()->setToolTip(QString::fromUtf8("快捷创建坐标系（待接入）"));
        _coordCircular->actionButton()->setEnabled(false);
        grid->addWidget(regLabel(QString::fromUtf8("坐标系"), QString("Coordinate System")), 0, 0);
        grid->addWidget(_coordCircular, 0, 1, 1, 4);

        // 旋转轴：三连单选格（V1.3 默认 X 轴）
        _circularAxis = new MxFunctions::mChoiceGroup({ QString::fromUtf8("X轴"), QString::fromUtf8("Y轴"), QString::fromUtf8("Z轴") }, box);
        _circularAxis->setObjectName(QString::fromUtf8("circularAxis"));
        _circularAxis->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        _circularAxis->setCurrentIndex(0);
        grid->addWidget(regLabel(QString::fromUtf8("旋转轴"), QString("Axis")), 1, 0);
        grid->addWidget(_circularAxis, 1, 1, 1, 4);

        _editCircularCount = new QLineEdit(box);
        _editCircularCount->setObjectName(QString::fromUtf8("circularCount"));
        _editCircularCount->setText(QString::fromUtf8("1"));
        grid->addWidget(regLabel(QString::fromUtf8("阵列数量"), QString("Number")), 2, 0);
        grid->addWidget(_editCircularCount, 2, 1, 1, 4);

        _editCircularAngle = new QLineEdit(box);
        _editCircularAngle->setObjectName(QString::fromUtf8("circularAngle"));
        _editCircularAngle->setText(QString::fromUtf8("360"));
        grid->addWidget(regLabel(QString::fromUtf8("总角度"), QString("Total Angle")), 3, 0);
        grid->addWidget(_editCircularAngle, 3, 1, 1, 4);
    }

    mPostParrayMimagePanel::mPostParrayMimagePanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);
        _mainLayout->setContentsMargins(6, 6, 6, 0);
        _mainLayout->setSpacing(6);

        // 隐形范围控制器（两条选择部件行共享）：仅提供"所有部件"本地化空态文案，
        // 范围弹窗自拾取条接入起不可达（云图/向量图同款降级口径）
        _partScope = new mPostPartScopeField(this);
        _partScope->setObjectType(1);   //部件
        _partScope->hide();

        /********************镜像与阵列********************/
        _mainFold = new FoldSection(QString::fromUtf8("镜像与阵列"), this);

        // 选择部件（mPost 通用拾取条）
        _symmetryPickController = buildPickRow(QString::fromUtf8("symmetryPick"));
        _mainFold->addRow(regLabel(QString::fromUtf8("选择部件"), QString("Select Parts")),
            _symmetryPickController->widget());

        // 操作选择与顺序（固定面板，位于选择部件之后、动作分组之前——原型终态）
        buildOrderPanel();
        _mainFold->addRow(_orderPanel);

        // 三个动作分组（默认收起：操作表未勾选）
        _groupBox_mirrorEnable = buildActionGroup(QString::fromUtf8("grpSymmetryMirror"));
        buildMirrorRows(_groupBox_mirrorEnable);
        _mainFold->addRow(_groupBox_mirrorEnable);

        _groupBox_rectEnable = buildActionGroup(QString::fromUtf8("grpSymmetryPlanar"));
        buildPlanarRows(_groupBox_rectEnable);
        _mainFold->addRow(_groupBox_rectEnable);

        _groupBox_circularEnable = buildActionGroup(QString::fromUtf8("grpSymmetryCircular"));
        buildCircularRows(_groupBox_circularEnable);
        _mainFold->addRow(_groupBox_circularEnable);

        // 底部操作行：自适应视图 | 清除 + 执行（主色）
        {
            QWidget* foot = new QWidget(this);
            foot->setObjectName(QString::fromUtf8("symmetryFoot"));
            QHBoxLayout* footLay = new QHBoxLayout(foot);
            footLay->setContentsMargins(0, 0, 0, 0);
            footLay->setSpacing(MxFunctions::UiMetrics::rowGap());
            _checkBox_adaptiveView = new QCheckBox(QString::fromUtf8("自适应视图"), foot);
            _checkBox_adaptiveView->setObjectName(QString::fromUtf8("chkSymmetryAutoFit"));
            _pushButton_clear = new QPushButton(QString::fromUtf8("清除"), foot);
            _pushButton_clear->setObjectName(QString::fromUtf8("symmetryClear"));
            _pushButton_clear->setFixedSize(QSize(40, MxFunctions::UiMetrics::controlHeight()));
            _pushButton_apply = new MxFunctions::mPrimaryButton(QString::fromUtf8("执行"), foot);
            _pushButton_apply->setObjectName(QString::fromUtf8("symmetryExecute"));
            _pushButton_apply->setFixedWidth(40);
            footLay->addWidget(_checkBox_adaptiveView);
            footLay->addStretch();
            footLay->addWidget(_pushButton_clear);
            footLay->addWidget(_pushButton_apply);
            _mainFold->addRow(foot);
        }

        // 状态行（镜像平面校验等提示；空时隐藏）
        _symmetryStatus = new QLabel(this);
        _symmetryStatus->setObjectName(QString::fromUtf8("symmetryStatus"));
        _symmetryStatus->setWordWrap(true);
        _symmetryStatus->setStyleSheet(QString::fromUtf8(
            "QLabel#symmetryStatus { color: %1; font-size: %2px; }")
            .arg(MxFunctions::UiPalette::mutedHex())
            .arg(MxFunctions::UiMetrics::helperFontSize()));
        _symmetryStatus->hide();
        _mainFold->addRow(_symmetryStatus);

        _mainFold->refreshLayout();
        m_foldWidgets.append(_mainFold);
        _mainLayout->addWidget(_mainFold);

        /********************轴向回转********************/
        //原型 axialRevolutionSection（details）无 open——默认收起
        _axialFold = new FoldSection(QString::fromUtf8("轴向回转"), this, QColor(), false);

        _axialPickController = buildPickRow(QString::fromUtf8("axialPick"));
        _axialFold->addRow(regLabel(QString::fromUtf8("选择部件"), QString("Select Parts")),
            _axialPickController->widget());

        _axialAxis = new MxFunctions::mChoiceGroup({ QString::fromUtf8("X轴"), QString::fromUtf8("Y轴"), QString::fromUtf8("Z轴") }, this);
        _axialAxis->setObjectName(QString::fromUtf8("axialAxis"));
        _axialAxis->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        _axialAxis->setCurrentIndex(0);	//V1.3 默认 X 轴
        _axialFold->addRow(regLabel(QString::fromUtf8("旋转轴"), QString("Axis")), _axialAxis);

        _editAxialCount = new QLineEdit(this);
        _editAxialCount->setObjectName(QString::fromUtf8("axialCount"));
        _editAxialCount->setText(QString::fromUtf8("1"));
        _axialFold->addRow(regLabel(QString::fromUtf8("阵列数量"), QString("Number")), _editAxialCount);

        _editAxialAngle = new QLineEdit(this);
        _editAxialAngle->setObjectName(QString::fromUtf8("axialAngle"));
        _editAxialAngle->setText(QString::fromUtf8("360"));	//V1.3 默认 360
        _axialFold->addRow(regLabel(QString::fromUtf8("总角度"), QString("Total Angle")), _editAxialAngle);

        {
            QWidget* foot = new QWidget(this);
            foot->setObjectName(QString::fromUtf8("axialFoot"));
            QHBoxLayout* footLay = new QHBoxLayout(foot);
            footLay->setContentsMargins(0, 0, 0, 0);
            footLay->setSpacing(MxFunctions::UiMetrics::rowGap());
            _checkBox_axialadaptiveView = new QCheckBox(QString::fromUtf8("自适应视图"), foot);
            _checkBox_axialadaptiveView->setObjectName(QString::fromUtf8("chkAxialAutoFit"));
            _pushButton_axialClear = new QPushButton(QString::fromUtf8("清除"), foot);
            _pushButton_axialClear->setObjectName(QString::fromUtf8("axialClear"));
            _pushButton_axialClear->setFixedSize(QSize(40, MxFunctions::UiMetrics::controlHeight()));
            _pushButton_axialApply = new MxFunctions::mPrimaryButton(QString::fromUtf8("执行"), foot);
            _pushButton_axialApply->setObjectName(QString::fromUtf8("axialExecute"));
            _pushButton_axialApply->setFixedWidth(40);
            footLay->addWidget(_checkBox_axialadaptiveView);
            footLay->addStretch();
            footLay->addWidget(_pushButton_axialClear);
            footLay->addWidget(_pushButton_axialApply);
            _axialFold->addRow(foot);
        }

        _axialStatus = new QLabel(this);
        _axialStatus->setObjectName(QString::fromUtf8("axialStatus"));
        _axialStatus->setWordWrap(true);
        _axialStatus->setStyleSheet(QString::fromUtf8(
            "QLabel#axialStatus { color: %1; font-size: %2px; }")
            .arg(MxFunctions::UiPalette::mutedHex())
            .arg(MxFunctions::UiMetrics::helperFontSize()));
        _axialStatus->hide();
        _axialFold->addRow(_axialStatus);

        _axialFold->refreshLayout();
        m_foldWidgets.append(_axialFold);
        _mainLayout->addWidget(_axialFold);
        _mainLayout->addStretch();

        // 范围文本变化（语言切换/回填）→ 两条拾取条空态默认文案（云图/向量图同款）
        connect(_partScope, &mPostPartScopeField::scopeTextChanged, this, [this](const QString& text) {
            if (_symmetryPickController != nullptr)
                _symmetryPickController->setDefultTip(text);
            if (_axialPickController != nullptr)
                _axialPickController->setDefultTip(text);
            });

        // 折叠组收起=退出组内拾取（云图/向量图"收折叠组退拾取"同口径——收起后
        // 拾取条不可见，继续视口拾取会误导）
        connect(_mainFold, &FoldSection::foldExpandChanged, this, [this](bool expand) {
            if (!expand && _symmetryPickController != nullptr && _symmetryPickController->isPickActive())
                _symmetryPickController->setPickActive(false);
            });
        connect(_axialFold, &FoldSection::foldExpandChanged, this, [this](bool expand) {
            if (!expand && _axialPickController != nullptr && _axialPickController->isPickActive())
                _axialPickController->setPickActive(false);
            });

        initialUI();

        // 清除/执行（V1.3：清除保留勾选和参数，只清已执行效果与状态行）
        connect(_pushButton_clear, &QPushButton::clicked, this, &mPostParrayMimagePanel::slot_clear);
        connect(_pushButton_apply, &QPushButton::clicked, this, &mPostParrayMimagePanel::slot_apply);
        connect(_pushButton_axialClear, &QPushButton::clicked, this, &mPostParrayMimagePanel::slot_axialClear);
        connect(_pushButton_axialApply, &QPushButton::clicked, this, &mPostParrayMimagePanel::slot_axialApply);

        // 数值校验器（数量=正整数；距离/总角度=数值，沿用原口径）
        QRegExp rx("-?\\d+(\\.\\d+)?");
        QRegExpValidator* validator = new QRegExpValidator(rx, this);
        QIntValidator* countValidator = new QIntValidator(1, 10000000, this);
        _editCircularCount->setValidator(countValidator);
        _editAxialCount->setValidator(countValidator);
        _editCircularAngle->setValidator(validator);
        _editAxialAngle->setValidator(validator);
        for (MxFunctions::mJoinedPairRow* pair : { _pairX, _pairY, _pairZ })
        {
            pair->leftEdit()->setValidator(countValidator);
            pair->rightEdit()->setValidator(validator);
        }

        // 操作表勾选 <-> 动作分组启用 双向联动（同值 setChecked 不发信号，无环）；
        // 分组随勾选态启停置灰（原型 disabled fieldset：legend 转灰、内容隐藏、
        // 不可交互——启用只由前置表格控制）；任一操作启用变化 → 执行钮解禁 + 状态行清空
        auto linkEnable = [this](QGroupBox* group, QCheckBox* orderCheck) {
            connect(orderCheck, &QCheckBox::toggled, group, [group](bool checked) {
                group->setEnabled(checked);
                group->setChecked(checked);
                });
            connect(group, &QGroupBox::toggled, orderCheck, [group, orderCheck](bool checked) {
                group->setEnabled(checked);
                orderCheck->setChecked(checked);
                });
            connect(orderCheck, &QCheckBox::toggled, this, [this]() {
                updateExecuteEnabled();
                setSymmetryStatus(QString());
                });
            };
        linkEnable(_groupBox_mirrorEnable, _checkBox_orderMirror);
        linkEnable(_groupBox_rectEnable, _checkBox_orderRect);
        linkEnable(_groupBox_circularEnable, _checkBox_orderCircular);

        refreshOrderButtons();
        updateExecuteEnabled();

        // 字段变更 → 状态行清空（原型任意 change 清状态的语义）
        connect(_mirrorPlanes, &MxFunctions::mChoiceGroup::currentIndexChanged, this, [this]() { setSymmetryStatus(QString()); });
        connect(_circularAxis, &MxFunctions::mChoiceGroup::currentIndexChanged, this, [this]() { setSymmetryStatus(QString()); });
        for (MxFunctions::mJoinedPairRow* pair : { _pairX, _pairY, _pairZ })
        {
            connect(pair->leftEdit(), &QLineEdit::textChanged, this, [this]() { setSymmetryStatus(QString()); });
            connect(pair->rightEdit(), &QLineEdit::textChanged, this, [this]() { setSymmetryStatus(QString()); });
        }
        connect(_editCircularCount, &QLineEdit::textChanged, this, [this]() { setSymmetryStatus(QString()); });
        connect(_editCircularAngle, &QLineEdit::textChanged, this, [this]() { setSymmetryStatus(QString()); });
        connect(_axialAxis, &MxFunctions::mChoiceGroup::currentIndexChanged, this, [this]() { setAxialStatus(QString()); });
        connect(_editAxialCount, &QLineEdit::textChanged, this, [this]() { setAxialStatus(QString()); });
        connect(_editAxialAngle, &QLineEdit::textChanged, this, [this]() { setAxialStatus(QString()); });

        // 切换界面语言
        connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)),
            this, SLOT(slot_changeLanguage(int)));
        slot_changeLanguage(MAppConfiguration::mAppConfiguration::getInstance()->_language);

        this->setMinimumWidth(300);
    }


    void mPostParrayMimagePanel::newParrayMimagePanel(int caseId)
    {
        Q_UNUSED(caseId);
    }

    void mPostParrayMimagePanel::initialUI()
    {
        // V1.3 原型初始态：三操作全部未启用（操作表未勾选、分组收起置灰、执行禁用）
        for (QGroupBox* group : { static_cast<QGroupBox*>(_groupBox_mirrorEnable),
            static_cast<QGroupBox*>(_groupBox_rectEnable), static_cast<QGroupBox*>(_groupBox_circularEnable) })
        {
            group->setEnabled(false);
            group->setChecked(false);
        }
        _checkBox_orderMirror->setChecked(false);
        _checkBox_orderRect->setChecked(false);
        _checkBox_orderCircular->setChecked(false);
        resetDefaultOrder();
        // 平面镜像：默认 X-Y（单选组恒有选中项，无需"未选平面"校验）
        _mirrorPlanes->setCurrentIndex(0);
        // 平面阵列：数量 1 / 距离 0
        for (MxFunctions::mJoinedPairRow* pair : { _pairX, _pairY, _pairZ })
        {
            pair->setLeftText(QString::fromUtf8("1"));
            pair->setRightText(QString::fromUtf8("0"));
        }
        // 圆周阵列：默认 X 轴 / 1 / 360
        _circularAxis->setCurrentIndex(0);
        _editCircularCount->setText(QString::fromUtf8("1"));
        _editCircularAngle->setText(QString::fromUtf8("360"));
        // 坐标系复位为全局坐标系（第 0 项）
        _coordMirror->combo()->setCurrentIndex(0);
        _coordPlanar->combo()->setCurrentIndex(0);
        _coordCircular->combo()->setCurrentIndex(0);
        // 自适应视图复位
        _checkBox_adaptiveView->setChecked(false);
        // 轴向回转：默认 X 轴 / 1 / 360
        _axialAxis->setCurrentIndex(0);
        _editAxialCount->setText(QString::fromUtf8("1"));
        _editAxialAngle->setText(QString::fromUtf8("360"));
        _checkBox_axialadaptiveView->setChecked(false);
        // 状态行复位
        setSymmetryStatus(QString());
        setAxialStatus(QString());
    }

    void mPostParrayMimagePanel::refreshOrderButtons()
    {
        if (!_orderListLayout) return;
        const int n = _orderListLayout->count();
        for (int i = 0; i < n; ++i)
        {
            QWidget* row = _orderListLayout->itemAt(i)->widget();
            if (!row) continue;
            QPushButton* up = row->findChild<QPushButton*>(QString::fromUtf8("pushButton_orderUp"));
            QPushButton* down = row->findChild<QPushButton*>(QString::fromUtf8("pushButton_orderDown"));
            if (up) up->setEnabled(i > 0);
            if (down) down->setEnabled(i < n - 1);
        }
    }

    void mPostParrayMimagePanel::resetDefaultOrder()
    {
        if (!_orderListLayout) return;
        auto findRow = [this](const QString& opId) -> QWidget* {
            for (int i = 0; i < _orderListLayout->count(); ++i)
            {
                QWidget* r = _orderListLayout->itemAt(i)->widget();
                if (r && r->property("opName").toString() == opId)
                    return r;
            }
            return nullptr;
            };
        // 默认顺序：平面镜像(mirror)、平面阵列/直线(rect)、圆周阵列/环形(circular)
        const QStringList defaultOrder = {
            QString::fromUtf8("mirror"),
            QString::fromUtf8("rect"),
            QString::fromUtf8("circular")
        };
        for (int i = 0; i < defaultOrder.size(); ++i)
        {
            QWidget* row = findRow(defaultOrder[i]);
            if (!row) continue;
            if (_orderListLayout->indexOf(row) != i)
            {
                _orderListLayout->removeWidget(row);
                _orderListLayout->insertWidget(i, row);
            }
        }
        refreshOrderButtons();
        syncGroupOrder();
    }

    /*
    * 操作表行序 → 动作分组行序同步（原型"动作表排序同步到原参数框"）：
    * 三组先整体摘除（takeRow 保控件），再按行序插回分组区段起点。
    */
    void mPostParrayMimagePanel::syncGroupOrder()
    {
        QFormLayout* form = _mainFold->layout();
        if (!form || !_orderListLayout) return;

        QGroupBox* groups[3] = { _groupBox_mirrorEnable, _groupBox_rectEnable, _groupBox_circularEnable };

        // 分组区段起点 = 三组当前最小行号
        int baseRow = -1;
        for (QGroupBox* g : groups)
        {
            int row = -1;
            QFormLayout::ItemRole role;
            form->getWidgetPosition(g, &row, &role);
            if (row >= 0 && (baseRow < 0 || row < baseRow))
                baseRow = row;
        }
        if (baseRow < 0) return;

        for (QGroupBox* g : groups)
        {
            int row = -1;
            QFormLayout::ItemRole role;
            form->getWidgetPosition(g, &row, &role);
            if (row < 0) continue;
            QFormLayout::TakeRowResult taken = form->takeRow(g);
            delete taken.labelItem;
            delete taken.fieldItem;
        }

        auto groupForOp = [this](const QString& opId) -> QGroupBox* {
            if (opId == QString::fromUtf8("mirror")) return _groupBox_mirrorEnable;
            if (opId == QString::fromUtf8("rect")) return _groupBox_rectEnable;
            if (opId == QString::fromUtf8("circular")) return _groupBox_circularEnable;
            return nullptr;
            };

        int insertAt = baseRow;
        for (int i = 0; i < _orderListLayout->count(); ++i)
        {
            QWidget* row = _orderListLayout->itemAt(i)->widget();
            if (!row) continue;
            QGroupBox* g = groupForOp(row->property("opName").toString());
            if (!g) continue;
            form->insertRow(insertAt++, g);
        }
        _mainFold->refreshLayout();
    }

    void mPostParrayMimagePanel::updateExecuteEnabled()
    {
        const bool anyEnabled = _groupBox_mirrorEnable->isChecked()
            || _groupBox_rectEnable->isChecked()
            || _groupBox_circularEnable->isChecked();
        _pushButton_apply->setEnabled(anyEnabled);
    }

    void mPostParrayMimagePanel::updateAdaptiveView()
    {
        // 自适应视图：视口适配当前模型
        mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;
        postRend->FitView();
    }

    void mPostParrayMimagePanel::setSymmetryStatus(const QString& text)
    {
        _symmetryStatus->setText(text);
        _symmetryStatus->setVisible(!text.isEmpty());
    }

    void mPostParrayMimagePanel::setAxialStatus(const QString& text)
    {
        _axialStatus->setText(text);
        _axialStatus->setVisible(!text.isEmpty());
    }

    void mPostParrayMimagePanel::slot_changeLanguage(int language)
    {
        const bool en = (language == 1);

        // 折叠组
        _mainFold->setTitle(en ? QString::fromUtf8("Mirror and Array") : QString::fromUtf8("镜像与阵列"));
        _axialFold->setTitle(en ? QString::fromUtf8("Axial Revolution") : QString::fromUtf8("轴向回转"));

        // 三个动作分组
        _groupBox_mirrorEnable->setTitle(en ? QString::fromUtf8("Mirror") : QString::fromUtf8("平面镜像"));
        _groupBox_rectEnable->setTitle(en ? QString::fromUtf8("Rectangular") : QString::fromUtf8("平面阵列"));
        _groupBox_circularEnable->setTitle(en ? QString::fromUtf8("Circular") : QString::fromUtf8("圆周阵列"));

        // 登记表行标签（坐标系/镜像平面/X方向阵列…/旋转轴/阵列数量/总角度/选择部件）
        for (const RegLabel& entry : _regLabels)
            entry.widget->setText(en ? entry.en : entry.zh);

        // 平面阵列列头
        _headCount->setText(en ? QString::fromUtf8("Count") : QString::fromUtf8("数量"));
        _headDistance->setText(en ? QString::fromUtf8("Distance") : QString::fromUtf8("距离"));

        // 坐标系下拉第 0 项"全局坐标系"
        const QString globalCsys = en ? QString::fromUtf8("Global Coordinate System") : QString::fromUtf8("全局坐标系");
        for (MxFunctions::mComboActionRow* coord : { _coordMirror, _coordPlanar, _coordCircular })
        {
            if (coord->combo()->count() > 0)
                coord->combo()->setItemText(0, globalCsys);
        }

        // 旋转轴单选组（语言切换 setOptions 保选中：先取索引→block→设→还原，踩坑 45）
        const QStringList axisOptions = en
            ? QStringList{ QString::fromUtf8("X Axis"), QString::fromUtf8("Y Axis"), QString::fromUtf8("Z Axis") }
            : QStringList{ QString::fromUtf8("X轴"), QString::fromUtf8("Y轴"), QString::fromUtf8("Z轴") };
        for (MxFunctions::mChoiceGroup* group : { _circularAxis, _axialAxis })
        {
            const int keep = group->currentIndex();
            group->blockSignals(true);
            group->setOptions(axisOptions);
            group->setCurrentIndex(keep);
            group->blockSignals(false);
        }

        // 操作选择与顺序面板
        _orderHeading->setText(en ? QString::fromUtf8("Operation Selection and Order") : QString::fromUtf8("操作选择与顺序"));
        for (int i = 0; i < _orderListLayout->count(); ++i)
        {
            QWidget* row = _orderListLayout->itemAt(i)->widget();
            if (!row) continue;
            const QString opId = row->property("opName").toString();
            if (QCheckBox* cb = row->findChild<QCheckBox*>())
            {
                if (opId == "mirror")
                    cb->setText(en ? QString::fromUtf8("Mirror") : QString::fromUtf8("平面镜像"));
                else if (opId == "rect")
                    cb->setText(en ? QString::fromUtf8("Rectangular") : QString::fromUtf8("平面阵列"));
                else if (opId == "circular")
                    cb->setText(en ? QString::fromUtf8("Circular") : QString::fromUtf8("圆周阵列"));
            }
            if (QPushButton* up = row->findChild<QPushButton*>(QString::fromUtf8("pushButton_orderUp")))
                up->setText(en ? QString::fromUtf8("Up") : QString::fromUtf8("上移"));
            if (QPushButton* down = row->findChild<QPushButton*>(QString::fromUtf8("pushButton_orderDown")))
                down->setText(en ? QString::fromUtf8("Down") : QString::fromUtf8("下移"));
        }

        // 选择部件（范围控制器重译 → scopeTextChanged 自动同步两条拾取条空态文案；
        // 拾取条内建计数/详情文案为拾取栈标准中文，云图/向量图同款限制留档）
        _partScope->refreshLanguage(language);

        // 底部操作行
        _checkBox_adaptiveView->setText(en ? QString::fromUtf8("Adaptive View") : QString::fromUtf8("自适应视图"));
        _pushButton_clear->setText(en ? QString::fromUtf8("Clear") : QString::fromUtf8("清除"));
        _pushButton_apply->setText(en ? QString::fromUtf8("Apply") : QString::fromUtf8("执行"));
        _checkBox_axialadaptiveView->setText(en ? QString::fromUtf8("Adaptive View") : QString::fromUtf8("自适应视图"));
        _pushButton_axialClear->setText(en ? QString::fromUtf8("Clear") : QString::fromUtf8("清除"));
        _pushButton_axialApply->setText(en ? QString::fromUtf8("Apply") : QString::fromUtf8("执行"));
    }

    void mPostParrayMimagePanel::slot_clear()
    {
        mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;
        auto postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        // 功能阵列接回（2026-11-06，契约 FR-22/A3）：clearArrayState 一并清除
        // 镜像/环形/路径/轴向回转全族，清除持久、重建不复活。
        // V1.3 语义：清除保留勾选和参数（原型 title"清除已执行的阵列效果，保留勾选和参数"）
        if (!parrayReadyGuard(postRender.get()))
            return;

        // 阵列收编（unify-addobject-arch）：全族清除=removeObject（无对象时等价现状空清安全）
        {
            const int _paId = PostCommon::firstSceneObjectId(postRender.get(), mxr::SceneObjectType::PartArray);
            if (_paId >= 0)
                postRender->removeObject(_paId);
        }
        postRend->update();

        setSymmetryStatus(QString());
    }

    void mPostParrayMimagePanel::slot_apply()
    {
        mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;
        auto postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        // 功能阵列接回（2026-11-06，契约 FR-22/A1）：applyArrayState 覆盖式语义
        if (!parrayReadyGuard(postRender.get()))
            return;

        const bool mirrorOn = _groupBox_mirrorEnable->isChecked();
        const bool planarOn = _groupBox_rectEnable->isChecked();
        const bool circularOn = _groupBox_circularEnable->isChecked();

        // 镜像（三面互斥单选；未启用族按原型只作用于启用槽：标志恒 false）
        const int mirrorIndex = _mirrorPlanes->currentIndex();
        const bool mirrorXY = mirrorOn && mirrorIndex == 0;
        const bool mirrorYZ = mirrorOn && mirrorIndex == 1;
        const bool mirrorZX = mirrorOn && mirrorIndex == 2;

        // 环形（旋转轴取单选组：0=X，1=Y，2=Z，与 ArrayAxis 同序；V1.3 默认 X）
        mxr::ArrayAxis polarAxis = mxr::ArrayAxis::Z;
        const int axisIndex = _circularAxis->currentIndex();
        if (axisIndex == 0)
        {
            polarAxis = mxr::ArrayAxis::X;
        }
        else if (axisIndex == 1)
        {
            polarAxis = mxr::ArrayAxis::Y;
        }
        int num = 1;
        float angle = 360.0f;
        if (circularOn)
        {
            num = _editCircularCount->text().toInt();
            angle = _editCircularAngle->text().toFloat();
            if (angle > 10000)
            {
                if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString::fromUtf8("总角度输入范围为0-10000，请重新输入！"), true);
                }
                else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString::fromUtf8("The total angle input range is 0-10000, please re-enter!"), true);
                }
                return;
            }
        }

        // 直线(平面阵列)：未启用族保持中性值（数量 1 / 距离 0，无可见阵列）
        int xNum = 1, yNum = 1, zNum = 1;
        float xOffset = 0.0f, yOffset = 0.0f, zOffset = 0.0f;
        if (planarOn)
        {
            xNum = _pairX->leftText().toInt();
            xOffset = _pairX->rightText().toFloat();
            if (xOffset > 10000)
            {
                if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString::fromUtf8("X偏移量输入范围为0-10000，请重新输入！"), true);
                }
                else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString::fromUtf8("The input range of x offset is 0-10000, please re-enter!"), true);
                }
                return;
            }
            yNum = _pairY->leftText().toInt();
            yOffset = _pairY->rightText().toFloat();
            if (yOffset > 10000)
            {
                if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString::fromUtf8("Y偏移量输入范围为0-10000，请重新输入！"), true);
                }
                else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString::fromUtf8("The input range of y offset is 0-10000, please re-enter!"), true);
                }
                return;
            }
            zNum = _pairZ->leftText().toInt();
            zOffset = _pairZ->rightText().toFloat();
            if (zOffset > 10000)
            {
                if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString::fromUtf8("Z偏移量输入范围为0-10000，请重新输入！"), true);
                }
                else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString::fromUtf8("The input range of z offset is 0-10000, please re-enter!"), true);
                }
                return;
            }
        }

        // 功能阵列下发（契约 FR-22/A1）：applyArrayState 覆盖式语义——内嵌清镜像/路径/
        // 环形/轴向四族 + 启用槽应用 + 执行序 setType 同次下发同次消费
        mxr::ArrayApplyParams params;
        params.mirrorXY = mirrorXY;
        params.mirrorXZ = mirrorZX;
        params.mirrorYZ = mirrorYZ;
        params.polarAxis = polarAxis;
        params.polarNum = num;
        params.polarTotalAngle = angle;
        params.pathNumX = xNum;
        params.pathNumY = yNum;
        params.pathNumZ = zNum;
        params.pathDisX = xOffset;
        params.pathDisY = yOffset;
        params.pathDisZ = zOffset;

        // 操作顺序：读取顺序列表行序（从上到下），映射到 GeometryArrayType（全排列）
        QStringList opSeq;
        for (int i = 0; i < _orderListLayout->count(); ++i)
        {
            if (QWidget* row = _orderListLayout->itemAt(i)->widget())
                opSeq << row->property("opName").toString();
        }
        const QString orderSeq = opSeq.join(",");
        if (orderSeq == "mirror,rect,circular")
            params.order = mxr::GeometryArrayType::MIRROR_PATH_POLAR;
        else if (orderSeq == "mirror,circular,rect")
            params.order = mxr::GeometryArrayType::MIRROR_POLAR_PATH;
        else if (orderSeq == "rect,mirror,circular")
            params.order = mxr::GeometryArrayType::PATH_MIRROR_POLAR;
        else if (orderSeq == "rect,circular,mirror")
            params.order = mxr::GeometryArrayType::PATH_POLAR_MIRROR;
        else if (orderSeq == "circular,mirror,rect")
            params.order = mxr::GeometryArrayType::POLAR_MIRROR_PATH;
        else if (orderSeq == "circular,rect,mirror")
            params.order = mxr::GeometryArrayType::POLAR_PATH_MIRROR;

        // 阵列收编（unify-addobject-arch）：整面板批量下发=addObject(PartArrayParams{mode=FullPanel})
        //（Q7=A 重复下发=改参替换保持既有 id，覆盖式"先清后应用"语义由既有实现承载）
        {
            mxr::PartArrayParams _pa;
            _pa.mode = mxr::PartArrayParams::FullPanel;
            _pa.array = params;
            postRender->addObject(_pa);
        }

        postRend->update();
        setSymmetryStatus(QString());

        // 自适应视图（勾选时执行）
        if (_checkBox_adaptiveView->isChecked())
        {
            updateAdaptiveView();
        }
    }

    void mPostParrayMimagePanel::slot_axialClear()
    {
        mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;
        auto postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        // 功能阵列接回（2026-11-06，契约 FR-22/A4）：clearAxialRevolveState 仅清轴向
        // 回转，镜像/环形/路径族不受影响。V1.3 语义：清除保留参数
        if (!parrayReadyGuard(postRender.get()))
            return;

        // 阵列收编（unify-addobject-arch）：仅清轴向=updateObject 动作位 ClearAxialOnly
        //（无对象时等价现状空清安全——clearAxialRevolveState 本就未绑定/未启用 fail-soft）
        {
            const int _paId = PostCommon::firstSceneObjectId(postRender.get(), mxr::SceneObjectType::PartArray);
            if (_paId >= 0)
            {
                mxr::PartArrayParams _pa;
                _pa.mode = mxr::PartArrayParams::ClearAxialOnly;
                postRender->updateObject(_paId, _pa);
            }
        }
        postRend->update();

        setAxialStatus(QString());
    }

    void mPostParrayMimagePanel::slot_axialApply()
    {
        mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;
        auto postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        // 功能阵列接回（2026-11-06，契约 FR-22/A2）：轴向回转直调改经新门面通道
        if (!parrayReadyGuard(postRender.get()))
            return;

        // 旋转轴取单选组：0=X，1=Y，2=Z，与 ArrayAxis 同序（V1.3 默认 X）
        mxr::ArrayAxis axialAxis = mxr::ArrayAxis::Z;
        const int axisIndex = _axialAxis->currentIndex();
        if (axisIndex == 0)
        {
            axialAxis = mxr::ArrayAxis::X;
        }
        else if (axisIndex == 1)
        {
            axialAxis = mxr::ArrayAxis::Y;
        }

        // 部件范围：拾取条已拾取部件 → 仅拾取部件回转；未拾取（空集）→ 全部件回转
        //（引擎语义：rotateParts 空集=不回转任何部件——"空=全部件"由 GUI 在此
        // 填满后下发达成，契约 §3.2/E-02，勿按字面传空集。原"通过材料"弹窗勾选
        // 口径随范围弹窗下线废止，拾取条 getAllData 为新数据源）
        std::set<int> rotateParts;
        {
            const auto& picked = _axialPickController->getAllData().pickedData;
            const auto it = picked.find(MxFunctions::PickDataType::Part);
            if (it != picked.end())
                for (const auto& item : it->second)
                    if (item.part_id >= 0)
                        rotateParts.insert(item.part_id);
        }
        if (rotateParts.empty())
        {
            const QVector<int> allIds = currentAllPartIds();
            if (allIds.isEmpty())
                return; // 无部件数据，无可回转对象
            for (int id : allIds)
                rotateParts.insert(id);
        }

        const int num = _editAxialCount->text().toInt();
        const float angle = _editAxialAngle->text().toFloat();
        if (angle > 10000)
        {
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString::fromUtf8("总角度输入范围为0-10000，请重新输入！"), true);
            }
            else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString::fromUtf8("The total angle input range is 0-10000, please re-enter!"), true);
            }
            return;
        }

        // 轴向回转为独立操作：applyAxialRevolve 内嵌"先清镜像/路径/环形场景图级联"
        //（HEAD 既有清场行为），再按部件集合重建按部件实例（绕全局轴过原点，
        // num 个实例含原始位，步距=总角度/num，与圆周阵列口径一致）
        mxr::AxialRevolveParams axParams;
        axParams.rotateParts = rotateParts;
        axParams.axis = axialAxis;
        axParams.num = num;
        axParams.totalAngle = angle;
        // 阵列收编（unify-addobject-arch）：轴向回转下发=addObject(PartArrayParams{mode=AxialOnly})
        //（Q7=A 重复下发=改参替换；"先清镜像/路径/环形级联"由既有实现内嵌承载）
        {
            mxr::PartArrayParams _pa;
            _pa.mode = mxr::PartArrayParams::AxialOnly;
            _pa.axial = axParams;
            postRender->addObject(_pa);
        }

        postRend->update();
        setAxialStatus(QString());

        // 自适应视图（勾选时执行）
        if (_checkBox_axialadaptiveView->isChecked())
        {
            updateAdaptiveView();
        }
    }

    void mPostParrayMimagePanel::hideEvent(QHideEvent* event)
    {
        //面板隐藏=拾取条收口（宿主 setMainWidget 切换面板时触发）：激活态解除+
        //清视口高亮；拾取条弹窗为 Qt::Popup 随宿主隐藏自动关闭（云图/向量图同款）
        if (_symmetryPickController != nullptr)
            _symmetryPickController->setPickWidgetVisible(false);
        if (_axialPickController != nullptr)
            _axialPickController->setPickWidgetVisible(false);
        mPanelBase::hideEvent(event);
    }

    void mPostParrayMimagePanel::showEvent(QShowEvent* event)
    {
        mPanelBase::showEvent(event);
        //隐藏期 setPickWidgetVisible(false) 的对称恢复（云图/向量图同款；已拾取
        //数据保留在控件数据模型，条体显示随数据自动恢复）
        if (_symmetryPickController != nullptr)
            _symmetryPickController->setPickWidgetVisible(true);
        if (_axialPickController != nullptr)
            _axialPickController->setPickWidgetVisible(true);
    }
}
