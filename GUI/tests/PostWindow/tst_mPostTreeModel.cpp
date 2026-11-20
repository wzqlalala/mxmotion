#pragma execution_character_set("utf-8")

// mPostTreeModel 回归测试（首批）
//
// 覆盖面：建树骨架、index/parent 导航、键-索引往返、增删节点与模型信号、
// 分组生命周期、data/setData 各角色、状态冒泡、图标资源映射、空模型/非法参数边界。
// 全部用例只需 QCoreApplication（无头运行），不触碰视口/数据管理器单例；
// setData(EditRole) 合法重命名路径依赖 getCVM 数据管理器，暂不覆盖（见各用例注释）。

#include <QtTest/QtTest>
#include <crtdbg.h>
#include <windows.h>
#include <dbghelp.h>

namespace
{
    // CRT 断言钩子：打印断言消息与调用栈后放行（无人值守环境不弹窗），
    // 用于定位 PostWindowd.dll 内的 vector 越界断言来源
    int crtReportHook(int, char* message, int* returnValue)
    {
        void* frames[40] = {};
        const USHORT n = CaptureStackBackTrace(0, 40, frames, nullptr);
        HANDLE proc = GetCurrentProcess();
        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        SymInitialize(proc, nullptr, TRUE);
        fprintf(stderr, "\n=== CRT ASSERT: %s\n", message);
        for (USHORT i = 0; i < n; ++i)
        {
            DWORD64 addr = (DWORD64)frames[i];
            char buf[sizeof(SYMBOL_INFO) + 256] = {};
            SYMBOL_INFO* sym = reinterpret_cast<SYMBOL_INFO*>(buf);
            sym->SizeOfStruct = sizeof(SYMBOL_INFO);
            sym->MaxNameLen = 255;
            DWORD64 disp = 0;
            if (SymFromAddr(proc, addr, &disp, sym))
                fprintf(stderr, "  [%02d] %s +0x%llx\n", i, sym->Name, (unsigned long long)disp);
            else
                fprintf(stderr, "  [%02d] 0x%llx (无符号)\n", i, (unsigned long long)addr);
        }
        fflush(stderr);
        *returnValue = 0;
        return FALSE;   // 继续走默认报告链（已重定向到 stderr）
    }
}

#include "mPostTreeModel.h"

using namespace PostWindow;
using namespace MxFunctions;

namespace
{
    // 便捷取"当前模型"的骨架节点（依赖 addPostModelTree 后 _currentModelID 已设置）
    mPostTreeItem* skeletonItem(mPostTreeModel& model, BrowserTreeType type, int phyID = 0)
    {
        return model.getChildrenItem(type, phyID);
    }
}

class TestPostTreeModel : public QObject
{
    Q_OBJECT

private slots:
    void emptyModelInitialState();
    void addPostModelTreeBuildsSkeleton();
    void addPostModelTreeNameNumbering();
    void navigationAndParents();
    void keyIndexRoundtrip();
    void parentItemLookups();
    void appendChildrenEmitsSignals();
    void removeRowsDeindexes();
    void removeTreeHelpers();
    void removeIndexsBatch();
    void globalCoordinateNotDeletable();
    void groupLifecycle();
    void groupDuplicateAndRenameRejections();
    void groupRenameSuccess();
    void stableNameItemIdSemantics();
    void renameValidatorHook();
    void setDataDisplayRole();
    void setDataStatusRoleBubbles();
    void removeRowsStatusAccounting();
    void setDataIconAndTypeRole();
    void setDataEditRoleRejections();
    void iconResourceStrings();
    void itemFlags();
};

//////////////////////////////////////////////////////////////////////////
// 空模型初始状态
//////////////////////////////////////////////////////////////////////////

void TestPostTreeModel::emptyModelInitialState()
{
    mPostTreeModel model;

    QCOMPARE(model.rowCount(QModelIndex()), 0);
    QCOMPARE(model.columnCount(QModelIndex()), 1);
    QCOMPARE(model.getCurrentModelID(), -1);

    QVERIFY(!model.index(0, 0, QModelIndex()).isValid());
    QVERIFY(!model.index(-1, 0, QModelIndex()).isValid());
    QVERIFY(!model.parent(QModelIndex()).isValid());
    QVERIFY(!model.data(QModelIndex(), Qt::DisplayRole).isValid());
    QCOMPARE(model.flags(QModelIndex()), Qt::NoItemFlags);

    QVERIFY(!model.setData(QModelIndex(), QStringLiteral("x"), Qt::EditRole));
    QVERIFY(!model.removeRows(0, 1, QModelIndex()));
    QVERIFY(!model.removeTreeItem(nullptr));
    QVERIFY(!model.removeTreeChildrenItem(nullptr));
    QVERIFY(!model.indexFromItem(nullptr).isValid());

    // 查找类接口对空模型应安全返回空
    const mItemKey emptyKey{ 1, 0, 0, PostModelParentItem };
    QCOMPARE(model.treeItemFromKey(emptyKey), nullptr);
    QVERIFY(!model.indexFromKey(emptyKey).isValid());
}

//////////////////////////////////////////////////////////////////////////
// addPostModelTree：骨架结构
//////////////////////////////////////////////////////////////////////////

