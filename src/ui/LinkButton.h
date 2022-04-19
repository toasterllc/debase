#pragma once
#include "Button.h"
#include "OpenURL.h"

namespace UI {

class LinkButton : public Button {
public:
    LinkButton() {
        label()->textAttr(WA_UNDERLINE);
        action(std::bind(&LinkButton::_action, this));
    }
    
    const auto& link() const { return _link; }
    template <typename T> bool link(const T& x) { return _set(_link, x); }
    
private:
    void _action() {
        OpenURL(_link.c_str());
    }
    
    std::string _link;
};

using LinkButtonPtr = std::shared_ptr<LinkButton>;

} // namespace UI
