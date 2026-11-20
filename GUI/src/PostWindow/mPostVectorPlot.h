#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"
#include "mPanelBase.h"
//对象选择拾取条仅作指针成员（_objectPickController），头内前向声明——按值包含
//mPostGeneralPickUp.h 会把 RebuildRender 头拖进无渲染依赖的无头测试目标
//(tst_mPostVectorPlotLogic)；工厂调用 createPickWidget 在本类 .cpp 包含

#include <QColor>
#include <QHash>
#include <QList>
#include <QPair>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector3D>

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;

class QComboBox;
class QPushButton;
class QToolButton;
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QLabel;
class QLineEdit;
class QCheckBox;
class QFrame;
class QHideEvent;
class QSpinBox;

namespace MxFunctions
{
    class mChoiceGroup;
    class mLegendBoundRow;
}

namespace PostData
{
    class mPostDataBase;
}

namespace MechPreData
{
    class mPostGUIData;
}

namespace mxr
{
    class REBUILD_PostRend;
}

namespace PostWindow
{
    class FbdColorPicker;
    class mPostPartScopeField;
    class mPostPickWidgetController;

    /*
    * 向量图渲染三入口（面板 apply/clear 与云图切帧/重绘插桩共用）：
    * - applyVectorPlot：按 guidata 记录的参数读场（readNodeVectorField，懒加载链路
    *   自带云图态保护）→写矢量哈希→先删后建 VectorGraph 场景对象（全帧实例扇出，
    *   节点位置自动叠加变形/爆炸/切面裁剪）。读场失败输出窗提示并返回 false（既有
    *   箭头保持）。B5 图例值域：读场后按 guidata 三档（逐帧=当前帧幅值现算/全程=
    *   全帧聚合缓存/手动=锁定值）解析值域随参数下发（数值着色才接管色条；解析结果
    *   写 guidata _vectorPlotResolved*，resolvedRange 出参供面板回显）。参数字段见
    *   mPostGUIData _vectorPlot* 注释（源视口直作用模型）。
    * - removeVectorPlot：清 VectorGraph 场景家族+guidata 开关复位（引擎侧值域接管
    *   随家族摘除复位，色条自动回云图值域）。
    * - refreshVectorPlotIfActive：_vectorPlotActive 时按记录参数重读当前帧场并重建
    *   （切帧/重绘插桩用；失败保持现状）。
    */
    bool MPOSTWINDOW_EXPORT applyVectorPlot(mxr::REBUILD_PostRend* postRend, MechPreData::mPostGUIData* guidata,
        QPair<double, double>* resolvedRange = nullptr);
    void MPOSTWINDOW_EXPORT removeVectorPlot(mxr::REBUILD_PostRend* postRend, MechPreData::mPostGUIData* guidata);
    void MPOSTWINDOW_EXPORT refreshVectorPlotIfActive(mxr::REBUILD_PostRend* postRend, MechPreData::mPostGUIData* guidata);

    //向量图参数折算纯逻辑（供 tests/PostWindow 无头断言；与 UI 控件解耦）
    namespace detail
    {
        //通道勾选表（索引 0=X,1=Y,2=Z,3=X+Y,4=Y+Z,5=Z+X,6=X+Y+Z，与面板色盘索引同域）
        //+对应色 → 引擎渲染通道表（输出顺序固定 X/Y/Z/XY/YZ/XZ/XYZ，仅收勾选项；
        //色值 0-255 域与 VectorGraphParams.labels 一致，渲染端 /255 归一）
        QVector<QPair<QString, QVector3D>> MPOSTWINDOW_EXPORT buildVectorPlotLabels(
            const bool channelChecked[7], const QColor channelColors[7]);

        //符号尺寸三选折算渲染倍率：0=自动(→1.0) 1=统一(→统一尺寸值) 2=缩放(→倍率值)
        //（本批三模式统一按倍率语义；按模长缩放长度列渲染库扩展遗留）
        double MPOSTWINDOW_EXPORT vectorPlotScale(int sizeMode, double uniformSize, double scaleBy);

        //着色方式 → 引擎 arrowType：0=数值(MinMax 色表)→1；1=自定义(通道色 Common)→0
        int MPOSTWINDOW_EXPORT vectorPlotArrowType(int colorModeIndex);

        //引擎通道名 → 面板通道索引（X/Y/Z/XY/YZ/XZ/XYZ → 0..6；未知返回 -1）
        int MPOSTWINDOW_EXPORT vectorPlotChannelIndex(const QString& channelName);

