#pragma once
#include "Window.h"
#include "View.h"

namespace UI {

class LabelTextField : public View {
public:
    LabelTextField(const ColorPalette& colors) : View(colors), _label(colors), _textField(colors) {}
    
    Size sizeIntrinsic(Size constraint) override {
        return _label.sizeIntrinsic(constraint);
    }
    
    bool layoutNeeded() const override {
        return View::layoutNeeded() || _label.layoutNeeded() || _textField.layoutNeeded();
    }
    
    bool layout(const Window& win) override {
        if (!View::layout(win)) return false;
        
        const Rect f = frame();
        const Size labelSize = _label.sizeIntrinsic({});
        _label.frame({f.origin, labelSize});
        
        const Size textFieldSize = {f.size.x-labelSize.x-_spacingX, 1};
        _textField.frame({f.origin+Size{labelSize.x+_spacingX, 0}, textFieldSize});
        
        _label.layout(win);
        _textField.layout(win);
        return true;
    }
    
    bool drawNeeded() const override {
        return View::drawNeeded() || _label.drawNeeded() || _textField.drawNeeded();
    }
    
    bool draw(const Window& win) override {
        if (!View::draw(win)) return false;
        _label.draw(win);
        _textField.draw(win);
        return true;
    }
    
    bool handleEvent(const Window& win, const Event& ev) override {
        return _textField.handleEvent(win, ev);
    }
    
    auto& label() { return _label; }
    auto& textField() { return _textField; }
    
    const auto& spacingX() const { return _spacingX; }
    template <typename T> void spacingX(const T& x) { _set(_spacingX, x); }
    
private:
    Label _label;
    TextField _textField;
    int _spacingX = 0;
};

using LabelTextFieldPtr = std::shared_ptr<LabelTextField>;

} // namespace UI