void TestPostTreeModel::addPostModelTreeBuildsSkeleton()
{
    mPostTreeModel model;
    model.addPostModelTree(101);

    QCOMPARE(model.rowCount(QModelIndex()), 1);
    QCOMPARE(model.getCurrentModelID(), 101);

    const QModelIndex rootIdx = model.index(0, 0, QModelIndex());
    QVERIFY(rootIdx.isValid());
    QCOMPARE(rootIdx.data(Qt::DisplayRole).toString(), QStringLiteral("后处理模型-1"));
    QCOMPARE(rootIdx.data(MechPreData::Roles::TypeRole).toInt(), int(PostTreeRootItem));
    QCOMPARE(rootIdx.data(MechPreData::Roles::ModelRole).toInt(), 101);
    QCOMPARE(model.parent(rootIdx), QModelIndex());

    // 骨架十节点：部件 / 分组 / 材料 / 属性 / 集合 / 坐标系 / 工况 / 标签 / 测量 / 云图
    //（默认固定生成；"结果"根已随 2D 类去节点化移除——曲线/Word报告/路径不再建树节点；
    // "工况"根下分"分析工况/派生工况"两类子根（分析工况子节点由导入链填充、
    // 派生工况由 store 定义集重建）；"坐标系"根默认挂全局坐标系子节点，标签为空容器）
    QCOMPARE(model.rowCount(rootIdx), 10);
    QStringList names;
    QList<int> types;
    for (int r = 0; r < 10; ++r)
    {
        names << model.index(r, 0, rootIdx).data(Qt::DisplayRole).toString();
        types << model.index(r, 0, rootIdx).data(MechPreData::Roles::TypeRole).toInt();
    }
    QCOMPARE(names, (QStringList{ QStringLiteral("部件"), QStringLiteral("分组"), QStringLiteral("材料"),
        QStringLiteral("属性"), QStringLiteral("集合"), QStringLiteral("坐标系"), QStringLiteral("工况"),
        QStringLiteral("标签"), QStringLiteral("测量"), QStringLiteral("云图") }));
    QCOMPARE(types, (QList<int>{ int(PostModelParentItem), int(PostModelGroupRootItem), int(PostMaterialRootItem),
        int(PostSectionRootItem), int(PostSetRootItem), int(PostCoordinateRootItem), int(PostCaseRootItem),
        int(PostMarkerRootItem), int(PostMeasureRootItem), int(ResultParentItem) }));

    // "工况"根下两类子根：分析工况 / 派生工况
    const QModelIndex caseRootIdx = model.index(6, 0, rootIdx);
    QCOMPARE(model.rowCount(caseRootIdx), 2);
    QCOMPARE(model.index(0, 0, caseRootIdx).data(MechPreData::Roles::TypeRole).toInt(), int(PostCaseAnalysisRootItem));
    QCOMPARE(model.index(1, 0, caseRootIdx).data(MechPreData::Roles::TypeRole).toInt(), int(PostCaseDerivedRootItem));

    // "坐标系"根默认挂"全局坐标系"子节点（唯一、不可删除/不可重命名）
    const QModelIndex coordRootIdx = model.index(5, 0, rootIdx);
    QCOMPARE(model.rowCount(coordRootIdx), 1);
    const QModelIndex globalCoordIdx = model.index(0, 0, coordRootIdx);
    QCOMPARE(globalCoordIdx.data(Qt::DisplayRole).toString(), QStringLiteral("全局坐标系"));
    QCOMPARE(globalCoordIdx.data(MechPreData::Roles::TypeRole).toInt(), int(PostCoordinateChildItem));
    QCOMPARE(globalCoordIdx.data(MechPreData::Roles::PhyidRole).toInt(), PostGlobalCoordinatePhyID);
    QCOMPARE(globalCoordIdx.data(MechPreData::Roles::ModelRole).toInt(), 101);
    QCOMPARE(model.parent(globalCoordIdx), coordRootIdx);
    QCOMPARE(model.getChildrenItem(PostCoordinateChildItem, PostGlobalCoordinatePhyID),
        model.itemFromIndex(globalCoordIdx));

    // "云图"根为结果类唯一容器（ResultParentItem + PostCloudRootPhyID）
    QCOMPARE(model.index(9, 0, rootIdx).data(MechPreData::Roles::PhyidRole).toInt(), PostCloudRootPhyID);

    // 骨架节点的父均指向模型根
    for (int r = 0; r < 10; ++r)
        QCOMPARE(model.parent(model.index(r, 0, rootIdx)), rootIdx);
}

void TestPostTreeModel::addPostModelTreeNameNumbering()
{
    mPostTreeModel model;
    model.addPostModelTree(1);
    model.addPostModelTree(2);
    model.addPostModelTree(3);

    QCOMPARE(model.rowCount(QModelIndex()), 3);
    QCOMPARE(model.index(0, 0, QModelIndex()).data(Qt::DisplayRole).toString(), QStringLiteral("后处理模型-1"));
    QCOMPARE(model.index(1, 0, QModelIndex()).data(Qt::DisplayRole).toString(), QStringLiteral("后处理模型-2"));
    QCOMPARE(model.index(2, 0, QModelIndex()).data(Qt::DisplayRole).toString(), QStringLiteral("后处理模型-3"));
    QCOMPARE(model.getCurrentModelID(), 3);
}

//////////////////////////////////////////////////////////////////////////
// 导航边界
//////////////////////////////////////////////////////////////////////////

void TestPostTreeModel::navigationAndParents()
{
    mPostTreeModel model;
    model.addPostModelTree(1);

    const QModelIndex rootIdx = model.index(0, 0, QModelIndex());

    // 非法列 / 越界行
    QVERIFY(!model.index(0, 1, QModelIndex()).isValid());
    QVERIFY(!model.index(1, 0, QModelIndex()).isValid());
    QVERIFY(!model.index(0, 0, rootIdx).parent().parent().isValid());

    // columnCount 对任意索引恒为 1
    QCOMPARE(model.columnCount(rootIdx), 1);
    QCOMPARE(model.columnCount(model.index(0, 0, rootIdx)), 1);

    // 隐形根的子行 = 顶层模型数；"部件"骨架节点的子行 = 0（坐标系根默认有全局坐标系子节点）
    QCOMPARE(model.rowCount(QModelIndex()), 1);
    QCOMPARE(model.rowCount(model.index(0, 0, rootIdx)), 0);
}

//////////////////////////////////////////////////////////////////////////
// 键-索引-节点 往返
//////////////////////////////////////////////////////////////////////////

void TestPostTreeModel::keyIndexRoundtrip()
{
    mPostTreeModel model;
    model.addPostModelTree(101);

    // 遍历两级所有节点做往返校验
    const QModelIndex rootIdx = model.index(0, 0, QModelIndex());
    for (int r = -1; r < model.rowCount(rootIdx); ++r)
    {
        const QModelIndex idx = (r < 0) ? rootIdx : model.index(r, 0, rootIdx);
        const mItemKey key = model.itemKeyFromIndex(idx);
        QCOMPARE(model.indexFromKey(key), idx);
        QCOMPARE(model.treeItemFromKey(key), model.itemFromIndex(idx));
    }

    // 不存在的键
    const mItemKey missing{ 7777, 0, 0, PostModelChildItem };
    QVERIFY(!model.indexFromKey(missing).isValid());
    QCOMPARE(model.treeItemFromKey(missing), nullptr);
}

//////////////////////////////////////////////////////////////////////////
// getParentItem / getChildrenItem 查找
//////////////////////////////////////////////////////////////////////////

