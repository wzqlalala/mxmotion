#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"
#include "mPanelBase.h"

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
class QVBoxLayout;
class QGridLayout;
class QLabel;
class QLineEdit;
class QCheckBox;
class QTableWidget;
class QHideEvent;

namespace MxFunctions
{
    class mChoiceGroup;
    class mGroupBox;
}

namespace mxr
{
    class REBUILD_PostRend;
    enum class PickFilter : unsigned long long;   //PublicStruct.h（不透明声明，成员仅存值传递）
}

namespace MechPreData
{
    class mPostGUIData;
}

namespace PostWindow
{
    class FbdColorPicker;
    class mPostObjectEditController;

    /*
    * 点草稿（原型 createPointControl 状态：文本+未应用标记）。
    * 置于命名空间级：渲染装配（detail::sectionCut*）与无头测试需直接构造。
    */
    struct SectionCutPointDraft
    {
        QString text;       //输入框当前文本
        bool dirty = false; //有未应用编辑
    };

    /*
    * 截面剖切参数草稿（编辑缓冲与已保存配置共用结构，V1.3 原型 draft）。
    * 置于命名空间级同 SectionCutPointDraft。
    */
    struct SectionCutDraft
    {
        QString name = QStringLiteral("Section_1");
        int scope = 0;                          //0 全部/1 部件/2 局部
        QStringList scopeParts;                 //部件拾取（视觉态）
        QStringList scopeElements;              //单元拾取（视觉态）
        int definition = 0;                     //0 法向轴/1 三点平面
        int normalMode = 0;                     //0 X轴/1 Y轴/2 Z轴/3 起点-终点/4 选择对象/5 向量
        bool reversed = false;
        QString vectorText = QStringLiteral("1, 0, 0");
        QString startText = QStringLiteral("0, 0, 0");
        QString endText = QStringLiteral("1, 0, 0");
        SectionCutPointDraft basePoint{ QStringLiteral("0, 0, 0") };		//创建记录时由模型 AABB 中心覆盖（无模型退化为原点）
        SectionCutPointDraft point1{ QStringLiteral("0, 0, 0") };
        SectionCutPointDraft point2{ QStringLiteral("1, 0, 0") };
        SectionCutPointDraft point3{ QStringLiteral("0, 1, 0") };
        int configuration = 0;                  //0 固定平面/1 随变形移动
        int side = 2;                           //0 负向侧/1 两侧/2 正向侧（原型默认 positive）
        QString offset = QStringLiteral("0");
        QString step = QStringLiteral("1");
        int styleMode = 0;                      //0 单平面/1 截面带
        QString bandwidth = QStringLiteral("1");
        bool fill = true;
        int colorMode = 0;                      //0 结果着色/1 统一颜色
        QColor color{ 0x16, 0x77, 0xC8 };
        bool crop = true;
        bool ghost = false;
        bool showPlane = false;
        bool showNormal = false;
        bool showMesh = false;
        bool showEdges = true;
    };

    /*
    * 截面剖切纯逻辑（渲染装配与 tests/PostWindow 无头断言共用；与 UI 控件解耦）。
    * 全部函数不触视口/渲染/结果库——渲染提交链（applySectionCuts 族）之外的部分。
    */
    namespace detail
    {
        /** 解析 "X, Y, Z"（逗号/中英文逗号/空白分隔，三个有限数；失败返回空表） */
        QList<double> MPOSTWINDOW_EXPORT parsePoint(const QString& text);

        /** 单数值文本有效（非空/可解析/有限） */
        bool MPOSTWINDOW_EXPORT finiteText(const QString& text);

        /** 平面法向 L13 校验问题码（仅向量/起点-终点模式产生文案） */
        enum class NormalIssue { None, InvalidComponents, ZeroDirection };
        NormalIssue MPOSTWINDOW_EXPORT normalIssue(const SectionCutDraft& d);

