#pragma once
namespace MxFunctions {
enum MapRelationType {
    MapRelation_None = 0,
    MapRelation_MotionActionBody, MapRelation_MotionBaseBody,
    MapRelation_MotionBodyI, MapRelation_MotionBodyJ,
    MapRelation_MotionAxisBody, MapRelation_MotionBodyA, MapRelation_MotionBodyB,
    MapRelation_MotionReferenceBody,
    MapRelation_MotionFirstJoint, MapRelation_MotionSecondJoint
};
}
