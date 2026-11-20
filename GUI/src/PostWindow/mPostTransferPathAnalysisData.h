#pragma once
#pragma execution_character_set("utf-8")

#include <QObject>
#include <QPointer>
#include <QVector>

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


namespace PostWindow {

	//传递函数文件数据
    class mPostTransferFunctionData : public QObject
	{
    public:
        // 默认构造函数
        mPostTransferFunctionData();

        // 析构函数
        ~mPostTransferFunctionData();

    public:
		QString                                     _transferPath;	        //传递路径
        QString                                     _type;                  //类型(加速度等)
		int                                         _outputType = -1;		//输出类型（1：幅值相位 2：实部虚部）
        int                                         _subcaseID;             //工况ID
        int                                         _resultType;            //结果类型（1：按频率 2：按点）
        QVector<double>                             _frequency;             //频率
        QVector<int>                                _nodeId;                //节点ID
        QVector<QHash<int, QPair<double, double>>>  _allComponentValue;     //所有分量对应的值
	};

    //输入力文件数据
    //响应数据
	class  mPostResponseData : public QObject
	{
	public:
		// 默认构造函数
        mPostResponseData();

		// 析构函数
		~mPostResponseData();

    public:
        QString                                     _type;                  //类型(加速度等)
        int                                         _outputType = -1;		//输出类型（1：幅值相位 2：实部虚部）
        int                                         _subcaseID;             //工况ID
        int                                         _resultType;            //结果类型（1：按频率 2：按点）
        QVector<double>                             _frequency;             //频率
        QVector<int>                                _nodeId;                //节点ID
        QVector<QHash<int, QPair<double, double>>>  _allComponentValue;     //所有分量对应的值
	};

    //输入力数据
    class  mPostForceData : public QObject
    {
    public:
        // 默认构造函数
        mPostForceData();

        // 析构函数
        ~mPostForceData();

    public:
        int                                         _forceType;             //输入力类型(1:单元力 2：节点力)
        int                                         _outputType = -1;		//输出类型（1：幅值相位 2：实部虚部）
        int                                         _subcaseID;             //工况ID
        int                                         _resultType;            //结果类型（仅单元力 1：按频率 2：按点）
        int                                         _elemenType;            //单元类型（仅单元力）
        QVector<double>                             _frequency;             //频率(节点力只有按频率输出的格式)
        QVector<int>                                _elementId;             //单元ID（仅单元力）
        QVector<QPair<int, int>>                    _nodeElementID;         //节点、单元ID（仅节点力）
        QVector<QHash<int, QPair<double, double>>>  _allComponentValue;     //所有分量对应的值
    };

    //所有TPA分析数据
	class mPostTransferPathAnalysisData : public QObject
	{
	public:

		// 默认构造函数
        mPostTransferPathAnalysisData();

		// 析构函数
		~mPostTransferPathAnalysisData();

        //去重
        QStringList removeDuplicates(const QStringList& list);
        QVector<int> removeDuplicates(const QVector<int>& vector);

        //获取所有工况ID
        QVector<int> getAllSubcaseID();

        //根据工况ID获取对应的结果类型
        QStringList getResultTypeBySubcaseID(int caseId);
        
        //通过工况ID和结果类型获取响应ID
        QVector<int> getResponseIDBySubcaseIDAndResultType(int caseID, QString resType);

        //通过工况、结果类型、响应ID获取所有频率
        QVector<double> getAllFrequency(int caseID, QString resType, int responseID);

        //通过工况、结果类型、响应ID、自由度索引获取所有分量数据
        QVector<QPair<double,double>> getAllSolverResponseData(int caseID, QString resType, int responseID, int componentindex);

        //通过工况、结果类型、响应ID、自由度索引获取指定频率范围内的所有分量数据
        QVector<QPair<double,double>> getAllSolverResponseDataByRange(int caseID, QString resType, int responseID, int componentindex, double min, double max);

        //通过工况、结果类型、响应ID获取所有路径名称
        QStringList getAllNodePathName(QString resType, int responseID);

        //通过路径、响应ID、自由度获取所有频率下的传递函数数据
        QHash<QString, QVector<QPair<double, double>>> getAllFrequencyTranseferData(QStringList paths, int responseID, int componentindex);

        //通过路径、响应ID、自由度获取指定频率范围内的传递函数数据
        QHash<QString, QVector<QPair<double, double>>> getAllFrequencyTranseferDataByRange(QStringList paths, int responseID, int componentindex, double min, double max);

        //通过路径截取的单元ID和自由度获取所有频率下对应的输入力数据
        QHash<QString, QVector<QPair<double, double>>> getAllFrequencyForceData(QStringList paths);
        QHash<QString, QVector<QPair<double, double>>> getAllFrequencyForceData(QStringList paths,int subcaseid);

        //通过路径截取的单元ID和自由度获取指定频率范围内对应的输入力数据
        QHash<QString, QVector<QPair<double, double>>> getAllFrequencyForceDataByRange(QStringList paths, double min, double max);

        //获取输入力数据的自由度
        int getForceIndex(QString str);

        //获取传递函数文件输出类型
        int getTransferFileType();

        //获取输入力文件输出类型
        int getForceFileType();

        //通过范围获取频率
        QVector<double> getFrequenceByRange(double min, double max);

        //通过频率获取对应总响应
        QVector<double> getTotalResponseByFrequence(int caseID, QString resType, int gridid, QString component, QVector<double> fres);


    public:
        QList<QPointer<mPostTransferFunctionData>>      _allTransferFunctionData;               //所有传递函数数据
		QList<QPointer<mPostResponseData>>              _allResponseData;                       //输入力文件所有响应数据
        QList<QPointer<mPostForceData>>                 _allForceData;                          //输入力文件所有力数据
        QVector<double>                                 _allFrequence;                          //所有频率
	};

}