        /** 几何校验问题码（面板 geometryError 的无 UI 版；判定顺序与文案链一致） */
        enum class GeomIssue
        {
            None, PendingPoints, InvalidBase, NoDirectionObject, InvalidNormal,
            InvalidThreePoints, PointsOutOfRange, CollinearPoints
        };
        GeomIssue MPOSTWINDOW_EXPORT geometryIssue(const SectionCutDraft& d);

        /** 草稿规范化签名（统一编辑契约 dirty 检测；含未应用点编辑标记） */
        QString MPOSTWINDOW_EXPORT sectionCutSignature(const SectionCutDraft& d);

        /** 解算单位法向：法向轴分支六模式（选择对象暂无数据源返回 false）；
        * 三点平面 = (P2−P1)×(P3−P1) 归一。几何无效时返回 false */
        bool MPOSTWINDOW_EXPORT sectionCutNormal(const SectionCutDraft& d, QVector3D& normal);

        /** 剖切基准点：法向轴分支=基点；三点平面=点1（解析失败返回 false） */
        bool MPOSTWINDOW_EXPORT sectionCutBasePoint(const SectionCutDraft& d, QVector3D& base);

        /** 平面落点 = 基点 + 法向×偏移 + 变形平移（随变形构型由调用方算 shift 传入） */
        QVector3D MPOSTWINDOW_EXPORT sectionCutVertex(const SectionCutDraft& d, const QVector3D& normal,
            const QVector3D& deformShift = QVector3D());

        /** 保留侧三态折算（reversed 翻转 0↔2）：0 负向侧/1 两侧/2 正向侧 */
        int MPOSTWINDOW_EXPORT effectiveKeepSide(int side, bool reversed);

        /** 单平面是否参与模型裁剪（"裁剪单元"折算）：裁剪关或保留两侧=不裁（零方程占位） */
        bool MPOSTWINDOW_EXPORT effectiveClipModel(bool crop, int side);

        /** 截面带两平面落点（first=偏移−带宽/2、second=偏移+带宽/2；两平面法向对夹出板带） */
        QPair<QVector3D, QVector3D> MPOSTWINDOW_EXPORT bandVertices(const SectionCutDraft& d,
            const QVector3D& normal, const QVector3D& deformShift = QVector3D());

        /** 最近节点位移（随变形构型平移近似：基点就近节点的当前帧位移；空表返回零向量） */
        QVector3D MPOSTWINDOW_EXPORT nearestNodeDisplacement(const QVector3D& base,
            const QHash<int, QVector3D>& nodePositions, const QHash<int, QVector3D>& displacements);
    }

    /*
    * 截面剖切渲染三入口（面板激活/保存/删除与云图切帧/重绘插桩共用，向量图同款）：
    * - applySectionCuts：按 guidata._sectionCutSpecs 全量重建——先按 _sectionCutEngineIds
    *   精删旧对象（与旧切面图共用 CuttingPlane 家族，禁全家删），再逐记录组装引擎参数
    *   提交（单平面 1 个/截面带双平面对夹 2 个；保留侧→ReverseNormal；激活表空=仅清理）。
    *   截面带/单平面均提交封盖截面（引擎切片随当前变量结果着色）；随变形构型按当前帧
    *   位移场对基点做最近节点平移近似。引擎 GL clip 槽位上限 7，超限截断并输出窗提示。
    * - removeSectionCuts：按台账 id 精删全部剖切对象+开关复位（参数投影保留）。
    * - refreshSectionCutIfActive：_sectionCutActive 时重提交（切帧/变量/变形系数变化后
    *   重切跟随；mPost3DResult 插桩调用）。
    */
    bool MPOSTWINDOW_EXPORT applySectionCuts(mxr::REBUILD_PostRend* postRend, MechPreData::mPostGUIData* guidata);
    void MPOSTWINDOW_EXPORT removeSectionCuts(mxr::REBUILD_PostRend* postRend, MechPreData::mPostGUIData* guidata);
    void MPOSTWINDOW_EXPORT refreshSectionCutIfActive(mxr::REBUILD_PostRend* postRend, MechPreData::mPostGUIData* guidata);

