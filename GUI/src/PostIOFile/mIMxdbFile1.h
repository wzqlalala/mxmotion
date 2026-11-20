#pragma once
#pragma execution_character_set("utf-8")
#include "miofile_global.h"

#include <QObject>
#include <QThread>
#include <QHash>
#include <QString>
#include <iostream>
#include <fstream>
#include <QStringList>
#include <QVector>
#include <set>
#include <mutex>

#include "mIOEnum.h"
#include "mElementFunction.h"
#include "mPostDataManager.h"
#include "mPostDataBase.h"
#include "mPostModelData.h"
#include "mPostEnumType.h"
#include "mPostResultDataStruct.h"
#include "tsl/robin_set.h"
 
#include "mModelData.h"
#include "mViewportModel.h"
#include "mViewportModelManager.h"
#include "PostCurveStruct.h"
#include "MxdbReaderCore/MxdbReaderBase.h"
#include "MxdbReaderCore/MxdbModelData.h"
#include "mPostOneFrameRendData.h"
#include "mPostAnimationRendData.h"
#include "mDataPost1.h"

#define DEBUGQHASH

namespace PostData
{
	class mPostDataBase;
	class mPostDataManager;
}

namespace MechPreData
{
	class mViewportModel;
}

using namespace std;
using namespace mxr;
using namespace PostData;

namespace MIOFile
{
#ifdef DEBUGQHASH

	template <typename TK, typename TV>
	QList<TK> getQHashKeys(const QHash<TK, TV>& hash) {
		QList<TK> keys;
		for (auto it = hash.constBegin(); it != hash.constEnd(); ++it) {
			keys.append(it.key());
		}
		return keys;
	}

	template <typename TK, typename TV>
	QList<TV> getQHashValues(const QHash<TK, TV>& hash) {
		QList<TV> values;
		for (auto it = hash.constBegin(); it != hash.constEnd(); ++it) {
			values.append(it.value());
		}
		return values;
	}
#endif // DEBUGQHASH


	struct NodeDataMxReader
	{
		int			ID{};
		double		Vertex[3]{};
	};

	class MIOFILE_EXPORT mIMxdbFile1 : public QThread
	{
		Q_OBJECT
	public:

		/** 默认构造函数 */
		mIMxdbFile1();

		/** 重载构造函数 initfile表示初始构型的文件，仅有模型数据  filenames所有帧文件 */
		mIMxdbFile1(const QString& initfile, const QStringList& filenames, QPointer<mPostDataBase> modeldb, QObject* parent = NULL);

		/** 析构函数 */
		~mIMxdbFile1();

		/** 获取后处理数据库名称 */
		QString getPostDataName();

		/** 设置单帧结果数据 */
		void setOneFrameRenderData(PostData::mPostOneFrameRendData* oneframe);

		/** 获取单帧结果数据 */
		PostData::mPostOneFrameRendData* getOneFrameRenderData();

		/** 设置输出文件 */
		void setOutputFile(bool flag);

		/** 设置AI模块标识 */
		void setAIStatus(bool flag);

        //柱坐标系转化
        QVector3D transVector(const QVector3D& x, const QVector3D& a, const QVector3D& b, const QVector3D& c);

        /** 设置初始帧文件名（用于按需读取时判断产品线/精度） */
        void setInitFileName(const QString& initFileName) { _initFileName = initFileName; }

        /** 获取初始帧文件名 */
        QString getInitFileName() const { return _initFileName; }

 public slots:

		/** 导入全部文件（获取模型数据和帧信息） */
		void slot_importAllMxDbFile();

		/** 读取指定单元包含的节点数据 */
		bool readElemData(const QString & pname, int f, const QString & var, const QString & compo, const QString & layer, const QString & complex, const QString & global, QVector<double> local, std::set<int> eleids, bool isexport = false);

		/** 动画读取 */
		void slot_importOneFrameData(PostData::mPostAnimationRendData* animationData, int f, int threadNum, const QString& pname, const QString& var, const QString& compo, const QString& layer, const QString& complex, bool hasstreamLine, QString streamLineVectorName, bool hasvectorArrowVector, QString vectorArrowVectorName);

		/** 读取单帧变量数据（主要用于云图和动画） */
		bool readCloudData(const QString& pname, int f, const QString& var, const QString& compo, const QString& layer, const QString& complex, bool isexport = false);

		/** 主要用于二次结果计算的时候计算变量 */
		bool readCloudDataResult(const QString& pname, int f, const QString& var, const QString& compo, const QString& layer, const QString& complex, bool isexport = false);

