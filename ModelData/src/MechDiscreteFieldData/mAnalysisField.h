#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXDiscreteFieldData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mModelEnum.h"
#include "mBasicEnum.h"
#include "mFieldBase.h"

#include <map>

//Qt
#include <QString>
#include <QStringList>
#include <QList>

namespace MechDiscreteFieldData
{
	/*
	* 分析场数据行结构：
	* 离散（节点/单元）模式使用 id；
	* 离散（单元面）模式使用 elementId + faceId；
	* 点云模式使用 x/y/z；
	* values 为分量值，长度恒等于分量数量。
	*/
	struct AnalysisFieldRow
	{
		QString id;					//节点/单元编号（离散模式）
		QString elementId, faceId;	//单元编号/面编号（单元面离散模式）
		QString x, y, z;			//点云坐标
		QStringList values;			//分量值，长度恒等于分量数量
	};

	class MXDISCRETEFIELDDATA_EXPORT mAnalysisField : public mFieldBase
	{
	public:
		/*
		* 定义方式
		*/
		enum class DefineMethod { Inline, TextFile };

		/*
		* 数据类型
		*/
		enum class DataType { PointCloud, ObjectDiscrete };

		/*
		* 缩放方式
		*/
		enum class ScaleMode { None, Uniform, NonUniform };

		/*
		* 容差类型
		*/
		enum class ToleranceType { Relative, Absolute };

		/*
		* 目标面映射算法
		*/
		enum class MappingAlgorithm { Surface, Volumetric };

		/*
		* 含参构造函数
		*/
		mAnalysisField(int id, const QString& name = "");

		/*
		* 默认构造函数（供反序列化使用）
		*/
		mAnalysisField() {};

		/*
		* 析构函数
		*/
		~mAnalysisField();

		/*
		* 场子类型：分析场
		*/
		MxFunctions::FieldType getFieldType() const override { return MxFunctions::FieldType::Analysis; }

		/*
		* 拷贝分析场数据
		*/
		mAnalysisField* copyAnalysisField();

		/*
		* 拷贝场数据（基类接口）
		*/
		mFieldBase* copyField() override { return copyAnalysisField(); }

		/*
		* 分析场名称
		*/
		void setAnalysisFieldName(const QString& name);
		QString getAnalysisFieldName() const;

		/*
		* 位置（节点/单元/单元面）
		*/
		void setLocate(MxFunctions::PickDataType locate);
		MxFunctions::PickDataType getLocate();

		/*
		* 分量数量
		*/
		void setComponentCount(int count);
		int getComponentCount() const;

		/*
		* 定义方式
		*/
		void setDefineMethod(DefineMethod method);
		DefineMethod getDefineMethod() const;

		/*
		* 数据类型
		*/
		void setDataType(DataType type);
		DataType getDataType() const;

		/*
		* 文本文档参数
		*/
		void setTextFilePath(const QString& path);
		QString getTextFilePath() const;

		void setTextStartRow(int row);
		int getTextStartRow() const;

		void setTextEndRow(int row);
		int getTextEndRow() const;

		void setTextStartColumn(int column);
		int getTextStartColumn() const;

		/*
		* 数据行
		*/
		void setRows(const QList<AnalysisFieldRow>& rows);
		//移动语义重载：大数据量下避免 N 行 × 分量 的全量深拷贝
		void setRows(QList<AnalysisFieldRow>&& rows);
		QList<AnalysisFieldRow> getRows() const;

		/*
		* 行数据可变存储（供表格模型等视图直接绑定，避免多份拷贝）
		*/
		QList<AnalysisFieldRow>& rowStorage() { return _rows; }

		/*
		* 按当前分量数量规整各行 values 长度（不足补空串、超出截断）
		*/
		void normalizeRowValues();

		/*
		* 默认场值（仅点云）
		*/
		void setDefaultValue(const QString& value);
		QString getDefaultValue() const;

		/*
		* 映射空间定义（仅点云）。
		* 坐标系与目标集合不存于本数据类，由 mMapRelationManager 维护映射关系
		* （Physical_Field -> Physical_Coordinate / Physical_Set）。
		*/
		void setScaleMode(ScaleMode mode);
		ScaleMode getScaleMode() const;

		void setUniformScale(double scale);
		double getUniformScale() const;

		void setScaleX(double scale);
		double getScaleX() const;

		void setScaleY(double scale);
		double getScaleY() const;

		void setScaleZ(double scale);
		double getScaleZ() const;

		/*
		* 映射控制（仅点云）
		*/
		void setToleranceType(ToleranceType type);
		ToleranceType getToleranceType() const;

		void setPositiveTolerance(double tolerance);
		double getPositiveTolerance() const;

		void setNegativeTolerance(double tolerance);
		double getNegativeTolerance() const;

		void setNeighborhoodTolerance(double tolerance);
		double getNeighborhoodTolerance() const;

		void setInterpolationTolerance(double tolerance);
		double getInterpolationTolerance() const;

		void setMappingAlgorithm(MappingAlgorithm algorithm);
		MappingAlgorithm getMappingAlgorithm() const;

		/*
		* 判断节点状态
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mAnalysisField& mgr);
		friend QDataStream& operator<<(QDataStream& out, mAnalysisField& mgr);

	public:
		//位置 节点/单元/单元面（PickDataType::Node/Element/ElementFace），默认节点
		MxFunctions::PickDataType _locate = MxFunctions::PickDataType::Node;

		//分量数量
		int _componentCount = 1;

		//定义方式
		DefineMethod _method = DefineMethod::Inline;

		//数据类型
		DataType _dataType = DataType::PointCloud;

		//文本文档参数
		QString _textFilePath;
		int _textStartRow = 1;
		int _textEndRow = 1;
		int _textStartColumn = 1;

		//数据行
		QList<AnalysisFieldRow> _rows;

		//默认场值（仅点云）
		QString _defaultValue;

		//映射空间（仅点云）。坐标系/目标集合不在此存储，见 mMapRelationManager
		ScaleMode _scaleMode = ScaleMode::None;
		double _uniformScale = 1.0;
		double _scaleX = 1.0;
		double _scaleY = 1.0;
		double _scaleZ = 1.0;

		//映射控制（仅点云）
		ToleranceType _toleranceType = ToleranceType::Relative;
		double _positiveTolerance = 0.5;
		double _negativeTolerance = 0.5;
		double _neighborhoodTolerance = 0.1;
		double _interpolationTolerance = 0.5;
		MappingAlgorithm _mappingAlgorithm = MappingAlgorithm::Surface;
	};
}
