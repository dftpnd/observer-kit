#pragma once

#include <cstdint>
#include <span>

namespace ovr::memory {
// View на байты одного кадра: владелец буфера — FrameSource (§10.14), сам view живёт один кадр.
struct RawFrame {
    std::span<const std::uint8_t> bytes;   // что вернула пачка read_batch() за кадр
    std::uintptr_t module_base;            // база модуля, от которой считаются RVA
    std::uint64_t frame_seq;               // монотонный номер кадра
    bool complete;                         // false = часть диапазонов пачки не прочитана
};

// complete == false разбор не запрещает: берём прочитанное, а с ненадёжных полей снимаются
// биты достоверности (§10.4). Частичный отказ — это флаги по диапазонам, а не исключение и не
// уроненная пачка (E6). bytes.empty() допустим в первом кадре и при is_stale(); тогда же
// complete == false.
}