		/** 读取单帧变量数据（主要用于云图和动画） */
		bool readCloudData(const QString& pname, int f, const QString& var, const QString& compo, const QString& layer, const QString& complex, const QString& global, QVector<double> local, bool isexport = false);

		//输出结果
		bool readResultData(const QString& pname, int f, const QString& var, const QStringList& compos, const QString& layer, const QString& complex, const QString& global, QVector<double> local, std::set<int>& ids, QHash<QString, QHash<int, double>>& allVals, int& flag);

		//输出结果
		bool readResultData(const QString& pname, int f, const QString& var, const QStringList& compos, const QString& layer, const QString& complex, const QString& global, QVector<double> local, QHash<QString, QHash<int, double>>& allVals, int& flag);

		/** 读取单帧变量数据（主要用于高级查询） */
		bool readCloudData(const QString& pname, int f, const QString& var, const QString& compo, const QString& layer, const QString& complex, const QString& global, QVector<double> local, Compones& componeData, bool isexport = false);

		/** 读取单帧变量数据（主要用于云图和动画） */
		bool readCloudData(const QString& pname, int f, const QString& var, const QString& compo, bool isexport = false);

		//输出结果功能
		bool readCloudData(const QString& pname, int f, const QString& var, const QStringList& compo,std::set<int>& ids, QHash<QString, QHash<int, double>>& allVals, int& flag);

		//输出结果功能
		bool readCloudData(const QString& pname, int f, const QString& var, const QStringList& compo, QHash<QString, QHash<int, double>>& allVals, int& flag);

		/** 读取单元节点变量：输出 单元id→(真实节点id→分量值数组)（分量顺序与变量分量表一致）；elementIds为空读全部，否则仅解码所选单元；只出参不写数据库缓存（主要用于截面力） */
		bool readElementNodeCompValues(const QString& pname, int f, const QString& var, QHash<int, QHash<int, QVector<double>>>& eleNodeCompVals, const std::vector<int>& elementIds = {});

		/** 读取单帧位移矢量数据（主要用于变形图） */
		void readDisplacementData(const QString& pname, int f, const QString& var);

		/** 读取单帧位移矢量数据（主要用于变形图） */
		void readDisplacementDataResult(const QString& pname, int f, const QString& complexstr);


		/** 读取时间-变量曲线数据  */    // 新增工况判断
		void readCurveData1(const QString& pname, const QString& var, const QStringList& compos, const QString& layer, const QString& complex, const QVector<int>& ids,
							QStringList& cnames, QVector<double>& xvalues, QHash<QString, QVector<double>>& yvalues, QString caseName = "All");

		/** 读取ID-变量曲线数据 */   // 新增工况判断
		void readCurveData2(const QString& pname, const QString& var, const QStringList& compos, const QString& layer, const QString& complex, const QVector<int>& ids, const set<int>& frameIds,
							QStringList& cnames, QHash<QString, QHash<int, double>>& yvalues, QString caseName = "All");

		/** 读取变量-变量曲线数据 */  // 新增工况判断
		void readCurveData3(const QString& pname, const QString& varx, const QString& compox, const QString& layerx, const QString& complexx,
							const QString& vary, const QStringList& composy, const QString& layery, const QString& complexy,
							const QVector<int>& ids, QStringList& cnames, QVector<double>& xvalues, QHash<QString, QVector<double>>& yvalues, QString caseName = "All");
		/*
		* 读取变量数据（磨平应力）
		*/
		bool readFlattenStress(const QString& pname, int f, const QString& var, const QString& compo, const QString& layer, const QString& complex, const QString& global, QVector<double> local, bool isexport = false);

		/** 三维矩阵乘以三维列向量 */
		QVector<double> multiplyMatrixVector(const QVector<double>& matrix, const QVector<double>& vector);

		/** 获取结果 */
		QHash<int, float> getRendDataValue() const;

		/** 获取位移结果 */
		QHash<int, QVector3D> getDisRendDataValue() const;

        /** 进度条显示*/
        void setProgressBarValue(std::streampos pos);

		/** 界面重构-读取时间-变量曲线数据  */
		void readTimeVarCurve(const QString& pname, const QString& var, QList<CurveKey> curves, const QVector<int>& ids,
			QStringList& cnames, QVector<double>& xvalues, QHash<QString, QVector<double>>& yvalues);

		/** 界面重构-读取ID-变量曲线数据  */
		void readIDVarCurve(const QString& pname, const QString& var, QList<CurveKey> curves, const QVector<int>& ids, const set<int>& frameIds,
			QStringList& cnames, QHash<QString, QHash<int, double>>& yvalues, QString caseName);

	private:

		/** 重载run函数 */
		void run() override;

		/** 判断文件状态 */
		bool openFileStatus(const QString& fname);

