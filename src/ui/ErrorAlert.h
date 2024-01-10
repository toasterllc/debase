#pragma once
#include "Alert.h"

namespace UI {

class ErrorAlert : public Alert {
public:
    ErrorAlert() {
        color            (colors().error);
        title()->text    ("Error");
    }
};

using ErrorAlertPtr = std::shared_ptr<ErrorAlert>;

} // namespace UI
