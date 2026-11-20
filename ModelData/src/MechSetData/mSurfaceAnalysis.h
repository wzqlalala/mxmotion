#pragma once
#pragma execution_character_set("utf-8")

#include "MXSetData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mSetBase.h"
#include "mModelStruct.h"

namespace MechSetData
{
	// 解析表面子类型
	enum SurfaceSubType
	{
		RevolutionSurface = 0,  // 旋转解析平面
		CylinderSurface = 1,    // 拉伸解析平面
		SegmentSurface = 2      // 解析线段
	};

	// 线段轮廓行：line（终点x1,y1）或 circl（圆弧终点x1,y1 + 圆心x2,y2）
	struct SegmentContourRow
	{
		QString rowType;   // "line" 或 "circl"
		double x1 = 0, y1 = 0;  // 终点坐标
		double x2 = 0, y2 = 0;  // 圆心坐标（circl有效，line时无效）
	};

	class MXSETDATA_EXPORT mSurfaceAnalysis : public mSetBase
	{
	public:
		mSurfaceAnalysis(int id, MxFunctions::SetType type, const QString& name = "",
			bool internal = false, bool isUserRename = false);
		mSurfaceAnalysis() {}
		~mSurfaceAnalysis();

		mSurfaceAnalysis* copySet(const int& id, const QString& name) override;
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info) override;

		// 子类型
		SurfaceSubType surfaceSubType() const { return _subType; }
		void setSurfaceSubType(SurfaceSubType t) { _subType = t; }
		QString surfaceSubTypeName() const;  // 返回"旋转解析平面"等
		void setSurfaceSubTypeByName(const QString& name);

		// ===== 旋转解析平面参数 =====
		double xa() const { return _xa; }
		double ya() const { return _ya; }
		double za() const { return _za; }
		void setXa(double v) { _xa = v; }
		void setYa(double v) { _ya = v; }
		void setZa(double v) { _za = v; }

		double xb() const { return _xb; }
		double yb() const { return _yb; }
		double zb() const { return _zb; }
		void setXb(double v) { _xb = v; }
		void setYb(double v) { _yb = v; }
		void setZb(double v) { _zb = v; }

		double r1() const { return _r1; }
		double z1() const { return _z1; }
		void setR1(double v) { _r1 = v; }
		void setZ1(double v) { _z1 = v; }

		double r2() const { return _r2; }
		double z2() const { return _z2; }
		void setR2(double v) { _r2 = v; }
		void setZ2(double v) { _z2 = v; }

		// ===== 拉伸解析平面 / 解析线段 共用局部坐标系 =====
		double ox() const { return _ox; }
		double oy() const { return _oy; }
		double oz() const { return _oz; }
		void setOx(double v) { _ox = v; }
		void setOy(double v) { _oy = v; }
		void setOz(double v) { _oz = v; }

		double xx() const { return _xx; }
		double xy() const { return _xy; }
		double xz() const { return _xz; }
		void setXx(double v) { _xx = v; }
		void setXy(double v) { _xy = v; }
		void setXz(double v) { _xz = v; }

		double zx() const { return _zx; }
		double zy() const { return _zy; }
		double zz() const { return _zz; }
		void setZx(double v) { _zx = v; }
		void setZy(double v) { _zy = v; }
		void setZz(double v) { _zz = v; }

		double sx() const { return _sx; }
		double sy() const { return _sy; }
		void setSx(double v) { _sx = v; }
		void setSy(double v) { _sy = v; }

		// ===== 拉伸解析平面独有：终点 =====
		double ex() const { return _ex; }
		double ey() const { return _ey; }
		double el() const { return _el; }
		void setEx(double v) { _ex = v; }
		void setEy(double v) { _ey = v; }
		void setEl(double v) { _el = v; }

		// ===== 解析线段独有：轮廓 =====
		const QVector<SegmentContourRow>& contours() const { return _contours; }
		void setContours(const QVector<SegmentContourRow>& rows) { _contours = rows; }
		void appendContour(const SegmentContourRow& row) { _contours.append(row); }
		void clearContours() { _contours.clear(); }
		int contourCount() const { return _contours.size(); }

		friend QDataStream& operator>>(QDataStream& in, mSurfaceAnalysis& obj);
		friend QDataStream& InStreamSurfaceAnalysisV3(QDataStream& in, mSurfaceAnalysis& obj);
		friend QDataStream& operator<<(QDataStream& out, mSurfaceAnalysis& obj);

	private:
		SurfaceSubType _subType = RevolutionSurface;

		// 旋转解析平面
		double _xa = 0, _ya = 0, _za = 0;
		double _xb = 0, _yb = 0, _zb = 0;
		double _r1 = 0, _z1 = 0;
		double _r2 = 0, _z2 = 0;

		// 拉伸 / 线段 共用局部坐标系
		double _ox = 0, _oy = 0, _oz = 0;
		double _xx = 0, _xy = 0, _xz = 0;
		double _zx = 0, _zy = 0, _zz = 0;
		double _sx = 0, _sy = 0;

		// 拉伸独有
		double _ex = 0, _ey = 0, _el = 0;

		// 线段独有
		QVector<SegmentContourRow> _contours;
	};
}
