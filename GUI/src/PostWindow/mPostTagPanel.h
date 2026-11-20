#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"
#include "mPanelBase.h"
#include "mxr/basic/PublicStruct.h"

#include <QColor>
#include <QList>
#include <QPointer>
#include <QMetaObject>

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;

namespace mxr
{
    class REBUILD_PostRend;
    enum class SelectionInfoType : uint8_t;
    class CCSMRendWidget;
    class InteractionAnnotationLabelTool;
    struct AnnotationLabelStyle;
}

namespace PostData { class mPostDataBase; }

class QComboBox;
class QPushButton;
class QVBoxLayout;
class QTableWidget;
class QLabel;
class QCheckBox;
class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;
class QPlainTextEdit;
class QTableWidgetItem;
class QHideEvent;

namespace MxFunctions
{
    class mChoiceGroup;
    class mGroupBox;
    class mPanelTable;
}

namespace PostWindow
{
    class FbdColorPicker;

    /*
    * 结果标签面板（V1.3 原型 #annotationParameters，差异裁定与遗留事项见 PostWindow.md 2026-11-11 条目）：
    * - 标签数据列表（管理折叠组）：类型筛选 + 名称搜索、复选/名称/类型/显隐(眼睛)/颜色表格，
    *   全选/已选计数/显示/隐藏/删除/新建（标签_N 递增占位条目，"保存修改"创建转正），
    *   全部标签显示设置三项（自动隐藏/底色/显示名称）；
    * - 标签定义（工具折叠组）：标签名称+颜色、数据字段+插入、标签内容多行、
    *   附着方式/附着对象三选（MxFunctions::mChoiceGroup）、拾取对象整行入口、
    *   窗口/坐标条件行、即时保存复选+取消编辑/保存修改操作行、显示样式细线框
    *   （MxFunctions::mGroupBox：字号字体/数值精度/文字对齐）；
    * - 高级设置（默认收起）：显示边框+边框样式、跟随对象移动（记录态未接）、
    *   锚定到屏幕（默认不勾选；已接引擎屏幕锚定：勾选即时作用于选中标签，
    *   创建/保存修改随样式整包）、排列方式（待接入禁用）。
        * 记录持久化（PostData 缓存/多模型隔离）为引擎侧遗留（交接文档 §4）；
        * 模型树"标签"子节点已联动（记录结构变化经 postTagRecordsChangedSig 重建
        * 分支，树子节点点击经 hub selectTagRecord 选中回填，2026-11-16 条目）。
        */
    class MPOSTWINDOW_EXPORT mPostTagPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        explicit mPostTagPanel(QWidget* parent = Q_NULLPTR);
        ~mPostTagPanel() override;

        /** 根据当前面板是否显示设置拾取开关及信号连接 */
        void setPanelPickAndSignal(bool isshow);

        /** 初始化界面（挂载后由 mPostInfoHub::createTagPanel 调用） */
        void newTagPanel(int caseId);

        /** 树"标签"分支重建源：记录快照（id, 名称）按列表序返回（记录为会话级，
        *  与面板管理表一一对应；phyID=记录 id，树侧据此建/寻子节点） */
        QVector<QPair<int, QString>> tagTreeRecords() const;

        /** 树"标签"子节点点击路由（原型：树节点点击=管理表行点击）：清搜索/类型
        *  筛选→选中该记录行滚动可见→表单回填已存值（记录不存在安全返回） */
        void selectRecord(int recordId);

    private:
        /** 标签记录（面板内存态；引擎侧 PostData 缓存为后续轮次遗留） */
        struct TagRecord
        {
            int id = -1;
            QString name;
            int typeIndex = 0;      //0节点/1单元/2部件（附着对象）
            int anchorId = -1;      //数据锚对象 id（节点/单元/部件；纯窗口/坐标创建=-1）——
                                    //{{字段}}解析与换帧取值用；放置方式切换不清除（数据关联保持）
            int attachMethod = 0;   //放置方式（0实体/1窗口/2坐标）——换帧引线跟随仅实体
                                    //（窗口无引线/坐标锚为固定坐标）；与数据锚解耦
            QColor color{ 0, 0, 0 };
            bool visible = true;
            QString content;        //标签内容（含 {{字段}} 占位）
            bool pending = false;   //"新建"占位条目（尚未在视口创建，名称列带"（待定义）"后缀）
        };