void TestPostTreeModel::parentItemLookups()
{
    mPostTreeModel model;
    model.addPostModelTree(101);

    const QModelIndex rootIdx = model.index(0, 0, QModelIndex());
    mPostTreeItem* partItem = model.itemFromIndex(model.index(0, 0, rootIdx));
    mPostTreeItem* groupItem = model.itemFromIndex(model.index(1, 0, rootIdx));
    mPostTreeItem* materialItem = model.itemFromIndex(model.index(2, 0, rootIdx));
    mPostTreeItem* sectionItem = model.itemFromIndex(model.index(3, 0, rootIdx));
    mPostTreeItem* setItem = model.itemFromIndex(model.index(4, 0, rootIdx));
    mPostTreeItem* cloudItem = model.itemFromIndex(model.index(9, 0, rootIdx));

    // getParentItem 默认 phyID=0 命中"部件/分组/材料/属性/集合/坐标系/工况/标签/测量"骨架；
    // "云图"(PostCloudRootPhyID) 为结果类唯一 ResultParentItem 容器（"结果"根已移除，
    // 按 -1 查找应返回 nullptr）
    QCOMPARE(model.getParentItem(PostModelParentItem), partItem);
    QCOMPARE(model.getParentItem(PostModelGroupRootItem), groupItem);
    QCOMPARE(model.getParentItem(PostMaterialRootItem), materialItem);
    QCOMPARE(model.getParentItem(PostSectionRootItem), sectionItem);
    QCOMPARE(model.getParentItem(PostSetRootItem), setItem);
    QCOMPARE(model.getParentItem(PostCoordinateRootItem), model.itemFromIndex(model.index(5, 0, rootIdx)));
    QCOMPARE(model.getParentItem(PostCaseRootItem), model.itemFromIndex(model.index(6, 0, rootIdx)));
    QCOMPARE(model.getParentItem(PostMarkerRootItem), model.itemFromIndex(model.index(7, 0, rootIdx)));
    QCOMPARE(model.getParentItem(PostMeasureRootItem), model.itemFromIndex(model.index(8, 0, rootIdx)));
    QCOMPARE(model.getParentItem(PostCaseAnalysisRootItem),
        model.itemFromIndex(model.index(0, 0, model.index(6, 0, rootIdx))));
    QCOMPARE(model.getParentItem(PostCaseDerivedRootItem),
        model.itemFromIndex(model.index(1, 0, model.index(6, 0, rootIdx))));
    QCOMPARE(model.getParentItem(ResultParentItem, -1), nullptr);
    QCOMPARE(model.getParentItem(ResultParentItem, PostCloudRootPhyID), cloudItem);

    // getChildrenItem（按当前模型）
    QCOMPARE(model.getChildrenItem(PostTreeRootItem, 0), model.itemFromIndex(rootIdx));
    QCOMPARE(model.getChildrenItem(PostModelParentItem, 0), partItem);
    // 指定其它 modelDataID 时应查无此节点（当前模型是 101）
    QCOMPARE(model.getChildrenItem(PostTreeRootItem, 0, -1, 9999), nullptr);

    // 未建任何树的类型
    QCOMPARE(model.getChildrenItem(threeDResultParentItem, 0), nullptr);
}

//////////////////////////////////////////////////////////////////////////
// appendChildren 与模型信号
//////////////////////////////////////////////////////////////////////////

void TestPostTreeModel::appendChildrenEmitsSignals()
{
    mPostTreeModel model;
    model.addPostModelTree(101);
    mPostTreeItem* partItem = skeletonItem(model, PostModelParentItem);
    const QModelIndex partIdx = model.indexFromItem(partItem);

    QSignalSpy insertSpy(&model, &mPostTreeModel::rowsInserted);

    // 批量插入 2 个部件子节点
    QVector<mPostTreeItem*> batch;
    batch << new mPostTreeItem(101, PostModelChildItem, QStringLiteral("part-1"));
    batch << new mPostTreeItem(101, PostModelChildItem, QStringLiteral("part-2"));
    model.appendChildren(partItem, std::move(batch));

    QCOMPARE(insertSpy.count(), 1);
    QCOMPARE(insertSpy.at(0).at(1).toInt(), 0);   // first
    QCOMPARE(insertSpy.at(0).at(2).toInt(), 1);   // last
    QCOMPARE(model.rowCount(partIdx), 2);
    QCOMPARE(model.index(0, 0, partIdx).data(Qt::DisplayRole).toString(), QStringLiteral("part-1"));
    QCOMPARE(model.index(1, 0, partIdx).data(MechPreData::Roles::PhyidRole).toInt(), 0);

    // 单节点插入
    model.appendChildren(partItem, new mPostTreeItem(101, PostModelChildItem, QStringLiteral("part-3")));
    QCOMPARE(insertSpy.count(), 2);
    QCOMPARE(model.rowCount(partIdx), 3);

    // 插入的节点应能按键反查（索引随插入维护）
    const mItemKey key = model.itemKeyFromIndex(model.index(2, 0, partIdx));
    QVERIFY(model.indexFromKey(key).isValid());

    // 空批量 / 空指针：不发信号不改结构
    model.appendChildren(partItem, QVector<mPostTreeItem*>());
    model.appendChildren(partItem, nullptr);
    QCOMPARE(insertSpy.count(), 2);
    QCOMPARE(model.rowCount(partIdx), 3);
}

//////////////////////////////////////////////////////////////////////////
// removeRows 与索引去注册
//////////////////////////////////////////////////////////////////////////

void TestPostTreeModel::removeRowsDeindexes()
{
    mPostTreeModel model;
    model.addPostModelTree(101);
    mPostTreeItem* partItem = skeletonItem(model, PostModelParentItem);
    const QModelIndex partIdx = model.indexFromItem(partItem);

    QVector<mPostTreeItem*> batch;
    for (int i = 0; i < 3; ++i)
        batch << new mPostTreeItem(101, PostModelChildItem, QString("p%1").arg(i));
    model.appendChildren(partItem, std::move(batch));

    const mItemKey victimKey = model.itemKeyFromIndex(model.index(1, 0, partIdx));

    QSignalSpy removeSpy(&model, &mPostTreeModel::rowsRemoved);
    QVERIFY(model.removeRows(1, 1, partIdx));

    QCOMPARE(removeSpy.count(), 1);
    QCOMPARE(removeSpy.at(0).at(1).toInt(), 1);   // first
    QCOMPARE(model.rowCount(partIdx), 2);
    QCOMPARE(model.index(0, 0, partIdx).data(Qt::DisplayRole).toString(), QStringLiteral("p0"));
    QCOMPARE(model.index(1, 0, partIdx).data(Qt::DisplayRole).toString(), QStringLiteral("p2"));

    // 被删节点的键应失效
    QVERIFY(!model.indexFromKey(victimKey).isValid());
    QCOMPARE(model.treeItemFromKey(victimKey), nullptr);

    // 越界拒绝
    QVERIFY(!model.removeRows(-1, 1, partIdx));
    QVERIFY(!model.removeRows(0, 3, partIdx));
}

