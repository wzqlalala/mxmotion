#pragma once

//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

//Function
#include <mEditDiagnosis.h>

//QT
#include <QVBoxLayout>
#include <QWidget>
#include <QListWidgetItem>
#include <QTableWidget>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QLabel>
#include <QToolButton>
#include <QMdiSubWindow>
#include <QComboBox>
#include <qvector3d.h>


#include "mPanelBase.h"
#include "mUnitSystemManager.h"
#include <set>
//BasicData
#include "mPhysicalFieldEnum.h"
#include "mCoordinateManager.h"
#include "mCoordinateBase.h"
//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mxr/view/CCSMPreRender.h"
#include "mxr/view/HighlightController.h"
#include "mxr/view/mFontRender.h"

#include "InteractionCore/Event/Action.h"
#include "InteractionCore/InteractionSystem.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
namespace MechMainWindow
{
    //特征选择分类
    enum FeaturePick
    {
        None,

        //---------------特征--------------//
        //选择对象
        FeatureObj,

        //拉伸-选择对象
        StretchObj,

        //拉伸-选择拉伸方向
        StretchDirection,

        //旋转轴
        RotationAxis,

        //旋转-轴上一点
        RotationAxisPnt,

        //扫掠-选择扫掠路径
        SweepLine,

        //拔模中性面
        DraftNeutralFace,

        //孔放置面
        HoleFace,

        //---------------变换--------------//
        //平移方向
        OffsetDirection,

        //平移起始点
        OffsetStartPnt,

        //平移终止点
        OffsetEndPnt,

        //旋转轴
        RotateAxis,

        //旋转-轴上一点
        RotateAxisPnt,

        //镜像平面法向
        MirrorVec,

        //镜像平面法向上一点
        MirrorVecPnt,

        //缩放基准点
        ZoomBasePnt,

        //线性阵列方向
        LineArrayDirection,

        //环形阵列旋转轴
        RingArrayAxis,

        //环形阵列旋转轴上一点
        RingArrayAxisPnt,

        //---------------定位操作--------------//
        //初始坐标系
        LocationSourceCoordinate,

        //目标坐标系
        LocationTargetCoordinate,

        //初始几何点A
        LocationSourcePntA,

        //初始几何点B
        LocationSourcePntB,

        //初始几何点C
        LocationSourcePntC,

        //目标几何点A
        LocationTargetPntA,

        //目标几何点B
        LocationTargetPntB,

        //目标几何点C
        LocationTargetPntC,

        //---------------布尔操作--------------//
        //布尔求差对象B
        SubPickObjB,

        //---------------流体域----------------//
        Fluid,

        //---------------分割操作--------------//
        //分割面自定义向量
        SpFaceCustomVec,

        //分割面自定义向量
        SpFaceDtmCustomVec,

        //分割体扫掠自定义向量
        SpSolidSweepCustomVec,

        //分割体基准面自定义向量
        SpSolidDatumCustomVec,

        //几何点A
        SpFacePntA,

        //几何点B
        SpFacePntB,

        //几何线
        SpFaceGeoLine,

        //边界线
        SpSolidBndLine,

        //扫掠线
        SpSolidSwLine,

        //几何面
        SpSolidGeoFace,

        //几何线
        SpLine,

        //外部剪裁边
        ExternalCutEdge,

        //特征投影
        ProjectionObj,

        //特征投影
        ProjectionVec,

        //---------------去除操作--------------//
        //自动去除
        AutoRemoval,

        //几何线A
        SplineCombineA,

        //几何线B
        SplineCombineB,

        //增加硬点
        AddHardPoint,

        //重复面
        DuplicateFaces,

        //修复分割边
        RepairSplitEdge,

        //修复分割边
        ExtraEdges,

        //修复缺失面
        MissingFaces,

        //重复线
        DuplicateLines,

        //拟合曲线
        FitCurves,

        //直接建模
        DirectModel,

        //---------------梁截面--------------//
        //自定义梁截面-平面法向自定义向量(拾取几何线自动填充方向)
        SectionNormalCustomVec,

        //自定义梁截面-截面基准点自定义坐标(拾取几何点自动填充坐标)
        SectionBasePointCustomPnt,