        /** 构建面板控件（语言文本由 slot_changeLanguage 设置） */
        void setupUI();

        /** 连体对（字号字体/数值精度两行用）：容器描边、内部两控件零间距、格间竖线 */
        QWidget* makeJoinedPair(QWidget* first, QWidget* second);

        /** 按当前附着对象域收集拾取结果全部 id（后处理：网格点/网格单元/网格部件） */
        bool getPickResultIDs(mxr::SelectionInfoType selType, std::vector<int>& outAllIds);

        /** 当前视口的注释标签工具（前/后处理分支；无效上下文或未注册返回 nullptr） */
        mxr::InteractionAnnotationLabelTool* annotationLabelTool();

        /** 激活注释标签工具（进入面板时调用：一击放置/拖拽/双击编辑的事件路由依赖活动工具） */
        void activateAnnotationLabelTool();

        /** 节点 id -> 世界坐标（后处理=当前帧节点坐标+位移；前处理=几何引擎顶点属性，
        *  管线与量测工具 nodeCoordinate 一致——实体依附标签的锚点来源） */
        bool nodeWorldPos(int nodeId, QVector3D& out);

        /** 拾取对象锚点 -> 世界点（0节点=nodeWorldPos；1单元=全节点均值形心；
        *  2部件=部件全部不重复节点均值（前处理=几何形心 getCentroid））——
        *  实体依附引线非文字端的落点；不可得返回 false */
        bool pickAnchorWorld(int domain, int id, QVector3D& out);

        /** 解析标签内容 {{字段}} 占位（2026-11-13 全字段）：全局字段（当前帧/
        *  仿真步/模型名称/工况名称，标注面板同源）任意附着方式可解析；对象
        *  字段（编号/名称/坐标）与结果数值需实体锚；取不到写 N/A */
        QString resolveFields(const QString& raw, int domain, int anchorId);

        /** 标签相对锚点的放置偏移（视口包围盒半径比例；取不到包围盒回退固定值） */
        QVector3D sceneLabelOffset() const;

        /** 窗口依附屏幕锚点 → 世界点（九宫格+水平/垂直%；旋转中心视深反投影，
        *  与引擎拾取空背景回退同语义；视口/相机不可用返回 false）。
        *  出屏钳制由创建流程用引擎精确包围盒（getAnnotationLabelScreenBox）执行 */
        bool windowAnchorWorld(QVector3D& out) const;

        /** 窗口标签出屏钳制（创建与"保存修改"重定位共用）：取引擎精确屏幕包围盒，
        *  越界即原视深平移回屏，任意水平/垂直百分比下文字+框+背景完整可见 */
        void clampWindowLabelIntoView(mxr::InteractionAnnotationLabelTool* tool, const QString& name);

        /** 拾取域随附着对象联动：PickFilter + 工具栏广播号（5网格点/8网格单元/9网格部件） */
        void currentPickDomain(mxr::PickFilter& filter, int& pickKind, mxr::SelectionInfoType& selType) const;

        /** 面板隐藏（切到其他面板）时退出拾取模式 */
        void hideEvent(QHideEvent* event) override;

        /** 面板显示（进入标签面板）时激活注释标签工具 */
        void showEvent(QShowEvent* event) override;

        /** 空态占位标签随表格 viewport 宽度 */
        bool eventFilter(QObject* watched, QEvent* event) override;

        /** 退出拾取模式：取消拾取入口勾选并恢复 PickNothing */
        void exitPickMode();

