#pragma once
#pragma execution_character_set("utf-8")

#include <QObject>
#include <QPointer>
#include <QHash>
#include <QVector>

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


namespace PostWindow {

	//初始模态数据
    class initialPanelData : public QObject
	{
    public:
        // 默认构造函数
        initialPanelData();

        // 析构函数
        ~initialPanelData();

    public:
		QString eigenPanel;				            // 模态类型
		int subcaseID;							    // 工况ID
		QHash<int, double>	initialPanelFrequnecy;	// 模态阶数,阶数原始频率
	};

    //频率对应模态数据
	class  mPostFrequencyPanelData : public QObject
	{
	public:
		// 默认构造函数
        mPostFrequencyPanelData();

		// 析构函数
		~mPostFrequencyPanelData();

        //通过工况ID和结果类型获取GridID
        int getGridIDBySubcaseIDAndResultType(int caseID, QString resType);

        //通过工况ID和结果类型获取分量
        bool getComponentBySubcaseIDAndResultType(int caseID, QString resType, int id, QString &compo);

        //通过工况ID和结果类型获取频率
        bool getFrequencyBySubcaseIDAndResultType(int caseID, QString resType,int gridid, QString component,double &fre);

        //通过工况ID和结果类型获取总响应
        bool getTotalResponseBySubcaseIDAndResultType(int caseID, QString resType, int gridid, QString component, double &totalRes);

        //通过工况ID和结果类型获取存在影响的模态阶数
        QSet<QString> getPanelIndexBySubcaseIDAndResultType(int caseID, QString resType);

        //根据频率获取总响应
        double getTotalResponseByFrequence(int caseID, QString resType, int gridid, QString component, double fre);

        //获取去除模态响应的总响应
        double getNewResponseAfterExcludePanel(int caseID, QString resType, int grid, QString com, double fre, QVector<QString> excludeIndex, double ratio);

        //获取频率下各模态及计算的百分比
        void getPanelPercentByFrequence(int caseID, QString resType, double frequence, int gridid, QString component, QList<QPair<QString, double>> &panelPercents);
	
        //获取频率下各模态及计算的幅值
        void getPanelMagnitudeByFrequence(int caseID, QString resType, double frequence, int gridid, QString com, QList<QPair<QString, double>> &panelMagnitude);
    public:
        QString participationFactors_;				    //参与因素
        int subcaseId_;                                 //工况ID
        double frequence_;                              //频率
        int gridID_;                                    //gridID
        QString component_;                             //分量名
        double totalResponse_;                          //总响应
        float totalAngle_;                              //总相位角
        QHash<QString, QPair<double, double>> panelResponse_; //模态阶数,频率模态阶数响应<实部,虚部>
	};

	class mPostPanelParticipationData : public QObject
	{
	public:

		// 默认构造函数
		mPostPanelParticipationData();

		// 析构函数
		~mPostPanelParticipationData();

        //获取所有工况ID
        QVector<int> getAllSubcaseID();

        //根据工况ID获取对应的结果类型
        QStringList getResultTypeBySubcaseID(int caseId);

        //去重
        QStringList removeDuplicates(const QStringList& list);
        QVector<int> removeDuplicates(const QVector<int>& vector);

        //通过工况ID和结果类型获取GridID
        QVector<int> getGridIDBySubcaseIDAndResultType(int caseID, QString resType);

        //通过工况ID和结果类型获取分量
        QStringList getComponentBySubcaseIDAndResultType(int caseID, QString resType, int gridid);

        //通过工况ID和结果类型获取所有频率
        QVector<double> getAllFrequencyBySubcaseIDAndResultType(int caseID, QString resType,int gridid, QString component);

        //通过工况ID和结果类型获取所有总响应
        QVector<double> getAllTotalResponseBySubcaseIDAndResultType(int caseID, QString resType, int gridid, QString component);

        //通过工况ID和结果类型获取存在影响的模态阶数
        QSet<QString> getPanelIndexBySubcaseIDAndResultType(int caseID, QString resType);

        //通过范围获取频率
        QVector<double> getFrequenceByRange(double min, double max);

        //通过频率获取对应总响应
        QVector<double> getTotalResponseByFrequence(int caseID, QString resType, int gridid, QString component, QVector<double> fres);

        //获取频率下总响应
        double getTotalResponseByFrequence(int caseID, QString resType, int gridid, QString component, double fres);

        //获取去除模态响应的总响应
        QVector<double> getNewResponseAfterExcludePanel(int caseID, QString resType, int grid, QString com, QVector<double> fres, QVector<QString> excludeIndex, double ratio);

        //获取频率下各模态及计算的百分比
        void getPanelPercentByFrequence(int caseID, QString resType, double frequence, int gridid, QString com, QList<QPair<QString, double>> &PanelPercents);

        //获取频率下各模态及计算的幅值
        void getPanelMagnitudeByFrequence(int caseID, QString resType, double frequence, int gridid, QString com, QList<QPair<QString, double>> &PanelMagnitude);
	
        //获取所有的模态名称
        QList<QString> getAllPanelName(int caseID, QString resType);

        //根据模态获取对应所有频率及幅值
        void getAllFreAndMagnitudeByPanel(int caseID, QString resType, QString Panel, QVector<double> &fres, int gridid, QString com, QVector<double> &magnitudes);
    
        //获取初始模态频率
        //double getInitialPanelFrequency(int caseID, int Panel);
    
    public:
        //QList<QPointer<initialPanelData>> allIniPanelData_;                     //所有初始模态数据
		QList<QPointer<mPostFrequencyPanelData>> allFrequencyPanelData_;        //所有频率模态数据
        QVector<double> allFrequence_;                                        //所有频率
	};
}
