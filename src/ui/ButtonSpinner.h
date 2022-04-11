#pragma once
#include "Button.h"

namespace UI {

class ButtonSpinner {
public:
    ButtonSpinner() {}
    
    ButtonSpinner(ButtonPtr b) : _button(b), _backup(*_button) {
        _button->enabled(true);
        _button->interaction(false);
        _button->label()->prefix(std::string("  "));
    }
    
    ~ButtonSpinner() {
        if (_button) *_button = _backup;
    }
    
    void animate() {
        _button->label()->suffix(std::string(" ") + _Animation[_idx]);
        _idx++;
        if (_idx >= std::size(_Animation)) _idx = 0;
    }
    
private:
    static constexpr const char* _Animation[] = { "|", "/", "-", "\\" };
    ButtonPtr _button;
    Button _backup;
    size_t _idx = 0;
};

using ButtonSpinnerPtr = std::shared_ptr<ButtonSpinner>;

} // namespace UI