        /** 状态行写入（空文案整行收起，对齐原型空态不留白） */
        void setStatusLabel(const QString& text);

        /** 状态行刷新拾取进度（已选对象清单） */
        void updatePickStatus();

        /** 拾取入口文案随附着对象域联动（armed=进行中文案） */
        QString pickEntryLabel(bool armed, int lang) const;

        /** 列表当前选中行对应的记录下标（无选中返回 -1） */
        int selectedRecordIndex() const;

        /** 按面板当前输入组装引擎标签样式（创建与"保存修改"同口径） */
        mxr::AnnotationLabelStyle currentLabelStyle() const;

        /** 即时保存（勾选态）：面板编辑即时应用到列表选中标签的文字+文字框+
        *  背景并同步记录；未勾选/无选中/空内容不动作（应用走"保存修改"） */
        void instantApplyToSelection();

        /** 按记录已存值回填面板表单（"取消编辑"用） */
        void formFromRecord(const TagRecord& rec);

        /** 创建路径记录写入（upsert）：同名占位条目转正（更新附着/内容/颜色并清
        *  pending），无同名才新增列表行——与渲染层标签保持一一对应 */
        void upsertRecordRow(const QString& labelName, int anchorId);

        /** 按记录插入列表行并选中滚动可见（记录须已入 _records；upsert 新增分支
        *  与"新建"占位条目共用），返回可见行号 */
        int appendRecordRow(const TagRecord& rec);

        /** 拾取入口/清空按钮文案与可用性刷新 */
        void refreshPickEntryText();

        /** 记录 id 对应的列表数据下标（不存在返回 -1） */
        int dataIndexById(int id) const;

        /** 按行数据渲染管理列表各单元格 */
        void renderRow(int row);

        /** 管理列表空表提示与显隐同步 */
        void refreshEmptyState();

        /** 管理列表联动：过滤显行、已选计数与按钮启停 */
        void syncManagerState();

        /** 管理列表按当前筛选条件显隐行 */
        void applyManagerFilter();

        /** 勾选行整行高亮 */
        void applyCheckedRowBackground(int row);

        /** 勾选记录批量显隐（显示/隐藏按钮） */
        void setCheckedRecordsVisible(bool visible);

        /** 附着对象类型文案（0节点/1单元/2部件） */
        QString typeName(int typeIndex, int lang) const;

        /** 弹窗贴锚点控件下方显示（越界收回屏内） */
        void popupUnder(QWidget* popup, QWidget* anchor);

        /** 色块按钮换色（内联样式刷新底色） */
        void applySwatchColor(QPushButton* swatch, const QColor& color);

        /** 记录结构变化（新建/转正/删除）后通知后处理树重建"标签"分支子节点 */
        void notifyTreeRecordsChanged();

    private slots:
        /** 切换中英文 */
        void slot_changeLanguage(int language);

        /** 换帧跟随（postCloudFrameChanged，标注面板同款信号）：动画播放/云图切帧时
        *  实体+节点锚且含 {{字段}} 的标签重取当前帧值刷新文字（{{结果数值}} 跟随动画） */
        void slot_frameChanged(int frame);

        /** 拾取入口：点击进入/退出拾取（程序化调用可显式传 true 进入拾取态） */
        void slot_pickEntryClicked(bool checked = false);

        /** 接收拾取信号 */
        void slot_finishPick(mxr::FinishPickInfo info);

        /** 创建/同名更新标签（原"创建"语义；"保存修改"无选中记录时的创建分支） */
        void slot_createButton();

        /** 取消编辑按钮：面板表单回退到列表选中标签的已存值 */
        void slot_cancelEditClicked();

        /** 保存修改按钮：把面板编辑应用到选中标签——文字+文字框+背景（样式整包），
        *  附着方式参数（窗口=标签重定位/坐标=仅改引线锚点/实体=新拾取改绑锚点）；
        *  无选中记录=创建新标签 */
        void slot_applyEditClicked();

