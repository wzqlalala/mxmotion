#include "mIF06File.h"
#include "math.h"
#include "mGlobalSignals.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QTextStream>
#include <QDirModel>
#include <QTextCodec>
#include <QSet>
#include <QDebug>
#include <QTransform>

#define EPS 1e-10
using namespace MxFunctions;
namespace MIOFile
{
	mIF06File::mIF06File(const QString& filename) :_fileName(filename)
	{
		_f06PostFile = new mIF06PostFile;
	}

	mIF06File::~mIF06File()
	{
		if (_f06PostFile != nullptr)
			delete _f06PostFile;
	}

	QString mIF06File::getmxdbFilePath()
	{
		return _mxdbFilePath;
	}

	void mIF06File::run()
	{
		//判断文件是否存在
		QFileInfo finfo(_fileName);
		if (!finfo.exists())
			return;
		QString fsuffix = finfo.suffix();
		if (fsuffix == QString("f06"))
		{
			mGlobalSignals::getInstance()->outputMessageSig(0, QString("正在读取f06文件!"), true);
			//读取f06文件，读取后处理的数据
			_f06PostFile->ReadFile(_fileName);
			mGlobalSignals::getInstance()->outputMessageSig(0, QString("正在写mxdb文件!"), true);
			//写成后处理文件，mxdb0，mxbd1。
			_mxdbFilePath = _f06PostFile->WriteFile();
		}
		else
		{
		}

	}

	mIF06PostFile::~mIF06PostFile()
	{
		QHashIterator<int, ThirdFrameData*> iter(_caseID_Datas);
		while (iter.hasNext())
		{
			iter.next();
			ThirdFrameData* thirdFrameData = iter.value();
			delete thirdFrameData;
		}
		_caseID_Datas.clear();

		for (NodeData* nodeData : _nodeVertex)
			delete nodeData;
		_nodeVertex.clear();
		QHashIterator<int, QVector<OneElementData*>> iter1(_partID_Elements);
		while (iter1.hasNext())
		{
			iter1.next();

			QVector<OneElementData*> elementDatas = iter1.value();
			for (OneElementData* elementData : elementDatas)
				delete elementData;
		}
		_partID_Elements.clear();


	}