        //自定义梁截面-Y轴方向自定义向量(拾取几何线自动填充方向)
        SectionYAxisCustomVec
    };

    enum GeoOperationType
    {
        //拉伸
        GeoStretch,

        //旋转
        GeoRotation,

        //扫掠
        GeoSweep,

        //倒角
        GeoChamfering,

        //抽壳
        GeoShellRemoval,

        //拔模
        GeoWithdrawingPattern,

        //放样
        GeoLoft,

        //创建孔
        GeoHole,

        //平移
        GeoOffset,

        //旋转
        GeoRotate,

        //镜像
        GeoMirrorimage,

        //缩放
        GeoZoom,

        //线性阵列
        GeoLinearArray,

        //环形阵列
        GeoRingArray,

        //定位
        GeoLocation,

        // 布尔求和Summation
        GeoSummation,

        // 布尔求差SUSUBTRAC
        GeoSusubtrac,

        // 布尔求交INTERSE
        GeoInterse,

        //流体域
        GeoFluid,

        //中面
        GeoMidFace,

        //中线
        GeoMidLine,

        //边界面
        GeoBoundaryFace,

        //梁截面

        // 分割线
        GeoSplitLine,

        // 分割面
        GeoSplitFace,

        // 分割体
        GeoSplitSolid,

        // 合并线
        GeoMergeLine,

        // 合并面
        GeoMergeFace,

        // 合并体
        GeoMergeSolid,

        // 删除面
        GeoDeleteFace,

        // 删除体
        GeoDeleteSolid,

        // 删除线（仅独立线）
        GeoDeleteLine,

        // 短边
        GeoCheckShortEdge,

        // 锐边
        GeoCheckSharpEdge,

        // 开放边
        GeoCheckOpenEdge,

        // 小型面
        GeoCheckSmallFace,

        // 微小实体
        GeoCheckTinySolid,

        //取消裁剪面
        GeoCancelTrimFace,

        //修补缝隙
        GeoRepairCracks,

        //填充曲面
        GeoFillSurface,

        //共享拓扑
        GeoSharedTopology,

        //取消共享拓扑
        GeoUnSharedTopology,

        //强制共享拓扑
        GeoForceSharedTopology,

        //线延长
        GeoLineLengthen,

        //面偏置
        GeoFaceOffset,

        //特征投影
        GeoFeatureProjection,

        //面延伸
        GeoFaceExtend,

        //线连接
        GeoLineCombine,

        //几何清理


        //硬点删除
        GeoHardPoint,

        //几何干涉检查
        GeoInterferenceCheck,

        //去除重复线
        GeoRemoveDuplicateLines,

        //拟合曲线
        GeoFitCurves,

        //修复分割边
        GeoRepairSplitEdge,

        //去除额外边
        GeoRepairExtraEdge,

        //去除重复面
        GeoRemoveRepeatFace,

        //几何实体化-拼接
        GeoStitch,

        //几何实体化-间距
        GeoGap,

        //几何实体化-缺失面
        GeoMissingFace,

        //几何实体化-逆向建模
        GeoReverseModel,

        //自动去除
        GeoAutoRemoval,

        //直接建模-移动面
        GeoDirectModelingOffsetFace,

        //直接建模-旋转面
        GeoDirectModelingRotateFace,

        //直接建模-替换面
        GeoDirectModelingReplaceFace,

        //直接建模-偏置面
        //GeoDirectModelingOffsetFace,

        //直接建模-调整圆角
        GeoDirectModelingResizeFillet
    };


    class GeoPickWidget : public QWidget {
        Q_OBJECT

    public:
        explicit GeoPickWidget(QWidget* parent = nullptr);
        ~GeoPickWidget();

        /*
        * 清空高亮
        */
        void clearHighlight();

        /*
        * 设置拾取模式对应当前下拉选项
        */
        void setPickModeByCombo(QComboBox* combo);


        QLabel* pickLabel;
        QComboBox* pickObjTypeCombo;
        QToolButton* pickBtn;
        FeaturePick _featPick;

        //当前拾取类型
        QString strPickType;

    private:
        QHBoxLayout* pickLayout;

    };