        /** 数据字段插入按钮：把 {{当前字段}} 插入标签内容光标处 */
        void slot_insertFieldClicked();

        /** 附着方式切换：窗口/坐标条件行显隐 + 退出拾取 */
        void slot_attachMethodChanged(int index);

        /** 附着对象切换：拾取域联动 + 清空已选 */
        void slot_attachObjectChanged(int index);

        /** 搜索框过滤标签记录 */
        void slot_searchChanged(const QString& text);

        /** 类型筛选切换标签记录 */
        void slot_typeFilterChanged(int index);

        /** 全选按钮：全勾/全不勾可见记录 */
        void slot_selectAllClicked();

        /** 显示/隐藏按钮：勾选记录批量显隐 */
        void slot_showSelectedClicked();
        void slot_hideSelectedClicked();

        /** 删除按钮：移除勾选记录 */
        void slot_deleteButton();

        /** 新建按钮：列表新增占位条目（标签_N 递增命名），名称回填标签定义区 */
        void slot_newButton();

        /** 列表复选变化：同步已选计数 */
        void slot_itemChanged(QTableWidgetItem* item);

        /** 颜色单元格点击：弹出取色弹窗 */
        void slot_showColorPopup(int row);

        /** 标签名称行色块取色 */
        void slot_nameColorClicked();

        /** 显示边框勾选联动边框样式下拉可用性 */
        void slot_borderEnabledChanged(int state);

    private:
        QVBoxLayout*               _mainLayout{};      //主布局

        //--------- 标签数据列表 ---------
        FoldSection*               _manageFold{};      //管理折叠组
        QComboBox*                 _typeFilterCombo{}; //类型筛选
        QLineEdit*                 _searchEdit{};      //搜索名称
        MxFunctions::mPanelTable*       _table{};           //记录列表（复选/名称/类型/显隐/颜色；共享管理表格件，空态覆盖层内置）
        QPushButton*               _btnSelectAll{};    //全选
        QPushButton*               _btnShow{};         //显示
        QPushButton*               _btnHide{};         //隐藏
        QPushButton*               _btnDelete{};       //删除
        QPushButton*               _btnNew{};          //新建（蓝实心；新增标签_N 占位条目）
        QLabel*                    _managerStatusLabel{};   //已选 N 项状态行
        QCheckBox*                 _checkAutoHide{};   //全局：自动隐藏
        QCheckBox*                 _checkFill{};       //全局：底色
        QCheckBox*                 _checkShowName{};   //全局：显示名称

        //--------- 标签定义 ---------
        FoldSection*               _toolFold{};        //定义折叠组
        PanelLabel*                _nameRowLabel{};    //标签名称行标签
        QLineEdit*                 _nameEdit{};        //标签名称
        QPushButton*               _nameColorSwatch{}; //标签文字颜色色块
        PanelLabel*                _fieldRowLabel{};   //数据字段行标签
        QComboBox*                 _fieldCombo{};      //数据字段
        QPushButton*               _btnInsertField{};  //插入数据字段（＋）
        PanelLabel*                _contentRowLabel{}; //标签内容行标签
        QPlainTextEdit*            _contentEdit{};     //标签内容（4 行）
        PanelLabel*                _methodRowLabel{};  //附着方式行标签
        MxFunctions::mChoiceGroup* _methodChoice{};    //附着方式（实体/窗口/坐标）
        PanelLabel*                _objectRowLabel{};  //附着对象行标签
        MxFunctions::mChoiceGroup* _objectChoice{};    //附着对象（节点/单元/部件）
        PanelLabel*                _pickRowLabel{};    //拾取对象行标签
        QWidget*                   _pickRowHost{};     //拾取行容器（PickEntryRow；setRowVisible 行定位用）
        QPushButton*               _pickEntry{};       //拾取对象入口（"请选择"按钮，图2样式）
        PanelLabel*                _windowAnchorLabel{};   //窗口定位行标签（条件行）
        QComboBox*                 _windowAnchorCombo{};    //窗口定位（窗口附着条件行）
        PanelLabel*                _windowXLabel{};    //水平位置行标签
        QSpinBox*                  _windowXSpin{};     //水平位置（%）
        PanelLabel*                _windowYLabel{};    //垂直位置行标签
        QSpinBox*                  _windowYSpin{};     //垂直位置（%）
        PanelLabel*                _systemLabel{};     //坐标系行标签（条件行）
        QComboBox*                 _systemCombo{};     //坐标系（坐标附着条件行）
        PanelLabel*                _coordXLabel{};     //X 行标签
        QDoubleSpinBox*            _coordXSpin{};      //X
        PanelLabel*                _coordYLabel{};     //Y 行标签
        QDoubleSpinBox*            _coordYSpin{};      //Y
        PanelLabel*                _coordZLabel{};     //Z 行标签
        QDoubleSpinBox*            _coordZSpin{};      //Z
        QCheckBox*                 _checkInstantSave{};//即时保存（图2：复选框；勾选=面板编辑即时应用到选中标签）
        QPushButton*               _btnCancelEdit{};   //取消编辑（表单回退到选中标签已存值）
        QPushButton*               _btnApplyEdit{};    //保存修改（选中=应用文字+框+背景；无选中=创建新标签）
        QLabel*                    _statusLabel{};     //状态行