//////////////////////////////////////////////////////////////////////////
// removeTreeItem / removeTreeChildrenItem
//////////////////////////////////////////////////////////////////////////

void TestPostTreeModel::removeTreeHelpers()
{
    mPostTreeModel model;
    model.addPostModelTree(1);
    model.addPostModelTree(2);

    // 删除一个顶层模型树（连同骨架子树）
    mPostTreeItem* second = model.getChildrenItem(PostTreeRootItem, 0, -1, 2);
    QVERIFY(second);
    QVERIFY(model.removeTreeItem(second));
    QCOMPARE(model.rowCount(QModelIndex()), 1);
    QCOMPARE(model.getChildrenItem(PostTreeRootItem, 0, -1, 2), nullptr);

    // 行为记录：removeTreeItem 不回退 _currentModelID，删除后按"当前模型"查骨架返回 nullptr。
    // （若把该 nullptr 传给 appendChildren，其内部 fallback 到未初始化计数数组的隐形根，
    //   触发 vector 越界断言——产品缺陷，暂不在测试中触碰）
    QCOMPARE(model.getChildrenItem(PostModelParentItem, 0), nullptr);

    // 清空"部件"子节点：显式按 modelDataID=1 定位（当前模型 ID 已悬空）
    mPostTreeItem* partItem = model.getChildrenItem(PostModelParentItem, 0, -1, 1);
    QVERIFY(partItem);
    model.appendChildren(partItem, new mPostTreeItem(1, PostModelChildItem, QStringLiteral("x")));
    QVERIFY(model.removeTreeChildrenItem(partItem));
    QCOMPARE(model.rowCount(model.indexFromItem(partItem)), 0);
    QVERIFY(!model.removeTreeChildrenItem(partItem));   // 已空再删返回 false
}

//////////////////////////////////////////////////////////////////////////
// removeIndexs：跨父批量删除
//////////////////////////////////////////////////////////////////////////

void TestPostTreeModel::removeIndexsBatch()
{
    mPostTreeModel model;
    model.addPostModelTree(1);
    mPostTreeItem* partItem = skeletonItem(model, PostModelParentItem);
    const QModelIndex partIdx = model.indexFromItem(partItem);

    QVector<mPostTreeItem*> batch;
    batch << new mPostTreeItem(1, PostModelChildItem, QStringLiteral("a"));
    batch << new mPostTreeItem(1, PostModelChildItem, QStringLiteral("b"));
    batch << new mPostTreeItem(1, PostModelChildItem, QStringLiteral("c"));
    batch << new mPostTreeItem(1, PostModelChildItem, QStringLiteral("d"));
    model.appendChildren(partItem, std::move(batch));

    // 删除 a、c（乱序给，验证按父分组、行号从大到小删）
    QModelIndexList victims{ model.index(2, 0, partIdx), model.index(0, 0, partIdx) };
    model.removeIndexs(victims);

    QCOMPARE(model.rowCount(partIdx), 2);
    QCOMPARE(model.index(0, 0, partIdx).data(Qt::DisplayRole).toString(), QStringLiteral("b"));
    QCOMPARE(model.index(1, 0, partIdx).data(Qt::DisplayRole).toString(), QStringLiteral("d"));
}

//////////////////////////////////////////////////////////////////////////
// 全局坐标系节点：随骨架默认创建，不可删除 / 不可重命名，随模型根整树拆除
//////////////////////////////////////////////////////////////////////////

void TestPostTreeModel::globalCoordinateNotDeletable()
{
    mPostTreeModel model;
    model.addPostModelTree(1);

    mPostTreeItem* coordRoot = skeletonItem(model, PostCoordinateRootItem);
    mPostTreeItem* globalItem = model.getChildrenItem(PostCoordinateChildItem, PostGlobalCoordinatePhyID);
    QVERIFY(coordRoot);
    QVERIFY(globalItem);
    const QModelIndex coordRootIdx = model.indexFromItem(coordRoot);
    const QModelIndex globalIdx = model.indexFromItem(globalItem);

    // removeRows 单删 / 整批删均被拒，节点原样保留、不发移除信号
    QSignalSpy removeSpy(&model, &mPostTreeModel::rowsRemoved);
    QVERIFY(!model.removeRows(0, 1, coordRootIdx));
    QVERIFY(!model.removeRows(0, 2, coordRootIdx));   // 越界 + 含保护节点，仍拒
    QCOMPARE(removeSpy.count(), 0);
    QCOMPARE(model.rowCount(coordRootIdx), 1);
    QVERIFY(model.getChildrenItem(PostCoordinateChildItem, PostGlobalCoordinatePhyID) == globalItem);

    // removeTreeItem / removeTreeChildrenItem / removeIndexs 同样收口被拒
    QVERIFY(!model.removeTreeItem(globalItem));
    QVERIFY(!model.removeTreeChildrenItem(coordRoot));
    model.removeIndexs({ globalIdx });
    QCOMPARE(model.rowCount(coordRootIdx), 1);
    QCOMPARE(model.index(0, 0, coordRootIdx).data(Qt::DisplayRole).toString(), QStringLiteral("全局坐标系"));

    // 不可重命名（EditRole 判型拦截，文本不变）
    QVERIFY(!model.setData(globalIdx, QStringLiteral("新名字"), Qt::EditRole));
    QCOMPARE(globalIdx.data(Qt::DisplayRole).toString(), QStringLiteral("全局坐标系"));

    // 删模型根（整树拆除）不受守卫影响：全局坐标系随模型根生死
    mPostTreeItem* treeRoot = model.getChildrenItem(PostTreeRootItem, 0, -1, 1);
    QVERIFY(treeRoot);
    QVERIFY(model.removeTreeItem(treeRoot));
    QCOMPARE(model.rowCount(QModelIndex()), 0);
    QCOMPARE(model.getChildrenItem(PostCoordinateChildItem, PostGlobalCoordinatePhyID), nullptr);

    // 重建后全局坐标系子节点随新骨架恢复
    model.addPostModelTree(2);
    mPostTreeItem* reborn = model.getChildrenItem(PostCoordinateChildItem, PostGlobalCoordinatePhyID);
    QVERIFY(reborn);
    QCOMPARE(model.indexFromItem(reborn).data(Qt::DisplayRole).toString(), QStringLiteral("全局坐标系"));
}

