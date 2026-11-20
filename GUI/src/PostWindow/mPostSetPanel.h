#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include "mpostwindow_global.h"
#include <QDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QTableWidget>
#include <QPointer>

#include "mPanelBase.h"
#include "mPostDataBase.h"
#include "PickCommonFunction.h"

class QCheckBox;

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


using namespace PostData;

namespace PostWindow
{
    /*
    * 后处理集合面板：界面/交互对齐前处理集合面板（Mech 版 mSetPanel）——
    * 集合定义（名称/集合类型只读/拾取对象）+ 集合数据（成员表格 序号/ID/面号|边编号
    * + 全选/已选计数/删除）+ 底部（清空/确定）。
    * 数据走后处理链路：成员实时写入 mPostDataBase 集合哈希并刷新模型树（会话级，
    * 不写回 mxdb），树右键显隐沿用既有集合分类节点。
    * 类型（与模型页"集合"组按钮序一致）：0部件集/1节点集/2单元边集/3单元面集/4单元集
    */
    class MPOSTWINDOW_EXPORT mPostSetPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        /** 默认构造函数 */
        mPostSetPanel(QWidget* parent = Q_NULLPTR);

        /** 析构函数 */
        ~mPostSetPanel();

        /** 新建集合面板（setType 与模型页按钮序一致：0部件集/1节点集/2单元边集/3单元面集/4单元集；
            自动命名并立即在库内建集（空集）刷新模型树——对齐前处理"立即创建上树"语义） */
        void newSetPanel(int caseid, int setType = -1);

        //编辑已有集合（从树上集合子节点进入；typeName 为类型中文，name 为集合名）
        void editSet(const QString& name, const QString& typeName);

        //按名编辑：库内五张哈希自动判型（树单击集合子节点入口；未找到提示并保持面板现状）
        void editSetByName(const QString& name);

        //树重命名/删除集合→面板同步（hub 转发）：改名跟随新名；被删则同型重开新集
        void onSetRenamed(const QString& oldName, const QString& newName);
        void onSetsDeleted(const QStringList& names);

        //兼容旧挂载链（slot_updatePostSetTree→hub editSet(viewname)）：按视口重定位上下文
        void setCurViewName(const QString& viewName);

        //当前集合类型中文名（未开始编辑返回空）
        QString currentSetType() const { return _typeName; }

	private:
        //成员：主ID + 子号（单元表面集=0基局部面号、单元边集=0基局部边号，其余-1）
        struct MemberEntry { int id = 0; int sub = -1; };

        //——数据辅助——
        void updateContextByViewName(const QString& viewName);      // 根据 viewName 重新定位 _rWidget 与 _db
        QPointer<mPostModelData> currentModelData();                // 当前激活后处理视口对应帧的模型数据
        bool isNameTakenInDatabase(const QString& name, const QString& exceptName = QString()) const; // 库内重名检查（exceptName 排除自身）
        //当前成员写入库（name 集合，按类型选哈希）并触发模型树重建；返回是否写入
        bool persistMembersToDatabase(const QString& name, const QString& typeName);
        //拾取数据 → 成员（面集经渲染面ID换算局部面号、边集经节点对反查局部边号；失败计入 unresolved）
        std::vector<std::pair<int, int>> resolveFaceSetMembers(const std::vector<mxr::MeshSelectInfoData>& faceData);
        std::vector<std::pair<int, int>> resolveEdgeSetMembers(const std::vector<mxr::MeshSelectInfoData>& lineData);
        //成员 → 拾取格式数据（高亮回放用：面集局部面号反查渲染面ID，边集局部边号展开节点对）
        std::vector<mxr::MeshSelectInfoData> membersToPickData() const;
        //按当前类型从拾取容器取增量数据并合并进 _members（addOrRemove：true并入/false去除）
        void absorbPickData(bool addOrRemove);
        //成员表格/计数/按钮状态刷新；高亮回放（写容器+CLEAR/ADD，回放期间屏蔽容器信号）
        void refreshMemberTable();
        void replayHighlight();
        //拾取区复位（新建/编辑集合共用）：失活拾取、清待添加、自动添加复位、caption 回初始
        void resetPickArea();
        //拾取入口动态文案（"拾取节点/待添加 N 个/已拾取 N 个"，随模式与缓冲差集刷新）
        void refreshPickCaption();
        //拾取容器数据 -> 成员口径（面/边经换算；与 absorbPickData 同源）
        std::vector<MemberEntry> resolvePickedEntries(const std::vector<mxr::MeshSelectInfoData>& picked);
        //并入成员（按 主ID+子号 去重）；返回是否发生变化
        bool mergePendingIntoMembers(const std::vector<MemberEntry>& entries);
        //集合名 -> 类型中文（在库内五张哈希中查找）
        QString setTypeNameInDatabase(const QString& name) const;

