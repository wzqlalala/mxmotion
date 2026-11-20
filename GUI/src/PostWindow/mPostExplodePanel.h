#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

#include <QWidget>
#include <QPointer>
#include <QHash>
#include <QVector3D>
#include <set>

#include "mPanelBase.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


namespace PostData
{
    class mPostDataBase;
};

class QCheckBox;
class QComboBox;
class QEvent;
class QHideEvent;
class QLabel;
class QLineEdit;
class QPushButton;
class QShowEvent;
class QTableWidget;
class QTableWidgetItem;

namespace MxFunctions
{
    class mGroupBox;
    class mDirectionRow;
    class mPanelTable;
    class mPrimaryButton;
}

namespace PostWindow
{
    class mPostObjectEditController;
    class mPostPartScopeField;
    class mPostPickWidgetController;

    /*
    * 爆炸视图参数栏（对齐 V1.3 #explosionParameters 运行时重排终态，共享控件版）：
    * 折叠组"爆炸图管理"（搜索+M2 管理表：复选列 22/名称[点击编辑/↕排序]/操作列 62，
    *   尾 全选+已选计数 ‖ 删除 40 ‖ 新建爆炸图主蓝）+可折叠"爆炸图控制"
    *   （爆炸图名称 + 选择部件=mPost 通用拾取条[隐形 mPostPartScopeField 文案控制器]
    *   + 爆炸模式 + 距离 + 平移方向=mGroupBox 骑线框
    *   [mDirectionRow 定义方式+反向切换钮；起点/终点各两行"拾取x|选择节点""x坐标|输入"；
    *   选择对象/向量条件行] + 缩放方式 + 变形系数[单值 ‖ X/Y/Z方向系数三行独立行] +
    *   内联错误行 + 自适应视图‖复位40‖−26‖＋26主蓝 + 统一编辑契约 footer）。
    * 外部接口与旧面板保持一致。
    */
    class MPOSTWINDOW_EXPORT mPostExplodePanel : public mPanelBase
    {
        Q_OBJECT
    public:
        explicit mPostExplodePanel(QWidget* parent = nullptr);

        void newExplodePanel(int caseid);
        void setCurViewName(const QString& viewName);

        void setExplodeTranslate(int x, int y, int z);
        void setExplodeModelCenterConsistent(int value);
        void setExplodeModelCenterInconsistent(int x, int y, int z);
        void setExplodeAutoFit(int type);
        void setExplodePlus();
        void setExplodeMinus();
        void setExplodeReset();
        void setExplodeAutoGenerate();
        void setExplodeTranslateX(int x);
        void setExplodeTranslateY(int y);
        void setExplodeTranslateZ(int z);

    public slots:
        void updatePickFilterAndSymbol(bool flag);

    private slots:
        void slot_changeLanguage(int language);
        void slot_plusSlot();
        void slot_minusSlot();
        void slot_reset();

        /** 全选按钮：全勾/全不勾（仅可见自建条目，隐藏勾选保留） */
        void slot_selectAllClicked();

        /** 新建爆炸图（V1.3 beginCreate：登记唯一 ID+默认名并选中激活，激活即复位样式） */
        void slot_newView();

        /** 删除按钮：移除勾选条目（常驻条目不删） */
        void slot_deleteViews();

        /** 行复选变化：回写条目勾选态并刷新行选中底色 */
        void slot_checkToggled();

        /** 操作列"激活/已激活"点击：切换激活爆炸图并应用到视口 */
        void slot_activateView();

        /** 爆炸模式/缩放方式切换：条件行显隐 + 错误行刷新 */
        void slot_modeChanged(int index);
        void slot_scaleChanged(int index);

        /** 搜索过滤（仅筛列表，不改选择/激活）；行点击=选中编辑 */
        void slot_searchChanged(const QString& text);
        void slot_cellClicked(int row, int column);

    private:
        /*
        * 爆炸图条目（表格行数据，行 UserRole 存 id 关联）：
        * kind 0=不显示爆炸（常驻，激活即清除爆炸显示）
        * kind 1=自动爆炸图（常驻，激活即按当前模型自动生成）
        * kind 2=用户自建爆炸图（新建插入在两个常驻条目之间；可勾选，勾选即行选中高亮；
        *   记录爆炸：state 存视口各部件的爆炸位移，激活时恢复记录、被切换走时落账当前状态；
        *   draft 为控制字段编辑缓冲（V1.3 统一编辑契约：保存写回、取消恢复））
        * 两个常驻条目复选框置灰不可勾选，仅自建条目有勾选/选中状态
        */
        struct ExplodeDraft
        {
            std::set<int> pickedParts;    //拾取条已拾取部件集合（空=所有部件，V1.3 拾取条数据源）
            int mode = 0;               //爆炸模式（0 平移/1 模型中心/2 所选部件中心）
            int scale = 0;              //缩放方式（0 等比例/1 分方向）
            QString factor = QStringLiteral("1");
            QString x = QStringLiteral("1");
            QString y = QStringLiteral("1");
            QString z = QStringLiteral("1");
            QString distance = QStringLiteral("1");
            int direction = 0;          //平移方向（0..5 同方向下拉）
            bool reversed = false;      //反向（V1.3 反向切换钮，入 dirty 签名）
            QString start = QStringLiteral("0,0,0");
            QString end = QStringLiteral("1,0,0");
            QString vector = QStringLiteral("1,0,0");
            bool autofit = false;
        };
        struct ExplodeEntry
        {
            int        id = 0;
            int        kind = 2;
            QString    name;
            bool       checked = false;		//行复选（自建条目；供全选/删除）
            QHash<int, QVector3D> state;	//记录的爆炸状态（部件ID->爆炸位移，仅自建条目）
            ExplodeDraft draft;               //控制字段编辑缓冲（自建条目；V1.3）
        };

