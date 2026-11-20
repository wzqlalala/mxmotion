#pragma once

#include <QLineEdit>

namespace MxFunctions {

// The shared ribbon needs a QLineEdit-compatible type, not an expression evaluator.
class mCalcLineEdit : public QLineEdit
{
public:
    using QLineEdit::QLineEdit;
};

} // namespace MxFunctions
