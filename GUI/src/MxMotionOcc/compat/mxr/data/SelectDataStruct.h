#pragma once

// Original picker value type. result_id is a true global CAD topology ID, a
// supplied FEM node ID or an actual connectivity ordinal, according to type.
// part_id is retained for assembly/FEM disambiguation; -1 means unspecified.
struct RenderPickResultData {
    int result_id = -1;
    int result_id2 = -1;
    int part_id = -1;
    RenderPickResultData() = default;
    RenderPickResultData(int id, int id2 = -1, int part = -1)
        : result_id(id), result_id2(id2), part_id(part) {}
};

inline bool operator==(const RenderPickResultData& a, const RenderPickResultData& b)
{
    return a.result_id == b.result_id && a.result_id2 == b.result_id2 && a.part_id == b.part_id;
}
inline bool operator!=(const RenderPickResultData& a, const RenderPickResultData& b) { return !(a == b); }