        /** 构建面板控件（语言文本由 slot_changeLanguage 设置） */
        void setupUI();

        /** 以 _entries 为准重灌表格（结构变化/语言切换共用，保留勾选与激活态；
        *  按搜索词隐藏不匹配行；当前编辑行名称蓝字） */
        void rebuildTable();

        /** 按行数据渲染名称与操作按钮（已激活蓝字蓝框/未激活灰字灰框） */
        void renderRow(int row);

        /** 勾选行选中底色：自建条目勾选后名称区浅蓝高亮，未勾选白底 */
        void refreshRowCheckVisual(int row);

        /** id 对应条目指针（不存在返回 nullptr） */
        ExplodeEntry* entryById(int id);

        /** 行号对应条目 id（无关联返回 -1） */
        int entryIdAt(int row) const;

        /** 切换激活条目（恒有且仅 1 个已激活）并应用：不显示爆炸→清除；自动爆炸图→自动生成；自建→应用其记录的爆炸状态（切换前先把当前状态落账到原激活自建条目） */
        void activateEntry(int id, bool force = false);

        /** 把视口当前爆炸状态记录到正在激活的自建条目（渲染端不可用时保留原记录） */
        void captureActiveEntryState();

        /** 应用自建条目记录的爆炸状态到视口（按自适应视图设置收尾） */
        void applyEntryState(ExplodeEntry* e);

        /******** 统一编辑契约（V1.3；仅自建条目可编辑） ********/
        /** 当前编辑条目（自建；无返回 nullptr） */
        ExplodeEntry* editingEntry();

        /** 编辑缓冲读写：UI→缓冲 / 缓冲→UI（装载期间屏蔽变更通知） */
        ExplodeDraft collectUIDraft() const;
        void loadDraftToUI(const ExplodeDraft& d);
        /** 编辑缓冲规范化签名（dirty 检测） */
        static QString draftSignature(const ExplodeDraft& d, const QString& name);

        /** 名称校验（空/与其他自建条目重名） */
        QString explodeNameError() const;

        /** 编辑态刷新：控制区整体启停（常驻条目激活或删空时禁用）+当前行视觉 */
        void updateEditState();

        /** 选中/新建/取消/保存（原型 select/beginCreate/cancelEdit/commit） */
        void selectEntry(int id);
        void beginCreate();
        void cancelEdit();
        bool commitEdit(bool immediate);

        /** 管理表状态行（全选文案+已选计数） */
        void refreshManagerState();

        /** 模式切换后按当前模式/缩放方式显隐条件行（距离/平移方向框、缩放方式、变形系数单值/XYZ 三行；方向附属行随方向类型） */
        void applyModeRows();

        /** 底部调节控件（自适应视图/复位/−/+）随激活态可用性："不显示爆炸"激活时置灰 */
        void updateExplodeCtrlsEnabled();

        /** 控制字段校验（原型 validateExplosionSettings）：返回错误文案（空=有效） */
        QString settingsError() const;
        /** 内联错误行刷新（空文案收起） */
        void refreshErrorLine();

        /** 拾取条当前已拾取部件集合（空=未拾取；仅读拾取栈数据，不做全部件兜底） */
        std::set<int> pickedPartIds() const;

        /** 勾选/拾取条目对应的部件集合（拾取条已拾取→该集合；空→全部部件） */
        std::set<int> getSelectedPartIds();

        /** 由方向/距离输入构造平移向量（isPlus 决定正负向；反向钮翻转） */
        QVector3D getTranslateVector(bool isPlus);

        /** 由缩放方式/变形系数构造模型中心爆炸向量（isPlus 决定正负向） */
        QVector3D getModelCenterVector(bool isPlus);

        /** 按当前模型自动生成爆炸图并按"自适应视图"适应视口 */
        void autogenerate();

        /** 通用：以给定方向向量驱动爆炸（plus=true 正向/反向） */
        void applyExplode(bool plus);

