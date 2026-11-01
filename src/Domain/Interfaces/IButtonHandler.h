#pragma once

#include <cstdint>

namespace glp::domain {

enum class ButtonEvent {
    None,
    ShortPress,
    LongPress
};

class IButtonHandler {
public:
    virtual ~IButtonHandler() = default;
    virtual void init() = 0;
    virtual ButtonEvent update() = 0;
    virtual uint8_t getPin() const = 0;
};

} // namespace glp::domain

