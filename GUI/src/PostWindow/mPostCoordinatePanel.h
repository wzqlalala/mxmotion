#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QVector3D>
#include <QMap>
#include <QList>
#include <QString>
#include <QPair>

#include "mpostwindow_global.h"
#include "mPanelBase.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）
namespace MechMainWindow { }
using namespace MechMainWindow;

class QLineEdit;
class QLabel;
class QCheckBox;
class QPushButton;
class QVBoxLayout;
class QGridLayout;

namespace MxFunctions
{
    class mCheckComboBox;
    class mChoiceGroup;
    class mGroupBox;
    class mGeneralPickButton;
    class mAxisCoordRow;
}

namespace PostWindow
{
    /*
    * 后处理坐标系参数面板（界面操作期）：
    * 对照前处理 mCoordinatePanel 的"基本信息/位置与方向"两折叠框结构，
    * 直角/柱/球三类型经"类型"下拉切换（Ribbon 三入口按类型预设）。
    *
    * 组合控件（前处理拾取件在 MainWindow 模块下、PostWindow 隔离禁令不可复用，
    * 改由 MxFunctions 公共控件组合）：
    * - 点行 = mGroupBox(标题联动) + mGeneralPickButton(拾取钮) + mAxisCoordRow(X/Y/Z三格) + 应用钮；
    * - 方向行 = mGroupBox(标题联动) + mCheckComboBox(全局X/Y/Z轴/自定义矢量) + mAxisCoordRow(自定义矢量) + 反向。
    *
    * 数据口径（用户裁定 2026-11-18）：后处理与前处理完全分离，禁止复用前处理
    * MechCoordinateData 数据格式（显式/隐式两态，隐式会破坏显示编译）——本期树联动/
    * 编辑交互跑在面板内会话级记录上（界面操作载体），真实后处理数据格式与
    * 落库/渲染/拾取功能后续立项替换。
    */
    class MPOSTWINDOW_EXPORT mPostCoordinatePanel : public mPanelBase
    {
        Q_OBJECT

    public:
        explicit mPostCoordinatePanel(QWidget* parent = nullptr);

        /*
        * 新建坐标系（coordType 0直角/1柱/2球）：会话记录登记+按类型预设面板，
        * 发 createUpdateCoordinateSig(true,id) 驱动树"坐标系"分支重建
        */
        void newCoordinatePanel(int coordType);

        /*
        * 编辑坐标系（树"坐标系"子节点单击/右键"编辑"入口）：按会话记录回填；
        * 全局坐标系(id=-1)只读（基本信息禁用、位置与方向隐藏）
        */
        void editCoordinatePanel(int coordinateId);

        //退出编辑态（会话记录保留，面板编辑对象置空）
        void uneditCoordinatePanel();

        //树侧内联重命名同步：面板正编辑该坐标系时名称行跟随（避免下次提交回写旧名）
        void onCoordinateRenamedExternally(int coordinateId, const QString& newName);

        /*
        * 树"坐标系"分支联动出口（标签面板 tagTreeRecords 同款模式，树不直接改数据）：
        * coordinateTreeRecords=记录快照（分支重建源；面板未创建返回空）；
        * delete/copy/clear/rename=右键菜单与内联重命名的记录操作（树侧菜单经 hub 路由，
        * 单一数据源在面板；记录变更由面板发 createUpdateCoordinateSig 驱动分支重建）
        */
        QVector<QPair<int, QString>> coordinateTreeRecords() const;
        bool deleteCoordinateRecords(const QList<int>& ids);
        int copyCoordinateRecord(int id);                          // 返回新记录 id（<0=源不存在）
        void clearCoordinateRecords();
        void renameCoordinateRecord(int coordinateId, const QString& newName);   // 查重后的最终写入

        //ui-shot 状态驱动（测量面板 slot_debugInjectDistanceRecord 先例）：
        //类型循环切换 直角→柱→球→直角，供三类型界面截图走查
        Q_INVOKABLE void slot_debugNextType();
        //定义方式切换 三点↔基准轴（走真实切换链，供方向行布局截图走查）
        Q_INVOKABLE void slot_debugToggleDefine();

    private:
        void setupUI();
        void connectUI();

        //会话级坐标系记录（界面操作数据载体；真实数据格式后续立项）
        struct CoordRecord
        {
            int id = -1;
            QString name;
            int typeIdx = 0;        // 0直角/1柱/2球
        };
        CoordRecord* recordById(int id);
        const CoordRecord* recordById(int id) const;
        bool coordinateNameExists(const QString& name, int excludeId) const;

        //名称+类型回填（编辑进入/复用重置共用；屏蔽信号防连锁）
        void fillNameAndType(const QString& name, int typeIdx);
        //输入复位（定义方式/主轴/点位/方向回默认，新建与编辑进入共用）
        void resetInputDefaults();
        //用户输入痕迹标记（新建草稿复用判定依据：未编辑草稿重复点创建入口复用）
        void markDraftTouched() { _draftTouched = true; }

        //点拾取组合行（标题随类型/主轴联动）
        struct PointRow
        {
            MxFunctions::mGroupBox* group = nullptr;
            MxFunctions::mGeneralPickButton* pickBtn = nullptr;
            MxFunctions::mAxisCoordRow* coord = nullptr;
            QPushButton* applyBtn = nullptr;

            void setCoord(const QVector3D& v);
            QVector3D coordValue() const;
        };

        //方向组合行（基准轴方式；标题随类型/主轴联动）
        struct DirRow
        {
            MxFunctions::mGroupBox* group = nullptr;
            MxFunctions::mCheckComboBox* combo = nullptr;     // 全局X/Y/Z轴/自定义矢量
            MxFunctions::mAxisCoordRow* coord = nullptr;      // 自定义矢量（仅"自定义矢量"档显示）
            QCheckBox* reverse = nullptr;