        //矢量场幅值极值 <min,max>（空场返回 (0,0)=不可用标记，与云图 getMaxMinByFrameID 空读同判）
        QPair<double, double> MPOSTWINDOW_EXPORT vectorFieldRange(const QHash<int, QVector3D>& field);

        //图例范围三档解析（mode=中文文本 逐帧/全程/手动，与云图 _legendRange 文本比较口径同款）：
        //手动=manual 锁定值（不可用回退 frameRange）；全程=allRange（不可用回退 frameRange）；
        //逐帧/最终回退=frameRange。不可用=max<min 或 (0,0)。返回 <min,max>
        QPair<double, double> MPOSTWINDOW_EXPORT resolveVectorLegendRange(const QString& mode, double manualMax,
            double manualMin, const QPair<double, double>& frameRange, const QPair<double, double>& allRange);
    }

    /*
    * 向量图参数面板（结果页"结果图形"组"向量图"入口；V1.3 原型 #vectorParameters，
    * 前处理原型内嵌后处理文档运行时构建——克隆云图参数前两组+动态绘图设置/图例组，
    * 差异表见 PostWindow.md 2026-11-11 条目）。
    *
    * 结构四折叠组：
    * - 向量变量：工况/帧(+四帧导航钮)数据驱动；变量=位移/速度/转动速度/加速度/力
    *   （默认转动速度）；层与使用角点数据禁用占位（原型 unavailable 语义）；
    *   变形系数内联输入+齿轮跳转变形控制；
     * - 对象及坐标系（对象选择=mPost 拾取栈拾取条，2026-11-18 云图同款替换）：
     *   [类型下拉 | CBBlue 拾取条] 单行（点击条体激活/再点停止，内建详情弹窗+清空钮，
     *   激活/失活经 slot_objectPickActiveChanged 设视口拾取过滤器与工具栏白名单，
     *   收割/高亮/重入回显由控制器自管）；范围弹窗下线（mPostPartScopeField 隐形
     *   仅供"所有X"文案），范围/类型持久化 guidata；坐标系=裸下拉（全局坐标系+按
     *   guidata 回填自定义坐标系）+26×26"＋"（接 clickPostCoordinateCreateSig 全局链，
     *   云图同款）；使用跟踪坐标（启用写 guidata，云图同款）|边中点值（默认勾+禁用）
     *   组尾双列区（云图 tail row 同款）；
    * - 绘图设置：分量 X/Y/Z 与合量 X+Y/Y+Z/Z+X 连体复选、显示数值/标签前缀/X+Y+Z
    *   三列复选行（默认仅 X+Y+Z 勾）、"设置 ›"开显示设置弹窗（字号字体+分量及合量
    *   颜色 4 列色盘+着色方式+符号尺寸三选拼数值+符号形式三选拼箭头样式）；
     * - 图例（控件与功能对齐云图，2026-11-14；值域接线 2026-11-17）：模式/图例范围逐帧·全程·手动
     *   mChoiceGroup（带说明 tooltip）/最大最小值=库 mLegendBoundRow 边界行（输入+复位+显隐；
     *   眼睛真作用视口色表最大/最小值线=云图 slot_setPositionVisible 同款；B5 值域已接线——
     *   逐帧/全程档应用后只读回显解析值域（云图 12 位小数同口径），手动档输入锁定下发，
     *   复位=回解析实际范围；引擎侧数值着色期间 VectorGraph 值域接管色条与 MinMax 箭头
     *   采样域，家族摘除自动回云图域）/插值阶数=QSpinBox
     *   (2..30，云图同款)/平滑方式/分布方式/渲染图例三钮(隐藏·复位·自定义：隐藏/复位真作用
     *   视口色表=云图 slot_showOrHideColorBar/slot_resetColorBar 同款；自定义=展开图例自定义
     *   子组，原型 vectorLegendCustomSection 显式定义的向量专属行为)/信息标签三钮(隐藏·复位·
     *   自定义，云图同款；显隐与复位作用视口右上文本、自定义开注释面板弹窗)；倍率行已删
     *   （2026-11-14 用户定案暂不需要）；"自定义"展开图例自定义子组（类型/位置/数值精度/
     *   数值格式/色阶数量/图例标题，默认收起）。
    *
    * 操作条=清除渲染(禁用)｜应用渲染；状态行 role=status（任意变更清空）。
    * 校验与文案全按原型 apply 链四步；应用后仅内存 requested 态+状态行提示
    * （"当前结果未接入向量数据"——引擎未接入，原型即纯配置页）。
    */
    class MPOSTWINDOW_EXPORT mPostVectorPlotPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        explicit mPostVectorPlotPanel(QWidget* parent = nullptr);

