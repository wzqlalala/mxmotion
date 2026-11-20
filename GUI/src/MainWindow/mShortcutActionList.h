#pragma once
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"
#include <QObject>
#include <QDebug>
#include <stdexcept>
#include <mIOFileManager.h>
#include <mAppConfiguration.h>
#include <QtWidgets>
#include "mGlobalSignals.h"
#include <mViewportModelManager.h>
#include <mViewportModel.h>
#include <mGeoManager.h>
//#include <mGeoInfoCalculate.h> //【OCC摘除·S10/P0-1 终局3】死 include（零使用；经 mGeoDataModelSet 拉 OCC 链）


//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"

//Geometry
//#include <mGeoCommandFactory.h> //【OCC摘除·S10/P0-1 终局2】死 include（零使用）
#include <mMechGlobalSignals.h>
#include <PickCommonFunction.h>

namespace MechMainWindow
{

    class ShortcutActionList : public QObject
    {
        Q_OBJECT

    public:
        static ShortcutActionList* instance();

        void connectPartMoveRotate();
        void disconnectPartMoveRotate();

        ShortcutActionList(const ShortcutActionList&) = delete;
        ShortcutActionList& operator=(const ShortcutActionList&) = delete;

        // ─────────────────────────────────────────
        //  Geometry 相关功能
        // ─────────────────────────────────────────
        void geometryDeleteFeature();

        void geometryDelete();

        // 几何撤消/重做（GeoHistory）：撤消/重做当前视口几何历史一步，
        // 随后全量刷新几何树与视口。分别绑定 Ctrl+Z / Ctrl+Y。
        void geometryUndo();
        void geometryRedo();

        // 撤消/重做恢复后全量刷新：几何树按当前 PartManager 重建，视口重渲染。
        void refreshAfterHistoryRestore();


        // ─────────────────────────────────────────
        //  通用功能
        // ─────────────────────────────────────────
        void save();

        //清空所有拾取
        void clearPick();

        //选中等体积的实体或者等面积的面
        void pickAllEqualGeo();

    //设置辅助选项
    private slots:

        void slot_subWindowActivated(int viewportID);

        ////移动向量起点和向量
        void slot_GeometryMoveAction(const float pos[3], const float moveVec[3]);

        ////旋转：旋转轴起点、旋转轴方向和旋转角度
        void slot_GeometryRotateAction(const float pos[3], const float rotateDir[3], const float angle);

    private:
        ShortcutActionList() = default;

        void connectSignals();

        //获取当前拾取类型是拾取列表
        std::set<int> getCurrentPickType(PickDataType& type);

        //创建几何编辑列表
        //QList<MGeoCommand::EditShape> createEditShape(const std::set<int>& ids, PickDataType pickType); //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
    };

}