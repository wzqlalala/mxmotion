#include "mCoordinateBase.h"

#include <QMatrix4x4>
#include <cmath>

using namespace MxFunctions;
namespace MechCoordinateData
{

	mCoordinateBase::mCoordinateBase(int id, CoordinateType type, const QString& name)
		: _coordinateType(type), _coordinateName(name), _id(id)
	{
		if (name.isEmpty())
		{
			_coordinateName = QString("坐标系_%1").arg(_id);
		}

		_method = "基准轴";
		_origin = QVector3D(0, 0, 0);
		_mainIndex = 0;
		_dirIndex = 0;
		_xPoint = QVector3D(1, 0, 0);
		_xyPoint = QVector3D(0, 1, 0);
		_xLine = QVector3D(1, 0, 0);
		_yLine = QVector3D(0, 1, 0);
		_mainBase = 0;
		_dirBase = 0;
		_mainReverse = 1;
		_dirReverse = 1;
		_xReverse = 1;
	}

	mCoordinateBase::~mCoordinateBase()
	{

	}

	void mCoordinateBase::setCoordinateId(int id)
	{
		_id = id;
	}

	int mCoordinateBase::getCoordinateId()
	{
		return _id;
	}

	void mCoordinateBase::setCoordinateName(const QString& name)
	{
		_coordinateName = name;
	}

	QString mCoordinateBase::getCoordinateName()
	{
		return _coordinateName;
	}

	void mCoordinateBase::setCoordinateType(CoordinateType type)
	{
		_coordinateType = type;
	}

	CoordinateType mCoordinateBase::getCoordinateType()
	{
		return _coordinateType;
	}