        /** 打开/刷新面板（入口点击；工况/帧按当前后处理视口结果库重建，选择保持） */
        void updateVectorPlotPanel();

    protected:
        /** 面板隐藏=拾取控件收口+关显示设置弹窗（宿主 setMainWidget 切面板时触发；
        * showEvent 对称恢复——setPickWidgetVisible(true/false) 一对） */
        void hideEvent(QHideEvent* event) override;
        void showEvent(QShowEvent* event) override;

    private:
        /** 构建面板控件（语言文本由 applyLanguageTexts 设置） */
        void setupUI();

        /** 语言切换标签登记（显式指针；findChildren 按序猜测会随嵌套结构漂移） */
        struct LabelText { PanelLabel* label; const char* cn; const char* en; };
        void regLabel(PanelLabel* label, const char* cn, const char* en);
        QList<LabelText> _labelTexts;

        /** 连体复选组（原型 mirror-planes 复选版）：等宽项+竖线分隔共用外框 */
        QWidget* buildCheckStrip(const QStringList& names, QList<QCheckBox*>& checks);

        /** 26×26 色块钮（底色=当前色，点击开取色弹窗） */
        QPushButton* buildSwatch(const QColor& color);
        void applySwatchColor(QPushButton* swatch, const QColor& color);

        /** 弹窗贴锚点控件下方显示（越界收回屏内；同标签/查询面板口径） */
        void popupUnder(QWidget* popup, QWidget* anchor);

        /** 数据源：当前后处理视口结果库（无后处理视口返回空） */
        PostData::mPostDataBase* currentPostDB() const;
        void refreshCaseList(const QString& preferCase = QString(), const QString& preferFramePrefix = QString());
        void refreshFrameList(const QString& caseName, const QString& preferFramePrefix = QString());
        void refreshFrameButtons();

        /** 变量列表数据驱动填充：当前帧节点矢量变量（filterDeformVariables 口径，
         *  同变形面板；默认首项=位移，guidata 已有选择回填） */
        void refreshVariableList(int frameId);

        /** 当前激活后处理视口与云图 guidata（面板 apply/回填共用；任一缺失返回 false） */
        bool currentCloudContext(mxr::REBUILD_PostRend** postRend, MechPreData::mPostGUIData** guidata);

        /** apply 前把视口切到面板选中工况/帧（工况走云图面板 selectCaseByName 标准链、
         *  帧走全局 changedFrame 信号——高级查询同款），保证"所画即所选" */
        void syncViewToPanelSelection(mxr::REBUILD_PostRend* postRend, MechPreData::mPostGUIData* guidata);

        /** 打开面板时按 guidata 回填：变量选择 + 已应用态（_requested/清除按钮）；
         *  通道/颜色等细节不回填（渲染态存 guidata 随视口存续，重应用以当前 UI 为准） */
        void restoreFromGUIData();

        /** 填充全部下拉/单选组选项并设默认选择（构造一次；语言重填保持选择） */
        void initOptions();

        /** 退出拾取（Esc/切类型/开弹窗/收折叠组/应用渲染共用）：失活拾取栈控件，
        *  视口过滤器/高亮/工具栏收口走 pickActiveChanged 链；未激活时无操作 */
        void endPick();

        /** 显示设置弹窗（Qt::Popup 构造期建壳；原型 #vectorDisplayPopup） */
        void ensureDisplayPopup();
        void openDisplayPopup();       //只开（校验失败定位弹窗内字段用）
        void slot_displayOpenClicked();
        void closeDisplayPopup();

        /** 原型 refreshDisplay：符号尺寸模式/着色方式/显示数值联动启停 */
        void refreshDisplayState();

        /** 原型 refreshRange：图例范围手动/非手动联动（只读、占位、manual 缓存回填） */
        void refreshLegendRange();

        /** 信息标签三钮状态刷新（可见分支=隐藏+复位/自定义可用；隐藏分支=显示+两钮禁用） */
        void setInfoLabelButtonsState(bool visible);

        /** 应用链（原型 apply 四步校验；弹窗内字段无效先开弹窗定位） */
        void slot_applyClicked();
        void slot_clearClicked();
        bool validateForApply(QString* error);
        void setStatus(const QString& text);
        void clearStatus();

        /** 图例复位（云图 slot_resetColorBar 同款：视口渲染图例位置复位；原型 reset 的
         *  组快照恢复为演示级语义，功能对齐云图后废除） */
        void slot_legendResetClicked();

        /** 边界行眼睛显隐（云图 slot_setPositionVisible 同款：视口色表最大/最小值线） */
        void slot_legendBoundVisibilityToggled(bool isshow);