    private slots:
        //名称提交（重命名：库内 key 迁移 + 刷树，重名拒绝回退）
        void slot_commitName();
        //成员表格选择变化：已选/共 计数与全选/删除按钮状态
        void slot_memberSelectionChanged();
        void slot_selectAllMembers();
        //删除选中成员（仅移出集合）
        void slot_removeMembers();
        //拾取行：拾取入口切换（armed 设过滤器）；清空拾取缓冲；自动添加切换；添加暂存成员
        void slot_pickToggled(bool checked);
        void slot_clearPick();
        void slot_autoAddToggled(bool checked);
        void slot_addMembers();
        //拾取容器数据到达（armed 时按模式自动并入或转入待添加暂存）
        void slot_pickDataArrived();

    private:
        QPointer<mPostDataBase> _db = nullptr;                      // 后处理数据库（newSetPanel/updateContext 时定位）
        QPointer<mxr::REBUILD_PostRend> _rWidget = nullptr;         // 后处理渲染视口

        QString _typeName;                  // 当前集合类型中文（部件集/节点集/单元边集/单元面集/单元集）
        QString _setName;                   // 当前集合名（空=未开始编辑）
        bool _replaying = false;            // 高亮回放中（屏蔽容器信号，防自触发回环）
        bool _pickArmed = false;            // 拾取激活
        bool _autoAdd = false;              // 自动添加（原型 collection-auto-add）：拾取即入表，否则暂存待添加

        std::vector<MemberEntry> _members;
        std::vector<MemberEntry> _pending;  // 待添加暂存（手动模式：拾取缓冲中尚未并入成员的对象）

        //——界面——
        QVBoxLayout* _mainLayout;
        FoldSection* _defineFold = nullptr;     // 集合定义
        FoldSection* _dataFold = nullptr;       // 集合数据

        QLabel* _labelName = nullptr;
        QLineEdit* _lineEditName = nullptr;     // 名称（editingFinished 提交重命名）
        QLabel* _hintLabel = nullptr;
        QLineEdit* _labelType = nullptr;        // 集合类型（置灰只读，原型不可修改）
        QLabel* _labelPick = nullptr;

        //拾取对象（V1.3 通用拾取入口形态，同曲线提取面板）：全宽入口钮（左侧蓝竖条+
        //动态文案"拾取节点/待添加 N 个/已拾取 N 个"）+ 钮框内右侧 查看/清空 两小图标钮；
        //下方"自动添加"复选 + "添加"按钮行（原型 collection-auto-add-row）
        QPushButton* _pickEntry = nullptr;      // 入口钮（checkable=armed）
        QPushButton* _pickInspectBtn = nullptr; // 查看（禁用占位，同曲线提取）
        QPushButton* _pickClearBtn = nullptr;   // 清空拾取缓冲（可用）
        QCheckBox* _autoAddCheck = nullptr;     // 自动添加
        QPushButton* _addBtn = nullptr;         // 添加（把待添加暂存并入成员）

        QTableWidget* _tableMembers = nullptr;  // 成员表格（序号/ID/面号|边编号；空态行提示随模式）
        QWidget* _widgetMemberFooter = nullptr;
        QPushButton* _btnSelectAllMembers = nullptr;  // 全选/取消
        QLabel* _labelMemberCount = nullptr;          // 已选 N / 共 M
        QPushButton* _btnRemoveMembers = nullptr;     // 删除
    };
}
