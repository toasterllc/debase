#pragma once
#include <algorithm>
#include "Panel.h"
#include "Button.h"
#include "UTF8.h"

namespace UI {

class Menu : public Panel {
public:
    // Padding(): the additional width/height on top of the size of the buttons
    static constexpr Size Padding() {
        return Size{2*(_BorderSize+_InsetX), 2*_BorderSize};
    }
    
    Menu(const ColorPalette& colors) : colors(colors) {}
    
//    ButtonPtr updateMouse(const Point& p) {
//        Rect frame = Panel::frame();
//        Point off = p-frame.point;
//        bool mouseActive = HitTest(frame, p);
//        
//        ButtonPtr mouseOverButton = nullptr;
//        for (ButtonPtr button : buttons) {
//            button->mouseActive = mouseActive;
//            bool hit = button->hitTest(off, {1,1});
//            if (hit && !mouseOverButton) {
//                button->highlight(true);
//                mouseOverButton = button;
//            } else {
//                button->highlight(false);
//            }
//        }
//        
//        return mouseOverButton;
//    }
    
    void layout() override {
        // Short-circuit if layout isn't needed
        if (!_layoutNeeded) return;
        _layoutNeeded = false;
        
        // Find the longest button to set our width
        int width = 0;
        for (ButtonPtr button : buttons) {
            width = std::max(width, button->frame.size.x);
        }
        
        width += Padding().x;
        
        const Size sizeMax = containerSize-frame().point;
        const int x = _BorderSize+_InsetX;
        int y = _BorderSize;
        int height = Padding().y;
        size_t idx = 0;
        for (ButtonPtr button : buttons) {
            int newHeight = height;
            // Add space for separator
            // If we're not the first button, add space for the separator at the top of the button
            if (idx) {
                y += _SeparatorHeight;
                newHeight += _SeparatorHeight;
            }
            
            // Set button position (after separator)
            Rect& f = button->frame;
            f.point = {x,y};
            
            // Add space for button
            y += f.size.y;
            newHeight += f.size.y;
            
            // Set the expanded hit test size so that the menu doesn't have any dead zones
            if (!idx) button->hitTestExpand.t = 1;
            button->hitTestExpand.b = 1;
            
            // Bail if the button won't fit in the available height
            if (allowTruncate && newHeight>sizeMax.y) break;
            
            height = newHeight;
            idx++;
        }
        _buttonCount = idx;
        
        setSize({width, height});
    }
    
    void draw() override {
        drawNeeded = _drawNeeded();
        if (!drawNeeded) return;
        Panel::draw();
        
        const int width = bounds().size.x;
        
        for (size_t i=0; i<_buttonCount; i++) {
            ButtonPtr button = buttons[i];
            button->draw(*this);
            
            // Draw separator
            if (i != _buttonCount-1) {
                Window::Attr color = attr(colors.menu);
                drawLineHoriz({0, button->frame.ymax()+1}, width);
            }
        }
        
        // Draw border
        {
            Window::Attr color = attr(colors.menu);
            drawBorder();
            
            // Draw title
            if (!title.empty()) {
                Window::Attr bold = attr(A_BOLD);
                int offX = (width-(int)UTF8::Strlen(title))/2;
                drawText({offX,0}, " %s ", title.c_str());
            }
        }
    }
    
//    Event handleEvent(const Event& ev) override {
//        // Let caller handle window resize
//        if (ev.type == Event::Type::WindowResize) return ev;
//        
//        if (ev.type == Event::Type::Mouse) {
//            bool hit = HitTest(frame(), ev.mouse.point);
//            
//            // Update the mouseActive state for all of our buttons
//            for (ButtonPtr button : buttons) {
//                button->mouseActive(hit);
//            }
//            
//            if (ev.mouseDown() && !hit) {
////                assert(dismissAction);
//                if (dismissAction) dismissAction(*this);
//                return {};
//            }
//        
//        } else if (ev.type == Event::Type::KeyEscape) {
////            assert(dismissAction);
//            if (dismissAction) dismissAction(*this);
//            return {};
//        }
//        
//        // See if any of the buttons want the event
//        for (ButtonPtr button : buttons) {
//            Event e = button->handleEvent(*this, ev);
//            if (!e) {
////                assert(dismissAction);
//                if (dismissAction) dismissAction(*this);
//                return {};
//            }
//        }
//        
//        return {};
//    }
    
