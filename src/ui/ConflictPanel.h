#pragma once

namespace UI {

class ConflictPanel : public Panel {
public:
    
    // MARK: - View Overrides
    void layout() override {
        Panel::layout();
        
    }
    
private:
};

using ConflictPanelPtr = std::shared_ptr<ConflictPanel>;

} // namespace UI
