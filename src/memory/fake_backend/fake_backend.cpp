#include "fake_backend.hpp"

#include <cstring>

namespace ovr::memory {

bool FakeBackend::load(std::string_view map_path) {
    ++load_calls_;
    DumpLoadResult result = load_dump_map(std::string(map_path));
    if (!result.ok) {
        entries_.clear();
        loaded_ = false;
        return false;
    }
    entries_ = std::move(result.entries);
    loaded_ = !entries_.empty();
    return loaded_;
}

bool FakeBackend::attach(std::uint32_t) {
    return loaded_;
}

std::uintptr_t FakeBackend::module_base(std::string_view module) const {
    for (const auto& entry : entries_) {
        if (entry.module == module) {
            return entry.base;
        }
    }
    return 0;
}

MapEntry* FakeBackend::locate(std::uintptr_t addr, std::uint32_t size) {
    if (size == 0) {
        return nullptr;
    }
    for (auto& entry : entries_) {
        if (addr >= entry.base && size <= entry.bytes.size() &&
            addr - entry.base <= entry.bytes.size() - size) {
            return &entry;
        }
    }
    return nullptr;
}

bool FakeBackend::blocked(const std::vector<FailRange>& ranges, std::uintptr_t addr,
                          std::uint32_t size) const {
    for (const auto& range : ranges) {
        const bool overlaps = addr < range.begin ? range.begin - addr < size
                                                 : addr - range.begin < range.size;
        if (overlaps) {
            return true;
        }
    }
    return false;
}

bool FakeBackend::read_raw(std::uintptr_t addr, void* dst, std::uint32_t size) {
    if (dst == nullptr || blocked(fail_ranges_, addr, size)) {
        return false;
    }
    MapEntry* entry = locate(addr, size);
    if (entry == nullptr) {
        return false;
    }
    const std::size_t offset = static_cast<std::size_t>(addr - entry->base);
    std::memcpy(dst, entry->bytes.data() + offset, size);
    return true;
}

void FakeBackend::read_batch(std::span<const ReadReq> reqs) {
    ++batch_calls_;
    for (std::size_t i = 0; i < reqs.size(); ++i) {
        const ReadReq& req = reqs[i];
        const bool ok = i < kMaxReadRequests && read_raw(req.addr, req.dst, req.size);
        if (req.result) {
            *req.result = ok ? ReadResult{static_cast<const std::uint8_t*>(req.dst), req.size, true}
                             : ReadResult{};
        }
    }
}

bool FakeBackend::write_raw(std::uintptr_t addr, const void* src, std::uint32_t size) {
    if (src == nullptr || blocked(write_fail_ranges_, addr, size)) {
        return false;
    }
    MapEntry* entry = locate(addr, size);
    if (entry == nullptr) {
        return false;
    }
    const std::size_t offset = static_cast<std::size_t>(addr - entry->base);
    std::memcpy(entry->bytes.data() + offset, src, size);
    ++write_calls_;
    write_bytes_ += size;
    return true;
}

void FakeBackend::write_batch(std::span<const WriteReq> reqs, std::span<WriteResult> out) {
    const std::size_t count = reqs.size() < out.size() ? reqs.size() : out.size();
    for (std::size_t index = 0; index < count; ++index) {
        const bool ok = write_raw(reqs[index].addr, reqs[index].src, reqs[index].size);
        out[index].written = ok ? reqs[index].size : 0;
        out[index].ok = ok;
    }
}

void FakeBackend::set_fail_range(std::uintptr_t addr, std::uint32_t size) {
    if (size != 0) {
        fail_ranges_.push_back(FailRange{addr, size});
    }
}

void FakeBackend::set_write_fail_range(std::uintptr_t addr, std::uint32_t size) {
    if (size != 0) {
        write_fail_ranges_.push_back(FailRange{addr, size});
    }
}

std::uint32_t FakeBackend::batch_calls() const {
    return batch_calls_;
}

std::uint32_t FakeBackend::write_calls() const {
    return write_calls_;
}

std::uint64_t FakeBackend::write_bytes() const {
    return write_bytes_;
}

std::uint32_t FakeBackend::load_calls() const {
    return load_calls_;
}

}