        /** 语言文案全量刷新 */
        void applyLanguageTexts();

    private slots:
        void slot_changeLanguage(int language);

        /** 工况→帧级联 */
        void slot_chooseCase(const QString& text);
        /** 帧导航四钮 */
        void slot_firstFrame();
        void slot_previousFrame();
        void slot_nextFrame();
        void slot_lastFrame();

        /** 对象类型切换：退出拾取+清拾取数据（云图 slot_chooseObjectType 同口径） */
        void slot_objectTypeChanged(int index);

        /** 拾取控件激活/失活收口（云图 slot_objectPickActiveChanged 同款）：激活=按类型
        * 设视口过滤器（无效后处理上下文回退失活）；失活=清过滤器+恢复工具栏拾取类型；
        * 收割/高亮/重入回显由拾取栈控制器自管 */
        void slot_objectPickActiveChanged(bool active);

        /** 齿轮跳转变形控制（原型 openContourDeformationSettings → 变形设置面板） */
        void slot_openDeformation();

        /** 分量及合量色块/字色块取色（共用取色弹窗，目标=点击色块） */
        void slot_componentColorClicked();
        void slot_fontColorClicked();

        /** 符号尺寸模式切换（右侧数值/倍率显隐） */
        void slot_sizeModeChanged(int index);

        /** 图例范围切换（原型 legendRangeControl change → refreshRange） */
        void slot_legendRangeChanged(int index);

        /** 最大/最小值重置（原型 legend-reset：回默认 1/0） */
        void slot_legendMaxReset();
        void slot_legendMinReset();

        /** 渲染图例三钮：隐藏(↔显示)/复位/自定义(展开收起图例自定义组) */
        void slot_legendVisibilityClicked();
        void slot_legendCustomClicked();

        /** 信息标签三钮（云图同款：显隐切换/恢复默认/开注释面板弹窗） */
        void slot_labelHide();
        void slot_labelReset();
        void slot_labelCustom();

    private:
        QVBoxLayout*                _mainLayout = nullptr;

        //--------- 操作条与状态行 ---------
        QPushButton*                _btnClear = nullptr;     //清除渲染（未提交时禁用）
        QPushButton*                _btnApply = nullptr;     //应用渲染（主蓝）
        bool                        _requested = false;      //已提交向量参数（内存态）
        QLabel*                     _statusLabel = nullptr;  //vector-status（空文案收起）

        //--------- 向量变量 ---------
        FoldSection*                _variableFold = nullptr;
        QComboBox*                  _comboCase = nullptr;
        QComboBox*                  _comboFrame = nullptr;
        QPushButton*                _btnFirstFrame = nullptr;
        QPushButton*                _btnPreviousFrame = nullptr;
        QPushButton*                _btnNextFrame = nullptr;
        QPushButton*                _btnLastFrame = nullptr;
        QComboBox*                  _comboVariable = nullptr;    //位移/速度/转动速度/加速度/力
        QComboBox*                  _comboLayer = nullptr;       //禁用占位
        QCheckBox*                  _checkCornerData = nullptr;  //禁用占位
        QLineEdit*                  _editDeformCoef = nullptr;   //变形系数（默认 1）
        QToolButton*                _btnOpenDeformation = nullptr; //齿轮跳转变形控制

        //--------- 对象及坐标系（对象选择=mPost 拾取栈拾取条，2026-11-18 云图同款替换） ---------
        FoldSection*                _objectFold = nullptr;
        QComboBox*                  _objectTypeCombo = nullptr;    //对象类型下拉（0=单元/1=部件；默认部件；objectName=vectorObjectType 沿用）
        mPostPickWidgetController*  _objectPickController = nullptr; //对象选择拾取控件（CBBlue 颜色块；数据进程态，消费走 getAllData()）
        mPostPartScopeField*        _btnObjectScope = nullptr;     //范围状态控制器（隐形；云图同款仅提供"所有X"默认文案+持久化口径，弹窗下线）
        QComboBox*                  _comboCoordSystem = nullptr; //全局坐标系+自定义坐标系回填（云图同款）
        QToolButton*                _btnCoordAdd = nullptr;      //快速新建坐标系（接全局信号，云图同款）
        QCheckBox*                  _checkTrackingCoord = nullptr;   //使用跟踪坐标（写 guidata，云图同款）
        QCheckBox*                  _checkMidsideValues = nullptr;   //边中点值（默认勾+禁用，云图同款）
        QWidget*                    _coordTailRow = nullptr;         //组尾双列区（跟踪坐标|边中点值）