		/** 读取初始模型数据 */
		bool readInitModelDatas(QPointer<mPostDataBase> modeldb);

		/** 判断单元某些位置节点是否为同一个节点 */
		bool isSameNode(int* ids, std::vector<int> poss);

		/** 将D3H20组成D3P15 */
		void d3h20ToD3p15(int* ids, std::vector<int>& nodeIds);

		/** 根据产品线创建对应的 MxdbReader；version 为 0 时按 _productLine 选择，否则优先按版本号选择 */
		std::unique_ptr<MxdbReader::MxdbReaderBase> createMxdbReader(std::ifstream& fs, uint32_t version = 0) const;

		/** 根据产品线/文件内容创建对应的 MxdbReader（用于按需读取时自动判断显式 v05+） */
		std::unique_ptr<MxdbReader::MxdbReaderBase> createMxdbReaderForFile(std::ifstream& fs, const QString& fileName) const;

		/** 将 PostVariableInfo 转换为 MxdbVariable，用于 MxdbReader */
		MxdbReader::MxdbVariable convertToMxdbVariable(const PostData::PostVariableInfo& info) const;

		/** 通用按需读取变量值 */
		bool readVariableValuesHelper(
			const QString& frameFile,
			const PostData::PostVariableInfo& varInfo,
			const std::vector<int>& entityIds,
			std::vector<MxdbReader::MxdbVariableValue>& outValues);

		/** 帧文件 reader 缓存：同一帧文件复用已打开的 reader（reader 持成员流引用，
		*缓存生命周期与实例一致；切帧/切变量不再重复构造 reader 与重开文件） */
		MxdbReader::MxdbReaderBase* ensureFrameReader(const QString& frameFile);

		/** 根据单元类型计算单元-节点面格式中每个节点的分量数 */
		int elementTypeToComVarCount(MxFunctions::ElementType eleType) const;

		/** 通用按需读取单元-节点面格式变量值 */
		bool readElementNodeFaceValuesHelper(
			const QString& frameFile,
			const PostData::PostVariableInfo& varInfo,
			PostData::mPostModelData* modelData,
			const std::vector<int>& elementIds,
			std::vector<MxdbReader::MxdbElementNodeFaceValue>& outValues);

	signals:

		/** 发送信号，反馈文件读取情况 参数type表示错误的类型 */
		void readMxdbErrorSig(IOFileErrorType type = UnknownError);

		/** 发送信号，文件读取成功 */
		void finishedMxdbFileSig(const QString& fname);

		/** 发送信号，读取全部帧文件成功 */
		void finishedAllMxdbSig(const QString& postname);

		/** 发送信号，读取全部帧文件成功 */
		void finishedImportAllMxDbFileSig();

		/** 发送信号，读取全部帧文件成功 */
		void readFileSuccessSig(const QString& postname);



	private:

		unsigned int				_version{};												//版本号
		QString						_initFileName{}; 										//初始构型文件
		QStringList					_frameFiles{};											//帧文件
		//按需读取帧文件缓存（ensureFrameReader）：reader 持 _frameStream 引用，
		//成员化保证缓存 reader 的流生命周期与实例一致
		std::ifstream				_frameStream{};
		std::unique_ptr<MxdbReader::MxdbReaderBase> _frameReader{};
		QString						_frameReaderFile{};										//缓存 reader 当前打开的帧文件
		QString						_postDataName{};										//后处理数据库名称
		PostData::mPostOneFrameRendData*		_oneFrameRender{};																	//单帧结果数据
		int							_productLine{ 0 };														//产品线类型：0-隐式动力学，1-显式，2-电机
		int							_frame{};												//生成动画需要的参数
		PostData::mPostAnimationRendData*		_animationData{};
		int							_threadNum{};
		bool						_hasstreamLine{};
		QString						_streamLineVectorName{};
		bool						_hasvectorArrowVector{};
		QString						_vectorArrowVectorName{};
		QString						_pname{};
		QString						_var{};
		QString						_compo{};
		QString						_layer{};
		QString						_complex{};
		QHash<int, float>			_rendDataValue{};
		QHash<int, QVector3D>		_disDataValue{};
		std::mutex					_mutex_for_mulitfile_read{};
		int frames = 0;
		bool _outputFile = false;

		QPointer<mPostDataBase> _modeldb;

		QHash<int, int> _flag;  //key 帧号  value -1标识与其他不相同  0-n表示与其他帧相同

		tsl::robin_set<int> _twoDimFaceId{};   //二维单元面ID

        //模型文件大小
        quint64 _fileSize;

		bool   _isAI = false;//标识是否为AI模块数据读取
	};
}