    class MMAINWINDOW_EXPORT mGeoPanelBase : public mPanelBase
    {
        Q_OBJECT

    public:

        /*
        * 构造函数
        */
        mGeoPanelBase(QWidget* parent = nullptr);

        /*
        * 析构函数
        */
        ~mGeoPanelBase();

        /*
        * 隐藏特征界面
        */
        void hideAllFold();

        /*
        * 创建水平分割线
        */
        QFrame* createHorizontalLine(QWidget* parent);

        /*
        * 获取列表中所有id
        */
        std::vector<int> GetTableIdsUseVector();
        std::set<int> GetTableIdsUseSet();


        /*
        * 高亮列表中几何
        */
        void setGeoHighlight(QTableWidget* table, QString type);

        /*
        * 设置坐标
        */
        void setCoord(QVector3D vec, QLineEdit* x, QLineEdit* y, QLineEdit* z);

        /*
        * 翻转向量坐标-三段式
        */
        void reverseVecAction(QLineEdit* Vx, QLineEdit* Vy, QLineEdit* Vz);

        /*
        * 翻转向量坐标-整体式
        */
        void reverseVecAction(QLineEdit* LineEditVec);

        /*
        * QString字符串转QVector3D
        */
        QVector3D extractVectorFromLineEdit(QString text, const QVector3D& defaultValue = QVector3D(0, 0, 0), bool* ok = nullptr);

        /*
        * 设置坐标-整体式
        */
        void setVectorToLineEdit(QLineEdit* lineEdit, const QVector3D& vector, int precision = 2);

        /*
        * 获取下拉框所选方向
        */
        void getDir(double* md[3], int index, QLineEdit* lineEdit = nullptr);

        /*
        * 取消高亮几何
        */
        void disHighlight(int& id, QString type);

        /*
      * 取消高亮符号
      */
        void disHighlightSymbol(SymbolSelectInfoData data);

        /*
        * 高亮
        */
        void Highlight(int id, QString type);

        /*
      * 高亮符号
      */
        void HighlightSymbol(int id, QString SymbolStr);

        /*
        * 折叠块插入并显示
        */
        void displayAfterInsertFolds(QList<int> index);

        /*
        * 连接单位制响应信号
        */
        void connectUnitSystemResponse();

        /*
        * 断开单位制响应信号
        */
        void disconnectUnitSystemResponse();

        /*
        * 进入面板时连接全局响应信号（拾取、单位制等），由 mInfoWindow::createGeoXXX 在
        * 进入面板后调用。构造函数不再连接：面板经 ensurePanel 缓存复用，构造只执行一次，
        * 若在构造时连接，uneditGeometry 断开后再次进入面板将无法恢复信号
        */
        virtual void connectGeoSignals();

        /*
        * 退出面板时断开全部全局响应信号，由 mInfoWindow::uneditGeometry 统一调用
        */
        virtual void disconnectGeoSignals();

        /*
        * 将当前值转换为默认值(mm)
        */
        double getDefaultValue(const Unit& unit, double val);

        /*
        * 清空指定表格行
        */
        void clearTableItems(QTableWidget* table);

        /*
        * 按当前单位更新对应控件和值
        */
        void updateLengthUnitWidget();
        void updateAngleUnitWidget();


        bool isConnectedUnit = false;
        QList< QPair<QLabel*, mCalcLineEdit*>> m_LengthLabels;
        QList< QPair<QLabel*, mCalcLineEdit*>> m_AngleLabels;

        QVBoxLayout* _mainLayout;
        QTableWidget* pickObjTable;

        //表格右键菜单
        QAction* delAction = nullptr;

        QList<FoldSection*> m_FoldSections;

        //int lastPickId;

        //当前特征模块
        GeoOperationType currentFeature;
        QString currentType;

        GeoPickWidget* geoPickWidget;
        GeoPickWidget* geoPickWidget_2;

    private slots:
        /*
        * 拾取表格右键菜单
        */
        void slot_contextMenuTableData(const QPoint& pos);

        //切换单位制
        void slot_unitSystemResponse(UnitType type, QVector<int> idx);

        void slot_angleUnitChanged(int idx);



    };
}