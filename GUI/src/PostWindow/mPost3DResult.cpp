#include "mPost3DResult.h"

//视口命名协议（仅头文件纯函数；初始视口显示名接管见 slot_finishedImportFiles 尾部）
#include "mPostViewportNaming.h"

#include <QFormLayout>
#include <QCoreApplication>
#include <QDir>
#include <QApplication>
#include <QCollator>
#include <QRadioButton>
#include <QElapsedTimer>
#include <QFile>
#include <QDateTime>
#include <QDoubleValidator>
#include <QListWidget>
#include <QStyle>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <array>
#include <cmath>

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mDockLayoutManager.h"
#include "mViewTreeFunction.h"

//MxFunctions
#include "mGlobalSignals.h"
#include "mContainerOperation.h"
#include "mAppConfiguration.h"
#include "mCommonFunctions.h"   // S5：G5 台账寻址助手（PostCommon）

//BasicData
#include "mPhysicalFieldEnum.h"
#include "mCaseBase.h"
#include "mCaseManager.h"

//Post
#include "mPostDataBase.h"
#include "mIMxdbFile1.h"
#include "mMxdbThread.h"
#include "NativeReadersIO/mIONativeFile.h"
#include "VtkIO/mIOVtkFile.h"
#include "CameraManager.h"
#include "mPostParseMesh.h"
#include "mPostMeshSignature.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mxr/view/REBUILD_PostModelRender.h"
#include "mxr/view/mPostFrameText.h"
#include <InteractionCore/InteractionSystem.h>
#include <InteractionCore/Dialog/Legend/CLegendEditorWidget.h>
#include "PostViewportWidget.h"
#include "mxr/view/REBUILD_PostOneFrameRender.h"
#include "mPostOneFrameRendData.h"
#include "mPostRigidWallData.h"

//（mPostSlider 已迁移共享库为 MxFunctions::mSlider——V1.3 云图面板收敛）
#include "mPostRealtimeAnimWidget.h"

//v1.4（anim-tab-v13）QToolBar 化：工具栏壳=mCenterMainWindow::initAnimToolBar 预创建，
//经 MechCommon::animToolBarProvider 取壳回挂（provider 通道先例=viewDisplayBarProvider，
//mCenterMainWindow.cpp:112-114 注册/mDockLayoutManager 消费）；原 ADS dock 承载
//（CDockManager 落位+CDockWidget 包裹）随形态退役
#include "mCenterWindowProvider.h"
#include <QToolBar>		//壳挂载/取用（addWidget/show）
#include <QHideEvent>	//面板退出瞬时态收口（§9 契约）
#include <QTimer>		//v1.4 初值同步延迟一拍（壳/视图树完成同步）
#include "../PreData/PickCommonFunction.h"   //UpdatePostWidgetHighlight（拾取武装态清缓冲高亮）
#include <QScreen>
#include <QGuiApplication>


//窗口
#include "mPostInfoHub.h"
#include "mPostValueProvider.h"
#include <QDateTime>      //P5-2 审计修复：装载中提示时间窗限频
#include "mPostDeformation.h"
#include "mPostVectorPlot.h"   // refreshVectorPlotIfActive（向量图跟随切帧/重绘）
#include "mPostSectionCut.h"   // refreshSectionCutIfActive（截面剖切跟随切帧/重绘/变形系数）
#include "mPostPartScopeField.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mOtherFunction.h"



using namespace MxFunctions;
using namespace MIOFile;
using namespace MechCommon;
using namespace PostCommon;
using namespace PostData;
using namespace MechPreData;

namespace
{
    struct PerfTimer
    {
        QElapsedTimer timer;
        QByteArray name;
        PerfTimer(const char* n) : name(n) { timer.start(); qDebug() << "[Enter]" << name.constData(); }
        ~PerfTimer() { qDebug() << "[Exit]" << name.constData() << "took" << timer.elapsed() << "ms"; }
    };

    // S6 切变量全链诊断埋点：slot_showCloud 总闸表（RAII）——入口构造、所有 return
    // 路径（早退/兜底/正常尾）析构统一落总耗时；纯计时输出，零行为改动
    struct CloudSwitchTimer
    {
        QElapsedTimer timer;
        QString var;
        explicit CloudSwitchTimer(const QString& v) : var(v) { timer.start(); }
        ~CloudSwitchTimer()
        {
            qDebug() << "[CloudSwitch] total=" << timer.elapsed() << "ms var=" << var;
        }
    };

    // mxdb 新旧分支读数据失败的历史行为是静默（切变量无图例无颜色变化无从排查），
    // 统一落输出窗口；文案口径与 slot_showCloud native 分支一致
    void report3DReadFailure(int frame, const QString& varstr, const QString& compostr,
        const QString& postPathName)
    {
        const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
        mGlobalSignals::getInstance()->outputMessageSig(2,
            isChinese ? QStringLiteral("读取结果分量失败：帧%1，变量%2，分量%3（%4），详见调试日志")
                            .arg(frame).arg(varstr).arg(compostr).arg(postPathName)
                      : QStringLiteral("Failed to read result component: frame %1, variable %2, component %3 (%4); see the debug log for details")
                            .arg(frame).arg(varstr).arg(compostr).arg(postPathName),
            true);
    }

    // S6 读数缓存命中复核（段c）：镜像 mPostDataBase::addResultValue 的
    // _resultVarNames 登记格式（varName[+" "compoName]，mPostDataBase.cpp:1943-1950），
    // 库写侧登记格式若改动须同步此处
    QString expectedResultVarName(const QString& varstr, const QString& compostr)
    {
        const QString varName = varstr.trimmed();
        const QString compoName = compostr.trimmed();
        QString resultName = varName;
        if (!varName.isEmpty() && !compoName.isEmpty()
            && !compoName.contains(varName, Qt::CaseInsensitive))
            resultName = varName + QStringLiteral(" ") + compoName;
        return resultName;
    }
}

using namespace MechCaseData;
namespace PostWindow
{
    MIOFile::mIONativeFile* mPost3DResultPanel::nativeReaderFor(const QString& postFile) const
    {
        return _nativeReaders.value(postFile);
    }

    MIOFile::mIOVtkFile* mPost3DResultPanel::vtkReaderFor(const QString& postFile) const
    {
        return _vtkReaders.value(postFile);
    }

    QString mPost3DResultPanel::initFileFor(const QString& postName) const
    {
        return _postInitFiles.value(postName);
    }

    MIOFile::mIMxdbFile1* mPost3DResultPanel::mxdbOldReaderFor(const QString& postFile)
    {
        if (MIOFile::mIMxdbFile1* existing = _mxdbOldReaderPool.value(postFile))
            return existing;
        MIOFile::mIMxdbFile1* reader = new MIOFile::mIMxdbFile1();
        reader->setParent(this);
        //老格式 getFile() 即 .mxdb0 初始帧文件，池键兼作产品线/精度探测入参
        reader->setInitFileName(postFile);
        _mxdbOldReaderPool.insert(postFile, reader);
        return reader;
    }

    MIOFile::mMxdbThread* mPost3DResultPanel::mxdbNewReaderFor(const QString& postFile, QPointer<PostData::mPostDataBase> modeldb)
    {
        if (MIOFile::mMxdbThread* existing = _mxdbNewReaderPool.value(postFile))
            return existing;
        MIOFile::mMxdbThread* reader = new MIOFile::mMxdbThread(postFile, modeldb, this);
        _mxdbNewReaderPool.insert(postFile, reader);
        return reader;
    }

    bool mPost3DResultPanel::isLazyNativeResult(const QString& postFile) const
    {
        return nativeReaderFor(postFile) != nullptr;
    }

    bool mPost3DResultPanel::isLazyVtkResult(const QString& postFile) const
    {
        return vtkReaderFor(postFile) != nullptr;
    }

    mPost3DResultPanel::mPost3DResultPanel(QWidget* parent) : mPanelBase(parent)
    {
        _curPostRend = nullptr;
        _animDeletePending = true;	//v1.4 单动源互斥一次性旗（首次播放放行消费）
        _animBarAction = nullptr;

        //v1.4 前后处理上下文显隐（开放问题 6 结论：前处理隐藏/后处理常显）：
        //边沿=currentViewportChanged 增量刷新；初值=构造期求值一次（T-2/J-02：
        //信号无重放面板构造晚于最后一次激活即漏、同 ID 激活去重早退不发、
        //restoreLayout 的 center->restoreState 会把上一会话"可见"写回——只靠边沿必漏）
        connect(MechPreData::mViewportModelManager::getInstance(),
            &MechPreData::mViewportModelManager::currentViewportChanged,
            this, &mPost3DResultPanel::slot_viewportContextChanged);
        QTimer::singleShot(0, this, &mPost3DResultPanel::slot_viewportContextChanged);	//初值延迟一拍（壳/视图树完成同步）

        _mainLayout = new QVBoxLayout(this);


        /********************操作条********************/
        QWidget* actionBar = new QWidget(this);
        QHBoxLayout* actionBarLayout = new QHBoxLayout(actionBar);
        actionBarLayout->setContentsMargins(4, 4, 4, 2);
        actionBarLayout->setSpacing(6);
        _btnSavePreset = new QPushButton("缓存云图");
        _btnClearRendering = new QPushButton("清除渲染");
        _btnClearRendering->setEnabled(false);
        //V1.3：应用渲染=原型 symmetry-execute 恒主色（用户拍板删"待应用高亮"增强，
        //样式统一取共享库 mPrimaryButton/令牌，不再本地 QSS）
        _btnApplyRendering = new MxFunctions::mPrimaryButton("应用渲染");
        //三按钮等宽且允许随窄栏压缩（全局 QSS 的 width:80 与文字宽不再是硬下限）
        for (QPushButton* btn : { _btnSavePreset, _btnClearRendering, _btnApplyRendering })
        {
            btn->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
            actionBarLayout->addWidget(btn, 1);
        }
        _mainLayout->addWidget(actionBar);

        /********************云图变量********************/
        _changeFold = new FoldSection("云图变量", this);
        _comboCase = new QComboBox();
        _comboFrame = new QComboBox();
        _comboFrame->setObjectName("comboFrame");	//ui-shot 点分驱动用（切帧）
        _btnFirstFrame = new QPushButton("");
        _btnPreFrame = new QPushButton("");
        _btnNextFrame = new QPushButton("");
        _btnLastFrame = new QPushButton("");
        _btnFirstFrame->setIcon(QIcon(":/mMainWidgets/Icon/post/diyizheng.png"));
        _btnPreFrame->setIcon(QIcon(":/mMainWidgets/Icon/post/shangyizheng.png"));
        _btnNextFrame->setIcon(QIcon(":/mMainWidgets/Icon/post/xiayizheng.png"));
        _btnLastFrame->setIcon(QIcon(":/mMainWidgets/Icon/post/zuihouyizheng.png"));
        //帧导航按钮行：四钮均分填满字段列（与上方帧下拉框左右对齐）。
        //空文字按钮仍受全局 QSS QPushButton{width:80px} 支配（minHint=82px/个，
        //四钮一行约 368px 布局最小宽），窄参数栏下会把面板顶出可视区出现水平滚动条——
        //显式最小宽 26px 覆盖（布局取显式值），实测整行最小宽回落至 144px
        //实时动画 v1.2（用户拍板）：云图界面不放"实时动画"开关按钮（原帧导航行第 5 位
        //"实时动画" checkable 按钮及其 toggled 接线已删），浮层改按帧数自动显隐
        //（多帧→挂载+使能/单帧→停播+禁用，见 updateRealtimeAnimEnabled）
        //实时动画 v1.1：录制发起广播（录制入口在引擎链 RecordAnimation/RecordScreen 调用前发射）
        //→ 播放中的浮层按 v1 停止语义让路自停（录制×实时互斥正向；hub 单例与面板同生命周期）
        connect(mPostInfoHub::getInstance(), &mPostInfoHub::recordingStarted,
            this, &mPost3DResultPanel::slot_realtimeRecordingStarted);
        QWidget* frameNavRow = new QWidget();
        QHBoxLayout* frameNavLayout = new QHBoxLayout(frameNavRow);
        frameNavLayout->setContentsMargins(0, 0, 0, 0);
        frameNavLayout->setSpacing(6);
        for (QPushButton* btn : { _btnFirstFrame, _btnPreFrame, _btnNextFrame, _btnLastFrame })
        {
            btn->setMinimumSize(QSize(26, 24));
            frameNavLayout->addWidget(btn, 1);
        }
        _comboVariable = new QComboBox();
        _comboVariable->setObjectName("comboVariable");	//ui-shot 点分驱动用（切变量）
        _comboComponent = new QComboBox();
        _comboComponent->setObjectName("comboComponent");	//ui-shot 点分驱动用（切分量）
        _comboComplex = new QComboBox();
        _comboComplex->addItems({ "实部", "虚部", "幅值", "相位" });
        _comboLayer = new QComboBox();

        //工况
        _changeFold->addRow(new PanelLabel("工况"), _comboCase);
        //帧（PanelLabel 统一标签列宽；QLabel 会缩成文字宽破坏列对齐——V1.3 对照轮修正）
        _changeFold->addRow(new PanelLabel("帧"), _comboFrame);
        //帧切换按钮（均分行容器，左右缘与上方帧下拉框字段列对齐）
        _changeFold->addRow(new PanelLabel(""), frameNavRow);
        //变量
        _changeFold->addRow(new PanelLabel("变量"), _comboVariable);
        //分量
        _changeFold->addRow(new PanelLabel("分量"), _comboComponent);
        //层（顺序对齐原型：分量→层→复数）
        _changeFold->addRow(new PanelLabel("层"), _comboLayer);
        //复数
        _changeFold->addRow(new PanelLabel("复数"), _comboComplex);
        //使用角点数据（渲染TODO）
        _checkCornerData = new QCheckBox("使用角点数据");
        //变形系数（内联快捷输入 + 齿轮跳转变形控制面板；等值面接管共享组时隐藏本半区，同原型）
        auto lableDeform = new QLabel("变形系数");
        _editInlineDeformCoef = new QLineEdit();
        _editInlineDeformCoef->setText(MxFunctions::mNumberToString(1, 'f', 6));
        _editInlineDeformCoef->setValidator(new QDoubleValidator(this));
        _btnOpenDeformation = new QToolButton();
        _btnOpenDeformation->setIcon(QIcon(":/mMainWidgets/Icon/function/shezhi.svg"));
        _btnOpenDeformation->setToolTip("打开变形控制");
        _btnOpenDeformation->setFixedSize(QSize(26, 26));
        _deformCoefRow = new QWidget();
        QHBoxLayout* deformCoefLayout = new QHBoxLayout(_deformCoefRow);
        deformCoefLayout->setContentsMargins(0, 0, 0, 0);
        deformCoefLayout->setSpacing(MxFunctions::UiMetrics::rowGap());
        deformCoefLayout->addWidget(lableDeform, 0);
        deformCoefLayout->addWidget(_editInlineDeformCoef, 1);
        deformCoefLayout->addWidget(_btnOpenDeformation, 0);
        //组尾双列区（原型 tail-check-composite）：角点数据 | 变形系数 两等分通栏
        _variableTailRow = new QWidget();
        QHBoxLayout* variableTailLayout = new QHBoxLayout(_variableTailRow);
        variableTailLayout->setContentsMargins(0, 0, 0, 0);
        variableTailLayout->setSpacing(MxFunctions::UiMetrics::rowGap());
        variableTailLayout->addWidget(_checkCornerData, 1);
        variableTailLayout->addWidget(_deformCoefRow, 1);
        _changeFold->addRow(_variableTailRow, static_cast<QWidget*>(nullptr));
        //窄参数栏自适应：字段（帧下拉+四个导航按钮等）放不下时自动换行到标签下方，
        //宽栏仍两列——本组与对象及坐标系/数据处理为云图/等值面共享分组，两面板同生效
    /*    _changeFold->layout()->setRowWrapPolicy(QFormLayout::WrapLongRows);*/

        _changeFold->refreshLayout();
        m_foldWidgets.append(_changeFold);
        _mainLayout->addWidget(_changeFold);

        /********************对象及坐标系********************/
        //本折叠组字段渲染端暂未对接，先落UI并持久化到 guidata（TODO）
        _objectFold = new FoldSection("对象及坐标系", this);

        //对象选择：mPost 拾取栈拾取条（2026-11-18 由 mObjectPickComposite 复合行替换，
        //PostWindow.md 修改记录留档）——[类型下拉 | 拾取条] 单行并排（用户拍板）。
        //拾取条内建：点击条体激活拾取/再点停止（deactivatePick 开关语义）、详情弹窗
        //（分组+逐项删/按类型整删）、清空钮；激活/失活的视口过滤器与工具栏白名单由
        //slot_objectPickActiveChanged 收口，收割/高亮/重入回显由控制器自管。
        //_btnObjectScope 仍为隐形文案控制器（提供"所有X"本地化默认文案+guidata 持久化
        //口径，范围选项弹窗自该行下线，不再可达）
        _objectTypeCombo = new QComboBox(this);
        _objectTypeCombo->addItems({ "单元", "部件" });
        //V1.3 默认对象类型=部件（原型 renderObjectType value=part checked）
        _objectTypeCombo->setCurrentIndex(1);
        //CBBlue 颜色块对齐前处理拾取控件参考样式（点面接触同款蓝竖条；后处理 v1 高亮
        //走共享容器暂不同色，色块当前为视觉标识，临时高亮层接入后与视口高亮同色）
        _objectPickController = mPostGeneralPickUp::createPickWidget(CBBlue, this);
        _objectPickController->setSelectSetBtnVisible(false);         //后处理无集合语义
        _objectPickController->setKeepHighlightOnDeactivate(false);   //对齐原口径：解除拾取即清视口高亮
        _objectPickController->setAllowedTypes([this]() {
            //激活时按当前类型发工具栏白名单（0=单元/1=部件）
            QSet<MxFunctions::PickDataType> types;
            types.insert(_objectTypeCombo && _objectTypeCombo->currentIndex() == 0
                ? MxFunctions::PickDataType::Element : MxFunctions::PickDataType::Part);
            return types;
            });
        _btnObjectScope = new mPostPartScopeField(this);
        _btnObjectScope->hide();	//V1.3 起仅作范围状态/弹窗控制器，不再呈现字段按钮
        //类型与范围控制器同步（默认部件；构造期 scopeTextChanged 未连接，显式设默认文案）
        _btnObjectScope->setObjectType(_objectTypeCombo->currentIndex() == 0 ? 0 : 1);
        _objectPickController->setDefultTip(_btnObjectScope->scopeText());
        _objectFold->addRow(new PanelLabel("对象选择"), _objectTypeCombo, _objectPickController->widget());

        //坐标系 + 快速新建坐标系按钮（V1.3 已启用；用户拍板：优先接现有链路，无可接时提示待接入）
        _comboCoordSystem = new QComboBox();
        _comboCoordSystem->addItem("全局坐标系");
        _btnCoordAdd = new QToolButton();
        _btnCoordAdd->setText("＋");
        _btnCoordAdd->setToolTip("快速新建坐标系");
        _btnCoordAdd->setFixedSize(QSize(26, 26));
        QWidget* coordRow = new QWidget();
        QHBoxLayout* coordLayout = new QHBoxLayout(coordRow);
        coordLayout->setContentsMargins(0, 0, 0, 0);
        coordLayout->setSpacing(0);
        coordLayout->addWidget(_comboCoordSystem, 1);
        coordLayout->addWidget(_btnCoordAdd);
        _objectFold->addRow(new PanelLabel("坐标系"), coordRow);

        //组尾双列区（原型 coordinate-flags tail-check-composite）：使用跟踪坐标 | 边中点值
        _checkTrackingCoord = new QCheckBox("使用跟踪坐标");
        _checkMidsideValues = new QCheckBox("边中点值");
        _checkMidsideValues->setChecked(true);
        _checkMidsideValues->setEnabled(false);
        _coordTailRow = new QWidget();
        QHBoxLayout* coordTailLayout = new QHBoxLayout(_coordTailRow);
        coordTailLayout->setContentsMargins(0, 0, 0, 0);
        coordTailLayout->setSpacing(MxFunctions::UiMetrics::rowGap());
        coordTailLayout->addWidget(_checkTrackingCoord, 1);
        coordTailLayout->addWidget(_checkMidsideValues, 1);
        _objectFold->addRow(_coordTailRow, static_cast<QWidget*>(nullptr));

        _objectFold->refreshLayout();
        m_foldWidgets.append(_objectFold);
        _mainLayout->addWidget(_objectFold);

        /********************数据处理********************/
        _planishFold = new FoldSection("数据处理", this);

        //数据磨平：方式下拉 + 跨部件复选 同行，同原型（细分档位渲染端暂仅支持两态映射：无→默认，其余→磨平）
        _comboSmoothMethod = new QComboBox();
        _comboSmoothMethod->addItems({ "无", "简单磨平", "最大值", "最小值", "高级磨平", "差值磨平", "角点最大值", "角点最小值", "角点极值" });
        _checkAcrossParts = new QCheckBox("跨部件");
        _checkAcrossParts->setEnabled(false);
        QWidget* smoothRow = new QWidget();
        QHBoxLayout* smoothLayout = new QHBoxLayout(smoothRow);
        smoothLayout->setContentsMargins(0, 0, 0, 0);
        smoothLayout->setSpacing(4);
        smoothLayout->addWidget(_comboSmoothMethod, 1);
        smoothLayout->addWidget(_checkAcrossParts, 1);
        _planishFold->addRow(new PanelLabel("数据磨平"), smoothRow);

        //高级磨平变化率行（仅磨平方式=高级磨平时显示，渲染TODO）
        _checkVariationEnabled = new QCheckBox("变化率<");
        _editVariationLimit = new QLineEdit();
        _editVariationLimit->setText("0");
        _editVariationLimit->setValidator(new QDoubleValidator(this));
        _editVariationLimit->setEnabled(false);
        _variationRow = new QWidget();
        QHBoxLayout* variationLayout = new QHBoxLayout(_variationRow);
        variationLayout->setContentsMargins(0, 0, 0, 0);
        variationLayout->setSpacing(4);
        variationLayout->addWidget(_checkVariationEnabled);
        variationLayout->addWidget(_editVariationLimit, 1);
        variationLayout->addWidget(new QLabel("%"));
        _planishFold->addRow(new PanelLabel("高级磨平"), _variationRow);
        _planishFold->setRowVisible(_variationRow, false);

        //数值过滤：V1.3 三段拼接行 [左输入|无/大于/小于/之间|右输入]（原型 vp-range-filter；
        //禁用规则控件内置：大于→仅左、小于→仅右、之间→双启、无→双禁，禁用保值；
        //"等于"按原型删除——后端 EQUAL 分支保留兼容旧会话数据）
        _filterRow = new MxFunctions::mValueFilterRow(this);
        _filterRow->leftEdit()->setValidator(new QDoubleValidator(this));
        _filterRow->rightEdit()->setValidator(new QDoubleValidator(this));
        _filterRow->setLeftText(MxFunctions::mNumberToString(0, 'f', 6));
        _filterRow->setRightText(MxFunctions::mNumberToString(0, 'f', 6));
        _planishFold->addRow(new PanelLabel("数值过滤"), _filterRow);
        _planishFold->layout()->setRowWrapPolicy(QFormLayout::WrapLongRows);

        _planishFold->refreshLayout();
        m_foldWidgets.append(_planishFold);
        _mainLayout->addWidget(_planishFold);

        /********************图例（V1.3 更名，原"图例和标签"；信息标签行用户定案保留）********************/
        FoldSection* rangeFold = new FoldSection("图例", this);
        _comboRangeMode = new QComboBox();
        _comboRangeMode->addItems({ "所有部件", "显示部件" });
        //最大/最小值行=图例边界行（输入+复位+显隐，库 mLegendBoundRow；
        //复位恢复当前帧原始值域、眼睛独立控制极值显隐——语义沿用原三联行）
        _legendMax = new MxFunctions::mLegendBoundRow(this);
        _legendMin = new MxFunctions::mLegendBoundRow(this);
        _legendMax->lineEdit()->setText(MxFunctions::mNumberToString(0, 'f', 12));
        _legendMin->lineEdit()->setText(MxFunctions::mNumberToString(0, 'f', 12));
        _legendMax->lineEdit()->setObjectName("editLegendMax");	//ui-shot 文本驱动/自动化定位（沿用旧名）
        _legendMin->lineEdit()->setObjectName("editLegendMin");
        _legendMax->setTooltips("刷新最大值", "最大值：当前隐藏，点击显示", "最大值：当前显示，点击隐藏");
        _legendMin->setTooltips("刷新最小值", "最小值：当前隐藏，点击显示", "最小值：当前显示，点击隐藏");
        //图标沿用旧资源（用户定案）：复位=reset.png；眼睛=xianshi(On)/quxiao(Off) 双态
        //（业务 qrc 资源经控件图标注入口传入，库不持有业务资源）
        const QIcon resetIcon(QStringLiteral(":/mMainWidgets/Icon/post/reset.png"));
        QIcon eyeIcon;
        eyeIcon.addFile(QStringLiteral(":/mMainWidgets/Icon/post/xianshi.svg"), QSize(), QIcon::Normal, QIcon::On);   //选中态=显示
        eyeIcon.addFile(QStringLiteral(":/mMainWidgets/Icon/post/quxiao.svg"), QSize(), QIcon::Normal, QIcon::Off);  //未选中态=隐藏
        for (MxFunctions::mLegendBoundRow* row : { _legendMax, _legendMin })
        {
            row->setResetIcon(resetIcon);
            row->setEyeIcon(eyeIcon);
        }
        connect(_legendMax->lineEdit(), &QLineEdit::returnPressed, this, &mPost3DResultPanel::slot_editValueChanged);
        connect(_legendMin->lineEdit(), &QLineEdit::returnPressed, this, &mPost3DResultPanel::slot_editValueChanged);
        connect(_legendMax, &MxFunctions::mLegendBoundRow::resetClicked, this, &mPost3DResultPanel::slot_resetMostValue);
        connect(_legendMin, &MxFunctions::mLegendBoundRow::resetClicked, this, &mPost3DResultPanel::slot_resetMostValue);
        connect(_legendMax, &MxFunctions::mLegendBoundRow::visibilityToggled, this, &mPost3DResultPanel::slot_setPositionVisible);
        connect(_legendMin, &MxFunctions::mLegendBoundRow::visibilityToggled, this, &mPost3DResultPanel::slot_setPositionVisible);

        //模式
        rangeFold->addRow(new PanelLabel("模式"), _comboRangeMode);
        //图例范围：共框单选组（原型 mirror-planes，逐帧/全程/手动，各项带说明 tooltip；
        //逐帧=HyperView Dynamic Scale 默认，全程=Fixed Scale，手动=锁定上下限）
        _legendRangeChoice = new MxFunctions::mChoiceGroup({ "逐帧", "全程", "手动" }, this);
        _legendRangeChoice->setObjectName("legendRangeChoice");	//ui-shot 点分驱动用（切图例范围模式）
        _legendRangeChoice->setOptionTooltips({
            QString("当前帧的数值范围"), QString("当前工况整个分析过程的数值范围"),
            QString("手动设置最大值和最小值") });
        rangeFold->addRow(new PanelLabel("图例范围"), _legendRangeChoice);
        //最大值/最小值
        rangeFold->addRow(new PanelLabel("最大值"), _legendMax);
        rangeFold->addRow(new PanelLabel("最小值"), _legendMin);
        refreshLegendBoundEditState();	//初始逐帧档=不可编辑（含复位；眼睛独立）
        /********************样式（并入图例）********************/
        //插值阶数（V1.3 拆三行：阶数独立行；平滑方式/分布方式各为二选一单选组，原型 vp-contour-binary）
        _spinOrder = new QSpinBox();
        _spinOrder->setMinimum(2);
        _spinOrder->setMaximum(30);
        _spinOrder->setSingleStep(1);
        _spinOrder->setValue(9);
        connect(_spinOrder, SIGNAL(valueChanged(int)), this, SLOT(slot_discreteOrder(int)));
        rangeFold->addRow(new PanelLabel("插值阶数"), _spinOrder);

        _interpSmoothChoice = new MxFunctions::mChoiceGroup({ "平滑", "离散" }, this);
        connect(_interpSmoothChoice, &MxFunctions::mChoiceGroup::currentTextChanged,
            this, &mPost3DResultPanel::slot_discreteCombox);
        rangeFold->addRow(new PanelLabel("平滑方式"), _interpSmoothChoice);

        _interpSpacingChoice = new MxFunctions::mChoiceGroup({ "等差", "等比" }, this);
        connect(_interpSpacingChoice, &MxFunctions::mChoiceGroup::currentTextChanged,
            this, &mPost3DResultPanel::slot_discreteTypeComboBox);
        rangeFold->addRow(new PanelLabel("分布方式"), _interpSpacingChoice);

        //渲染图例
        _btnShow = new QPushButton("隐藏");
        _btnReset = new QPushButton("复位");
        _btnCustom = new QPushButton("自定义");
        connect(_btnShow, SIGNAL(clicked()), this, SLOT(slot_showOrHideColorBar()));
        connect(_btnReset, SIGNAL(clicked()), this, SLOT(slot_resetColorBar()));
        connect(_btnCustom, SIGNAL(clicked()), this, SLOT(slot_customColorBar()));
        //父对象必须留空：此处误传面板本体会令 QLayout 试图装成面板顶层布局（面板已有
        //_mainLayout → 构造期告警 already has a layout）；无父布局经 addRow 加入表单时所有权移交
        QHBoxLayout* renderHb = new QHBoxLayout();
        renderHb->setContentsMargins(0, 0, 0, 0);
        renderHb->setSpacing(0);
        //三按钮等分且允许随窄栏压缩（同操作条）：全局 QSS 的 width:80 使三钮一行
        //最小宽约 246px，窄参数栏字段列放不下时"自定义"被折叠块右缘裁掉
        for (QPushButton* btn : { _btnShow, _btnReset, _btnCustom })
        {
            btn->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
            //三字文案（自定义）+边距硬下限：窄栏等分压缩或分布异常低于 52 时文字被裁
            //（2026-10-26 三报实证：dock~306 时第三按钮被压至 ~42px，"自定义"文字出不全）
            btn->setMinimumWidth(52);
            renderHb->addWidget(btn, 1);
        }
        rangeFold->addRow(new PanelLabel("渲染图例"), renderHb);

        //信息标签（隐藏/复位为独立动作；自定义打开注释面板——模型/工况/帧号/时间/文件
        //字段选择与字体配置在注释面板，避免两处写右上角文本互相覆盖。
        //V1.3 原型删了该行，用户定案功能保留——见差异表裁定表）
        _btnLabelHide = new QPushButton("隐藏");
        _btnLabelReset = new QPushButton("复位");
        _btnLabelCustom = new QPushButton("自定义");
        //同渲染图例行：父对象留空（误传面板本体=构造期 already has a layout 告警）
        QHBoxLayout* lableHb = new QHBoxLayout();
        lableHb->setContentsMargins(0, 0, 0, 0);
        lableHb->setSpacing(0);
        //同渲染图例行：三按钮等分、可随窄栏压缩
        for (QPushButton* btn : { _btnLabelHide, _btnLabelReset, _btnLabelCustom })
        {
            btn->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
            btn->setMinimumWidth(52);   //同渲染图例行
            lableHb->addWidget(btn, 1);
        }
        rangeFold->addRow(new PanelLabel("信息标签"), lableHb);

        rangeFold->refreshLayout();
        m_foldWidgets.append(rangeFold);
        _mainLayout->addWidget(rangeFold);

        /********************信息********************/
        _infoFold = new FoldSection("信息", this);
        _infoFold->installEventFilter(this);

        _labelCase = new QLabel();
        _labelCase->setText("1");
        _labelTotalFrame = new QLabel();
        _labelTotalFrame->setText("1");
        _labelMax = new QLabel();
        _labelMax->setText("1");
        _labelMin = new QLabel();
        _labelMin->setText("1");
        //工况数
        _infoFold->addRow(new PanelLabel("工况数"), _labelCase);
        //总帧数
        _infoFold->addRow(new PanelLabel("总帧数"), _labelTotalFrame);
        //默认最大值
        _infoFold->addRow(new PanelLabel("默认最大值"), _labelMax);
        //默认最小值
        _infoFold->addRow(new PanelLabel("默认最小值"), _labelMin);

        // ===== 光源设置（V1.3：入口移至面板底部折叠组外整行，原型 contour-light-entry）=====
        initLightWidget(this);

        _infoFold->refreshLayout();
        m_foldWidgets.append(_infoFold);
        _mainLayout->addWidget(_infoFold);

        //光源入口（原型 advanced-options-button：光源设置靠左 + › 靠右 space-between，
        //hover/展开=selected 底+focus 描边；checkable 与弹窗开合联动）
        _btnLight = new QPushButton(QStringLiteral("光源设置"), this);
        _btnLight->setObjectName("cloudLightEntry");
        _btnLight->setCheckable(true);
        _btnLight->setCursor(Qt::PointingHandCursor);
        _btnLight->setFixedHeight(MxFunctions::UiMetrics::controlHeight());
        _btnLight->setStyleSheet(
            "QPushButton{text-align:left;padding-left:" + QString::number(MxFunctions::UiMetrics::menuItemInset()) +
            "px;border:1px solid " + MxFunctions::UiPalette::borderHex() + ";border-radius:" +
            QString::number(MxFunctions::UiMetrics::zoneRadius()) + "px;background:" +
            MxFunctions::UiPalette::surfaceHex() + ";color:" + MxFunctions::UiPalette::textHex() +
            ";font-size:" + QString::number(MxFunctions::UiMetrics::bodyFontSize()) + "px;}"
            "QPushButton:hover, QPushButton:checked{background:" + MxFunctions::UiPalette::selectedHex() +
            ";border-color:" + MxFunctions::UiPalette::focusHex() + ";}");
        //右缘"›"箭头（子 QLabel 透明鼠标事件，布局 stretch 推到右端；原型 DOM 第二 span）
        auto* lightArrow = new QLabel(QString::fromUtf8("›"), _btnLight);
        lightArrow->setAttribute(Qt::WA_TransparentForMouseEvents);
        lightArrow->setStyleSheet("QLabel{background:transparent;color:" +
            MxFunctions::UiPalette::textHex() + ";font-size:" +
            QString::number(MxFunctions::UiMetrics::bodyFontSize()) + "px;}");
        auto* lightEntryLayout = new QHBoxLayout(_btnLight);
        lightEntryLayout->setContentsMargins(MxFunctions::UiMetrics::menuItemInset(), 0,
            MxFunctions::UiMetrics::menuItemInset(), 0);
        lightEntryLayout->addStretch();
        lightEntryLayout->addWidget(lightArrow);
        connect(_btnLight, &QPushButton::toggled, this, &mPost3DResultPanel::slot_toggleLightPopup);
        _mainLayout->addWidget(_btnLight);
        _mainLayout->addStretch();


        //窄参数栏自适应：QComboBox 默认 minimumSizeHint 取最长条目文字宽（Qt 对非可编辑
        //组合框的最小宽即内容宽，不可压缩），长工况名/变量名会把面板最小宽度顶高，窄栏下
        //共享分组行右侧被裁切——改为最小内容长度策略后可压缩到省略显示，宽度仍随布局伸展
        const QList<QComboBox*> shrinkableCombos = findChildren<QComboBox*>();
        for (QComboBox* combo : shrinkableCombos)
        {
            combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
            combo->setMinimumContentsLength(4);
        }
        //插值阶数行两下拉已换共框单选组（mChoiceGroup）：自绘控件 minimumSizeHint=sizeHint
        //不可压缩，窄参数栏字段列放不下时整组被折叠块右缘裁掉——改 Ignored 允许随列宽
        //收缩（选项文字窄态走省略，宽栏外观不变）；图例范围三选一组同款处理
        for (MxFunctions::mChoiceGroup* choice : { _legendRangeChoice, _interpSmoothChoice, _interpSpacingChoice })
            choice->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        //窄参数栏自适应：QLineEdit 的 minimumSizeHint 随文本长度增长（图例和标签
        //最大/最小值行实证：22 字符科学计数法数值把输入框 hint 顶到 ~169px，行
        //最小宽 ~221px 超出字段列，重置/显隐按钮组被顶出折叠块右缘）——水平策略
        //统一 Ignored 后输入框可压缩（文本在框内滚动不再外溢）；微调框/下拉的
        //内部输入框由父控件自管几何，跳过
        for (QLineEdit* edit : findChildren<QLineEdit*>())
        {
            QWidget* pw = edit->parentWidget();
            if (qobject_cast<QAbstractSpinBox*>(pw) != nullptr || qobject_cast<QComboBox*>(pw) != nullptr)
                continue;
            edit->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        }
        //窄参数栏自适应（对象及坐标系/数据处理等分组，2026-10-26 四报）：QCheckBox
        //默认水平策略不可压缩、范围字段按钮受全局 QSS width:80 支配——窄栏下行最小
        //宽超出字段列时整行被折叠块右缘裁掉（"边中点值"复选框/"所有部件"字段按钮/
        //坐标系下拉+"＋"按钮实证）。统一 Ignored + 自身文字宽下限（同图例和标签分组
        //手法）：任何分布下文字完整，行最小宽不超 dock 最小字段列
        for (QCheckBox* check : findChildren<QCheckBox*>())
        {
            check->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
            check->setMinimumWidth(check->minimumSizeHint().width());
        }
        //（V1.3 起范围字段按钮退居隐形控制器，不再参与行宽；对象选择复合行/过滤行
        //内部子件自管几何，无需此处适配）

        //connect
        connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::create3DResultViewport, this, &mPost3DResultPanel::slot_create3DResultViewport);
        connect(this, SIGNAL(finishedReadPostFileSig(const QString&)), this, SLOT(slot_finishedImportFiles(const QString&)));
        connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::updateResultInfoGUI, this, &mPost3DResultPanel::updateResultInfoGUI);
        connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::updatePostViewName, this, &mPost3DResultPanel::setCurViewName);
        connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::changedFrame, this, &mPost3DResultPanel::slot_changedFrame);

        //【FR-09 二期（cloud-panel-frame-sync）】停机同步旁听（与云图菜单构造函数两条
        //旁听 mPostCloudMenuWidget.cpp:94-118 逐字对称）：postCloudFrameChanged 覆盖四停机点
        //——暂停/手动停止（mPostAnimation Q3-B 补发）、浮层停机（宿主 P2-2 补发+改动点 5
        //显式自调）、非循环自然播完（渲染库 playTick 停机补发→探测器链转发）、删除动画
        //（用户交互链）。播放中（任一动源）到达的逐帧通知按 FR-07/FR-08 冻结口径忽略；
        //停机后到达自调 updateResultInfoGUI 完成一次性完整回填=解除冻结+恢复逐操作刷新。
        //旁听内置迷你收敛段（对称复刻云图菜单失步收敛段判据）：预生成停机时镜像链只写
        //_cloudFrame 不写索引（面板侧失步对应物），先以会话 currentFrame 经帧名映射对齐
        //guidata->_cloudFrameIndex/_cloudFrame 再回填（浮层路径两帧字段逐 tick 新鲜，收敛
        //幂等 no-op）——自收敛设计使面板正确性不依赖两面板 connect 次序（01 §6.10 同批收敛）
        connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::postCloudFrameChanged, this,
            [this](int)
        {
            REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
            if (postRend == nullptr || postRend->getPostRender() == nullptr) return;
            if (postRend->getPostRender()->playbackState() == mxr::PlaybackState::Playing) return;
            if (isRealtimeAnimPlaying()) return;
            auto postRender = postRend->getPostRender();
            mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postRend->getPostDataName());
            mPostGUIData* guidata = postDB != nullptr ? getCVM->getGUIDataByPostRender(postRend) : nullptr;
            if (postDB != nullptr && guidata != nullptr
                && postRender->displayMode() != mxr::DisplayMode::ParametricAnimation
                && _comboFrame->count() > 0)
            {
                const int sessionFi = postRender->currentFrame();
                if (sessionFi > 0)
                {
                    for (int i = 0; i < _comboFrame->count(); ++i)
                    {
                        const QString itemText = _comboFrame->itemText(i);
                        const QString itemFrameName = itemText.contains(":") ? itemText.split(":").at(0) : itemText;
                        if (postDB->getFrameNumByCaseAndFrameName(guidata->_cloudCase, itemFrameName) == sessionFi)
                        {
                            if (i != guidata->_cloudFrameIndex)
                                guidata->_cloudFrameIndex = i;
                            break;
                        }
                    }
                    if (sessionFi != guidata->_cloudFrame)
                        guidata->_cloudFrame = sessionFi;
                }
            }
            updateResultInfoGUI();
        });
        //【FR-09 二期④】删除动画停机旁证：updateAnimationMenuGUI 在 stop 完成、会话当前帧
        //仍可读的时点到达（帧表保留可读）；viewname 过滤+双播放态守卫对称，非播放态自调
        //updateResultInfoGUI（生成完成自动播放起点到达时 state==Playing 被守卫跳过）
        connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::updateAnimationMenuGUI, this,
            [this](QString viewname)
        {
            if (viewname != _curViewName) return;
            //【引擎合一】删除动画链停机（会话已 Stopped）：浮层引擎态复位，
            //isPlaying 归假使下方闸放行 updateResultInfoGUI 解冻。
            //【unify-anim-playback】收口形态无关：参数化引擎档同链（enginePlayback 门）
            if (_realtimeAnimWidget != nullptr && _realtimeAnimWidget->enginePlayback())
                _realtimeAnimWidget->pausePlayback();
            REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
            if (postRend == nullptr || postRend->getPostRender() == nullptr) return;
            if (postRend->getPostRender()->playbackState() == mxr::PlaybackState::Playing) return;
            if (isRealtimeAnimPlaying()) return;
            updateResultInfoGUI();
        });

        connect(_btnPreFrame, &QPushButton::clicked, this, &mPost3DResultPanel::slot_changeFrame);
        connect(_btnFirstFrame, &QPushButton::clicked, this, &mPost3DResultPanel::slot_changeFrame);
        connect(_btnNextFrame, &QPushButton::clicked, this, &mPost3DResultPanel::slot_changeFrame);
        connect(_btnLastFrame, &QPushButton::clicked, this, &mPost3DResultPanel::slot_changeFrame);

        connect(_comboCase, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slot_chooseCase(const QString&)));
        connect(_comboFrame, SIGNAL(currentTextChanged(const QString&)), this, SLOT(slot_chooseFrame(const QString&)));
        connect(_comboVariable, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slot_chooseVariable(const QString&)));
        connect(_comboComponent, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slot_chooseCompo(const QString&)));
        connect(_comboLayer, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slot_chooseLayer(const QString&)));
        connect(_comboComplex, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slot_chooseComplex(const QString&)));
        connect(_comboRangeMode, SIGNAL(activated(int)), this, SLOT(slot_chooseRangeType(int)));
        connect(_comboSmoothMethod, SIGNAL(currentIndexChanged(int)), this, SLOT(slot_chooseSmoothMethod(int)));

        //操作条
        connect(_btnSavePreset, &QPushButton::clicked, this, &mPost3DResultPanel::slot_saveCloudPreset);
        connect(_btnClearRendering, &QPushButton::clicked, this, &mPost3DResultPanel::slot_clearRendering);
        connect(_btnApplyRendering, &QPushButton::clicked, this, &mPost3DResultPanel::slot_applyRendering);

        //内联变形系数 + 齿轮
        connect(_editInlineDeformCoef, SIGNAL(returnPressed()), this, SLOT(slot_applyInlineDeformation()));
        connect(_btnOpenDeformation, &QToolButton::clicked, this, &mPost3DResultPanel::slot_openDeformation);

        //对象及坐标系（渲染TODO，仅持久化；2026-11-18 拾取栈接线）
        connect(_objectTypeCombo, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slot_chooseObjectType(const QString&)));
        //拾取激活/失活收口：视口拾取过滤器按对象类型直设（勿用 activePickSig——该信号
        //只同步前处理工具栏按钮组，对后处理视口不生效）；失活恢复工具栏全部拾取类型
        connect(_objectPickController, &mPostPickWidgetController::pickActiveChanged,
            this, &mPost3DResultPanel::slot_objectPickActiveChanged);
        //范围文本变化（点选/回填/类型切换/语言）→ 拾取条空态默认文案（原型 renderObjectLabel）
        connect(_btnObjectScope, &mPostPartScopeField::scopeTextChanged, this, [this](const QString& text) {
            if (_objectPickController != nullptr)
                _objectPickController->setDefultTip(text);
            });
        //快速新建坐标系：接模型页同款全局信号（树侧槽现阶段提示"功能开发中"，
        //链路就绪后自动升级，用户拍板口径：优先接现有链路）
        connect(_btnCoordAdd, &QToolButton::clicked, this, []() {
            mMechGlobalSignals::getInstance()->clickPostCoordinateCreateSig(0);   //0=直角
            });
        connect(_checkCornerData, &QCheckBox::toggled, this, [this](bool checked) {
            REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
            if (postRend == nullptr) return;
            mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
            if (guidata == nullptr) return;
            guidata->_useCornerData = checked;
            });
        connect(_checkTrackingCoord, &QCheckBox::toggled, this, [this](bool checked) {
            REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
            if (postRend == nullptr) return;
            mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
            if (guidata == nullptr) return;
            guidata->_useTrackingCoord = checked;
            });

        //数据处理
        connect(_checkVariationEnabled, &QCheckBox::toggled, this, &mPost3DResultPanel::slot_variationToggled);
        connect(_checkAcrossParts, &QCheckBox::toggled, this, [this](bool checked) {
            REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
            if (postRend == nullptr) return;
            mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
            if (guidata == nullptr) return;
            guidata->_smoothAcrossParts = checked;   //渲染TODO：跨部件磨平
            });
        connect(_editVariationLimit, &QLineEdit::editingFinished, this, [this]() {
            bool ok = false;
            double value = _editVariationLimit->text().toDouble(&ok);
            if (!ok) return;   //校验器外的空串/非法输入不落库
            REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
            if (postRend == nullptr) return;
            mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
            if (guidata == nullptr) return;
            guidata->_smoothVariationLimit = value;   //渲染TODO：高级磨平
            });
        //数值过滤（V1.3 三段行）：模式切换/回车均只记脏，点"应用渲染"经 dispatchCloudFilter
        //按当前模式/左右值统一下发（启用规则控件内置）
        connect(_filterRow, &MxFunctions::mValueFilterRow::modeChanged, this, [this](int mode) {
            Q_UNUSED(mode);
            markCloudDirty();
            });
        connect(_filterRow->leftEdit(), &QLineEdit::returnPressed, this, &mPost3DResultPanel::markCloudDirty);
        connect(_filterRow->rightEdit(), &QLineEdit::returnPressed, this, &mPost3DResultPanel::markCloudDirty);

        //图例范围（渲染TODO，仅持久化）
        connect(_legendRangeChoice, &MxFunctions::mChoiceGroup::currentIndexChanged, this, &mPost3DResultPanel::slot_chooseLegendRange);

        //信息标签（隐藏/复位为独立动作；自定义打开注释面板；用户定案保留）
        connect(_btnLabelHide, &QPushButton::clicked, this, &mPost3DResultPanel::slot_labelHide);
        connect(_btnLabelReset, &QPushButton::clicked, this, &mPost3DResultPanel::slot_labelReset);
        connect(_btnLabelCustom, &QPushButton::clicked, this, &mPost3DResultPanel::slot_labelCustom);
        this->setMinimumWidth(300);

    }

    //v1.4 保险收尾（面板 hub 单例正常不析构，本函数纯防御）：先断四路信号防删除期
    //回环，再关设置弹窗、从工具栏壳摘除内容件（壳归主窗口所有不删）并回收控件条
    mPost3DResultPanel::~mPost3DResultPanel()
    {
        if (_realtimeAnimWidget == nullptr)
            return;
        //FR-13：面板析构退出未变形档——恢复既定系数防视口滞留置零态（QPointer 失效
        //时自动跳过；恢复后武装已清）
        if (_undeformedArmedRend != nullptr)
            restoreRealtimeDeformedShape(_undeformedArmedRend.data());
        _realtimeAnimWidget->closeSettingsPopup();
        if (_animBarAction != nullptr)
        {
            if (QWidget* bar = _realtimeAnimWidget->parentWidget())
            {
                if (auto* toolBar = qobject_cast<QToolBar*>(bar))
                    toolBar->removeAction(_animBarAction);
            }
            _animBarAction = nullptr;
        }
        _realtimeAnimWidget->deleteLater();
        _realtimeAnimWidget.clear();
    }

    //v1.4：面板退出瞬时态收口（§9 契约——宿主 setMainWidget 切换面板时触发，等值面/
    //切面图接管共享组等场景）：关闭设置弹窗防悬挂；播放链不动（切面板不切上下文，
    //与等值面接管共享组既定口径一致）
    void mPost3DResultPanel::initLightWidget(QWidget* parent)
    {
        //V1.3 光源弹窗（原型 lightSettingsPopup）：宽 300 锚定入口；标题栏"光源设置"+×；
        //光源位置 跟随/固定（XYZ 仅固定启用，三段拼接）；环境光/漫反射光/镜面光 三滑杆
        //（原型 0~1 步 0.01 默认 0.3——实现沿用 0~10 档 ÷10 与 guidata 既有存储刻度，见差异表）；
        //强度滑杆按原型删除（shiness 用 guidata 存量值下发，渲染行为不变）；
        //Qt::Popup 形态与对象范围弹窗同族（外点/Esc 自动关闭，无底部按钮）
        _lightWidget = new QWidget(parent, Qt::Popup | Qt::FramelessWindowHint);
        _lightWidget->setObjectName("LightWidget");
        _lightWidget->setAttribute(Qt::WA_StyledBackground, true);
        _lightWidget->setStyleSheet(QString(
            "QWidget#LightWidget{background:%1;border:1px solid %2;border-radius:%3px;}")
            .arg(MxFunctions::UiPalette::surfaceHex(), MxFunctions::UiPalette::borderHex())
            .arg(MxFunctions::UiMetrics::zoneRadius()));

        QVBoxLayout* outer = new QVBoxLayout(_lightWidget);
        outer->setContentsMargins(1, 1, 1, 1);
        outer->setSpacing(0);

        //标题栏 + 关闭钮（原型 picker-popup-header）
        auto* titleBar = new QWidget(_lightWidget);
        titleBar->setObjectName("LightWidgetTitleBar");
        titleBar->setFixedHeight(32);
        titleBar->setStyleSheet("QWidget#LightWidgetTitleBar{background:" +
            MxFunctions::UiPalette::headerHex() + ";}");
        QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
        titleLayout->setContentsMargins(10, 5, 10, 5);
        titleLayout->setSpacing(3);
        QLabel* titleLabel = new QLabel(QStringLiteral("光源设置"), titleBar);
        titleLabel->setStyleSheet(QString("QLabel{color:%1;font-size:%2px;font-weight:600;background:transparent;}")
            .arg(MxFunctions::UiPalette::textHex()).arg(MxFunctions::UiMetrics::bodyFontSize()));
        QToolButton* closeBtn = new QToolButton(titleBar);
        closeBtn->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/close3x.png")));
        closeBtn->setIconSize(QSize(22, 22));
        closeBtn->setToolTip(QStringLiteral("关闭光源设置"));
        closeBtn->setStyleSheet(
            "QToolButton{background:transparent;border:none;}"
            "QToolButton:hover{background:" + MxFunctions::UiPalette::hoverHex() + ";border-radius:2px;}");
        connect(closeBtn, &QToolButton::clicked, this, [this]() { slot_toggleLightPopup(false); });
        titleLayout->addWidget(titleLabel, 1);
        titleLayout->addWidget(closeBtn);
        outer->addWidget(titleBar);

        // ===== 内容区（原型 light-settings-body：grid gap 10 padding 8） =====
        const QString fieldFrame = QString(
            "QComboBox,QLineEdit{background:%1;border:1px solid %2;border-radius:%3px;color:%4;font-size:%5px;padding:0 6px;}"
            "QComboBox::drop-down{border:0;background:transparent;}"
            "QComboBox QAbstractItemView{background:%1;color:%4;selection-background-color:%6;selection-color:%4;}"
            "QLineEdit:disabled{color:%7;}")
            .arg(MxFunctions::UiPalette::surfaceHex(), MxFunctions::UiPalette::borderHex())
            .arg(MxFunctions::UiMetrics::zoneRadius()).arg(MxFunctions::UiPalette::textHex())
            .arg(MxFunctions::UiMetrics::bodyFontSize())
            .arg(MxFunctions::UiPalette::selectedHex(), MxFunctions::UiPalette::disabledHex());

        QGridLayout* grid = new QGridLayout();
        grid->setContentsMargins(10, 10, 10, 10);
        grid->setHorizontalSpacing(8);
        grid->setVerticalSpacing(10);
        grid->setColumnStretch(2, 1);
        int r = 0;

        //光源位置行：[光源位置][跟随/固定][X|Y|Z 三段拼接（仅固定启用）]
        QLabel* lbPos = new QLabel(QStringLiteral("光源位置"), _lightWidget);
        _comboLightPos = new QComboBox(_lightWidget);
        _comboLightPos->setFixedHeight(MxFunctions::UiMetrics::controlHeight());
        _comboLightPos->setStyleSheet(fieldFrame);
        _comboLightPos->addItems({ QStringLiteral("跟随"), QStringLiteral("固定") });   //V1.3：原"自定义"更名"固定"
        connect(_comboLightPos, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slot_setLightIsDependentCamera(const QString&)));

        _lineEditX = new QLineEdit(QStringLiteral("0"), _lightWidget);
        _lineEditY = new QLineEdit(QStringLiteral("0"), _lightWidget);
        _lineEditZ = new QLineEdit(QStringLiteral("0"), _lightWidget);
        QWidget* wCoord = new QWidget(_lightWidget);
        QHBoxLayout* coordLay = new QHBoxLayout(wCoord);
        coordLay->setContentsMargins(0, 0, 0, 0);
        coordLay->setSpacing(0);
        const int cr = MxFunctions::UiMetrics::zoneRadius();
        const int ctrlH = MxFunctions::UiMetrics::controlHeight();
        for (int i = 0; i < 3; ++i)
        {
            QLineEdit* edit = (i == 0) ? _lineEditX : (i == 1) ? _lineEditY : _lineEditZ;
            edit->setFixedHeight(ctrlH);
            //三段拼接：首左圆角/末右圆角/中间去左边框（原型 .light-coordinate）
            const QString radius = (i == 0) ? QString("%1px 0 0 %1px").arg(cr)
                : (i == 2) ? QString("0 %1px %1px 0").arg(cr) : QString("0px");
            edit->setStyleSheet(QString(
                "QLineEdit{background:%1;border:1px solid %2;border-left-width:%3;border-radius:%4;color:%5;font-size:%6px;padding:0 6px;}"
                "QLineEdit:disabled{color:%7;}")
                .arg(MxFunctions::UiPalette::surfaceHex(), MxFunctions::UiPalette::borderHex())
                .arg(i == 0 ? 1 : 0).arg(radius)
                .arg(MxFunctions::UiPalette::textHex()).arg(MxFunctions::UiMetrics::bodyFontSize())
                .arg(MxFunctions::UiPalette::disabledHex()));
            edit->setValidator(new QRegularExpressionValidator(
                QRegularExpression(QStringLiteral("^-?[0-9.]*$")), edit));   //允许负数和小数
            connect(edit, SIGNAL(returnPressed()), this, SLOT(slot_setLightPosition()));
            edit->setEnabled(false);   //默认"跟随"，坐标框禁用（仅固定启用）
            coordLay->addWidget(edit, 1);
        }
        grid->addWidget(lbPos, r, 0);
        grid->addWidget(_comboLightPos, r, 1);
        grid->addWidget(wCoord, r, 2);
        r++;

        //三滑杆行（环境光/漫反射光/镜面光）：[标签][0][滑杆][1]（原型 range-composite 只读上下限）
        struct LightRow { const char* label; };
        const LightRow rows[] = { { "环境光" }, { "漫反射光" }, { "镜面光" } };
        MxFunctions::mSlider** sliderSlots[] = { &_sliderAmbient, &_sliderDiffuse, &_sliderMirror };
        for (int i = 0; i < 3; ++i)
        {
            MxFunctions::mSlider* slider = new MxFunctions::mSlider(Qt::Horizontal);
            *sliderSlots[i] = slider;
            slider->setRange(0, 10);   //0~10 档 ÷10 = 0~1（guidata 既有刻度）
            slider->setValue(3);       //原型默认 0.3
            slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            connect(slider, SIGNAL(valueChanged(int)), this, SLOT(slot_setLightParameters(int)));

            QLabel* lbSlider = new QLabel(QString::fromUtf8(rows[i].label), _lightWidget);
            QLabel* lbLo = new QLabel(QStringLiteral("0"), _lightWidget);
            QLabel* lbHi = new QLabel(QStringLiteral("1"), _lightWidget);
            for (QLabel* lb : { lbSlider, lbLo, lbHi })
                lb->setStyleSheet(QString("QLabel{background:transparent;color:%1;font-size:%2px;}")
                    .arg(lb == lbSlider ? MxFunctions::UiPalette::textHex() : MxFunctions::UiPalette::mutedHex())
                    .arg(MxFunctions::UiMetrics::bodyFontSize()));
            grid->addWidget(lbSlider, r, 0);
            grid->addWidget(lbLo, r, 1, Qt::AlignRight | Qt::AlignVCenter);
            grid->addWidget(slider, r, 2);
            grid->addWidget(lbHi, r, 3, Qt::AlignLeft | Qt::AlignVCenter);
            r++;
        }

        outer->addLayout(grid);
        //原型定宽 300（高度随内容自适应；滑杆 Expanding 不约束 hint，必须钉宽防撑宽）
        _lightWidget->setFixedWidth(300);
        _lightWidget->adjustSize();
        _lightWidget->hide();
    }

    bool mPost3DResultPanel::new3DResultPanel(bool isRead, int caseid, QStringList paths,
        const QString& modelFilePath, bool notifyScanHint)
    {
        //读取结果文件
        if (isRead)
        {
            //通过分析工况创建（目录扫描分支无显式伴随网格概念，维持自动探测）
            if (caseid != -1)
            {
                _caseId = caseid;
                //mCaseBase* caseBase = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getCaseData(_caseId);
                //if (caseBase == nullptr)        return false;
                //_mxdbDirPath = caseBase->getSolveDir();
                return readPostResultFile();
            }
            //直接导入结果文件
            else
            {
                _caseId = caseid;
                return filterFiles(paths, modelFilePath, notifyScanHint);
            }
        }
        else
        {
            //根据已有数据创建三维结果节点和视口
            MechCommon::mMechGlobalSignals::getInstance()->create3DResultSig();

            if (_curPostRend == nullptr) return false;

            REBUILD_PostRend* postRend = _curPostRend;
            if (postRend == nullptr) return false;
            QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(caseid);
            if (postDB == nullptr) return false;
            QString postname = postDB->getName();
            postRend->setPostDataName(postname);

            QString viewName = _curPostRend->getViewportName();
            update3DResultPanel(viewName);
        }
        return true;
    }

    void mPost3DResultPanel::update3DResultViewport(int caseId, QStringList viewports)
    {
        _isUpdateResult = true;
        _updateViewNames = viewports;
        _caseId = caseId;

        readPostResultFile();
    }

    void mPost3DResultPanel::update3DResultPanel(QString viewname)
    {
        REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderByViewName(viewname);
        if (rebuildPostRend == nullptr)
        {
            return;
        }

        auto  postRender = rebuildPostRend->getPostRender();
        if (postRender == nullptr)
        {
            return;
        }

        QString postName = rebuildPostRend->getPostDataName();
        QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        if (modeldb == nullptr)
        {
            return;
        }

        //获取GUI数据
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(rebuildPostRend);
        if (guidata == nullptr)
        {
            return;
        }

        //同步面板当前视口为本次刷新目标：下方 slot_chooseCase/chooseFrame 级联
        //按 _curViewName 查渲染器（正常交互里两者一致；会话恢复批量应用时
        //_curViewName 停留在回放绑定视口，不同步会使全部视口的工况/帧/变量
        //级联打到同一视口，其余视口无数据呈空白）
        //R2 Q-LIFE-1 审计修复：本函数是 _curViewName 第 4 写者，此前直写无浮层守卫
        //——导入/会话恢复级联对视口 B 调用时（浮层挂 A 播放中），下一 tick 按 B 取
        //渲染器驱 B 切帧全链（B 被误驱、A 冻结），与 P3-1 同类旁路。守卫统一走
        //switchViewGuard：异名即关浮层+按钮归位；同名（导入到当前视口）零动作，
        //浮层存活+setFrameCount 动态刷新的既有行为不变
        switchViewGuard(viewname);
        _curViewName = viewname;
        _curPostRend = rebuildPostRend;
        // S6 v1.2 播放快速路径配套：新结果数据落地本视口（首次导入/更新导入/
        // 会话恢复/应用到视口，统一收口于本函数）——复位 slot_chooseVariable
        // 七元组身份缓存，强制随后级联（slot_chooseCase→slot_chooseFrame→
        // slot_chooseVariable）首跑走完整路径：分量/层列表按新库重建+数据纪元
        // bump。否则同视口换文件且各下拉文本与旧身份巧合同名时，播放快速路径
        // 会误判"身份未变"跳过列表重建，guidata 分量/层列表滞留旧文件内容。
        // P1-1 判别式本身未动：真实身份变化照旧 bump；本复位语义即"数据纪元
        // 已变"，与 notifyDataChanged 注释口径（值数据将变化→置待重验）一致
        _lastChooseVarIdentity.clear();

        //获取所有工况（分析工况在前 + 派生工况拼接在后——D5 修订：派生工况进
        //当前视口下拉；getAllCaseNames 排除派生，其余消费方零感知）
        QStringList cnames = modeldb->getAllCaseNames();
        cnames.removeAll(QString()); // 无工况帧（如新版格式无变量的初始帧）不进工况下拉
        const QStringList derivedCnames = modeldb->getDerivedCaseNames();
        for (const QString& derived : derivedCnames)
        {
            if (!cnames.contains(derived))
                cnames.append(derived);
        }
        _comboCase->blockSignals(true);
        _comboCase->clear();
        _comboCase->addItems(cnames);
        if (!cnames.isEmpty())
            _comboCase->setCurrentIndex(0);
        _comboCase->blockSignals(false);
        guidata->_cloudCases = cnames;
        // 由于 addItems/setCurrentIndex 在信号阻塞期间完成，currentIndexChanged 不会触发，
        // 这里手动触发工况选择，使后续帧、变量级联刷新。
        if (!cnames.isEmpty())
            slot_chooseCase(cnames.first());
        //级联为手动调用（sender 非帧下拉），延迟口径下只记脏不绘——此处显式整幅重绘，
        //保持旧版"导入/新建/批量恢复后首显"行为（时序同旧版：级联→绘制→FitView）
        drawCloudMap();

        //更新三维结果菜单栏
        MechCommon::mMechGlobalSignals::getInstance()->update3DResultMenu(viewname);
        MechCommon::mMechGlobalSignals::getInstance()->updateResultMenuGUI();

        rebuildPostRend->FitView();
        // R5 缺陷修复：纯异步链下帧实例多未就绪，FitView 因包围盒空早退（与
        // mBaseRend.cpp:813 aabb.valid() 同判据）——未生效则置挂起旗标，由
        // slot_showCloud 就绪主链尾部补偿一次（只补导入首显，防切帧跳相机）
        if (!rebuildPostRend->getAABBOfView().valid())
        {
            guidata->_pendingFitView = true;
        }
        rebuildPostRend->updatePaint();
    }

    void mPost3DResultPanel::selectCaseByName(const QString& viewname, const QString& caseName)
    {
        REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderByViewName(viewname);
        if (rebuildPostRend == nullptr)
        {
            return;
        }

        QString postName = rebuildPostRend->getPostDataName();
        QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        if (modeldb == nullptr)
        {
            return;
        }

        //获取GUI数据
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(rebuildPostRend);
        if (guidata == nullptr)
        {
            return;
        }

        //工况列表 = 分析工况 + 派生工况（口径同 update3DResultPanel 填充点）
        QStringList cnames = modeldb->getAllCaseNames();
        const QStringList derivedCnames = modeldb->getDerivedCaseNames();
        for (const QString& derived : derivedCnames)
        {
            if (!cnames.contains(derived))
                cnames.append(derived);
        }
        if (!cnames.contains(caseName))
            return;

        //同步面板当前视口（同 update3DResultPanel 的守卫口径）；派生帧物化=新结果
        //数据落地，复位变量身份缓存强制级联走完整路径
        switchViewGuard(viewname);
        _curViewName = viewname;
        _curPostRend = rebuildPostRend;
        _lastChooseVarIdentity.clear();

        _comboCase->blockSignals(true);
        _comboCase->clear();
        _comboCase->addItems(cnames);
        _comboCase->setCurrentText(caseName);
        _comboCase->blockSignals(false);
        guidata->_cloudCases = cnames;
        //重灌在信号阻塞下完成，手动触发工况级联：帧表/变量重建→标准绘制链接管
        slot_chooseCase(caseName);
    }

    bool mPost3DResultPanel::readPostResultFile()
    {
        if (_mxdbDirPath == nullptr)
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("未找到结果文件路径！"), true);
            return false;
        }

        QDir resultDir(_mxdbDirPath);
        if (!resultDir.exists())
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("未找到结果文件"), true);
            return false;
        }

        //获取文件夹下的所有结果文件
        resultDir.setFilter(QDir::Files | QDir::NoSymLinks);
        QStringList nameFilters;
        nameFilters << "*.mxdb" << "*.mxdb0" << "*.op2" << "*.rst" << "*.rth" << "*.odb" << "*.h3d" << "d3plot"
                    << "*.vtk" << "*.vtp" << "*.vtu" << "*.vtm" << "*.pvtp" << "*.pvtu";
        resultDir.setNameFilters(nameFilters);

        // 获取所有文件
        QStringList allResultFiles;
        QStringList fileList = resultDir.entryList();
        for (const QString& fileName : fileList)
        {
            QString fullPath = resultDir.absoluteFilePath(fileName);
            allResultFiles.append(fullPath);
        }

        if (allResultFiles.size() < 1)
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("未找到结果文件！"), true);
            return false;
        }

        //筛选新旧格式文件读取
        return filterFiles(allResultFiles);
    }

    bool mPost3DResultPanel::filterFiles(const QStringList paths, const QString& modelFilePath,
        bool notifyScanHint)
    {
        // vtk 系文件自动成组（多选 = 帧序列，文件名自然排序为帧序），与其余格式
        // 混选互不干扰：其余文件仍走 native/mxdb 原有分类逻辑
        QStringList vtkFiles;
        QStringList others;
        for (const QString& name : paths)
        {
            if (name.trimmed().isEmpty())
                continue;
            if (MIOFile::mIOVtkFile::isVtkFile(name))
                vtkFiles.append(name.trimmed());
            else
                others.append(name);
        }
        const bool vtkOk = vtkFiles.isEmpty() ? true : importVtkFile(vtkFiles);
        if (others.isEmpty())
            return vtkOk;

        QHash<unsigned int, int> versions{};
        versions.insert(0xA1B1C202, 0);
        versions.insert(0xA1B1C201, 0);
        // 新版记录式格式（v07.mxdb 规范，单文件多工况多帧，无 .mxdb0）
        versions.insert(0xA1B1C601, 0);
        versions.insert(0xA1B1C602, 0);

        //读文件指针
        QString nativeFile;
        QString bdfFile;
        if (MIOFile::mIONativeFile::tryGetNativeSelection(others, &nativeFile, &bdfFile))
        {
            //显式传入的伴随网格优先于paths里多选的.bdf（服务接口modelFilePath透传）
            const QString effectiveBdf = modelFilePath.isEmpty()
                ? bdfFile
                : QDir::cleanPath(QFileInfo(modelFilePath).absoluteFilePath());
            return importNativeFile(nativeFile, effectiveBdf) && vtkOk;
        }

        std::ifstream readf;
        //统计新格式文件
        for (auto name : others)
        {
            if (name.trimmed() == "")
                continue;
            readf.open(name.trimmed().toLocal8Bit(), ios::in | ios::binary);
            if (!readf.is_open())
            {
                continue;
            }

            readf.seekg(0);
            unsigned int v;
            readf.read((char*)&v, sizeof(unsigned int));

            if (versions.constFind(v) != versions.cend())
            {
                versions[v]++;
            }

            readf.close();
        }

        //所选文件没有新格式文件
        if (versions[0xA1B1C202] == 0 && versions[0xA1B1C201] == 0
            && versions[0xA1B1C601] == 0 && versions[0xA1B1C602] == 0)
        {
            return importOldFile(others) && vtkOk;
        }
        else if ((versions[0xA1B1C202] == 1 || versions[0xA1B1C201] == 1
            || versions[0xA1B1C601] == 1 || versions[0xA1B1C602] == 1) && others.size() == 1)//所选文件只有一个新格式文件
        {
            return importNewFile(others[0], notifyScanHint) && vtkOk;
        }
        return false;
    }

    void mPost3DResultPanel::importRepeatModel(QString modelName)
    {
        //QString postName = modelName;
        ////新建视口并存储数据图
        //REBUILD_PostRend* postRend = _curPostRend;
        //if (postRend == nullptr)
        //	return;
        //QString oldName = postRend->getPostDataName();
        //QPointer<mPostDataBase> dp = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(oldName);
        //if (dp != nullptr)
        //{
        //	if (postRend != nullptr)
        //	{
        //		oldName = postRend->getPostDataName();
        //		QPointer<mPostDataBase> dp = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(oldName);
        //		if (dp != nullptr)
        //		{
        //			if (oldName != postName)
        //			{
        //				int count = mViewportManager::getInstance()->getRepeatModelViewCount(oldName);
        //				if (count <= 1)
        //				{
        //					//删除数据库
        //					getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).deletePostDatabase(oldName);
        //				}
        //				else
        //				{
        //					//清除该视图对应的模型数据库中变化的数据
        //					getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).deleteViewData(postRend->getViewportName(), oldName);
        //				}
        //			}
        //			QString viewName = postRend->getViewportName();
        //			PostCommon::CameraManager::getInstance().removeObserver(postRend);
        //			postRend = mViewportManager::getInstance()->clearViewport3D(postRend);
        //		}
        //	}
        //}
        //else
        //{
        //	mSubWindow* msWindow = mViewportManager::getInstance()->getSubWindowFromViewportREBUILD(postRend);
        //	mViewportManager::getInstance()->activeSub(msWindow);
        //}

        //_curPostRend = postRend;
        //postRend->setPostDataName(postName);
        //auto postrender = postRend->getPostRender();

        //if (postrender != nullptr)
        //{

        //	auto postRender = postRend->getPostRender();
        //	connect(postRender.get(), &mxr::REBUILD_PostRender::finishedPickSig, mViewportManager::getInstance(), &mViewportManager::finishedPickSigManger);
        //	connect(postRender.get(), &mxr::REBUILD_PostRender::updateAnimationPanelSig, mViewportManager::getInstance(), &mViewportManager::updateAnimationPanelSigManger);
        //	connect(postRender.get(), &mxr::REBUILD_PostRender::finishedDragSig, mViewportManager::getInstance(), &mViewportManager::finishedDragSigManger);

        //}

        //QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        //int frameId = 1;
        //QString viewName = postRend->getViewportName();

        ////设置当前视图对应的部件的显隐、颜色数据以及部件包含的单元的显隐、选中数据
        //auto model = modeldb->getModelData(frameId);
        //if (model == nullptr)
        //	return;
        //model->setCurViewPartData(postRend->getViewportName());

        ////重构渲染面
        //mPostParseMesh* parse = new mPostParseMesh(this);
        //parse->ParseMesh(model);
        //delete parse;
        //parse = nullptr;

        //QString str = postName + "_##_" + viewName;
        ////读取后处理文件完成
        //emit finishedReadPostFileSig(str);

        //// 没导入完成前禁止鼠标操作
        //QApplication::restoreOverrideCursor();
    }

    bool mPost3DResultPanel::importNativeFile(const QString& filename,
        const QString& explicitBdfFile)
    {
        QFileInfo fileInfo(filename);
        const QString resultFile = QDir::cleanPath(fileInfo.absoluteFilePath());
        const QString suffix = fileInfo.suffix().toLower();
        const bool isD3plot = fileInfo.fileName().compare(QStringLiteral("d3plot"), Qt::CaseInsensitive) == 0;
        const bool isNative = suffix == QStringLiteral("op2") ||
                              suffix == QStringLiteral("rst") ||
                              suffix == QStringLiteral("rth") ||
                              suffix == QStringLiteral("odb") ||
                              suffix == QStringLiteral("h3d") ||
                              isD3plot;
        if (!fileInfo.exists() || !isNative)
            return false;
        if (MIOFile::mIONativeFile* existing = nativeReaderFor(resultFile))
        {
            if (existing->isModelAlive())
                return true;   //同文件且原实例仍在：沿用既有reader（同文件多实例暂不支持，第二棵树无数据）
            //悬空reader（所属模型已删，如mtime"删旧重建"）：废弃后走全新导入，
            //否则重建实例被去重早退挡住、静默拿不到结果数据
            _nativeReaders.remove(resultFile);
            _nativeReaderOwner.remove(existing);
            if (existing->isRunning())
                existing->wait();   //极端时序兜底：首次导入尚未完成模型即被删
            existing->deleteLater();
        }

        // OP2 伴随 BDF：显式传入优先，否则沿用旧候选规则探测；找不到不阻断，
        // 交由库自行探测（OP2 内嵌几何或同目录 .bdf/.nas），均无才在读取线程报错
        QString bdfFile;
        if (suffix == QStringLiteral("op2"))
        {
            if (explicitBdfFile.isEmpty())
            {
                bdfFile = MIOFile::mIONativeFile::findCompanionBdf(resultFile);
            }
            else
            {
                //显式伴随网格同步校验：不存在/后缀非法直接失败（上层回滚模型树，
                //服务调用方经syncErrorDetail感知、showPostResult重建重放返回InvalidKey）
                //——留到读取线程才失败的话key已返回，只能靠异步回滚作废key
                const QFileInfo bdfInfo(explicitBdfFile);
                const QString bdfSuffix = bdfInfo.suffix().toLower();
                if (!bdfInfo.exists() ||
                    (bdfSuffix != QStringLiteral("bdf") && bdfSuffix != QStringLiteral("nas")))
                {
                    const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
                    mGlobalSignals::getInstance()->outputMessageSig(2, isChinese
                        ? QStringLiteral("OP2伴随网格文件不存在或不是.bdf/.nas：%1").arg(explicitBdfFile)
                        : QStringLiteral("OP2 companion mesh file does not exist or is not .bdf/.nas: %1").arg(explicitBdfFile),
                        true);
                    return false;
                }
                bdfFile = QDir::cleanPath(bdfInfo.absoluteFilePath());
            }
        }

        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        QPointer<mPostDataBase> modeldbs = new mPostDataBase(resultFile);
        modeldbs->setCaseId(_caseId);
        getCVM->getModelData()->addPhysicalFieldP(modeldbs);

        MIOFile::mIONativeFile* reader = new MIOFile::mIONativeFile(resultFile, bdfFile, modeldbs, this);
        _nativeReaders.insert(resultFile, reader);
        //捕获宿主模型：读取线程内的解析失败发生在本函数返回之后，届时当前模型可能
        //已漂移，失败分支按捕获ID删库/通知（见 slot_nativeThreadFinished）
        _nativeReaderOwner.insert(reader, getCVM->getModelData()->getModelDataID());
        connect(reader, &MIOFile::mIONativeFile::finishedNativeFileSig,
            this, &mPost3DResultPanel::slot_nativeThreadFinished);
        reader->start();
        return true;
    }

    bool mPost3DResultPanel::importVtkFile(const QStringList& filenames)
    {
        if (filenames.isEmpty())
            return false;
        // 组键 = 排序后首文件规范化路径（帧序同时在此确定）
        const QStringList series = MIOFile::mIOVtkFile::sortAsSeries(filenames);
        const QString resultFile = QDir::cleanPath(QFileInfo(series.first()).absoluteFilePath());
        if (MIOFile::mIOVtkFile* existing = vtkReaderFor(resultFile))
        {
            if (existing->isModelAlive())
                return true;   //同组文件且原实例仍在：沿用既有reader（语义同 native 链路）
            _vtkReaders.remove(resultFile);
            _vtkReaderOwner.remove(existing);
            if (existing->isRunning())
                existing->wait();
            existing->deleteLater();
        }

        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        QPointer<mPostDataBase> modeldbs = new mPostDataBase(resultFile);
        modeldbs->setCaseId(_caseId);
        getCVM->getModelData()->addPhysicalFieldP(modeldbs);

        MIOFile::mIOVtkFile* reader = new MIOFile::mIOVtkFile(series, modeldbs, this);
        _vtkReaders.insert(resultFile, reader);
        //捕获宿主模型（同 _nativeReaderOwner：失败分支按捕获ID删库/通知）
        _vtkReaderOwner.insert(reader, getCVM->getModelData()->getModelDataID());
        connect(reader, &MIOFile::mIOVtkFile::finishedVtkFileSig,
            this, &mPost3DResultPanel::slot_vtkThreadFinished);
        reader->start();
        return true;
    }

    bool mPost3DResultPanel::importNewFile(const QString& filename, bool notifyScanHint)
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        //重导同文件：摘除池内旧 reader（其 modeldb 已随旧库删除而悬空，沿用只会读取失败）
        if (MIOFile::mMxdbThread* pooled = _mxdbNewReaderPool.take(filename))
            pooled->deleteLater();
        //auto db = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(filename);
        //if (db != nullptr)
        //{
        //	QFileInfo file(filename);
        //	QString modelName = file.baseName();
        //	//导入重复模型
        //	importRepeatModel(modelName);
        //	return;
        //}

        QPointer<mPostDataBase> modeldbs = new mPostDataBase(filename);
        modeldbs->setCaseId(_caseId);
        getCVM->getModelData()->addPhysicalFieldP(modeldbs);
        mMxdbThread* mxdbImport = new mMxdbThread(filename, modeldbs);
        //新版格式大文件扫描提示开关：仅用户手动导入提示（会话恢复的自动导入静默）
        mxdbImport->setNotifyScanHint(notifyScanHint);
        //捕获宿主模型（同 _nativeReaderOwner：失败分支按捕获ID操作，防切模型漂移）
        _mxdbThreadOwner.insert(mxdbImport, getCVM->getModelData()->getModelDataID());
        connect(mxdbImport, &mMxdbThread::finished, this, &mPost3DResultPanel::slot_mxdbThreadFinished);
        mxdbImport->start();
        return true;
    }

    bool mPost3DResultPanel::importOldFile(const QStringList& filenames)
    {
        return this->openPostFileNames(filenames);
    }

    bool mPost3DResultPanel::openPostFileNames(const QStringList& names)
    {
        if (names.size() < 1)
        {
            return false;
        }
        /**********************整理选中的文件*************************/
        //按照名称筛选分类
        QMultiHash<QString, QString> postframefiles;
        QHash<QString, QString> postinitfile;

        //判断是否选中
        bool isSelect = false;
        QFileInfoList fileInfos;
        for (auto name : names)
        {
            //获取选中文件中的mxdb0文件
            if (name.contains(".mxdb0"))
            {
                QFileInfo iniinfo(name);
                fileInfos.append(iniinfo);
                isSelect = true;
            }
        }

        if (!isSelect)
        {
            //获取当前文件夹下所有的mxdb0文件
            QFileInfo initinfo(names.at(0));
            QDir filedir = initinfo.absoluteDir();
            QStringList db0filter = QStringList{ QString("*.mxdb0") };
            fileInfos = filedir.entryInfoList(db0filter, QDir::Files, QDir::Time | QDir::Reversed);
        }

        for (QFileInfo finfo : fileInfos)
        {
            postinitfile.insert(finfo.completeBaseName(), finfo.absoluteFilePath());
        }
        //提取所有选中的mxdb文件
        QStringList initnames = postinitfile.keys();
        const bool hasEmptyInit = initnames.contains(QString());
        for (QString filename : names)
        {
            QFileInfo finfo(filename);
            QString fsuffix = finfo.suffix();
            QString fname = finfo.completeBaseName();
            if (fsuffix != "mxdb")
                continue;
            QString bestInit;
            int bestLen = -1;
            for (QString initname : initnames)
            {
                if (initname.isEmpty())
                    continue;
                int nbyte = initname.size();
                if (nbyte + 1 <= fname.size() && fname.left(nbyte) == initname && fname.at(nbyte) == "_")
                {
                    if (nbyte > bestLen)
                    {
                        bestLen = nbyte;
                        bestInit = initname;
                    }
                }
            }
            if (!bestInit.isEmpty())
            {
                postframefiles.insert(bestInit, filename);
            }
            else if (hasEmptyInit && initnames.size() == 1)
            {
                postframefiles.insert(QString(), filename);
            }
        }
        QStringList postnames = postframefiles.uniqueKeys();

        if (postnames.size() <= 0)
        {
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig("结果文件没有帧文件或者初始构型文件！");
            }
            else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("The result file does not have a frame file or initial configuration file!"), true);
            }
            return false;
        }

        for (QString pname : postnames)
        {
            QStringList framefiles = postframefiles.values(pname);
            if (framefiles.size() < 1)
            {
                if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString("结果 %1 没有帧文件！").arg(pname), true);
                }
                else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString("Result %1 no frame file!").arg(pname), true);
                }
                continue;
            }
            //若没有选择初始构型文件，则自动获取
            QString initfile = postinitfile.value(pname);
            if (initfile.isEmpty())
            {
                if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString("结果 %1 没有初始构型文件！").arg(pname), true);
                }
                else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString("Result %1 no initial configuration file available!").arg(pname), true);
                }
                continue;
            }
            //对帧文件进行排序
            QCollator collator;
            collator.setNumericMode(true);
            std::sort(framefiles.begin(), framefiles.end(), [&collator](const QString& str1, const QString& str2) {
                return collator.compare(str1, str2) < 0;
                });

            //重导同结果：摘除老格式池内旧 reader（库查找已落空，沿用只会读取失败）
            if (MIOFile::mIMxdbFile1* pooled = _mxdbOldReaderPool.take(initfile))
                pooled->deleteLater();
            QFileInfo inifinfo(initfile);
            //QString dbName = inifinfo.baseName();
            //QPointer<mPostDataBase> db = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(dbName);
            //if (db != nullptr)
            //{
            //	importRepeatModel(dbName);
            //	return;
            //}

            QString inifname = inifinfo.completeBaseName();
            if (inifname.isEmpty())
                inifname = inifinfo.dir().dirName();
            QPointer<mPostDataBase> modeldbs = new mPostDataBase(inifname);
            modeldbs->setCaseId(_caseId);
            modeldbs->setDataFlag(true);
            modeldbs->setFile(initfile);
            getCVM->getModelData()->addPhysicalFieldP(modeldbs);
            //记录 postName -> 初始帧文件，供后续按需读取判断产品线使用
            _postInitFiles.insert(pname, initfile);
            //读取文件
            mIMxdbFile1* mxdbThread1 = new mIMxdbFile1(initfile, framefiles, modeldbs);
            //捕获宿主模型（同 _nativeReaderOwner；整组文件同宿主，逐线程登记）
            _mxdbOldThreadOwner.insert(mxdbThread1, getCVM->getModelData()->getModelDataID());
            connect(mxdbThread1, SIGNAL(readFileSuccessSig(const QString&)), this, SLOT(slot_readFileSuccess(const QString&)));
            // 这个信号和槽函数是有边界的，不会影响到其他的slot_mxdbThreadFinished
            connect(mxdbThread1, SIGNAL(finishedImportAllMxDbFileSig()), this, SLOT(slot_mxdbOldThreadFinished()));
            mxdbThread1->start();
        }
        return true;
    }

    void mPost3DResultPanel::getLongestInitFileName(QStringList& ininames)
    {
        if (ininames.isEmpty())
        {
            return;
        }

        int maxLength = 0;
        foreach(const QString & str, ininames)
        {
            maxLength = qMax(maxLength, str.length());
        }

        QStringList filteredList;
        foreach(const QString & str, ininames)
        {
            if (str.length() == maxLength)
            {
                filteredList.append(str);
            }
        }
        ininames = filteredList;
    }

    void mPost3DResultPanel::updateInformation()
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;

        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;
        QString postname = postRend->getPostDataName();

        mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
        if (postDB == nullptr)
            return;

        //获取GUI数据
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr)
            return;

        int totalcase = postDB->getAllCaseNames().size();
        int totalframe = postDB->getTotalFrameCount();
        _labelCase->setText(MxFunctions::mNumberToString(totalcase));
        _labelTotalFrame->setText(MxFunctions::mNumberToString(totalframe));
        _labelMax->setText(MxFunctions::mNumberToString(guidata->_cloudMaxValue));
        _labelMin->setText(MxFunctions::mNumberToString(guidata->_cloudMinValue));
    }

    int mPost3DResultPanel::saveCloudPresetSnapshot()
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return -1;

        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return -1;

        MechPreData::CloudPresetSnapshot snapshot;
        snapshot._presetName = QString("云图缓存_%1").arg(guidata->_cloudPresets.size() + 1);
        //变量组与范围值取面板当前状态：guidata 仅在绘制时提交（slot_showCloud），
        //未重新应用渲染前是旧值（曾出现切换分量后缓存仍是旧分量的问题）
        snapshot._cloudCase = _comboCase->currentText();
        snapshot._cloudFrame = _comboFrame->currentText();
        snapshot._cloudVarName = _comboVariable->currentText();
        snapshot._cloudCompo = _comboComponent->currentText();
        snapshot._cloudLayerName = _comboLayer->currentText();
        snapshot._cloudComplex = _comboComplex->currentText();
        snapshot._smoothMethod = _comboSmoothMethod->currentText();
        //数据处理组其余两项取面板当前状态（与变量组同口径；跨部件 guidata 仅在交互时同步）
        snapshot._smoothAcrossParts = _checkAcrossParts->isChecked();
        snapshot._smoothVariationEnabled = _checkVariationEnabled->isChecked();
        snapshot._smoothVariationLimit = _editVariationLimit->text().toDouble();
        snapshot._rangeMode = _comboRangeMode->currentText();
        snapshot._cloudMaxValue = _legendMax->lineEdit()->text().toDouble();
        snapshot._cloudMinValue = _legendMin->lineEdit()->text().toDouble();
        //离散由各自控件交互即时提交 guidata，直接读取即可
        snapshot._cloudDiscrete = guidata->_cloudDiscrete;
        snapshot._cloudDiscreteNum = guidata->_cloudDiscreteNum;
        snapshot._cloudIsEquivariance = guidata->_cloudIsEquivariance;
        //——对象及坐标系——
        snapshot._cloudCoordinate = guidata->_cloudCoordinate;
        snapshot._currentCoordinateName = guidata->_currentCoordinateName;
        snapshot._renderCoordSystem = guidata->_renderCoordSystem;
        //——图例与显隐标注——
        snapshot._legendRange = guidata->_legendRange;
        snapshot._cloudShowMax = guidata->_cloudShowMax;
        snapshot._cloudShowMin = guidata->_cloudShowMin;
        snapshot._cloudShowMaxLine = guidata->_cloudShowMaxLine;
        snapshot._cloudShowMinLine = guidata->_cloudShowMinLine;

        guidata->_cloudPresets.append(snapshot);
        return guidata->_cloudPresets.size() - 1;
    }

    bool mPost3DResultPanel::eventFilter(QObject* obj, QEvent* ev)
    {
        //信息组收起时联动关闭光源弹窗（面板级互斥，原型 closeLightSettings 时机之一）
        if (obj == _infoFold && ev->type() == QEvent::Hide && _lightWidget != nullptr)
            _lightWidget->hide();
        //光源弹窗隐藏（外点/Esc/关闭钮）→ 归位入口钮 checked 态（屏蔽回路触发）
        if (obj == _lightWidget && ev->type() == QEvent::Hide && _btnLight != nullptr
            && _btnLight->isChecked())
        {
            _btnLight->blockSignals(true);
            _btnLight->setChecked(false);
            _btnLight->blockSignals(false);
        }
        return QWidget::eventFilter(obj, ev);
    }

    void mPost3DResultPanel::hideEvent(QHideEvent* event)
    {
        //面板退出（宿主 setMainWidget 切换面板触发，见技能 §9）：退出全部瞬时态——
        //拾取态、光源弹窗；已拾取数据与面板参数保留（拾取条弹窗为 Qt::Popup，
        //随宿主隐藏自动关闭，防悬挂双保险同旧清单弹窗口径）
        mPanelBase::hideEvent(event);
        //拾取控件收口：失活（清视口过滤器/高亮/工具栏白名单由 pickActiveChanged 链收口）
        //并置隐藏态拒绝再激活；面板重显由 showEvent 对称恢复。
        //守卫：折叠组已被等值面面板接管（拾取条物理在其布局内）时只失活不隐藏——
        //setPickWidgetVisible(false) 的显式隐藏跨 reparent 保持，会把外借中的拾取条
        //藏死且本面板 showEvent 在等值面激活期间不会发生（当前挂载时序为"先切面板
        //后接管"，此守卫不触发，纯防时序变化；接管期的恢复/收口由等值面侧调用
        //setSharedObjectPickVisible/deactivateSharedObjectPick 承担）
        if (_objectPickController != nullptr)
        {
            if (_objectFold != nullptr && _objectFold->parentWidget() != this)
                deactivateSharedObjectPick();
            else
                _objectPickController->setPickWidgetVisible(false);
        }
        if (_lightWidget != nullptr && _lightWidget->isVisible())
            _lightWidget->close();
        //v1.4 动画设置弹窗随面板退出收口（防悬挂；原独立 hideEvent 实现并入此处）
        if (_realtimeAnimWidget != nullptr)
            _realtimeAnimWidget->closeSettingsPopup();
    }

    void mPost3DResultPanel::showEvent(QShowEvent* event)
    {
        mPanelBase::showEvent(event);
        //隐藏期 setPickWidgetVisible(false) 的对称恢复（控件可再激活；
        //已拾取数据保留在控件数据模型，条体显示随数据自动恢复）
        if (_objectPickController != nullptr)
            _objectPickController->setPickWidgetVisible(true);
    }

    void mPost3DResultPanel::slot_saveCloudPreset()
    {
        int presetIndex = saveCloudPresetSnapshot();
        if (presetIndex < 0) return;

        //树上在"云图"根下登记缓存节点（接收端仅缓存界面参数、不新建视口，2026-11-05
        //去视口化；恢复＝单击缓存节点回填重绘，或回填后点"应用渲染"）
        MechCommon::mMechGlobalSignals::getInstance()->createCloudPresetSig(_curViewName, presetIndex);
    }

    void mPost3DResultPanel::slot_clearRendering()
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;

        auto postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        //清除云图着色，恢复部件基色显示；不删缓存节点、不清工作参数
        postRender->updateAllModelOperate(ShowSolidColor);
        postRend->updatePaint();
        if (mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend))
            guidata->_cloudRenderApplied = false;
        _btnClearRendering->setEnabled(false);
    }

    void mPost3DResultPanel::slot_applyRendering()
    {
        //应用前校验：视口/数据就绪，工况、帧、变量有效（帧号换算同 slot_showCloud）
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        mPostGUIData* guidata = (postRend != nullptr) ? getCVM->getGUIDataByPostRender(postRend) : nullptr;
        QString postName = (postRend != nullptr) ? postRend->getPostDataName() : QString();
        QPointer<mPostDataBase> postDB = postName.isEmpty() ? nullptr
            : getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        if (postRend == nullptr || guidata == nullptr || postDB == nullptr)
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("云图数据未就绪，无法应用渲染！"), true);
            return;
        }
        if (_comboCase->currentText().isEmpty() || _comboFrame->currentText().isEmpty() || _comboVariable->currentText().isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("请先选择云图的工况、帧与变量！"), true);
            return;
        }
        QString fName = _comboFrame->currentText();
        if (fName.contains(":")) fName = fName.split(":").at(0);
        int fi = postDB->getFrameNumByCaseAndFrameName(_comboCase->currentText(), fName);
        if (fi <= 0 || postDB->getModelData(fi) == nullptr)
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("当前帧没有结果数据，无法应用渲染！"), true);
            return;
        }

        guidata->_cloudRenderApplied = true;
        _btnClearRendering->setEnabled(true);

        //上下限/范围类待提交：整幅重绘前经 slot_setMostValue 落库（图例范围"手动"档
        //锁定值域=guidata._cloudMaxValue/MinValue 由此写入；逐帧/全程档绘制时按模式重算）
        if (_rangeEditPending)
        {
            slot_setMostValue(true);
            _rangeEditPending = false;
        }

        //离散三件套待下发：仅在用户改动/回填恢复过后重放（先重放到渲染会话再整幅
        //重绘；帧切换不重放，渲染会话自带状态）
        if (_discreteEditPending)
        {
            if (auto postRender = postRend->getPostRender())
                applyDiscreteSettings(postRender.get(), guidata);
            _discreteEditPending = false;
        }

        drawCloudMap();

        //数值过滤随应用统一下发（模式/过滤值取当前面板；边界取重绘后图例值域）
        dispatchCloudFilter();
    }

    void mPost3DResultPanel::slot_openDeformation()
    {
        //经总线打开变形控制面板（hub 接收端调度）
        MechCommon::mMechGlobalSignals::getInstance()->clickDeformationSig();
    }

    void mPost3DResultPanel::slot_applyInlineDeformation()
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;
        if (postRend->getViewportName() != _curViewName)
            return;

        auto postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return;

        if (postRender->frameProxy(postRender->currentFrame()) == nullptr) return;

        //内联输入按"等比例"语义写回 guidata 并应用
        bool ok = false;
        double val = _editInlineDeformCoef->text().trimmed().toDouble(&ok);
        if (!ok)
        {
            _editInlineDeformCoef->setText(MxFunctions::mNumberToString(1, 'f', 6));
            val = 1.0;
        }

        guidata->_deformType = QString("一致");
        guidata->_deformFactor = QVector3D(val, val, val);
        guidata->_deformUniformScale = val;	//等比例语义，与变形面板分开保存口径一致
        applyPostDeformation(postRend, guidata);
        //截面剖切跟随变形系数：切片算法快照含变形，重提交即按新几何重切
        refreshSectionCutIfActive(postRend, guidata);
    }

    void mPost3DResultPanel::slot_chooseObjectType(const QString& text)
    {
        //范围字段随对象类型联动（单元=3 项/部件=5 项）；V1.3：类型切换后范围重置"所有X"
        //（原型 dataset.renderObjectScope='all'），原越界回退逻辑保留在控制器内
        _btnObjectScope->setObjectType(_objectTypeCombo->currentIndex() == 0 ? 0 : 1);
        _btnObjectScope->setScope(1);

        //类型切换语义重置：清空拾取数据并解除拾取（旧类型数据不带入新类型防空集混计；
        //拾取过滤器按激活时类型设定，切换后旧过滤器失效，统一解除由用户重新进入拾取）
        if (_objectPickController != nullptr)
        {
            if (_objectPickController->isPickActive())
                _objectPickController->setPickActive(false);
            _objectPickController->displayData(PickWidgetData{});
            _objectPickController->setDefultTip(_btnObjectScope->scopeText());
        }

        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;

        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return;

        guidata->_renderObjectType = text;   //渲染TODO：渲染对象类型（信号携带的新类型文本）
        guidata->_renderObjectScope = _btnObjectScope->scopeText();
    }

    void mPost3DResultPanel::slot_objectPickActiveChanged(bool active)
    {
        if (_objectPickController == nullptr) return;
        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (active)
        {
            //激活前置校验（原 slot_objectPickToggled 口径）：后处理上下文无效立即失活回退
            if (postRend == nullptr || postRend->getPostRender() == nullptr)
            {
                _objectPickController->setPickActive(false);
                return;
            }
            //按对象类型设过滤器（单元=单元拾取/部件=部件拾取）；收割（ReadyToReadContainer
            //连接）、高亮与重入回显（激活时历史数据写回选择容器）由拾取栈控制器自管
            postRend->setPickFilter(_objectTypeCombo->currentIndex() == 0
                ? mxr::PickFilter::PickMeshElement : mxr::PickFilter::PickMeshPart);
        }
        else
        {
            //失活：清过滤器与拾取高亮（控制器已清共享选中容器），恢复工具栏全部拾取类型
            if (postRend != nullptr)
                postRend->setPickFilter(mxr::PickFilter::PickNothing);
            emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>());
        }
    }

    void mPost3DResultPanel::setDeformationRowVisible(bool visible)
    {
        //V1.3 尾部双列区：变形系数是双列区右半区（同组行保留、仅隐藏右半），角点数据复选保留
        if (_deformCoefRow == nullptr) return;
        _deformCoefRow->setVisible(visible);
    }

    void mPost3DResultPanel::setSharedObjectPickVisible(bool visible)
    {
        //共享拾取条可见性转发（等值面接管方调用，语义见头文件注释）
        if (_objectPickController == nullptr) return;
        _objectPickController->setPickWidgetVisible(visible);
    }

    void mPost3DResultPanel::deactivateSharedObjectPick()
    {
        //仅收口武装态（清视口过滤器/高亮由 pickActiveChanged 链承担），不动可见性
        if (_objectPickController == nullptr || !_objectPickController->isPickActive())
            return;
        _objectPickController->setPickActive(false);
    }

    void mPost3DResultPanel::slot_chooseSmoothMethod(int index)
    {
        //高级磨平变化率行仅"高级磨平"档位显示（同原型条件行）
        if (_planishFold != nullptr && _variationRow != nullptr)
            _planishFold->setRowVisible(_variationRow, _comboSmoothMethod->itemText(index) == QString("高级磨平"));

        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;

        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return;

        //保持与旧版"默认/磨平"两态一致的映射：无→默认，其余→磨平
        guidata->_smoothMethod = _comboSmoothMethod->itemText(index);
        guidata->_cloudSmooth = (index == 0);
        guidata->_cloudPlanish = (index >= 1);
        _checkAcrossParts->setEnabled(index != 0);
        //延迟应用：磨平方式待"应用渲染"生效（slot_showCloud 绘制时现读 _comboSmoothMethod）
        markCloudDirty();
    }

    void mPost3DResultPanel::slot_variationToggled(bool checked)
    {
        _editVariationLimit->setEnabled(checked);

        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;

        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return;

        guidata->_smoothVariationEnabled = checked;   //渲染TODO：高级磨平
    }

    void mPost3DResultPanel::refreshLegendBoundEditState()
    {
        //仅"手动"档可改上下限（逐帧/全程值域自动计算，复位随之锁定——复位语义=
        //恢复自动值域，仅在手动锁定时有意义）；眼睛显隐独立不受档位影响。
        //统一出口：构造/图例范围切换/模式切换/回填四处共调，曾各自为政互相覆盖
        //（切换槽设禁用后被模式槽按"所有部件"档重启用，非手动档仍可编辑——
        //2026-11-13 用户走查反馈收口）
        const bool editable = _legendRangeChoice->currentText() == QString("手动");
        _legendMax->setEditEnabled(editable);
        _legendMax->setResetEnabled(editable);
        _legendMin->setEditEnabled(editable);
        _legendMin->setResetEnabled(editable);
    }

    void mPost3DResultPanel::slot_chooseLegendRange()
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;

        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return;

        const QString range = _legendRangeChoice->currentText();
        guidata->_legendRange = range;
        //上下限编辑/复位随档位启停（仅手动可改，统一出口）
        refreshLegendBoundEditState();
        //延迟应用：图例范围切换与其它云图参数一致，待"应用渲染"时按当前范围模式/图例
        //范围统一解析值域（slot_chooseRangeType 仅持久化模式+记待应用+记脏）
        slot_chooseRangeType(_comboRangeMode->currentIndex());
    }

    void mPost3DResultPanel::slot_labelHide()
    {
        //信息标签显隐切换（同渲染图例 slot_showOrHideColorBar 模式：
        //隐藏态按钮变"显示"并禁用复位/自定义，内容与字体由注释面板管理）
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;
        auto postRender = postRend->getPostRender();
        if (postRender == nullptr) return;
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return;
        mxr::mPostFrameText* viewtext = postRender->getPostFrameText();
        if (viewtext == nullptr) return;

        guidata->_cloudInfoLabelVisible = !guidata->_cloudInfoLabelVisible;
        if (guidata->_cloudInfoLabelVisible)
        {
            _btnLabelHide->setText("隐藏");
            _btnLabelReset->setDisabled(false);
            _btnLabelCustom->setDisabled(false);
            viewtext->setTopRightTextVisual(true);
        }
        else
        {
            _btnLabelHide->setText("显示");
            _btnLabelReset->setDisabled(true);
            _btnLabelCustom->setDisabled(true);
            viewtext->setTopRightTextVisual(false);
        }
        postRend->updatePaint();
    }

    void mPost3DResultPanel::slot_labelReset()
    {
        //信息标签-复位：恢复默认显示状态（可见+默认字体15px/黑色），与图例复位按钮同口径
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;
        auto postRender = postRend->getPostRender();
        if (postRender == nullptr) return;
        mxr::mPostFrameText* viewtext = postRender->getPostFrameText();
        if (viewtext == nullptr) return;
        if (mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend))
            guidata->_cloudInfoLabelVisible = true;
        viewtext->setTopRightTextSize(15.0f);
        viewtext->setTopRightTextColor(QVector3D(0, 0, 0));
        viewtext->setTopRightTextVisual(true);
        //按钮态恢复可见分支（复位仅在可见态可点，此处为防御性归位）
        _btnLabelHide->setText("隐藏");
        _btnLabelReset->setDisabled(false);
        _btnLabelCustom->setDisabled(false);
        postRend->updatePaint();
    }

    void mPost3DResultPanel::slot_labelCustom()
    {
        //自定义=信息标签配置弹窗（共享注释面板顶层窗口化，Info dock 不跳转；
        //clickCreateNoteSig 因此无发射方转入休眠，同 clickPostNoteSig）
        mPostInfoHub::getInstance()->openNotesAsDialog(_caseId);
    }

    //R2 Q-LIFE-1 审计修复：跨视口浮层守卫公共助手——_curViewName 全部异名写点统一调用
    //（setCurViewName/reloadCloudFromGUIData/update3DResultPanel/slot_create3DResultViewport；
    //filterFiles 尾 :3489 为同视口改名接管，写前后解析同一渲染器，不在守卫面）。
    //v1.4 语义=异名且动画控件条存活→停播放+关设置弹窗（Q-FIX-1 停止点补发语义经
    //pausePlayback 链保留；工具栏常显不隐藏，D-08）。
    //同名调用（面板刷新/导入同视口级联）零动作；新视口禁用域刷新由后续
    //updateResultInfoGUI→updateRealtimeAnimEnabled 接续完成（04 J-11）
    void mPost3DResultPanel::switchViewGuard(const QString& newViewName)
    {
        if (newViewName == _curViewName)
            return;
        //v1.4 常显口径（原 hideRealtimeAnimDock 退役）：异名切视口=停播放（Q-FIX-1
        //停止点补刷经 pausePlayback→slot_realtimePlayToggled !playing 分支照常走）
        //+关设置弹窗（退出瞬时态，§9 契约）。禁用域刷新交由既有收口链
        //updateResultInfoGUI→updateRealtimeAnimEnabled 接续完成（04 J-11：此处取渲染
        //会话按旧 _curViewName 求值会过期，不在此刷新）；工具栏常显不隐藏
        if (_realtimeAnimWidget != nullptr)
        {
            _realtimeAnimWidget->pausePlayback();
            _realtimeAnimWidget->closeSettingsPopup();
        }
        //【审计 B-02】镜像三通道重绑到新视口门面（newViewName 直传——本守卫调用时点
        //四写点尚未更新 _curViewName；新视口门面未就绪=保持断开，挂载出口重入补接）
        rebindRealtimeEngineMirrorConns(newViewName);
    }

    void mPost3DResultPanel::setCurViewName(const QString& viewName)
    {
        //重绑同一视口=仅面板切换（如自等值面/切面图面板点"云图"按钮切回，视口与
        //数据均未变）：跳过末尾整幅云图重绘。slot_showCloud→setRendCurrentFrameData
        //会整体重建单帧渲染器，等值面（contour 渲染器挂在单帧渲染器上）随之丢失
        //（2026-11-05用户定案：仅切面板保持等值面，清除唯一入口是"清除等值面"按钮）
        const bool sameViewport = (viewName == _curViewName);
        //实时动画 v1（假设 7）：视口切换时旧视口控件条停播+关弹窗——R2 Q-LIFE-1/Q-FIX-3
        //守卫迁入 switchViewGuard 公共助手（四写点同款）。v1.4 常显：新视口挂载/禁用域
        //刷新由下方 updateResultInfoGUI→updateRealtimeAnimEnabled 接续完成
        switchViewGuard(viewName);
        _curViewName = viewName;

        //云图面板重新编辑时确保共享折叠组已归还（等值面面板 hideEvent 兜底之外的主动回收）
        reattachSharedSections();

        // 切回云图视图时，清理该视口渲染器上可能残留的切面裁剪状态，
        // 避免从切面图切换回来后模型仍被裁剪。
        // 仅在数据已就绪时执行，防止导入/初始化阶段触发空数据绘制导致崩溃。
        REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderByViewName(_curViewName);
        if (rebuildPostRend == nullptr)
            return;

        QString postName = rebuildPostRend->getPostDataName();
        if (postName.isEmpty())
            return;

        QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        if (modeldb == nullptr)
            return;

        // 获取GUI数据，确认云图已经初始化过
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(rebuildPostRend);
        if (guidata == nullptr)
            return;

        // 清除渲染按钮状态随目标视口的已应用状态同步，避免残留上一视口的可用态
        _btnClearRendering->setEnabled(guidata->_cloudRenderApplied);

        // 只有已有有效帧数据时才清理切面并重绘云图
        if (modeldb->getModelData(guidata->_cloudFrame) == nullptr)
            return;

        auto postRender = rebuildPostRend->getPostRender();
        if (postRender != nullptr)
        {
            // 循环删除所有现有切面，直到没有可删除的为止
            for (int i = 0; i < 8; ++i)
            {
                // S5 G5：切面族迁台账（旧 deleteCuttingPlane(0)→removeObject 寻型 id）
                PostCommon::removeAllSceneObjects(postRender.get(), mxr::SceneObjectType::CuttingPlane);
            }
        }

        // 切换视口时先同步面板数据，防止 _comboCase/_comboFrame 仍是旧模型值
        // 导致 slot_showCloud 中计算 frame = -1，进而构造 FrameDataProxy 时空指针崩溃。
        updateResultInfoGUI();

        if (!sameViewport)
        {
            // 触发云图重绘，恢复云图显示
            drawCloudMap();
        }
        else
        {
            //同视口仅刷一帧：上方切面删除等 actor 增删需要一次重绘生效，
            //但不能走整幅重绘（会重建单帧渲染器丢等值面）
            //截面剖切对象同被上方家族清理删除，同视口分支在此按 guidata 记录重建
            refreshSectionCutIfActive(rebuildPostRend, guidata);
            rebuildPostRend->updatePaint();
        }
    }

    void mPost3DResultPanel::reloadCloudFromGUIData(const QString& viewName)
    {
        //防护同 setCurViewName：视口/数据库/guidata 就绪且帧数据有效才重绘
        //P3-1 审计修复（R1）：跨视口驱动守卫与 setCurViewName 同款——外部面板（如
        //变形面板）以其它视口名驱动本函数时关闭浮层，防 tick 按新 _curViewName 取
        //渲染器跨视口误驱、旧视口画面冻结。R2 Q-LIFE-1：守卫迁入 switchViewGuard
        //公共助手；v1.2 起新视口浮出/收起由下方 updateResultInfoGUI 接续
        switchViewGuard(viewName);
        _curViewName = viewName;

        REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderByViewName(_curViewName);
        if (rebuildPostRend == nullptr)
            return;

        QString postName = rebuildPostRend->getPostDataName();
        if (postName.isEmpty())
            return;

        QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        if (modeldb == nullptr)
            return;

        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(rebuildPostRend);
        if (guidata == nullptr)
            return;

        if (modeldb->getModelData(guidata->_cloudFrame) == nullptr)
            return;

        updateResultInfoGUI();
        //同 slot_chooseVariable 重绘前置位：drawCloudMap 仅在 _isDrawCloudMap 时落 slot_showCloud
        _isDrawCloudMap = true;
        drawCloudMap();
    }

    QList<FoldSection*> mPost3DResultPanel::sharedSections() const
    {
        return { _changeFold, _objectFold, _planishFold };
    }

    void mPost3DResultPanel::detachSharedSections()
    {
        if (_changeFold) _mainLayout->removeWidget(_changeFold);
        if (_objectFold) _mainLayout->removeWidget(_objectFold);
        if (_planishFold) _mainLayout->removeWidget(_planishFold);
    }

    void mPost3DResultPanel::reattachSharedSections()
    {
        if (_changeFold == nullptr || _objectFold == nullptr || _planishFold == nullptr)
            return;

        //已在位（未被接管）则跳过
        if (_changeFold->parentWidget() == this && _changeFold->isVisibleTo(this))
            return;

        //插回操作条之后原位（0=操作条，1/2/3=三个共享折叠组，其后为图例和标签/光源）
        _mainLayout->insertWidget(1, _changeFold);
        _mainLayout->insertWidget(2, _objectFold);
        _mainLayout->insertWidget(3, _planishFold);
        _changeFold->refreshLayout();
        _objectFold->refreshLayout();
        _planishFold->refreshLayout();
    }

    void mPost3DResultPanel::syncEditContextToView(const QString& viewName)
    {
        //接管期间的面板外上下文同步（调用方：等值面面板 setCurViewName 接管共享组前）：
        //见头文件注释。无 guidata 的视口（无云图数据的派生视口）仍切换上下文——后续
        //共享组写入会因 guidata 判空自然失效，好过静默改写上一个编辑视口的设置
        if (viewName.isEmpty() || viewName == _curViewName)
            return;

        //跨视口浮层销毁语义与 setCurViewName 一致（异名切上下文先收旧视口浮层）
        switchViewGuard(viewName);
        _curViewName = viewName;

        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        mPostGUIData* guidata = (postRend != nullptr) ? getCVM->getGUIDataByPostRender(postRend) : nullptr;
        //清除渲染按钮态随目标视口已应用状态同步（同 setCurViewName 口径）
        if (guidata != nullptr)
            _btnClearRendering->setEnabled(guidata->_cloudRenderApplied);

        //纯回填：共享折叠组连同本面板其余控件按目标视口 guidata 重建显示态，
        //不触发绘制链（切面保留/渲染器不动，等值面不受影响）
        updateResultInfoGUI();
    }

    void mPost3DResultPanel::startFilter(QString min, QString max, QString type)
    {
        REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderWidget();
        if (rebuildPostRend == nullptr) return;

        if (rebuildPostRend->getViewportName() != _curViewName)
            return;

        auto  postRender = rebuildPostRend->getPostRender();
        if (postRender == nullptr) return;
        QString postName = rebuildPostRend->getPostDataName();

        //获取GUI数据
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(rebuildPostRend);
        if (guidata == nullptr) return;
        guidata->_filterOpen = true;
        guidata->_filterRule = type;

        QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        if (modeldb == nullptr)
            return;

        // S5 GUI 迁移（引擎缺通道移交项，编译强制）：getOneFrameRender 下钻通道随门面
        // 直接替换消亡，REBUILD_PostModelRender 实例级过滤等值渲染（createSingle/
        // DoubleContour）暂无 G 通道等价物——视觉挂起待渲染批次补通道后恢复（v12
        // V-4 下钻盘点遗留；05 实施记录“新发现调用点”章登记）。数据侧过滤类型经
        // frameProxy 查询代理保留（重开过滤时即时生效）

        mxr::IPostModelRenderDataProxy* model_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(postRender->frameProxy(postRender->currentFrame())->getModelRenderDataProxy());

        // 查询查询数据代理
        mxr::IPostRendQueryDataProxy* query_proxy1 = dynamic_cast<mxr::IPostRendQueryDataProxy*>(postRender->frameProxy(postRender->currentFrame())->getRendQueryDataProxy());

        //htx 添加补丁，解决-云图中过滤-修改参数有可能会导致崩溃的问题 后续可以删除
        if (min.toFloat() == 0)
        {
            min = "0.0000000001";
        }

        float curVal;
        if (type == "之间")
        {
            float vMin = min.toFloat();
            float vMax = max.toFloat();
            query_proxy1->setFilterType(mxr::FilterType::BETWEEN);
            // S5 移交项（引擎缺通道）：modelRender->createDoubleContour(vMin, vMax);  // 门面无实例级过滤等值通道，待渲染批次恢复
            guidata->_filterMin = min.toDouble();
            guidata->_filterMax = max.toDouble();

        }
        else if (type == "大于")
        {
            curVal = min.toFloat();
            query_proxy1->setFilterType(mxr::FilterType::ABOVE);
            // S5 移交项（引擎缺通道）：modelRender->createSingleContour(curVal);  // 门面无实例级过滤等值通道，待渲染批次恢复
            guidata->_filterMin = min.toDouble();
            guidata->_filterMax = max.toDouble();
        }
        else if (type == "小于")
        {
            curVal = max.toFloat();
            query_proxy1->setFilterType(mxr::FilterType::BELOW);
            // S5 移交项（引擎缺通道）：modelRender->createSingleContour(curVal);  // 门面无实例级过滤等值通道，待渲染批次恢复
            guidata->_filterMin = min.toDouble();
            guidata->_filterMax = max.toDouble();
        }
        else if (type == "等于")
        {
            curVal = min.toFloat();
            query_proxy1->setFilterType(mxr::FilterType::EQUAL);
            // S5 移交项（引擎缺通道）：modelRender->createSingleContour(curVal);  // 门面无实例级过滤等值通道，待渲染批次恢复
            guidata->_filterMin = min.toDouble();
            guidata->_filterMax = max.toDouble();
        }
        //qDebug() << "========================================"<<guidata->_filterMin << guidata->_filterMax;
        rebuildPostRend->updatePaint();
    }

    void mPost3DResultPanel::clearFilter()
    {
        REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderWidget();
        if (rebuildPostRend == nullptr) return;
        if (rebuildPostRend->getViewportName() != _curViewName)
            return;

        auto  postRender = rebuildPostRend->getPostRender();
        if (postRender == nullptr) return;

        //获取GUI数据
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(rebuildPostRend);
        if (guidata == nullptr) return;

        auto dataProxy = postRender->frameProxy(postRender->currentFrame());
        if (dataProxy == nullptr)
            return;

        QString postname = rebuildPostRend->getPostDataName();
        QPointer<mPostDataBase> postdb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
        if (postdb == nullptr)
            return;

        // 查询查询数据代理
        mxr::IPostRendQueryDataProxy* query_proxy1 = dynamic_cast<mxr::IPostRendQueryDataProxy*>(dataProxy->getRendQueryDataProxy());
        query_proxy1->setFilterType(mxr::FilterType::NONE);

        // S5 GUI 迁移（引擎缺通道移交项，编译强制）：getOneFrameRender 下钻通道随门面
        // 直接替换消亡——实例级过滤等值清理（deleteSingle/DoubleContour）暂无 G 通道
        // 等价物，视觉挂起待渲染批次补通道后恢复；数据侧过滤类型复位保留

        rebuildPostRend->updatePaint();
        guidata->_filterOpen = false;
    }

    void mPost3DResultPanel::exportFilterSTL(const QString& fileName)
    {
        const int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;

        //防重入：上一次导出还在进行
        if (_stlExportThread != nullptr)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, lang == 0
                ? QString("STL文件正在导出中，请稍候...")
                : QString("STL export is in progress, please wait..."), true);
            return;
        }

        //上下文校验（同 startFilter）
        REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderWidget();
        if (rebuildPostRend == nullptr) return;
        if (rebuildPostRend->getViewportName() != _curViewName) return;

        auto postRender = rebuildPostRend->getPostRender();
        if (postRender == nullptr) return;

        QString postName = rebuildPostRend->getPostDataName();
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(rebuildPostRend);
        if (guidata == nullptr) return;

        //过滤必须已开启，且云图已绘制
        if (!guidata->_filterOpen)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, lang == 0
                ? QString("请先开启过滤，再导出STL！")
                : QString("Please enable the filter before exporting STL!"), true);
            return;
        }
        if (guidata->_cloudVarName.isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, lang == 0
                ? QString("请先绘制云图，再导出STL！")
                : QString("Please draw the contour before exporting STL!"), true);
            return;
        }

        QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        if (modeldb == nullptr) return;
        QPointer<mPostModelData> modelData = modeldb->getModelData(guidata->_cloudFrame);
        if (modelData == nullptr)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, lang == 0
                ? QString("当前帧模型数据为空，无法导出STL！")
                : QString("No model data for the current frame, cannot export STL!"), true);
            return;
        }

        //标量场：按界面当前 变量/分量/层/复数/帧 强制重读一次，保证与云图显示一致。
        //注意 mIMxdbFile1 的两个 readCloudData 重载各有一个坑：
        //  1) 8参重载（带坐标系参数）：循环正确（按 _varCount/shape），但节点变量的
        //     value1 以“连续节点id”为 key（:1570）——下方映射时需换算回真实id；
        //  2) 7参重载：value1 以“真实节点id”为 key，但循环上限误用 _nCompo（:1035），
        //     多分量变量（如 Displacement）直接越界崩溃，不可调用。
        // 因此这里固定调用 8 参重载，并在场映射处做 连续id→真实id 换算。
        bool fieldReady = false;
        {
            QApplication::setOverrideCursor(Qt::WaitCursor);
            mIMxdbFile1* mxdbReader = new mIMxdbFile1;
            mxdbReader->setInitFileName(_postInitFiles.value(postName));
            const bool readOk = mxdbReader->readCloudData(postName, guidata->_cloudFrame,
                guidata->_cloudVarName, guidata->_cloudCompo, guidata->_cloudLayerName,
                guidata->_cloudComplex, QString("Global"), QVector<double>{}, true);
            delete mxdbReader;
            QApplication::restoreOverrideCursor();
            const Compones& fresh = modeldb->getComponeValues();
            fieldReady = readOk && (!fresh.value1.isEmpty() || !fresh.value3.isEmpty()
                || !fresh.dRValue.isEmpty() || !fresh.fRValue.isEmpty());
        }
        // 强制重读成功 ⇒ value1 的 key 是连续节点id；重读失败退回缓存时按真实id理解
        const bool fieldKeyIsCid = fieldReady;
        if (!fieldReady)
        {
            //兜底：重读失败（如按帧懒加载的 OP2）且缓存非空时仍可按缓存导出
            const Compones& cached = modeldb->getComponeValues();
            const bool cacheUsable = !cached.value1.isEmpty() || !cached.value3.isEmpty()
                || !cached.dRValue.isEmpty() || !cached.fRValue.isEmpty();
            if (!cacheUsable)
            {
                mGlobalSignals::getInstance()->outputMessageSig(1, lang == 0
                    ? QString("云图数据读取失败，无法导出STL！")
                    : QString("Failed to read contour data, cannot export STL!"), true);
                return;
            }
            mGlobalSignals::getInstance()->outputMessageSig(1, lang == 0
                ? QString("云图数据重读失败，已按当前缓存数据导出！")
                : QString("Contour data re-read failed, exporting with cached data!"), true);
        }
        const Compones& comp = modeldb->getComponeValues();

        //规则与范围映射：之间→区间；大于/小于→单值保侧；等于→窄容差区间
        STLFilterRule rule = STLFilterRule::Between;
        double vmin = guidata->_filterMin;
        double vmax = guidata->_filterMax;
        if (guidata->_filterRule == QString("大于"))
        {
            rule = STLFilterRule::Above;
        }
        else if (guidata->_filterRule == QString("小于"))
        {
            rule = STLFilterRule::Below;
        }
        else if (guidata->_filterRule == QString("等于"))
        {
            rule = STLFilterRule::Equal;
            const double delta = std::max(std::abs(vmin), 1.0) * 1e-6;
            vmin = vmin - delta;
            vmax = vmin + 2 * delta; // 展开为 [v-δ, v+δ]
        }
        else
        {
            if (vmin > vmax)
            {
                mGlobalSignals::getInstance()->outputMessageSig(1, lang == 0
                    ? QString("过滤范围无效：最小值大于最大值！")
                    : QString("Invalid filter range: min is greater than max!"), true);
                return;
            }
        }

        //--------- GUI 线程上完成纯数据快照，后台线程只读 ---------
        PostSTLExportInput input;
        input.rule = rule;
        input.vmin = vmin;
        input.vmax = vmax;
        input.outputPath = fileName;

        //节点表：真实节点id -> 连续下标 + 坐标
        QHash<int, quint32> nodeIndex;
        const QVector<int> partIds = modelData->getAllPartsIDs();
        quint32 nodeCount = 0;
        for (int partId : partIds)
        {
            const QList<int> realIds = modelData->getNodeRealIds(partId);
            for (int rId : realIds)
            {
                if (!nodeIndex.contains(rId))
                    nodeIndex.insert(rId, nodeCount++);
            }
        }
        input.x.resize(nodeCount);
        input.y.resize(nodeCount);
        input.z.resize(nodeCount);

        //坐标与界面显示构型保持一致：显示变形时叠加 变形系数×当前帧位移
        //（DisplacementValuesR 以连续节点id为key，与渲染侧同一个数据源）
        //注意：渲染侧变形系数默认(1,1,1)（默认显示当前构型），guidata 未交互过时为空，
        //此时按渲染默认系数处理；"仅显示初始构型"时渲染系数被置零，导出不叠加变形。
        QVector3D deformFactor(0, 0, 0);
        if (guidata->_shapeType != QString("仅显示初始构型"))
        {
            deformFactor = guidata->_shapeType.isEmpty() ? QVector3D(1, 1, 1)
                : guidata->_deformFactor;
        }
        const bool applyDeform = (deformFactor != QVector3D(0, 0, 0));
        QHash<int, QVector3D> displacementMap;
        if (applyDeform)
        {
            displacementMap = modelData->getAllDisplacementDataR();
        }
        for (auto it = nodeIndex.constBegin(); it != nodeIndex.constEnd(); ++it)
        {
            const QVector3D base = modelData->getVectorById(it.key());
            QVector3D pos = base;
            if (applyDeform)
            {
                const int cId = modelData->_nodedata.nodeidmap.value(it.key(), -1);
                const QVector3D disp = cId >= 0 ? displacementMap.value(cId) : QVector3D();
                pos = base + QVector3D(deformFactor.x() * disp.x(), deformFactor.y() * disp.y(), deformFactor.z() * disp.z());
            }
            const quint32 idx = it.value();
            input.x[idx] = pos.x();
            input.y[idx] = pos.y();
            input.z[idx] = pos.z();
        }

        //标量场（与云图显示同源）：合并实体/壳与单/双精度容器
        QHash<int, double> entityVals;
        for (auto it = comp.value1.constBegin(); it != comp.value1.constEnd(); ++it)
            entityVals.insert(it.key(), it.value());
        for (auto it = comp.value2.constBegin(); it != comp.value2.constEnd(); ++it)
            entityVals.insert(it.key(), it.value());
        for (auto it = comp.value3.constBegin(); it != comp.value3.constEnd(); ++it)
            entityVals.insert(it.key(), it.value());
        for (auto it = comp.value4.constBegin(); it != comp.value4.constEnd(); ++it)
            entityVals.insert(it.key(), it.value());

        input.field.assign(nodeCount, 0.0);
        std::vector<double> nodeSum;
        std::vector<std::size_t> nodeCnt;

        if (comp.flag == 2)
        {
            //单元节点格式：每个单元给出其节点上的值，对节点取平均
            nodeSum.assign(nodeCount, 0.0);
            nodeCnt.assign(nodeCount, 0);
            auto accumulateEleNode = [&nodeSum, &nodeCnt, &nodeIndex](const auto& src)
                {
                    for (auto eit = src.constBegin(); eit != src.constEnd(); ++eit)
                    {
                        for (auto nit = eit.value().constBegin(); nit != eit.value().constEnd(); ++nit)
                        {
                            const auto idxIt = nodeIndex.constFind(nit.key());
                            if (idxIt == nodeIndex.constEnd())
                                continue;
                            nodeSum[idxIt.value()] += nit.value();
                            ++nodeCnt[idxIt.value()];
                        }
                    }
                };
            accumulateEleNode(comp.dRValue);
            accumulateEleNode(comp.dQValue);
            accumulateEleNode(comp.fRValue);
            accumulateEleNode(comp.fQValue);
            for (quint32 i = 0; i < nodeCount; ++i)
            {
                if (nodeCnt[i] > 0)
                    input.field[i] = nodeSum[i] / nodeCnt[i];
            }
        }
        else if (comp.flag == 1)
        {
            //单元格式：单元值平均到节点（与参考实现一致，随单元遍历累计）
            nodeSum.assign(nodeCount, 0.0);
            nodeCnt.assign(nodeCount, 0);
        }
        else
        {
            //节点格式：直接映射。
            //8参 readCloudData 的 value1 以连续节点id为 key，这里换算回真实id再查节点表
            auto realIdOfKey = [&](int key) -> int
                {
                    if (fieldKeyIsCid && key >= 0 && key < modelData->_nodedata.nodeamount)
                        return modelData->_nodedata.ids[key];
                    return key;
                };
            for (auto it = entityVals.constBegin(); it != entityVals.constEnd(); ++it)
            {
                const auto idxIt = nodeIndex.constFind(realIdOfKey(it.key()));
                if (idxIt != nodeIndex.constEnd())
                    input.field[idxIt.value()] = it.value();
            }
        }

        //单元遍历：使用全量实体单元（不随界面显隐状态打洞），按 ucp 支持的
        //四种类型分块，其余类型跳过并计数
        std::array<PostSTLCellBlock, 4> blocks;
        blocks[PostSTLCellBlock::Tetra].kind = PostSTLCellBlock::Tetra;
        blocks[PostSTLCellBlock::Hexa].kind = PostSTLCellBlock::Hexa;
        blocks[PostSTLCellBlock::Wedge].kind = PostSTLCellBlock::Wedge;
        blocks[PostSTLCellBlock::Pyramid].kind = PostSTLCellBlock::Pyramid;
        std::size_t skippedCount = 0;

        for (int partId : partIds)
        {
            QPointer<mPostPartData> partData = modelData->getPartData(partId);
            if (partData == nullptr)
                continue;
            const postelementdata& ped = partData->_parteledata;
            for (std::size_t cid = 0; cid < ped.cell_rid.size(); ++cid)
            {
                const int eleRid = ped.cell_rid[cid];
                int dim = 0;
                const std::vector<int> eleNodeRealIds = modelData->getNodeIdsByEleIdWithoutBuild(partId, eleRid, dim);
                if (dim != 3)
                {
                    ++skippedCount;
                    continue;
                }

                PostSTLCellBlock::Kind kind;
                switch (ped.cell_types[cid])
                {
                case PostCelllType::MX_TETRA: kind = PostSTLCellBlock::Tetra; break;
                case PostCelllType::MX_HEXAHEDRON: kind = PostSTLCellBlock::Hexa; break;
                case PostCelllType::MX_WEDGE: kind = PostSTLCellBlock::Wedge; break;
                case PostCelllType::MX_PYRAMID: kind = PostSTLCellBlock::Pyramid; break;
                default:
                    ++skippedCount;
                    continue;
                }
                if (eleNodeRealIds.size() != static_cast<int>(PostSTLCellBlock::nodeCountOf(kind)))
                {
                    ++skippedCount;
                    continue;
                }

                PostSTLCellBlock& block = blocks[kind];
                const std::size_t connSizeBefore = block.connectivity.size();
                bool valid = true;
                for (int rId : eleNodeRealIds)
                {
                    const auto idxIt = nodeIndex.constFind(rId);
                    if (idxIt == nodeIndex.constEnd())
                    {
                        valid = false;
                        break;
                    }
                    block.connectivity.push_back(idxIt.value());
                }
                if (!valid)
                {
                    //回滚本单元已写入的连接关系
                    block.connectivity.resize(connSizeBefore);
                    ++skippedCount;
                    continue;
                }

                //单元变量：把单元值累计到该单元各节点
                if (comp.flag == 1)
                {
                    const auto valIt = entityVals.constFind(eleRid);
                    if (valIt != entityVals.constEnd())
                    {
                        const double ev = valIt.value();
                        for (int rId : eleNodeRealIds)
                        {
                            const quint32 idx = nodeIndex.value(rId);
                            nodeSum[idx] += ev;
                            ++nodeCnt[idx];
                        }
                    }
                }
            }
        }

        if (comp.flag == 1)
        {
            for (quint32 i = 0; i < nodeCount; ++i)
            {
                if (nodeCnt[i] > 0)
                    input.field[i] = nodeSum[i] / nodeCnt[i];
            }
        }

        bool hasSolid = false;
        for (const auto& block : blocks)
        {
            if (block.cellCount() > 0)
            {
                hasSolid = true;
                break;
            }
        }
        if (!hasSolid)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, lang == 0
                ? QString("当前模型没有可导出的三维实体单元（壳/杆/梁单元暂不支持STL导出）！")
                : QString("No solid cells available for STL export (shell/beam cells are not supported yet)!"), true);
            return;
        }
        for (const auto& block : blocks)
        {
            if (block.cellCount() > 0)
                input.blocks.push_back(block);
        }
        if (skippedCount > 0)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, lang == 0
                ? QString("已跳过 %1 个不支持的单元（仅支持四面体/六面体/楔形体/金字塔）").arg(skippedCount)
                : QString("Skipped %1 unsupported cells (only tetra/hexa/wedge/pyramid are supported)").arg(skippedCount), true);
        }

        //--------- 启动后台线程计算 + 写盘 ---------
        _stlExportThread = new mPostSTLExportThread(std::move(input));
        connect(_stlExportThread, &mPostSTLExportThread::finished, this, [this, fileName, lang]()
            {
                if (_stlExportThread == nullptr)
                    return;
                const PostSTLExportResult res = _stlExportThread->result();
                mPostSTLExportThread* thread = _stlExportThread;
                _stlExportThread = nullptr;
                thread->disconnect();
                thread->deleteLater();

                mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);

                if (res.ok)
                {
                    const QString droppedNote = res.droppedTriangleCount > 0
                        ? (lang == 0
                            ? QString("，已剔除内部悬空面片 %1 个").arg((qulonglong)res.droppedTriangleCount)
                            : QString(", dropped %1 interior dangling facets").arg((qulonglong)res.droppedTriangleCount))
                        : QString();
                    mGlobalSignals::getInstance()->outputMessageSig(0, lang == 0
                        ? QString("文件 %1 导出成功！（点数 %2，三角形数 %3%4）").arg(fileName).arg((qulonglong)res.pointCount).arg((qulonglong)res.triangleCount).arg(droppedNote)
                        : QString("File %1 exported successfully! (points: %2, triangles: %3%4)").arg(fileName).arg((qulonglong)res.pointCount).arg((qulonglong)res.triangleCount).arg(droppedNote), true);
                }
                else if (res.errorDetail == QString("no triangles generated"))
                {
                    mGlobalSignals::getInstance()->outputMessageSig(1, lang == 0
                        ? QString("过滤范围内未生成任何面片，请检查过滤范围！")
                        : QString("No facets generated within the filter range. Please check the filter values!"), true);
                }
                else if (res.errorDetail == QString("failed to write stl file"))
                {
                    mGlobalSignals::getInstance()->outputMessageSig(1, lang == 0
                        ? QString("STL文件写入失败：%1").arg(fileName)
                        : QString("Failed to write STL file: %1").arg(fileName), true);
                }
                else
                {
                    mGlobalSignals::getInstance()->outputMessageSig(1, lang == 0
                        ? QString("STL导出失败：%1").arg(res.errorDetail)
                        : QString("STL export failed: %1").arg(res.errorDetail), true);
                }
            });

        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(true);
        mGlobalSignals::getInstance()->outputMessageSig(0, lang == 0
            ? QString("正在导出STL文件...（节点 %1，四面体 %2，六面体 %3，楔形体 %4，金字塔 %5，已跳过 %6 个单元%7）")
            .arg((qulonglong)nodeCount)
            .arg((qulonglong)blocks[PostSTLCellBlock::Tetra].cellCount())
            .arg((qulonglong)blocks[PostSTLCellBlock::Hexa].cellCount())
            .arg((qulonglong)blocks[PostSTLCellBlock::Wedge].cellCount())
            .arg((qulonglong)blocks[PostSTLCellBlock::Pyramid].cellCount())
            .arg((qulonglong)skippedCount)
            .arg(applyDeform ? QString("，含变形") : QString())
            : QString("Exporting STL file... (nodes: %1, tetra: %2, hexa: %3, wedge: %4, pyramid: %5, skipped cells: %6%7)")
            .arg((qulonglong)nodeCount)
            .arg((qulonglong)blocks[PostSTLCellBlock::Tetra].cellCount())
            .arg((qulonglong)blocks[PostSTLCellBlock::Hexa].cellCount())
            .arg((qulonglong)blocks[PostSTLCellBlock::Wedge].cellCount())
            .arg((qulonglong)blocks[PostSTLCellBlock::Pyramid].cellCount())
            .arg((qulonglong)skippedCount)
            .arg(applyDeform ? QString(", with deformation") : QString()), true);
        _stlExportThread->start();
    }


    void mPost3DResultPanel::showValue()
    {
        //获取拾取模式
        int num = 0;
        PickDataType pickDataType = getCVM->getPostPickDataType(num);
        if (pickDataType != PickDataType::Node && pickDataType != PickDataType::Element)
        {
            return;
        }

        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;

        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        //获取GUI数据
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return;

        //获取后处理数据
        QString postname = postRend->getPostDataName();
        QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
        if (postDB == nullptr)
            return;

        mPostModelData* modeldata = postDB->getModelData(guidata->_cloudFrame);
        if (modeldata == nullptr) return;

        int flag = 0;
        QHash<int, double> values = postDB->getResultDataByFrameID(guidata->_cloudFrame, flag);
        QHash<int, QHash<int, double>> eleNodeVals = postDB->getEleNodeResultDataByFrameID(guidata->_cloudFrame);
        QVector3D deformationScale = postRender->frameProxy(postRender->currentFrame())->getRendQueryDataProxy()->getDeformationScale();
        const QHash<int, QVector3D> dis = postDB->getDisplacementDataByFrameID(guidata->_cloudFrame);
        QVector<QPair<QVector3D, QString>> pos_txt;
        QVector<QVector3D> pos;

        mxr::IPostRendQueryDataProxy* query_proxy1 = dynamic_cast<mxr::IPostRendQueryDataProxy*>(postRender->frameProxy(postRender->currentFrame())->getRendQueryDataProxy());
        if (query_proxy1 == nullptr)
            return;
        mxr::PostNodeOrElement nodeOrElement = query_proxy1->getNodeOrElement();

        std::vector<MeshSelectInfoData> res;
        if (nodeOrElement == mxr::PostNodeOrElement::PostNode)
        {
            if (pickDataType == PickDataType::Element)
            {
                mGlobalSignals::getInstance()->outputMessageSig("当前变量为节点变量，请拾取节点查看值！");
                return;
            }
            res = getCVM->getPostRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
            if (res.size() == 0) return;

            for (auto r : res)
            {
                _ids.insert(r.data_id.element_id);
            }

            for (auto iter = _ids.begin(); iter != _ids.end(); ++iter)
            {
                int cId = modeldata->_nodedata.nodeidmap.value(*iter);
                double value = values.value(cId);
                QVector3D vertex = modeldata->getCoordinates(cId) + deformationScale * dis.value(cId);
                QString stringValue = QString("%1").arg(value);
                pos_txt.push_back({ {vertex.x(), vertex.y(), vertex.z()}, stringValue });
            }
        }
        else if (nodeOrElement == mxr::PostNodeOrElement::PostElement)
        {
            if (pickDataType == PickDataType::Node)
            {
                mGlobalSignals::getInstance()->outputMessageSig("当前变量为单元变量，请拾取单元查看值！");
                return;
            }
            res = getCVM->getPostRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
            if (res.size() == 0) return;

            for (auto r : res)
            {
                _ids.insert(r.data_id.element_id);
            }

            for (auto iter = _ids.begin(); iter != _ids.end(); ++iter)
            {
                int element = *iter;
                int partid = modeldata->getPartIDByEleID(element);
                QList<int> nodeid = modeldata->getElementNodeId(partid, element);

                QVector3D vertex = { 0.0, 0.0, 0.0 };
                for (int id : nodeid)
                {
                    vertex += modeldata->getCoordinates(id) + deformationScale * dis.value(id);
                }
                vertex = vertex / double(nodeid.size());

                float value = values.value(element);
                QString stringValue = QString("%1").arg(value);
                pos_txt.push_back({ {vertex.x(), vertex.y(), vertex.z()}, stringValue });
            }
        }
        else if (nodeOrElement == mxr::PostNodeOrElement::PostElementNode)
        {
            res = getCVM->getPostRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
            if (res.size() == 0) return;

            for (auto r : res)
            {
                _ids.insert(r.data_id.element_id);
            }

            for (auto iter = _ids.begin(); iter != _ids.end(); ++iter)
            {
                int element = *iter;
                int partid = modeldata->getPartIDByEleID(element);
                QList<int> nodeid = modeldata->getElementNodeId(partid, element);

                QVector3D vertex = { 0.0, 0.0, 0.0 };
                for (int id : nodeid)
                {
                    vertex += modeldata->getCoordinates(id) + deformationScale * dis.value(id);
                }
                vertex = vertex / double(nodeid.size());

                double sum = 0;
                QHash<int, double> nodeVals = eleNodeVals[element];
                for (int n = 0; n < nodeid.size(); ++n)
                {
                    sum += nodeVals[nodeid[n]];
                }
                double value = sum / nodeid.size();
                QString stringValue = QString("%1").arg(value);
                pos_txt.push_back({ {vertex.x(), vertex.y(), vertex.z()}, stringValue });
            }
        }

        QVector<QVector3D> pos1;
        QVector<QString> txt;
        for (auto pos_txt0 : pos_txt)
        {
            pos1.push_back(pos_txt0.first);
            txt.push_back(pos_txt0.second);
        }
        auto fontRender = postRend->getFontRender();
        if (fontRender != nullptr)
        {
            getCVM->getPostRenderWidget()->getFontRender()->appendCommonFont("postValue", pos1, txt);
        }

        getCVM->clearPick();
    }

    void mPost3DResultPanel::hideValue()
    {
        //获取拾取模式
        int num = 0;
        PickDataType pickDataType = getCVM->getPostPickDataType(num);
        if (pickDataType != PickDataType::Node && pickDataType != PickDataType::Element)
        {
            return;
        }

        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;

        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        //获取GUI数据
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return;

        //获取后处理数据
        QString postname = postRend->getPostDataName();
        QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
        if (postDB == nullptr)
            return;

        mPostModelData* modeldata = postDB->getModelData(guidata->_cloudFrame);
        if (modeldata == nullptr) return;

        int flag = 0;
        QHash<int, double> values = postDB->getResultDataByFrameID(guidata->_cloudFrame, flag);
        QHash<int, QHash<int, double>> eleNodeVals = postDB->getEleNodeResultDataByFrameID(guidata->_cloudFrame);
        QVector3D deformationScale = postRender->frameProxy(postRender->currentFrame())->getRendQueryDataProxy()->getDeformationScale();
        const QHash<int, QVector3D> dis = postDB->getDisplacementDataByFrameID(guidata->_cloudFrame);
        QVector<QPair<QVector3D, QString>> pos_txt;
        QVector<QVector3D> pos;

        mxr::IPostRendQueryDataProxy* query_proxy1 = dynamic_cast<mxr::IPostRendQueryDataProxy*>(postRender->frameProxy(postRender->currentFrame())->getRendQueryDataProxy());
        if (query_proxy1 == nullptr)
            return;
        mxr::PostNodeOrElement nodeOrElement = query_proxy1->getNodeOrElement();

        std::vector<MeshSelectInfoData> res;
        set<int> reduceIDs;
        if (nodeOrElement == mxr::PostNodeOrElement::PostNode)
        {
            res = getCVM->getPostRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
            if (res.size() == 0) return;

            for (auto r : res)
            {
                reduceIDs.insert(r.data_id.element_id);
            }
        }
        else
        {
            res = getCVM->getPostRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
            if (res.size() == 0) return;

            for (auto r : res)
            {
                reduceIDs.insert(r.data_id.element_id);
            }
        }

        for (int id : reduceIDs)
        {
            _ids.erase(id);
        }

        auto rend = postRend->getFontRender();
        if (rend == nullptr)
        {
            return;
        }
        rend->clearCommonFont();

        if (nodeOrElement == mxr::PostNodeOrElement::PostNode)
        {
            for (auto iter = _ids.begin(); iter != _ids.end(); ++iter)
            {
                for (auto iter = _ids.begin(); iter != _ids.end(); ++iter)
                {
                    int cId = modeldata->_nodedata.nodeidmap.value(*iter);
                    double value = values.value(cId);
                    QVector3D vertex = modeldata->getCoordinates(cId) + deformationScale * dis.value(cId);
                    QString stringValue = QString("%1").arg(value);
                    pos_txt.push_back({ {vertex.x(), vertex.y(), vertex.z()}, stringValue });
                }
            }
        }
        else if (nodeOrElement == mxr::PostNodeOrElement::PostElement)
        {
            for (auto iter = _ids.begin(); iter != _ids.end(); ++iter)
            {
                int element = *iter;
                int partid = modeldata->getPartIDByEleID(element);
                QList<int> nodeid = modeldata->getElementNodeId(partid, element);

                QVector3D vertex = { 0.0, 0.0, 0.0 };
                for (int id : nodeid)
                {
                    vertex += modeldata->getCoordinates(id) + deformationScale * dis.value(id);
                }
                vertex = vertex / double(nodeid.size());

                double sum = 0;
                QHash<int, double> nodeVals = eleNodeVals[element];
                for (int n = 0; n < nodeid.size(); ++n)
                {
                    sum += nodeVals[nodeid[n]];
                }
                double value = sum / nodeid.size();
                QString stringValue = QString("%1").arg(value);
                pos_txt.push_back({ {vertex.x(), vertex.y(), vertex.z()}, stringValue });
            }
        }
        else
        {
            for (auto iter = _ids.begin(); iter != _ids.end(); ++iter)
            {
                int element = *iter;
                int partid = modeldata->getPartIDByEleID(element);
                QList<int> nodeid = modeldata->getElementNodeId(partid, element);

                QVector3D vertex = { 0.0, 0.0, 0.0 };
                for (int id : nodeid)
                {
                    vertex += modeldata->getCoordinates(id) + deformationScale * dis.value(id);
                }
                vertex = vertex / double(nodeid.size());

                double sum = 0;
                QHash<int, double> nodeVals = eleNodeVals[element];
                for (int n = 0; n < nodeid.size(); ++n)
                {
                    sum += nodeVals[nodeid[n]];
                }
                double value = sum / nodeid.size();
                QString stringValue = QString("%1").arg(value);
                pos_txt.push_back({ {vertex.x(), vertex.y(), vertex.z()}, stringValue });
            }
        }

        QVector<QVector3D> pos1;
        QVector<QString> txt;
        for (auto pos_txt0 : pos_txt)
        {
            pos1.push_back(pos_txt0.first);
            txt.push_back(pos_txt0.second);
        }
        auto fontRender = postRend->getFontRender();
        if (fontRender != nullptr)
        {
            fontRender->appendCommonFont("postValue", pos1, txt);
        }

        //清除高亮
        getCVM->clearPick();
    }

    void mPost3DResultPanel::hideAll()
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;

        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;
        _ids.clear();

        auto rend = postRend->getFontRender();
        if (rend == nullptr)
        {
            return;
        }
        rend->clearCommonFont();
        getCVM->clearPick();
    }

    void mPost3DResultPanel::switchToMaxFrame()
    {
        REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderByViewName(_curViewName);
        if (rebuildPostRend == nullptr) return;

        auto  postRender = rebuildPostRend->getPostRender();
        if (postRender == nullptr) return;

        QString postName = rebuildPostRend->getPostDataName();
        QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        if (modeldb == nullptr)	return;

        //获取GUI数据
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(rebuildPostRend);
        if (guidata == nullptr) return;

        //读取变量
        QString varstr = guidata->_cloudVarName;
        //SolidColor为伪变量，无结果数据，不存在最大值帧
        if (varstr == QString("SolidColor"))
            return;
        QString compostr = guidata->_cloudCompo;
        QString layerstr = guidata->_cloudLayerName;
        QString complexstr = guidata->_cloudComplex;
        QString coordinateName = QString("Global");
        QVector<double> userCoord{};
        QString postPathName = modeldb->getFile();

        QList<int> frames = modeldb->getFrameIds();
        // native 懒加载格式逐帧真读会触发整个文件逐帧摊平（大文件卡顿数十秒），
        // 暂保持跳过——最值搜索基于已发布帧的缓存，结果可能不含未读帧，明确提示而非静默
        const bool isNative = isLazyNativeResult(postPathName);
        if (isNative)
        {
            const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
            mGlobalSignals::getInstance()->outputMessageSig(2,
                isChinese ? QStringLiteral("原生结果文件（%1）暂不支持全局最值帧搜索：仅统计已加载帧，最大值帧可能不准")
                                .arg(postPathName)
                          : QStringLiteral("Global max-frame search is not fully supported for native result files (%1): only loaded frames are counted, the located frame may be inaccurate")
                                .arg(postPathName),
                true);
        }
        for (int frame : frames)
        {
            if (isNative)
                continue;

            if (modeldb->getDataFlag())
            {
                mIMxdbFile1* mxdbReader = new mIMxdbFile1;
                mxdbReader->setInitFileName(_postInitFiles.value(postName));
                if (varstr == "ElementNodalForce")
                {
                    if (!mxdbReader->readCloudData(postName, frame, varstr, compostr))
                    {
                        report3DReadFailure(frame, varstr, compostr, postPathName);
                    }
                }
                else
                {
                    if (!mxdbReader->readCloudData(postName, frame, varstr, compostr, layerstr, complexstr, coordinateName, userCoord))
                    {
                        report3DReadFailure(frame, varstr, compostr, postPathName);
                    }
                }
                delete mxdbReader;
            }
            else
            {
                // 这里需要处理变量值
                mMxdbThread* mxdbReader = new mMxdbThread(postPathName, modeldb, this);
                // 读取变量值
                if (!mxdbReader->readVarDatas(postPathName, frame, varstr, compostr, layerstr, complexstr, userCoord))
                {
                    report3DReadFailure(frame, varstr, compostr, postPathName);
                }
                delete mxdbReader;
            }
        }

        QHash<int, QPair<double, double>> allResults;
        for (int i = 0; i < frames.size(); i++)
        {
            QPair<double, double> pair = modeldb->getMaxMinByFrameID(frames[i]);
            allResults[frames[i]] = pair;
        }

        //查找结果最大值所在工况和帧
        int maxKey = 0;
        double maxValue = std::numeric_limits<double>::lowest();
        for (auto it = allResults.constBegin(); it != allResults.constEnd(); ++it)
        {
            double value = it.value().first;
            if (value > maxValue)
            {
                maxValue = value;
                maxKey = it.key();
            }
        }

        QString caseName = modeldb->getCaseNameByFrameNum(maxKey);
        //跳转=立即应用（同帧导航语义）：守卫屏蔽级联散绘，尾部统一整幅重绘
        _frameNavGuard = true;
        _comboCase->setCurrentText(caseName);

        QString index = modeldb->getFrameNameByFrameNum(maxKey);
        QStringList fnames = modeldb->getCaseFrameNames(caseName);
        for (auto name : fnames)
        {
            QString fName;
            if (name.contains(":"))
                fName = name.split(":").at(0);
            if (fName == index)
            {
                _comboFrame->setCurrentText(name);
                break;
            }
        }
        _frameNavGuard = false;
        drawCloudMap();
    }

    bool mPost3DResultPanel::allFrameRange(const QString& postName, QPointer<PostData::mPostDataBase> modeldb,
        const QString& varstr, const QString& compostr, const QString& layerstr, const QString& complexstr,
        QPair<double, double>& out)
    {
        //SolidColor 无结果数据；native/VTK 懒加载逐帧真读代价被明确规避（同
        //switchToMaxFrame 注释），返回 false 由调用方回退逐帧值域
        if (modeldb == nullptr || varstr.isEmpty() || varstr == QString("SolidColor"))
            return false;
        const QString postPathName = modeldb->getFile();
        if (isLazyNativeResult(postPathName) || isLazyVtkResult(postPathName))
            return false;

        QList<int> frames = modeldb->getFrameIds();
        if (frames.isEmpty())
            return false;

        //缓存按（结果名+变量身份+帧数）：切帧零开销命中；换变量首帧逐帧真读一次
        //（与 switchToMaxFrame 同口径：同步读+等待光标；帧数据按帧号独立存储，
        //读其它帧不影响当前显示帧）
        const QString key = postName + QChar('|') + varstr + QChar('|') + compostr + QChar('|') +
            layerstr + QChar('|') + complexstr;
        auto cached = _allFrameRanges.constFind(key);
        if (cached != _allFrameRanges.constEnd() && _allFrameRangeFrameCounts.value(key) == frames.size())
        {
            out = cached.value();
            return true;
        }

        QString coordinateName = QString("Global");
        QVector<double> userCoord{};
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        for (int frame : frames)
        {
            if (modeldb->getDataFlag())
            {
                mIMxdbFile1* mxdbReader = new mIMxdbFile1;
                mxdbReader->setInitFileName(_postInitFiles.value(postName));
                if (varstr == "ElementNodalForce")
                    mxdbReader->readCloudData(postName, frame, varstr, compostr);
                else
                    mxdbReader->readCloudData(postName, frame, varstr, compostr, layerstr, complexstr, coordinateName, userCoord);
                delete mxdbReader;
            }
            else
            {
                mMxdbThread* mxdbReader = new mMxdbThread(postPathName, modeldb, this);
                mxdbReader->readVarDatas(postPathName, frame, varstr, compostr, layerstr, complexstr, userCoord);
                delete mxdbReader;
            }
        }
        QApplication::restoreOverrideCursor();

        //逐帧最值聚合（getMaxMinByFrameID 返回 <max,min>；未读成功的空对 (0,0) 跳过）
        double gmin = DBL_MAX;
        double gmax = -DBL_MAX;
        for (int frame : frames)
        {
            QPair<double, double> p = modeldb->getMaxMinByFrameID(frame);
            if (p.second >= p.first)
                continue;
            gmax = qMax(gmax, p.first);
            gmin = qMin(gmin, p.second);
        }
        if (gmax <= gmin)
            return false;

        _allFrameRanges.insert(key, qMakePair(gmin, gmax));
        _allFrameRangeFrameCounts.insert(key, frames.size());
        out = qMakePair(gmin, gmax);
        return true;
    }

    void mPost3DResultPanel::resolveFullRange(float& maxval, float& minval, const QString& postName,
        QPointer<PostData::mPostDataBase> modeldb, const QString& varstr, const QString& compostr,
        const QString& layerstr, const QString& complexstr, mxr::IPostRendQueryDataProxy* queryProxy,
        MechPreData::mPostGUIData* guidata)
    {
        //图例范围三模式（见"图例和标签"组，HyperView Fixed/Dynamic Scale 对应）：
        //手动=guidata 锁定值域（用户改上下限后跨帧保持，slot_setMostValue 应用时写回）；
        //全程=全帧统一值域；逐帧=当前帧原始值域。全程不可得（native/VTK 懒加载等）
        //同样回退当前帧原始值域
        if (guidata != nullptr && guidata->_legendRange == QString("手动"))
        {
            if (guidata->_cloudMaxValue > guidata->_cloudMinValue)
            {
                maxval = float(guidata->_cloudMaxValue);
                minval = float(guidata->_cloudMinValue);
                return;
            }
        }
        if (guidata == nullptr || guidata->_legendRange == QString("全程"))
        {
            QPair<double, double> g;
            if (allFrameRange(postName, modeldb, varstr, compostr, layerstr, complexstr, g))
            {
                maxval = float(g.second);
                minval = float(g.first);
                return;
            }
        }
        maxval = queryProxy->getOriginalMaxData();
        minval = queryProxy->getOriginalMinData();
    }

    void mPost3DResultPanel::setShowMode()
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;

        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        auto viewportPrivate = VMManager->getCurrentViewportPrivate();
        if (viewportPrivate == nullptr) return;
        switch (viewportPrivate->_showViewCheckedStatus)
        {
        case 0:postRender->setShowFuntion(ShowFuntion::WireEdge);
            break;
        case 1:postRender->setShowFuntion(ShowFuntion::WireFrame);
            break;
        case 2:postRender->setShowFuntion(ShowFuntion::ElementFace);
            break;
        case 3:postRender->setShowFuntion(ShowFuntion::SmoothShaded);
            break;
        }
        postRend->updatePaint();
    }

    QStringList mPost3DResultPanel::sortQStringList(const QStringList& frameIds)
    {
        // 使用自定义的比较函数进行排序
        QStringList sortedList = frameIds;
        std::sort(sortedList.begin(), sortedList.end(), [](const QString& a, const QString& b) {
            // 将字符串转换为整数，然后比较大小
            int intA = a.split(":").at(0).toInt();
            int intB = b.split(":").at(0).toInt();
            return intA < intB;  // 从小到大排序
            });
        return sortedList;
    }

    void mPost3DResultPanel::drawCloudMap()
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;
        if (_isDrawCloudMap)
            this->slot_showCloud();
    }

    void mPost3DResultPanel::setCloudDirty(bool dirty)
    {
        //V1.3：应用渲染恒主色（用户拍板删 cloudPending 待应用高亮），脏标记仅内部记账
        //（应用/回填清零语义保留，无按钮视觉联动）
        if (_cloudParamDirty == dirty)
            return;
        _cloudParamDirty = dirty;
    }

    void mPost3DResultPanel::markCloudDirty()
    {
        setCloudDirty(true);
    }

    void mPost3DResultPanel::applyDiscreteSettings(mxr::REBUILD_PostRender* postRender, MechPreData::mPostGUIData* guidata)
    {
        if (postRender == nullptr || guidata == nullptr)
            return;
        //口径与三个即时槽（slot_discreteCombox/Order/TypeComboBox）原先的下发一致：
        //_cloudDiscrete==1 即"离散"；_cloudIsEquivariance==0 即"等差"
        // 离散三口收编（unify-addobject-arch）：一口 ColorLegend 掩码段（Dispersed|Order|Equivariance）
        mxr::ColorLegendParams _lg;
        _lg.updateMask = mxr::ColorLegendParams::Mask_Dispersed
            | mxr::ColorLegendParams::Mask_DispersedOrder | mxr::ColorLegendParams::Mask_Equivariance;
        _lg.dispersed = (guidata->_cloudDiscrete == 1);
        _lg.dispersedOrder = guidata->_cloudDiscreteNum;
        const bool isEquivariance = (guidata->_cloudIsEquivariance == 0);
        _lg.equivariance = isEquivariance;
        PostCommon::ensureUpdateSceneObject(postRender, _lg);   // 本函数形参=裸指针
        // S1：伴生窗直写原样保留（窗显示元素域，不改道面）
        if (auto* colorTable = postRender->getColorTableWindow())
            colorTable->setIsEquivariance(isEquivariance);
        postRender->updateAllModelOperate(UpdateMinMax);
    }

    void mPost3DResultPanel::dispatchCloudFilter()
    {
        //V1.3 三段行取值映射（控件模式 0无/1大于/2小于/3之间）：
        //大于=(左值,当前图例max)、小于=(当前图例min,右值)、之间=(左值,右值)——单值模式
        //另一侧边界取当前图例范围（沿用旧云图过滤后端双值接口）；无=清除
        const int mode = _filterRow->mode();
        if (mode == MxFunctions::mValueFilterRow::None)
        {
            MechCommon::mMechGlobalSignals::getInstance()->clickCloudClearFilterSig();
            return;
        }

        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;

        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return;

        bool okL = false;
        bool okR = false;
        const double left = _filterRow->leftText().trimmed().toDouble(&okL);
        const double right = _filterRow->rightText().trimmed().toDouble(&okR);

        QString minText = MxFunctions::mNumberToString(guidata->_cloudMinValue);
        QString maxText = MxFunctions::mNumberToString(guidata->_cloudMaxValue);
        QString modeText;
        if (mode == MxFunctions::mValueFilterRow::Greater)
        {
            if (!okL) return;
            maxText = MxFunctions::mNumberToString(left);
            modeText = QString("大于");
        }
        else if (mode == MxFunctions::mValueFilterRow::Less)
        {
            if (!okR) return;
            minText = MxFunctions::mNumberToString(right);
            modeText = QString("小于");
        }
        else
        {
            if (!okL || !okR) return;
            minText = MxFunctions::mNumberToString(left);
            maxText = MxFunctions::mNumberToString(right);
            modeText = QString("之间");
        }
        MechCommon::mMechGlobalSignals::getInstance()->clickCloudStartFilterSig(minText, maxText, modeText);
    }

    bool mPost3DResultPanel::isContainStr(const QStringList& names, QString str, QString& name)
    {
        for (auto it : names)
        {
            if (it.contains(str, Qt::CaseInsensitive))
            {
                name = it;
                return true;
            }
        }
        return false;
    }

    void mPost3DResultPanel::slot_mxdbThreadFinished()
    {
        //获取信号
        QString postname;
        mMxdbThread* mxdb = qobject_cast<mMxdbThread*>(sender());
        postname = mxdb->getModelName();
        if (mxdb->isReadSuccess() == false)
        {
            //失败按捕获的宿主模型删库/通知（期间切模型会使 getCVM 指向别的模型）。
            //D-4：原直接 return 连失败信号都不发——会话恢复期间 _importing 永不复位，
            //恢复收尾与保存/打开会话全部卡死（与旧格式当年只修发射点同型）
            const int ownerID = _mxdbThreadOwner.value(mxdb, -1);
            _mxdbThreadOwner.remove(mxdb);
            auto* ownerModel = ownerID >= 0
                ? MechPreData::mViewportModelManager::getInstance()->getViewportModel(ownerID)
                : getCVM;
            if (ownerModel != nullptr && ownerModel->getModelData() != nullptr)
                ownerModel->getModelData()->getManager<mPostDataManager>(Physical_Post).deletePostDatabase(postname);
            QApplication::restoreOverrideCursor();
            emit MechCommon::mMechGlobalSignals::getInstance()->postResultImportFailedSig(ownerID);
            return;
        }
        if (mxdb == nullptr)
        {
            QApplication::restoreOverrideCursor();
            return;
        }
        //归属中转（线程即将销毁，下游失败发射点据此发通知）
        _finishedImportOwnerID = _mxdbThreadOwner.value(mxdb, -1);
        _mxdbThreadOwner.remove(mxdb);
        mxdb->exit();
        mxdb->wait();
        delete mxdb;
        mxdb = nullptr;

        ////新建视口并存储数据图
        //REBUILD_PostRend* postRend = _curPostRend;
        //bool isNew = false;
        //QString oldName = "";
        //if (postRend != nullptr)
        //{
        //	oldName = postRend->getPostDataName();
        //	QPointer<mPostDataBase> dp = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(oldName);
        //	if (dp != nullptr)
        //	{
        //		if (oldName != postname)
        //		{
        //			int count = mViewportManager::getInstance()->getRepeatModelViewCount(oldName);
        //			if (count <= 1)
        //			{
        //				//删除数据库
        //				MIOFile::mPostDatabaseSingleton::getInstance()->deletePostDatabase(oldName);
        //			}
        //			else
        //			{
        //				//清除该视图对应的模型数据库中变化的数据
        //				MIOFile::mPostDatabaseSingleton::getInstance()->deleteViewData(postRend->getViewportName(), oldName);
        //			}
        //		}
        //		QString viewName = postRend->getViewportName();
        //		if (dp != nullptr)
        //			dp->clearViewNameFrameData(viewName);
        //		PostCommon::CameraManager::getInstance().removeObserver(postRend);
        //		postRend = mViewportManager::getInstance()->clearViewport3D(postRend);
        //	}
        //}
        //else
        //{
        //	return;
        //}

        //_curPostRend = postRend;
        //postRend->setPostDataName(postname);
        //auto postrender = postRend->getPostRender();
        //mSubWindow* msWindow = mViewportManager::getInstance()->getSubWindowFromViewportREBUILD(postRend);
        //mViewportManager::getInstance()->subWindowActivated(msWindow);

        //mGlobalSignals::getInstance()->setProgressBarValue(96);
        //if (postrender != nullptr)
        //{
        //	connect(postrender.get(), &mxr::REBUILD_PostRender::finishedPickSig, mViewportManager::getInstance(), &mViewportManager::finishedPickSigManger);
        //	connect(postrender.get(), &mxr::REBUILD_PostRender::updateAnimationPanelSig, mViewportManager::getInstance(), &mViewportManager::updateAnimationPanelSigManger);
        //	connect(postrender.get(), &mxr::REBUILD_PostRender::finishedDragSig, mViewportManager::getInstance(), &mViewportManager::finishedDragSigManger);
        //}

        //QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
        //int frameId = 1;
        //QString viewName = postRend->getViewportName();
        //mxr::PostFrameDataProxy* frame_data_proxy = modeldb->getFrameDataProxyByFrameId(viewName, frameId);
        //if (frame_data_proxy == nullptr)
        //{
        //	mxr::PostFrameDataProxy* frame_data_proxy = mxr::createFrameDataProxy(frameId, postRend->getViewportName(), modeldb);
        //	modeldb->setFrameDataProxyByFrameId(postRend->getViewportName(), frameId, frame_data_proxy);
        //}

        //mViewportManager::getInstance()->update_all_gui_data();

        QString str = postname;
        //读取后处理文件完成
        emit finishedReadPostFileSig(str);
        mGlobalSignals::getInstance()->setProgressBarValue(100);

        QApplication::restoreOverrideCursor();
    }

    void mPost3DResultPanel::slot_nativeThreadFinished(const QString& postname)
    {
        MIOFile::mIONativeFile* reader = qobject_cast<MIOFile::mIONativeFile*>(sender());
        if (reader == nullptr)
            return;

        if (postname.isEmpty())
        {
            //失败回滚按导入启动时捕获的宿主模型删库（期间用户切模型会使 getCVM
            //指向别的模型，误删他模型的结果库）；捕获缺失时退回当前模型保持原行为
            const int ownerID = _nativeReaderOwner.value(reader, -1);
            _nativeReaderOwner.remove(reader);
            const QString databaseName = reader->getPostDataName();
            if (!databaseName.isEmpty())
            {
                auto* ownerModel = ownerID >= 0
                    ? MechPreData::mViewportModelManager::getInstance()->getViewportModel(ownerID)
                    : getCVM;
                if (ownerModel != nullptr && ownerModel->getModelData() != nullptr)
                    ownerModel->getModelData()->getManager<mPostDataManager>(Physical_Post).deletePostDatabase(databaseName);
            }
            _nativeReaders.remove(_nativeReaders.key(reader, QString()));
            reader->deleteLater();
            mGlobalSignals::getInstance()->setProgressBarValue(100);
            QApplication::restoreOverrideCursor();
            //导入失败通知（postModelID=捕获的宿主模型：会话恢复队列据此跳过该条，
            //后处理树窗口回滚槽据此回收漏斗新建的空模型树并作废key，不发则恢复链
            //等待永不到来的 updateResultTreeSig 而卡死）
            emit MechCommon::mMechGlobalSignals::getInstance()->postResultImportFailedSig(ownerID);
            return;
        }

        //归属中转（成功路径 reader 常驻 _nativeReaders 供懒读，owner 映射保留）：
        //下游 slot_finishedImportFiles 结果库无效分支据此发失败通知
        _finishedImportOwnerID = _nativeReaderOwner.value(reader, -1);
        emit finishedReadPostFileSig(postname);
        mGlobalSignals::getInstance()->setProgressBarValue(100);
        QApplication::restoreOverrideCursor();
    }

    void mPost3DResultPanel::slot_vtkThreadFinished(const QString& postname)
    {
        MIOFile::mIOVtkFile* reader = qobject_cast<MIOFile::mIOVtkFile*>(sender());
        if (reader == nullptr)
            return;

        if (postname.isEmpty())
        {
            //失败回滚按导入启动时捕获的宿主模型删库（同 slot_nativeThreadFinished）
            const int ownerID = _vtkReaderOwner.value(reader, -1);
            _vtkReaderOwner.remove(reader);
            const QString databaseName = reader->getPostDataName();
            if (!databaseName.isEmpty())
            {
                auto* ownerModel = ownerID >= 0
                    ? MechPreData::mViewportModelManager::getInstance()->getViewportModel(ownerID)
                    : getCVM;
                if (ownerModel != nullptr && ownerModel->getModelData() != nullptr)
                    ownerModel->getModelData()->getManager<mPostDataManager>(Physical_Post).deletePostDatabase(databaseName);
            }
            _vtkReaders.remove(_vtkReaders.key(reader, QString()));
            reader->deleteLater();
            mGlobalSignals::getInstance()->setProgressBarValue(100);
            QApplication::restoreOverrideCursor();
            emit MechCommon::mMechGlobalSignals::getInstance()->postResultImportFailedSig(ownerID);
            return;
        }

        //归属中转（成功路径 reader 常驻 _vtkReaders 供懒读，owner 映射保留）
        _finishedImportOwnerID = _vtkReaderOwner.value(reader, -1);
        emit finishedReadPostFileSig(postname);
        mGlobalSignals::getInstance()->setProgressBarValue(100);
        QApplication::restoreOverrideCursor();
    }

    void mPost3DResultPanel::slot_mxdbOldThreadFinished()
    {
        //获取信号
        QString postname;
        mIMxdbFile1* mxdb = qobject_cast<mIMxdbFile1*>(sender());
        if (mxdb != nullptr)
        {
            postname = mxdb->getPostDataName();
            //归属中转（线程即将销毁）：失败时 postname 为空串到达下游
            //slot_finishedImportFiles，经 _finishedImportOwnerID 带出宿主模型ID
            _finishedImportOwnerID = _mxdbOldThreadOwner.value(mxdb, -1);
            _mxdbOldThreadOwner.remove(mxdb);
        }
        disconnect(mxdb);
        mxdb->exit();
        mxdb->wait();
        delete mxdb;
        mxdb = nullptr;


        mGlobalSignals::getInstance()->setProgressBarValue(97);

        ////新建视口并存储数据图
        //REBUILD_PostRend* postRend = _curPostRend;
        //bool isNew = false;
        //QString oldName = "";
        //if (postRend != nullptr)
        //{
        //	oldName = postRend->getPostDataName();
        //	QPointer<mPostDataBase> dp = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(oldName);
        //	if (dp != nullptr)
        //	{
        //		if (oldName != postname)
        //		{
        //			int count = mViewportManager::getInstance()->getRepeatModelViewCount(oldName);
        //			if (count <= 1)
        //			{
        //				//删除数据库
        //				MIOFile::mPostDatabaseSingleton::getInstance()->deletePostDatabase(oldName);
        //			}
        //			else
        //			{
        //				//清除该视图对应的模型数据库中变化的数据
        //				MIOFile::mPostDatabaseSingleton::getInstance()->deleteViewData(postRend->getViewportName(), oldName);
        //			}
        //		}
        //		QString viewName = postRend->getViewportName();
        //		if (dp != nullptr)
        //			dp->clearViewNameFrameData(viewName);
        //		PostCommon::CameraManager::getInstance().removeObserver(postRend);
        //		postRend = mViewportManager::getInstance()->clearViewport3D(postRend);
        //	}
        //}
        //else
        //{
        //	return;
        //}

        //_curPostRend = postRend;
        //postRend->setPostDataName(postname);
        //auto postrender = postRend->getPostRender();
        //mSubWindow* msWindow = mViewportManager::getInstance()->getSubWindowFromViewportREBUILD(postRend);
        //mViewportManager::getInstance()->subWindowActivated(msWindow);

        //if (postrender != nullptr)
        //{
        //	connect(postrender.get(), &mxr::REBUILD_PostRender::finishedPickSig, mViewportManager::getInstance(), &mViewportManager::finishedPickSigManger);
        //	connect(postrender.get(), &mxr::REBUILD_PostRender::updateAnimationPanelSig, mViewportManager::getInstance(), &mViewportManager::updateAnimationPanelSigManger);
        //	connect(postrender.get(), &mxr::REBUILD_PostRender::finishedDragSig, mViewportManager::getInstance(), &mViewportManager::finishedDragSigManger);
        //}

        //QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);

        //int frameId = 1;
        //QString viewName = postRend->getViewportName();
        //mxr::PostFrameDataProxy* frame_data_proxy = modeldb->getFrameDataProxyByFrameId(viewName, frameId);
        //if (frame_data_proxy == nullptr)
        //{
        //	mxr::PostFrameDataProxy* frame_data_proxy = mxr::createFrameDataProxy(frameId, postRend->getViewportName(), modeldb);
        //	modeldb->setFrameDataProxyByFrameId(postRend->getViewportName(), frameId, frame_data_proxy);
        //}

        //QString str = postname + "_##_" + viewName;
        //读取后处理文件完成（postname 为空=导入失败：空串传给
        //slot_finishedImportFiles 走其失败分支发 postResultImportFailedSig）
        QString str = postname;
        emit finishedReadPostFileSig(str);
        mGlobalSignals::getInstance()->setProgressBarValue(100);

        //失败不报成功（线程失败时 postname 为空）
        if (!postname.isEmpty())
        {
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig(0, QString("后处理文件导入成功！"), true);
            }
            else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
            {
                mGlobalSignals::getInstance()->outputMessageSig(0, QString("Post-processing file imported successfully!"), true);
            }
        }

        QApplication::restoreOverrideCursor();
        qDebug() << "刷新";
    }

    void mPost3DResultPanel::slot_finishedImportFiles(const QString& postname)
    {
        if (postname.isEmpty())
        {
            //导入失败通知（旧格式 mxdb 线程失败以空串到达此处；postModelID=发射前
            //中转的宿主模型ID——会话恢复队列据此跳过该条、回滚槽据此回收空树，
            //不发则恢复链卡死等待完成信号）
            emit MechCommon::mMechGlobalSignals::getInstance()->postResultImportFailedSig(_finishedImportOwnerID);
            return;
        }
        //结果库查 owner 模型而非 getCVM（同失败分支的捕获ID法）：导入期间用户切模型
        //会使 getCVM 指向别的模型——在错误模型上查不到库会被误判成导入失败，回滚槽
        //据此删掉导入成功且已注册的实例并作废有效key；宿主模型已被删（读取中途删除）
        //则库随模型俱亡，按失败收尾同样正确。查得的库也供下方 autoApplyNotes 使用
        auto* ownerModel = _finishedImportOwnerID >= 0
            ? MechPreData::mViewportModelManager::getInstance()->getViewportModel(_finishedImportOwnerID)
            : getCVM;
        mPostDataBase* postDB = (ownerModel != nullptr && ownerModel->getModelData() != nullptr)
            ? ownerModel->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname)
            : nullptr;
        if (postDB == nullptr)
        {
            //结果库无效：同样按导入失败通知（恢复链不可就此认为导入完成）
            emit MechCommon::mMechGlobalSignals::getInstance()->postResultImportFailedSig(_finishedImportOwnerID);
            return;
        }

        if (!_isUpdateResult)
        {
            //更新模型树数据,创建三维结果节点：经后处理树自身的更新入口
            //（以后处理树选中为上下文，无选中时为后处理模型根）
            emit MechCommon::mMechGlobalSignals::getInstance()->updateResultTreeSig();
            if (_curPostRend == nullptr) return;
            _curPostRend->setPostDataName(postname);

            mGlobalSignals::getInstance()->setProgressBarValue(98);
            //激活后处理视图
            QString viewName = _curPostRend->getViewportName();
            mViewTreeFunction::activatePostViewport(viewName);

            //更新云图参数面板数据
            update3DResultPanel(viewName);
            //更新模型树部件
            if (getCVM->isPostModel())
            {
                MechCommon::mMechGlobalSignals::getInstance()->updatePostModelTree();
            }
            mGlobalSignals::getInstance()->setProgressBarValue(99);

            //更新后处理渲染视角
            _curPostRend->FitView();
            // R5 缺陷修复（同 update3DResultPanel 收尾点）：FitView 因包围盒空早退时
            // 置挂起旗标，待帧实例就绪后由 slot_showCloud 补偿一次
            if (auto* finishGuidata = getCVM->getGUIDataByPostRender(_curPostRend))
            {
                if (!_curPostRend->getAABBOfView().valid())
                {
                    finishGuidata->_pendingFitView = true;
                }
            }
            _curPostRend->updatePaint();

            // 导入完成后自动显示标注信息
            mPostInfoHub::getInstance()->autoApplyNotes(postDB->getCaseId());

            //初始视口显示名接管：导入收尾链（含面板 _curViewName 全链）以协议名工作
            //完毕后才改名——中途改名会使 getPostRenderByViewName/getGUIDataByViewName
            //按名查找落空（变量/分量下拉为空的故障根因）；改名后同步面板当前名。
            //仅当当前视口仍是裸初始协议名时接管（收编/接管过则跳过，幂等）
            //【二分禁用已解除，2026-10-13：灰显根因为 mMenuWidget 正则坏转义（\\d 误写 \d），
            //与本接管无关，恢复启用】
            if (_curViewName == PostWindow::mViewportName::initialCloud(getCVM->getModelData()->getModelDataID()))
            {
                auto* vm = MechPreData::mViewportModelManager::getInstance()->getViewportModel(
                    getCVM->getModelData()->getModelDataID());
                const QString displayName = getCVM->getModelData()->getModelDataName()
                    + QString::fromUtf8("· ") + QString::fromUtf8("云图");
                if (vm != nullptr && vm->renameViewport(
                    vm->getViewportIDByName(_curViewName), displayName))
                {
                    _curViewName = displayName;
                }
            }
        }
        else
        {
            updateResultView(_updateViewNames, postname);
            mGlobalSignals::getInstance()->setProgressBarValue(98);
            _isUpdateResult = false;
        }
    }

    void mPost3DResultPanel::slot_create3DResultViewport(QString viewport)
    {
        if (!getCVM->getViewport(viewport))
        {
            //创建视口
            mViewTreeFunction::newViewport(MxFunctions::ViewportPost3D, viewport);
        }
        //R2 Q-LIFE-1 审计修复：_curViewName 第 5 写者补同款浮层守卫（公共助手）——
        //播放中新建/激活其它视口级联至此直写 _curViewName，会把浮层 tick 驱动重定向
        //到新视口（其无数据时每 tick 早退=播放静默冻结，旧视口同冻），P3-1 同类旁路。
        //异名即关浮层+按钮归位；同名零动作
        switchViewGuard(viewport);
        _curPostRend = getCVM->getPostRenderByViewName(viewport);
        _curViewName = viewport;
    }

    void mPost3DResultPanel::slot_chooseCase(const QString& cname)
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;
        // S5 D14 数据纪元：值数据将变化——已装载帧条目置待重验（绑定/重建时工厂重读新值）
        if (auto* sessionFacade = postRend->getPostRender().get())
        {
            sessionFacade->notifyDataChanged();
        }

        //获取GUI数据
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return;

        QString postName = postRend->getPostDataName();
        mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        if (postDB == nullptr)
            return;

        QStringList fnames = postDB->getCaseFrameNames(cname);
        fnames = sortQStringList(fnames);
        guidata->_cloudFrames = fnames;

        QStringList names = postDB->getCaseFrameShortNames(cname);
        names = sortQStringList(names);
        guidata->_cloudShortFrames = names;

        _comboFrame->blockSignals(true);
        _comboFrame->clear();
        _comboFrame->addItems(fnames);
        if (!fnames.isEmpty())
            _comboFrame->setCurrentIndex(0);
        _comboFrame->blockSignals(false);
        //实时动画 v1.2（观察项 A 驱动点扩展）→ v1.4 常显：帧表随工况重建→面板级动画
        //上下文重建——单动源互斥旗置回（首次播放放行再消费，与 v1.3 创建即停同频）；
        //挂载/禁用域/帧范围/帧时间轴刷新（无结果停播+全禁，D-08）
        _animDeletePending = true;
        updateRealtimeAnimEnabled();
        // 手动触发帧选择，避免 addItems/setCurrentIndex 在信号阻塞下未发射 currentIndexChanged
        if (!fnames.isEmpty())
            slot_chooseFrame(fnames.first());
    }

    void mPost3DResultPanel::slot_chooseFrame(const QString&)
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr)
        {
            return;
        }

        //获取GUI数据
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return;

        QString postName = postRend->getPostDataName();
        mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        if (postDB == nullptr)
            return;

        //获取帧号（根据当前帧显示的帧名称与工况名获取帧号）
        int fi = 0;
        QString fName;
        QString framestr = _comboFrame->currentText();
        if (framestr.contains(":"))
            fName = framestr.split(":").at(0);
        QString cur_text = _comboCase->currentText();
        fi = postDB->getFrameNumByCaseAndFrameName(cur_text, fName);
        //实时动画 v1：面板侧切帧（帧下拉/导航钮）同步浮层滑条与帧号显示
        //（applyIndex 不回发信号，无环路）。v1.1 观察项 B：上移到 fi<=0 早退之前——
        //帧名解析失败帧上浮层滑条同样回同步，防浮层与面板显示脱节（早退分支原跳过同步）
        if (_realtimeAnimWidget != nullptr)
            _realtimeAnimWidget->setFrameIndex(_comboFrame->currentIndex());
        //帧名解析失败（fi<=0：越界索引触发空文本切换、工况/帧表暂态不一致等）时保持
        //现状直接返回：继续走会按 fi=0 把变量表重建为仅 SolidColor，当前变量被静默
        //重置、云图退化成部件基色且图例清零（真机走查 2026-11-09 单帧结果越界切帧复现）
        if (fi <= 0)
            return;

        //获取该帧的变量
        QStringList vnames = postDB->getVariableByFrameID(fi);
        //首项插入伪变量SolidColor：不读取结果数据，按部件自身颜色显示
        vnames.prepend(QString("SolidColor"));
        //恢复源取本视口的持久状态而非共享控件残留文字：云图面板单实例跨模型共用，
        //读控件会把上一结果文件的变量选择泄漏进新导入文件（同名变量被误恢复）
        QString oldVar = guidata->_cloudVarName;
        int vari = vnames.indexOf(oldVar);

        // S6 v1.2 播放快速路径（真机"播放中切变量点了没反应/选中态闪烁/打开无
        // 选中高亮"修复）：实时播放每 tick 级联至此，下方 clear/addItems 每帧拆建
        // 变量下拉框——用户正打开的弹窗被关闭、选中态被复位，即三项真机现象的
        // 直接来源。变量表、guidata 侧列表与期望选中项均与控件现状一致时（逐帧
        // 播放常态）跳过拆建与变形表重算：变形表仅由变量名集合决定
        // （getVariableCompos/getVariableType 均按名查询，与帧无关），输入不变
        // 则输出不变。前提 _lastChooseVarIdentity 非空：新结果数据落地收口
        // update3DResultPanel 已将其复位，保证导入/更新后首帧必走完整拆建。
        QString wantVar;
        if (vari >= 0 && !oldVar.isEmpty())
            wantVar = oldVar;
        else if (!vnames.isEmpty())
            wantVar = vnames.first();
        bool varComboIntact = !_lastChooseVarIdentity.isEmpty()
            && wantVar == _comboVariable->currentText()
            && guidata->_cloudVarNames == vnames
            && _comboVariable->count() == vnames.size();
        if (varComboIntact)
        {
            for (int i = 0; i < vnames.size(); ++i)
            {
                if (_comboVariable->itemText(i) != vnames.at(i))
                {
                    varComboIntact = false;
                    break;
                }
            }
        }
        if (!varComboIntact)
        {
            //【FR-07 二期（cloud-panel-frame-sync）】播放中变量恢复段仲裁（04 评审 C1）：
            //本段身兼两职——FR-07 要冻结的"弹回执行者"（用户面板待生效改动被 guidata
            //旧值回写弹回）与一期 FR-03 依赖的"云图菜单→面板"同步器（云图菜单播放中
            //即时应用写 guidata 后，靠本段把面板选中项拽到新选择，tick 渲染链才取得到）。
            //两种失配代码形态相同、语义相反，以面板待生效脏标 _cloudParamDirty 仲裁：
            //①dirty=true＝面板有待生效改动（延迟应用语义，guidata 滞后）→ 面板优先，
            //  整段跳过保持用户选择——选中项经本 tick 下方手动派发→slot_showCloud
            //  "顺带应用"落 guidata（自愈收敛：应用后下一 tick 起走 varComboIntact 快路径）；
            //②dirty=false＝面板无待生效（外部写 guidata，如云图菜单播放中改变量）→
            //  不拆建列表仅静默 setCurrentText 恢复选中项——列表稳定性（FR-07①）与
            //  FR-03 跨面板传播两全，派发链读点取到 guidata 侧最新选择；
            //变量表真变（异构帧数据/切工况暂态，逐项比对失配）仍走 legacy 拆建恢复
            //（数据正确性优先，与现状一致的退化路径）；非播放态本守卫不生效，现状
            //拆建路径逐字保留。已知不精确处（评审备案）：dirty 亦被范围/离散类待应用
            //置位，该窗口内到达的云图菜单变量改动会被判面板优先而吃掉（现状语义恒
            //菜单优先），见 05-实施记录-二期备案项
            auto postRenderInfo = postRend->getPostRender();
            const bool cloudPlaying = postRenderInfo != nullptr
                && (postRenderInfo->playbackState() == mxr::PlaybackState::Playing
                    || isRealtimeAnimPlaying());
            bool varListUnchanged = _comboVariable->count() == vnames.size();
            for (int i = 0; varListUnchanged && i < vnames.size(); ++i)
            {
                if (_comboVariable->itemText(i) != vnames.at(i))
                    varListUnchanged = false;
            }
            if (cloudPlaying && varListUnchanged && _cloudParamDirty)
            {
                //面板待生效优先：保持用户选择（列表逐项一致，列表同步与变形表重算
                //均为幂等零效果，S6 v1.2 快路径同款结论，一并跳过）
            }
            else if (cloudPlaying && varListUnchanged)
            {
                //外部写 guidata：静默恢复选中项（不拆建——FR-07① 列表稳定不闪）
                if (vari >= 0 && !oldVar.isEmpty())
                {
                    _comboVariable->blockSignals(true);
                    _comboVariable->setCurrentText(oldVar);
                    _comboVariable->blockSignals(false);
                }
            }
            else
            {
                _comboVariable->blockSignals(true);
                _comboVariable->clear();
                _comboVariable->addItems(vnames);
                // 优先恢复本视口上次的变量；没有历史变量（如新导入文件的视口）则默认选中
                // 首项伪变量 SolidColor（按部件基色显示，产品确认的默认项）
                if (vari >= 0 && !oldVar.isEmpty())
                    _comboVariable->setCurrentText(oldVar);
                else if (!vnames.isEmpty())
                    _comboVariable->setCurrentIndex(0);
                _comboVariable->blockSignals(false);
                guidata->_cloudVarNames = vnames;
                //变形变量列表同步：当前帧节点变量且前三分量为空间矢量（X/Y/Z 或 T1/T2/T3；
                //首项=文件位移变量，作变形默认；张量/标量/伪变量 SolidColor 不入表）
                {
                    QHash<QString, QStringList> composMap;
                    QHash<QString, int> varTypes;
                    for (const QString& name : vnames)
                    {
                        if (name == QString("SolidColor"))
                            continue;
                        composMap.insert(name, postDB->getVariableCompos(name));
                        varTypes.insert(name, postDB->getVariableType(name));
                    }
                    guidata->_deformVarNames = detail::filterDeformVariables(vnames, composMap, varTypes);
                }
            }
        }
        // 手动触发变量选择，避免 addItems/setCurrentIndex 在信号阻塞下未发射 currentIndexChanged，
        // 导致导入后云图未绘制。
        QString selectedVar = _comboVariable->currentText();
        if (!selectedVar.isEmpty())
            slot_chooseVariable(selectedVar);

        updatePlanishEnabled();

        //帧切换=立即应用（用户在帧下拉直接选择时 sender 为 _comboFrame；工况级联为手动
        //调用/帧导航守卫期不在此绘——工况保持延迟，导航由尾部统一补绘）。
        //整幅重绘现读面板全部控件，会顺带应用其它待生效参数并清脏
        if (sender() == _comboFrame && !_frameNavGuard)
            drawCloudMap();
    }

    void mPost3DResultPanel::slot_chooseVariable(const QString& vname)
    {
        _isDrawCloudMap = false;

        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr)
        {
            return;
        }
        // S5 D14 数据纪元：值数据将变化——已装载帧条目置待重验（绑定/重建时工厂重读新值）
        //P1-1/P6-3 审计修复：身份判别跳过纪元 bump——实时播放每 tick 经 slot_changedFrame→
        //slot_chooseFrame 进入本槽（逐帧切换，变量/分量/层/复数/磨平身份未变），无条件
        //notifyDataChanged 使全帧表纪元逐帧 +1：打穿已装载帧"命中实例零动作复用"（播放
        //逐帧 GL 值重上传+后台 slicing 常开排队），录制让路后的逐帧走查也工厂重读磁盘
        //（P6-3 同根表现）。身份与 _cloudReadIdentityByKey 同源口径（视口|工况|变量|分量|层|复数|磨平）；
        //真实变量/分量/层/复数/磨平/工况切换身份必变，bump 不受影响（chooseCase/Compo/
        //Layer/Complex 各自入口另有无条件 bump 兜底）
        const QString chooseVarIdentity = QStringLiteral("%1|%2|%3|%4|%5|%6|%7")
            .arg(_curViewName, _comboCase->currentText(), vname,
                 _comboComponent->currentText(), _comboLayer->currentText(),
                 _comboComplex->currentText(),
                 _comboSmoothMethod->currentIndex() >= 1 ? QStringLiteral("1") : QStringLiteral("0"));
        if (chooseVarIdentity != _lastChooseVarIdentity)
        {
            _lastChooseVarIdentity = chooseVarIdentity;
            if (auto* sessionFacade = postRend->getPostRender().get())
            {
                sessionFacade->notifyDataChanged();
            }
        }
        else if (!chooseVarIdentity.isEmpty())
        {
            // S6 v1.2 播放快速路径：七元组身份与上次完全一致（实时播放每 tick 经
            // slot_chooseFrame 手动派发进入，视口|工况|变量|分量|层|复数|磨平全未变）
            // 时，跳过中段"分量/层列表重建+下拉重填"整段与自由节点分支，直进等价
            // 尾部链（_isDrawCloudMap=true→drawCloudMap→slot_showCloud）。中段每 tick
            // 拆建 _comboComponent/_comboLayer，与 slot_chooseFrame 的变量下拉拆建
            // 同帧叠加，放大真机"播放中切变量点了没反应/选中态闪烁"。读值真正需要
            // 的只是按当前 fi 的重绘链：slot_showCloud 自解析帧号并从各下拉
            // currentText 取参（分量/层/复数/磨平均被身份钉住），跨帧只变 fi。
            // 跳过段在身份未变下均为幂等零效果：列表重填（同项重灌）、自由节点分支
            // （仅由 varType(vname) 决定，与帧无关）、updatePlanishEnabled（计算后弃置）。
            // 守卫：guidata->_cloudVarName==vname（渲染链最近一次实际显示变量，在
            // slot_showCloud/slot_changedFrame 收口写入）——防新数据落地 guidata 未
            // 重建时误入；导入收口 update3DResultPanel 已复位身份缓存，此为双保险。
            // 守卫不过即落入下方完整路径，行为与既有代码一致。
            if (postRend->getViewportName() == _curViewName)
            {
                mPostDataBase* postDBFast = getCVM->getModelData()
                    ->getManager<mPostDataManager>(Physical_Post)
                    .getPostDatabase(postRend->getPostDataName());
                mPostGUIData* guidataFast = postDBFast != nullptr
                    ? getCVM->getGUIDataByPostRender(postRend) : nullptr;
                if (postDBFast != nullptr && guidataFast != nullptr
                    && guidataFast->_cloudVarName == vname)
                {
                    _isDrawCloudMap = true;
                    drawCloudMap();
                    return;
                }
            }
        }
        QString viewName = postRend->getViewportName();
        if (viewName != _curViewName)
        {
            return;
        }
        QString postName = postRend->getPostDataName();
        mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        if (postDB == nullptr)
            return;

        //获取GUI数据
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return;

        //分量/层的按工况记忆随视口走：先同步本视口的记忆表再查，避免新导入文件
        //沿用工况名相同的旧视口记忆（面板单实例跨模型共用的第二个泄漏点）
        _caseInfos = guidata->_caseInfos;

        QString caseName = _comboCase->currentText();
        PostData::oneCaseInfo* caseInfo = _caseInfos.value(caseName);

        if (caseInfo == nullptr)
        {
            caseInfo = new PostData::oneCaseInfo();
            caseInfo->caseName = caseName;
            caseInfo->varName = vname;
            _caseInfos.insert(caseName, caseInfo);
        }
        else
        {
            if (caseInfo->varName != vname)
            {
                delete caseInfo;
                caseInfo = nullptr;
                caseInfo = new PostData::oneCaseInfo();
                caseInfo->caseName = caseName;
                caseInfo->varName = vname;
                _caseInfos.insert(caseName, caseInfo);
            }
        }

        guidata->_caseInfos = _caseInfos;
        if (postDB == nullptr)
            return;

        //伪变量SolidColor：无分量/层/复数，不读取结果数据，按部件自身颜色显示
        if (vname == QString("SolidColor"))
        {
            guidata->_cloudCompos.clear();
            guidata->_cloudHaveComplex = false;
            guidata->_cloudHaveLayer = false;
            _comboComponent->blockSignals(true);
            _comboComponent->clear();
            _comboComponent->setDisabled(true);
            _comboLayer->blockSignals(true);
            _comboLayer->clear();
            _comboLayer->setDisabled(true);
            _comboComplex->setDisabled(true);
            //复数组合框重置为初始状态
            _comboComplex->blockSignals(true);
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                _comboComplex->setCurrentText("实部");
            else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
                _comboComplex->setCurrentText("Real");
            _comboComplex->blockSignals(false);
            _comboComponent->blockSignals(false);
            _comboLayer->blockSignals(false);

            //SolidColor无结果数据，禁用自由节点显示
            guidata->_cloudHaveFreeNode = false;
            MechCommon::mMechGlobalSignals::getInstance()->updateFreeNodeBtnStatus(true);

            updatePlanishEnabled();
            _isDrawCloudMap = true;
            //延迟应用：伪变量切换同样待"应用渲染"生效
            markCloudDirty();
            //【FR-07② 二期（cloud-panel-frame-sync）】预生成瞬态播放中无逐 tick 整幅重绘，
            //延迟应用的变量改动会被停机回填按"未应用的修改随之丢弃"语义静默丢弃——
            //即时应用一次：以会话当前帧静默对齐帧下拉 → 既有 drawCloudMap 整幅重绘
            //"顺带应用"（应用通道零新增，延迟应用语义本体不改写）→ 恢复冻结显示
            //（FR-08 口径一致）。浮层播放中不进（playbackState 恒 Stopped）；参数化插值域
            //不进（P4-1 双域防误伤）；非播放态不进（延迟应用原样）。帧名映射工况取
            //guidata->_cloudCase（04 建议 5：与一期失步收敛段同源，免疫 UI 时序差）；
            //未命中 fail-soft 放弃应用（帧对齐漂移由停机收敛段兜底）
            auto applySession = postRend->getPostRender();
            if (applySession != nullptr
                && applySession->playbackState() == mxr::PlaybackState::Playing
                && applySession->displayMode() == mxr::DisplayMode::TransientAnimation)
            {
                const int sessionFi = applySession->currentFrame();
                int hitIndex = -1;
                if (sessionFi > 0)
                {
                    for (int i = 0; i < _comboFrame->count(); ++i)
                    {
                        const QString itemText = _comboFrame->itemText(i);
                        const QString itemFrameName = itemText.contains(":") ? itemText.split(":").at(0) : itemText;
                        if (postDB->getFrameNumByCaseAndFrameName(guidata->_cloudCase, itemFrameName) == sessionFi)
                        {
                            hitIndex = i;
                            break;
                        }
                    }
                }
                if (hitIndex >= 0)
                {
                    const int frozenIdx = _comboFrame->currentIndex();
                    if (hitIndex != frozenIdx)
                    {
                        _comboFrame->blockSignals(true);
                        _comboFrame->setCurrentIndex(hitIndex);
                        _comboFrame->blockSignals(false);
                    }
                    _isDrawCloudMap = true;
                    drawCloudMap();	//整幅重绘顺带应用待生效参数并清脏（既有通道）
                    _comboFrame->blockSignals(true);
                    _comboFrame->setCurrentIndex(frozenIdx);
                    _comboFrame->blockSignals(false);
                }
            }
            return;
        }

        //获取帧号（根据当前帧显示的帧名称与工况名获取帧号）
        int fi = 0;
        QString fName;
        QString framestr = _comboFrame->currentText();
        if (framestr.contains(":"))
            fName = framestr.split(":").at(0);
        fi = postDB->getFrameNumByCaseAndFrameName(_comboCase->currentText(), fName);
        //获取该变量的分量
        QStringList cnames = postDB->getVariableCompos(vname);
        QString oldcompo = _comboComponent->currentText();

        _comboComponent->blockSignals(true);
        _comboComponent->clear();
        _comboComponent->blockSignals(false);
        if (cnames.size() == 0)
        {
            _comboComponent->setDisabled(true);
            _comboComplex->setDisabled(true);
            guidata->_cloudHaveComplex = false;


            //获取该变量的层
            QStringList lnames = postDB->getVariableLayer(vname);
            lnames.sort();

            QString oldlayer = _comboLayer->currentText();
            _comboLayer->blockSignals(true);
            _comboLayer->clear();
            _comboLayer->blockSignals(false);
            if (lnames.size() == 0)
            {
                _comboLayer->setDisabled(true);
                guidata->_cloudHaveLayer = false;
            }
            else
            {
                _comboLayer->setDisabled(false);
                guidata->_cloudHaveLayer = true;
                _comboLayer->blockSignals(true);
                _comboLayer->addItems(lnames);
                _comboLayer->blockSignals(false);
                guidata->_cloudLayerNames = lnames;
                if (caseInfo->layerName != "")
                {
                    _comboLayer->setCurrentText(caseInfo->layerName);
                }
                else
                {
                    QString temp{};
                    if (isContainStr(lnames, "Max", temp))
                        _comboLayer->setCurrentText(temp);
                }
            }
        }
        else
        {
            _comboComponent->setDisabled(false);
            _comboComponent->blockSignals(true);
            _comboComponent->addItems(cnames);
            _comboComponent->blockSignals(false);
            guidata->_cloudCompos = cnames;

            QString temp{};
            if (caseInfo->compoName != "")
            {
                _comboComponent->setCurrentText(caseInfo->compoName);
                //防止不触发slot_chooseCompo槽函数
                if (caseInfo->compoName == _comboComponent->itemText(0))
                {
                    slot_chooseCompo(caseInfo->compoName);
                }
            }
            else
            {
                if (isContainStr(cnames, "Mag", temp))
                {
                    _comboComponent->setCurrentText(temp);
                    bool iscomplex = postDB->isVarCompoComplex(vname, temp);
                    //设置控件显隐
                    _comboComplex->setDisabled(!iscomplex);
                    guidata->_cloudHaveComplex = iscomplex;
                    slot_chooseCompo(temp);
                }
                else if (isContainStr(cnames, "Mises", temp))
                {
                    _comboComponent->setCurrentText(temp);
                    slot_chooseCompo(temp);
                }
                else if (isContainStr(cnames, "Pressure", temp))
                {
                    _comboComponent->setCurrentText(temp);
                    bool iscomplex = postDB->isVarCompoComplex(vname, temp);
                    //设置控件显隐
                    _comboComplex->setDisabled(!iscomplex);
                    guidata->_cloudHaveComplex = iscomplex;
                    slot_chooseCompo(temp);
                }
            }

            //获取该变量的层
            QStringList lnames = postDB->getVariableLayer(vname);
            lnames.sort();

            QString oldlayer = _comboLayer->currentText();
            _comboLayer->blockSignals(true);
            _comboLayer->clear();
            _comboLayer->blockSignals(false);
            if (lnames.size() == 0)
            {
                _comboLayer->setDisabled(true);
                guidata->_cloudHaveLayer = false;
            }
            else
            {
                _comboLayer->setDisabled(false);
                guidata->_cloudHaveLayer = true;
                _comboLayer->blockSignals(true);
                _comboLayer->addItems(lnames);
                _comboLayer->blockSignals(false);
                guidata->_cloudLayerNames = lnames;
                if (caseInfo->layerName != "")
                {
                    _comboLayer->setCurrentText(caseInfo->layerName);
                }
                else
                {
                    QString temp{};
                    if (isContainStr(lnames, "Max", temp))
                        _comboLayer->setCurrentText(temp);
                }
            }
        }

        //控制大小控制的是点单元的大小而不是自由节点的大小（逻辑存疑）
        //根据当前帧是否包含自由节点以及当前变量是否为节点变量判断是否禁用自由节点操作按钮
        //bool hasFreeNode = postDB->isHaveFreeNode(fi);
        int varType = postDB->getVariableType(vname);
        if (/*hasFreeNode &&*/ varType == 0)
        {
            guidata->_cloudHaveFreeNode = true;
            MechCommon::mMechGlobalSignals::getInstance()->updateFreeNodeBtnStatus(false);
        }
        else
        {
            guidata->_cloudHaveFreeNode = false;
            MechCommon::mMechGlobalSignals::getInstance()->updateFreeNodeBtnStatus(true);
        }

        updatePlanishEnabled();
        _isDrawCloudMap = true;
        //延迟应用：变量切换待"应用渲染"生效（级联回填仍即时完成）
        markCloudDirty();
        //【FR-07② 二期（cloud-panel-frame-sync）】预生成瞬态播放中即时应用一次（与
        //SolidColor 出口同款：会话当前帧对齐→drawCloudMap 顺带应用→恢复冻结显示；
        //判据/映射/守卫口径逐字同源，帧名映射工况取 guidata->_cloudCase）
        auto applySession = postRend->getPostRender();
        if (applySession != nullptr
            && applySession->playbackState() == mxr::PlaybackState::Playing
            && applySession->displayMode() == mxr::DisplayMode::TransientAnimation)
        {
            const int sessionFi = applySession->currentFrame();
            int hitIndex = -1;
            if (sessionFi > 0)
            {
                for (int i = 0; i < _comboFrame->count(); ++i)
                {
                    const QString itemText = _comboFrame->itemText(i);
                    const QString itemFrameName = itemText.contains(":") ? itemText.split(":").at(0) : itemText;
                    if (postDB->getFrameNumByCaseAndFrameName(guidata->_cloudCase, itemFrameName) == sessionFi)
                    {
                        hitIndex = i;
                        break;
                    }
                }
            }
            if (hitIndex >= 0)
            {
                const int frozenIdx = _comboFrame->currentIndex();
                if (hitIndex != frozenIdx)
                {
                    _comboFrame->blockSignals(true);
                    _comboFrame->setCurrentIndex(hitIndex);
                    _comboFrame->blockSignals(false);
                }
                _isDrawCloudMap = true;
                drawCloudMap();	//整幅重绘顺带应用待生效参数并清脏（既有通道）
                _comboFrame->blockSignals(true);
                _comboFrame->setCurrentIndex(frozenIdx);
                _comboFrame->blockSignals(false);
            }
        }
    }

    void mPost3DResultPanel::slot_chooseCompo(const QString& cname)
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;
        // S5 D14 数据纪元：值数据将变化——已装载帧条目置待重验（绑定/重建时工厂重读新值）
        if (auto* sessionFacade = postRend->getPostRender().get())
        {
            sessionFacade->notifyDataChanged();
        }
        QString postName = postRend->getPostDataName();
        mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        if (postDB == nullptr)
            return;

        //获取GUI数据
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return;

        QString varName = _comboVariable->currentText();
        QString caseName = _comboCase->currentText();

        //找到当前分量对应的工况信息
        PostData::oneCaseInfo* caseInfo = _caseInfos.value(caseName);

        if (caseInfo == nullptr)
        {
            return;
        }
        caseInfo->compoName = _comboComponent->currentText();
        bool iscomplex = postDB->isVarCompoComplex(varName, cname);
        //设置控件显隐
        if (iscomplex)
        {
            _comboComplex->setDisabled(false);
            guidata->_cloudHaveComplex = true;
        }
        else
        {
            _comboComplex->setDisabled(true);
            guidata->_cloudHaveComplex = false;
        }

        //将复数组合框重置为初始状态（含分量切换后的历史复数恢复；
        //恢复值也在屏蔽区内设置，否则会经 slot_chooseComplex 触发一次多余重绘）
        _comboComplex->blockSignals(true);
        if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
        {
            _comboComplex->setCurrentText("实部");
        }
        else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
        {
            _comboComplex->setCurrentText("Real");
        }

        if (iscomplex)
        {
            if (caseInfo->complexName != "")
            {
                _comboComplex->setCurrentText(caseInfo->complexName);
            }
            else
            {
                caseInfo->complexName = _comboComplex->currentText();
            }
        }
        else
        {
            caseInfo->complexName = "";
        }
        _comboComplex->blockSignals(false);

        //延迟应用：分量切换待"应用渲染"生效
        markCloudDirty();
    }

    void mPost3DResultPanel::slot_chooseLayer(const QString&)
    {
        QString varName = _comboVariable->currentText();
        QString cName = _comboComponent->currentText();
        QString caseName = _comboCase->currentText();

        //找到当前分量对应的工况信息
        PostData::oneCaseInfo* caseInfo = _caseInfos.value(caseName);

        if (caseInfo == nullptr)
        {
            return;
        }
        caseInfo->layerName = _comboLayer->currentText();

        // S5 D14 数据纪元：值数据将变化——已装载帧条目置待重验（绑定/重建时工厂重读新值）
        if (REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName))
        {
            if (auto* sessionFacade = postRend->getPostRender().get())
            {
                sessionFacade->notifyDataChanged();
            }
        }

        //延迟应用：层切换待"应用渲染"生效
        markCloudDirty();
    }

    void mPost3DResultPanel::slot_chooseComplex(const QString&)
    {
        QString varName = _comboVariable->currentText();
        QString caseName = _comboCase->currentText();

        //找到当前分量对应的工况信息
        PostData::oneCaseInfo* caseInfo = _caseInfos.value(caseName);
        if (caseInfo == nullptr)
        {
            return;
        }

        caseInfo->complexName = _comboComplex->currentText();

        // S5 D14 数据纪元：值数据将变化——已装载帧条目置待重验（绑定/重建时工厂重读新值）
        if (REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName))
        {
            if (auto* sessionFacade = postRend->getPostRender().get())
            {
                sessionFacade->notifyDataChanged();
            }
        }

        //延迟应用：复数切换待"应用渲染"生效
        markCloudDirty();
    }

    void mPost3DResultPanel::slot_chooseRangeType(int index)
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;

        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return;
        guidata->_rangeIndex = index;

        //上下限编辑/复位启停只随图例范围档（仅手动可改）；模式档不再覆盖启停
        //（模式只决定非手动档的值域计算口径：所有部件/显示部件——2026-11-13 收口，
        //原"所有部件档强制启用"会盖掉图例范围切换的禁用）
        refreshLegendBoundEditState();

        //延迟应用：值域解析/着色在"应用渲染"时由 slot_showCloud 按当前模式统一执行
        //（:4826 起按模式走 resolveFullRange/可视部件值域并同步编辑框/guidata/色表）
        _rangeEditPending = true;
        markCloudDirty();
    }

    void mPost3DResultPanel::slot_setPositionVisible(bool isshow)
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;

        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        //存储界面信息
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return;

        //获取信号源（V1.3 图例边界行的显隐钮；两行各接本槽）
        MxFunctions::mLegendBoundRow* row = qobject_cast<MxFunctions::mLegendBoundRow*>(sender());
        if (row == _legendMax) {
            // 最值标注收编（unify-addobject-arch）：单侧掩码切换（首建=全量缺省隐藏=现状初始态）
            mxr::MinMaxLabelParams _mm;
            _mm.updateMask = mxr::MinMaxLabelParams::Mask_Max;
            _mm.maxVisible = isshow;
            PostCommon::ensureUpdateSceneObject(postRender.get(), _mm);
            guidata->_cloudShowMaxLine = isshow;
        }
        else if (row == _legendMin) {
            mxr::MinMaxLabelParams _mm;
            _mm.updateMask = mxr::MinMaxLabelParams::Mask_Min;
            _mm.minVisible = isshow;
            PostCommon::ensureUpdateSceneObject(postRender.get(), _mm);
            guidata->_cloudShowMinLine = isshow;
        }
        postRend->updatePaint();
        mMechGlobalSignals::getInstance()->updateResultMenuGUI();
    }

    void mPost3DResultPanel::slot_resetMostValue()
    {
        //获取信号发射者（V1.3 图例边界行的复位钮；两行各接本槽）
        MxFunctions::mLegendBoundRow* row = qobject_cast<MxFunctions::mLegendBoundRow*>(sender());
        if (row == nullptr)
            return;
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr)
            return;

        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;
        if (postRender->frameProxy(postRender->currentFrame()) == nullptr) return;
        mxr::IPostRendQueryDataProxy* query_proxy = dynamic_cast<mxr::IPostRendQueryDataProxy*>(postRender->frameProxy(postRender->currentFrame())->getRendQueryDataProxy());

        float maxval = query_proxy->getOriginalMaxData();
        float minval = query_proxy->getOriginalMinData();
        //值域非法（倒挂/非有限）＝数据未就绪：复位无对象，静默返回不弹打扰
        if (!(maxval >= minval) || !std::isfinite(maxval) || !std::isfinite(minval))
            return;

        //延迟应用：复位只把当前帧原始值域写回编辑框，提交与着色在"应用渲染"时进行
        QLineEdit* edit = (row == _legendMax) ? _legendMax->lineEdit() : _legendMin->lineEdit();
        const float value = (row == _legendMax) ? maxval : minval;
        edit->blockSignals(true);
        edit->setText(MxFunctions::mNumberToString(value, 'f', 12));
        edit->blockSignals(false);
        _rangeEditPending = true;
        markCloudDirty();
    }

    void mPost3DResultPanel::slot_editValueChanged()
    {
        //延迟应用：上下限在"应用渲染"时经 slot_setMostValue 统一提交（手动档锁定值域由此写入 guidata）
        _rangeEditPending = true;
        markCloudDirty();
    }

    void mPost3DResultPanel::slot_discreteCombox(const QString& flag)
    {
        REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderByViewName(_curViewName);
        if (rebuildPostRend == nullptr) return;

        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(rebuildPostRend);
        if (guidata != nullptr)
        {
            if (flag != "离散")
                guidata->_cloudDiscrete = 0;
            else
                guidata->_cloudDiscrete = 1;
        }
        //延迟应用：离散开关在"应用渲染"时经 applyDiscreteSettings 统一下发
        _discreteEditPending = true;
        markCloudDirty();
    }

    void mPost3DResultPanel::slot_discreteOrder(int order)
    {
        REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderByViewName(_curViewName);
        if (rebuildPostRend == nullptr) return;

        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(rebuildPostRend);
        if (guidata != nullptr)
            guidata->_cloudDiscreteNum = order;
        //延迟应用：离散阶数在"应用渲染"时经 applyDiscreteSettings 统一下发
        _discreteEditPending = true;
        markCloudDirty();
    }

    void mPost3DResultPanel::slot_discreteTypeComboBox(const QString& text)
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;

        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata != nullptr)
        {
            if (text == QString("等比") || text == QString("Equal ratio"))
                guidata->_cloudIsEquivariance = 1;
            else
                guidata->_cloudIsEquivariance = 0;
        }
        //延迟应用：等差/等比在"应用渲染"时经 applyDiscreteSettings 统一下发
        _discreteEditPending = true;
        markCloudDirty();
    }

    void mPost3DResultPanel::slot_showOrHideColorBar()
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;

        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        guidata->_isColorTableShow = !guidata->_isColorTableShow;
        if (guidata->_isColorTableShow)
        {
            _btnShow->setText("隐藏");
            _btnCustom->setDisabled(false);
            _btnReset->setDisabled(false);

            postRender->getColorTableWindow()->setVisiable(true);
            postRend->updatePaint();
        }
        else
        {
            _btnShow->setText("显示");
            _btnCustom->setDisabled(true);
            _btnReset->setDisabled(true);
            postRender->getColorTableWindow()->setVisiable(false);
            postRend->updatePaint();
        }
    }

    void mPost3DResultPanel::slot_customColorBar()
    {
        //自定义=弹出渲染侧 Edit Legend——与视口内双击图例为同一窗口（PostViewportWidget
        //双击路径同源）：经当前视口 InteractionSystem 的共享编辑器实例，配置直接作用于
        //该视口色带。旧 mCustomColorBar 弹窗停用（成员保留未建）。
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;

        mxr::ZsEngine_NephogramWindow* colorTable = postRend->getColorTableRender();
        //空白视口（未显示云图，色带无内容）不弹窗——守卫条件同双击路径
        if (colorTable == nullptr || colorTable->GetColorCount() <= 0) return;

        MechPreData::PostViewportWidget* postWidget = getCVM->getPostWidget();
        if (postWidget == nullptr) return;
        mxr::CLegendEditorWidget* editor = postWidget->interaction()->getLegendEditor();
        if (editor == nullptr) return;

        editor->setColorTable(colorTable);
        editor->show();
        editor->raise();
        editor->activateWindow();
    }

    void mPost3DResultPanel::slot_resetColorBar()
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;
        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        postRender->getColorTableWindow()->resetPos();
        postRend->updatePaint();
    }

    void mPost3DResultPanel::slot_toggleLightPopup(bool istrue)
    {
        //弹窗被外部销毁（ui-shot 用后即焚）后按需重建
        if (_lightWidget == nullptr && istrue)
            initLightWidget(this);
        if (_lightWidget == nullptr)
            return;
        //入口钮 toggled 驱动（V1.3：面板底部整行入口；弹窗 Qt::Popup 外点/Esc 自动关闭，
        //关闭时经 hide 联动归位入口 checked 态）
        if (!istrue)
        {
            if (_lightWidget->isVisible())
                _lightWidget->close();   //Qt::Popup close 即隐藏
            if (_btnLight != nullptr && _btnLight->isChecked())
            {
                _btnLight->blockSignals(true);
                _btnLight->setChecked(false);
                _btnLight->blockSignals(false);
            }
            return;
        }
        if (_lightWidget->isVisible())
            return;
        //打开前从渲染读回当前光照状态，保证界面显示与渲染一致
        syncLightWidgetFromRender();
        _lightWidget->adjustSize();   //首次 show 前 height() 是 0，必须先算尺寸再定位
        //锚定入口钮右侧、与入口顶对齐（原型 placeAnchoredPopup 口径；右侧越界翻左侧）
        QPoint entryTopRight = _btnLight->mapToGlobal(_btnLight->rect().topRight());
        QPoint pos(entryTopRight.x() + 8, entryTopRight.y());
        QScreen* scr = QGuiApplication::screenAt(pos);
        const QRect avail = (scr ? scr : QGuiApplication::primaryScreen())->availableGeometry();
        if (pos.x() + _lightWidget->width() > avail.right())
            pos.setX(entryTopRight.x() - _btnLight->width() - 8 - _lightWidget->width());
        pos.setX(qBound(avail.left() + 4, pos.x(), qMax(avail.left() + 4, avail.right() - _lightWidget->width() - 4)));
        pos.setY(qBound(avail.top(), pos.y(), qMax(avail.top(), avail.bottom() - _lightWidget->height())));
        _lightWidget->move(pos);
        _lightWidget->show();
        _lightWidget->raise();
        //Qt::Popup 隐藏（外点/Esc/关闭钮）→ 经 eventFilter（Hide）归位入口 checked 态
        _lightWidget->installEventFilter(this);
    }

    void mPost3DResultPanel::syncLightWidgetFromRender()
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return;

        //blockSignals：回填控件不得触发 slot 反向写渲染
        _comboLightPos->blockSignals(true);
        _sliderAmbient->blockSignals(true);
        _sliderDiffuse->blockSignals(true);
        _sliderMirror->blockSignals(true);

        // S5 移交项（引擎缺通道，用户在途光源联动特性）：渲染侧新旧门面均无
        // getLightIsDependentCamera/getLightPosition/getLightParameters 只读通道——
        // 读回段挂起待渲染批次补通道；面板暂以中性缺省填充（follow=false→固定档）
        const bool follow = false;   // 移交项：postRend->getLightIsDependentCamera();
        _comboLightPos->setCurrentIndex(follow ? 0 : 1);
        const bool enabled = !follow;
        _lineEditX->setEnabled(enabled);
        _lineEditY->setEnabled(enabled);
        _lineEditZ->setEnabled(enabled);

        const QVector3D pos;   // 移交项：postRend->getLightPosition();
        _lineEditX->setText(QString::number(pos.x()));
        _lineEditY->setText(QString::number(pos.y()));
        _lineEditZ->setText(QString::number(pos.z()));

        //渲染浮点值 → 滑条档位（与 slot_setLightParameters 的 ÷10 换算互逆；V1.3 三滑杆）
        QVector3D ambient, diffuse, specular;
        // 移交项：postRend->getLightParameters(ambient, diffuse, specular, shiness);
        _sliderAmbient->setValue(qBound(0, qRound(ambient.x() * 10.0f), 10));
        _sliderDiffuse->setValue(qBound(0, qRound(diffuse.x() * 10.0f), 10));
        _sliderMirror->setValue(qBound(0, qRound(specular.x() * 10.0f), 10));

        _comboLightPos->blockSignals(false);
        _sliderAmbient->blockSignals(false);
        _sliderDiffuse->blockSignals(false);
        _sliderMirror->blockSignals(false);

        //guidata 同步为渲染真值，保持视口切换回填路径与本弹窗一致
        guidata->_lightType = follow ? 1 : 2;
        guidata->_lightPos = QStringLiteral("%1,%2,%3").arg(
            QString::number(pos.x()), QString::number(pos.y()), QString::number(pos.z()));
        guidata->_lightAmbient = _sliderAmbient->value();
        guidata->_lightDiffuse = _sliderDiffuse->value();
        guidata->_lightMirror = _sliderMirror->value();
        //（_lightIntensity 无 UI 入口后不再回写，存量值继续作为 shiness 下发）
    }

    void mPost3DResultPanel::slot_setLightIsDependentCamera(const QString& istrue)
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;
        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return;

        if (istrue == "跟随")
            guidata->_lightType = 1;
        else
            guidata->_lightType = 2;

        postRend->setLightIsDependentCamera(istrue == "跟随");
        postRend->updatePaint();

        //V1.3：位置选项 跟随/固定——固定（原"自定义"）才启用 X/Y/Z
        const bool enabled = (istrue == "固定");
        _lineEditX->setEnabled(enabled);
        _lineEditY->setEnabled(enabled);
        _lineEditZ->setEnabled(enabled);
    }

    void mPost3DResultPanel::slot_setLightPosition()
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return;

        //获取球心和半径
        const float x = _lineEditX->text().simplified().toFloat();
        const float y = _lineEditY->text().simplified().toFloat();
        const float z = _lineEditZ->text().simplified().toFloat();

        QVector3D position(0, 0, 0);
        position.setX(x);
        position.setY(y);
        position.setZ(z);
        //必须走外层 REBUILD_PostRend 接口：该值经 myPaintEventHandle 每帧
        //下发 u_light.position（内层 REBUILD_PostRender 的同名字段无人消费）
        postRend->setLightPosition(position);
        postRend->updatePaint();

        guidata->_lightPos = QStringLiteral("%1,%2,%3").arg(
            QString::number(x), QString::number(y), QString::number(z));
    }

    void mPost3DResultPanel::slot_setLightParameters(int value)
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;
        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return;

        QVector3D ambient(0, 0, 0), diffuse(0, 0, 0), specular(0, 0, 0);

        int iamb = _sliderAmbient->value();
        int idif = _sliderDiffuse->value();
        int ispe = _sliderMirror->value();
        //V1.3 删强度滑杆：shiness 用 guidata 存量值（无存量时回落 32 旧缺省），渲染行为不变
        float shiness = guidata->_lightIntensity > 0 ? guidata->_lightIntensity : 32.f;
        guidata->_lightAmbient = iamb;
        guidata->_lightDiffuse = idif;
        guidata->_lightMirror = ispe;

        float famb = iamb / 10.0;
        ambient.setX(famb);
        ambient.setY(famb);
        ambient.setZ(famb);
        float fdif = idif / 10.0;
        diffuse.setX(fdif);
        diffuse.setY(fdif);
        diffuse.setZ(fdif);
        float fspe = ispe / 10.0;
        specular.setX(fspe);
        specular.setY(fspe);
        specular.setZ(fspe);

        postRend->setLightParameters(ambient, diffuse, specular, shiness);
        postRend->updatePaint();

    }


    // FR-08 渐进装载兜底统一入口（补绘挂起集合版，2026-11-16）：原"单连接+后到拆线"
    // 会被下一帧派发拆掉尚未到达的补绘（渐进装载期装载收敛>派发间隔时整遍停在起始
    // 帧），且补绘重入 drawCloudMap 读到冻结下拉值（补的永远是旧帧）。挂起帧集合+
    // 常驻连接：frameReady 命中集合才消费，补绘时临时把下拉指向挂起帧（blockSignals+
    // 恢复冻结显示，FR-08 手动派发 P2-1 同款回写），多帧挂起互不拆线；arm 即重连自愈
    // 换 sender/换视口。slot_showCloud 兜底/路径B 兜底/实时播放快路径三处共用。
    void mPost3DResultPanel::armFrameLoadFallback(int fi, mxr::REBUILD_PostRender* postRender)
    {
        _frameReadyPending.insert(fi);
        if (_frameReadyRedrawConn)
            disconnect(_frameReadyRedrawConn);   // arm 即重连（自愈换 sender/换视口；挂起非空期间补绘链常驻）
        _frameReadyRedrawConn = connect(postRender, &mxr::REBUILD_PostRender::frameReady,
            this, [this](int readyFrame) {
                if (!_frameReadyPending.contains(readyFrame)) return;
                _frameReadyPending.remove(readyFrame);
                // S6：挂起清空=装载收敛完毕，同步解除两条常驻连接
                if (_frameReadyPending.isEmpty())
                {
                    if (_frameReadyRedrawConn)
                    {
                        disconnect(_frameReadyRedrawConn);
                        _frameReadyRedrawConn = {};
                    }
                    if (_frameLoadErrorConn)
                    {
                        disconnect(_frameLoadErrorConn);
                        _frameLoadErrorConn = {};
                    }
                }
                //P6-1 审计修复（兜底重入抑制·第二道）：录制走查独占期吞掉兜底补绘重入——
                //后台装载完成落在录制两步走查之间时不得重绑（成片错帧）；装载本身
                //已完成（帧表常驻），录制尾还原与后续 GUI 切帧正常命中
                if (mPostInfoHub::getInstance()->isRecordingActive())
                    return;
                // 补绘指向挂起帧：slot_showCloud 按 currentText 解析帧号，冻结显示语义
                // 下下拉常态停在旧帧，不临时指向则补的永远是旧帧（渐进装载永不上屏）
                REBUILD_PostRend* pendRend = getCVM->getPostRenderByViewName(_curViewName);
                QPointer<mPostDataBase> pendDb = pendRend != nullptr
                    ? getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pendRend->getPostDataName())
                    : nullptr;
                int pendIndex = -1;
                if (pendDb != nullptr)
                {
                    const QString pendCase = _comboCase->currentText();
                    for (int i = 0; i < _comboFrame->count(); ++i)
                    {
                        const QString pendText = _comboFrame->itemText(i);
                        const QString pendName = pendText.contains(":") ? pendText.split(":").at(0) : pendText;
                        if (pendDb->getFrameNumByCaseAndFrameName(pendCase, pendName) == readyFrame)
                        {
                            pendIndex = i;
                            break;
                        }
                    }
                }
                if (pendIndex < 0)
                    return;   // 下拉无此帧（工况命名差异）：帧已装载常驻，后续派发链可正常命中
                const int frozenIdx = _comboFrame->currentIndex();
                _comboFrame->blockSignals(true);
                _comboFrame->setCurrentIndex(pendIndex);
                drawCloudMap();   // 装载完成→补绘云图（同会话同视口，指向挂起帧）
                _comboFrame->setCurrentIndex(frozenIdx);
                _comboFrame->blockSignals(false);
            });
        // S6 帧装载回归诊断（2026-11-09）：渲染侧切片装载/工厂重读失败经 loadError
        // 上报（门面转发；帧表工厂失败/buildEntry 失败），此前 GUI 零订阅——失败帧
        // 永无 frameReady，用户死等"装载中"无任何原因透出
        if (_frameLoadErrorConn)
            disconnect(_frameLoadErrorConn);
        _frameLoadErrorConn = connect(postRender, &mxr::REBUILD_PostRender::loadError,
            this, [this](int errFrame, const QString& cause) {
                if (!_frameReadyPending.contains(errFrame)) return;
                _frameReadyPending.remove(errFrame);
                if (_frameReadyPending.isEmpty())
                {
                    if (_frameLoadErrorConn)
                    {
                        disconnect(_frameLoadErrorConn);
                        _frameLoadErrorConn = {};
                    }
                    if (_frameReadyRedrawConn)
                    {
                        disconnect(_frameReadyRedrawConn);
                        _frameReadyRedrawConn = {};
                    }
                }
                mGlobalSignals::getInstance()->outputMessageSig(2,
                    QString("帧 %1 装载失败：%2（切帧重试或重新导入）").arg(errFrame).arg(cause), true);
            });
        postRender->requestFrames(fi, fi);
    }

    // 播放推进轻量镜像（引擎合一 2026-11-16，签名见头文件注释）：全部代理现取零重绘，
    // 手动/全程/逐帧三值域模式口径同动画面板 slot_setCurrentFrame 的 Animation 分支
    void mPost3DResultPanel::mirrorFrameValueDisplay(REBUILD_PostRend* postRend, mxr::REBUILD_PostRender* postRender,
        MechPreData::mPostGUIData* frameGui, const QPointer<PostData::mPostDataBase>& frameDb, int fi)
    {
        mxr::PostFrameDataProxy* tickProxy = postRender->frameProxy(fi);
        mxr::IPostRendQueryDataProxy* query_proxy = tickProxy != nullptr ? dynamic_cast<mxr::IPostRendQueryDataProxy*>(tickProxy->getRendQueryDataProxy()) : nullptr;
        mxr::IPostModelRenderDataProxy* model_query_proxy = tickProxy != nullptr ? dynamic_cast<mxr::IPostModelRenderDataProxy*>(tickProxy->getModelRenderDataProxy()) : nullptr;
        if (query_proxy == nullptr || model_query_proxy == nullptr || frameGui == nullptr)
            return;
        const mxr::PostNodeOrElement nodeOrElement = query_proxy->getNodeOrElement();
        const std::pair<int, float> maxid = model_query_proxy->getOneTotalMaxID();
        const std::pair<int, float> minid = model_query_proxy->getOneTotalMinID();
        if (nodeOrElement == mxr::PostNodeOrElement::PostElement)
        {
            postRender->getColorTableWindow()->setMaxFootLabel(0, "max: %E", query_proxy->getCurrentMaxData());
            postRender->getColorTableWindow()->setMaxFootLabel(1, "at Elem# %d", maxid.first);
            postRender->getColorTableWindow()->setMinFootLabel(2, "min: %E", query_proxy->getCurrentMinData());
            postRender->getColorTableWindow()->setMinFootLabel(3, "at Elem# %d", minid.first);
        }
        else
        {
            postRender->getColorTableWindow()->setMaxFootLabel(0, "max: %E", query_proxy->getCurrentMaxData());
            postRender->getColorTableWindow()->setMaxFootLabel(1, "at Node# %d", maxid.first);
            postRender->getColorTableWindow()->setMinFootLabel(2, "min: %E", query_proxy->getCurrentMinData());
            postRender->getColorTableWindow()->setMinFootLabel(3, "at Node# %d", minid.first);
        }
        //图例范围三模式（HyperView Fixed/Dynamic Scale 对应）：手动=guidata 锁定值域、
        //全程=全帧统一值域、逐帧=当前帧实际最值
        QPair<double, double> allRange{};
        bool allRangeValid = false;
        if (frameGui->_legendRange == QString("手动"))
        {
            if (frameGui->_cloudMaxValue > frameGui->_cloudMinValue)
            {
                allRange = qMakePair(double(frameGui->_cloudMinValue), double(frameGui->_cloudMaxValue));
                allRangeValid = true;
            }
        }
        else if (frameGui->_legendRange == QString("全程"))
        {
            if (auto cloudPanel = mPostInfoHub::getInstance()->cloudResultPanel())
                allRangeValid = cloudPanel->allFrameRange(postRend->getPostDataName(), frameDb.data(),
                    frameGui->_cloudVarName, frameGui->_cloudCompo,
                    frameGui->_cloudLayerName, frameGui->_cloudComplex, allRange);
        }
        if (allRangeValid)
        {
            // 色表值域收编（C4 仅窗单效：本函数无伴生 setMinMaxData，现状只写窗）
            mxr::ColorLegendParams _lg;
            _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
            _lg.targetFrame = fi;
            _lg.valueMin = float(allRange.first);
            _lg.valueMax = float(allRange.second);
            PostCommon::ensureUpdateSceneObject(postRender, _lg);   // 本函数形参=裸指针
        }
        else
        {
            mxr::ColorLegendParams _lg;
            _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
            _lg.targetFrame = fi;
            _lg.valueMin = query_proxy->getCurrentMinData();
            _lg.valueMax = query_proxy->getCurrentMaxData();
            PostCommon::ensureUpdateSceneObject(postRender, _lg);   // 本函数形参=裸指针
        }
        //切帧后保持云图设置的点大小（帧实例已就绪方可调用）
        if (postRender->frameProxy(postRender->currentFrame()) != nullptr)
            postRender->setPointSize(frameGui->_pointSize);
        postRend->updatePaint();
    }

    void mPost3DResultPanel::slot_showCloud()
    {
        REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderByViewName(_curViewName);
        if (rebuildPostRend == nullptr)
        {
            return;
        }

        auto  postRender = rebuildPostRend->getPostRender();
        if (postRender == nullptr) return;

        QString postName = rebuildPostRend->getPostDataName();
        QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        if (modeldb == nullptr)
            return;

        //获取GUI数据
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(rebuildPostRend);
        if (guidata == nullptr) return;

        //存储工况
        guidata->_cloudCase = _comboCase->currentText();

        //存储帧名称
        guidata->_cloudFrameIndex = _comboFrame->currentIndex();

        //获取帧
        int fi = 1;
        //获取帧号（根据当前帧显示的帧名称与工况名获取帧号）
        QString fName;
        QString framestr = _comboFrame->currentText();
        if (framestr.contains(":"))
            fName = framestr.split(":").at(0);
        fi = modeldb->getFrameNumByCaseAndFrameName(guidata->_cloudCase, fName);
        if (fi <= 0)
        {
            // 如果当前面板保留的是旧模型的 case/frame，尝试回到新模型默认第一帧
            QStringList cases = modeldb->getAllCaseNames();
            if (!cases.isEmpty())
                guidata->_cloudCase = cases.first();
            QStringList frameNames = modeldb->getCaseFrameNames(guidata->_cloudCase);
            if (!frameNames.isEmpty())
            {
                framestr = frameNames.first();
                if (framestr.contains(":"))
                    fName = framestr.split(":").at(0);
                else
                    fName = framestr;
                fi = modeldb->getFrameNumByCaseAndFrameName(guidata->_cloudCase, fName);
            }
        }
        if (fi <= 0)
        {
            return;
        }

        bool isNew = false;
        //捕获上一帧号（用于失效单元增量显隐对比），在覆盖 _cloudFrame 之前取值
        int lastFi = guidata->_cloudFrame;
        if (fi != guidata->_cloudFrame)
            isNew = true;
        guidata->_cloudFrame = fi;
        //获取变量
        QString varstr = _comboVariable->currentText();
        guidata->_cloudVarName = varstr;
        //伪变量SolidColor：不读取结果数据，按部件自身颜色显示
        const bool isSolidColor = (varstr == QString("SolidColor"));

        //获取分量
        QString compostr = _comboComponent->currentText();
        guidata->_cloudCompo = compostr;
        //获取层
        QString layerstr = _comboLayer->currentText();
        guidata->_cloudLayerName = layerstr;
        //获取复数
        bool isdisp = false;
        QString complexstr;
        if (_comboComplex->isEnabled())
        {
            complexstr = _comboComplex->currentText();

            if (guidata->_cloudComplex != complexstr)
                isdisp = true;
        }
        guidata->_cloudComplex = complexstr;
        //根据是否切换帧获取数据

        QString coordinateName = QString("Global");
        QVector<double> userCoord{};

        //显示
        QString postPathName = modeldb->getFile();

        bool needPlanish = _comboSmoothMethod->currentIndex() >= 1 && _comboSmoothMethod->isEnabled();
        guidata->_cloudPlanish = needPlanish;

        // S6 读数缓存命中门（段c，真机重复读 846~936ms/次）：结果数据导入后不可变，
        // 同一（帧，分支|变量|分量|复数|层|磨平）重复切换直接复用 _resultDatas 既有表，
        // 跳过文件重读。命中须过三重门，任一不过照旧走读数分支：
        // ① GUI 读取身份一致（按 postPath|帧 登记，磨平/复数/层/分支未入库名，由身份兜住）；
        // ② 库侧名字登记一致（镜像 addResultValue 登记格式；标量运算 setScalarOperationData
        //    失效名字 mPostDataBase.cpp:778、无名两参 distributeFrameIdAndResult :1661、
        //    电机 partNode 清表 :1961-1964——库侧写路径结构性防假命中）；
        // ③ 类型已登记且数据表非空（新导入/电机 _resultNodeDatas 路径自然失配）。
        // 读数失败路径早退不登记（登记点在四分支汇入后），首读照旧
        const bool _readCacheEligible = !isSolidColor;
        const bool _readBranchNative = _readCacheEligible && isLazyNativeResult(postPathName);
        const bool _readBranchVtk = _readCacheEligible && !_readBranchNative && isLazyVtkResult(postPathName);
        const bool _readBranchMxdbOld = _readCacheEligible && !_readBranchNative && !_readBranchVtk && modeldb->getDataFlag();
        const QString _readCacheKey = postPathName + QStringLiteral("|") + QString::number(fi);
        const QString _readIdentity = QStringLiteral("%1|%2|%3|%4|%5|%6")
            .arg(_readBranchNative ? QStringLiteral("native")
                 : _readBranchVtk ? QStringLiteral("vtk")
                 : _readBranchMxdbOld ? QStringLiteral("mxdbold")
                 : QStringLiteral("thread"))
            .arg(varstr, compostr, complexstr, layerstr)
            .arg(needPlanish ? 1 : 0);
        bool readCacheHit = false;
        if (_readCacheEligible
            && _cloudReadIdentityByKey.value(_readCacheKey) == _readIdentity
            && modeldb->getFrameResultType(fi) >= 0
            && !modeldb->getResultDataByFrameID(fi).isEmpty()
            && modeldb->getResultVarNameByFrameID(fi) == expectedResultVarName(varstr, compostr))
        {
            readCacheHit = true;
        }

        //派生工况帧（批次A1）：值已随生成物化在内存 _resultDatas，跳过全部文件读分支
        //（派生帧号在结果文件帧表之外，文件读必错）；变量守卫——派生帧仅生成变量有
        //数据，切到其它变量时提示并保持当前画面
        const bool derivedFrame = modeldb->isDerivedFrame(fi);
        if (derivedFrame && !isSolidColor
            && modeldb->getResultVarNameByFrameID(fi) != expectedResultVarName(varstr, compostr))
        {
            const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
            mGlobalSignals::getInstance()->outputMessageSig(2,
                isChinese ? QStringLiteral("派生工况仅包含生成变量 %1 的数据，无法切换到 %2。")
                                    .arg(modeldb->getResultVarNameByFrameID(fi))
                                    .arg(expectedResultVarName(varstr, compostr))
                          : QStringLiteral("The derived case only contains data of the generated variable %1; cannot switch to %2.")
                                    .arg(modeldb->getResultVarNameByFrameID(fi))
                                    .arg(expectedResultVarName(varstr, compostr)),
                true);
            return;
        }

        if (!isSolidColor && !readCacheHit && !derivedFrame)
        {
            //统一取值 provider（阶段2 二批）：四格式发布式分支收敛（面板级缓存/派生帧门在外）；
            //本站点四格式失败一律中止重绘（原 native/vtk/mxdb 各分支一致——失败路径早退
            //不进读数身份登记）；磨平走 needPlanish、ElementNodalForce 特判在适配器内，
            //老格式 reader 经面板池复用（原临时实例，池化后免逐次重开文件）
            IPostValueProvider* provider = valueProviderFor(modeldb);
            ValueRequest req;
            req.frame = fi;
            req.var = varstr;
            req.compo = compostr;
            req.layer = layerstr;
            req.complexMode = complexstr;
            req.coordinateName = coordinateName;
            req.userCoord = userCoord;
            req.flattenStress = needPlanish;
            const bool readOk = provider != nullptr && provider->publish(req);
            const bool isNativeOrVtk = provider != nullptr
                && (provider->sourceKind() == IPostValueProvider::SourceKind::Native
                    || provider->sourceKind() == IPostValueProvider::SourceKind::Vtk);
            delete provider;
            if (!readOk)
            {
                if (isNativeOrVtk)
                {
                    // 静默返回会让"切变量无图例无颜色变化"无从排查，这里落到输出窗口
                    const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
                    mGlobalSignals::getInstance()->outputMessageSig(2,
                        isChinese ? QStringLiteral("读取结果分量失败：帧%1，变量%2，分量%3（%4），详见调试日志")
                                        .arg(fi).arg(varstr).arg(compostr).arg(postPathName)
                                  : QStringLiteral("Failed to read result component: frame %1, variable %2, component %3 (%4); see the debug log for details")
                                        .arg(fi).arg(varstr).arg(compostr).arg(postPathName),
                        true);
                }
                else
                {
                    report3DReadFailure(fi, varstr, compostr, postPathName);
                }
                return;
            }
        }

        // S6 读数身份登记（段c）：四分支任一汇入即读数成功（失败路径已早退不登记），
        // 登记供下次同（帧，身份）切换命中
        if (!isSolidColor && !readCacheHit)
            _cloudReadIdentityByKey[_readCacheKey] = _readIdentity;

        //处理失效单元：对比上/当前帧失效集合，增量设置单元显隐；返回值标识是否有失效显隐变化
        bool hasFailedChange = false;
        hasFailedChange = processFailedElements(modeldb, rebuildPostRend, fi, lastFi);

        //变形变量非默认时按其矢量场覆盖位移哈希（须在建帧代理前：代理创建时物化位移数据）
        applyDeformVariableField(rebuildPostRend, guidata);

        // S5 门面直接替换：三连调残余（createFrameDataProxy/setCurFrameDataProxy/
        // setRendCurrentFrameData）收敛 attachModel——装载工厂随绑定注入，GUI 不再
        // 手造代理入壳
        // S6 归属优化（段a）：本地探针代理整段省略——createFrameDataProxy 每次全量
        // 构造 FrameDataProxy（4 子代理；PostModelRenderDataProxy ctor 即 buildIndex/
        // updateAABB/calcModelAABB/calculateEdgeLine，REBUILD_DataProxy.cpp:37-50），
        // 真机 2.3~2.5s/次，且构造产物仅余"帧可读性前置守卫"一职（下游统一现取
        // hosted=frameProxy(fi)，D9 现取契约），探针本体从未析构=每次全量代理泄漏。
        // 工厂空判条件（REBUILD_FrameDataProxy.cpp:88 !db||!getModelDataByFrameID）
        // 在 GUI 侧以同库同帧查询等价替代（同表同判，语义等价）
        const bool frameReadable = (modeldb->getModelData(fi) != nullptr);
        if (!frameReadable)
        {
            // S6 帧装载回归诊断（2026-11-09）：此处原为静默 return——帧模型数据未在
            // 导入期进入 modeldb（_modelDatabase 无该帧）时用户只见空白无任何解释。
            // 帧装载是导入期一次性完成的（appendModelData 全帧注册，见
            // MxdbDataBuilder.cpp:212/:722），切帧链不做按需装载；此处为 null 即导入丢帧
            mGlobalSignals::getInstance()->outputMessageSig(2,
                QString("帧 %1 模型数据未装载（导入期读取失败或被跳过），请重新导入并查看调试日志").arg(fi), true);
            return;
        }
        postRender->attachModel(modeldb.data(), rebuildPostRend->getViewportName());
        // S5 D14 数据纪元：变量/分量/工况切换处理已 notifyDataChanged()——绑定/重建时
        // 惰性重读新值（revalidateIfStale），本函数无需感知变量切换
        // S5 首绑/同帧重绘必须显式绑定：emit=true 绕过同帧去重——异步装载完成后的重入
        // seek(fi) 会命中"同帧&&就绪→零动作"分支跳过 bind，实例保持隐藏=模型不显示；
        // 同帧变量切换也依赖此路径触发纪元重验（bind→revalidateIfStale 工厂重读新值）
        const bool seekOk = postRender->seekFrame(fi, mxr::FrameChangeSource::UserSeek, true);
        QPointer<PostData::mPostModelData> modelData = modeldb->getModelData(fi);
        if (modelData == nullptr)
        {
            return;
        }

        // S5 F8 契约：托管代理一次性现取（换帧/重装载后须重取）；未装载帧兜底（Q10：
        // 保持上一帧画面+提示，不崩）。渐进装载为后台分片——frameReady(fi) 单发重入
        // drawCloudMap 完成补绘（防多次守护返回叠加连接）
        // 渲染侧契约（bind 未就绪路由，见渲染线 D16 修复）：seekFrame false 语义=目标帧未绑定
        // （未登记/未就绪/纪元陈旧）；收敛唯一通道=frameReady 单发重入；渲染侧承诺 tick 重建完成后必发 frameReady。
        // 兜底门=seekFrame 返回值（frameProxy(fi) 对纪元陈旧/ProxyOnly 恒非空，不能作门）；
        // hosted 以目标帧 fi 键现取供下游 query_proxy/model_proxy 消费（勿回改 currentFrame()）
        // 渲染侧已修复 tick 纪元跳过缺口（2026-11-05 install），异步链为唯一收敛通道；
        // 勿 reintroduce 同步重建捷径（D16 同族内联重建风险）
        mxr::PostFrameDataProxy* hosted = postRender->frameProxy(fi);
        if (!seekOk || hosted == nullptr)
        {
            // 渐进装载兜底：挂起集合+常驻连接补绘统一入口（FR-08，2026-11-16 收敛至
            // armFrameLoadFallback——frameReady 命中挂起帧才补绘，临时指向挂起帧再恢复冻结显示）
            armFrameLoadFallback(fi, postRender.get());
            return;
        }
        mxr::IPostRendQueryDataProxy* query_proxy = dynamic_cast<mxr::IPostRendQueryDataProxy*>(hosted->getRendQueryDataProxy());
        if (query_proxy != nullptr)
        {
            for (auto it : modeldb->getAllRigidWallDatas())
            {
                QString name = it->getRigidWallName();
                QVector3D endPos = modelData->_frameInfo.getRigidWallEndPos(name);
                it->updatePos(endPos);
                query_proxy->appendRigidWallData(it);
            }
        }
        // B1b(R18-2①) 刚体墙替代重放（R22-H1 恢复）：append 数据错过实例构建期
        // ImportOperate，改走 GUI 可达替代入口 ReplaceOnePartOperate 逐墙重建
        // partRender（幂等覆盖；渲染库修复落地后改回单发
        // UpdateAllRigidWallOperate(ImportOperate)，移交依赖不变）
        {
            std::set<QString> appendedWallNames;
            for (auto it : modeldb->getAllRigidWallDatas())
                appendedWallNames.insert(it->getRigidWallName());
            if (!appendedWallNames.empty())
                // S5 G5（D9 载体）：RigidWall=登记型——寻/登台账 id 后 updateObject
                // ReplaceOnePartOperate 逐墙重建（query_proxy 现读已含 append 数据）
                PostCommon::updateRigidWallOperate(postRender.get(), (int)mxr::ReplaceOnePartOperate,
                    appendedWallNames);
        }

        //显示自由节点
        if (guidata->_cloudShowFreeNode && !guidata->_cloudDeleteFreeNode)
        {
            modeldb->showFreeNode(guidata->_cloudFrame, guidata->_cloudRelatedCloud);
            postRender->updateAllModelOperate(ShowFreeNode);
        }
        else if (!guidata->_cloudShowFreeNode)
        {
            modeldb->hideFreeNode(guidata->_cloudFrame);
            postRender->updateAllModelOperate(ClearFreeNode);
        }
        else if (guidata->_cloudDeleteFreeNode)
        {
            modeldb->deleteFreeNode(guidata->_cloudFrame);
            postRender->updateAllModelOperate(ClearFreeNode);
        }

        //数据读取完成,渲染窗口可接入变量数据
        mxr::IPostModelRenderDataProxy* model_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(hosted->getModelRenderDataProxy());
        if (model_proxy != nullptr)
        {
            if (hasFailedChange)
            {
                //存在失效单元显隐变化：需重建渲染索引(buildIndex)使 is_show 生效，同时更新颜色
                model_proxy->RebuildModelData();
            }
            else
            {
                model_proxy->updateValue();
                }
        }

        //必须在 setRendCurrentFrameData 与 updateValue 之后调用：
        //applyPostDeformation 内部会经 setMinMaxData->getMinMaxLocation 走 _oneFrameRender 上的旧代理取最值位置，
        //若早于二者执行，旧代理的 _part_min_max（上一变量的节点/单元ID）会与已切换的帧结果类型错配，导致 getVertexs 中 map::at 越界崩溃
        {
            applyPostDeformation(rebuildPostRend, guidata);
            }
        //FR-13 未变形武装期重断言：applyPostDeformation 每次重绘按既定系数覆盖渲染态，
        //未变形档须紧随其后重写置零（非武装态=一次指针比较零动作，重绘链零回归）
        reassertRealtimeUndeformedShape(rebuildPostRend);

        //向量图跟随重绘：按 guidata 记录参数重读当前帧矢量场并重建（未应用/读场失败不动作；
        //变形已应用保证箭头基点在变形后模型上）
        refreshVectorPlotIfActive(rebuildPostRend, guidata);

        //截面剖切跟随重绘：按 guidata 记录参数重提交（封盖截面按当前变量结果着色重切；
        //须在变形应用与色表值域更新前后的时序无敏感——重提交自身即时读当前状态）
        refreshSectionCutIfActive(rebuildPostRend, guidata);

        {
            // 色表值域收编（C4 仅窗单效：本函数无伴生 setMinMaxData）；具名映射与现状
            // (maxval,minval) 形参序状态等价（窗内 std::min/max 归一）
            mxr::ColorLegendParams _lg;
            _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
            _lg.targetFrame = hosted->getFrameID();
            _lg.valueMin = query_proxy->getCurrentMinData();
            _lg.valueMax = query_proxy->getCurrentMaxData();
            PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
            }
        if (hasFailedChange)
        {
            //此场景下 OSG 窗口跟随，未使用视口也要同步变换
            postRender->updateAllModelOperate(UpdateModel);
        }
        {
            postRender->updateAllModelOperate(UpdateVariableValue);
            }
        {
            postRender->updateAllModelOperate(UpdateMinMax);
            }


        if (isSolidColor)
        {
            //伪变量SolidColor：不显示云图，按部件自身颜色渲染
            postRender->updateAllModelOperate(ShowSolidColor);
        }
        else
        {
            const auto frame_type = modeldb->getFrameResultType(fi); // 值的类型
            if (frame_type == 0) //节点
            {
                postRender->updateAllModelOperate(ShowNodeColor);
            }
            else if (frame_type == 1) //单元
            {
                postRender->updateAllModelOperate(ShowElementColor);
                    }
        }

        // R5 导入首显补偿：导入收尾两处 FitView 因帧实例未就绪早退时置位的挂起旗标，
        // 在首个就绪重入此处消费（此刻 AABB 有效，相机真正适配模型）；只补首次
        // 装载——旗标复位后切帧不再动相机。早退分支（frameReady 重入前）不清旗标
        if (guidata->_pendingFitView)
        {
            guidata->_pendingFitView = false;
            rebuildPostRend->FitView();
        }

        if (isSolidColor)
        {
            //隐藏色标并按部件颜色渲染
            postRender->getColorTableWindow()->setVisiable(false);
            postRender->updateAllModelOperate(ColorAllPartOperate);
        }
        else
        {
            postRender->getColorTableWindow()->setVisiable(true);
        }
        std::set<int> hideParts = getCVM->getModelPartVisualData(rebuildPostRend->getViewportName());
        auto id_all = query_proxy->getAllPartID();

        if (auto* stateEditor = query_proxy->partStateEditor())
        {
            stateEditor->setVisibleParts(id_all);
            stateEditor->setInvisibleParts(hideParts);
        }
        getCVM->updateAllPostView();
        for (auto id : hideParts)
        {
            modelData->setMeshPartVisual(rebuildPostRend->getViewportName(), id, false);
        }

        //【极值显示冻结（cloud-panel-frame-sync 二期追加，FR-07 边界扩展）】播放中（任一
        //动源：会话 playbackState()==Playing 或浮层 isRealtimeAnimPlaying()，与 FR-09 旁听
        //守卫同口径）冻结本面板两处极值显示回写——图例范围上/下限编辑框与信息分组
        //"默认最大值/默认最小值"标签停驻旧值；值域计算与渲染驱动（setCurrentMaxData/
        //setValueMinMax/setMinMaxData 等）与 guidata 真值写入照常（3D 视口图例色标逐帧
        //照常变化，guidata 为停机同步 FR-09 的数据源）；停机经停机同步链 updateResultInfoGUI
        //一次性回填停机真值（解除冻结）
        const bool frozenExtremes = (postRender->playbackState() == mxr::PlaybackState::Playing)
            || isRealtimeAnimPlaying();
        //SolidColor无结果数据，跳过范围计算与色标数值更新
        if (!isSolidColor)
        {
            float minval = FLT_MAX;
            float maxval = -FLT_MAX;

            int index = _comboRangeMode->currentIndex();
            if (index == 0)
            {
                //全帧统一值域按图例范围模式解析（逐帧/全程/手动），不可得回退当前帧原始值域
                resolveFullRange(maxval, minval, postName, modeldb, varstr, compostr,
                    layerstr, complexstr, query_proxy, guidata);
                _legendMax->lineEdit()->blockSignals(true);
                _legendMin->lineEdit()->blockSignals(true);
                if (!frozenExtremes)	//【极值显示冻结（cloud-panel-frame-sync 二期追加）】播放中跳过回写：面板停驻旧值，值域计算/渲染驱动照常
                {
                    _legendMax->lineEdit()->setText(MxFunctions::mNumberToString(maxval, 'f', 12));
                    _legendMin->lineEdit()->setText(MxFunctions::mNumberToString(minval, 'f', 12));
                }
                _legendMax->lineEdit()->blockSignals(false);
                _legendMin->lineEdit()->blockSignals(false);
                query_proxy->setCurrentMaxData(maxval);
                query_proxy->setCurrentMinData(minval);
                // 色表值域收编（C4 双效点：与本函数尾段 setMinMaxData 伴生——窗写段，实例写段见尾段）
                {
                    mxr::ColorLegendParams _lg;
                    _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;   // 窗写段（现状首笔）
                    _lg.valueMin = minval;
                    _lg.valueMax = maxval;
                    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
                }
                this->slot_setMostValue(false);
                guidata->_cloudMaxValue = maxval;
                guidata->_cloudMinValue = minval;
            }
            else if (index == 1)
            {
                auto cur_min_max = model_proxy->getPartMinMax();
                minval = cur_min_max.first;
                maxval = cur_min_max.second;
                //半就绪防御：无值部件的哨兵条目 {FLT_MAX,-FLT_MAX} 参与聚合会得
                //min=FLT_MAX>max（倒挂），按帧无数据回落 0/0（同图例范围档兜底语义）
                if (!(maxval >= minval) || !std::isfinite(maxval) || !std::isfinite(minval))
                {
                    maxval = 0.0f;
                    minval = 0.0f;
                }

                _legendMax->lineEdit()->blockSignals(true);
                _legendMin->lineEdit()->blockSignals(true);
                if (!frozenExtremes)	//【极值显示冻结（cloud-panel-frame-sync 二期追加）】播放中跳过回写：面板停驻旧值，值域计算/渲染驱动照常
                {
                    _legendMax->lineEdit()->setText(MxFunctions::mNumberToString(maxval, 'f', 12));
                    _legendMin->lineEdit()->setText(MxFunctions::mNumberToString(minval, 'f', 12));
                }
                _legendMax->lineEdit()->blockSignals(false);
                _legendMin->lineEdit()->blockSignals(false);
                query_proxy->setCurrentMinData(minval);
                query_proxy->setCurrentMaxData(maxval);
                // 色表值域收编（C4 双效点）：默认值域段+窗写段一口两掩码（顺序按现状），实例写段见尾段
                {
                    mxr::ColorLegendParams _lg;
                    _lg.updateMask = mxr::ColorLegendParams::Mask_DefaultRange
                        | mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
                    _lg.valueMin = minval;
                    _lg.valueMax = maxval;
                    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
                }
                this->slot_setMostValue(false);
                guidata->_cloudMaxValue = maxval;
                guidata->_cloudMinValue = minval;
            }

            // 色表值域收编（C4 双效点·实例写段）：旧 setMinMaxData 原体（含窗写+刷新链）
            {
                mxr::ColorLegendParams _lg;
                _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRange;
                _lg.valueMin = minval;
                _lg.valueMax = maxval;
                PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
            }

            float maxvalue = query_proxy->getOriginalMaxData();
            float minvalue = query_proxy->getOriginalMinData();
            guidata->_cloudDefualtMax = maxvalue;
            guidata->_cloudDefualtMin = minvalue;
        }

        //信息标签显隐跟随 guidata（内容/字体由注释面板管理；隐藏与复位见 slot_labelHide/Reset）
        if (mxr::mPostFrameText* viewtext = postRender->getPostFrameText())
            viewtext->setTopRightTextVisual(guidata->_cloudInfoLabelVisible);

        ////更新视图文本
        //mPostFrameText* viewtext = postRender->getPostFrameText();
        //if (viewtext != nullptr)
        //{
        //	QString model = modeldb->getName();
        //	QString casetext = modeldb->getFrameCase(fi);
        //	QString framename = modeldb->getFrameNameByFrameNum(fi);
        //	mPostModelData* modeldata = modeldb->getModelData(fi);
        //	QString type,timestr;
        //	if (!modeldata->_frameInfo._isFrequency)
        //	{
        //		type = "Time";
        //		double time = modeldb->getFrameTime(fi);
        //		timestr = MxFunctions::mNumberToString(time, 'E', 6);
        //	}
        //	else
        //	{
        //		type = "Frequency";
        //		double fre = modeldata->_frameInfo._frequency._modalFreq;
        //		timestr = MxFunctions::mNumberToString(fre, 'E', 6);
        //	}

        //	viewtext->setTopRightText(1, QString("Model:%1 Case:%2")
        //		.arg(model).arg(casetext));
        //	viewtext->setTopRightText(2, QString("Frame:%3 %4:%5")
        //		.arg(framename).arg(type).arg(timestr));
        //	postRender->update();
        //}

        postRender->getColorTableWindow()->SetTitle(0, "MxSim");
        if (guidata->_cloudLayerName.isEmpty())
        {
            QString title = guidata->_cloudVarName;
            if (!guidata->_cloudCompo.isEmpty()) {
                title += " - " + guidata->_cloudCompo;
            }
            postRender->getColorTableWindow()->SetTitle(1, title.toStdString().c_str());
        }
        else
        {
            QString title = guidata->_cloudVarName;
            if (!guidata->_cloudCompo.isEmpty()) {
                title += " - " + guidata->_cloudCompo;
            }
            if (!guidata->_cloudLayerName.isEmpty()) {
                title += " : " + guidata->_cloudLayerName;
            }
            postRender->getColorTableWindow()->SetTitle(1, title.toStdString().c_str());
        }
        //桥梁杆实体化显示
        if (guidata->_cloudShowSBR)
            postRender->updateAllModelOperate(ShowBeamShellVolume);
        else
            postRender->updateAllModelOperate(ClearBeamShellVolume);

        //设置显示模式
        setShowMode();

        //设置点大小
        postRender->setPointSize(guidata->_pointSize);

        if (isNew && !frozenExtremes)	//【极值显示冻结】播放中跳过"默认最大值/默认最小值"标签回写（停机经停机同步链 updateResultInfoGUI 回填真值）
        {
            //更新信息显示
            this->updateInformation();
        }

        //获取颜色表指针
        const auto& ptColorTable = postRender->getColorTableWindow();
        //获取颜色条对应颜色
        _defaultColors.clear();
        unsigned char* colorTable = ptColorTable->GetColorData();
        const int colorTableRow = ptColorTable->GetColorCount() * 3;
        if (colorTable != nullptr && colorTableRow > 0)//【卡顿修复】空缓冲防御：图例数据未就绪时跳过取色（曾致空表 AV）
        for (int i = colorTableRow - 1; i > 0; i = i - 3)
        {
            //填充自定义颜色条颜色数据
            QColor color;
            color.setRed(colorTable[i - 2]);
            color.setGreen(colorTable[i - 1]);
            color.setBlue(colorTable[i]);
            _defaultColors.push_back(color);
        }
        //获取颜色条对应数值表
        _defaultValues.clear();
        std::vector<float> valueTable = ptColorTable->GetValue();
        for (int i = 0; i < valueTable.size(); i++)
        {
            QString value = MxFunctions::mNumberToString(valueTable.at(i), 'e', 3);
            _defaultValues.push_back(value);
        }
        guidata->_defaultColors = _defaultColors;
        guidata->_defaultValues = _defaultValues;

        MechCommon::mMechGlobalSignals::getInstance()->updateResultMenuGUI();

        // 通知标注面板等跟随云图当前帧刷新
        MechCommon::mMechGlobalSignals::getInstance()->postCloudFrameChanged(guidata->_cloudFrame);

        //云图重绘完成（携带视口名：等值面面板据此按存储阈值随新帧自动重提取，
        //HyperView 语义——仅重绘本面板绑定视口时联动）
        //整幅重绘成功=当前面板 UI 已全部应用（含顺带应用的待生效参数），清"待应用"提示
        setCloudDirty(false);
        emit cloudRedrawnSig(_curViewName);
    }

    void mPost3DResultPanel::slot_setMostValue(bool flag)
    {
        //获取最大值
        double maxval = _legendMax->lineEdit()->text().trimmed().toFloat();
        double minval = _legendMin->lineEdit()->text().trimmed().toFloat();
        if (minval == FLT_MAX)
        {
            maxval = 0.0f;
            minval = 0.0f;
        }
        //非有限值（哨兵/±inf 文本化回读）按无数据处理，不参与上下限比较
        if (!std::isfinite(maxval) || !std::isfinite(minval))
        {
            maxval = 0.0;
            minval = 0.0;
        }
        if (maxval < minval) {
            mGlobalSignals::getInstance()->outputMessageSig(2, QString("最大值不能小于最小值！"), true);
            return;
        }

        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;

        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        //存储界面信息
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr)
            return;
        mxr::IPostRendQueryDataProxy* query_proxy1 = dynamic_cast<mxr::IPostRendQueryDataProxy*>(postRender->frameProxy(postRender->currentFrame())->getRendQueryDataProxy());
        if (query_proxy1 == nullptr)
            return;
        if (flag)
        {
            query_proxy1->setCurrentMaxData(maxval);
            query_proxy1->setCurrentMinData(minval);
            // 色表值域收编（C4 双效点）：实例写段+窗写段两笔（顺序按现状）；具名映射与
            // 现状 (maxval,minval) 窗形参序状态等价（窗内 std::min/max 归一）
            {
                mxr::ColorLegendParams _lg;
                _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRange;
                _lg.valueMin = minval;
                _lg.valueMax = maxval;
                PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
            }
            {
                mxr::ColorLegendParams _lg;
                _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
                _lg.valueMin = minval;
                _lg.valueMax = maxval;
                PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
            }
            if (guidata != nullptr)
            {
                guidata->_cloudMaxValue = maxval;
                guidata->_cloudMinValue = minval;
            }
        }
        QString postname = postRend->getPostDataName();
        QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);

        if (modeldb == nullptr)
            return;

        postRender->getColorTableWindow()->SetTitle(0, "MxSim");
        if (guidata->_cloudLayerName.isEmpty())
        {
            QString title = guidata->_cloudVarName;
            if (!guidata->_cloudCompo.isEmpty()) {
                title += " - " + guidata->_cloudCompo;
            }
            postRender->getColorTableWindow()->SetTitle(1, title.toStdString().c_str());
        }
        else
        {
            QString title = guidata->_cloudVarName;
            if (!guidata->_cloudCompo.isEmpty()) {
                title += " - " + guidata->_cloudCompo;
            }
            if (!guidata->_cloudLayerName.isEmpty()) {
                title += " : " + guidata->_cloudLayerName;
            }
            postRender->getColorTableWindow()->SetTitle(1, title.toStdString().c_str());
        }




        mxr::IPostModelRenderDataProxy* model_query_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(postRender->frameProxy(postRender->currentFrame())->getModelRenderDataProxy());
        if (model_query_proxy != nullptr)
        {
            mxr::PostNodeOrElement nodeOrElement = query_proxy1->getNodeOrElement();
            std::pair<int, float> maxid = model_query_proxy->getOneTotalMaxID();
            std::pair<int, float> minid = model_query_proxy->getOneTotalMinID();
            if (nodeOrElement == mxr::PostNodeOrElement::PostElement)
            {
                postRender->getColorTableWindow()->setMaxFootLabel(0, "max: %E", query_proxy1->getCurrentMaxData());
                postRender->getColorTableWindow()->setMaxFootLabel(1, "at Elem# %d", maxid.first);
                postRender->getColorTableWindow()->setMinFootLabel(2, "min: %E", query_proxy1->getCurrentMinData());
                postRender->getColorTableWindow()->setMinFootLabel(3, "at Elem# %d", minid.first);
            }
            else
            {
                postRender->getColorTableWindow()->setMaxFootLabel(0, "max: %E", query_proxy1->getCurrentMaxData());
                postRender->getColorTableWindow()->setMaxFootLabel(1, "at Node# %d", maxid.first);
                postRender->getColorTableWindow()->setMinFootLabel(2, "min: %E", query_proxy1->getCurrentMinData());
                postRender->getColorTableWindow()->setMinFootLabel(3, "at Node# %d", minid.first);
            }
        }
        postRend->updatePaint();
        mMechGlobalSignals::getInstance()->updateResultMenuGUI();
    }

    void mPost3DResultPanel::slot_changeFrame()
    {
        QPushButton* pb = qobject_cast<QPushButton*>(sender());
        if (pb == nullptr) return;

        REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderByViewName(_curViewName);
        if (rebuildPostRend == nullptr) return;

        auto  postRender = rebuildPostRend->getPostRender();
        if (postRender == nullptr) return;

        QString postName = rebuildPostRend->getPostDataName();
        QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        if (modeldb == nullptr)
            return;

        //只切换当前工况的帧
        /*QString casename = _comboCase->currentText();
        QStringList frameList = modeldb->getCaseFrameNames(casename);
        frameList = sortQStringList(frameList);
        if (frameList.size() == 0) return;
        QString currentFrame;

        if (pb == _btnFirstFrame)
        {
            currentFrame = frameList[0];
        }
        else if (pb == _btnPreFrame)
        {
            QString cur = _comboFrame->currentText();
            int index = frameList.indexOf(cur);

            int curindex = index - 1;
            if (curindex >= 0)
            {
                currentFrame = frameList[curindex];
            }
            else
            {
                currentFrame = frameList[index];
            }
        }
        else if (pb == _btnNextFrame)
        {
            QString cur = _comboFrame->currentText();
            int index = frameList.indexOf(cur);

            int curindex = index + 1;
            if (curindex < frameList.size())
            {
                currentFrame = frameList[curindex];
            }
            else
            {
                currentFrame = frameList[index];
            }
        }
        else if (pb == _btnLastFrame)
        {
            int index = frameList.size();
            currentFrame = frameList[index - 1];
        }
        _comboFrame->setCurrentText(currentFrame);*/

        //所有帧号
        QList<int> frameList = modeldb->getFrameIds();
        if (frameList.size() == 0) return;
        std::sort(frameList.begin(), frameList.end());

        int frame;
        QString currentFrame;
        QString currentCase;

        QString curC = _comboCase->currentText();
        QString curF = _comboFrame->currentText();
        if (pb == _btnFirstFrame)
        {
            frame = frameList[0];
            currentCase = modeldb->getCaseNameByFrameNum(frame);
            QStringList frameNameList = modeldb->getCaseFrameNames(currentCase);
            //阶数
            currentFrame = frameNameList[frame - 1];
        }
        else if (pb == _btnPreFrame)
        {
            QString curFrameName;
            if (curF.contains(":"))
                curFrameName = curF.split(":").at(0);
            int framenum = modeldb->getFrameNumByCaseAndFrameName(curC, curFrameName);

            int curindex = framenum - 1;
            if (curindex >= 1)
            {
                currentCase = modeldb->getCaseNameByFrameNum(curindex);
                QStringList frameNameList = modeldb->getCaseFrameNames(currentCase);
                currentFrame = frameNameList[curindex - 1];
            }
            else
            {
                currentCase = modeldb->getCaseNameByFrameNum(framenum);
                QStringList frameNameList = modeldb->getCaseFrameNames(currentCase);
                currentFrame = frameNameList[framenum - 1];
            }
        }
        else if (pb == _btnNextFrame)
        {
            QString curFrameName;
            if (curF.contains(":"))
                curFrameName = curF.split(":").at(0);
            int framenum = modeldb->getFrameNumByCaseAndFrameName(curC, curFrameName);

            int curindex = framenum + 1;
            if (curindex <= frameList.size())
            {
                currentCase = modeldb->getCaseNameByFrameNum(curindex);
                QStringList frameNameList = modeldb->getCaseFrameNames(currentCase);
                currentFrame = frameNameList[curindex - 1];
            }
            else
            {
                currentCase = modeldb->getCaseNameByFrameNum(framenum);
                QStringList frameNameList = modeldb->getCaseFrameNames(currentCase);
                currentFrame = frameNameList[framenum - 1];
            }
        }
        else if (pb == _btnLastFrame)
        {
            frame = frameList[frameList.size() - 1];
            currentCase = modeldb->getCaseNameByFrameNum(frame);
            QStringList frameNameList = modeldb->getCaseFrameNames(currentCase);
            //阶数
            currentFrame = frameNameList[frame - 1];
        }
        //帧导航=立即应用：守卫期间级联（工况/帧/变量回填）只记脏不散绘，此处统一按
        //当前面板设置整幅重绘一次（覆盖跨工况 setCurrentText 同文本不发射信号的边角）
        _frameNavGuard = true;
        _comboCase->setCurrentText(currentCase);
        _comboFrame->setCurrentText(currentFrame);
        _frameNavGuard = false;
        drawCloudMap();
    }

    //实时动画播放 v1（HyperView 式）：v1.4 起常显工具栏（原按帧数自动显隐随 ADS 退役）。
    //P2-2 审计修复：实时动画播放态对外查询——控件条空/未播放均 false（测量面板
    //Q3-B 门控扩展消费；定义置于 cpp 因头文件仅前向声明控件条类，不完整类型不可解引）。
    //v1.4 常显后控件条与面板同生命周期挂壳复用，播放态查询口径不变
    bool mPost3DResultPanel::isRealtimeAnimPlaying() const
    {
        return _realtimeAnimWidget.data() != nullptr && _realtimeAnimWidget.data()->isPlaying();
    }

    //v1.4 退役说明（原"动画播放"停靠页签的 ADS 落位助手区，04 评审已核实退役清单完备）：
    //kAnimDockName/animDockManager/placeAnimDockBelowViewport 文件局部 namespace、
    //ensureRealtimeAnimDock/hideRealtimeAnimDock 的 dock 包裹/复用/守卫旗逻辑随 ADS
    //形态反转整体退役；老布局 DockingState 残留"动画播放"占位 dock 的清理 defense
    //落在 mDockLayoutManager::restoreLayout 尾部（findDockWidget→removeDockWidget，幂等）

    //实时动画 v1.4 常显（D-08"总是显示"+第 2 轮形态反转）：原 updateRealtimeAnimVisibility
    //"创建/隐藏"语义重写为"挂载+使能管理"语义——工具栏壳进入后处理上下文即挂载并常显；
    //无渲染会话或无结果→停播+全禁（读数"暂无动画帧"）；单帧→帧形态走带禁/参数化按边界
    //可用（开放问题 7 结论：generateAnimation 参数化分支仅查 boundProxy，帧表旁路）；
    //多帧→帧范围/帧时间轴刷新+禁用解除。原"多帧自动显示/单帧自动隐藏/手动关闭不回弹"
    //整组随 ADS 形态退役。调用点=帧表重建两处动态刷点（slot_chooseCase/updateResultInfoGUI）
    void mPost3DResultPanel::updateRealtimeAnimEnabled()
    {
        ensureRealtimeAnimMounted();
        if (_realtimeAnimWidget == nullptr)
            return;	//壳/provider 未就绪（理论不达）：控件条保持未建，本出口下次重入

        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        auto postRender = (postRend != nullptr) ? postRend->getPostRender() : nullptr;
        const int frameCount = _comboFrame->count();
        if (postRender == nullptr || frameCount <= 0)
        {
            //无结果：先停播放（停机点补刷链经 pausePlayback→slot_realtimePlayToggled
            //!playing 分支照常走）再全禁——工具栏常显不隐藏（D-08：停播+禁用替代原
            //"停播+隐藏"；控件条会话/形态保持，恢复多帧后禁用解除）
            _realtimeAnimWidget->pausePlayback();
            _realtimeAnimWidget->setEnabledContext(0);
            return;
        }

        //FR-13 结果切换退出未变形档：武装会话与当前渲染会话不一致（结果切换/视口重建，
        //QPointer 失效或指向旧会话）→构形会话回"变形后"+清武装——恢复由新会话重绘链
        //按 guidata 权威值自愈（applyPostDeformation 每次重绘重写既定系数）；同会话
        //（切工况刷新帧表）武装保持，未变形观感延续
        if (_undeformedArmedRend != postRend)
        {
            if (_realtimeAnimWidget != nullptr
                && _realtimeAnimWidget->settings().shape == mPostRealtimeAnimWidget::ShapeMode::Undeformed)
                _realtimeAnimWidget->resetShapeToDeformed();
            _undeformedArmedRend.clear();
        }

        //帧范围动态刷新（切工况/切结果帧表变化→控件条刷新，不重建不闪动）+帧时间轴
        //批量下发（FR-09/11/14 真接入数据源：时间档读数/W2 时间域映射/倍率缩放）
        _realtimeAnimWidget->setFrameCount(frameCount, _comboFrame->currentIndex());
        _realtimeAnimWidget->setEnabledContext(frameCount);
        _realtimeAnimWidget->setFrameTimes(animFrameTimes());
    }

    //帧时间轴批量读取（开放问题 1 结论：mPostDataBase::getFrameTime 真读，1 基帧号）。
    //按帧下拉项名解析帧号对齐（slot_chooseFrame 同款"名称:"前缀解析——排序后下拉序≠
    //帧号序）；帧号解析失败/数据缺帧按 0 记（→全帧<=0 判无帧时间数据）。数十帧量级
    //O(N) 一次直读，随帧表刷新调用（滑条/读数刷新即时，01 非功能 2）
    QVector<double> mPost3DResultPanel::animFrameTimes()
    {
        QVector<double> times;
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr)
            return times;
        const QString postName = postRend->getPostDataName();
        QPointer<mPostDataBase> db = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        if (db == nullptr)
            return times;
        const QString caseName = _comboCase->currentText();
        const int n = _comboFrame->count();
        times.reserve(n);
        for (int i = 0; i < n; ++i)
        {
            const QString itemText = _comboFrame->itemText(i);
            const QString frameName = itemText.contains(":") ? itemText.split(":").at(0) : itemText;
            const int frameNum = db->getFrameNumByCaseAndFrameName(caseName, frameName);
            times.append(frameNum > 0 ? db->getFrameTime(frameNum) : 0.0);
        }
        return times;
    }

    //【审计 B-02】引擎档镜像/收口三通道按视口门面重绑：断开旧视口实例上的三条 connect
    //（QMetaObject::Connection 簿记，mPostAnimation _frameChangedConns 先例），按给定
    //视口重建。三条 lambda 体均按 _curViewName 现查门面+enginePlayback 门（事件到达时
    //_curViewName 已是本视图，与重绑时点解耦）；门面未就绪（先挂载后加载）=保持断开，
    //后续挂载/切视口重入重试。调用点=挂载出口（ensureRealtimeAnimMounted 尾，每次重入
    //幂等重绑）+switchViewGuard 异名分支（新视口名直传——调用时点 _curViewName 未切）
    void mPost3DResultPanel::rebindRealtimeEngineMirrorConns(const QString& viewName)
    {
        if (_rtMirrorFrameConn)
        {
            disconnect(_rtMirrorFrameConn);
            _rtMirrorFrameConn = QMetaObject::Connection();
        }
        if (_rtMirrorStepConn)
        {
            disconnect(_rtMirrorStepConn);
            _rtMirrorStepConn = QMetaObject::Connection();
        }
        if (_rtMirrorDirConn)
        {
            disconnect(_rtMirrorDirConn);
            _rtMirrorDirConn = QMetaObject::Connection();
        }
        REBUILD_PostRend* rend = getCVM->getPostRenderByViewName(viewName);
        if (rend == nullptr || rend->getPostRender() == nullptr)
            return;	//门面未就绪：三通道保持断开（挂载/切视口重入重试）
        auto pr = rend->getPostRender();
        //帧通道（引擎合一 2026-11-16 既有 lambda 原样迁移）：PlayAdvance 源过滤+录制互斥
        //+自然停机+值就绪复核+帧锚前推+轻量镜像+面板跟随
        _rtMirrorFrameConn = connect(pr.get(), &mxr::REBUILD_PostRender::frameChanged, this,
            [this](int frame, mxr::FrameChangeSource source)
        {
            if (source != mxr::FrameChangeSource::PlayAdvance) return;
            if (_realtimeAnimWidget == nullptr || !_realtimeAnimWidget->enginePlayback()) return;
            REBUILD_PostRend* rend = getCVM->getPostRenderByViewName(_curViewName);
            if (rend == nullptr || rend->getPostRender() == nullptr) return;
            auto pr = rend->getPostRender();
            //录制互斥（引擎态无 tick 闸，录制开始在此拦停）：暂停→toggled(false)→停机补发链
            if (mPostInfoHub::getInstance()->isRecordingActive())
            {
                _realtimeAnimWidget->pausePlayback();
                return;
            }
            //自然停机（非循环播完/删除动画链已停会话）：浮层按钮态复位→停机点补发链照常走
            if (pr->playbackState() != mxr::PlaybackState::Playing)
            {
                _realtimeAnimWidget->pausePlayback();
                return;
            }
            mPostGUIData* gui = getCVM->getGUIDataByPostRender(rend);
            QPointer<mPostDataBase> engDb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(rend->getPostDataName());
            //值就绪复核：换变量/分量后未重读帧（值仍是旧变量）→ 自动暂停引擎（正确性
            //优先；重播转惰性物化混合档逐帧补读续播）
            const bool dataReady = gui != nullptr && engDb != nullptr
                && gui->_cloudVarName != QString("SolidColor")
                && engDb->getFrameResultType(frame) >= 0
                && !engDb->getResultDataByFrameID(frame).isEmpty()
                && engDb->getResultVarNameByFrameID(frame) == expectedResultVarName(_comboVariable->currentText(), _comboComponent->currentText());
            if (!dataReady)
            {
                _realtimeAnimWidget->pausePlayback();
                mGlobalSignals::getInstance()->outputMessageSig(2,
                    QString("变量/分量已切换且帧 %1 结果数据未重读，实时播放已自动暂停——重新点播放将逐帧补读续播").arg(frame), true);
                return;
            }
            if (gui != nullptr)
                gui->_cloudFrame = frame;   // 真实帧锚前推（同帧幂等判据消费）
            mirrorFrameValueDisplay(rend, pr.get(), gui, engDb, frame);
            MechCommon::mMechGlobalSignals::getInstance()->postCloudFrameChanged(frame);
        });
        //参数化镜像通道（unify-anim-playback 既有 lambda 原样迁移）：档位域事件物理隔离
        //+录制互斥+自然停机收口+mirrorEngineStep+updatePaint 尾补
        _rtMirrorStepConn = connect(pr.get(), &mxr::REBUILD_PostRender::interpolationAdvanced, this,
            [this](int step, int direction)
        {
            if (_realtimeAnimWidget == nullptr || !_realtimeAnimWidget->enginePlayback()) return;
            REBUILD_PostRend* stepRend = getCVM->getPostRenderByViewName(_curViewName);
            if (stepRend == nullptr || stepRend->getPostRender() == nullptr) return;
            auto stepPr = stepRend->getPostRender();
            //录制互斥（同帧通道）：暂停→toggled(false)→停机补发链
            if (mPostInfoHub::getInstance()->isRecordingActive())
            {
                _realtimeAnimWidget->pausePlayback();
                return;
            }
            //自然停机（Once 播完/收口链已停会话，同帧通道同构）：浮层按钮态复位→停机点
            //补发链照常走。停机拍先于镜像判停机（单次停机拍滑条停于末档前一档，与帧通道
            //现状等价——评审建议-3 零代码口径）
            if (stepPr->playbackState() != mxr::PlaybackState::Playing)
            {
                _realtimeAnimWidget->pausePlayback();
                return;
            }
            _realtimeAnimWidget->mirrorEngineStep(step, direction);
            stepRend->updatePaint();
        });
        //方向内建换向回流通道（unify-anim-playback 既有 lambda 原样迁移）：PingPong 端点
        //换向基线同步（防 reconfigure 下发过时方向）
        _rtMirrorDirConn = connect(pr.get(), &mxr::REBUILD_PostRender::playbackDirectionChanged, this,
            [this](int direction)
        {
            if (_realtimeAnimWidget == nullptr || !_realtimeAnimWidget->enginePlayback()) return;
            _realtimeAnimWidget->mirrorEngineDirection(direction);
        });
    }

    //v1.4 挂载出口（原 ensureRealtimeAnimDock 停靠化退役替代）：工具栏壳=mCenterMainWindow
    //::initAnimToolBar 预创建（restoreLayout 前已存在→saveState/restoreState 位置持久化
    //成立，03-技术路线 §2.1 方案 A 核心依据），本面板经 MechCommon::animToolBarProvider
    //取壳回挂内容件（幂等：已挂跳过）。四路信号接线与 v1.2 完全一致（协议不变）；
    //P2-3 帧对齐段（原 v1.3 创建流程迁移，创建一次性执行）；壳 installEventFilter
    //（拖动/脱停→控件条关设置弹窗）
    void mPost3DResultPanel::ensureRealtimeAnimMounted()
    {
        auto barProvider = MechCommon::animToolBarProvider();
        QToolBar* bar = barProvider ? barProvider() : nullptr;
        if (bar == nullptr)
            return;		//壳未就绪（理论不达）：控件条保持未建，本出口下次重入

        if (_realtimeAnimWidget == nullptr)
        {
            //控件条（原型紧凑单行）+四路信号接线（协议不变）；默认帧动画形态、不自动播放
            _realtimeAnimWidget = new mPostRealtimeAnimWidget(_comboFrame->count(),
                _comboFrame->currentIndex());
            connect(_realtimeAnimWidget, &mPostRealtimeAnimWidget::frameIndexChanged,
                this, &mPost3DResultPanel::slot_realtimeFrameChanged);
            //v1.1：播放开关（互斥即时判定）、插值档（参数化形态驱动）、形态切换（渲染侧收尾）
            connect(_realtimeAnimWidget, &mPostRealtimeAnimWidget::playbackToggled,
                this, &mPost3DResultPanel::slot_realtimePlayToggled);
            connect(_realtimeAnimWidget, &mPostRealtimeAnimWidget::interpolationStepChanged,
                this, &mPost3DResultPanel::slot_realtimeInterpolationChanged);
            connect(_realtimeAnimWidget, &mPostRealtimeAnimWidget::modeChanged,
                this, &mPost3DResultPanel::slot_realtimeModeChanged);
            //D-05 升格 FR-13：构形切换渲染收口（未变形=置零武装/变形后=恢复既定系数），
            //与四路信号面板收口同构（信号由控件条发、渲染动作收口在面板）
            connect(_realtimeAnimWidget, &mPostRealtimeAnimWidget::shapeChanged,
                this, &mPost3DResultPanel::slot_realtimeShapeChanged);
            //【FR-05】浮层跳帧桥（cloud-panel-frame-sync）：云图菜单播放中选帧→选帧链
            // seekFrame 已完成画面跳转，此处把新帧对齐到播放载体（浮层基帧）与面板显示，
            //下一 tick 自新帧续播。事件无视口签名——串扰已被 Q-FIX-1"异名切视口=停播放"
            //规则结构性排除（桥活跃期不存在其它视口输入源）；非播放态/非帧形态/O(1)
            //快路径（事件帧==面板当前帧，播放 tick 自身通知零动作）三重过滤。回写走
            //P2-1 同款 blockSignals+setFrameIndex（均不回发信号，无回环）
            connect(MechCommon::mMechGlobalSignals::getInstance(),
                &MechCommon::mMechGlobalSignals::postCloudFrameChanged, this,
                [this](int f)
            {
                if (_realtimeAnimWidget == nullptr) return;
                if (!_realtimeAnimWidget->isPlaying()) return;
                if (_realtimeAnimWidget->mode() != mPostRealtimeAnimWidget::AnimMode::Frame) return;
                REBUILD_PostRend* bridgeRend = getCVM->getPostRenderByViewName(_curViewName);
                if (bridgeRend == nullptr) return;
                QPointer<mPostDataBase> bridgeDb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(bridgeRend->getPostDataName());
                if (bridgeDb == nullptr) return;
                //O(1) 快路径：事件帧==面板当前帧（播放 tick 自身通知/同帧重发）零动作
                const QString curText = _comboFrame->currentText();
                const QString curName = curText.contains(":") ? curText.split(":").at(0) : curText;
                if (bridgeDb->getFrameNumByCaseAndFrameName(_comboCase->currentText(), curName) == f)
                    return;
                //P2-1 同款映射回写：遍历帧下拉按"帧名:"前缀解析匹配事件帧；未命中
                //（如工况命名差异）静默跳过保持现状
                for (int i = 0; i < _comboFrame->count(); ++i)
                {
                    const QString bridgeText = _comboFrame->itemText(i);
                    const QString bridgeName = bridgeText.contains(":") ? bridgeText.split(":").at(0) : bridgeText;
                    if (bridgeDb->getFrameNumByCaseAndFrameName(_comboCase->currentText(), bridgeName) == f)
                    {
                        _comboFrame->blockSignals(true);
                        _comboFrame->setCurrentIndex(i);
                        _comboFrame->blockSignals(false);
                        _realtimeAnimWidget->setFrameIndex(i);
                        break;
                    }
                }
            });
            //【引擎合一 2026-11-16+B-02】播放推进镜像/收口三通道（frameChanged 镜像/
        //interpolationAdvanced 镜像/playbackDirectionChanged 回流）已迁出一次性块，改经
        //rebindRealtimeEngineMirrorConns 按当前视口门面建立/重绑（挂载出口尾+切视口双
        //调用点）——原一次性绑死首挂载时点门面实例，多视口/先挂载后加载路径下事件全落
        //旧实例（镜像失随/停机回写失效/幽灵归属）。lambda 体与门语义逐字保留（见助手）
        //【阻塞-2·unify-anim-playback】引擎档滑条按下推进停役/松手恢复：按下=会话 pause
        //（拖动本地预览不被逐 tick 镜像抢写；不经 pausePlayback——不清 _enginePlayback、
        //不复位按钮）；松手=会话恢复（仅 Paused 态回 play——帧模式松手 seek 续播前镜像
        //通道以 Playing 判停机，须先恢复；参数化松手经定位三连 pause 幂等重暂停后定位+
        //续推）。Once 自然停机/互斥收口等真实停机不经本通道（suspended=false 仅滑条松手
        //路径发射，播放钮态由浮层侧校验）。widget 级信号不依赖视口门面——原 consRend
        //门（审计 B-02 四重守卫之一）解除，先挂载后加载路径不再漏接
        connect(_realtimeAnimWidget, &mPostRealtimeAnimWidget::engineAdvanceSuspendRequested,
            this, [this](bool suspended)
        {
            if (_realtimeAnimWidget == nullptr || !_realtimeAnimWidget->enginePlayback()) return;
            REBUILD_PostRend* suspRend = getCVM->getPostRenderByViewName(_curViewName);
            if (suspRend == nullptr || suspRend->getPostRender() == nullptr) return;
            if (suspended)
                suspRend->getPostRender()->pause();
            else if (suspRend->getPostRender()->playbackState() == mxr::PlaybackState::Paused)
                suspRend->getPostRender()->play();
        });
        //【引擎合一】播放中弹窗设置（速度/步长/方向/循环/范围）即时下发会话（FR-12 延伸）
        connect(_realtimeAnimWidget, &mPostRealtimeAnimWidget::engineSettingsChanged,
            this, &mPost3DResultPanel::reconfigureRealtimeEngine);
        bar->installEventFilter(_realtimeAnimWidget.data());	//壳拖动/脱停→关弹窗

            //P2-3 审计修复（v1.3 创建段迁移，保留）：创建前以渲染会话当前帧对齐帧下拉
            //（快照失真防御）——预生成动画走查后画面停在停止时帧而下拉残留旧选择，控件条
            //快照=旧下拉值，首 tick 即把画面拉回旧帧附近。对齐走 blockSignals（画面已在
            //该帧不重绘——切帧全链会重建单帧渲染器丢等值面，同 setCurViewName 注释口径）；
            //blockSignals 取回旧态恢复而非无条件 false：updateResultInfoGUI 调用链上外层
            //已对 _comboFrame blockSignals，提前解除会放行后续写点信号
            REBUILD_PostRend* alignRend = getCVM->getPostRenderByViewName(_curViewName);
            auto alignRender = (alignRend != nullptr) ? alignRend->getPostRender() : nullptr;
            if (alignRender != nullptr)
            {
                const int sessionFrame = alignRender->currentFrame();
                const QString alignPost = alignRend->getPostDataName();
                QPointer<mPostDataBase> alignDb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(alignPost);
                if (alignDb != nullptr && sessionFrame > 0)
                {
                    const QString alignCase = _comboCase->currentText();
                    for (int i = 0; i < _comboFrame->count(); ++i)
                    {
                        const QString alignText = _comboFrame->itemText(i);
                        const QString alignName = alignText.contains(":") ? alignText.split(":").at(0) : alignText;
                        if (alignDb->getFrameNumByCaseAndFrameName(alignCase, alignName) == sessionFrame)
                        {
                            const bool wasBlocked = _comboFrame->blockSignals(true);
                            _comboFrame->setCurrentIndex(i);
                            _comboFrame->blockSignals(wasBlocked);
                            break;
                        }
                    }
                }
            }
        }
        //回挂壳（幂等：控件条已挂跳过；壳预创建为空，首挂即占位内容件）
        if (_realtimeAnimWidget->parentWidget() != bar)
            _animBarAction = bar->addWidget(_realtimeAnimWidget);
        bar->show();	//后处理上下文常显（前处理隐藏由 slot_viewportContextChanged 驱动）
        //【审计 B-02】镜像三通道按当前视口重绑（每次挂载重入幂等执行——先挂载后加载
        //路径在此补接：首挂时门面未就绪保持断开，结果加载后本出口重入时接通）
        rebindRealtimeEngineMirrorConns(_curViewName);
    }

    //v1.4 前后处理上下文显隐（开放问题 6 结论：前处理隐藏/后处理常显——原型仅云图工作区
    //显示工具栏；D-08"总是显示"约束的是后处理上下文内的结果态显隐，工作区切换属另一
    //维度）。边沿=currentViewportChanged 增量刷新；初值=构造期 singleShot 求值一次
    //（T-2/J-02 三场景：信号无重放/同 ID 激活去重早退不发/restoreState 回写"可见"）。
    //判定=当前视口 getPostRenderWidget() 判空（SKILL 踩坑 14 同款前/后处理判定；
    //getCurrentViewportModel 判空防御——前置空指针解引即崩）
    void mPost3DResultPanel::slot_viewportContextChanged()
    {
        auto barProvider = MechCommon::animToolBarProvider();
        QToolBar* bar = barProvider ? barProvider() : nullptr;
        if (bar == nullptr)
            return;
        MechPreData::mViewportModel* curModel = MechPreData::mViewportModelManager::getInstance()->getCurrentViewportModel();
        const bool postContext = (curModel != nullptr && curModel->getPostRenderWidget() != nullptr);
        bar->setVisible(postContext);
        if (postContext)
        {
            updateRealtimeAnimEnabled();	//回后处理：挂载/禁用域/帧表按当前视口刷新（幂等）
        }
        else if (_realtimeAnimWidget != nullptr)
        {
            //切前处理：先停播放再隐藏（与 v1.3 跨视口语义等价——隐藏态播放=测量标注/
            //录制消费方无感知失步；停机点补刷经 pausePlayback 既有链照常走）
            _realtimeAnimWidget->pausePlayback();
            _realtimeAnimWidget->closeSettingsPopup();
            //FR-13：退出后处理上下文=退出未变形档——恢复既定系数（变形面板权威值，
            //QPointer 失效时自动跳过）+构形会话回"变形后"，下次进入后处理重新选择
            if (_undeformedArmedRend != nullptr)
            {
                restoreRealtimeDeformedShape(_undeformedArmedRend.data());
                _realtimeAnimWidget->resetShapeToDeformed();
            }
        }
    }

    //ui-shot 弹窗截图驱动（mUiShotController::dialogSlots 登记名"mAnimSettingsPopup"，
    //T-3 验收路径）：确保挂载后打开设置弹窗（--ui-shot-dialog 经 invokeMethod 调用）
    void mPost3DResultPanel::slot_openAnimSettingsForShot()
    {
        ensureRealtimeAnimMounted();
        if (_realtimeAnimWidget != nullptr)
            _realtimeAnimWidget->openSettingsPopup();
    }

    //ui-shot 类型菜单截图驱动（dialogSlots 登记名"animTypeMenu"，R3 盲区闭合）：确保挂载
    //后非模态弹出类型菜单（showTypeMenuForShot→QMenu::popup，grab 后"用后即焚"由菜单
    //QPointer 懒重建兜底）
    void mPost3DResultPanel::slot_openAnimTypeMenuForShot()
    {
        ensureRealtimeAnimMounted();
        if (_realtimeAnimWidget != nullptr)
            _realtimeAnimWidget->showTypeMenuForShot();
    }


    //（ensureRealtimeAnimDock/hideRealtimeAnimDock 已随 v1.4 ADS 形态退役：dock 包裹/
    //复用/用户关闭检测/程序性隐藏出口由"挂壳+使能管理"替代，见 updateRealtimeAnimEnabled/
    //ensureRealtimeAnimMounted；Q-FIX-1 停止点补刷语义经 pausePlayback 链原样保留）

    //浮层帧序变化 → 驱动 _comboFrame 切帧：复用 slot_chooseFrame→slot_showCloud
    //全链（seekFrame+渐进装载兜底：未装载帧保持上一帧+后台装载+frameReady 补绘）
    void mPost3DResultPanel::slot_realtimeFrameChanged(int index)
    {
        if (_realtimeAnimWidget == nullptr) return;
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;
        auto postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        //共存规则 v1.1（录制互斥·反向闸）：录制进行中→浮层暂停并吞掉本次帧变化
        //（录制链独占逐帧 seekFrame；tick/滑条/步进等一切浮层驱动的帧变化在此拦停）
        if (mPostInfoHub::getInstance()->isRecordingActive())
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, "录制进行中，实时动画已暂停！", true);
            _realtimeAnimWidget->pausePlayback();
            return;
        }

        //共存规则反向（tick 时探测）：预生成动画正走门面 play 在播 → 实时自暂停，
        //同一时刻视口内只保留一个动源；画面留给预生成动画。
        //【unify-anim-playback 归属豁免】浮层引擎档在播（帧模式引擎档逐 tick seek/滑条
        //松手提交）时本槽到达，Playing 是自身播放而非预生成冲突——裸判自阻（FR-04 边界
        //必测项）：归属=浮层（enginePlayback）时不自暂停，归属=预生成时才自暂停
        if (postRender->playbackState() == mxr::PlaybackState::Playing
            && !_realtimeAnimWidget->enginePlayback())
        {
            _realtimeAnimWidget->pausePlayback();
            return;
        }

        if (index < 0 || index >= _comboFrame->count()) return;
        if (!_realtimeAnimWidget->isPlaying())
        {
            if (index != _comboFrame->currentIndex())
                _comboFrame->setCurrentIndex(index);	//触发 slot_chooseFrame 全链
            return;
        }
        //【2026-11-16 播放快路径】播放 tick 只通知渲染层做帧表 bind 翻转，不再走全量
        //切帧链（旧实现每 tick 经 slot_chooseFrame→slot_showCloud 全链重绘 + emit=true
        //扇出双轮：读数门/失效单元/变形场/色表图例全套每帧重跑）。就绪帧：seekFrame
        //翻转显隐（单发 frameChanged，轻量镜像跟随）+ 脚标/图例值域代理现取轻刷 +
        //postCloudFrameChanged 面板跟随；未就绪帧：会话后台装载+armFrameLoadFallback
        //挂起集合补绘（首遍渐进上屏）。全量切帧链只在身份变化链与非播放态手动派发运行。
        //同帧幂等（FR-08 二期语义保持）：index 是 0 基下拉下标、_cloudFrame 是真实帧号
        //（快路径/补绘链写入），解析真实帧号后比较；解析失败 fail-open 走原全量派发链。
        mPostGUIData* frameGui = getCVM->getGUIDataByPostRender(postRend);
        int tickFi = -1;
        QPointer<mPostDataBase> frameDb{};
        if (frameGui != nullptr)
        {
            frameDb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postRend->getPostDataName());
            if (frameDb != nullptr)
            {
                const QString tickText = _comboFrame->itemText(index);
                const QString tickName = tickText.contains(":") ? tickText.split(":").at(0) : tickText;
                tickFi = frameDb->getFrameNumByCaseAndFrameName(_comboCase->currentText(), tickName);
            }
        }
        if (tickFi > 0 && frameGui != nullptr)
        {
            if (tickFi == frameGui->_cloudFrame)
                return;	//同帧幂等（tick 自身通知/同帧重发零动作，两空间一致的真实帧号比较）
            //【值就绪门·2026-11-16】结果值由 GUI 读数门按（文件,帧,变量身份）物化进库
            //（导入只注册模型数据，工厂代理的着色值就查这个库）——值未物化或变量身份
            //不符的帧走原全量派发链（读数物化→工厂建实例→挂起补绘上屏，保持旧链
            //"读→建"不变式）；值已就绪帧才走快路径。SolidColor 无结果值语义同走全链。
            const bool isSolidColorTick = (frameGui->_cloudVarName == QString("SolidColor"));
            const bool frameDataReady = !isSolidColorTick
                && frameDb->getFrameResultType(tickFi) >= 0
                && !frameDb->getResultDataByFrameID(tickFi).isEmpty()
                && frameDb->getResultVarNameByFrameID(tickFi) == expectedResultVarName(_comboVariable->currentText(), _comboComponent->currentText());
            if (!frameDataReady)
            {
                // 全量派发链：读数物化→全链→挂起补绘上屏（与旧逐 tick 行为一致）
                const int frozenIndex = _comboFrame->currentIndex();
                _comboFrame->blockSignals(true);
                _comboFrame->setCurrentIndex(index);
                slot_chooseFrame(_comboFrame->itemText(index));
                drawCloudMap();
                _comboFrame->setCurrentIndex(frozenIndex);
                _comboFrame->blockSignals(false);
                return;
            }
            if (postRender->seekFrame(tickFi, mxr::FrameChangeSource::PlayAdvance))
            {
                frameGui->_cloudFrame = tickFi;   // 真实帧锚前推（同帧幂等判据消费）
                // 轻量镜像：脚标/图例值域/点大小代理现取零重绘（统一助手，与引擎单通道共用）
                mirrorFrameValueDisplay(postRend, postRender.get(), frameGui, frameDb, tickFi);
                // 通知标注面板等跟随动画当前帧刷新（测量面板播放门控自拦）
                MechCommon::mMechGlobalSignals::getInstance()->postCloudFrameChanged(tickFi);
            }
            else
            {
                // 未就绪：seekFrame 内已 scheduleBuild 后台装载，挂起集合补绘在装载完成时上屏
                armFrameLoadFallback(tickFi, postRender.get());
            }
            return;
        }
        // 解析失败 fail-open（帧名差异等）：走原全量派发链（宁多跑一次全链不吞帧）
        const int frozenIndex = _comboFrame->currentIndex();
        _comboFrame->blockSignals(true);
        _comboFrame->setCurrentIndex(index);
        slot_chooseFrame(_comboFrame->itemText(index));
        drawCloudMap();
        _comboFrame->setCurrentIndex(frozenIndex);
        _comboFrame->blockSignals(false);
    }

    //实时动画 v1.1：浮层播放开关——互斥即时判定（消除"按下播放后等一个 tick 才被
    //反向闸拦停"的窗口）；放行不代启（浮层 QTimer 已自启），参数化形态由首个插值档
    //懒生成。pausePlayback 回发 playbackToggled(false) 在本槽 !playing 分支自然短路，无回环
    void mPost3DResultPanel::slot_realtimePlayToggled(bool playing)
    {
        if (_realtimeAnimWidget == nullptr) return;
        if (!playing)
        {
            //【引擎合一】引擎态停/暂停：会话播放器 pause()（保留当前帧；非循环自然停机
            //已由 playTick 置 Stopped，pause 对 Stopped no-op）。_realtimeEngineActive
            //落账清除，随后停机点补发链照常走。
            //【unify-anim-playback】本收口形态无关：帧模式引擎档与参数化引擎档同链
            //（清位路径经浮层 enginePlayback 门，归属清空+会话 pause）
            if (_realtimeEngineActive)
            {
                _realtimeEngineActive = false;
                REBUILD_PostRend* engRend = getCVM->getPostRenderByViewName(_curViewName);
                if (engRend != nullptr && engRend->getPostRender() != nullptr)
                    engRend->getPostRender()->pause();
            }
            //P2-2 审计修复（停止补刷）：浮层停/暂停/自然结束不再有逐帧通知（播放期
            //测量门控拦停的逐帧重估须有停止点收敛）——停机点补发一次
            //postCloudFrameChanged，消费方按当前帧补刷（测量门此时放行=非播放态落定
            //即刷，与预生成动画"停止点补发通知在旗标复位后到达"口径对齐）。录制让路
            //广播早于录制簿记置位，补发先于录制走查无采集面；录制活跃期（竞态拦停
            //路径）静默不补发
            if (mPostInfoHub::getInstance()->isRecordingActive())
                return;
            REBUILD_PostRend* stopRend = getCVM->getPostRenderByViewName(_curViewName);
            if (stopRend == nullptr) return;
            mPostGUIData* stopGui = getCVM->getGUIDataByPostRender(stopRend);
            if (stopGui == nullptr) return;
            MechCommon::mMechGlobalSignals::getInstance()->postCloudFrameChanged(stopGui->_cloudFrame);
            //【FR-04】浮层停机同步显式化（cloud-panel-frame-sync）：云图菜单同步收口
            //updateResultMenuGUI 在停机点补发一次（02 §FR-04"同一触发点追加云图菜单
            //强制同步"口径的显式落地；与云图菜单对 postCloudFrameChanged 的停机旁听链
            //互为冗余，均为停机一次性动作，播放中判定守卫由云图菜单侧判据承担）
            MechCommon::mMechGlobalSignals::getInstance()->updateResultMenuGUI();
            //【FR-09 二期】浮层停机同步显式化（参数面板侧，cloud-panel-frame-sync）：
            //自调 updateResultInfoGUI 完整回填=解除冻结+恢复逐操作刷新；与云图菜单侧
            //显式补发对称，与上方 postCloudFrameChanged 经本面板停机旁听链互为冗余
            //（均停机一次性动作，完整回填幂等）
            updateResultInfoGUI();
            return;
        }
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;
        auto postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        if (mPostInfoHub::getInstance()->isRecordingActive())
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, "录制进行中，不可实时播放！", true);
            _realtimeAnimWidget->pausePlayback();
            return;
        }
        //预生成动画正走会话播放器（瞬态或参数化 play）→ 实时自暂停（v1 反向互斥即时生效）
        if (postRender->playbackState() == mxr::PlaybackState::Playing)
            _realtimeAnimWidget->pausePlayback();

        //单动源互斥·时机后移（v1.4 常显口径，04 J-10 复核口径）：原"创建即 deleteAnimation
        //停预生成动画"随常显退役——常显下进入后处理即挂载，保留原时机则用户未碰工具栏
        //预生成动画就被停=行为回归；改在播放放行路径上消费 _animDeletePending 一次性旗——
        //含预生成反向闸命中路径（反向闸无 return：命中=实时自暂停+旗照常消费，首次点击=
        //自暂停+停预生成、二次点击=开播）；录制拒启闸带 return 在前，录制中首次播放不消费
        //旗，旗留存到录制结束后首次播放。deleteAnimation 复用既有停止链（stop 回停止时帧
        //+定时器停+状态复位，删动画不删数据）
        if (_animDeletePending)
        {
            _animDeletePending = false;
            mPostInfoHub::getInstance()->deleteAnimation(_curViewName);
        }
        //【阻塞-1·unify-anim-playback】播放请求存活校验：上方预生成反向闸（:6520，无
        //return）命中时 pausePlayback 已复位播放钮+停自驱表——本次播放请求作废，直接
        //返回（不路由参数化引擎承载、不进帧分诊，防"按钮复位但引擎照常开播"幽灵播放），
        //保住既有"首次点击=自暂停+停预生成、二次点击=开播"语义（上方注释口径；与浮层
        //自身拒启早退 slot_playToggled !isActive 同构）
        if (!_realtimeAnimWidget->isPlaying())
            return;

        //【unify-anim-playback 形态分诊·Q2=B 解除面】非 Frame 形态=参数化形态恒引擎档：
        //线性/正弦生成即就绪（同步体毫秒级）、插值单条目不落帧表，无"值未全物化"前提
        //（02 §3.4），点播放即进引擎档承载（唯一不进播放前置=懒生成被无数据门拒绝）
        if (_realtimeAnimWidget->mode() != mPostRealtimeAnimWidget::AnimMode::Frame)
        {
            ensureParametricEnginePlayback(postRender.get());
            return;
        }
        const auto& rtSettings = _realtimeAnimWidget->settings();
        //【Q2=B 解除面】PingPong return 删除：引擎档内建换向承担往返（播放方式随参数
        //下发；"值未全就绪+往返"组合自然落下方混合档逃生口，02 §3.4 产品定案）
        //【Q2=B 解除面】参数化残留态防御：回帧模式播放遇暂停态残留参数化显示形态→
        //stop 回 StaticFrame 再走帧分诊（先例 slot_realtimeModeChanged 收尾；播放态被
        //上方存活校验+反向闸挡住不入此路）
        if (postRender->displayMode() == mxr::DisplayMode::ParametricAnimation)
            postRender->stop();
        //【引擎合一 2026-11-16】帧形态引擎分诊：播放范围全帧值已物化（读数缓存三重门
        //库侧判据）→ 会话播放器驱动（playTick 每 tick 补发 PlayAdvance frameChanged，
        //推进/装载/淘汰/纪元全在渲染层闭环，轻量镜像经镜像单通道跟随）。未全就绪→
        //浮层自驱混合档（定时器已由播放钮启动，逐 tick 值就绪门+全量派发链惰性物化，
        //与历史行为一致）——逃生口含"未就绪+往返"组合（02 §3.4）
        const auto domain = _realtimeAnimWidget->frameDomain();
        if (!domain.valid)
            return;
        QPointer<mPostDataBase> engDb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postRend->getPostDataName());
        if (engDb == nullptr)
            return;
        const QString engCase = _comboCase->currentText();
        const QString engVar = _comboVariable->currentText();
        const QString engCompo = _comboComponent->currentText();
        bool allReady = true;
        int firstFi = -1, lastFi = -1, curFi = -1;
        for (int i = domain.lo; i <= domain.hi; ++i)
        {
            const QString text = _comboFrame->itemText(i);
            const QString name = text.contains(":") ? text.split(":").at(0) : text;
            const int fi = engDb->getFrameNumByCaseAndFrameName(engCase, name);
            if (fi <= 0) { allReady = false; break; }
            if (firstFi < 0) firstFi = fi;
            lastFi = fi;
            if (i == _realtimeAnimWidget->frameIndex())
                curFi = fi;
            allReady = engDb->getFrameResultType(fi) >= 0
                && !engDb->getResultDataByFrameID(fi).isEmpty()
                && engDb->getResultVarNameByFrameID(fi) == expectedResultVarName(engVar, engCompo);
            if (!allReady) break;
        }
        if (!allReady || firstFi <= 0 || curFi <= 0)
            return;	//未全就绪：保持浮层自驱（混合档）
        //【A-1 修复·unify-anim-playback】暂停恢复态判定（先于尾端检查）：会话 Paused=
        //上次引擎档暂停的续推请求——FR-06③ 口径=按会话账面（_aniCurrentFrame/
        //_aniDirection 全在会话落账）原样续推，尾端不跳对端、不重定位（PingPong 端点
        //暂停→续推由 playTick 端点分支翻向，Loop 端点恢复=首 tick 原生回绕等价，Once
        //端点恢复=按字面语义自然停机）。浮层镜像播放期逐 tick 回流、暂停态滑条/帧下拉
        //改动经既有链同步会话账面，恢复时 frameIndex 与会话当前帧恒一致，无重定位必要
        const bool resumeFromPause = (postRender->playbackState() == mxr::PlaybackState::Paused);
        // FR-08 起播方向尾端检查复刻（引擎档，对齐浮层 applyPlayStart；仅全新起播生效
        //——"首次点播放"口径，恢复态跳过见上）：正向在终点/反向在起点→先跳对端起点
        //再播（否则引擎档首 tick 即回绕/自停=按下无反应）
        if (!resumeFromPause)
        {
            if (_realtimeAnimWidget->direction() > 0 && curFi >= lastFi)
                curFi = firstFi;
            else if (_realtimeAnimWidget->direction() < 0 && curFi <= firstFi)
                curFi = lastFi;
        }
        // 引擎档：参数下发+定位+起播（_aniTimer 推进，事件经镜像单通道回流）
        _realtimeAnimWidget->enterEnginePlayback();
        _realtimeEngineActive = true;
        postRender->setFrameRate(qMax(1, static_cast<int>(rtSettings.fps)));
        postRender->setLoop(rtSettings.playStyle == mPostRealtimeAnimWidget::PlayStyle::Loop);
        //【unify-anim-playback】往返开关下发（播放方式三态映射：Once=(false,false)/
        //Loop=(true,false)/PingPong=(false,true)；到端点会话内建换向，镜像方向回流同步基线）
        postRender->setPingPong(rtSettings.playStyle == mPostRealtimeAnimWidget::PlayStyle::PingPong);
        postRender->setPlaybackDirection(_realtimeAnimWidget->direction());
        postRender->setFrameInterval(static_cast<int>(rtSettings.stepSize));
        postRender->setPlayRange(firstFi, lastFi);
        //【A-1 修复】恢复态不 seek：同帧 seek 看似 no-op，但恢复场景 curFi 已与账面一致，
        //重定位既无必要也会在账面外重入定位链；全新起播照常定位起播帧
        if (!resumeFromPause)
            postRender->seekFrame(curFi, mxr::FrameChangeSource::PlayAdvance);
        //【F-1 修复·unify-anim-playback】TransientAnimation 模式前置：playTick 瞬态分支
        //以 _mode==TransientAnimation 为门——新会话 _mode 初值 StaticFrame，无前置则引擎
        //档 playTick 双分支都不命中（当前帧账面空转、画面不动、到端点即假停机）。幂等
        //（同值再设=零副作用）；先例=预生成恢复路径 mPostAnimation.cpp:939 同款
        if (postRender->displayMode() != mxr::DisplayMode::TransientAnimation)
            postRender->setDisplayMode(mxr::DisplayMode::TransientAnimation);
        postRender->play();
    }

    //引擎合一（2026-11-16）：浮层引擎态在播→pausePlayback 停播（预生成动画链互斥入口，
    //动画面板 slot_PlayAnimation 经 cloudResultPanel 调用）。pausePlayback→toggled(false)
    //→本面板引擎停机分支（会话 pause）+停机点补发链照常走
    void mPost3DResultPanel::stopRealtimeEngineIfActive()
    {
        if (_realtimeAnimWidget != nullptr && _realtimeAnimWidget->enginePlayback())
            _realtimeAnimWidget->pausePlayback();
    }

    // 引擎档播放中设置即时下发（引擎合一 2026-11-16）：浮层 engineSettingsChanged
    // → 会话现值重下发（setFrameRate 播放中重启定时器=速度即调即生效；方向/步长/
    // 循环下一 tick 生效；范围重设 playTick 回绕域）。
    //【unify-anim-playback】改往返不再降档（原"自动暂停转浮层自驱档"分支随 Q2=B
    //移除）：setPingPong 参数下发即时生效，推进至端点即换向（FR-05 验收③）
    void mPost3DResultPanel::reconfigureRealtimeEngine()
    {
        if (_realtimeAnimWidget == nullptr || !_realtimeAnimWidget->enginePlayback())
            return;
        const auto& rtSettings = _realtimeAnimWidget->settings();
        REBUILD_PostRend* rend = getCVM->getPostRenderByViewName(_curViewName);
        if (rend == nullptr || rend->getPostRender() == nullptr) return;
        auto postRender = rend->getPostRender();
        postRender->setFrameRate(qMax(1, static_cast<int>(rtSettings.fps)));
        postRender->setLoop(rtSettings.playStyle == mPostRealtimeAnimWidget::PlayStyle::Loop);
        //播放方式三态映射（Once=(false,false)/Loop=(true,false)/PingPong=(false,true)）；
        //往返中改循环→下一端点按回绕不再往返（playTick 端点分支按现值分流，字段即改即生效）
        postRender->setPingPong(rtSettings.playStyle == mPostRealtimeAnimWidget::PlayStyle::PingPong);
        postRender->setPlaybackDirection(_realtimeAnimWidget->direction());
        postRender->setFrameInterval(static_cast<int>(rtSettings.stepSize));
        //范围块限 Frame 模式：参数化域恒 1..20 无范围语义（02 §3.3；W2 时间域映射
        //只作用于帧形态帧序域）
        if (_realtimeAnimWidget->mode() == mPostRealtimeAnimWidget::AnimMode::Frame)
        {
            const auto domain = _realtimeAnimWidget->frameDomain();
            if (domain.valid)
            {
                QPointer<mPostDataBase> engDb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(rend->getPostDataName());
                if (engDb != nullptr)
                {
                    const QString engCase = _comboCase->currentText();
                    const QString loText = _comboFrame->itemText(domain.lo);
                    const QString loName = loText.contains(":") ? loText.split(":").at(0) : loText;
                    const QString hiText = _comboFrame->itemText(domain.hi);
                    const QString hiName = hiText.contains(":") ? hiText.split(":").at(0) : hiText;
                    const int firstFi = engDb->getFrameNumByCaseAndFrameName(engCase, loName);
                    const int lastFi = engDb->getFrameNumByCaseAndFrameName(engCase, hiName);
                    if (firstFi > 0 && lastFi > 0)
                        postRender->setPlayRange(firstFi, lastFi);
                }
            }
        }
    }

    //实时动画 v1.1 插值档变化（unify-anim-playback 职能改造）：录制互斥+反向闸（归属
    //豁免）+懒生成判据保留（暂停态定位前置——引擎档播放承载已前移到分诊
    //ensureParametricEnginePlayback）；插值直驱退役（GUI 双份公式消除）改会话单点
    //positionInterpolation 静态定位（复用 setAnimationFrameValues 同式公式，GUI 层
    //setInterpolationParam 调用归零——01 FR-02 验收③口径）
    void mPost3DResultPanel::slot_realtimeInterpolationChanged(int step)
    {
        if (_realtimeAnimWidget == nullptr) return;
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;
        auto postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        //录制互斥（同帧模式反向闸）：录制独占期间吞掉插值档变化并暂停
        if (mPostInfoHub::getInstance()->isRecordingActive())
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, "录制进行中，实时动画已暂停！", true);
            _realtimeAnimWidget->pausePlayback();
            return;
        }
        //预生成动画互斥（反向）：会话播放器在播且归属=预生成 → 实时自暂停
        //【unify-anim-playback 归属豁免】浮层引擎档在播时本槽经滑条松手提交到达，
        //Playing 是自身播放而非预生成冲突——裸判自阻（FR-04 边界必测项）
        if (postRender->playbackState() == mxr::PlaybackState::Playing
            && !_realtimeAnimWidget->enginePlayback())
        {
            _realtimeAnimWidget->pausePlayback();
            return;
        }

        //懒生成（暂停态/停止态定位前置）：参数化形态不存在或类型与当前形态不符（线性↔
        //正弦切换后）→ 按形态重新生成（生成即就绪，毫秒级；"先停后生成"内建；boundProxy
        //门=云图未显示拒绝生成，fail-soft 返 false 不清帧表不清绑定）
        const int mode = static_cast<int>(_realtimeAnimWidget->mode());
        const mxr::AnimationParams::Type type = (mode == static_cast<int>(mPostRealtimeAnimWidget::AnimMode::Linear))
            ? mxr::AnimationParams::Type::Linear
            : mxr::AnimationParams::Type::Sinusoidal;
        if (postRender->displayMode() != mxr::DisplayMode::ParametricAnimation
            || postRender->lastAnimationParams().type != type)
        {
            mxr::AnimationParams params;
            params.type = type;
            params.startFrame = 1;
            params.endFrame = mPostRealtimeAnimWidget::kParamStepCount;	//插值档位数（=动画面板 totalf）
            params.frameRate = 10;	//落账仅为参数快照一致性（推进周期由参数下发 setFrameRate 承担）
            //引擎 AnimationParams::loop 映射（04 T-1/J-01）：播放方式=循环→true，
            //单次/往返→false（往返语义由 setPingPong 参数下发承担）
            params.loop = (_realtimeAnimWidget->settings().playStyle
                == mPostRealtimeAnimWidget::PlayStyle::Loop);
            if (!postRender->generateAnimation(params))
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, "当前无云图数据，无法生成参数化动画！", true);
                _realtimeAnimWidget->pausePlayback();
                return;
            }
        }

        //静态定位路由（unify-anim-playback）：
        //引擎档播放中提交（滑条松手）=pause+定位+play 单事件内三连（无 tick 间隙，02
        //§3.2 "静态定位+play 续推"；positionInterpolation 带 Playing 拒绝门故必在 pause
        //之后；pause 对 Paused 幂等——滑条按下侧推进停役后松手路径重入安全）
        if (_realtimeAnimWidget->enginePlayback())
        {
            postRender->pause();
            postRender->positionInterpolation(step);
            postRender->play();
            return;
        }
        //暂停态/停止态定位（滑条松手提交/步进钮）：单次静态插值定位，不启动播放
        //（定位入口不发射——GUI 本地镜像由 applyStep 已更新）
        postRender->positionInterpolation(step);
    }

    //参数化形态（线性/正弦）引擎档承载（unify-anim-playback 2026-11-17）：点播放同步
    //路径调用（分诊解除面路由）。恒引擎档：生成即就绪（同步体毫秒级）、插值单条目不落
    //帧表，无"值未全物化"前提（02 §3.4），唯一不进播放前置=懒生成被无数据门拒绝
    //（01 §6-1）。序=懒生成（原 slot_realtimeInterpolationChanged 首 tick 链搬迁，时机
    //前移到点播放——首播延迟只早不晚）→参数下发→档位域尾端检查→enterEnginePlayback
    //→定位→play()
    void mPost3DResultPanel::ensureParametricEnginePlayback(mxr::REBUILD_PostRender* postRender)
    {
        if (postRender == nullptr || _realtimeAnimWidget == nullptr) return;
        const auto& rtSettings = _realtimeAnimWidget->settings();

        //懒生成：参数化形态不存在或类型与当前形态不符（线性↔正弦切换后）→ 按形态
        //重新生成（生成即就绪，毫秒级；"先停后生成"内建；boundProxy 门=云图未显示
        //拒绝生成，fail-soft 返 false 不清帧表不清绑定）
        const int mode = static_cast<int>(_realtimeAnimWidget->mode());
        const mxr::AnimationParams::Type type = (mode == static_cast<int>(mPostRealtimeAnimWidget::AnimMode::Linear))
            ? mxr::AnimationParams::Type::Linear
            : mxr::AnimationParams::Type::Sinusoidal;
        //【B-01 修复】显式新播标记：generateAnimation"先停后生成"经 pause() 落 Paused
        //而非 Stopped（R8b-07 pause 精度语义，session :1261-1268）——懒生成发生后
        //playbackState==Paused 不再是"浮层引擎档暂停残留"（暂停态/播放中切型再播、帧档
        //暂停切参数化等场景全部误判恢复态，反向+Once 首播即停/反向+PingPong 方向性错误
        //——审计 B-01）。生成动作本身即宣告非续推：本次发生懒生成→强制全新起播
        bool freshGenerated = false;
        if (postRender->displayMode() != mxr::DisplayMode::ParametricAnimation
            || postRender->lastAnimationParams().type != type)
        {
            freshGenerated = true;
            mxr::AnimationParams params;
            params.type = type;
            params.startFrame = 1;
            params.endFrame = mPostRealtimeAnimWidget::kParamStepCount;	//插值档位数（=动画面板 totalf）
            params.frameRate = 10;	//落账仅为参数快照一致性（推进周期由下行 setFrameRate 承担）
            params.loop = (rtSettings.playStyle == mPostRealtimeAnimWidget::PlayStyle::Loop);
            if (!postRender->generateAnimation(params))
            {
                //无数据门拒绝（01 §6-1 边界）：现状文案+pausePlayback 复位播放钮，
                //不入播放态（enterEnginePlayback 未落账，归属零污染）
                mGlobalSignals::getInstance()->outputMessageSig(2, "当前无云图数据，无法生成参数化动画！", true);
                _realtimeAnimWidget->pausePlayback();
                return;
            }
        }

        //参数下发（序对齐帧模式引擎档承载段）：播放方式三态映射 Once=(false,false)/
        //Loop=(true,false)/PingPong=(false,true)；setFrameRate/setFrameInterval/
        //setPlayRange 落账在 play() 前，下一 tick 即按新参数推进
        _realtimeAnimWidget->enterEnginePlayback();
        _realtimeEngineActive = true;
        postRender->setFrameRate(qMax(1, static_cast<int>(rtSettings.fps)));
        postRender->setLoop(rtSettings.playStyle == mPostRealtimeAnimWidget::PlayStyle::Loop);
        postRender->setPingPong(rtSettings.playStyle == mPostRealtimeAnimWidget::PlayStyle::PingPong);
        postRender->setPlaybackDirection(_realtimeAnimWidget->direction());
        postRender->setFrameInterval(static_cast<int>(rtSettings.stepSize));
        postRender->setPlayRange(1, mPostRealtimeAnimWidget::kParamStepCount);
        //【A-1 修复+B-01 修复·unify-anim-playback】暂停恢复态判定：playbackState==Paused
        //且本次未发生懒生成（freshGenerated 否证——"先停后生成"实际落 Paused 而非
        //Stopped，见上方 B-01 注释）——FR-06③ 口径=按会话账面（_aniCurrentFrame/
        //_aniDirection 全在会话落账）原样续推，尾端不跳对端、不重定位（PingPong 端点
        //暂停→续推由 playTick 端点分支翻向，Loop 端点恢复=首 tick 原生回绕等价，Once
        //端点恢复=按字面语义自然停机）。浮层镜像播放期逐 tick 回流、暂停态滑条提交经
        //静态定位路由同步账面，恢复时 step/_direction 与会话账面恒一致，无重定位必要。
        //残余知悉面（审计 B-04/维度C 维持判定）：预生成参数化暂停后浮层同型点播=接管
        //续推（懒生成跳过、Paused、无浮层暂停史），为第 2 轮观察-3 择期收紧项非本修复面
        const bool resumeFromPause = (!freshGenerated
            && postRender->playbackState() == mxr::PlaybackState::Paused);
        //档位域起播方向尾端检查复刻（帧模式承载段同式，域=1..kParamStepCount；仅全新
        //起播生效——"首次点播放"口径，恢复态跳过见上）：正向在终点/反向在起点→先跳
        //对端起点再播（否则首 tick 即回绕/换向/自停=按下无反应）
        int curStep = _realtimeAnimWidget->step();
        if (!resumeFromPause)
        {
            if (_realtimeAnimWidget->direction() > 0 && curStep >= mPostRealtimeAnimWidget::kParamStepCount)
                curStep = 1;
            else if (_realtimeAnimWidget->direction() < 0 && curStep <= 1)
                curStep = mPostRealtimeAnimWidget::kParamStepCount;
            //档位定位（positionInterpolation：clamp+写账面+按 _postMode 单点重算插值，
            //不发射——GUI 本地镜像由 applyStep 维护；仅全新起播，恢复态账面即权威）
            postRender->positionInterpolation(curStep);
        }
        //起播（enterEnginePlayback 已停自驱表，滑条 applyPlayStart 尾端跳被 !isActive
        //守卫跳过，全新起播尾端检查由上方承担；恢复续推由 playTick 按账面方向推进）
        postRender->play();
    }

    //实时动画 v1.1：浮层形态切换——浮层侧已静默停播放并完成控件语义换挡，此处仅做
    //渲染侧收尾：回帧模式时退出浮层自生成的参数化形态（stop→StaticFrame，当前帧=
    //停止时帧保留，帧走查行为与 v1 完全一致）；会话播放器在播（预生成互斥反向态）
    //不动渲染（画面归预生成动画，浮层已被 tick 闸拦停）。进参数化形态无渲染动作
    //（首个插值档懒生成）
    void mPost3DResultPanel::slot_realtimeModeChanged(int mode)
    {
        if (_realtimeAnimWidget == nullptr) return;
        //【引擎合一】帧形态引擎态在播 → 形态换挡=用户主动语义，会话播放器 pause 保留
        //当前帧（widget quietStop 已清引擎态；重播=面板重新分诊）。
        //【unify-anim-playback】本收口形态无关：参数化引擎档同链（enginePlayback 门+
        //_realtimeEngineActive 清位+会话 pause）
        if (_realtimeEngineActive)
        {
            _realtimeEngineActive = false;
            REBUILD_PostRend* modeRend = getCVM->getPostRenderByViewName(_curViewName);
            if (modeRend != nullptr && modeRend->getPostRender() != nullptr)
                modeRend->getPostRender()->pause();
        }
        if (mode != static_cast<int>(mPostRealtimeAnimWidget::AnimMode::Frame)) return;
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;
        auto postRender = postRend->getPostRender();
        if (postRender == nullptr) return;
        if (postRender->displayMode() == mxr::DisplayMode::ParametricAnimation
            && postRender->playbackState() != mxr::PlaybackState::Playing)
            postRender->stop();
    }

    //---- FR-13 未变形构形真接入（D-05 升格，渲染侧三件+槽）----
    //构形置零（未变形档渲染写）：既定系数记录=不动 guidata（变形面板权威值零改动，
    //恢复时按其现值走 applyPostDeformation 权威链）；写门面 setDeformationScale(0,0,0)
    //（h:188，会话单副本+全实例扇出）+当前帧 queryProxy 同步置零（置零先例
    //mPostDeformation.cpp:92-93 同款双写）；deformationChanged 广播（h:457）由门面
    //照常触发，测量标注等消费方自动跟随。OSG 按需渲染，纯状态写入后尾补 updatePaint()
    //（F1 口径，slot_realtimeInterpolationChanged 同款惯例先例 mPostDeformation.cpp:191）
    void mPost3DResultPanel::applyRealtimeUndeformedShape(REBUILD_PostRend* postRend)
    {
        if (postRend == nullptr)
            return;
        auto postRender = postRend->getPostRender();
        if (postRender == nullptr)
            return;
        if (auto dataProxy = postRender->frameProxy(postRender->currentFrame()))
        {
            if (auto queryProxy = dynamic_cast<mxr::IPostRendQueryDataProxy*>(dataProxy->getRendQueryDataProxy()))
                queryProxy->setDeformationScale(QVector3D(0, 0, 0));
        }
        postRender->setDeformationScale(QVector3D(0, 0, 0));
        postRend->updatePaint();
    }

    //恢复既定系数（切回"变形后"/退出工具栏/结果切换/面板析构统一出口）：走变形控制
    //权威链 applyPostDeformation（既定系数=guidata->_deformFactor 现值+缩放策略/初始
    //构型档/图例重算全套照常，updatePaint 内建）+清武装
    void mPost3DResultPanel::restoreRealtimeDeformedShape(REBUILD_PostRend* postRend)
    {
        _undeformedArmedRend.clear();
        if (postRend == nullptr)
            return;
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr)
            return;
        applyPostDeformation(postRend, guidata);
    }

    //云图重绘链重断言：applyPostDeformation 每次重绘按既定系数覆盖渲染态（slot_showCloud/
    //渐进装载补绘两处调用点），未变形武装期须紧随其后重写置零，否则首次切帧/重绘即失效；
    //非武装态/会话已换（QPointer 失效）/构形会话已回"变形后"→零动作（两处调用点插入
    //恒为一次指针比较，重绘链零回归）
    void mPost3DResultPanel::reassertRealtimeUndeformedShape(REBUILD_PostRend* postRend)
    {
        if (_undeformedArmedRend != postRend)
            return;
        if (_realtimeAnimWidget == nullptr
            || _realtimeAnimWidget->settings().shape != mPostRealtimeAnimWidget::ShapeMode::Undeformed)
            return;
        applyRealtimeUndeformedShape(postRend);
    }

    //构形切换渲染收口（控件条 shapeChanged，D-05 升格 FR-13）：未变形=置零武装；
    //变形后=恢复既定系数+清武装。纯状态写入不触播放链（录制 playbackState=Stopped 期/
    //预生成互斥组合均不受扰——与四路协议面板收口同构，互斥闸不在此路径）
    void mPost3DResultPanel::slot_realtimeShapeChanged(int mode)
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr)
            return;
        if (mode == static_cast<int>(mPostRealtimeAnimWidget::ShapeMode::Undeformed))
        {
            _undeformedArmedRend = postRend;
            applyRealtimeUndeformedShape(postRend);
        }
        else
            restoreRealtimeDeformedShape(postRend);
    }

    //实时动画 v1.1：录制发起广播接收端——播放中的浮层按 v1 停止语义让路自停（停表+
    //回起点），画面与逐帧控制权完整让给录制链。广播点在录制入口引擎调用之前且先于
    //录制簿记置位（直连同步执行）：让路切帧不受反向闸拦截，也不会落在录制逐帧走查中途；
    //暂停态浮层不动（用户手动操作由 tick 闸兜底拦停）
    void mPost3DResultPanel::slot_realtimeRecordingStarted()
    {
        if (_realtimeAnimWidget == nullptr) return;
        if (!_realtimeAnimWidget->isPlaying()) return;
        //P6-1 审计修复：让路改 yieldForRecording——仅停表+浮层视觉归位（不发
        //frameIndexChanged）。旧 stopPlayback 无条件回帧 0（emit=true）触发切帧全链：
        //帧 0 未装载时兜底后台装载的 frameReady 补绘落在录制两步走查之间重绑帧 0
        //（成片错帧）；配合两处 frameReady 回调的 isRecordingActive 守卫（第二道）收口。
        //停止钮"停止回起点"全链语义不变（stopPlayback 保留）
        _realtimeAnimWidget->yieldForRecording();
    }

    void mPost3DResultPanel::slot_planishToggled(bool checked)
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;

        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return;

        guidata->_cloudSmooth = checked;
        slot_showCloud();
    }

    void mPost3DResultPanel::slot_planishIndexChanged(int index)
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;

        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return;

        guidata->_cloudSmooth = (index == 0);
        slot_showCloud();
    }

    void mPost3DResultPanel::updatePlanishEnabled()
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;

        QString postName = postRend->getPostDataName();
        mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        if (postDB == nullptr) return;

        QString varName = _comboVariable->currentText();
        if (varName.isEmpty()) return;

        int varType = postDB->getVariableType(varName);
    }

    void mPost3DResultPanel::updateResultInfoGUI()
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr) return;

        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        QString postname = postRend->getPostDataName();
        mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
        if (postDB == nullptr)
            return;

        //获取GUI数据
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr)
            return;

        _comboCase->blockSignals(true);
        _comboVariable->blockSignals(true);
        _comboComplex->blockSignals(true);
        _comboFrame->blockSignals(true);
        _comboComponent->blockSignals(true);
        _comboLayer->blockSignals(true);
        _comboRangeMode->blockSignals(true);
        _legendMax->blockSignals(true);     //图例边界行：visibilityToggled/resetClicked 随回填屏蔽
        _legendMin->blockSignals(true);
        _interpSmoothChoice->blockSignals(true);
        _spinOrder->blockSignals(true);
        _interpSpacingChoice->blockSignals(true);
        _lineEditX->blockSignals(true);
        _lineEditY->blockSignals(true);
        _lineEditZ->blockSignals(true);
        _sliderDiffuse->blockSignals(true);
        _sliderAmbient->blockSignals(true);
        _sliderMirror->blockSignals(true);
        _checkCornerData->blockSignals(true);
        _editInlineDeformCoef->blockSignals(true);
        _objectTypeCombo->blockSignals(true);
        _comboCoordSystem->blockSignals(true);
        _checkTrackingCoord->blockSignals(true);
        _checkMidsideValues->blockSignals(true);
        _comboSmoothMethod->blockSignals(true);
        _checkAcrossParts->blockSignals(true);
        _checkVariationEnabled->blockSignals(true);
        _editVariationLimit->blockSignals(true);
        _filterRow->blockSignals(true);     //三段行：modeChanged 随回填屏蔽（启用规则内部仍生效）
        _legendRangeChoice->blockSignals(true);

        _comboCase->clear();
        //派生工况拼接：guidata 缓存可能早于生成时刻（其它视口），postDB 现查补齐
        QStringList caseItems = guidata->_cloudCases;
        const QStringList derivedCaseItems = postDB->getDerivedCaseNames();
        for (const QString& derived : derivedCaseItems)
        {
            if (!caseItems.contains(derived))
                caseItems.append(derived);
        }
        _comboCase->addItems(caseItems);
        _comboCase->setCurrentText(guidata->_cloudCase);
        _comboVariable->clear();
        _comboVariable->addItems(guidata->_cloudVarNames);
        _comboVariable->setCurrentText(guidata->_cloudVarName);
        _comboComplex->setCurrentText(guidata->_cloudComplex);
        _comboComplex->setDisabled(!guidata->_cloudHaveComplex);
        _comboFrame->clear();
        _comboFrame->addItems(guidata->_cloudFrames);
        _comboFrame->setCurrentIndex(guidata->_cloudFrameIndex);
        //实时动画 v1.2（观察项 A 驱动点扩展+setCurViewName 跨视口语义接续）→ v1.4 常显：
        //帧表重建→单动源互斥旗置回+挂载/禁用域/帧范围/帧时间轴刷新（无结果/单帧停播+
        //禁用态，工具栏不消失，D-08）。内部 _comboFrame 对齐走"取回旧态恢复"式
        //blockSignals，不解除本函数外层阻塞
        _animDeletePending = true;
        updateRealtimeAnimEnabled();

        _comboComponent->clear();
        _comboComponent->addItems(guidata->_cloudCompos);
        _comboComponent->setCurrentText(guidata->_cloudCompo);
        _comboLayer->clear();
        _comboLayer->addItems(guidata->_cloudLayerNames);
        _comboLayer->setCurrentText(guidata->_cloudLayerName);
        _comboLayer->setDisabled(!guidata->_cloudHaveLayer);
        int index = guidata->_rangeIndex;
        _comboRangeMode->setCurrentIndex(index);
        //图例边界行回填：值域文本 + 眼睛显隐（编辑/复位启停随图例范围档统一刷新，
        //见下方 _legendRangeChoice 回填后的 refreshLegendBoundEditState）
        _legendMax->lineEdit()->setText(MxFunctions::mNumberToString(guidata->_cloudMaxValue, 'f', 12));
        _legendMin->lineEdit()->setText(MxFunctions::mNumberToString(guidata->_cloudMinValue, 'f', 12));
        _legendMax->setValueVisible(guidata->_cloudShowMaxLine);
        _legendMin->setValueVisible(guidata->_cloudShowMinLine);
        _interpSmoothChoice->setCurrentIndex(guidata->_cloudDiscrete);
        _interpSpacingChoice->setCurrentIndex(guidata->_cloudIsEquivariance);
        _spinOrder->setValue(guidata->_cloudDiscreteNum);

        //云图变量：角点数据 + 内联变形系数（等比例取 X 分量）
        _checkCornerData->setChecked(guidata->_useCornerData);
        QVector3D deformFactor = guidata->_deformFactor;
        if (deformFactor.isNull())
            deformFactor = QVector3D(1, 1, 1);
        _editInlineDeformCoef->setText(MxFunctions::mNumberToString(deformFactor.x(), 'f', 6));

        //对象及坐标系（渲染TODO，仅回填）；类型下拉回填后联动范围控制器与拾取条默认文案。
        //2026-11-18 口径：范围选项弹窗已下线，回填恒回默认"所有X"（旧会话持久化的
        //"通过材料"等范围文本不再回显）；拾取数据为进程态，回填/切 case 不带旧上下文
        _objectTypeCombo->setCurrentText(guidata->_renderObjectType);
        _btnObjectScope->setObjectType(_objectTypeCombo->currentIndex() == 0 ? 0 : 1);
        _btnObjectScope->setScope(1);
        if (_objectPickController != nullptr)
        {
            _objectPickController->displayData(PickWidgetData{});
            _objectPickController->setDefultTip(_btnObjectScope->scopeText());
        }
        _comboCoordSystem->clear();
        _comboCoordSystem->addItem("全局坐标系");
        for (const QString& name : guidata->_cloudCoordinateNames)
        {
            if (name != QString("全局坐标系"))
                _comboCoordSystem->addItem(name);
        }
        if (!guidata->_renderCoordSystem.isEmpty())
            _comboCoordSystem->setCurrentText(guidata->_renderCoordSystem);
        _checkTrackingCoord->setChecked(guidata->_useTrackingCoord);
        _checkMidsideValues->setChecked(guidata->_midsideValues);

        //数据处理
        if (!guidata->_smoothMethod.isEmpty())
            _comboSmoothMethod->setCurrentText(guidata->_smoothMethod);
        else
            _comboSmoothMethod->setCurrentIndex(0);
        //跨部件：磨平方式=无时禁用但保留勾选（同原型；启用条件与 slot_chooseSmoothMethod 同口径）
        _checkAcrossParts->setEnabled(_comboSmoothMethod->currentText() != QString("无"));
        _checkAcrossParts->setChecked(guidata->_smoothAcrossParts);
        if (_planishFold != nullptr && _variationRow != nullptr)
            _planishFold->setRowVisible(_variationRow, _comboSmoothMethod->currentText() == QString("高级磨平"));
        _checkVariationEnabled->setChecked(guidata->_smoothVariationEnabled);
        _editVariationLimit->setText(MxFunctions::mNumberToString(guidata->_smoothVariationLimit, 'f', 6));
        _editVariationLimit->setEnabled(guidata->_smoothVariationEnabled);
        //数值过滤回填（V1.3 三段行）：模式按持久化规则映射（大于/小于/之间；
        //旧会话"等于"等未收录规则回退"无"），左右值取持久化边界（未开启过滤保持现值）
        if (guidata->_filterOpen && guidata->_filterRule == QString("大于"))
            _filterRow->setMode(MxFunctions::mValueFilterRow::Greater);
        else if (guidata->_filterOpen && guidata->_filterRule == QString("小于"))
            _filterRow->setMode(MxFunctions::mValueFilterRow::Less);
        else if (guidata->_filterOpen && guidata->_filterRule == QString("之间"))
            _filterRow->setMode(MxFunctions::mValueFilterRow::Between);
        else
            _filterRow->setMode(MxFunctions::mValueFilterRow::None);
        if (guidata->_filterOpen)
        {
            _filterRow->setLeftText(MxFunctions::mNumberToString(guidata->_filterMin, 'f', 6));
            _filterRow->setRightText(MxFunctions::mNumberToString(guidata->_filterMax, 'f', 6));
        }

        //图例范围（渲染TODO，仅回填；共框单选组按文本定位）
        _legendRangeChoice->setCurrentText(guidata->_legendRange.isEmpty()
            ? QString("逐帧") : guidata->_legendRange);
        //上下限编辑/复位启停随回填档位统一刷新（回填不触发切换槽，需显式调）
        refreshLegendBoundEditState();

        //信息标签按钮状态跟随当前视口（同渲染图例：隐藏态按钮变"显示"并禁用其余；
        //setText/setDisabled 不触发 clicked，无需 blockSignals；用户定案保留该功能）
        _btnLabelHide->setText(guidata->_cloudInfoLabelVisible ? QString("隐藏") : QString("显示"));
        _btnLabelReset->setDisabled(!guidata->_cloudInfoLabelVisible);
        _btnLabelCustom->setDisabled(!guidata->_cloudInfoLabelVisible);

        this->updateInformation();

        _caseInfos = guidata->_caseInfos;

        const QStringList parts = guidata->_lightPos.split(',');
        if (parts.size() == 3)
        {
            _lineEditX->setText(parts.at(0));
            _lineEditY->setText(parts.at(1));
            _lineEditZ->setText(parts.at(2));
        }

        if (guidata->_lightType == 1)
        {
            _comboLightPos->setCurrentIndex(0);
            _lineEditX->setEnabled(false);
            _lineEditY->setEnabled(false);
            _lineEditZ->setEnabled(false);
        }
        else
        {
            _comboLightPos->setCurrentIndex(1);
            _lineEditX->setEnabled(true);
            _lineEditY->setEnabled(true);
            _lineEditZ->setEnabled(true);
        }
        _sliderAmbient->setValue(guidata->_lightAmbient);
        _sliderDiffuse->setValue(guidata->_lightDiffuse);
        _sliderMirror->setValue(guidata->_lightMirror);

        _comboCase->blockSignals(false);
        _comboVariable->blockSignals(false);
        _comboComplex->blockSignals(false);
        _comboFrame->blockSignals(false);
        _comboComponent->blockSignals(false);
        _comboLayer->blockSignals(false);
        _comboRangeMode->blockSignals(false);
        _legendMax->blockSignals(false);
        _legendMin->blockSignals(false);
        _interpSmoothChoice->blockSignals(false);
        _spinOrder->blockSignals(false);
        _interpSpacingChoice->blockSignals(false);
        _lineEditX->blockSignals(false);
        _lineEditY->blockSignals(false);
        _lineEditZ->blockSignals(false);
        _sliderDiffuse->blockSignals(false);
        _sliderAmbient->blockSignals(false);
        _sliderMirror->blockSignals(false);
        _checkCornerData->blockSignals(false);
        _editInlineDeformCoef->blockSignals(false);
        _objectTypeCombo->blockSignals(false);
        _comboCoordSystem->blockSignals(false);
        _checkTrackingCoord->blockSignals(false);
        _checkMidsideValues->blockSignals(false);
        _comboSmoothMethod->blockSignals(false);
        _checkAcrossParts->blockSignals(false);
        _checkVariationEnabled->blockSignals(false);
        _editVariationLimit->blockSignals(false);
        _filterRow->blockSignals(false);
        _legendRangeChoice->blockSignals(false);

        //回填后面板与 guidata 对齐：未应用的修改随之丢弃（回填语义，同旧版"改了没回车"），清"待应用"提示。
        //离散三件套同样清位：guidata 可能是未动过控件的默认值，重放会覆盖该视口渲染
        //会话默认态——恢复链的离散回填不生效是与旧版一致的既有缺口，不在本次范围
        _rangeEditPending = false;
        _discreteEditPending = false;
        setCloudDirty(false);
    }

    void mPost3DResultPanel::applyResultToViewports(const QStringList& views, const QString& postname)
    {
        updateResultView(views, postname);
    }

    void mPost3DResultPanel::updateResultView(QStringList views, QString postname)
    {
        //先单遍抓稳全部渲染器再统一应用：clearViewport3D 会 take+insert _viewports
        //并销毁旧私有数据，边迭代边应用会使后续按名查找落在被重组的哈希上
        //（表现为查不到渲染器/迭代失效），故指针采集与应用两阶段分离
        QList<QPair<QString, REBUILD_PostRend*>> rends;
        const QList<QWidget*> subs = getCVM->getSubWindows();
        for (QWidget* w : subs)
        {
            auto* p = getCVM->getViewportPrivate(w);
            if (p == nullptr) continue;
            auto* rend = dynamic_cast<REBUILD_PostRend*>(p->getRendWidget());
            if (rend != nullptr)
            {
                rends.append({ p->getViewportName(), rend });
            }
        }
        for (int i = 0; i < views.size(); i++)
        {
            REBUILD_PostRend* postRend = nullptr;
            for (const auto& pair : rends)
            {
                if (pair.first == views[i]) { postRend = pair.second; break; }
            }
            if (postRend != nullptr)
            {
                //在视口部件原生的渲染器上绑定库名并经面板级联刷新。不得走
                //clearViewport3D：它会以裸 new REBUILD_PostRend 替换私有数据中的
                //渲染器，新对象未接入视口部件的显示表面，后续级联全部落在不可见
                //的悬空渲染器上（视口空白）；该方法其余调用点均已废弃（同因）
                postRend->setPostDataName(postname);
                update3DResultPanel(views[i]);
            }
        }
    }

    void mPost3DResultPanel::setDefaultDiscreteOrder(int order)
    {
        _spinOrder->setValue(order);
    }

    void mPost3DResultPanel::slot_changedFrame(int f)
    {
        //MXR_LOG_ERROR("FrameChanged %d", f);
        REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderByViewName(_curViewName);
        if (rebuildPostRend == nullptr) return;

        auto  postRender = rebuildPostRend->getPostRender();
        if (postRender == nullptr) return;

        QString postName = rebuildPostRend->getPostDataName();

        postRender->updateAllModelOperate(ShowSolidColor);

        //获取GUI数据
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(rebuildPostRend);
        if (guidata == nullptr) return;

        //存储工况
        guidata->_cloudCase = _comboCase->currentText();
        //获取帧
        int fi = f;
        bool isNew = false;
        //捕获上一帧号（用于失效单元增量显隐对比），在覆盖 _cloudFrame 之前取值
        int lastFi = guidata->_cloudFrame;
        if (fi != guidata->_cloudFrame)
            isNew = true;
        guidata->_cloudFrame = fi;
        //获取变量
        QString varstr = _comboVariable->currentText();
        guidata->_cloudVarName = varstr;
        //伪变量SolidColor：不读取结果数据，按部件自身颜色显示
        const bool isSolidColor = (varstr == QString("SolidColor"));

        //获取分量
        QString compostr = _comboComponent->currentText();
        guidata->_cloudCompo = compostr;
        //获取层
        QString layerstr = _comboLayer->currentText();
        guidata->_cloudLayerName = layerstr;
        //获取复数
        bool isdisp = false;
        QString complexstr;
        if (_comboComplex->isEnabled())
        {
            complexstr = _comboComplex->currentText();

            if (guidata->_cloudComplex != complexstr)
                isdisp = true;
        }
        guidata->_cloudComplex = complexstr;
        //根据是否切换帧获取数据

        //获取后处理数据库名称
        QString postname = rebuildPostRend->getPostDataName();

        QString coordinateName = QString("Global");
        QVector<double> userCoord{};

        ////显示
        QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);

        if (modeldb == nullptr)
            return;

        QString postPathName = modeldb->getFile();

        // S6 读数缓存命中门（段c，同 slot_showCloud 三重门口径，无磨平维度——
        // 本槽读数级联无 planish 入口）：同帧回看/切帧回跳直复用既有 _resultDatas
        // 表（同帧表键），首次读照旧；失败路径早退不登记
        const bool _readCacheEligible = !isSolidColor;
        const bool _readBranchNative = _readCacheEligible && isLazyNativeResult(postPathName);
        const bool _readBranchVtk = _readCacheEligible && !_readBranchNative && isLazyVtkResult(postPathName);
        const bool _readBranchMxdbOld = _readCacheEligible && !_readBranchNative && !_readBranchVtk && modeldb->getDataFlag();
        const QString _readCacheKey = postPathName + QStringLiteral("|") + QString::number(fi);
        const QString _readIdentity = QStringLiteral("%1|%2|%3|%4|%5")
            .arg(_readBranchNative ? QStringLiteral("native")
                 : _readBranchVtk ? QStringLiteral("vtk")
                 : _readBranchMxdbOld ? QStringLiteral("mxdbold")
                 : QStringLiteral("thread"))
            .arg(varstr, compostr, complexstr, layerstr);
        bool readCacheHit = false;
        if (_readCacheEligible
            && _cloudReadIdentityByKey.value(_readCacheKey) == _readIdentity
            && modeldb->getFrameResultType(fi) >= 0
            && !modeldb->getResultDataByFrameID(fi).isEmpty()
            && modeldb->getResultVarNameByFrameID(fi) == expectedResultVarName(varstr, compostr))
        {
            readCacheHit = true;
            qDebug() << "[CloudFrame] readData(cache-hit) frame=" << fi << "var=" << varstr;
        }

        //派生工况帧（批次A1，同 slot_showCloud 守卫）：内存数据免文件读 + 变量守卫
        const bool derivedFrame = modeldb->isDerivedFrame(fi);
        if (derivedFrame && !isSolidColor
            && modeldb->getResultVarNameByFrameID(fi) != expectedResultVarName(varstr, compostr))
        {
            const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
            mGlobalSignals::getInstance()->outputMessageSig(2,
                isChinese ? QStringLiteral("派生工况仅包含生成变量 %1 的数据，无法切换到 %2。")
                                    .arg(modeldb->getResultVarNameByFrameID(fi))
                                    .arg(expectedResultVarName(varstr, compostr))
                          : QStringLiteral("The derived case only contains data of the generated variable %1; cannot switch to %2.")
                                    .arg(modeldb->getResultVarNameByFrameID(fi))
                                    .arg(expectedResultVarName(varstr, compostr)),
                true);
            return;
        }

        if (!isSolidColor && !readCacheHit && !derivedFrame)
        {
            //统一取值 provider（阶段2 二批）：四格式发布式分支收敛（面板级缓存/派生帧门在外）；
            //本变体无磨平链，四格式失败一律中止（失败早退不进读数身份登记）；
            //ElementNodalForce 特判在适配器内，老格式 reader 经面板池复用
            IPostValueProvider* provider = valueProviderFor(modeldb);
            ValueRequest req;
            req.frame = fi;
            req.var = varstr;
            req.compo = compostr;
            req.layer = layerstr;
            req.complexMode = complexstr;
            req.coordinateName = coordinateName;
            req.userCoord = userCoord;
            const bool readOk = provider != nullptr && provider->publish(req);
            const bool isNativeOrVtk = provider != nullptr
                && (provider->sourceKind() == IPostValueProvider::SourceKind::Native
                    || provider->sourceKind() == IPostValueProvider::SourceKind::Vtk);
            delete provider;
            if (!readOk)
            {
                if (isNativeOrVtk)
                {
                    // 静默返回会让"切变量无图例无颜色变化"无从排查，这里落到输出窗口
                    const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
                    mGlobalSignals::getInstance()->outputMessageSig(2,
                        isChinese ? QStringLiteral("读取结果分量失败：帧%1，变量%2，分量%3（%4），详见调试日志")
                                        .arg(fi).arg(varstr).arg(compostr).arg(postPathName)
                                  : QStringLiteral("Failed to read result component: frame %1, variable %2, component %3 (%4); see the debug log for details")
                                        .arg(fi).arg(varstr).arg(compostr).arg(postPathName),
                        true);
                }
                else
                {
                    report3DReadFailure(fi, varstr, compostr, postPathName);
                }
                return;
            }
        }

        // S6 读数身份登记（段c，同 slot_showCloud：失败路径早退不登记）
        if (!isSolidColor && !readCacheHit)
            _cloudReadIdentityByKey[_readCacheKey] = _readIdentity;

        //处理失效单元：对比上/当前帧失效集合，增量设置单元显隐；返回值标识是否有失效显隐变化
        bool hasFailedChange = processFailedElements(modeldb, rebuildPostRend, fi, lastFi);

        //变形变量非默认时按其矢量场覆盖位移哈希（须在建帧代理前：代理创建时物化位移数据）
        applyDeformVariableField(rebuildPostRend, guidata);

        // S5 门面直接替换：三连调残余收敛 attachModel+seekFrame（同链一口径）
        // S6 段a：探针代理省略（同 slot_showCloud 口径：工厂空判等价式现取替代，
        // 下游 hostedCur=frameProxy(currentFrame) 现取不受影响）
        const bool frameReadable = (modeldb->getModelData(fi) != nullptr);
        if (!frameReadable)
        {
            // S6 帧装载回归诊断（2026-11-09）：同 slot_showCloud 口径，静默 return
            // 改显式透出（帧模型数据导入期缺失，非装载中，勿死等 frameReady）
            mGlobalSignals::getInstance()->outputMessageSig(2,
                QString("帧 %1 模型数据未装载（导入期读取失败或被跳过），请重新导入并查看调试日志").arg(fi), true);
            return;
        }
        postRender->attachModel(modeldb.data(), rebuildPostRend->getViewportName());
        // S5 D14 数据纪元：同 slot_showCloud 口径（切换处理 notifyDataChanged 驱动惰性重验）
        // S5 首绑/同帧重绘必须显式绑定：emit=true 绕过同帧去重——异步装载完成后的重入
        // seek(fi) 会命中"同帧&&就绪→零动作"分支跳过 bind，实例保持隐藏=模型不显示；
        // 同帧变量切换也依赖此路径触发纪元重验（bind→revalidateIfStale 工厂重读新值）
        // 兜底与 slot_showCloud 的 !seekOk 兜底门同口径（2026-11-16 收敛至 armFrameLoadFallback 统一维护）
        const bool seekOk = postRender->seekFrame(fi, mxr::FrameChangeSource::UserSeek, true);
        if (!seekOk)
        {
            // 渐进装载兜底：同 slot_showCloud 口径，收敛至 armFrameLoadFallback 统一维护
            armFrameLoadFallback(fi, postRender.get());
            return;
        }
        {
            applyPostDeformation(rebuildPostRend, guidata);
        }
        //FR-13 未变形武装期重断言（渐进装载补绘同 slot_showCloud 口径，防切帧即失效）
        reassertRealtimeUndeformedShape(rebuildPostRend);

        //向量图跟随切帧：按 guidata 记录参数重读当前帧矢量场并重建（未应用/读场失败
        //保持既有箭头——切到无该变量数据的帧不闪断）
        refreshVectorPlotIfActive(rebuildPostRend, guidata);

        //截面剖切跟随切帧：随变形构型按当前帧位移场平移基点后重切（固定平面在世界系原位重切）
        refreshSectionCutIfActive(rebuildPostRend, guidata);

        QPointer<mPostModelData> modelData = modeldb->getModelData(fi);
        if (modelData == nullptr)
            return;

        mxr::PostFrameDataProxy* hostedCur = postRender->frameProxy(postRender->currentFrame());
        if (hostedCur == nullptr)
            return;   // 未装载帧：保持上一帧画面，不崩（装载完成重入见上方 !seekOk 兜底）
        mxr::IPostRendQueryDataProxy* query_proxy = dynamic_cast<mxr::IPostRendQueryDataProxy*>(hostedCur->getRendQueryDataProxy());
        if (query_proxy == nullptr)
            return;
        for (auto it : modeldb->getAllRigidWallDatas())
        {
            QString name = it->getRigidWallName();
            QVector3D endPos = modelData->_frameInfo.getRigidWallEndPos(name);
            it->updatePos(endPos);
            query_proxy->appendRigidWallData(it);
        }
        // B1b(R18-2①) 刚体墙替代重放（R22-H1 恢复）：append 数据错过实例构建期
        // ImportOperate，改走 GUI 可达替代入口 ReplaceOnePartOperate 逐墙重建
        // partRender（幂等覆盖；渲染库修复落地后改回单发
        // UpdateAllRigidWallOperate(ImportOperate)，移交依赖不变）
        {
            std::set<QString> appendedWallNames;
            for (auto it : modeldb->getAllRigidWallDatas())
                appendedWallNames.insert(it->getRigidWallName());
            if (!appendedWallNames.empty())
                // S5 G5（D9 载体）：RigidWall=登记型——寻/登台账 id 后 updateObject
                // ReplaceOnePartOperate 逐墙重建（query_proxy 现读已含 append 数据）
                PostCommon::updateRigidWallOperate(postRender.get(), (int)mxr::ReplaceOnePartOperate,
                    appendedWallNames);
        }

        // S5 门面直接替换：绑定已由上方 attachModel+seekFrame 消化（原外壳
        // setRendCurrentFrameData 手造代理注入路径随移除集消亡）

        //显示自由节点
        if (guidata->_cloudShowFreeNode && !guidata->_cloudDeleteFreeNode)
        {
            modeldb->showFreeNode(guidata->_cloudFrame, guidata->_cloudRelatedCloud);
            postRender->updateAllModelOperate(ShowFreeNode);
        }
        else if (!guidata->_cloudShowFreeNode)
        {
            modeldb->hideFreeNode(guidata->_cloudFrame);
            postRender->updateAllModelOperate(ClearFreeNode);
        }
        else if (guidata->_cloudDeleteFreeNode)
        {
            modeldb->deleteFreeNode(guidata->_cloudFrame);
            postRender->updateAllModelOperate(ClearFreeNode);
        }

#ifdef DEBUGRESULT
        QHash<int, double> result = modeldb->getResultDataByFrameID(fi);
        QList<int> keys = mViewportManager::getInstance()->getAllKeys(result);
        QVector<double> values = mViewportManager::getInstance()->getAllValues(result);
        QVector<double>::iterator max = std::max_element(std::begin(values), std::end(values));
        QVector<double>::iterator min = std::min_element(std::begin(values), std::end(values));
        double minValue = *min;
        double maxValue = *max;
#endif // DEBUG

        //TODO:数据读取完成,渲染窗口可接入变量数据
        // S5 F8 契约：托管代理现取+空判兜底（未装载帧保持上一帧，不崩）；判据键=目标帧 fi
        // （与 slot_showCloud 兜底门同口径，消除对 seek 后 currentFrame 时序的依赖）
        mxr::PostFrameDataProxy* hostedFrame = postRender->frameProxy(fi);
        if (hostedFrame == nullptr)
            return;
        mxr::IPostModelRenderDataProxy* model_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(hostedFrame->getModelRenderDataProxy());
        if (model_proxy != nullptr)
        {
            if (hasFailedChange)
            {
                //存在失效单元显隐变化：需重建渲染索引(buildIndex)使 is_show 生效，同时更新颜色
                model_proxy->RebuildModelData();
            }
            else
            {
                model_proxy->updateValue();
            }
        }

        if (isSolidColor)
            postRender->getColorTableWindow()->setVisiable(false);
        else
        {
            // 色表值域收编（C4 仅窗单效：本函数无伴生 setMinMaxData 直调）
            mxr::ColorLegendParams _lg;
            _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
            _lg.targetFrame = postRender->frameProxy(postRender->currentFrame())->getFrameID();
            _lg.valueMin = query_proxy->getCurrentMinData();
            _lg.valueMax = query_proxy->getCurrentMaxData();
            PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
        }

        if (hasFailedChange)
        {
            //重新绑定 OSG 几何索引，使重建后的单元可见性生效
            postRender->updateAllModelOperate(UpdateModel);
        }
        {
            postRender->updateAllModelOperate(UpdateVariableValue);
        }
        {
            postRender->updateAllModelOperate(UpdateMinMax);
        }
        {
            rebuildPostRend->updatePaint();
        }

        //【极值显示冻结（cloud-panel-frame-sync 二期追加，FR-07 边界扩展）】同 slot_showCloud
        //口径：播放中（任一动源：会话 Playing 或浮层 isRealtimeAnimPlaying()）冻结图例范围
        //上/下限编辑框与"默认最大值/默认最小值"标签回写，面板停驻旧值；值域计算与渲染
        //驱动/guidata 真值写入照常，停机经停机同步链 updateResultInfoGUI 回填真值
        const bool frozenExtremes = (postRender->playbackState() == mxr::PlaybackState::Playing)
            || isRealtimeAnimPlaying();
        if (varstr == "SolidColor")
        {
            postRender->updateAllModelOperate(ShowSolidColor);
        }

        //获取变量类型
        int vartype = modeldb->getVariableType(varstr);
        if (varstr == QString("SolidColor"))
        {
            if (isNew)
            {
                rebuildPostRend->updatePaint();
            }
            postRender->updateAllModelOperate(ColorAllPartOperate);
        }
        else
        {
            std::set<int> hideParts = getCVM->getModelPartVisualData(rebuildPostRend->getViewportName());
            getCVM->setPostAllPartVisual(rebuildPostRend->getViewportName(), true);
            getCVM->setPostSomePartVisual(rebuildPostRend->getViewportName(), hideParts, false);
            getCVM->updateAllPostView();
            for (auto id : hideParts)
            {
                modelData->setMeshPartVisual(rebuildPostRend->getViewportName(), id, false);
            }

            float minval = FLT_MAX;
            float maxval = -FLT_MAX;

            int index = _comboRangeMode->currentIndex();
            if (index == 0)
            {
                //全帧统一值域按图例范围模式解析（逐帧/全程/手动），不可得回退当前帧原始值域
                resolveFullRange(maxval, minval, postname, modeldb, varstr, compostr,
                    layerstr, complexstr, query_proxy, guidata);
                _legendMax->lineEdit()->blockSignals(true);
                _legendMin->lineEdit()->blockSignals(true);
                if (!frozenExtremes)	//【极值显示冻结（cloud-panel-frame-sync 二期追加）】播放中跳过回写：面板停驻旧值，值域计算/渲染驱动照常
                {
                    _legendMax->lineEdit()->setText(MxFunctions::mNumberToString(maxval, 'f', 12));
                    _legendMin->lineEdit()->setText(MxFunctions::mNumberToString(minval, 'f', 12));
                }
                _legendMax->lineEdit()->blockSignals(false);
                _legendMin->lineEdit()->blockSignals(false);
                query_proxy->setCurrentMaxData(maxval);
                query_proxy->setCurrentMinData(minval);
                // 色表值域收编（C4 双效点：与本函数尾段 setMinMaxData 伴生——窗写段，实例写段见尾段）
                {
                    mxr::ColorLegendParams _lg;
                    _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;   // 窗写段（现状首笔）
                    _lg.valueMin = minval;
                    _lg.valueMax = maxval;
                    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
                }
                this->slot_setMostValue(false);
                guidata->_cloudMaxValue = maxval;
                guidata->_cloudMinValue = minval;
            }
            else if (index == 1)
            {
                auto cur_min_max = model_proxy->getPartMinMax();
                minval = cur_min_max.first;
                maxval = cur_min_max.second;
                //半就绪防御：无值部件的哨兵条目 {FLT_MAX,-FLT_MAX} 参与聚合会得
                //min=FLT_MAX>max（倒挂），按帧无数据回落 0/0（同图例范围档兜底语义）
                if (!(maxval >= minval) || !std::isfinite(maxval) || !std::isfinite(minval))
                {
                    maxval = 0.0f;
                    minval = 0.0f;
                }

                _legendMax->lineEdit()->blockSignals(true);
                _legendMin->lineEdit()->blockSignals(true);
                if (!frozenExtremes)	//【极值显示冻结（cloud-panel-frame-sync 二期追加）】播放中跳过回写：面板停驻旧值，值域计算/渲染驱动照常
                {
                    _legendMax->lineEdit()->setText(MxFunctions::mNumberToString(maxval, 'f', 12));
                    _legendMin->lineEdit()->setText(MxFunctions::mNumberToString(minval, 'f', 12));
                }
                _legendMax->lineEdit()->blockSignals(false);
                _legendMin->lineEdit()->blockSignals(false);
                query_proxy->setCurrentMinData(minval);
                query_proxy->setCurrentMaxData(maxval);
                // 色表值域收编（C4 双效点）：默认值域段+窗写段一口两掩码（顺序按现状），实例写段见尾段
                {
                    mxr::ColorLegendParams _lg;
                    _lg.updateMask = mxr::ColorLegendParams::Mask_DefaultRange
                        | mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
                    _lg.valueMin = minval;
                    _lg.valueMax = maxval;
                    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
                }
                this->slot_setMostValue(false);
                guidata->_cloudMaxValue = maxval;
                guidata->_cloudMinValue = minval;
            }

            // 色表值域收编（C4 双效点·实例写段）：旧 setMinMaxData 原体（含窗写+刷新链）
            {
                mxr::ColorLegendParams _lg;
                _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRange;
                _lg.valueMin = minval;
                _lg.valueMax = maxval;
                PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
            }
            postRender->updateAllModelOperate(ColorAllPartOperate);
        }

        // 查询查询数据代理
        mxr::IPostRendQueryDataProxy* query_proxy1 = dynamic_cast<mxr::IPostRendQueryDataProxy*>(postRender->frameProxy(postRender->currentFrame())->getRendQueryDataProxy());
        float maxvalue = query_proxy->getOriginalMaxData();
        float minvalue = query_proxy->getOriginalMinData();
        guidata->_cloudDefualtMax = maxvalue;
        guidata->_cloudDefualtMin = minvalue;

        //信息标签显隐跟随 guidata（内容/字体由注释面板管理；隐藏与复位见 slot_labelHide/Reset）
        if (mxr::mPostFrameText* viewtext = postRender->getPostFrameText())
            viewtext->setTopRightTextVisual(guidata->_cloudInfoLabelVisible);

        ////更新视图文本
        //mPostFrameText* viewtext = postRender->getPostFrameText();
        //if (viewtext != nullptr)
        //{
        //	QString model = modeldb->getName();
        //	QString casetext = modeldb->getFrameCase(fi);
        //	QString framename = modeldb->getFrameNameByFrameNum(fi);
        //	mPostModelData* modeldata = modeldb->getModelData(fi);
        //	QString type, timestr;
        //	if (!modeldata->_frameInfo._isFrequency)
        //	{
        //		type = "Time";
        //		double time = modeldb->getFrameTime(fi);
        //		timestr = MxFunctions::mNumberToString(time, 'E', 6);
        //	}
        //	else
        //	{
        //		type = "Frequency";
        //		double fre = modeldata->_frameInfo._frequency._modalFreq;
        //		timestr = MxFunctions::mNumberToString(fre, 'E', 6);
        //	}

        //	viewtext->setTopRightText(4, QString("Model:%1 Case:%2")
        //		.arg(model).arg(casetext));
        //	viewtext->setTopRightText(5, QString("Frame:%3 %4:%4")
        //		.arg(framename).arg(type).arg(timestr));
        //	postRender->update();
        //}

        postRender->getColorTableWindow()->SetTitle(0, "MxSim");
        if (guidata->_cloudLayerName.isEmpty())
        {
            QString title = guidata->_cloudVarName;
            if (!guidata->_cloudCompo.isEmpty()) {
                title += " - " + guidata->_cloudCompo;
            }
            postRender->getColorTableWindow()->SetTitle(1, title.toStdString().c_str());
        }
        else
        {
            QString title = guidata->_cloudVarName;
            if (!guidata->_cloudCompo.isEmpty()) {
                title += " - " + guidata->_cloudCompo;
            }
            if (!guidata->_cloudLayerName.isEmpty()) {
                title += " : " + guidata->_cloudLayerName;
            }
            postRender->getColorTableWindow()->SetTitle(1, title.toStdString().c_str());
        }

        //桥梁杆实体化显示
        if (guidata->_cloudShowSBR)
            postRender->updateAllModelOperate(ShowBeamShellVolume);
        else
            postRender->updateAllModelOperate(ClearBeamShellVolume);

        //离散球单元显示
        postRender->updateAllModelOperate(ShowDiscreteBall);

        //设置显示模式
        setShowMode();

        //设置点大小
        postRender->setPointSize(guidata->_pointSize);

        if (isNew && !frozenExtremes)	//【极值显示冻结】播放中跳过"默认最大值/默认最小值"标签回写（停机经停机同步链 updateResultInfoGUI 回填真值）
        {
            //更新信息显示
            this->updateInformation();
        }

        MechCommon::mMechGlobalSignals::getInstance()->updateResultMenuGUI();

        // 通知标注面板等跟随云图当前帧刷新
        MechCommon::mMechGlobalSignals::getInstance()->postCloudFrameChanged(guidata->_cloudFrame);

        //动画/外部切帧整幅重绘同样现读面板控件=UI 已全部应用，清"待应用"提示
        setCloudDirty(false);

        rebuildPostRend->FitView();
        rebuildPostRend->updatePaint();

        std::set<int> hideParts = getCVM->getModelPartVisualData(rebuildPostRend->getViewportName());
        getCVM->setPostSomePartVisual(_curViewName, hideParts, false);
        for (auto id : hideParts)
        {
            modelData->setMeshPartVisual(_curViewName, id, false);
        }
        //P2-1 审计修复：高级查询跳帧回写帧下拉与浮层（三方失步防御）——本槽此前直改
        //guidata 帧字段+seekFrame 而不回写 _comboFrame/浮层：暂停态浮层滑条/面板下拉
        //与画面三方失步，再按播放从旧位置跳走。回写走 blockSignals+setFrameIndex
        //（均不回发信号，无回环）；播放中回写浮层基帧=下一 tick 自查询帧续播（查询定位
        //不再被旧播放序列覆盖）
        {
            const int jumpFrame = guidata->_cloudFrame;
            QString jumpPost = rebuildPostRend->getPostDataName();
            QPointer<mPostDataBase> jumpDb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(jumpPost);
            if (jumpDb != nullptr)
            {
                const QString jumpCase = _comboCase->currentText();
                for (int i = 0; i < _comboFrame->count(); ++i)
                {
                    const QString jumpText = _comboFrame->itemText(i);
                    const QString jumpName = jumpText.contains(":") ? jumpText.split(":").at(0) : jumpText;
                    if (jumpDb->getFrameNumByCaseAndFrameName(jumpCase, jumpName) == jumpFrame)
                    {
                        _comboFrame->blockSignals(true);
                        _comboFrame->setCurrentIndex(i);
                        _comboFrame->blockSignals(false);
                        if (_realtimeAnimWidget != nullptr)
                            _realtimeAnimWidget->setFrameIndex(i);
                        break;
                    }
                }
            }
        }
    }

    //基于当前 is_show 状态重建模型面数据
    void rebuildFacesForModelData(QPointer<PostData::mPostModelData> modelData)
    {
        PerfTimer _t("rebuildFacesForModelData");
        if (modelData == nullptr)
            return;
        mPostParseMesh* parseMesh = new mPostParseMesh(nullptr);
        parseMesh->ParseMesh(modelData);
        delete parseMesh;
        parseMesh = nullptr;
    }

    // 检查共享 part 数据当前 is_show 状态是否已等于目标状态（全部单元显示，仅 loseEles 隐藏）。
    // 判定直接实测分类建面的真实输入（cell_info[].is_show），而非任何簿记表：
    // 第一道 O(总单元数) 纯布尔扫描（与 mPostPartData::getMaskEleNum 同款，
    // 参照既有先例 mPostResultPath::computeVisibilityFingerprint 的低频显隐指纹口径），
    // 要求隐藏单元总数恰为失效集大小；第二道逐失效单元 isMaskEle 复核确已隐藏。
    // 鸽笼原理：两道合取 ⟹ 隐藏集恰为当前失效集——任何链外残留隐藏（手动隐藏单元/
    // 集合操作/其他帧失效残留）都会使隐藏计数不符被判为"未就绪"走原全量路径，无漏判。
    static bool isVisualStateAlreadyApplied(QPointer<PostData::mPostModelData> modelData, const std::set<int>& loseEles)
    {
        if (modelData == nullptr)
            return false;

        int hiddenTotal = 0;
        for (auto part : modelData->_partdata)
        {
            if (part == nullptr)
                continue;
            hiddenTotal += part->getMaskEleNum();
        }
        if (hiddenTotal != static_cast<int>(loseEles.size()))
            return false;

        for (int eleRid : loseEles)
        {
            int partid = modelData->getPartIDByEleID(eleRid);
            if (partid < 0)
                return false;
            QPointer<PostData::mPostPartData> part = modelData->getPartData(partid);
            if (part == nullptr || !part->isMaskEle(eleRid))
                return false;
        }
        return true;
    }

    // S6 回归修复（克隆帧面片未物化豁口）：检测目标帧 modelData 自身的分类面数据
    // 是否已物化（classifyAndWriteFaces 已对该帧各部件执行过、_partfacedata 非空）。
    // mxdb 引用帧（sameFrame=0/n）在导入期刻意不做面解析（mMxdbThread.cpp "不在导入期
    // 单独做网格面解析"），其面片必须由显示期 classify 现场构建；而跨帧面缓存按
    // computeMeshFaceCacheKey 把引用帧解析到被引用帧，引用帧首访会共享源帧的
    // cache/failureSignature，若仅凭"缓存+签名未变+is_show 已就绪"跳过，克隆帧自身的
    // 面片缓冲将永远为空 → updateIndex 空索引 → 切帧空白。故守卫跳过前必须确认
    // 目标帧面片已物化。判定口径：有单元的部件其 face_node_counts 必须非空
    // （classifyAndWriteFaces 会 resize face_node_counts 为边界+内部面总数）。
    // 零单元部件 face 数据合法为空，不参与判定。
    static bool isFaceBufferMaterialized(QPointer<PostData::mPostModelData> modelData)
    {
        if (modelData == nullptr)
            return false;
        for (auto part : modelData->_partdata)
        {
            if (part == nullptr)
                continue;
            const bool hasCells = !part->_parteledata.cell_types.empty();
            if (hasCells && part->_partfacedata.face_node_counts.empty())
                return false;
        }
        return true;
    }

    // 基于缓存复用策略重建模型面数据
    // 返回值：true 表示使用了缓存复用（完全复用、仅重新分类或零动作跳过），false 表示执行了完整重建
    bool rebuildFacesForModelDataWithCache(
        QPointer<PostData::mPostDataBase> postDB,
        QPointer<PostData::mPostModelData> modelData,
        const QString& viewportName,
        int frame)
    {
        QElapsedTimer totalTimer;
        totalTimer.start();

        if (postDB == nullptr || modelData == nullptr)
            return false;

        using namespace PostData;

        std::shared_ptr<mPostModelData::MeshFaceCache> cache = postDB->getMeshFaceCacheForFrame(frame);
        std::set<int> currentLoseEles = modelData->getLoseElems();

        // 渲染线移交项1建议①②（渲染线移交清单-GUI协调项-2026-11-06）：失效集对比前置 + 触发去重。
        // 建面的合理触发条件是网格拓扑/失效单元集变化，而非"变量切换"事件本身。零动作判定
        // （四条件同时成立时跳过，跳过为行为等价）：
        //  1) 该帧建面缓存在——拓扑输入与上次建面一致的前提；
        //  2) 失效集签名未变——cache->failureSignature 即上次建面时的失效集哈希，
        //     classifyAndWriteFaces 是其输入 (拓扑, is_show) 的纯函数（mPostParseMesh.cpp:652
        //     仅消费 cell_info[].is_show），输入相同则输出与现面数据逐位一致；
        //  3) is_show 实测已等于目标状态（全显示+仅当前失效集隐藏，见 isVisualStateAlreadyApplied）。
        //  4) 目标帧自身面片缓冲已物化（见 isFaceBufferMaterialized）——克隆帧豁口修复：
        //     引用帧与源帧共享跨帧缓存与签名，前三条件在克隆帧首访时同样成立，但该帧
        //     面片尚未 classify，跳过将导致空索引空白，必须放行走 classify。
        // 一次切变量的重复绘制入口（slot_chooseVariable/slot_chooseCompo 等多路汇入同链）与
        // count=0→count=0 切换均在此零动作返回，消除 ~4.2s 级全量 classifyAndWriteFaces 重复执行。
        const bool guardCacheValid = (cache != nullptr);
        const bool guardSigMatch = guardCacheValid &&
            (cache->failureSignature == mPostMeshSignature::computeFailureSignature(modelData));
        const bool guardVisualApplied = guardSigMatch &&
            isVisualStateAlreadyApplied(modelData, currentLoseEles);
        // S6 回归修复：第 4 条件——目标帧自身面片缓冲已物化。克隆帧首访时前三条件
        // 皆成立（共享源帧缓存/签名/is_show 状态），但本帧面片从未 classify，必须放行。
        const bool guardFacesReady = guardVisualApplied &&
            isFaceBufferMaterialized(modelData);
        if (guardFacesReady)
        {
            return true;
        }

        // 缓存存在时，直接复用拓扑并按当前帧失效集合重新分类
        if (cache != nullptr && !cache->partTopologies.empty())
        {
            QElapsedTimer incTimer;
            incTimer.start();

            // 防御：缓存拓扑的 part 数量必须与当前模型一致，否则降级为完整重建
            if (static_cast<int>(cache->partTopologies.size()) == static_cast<int>(modelData->_partdata.size()))
            {
                modelData->setAllEleShow(viewportName);

                for (auto eleId : currentLoseEles)
                {
                    int partid = modelData->getPartIDByEleID(eleId);
                    if (partid >= 0)
                        modelData->setEleVisual(viewportName, partid, eleId, false);
                }

                int partIdx = 0;
                for (auto part : modelData->_partdata)
                {
                    if (part == nullptr)
                    {
                        ++partIdx;
                        continue;
                    }
                    mPostParseMesh::classifyAndWriteFaces(part, cache->partTopologies[partIdx]);
                    ++partIdx;
                }


                return true;
            }
        }

        QElapsedTimer rebuildTimer;
        rebuildTimer.start();

        // 缓存未命中：完整重建并缓存拓扑
        modelData->setAllEleShow(viewportName);

        for (auto eleId : currentLoseEles)
        {
            int partid = modelData->getPartIDByEleID(eleId);
            if (partid >= 0)
                modelData->setEleVisual(viewportName, partid, eleId, false);
        }

        mPostParseMesh parseMesh(nullptr);
        for (auto part : modelData->_partdata)
        {
            if (part == nullptr)
                continue;
            parseMesh.buildMeshFace(part);
        }

        std::shared_ptr<mPostModelData::MeshFaceCache> newCache = std::make_shared<mPostModelData::MeshFaceCache>();
        newCache->signature = mPostMeshSignature::computeFullSignature(modelData);
        newCache->failureSignature = mPostMeshSignature::computeFailureSignature(modelData);
        for (auto part : modelData->_partdata)
        {
            if (part == nullptr)
            {
                newCache->partTopologies.emplace_back();
                continue;
            }
            UniqueFaceTopology topology;
            mPostParseMesh::buildUniqueFaceTopology(part, topology);
            newCache->partTopologies.push_back(std::move(topology));
        }
        postDB->setMeshFaceCacheForFrame(frame, newCache);
        return false;

    }

    bool processFailedElements(QPointer<PostData::mPostDataBase> modeldb, mxr::REBUILD_PostRend* rebuildPostRend, int fi, int lastFi)
    {
        if (modeldb == nullptr || rebuildPostRend == nullptr)
            return false;

        QString viewportName = rebuildPostRend->getViewportName();

        //获取上一帧失效单元集合
        std::set<int> lastLoseEles{};
        QPointer<mPostModelData> lastModelData = modeldb->getModelData(lastFi);
        if (lastModelData != nullptr)
        {
            lastLoseEles = lastModelData->getLoseElems();
        }

        //获取当前帧失效单元集合
        QPointer<mPostModelData> modelData = modeldb->getModelData(fi);
        if (modelData == nullptr)
            return false;
        std::set<int> loseEles = modelData->getLoseElems();
        int totalEleCount = modelData->getElementAmount();

        //若上/当前帧均无失效单元，仍需恢复共享 part 数据上可能残留的隐藏状态并重建面，
        //避免动画视图或其他帧残留的隐藏状态污染云图当前帧面重建。
        //该防御语义由 rebuildFacesForModelDataWithCache 内部前置零动作判定统一覆盖
        //（移交项1建议①）：is_show 实测无残留且失效签名未变时零动作直接返回；
        //判有残留时仍走原"全恢复→按当前失效集隐藏→逐部件重分类"路径，语义不变。
        //原外层 setAllEleShow 已冗余（非跳过路径函数内部同款执行；跳过路径已证明全显示）。
        if (lastLoseEles.empty() && loseEles.empty())
        {
            rebuildFacesForModelDataWithCache(modeldb, modelData, viewportName, fi);
            return true;
        }

        //获取需要恢复显示的单元（上一帧失效但当前帧不再失效）
        std::set<int> needShowEles{};
        for (auto id : lastLoseEles)
        {
            if (loseEles.count(id) == 0)
            {
                needShowEles.insert(id);
            }
        }

        // needShow/needHide 数量已在上方统一输出，此处不再重复打印

        // 重建面：rebuildFacesForModelDataWithCache 内部会先恢复所有单元显示，
        // 再按当前帧失效集合隐藏并重新分类/重建面，避免多帧共享 part 数据时
        // 残留隐藏状态污染当前帧。
        rebuildFacesForModelDataWithCache(modeldb, modelData, viewportName, fi);

        //存在失效单元显隐变化，调用方需重建渲染索引（buildIndex）使新面数据生效
        return true;
    }

    bool hideFailedElementsForFrame(QPointer<PostData::mPostDataBase> modeldb, mxr::REBUILD_PostRend* rebuildPostRend, int fi)
    {
        PerfTimer _t("hideFailedElementsForFrame");
        if (modeldb == nullptr || rebuildPostRend == nullptr)
            return false;

        QString viewportName = rebuildPostRend->getViewportName();

        QPointer<mPostModelData> modelData = modeldb->getModelData(fi);
        if (modelData == nullptr)
            return false;

        std::set<int> loseEles = modelData->getLoseElems();
        int totalEleCount = modelData->getElementAmount();

        if (loseEles.empty())
        {
            // 即使当前帧无失效单元，恢复共享 part 数据残留隐藏状态并重建面的防御语义
            // 也由 rebuildFacesForModelDataWithCache 内部前置零动作判定统一覆盖
            // （移交项1建议①）：无残留且失效签名未变时零动作；判有残留时仍走原
            // "全恢复→重建"路径。原外层 setAllEleShow 已冗余，理由同 processFailedElements。
            rebuildFacesForModelDataWithCache(modeldb, modelData, viewportName, fi);
            return true;
        }

        // 防御：若失效单元数量占总单元比例过高，极有可能是失效集合解析错误
        //（例如把全部单元误标记为失效），此时不执行隐藏并输出告警。
        double loseRatio = totalEleCount > 0 ? (double)loseEles.size() / (double)totalEleCount : 0.0;
        if (loseRatio > 0.9)
        {
            return false;
        }

        // 基于当前 is_show 状态重新构建面数据；
        // rebuildFacesForModelDataWithCache 内部会先恢复所有单元显示并隐藏当前帧失效单元。
        QElapsedTimer timer;
        timer.start();
        rebuildFacesForModelDataWithCache(modeldb, modelData, viewportName, fi);

        return true;
    }

    bool hideFailedElementsForFrameWithCache(
        QPointer<PostData::mPostDataBase> modeldb,
        mxr::REBUILD_PostRend* rebuildPostRend,
        int fi,
        const std::set<int>& lastLoseEles)
    {
        Q_UNUSED(lastLoseEles);
        PerfTimer _t("hideFailedElementsForFrameWithCache");
        if (modeldb == nullptr || rebuildPostRend == nullptr)
            return false;

        QPointer<mPostModelData> modelData = modeldb->getModelData(fi);
        if (modelData == nullptr)
            return false;

        std::set<int> loseEles = modelData->getLoseElems();
        int totalEleCount = modelData->getElementAmount();

        // 防御：若失效单元数量占总单元比例过高，极有可能是失效集合解析错误
        double loseRatio = totalEleCount > 0 ? (double)loseEles.size() / (double)totalEleCount : 0.0;
        if (loseRatio > 0.9)
        {
            return false;
        }

        QString viewportName = rebuildPostRend->getViewportName();
        QElapsedTimer timer;
        timer.start();
        rebuildFacesForModelDataWithCache(modeldb, modelData, viewportName, fi);
        return true;
    }

    bool restoreFailedElementsForFrame(QPointer<PostData::mPostDataBase> modeldb, mxr::REBUILD_PostRend* rebuildPostRend, int fi)
    {
        if (modeldb == nullptr || rebuildPostRend == nullptr)
            return false;

        QString viewportName = rebuildPostRend->getViewportName();

        QPointer<mPostModelData> modelData = modeldb->getModelData(fi);
        if (modelData == nullptr)
            return false;

        // 无论是否存在失效单元，都先恢复当前视图所有单元为显示，
        // 再重建面，确保跨视图/跨帧残留的隐藏状态被彻底清除。
        modelData->setAllEleShow(viewportName);
        rebuildFacesForModelData(modelData);

        return true;
    }
}
