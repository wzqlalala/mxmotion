#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXGlobalParamData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QDataStream>
#include <QHash>
#include <QStringList>

namespace MechGlobalParamData
{
	class MXGLOBALPARAMDATA_EXPORT mGlobalOutputControlData
	{
	public:
		mGlobalOutputControlData();
		~mGlobalOutputControlData();

		// 是否已创建（用于树节点显示）
		bool isCreated() const;
		void setCreated(bool created);

		// 获取/设置输出变量勾选结果
		QHash<QString, QStringList> getOutputSelections() const;
		void setOutputSelections(const QHash<QString, QStringList>& selections);

		friend QDataStream& operator>>(QDataStream& in, mGlobalOutputControlData& obj);
		friend QDataStream& InStreammGlobalOutputControlDataV3(QDataStream& in, mGlobalOutputControlData& obj);
		friend QDataStream& operator<<(QDataStream& out, mGlobalOutputControlData& obj);

	private:
		bool _created = true;
		QHash<QString, QStringList> _outputSelections;
	};
}
