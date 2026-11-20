#pragma once
#include "miofile_global.h"

//Qt
#include <QThread>
#include <QPointer>
#include <iostream>
#include <fstream>
#include <QMap>
#include <set>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QHash>

#include "mPostDataBase.h"
#include "mPostModelData.h"
#include "mPostEnumType.h"
#include "mPostResultDataStruct.h"
#include "tsl/robin_set.h"
#include "PostCurveStruct.h"
#include "MxdbReaderCore/MxdbReaderBase.h"
#include "MxdbReaderCore/NewSingleMxdbReader.h"
#include "MxdbReaderCore/NewFormatMxdbReader.h"
#include <memory>

namespace PostData
{
	class mPostModelData;
	class mPostDataBase;
	class mPostDataManager;
}
using namespace PostData;

namespace MIOFile
{
	class MIOFILE_EXPORT mMxdbThread : public QThread
	{
		Q_OBJECT

	public:
		mMxdbThread(const QString& inp_file, QPointer<mPostDataBase> modeldb, QObject* parent = NULL);

		~mMxdbThread();

		bool isReadSuccess();

		QString getPathName();

        QString getModelName();

        void setProgressBarValue(std::streampos pos);

    public:
        /*
        * 读取模型数据和帧信息
        */
        bool readModelAndFrameInfo();

		/*
		* 新版格式大文件首次扫描提示开关（默认开；会话恢复的自动导入置 false 静默）
		*/
		void setNotifyScanHint(bool on) { _notifyScanHint = on; }

        bool readModelAndFrameInfo_TestData();

        /*
        * 读取变量数据（添加自定义坐标系）
        */
        bool readVarDatas(const QString& filePath, int f, const QString& varName, const QString& compoName, const QString& layer, const QString& complex, QVector<double> local);

        //输出结果
        bool readVarDatas(const QString& filePath, int f, const QString& varName, const QStringList& compos, const QString& layer, const QString& complex, QVector<double> local, std::set<int> selids, QHash<QString, QHash<int, double>>& allVals, int& flag);

        //输出结果
        bool readVarDatas(const QString& filePath, int f, const QString& varName, const QStringList& compos, const QString& layer, const QString& complex, QVector<double> local, QHash<QString, QHash<int, double>>& allVals, int& flag);

        /** 读取指定单元包含的节点数据 */
        bool readElemData(const QString& filePath, int f, const QString& varName, const QString& compoName, const QString& layer, const QString& complex, QVector<double> local, std::set<int> eleids);

        /*
        * 读取变量数据（磨平应力）
        */
        bool readFlattenStress(const QString& filePath, int f, const QString& varName, const QString& compoName, const QString& layer, const QString& complex, QVector<double> local);

        /*
        * 读取变量数据（用于高级查询模块）
        */
        bool readVarDatas(const QString& filePath, int f, const QString& varName, const QString& compoName, const QString& layer, const QString& complex, Compones& componeData, QVector<double> local);

        // TODO:读取矢量数据主要用于流线图和矢量图
        // 存到mOutputDatabase中_vectorData中,这里只会传的变量
        bool readVectorData(const QString& filePath, int f, const QString& varName);

        /*
        *读取ID-变量曲线数据
        */
        void readCurveData(
            const QString& pname,
            const QString& var,
            const QStringList& compos,
            const QString& layer,
            const QString& complex,
            const QVector<int>& ids,
            const std::set<int>& frameIds,
            QStringList& cnames,
            QHash<QString, QHash<int, double>>& yvaluesD,
            QHash<QString, QHash<int, float>>& yvaluesF,
            QString caseName = "All");

        /** 读取时间-变量曲线数据 */
        void readTimeVarCurveData(
            const QString& pname,
            const QString& var,
            const QStringList& compos,
            const QString& layer,
            const QString& complex,
            const QVector<int>& ids,
            QStringList& cnames,
            QVector<double>& xvaluesD,
            QHash<QString, QVector<double>>& yvaluesD,
            QHash<QString, QVector<float>>& yvaluesF,
            QString caseName = "All");