//////////////////////////////////////////////////////////////////////////
// 分组生命周期
//////////////////////////////////////////////////////////////////////////

void TestPostTreeModel::groupLifecycle()
{
    mPostTreeModel model;
    model.addPostModelTree(1);

    mPostTreeItem* groupRoot = skeletonItem(model, PostModelGroupRootItem);
    const QModelIndex groupRootIdx = model.indexFromItem(groupRoot);
    QVERIFY(groupRoot);

    // 非分组根节点上建组被拒绝
    mPostTreeItem* partItem = skeletonItem(model, PostModelParentItem);
    QCOMPARE(model.addPostModelGroup(partItem, QStringLiteral("g"), {}), nullptr);

    // 建组：引用子节点文本与部件 ID
    mPostTreeItem* g1 = model.addPostModelGroup(groupRoot, QStringLiteral("g1"), { 1, 2 },
        { QStringLiteral("part-A"), QStringLiteral("part-B") });
    QVERIFY(g1);
    QCOMPARE(groupRoot->getChildrenSize(), 1);
    QCOMPARE(model.rowCount(groupRootIdx), 1);

    const QModelIndex g1Idx = model.indexFromItem(g1);
    QCOMPARE(g1Idx.data(MechPreData::Roles::TypeRole).toInt(), int(PostModelGroupParentItem));
    QCOMPARE(model.rowCount(g1Idx), 2);
    QCOMPARE(model.index(0, 0, g1Idx).data(Qt::DisplayRole).toString(), QStringLiteral("part-A"));
    QCOMPARE(model.index(0, 0, g1Idx).data(MechPreData::Roles::PhyidRole).toInt(), 1);
    QCOMPARE(model.index(1, 0, g1Idx).data(MechPreData::Roles::PhyidRole).toInt(), 2);
    QCOMPARE(model.index(1, 0, g1Idx).data(MechPreData::Roles::TypeRole).toInt(), int(PostModelGroupChildItem));

    // 部件 ID 列表
    QCOMPARE(model.getPostModelGroupPartIds(g1), (QVector<int>{ 1, 2 }));
    QCOMPARE(model.getPostModelGroupPartIds(groupRoot).size(), 0);   // 非分组节点返回空

    // 查找
    QCOMPARE(model.findPostModelGroup(groupRoot, QStringLiteral("g1")), g1);
    QCOMPARE(model.findPostModelGroup(groupRoot, QStringLiteral("missing")), nullptr);
    QCOMPARE(model.findPostModelGroup(partItem, QStringLiteral("g1")), nullptr);   // 非分组根

    // 空名/未提供部件名的组也可建（名称缺省为空字符串）
    mPostTreeItem* g2 = model.addPostModelGroup(groupRoot, QStringLiteral("g2"), { 7 });
    QVERIFY(g2);
    QCOMPARE(model.rowCount(model.indexFromItem(g2)), 1);
    QCOMPARE(model.index(0, 0, model.indexFromItem(g2)).data(Qt::DisplayRole).toString(), QString());

    // 快照：名称 -> 部件 ID 列表（用于非持久化重建）
    const QHash<QString, QVector<int>> groups = model.getPostModelGroups(groupRoot);
    QCOMPARE(groups.size(), 2);
    QCOMPARE(groups.value(QStringLiteral("g1")), (QVector<int>{ 1, 2 }));
    QCOMPARE(groups.value(QStringLiteral("g2")), (QVector<int>{ 7 }));
    QCOMPARE(model.getPostModelGroups(partItem).size(), 0);   // 非分组根返回空

    // 删组
    QVERIFY(model.removePostModelGroup(g1));
    QCOMPARE(model.rowCount(groupRootIdx), 1);
    QCOMPARE(model.findPostModelGroup(groupRoot, QStringLiteral("g1")), nullptr);
    QVERIFY(!model.removePostModelGroup(groupRoot));   // 非分组节点拒绝
    QVERIFY(!model.removePostModelGroup(nullptr));
}

//////////////////////////////////////////////////////////////////////////
// 分组的重名拒绝
// 注：renamePostModelGroup 成功路径已可达——setData 重命名改本地判型后
// PostModelGroupParentItem 走"兄弟查重"分支（2026-10-13），正向用例见 groupRenameSuccess
//////////////////////////////////////////////////////////////////////////

void TestPostTreeModel::groupDuplicateAndRenameRejections()
{
    mPostTreeModel model;
    model.addPostModelTree(1);
    mPostTreeItem* groupRoot = skeletonItem(model, PostModelGroupRootItem);

    mPostTreeItem* g1 = model.addPostModelGroup(groupRoot, QStringLiteral("g1"), { 1 });
    mPostTreeItem* g2 = model.addPostModelGroup(groupRoot, QStringLiteral("g2"), { 2 });
    QVERIFY(g1 && g2);
    QCOMPARE(model.rowCount(model.indexFromItem(groupRoot)), 2);

    // 同名建组拒绝
    QCOMPARE(model.addPostModelGroup(groupRoot, QStringLiteral("g1"), { 3 }), nullptr);
    QCOMPARE(model.rowCount(model.indexFromItem(groupRoot)), 2);

    // 重命名到已有名称拒绝（不产生任何结构变化）
    QVERIFY(!model.renamePostModelGroup(g2, QStringLiteral("g1")));
    QCOMPARE(model.indexFromItem(g1).data(Qt::DisplayRole).toString(), QStringLiteral("g1"));
    QCOMPARE(model.indexFromItem(g2).data(Qt::DisplayRole).toString(), QStringLiteral("g2"));
    QVERIFY(!model.renamePostModelGroup(nullptr, QStringLiteral("x")));

    // setData 直改分组名到兄弟重名同样拒绝（分组无管理器同步，仅兄弟查重）
    QVERIFY(!model.setData(model.indexFromItem(g2), QStringLiteral("g1"), Qt::EditRole));
    QCOMPARE(model.indexFromItem(g2).data(Qt::DisplayRole).toString(), QStringLiteral("g2"));
}

//////////////////////////////////////////////////////////////////////////
// 分组重命名成功路径（分组为树内自有数据，改名即文本更新，部件引用不变）
//////////////////////////////////////////////////////////////////////////