    /*
    * 截面剖切参数面板（结果页"空间显示"组入口；V1.3 原型 #sectionCutParameters，
    * 前处理原型内嵌后处理文档运行时构建）。
    *
    * V1.3（2026-11-12 修订）结构：
    * - 截面管理折叠（#sectionCutManagerSection）：搜索 + 管理表（复选/名称[可排序]/
    *   激活）+ 全选/计数/删除/新建 + 状态行；多截面记录，新建立即登记 Section_N 并
    *   选中激活；编辑对象/删除勾选/激活状态三者独立，多截面可同时激活分别停用；
    *   切换回填各自保存参数（空态"暂无截面/无匹配截面"）；
    * - 编辑器（三折叠组）：
    *   剖切定义：截面名称+截面颜色(26 色块)、剖切范围（全部/部件/局部 + 连体拾取钮）；
    *   剖切几何定义：细线框"基准剖切平面"（定义方式 法向轴/三点平面；法向轴分支 =
    *   平面法向下拉(X轴/Y轴/Z轴/起点-终点/选择对象/向量)+反向钮+条件行、基点点控件；
    *   三点分支 = 点1..3 点控件）、剖切面偏移(mm)、移动步长(−/步长/＋ 连体 1:2:1)、
    *   几何构型（固定平面/随变形移动）、保留侧（负向侧/两侧/正向侧）；
    * - 参数操作区（#sectionCutEditCancel/#sectionCutSubmit，位于截面样式上方）：
    *   统一编辑契约（mPostObjectEditController）——即时保存复选 + 取消编辑/保存修改
    *   整行等宽双按钮 + 状态行；无修改置灰、未保存切换三选一确认、即时保存直写；
    * - 截面样式：剖切模式（单平面/截面带，带模式显带宽行）、填充截面复选+填充方式
    *   （结果着色/统一颜色）联动色块、细线框"裁剪显示"（裁剪单元/剔除侧半透明）、
    *   细线框"辅助显示"（剖切平面/法向/截面网格/模型特征边）。
    *
    * 数据：记录（编辑缓冲+已保存配置）按结果模型独立保存（原型 records 语义，
    * 面板内存不持久化）；dirty=编辑缓冲签名 vs 已保存签名；取消恢复已保存配置。
    * 引擎未接入（原型即纯配置页无提交点）：剖切渲染与真实拾取为遗留项，
    * 拾取入口仅武装态视觉，Esc 退出（原型 keydown Escape endPick）。
    */
    class MPOSTWINDOW_EXPORT mPostSectionCutPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        explicit mPostSectionCutPanel(QWidget* parent = nullptr);

        /** 打开/刷新面板（入口点击；会话记录按当前后处理视口结果库恢复，无记录=原型空态） */
        void updateSectionCutPanel();

    protected:
        /** 面板隐藏=退出拾取武装态（宿主 setMainWidget 切换面板时触发） */
        void hideEvent(QHideEvent* event) override;

    private:
        /** 截面记录（原型 records 项：编辑缓冲 editor.draft 与已保存配置 applied 分开，
        *  激活读取已保存配置，取消恢复已保存值） */
        struct SectionRecord
        {
            QString id;
            QString name;
            SectionCutDraft editorDraft;    //当前编辑缓冲（含未应用点编辑）
            SectionCutDraft saved;          //已保存配置
        };

        /** 每结果模型独立会话（原型"每个结果模型独立保留会话草稿"） */
        struct ModelSession
        {
            QList<SectionRecord> records;
            QStringList checkedIds;         //勾选（仅批量删除）
            QSet<QString> activeIds;        //激活（与编辑/勾选独立）
            QString currentId;              //当前编辑记录
            int nextId = 1;
        };

        /** 点控件组（原型 createPointControl：拾取钮+应用钮行、坐标输入、错误行） */
        struct PointField
        {
            QPushButton* pick = nullptr;    //拾取点（蓝条入口钮，可 check）
            QPushButton* apply = nullptr;   //应用（40px）
            QLineEdit* edit = nullptr;      //坐标 X, Y, Z
            QLabel* error = nullptr;        //单点错误行（空文案收起）
        };