	QString mCoordinateBase::getCoordinateTypeStr()
	{
		switch (_coordinateType)
		{
		case Cartesian:
			return QString("直角坐标系");
		case Cylindrical:
			return QString("圆柱坐标系");
		case Spherical:
			return QString("球坐标系");
		case DiscreteCoordinate:
			return QString("离散坐标系");
		default:
			return QString();
		}
	}
	void mCoordinateBase::setMethod(const QString& method)
	{
		_method = method;
	}
	QString mCoordinateBase::getMethod() const
	{
		return _method;
	}
	void mCoordinateBase::setOrigin(const QVector3D& origin)
	{
		_origin = origin;
	}
	QVector3D mCoordinateBase::getOrigin() const
	{
		return _origin;
	}
	void mCoordinateBase::setMainIndex(int index)
	{
		_mainIndex = index;
		//全部初始化为正向
		_mainReverse = 1;
		_dirReverse = 1;
	}
	int mCoordinateBase::getMainIndex() const
	{
		return _mainIndex;
	}
	void mCoordinateBase::setDirIndex(int index)
	{
		_dirIndex = index;
		//全部初始化为正向
		_mainReverse = 1;
		_dirReverse = 1;
	}
	int mCoordinateBase::getDirIndex() const
	{
		return _dirIndex;
	}
	void mCoordinateBase::setXPoint(const QVector3D& point)
	{
		_xPoint = point;
	}
	QVector3D mCoordinateBase::getXPoint() const
	{
		return _xPoint;
	}
	void mCoordinateBase::setXYPoint(const QVector3D& point)
	{
		_xyPoint = point;
	}
	QVector3D mCoordinateBase::getXYPoint() const
	{
		return _xyPoint;
	}
	void mCoordinateBase::setXLine(const QVector3D& line)
	{
		_xLine = line;
	}
	QVector3D mCoordinateBase::getXLine() const
	{
		return _xLine;
	}
	void mCoordinateBase::setYLine(const QVector3D& line)
	{
		_yLine = line;
	}
	QVector3D mCoordinateBase::getYLine() const
	{
		return _yLine;
	}
	void mCoordinateBase::setMainBase(int base)
	{
		_mainBase = base;
		if (base == 1)
		{
			_xLine = QVector3D(1, 0, 0);
		}
		else if (base == 2)
		{
			_xLine = QVector3D(0, 1, 0);
		}
		else if (base == 3)
		{
			_xLine = QVector3D(0, 0, 1);
		}
		//全部初始化为正向
		_mainReverse = 1;
		_dirReverse = 1;
	}
	int mCoordinateBase::getMainBase() const
	{
		return _mainBase;
	}
	void mCoordinateBase::setDirBase(int base)
	{
		_dirBase = base;
		if (base == 1)
		{
			_yLine = QVector3D(1, 0, 0);
		}
		else if (base == 2)
		{
			_yLine = QVector3D(0, 1, 0);
		}
		else if (base == 3)
		{
			_yLine = QVector3D(0, 0, 1);
		}
		//全部初始化为正向
		_mainReverse = 1;
		_dirReverse = 1;
	}
	int mCoordinateBase::getDirBase() const
	{
		return _dirBase;
	}
	void mCoordinateBase::setMainReverse()
	{
		_mainReverse *= -1;
	}
	void mCoordinateBase::setDirReverse()
	{
		_dirReverse *= -1;
	}
	void mCoordinateBase::setXPointReverse()
	{
		_xReverse *= -1;
	}
	TransformStep mCoordinateBase::getTransformStep(const QString& type, const QString& value)
	{
		MechCoordinateData::TransformStep step;
		if (type.startsWith("偏移"))
		{
			step.type = MechCoordinateData::TransformType::Translation;
			if (type.contains("X"))
			{
				step.value = QVector3D(value.toDouble(), 0, 0);
			}
			else if (type.contains("Y"))
			{
				step.value = QVector3D(0, value.toDouble(), 0);
			}
			else if (type.contains("Z"))
			{
				step.value = QVector3D(0, 0, value.toDouble());
			}
		}
		else if (type.startsWith("旋转"))
		{
			step.type = MechCoordinateData::TransformType::Rotation;
			if (type.contains("X"))
			{
				step.value = QVector3D(value.toDouble(), 0, 0);
			}
			else if (type.contains("Y"))
			{
				step.value = QVector3D(0, value.toDouble(), 0);
			}
			else if (type.contains("Z"))
			{
				step.value = QVector3D(0, 0, value.toDouble());
			}
		}
		else
		{
			step.type = MechCoordinateData::TransformType::Flip;
			QVector<int> flipMap = { 1, 2, 3 }; // index 0 -> X, 1 -> Y, 2 -> Z
			int srcIdx = 0;
			if (type.contains("X"))
				srcIdx = 0;
			else if (type.contains("Y"))
				srcIdx = 1;
			else if (type.contains("Z"))
				srcIdx = 2;
			auto codeFor = [](const QString& s) -> int {
				if (s == "+X") return 1;
				if (s == "-X") return -1;
				if (s == "+Y") return 2;
				if (s == "-Y") return -2;
				if (s == "+Z") return 3;
				if (s == "-Z") return -3;
				return 0;
				};

			int code = codeFor(value);
			if (code == 0) {
				// fallback: keep identity
			}
			else {
				// 只修改被选中的源轴的映射，其它轴保持原样
				flipMap[srcIdx] = code;
			}
			step.flipMap = flipMap;
		}
		return step;
	}
	void mCoordinateBase::getTransformStep(int idx, QString& type, QString& value)
	{
		const auto& step = this->getTransformQueue()[idx];

		switch (step.type)
		{
		case MechCoordinateData::TransformType::Translation:
			// 判断哪个轴有非零值
			if (step.value.x() != 0.0f) {
				type = "偏移X";
				value = QString::number(step.value.x());
			}
			else if (step.value.y() != 0.0f) {
				type = "偏移Y";
				value = QString::number(step.value.y());
			}
			else if (step.value.z() != 0.0f) {
				type = "偏移Z";
				value = QString::number(step.value.z());
			}
			break;

		case MechCoordinateData::TransformType::Rotation:
			if (step.value.x() != 0.0f) {
				type = "旋转X";
				value = QString::number(step.value.x());
			}
			else if (step.value.y() != 0.0f) {
				type = "旋转Y";
				value = QString::number(step.value.y());
			}
			else if (step.value.z() != 0.0f) {
				type = "旋转Z";
				value = QString::number(step.value.z());
			}
			break;

		case MechCoordinateData::TransformType::Flip:
		{
			// 找出被翻转的轴
			// 原本是 [1, 2, 3]，修改后可能是 [1, -2, 3] 表示 Y 轴被翻转为 -Y
			QStringList axes = { "X", "Y", "Z" };

			for (int axis = 0; axis < 3; ++axis)
			{
				int code = step.flipMap[axis];
				// 计算对应轴的类型
				QString axisType = QString("翻转%1").arg(axes[axis]);

				// 计算当前轴的新方向，例如“+X” “-Z”
				QString dir;
				switch (code)
				{
				case 1: dir = "+X"; break;
				case -1: dir = "-X"; break;
				case 2: dir = "+Y"; break;
				case -2: dir = "-Y"; break;
				case 3: dir = "+Z"; break;
				case -3: dir = "-Z"; break;
				default: dir = "?"; break;
				}

				// 只记录发生变化的轴（即与原始 [1,2,3] 不同的项）
				if (code != axis + 1)
				{
					type = axisType;
					value = dir;
					break;
				}
			}
			break;
		}
		}
	}
	void mCoordinateBase::replaceTransformStep(int idx, const QString& value)
	{
		if (idx < 0 || idx >= _transformQueue.size())
			return;

		auto& step = _transformQueue[idx];

		switch (step.type)
		{
		case MechCoordinateData::TransformType::Translation:
			// 根据已有的轴方向更新偏移量
			if (step.value.x() != 0)
				step.value = QVector3D(value.toDouble(), 0, 0);
			else if (step.value.y() != 0)
				step.value = QVector3D(0, value.toDouble(), 0);
			else if (step.value.z() != 0)
				step.value = QVector3D(0, 0, value.toDouble());
			else
				step.value = QVector3D(value.toDouble(), 0, 0);
			break;

		case MechCoordinateData::TransformType::Rotation:
			// 根据已有的旋转轴更新角度
			if (step.value.x() != 0)
				step.value = QVector3D(value.toDouble(), 0, 0);
			else if (step.value.y() != 0)
				step.value = QVector3D(0, value.toDouble(), 0);
			else if (step.value.z() != 0)
				step.value = QVector3D(0, 0, value.toDouble());
			else
				step.value = QVector3D(value.toDouble(), 0, 0);
			break;

		case MechCoordinateData::TransformType::Flip:
		{
			// 处理翻转映射
			auto codeFor = [](const QString& s) -> int {
				if (s == "+X") return 1;
				if (s == "-X") return -1;
				if (s == "+Y") return 2;
				if (s == "-Y") return -2;
				if (s == "+Z") return 3;
				if (s == "-Z") return -3;
				return 0;
				};

			int code = codeFor(value);
			if (code != 0)
			{
				// 找出当前 flipMap 中被操作的轴
				for (int i = 0; i < step.flipMap.size(); ++i)
				{
					int oldVal = step.flipMap[i];
					if (abs(oldVal) == 1 || abs(oldVal) == 2 || abs(oldVal) == 3)
					{
						// 用 code 替换当前这一个
						step.flipMap[i] = code;
						break;
					}
				}
			}
			break;
		}
		default:
			break;
		}
	}
	void mCoordinateBase::addTransformStep(const QString& type, const QString& value)
	{
		addTransformStep(getTransformStep(type, value));
	}
	QVector<QVector3D> mCoordinateBase::getXYZAxis(bool& success, QString& msg, QVector3D& origin) const
	{
		QVector3D x(1, 0, 0), y(0, 1, 0), z(0, 0, 1);
		int index = 0;
		QVector3D origin_local = _origin;
		if (_method == "三点")
		{
			if (_mainIndex == 0)//X
			{
				x = _xPoint - _origin;
				x *= _xReverse;
				if (_dirIndex == 0)//Y
				{
					y = _xyPoint - _origin;
					index = 2;
				}
				else//Z
				{
					z = _xyPoint - _origin;
					index = 1;
				}
			}
			else if (_mainIndex == 1)//Y
			{
				y = _xPoint - _origin;
				y *= _xReverse;
				if (_dirIndex == 0)//X
				{
					x = _xyPoint - _origin;
					index = 2;
				}
				else//Z
				{
					z = _xyPoint - _origin;
					index = 0;
				}
			}
			else//Z
			{
				z = _xPoint - _origin;
				z *= _xReverse;
				if (_dirIndex == 0)//X
				{
					x = _xyPoint - _origin;
					index = 1;
				}
				else//Y
				{
					y = _xyPoint - _origin;
					index = 0;
				}
			}
		}
		else
		{
			if (_mainIndex == 0)//X
			{
				x = _xLine * _mainReverse;
				if (_dirIndex == 0)//Y
				{
					y = _yLine * _dirReverse;
					index = 2;
				}
				else//Z
				{
					z = _yLine * _dirReverse;
					index = 1;
				}
			}
			else if (_mainIndex == 1)//Y
			{
				y = _xLine * _mainReverse;
				if (_dirIndex == 0)//X
				{
					x = _yLine * _dirReverse;
					index = 2;
				}
				else//Z
				{
					z = _yLine * _dirReverse;
					index = 0;
				}
			}
			else//Z
			{
				z = _xLine * _mainReverse;
				if (_dirIndex == 0)//X
				{
					x = _yLine * _dirReverse;
					index = 1;
				}
				else//Y
				{
					y = _yLine * _dirReverse;
					index = 0;
				}
			}
		}

		// 归一化
		if (!x.isNull()) x.normalize();
		if (!y.isNull()) y.normalize();
		if (!z.isNull()) z.normalize();

		if (index == 0)//缺失X
		{
			// 判断y和z是否平行
			if (QVector3D::crossProduct(y, z).length() < 1e-6)
			{
				success = false;
				msg = "Y和Z平行，无法构建坐标系";
				return QVector<QVector3D>{x, y, z};
			}

			if (_mainIndex == 1)//Y为主轴方向
			{
				//根据yz平面计算X
				x = QVector3D::crossProduct(y, z);
				x.normalize();
				//再根据Y和X计算Z
				z = QVector3D::crossProduct(x, y);
				z.normalize();
			}
			else//Z为主轴方向
			{
				//根据yz平面计算X
				x = QVector3D::crossProduct(y, z);
				x.normalize();
				//再根据Z和X计算Y
				y = QVector3D::crossProduct(z, x);
				y.normalize();
			}
		}
		else if (index == 1)//缺失Y
		{
			// 判断x和z是否平行
			if (QVector3D::crossProduct(x, z).length() < 1e-6)
			{
				success = false;
				msg = "X和Z平行，无法构建坐标系";
				return QVector<QVector3D>{x, y, z};
			}

			if (_mainIndex == 0)//X为主轴方向
			{
				//根据xz平面计算Y
				y = QVector3D::crossProduct(z, x);
				y.normalize();
				//再根据X和Y计算Z
				z = QVector3D::crossProduct(x, y);
				z.normalize();
			}
			else//Z为主轴方向
			{
				//根据xz平面计算Y
				y = QVector3D::crossProduct(z, x);
				y.normalize();
				//再根据Z和Y计算X
				x = QVector3D::crossProduct(y, z);
				x.normalize();
			}
		}
		else//缺失Z
		{
			// 判断x和y是否平行
			if (QVector3D::crossProduct(x, y).length() < 1e-6)
			{
				success = false;
				msg = "X和Y平行，无法构建坐标系";
				return QVector<QVector3D>{x, y, z};
			}

			if (_mainIndex == 0)//X为主轴方向
			{
				//根据xy平面计算z
				z = QVector3D::crossProduct(x, y);
				z.normalize();
				//再根据X和Z计算Y
				y = QVector3D::crossProduct(z, x);
				y.normalize();
			}
			else//Y为主轴方向
			{
				//根据xy平面计算z
				z = QVector3D::crossProduct(x, y);
				z.normalize();
				//再根据Y和Z计算X
				x = QVector3D::crossProduct(y, z);
				x.normalize();
			}
		}

		// 3. 应用变换队列（不改成员，只改局部）
		// ----------------------
		for (const TransformStep& step : _transformQueue)
		{
			if (step.type == TransformType::Translation)
			{
				origin_local += step.value;
			}
			else if (step.type == TransformType::Rotation)
			{
				QMatrix4x4 rot;
				rot.setToIdentity();
				rot.rotate(step.value.x(), x); // 绕局部X轴
				rot.rotate(step.value.y(), y); // 绕局部Y轴
				rot.rotate(step.value.z(), z); // 绕局部Z轴

				// 将局部坐标原点作为旋转中心
				QMatrix4x4 translateToOrigin, translateBack;
				translateToOrigin.translate(-origin_local);
				translateBack.translate(origin_local);

				QMatrix4x4 totalRot = translateBack * rot * translateToOrigin;

				// 更新坐标轴
				x = (rot * x).normalized();
				y = (rot * y).normalized();
				z = (rot * z).normalized();
			}
			else if (step.type == TransformType::Flip)
			{
				if (step.flipMap.size() != 3)
				{
					success = false;
					msg = "Flip 参数错误";
					return { x, y, z };
				}

				const float EPS = 1e-6f;
				QVector3D oldX = x, oldY = y, oldZ = z;

				// helper：按 code(±1/±2/±3) 取出对应旧轴并加符号
				auto axisByCode = [&](int code) -> QVector3D {
					int a = std::abs(code);
					QVector3D v = (a == 1) ? oldX : (a == 2 ? oldY : oldZ);
					if (code < 0) v = -v;
					return v;
					};

				// 标记哪些轴是用户明确指定的（即 flipMap[i] != default (i+1)）
				QVector<bool> userSpecified = {
					(step.flipMap[0] != 1),
					(step.flipMap[1] != 2),
					(step.flipMap[2] != 3)
				};

				// 先把明确指定的轴固定下来
				QVector3D newX = x, newY = y, newZ = z;
				if (userSpecified[0])
				{
					if (step.flipMap[0] == -1)
					{
						newX = -oldX;
					}
					else if (fabs(step.flipMap[0]) == 2)
					{
						userSpecified[1] = true;
						newX = oldY * step.flipMap[0] / fabs(step.flipMap[0]);
						newY = oldX * step.flipMap[0] / fabs(step.flipMap[0]);
					}
					else if (fabs(step.flipMap[0]) == 3)
					{
						userSpecified[2] = true;
						newX = oldZ * step.flipMap[0] / fabs(step.flipMap[0]);
						newZ = oldX * step.flipMap[0] / fabs(step.flipMap[0]);
					}
				}
				else if (userSpecified[1])
				{
					if (step.flipMap[1] == -2)
					{
						newY = -oldY;
					}
					else if (fabs(step.flipMap[1]) == 1)
					{
						userSpecified[0] = true;
						newX = oldY * step.flipMap[1] / fabs(step.flipMap[1]);
						newY = oldX * step.flipMap[1] / fabs(step.flipMap[1]);
					}
					else if (fabs(step.flipMap[1]) == 3)
					{
						userSpecified[2] = true;
						newY = oldZ * step.flipMap[1] / fabs(step.flipMap[1]);
						newZ = oldY * step.flipMap[1] / fabs(step.flipMap[1]);
					}
				}
				else if (userSpecified[2])
				{
					if (step.flipMap[2] == -3)
					{
						newZ = -oldZ;
					}
					else if (fabs(step.flipMap[2]) == 1)
					{
						userSpecified[0] = true;
						newX = oldZ * step.flipMap[2] / fabs(step.flipMap[2]);
						newZ = oldX * step.flipMap[2] / fabs(step.flipMap[2]);
					}
					else if (fabs(step.flipMap[2]) == 2)
					{
						userSpecified[1] = true;
						newY = oldZ * step.flipMap[2] / fabs(step.flipMap[2]);
						newZ = oldY * step.flipMap[2] / fabs(step.flipMap[2]);
					}
				}
				if (userSpecified[0] && userSpecified[1]) {
					// 都指定了 X、Y：用它们叉乘求 Z（右手）
					if (newX.length() < EPS || newY.length() < EPS) {
						success = false; msg = "Flip 导致零向量"; return { x, y, z };
					}
					QVector3D zFromXY = QVector3D::crossProduct(newX, newY);
					if (zFromXY.length() < EPS) { success = false; msg = "Flip 后 X 与 Y 平行"; return { x, y, z }; }
					newZ = zFromXY.normalized();

					// 若用户明确指定了 Z（userSpecified[2]==true），尝试匹配符号
					if (userSpecified[2]) {
						QVector3D attemptedZ = axisByCode(step.flipMap[2]);
						if (attemptedZ.length() >= EPS && QVector3D::dotProduct(newZ, attemptedZ.normalized()) < 0.0f) {
							newZ *= -1.0f;
						}
						// 重新修正 newY 以保证正交且右手（尽量保留 newX）
						newY = QVector3D::crossProduct(newZ, newX).normalized();
					}
					else {
						// 若用户未指定 Z，则用 newZ and re-orthogonalize newY 保证正交
						newY = QVector3D::crossProduct(newZ, newX).normalized();
					}
				}

				else if (userSpecified[0] && userSpecified[2]) {
					// X 和 Z 指定 -> 用 Z x X 求 Y
					if (newX.length() < EPS || newZ.length() < EPS) { success = false; msg = "Flip 导致零向量"; return { x, y, z }; }
					QVector3D yFromZX = QVector3D::crossProduct(newZ, newX);
					if (yFromZX.length() < EPS) { success = false; msg = "Flip 后 Z 与 X 平行"; return { x, y, z }; }
					newY = yFromZX.normalized();
					// 若用户指定了 Y，调整符号使其尽量一致
					if (userSpecified[1]) {
						QVector3D attemptedY = axisByCode(step.flipMap[1]);
						if (attemptedY.length() >= EPS && QVector3D::dotProduct(newY, attemptedY.normalized()) < 0.0f) newY *= -1.0f;
					}
				}
				else if (userSpecified[1] && userSpecified[2]) {
					// Y 和 Z 指定 -> 用 X = Y x Z
					if (newY.length() < EPS || newZ.length() < EPS) { success = false; msg = "Flip 导致零向量"; return { x, y, z }; }
					QVector3D xFromYZ = QVector3D::crossProduct(newY, newZ);
					if (xFromYZ.length() < EPS) { success = false; msg = "Flip 后 Y 与 Z 平行"; return { x, y, z }; }
					newX = xFromYZ.normalized();
					if (userSpecified[0]) {
						QVector3D attemptedX = axisByCode(step.flipMap[0]);
						if (attemptedX.length() >= EPS && QVector3D::dotProduct(newX, attemptedX.normalized()) < 0.0f) newX *= -1.0f;
					}
				}
				else if (userSpecified[0]) {
					// 只有 X 指定：以 newX 为基，优先用 oldY 作为参考生成 newZ，然后 newY = z x x
					if (newX.length() < EPS) { success = false; msg = "Flip 导致零向量"; return { x, y, z }; }
					QVector3D ref = oldY.length() >= EPS ? oldY : oldZ;
					QVector3D zFrom = QVector3D::crossProduct(newX, ref);
					if (zFrom.length() < EPS) { // 参考向量不可用 -> 换另一个参考
						ref = oldZ;
						zFrom = QVector3D::crossProduct(newX, ref);
						if (zFrom.length() < EPS) { success = false; msg = "Flip 无法生成正交轴"; return { x, y, z }; }
					}
					newZ = zFrom.normalized();
					newY = QVector3D::crossProduct(newZ, newX).normalized();
				}

				else if (userSpecified[1]) {
					// 只有 Y 指定：以 newY 为基，参考 oldX 生成 newZ，然后 newX = y x z
					if (newY.length() < EPS) { success = false; msg = "Flip 导致零向量"; return { x, y, z }; }
					QVector3D ref = oldX.length() >= EPS ? oldX : oldZ;
					QVector3D zFrom = QVector3D::crossProduct(ref, newY); // ref x newY gives Z
					if (zFrom.length() < EPS) { ref = oldZ; zFrom = QVector3D::crossProduct(ref, newY); if (zFrom.length() < EPS) { success = false; msg = "Flip 无法生成正交轴"; return { x, y, z }; } }
					newZ = zFrom.normalized();
					newX = QVector3D::crossProduct(newY, newZ).normalized();
				}
				else if (userSpecified[2]) {
					// 只有 Z 指定：以 newZ 为基，参考 oldX 生成 newY，然后 newX = y x z
					if (newZ.length() < EPS) { success = false; msg = "Flip 导致零向量"; return { x, y, z }; }
					QVector3D ref = oldX.length() >= EPS ? oldX : oldY;
					QVector3D yFrom = QVector3D::crossProduct(newZ, ref); // z x ref -> y
					if (yFrom.length() < EPS) { ref = oldY; yFrom = QVector3D::crossProduct(newZ, ref); if (yFrom.length() < EPS) { success = false; msg = "Flip 无法生成正交轴"; return { x, y, z }; } }
					newY = yFrom.normalized();
					newX = QVector3D::crossProduct(newY, newZ).normalized();
				}
				else {
					// 没有任何用户指定：保持原轴（或按默认行为）
					newX = oldX; newY = oldY; newZ = oldZ;
				}

				// 归一化并赋回
				x = newX.normalized();
				y = newY.normalized();
				z = newZ.normalized();
			}
		}

		// ----------------------
		// 4. 输出
		// ----------------------
		success = true;
		if (_id != -1)
		{
			msg = QString("%1 更新成功").arg(_coordinateName);
		}
		origin = origin_local; // 输出最终原点
		return { x, y, z };
	}
	mCoordinateBase* mCoordinateBase::copyCoordinate(const int& id, const QString& name)
	{
		mCoordinateBase* coord = new mCoordinateBase(id, getCoordinateType(), name);
		coord->_method = _method;
		coord->_origin = _origin;
		coord->_mainIndex = _mainIndex;
		coord->_dirIndex = _dirIndex;
		coord->_xPoint = _xPoint;
		coord->_xyPoint = _xyPoint;
		coord->_xReverse = _xReverse;
		coord->_xLine = _xLine;
		coord->_yLine = _yLine;
		coord->_mainBase = _mainBase;
		coord->_mainReverse = _mainReverse;
		coord->_dirBase = _dirBase;
		coord->_dirReverse = _dirReverse;
		return coord;
	}
}
