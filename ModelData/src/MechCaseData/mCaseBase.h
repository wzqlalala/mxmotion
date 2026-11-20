#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"

//#include "mLoadBase.h"
//#include "mBoundaryBase.h"
#include "mLoadBoundaryBase.h"
#include <QString>
#include <unordered_map>
#include <QHash>
#include <QStringList>

namespace MechCaseData
{
	class MXCASEDATA_EXPORT mCaseBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mCaseBase(int id ,MxFunctions::CaseType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mCaseBase() {}

		/*
		* 析构函数
		*/
		virtual ~mCaseBase();

		/*
		* 设置工况ID
		*/
		void setCaseId(int id);

		/*
		* 获取工况ID
		*/
		int getCaseId();

		/*
		* 设置工况名
		*/
		void setCaseName(const QString& name);

		/*
		* 获取输出文件类型
		*/
		QString getOutputType();

		/*
		* 设置输出文件类型
		*/
		void setOutputType(const QString& type);

		/*
		* 获取工况名
		*/
		QString getCaseName();

		/*
		* 设置工况类型
		*/
		void setCaseType(MxFunctions::CaseType type);
		/*
		* 获取工况类型
		*/
		MxFunctions::CaseType getCaseType();

		/*
		* 获取工况英文
		*/
		QString caseTypeToStringEn();

		/*
		* 获取工况类型的字符串
		*/
		QString getCaseTypeStr();

		/*
		* 设置工况的描述
		*/
		void setCaseDescribe(const QString& describe);

		/*
		 * 获取工况的描述
		 */
		QString getCaseDescribe();

		/*
		* 获取历史输出启用状态
		*/
		bool getHistoryOutputEnabled() const;

		/*
		* 设置历史输出启用状态
		*/
		void setHistoryOutputEnabled(bool enabled);

		/*
		* 拷贝分析工况
		*/
		virtual mCaseBase* copyCase(const int& id, const QString& name) = 0;

		/*
		* 获取输出控制
		*/
		QHash<QString, QStringList>& getOutputs();

		/*
		* 设置输出控制
		*/
		void setOutputs(QHash<QString, QStringList> outputs);

		/*
		* 获取输出控制选择集合
		*/
		QHash<QString, QHash<QString, QStringList>>& getSelectedSet();

		/*
		* 设置输出控制选择集合
		*/
		void setSelectedSet(QHash<QString, QHash<QString, QStringList>> selectedset);

		/*
		* 获取PCH输出控制
		*/
		QHash<QString, QStringList>& getPchOutputs();

		/*
		* 设置PCH输出控制
		*/
		void setPchOutputs(QHash<QString, QStringList> outputs);

		/*
		* 获取PCH输出控制选择集合
		*/
		QHash<QString, QHash<QString, QStringList>>& getPchSelectedSet();

		/*
		* 设置PCH输出控制选择集合
		*/
		void setPchSelectedSet(QHash<QString, QHash<QString, QStringList>> selectedset);

		/*
		* 获取历史输出控制
		*/
		QHash<QString, QStringList>& getHistoryOutputs();

		/*
		* 设置历史输出控制
		*/
		void setHistoryOutputs(QHash<QString, QStringList> outputs);

		/*
		* 获取动刚度结果选中项
		*/
		QStringList getHistoryKdOutputs();

		/*
		* 设置动刚度结果选中项
		*/
		void setHistoryKdOutputs(QStringList outputs);

		/*
		* 获取求解文件路径
		*/
		QString getSolveDir() { return _solveDir; }

		/*
		* 设置求解文件路径
		*/
		void setSolveDir(const QString& dir) { _solveDir = dir; }

		/*
		* 获取历史输出文件格式
		*/
		QString getHstFileType() const;

		/*
		* 设置历史输出文件格式
		*/
		void setHstFileType(const QString& val);

		/*
		* 获取历史输出数据类型
		*/
		QString getHstDataType() const;

		/*
		* 设置历史输出数据类型
		*/
		void setHstDataType(const QString& val);

		/*
		* 获取历史输出动刚度值
		*/
		QString getHstDynamicStiff() const;

		/*
		* 设置历史输出动刚度值
		*/
		void setHstDynamicStiff(const QString& val);

		friend QDataStream& operator>>(QDataStream& in, mCaseBase& obj);
		friend QDataStream& InStreammCaseBaseV3(QDataStream& in, mCaseBase& obj);
		friend QDataStream& operator<<(QDataStream& out, mCaseBase& obj);

	protected:
		/*
		* 复制基类成员到目标对象（供子类 copyCase 调用）
		* 注意：_id 和 _caseName 由子类通过构造函数传入，此处不覆盖
		*/
		void copyBaseDataTo(mCaseBase* target) const;

	private:

		//工况ID
		int _id;

		//工况名字
		QString _caseName;

		//工况类型
		MxFunctions::CaseType _caseType;

		//求解文件路径
		QString _solveDir;

		//工况描述
		QString _caseDescribe;

		//输出
		QHash<QString, QStringList> _outputs;

		//输出变量选择集合映射
		QHash<QString, QHash<QString, QStringList>> _selectedset;

		//PCH输出
		QHash<QString, QStringList> _pch_outputs;

		//PCH输出变量选择集合映射
		QHash<QString, QHash<QString, QStringList>> _pch_selectedset;

		//历史输出
		QHash<QString, QStringList> _historyOutputs;

		//动刚度结果
		QStringList _historyKdOutputs;

		//历史输出启用状态
		bool _historyOutputEnabled = false;

		//历史输出文件格式
		QString _hstFileType = "csv";

		//历史输出数据类型
		QString _hstDataType = "无";

		//历史输出动刚度值
		QString _hstDynamicStiff;

		//输出文件类型
		QString _outputType = "";
	};
}