        /** 构建面板控件（语言文本由 slot_changeLanguage 设置） */
        void setupUI();
        void setupManagerSection();
        void setupEditorHost();

        /** 语言切换标签登记（显式指针；findChildren 按序猜测会随嵌套结构漂移，
        *  首轮实测把盒组内标签误刷成组外行文案） */
        struct LabelText { PanelLabel* label; const char* cn; const char* en; };
        void regLabel(PanelLabel* label, const char* cn, const char* en);
        QList<LabelText> _labelTexts;

        /** 基准剖切平面分支内构建一组点行（label+拾取/应用行+坐标行+错误行）；
        *  index：0 基点 / 1..3 点1..3（接线 lambda 捕获，刷新走 refreshAllPointFields） */
        void buildPointRows(QGridLayout* grid, int& row, int index);

        /******** 会话与记录（V1.3 统一编辑契约） ********/
        /** 当前结果库名（无后处理视口时空串=公共默认会话） */
        QString currentDraftKey() const;
        ModelSession& session();
        const ModelSession& session() const;
        SectionRecord* currentRecord();
        const SectionRecord* currentRecord() const;
        /** 当前记录编辑缓冲（无记录时返回静态默认，只读） */
        SectionCutDraft& working();
        const SectionCutDraft& working() const;

        /** 新建记录默认几何文本：基点=当前模型 AABB 中心，三点平面以中心为锚
        *  （默认平面过模型中心，杜绝默认值落在模型外致全模型被裁；无模型退化为原点） */
        void applyDefaultPointTexts(SectionCutDraft& d);

        void loadDraftToUI(const SectionCutDraft& d);
        void storeUIToDraft();

        /** 名称校验（原型 nameError：空/查重） */
        QString nameError() const;

        /** 管理表渲染/状态（搜索过滤+名称排序+空态+当前行高亮+激活钮） */
        void renderManagerTable();
        void refreshManagerState();
        void refreshManagerStatus();
        QList<const SectionRecord*> visibleRecords() const;

        /** 统一编辑动作（原型 beginCreate/select/cancelEdit/commit） */
        void beginCreate();
        void selectRecord(const QString& id);
        void cancelEdit();
        bool commitEdit(bool immediate);
        void removeChecked();
        void setActive(const QString& id, bool on);

        /** 激活记录 → guidata._sectionCutSpecs 全量重投影并提交渲染
        *  （applySectionCuts；激活/保存/删除/新建后调用。无后处理视口时静默跳过——
        *  会话仍在面板内存，进入视口后激活生效） */
        void syncActiveSectionsToRender();

        /** 校验（原型 validate/geometryError）：错误行文案、± 按钮启停；返回整体有效 */
        bool validate();
        QString geometryError() const;

        /** 平面法向 L13 校验文案（向量/起点-终点非法；空=有效） */
        QString normalErrorMessage() const;

        /** ± 步进（原型 move：offset ± step，14 位有效数字回写） */
        void moveOffset(int direction);

        /** 定义方式分支切换（原型 syncDefinition：分支显隐+控件启停+校验） */
        void syncDefinitionBranch();

        /** 平面法向模式切换（原型 L13 render：条件行显隐） */
        void syncDirectionExtras();

        /** 点控件刷新（启用态/应用钮/错误行/armed 文案），按 _activePoint/分支有效性 */
        void refreshAllPointFields();
        SectionCutPointDraft& pointDraftAt(int index);
        const SectionCutPointDraft& pointDraftAt(int index) const;

        /** 剖切范围拾取钮文案/可用性（原型 refreshPick） */
        void refreshScopePick();

        /** 截面样式联动（原型 refreshStyle：带宽行显隐、填充方式/色块/半透明启停） */
        void refreshStyle();

        /** 结束拾取武装态（范围拾取 + 点拾取，原型 endPick） */
        void endPick();

        /** 拾取武装（点拾取/范围拾取共用）：保存视口原过滤器并下发新过滤器；
        *  连接建立由各武装入口自理（回传通道随拾取类型不同），返回 false=无可用视口 */
        bool armPick(mxr::PickFilter filter);
        /** 解除武装：恢复过滤器+清拾取高亮+断开回传连接（未武装时安全空操作） */
        void disarmPick();

