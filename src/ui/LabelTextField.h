#pragma once
#include "Window.h"
#include "View.h"

namespace UI {

class LabelTextField : public View {
public:
    Size sizeIntrinsic(Size constraint) override {
        return _label->sizeIntrinsic(constraint);
    }
    
    void layout() override {
        const Size s = size();
        const Size labelSize = _label->sizeIntrinsic({});
        _label->frame({{}, labelSize});
        
        const Size textFieldSize = {s.x-labelSize.x-_spacingX, 1};
        _textField->frame({{labelSize.x+_spacingX, 0}, textFieldSize});
        
//        _label.layout(win);
//        _textField.layout(win);
    }
    
    auto& label() { return _label; }
    auto& textField() { return _textField; }
    
    const auto& spacingX() const { return _spacingX; }
    template <typename T> void spacingX(const T& x) { _set(_spacingX, x); }
    
private:
    LabelPtr _label         = subviewCreate<Label>();
    TextFieldPtr _textField = subviewCreate<TextField>();
    int _spacingX = 0;
};

using LabelTextFieldPtr = std::shared_ptr<LabelTextField>;

} // namespace UI