            QVector3D vectorValue() const;                    // 按下拉档位取矢量（全局轴取单位矢量）
        };

        //类型独立草稿（原型 per-type draft：三类型各自保存/恢复面板输入）
        struct TypeDraft
        {
            bool valid = false;
            QString method;                                   // 三点/基准轴
            QVector3D origin, xPoint, xyPoint;
            QVector3D xLine, yLine;
            int dirMode1 = 0, dirMode2 = 0;
            bool rev1 = false, rev2 = false;
            int mainIndex = 0, dirIndex = 0;
        };

        void saveTypeDraft(int typeIdx);
        void restoreTypeDraft(int typeIdx);

        //行显隐编排（主轴/第二轴/参考平面/点行/方向行/球面说明/快速创建行）
        void updatePosFoldLayout();
        //拾取组/方向组标题联动（随 类型×定义方式×主轴）
        void updatePointGroupTitles();
        //参考平面/第二轴选项随主轴联动（选项=包含主轴的两平面，第二轴由平面推导）
        void applyAxisReferences();
        //快速创建行仅"三点"方式显示；随勾选态切应用钮显隐
        void updateQuickRowVisibility();

        //拾取钮互斥点亮（纯 UI 武装态；视口拾取链路为后续功能接入）
        void activateFirstVisiblePickButton();
        void resetPickArmedState(bool keepFirstVisible = false);

        //语言内文案（中/英）取值辅助
        static QString axLetter(int axisIdx);
        QString pickButtonLabel(bool armed) const;

    private slots:
        void slot_onTypeChanged(int index);
        void slot_onDefineChanged(const QString& text);
        void slot_onMainAxisChanged(const QString& text);
        void slot_onPlaneChoiceChanged(const QString& text);
        void slot_onSecondAxisChanged(const QString& text);
        void slot_onNameTextChanged(const QString& text);
        //名称提交（回车）：查重后会话记录改名并广播树重建
        void slot_commitName();
        void slot_changeLanguage(int language);

    protected:
        void showEvent(QShowEvent* event) override;
        void hideEvent(QHideEvent* event) override;

    private:
        QVBoxLayout* _mainLayout = nullptr;

        /* ===== 会话级记录（界面操作数据载体，真实数据格式后续立项） ===== */
        QList<CoordRecord> _records;                        // 用户坐标系记录（id 升序）
        int _nextCoordId = 1;                               // 记录 id 发号器
        int _curCoordId = -1;                               // 当前编辑记录 id（编辑全局时为 -1，未编辑也是 -1，以 isEditing 区分）

        //新建草稿标记（复用守卫用）：_isNewDraft=当前对象由 newCoordinatePanel 新建；
        //_draftTouched=创建归一化之后用户改过输入。未编辑草稿重复点击创建入口时
        //复用同一记录，不再连建多个"坐标系N"
        bool _isNewDraft = false;
        bool _draftTouched = false;

        /* ===== 基本信息 ===== */
        FoldSection* _foldBasic = nullptr;
        PanelLabel* _nameLabel = nullptr;
        QLineEdit* _nameEdit = nullptr;
        QLabel* _nameErrorLabel = nullptr;                     // 名称校验错误行（空名红框，按需插拔）
        PanelLabel* _typeLabel = nullptr;
        MxFunctions::mCheckComboBox* _comboType = nullptr;     // 类型(直角/柱/球)
        PanelLabel* _defineLabel = nullptr;
        MxFunctions::mChoiceGroup* _defineChoice = nullptr;    // 定义方式(三点/基准轴)
        QCheckBox* _quickCreate = nullptr;                     // 快速创建(勾选:拾取即应用)
        QWidget* _quickRow = nullptr;                          // 快速创建跨列行(缩进与定义方式对齐)

        /* ===== 位置与方向 ===== */
        FoldSection* _foldPos = nullptr;
        PanelLabel* _mainAxisLabel = nullptr;
        MxFunctions::mChoiceGroup* _mainAxisChoice = nullptr;  // 坐标系主轴(X轴/Y轴/Z轴，仅直角显示)
        PanelLabel* _secondAxisLabel = nullptr;
        MxFunctions::mChoiceGroup* _secondAxisChoice = nullptr;// 第二个轴(选项随主轴联动，直角+基准轴显示)
        PanelLabel* _planeLabel = nullptr;
        MxFunctions::mChoiceGroup* _planeChoice = nullptr;     // 参考平面(XOY/XOZ，直角+三点显示)
        PointRow _originRow;                                   // 原点
        PointRow _pt1Row;                                      // 轴正向点(标题随类型联动)
        PointRow _pt2Row;                                      // 平面点(标题随类型联动)
        DirRow _dir1Row;                                       // 主轴方向(标题随类型联动)
        DirRow _dir2Row;                                       // 第二轴方向(标题随类型联动)
        QLabel* _sphereNoteLabel = nullptr;                    // 球坐标 θ/φ 说明行(仅球显示)

        /* ===== 面板内 UI 状态 ===== */
        int _curTypeIdx = 0;                                   // 当前类型 0直角/1柱/2球
        int _mainIndex = 0;                                    // 主轴索引
        int _dirIndex = 0;                                     // 参考平面/第二轴共用方向索引
        QMap<int, TypeDraft> _typeDrafts;                      // per-type 草稿
        QVector<QVector3D> _methodThreePoints;                 // 三点方式缓存(定义方式切换用)
        QVector3D _methodBasisXLine, _methodBasisYLine;        // 基准轴方式缓存
        int _methodBasisDir1 = 0, _methodBasisDir2 = 0;
        bool _methodBasisRev1 = false, _methodBasisRev2 = false;
        bool _methodBasisValid = false;
    };
}
