#include "beamView.h"
#include <QMultiHash>
#include <QString>

namespace MechSectionShapeData
{
	QMultiHash<QString, void*> beamView::_nameAisPair = QMultiHash<QString, void*>{};
	QMultiHash<QString, void*> beamView::_secNameAisPair = QMultiHash<QString, void*>{};
}