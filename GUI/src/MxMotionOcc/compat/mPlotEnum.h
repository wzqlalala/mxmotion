#pragma once

namespace PlotView {
// Match Qwt's axis identifiers; the original GUI passes these straight to
// getQwtPlot()->setAxisFont(). They are not serialized MotionEngine enums.
enum AxisId { Y_Left=0, Y_Right=1, X_Bottom=2, X_Top=3 };
enum PlotType { LineChart=0, ScatterChart=1, StepChart=2 };
}
