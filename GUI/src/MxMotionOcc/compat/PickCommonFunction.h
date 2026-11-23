#pragma once
#include "render/LegacyRender.h"
namespace MechPreData {
inline std::vector<int> getPreWidgetSelectPartID(mxr::CCSMRendWidget* view) { return view ? view->selectedParts() : std::vector<int>{}; }
inline void setPreWidgetSelectPartID(mxr::CCSMRendWidget* view,const std::vector<int>& ids) { if(view)view->setSelectedParts(ids,false); }
inline void addPreWidgetSelectPartID(mxr::CCSMRendWidget* view,const std::vector<int>& ids) {
    if(!view)return;auto selected=view->selectedParts();
    for(int id:ids)if(std::find(selected.begin(),selected.end(),id)==selected.end())selected.push_back(id);
    view->setSelectedParts(selected,false);
}
inline void reducePreWidgetSelectPartID(mxr::CCSMRendWidget* view,const std::vector<int>& ids) {
    if(!view)return;auto selected=view->selectedParts();
    selected.erase(std::remove_if(selected.begin(),selected.end(),[&](int id) { return std::find(ids.begin(),ids.end(),id)!=ids.end(); }),selected.end());
    view->setSelectedParts(selected,false);
}
inline void clearPreWidgetSelectPartID(mxr::CCSMRendWidget* view) { if(view)view->setSelectedParts({},false); }
inline void UpdatePreWidgetHighlight(mxr::CCSMRendWidget* view) { if(view) { view->updatePaint();emit view->getSelectionContainer()->ReadyToReadContainer(); } }
inline void UpdatePreWidgetHighlightWithoutRTRC(mxr::CCSMRendWidget* view) { if(view)view->updatePaint(); }
inline void ClearPreWidgetGeoPick(mxr::CCSMRendWidget* view) { clearPreWidgetSelectPartID(view); }
inline void ClearPreWidgetMeshPick(mxr::CCSMRendWidget* view) { if(view)view->getSelectionContainer()->mesh.values.clear(); }
inline void ClearPreWidgetSymbolPick(mxr::CCSMRendWidget* view) {
    if(view)view->getSelectionContainer()->getSymbolSelectData()->UpdatePickData(
        mxr::SelectionOperate::CLEAR,mxr::SymbolSelectionInfoType::Symbol_All,{});
}
}