        /** 点拾取回传（基点/点1..3）：节点坐标含当前帧位移，回填并置已应用 */
        void handlePointPicked(int index);

        /** 范围拾取回传：容器累计集 → 草稿 scopeParts/scopeElements（id 串） */
        void handleScopePicked();

        /** 查看已选（弹窗列表：部件/单元 id）/ 清空已选（清集+清高亮） */
        void showScopePickList();
        void clearScopePick();

        /** 色块按钮换色（内联样式刷新底色） */
        void applySwatchColor(QPushButton* swatch, const QColor& color);

        /** 弹窗贴锚点控件下方显示（越界收回屏内；标签面板同款） */
        void popupUnder(FbdColorPicker* popup, QWidget* anchor);

        /** 语言文案全量刷新 */
        void applyLanguageTexts();

    private slots:
        void slot_changeLanguage(int language);

        /** 剖切范围切换：退出拾取 + 拾取钮联动 */
        void slot_scopeChanged(int index);

        /** 范围拾取入口：进入/退出武装态（原型 pick click） */
        void slot_scopePickClicked(bool checked = false);

        /** 定义方式切换 */
        void slot_definitionChanged(int index);

        /** 平面法向模式切换/反向 */
        void slot_normalModeChanged(int index);
        void slot_reverseClicked(bool checked);

        /** ± 步进 */
        void slot_stepDecrease();
        void slot_stepIncrease();

        /** 剖切模式/填充截面/裁剪单元联动 */
        void slot_styleModeChanged(int index);
        void slot_fillToggled(bool checked);
        void slot_cropToggled(bool checked);

        /** 截面颜色取色 */
        void slot_colorSwatchClicked();

        /** 管理表：搜索/全选/删除/新建/排序/行点击 */
        void slot_searchChanged(const QString& text);
        void slot_selectAllClicked();
        void slot_deleteClicked();
        void slot_newClicked();
        void slot_headerClicked(int section);
        void slot_cellClicked(int row, int column);

    private:
        QVBoxLayout*                _mainLayout = nullptr;

        //--------- 截面管理（V1.3） ---------
        FoldSection*                _managerFold = nullptr;
        QLineEdit*                  _searchEdit = nullptr;       //搜索截面名称
        QTableWidget*               _managerTable = nullptr;     //复选/名称/激活
        QPushButton*                _selectAllBtn = nullptr;
        QLabel*                     _selectedCount = nullptr;    //已选 N 项（筛选外 M）
        QPushButton*                _deleteBtn = nullptr;
        QPushButton*                _newBtn = nullptr;           //新建（主蓝）
        QLabel*                     _managerStatus = nullptr;    //校验问题状态行（红）
        int                         _sortOrder = 0;              //名称排序：0 原序/1 升/2 降
        bool                        _renderingTable = false;     //渲染期间屏蔽表信号

        //--------- 编辑器宿主（三折叠组；无记录时隐藏） ---------
        QWidget*                    _editorHost = nullptr;
        mPostObjectEditController*  _editor = nullptr;           //统一编辑契约

        //--------- 剖切定义 ---------
        FoldSection*                _definitionFold = nullptr;
        QLineEdit*                  _nameEdit = nullptr;
        QPushButton*                _colorSwatch = nullptr;     //截面颜色 26×26
        QComboBox*                  _scopeCombo = nullptr;      //全部/部件/局部
        QWidget*                    _scopePickHost = nullptr;   //连体拾取钮框（叠放钮宿主）
        QPushButton*                _scopePick = nullptr;       //连体拾取钮（拾取部件/单元）
        QPushButton*                _scopePickInspect = nullptr;//查看已选（禁用占位，叠于拾取钮框内右侧）
        QPushButton*                _scopePickClear = nullptr;  //清空已选（叠于拾取钮框内最右）

