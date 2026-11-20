#pragma once
#pragma execution_character_set("utf-8")
#include "miofile_global.h"
#include "mBasicEnum.h"
#include <QThread>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QVector>
#include <QVector3D>
#include <QString>
#include <iostream>
#include <fstream>
#include <set>

using namespace std;
namespace MIOFile
{
	struct OneElementData;
	struct NodeData
	{
		NodeData() {};

		NodeData(double *vertex) : _vertex(vertex) {};

		void appendShellElement(OneElementData* ele)
		{
			_shellElements.insert(ele);
		};

		set<OneElementData*> getShellElements()
		{
			return _shellElements;
		};

		//获取节点坐标
		QVector3D getVertex()
		{
			return QVector3D(_vertex[0], _vertex[1], _vertex[2]);
		};

		//获取节点力对应的单元
		QVector3D getForce(int elementID, int frameID)
		{
			return _nodeElementForce.value(frameID).value(elementID);
		};

		~NodeData()
		{
			delete _vertex;
		}

		set<OneElementData*>				_shellElements{};					//三角形壳或者四边形壳
		QHash<int, QHash<int, QVector3D>>	_nodeElementForce{};				//节点的平衡力(帧ID-单元ID-力)
		double*								_vertex{};
	};

	struct OneElementData
	{
		OneElementData(QString elementType, int ID, QVector<int> list) :_elementType(elementType), _ID(ID), _index(list) {};
		QString								_elementType{};						//mx的名称格式
		int									_ID{};
		QVector<int>						_index{};
	};
	struct ThirdVariable
	{
		QVector<QString>					_variablesNames{};						//所有分量名称
		int									multiShapeVariablesNum{};				//多形状变量个数
		bool								isVector{};								//是否计算矢量模
		QVector<int>						_layerIDs{};							//层ID
		int									nodeOrElement{};						//节点或者单元变量
		QVector<int>						IDs{};									//分量对应的ID
		QVector<double>						_variables{};							//分量(每一个分量的值)
	};

	struct LoadForce
	{
		QVector<int>						IDs{};									//分量对应的ID
		int									cengshu{};
		QVector<double>						_variables{};							//分量(每一个分量的值)
	};

	struct ThirdFrameData
	{
		float								time{};
		QString								caseName{};
		QHash<QString, ThirdVariable>		_allVariables{};						//变量名称-所有分量的值
		QHash<QString, LoadForce>			_loadForces{};							//变量名称-值（载荷提取专用）
	};

	class mIF06PostFile;
	class MIOFILE_EXPORT mIF06File : public QThread
	{
		Q_OBJECT

	public:

		/** 默认构造函数 */
		mIF06File(const QString& filename);

		/** 析构函数 */
		~mIF06File();

		/** 获取生成的mxdb所在的路径 */
		QString getmxdbFilePath();

	private:

		/** 重载run函数 */
		void run() override;

	private:

		QString				_fileName{};				//打开的文件名
		mIF06PostFile*		_f06PostFile{};			//读f06文件的对象
		QString				_mxdbFilePath{};			//生成的后处理的文件所在的路径

	};

	class MIOFILE_EXPORT mIF06PostFile : public QObject
	{
		Q_OBJECT

	public:

		mIF06PostFile() {};

		~mIF06PostFile();

		/** 读取文件 */
		void ReadFile(const QString& filename);

		/** 写文件 */
		QString WriteFile();

		/** 读取模型 */
		void ReadModelData(QFile &file, bool is2xin = true);

		/** 读取变量 */
		void ReadVariablesData(QFile &file);

	private:
		/** 读单元 */
		void ReadElementData(QFile &file, bool is2xin = true);

		/** 读铺层变量 */
		void ReadLayeredComposite(QFile &file, ThirdVariable &thirdVariable, LoadForce &shell);

		/** 读strain in xxx变量 */
		void ReadStrain(QFile &file, ThirdVariable &thirdVariable);

		/** 读stress in beam变量 */
		void ReadBeamStress(QFile &file, ThirdVariable &thirdVariable, LoadForce &beamStress);

		/** 读stress in rod变量 */
		void ReadRodStressStrain(QFile &file, ThirdVariable &thirdVariable);

		/** 读stress in shear变量 */
		void ReadShearStress(QFile &file, ThirdVariable &thirdVariable, LoadForce &shearStress);

		/** 读位移变量 */
		void ReadDisplacement(QFile &file, ThirdVariable &thirdVariable1, ThirdVariable &thirdVariable2);

		/** 读Shell Force */
		void ReadShellForce(QFile &file, LoadForce &shellForce, LoadForce &shellForceMoment);

		/** 读shear Load */
		void ReadShearLoad(QFile &file, LoadForce &shearLoad);

		/** 读barEndload */
		void ReadBarEndLoad(QFile &file, LoadForce &barEndLoad);

		/** 读平衡节点力 */
		void ReadGridPointForceBalance(QFile &file, int caseID);

		/** 写QString64位 */
		void writeQString64(ofstream &writef, const QString& str);

		/** 写QString32位 */
		void writeQString32(ofstream &writef, const QString& str);

		/** 计算 */
		void caculate(QVector<double> &values);

		/** 计算barendload */
		void calculateBarEndLoad(int caseID, int eleID, QVector<double> &axil);

		/** 获取两个节点位于壳的局部索引 */
		int* getShellNumByIndex(QVector<int> indexs, int globalID1, int globalID2);

	private:
		QString											_filename{};					//文件名
		QString											currentline{};					//当前行
		QHash<int, NodeData*>							_nodeVertex{};					//节点信息
		QString											_lastString{};					//上一个单元的信息
		QHash<int, QVector<OneElementData*>>			_partID_Elements{};				//属性ID和单元
		QHash<int, int*>								_barID_NodeID{};				//梁单元的单元和节点索引
		double											_shearThickness{};				//剪切板厚度
		QHash<int, ThirdFrameData*>						_caseID_Datas{};				//帧ID和帧信息
		int												_version{};						//版本号(2008、2020)


		static QVector<QString>							_layeredComposite;				//铺层
		static QVector<QString>							_stressBeamNames;				//梁应力
		static QVector<QString>							_stressRodNames;				//杆应力
		static QVector<QString>							_stressShearNames;				//剪切板应力
		static QVector<QString>							_strainRodNames;				//杆应变
		static QHash<QString, QString>					_varibleName_Name;
	};
}