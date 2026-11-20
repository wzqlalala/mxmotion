#pragma once
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"
#include <QObject>
#include "mAPIServer.h"

using namespace APIServer;
// 全局Pythonm命令
namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mPythonCommand : public QObject
    {
        Q_OBJECT
    public:
        /*
        * 获取单例
        */
        static mPythonCommand* getInstance();

        /*
        * 禁止拷贝
        */
        mPythonCommand(const mPythonCommand&) = delete;
        mPythonCommand& operator=(const mPythonCommand&) = delete;

        /*
        * 注册API
        */
        void registerAPI();
    private:

        /*
        * 删除注册
        */
        void unregisterAPI();

        QVariant onOptimTestAPI(const APIServer::APIParams& params, QString& error);
        /*
        * API Server 回调：启动求解
        */
        QVariant onStartSolverAPI(const APIServer::APIParams& params, QString& error);

        /*
        * API Server 回调：创建面面接触
        */
        QVariant onCreateFaceToFaceContactAPI(const APIServer::APIParams& params, QString& error);

        /*
        * API Server 回调：创建求解控制
        */
        QVariant onCreateSolverControlAPI(const APIServer::APIParams& params, QString& error);

        /*
        * API Server 回调：创建固定约束边界
        */
        QVariant onCreateFixedBoundConditAPI(const APIServer::APIParams& params, QString& error);

        /*
        * API Server 回调：创建温度边界
        */
        QVariant onCreateTemperatureBoundConditAPI(const APIServer::APIParams& params, QString& error);

        /*
        * API Server 回调：创建集中力载荷
        */
        QVariant onCreateLoadForceAPI(const APIServer::APIParams& params, QString& error);

        /*
        * API Server 回调：创建对流换热载荷
        */
        QVariant onCreateLoadConvectionAPI(const APIServer::APIParams& params, QString& error);

        /*
        * API Server 回调：创建线弹性各向同性材料
        */
        QVariant onCreateIsotropicMaterialAPI(const APIServer::APIParams& params, QString& error);

        /*
        * API Server 回调：创建热学各项同性热传导材料
        */
        QVariant onCreateHeatMaterialAPI(const APIServer::APIParams& params, QString& error);

        /*
        * API Server 回调：创建三维实体截面属性
        */
        QVariant onCreateSolidPropertyAPI(const APIServer::APIParams& params, QString& error);

        /*
        * API Server 回调：创建静力学分析
        */
        QVariant onCreateLinearStaticAPI(const APIServer::APIParams& params, QString& error);

        /*
        * API Server 回调：创建稳态传热分析
        */
        QVariant onCreateSteadyHeatAPI(const APIServer::APIParams& params, QString& error);

        /*
        * API Server 回调：创建模态分析
        */
        QVariant onCreateModalAnalysisAPI(const APIServer::APIParams& params, QString& error);

        /*
        * API Server 回调：创建连接->绑定
        */
        QVariant onCreateBindAPI(const APIServer::APIParams& params, QString& error);

        /*
        * API Server 回调：创建初始温度预定义场
        */
        QVariant onCreateInitTempPredefinedFieldAPI(const APIServer::APIParams& params, QString& error);

        /*
        * API Server 回调：设置前视图方向
        */
        QVariant onSetViewDirectionAPI(const APIServer::APIParams& params, QString& error);
    
    private:
        explicit mPythonCommand(QObject* parent = nullptr);//构造函数必须私有
        ~mPythonCommand();
        static mPythonCommand* m_instance;
    };
}