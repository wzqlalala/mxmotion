#pragma once

namespace MIOFile
{
	class CurveKey
	{
	public:
		// 自动生成名称
		QString toString() const {
			QStringList parts;
			if (!component.isEmpty()) parts << component;
			if (!layer.isEmpty()) parts << layer;
			if (!cplx.isEmpty()) parts << cplx;
			return parts.join("-");
		}

		QList<CurveKey> generateCurveKeys(const QStringList& components, const QStringList& layers, const QStringList& cplxForms)
		{
			QList<CurveKey> result;
			QStringList layerList = layers.isEmpty() ? QStringList{ "" } : layers;
			QStringList cplxList = cplxForms.isEmpty() ? QStringList{ "" } : cplxForms;
			for (const QString& comp : components)
				for (const QString& layer : layerList)
					for (const QString& cplx : cplxList)
					{
						CurveKey key;
						key.component = comp;
						key.layer = layer;
						key.cplx = cplx;
						result << key;
					}
			return result;
		}

	public:
		QString component; // 分量，如 "x"
		QString layer;     // 层，如 "layer1"，可以为空
		QString cplx;      // 复数形式，如 "实部"，可以为空
	};
}
