#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include "MPreData_global.h"

//Qt
#include <QString>
#include <QList>
#include <vector>
#include <unordered_map>
#include <set>

#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"
#include "mModelEnum.h"

namespace MechPreData
{
	class MPREDATA_EXPORT mDataStatusManager
	{
	public:
		mDataStatusManager() {}

		void clearAllData();

		void clearNormalInfoData();

		QList<outputInfoData>& updataDynamicInfoData();

		void updateDynamicInfoData(MxFunctions::PhysicalFieldType phyType, int phyID);

		void updateDynamicInfoData(QHash<MxFunctions::PhysicalFieldType,std::vector<int>> phyData);

		void deleteDynamicInfoData(MxFunctions::PhysicalFieldType phyType, std::vector<int> phyIDs);

		void resetName(MxFunctions::PhysicalFieldType phyType,const QString& oldName, const QString& newName, const int& ID);

		void readFileErrorInfo(const QString& filename);

		//类型→模块名规范取值（信息表模块列与单对象刷新的列表/表格删除匹配的
		//唯一口径：全量树重建等外部写行处必须经此取名，禁止手写字面量——
		//曾因"求解方案"vs"方案"不一致导致删除永不命中、告警行永久残留）
		static QString moduleOf(MxFunctions::PhysicalFieldType phyType);

		QList<outputInfoData> getNormalInfoData() { return _normalInfoData; }

		QList<outputInfoData> getDynamicInfoData() { return _dynamicInfoData; }

		void AddListOutputInfoData(QList<outputInfoData>& tabelDynamic, const QVector<QPair<QString, QString>>& info, const QString& module, const QString& phyName, const int& ID);

	private:
		void getDeleteMapRelation(QHash<QString, QSet<int>> deleteInfoData);

		void removeDynamicInfoData(const QString& module, const std::vector<int>& phyIDs);

        void stateInfoFunction(MxFunctions::PhysicalFieldType phyType, QList<outputInfoData>& tabelDynamic, QHash<QString, QSet<int>>& deleteInfoData,
            QHash<int, MxFunctions::BrowserTreeStatus>& phyIDToStatus, std::vector<int>& phyIDs, const QString& phyName, int id);

	private:
		//常态信息
		QList<outputInfoData> _normalInfoData;

		//动态信息
		QList<outputInfoData> _dynamicInfoData;
	};
}