	void mIF06PostFile::ReadFile(const QString & filename)
	{
		_filename = filename;
		QTextCodec *codeC = QTextCodec::codecForName("GBK");
		QFile file(filename);
		QFileInfo l_info(filename);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))//以读的方式打开文件
			return;

		bool _hasModel{false};//判断是否有模型信息

		while (!file.atEnd())
		{
			//读取版本
			if (currentline.contains("Version"))
			{
				QStringList linelist = currentline.split("Version");
				if (linelist.size() == 2)
					_version = linelist.at(1).simplified().mid(0, 4).toInt();
				currentline = file.readLine().simplified();
			}
			//读取工况信息
			else if (currentline.contains("C A S E C O N T R O L E C H O", Qt::CaseInsensitive))
			{
				while (!file.atEnd())
				{
					currentline = file.readLine().simplified();
					if (currentline.contains("SUBCASE") && !currentline.contains("$"))
					{
						QStringList linelist = currentline.split(" ");
						if (linelist.size() == 3)
						{
							int ID = linelist.last().toInt();
							currentline = file.readLine().simplified();
							while (!file.atEnd())
							{
								if (currentline.contains("$"))
								{
									linelist = currentline.split(":");
									break;
								}
								else if (currentline.contains("="))
								{
									linelist = currentline.split("=");
									break;
								}
								else
									currentline = file.readLine().simplified();
							}
							ThirdFrameData *frameData = new ThirdFrameData;
							frameData->caseName = linelist.last();
							_caseID_Datas[ID] = frameData;
						}
					}
					else if (currentline.contains("MSC Nastran") || currentline.contains("MD NASTRAN"))
					{
						currentline = file.readLine().simplified();
						break;
					}
				}
			}
			//读取节点信息
			else if (currentline.contains("S O R T E D B U L K D A T A E C H O", Qt::CaseInsensitive))
			{
				_hasModel = true;
				ReadModelData(file);
			}
			else if (currentline.contains("I N P U T B U L K D A T A E C H O", Qt::CaseInsensitive))
			{
				_hasModel = true;
				ReadModelData(file, false);
			}
			else if (currentline.contains("SUBCASE", Qt::CaseInsensitive) && currentline.at(0) == "0")
			{
				if (!_hasModel)
				{
					file.close();
					return;
				}
				ReadVariablesData(file);
			}
			else
			{
				currentline = file.readLine().simplified();
			}
		}

		file.close();

		//计算杆轴力的提取
		QHashIterator<int, ThirdFrameData*> iter(_caseID_Datas);
		while (iter.hasNext())
		{
			iter.next();
			if (iter.value()->_loadForces.contains("BarEndLoad"))
			{
				LoadForce &loadForce = iter.value()->_loadForces["BarEndLoad"];
				for (int i = 0; i < loadForce.IDs.size(); ++i)
				{
					int Id = loadForce.IDs.at(i);
					QVector<double> temp = loadForce._variables.mid(4 * i, 4);
					calculateBarEndLoad(iter.key(), loadForce.IDs.at(i), temp);
					loadForce._variables[4 * i] = temp.at(0);
					loadForce._variables[4 * i + 1] = temp.at(1);
					loadForce._variables[4 * i + 2] = temp.at(2);
					loadForce._variables[4 * i + 3] = temp.at(3);
				}
			}
		}
	}

	QString mIF06PostFile::WriteFile()
	{
		QFileInfo finfo(_filename);
		QString name = finfo.baseName();
		QString path = finfo.absolutePath() + "/" + name;

		QDir dir(path);
		if (dir.exists(path))
		{
			dir.removeRecursively();
		}
		dir.mkpath(path);

		//模型文件
		//打开文件
		QString modelFileName = path + "/" + name + ".mxdb0";
		QFile file(modelFileName);
		file.open(QIODevice::WriteOnly);
		ofstream writef;
		writef.open(modelFileName.toLocal8Bit(), ios::out | ios::binary);
		if (writef)
		{
			//写入版本号
			int version = 0xA1B1C102;
			writef.write((char*)&version, sizeof(int));

			//节点数目
			int nodeNum = _nodeVertex.size();
			writef.write((char*)&nodeNum, sizeof(int));

			QList<int> nodeIDs = _nodeVertex.keys();
			sort(nodeIDs.begin(), nodeIDs.end());
			for (int i = 0; i < nodeNum; i++)
			{
				//节点ID
				int ID = nodeIDs.at(i);
				writef.write((char*)&ID, sizeof(int));

				//节点坐标
				writef.write((char*)_nodeVertex.value(ID)->_vertex, 3 * sizeof(double));
			}


			QList<int> partIDs = _partID_Elements.keys();
			partIDs.toSet().toList();
			sort(partIDs.begin(), partIDs.end());

			//part个数
			int partNum = partIDs.size();
			writef.write((char*)&partNum, sizeof(int));

			for (int i = 0; i < partIDs.size(); i++)
			{
				int PID = partIDs.at(i);

				//part名字
				QString partName = QString("part_%1").arg(i + 1);
				this->writeQString64(writef, partName);

				QVector<OneElementData*> elements = _partID_Elements.value(PID);
				if (elements.size() != 0)
				{
					int elementNum = elements.size();
					writef.write((char*)&elementNum, sizeof(int));

					for (int i = 0; i < elementNum; i++)
					{
						OneElementData *elementData = elements.at(i);

						//单元ID
						writef.write((char*)&elementData->_ID, sizeof(int));

						//单元类型字符串
						this->writeQString32(writef, elementData->_elementType);

						//单元包含的节点
						writef.write((char*)elementData->_index.data(), sizeof(int) * elementData->_index.size());
					}
				}
			}
		}
		//关闭文件
		writef.close();
		//帧文件
		QHashIterator<int, ThirdFrameData*> iter(_caseID_Datas);
		while (iter.hasNext())
		{
			iter.next();
			int frameID = iter.key();
			ThirdFrameData *thirdFrameData = iter.value();
			if (thirdFrameData == nullptr)
				continue;

			QString frameFileName = QString("%1/%2_%3.mxdb").arg(path).arg(name).arg(frameID);
			QFile file(frameFileName);
			file.open(QIODevice::WriteOnly);
			writef.open(frameFileName.toLocal8Bit(), ios::out | ios::binary);
			if (writef)
			{
				//写入版本号
				int version = 0xA1B1C102;
				writef.write((char*)&version, sizeof(int));

				//写工况名
				this->writeQString64(writef, thirdFrameData->caseName);

				//时间
				writef.write((char*)&frameID, sizeof(double));

				//f
				int f = 0;
				writef.write((char*)&f, sizeof(int));

				//变量个数
				int variableNum = thirdFrameData->_allVariables.size() + thirdFrameData->_loadForces.size();
				writef.write((char*)&variableNum, sizeof(int));

				//普通变量
				QHashIterator<QString, ThirdVariable> iterVariable(thirdFrameData->_allVariables);
				while (iterVariable.hasNext())
				{
					iterVariable.next();

					//结果类型
					int type = 0;
					writef.write((char*)&type, sizeof(int));

					//变量名
					QString variableName = iterVariable.key();
					this->writeQString64(writef, variableName);

					//类型
					writef.write((char*)&iterVariable.value().nodeOrElement, sizeof(int));

					//多形状变量个数
					int value = 1;
					writef.write((char*)&value, sizeof(int));

					//层数
					int	ceng = iterVariable.value()._layerIDs.size();
					writef.write((char*)&ceng, sizeof(int));

					//层名称
					for (int i = 0; i < ceng; i++)
					{
						QString cengname = QString("%1").arg(iterVariable.value()._layerIDs.at(i));
						this->writeQString32(writef, cengname);
					}

					//分量数
					const QVector<QString> &variablesNames = iterVariable.value()._variablesNames;
					int numberOfComponents = variablesNames.size();
					writef.write((char*)&numberOfComponents, sizeof(int));

					//是否计算模
					writef.write((char*)&iterVariable.value().isVector, sizeof(bool));

					//各个分量
					for (int i = 0; i < numberOfComponents; i++)
					{
						//分量名
						this->writeQString32(writef, variablesNames.at(i));

						//是否为复数
						bool isComplex = false;
						writef.write((char*)&isComplex, sizeof(bool));
					}

					//该变量开始的字节位置
					long long int tellg = (long long int)writef.tellp() + sizeof(long long int);
					writef.write((char*)&tellg, sizeof(long long int));

					//变量具体信息
					const QVector<int> &IDs = iterVariable.value().IDs;
					const QVector<double> &variables = iterVariable.value()._variables;

					//个数
					int IDSize = iterVariable.value().IDs.size();
					writef.write((char*)&IDSize, sizeof(int));

					if (ceng == 0)
					{
						ceng = 1;
					}
					int interval = ceng * numberOfComponents;
					double *xus = new double[interval];
					fill(xus, xus + interval, 0);
					for (int j = 0; j < IDSize; j++)
					{
						//ID
						int ID = IDs.at(j);
						writef.write((char*)&ID, sizeof(int));

						//实部
						for (int k = 0; k < interval; k++)
						{
							writef.write((char*)&variables.at(j * interval + k), sizeof(double));
						}

						//虚部
						writef.write((char*)&xus, sizeof(double) * interval);

					}
					delete xus;

				}

				//载荷提取变量
				QHashIterator<QString, LoadForce> iterLoadForce(thirdFrameData->_loadForces);
				while (iterLoadForce.hasNext())
				{
					iterLoadForce.next();

					//结果类型
					int type = 2;
					writef.write((char*)&type, sizeof(int));

					//变量名
					QString variableName = iterLoadForce.key();
					this->writeQString64(writef, variableName);

					//该变量开始的字节位置
					long long int tellg = (long long int)writef.tellp() + sizeof(long long int);
					writef.write((char*)&tellg, sizeof(long long int));

					const LoadForce &loadForce = iterLoadForce.value();
					if (loadForce.cengshu == 0)
					{
						int n = loadForce.IDs.size();
						int interval = loadForce._variables.size() / n;
						for (int i = 0; i < n; i++)
						{
							//ID
							int ID = loadForce.IDs.at(i);
							writef.write((char*)&ID, sizeof(int));

							QVector<double> values = loadForce._variables.mid(i * interval, interval);
							writef.write((char*)values.data(), sizeof(double) * interval);
						}
					}
					else
					{
						int n = loadForce.IDs.size();
						int interval = loadForce._variables.size() / (n);
						for (int i = 0; i < n; i++)
						{
							//ID
							int ID = loadForce.IDs.at(i);
							writef.write((char*)&ID, sizeof(int));

							//层数
							writef.write((char*)&loadForce.cengshu, sizeof(int));

							QVector<double> values = loadForce._variables.mid(i * interval, interval);
							writef.write((char*)values.data(), sizeof(double) * interval);
						}
					}
				}

				//写入版本号
				version = 0xA1B1C1D2;
				writef.write((char*)&version, sizeof(int));
			}
			writef.close();
		}

		return path;

	}

	void mIF06PostFile::ReadModelData(QFile & file, bool is2xin)
	{
		currentline = file.readLine().simplified();
		currentline = file.readLine().simplified();

		QStringList linelist;
		while (!file.atEnd())
		{
			if (currentline.contains("MSC Nastran") || currentline.contains("MD NASTRAN"))
			{
				currentline = file.readLine().simplified();
				break;
			}
			currentline = file.readLine().trimmed();

			int index = currentline.indexOf("-");
			if (index > 0)
			{
				currentline.remove(0, index + 1);
				if (currentline.contains("*") && _lastString.isEmpty())//多读一行
				{
					_lastString = currentline.trimmed().remove("        ");
				}
				else//不是即组合
				{
					_lastString += currentline.trimmed().remove("        ");
					//读取单元
					ReadElementData(file, is2xin);
					_lastString = "";
				}
			}
		}

		//_partID_ElementDatas
	}

	void mIF06PostFile::ReadVariablesData(QFile & file)
	{
		QStringList linelist = currentline.split("SUBCASE");
		if (linelist.size() < 1)
		{
			return;
		}
		int subcaseID = linelist.at(1).toInt();
		ThirdFrameData *frameData = _caseID_Datas.value(subcaseID);
		if (frameData == nullptr)
		{
			currentline = file.readLine();
			return;
		}
		currentline = file.readLine();
		while (currentline.simplified().size() == 0)
		{
			currentline = file.readLine();
		}
		currentline = currentline.replace("  ", "*");
		currentline = currentline.replace(" ", "");
		currentline = currentline.replace("*", " ");
		currentline = currentline.simplified();
		QString variableName = currentline;
		int index = currentline.indexOf("(");
		if (index >= 0)
		{
			variableName = currentline.remove(index, variableName.size() - index).simplified();
		}
		variableName = _varibleName_Name.value(variableName);
		if (variableName == "Shell Stress(E)" || variableName == "Shell Strain(E)")//铺层的壳
		{
			if (!frameData->_allVariables.contains(variableName))
			{
				ThirdVariable variable;
				variable.nodeOrElement = 1;
				variable._variablesNames = _layeredComposite;
				frameData->_allVariables.insert(variableName, variable);

				if (variableName.contains("Stress"))
				{
					frameData->_loadForces.insert("ShellStress", LoadForce());
				}
				else
				{
					frameData->_loadForces.insert("ShellStrain", LoadForce());
				}
			}
			ThirdVariable &thirdVariable = frameData->_allVariables[variableName];
			if (variableName.contains("Stress"))
			{
				ReadLayeredComposite(file, thirdVariable, frameData->_loadForces["ShellStress"]);
			}
			else
			{
				ReadLayeredComposite(file, thirdVariable, frameData->_loadForces["ShellStrain"]);
			}

		}
		else if (variableName == "Displacement")
		{
			currentline = file.readLine().simplified();
			currentline = file.readLine().simplified();

			if (!frameData->_allVariables.contains("Displacement"))
			{
				linelist = currentline.split(" ");
				if (linelist.size() >= 3)
				{
					linelist.removeFirst();//移除POINT
					linelist.removeFirst();//移除ID
					linelist.removeFirst();//移除TYPE
					ThirdVariable variable1;
					ThirdVariable variable2;
					for (int i = 0; i < 3; i++)
					{
						variable1._variablesNames.append(linelist.at(i));
					}
					for (int i = 3; i < 6; i++)
					{
						variable2._variablesNames.append(linelist.at(i));
					}
					variable1.isVector = true;
					variable2.isVector = false;
					frameData->_allVariables.insert("Displacement", variable1);
					frameData->_allVariables.insert("Displacement(R)", variable2);
				}
			}
			ThirdVariable &thirdVariable1 = frameData->_allVariables["Displacement"];
			ThirdVariable &thirdVariable2 = frameData->_allVariables["Displacement(R)"];
			ReadDisplacement(file, thirdVariable1, thirdVariable2);
		}
		else if (variableName == "Beam Stress(E)")
		{
			if (!frameData->_allVariables.contains(variableName))
			{
				ThirdVariable variable;
				variable.nodeOrElement = 1;
				variable._variablesNames = _stressBeamNames;
				frameData->_allVariables.insert(variableName, variable);

				frameData->_loadForces.insert("BarStress", LoadForce());
			}
			ThirdVariable &thirdVariable = frameData->_allVariables[variableName];
			LoadForce &barStress = frameData->_loadForces["BarStress"];
			ReadBeamStress(file, thirdVariable, barStress);
		}
		else if (variableName == "Rod Stress(E)")
		{
			if (!frameData->_allVariables.contains(variableName))
			{
				ThirdVariable variable;
				variable.nodeOrElement = 1;
				variable._variablesNames = _stressRodNames;
				frameData->_allVariables.insert(variableName, variable);
			}
			ThirdVariable &thirdVariable = frameData->_allVariables[variableName];
			ReadRodStressStrain(file, thirdVariable);
		}
		else if (variableName == "Rod Strain(E)")
		{
			if (!frameData->_allVariables.contains(variableName))
			{
				ThirdVariable variable;
				variable.nodeOrElement = 1;
				variable._variablesNames = _strainRodNames;
				frameData->_allVariables.insert(variableName, variable);
			}
			ThirdVariable &thirdVariable = frameData->_allVariables[variableName];
			ReadRodStressStrain(file, thirdVariable);
		}
		else if (variableName == "Shear Stress(E)")
		{
			if (!frameData->_allVariables.contains(variableName))
			{
				ThirdVariable variable;
				variable.nodeOrElement = 1;
				variable._variablesNames = _stressShearNames;
				frameData->_allVariables.insert(variableName, variable);

				frameData->_loadForces.insert("ShearStress", LoadForce());
			}
			ThirdVariable &thirdVariable = frameData->_allVariables[variableName];
			LoadForce &shearStress = frameData->_loadForces["ShearStress"];
			ReadShearStress(file, thirdVariable, shearStress);
		}
		else if (variableName == "ShellForceResultant")//载荷提取-壳力/力矩
		{
			if (!frameData->_loadForces.contains("ShellForceResultant"))
			{
				frameData->_loadForces.insert("ShellForceResultant", LoadForce());
				frameData->_loadForces.insert("ShellForceMoment", LoadForce());
			}
			LoadForce &shellForce = frameData->_loadForces["ShellForceResultant"];
			LoadForce &shellMoment = frameData->_loadForces["ShellForceMoment"];
			ReadShellForce(file, shellForce, shellMoment);
		}
		else if (variableName == "ShearLoad")//载荷提取-剪切板力
		{
			if (!frameData->_loadForces.contains("ShearLoad"))
			{
				frameData->_loadForces.insert("ShearLoad", LoadForce());
			}
			LoadForce &shearLoad = frameData->_loadForces["ShearLoad"];
			ReadShearLoad(file, shearLoad);
		}
		else if (variableName == "BarEndLoad")
		{
			if (!frameData->_loadForces.contains("BarEndLoad"))
			{
				frameData->_loadForces.insert("BarEndLoad", LoadForce());
			}
			LoadForce &barEndLoad = frameData->_loadForces["BarEndLoad"];
			ReadBarEndLoad(file, barEndLoad);
		}
		else if (variableName == "Grid Point Force Balance")
		{
			ReadGridPointForceBalance(file, subcaseID);
		}
	}
	void mIF06PostFile::ReadElementData(QFile & file, bool is2xin)
	{
		QString type = _lastString.mid(0, 8).remove("*").simplified();
		if (_version == 2008)
		{

		}
		else
		{
			if (is2xin)
			{
				int index = _lastString.lastIndexOf("*");
				if (index > 1)
				{
					_lastString.remove(index - 1, 9);
				}
			}
			else
			{
				int index = _lastString.indexOf("*");
				if (index >= 0)
				{
					_lastString.remove(index, 1);
					index = _lastString.lastIndexOf("*");
					int yu = index % 8;
					_lastString.replace("*", QString(8 - yu, ' '));
				}
			}
		}
		int size = _lastString.size();
		if (type == "GRID")
		{
			double *vertex = new double[3];
			vertex[0] = _lastString.mid(16, 8).toDouble();
			vertex[1] = _lastString.mid(24, 8).toDouble();
			if (size > 56)
			{
				vertex[2] = _lastString.mid(48, 8).toDouble();
			}
			else
			{
				vertex[2] = _lastString.mid(32, 8).toDouble();
			}
			NodeData *nodeData = _nodeVertex[_lastString.mid(8, 8).toInt()];
			if (nodeData == nullptr)
			{
				nodeData = new NodeData();
				_nodeVertex[_lastString.mid(8, 8).toInt()] = nodeData;
			}
			nodeData->_vertex = vertex;
		}
		else if (type == "CONM2")
		{
			OneElementData *element = new OneElementData(QString("Mass1"), _lastString.mid(8, 8).toInt(), QVector<int>{_lastString.mid(16, 8).toInt()});
			_partID_Elements[0].append(element);
		}
		else if (type == "CBAR" || type == "CBEAM")
		{
			OneElementData *element = new OneElementData(QString("D3B2"), _lastString.mid(8, 8).toInt(), QVector<int>{_lastString.mid(24, 8).toInt(), _lastString.mid(32, 8).toInt()});
			_partID_Elements[_lastString.mid(16, 8).toInt()].append(element);
			int *index = new int[2];
			index[0] = element->_index.at(0);
			index[1] = element->_index.at(1);
			_barID_NodeID.insert(_lastString.mid(8, 8).toInt(), index);
		}
		else if (type == "CROD")
		{
			OneElementData *element = new OneElementData(QString("D3R2"), _lastString.mid(8, 8).toInt(), QVector<int>{_lastString.mid(24, 8).toInt(), _lastString.mid(32, 8).toInt()});
			_partID_Elements[_lastString.mid(16, 8).toInt()].append(element);
			int *index = new int[2];
			index[0] = element->_index.at(0);
			index[1] = element->_index.at(1);
			_barID_NodeID.insert(_lastString.mid(8, 8).toInt(), index);
		}
		else if (type == "CELAS1" || type == "CELAS2")
		{
			OneElementData *element = new OneElementData(QString("ELAS1"), _lastString.mid(8, 8).toInt(), QVector<int>{_lastString.mid(24, 8).toInt(), _lastString.mid(32, 8).toInt()});
			_partID_Elements[_lastString.mid(16, 8).toInt()].append(element);
		}
		else if (type == "CTRIA3" || type == "CTRIAR")
		{
			OneElementData *element = new OneElementData(QString("CPDSG"), _lastString.mid(8, 8).toInt(), QVector<int>{_lastString.mid(24, 8).toInt(), _lastString.mid(32, 8).toInt(), _lastString.mid(40, 8).toInt()});
			_partID_Elements[_lastString.mid(16, 8).toInt()].append(element);
		}
		else if (type == "CSHEAR")
		{
			OneElementData *element = new OneElementData(QString("SPQ4"), _lastString.mid(8, 8).toInt(), QVector<int>{_lastString.mid(24, 8).toInt(), _lastString.mid(32, 8).toInt(), _lastString.mid(40, 8).toInt(), _lastString.mid(48, 8).toInt()});
			_partID_Elements[_lastString.mid(16, 8).toInt()].append(element);
			for (int index : element->_index)
			{
				NodeData *nodeData = _nodeVertex[index];
				if (nodeData == nullptr)
				{
					nodeData = new NodeData();
					_nodeVertex[index] = nodeData;
				}
				nodeData->appendShellElement(element);
			}
		}
		else if (type == "CQUAD4")
		{
			OneElementData *element = new OneElementData(QString("QPH"), _lastString.mid(8, 8).toInt(), QVector<int>{_lastString.mid(24, 8).toInt(), _lastString.mid(32, 8).toInt(), _lastString.mid(40, 8).toInt(), _lastString.mid(48, 8).toInt()});
			_partID_Elements[_lastString.mid(16, 8).toInt()].append(element);
		}
		else if (type == "CTETRA")
		{
			OneElementData *element = new OneElementData(QString("D3T4"), _lastString.mid(8, 8).toInt(), QVector<int>{_lastString.mid(24, 8).toInt(), _lastString.mid(32, 8).toInt(), _lastString.mid(40, 8).toInt(), _lastString.mid(48, 8).toInt()});
			_partID_Elements[_lastString.mid(16, 8).toInt()].append(element);
		}
		else if (type == "CPENTA")
		{
			OneElementData *element = new OneElementData(QString("D3P5"), _lastString.mid(8, 8).toInt(), QVector<int>{_lastString.mid(24, 8).toInt(), _lastString.mid(32, 8).toInt(), _lastString.mid(40, 8).toInt(), _lastString.mid(48, 8).toInt(), _lastString.mid(56, 8).toInt()});
			_partID_Elements[_lastString.mid(16, 8).toInt()].append(element);
		}
		else if (type == "CHEXA")
		{
			OneElementData *element = new OneElementData(QString("D3H8"), _lastString.mid(8, 8).toInt(), QVector<int>{_lastString.mid(24, 8).toInt(), _lastString.mid(32, 8).toInt(), _lastString.mid(40, 8).toInt(), _lastString.mid(48, 8).toInt(), _lastString.mid(56, 8).toInt(), _lastString.mid(64, 8).toInt(), _lastString.mid(72, 8).toInt(), _lastString.mid(80, 8).toInt()});
			_partID_Elements[_lastString.mid(16, 8).toInt()].append(element);
		}
		else if (type == "PSHEAR")//剪切板厚度
		{
			_shearThickness = _lastString.mid(24, _lastString.size() - 24).toDouble();
		}
	}

	void mIF06PostFile::ReadLayeredComposite(QFile & file, ThirdVariable &thirdVariable, LoadForce &shell)
	{
		currentline = file.readLine().simplified();
		currentline = file.readLine().simplified();
		QStringList linelist;
		while (!file.atEnd())
		{
			currentline = file.readLine().simplified();
			if (currentline.contains("MSC Nastran") || currentline.contains("***") || currentline.contains("MD NASTRAN"))
			{
				break;
			}
			linelist = currentline.split(" ");
			linelist.removeFirst();//移除0
			int ID = linelist.takeFirst().toInt();
			int layerID = linelist.takeFirst().toInt();
			QVector<double> shellValues(9, 0.0);
			for (int i = 0; i < linelist.size(); i++)
			{
				thirdVariable._variables.append(linelist.at(i).toDouble());
				shellValues[i] = linelist.at(i).toDouble();
			}
			swap(shellValues[3], shellValues[5]);
			shellValues[3] = shellValues[2];
			shellValues[2] = shellValues[1];
			shellValues[1] = shellValues[0];
			caculate(shellValues);
			shell._variables.append(shellValues);
			if (!thirdVariable.IDs.isEmpty() && thirdVariable.IDs.last() == ID)
			{

			}
			else
			{
				thirdVariable.IDs.append(ID);
				shell.IDs.append(ID);
			}
			if (1 == thirdVariable.IDs.size())
			{
				thirdVariable._layerIDs.append(layerID);
				shell.cengshu++;
			}
		}
	}

	void mIF06PostFile::ReadStrain(QFile & file, ThirdVariable & thirdVariable)
	{
		currentline = file.readLine().simplified();
		currentline = file.readLine().simplified();
		QStringList linelist;
		while (!file.atEnd())
		{
			currentline = file.readLine().simplified();
			if (currentline.contains("MSC Nastran") || currentline.contains("***") || currentline.contains("MD NASTRAN"))
			{
				break;
			}
			linelist = currentline.split(" ");
			linelist.removeFirst();//移除0
			int ID = linelist.takeFirst().toInt();
			int layerID = linelist.takeFirst().toInt();
			for (int i = 0; i < linelist.size(); i++)
			{
				thirdVariable._variables.append(linelist.at(i).toDouble());
			}
			if (!thirdVariable.IDs.isEmpty() && thirdVariable.IDs.last() == ID)
			{

			}
			else
			{
				thirdVariable.IDs.append(ID);
			}
			if (1 == thirdVariable.IDs.size())
			{
				thirdVariable._layerIDs.append(layerID);
			}
		}
	}

	void mIF06PostFile::ReadBeamStress(QFile & file, ThirdVariable & thirdVariable, LoadForce &beamStress)
	{
		currentline = file.readLine().simplified();
		currentline = file.readLine().simplified();
		QStringList linelist;
		while (!file.atEnd())
		{
			currentline = file.readLine().simplified();
			if (currentline.contains("MSC Nastran") || currentline.contains("***") || currentline.contains("MD NASTRAN"))
			{
				break;
			}
			linelist = currentline.split(" ");
			linelist.removeFirst();//移除0
			int ID = linelist.takeFirst().toInt();
			thirdVariable.IDs.append(ID);
			beamStress.IDs.append(ID);
			QVector<double> values;
			values.reserve(13);
			for (int i = 0; i < linelist.size(); i++)
			{
				values.append(linelist.at(i).toDouble());
			}

			currentline = file.readLine().simplified();
			linelist = currentline.split(" ");
			for (int i = 0; i < linelist.size(); i++)
			{
				values.append(linelist.at(i).toDouble());
			}
			thirdVariable._variables.append(values);
			beamStress._variables.append(QVector<double>{values.at(0), values.at(7), values.at(1), values.at(8), values.at(2), values.at(9), values.at(3), values.at(10), values.at(4), values.at(5), values.at(11), values.at(6), values.at(12)});
		}
	}

	void mIF06PostFile::ReadRodStressStrain(QFile & file, ThirdVariable & thirdVariable)
	{
		currentline = file.readLine().simplified();
		currentline = file.readLine().simplified();
		QStringList linelist;
		while (!file.atEnd())
		{
			currentline = file.readLine().simplified();
			if (currentline.contains("MSC Nastran") || currentline.contains("***") || currentline.contains("MD NASTRAN"))
			{
				break;
			}
			linelist = currentline.split(" ");
			int ID = linelist.takeFirst().toInt();
			thirdVariable.IDs.append(ID);
			for (int i = 0; i < 2; i++)
			{
				thirdVariable._variables.append(linelist.at(i).toDouble());
			}
			ID = linelist.takeFirst().toInt();
			thirdVariable.IDs.append(ID);
			for (int i = 2; i < 4; i++)
			{
				thirdVariable._variables.append(linelist.at(i).toDouble());
			}
		}
	}

	void mIF06PostFile::ReadShearStress(QFile & file, ThirdVariable & thirdVariable, LoadForce &shearStress)
	{
		currentline = file.readLine().simplified();
		currentline = file.readLine().simplified();
		QStringList linelist;
		while (!file.atEnd())
		{
			currentline = file.readLine().simplified();
			if (currentline.contains("MSC Nastran") || currentline.contains("***") || currentline.contains("MD NASTRAN"))
			{
				break;
			}
			linelist = currentline.split(" ");
			int ID = linelist.takeFirst().toInt();
			thirdVariable.IDs.append(ID);
			shearStress.IDs.append(ID);
			for (int i = 0; i < 2; i++)
			{
				thirdVariable._variables.append(linelist.at(i).toDouble());
				shearStress._variables.append(linelist.at(i).toDouble());
			}
			shearStress._variables.append(_shearThickness);

			ID = linelist.at(2).toInt();
			thirdVariable.IDs.append(ID);
			shearStress.IDs.append(ID);
			for (int i = 3; i < 5; i++)
			{
				thirdVariable._variables.append(linelist.at(i).toDouble());
				shearStress._variables.append(linelist.at(i).toDouble());
			}
			shearStress._variables.append(_shearThickness);
		}
	}

	void mIF06PostFile::ReadDisplacement(QFile &file, ThirdVariable &thirdVariable1, ThirdVariable &thirdVariable2)
	{
		QStringList linelist;
		while (!file.atEnd())
		{
			currentline = file.readLine().simplified();
			if (currentline.contains("MSC Nastran") || currentline.contains("***") || currentline.contains("MD NASTRAN"))
			{
				break;
			}
			linelist = currentline.split(" ");
			int ID = linelist.takeFirst().toInt();
			linelist.removeFirst();//移除G
			for (int i = 0; i < 3; i++)
			{
				thirdVariable1._variables.append(linelist.at(i).toDouble());
			}
			for (int i = 3; i < 6; i++)
			{
				thirdVariable2._variables.append(linelist.at(i).toDouble());
			}
			thirdVariable1.IDs.append(ID);
			thirdVariable2.IDs.append(ID);
		}
	}

	void mIF06PostFile::ReadShellForce(QFile & file, LoadForce &shellForce, LoadForce &shellForceMoment)
	{
		currentline = file.readLine().simplified();
		currentline = file.readLine().simplified();
		currentline = file.readLine().simplified();

		QStringList linelist;
		while (!file.atEnd())
		{
			currentline = file.readLine().simplified();
			if (currentline.contains("MSC Nastran") || currentline.contains("***") || currentline.contains("MD NASTRAN"))
			{
				break;
			}
			linelist = currentline.split(" ");
			if (linelist.size() == 11)
			{
				linelist.removeFirst();
				int ID = linelist.takeFirst().toInt();
				linelist.removeFirst();
				QVector<double> forcevalues(9, 0.0);
				QVector<double> momentvalues(9, 0.0);
				//FX、FY、FZ
				for (int i = 0; i < 3; i++)
				{
					forcevalues[i + 1] = linelist.takeFirst().toDouble();
				}
				//MX、MY、MZ
				for (int i = 0; i < 3; i++)
				{
					momentvalues[i + 1] = linelist.takeFirst().toDouble();
				}
				caculate(forcevalues);
				caculate(momentvalues);
				shellForce.IDs.append(ID);
				shellForceMoment.IDs.append(ID);
				shellForce._variables.append(forcevalues);
				shellForce._variables.append(forcevalues);
				shellForceMoment._variables.append(momentvalues);
				shellForceMoment._variables.append(momentvalues);
			}
		}

	}

	void mIF06PostFile::ReadShearLoad(QFile & file, LoadForce & shearLoad)
	{
		currentline = file.readLine().simplified();
		currentline = file.readLine().simplified();
		currentline = file.readLine().simplified();

		QStringList linelist;
		while (!file.atEnd())
		{
			currentline = file.readLine().simplified();
			if (currentline.contains("MSC Nastran") || currentline.contains("***") || currentline.contains("MD NASTRAN"))
			{
				break;
			}
			linelist = currentline.split(" ");
			if (linelist.size() == 10)
			{
				linelist.removeFirst();
				int ID = linelist.takeFirst().toInt();
				currentline = file.readLine().simplified();
				linelist = currentline.split(" ");

				QVector<double> forcevalues(4, 0.0);
				//FX、FY、FZ
				for (int i = 0; i < 4; i++)
				{
					forcevalues[i] = linelist.at(2 * i + 1).toDouble();
				}
				shearLoad.IDs.append(ID);
				shearLoad._variables.append(forcevalues);
			}
		}

	}

	void mIF06PostFile::ReadBarEndLoad(QFile & file, LoadForce & barEndLoad)
	{
		currentline = file.readLine().simplified();
		currentline = file.readLine().simplified();

		QStringList linelist;
		while (!file.atEnd())
		{
			currentline = file.readLine().simplified();
			if (currentline.contains("MSC Nastran") || currentline.contains("***") || currentline.contains("MD NASTRAN"))
			{
				break;
			}
			linelist = currentline.split(" ");
			if (linelist.size() == 9)
			{
				int ID = linelist.takeFirst().toInt();

				double axial = linelist.at(6).toDouble();

				barEndLoad.IDs.append(ID);
				QVector<double> tempAxial(4, axial);
				//calculateBarEndLoad(ID, tempAxial);
				barEndLoad._variables.append(tempAxial);
			}
			else
			{
				int ID = linelist.takeFirst().toInt();
				double axial = linelist.at(0).toDouble();
				barEndLoad.IDs.append(ID);
				QVector<double> tempAxial(4, axial);
				//calculateBarEndLoad(ID, tempAxial);
				barEndLoad._variables.append(tempAxial);

				ID = linelist.at(2).toInt();
				axial = linelist.at(3).toDouble();
				barEndLoad.IDs.append(ID);
				tempAxial = QVector<double>(4, axial);
				//calculateBarEndLoad(ID, tempAxial);
				barEndLoad._variables.append(tempAxial);
			}
		}
	}

	void mIF06PostFile::ReadGridPointForceBalance(QFile & file, int caseID)
	{
		currentline = file.readLine().simplified();
		currentline = file.readLine().simplified();

		QStringList linelist;
		while (!file.atEnd())
		{
			currentline = file.readLine().simplified();
			if (currentline.contains("MSC Nastran") || currentline.contains("***") || currentline.contains("MD NASTRAN"))
			{
				break;
			}
			linelist = currentline.split(" ");
			if (linelist.size() == 9 && linelist.at(0).simplified() != "0")
			{
				int nodeID = linelist.takeFirst().toInt();

				int eleID = linelist.takeFirst().toInt();

				QVector3D force(linelist.at(1).toDouble(), linelist.at(2).toDouble(), linelist.at(3).toDouble());

				_nodeVertex[nodeID]->_nodeElementForce[caseID][eleID] = force;
			}
		}

	}

	void mIF06PostFile::writeQString64(ofstream & writef, const QString & str)
	{
		char strchar[64];
		memcpy(strchar, str.toStdString().c_str(), 64);
		writef.write(strchar, 64 * sizeof(char));
	}
	void mIF06PostFile::writeQString32(ofstream & writef, const QString & str)
	{
		char strchar[32];
		memcpy(strchar, str.toStdString().c_str(), 32);
		writef.write(strchar, 32 * sizeof(char));
	}

	void mIF06PostFile::caculate(QVector<double>& values)
	{
		double SIG = 0.5 * (values[1] + values[2]);
		double PROJ = 0.5 * (values[1] - values[2]);
		double TAUMAX = PROJ * PROJ + values[3] * values[3];

		TAUMAX = sqrt(TAUMAX);
		if (TAUMAX <= EPS)
		{
			TAUMAX = 0.;
		}
		// 计算角度
		double TXY2 = values[3] * 2;
		PROJ *= 2.;
		// 计算主方向上
		values[6] = SIG + TAUMAX;
		values[7] = SIG - TAUMAX;
		values[8] = TAUMAX;
		// 计算VonMises
		SIG = values[0] * values[0] + values[1] * values[1] - values[0] * values[1];
		SIG = sqrt(SIG);
		if (SIG <= EPS)
		{
			SIG = 0.;
		}
		values[0] = SIG;
	}

	void mIF06PostFile::calculateBarEndLoad(int caseID, int eleID, QVector<double>& axil)
	{
		double &axil1 = axil[2];
		double &axil2 = axil[3];

		int rev = 1;

		//获取杆梁单元的全局节点索引
		int *globalIndex = _barID_NodeID.value(eleID);

		//获取两个索引的公共壳单元
		set<OneElementData*> eles1 = _nodeVertex.value(globalIndex[0])->getShellElements();
		set<OneElementData*> eles2 = _nodeVertex.value(globalIndex[1])->getShellElements();
		set<OneElementData*> shellEles;
		std::set_intersection(eles1.begin(), eles1.end(), eles2.begin(), eles2.end(), inserter(shellEles, shellEles.begin()));

		for (OneElementData *shell : shellEles)
		{
			QVector<int> indexs = shell->_index;

			// 确定节点位于单元的局部节点索引
			int *index;
			index = getShellNumByIndex(indexs, globalIndex[0], globalIndex[1]);

			// 确定该边是第几边
			int size;
			int edge1[2], edge2[2], edgeI[2];

			size = (int)indexs.size();

			if (abs(index[0] - index[1]) > 1)
			{
				if (index[1] == size - 1)
				{
					edge2[0] = index[1] - 1;
					edge2[1] = index[1];
					edgeI[0] = index[1];
					edgeI[1] = index[0];
					edge1[0] = 0; edge1[1] = 1;
					rev = -1;
				}
				else
				{
					edge2[0] = 0; edge2[1] = 1;
					edge1[0] = index[0] - 1; edge1[1] = index[0];
					edgeI[0] = index[0]; edgeI[1] = index[1];
				}
			}
			else
			{
				if (index[0] < index[1])
				{
					edgeI[0] = index[0];
					edgeI[1] = index[1];
					// 如果index1是第一个节点
					if (index[0] == 0)
					{
						edge1[0] = size - 1;
						edge1[1] = 0;
					}
					else
					{
						edge1[0] = index[0] - 1;
						edge1[1] = index[0];
					}

					// 如果index2是最后一个节点
					if (index[1] == size - 1)
					{
						edge2[0] = size - 1;
						edge2[1] = 0;
					}
					else
					{
						edge2[0] = index[1];
						edge2[1] = index[1] + 1;
					}
				}
				else
				{
					edgeI[0] = index[1];
					edgeI[1] = index[0];

					// 如果index1是第一个节点
					if (index[1] == 0)
					{
						edge2[0] = size - 1;
						edge2[1] = 0;
					}
					else
					{
						edge2[0] = index[1] - 1;
						edge2[1] = index[1];
					}

					// 如果index2是最后一个节点
					if (index[0] == size - 1)
					{
						edge1[0] = size - 1;
						edge1[1] = 0;
					}
					else
					{
						edge1[0] = index[0];
						edge1[1] = index[0] + 1;
					}
					rev = -1;
				}
			}



			// 获取坐标
			QVector3D edgeI1 = _nodeVertex.value(indexs[edgeI[0]])->getVertex();
			QVector3D edgeI2 = _nodeVertex.value(indexs[edgeI[1]])->getVertex();
			QVector3D edge1_1 = _nodeVertex.value(indexs[edge1[0]])->getVertex();
			QVector3D edge1_2 = _nodeVertex.value(indexs[edge1[1]])->getVertex();
			QVector3D edge2_1 = _nodeVertex.value(indexs[edge2[0]])->getVertex();
			QVector3D edge2_2 = _nodeVertex.value(indexs[edge2[1]])->getVertex();

			QVector3D u, v, w;
			u = edgeI2 - edgeI1;
			u.normalize();

			v = edge1_2 - edge1_1;
			v.normalize();

			w = QVector3D::crossProduct(u, v);
			QTransform temp(u.x(), v.x(), w.x(), u.y(), v.y(), w.y(), u.z(), v.z(), w.z());
			QTransform m1 = temp.inverted();

			v = edge2_2 - edge2_1;
			v.normalize();
			w = QVector3D::crossProduct(u, v);
			QTransform temp2(u.x(), v.x(), w.x(), u.y(), v.y(), w.y(), u.z(), v.z(), w.z());
			QTransform m2 = temp2.inverted();

			// 获取两个节点内力矢量
			QVector3D vec1 = _nodeVertex.value(globalIndex[0])->getForce(shell->_ID, caseID);
			QVector3D vec2 = _nodeVertex.value(globalIndex[1])->getForce(shell->_ID, caseID);

			QVector3D gpf1 = QVector3D(m1.m11() * vec1.x() + m1.m12() * vec1.y() + m1.m13() * vec1.z(),
									   m1.m21() * vec1.x() + m1.m22() * vec1.y() + m1.m23() * vec1.z(),
									   m1.m31() * vec1.x() + m1.m32() * vec1.y() + m1.m33() * vec1.z());
			QVector3D gpf2 = QVector3D(m2.m11() * vec2.x() + m2.m12() * vec2.y() + m2.m13() * vec2.z(),
									   m2.m21() * vec2.x() + m2.m22() * vec2.y() + m2.m23() * vec2.z(),
									   m2.m31() * vec2.x() + m2.m32() * vec2.y() + m2.m33() * vec2.z());

			axil1 += rev * gpf1.x();
			axil2 -= rev * gpf2.x();
		}

	}

	int * mIF06PostFile::getShellNumByIndex(QVector<int> meshIndex, int globalID1, int globalID2)
	{
		int *res = new int[2];
		set<int> index = {globalID1,globalID2};
		if (meshIndex.size() == 3)
		{
			if (index == set<int>{meshIndex[0], meshIndex[1]})
			{
				res[0] = 0;
				res[1] = 1;
			}
			else if (index == set<int>{meshIndex[1], meshIndex[2]})
			{
				res[0] = 1;
				res[1] = 2;
			}
			else if (index == set<int>{meshIndex[2], meshIndex[0]})
			{
				res[0] = 2;
				res[1] = 0;
			}
		}
		else
		{
			if (index == set<int>{meshIndex[0], meshIndex[1]})
			{
				res[0] = 0;
				res[1] = 1;
			}
			else if (index == set<int>{meshIndex[1], meshIndex[2]})
			{
				res[0] = 1;
				res[1] = 2;
			}
			else if (index == set<int>{meshIndex[2], meshIndex[3]})
			{
				res[0] = 2;
				res[1] = 3;
			}
			else if (index == set<int>{meshIndex[3], meshIndex[0]})
			{
				res[0] = 3;
				res[1] = 0;
			}
		}

		return res;
	}


	QVector<QString> mIF06PostFile::_layeredComposite = QVector<QString>{"Normal-1", "Normal-2", "Shear-12", "Shear XZ-Mat", "Shear YZ-Mat", "Angle", "Major", "Minor", "Max Shear"};

	QVector<QString> mIF06PostFile::_stressBeamNames = QVector<QString>{"SA1", "SA2", "SA3", "SA4", "Axial Stress", "SA-Max", "SA-Min", "SB1", "SB2", "SB3", "SB4", "SB-Max", "SB-Min"};

	QVector<QString> mIF06PostFile::_stressRodNames = QVector<QString>{"Axial Stress", "Torsional Stress"};

	QVector<QString> mIF06PostFile::_stressShearNames = QVector<QString>{"Max Shear", "Avg Shear"};

	QVector<QString> mIF06PostFile::_strainRodNames = QVector<QString>{"Axial Strain", "Torsional Strain"};

	QHash<QString, QString> mIF06PostFile::_varibleName_Name =
		QHash<QString, QString>{{"DISPLACEMENT VECTOR","Displacement"}
								, {"STRESSES IN BAR ELEMENTS","Beam Stress(E)"}, {"STRESSES IN ROD ELEMENTS","Rod Stress(E)"}, {"STRESSES IN SHEAR PANELS","Shear Stress(E)"},{"STRESSES IN LAYERED COMPOSITE ELEMENTS","Shell Stress(E)"}
								,{"STRAINS IN BAR ELEMENTS","Beam Strain(E)"}, {"STRAINS IN ROD ELEMENTS","Rod Strain(E)"}, {"STRAINS IN SHEAR PANELS","Shear Strain(E)"},{"STRAINS IN LAYERED COMPOSITE ELEMENTS","Shell Strain(E)"}
								,{"FORCES IN QUADRILATERAL ELEMENTS","ShellForceResultant"}, {"FORCES ACTING ON SHEAR PANEL ELEMENTS","ShearLoad"},{"FORCES IN BAR ELEMENTS","BarEndLoad"},{"FORCES IN ROD ELEMENTS","BarEndLoad"}
								, {"GRID POINT FORCE BALANCE", "Grid Point Force Balance"}
	};
}


