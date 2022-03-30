#pragma once

template <typename T, typename ...Args>
inline auto MakeShared(Args&&... args) {
    return std::make_shared<typename T::element_type>(std::forward<Args>(args)...);
}
