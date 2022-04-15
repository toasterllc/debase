#pragma once
#include "Button.h"
#include "Label.h"

namespace UI {

class ButtonSpinner : public Label {
public:
    static std::shared_ptr<ButtonSpinner> Create(ButtonPtr button) {
        return button->subviewCreate<ButtonSpinner>(button);
    }
    
    ButtonSpinner(ButtonPtr button) : _s{.tracked=button->label()} {
        *((Label*)this) = *_s.tracked;
//        _s.label = _s.button->subviewCreate<Label>();
//        *_s.label = *button->label();
        prefix("  ");
//        _s.button->interaction(false);
//        _s.button->label()->prefix(std::string("  "));
    }
    
    ButtonSpinner(ButtonSpinner&& x) {
        std::swap(_s, x._s);
    }
    
    ButtonSpinner& operator =(ButtonSpinner&& x) {
        std::swap(_s, x._s);
        return *this;
    }
    
//    ~ButtonSpinner() {
//        if (_s.button) {
//            *_s.button = _s.backupButton;
//            *_s.button->label() = _s.backupLabel;
//            // Ensure a redraw
//            _s.button->eraseNeeded(true);
//        }
//    }
    
    bool layoutNeeded() const override {
        return true;
    }
    
    void layout() override {
        frame(_s.tracked->frame());
    }
    
    void animate() {
//        _s.label->frame(_s.button->label()->frame());
        suffix(std::string(" ") + _Animation[_s.idx]);
//        _s.button->label()->suffix(std::string(" ") + _Animation[_s.idx]);
        _s.idx++;
        if (_s.idx >= std::size(_Animation)) _s.idx = 0;
    }
    
//    operator bool() const { return (bool)_s.button; }
    
private:
//    static constexpr const char* _Animation[] = { "⢿","⣻","⣽","⣾","⣷","⣯","⣟","⡿" };
//    static constexpr const char* _Animation[] = { "⠋","⠙","⠹","⠸","⠼","⠴","⠦","⠧","⠇","⠏" };
    static constexpr const char* _Animation[] = { "⡇","⠏","⠛","⠹","⢸","⣰","⣤","⣆" };
//    static constexpr const char* _Animation[] = { "⡏","⠟","⠻","⢹","⣸","⣴","⣦","⣇" };
    
    struct {
        LabelPtr tracked;
        size_t idx = 0;
    } _s;
};

using ButtonSpinnerPtr = std::shared_ptr<ButtonSpinner>;

} // namespace UI
