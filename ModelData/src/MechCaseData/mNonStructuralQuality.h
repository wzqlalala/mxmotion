#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>


#include <map>
#include<QStringList>
#include "mModelEnum.h"

namespace MechCaseData
{
	class MXCASEDATA_EXPORT mNonStructuralQuality
	{
	public:
		/*
		* 重载构造函数
		*/
		mNonStructuralQuality(int id, const QString& name = "");

		//添加可传 一对多、一对一类别构造参数的构造函数
		mNonStructuralQuality(int id, MxFunctions::Non type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mNonStructuralQuality() {}

		/*
		* 析构函数
		*/
		~mNonStructuralQuality();

		/*
		* 设置非结构质量ID
		*/
		void setNsmId(int id);

		/*
		* 获取非结构质量ID
		*/
		int getNonStructuralQualityId();

		/*
		* 设置非结构质量名
		*/
		void setNsmName(const QString& name);

		/*
		* 获取非结构质量名称
		*/
		QString getNonStructuralQualityName();

		/*
		* 获取非结构质量初始化名称
		*/
		QString getNonStructuralQualityStr();

		/*
		* 拷贝非结构质量属性
		*/
		mNonStructuralQuality* copyNonStructuralQuality(const int & id,const QString& name);



		/*
		* 设置Union值
		*/
		void setUnionValue(const QString& value);
		/*
		* 获取Union值
		*/
		QString getUnionValue() const;

		/*
		* 设置截面属性数据
		*/
		void setCrossPropertyData(const std::map<QString, QString>& data);
		/*
		* 获取截面属性数据
		*/
		std::map<QString, QString>& getCrossPropertyData();
		const std::map<QString, QString>& getCrossPropertyData() const;

		/*
		* 设置单元数据
		*/
		void setElementData(const std::map<QString, QString>& data);
		/*
		* 获取单元数据
		*/
		std::map<QString, QString>& getElementData();
		const std::map<QString, QString>& getElementData() const;

		/*
		* 设置单元集数据
		*/
		void setElementSetData(const std::map<QString, QString>& data);
		/*
		* 获取单元集数据
		*/
		std::map<QString, QString>& getElementSetData();
		const std::map<QString, QString>& getElementSetData() const;

		MxFunctions::Non getTypeIndex();
		void setTypeIndex(MxFunctions::Non typeIndex);

		MxFunctions::PhysicalFieldType getPropIndex();
		void setPropIndex(MxFunctions::PhysicalFieldType propIndex);

		/*
		* 判读数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info);

		friend QDataStream& operator>>(QDataStream& in, mNonStructuralQuality& mgr);
		friend QDataStream& InStreammNonStructuralQualityV3(QDataStream& in, mNonStructuralQuality& obj);
		friend QDataStream& operator<<(QDataStream& out, mNonStructuralQuality& mgr);

	private:
		
		//非结构质量ID
		int _id;

		//非结构质量名字
		QString _nsmName;


		MxFunctions::Non _typeIndex = MxFunctions::Non::Union;
		MxFunctions::PhysicalFieldType _propIndex = MxFunctions::PhysicalFieldType::Physical_Property;

		QString _unionValue;

		std::map<QString, QString> _crossPropertyData;
		std::map<QString, QString> _elementData;
		std::map<QString, QString> _elementSetData;
	};
}