        //--------- 剖切几何定义 ---------
        FoldSection*                _geometryFold = nullptr;
        MxFunctions::mGroupBox*     _planeBox = nullptr;        //基准剖切平面细线框
        MxFunctions::mChoiceGroup*  _definitionChoice = nullptr;//法向轴/三点平面
        QWidget*                    _axisBranch = nullptr;      //法向轴分支
        QWidget*                    _threeBranch = nullptr;     //三点平面分支（默认隐藏）
        QComboBox*                  _normalCombo = nullptr;     //X轴/Y轴/Z轴/起点-终点/选择对象/向量
        QPushButton*                _reverseBtn = nullptr;      //反向（26，checkable）
        QWidget*                    _vectorRow = nullptr;       //条件行：方向向量
        QLineEdit*                  _vectorEdit = nullptr;
        QWidget*                    _startRow = nullptr;        //条件行：起点
        QLineEdit*                  _startEdit = nullptr;
        QWidget*                    _endRow = nullptr;          //条件行：终点
        QLineEdit*                  _endEdit = nullptr;
        QWidget*                    _objectRow = nullptr;       //条件行：选择对象（禁用占位）
        QLabel*                     _normalError = nullptr;     //平面法向错误行（L13）
        PointField                  _basePoint;
        PointField                  _threePoints[3];
        QLabel*                     _definitionError = nullptr; //几何校验错误行
        QLineEdit*                  _offsetEdit = nullptr;      //剖切面偏移（mm 后缀）
        QLineEdit*                  _stepEdit = nullptr;        //移动步长（连体中段）
        QPushButton*                _stepDec = nullptr;
        QPushButton*                _stepInc = nullptr;
        MxFunctions::mChoiceGroup*  _configChoice = nullptr;    //固定平面/随变形移动
        MxFunctions::mChoiceGroup*  _sideChoice = nullptr;      //负向侧/两侧/正向侧

        //--------- 截面样式 ---------
        FoldSection*                _styleFold = nullptr;
        MxFunctions::mChoiceGroup*  _modeChoice = nullptr;      //单平面/截面带
        QLineEdit*                  _bandwidthEdit = nullptr;   //带宽（条件行，setRowVisible 控制）
        QCheckBox*                  _fillCheck = nullptr;       //填充截面（行标签位）
        MxFunctions::mChoiceGroup*  _colorModeChoice = nullptr; //结果着色/统一颜色
        MxFunctions::mGroupBox*     _cropBox = nullptr;         //裁剪显示细线框
        QCheckBox*                  _cropCheck = nullptr;
        QCheckBox*                  _ghostCheck = nullptr;
        MxFunctions::mGroupBox*     _auxBox = nullptr;          //辅助显示细线框
        QCheckBox*                  _auxChecks[4] = {};         //剖切平面/法向/截面网格/模型特征边

        //--------- 会话与上下文 ---------
        QHash<QString, ModelSession> _sessions;
        QString                     _sessionKey;                //当前会话键（结果库名）
        QString                     _begunKey;                  //已取基线的会话键（重复进入不重置 dirty）
        QString                     _begunId;                   //已取基线的记录 id
        bool                        _loadingDraft = false;      //回填期间屏蔽控件信号写回
        bool                        _syncingUI = false;         //程序回设控件文本期间屏蔽 dirty 标记
        bool                        _langEn = false;

        //拾取武装态（同一时刻至多一个：范围拾取或某个点拾取）
        bool                        _scopePicking = false;
        int                         _activePoint = -1;          //-1 无；0 基点；1..3 三点

        //拾取武装的视口侧状态（armPick 置位；endPick 经 disarmPick 恢复）
        bool                        _pickFilterSaved = false;   //已保存视口原过滤器
        mxr::PickFilter             _savedPickFilter{};         //武装前的视口过滤器
        class QMetaObject::Connection _pickConn;                //回传信号连接（同刻至多一个）
        QStringList                 _scopeCarriedIds;           //范围拾取携带集（退出武装后容器被清，重武装时并回草稿）

        //--------- 颜色拾取弹窗 ---------
        FbdColorPicker*             _colorPicker = nullptr;
    };
}
