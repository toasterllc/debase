#pragma once
#include "Button.h"

namespace UI {

class ButtonSpinner {
public:
    ButtonSpinner() {}
    
    ButtonSpinner(ButtonPtr button) :
    _s{.button=button, .backupButton=*button, .backupLabel=*button->label()} {
        _s.button->interaction(false);
        _s.button->label()->prefix(std::string("  "));
    }
    
    ButtonSpinner(ButtonSpinner&& x) {
        std::swap(_s, x._s);
    }
    
    ButtonSpinner& operator =(ButtonSpinner&& x) {
        std::swap(_s, x._s);
        return *this;
    }
    
    ~ButtonSpinner() {
        if (_s.button) {
            *_s.button = _s.backupButton;
            *_s.button->label() = _s.backupLabel;
            // Ensure a redraw
            _s.button->eraseNeeded(true);
        }
    }
    
    void animate() {
        _s.button->label()->suffix(std::string(" ") + _Animation[_s.idx]);
        _s.idx++;
        if (_s.idx >= std::size(_Animation)) _s.idx = 0;
    }
    
    operator bool() const { return (bool)_s.button; }
    
private:
//    static constexpr const char* _Animation[] = { "⢿","⣻","⣽","⣾","⣷","⣯","⣟","⡿" };
//    static constexpr const char* _Animation[] = { "⠋","⠙","⠹","⠸","⠼","⠴","⠦","⠧","⠇","⠏" };
    static constexpr const char* _Animation[] = { "⡇","⠏","⠛","⠹","⢸","⣰","⣤","⣆" };
//    static constexpr const char* _Animation[] = { "⡏","⠟","⠻","⢹","⣸","⣴","⣦","⣇" };
    
    struct {
        ButtonPtr button;
        Button backupButton;
        Label backupLabel;
        size_t idx = 0;
    } _s;
};

using ButtonSpinnerPtr = std::shared_ptr<ButtonSpinner>;

} // namespace UI
