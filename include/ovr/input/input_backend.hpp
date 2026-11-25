#pragma once
#include "ovr/core/key.hpp"
namespace ovr::input {
using core::Key;
class IInputBackend {
public:
    virtual ~IInputBackend() = default;
    virtual bool press(Key) = 0;
    virtual bool release(Key) = 0;
    virtual bool is_down(Key) const = 0;
};
inline IInputBackend* create_sendinput() { return nullptr; }
inline IInputBackend* create_uinput() { return nullptr; }
}
