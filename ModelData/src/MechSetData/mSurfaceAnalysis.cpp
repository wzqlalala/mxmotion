#include "mSurfaceAnalysis.h"
#pragma execution_character_set("utf-8")

namespace MechSetData
{
	mSurfaceAnalysis::mSurfaceAnalysis(int id, MxFunctions::SetType type, const QString& name,
		bool internal, bool isUserRename)
		: mSetBase(id, type, name, internal, isUserRename)
	{
	}

	mSurfaceAnalysis::~mSurfaceAnalysis() {}

	mSurfaceAnalysis* mSurfaceAnalysis::copySet(const int& id, const QString& name)
	{
		auto* set = new mSurfaceAnalysis(id, getSetType(), name);
		set->_subType = _subType;
		set->_xa = _xa; set->_ya = _ya; set->_za = _za;
		set->_xb = _xb; set->_yb = _yb; set->_zb = _zb;
		set->_r1 = _r1; set->_z1 = _z1;
		set->_r2 = _r2; set->_z2 = _z2;
		set->_ox = _ox; set->_oy = _oy; set->_oz = _oz;
		set->_xx = _xx; set->_xy = _xy; set->_xz = _xz;
		set->_zx = _zx; set->_zy = _zy; set->_zz = _zz;
		set->_sx = _sx; set->_sy = _sy;
		set->_ex = _ex; set->_ey = _ey; set->_el = _el;
		set->_contours = _contours;
		return set;
	}

	MxFunctions::BrowserTreeStatus mSurfaceAnalysis::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		switch (_subType)
		{
		case RevolutionSurface:
			if (_xa == 0 && _ya == 0 && _za == 0 && _xb == 0 && _yb == 0 && _zb == 0)
			{
				info.append({ "警告", "旋转轴坐标未输入" });
			}
			if (_r1 == 0 && _z1 == 0)
			{
				info.append({ "警告", "起点截面参数未输入" });
			}
			if (_r2 == 0 && _z2 == 0)
			{
				info.append({ "警告", "终点截面参数未输入" });
			}
			break;

		case CylinderSurface:
			if (_ox == 0 && _oy == 0 && _oz == 0
				&& _xx == 0 && _xy == 0 && _xz == 0
				&& _zx == 0 && _zy == 0 && _zz == 0)
			{
				info.append({ "警告", "局部坐标系参数未输入" });
			}
			if (_sx == 0 && _sy == 0)
			{
				info.append({ "警告", "拉伸起点参数未输入" });
			}
			if (_ex == 0 && _ey == 0 && _el == 0)
			{
				info.append({ "警告", "拉伸终点参数未输入" });
			}
			break;

		case SegmentSurface:
			if (_ox == 0 && _oy == 0 && _oz == 0
				&& _xx == 0 && _xy == 0 && _xz == 0
				&& _zx == 0 && _zy == 0 && _zz == 0)
			{
				info.append({ "警告", "局部坐标系参数未输入" });
			}
			if (_sx == 0 && _sy == 0)
			{
				info.append({ "警告", "轮廓起点参数未输入" });
			}
			if (_contours.isEmpty())
			{
				info.append({ "警告", "解析线段轮廓未填写" });
			}
			break;

		default:
			info.append({ "警告", "未设置解析表面子类型" });
			break;
		}

		return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
	}

	QString mSurfaceAnalysis::surfaceSubTypeName() const
	{
		switch (_subType)
		{
		case RevolutionSurface: return "旋转解析平面";
		case CylinderSurface:   return "拉伸解析平面";
		case SegmentSurface:    return "解析线段";
		}
		return "旋转解析平面";
	}

	void mSurfaceAnalysis::setSurfaceSubTypeByName(const QString& name)
	{
		if (name == "旋转解析平面")
			_subType = RevolutionSurface;
		else if (name == "拉伸解析平面")
			_subType = CylinderSurface;
		else if (name == "解析线段")
			_subType = SegmentSurface;

		return;
	}
}
