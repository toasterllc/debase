#pragma once
#include "Button.h"
#include "ModalPanel.h"

namespace UI {

class ButtonSpinner {
public:
    ButtonSpinner() {}
    
    ButtonSpinner(ModalPanelPtr panel, ButtonPtr button) :
    _panel(panel), _button(button), _backupButton(*_button), _backupLabel(*(_button->label())) {
        _panel->suppressEvents(true);
        
        _button->enabled(true);
        _button->interaction(false);
        _button->label()->prefix(std::string("  "));
    }
    
    ~ButtonSpinner() {
        if (_panel) {
            _panel->suppressEvents(false);
            
//            _panel->eraseNeeded(true); // Hack to make sure label gets redraw properly
            *_button = _backupButton;
            *_button->label() = _backupLabel;
            // Ensure a redraw
            _button->eraseNeeded(true);
        }
    }
    
    void animate() {
        _button->label()->suffix(std::string(" ") + _Animation[_idx]);
        _idx++;
        if (_idx >= std::size(_Animation)) _idx = 0;
    }
    
private:
//    static constexpr const char* _Animation[] = { "⢿","⣻","⣽","⣾","⣷","⣯","⣟","⡿" };
//    static constexpr const char* _Animation[] = { "⠋","⠙","⠹","⠸","⠼","⠴","⠦","⠧","⠇","⠏" };
    static constexpr const char* _Animation[] = { "⡇","⠏","⠛","⠹","⢸","⣰","⣤","⣆" };
//    static constexpr const char* _Animation[] = { "⡏","⠟","⠻","⢹","⣸","⣴","⣦","⣇" };
    
    ModalPanelPtr _panel;
    ButtonPtr _button;
    Button _backupButton;
    Label _backupLabel;
    size_t _idx = 0;
};

using ButtonSpinnerPtr = std::shared_ptr<ButtonSpinner>;

} // namespace UI