        /** 读取变量-变量曲线数据 */
        void readVarVarCurveData(
            const QString& pname,
            const QString& varx,
            const QString& compox,
            const QString& layerx,
            const QString& complexx,
            const QString& vary,
            const QStringList& composy,
            const QString& layery,
            const QString& complexy,
            const QVector<int>& ids,
            QStringList& cnames,
            QVector<double>& xvaluesD,
            QVector<float>& xvaluesF,
            QHash<QString, QVector<double>>& yvaluesD,
            QHash<QString, QVector<float>>& yvaluesF,
            QString caseName = "All");


        /** 界面重构-读取时间-变量曲线数据 */
        void readTimeVarCurve(
            const QString& pname,
            const QString& var,
            QList<CurveKey> curves,
            const QVector<int>& ids,
            QStringList& cnames,
            QVector<double>& xvaluesD,
            QHash<QString, QVector<double>>& yvaluesD,
            QHash<QString, QVector<float>>& yvaluesF);

        ///** 界面重构-读取ID-变量曲线数据 */
        void readIDVarCurve(
            const QString& pname,
            const QString& var,
            QList<CurveKey> idCurves,
            const QVector<int>& ids,
            const std::set<int>& frameIds,
            QStringList& cnames,
            QHash<QString, QHash<int, double>>& yvaluesD,
            QHash<QString, QHash<int, float>>& yvaluesF,
            QString caseName);

    private:
        //柱坐标系转化
        QVector3D transVector(const QVector3D& x, const QVector3D& a, const QVector3D& b, const QVector3D& c);

        //判断数据精度是否是double
        bool DataPrecisionIsDouble();


	private:
		/*
		* 运行线程
		*/
		void run();

		/*
		* 判断版本号是否支持
		*/
		bool isMatchedVersion();

		/*
		* 当前文件是否为新版记录式格式（0xA1B1C601/602，v07.mxdb 规范）
		*/
		bool isNewFormatFile() const { return _version == 0xA1B1C601 || _version == 0xA1B1C602; }

		/*
		* 将文件指针跳转到某帧
		*/
		void moveReader(int f);

		/*
		* 判断文件打开状态
		*/
		bool openFileStatus(const QString& filename);

		//按需读取辅助：确保 _reader 已打开指定文件
		bool ensureReaderOpen(const QString& filename);

		//将 PostVariableInfo 转换为 MxdbVariable（逻辑同 mIMxdbFile1::convertToMxdbVariable）
		MxdbReader::MxdbVariable convertToMxdbVariable(const PostData::PostVariableInfo& info) const;

		//将 reader 解析出的帧变量信息写入对应帧的 PostFrameInfo._frameVars
		void convertFrameVariables(const std::vector<MxdbReader::MxdbVariable>& vars, int frameId);

		//201/202 的 Displacement 是普通变量，import 时预读到 DisplacementValuesR
		void fillLegacyDisplacement(MxdbReader::NewSingleMxdbReader* legacyReader, int frameId);

		//按需读取变量值
		bool readVariableValuesCore(const PostData::PostVariableInfo& varInfo,
			const std::vector<int>& entityIds,
			std::vector<MxdbReader::MxdbVariableValue>& outValues);

		//新格式（601/602）按需读取：按分量名/层（截面点名）/复数方式（实部/虚部/幅值/相位）
		//取每个实体一个标量值写入 outVals；valueType 返回 0 节点 / 1 单元。
		//值布局由 reader 返回的 MxdbVariableValue 携带（单元变量取第一结果位置），无旧格式布局假设。
		//键契约：节点值键 = 连续节点 cid（渲染端 _raw_value 按下标直取，v07 文件节点 ID 可稀疏，
		//文件 ID 须先经 nodeidmap 转换）；单元值键 = 文件单元 ID（消费端哈希查找）
		bool readNewFormatScalarValues(const PostData::PostVariableInfo& varInfo,
			const QString& compoName, const QString& layer, const QString& complex,
			QHash<int, double>& outVals, int& valueType,
			PostData::mPostModelData* modelData = nullptr);

