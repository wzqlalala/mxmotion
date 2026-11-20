#include "mCaseBase.h"

using namespace MxFunctions;
namespace MechCaseData
{

	mCaseBase::mCaseBase(int id,CaseType type, const QString& name)
		:  _caseType(type), _caseName(name), _id(id)
	{

		if (name.isEmpty())
		{
			_caseName = QString("%1_%2").arg(getCaseTypeStr()).arg(_id);
		}
	}

	mCaseBase::~mCaseBase()
	{
	}

	void mCaseBase::setCaseId(int id)
	{
		_id = id;
	}

	int mCaseBase::getCaseId()
	{
		return _id;
	}

	QString mCaseBase::getOutputType() {
		return _outputType;
	}

	void mCaseBase::setOutputType(const QString& type) {
		_outputType = type;
	}

	void mCaseBase::setCaseName(const QString& name)
	{
		_caseName = name;
	}

	QString mCaseBase::getCaseName()
	{
		return _caseName;
	}



	void mCaseBase::setCaseType(CaseType type)
	{
		_caseType = type;
	}

	CaseType mCaseBase::getCaseType()
	{
		return _caseType;
	}

	QString mCaseBase::caseTypeToStringEn()
	{
		switch (_caseType)
		{
		case MxFunctions::ImplicitLinearStatic:
			return QString("LinearStatic");
		case MxFunctions::ImplicitNonLinearStatic:
			return QString("NonlinStatic");
		case MxFunctions::ImplicitModal:
			return QString("Modal");
		case MxFunctions::ImplicitLinearBuckling:
			return QString("LinearBuckling");
		case MxFunctions::ImplicitPostBuckling:
			return QString("PostBuckling");
		case MxFunctions::ImplicitTransient:
			return QString("LinearTransi");
		case MxFunctions::ImplicitFrequency:
			return QString("FreqResp");
		case MxFunctions::ASCoupling:
			return QString("AcousticStructural");
		case MxFunctions::ImplicitGeneric:
			return QString("Superelement");
		case MxFunctions::ImplicitRandomVibration:
			return QString("RandomVib");
		case MxFunctions::ImplicitResponseSpectrum:
			return QString("Spectrum");
		case MxFunctions::ImplicitNonLinearDynamic:
			return QString("NonlinTransi");
		case MxFunctions::ImplicitTransientHeat:
			return QString("TransientHeat");
		case MxFunctions::ImplicitSteadyHeat:
			return QString("SteadyHeat");
		case MxFunctions::ImplicitStructuralCouplingHeat:
			return QString("CoupledThermalStruct");
		default:
			return QString("");
		}
	}

	QString mCaseBase::getCaseTypeStr()
	{
		switch (_caseType)
		{
		case MxFunctions::ImplicitLinearStatic:
			return QString("线性静力学分析");
		case MxFunctions::ImplicitNonLinearStatic:
			return QString("非线性静力学分析");
		case MxFunctions::ImplicitModal:
			return QString("模态分析");
		case MxFunctions::ImplicitLinearBuckling:
			return QString("线性屈曲分析");
		case MxFunctions::ImplicitPostBuckling:
			return QString("后屈曲分析");
		case MxFunctions::ImplicitTransient:
			return QString("线性瞬态分析");
		case MxFunctions::ImplicitFrequency:
			return QString("频响分析");
		case MxFunctions::ASCoupling:
			return QString("声固耦合分析");
		case MxFunctions::ImplicitGeneric:
			return QString("超单元分析");
		case MxFunctions::ImplicitRandomVibration:
			return QString("随机振动分析");
		case MxFunctions::ImplicitResponseSpectrum:
			return QString("响应谱分析");
		case MxFunctions::ImplicitNonLinearDynamic:
			return QString("非线性瞬态分析");
		case MxFunctions::ImplicitTransientHeat:
			return QString("瞬态传热分析");
		case MxFunctions::ImplicitSteadyHeat:
			return QString("稳态传热分析");
		case MxFunctions::ImplicitStructuralCouplingHeat:
			return QString("结构耦合传热分析");
		default:
			return QString("");
		}
	}
	void mCaseBase::setCaseDescribe(const QString & describe)
	{
		_caseDescribe = describe;
	}

	QString mCaseBase::getCaseDescribe()
	{
		return _caseDescribe;
	}

	QHash<QString, QStringList>& mCaseBase::getOutputs()
	{
		return _outputs;
	}

	void mCaseBase::setOutputs(QHash<QString, QStringList> outputs)
	{
		_outputs = outputs;
	}

	QHash<QString, QStringList>& mCaseBase::getPchOutputs()
	{
		return _pch_outputs;
	}

	void mCaseBase::setPchOutputs(QHash<QString, QStringList> outputs)
	{
		_pch_outputs = outputs;
	}

	QHash<QString, QStringList>& mCaseBase::getHistoryOutputs()
	{
		return _historyOutputs;
	}

	QHash<QString, QHash<QString, QStringList>>& mCaseBase::getSelectedSet() {
		return _selectedset;
	}

	void mCaseBase::setSelectedSet(QHash<QString, QHash<QString, QStringList>> selectedset) {
		_selectedset = selectedset;
	}

	QHash<QString, QHash<QString, QStringList>>& mCaseBase::getPchSelectedSet() {
		return _pch_selectedset;
	}

	void mCaseBase::setPchSelectedSet(QHash<QString, QHash<QString, QStringList>> selectedset) {
		_pch_selectedset = selectedset;
	}

	void mCaseBase::setHistoryOutputs(QHash<QString, QStringList> outputs)
	{
		_historyOutputs = outputs;
	}

	QStringList mCaseBase::getHistoryKdOutputs()
	{
		return _historyKdOutputs;
	}

	void mCaseBase::setHistoryKdOutputs(QStringList outputs)
	{
		_historyKdOutputs = outputs;
	}

	bool mCaseBase::getHistoryOutputEnabled() const
	{
		return _historyOutputEnabled;
	}

	void mCaseBase::setHistoryOutputEnabled(bool enabled)
	{
		_historyOutputEnabled = enabled;
	}

	QString mCaseBase::getHstFileType() const
	{
		return _hstFileType;
	}

	void mCaseBase::setHstFileType(const QString& val)
	{
		_hstFileType = val;
	}

	QString mCaseBase::getHstDataType() const
	{
		return _hstDataType;
	}

	void mCaseBase::setHstDataType(const QString& val)
	{
		_hstDataType = val;
	}

	QString mCaseBase::getHstDynamicStiff() const
	{
		return _hstDynamicStiff;
	}

	void mCaseBase::setHstDynamicStiff(const QString& val)
	{
		_hstDynamicStiff = val;
	}

	void mCaseBase::copyBaseDataTo(mCaseBase* target) const
	{
		target->_caseType = _caseType;
		target->_caseDescribe = _caseDescribe;
		target->_solveDir = _solveDir;
		target->_outputs = _outputs;
		target->_historyOutputs = _historyOutputs;
		target->_historyKdOutputs = _historyKdOutputs;
		target->_historyOutputEnabled = _historyOutputEnabled;
		target->_hstFileType = _hstFileType;
		target->_hstDataType = _hstDataType;
		target->_hstDynamicStiff = _hstDynamicStiff;
		target->_outputType = _outputType;
		target->_selectedset = _selectedset;
	}

}