        /** 语言标签登记（创建点登记中英文，语言切换全量重刷——显式表防序号漂移） */
        void regLabel(PanelLabel* label, const char* cn, const char* en);

        /** −/＋ 图标随使能态换色（EnabledChange） */
        void refreshStepIcons();
        bool eventFilter(QObject* watched, QEvent* event) override;

        /** 面板被隐藏（切到其他功能面板）：落账当前状态并清除爆炸显示，其他功能看到装配态模型 */
        void hideEvent(QHideEvent* event) override;

        /** 切回爆炸图面板：按激活条目恢复视口（自建→应用记录；自动→重新生成） */
        void showEvent(QShowEvent* event) override;

    private:
        QVBoxLayout*        _mainLayout{};

        //--------- 爆炸图管理 ---------
        FoldSection*        _manageFold{};
        QLineEdit*          _searchEdit{};     //搜索爆炸图名称（V1.3）
        MxFunctions::mPanelTable* _table{};       //爆炸图列表（复选/名称/操作；共享管理表格件）
        QPushButton*        _btnSelectAll{};    //全选
        QLabel*             _selectedCount{};   //已选计数（V1.3）
        MxFunctions::mPrimaryButton* _btnNew{}; //新建爆炸图（主操作蓝）
        QPushButton*        _btnDelete{};       //删除
        QList<ExplodeEntry> _entries;           //行数据（行 UserRole 存 id 关联）
        int                 _activeId{ 1 };     //当前激活条目 id（1=不显示爆炸）
        int                 _nextViewId{ 3 };   //新建爆炸图 id 计数（1/2 为常驻条目）
        int                 _newViewCount{ 0 }; //新建计数（命名"自定义爆炸图_N"）
        int                 _editingId{ -1 };   //当前编辑条目 id（自建；-1 无）
        bool                _loadingDraft{ false }; //缓冲装载期间屏蔽变更通知
        int                 _nameSortState{ 0 };//名称排序（0 未排序↕/1 升序↑/2 降序↓，V1.3）

        //--------- 爆炸图控制 ---------
        FoldSection*        _ctrlFold{};
        QLineEdit*          _nameEdit{};       //爆炸图名称（V1.3）
        mPostObjectEditController* _editor{};  //统一编辑契约（V1.3）
        mPostPickWidgetController* _pickController{};  //选择部件通用拾取条（mPost 栈，云图/向量图/阵列同款）
        mPostPartScopeField* _scopeCtl{};      //部件范围隐形控制器（仅提供"所有部件"本地化空态文案，弹窗不可达）
        QComboBox*          _comboMode{};      //爆炸模式（平移/从模型中心爆炸/从所选部件中心爆炸）
        QLineEdit*          _editDistance{};   //平移-距离（模式=平移时显示）
        QComboBox*          _comboScale{};     //缩放方式（等比例/分方向）
        QLineEdit*          _editFactor{};     //变形系数（等比例）
        QLineEdit*          _editX{};          //X方向系数（分方向，三行独立行）
        QLineEdit*          _editY{};
        QLineEdit*          _editZ{};
        MxFunctions::mGroupBox* _dirBox{};     //平移方向骑线细线框（V1.3 终态）
        MxFunctions::mDirectionRow* _dirRow{}; //定义方式下拉+反向切换钮（L13）
        QWidget*            _startRowWidget{}; //起点子块（[拾取起点|选择节点][起点坐标|输入]，方向=起点-终点时显示）
        QPushButton*        _startPickBtn{};   //起点-选择节点（拾取待接入置灰）
        QLineEdit*          _startEdit{};      //起点坐标
        QWidget*            _endRowWidget{};   //终点子块（同起点）
        QPushButton*        _endPickBtn{};     //终点-选择节点（拾取待接入置灰）
        QLineEdit*          _endEdit{};        //终点坐标
        QWidget*            _selDirRowWidget{}; //选择方向子行（方向=选择对象时显示）
        QPushButton*        _selDirBtn{};       //选择对象按钮（待接入置灰）
        QWidget*            _vecRowWidget{};    //方向向量子行（方向=向量时显示）
        QLineEdit*          _vecEdit{};         //方向向量
        QLabel*             _errorLine{};       //内联错误行（V1.3 explosionSettingsError，空时收起）
        QCheckBox*          _checkAutofit{};    //自适应视图
        QPushButton*        _btnReset{};        //复位（40）
        QPushButton*        _btnMinus{};        //−（26×26 图标钮）
        MxFunctions::mPrimaryButton* _btnPlus{};//＋（26×26 图标主蓝钮）

        QPointer<PostData::mPostDataBase> _postDB;	//当前后处理数据库

        //--------- 语言标签登记表 ---------
        struct LabelText
        {
            PanelLabel* label{};
            const char* cn{};
            const char* en{};
        };
        QList<LabelText> _labelTexts;
        bool _langEn{ false };
    };
}
