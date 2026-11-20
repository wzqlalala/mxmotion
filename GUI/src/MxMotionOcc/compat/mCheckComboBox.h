#pragma once

#include <QComboBox>

namespace MxFunctions {

// The OCC ribbon does not use the private SDK's multi-check model.
class mCheckComboBox : public QComboBox
{
public:
    using QComboBox::QComboBox;
};

} // namespace MxFunctions