		//桥接：将 reader 读出的变量值摊平为旧布局连续数组（喂给 calcu*）
		// 输出：ids（实体ID）、real（实部，每实体 ncompo 个连续值）、imag（虚部，复数时非空）
		// 返回：是否成功（变量名不匹配或读取失败返回 false）
		bool readVarValuesToArrays(const PostData::PostVariableInfo& curVarInfo,
			const std::vector<int>& entityIds,
			std::vector<int>& ids,
			std::vector<double>& real,
			std::vector<double>& imag);

		//按需读取单元-节点面变量值
		bool readElementNodeFaceValuesCore(const PostData::PostVariableInfo& varInfo,
			PostData::mPostModelData* modelData,
			const std::vector<int>& elementIds,
			std::vector<MxdbReader::MxdbElementNodeFaceValue>& outValues);

		//旧版变量块读取（委托 _reader；readLegacyVarHead 查 varByte 缓存，readLegacyVarValues 摊平）
		// vpE1/vpE2：实体块/壳块每实体值数（20/16/nCompo），(0,0) 两段式保留旧流读取（Stress(N) 待样本）
		bool readLegacyVarHead(long long varByte, int& typeOut, std::string& nameOut);
		bool readLegacyVarValues(int vpE1, int vpE2, bool isComplex,
			std::vector<int>& ids1, std::vector<double>& real1, std::vector<double>& imag1,
			std::vector<int>& ids2, std::vector<double>& real2, std::vector<double>& imag2);
		//两段式：先读块头+ids（供调用方计算动态 count），再读值（Stress(N) 专用，待样本迁移）
		bool readLegacyVarBlockHead(bool secondBlock, std::vector<int>& ids, int& numsOut);
		bool readLegacyVarBlockValues(int count, bool isComplex, std::vector<double>& real, std::vector<double>& imag);

		/*
		* 扫描文件统计每个部件的所有单元所包含的节点个数
		*/
		void scanFile(bool isD);

		/*
		* 递归读取装配信息的子节点
		* nums 子节点个数
		*/
		void readAssemblyInfos(const int nums, QList<QPair<QString, int>>& infos, QStringList& partNames);

		/*
		* 计算实体分量值
		* values： 变量值指针数组  nums： 单元或节点个数  lis：当前变量的分量名 ids：单元或节点ID  varIndex：分量索引
		*/
		QHash<int, double> calcuComponeValues(const double* values, int nums, const QStringList& lis, int* ids, int varIndex);
		//添加自定义坐标系
		QHash<int, double> calcuComponeValues(const double* values, int nums, const QStringList& lis, int* ids, int varIndex, QVector<double> local);
		
		/*
		* 计算实体分量值
		*/
		QHash<int, float> calcuComponeValues(float* values, int nums, const QStringList& lis, int* ids, int varIndex);
		//添加自定义坐标系
		QHash<int, float> calcuComponeValues(float* values, int nums, const QStringList& lis, int* ids, int varIndex, QVector<double> local);

		/*
		* 计算壳分量值
		*/
		QHash<int, double> calcuComponeValues1(double* values, int nums, const QStringList& lis, const QString& layer, int* ids, int varIndex);

		/*
		* 计算壳分量值
		*/
		QHash<int, float> calcuComponeValues1(float* values, int nums, const QStringList& lis, const QString& layer, int* ids, int varIndex);
	
		/*
		* 单元节点应力计算实体分量值
		* values： 变量值指针数组  nums： 单元或节点个数  lis：当前变量的分量名 ids：单元或节点ID  varIndex：分量索引
		*/
		QHash<int, QHash<int, double>> calcuEleNodeComponeValuesR(const double* values, int nums, const QStringList& lis, int* ids, int varIndex, QPointer<mPostModelData> modelData, QVector<double> local);

