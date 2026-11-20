#include "mPBeam.h"

namespace MechSectionShapeData
{
	mPBeam::mPBeam()
	{
	}
	mPBeam* mPBeam::_instance = nullptr;
	mPBeam* mPBeam::getInstance()
	{
			if (_instance == nullptr)
			{
				_instance = new mPBeam;
			}
			return _instance;
		
	}

	void mPBeam::setSecType(SecType t)
	{
		_Sectype = t;
	}

	void mPBeam::setName(QString name)
	{
		_name = name;
	}

	QString mPBeam::setDefaultNameForBeam(SecType type)
	{

		switch (type)
		{
		case Sec_L: _name = "L_Section";
			break;
		case Sec_H: _name = "H_Section";
			break;
		case Sec_I:_name = "I_Section";
			break;
		case Sec_T:_name = "T_Section";
			break;
		case Sec_Box:_name = "Box_Section";
			break;
		case Sec_Rec:_name = "Rec_Section";
			break;
		case Sec_Cir:_name = "Cir_Section";
			break;
		case Sec_Ann:_name = "Ann_Section";
			break;
		case Sec_Polygon:_name = "Polygon_Section";
			break;
		case Sec_Channel:_name = "Channel_Section";
			break;
		case Sec_Channel1:_name = "Channel1_Section";
			break;
		case Sec_Channel2:_name = "Channel2_Section";
			break;
		case Sec_I1:_name = "I1_Section";
			break;
		case Sec_T1:_name = "T1_Section";
			break;
		case Sec_T2:_name = "T2_Section";
			break;
		case Sec_Box1:_name = "Box1_Section";
			break;
		case Sec_Cross:_name = "Cross_Section";
			break;
		case Sec_Hat:_name = "Hat_Section";
			break;
		case Sec_Z:_name = "Z_Section";
			break;
		}
		_beamNamelist.push_back(_name);
		int i = 0;
		for (auto it = _beamNamelist.begin(); it != _beamNamelist.end(); it++)
		{
			if (it->toStdString() == _name.toStdString())
			{
				i++;
			}
		}
		auto a = QString::number(i, 10);
		_name = _name + "-" + a;

		return _name;

	}

	
}