void TestPostTreeModel::groupRenameSuccess()
{
    mPostTreeModel model;
    model.addPostModelTree(1);
    mPostTreeItem* groupRoot = skeletonItem(model, PostModelGroupRootItem);

    mPostTreeItem* g1 = model.addPostModelGroup(groupRoot, QStringLiteral("g1"), { 1, 2 }, { QStringLiteral("p1"), QStringLiteral("p2") });
    QVERIFY(g1);

    // 改名成功：文本更新、可按新名查回、部件 ID 列表保持
    QVERIFY(model.renamePostModelGroup(g1, QStringLiteral("改名组")));
    QCOMPARE(model.indexFromItem(g1).data(Qt::DisplayRole).toString(), QStringLiteral("改名组"));
    QCOMPARE(model.findPostModelGroup(groupRoot, QStringLiteral("改名组")), g1);
    QCOMPARE(model.findPostModelGroup(groupRoot, QStringLiteral("g1")), nullptr);
    QCOMPARE(model.getPostModelGroupPartIds(g1), (QVector<int>{ 1, 2 }));

    // 旧名释放后可再次建组
    QVERIFY(model.addPostModelGroup(groupRoot, QStringLiteral("g1"), { 3 }) != nullptr);
}

//////////////////////////////////////////////////////////////////////////
// removeRows 状态计数账目：按"被删子节点自己的状态"减，
// 不能按父节点当前状态减（否则子节点状态分布不一致时计数漂移、冒泡失真）
//////////////////////////////////////////////////////////////////////////

void TestPostTreeModel::removeRowsStatusAccounting()
{
    mPostTreeModel model;
    model.addPostModelTree(1);

    mPostTreeItem* partItem = skeletonItem(model, PostModelParentItem);
    const QModelIndex partIdx = model.indexFromItem(partItem);

    QVector<mPostTreeItem*> batch;
    batch << new mPostTreeItem(1, PostModelChildItem, QStringLiteral("c1"));
    batch << new mPostTreeItem(1, PostModelChildItem, QStringLiteral("c2"));
    model.appendChildren(partItem, std::move(batch));

    QModelIndex c1 = model.index(0, 0, partIdx);
    QModelIndex c2 = model.index(1, 0, partIdx);

    // c1 失败 -> 部件节点随之失败（冒泡）
    QVERIFY(model.setData(c1, int(Tree_Failure), MechPreData::Roles::StatusRole));
    QCOMPARE(partIdx.data(MechPreData::Roles::StatusRole).toInt(), int(Tree_Failure));

    // 删除的是正常状态的 c2：仍存在的 c1 是失败态，部件节点必须保持失败——
    // 旧实现按父节点状态（Failure）减计数，会把 {F:1,Y:1} 错减成 {F:0,Y:1}，
    // 父节点被误翻回 Tree_Yes
    QVERIFY(model.removeTreeItem(model.itemFromIndex(c2)));
    QCOMPARE(partIdx.data(MechPreData::Roles::StatusRole).toInt(), int(Tree_Failure));

    // 删除失败态的 c1 后：部件节点恢复正常
    c1 = model.index(0, 0, partIdx);
    QVERIFY(model.removeTreeItem(model.itemFromIndex(c1)));
    QCOMPARE(partIdx.data(MechPreData::Roles::StatusRole).toInt(), int(Tree_Yes));
}

//////////////////////////////////////////////////////////////////////////
// 名称记忆稳定ID：集合/材料/属性名称节点无数字ID数据源，重建序号会漂移
//////////////////////////////////////////////////////////////////////////

void TestPostTreeModel::stableNameItemIdSemantics()
{
    mPostTreeModel model;

    //同名同ID、异名异ID、单调递增
    const int a1 = model.stableNameItemId(QStringLiteral("set_A"));
    const int a2 = model.stableNameItemId(QStringLiteral("set_A"));
    const int b = model.stableNameItemId(QStringLiteral("set_B"));
    QVERIFY(a1 == a2);
    QVERIFY(a1 != b);
    QVERIFY(b > a1);

    //模拟重建（删子树后再建同名节点）：ID 不漂移，按 key 的选中记忆保持有效
    model.addPostModelTree(1);
    mPostTreeItem* setRoot = skeletonItem(model, PostSetRootItem);
    mItemKey keyA{ 1, a1, 0, PostSetElementChildItem };
    {
        mPostTreeItem* sub = new mPostTreeItem(1, PostSetElementRootItem, QStringLiteral("单元集合"));
        model.appendChildren(setRoot, sub);
        mPostTreeItem* child = new mPostTreeItem(1, PostSetElementChildItem, QStringLiteral("set_A"), model.stableNameItemId(QStringLiteral("set_A")));
        model.appendChildren(sub, child);
        QCOMPARE(model.treeItemFromKey(keyA), child);
    }
    model.removeTreeChildrenItem(setRoot);
    QCOMPARE(model.treeItemFromKey(keyA), nullptr);
    {
        mPostTreeItem* sub = new mPostTreeItem(1, PostSetElementRootItem, QStringLiteral("单元集合"));
        model.appendChildren(setRoot, sub);
        //重建时顺序变化（set_B 先建）：set_A 的 ID 仍为 a1，key 复位命中
        mPostTreeItem* childB = new mPostTreeItem(1, PostSetElementChildItem, QStringLiteral("set_B"), model.stableNameItemId(QStringLiteral("set_B")));
        model.appendChildren(sub, childB);
        mPostTreeItem* childA = new mPostTreeItem(1, PostSetElementChildItem, QStringLiteral("set_A"), model.stableNameItemId(QStringLiteral("set_A")));
        model.appendChildren(sub, childA);
        QCOMPARE(model.treeItemFromKey(keyA), childA);
    }
}

//////////////////////////////////////////////////////////////////////////
// renameValidator 钩子：合法重命名路径的查重/同步由视图注入，模型本体无单例
//////////////////////////////////////////////////////////////////////////

