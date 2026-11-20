#include "mMotionResultData.h"

namespace MechMotionData {

    mMotionMarkerFrame::mMotionMarkerFrame() = default;
    mMotionMarkerFrame::~mMotionMarkerFrame() = default;

    mMotionBodyFrame::mMotionBodyFrame() = default;
    mMotionBodyFrame::~mMotionBodyFrame() = default;

    mMotionJointFrame::mMotionJointFrame() = default;
    mMotionJointFrame::~mMotionJointFrame() = default;

    mMotionDriveFrame::mMotionDriveFrame() = default;
    mMotionDriveFrame::~mMotionDriveFrame() = default;

    // 所有 operator>>/operator<< 已集中到
    // mMotionDataInStream.h / mMotionDataOutStream.h 中内联实现

}
