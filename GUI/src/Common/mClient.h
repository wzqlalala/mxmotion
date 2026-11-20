#pragma once
#pragma execution_character_set("utf-8")

#define IP "192.168.8.1"
#define PORT 9898

#include <QMainWindow>

#include "mFloatingLicenseTest.h"
#include "mCommon_global.h"
#include <mutex>
//#include ""

namespace Ui { class mClient; };
class QTcpSocket;
namespace std {
	class mutex;
}


namespace Common
{
	class  COMMON_EXPORT mClient : public QMainWindow
	{
		Q_OBJECT

	private:

		/** 构造函数 */
		mClient(QWidget *parent = Q_NULLPTR);

		/** 析构函数 */
		~mClient();

		/** 禁止拷贝构造函数 */
		mClient(const mClient& src) = delete;

		/** 禁止赋值操作符 */
		mClient& operator=(const mClient& src) = delete;

	public:
		/** start */
		void start();

		/** 开始连接 */
		void startConnectServer(QString ip, QString port);

		/** 获取信息 */
		void getIPandHostName();

		/** 获取时间 */
		QString nowTime();

		/** 设置是否使用浮动许可 */
		void setIsUseFloatLicnese(bool flag);

	public:

		/** 获取单例指针 */
		static mClient* getInstance();

		/** 获取IP */
		QString getLocalIP();

	Q_SIGNALS:
		void IPISMAX();

	signals:

		/** 设置消息输出 */
		void setStatusBarMessage_sig(QString message);

	private slots:

		/** 读取数据 */
		void readData();

		/** 断开连接时的槽函数 */
		void disconnectReact();

		/** 设置消息输出 */
		void setStatusBarMessage_slot(QString message);

		/** 连接时发送信息 */
		void onConnected();

		/** 断开时发送信息 */
		void onDisconnected();

		/** 心跳包 */
		void writeHeartBeat();

		/** 断开 */
		void disconnectFromServer();

		/** 获取当前用户名 */
		QString getCurrentUsername();

	public:
		/** 材料模块 */
		void changeToMaterial();

		/** 属性模块 */
		void changeToProperty();

		/** 单元模块 */
		void changeToElement();

		/** 分析模块 */
		void changeToAnalyse();

		/** 载荷模块 */
		void changeToLoad();

		/** 边界条件模块 */
		void changeToBoundary();

		/** 初始条件模块 */
		void changeToCondition();

		/** 连接模块 */
		void changeToConnect();


		/** 接触模块 */
		void changeToContact();

		/** 刚性墙模块 */
		void changeToRigid();

		/** 气囊模块 */
		void changeToAirbag();

		/** 传感器模块 */
		void changeToSensor();

		/** 输出模块 */
		void changeToOutput();

		/** 作业模块 */
		void changeToJob();

		/** 集合模块 */
		void changeToSet();

		/** 曲线模块 */
		void changeToXYPlot();

		/** 坐标系模块 */
		void changeToCoordinate();

		/** 面模块 */
		void changeToFace();

		/** 重启动模块 */
		void changeToRestart();


		/** 提交求解模块 */
		void DynaSubmit();

		/** 几何模块 */
		void changeToGeometry();

		/** 网格模块 */
		void changeToMesh();


		/** 工具模块 */
		void changeToTool();

		/** 发生提交求解计算的信号 */
		void submitAndSlover(const QString& jobName);

		// 发送模块 
		void sendInfotoServer(const QString& moudle);

	public:
		/************Mech***********/
		// 网格
		void changeToMechMesh() { sendInfotoServer(QString("MechMesh")); }
		// 几何
		void changeToMechGeometry() { sendInfotoServer(QString("MechGeometry")); }
		// 材料
		void changeToMechMaterial() { sendInfotoServer(QString("MechMaterial")); }
		// 属性
		void changeToMechProperty() { sendInfotoServer(QString("MechProperty")); }
		// 单元
		void changeToMechElement() { sendInfotoServer(QString("MechElement")); }


		// 分析工况
		void changeToMechAnalyse() { sendInfotoServer(QString("MechAnalyse")); }
		// 载荷
		void changeToMechLoad() { sendInfotoServer(QString("MechLoad")); }
		// 边界条件
		void changeToMechBoundary() { sendInfotoServer(QString("MechBoundary")); }
		// 连接
		void changeToMechConnect() { sendInfotoServer(QString("MechConnect")); }
		// 接触
		void changeToMechContact() { sendInfotoServer(QString("MechContact")); }


		// 组合
		void changeToMechGroup() { sendInfotoServer(QString("MechGroup")); }
		// 输出
		void changeToMechOutput() { sendInfotoServer(QString("MechOutput")); }
		// 优化
		void changeToMechOptimize() { sendInfotoServer(QString("MechOptimize")); }
		// 任务列表
		void changeToMechJob() { sendInfotoServer(QString("MechJob")); }
		// 工具面板
		void changeToMechTool() { sendInfotoServer(QString("MechTool")); }


		// 集合面板
		void changeToMechSet() { sendInfotoServer(QString("MechSet")); }
		// 曲线
		void changeToMechXYPlot() { sendInfotoServer(QString("MechXYPlot")); }
		// 铺层列表
		void changeToMechLayers() { sendInfotoServer(QString("MechLayers")); }
		// 坐标系
		void changeToMechCoordinate() { sendInfotoServer(QString("MechCoordinate")); }
		// 离散系统
		void changeToMechDiscrete() { sendInfotoServer(QString("MechDiscrete")); }


		// 场列表
		void changeToMechField() { sendInfotoServer(QString("MechField")); }

		// 裂纹
		void changeToMechSrack() { sendInfotoServer(QString("MechSrack")); }


	private:
		static mClient*			_instance;
		static std::mutex		mutex_mClient;		//保证线程安全

		Ui::mClient*			_ui;
		QTcpSocket*				socket{};

		QTimer*					_sendTimer{};
		QTimer*					_isConnectTimer{};

		QString					_hostName{};
		QString					_IPaddr{};
		QString					_loginTime{};
		QString					_logoutTime{};
		QString					_moduleName{ "MxMech" };
		QString					_userName{};

		bool					_isUseFloatingLicense{ false };
		QString					_currentModule{};
		QStringList				_AllModule{

								QString("MechMesh"),		// 网格
								QString("MechGeometry"),	// 几何
								QString("MechMaterial"),	// 材料
								QString("MechProperty"),	// 属性
								QString("MechElement"),		// 单元

								QString("MechAnalyse"),		// 分析工况
								QString("MechLoad"),		// 载荷
								QString("MechBoundary"),	// 边界条件
								QString("MechConnect"),		// 连接
								QString("MechContact"),		// 接触

								QString("MechGroup"),		// 组合
								QString("MechOutput"),		// 输出
								QString("MechOptimize"),	// 优化
								QString("MechJob"),			// 任务列表
								QString("MechTool"),		// 工具面板

								QString("MechSet"),			// 集合面板
								QString("MechXYPlot"),		// 曲线
								QString("MechCoordinate"),	// 铺层列表
								QString("MechFace"),		// 坐标系
								QString("MechDiscrete"),	// 离散系统

								QString("MechField"),		// 场列表

		};
	};
}