void TestPostTreeModel::renameValidatorHook()
{
    mPostTreeModel model;
    model.addPostModelTree(1);
    mPostTreeItem* resultRoot = skeletonItem(model, ResultParentItem);
    mPostTreeItem* curve = new mPostTreeItem(1, CurveChildItem, QStringLiteral("曲线_1"), 1);
    model.appendChildren(resultRoot, curve);
    const QModelIndex curveIdx = model.indexFromItem(curve);

    //未注入钩子：合法重命名一律拒绝（模型独立使用时不允许改管理器数据）
    QVERIFY(!model.setData(curveIdx, QStringLiteral("新曲线"), Qt::EditRole));
    QCOMPARE(curveIdx.data(Qt::DisplayRole).toString(), QStringLiteral("曲线_1"));

    //注入钩子：放行时改名成功，钩子收到正确的项与新名
    QStringList seenNames;
    model.renameValidator = [&seenNames](mPostTreeItem* item, const QString& newName)
    {
        (void)item;
        seenNames << newName;
        return true;
    };
    QVERIFY(model.setData(curveIdx, QStringLiteral("新曲线"), Qt::EditRole));
    QCOMPARE(curveIdx.data(Qt::DisplayRole).toString(), QStringLiteral("新曲线"));
    QCOMPARE(seenNames, QStringList{ QStringLiteral("新曲线") });

    //钩子拒绝（如管理器重名）：setData 返回 false、文本不变
    model.renameValidator = [](mPostTreeItem*, const QString&) { return false; };
    QVERIFY(!model.setData(curveIdx, QStringLiteral("再改名"), Qt::EditRole));
    QCOMPARE(curveIdx.data(Qt::DisplayRole).toString(), QStringLiteral("新曲线"));

    //分组改名不依赖钩子（纯树内数据，兄弟查重在模型内）
    mPostTreeItem* groupRoot = skeletonItem(model, PostModelGroupRootItem);
    mPostTreeItem* g1 = model.addPostModelGroup(groupRoot, QStringLiteral("g1"), { 1 });
    QVERIFY(g1);
    model.renameValidator = [](mPostTreeItem*, const QString&) { return false; };   //即使钩子恒拒
    QVERIFY(model.renamePostModelGroup(g1, QStringLiteral("改名组")));
    QCOMPARE(model.indexFromItem(g1).data(Qt::DisplayRole).toString(), QStringLiteral("改名组"));
}

//////////////////////////////////////////////////////////////////////////
// setData：DisplayRole / Icon2Role / TypeRole
//////////////////////////////////////////////////////////////////////////

void TestPostTreeModel::setDataDisplayRole()
{
    mPostTreeModel model;
    model.addPostModelTree(1);
    const QModelIndex rootIdx = model.index(0, 0, QModelIndex());

    QSignalSpy spy(&model, &mPostTreeModel::dataChanged);
    QVERIFY(model.setData(rootIdx, QStringLiteral("改名"), Qt::DisplayRole));

    QCOMPARE(rootIdx.data(Qt::DisplayRole).toString(), QStringLiteral("改名"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).value<QModelIndex>(), rootIdx);
}

void TestPostTreeModel::setDataIconAndTypeRole()
{
    mPostTreeModel model;
    model.addPostModelTree(1);
    const QModelIndex partIdx = model.indexFromItem(skeletonItem(model, PostModelParentItem));

    QVERIFY(model.setData(partIdx, QStringLiteral(":/custom.png"), MechPreData::Roles::Icon2Role));
    QCOMPARE(partIdx.data(MechPreData::Roles::Icon2Role).toString(), QStringLiteral(":/custom.png"));

    QVERIFY(model.setData(partIdx, int(PostMaterialRootItem), MechPreData::Roles::TypeRole));
    QCOMPARE(partIdx.data(MechPreData::Roles::TypeRole).toInt(), int(PostMaterialRootItem));

    // 未知角色拒绝
    QVERIFY(!model.setData(partIdx, 1, Qt::UserRole - 1));
}

//////////////////////////////////////////////////////////////////////////
// setData：StatusRole 与状态冒泡
// 冒泡终点：后处理模型根（PostTreeRootItem）不再向上传递。
//////////////////////////////////////////////////////////////////////////

void TestPostTreeModel::setDataStatusRoleBubbles()
{
    mPostTreeModel model;
    model.addPostModelTree(1);

    const QModelIndex rootIdx = model.index(0, 0, QModelIndex());
    const QModelIndex partIdx = model.indexFromItem(skeletonItem(model, PostModelParentItem));

    QVector<mPostTreeItem*> batch;
    batch << new mPostTreeItem(1, PostModelChildItem, QStringLiteral("c1"));
    batch << new mPostTreeItem(1, PostModelChildItem, QStringLiteral("c2"));
    model.appendChildren(skeletonItem(model, PostModelParentItem), std::move(batch));

    const QModelIndex c1 = model.index(0, 0, partIdx);
    QCOMPARE(c1.data(MechPreData::Roles::StatusRole).toInt(), int(Tree_Yes));

    // 子节点失败 -> "部件"节点随之失败（冒泡），但顶层模型根保持不变（终点）
    QSignalSpy spy(&model, &mPostTreeModel::dataChanged);
    QVERIFY(model.setData(c1, int(Tree_Failure), MechPreData::Roles::StatusRole));

    QCOMPARE(c1.data(MechPreData::Roles::StatusRole).toInt(), int(Tree_Failure));
    QCOMPARE(partIdx.data(MechPreData::Roles::StatusRole).toInt(), int(Tree_Failure));
    QCOMPARE(rootIdx.data(MechPreData::Roles::StatusRole).toInt(), int(Tree_Yes));
    QVERIFY(spy.count() >= 1);

    // 同值设置：成功但不发 dataChanged
    const int before = spy.count();
    QVERIFY(model.setData(c1, int(Tree_Failure), MechPreData::Roles::StatusRole));
    QCOMPARE(spy.count(), before);

    // 已知缺陷（暂不锁，修复后补正向用例）：对顶层后处理模型根 setData(StatusRole)
    // 会访问隐形根 m_root 的 _childStatuCount——mPostTreeItem 默认构造（隐形根专用）
    // 未初始化该 vector（空），下标访问触发 MSVC "vector subscript out of range" 断言。
}

//////////////////////////////////////////////////////////////////////////
// setData：EditRole 拒绝路径
// 合法重命名依赖 getCVM -> mPostDataManager（视口单例链），本批不覆盖；
// 此处只锁纯模型即可判定的拒绝：空名、后处理骨架中不含的可重命名语义。
//////////////////////////////////////////////////////////////////////////

void TestPostTreeModel::setDataEditRoleRejections()
{
    mPostTreeModel model;
    model.addPostModelTree(1);
    const QModelIndex partIdx = model.indexFromItem(skeletonItem(model, PostModelParentItem));

    // 空名称拒绝
    QVERIFY(!model.setData(partIdx, QString(), Qt::EditRole));
    QCOMPARE(partIdx.data(Qt::EditRole).toString(), QStringLiteral("部件"));

    // PostModelParentItem 映射 Physical_Post，但 postRename 无此类型分支 -> 拒绝且文本不变
    QVERIFY(!model.setData(partIdx, QStringLiteral("newName"), Qt::EditRole));
    QCOMPARE(partIdx.data(Qt::EditRole).toString(), QStringLiteral("部件"));
}