    void track(const Event& mouseDownEvent) {
        auto mouseDownTime = std::chrono::steady_clock::now();
        UI::Event ev = mouseDownEvent;
        Event::MouseButtons mouseUpButtons = Event::MouseButtons::Right;
//        bool abort = false;
        
        for (;;) {
            draw();
            ev = nextEvent();
            
//            // Let caller handle window resize
//            if (ev.type == Event::Type::WindowResize) return ev;
            
            // See if any of the buttons want the event
            for (ButtonPtr button : buttons) {
                Event e = button->handleEvent(*this, ev);
                if (!e) {
    //                assert(dismissAction);
                    if (dismissAction) dismissAction(*this);
                    return;
                }
            }
            
//            // Ensure that only a single button is highlighted
//            {
//                bool found = false;
//                for (ButtonPtr button : buttons) {
//                    if (!found) found = (button->enabled && button->highlighted());
//                    else        button->highlighted(false);
//                }
//            }
            
            if (ev.type == Event::Type::Mouse) {
                // Update the mouseActive state for all of our buttons
                bool inside = HitTest(bounds(), ev.mouse.point);
                for (ButtonPtr button : buttons) {
                    button->mouseActive(inside);
                }
                
                // Handle mouse down
                if (ev.mouseDown()) {
    //                assert(dismissAction);
                    if (dismissAction) dismissAction(*this);
                    return;
                
                // Handle mouse up
                } else if (ev.mouseUp(mouseUpButtons)) {
                    if (!(mouseUpButtons & Event::MouseButtons::Left)) {
                        // If the right-mouse-up occurs soon enough after right-mouse-down, the menu should
                        // stay open and we should start listening for left-mouse-down events.
                        auto duration = std::chrono::steady_clock::now()-mouseDownTime;
                        if (duration < _StayOpenThresh) {
                            // Start listening for left mouse up
                            mouseUpButtons |= Event::MouseButtons::Left;
                        
                        } else {
                            // Right-mouse-up occurred long after right-mouse-down, so trigger
                            // the button that the mouse is over (if any)
                            // We have to call the button action manually because handleEvent()
                            // doesn't work with only a mouse-up event.
                            for (ButtonPtr button : buttons) {
                                Event e = button->trigger(ev, mouseUpButtons);
                                if (!e) break;
                            }
                            return;
                        }
                        
                        // Start listening for left mouse up
                        mouseUpButtons |= Event::MouseButtons::Left;
                        
                        // Right mouse up, but menu stays open
                        // Now start tracking both left+right mouse down
                    
                    }
                }
            
            } else if (ev.type == Event::Type::KeyEscape) {
    //            assert(dismissAction);
                if (dismissAction) dismissAction(*this);
                return;
            }
            
            
//            abort = (ev.type != Event::Type::Mouse);
            
//            // Check if we should abort
//            if (abort) {
//                break;
//            
//            // Handle mouse up
//            } else if (ev.mouseUp(mouseUpButtons)) {
//                if (!(mouseUpButtons & Event::MouseButtons::Left)) {
//                    // If the right-mouse-up occurs soon enough after right-mouse-down, the menu should
//                    // stay open and we should start listening for left-mouse-down events.
//                    // If the right-mouse-up occurs af
//                    auto duration = std::chrono::steady_clock::now()-mouseDownTime;
//                    if (duration >= _StayOpenThresh) break;
//                    
//                    // Start listening for left mouse up
//                    mouseUpButtons |= Event::MouseButtons::Left;
//                    
//                    // Right mouse up, but menu stays open
//                    // Now start tracking both left+right mouse down
//                } else {
//                    // Close the menu only if clicking outside of the menu, or clicking on an
//                    // enabled menu button.
//                    // In other words, don't close the menu when clicking on a disabled menu
//                    // button.
//                    if (!menuButton || menuButton->enabled) {
//                        break;
//                    }
//                }
//            }
        }
    }
    
    const ColorPalette& colors;
    Size containerSize;
    std::string title;
    std::vector<ButtonPtr> buttons;
    bool allowTruncate = false;
    std::function<void(Menu&)> dismissAction;
    
private:
    static constexpr int _BorderSize      = 1;
    static constexpr int _SeparatorHeight = 1;
    static constexpr int _InsetX          = 1;
    static constexpr int _KeySpacing      = 2;
    static constexpr int _RowHeight       = 2;
    
    static constexpr auto _StayOpenThresh = std::chrono::milliseconds(300);
    
    bool _drawNeeded() const {
        for (ButtonPtr button : buttons) {
            if (button->drawNeeded) return true;
        }
        return false;
    }
    
    bool _layoutNeeded = true;
    size_t _buttonCount = 0;
};

using MenuPtr = std::shared_ptr<Menu>;

} // namespace UI
