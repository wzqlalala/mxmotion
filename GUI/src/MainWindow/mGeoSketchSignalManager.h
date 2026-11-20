#include <QObject>
#include <QMap>
#include <QVector>
#include <QMetaObject>
#include <QVector3D>
#include <memory>
#include <QToolButton>
//#include <mGeoCommandSolidOfSketchOCC.h> //【OCC摘除·S10/P0-1 终局2】死 include（零使用）
//#include <mGeoCommandFactory.h> //【OCC摘除·S10/P0-1 终局2】死 include（零使用）
#include "mxr/basic/PublicStruct.h"
#include "mGeoSketchTypes.h"

class QShortcut;

class QWidget;

namespace MechMainWindow
{
    enum SketchPlane
    {
        CUSTOM,
        XOY,
        XOZ,
        YOZ,
        THREEPOINT
    };

    class mGeoSketchSignalManager : public QObject
    {
        Q_OBJECT
    public:
        // 获取单例实例
        static mGeoSketchSignalManager* instance();

        // 析构函数
        ~mGeoSketchSignalManager() override;

        //进入草图模式
        void enterSketch(SketchPlane plane);

        //退出草图模式
        void exitSketch();

        //-------------------------三点确定草图平面---------------------------//
        // 在mInfoWindow中显示三点坐标输入面板
        void showThreePointPlanePanel();

        // 通过三点坐标进入草图模式（三点不能共线）
        void enterSketchByThreePoints(const QVector3D& p1, const QVector3D& p2, const QVector3D& p3);

        // 三点坐标转换为草图平面：point为平面上的点（三点质心），normal为单位法向；三点共线或重合返回false
        static bool threePointsToPlane(const QVector3D& p1, const QVector3D& p2, const QVector3D& p3,
            QVector3D& point, QVector3D& normal);

        //-------------------------草绘实体---------------------------//
        // 连接按钮点击事件到几何创建逻辑
        void connectGeometryButton(QToolButton* button, mxr::SketchGModeType modeType);


        //-------------------------2D草绘模式---------------------------//
        // 连接按钮点击事件到草绘几何创建
        void connectSketchGeoButton(QToolButton* button, mxr::Sketch2DGeoType modeType);

        //-------------------------草绘约束---------------------------//
        // 连接约束按钮（尺寸/距离）；约束拾取+值输入交互由渲染组工具提供后生效
        void connectSketchConstraintButton(QToolButton* button);

        // 提交尺寸约束并求解（PlaneGCS）。
        // 渲染组工具通过 signal_SketchDimensionConstraint 信号传入 input；
        // 渲染组交付前也可作为测试入口直接调用。
        void submitDimensionConstraint(const EventDispatcher::mGeoSketchDimensionInput& input);

        // 清理所有连接
        void clearAllConnections();

        //清理草图特征id
        void clearSketchIds() { sketchIds.clear(); }

        //获取当前是否草图模式
        bool currentStatus() { return isSketch; }

        // 清除按钮选中状态
        void clearButtonSelection();

    signals:
        /*
        * 退出草图模式完成后发出（此时 isSketch 已为 false）。
        * 菜单栏据此移除"草图"动态页签：所有退出路径都经 exitSketch，
        * 在此统一通知，避免各调用点自行处理页签
        */
        void sketchExited();

    private:
        // 私有构造函数，防止外部创建实例
        explicit mGeoSketchSignalManager(QObject* parent = nullptr);

        // 按钮连接存储
        QMap<QToolButton*, QMetaObject::Connection> m_buttonConnections;

        // 当前选中的几何创建按钮（用于高亮显示）
        QToolButton* m_selectedButton = nullptr;

        // 设置按钮选中状态
        void selectButton(QToolButton* button);




        //-------------------------草绘实体---------------------------//
        // 清理之前的连接状态
        void cleanupPreviousState(mxr::SketchGModeType modeType);

        // 连接几何创建信号
        void connectGeometryCreationSignal(mxr::SketchGModeType modeType);

        /**************************************************************** */
        // 惰性建草图部件：首个 2D 图形创建时才分配 ID 并注册（避免预建空部件
        // 造成"部件 ID 数量 != 视口可见部件数量"）。curPartID 有效则直接复用。
        bool ensureSketchPart();
        /**************************************************************** */

