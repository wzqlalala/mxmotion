#pragma once
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"

#include <QString>
#include <QVariant>
#include <QVector>
#include <QSet>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>

namespace MechMainWindow
{
	//enum PropertyType
	//{
	//	CommonProperty,//通用
	//	StructuralProperty,//结构
	//	ThermalProperty,//热
	//	ElecticProperty,//电
	//	ElasticityProperty,//弹性
	//	ConventionalPlasticityProperty,//常规塑性
	//	HyperElasticProperty,//超弹性
	//	RigidBodyProperty,//刚体材料
	//	PiezoelectricProperty,//压电材料
	//	AcousticProperty,//声学材料
	//	OthersProperty,//其他
	//};
	// 单值
	struct MaterialScalar
	{
		double value;
		QString unit;
	};

	//离散值
	struct MaterialSingleCurve
	{
		QVector<double> x;
		QVector<double> y;
	};
	// 曲线集合
	struct MaterialCurve
	{
		QString xUnit;
		QString yUnit;
		QString xName;
		QHash<QString, MaterialSingleCurve> curves;
	};

	//材料属性
	struct MaterialProperty
	{
		QString category;//属性类型
		int valuetype;//属性值类型 0:单值 1:曲线
		QVariant value;  //属性值
	};
	//材料
	struct Material
	{
		int id;
		QString name;
		QString category;//材料类型
		QHash<QString, MaterialProperty> props;
	};
	//过滤字段
	struct MaterialFilter
	{
		QString keyword;   // 目前只按名称过滤
	};
	class MMAINWINDOW_EXPORT mMaterialModel : public QStandardItemModel
	{
		Q_OBJECT

	public:
		/*
		* 构造函数
		*/
		mMaterialModel(QObject* parent = nullptr);
		
		/*
		* 析构函数
		*/
		~mMaterialModel();

		/*
		* 注册材料元数据类型
		*/
		static void registerMeta();

		/*
		* 通过ID获取材料
		*/
		Material getMaterialById(int id);

		/*
		* 获取已有材料类型
		*/
		QSet<QString> getAllMaterialTypes();

		/*
		* 查询材料名称
		*/
		bool findMateiral(const QString& name);

		/*
		* 新增材料
		*/
		bool addMaterial(Material& mat);

		/*
		* 更新材料
		*/
		bool updateMaterial(Material& mat);

		/*
		* 删除材料
		*/
		void removeMaterial(const QSet<int>& idxs);

		/*
		* 复制材料
		*/
		void copyMaterial(const int& id);

		/*
		* 重命名材料
		*/
		void renameMaterial(const QString& oldname, const QString& newname);

	private:
		/*
		* 加载材料
		*/
		void loadMaterials();

		/*
		* 通过文件更新材料
		*/
		void loadMaterialByFile(const int& id, const QString& filepath);

		/*
		* 将csv表格中的材料提取，转存文件
		*/
		void materialsFromCsvToFile(const QString& csvpath);

		/*
		* 提取xml文件中的材料，转存文件
		*/
		void parseMatXML(const QString& xmlFilePath);
		QHash<QString,MaterialProperty> getProperty(
			const bool& issingle,
			const QHash<QString, QPair<QString, QString>>& id2prop,
			const QStringList& proplines);
		void getId2Prop(const QStringList& paramdetails ,QString& unit, QString& propname);

		/*
		* 存储.mat文件
		*/
		bool storeMaterialMat(const Material& mat);

		/*
		* 覆盖写.mat文件
		*/
		bool writeMaterialMat(const Material& mat);

		/*
		* 将属性转为字符串
		*/
		QString propToQString(const MaterialProperty& prop, const QString& propname);

		/*
		* 添加材料进汇总文件
		*/
		bool addMaterialIntoSummary(const Material& mat);

		/*
		* 删除.mat文件
		*/
		void deleteMaterialMat(const Material& mat);

		/*
		* 从汇总文件删除材料
		*/
		void deleteMaterialFromSummary(const Material& mat);

		/*
		* 在汇总文件重命名材料
		*/
		void renameMaterialFromSummary(const QString& oldname, const QString& newname);

		/*
		* 比对文件
		*/
		void compare();

		/*
		* 加载内存模型
		*/
		void loadIntoModel();

	protected:
		//重写树上item形式
		Qt::ItemFlags flags(const QModelIndex& index) const override;
		QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

	private:
		QVector<Material> m_Materials;
		//目前已有材料类型
		QSet<QString> m_MatTypes;
		//目前已有材料名称
		QSet<QString>m_MatNames;
		// category -> 父节点
		QMap<QString, QStandardItem*> m_categoryNodes;
	};


/****************************************************Service********************************************************************/

	class MMAINWINDOW_EXPORT mMaterialService :public QObject
	{
		Q_OBJECT
	public:
		/*
		* 构造函数
		*/
		mMaterialService(QObject* parent = nullptr);

		/*
		* 代理模型
		*/
		QAbstractItemModel* model() const; 

		/*
		* 设置过滤器
		*/
		void setFilter(const MaterialFilter& f);

		/*
		* 通过ID得到材料
		*/
		Material materialById(int id);

		/*
		* 添加材料
		*/
		bool addMaterial(Material& mat);

		/*
		* 更新材料
		*/
		bool updateMaterial(Material& mat);

		/*
		* 删除材料
		*/
		void removeMaterial(const QSet<int>& idxs);

		/*
		* 复制材料
		*/
		void copyMaterial(const int& id);

		/*
		* 重命名材料
		*/
		void renameMaterial(const QString& oldname, const QString& newname);

		/*
		* 获取已有材料类型
		*/
		QSet<QString> getAllMaterialTypes();

		/*
		* 查询材料名称
		*/
		bool findMateiral(const QString& name);

	private:
		mMaterialModel* m_source;     // 数据模型
		QSortFilterProxyModel* m_proxy;    // 过滤代理
	};
}

Q_DECLARE_METATYPE(MechMainWindow::MaterialScalar)
Q_DECLARE_METATYPE(MechMainWindow::MaterialCurve)