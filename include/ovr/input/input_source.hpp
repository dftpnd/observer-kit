#pragma once
#include "ovr/input/input_backend.hpp"
#include <memory>
#include <span>
namespace ovr::input {
class IInputSource {
public:
    virtual ~IInputSource() = default;
    virtual bool press(Key) = 0;
    virtual bool release(Key) = 0;
    virtual bool is_down(Key) const = 0;
    virtual std::span<const Key> whitelist() const = 0;
};
class UnavailableInput final : public IInputSource {
public:
    bool press(Key) override { return false; }
    bool release(Key) override { return false; }
    bool is_down(Key) const override { return false; }
    std::span<const Key> whitelist() const override { return {}; }
};
inline std::unique_ptr<IInputSource> create_input_source(IInputBackend*, std::span<const Key>) {
    return std::make_unique<UnavailableInput>();
}
}