        //--------- 显示样式（标签定义内细线框） ---------
        MxFunctions::mGroupBox*    _styleBox{};        //细线框（点击标题折叠）
        PanelLabel*                _fontStyleLabel{};  //字号字体行标签
        QComboBox*                 _fontSizeCombo{};   //字号
        QComboBox*                 _fontFamilyCombo{}; //字体
        PanelLabel*                _precisionStyleLabel{}; //数值精度行标签
        QSpinBox*                  _precisionSpin{};   //精确位数
        QComboBox*                 _formatCombo{};     //计数方式
        PanelLabel*                _alignStyleLabel{}; //文字对齐行标签
        MxFunctions::mChoiceGroup* _alignChoice{};     //文字对齐（左/中/右）

        //--------- 高级设置（默认收起） ---------
        FoldSection*               _advFold{};         //高级设置折叠组
        QCheckBox*                 _checkBorder{};     //显示边框
        QComboBox*                 _borderCombo{};     //边框样式（无/实线/虚线）
        QCheckBox*                 _checkFollow{};     //跟随对象移动
        QCheckBox*                 _checkAnchor{};     //锚定到屏幕
        PanelLabel*                _arrangeLabel{};    //排列方式行标签
        QComboBox*                 _arrangeCombo{};    //排列方式（待接入禁用）

        //--------- 标签记录 ---------
        QList<TagRecord>           _records;           //记录数据（行 UserRole 存 id 关联）
        int                        _nextId{ 1 };       //记录 id 计数
        int                        _nextLabelNum{ 1 }; //"新建"条目编号计数（标签_N；只增不回收）
        QList<int>                 _pickedIds;         //当前拾取收集的对象 id（按拾取序）

        QMetaObject::Connection    _pickConn{};        //拾取信号连接

        bool                       _isCurrPostViewport{ true };    //当前是不是后处理窗口
        bool                       _isCurrStateValid{ false };     //当前是不是处于可用状态
        //前处理时
        QPointer<mxr::CCSMRendWidget> _preRend;         //前处理视口
        //后处理时
        QPointer<mxr::REBUILD_PostRend> _postRend;      //后处理视口
        QPointer<PostData::mPostDataBase> _postDB;      //后处理数据库（节点坐标查询用）

        //--------- 颜色拾取弹窗 ---------
        FbdColorPicker*            _colorPicker{};     //取色弹窗
        int                        _colorRecordId{ -1 };    //正在取色的记录id（-1=标签名称色块）

        int                        _caseId{ -1 };      //上下文工况 id（仅记录）
    };
}
