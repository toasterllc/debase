#pragma once
#include <optional>
#include "Panel.h"
#include "Color.h"
#include "Label.h"

namespace UI {

class ModalPanel : public Panel {
public:
    ModalPanel(const ColorPalette& colors) : colors(colors), _title(colors), _message(colors) {
        _message.centerSingleLine(true);
        _message.wrap(true);
    }
    
    static constexpr Size BorderSize() { return {5,2}; }
    static constexpr Rect ContentRect(Size size) {
        return Inset({{},size}, BorderSize());
    }
    
    Size messageOffset() const {
        return {0, 0};
    }
    
    Size sizeIntrinsic(Size constraint) override {
        const Rect rect = ContentRect(constraint);
        const Size messageSize = _message.sizeIntrinsic({rect.size.x, 0});
        return {
            .x = constraint.x,
            .y = 2*BorderSize().y + messageOffset().y + messageSize.y,
        };
    }
    
    void layout() override {
        const Rect f = frame();
        const Rect rect = contentRect();
        const Point titlePos = {3,0};
        _title.attr(_color|A_BOLD);
        
        const int titleWidthMax = f.size.x-2*titlePos.x;
        const int titleWidth = std::min(titleWidthMax, _title.sizeIntrinsic({}).x);
        _title.frame({titlePos, {titleWidth, 1}});
        
        const Size messageSize = _message.sizeIntrinsic({rect.size.x, 0});
        _message.frame({rect.origin + messageOffset(), {rect.size.x, messageSize.y}});
        
//        _title.layout(*this);
//        _message.layout(*this);
    }
    
    void draw() override {
        if (erased()) {
            Window::Attr style = attr(_color);
//            drawRect(Inset(bounds(), {2,1}));
            drawRect(bounds());
        }
        
//        _title.draw(*this);
        if (!_title.text().empty()) {
            // Add spaces around title
            drawText(_title.frame().tl()-Size{1,0}, " ");
            drawText(_title.frame().tr()+Size{1,0}, " ");
        }
        
//        _message.draw(*this);
    }
    
    bool handleEvent(const Event& ev) override {
        // Dismiss upon mouse-up
        if (ev.mouseUp(Event::MouseButtons::Left|Event::MouseButtons::Right) ||
            ev.type == Event::Type::KeyEscape) {
            
            if (_dismissAction) {
                _dismissAction(*this);
            }
        }
        return true;
    }
    
    Rect contentRect() { return ContentRect(size()); }
    
    const ColorPalette& colors;
    
    View*const* subviews() override {
        return _subviews;
    }
    
    const auto& color() const { return _color; }
    template <typename T> void color(const T& x) { _set(_color, x); }
    
    auto& title() { return _title; }
    auto& message() { return _message; }
    
    const auto& dismissAction() const { return _dismissAction; }
    template <typename T> void dismissAction(const T& x) { _setAlways(_dismissAction, x); }
    
private:
    static constexpr int _MessageSpacingTop = 1;
    
    Color _color;
    Label _title;
    Label _message;
    std::function<void(ModalPanel&)> _dismissAction;
    View*const _subviews[3] = { &_title, &_message, nullptr };
};

using ModalPanelPtr = std::shared_ptr<ModalPanel>;

} // namespace UI