//////////////////////////////////////////////////////////////////////////
// 图标资源映射（getBrowserTreePic 的后处理子集）
//////////////////////////////////////////////////////////////////////////

void TestPostTreeModel::iconResourceStrings()
{
    mPostTreeModel model;
    model.addPostModelTree(1);

    const QModelIndex rootIdx = model.index(0, 0, QModelIndex());
    QCOMPARE(rootIdx.data(MechPreData::Roles::Icon2Role).toString(),
        QStringLiteral(":/mMainWidgets/Icon/BrowserTree/model3x.png"));
    QCOMPARE(model.index(0, 0, rootIdx).data(MechPreData::Roles::Icon2Role).toString(),
        QStringLiteral(":/mMainWidgets/Icon/BrowserTree/jihe3x.png"));
    QCOMPARE(model.index(1, 0, rootIdx).data(MechPreData::Roles::Icon2Role).toString(),
        QStringLiteral(":/mMainWidgets/Icon/BrowserTree/jihe.png"));
    QCOMPARE(model.index(2, 0, rootIdx).data(MechPreData::Roles::Icon2Role).toString(),
        QStringLiteral(":/mMainWidgets/Icon/BrowserTree/cailiao.png"));
    QCOMPARE(model.index(3, 0, rootIdx).data(MechPreData::Roles::Icon2Role).toString(),
        QStringLiteral(":/mMainWidgets/Icon/BrowserTree/jiemianshuxing.png"));
    QCOMPARE(model.index(4, 0, rootIdx).data(MechPreData::Roles::Icon2Role).toString(),
        QStringLiteral(":/mMainWidgets/Icon/post/svg/jihe.svg"));   //图标矢量化（png→svg）后同步
    // 坐标系/标签/测量：图标矢量化（png→svg 家族）；工况为结构性节点保持 fenxi.png
    QCOMPARE(model.index(5, 0, rootIdx).data(MechPreData::Roles::Icon2Role).toString(),
        QStringLiteral(":/mMainWidgets/Icon/post/svg/zuobiaoxi.svg"));
    QCOMPARE(model.index(6, 0, rootIdx).data(MechPreData::Roles::Icon2Role).toString(),
        QStringLiteral(":/mMainWidgets/Icon/BrowserTree/fenxi.png"));
    QCOMPARE(model.index(7, 0, rootIdx).data(MechPreData::Roles::Icon2Role).toString(),
        QStringLiteral(":/mMainWidgets/Icon/post/svg/biaoqian.svg"));
    QCOMPARE(model.index(8, 0, rootIdx).data(MechPreData::Roles::Icon2Role).toString(),
        QStringLiteral(":/mMainWidgets/Icon/post/svg/celianggongju.svg"));
    // 下标 9 现为"云图"根（"结果"根已移除），构造时指定 picString（yuntu.svg）
    QCOMPARE(model.index(9, 0, rootIdx).data(MechPreData::Roles::Icon2Role).toString(),
        QStringLiteral(":/mMainWidgets/Icon/post/svg/yuntu.svg"));

    // 自建节点按类型取默认图标（含空格文件名）
    mPostTreeItem* partItem = skeletonItem(model, PostModelParentItem);
    model.appendChildren(partItem, new mPostTreeItem(1, PostModelChildItem, QStringLiteral("p")));
    model.appendChildren(partItem, new mPostTreeItem(1, CurveChildItem, QStringLiteral("c")));
    model.appendChildren(partItem, new mPostTreeItem(1, streamlineChildItem, QStringLiteral("s")));

    const QModelIndex partIdx = model.indexFromItem(partItem);
    QCOMPARE(model.index(0, 0, partIdx).data(MechPreData::Roles::Icon2Role).toString(),
        QStringLiteral(":/mMainWidgets/Icon/BrowserTree/moxing3x.png"));
    QCOMPARE(model.index(1, 0, partIdx).data(MechPreData::Roles::Icon2Role).toString(),
        QStringLiteral(":/mMainWidgets/Icon/post/svg/quxian.svg"));
    QCOMPARE(model.index(2, 0, partIdx).data(MechPreData::Roles::Icon2Role).toString(),
        QStringLiteral(":/mMainWidgets/Icon/post/svg/liuxiantu.svg"));

    // 指定 picString 时优先使用
    model.appendChildren(partItem, new mPostTreeItem(1, PostModelChildItem, QStringLiteral("q"), 0, Tree_Yes,
        QStringLiteral(":/custom.png")));
    QCOMPARE(model.index(3, 0, partIdx).data(MechPreData::Roles::Icon2Role).toString(),
        QStringLiteral(":/custom.png"));

    // 部件集合根/子节点：矢量家族部件集图标 bujianji.svg（图标矢量化后不再复用部件图）
    model.appendChildren(partItem, new mPostTreeItem(1, PostSetPartRootItem, QStringLiteral("psr")));
    model.appendChildren(partItem, new mPostTreeItem(1, PostSetPartChildItem, QStringLiteral("psc")));
    QCOMPARE(model.index(4, 0, partIdx).data(MechPreData::Roles::Icon2Role).toString(),
        QStringLiteral(":/mMainWidgets/Icon/post/svg/bujianji.svg"));
    QCOMPARE(model.index(5, 0, partIdx).data(MechPreData::Roles::Icon2Role).toString(),
        QStringLiteral(":/mMainWidgets/Icon/post/svg/bujianji.svg"));
}

//////////////////////////////////////////////////////////////////////////
// flags
//////////////////////////////////////////////////////////////////////////

void TestPostTreeModel::itemFlags()
{
    mPostTreeModel model;
    model.addPostModelTree(1);

    const QModelIndex rootIdx = model.index(0, 0, QModelIndex());
    QCOMPARE(model.flags(rootIdx), Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
    QCOMPARE(model.flags(model.index(0, 0, rootIdx)), Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
}

//////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
    // CRT 断言改为输出到 stderr（钩子打印调用栈），避免无人值守（ctest/CI）环境下
    // Debug Assertion 弹窗阻塞进程
    _CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, crtReportHook);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);

    QCoreApplication app(argc, argv);
    TestPostTreeModel tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "tst_mPostTreeModel.moc"