        // 从按钮到模式类型的映射
        QMap<QToolButton*, mxr::SketchGModeType> m_buttonModeMap;

        // 平面完成连接存储
        QMap<mxr::SketchGModeType, QMetaObject::Connection> m_planeFinishConnections;

        // 几何创建连接存储
        QMap<mxr::SketchGModeType, QMetaObject::Connection> m_geometryCreateConnections;


        //-------------------------2D草绘模式---------------------------//
         // 清理之前的连接状态
        void cleanupSketchPreviousState(mxr::Sketch2DGeoType modeType);

        // 连接草绘创建信号
        void connectSketchCreationSignal(mxr::Sketch2DGeoType modeType);

        // 从按钮到模式类型的映射
        QMap<QToolButton*, mxr::Sketch2DGeoType> m_buttonSketchMap;

        // 草图创建连接存储
        QMap<mxr::Sketch2DGeoType, QMetaObject::Connection> m_SketchCreateConnections;

        //草图特征id
        QVector<QPair<int, int>> sketchIds; //partId and modelId

        //草图创建
        //MGeoCommand::mGeoCommandSketchGeometryOCC* m_SketchOCC; //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）

        //-------------------------三点确定草图平面---------------------------//
        //三点坐标输入面板
        QWidget* m_threePointPanel = nullptr;

        //创建三点坐标输入面板
        QWidget* createThreePointPlanePanel();

        QMetaObject::Connection sketchPlaneFinishConnection;

        bool hasCompound;

        bool isSketch = false;

        //当前正在操作的部件
        int curPartID = 0;

        //当前平面法向
        float curPlaneNVec[3] = { 0.0,0.0,0.0 };
        float curPlanePoint[3] = { 0.0,0.0,0.0 };

        //临时代码：渲染组约束交互交付前的应用内测试入口（F9），交付后移除
        void setupTestConstraintHook();
        void teardownTestConstraintHook();
        QShortcut* m_testConstraintShortcut = nullptr;
    private slots:
        // 线
        void slot_create2DGeometryLine(const float NVec[3], const std::vector <std::pair<std::vector<float>, std::vector<float>>>& linePnte);

        // 点
        void slot_create2DGeometryPoint(const float NVec[3], const float pointPnt[3]);

        // 圆、三点圆
        void slot_create2DGeometryCircle(const float NVec[3], const float circlePnt[3], const float radius);

        // 矩形
        void slot_create2DGeometryRectangle(const float NVec[3], const float vertexA[3], const float vertexB[3], const float vertexC[3], const float vertexD[3]);

        // 样条曲线
        void slot_create2DGeometrySPLine(const float NVec[3], const std::vector<std::vector<float>>& vertexs);

        //三点圆弧：法向，圆心和圆弧上的三个点
        void slot_create2DGeometry3PointArc(const float NVec[3], const float circlePnt[3], const std::vector <std::vector<float>>& vertexs);

        //椭圆
        void slot_create2DGeometryEllipse(const float NVec[3], const float centerPnt[3], const float vertexA[3], const float vertexB[3]);

        //正多边形：法向，中心，角点
        void slot_create2DGeometryPolygon(const float NVec[3], const float centerPnt[3], const std::vector <std::vector<float>>& vertexs);

        //平行四边形
        void slot_create2DGeometryParallelogram(const float NVec[3], const float vertexA[3], const float vertexB[3], const float vertexC[3], const float vertexD[3]);

        //椭圆弧:法向，椭圆中心，a轴，b轴，椭圆弧数据
        void slot_create2DGeometryEllipseArc(const float NVec[3], const float centerPnt[3], const float vertexA[3], const float vertexB[3], const std::vector <std::vector<float>>& vertexs);

        //扫掠圆弧:法向，圆心，圆弧起点，圆弧终点，圆弧数据
        void slot_create2DGeometrySweepArc(const float NVec[3], const float centerPnt[3], const float vertexStart[3], const float vertexEnd[3], const std::vector <std::vector<float>>& vertexs);

        //尺寸约束求解（渲染组工具信号入口）
        void slot_addSketchDimensionConstraint(const EventDispatcher::mGeoSketchDimensionInput& input);
    };
}