        //--------- 绘图设置 ---------
        FoldSection*                _plotFold = nullptr;
        QList<QCheckBox*>           _componentChecks;    //X/Y/Z（indices 0..2）
        QList<QCheckBox*>           _resultantChecks;    //X+Y/Y+Z/Z+X（indices 3..5）
        QCheckBox*                  _checkTotal = nullptr;       //X+Y+Z（index 6，默认勾）
        QCheckBox*                  _checkShowValues = nullptr;  //显示数值（默认不勾）
        QCheckBox*                  _checkLabelPrefix = nullptr; //标签前缀（默认勾）
        QPushButton*                _btnDisplayOpen = nullptr;   //“设置 ›”

        //分量及合量颜色（索引对 components：0=X..6=X+Y+Z；原型 colors 常量）
        QPushButton*                _componentSwatches[7] = {};
        QColor                      _componentColors[7];

        //--------- 显示设置弹窗 ---------
        QFrame*                     _displayPopup = nullptr;
        QComboBox*                  _comboFontSize = nullptr;
        QComboBox*                  _comboFontFamily = nullptr;
        QPushButton*                _btnFontColor = nullptr;     //数值文字颜色（默认 #424b57）
        QColor                      _fontColor = QColor(0x42, 0x4b, 0x57);
        QWidget*                    _paletteRow = nullptr;       //分量及合量颜色 4 列色盘（着色=自定义才显示）
        MxFunctions::mChoiceGroup*  _choiceColorMode = nullptr;  //数值/自定义（默认数值）
        MxFunctions::mChoiceGroup*  _choiceSizeMode = nullptr;   //自动/统一/缩放（默认自动）
        QLineEdit*                  _editSizeValue = nullptr;    //符号统一尺寸（默认 1）
        QLineEdit*                  _editScaleBy = nullptr;      //符号缩放倍率（默认 1）
        MxFunctions::mChoiceGroup*  _choiceSymbolPos = nullptr;  //头部/中点/尾部（默认尾部）
        QComboBox*                  _comboHeadForm = nullptr;    //箭头/线条（默认箭头）

        //--------- 图例（控件与交互对齐云图：边界行/阶数/信息标签=共享件与同款件，2026-11-14） ---------
        FoldSection*                _legendFold = nullptr;
        QComboBox*                  _comboLegendMode = nullptr;  //模式（所有部件/显示部件，云图同款）
        MxFunctions::mChoiceGroup*  _choiceLegendRange = nullptr;//逐帧/全程/手动（默认逐帧；带说明 tooltip）
        MxFunctions::mLegendBoundRow* _legendMax = nullptr;      //最大值边界行（输入+复位+显隐）
        MxFunctions::mLegendBoundRow* _legendMin = nullptr;      //最小值边界行
        QString                     _manualMax = QStringLiteral("1");    //手动上限缓存（原型 manual）
        QString                     _manualMin = QStringLiteral("0");
        QPair<double, double>       _resolvedRange{ 0.0, 0.0 };         //最近一次应用的解析值域 <min,max>（边界行回显/复位源，随 guidata _vectorPlotResolved* 同步）
        QSpinBox*                   _spinInterpOrder = nullptr;  //插值阶数（默认 9，2..30 云图同款）
        MxFunctions::mChoiceGroup*  _choiceSmoothMode = nullptr; //平滑/离散（默认平滑）
        MxFunctions::mChoiceGroup*  _choiceSpacing = nullptr;    //等差/等比（默认等差）
        QPushButton*                _btnLegendHide = nullptr;    //隐藏 ↔ 显示
        QPushButton*                _btnLegendReset = nullptr;   //复位
        QPushButton*                _btnLegendCustom = nullptr;  //自定义（打开渲染侧图例编辑器，云图同款）
        QPushButton*                _btnLabelHide = nullptr;     //信息标签隐藏 ↔ 显示（云图同款）
        QPushButton*                _btnLabelReset = nullptr;    //信息标签复位
        QPushButton*                _btnLabelCustom = nullptr;   //信息标签自定义（开注释面板弹窗）

        //--------- 上下文 ---------
        PostData::mPostDataBase*    _postDB = nullptr;
        bool                        _langEn = false;
        bool                        _legendHidden = false;       //渲染图例隐藏态（隐藏钮；=!guidata->_isColorTableShow，随回填同步）
        bool                        _infoLabelVisible = true;    //信息标签可见态（语言刷新复设按钮文案）
        FbdColorPicker*             _colorPicker = nullptr;      //取色弹窗（共用单实例）
        QPushButton*                _colorPickerTarget = nullptr; //当前取色目标色块
    };
}
