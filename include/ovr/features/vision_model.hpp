#pragma once
#include <cstdint>
namespace ovr::state { struct HostState; }
namespace ovr::features {
class IVisionModel {
public:
    virtual ~IVisionModel() = default;
    virtual bool is_visible(const state::HostState&, std::uint32_t, int) const = 0;
    virtual bool established() const = 0;
};
inline const IVisionModel& null_vision_model() {
    class Unavailable final : public IVisionModel {
        bool is_visible(const state::HostState&, std::uint32_t, int) const override { return false; }
        bool established() const override { return false; }
    };
    static const Unavailable instance;
    return instance;
}
}
