#pragma once
#include "ModalPanel.h"

namespace UI {

class UpdateAvailablePanel : public ModalPanel {
public:
    UpdateAvailablePanel(Version version) {
        width                           (26);
        title()->text                   ("debase v" + std::to_string(version) + " update");
        title()->align                  (UI::Align::Center);
        color                           (colors().menu);
        condensed                       (true);
        okButton()->label()->text       ("Download");
        dismissButton()->label()->text  ("Ignore");
        okButton()->action              ([] (UI::Button&) {});
        dismissButton()->action         ([] (UI::Button&) {});
        truncateEdges                   (UI::Edges{.l=0, .r=0, .t=0, .b=1});
    }
    
    bool handleEvent(const Event& ev) override {
        return false;
    }
};

using UpdateAvailablePanelPtr = std::shared_ptr<UpdateAvailablePanel>;

} // namespace UI