		/*
		* 单元节点应力计算实体分量值
		*/
		QHash<int, QHash<int, float>> calcuEleNodeComponeValuesR(float* values, int nums, const QStringList& lis, int* ids, int varIndex, QPointer<mPostModelData> modelData, QVector<double> local);

		
		QHash<int, double> calcuEleNodeComponeValR(const double* values, int nums, const QStringList& lis, int* ids, int varIndex, QPointer<mPostModelData> modelData, QVector<double> local);

		
		QHash<int, float> calcuEleNodeComponeValR(float* values, int nums, const QStringList& lis, int* ids, int varIndex, QPointer<mPostModelData> modelData, QVector<double> local);

		/*
		* 单元节点应力计算壳分量值
		*/
		QHash<int, QHash<int, double>> calcuEleNodeComponeValuesQ(double* values, int nums, const QStringList& lis, const QString& layer, int* ids, int varIndex, QPointer<mPostModelData> modelData, QVector<double> local);

		/*
		* 单元节点应力计算壳分量值
		*/
		QHash<int, QHash<int, float>> calcuEleNodeComponeValuesQ(float* values, int nums, const QStringList& lis, const QString& layer, int* ids, int varIndex, QPointer<mPostModelData> modelData, QVector<double> local);

		QHash<int, double> calcuEleNodeComponeValQ(double* values, int nums, const QStringList& lis, const QString& layer, int* ids, int varIndex, QPointer<mPostModelData> modelData, QVector<double> local);

		QHash<int, float> calcuEleNodeComponeValQ(float* values, int nums, const QStringList& lis, const QString& layer, int* ids, int varIndex, QPointer<mPostModelData> modelData, QVector<double> local);

		//梁、杆单元节点应力
		QHash<int, QHash<int, double>> calcuEleNodeComponeValues(double* values, int nums, const QStringList& lis,int* ids, int varIndex, QPointer<mPostModelData> modelData);

		//梁、杆单元节点应力
		QHash<int, QHash<int, float>> calcuEleNodeComponeValues(float* values, int nums, const QStringList& lis, int* ids, int varIndex, QPointer<mPostModelData> modelData);


		QHash<int, double> calcuEleNodeComponeVal(double* values, int nums, const QStringList& lis, int* ids, int varIndex, QPointer<mPostModelData> modelData);

		QHash<int, float> calcuEleNodeComponeVal(float* values, int nums, const QStringList& lis, int* ids, int varIndex, QPointer<mPostModelData> modelData);

	signals:
		// 读完数据将模型名字和模型路径名字发出
		void mxdbReadFinished(const QString& modelName, const QString& pathName);

	private slots:
		void slot_readVarValue();

	private:
		//解析结果标识
		bool _inpFlag;

		//文件版本号
		unsigned int _version;


		//MxdbReaderCore 读取流与读取器（201/202 为 NewSingleMxdbReader，601/602 为 NewFormatMxdbReader）
		std::ifstream _mxdbStream;
		std::unique_ptr<MxdbReader::MxdbReaderBase> _reader;
		//ensureReaderOpen 已成功 open 的文件：同文件复用 reader 不重扫帧索引（601/602 的
		//open 为全文件顺序扫描，池化实例按需读取时重复 open 是最大开销；open 失败时清空）
		QString _openedReaderFile;

		// readLegacyVarHead 记录的当前变量（readLegacyVarValues 用）
		long long _lastHeadVarByte = -1;

		//新格式整体变量（varType=2）首版不注册显示，只提示一次
		bool _newFormatWholeVarWarned = false;

		//新版格式大文件扫描提示开关（仅用户手动导入时发消息）
		bool _notifyScanHint = true;

		// 模型名字
		QString mModelName{};

		// 模型路径名字
		QString mPathName{};

        QHash<QString, int> _elenodes_num_per_part;
        QHash<QString, int> _ele_num_per_part;

		QPointer<mPostDataBase> _modeldb;

		QHash<int, int> _flag;  //key 帧号  value -1标识与其他不相同  0-n表示与其他帧相同

		tsl::robin_set<int> _twoDimFaceId{};   //二维单元面ID

        quint64 _fileSize;           //文件大小
		

			// 调试QHash
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
	};
}