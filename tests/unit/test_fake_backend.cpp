#include "memory/fake_backend/fake_backend.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <new>
#include <span>
#include <vector>

using ovr::memory::FakeBackend;
using ovr::memory::ReadReq;
using ovr::memory::WriteReq;
using ovr::memory::WriteResult;

namespace {

constexpr char kMapPath[] = "tests/fixtures/dumps/minimal.map.json";
constexpr char kMissingMap[] = "tests/fixtures/dumps/nope.map.json";
constexpr std::uintptr_t kClient = 0x180000000;
constexpr std::uintptr_t kEngine2 = 0x180800000;
constexpr std::uintptr_t kInput = 0x180A00000;
constexpr std::uint32_t kClientSpan = 4096;
constexpr std::uint32_t kEngine2Span = 2048;

// фикстура устроена так, что байт по смещению i внутри модуля равен (i & 0xFF)
std::uint64_t allocations = 0;
bool counting = false;
int failures = 0;

void check(bool condition) {
    if (!condition) {
        ++failures;
    }
}

std::span<const ReadReq> as_span(const std::vector<ReadReq>& reqs) {
    return std::span<const ReadReq>(reqs.data(), reqs.size());
}

void check_setup(FakeBackend& fake) {
    check(fake.load(kMapPath));
    check(fake.attach(424242));
    check(fake.load_calls() == 1);
    check(fake.module_base("client.dll") == kClient);
    check(fake.module_base("inputsystem.dll") == kInput);
    check(fake.module_base("engine2.dll") == kEngine2);
    check(fake.module_base("nope.dll") == 0);

    FakeBackend broken;
    check(!broken.load(kMissingMap));
    check(!broken.attach(1));
    check(broken.module_base("client.dll") == 0);
}

void check_pattern(FakeBackend& fake) {
    std::array<std::uint8_t, 8> bytes{};
    check(fake.read_raw(kClient + 0x40, bytes.data(), bytes.size()));
    const std::array<std::uint8_t, 8> expected{0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47};
    check(bytes == expected);

    std::array<std::uint8_t, 4> engine{};
    check(fake.read_raw(kEngine2 + 0x10, engine.data(), engine.size()));
    const std::array<std::uint8_t, 4> engine_expected{0x10, 0x11, 0x12, 0x13};
    check(engine == engine_expected);
}

void check_batch_equals_reads(FakeBackend& fake) {
    constexpr std::size_t kCount = 500;
    std::vector<ReadReq> single(kCount);
    std::vector<ReadReq> batch(kCount);
    std::vector<std::vector<std::uint8_t>> expected(kCount);
    std::vector<std::vector<std::uint8_t>> got(kCount);
    for (std::size_t index = 0; index < kCount; ++index) {
        const std::uint32_t size = static_cast<std::uint32_t>(1 + index % 8);
        const std::uintptr_t addr = kClient + (index * 5) % (kClientSpan - 8);
        expected[index].assign(size, 0xAA);
        got[index].assign(size, 0xAA);
        single[index] = ReadReq{addr, expected[index].data(), size};
        batch[index] = ReadReq{addr, got[index].data(), size};
    }

    std::size_t read_alone = 0;
    for (const auto& req : single) {
        if (fake.read_raw(req.addr, req.dst, req.size)) {
            ++read_alone;
        }
    }
    check(read_alone == kCount);

    const std::uint32_t batches_before = fake.batch_calls();
    fake.read_batch(as_span(batch));
    check(fake.batch_calls() == batches_before + 1);
    check(fake.load_calls() == 1);

    const std::size_t comparable =
        kCount < ovr::memory::kMaxReadRequests ? kCount : ovr::memory::kMaxReadRequests;
    for (std::size_t index = 0; index < comparable; ++index) {
        check(expected[index] == got[index]);
    }
    // E48: сверх лимита пачка не читает, поэтому её хвост остался заполнен 0xAA
    for (std::size_t index = comparable; index < kCount; ++index) {
        check(got[index].front() == 0xAA && got[index].back() == 0xAA);
    }
}

void check_out_of_map(FakeBackend& fake) {
    std::array<std::uint8_t, 8> bytes{};
    bytes.fill(0xAA);
    check(!fake.read_raw(kClient + kClientSpan - 2, bytes.data(), 8));
    check(bytes.front() == 0xAA && bytes.back() == 0xAA);

    bytes.fill(0xAA);
    check(!fake.read_raw(kClient - 4, bytes.data(), 8));
    check(bytes.front() == 0xAA);

    bytes.fill(0xAA);
    check(!fake.read_raw(kEngine2 + kEngine2Span, bytes.data(), 4));
    check(bytes.front() == 0xAA);

    bytes.fill(0xAA);
    check(!fake.read_raw(kClient + 0x20, bytes.data(), 0));
    check(bytes.front() == 0xAA);
    check(fake.load_calls() == 1);
}

void check_fail_range(FakeBackend& fake) {
    std::array<std::uint8_t, 4> bytes{};
    bytes.fill(0xAA);
    fake.set_fail_range(kClient + 0x100, 64);
    check(!fake.read_raw(kClient + 0x120, bytes.data(), bytes.size()));
    check(bytes.front() == 0xAA);
    check(fake.read_raw(kClient + 0x200, bytes.data(), bytes.size()));

    constexpr std::size_t kCount = 10;
    std::vector<ReadReq> reqs(kCount);
    std::vector<std::array<std::uint8_t, 4>> buffers(kCount);
    for (std::size_t index = 0; index < kCount; ++index) {
        buffers[index].fill(0xAA);
        reqs[index] = ReadReq{kClient + 0x100 + index * 4, buffers[index].data(), 4};
    }
    const std::uint32_t batches_before = fake.batch_calls();
    fake.read_batch(as_span(reqs));
    check(fake.batch_calls() == batches_before + 1);
    for (const auto& buffer : buffers) {
        check(buffer[0] == 0xAA && buffer[3] == 0xAA);
    }
    check(fake.load_calls() == 1);
}

void check_write_roundtrip(FakeBackend& fake) {
    const std::array<std::uint8_t, 4> payload{0x11, 0x22, 0x33, 0x44};
    const std::uint32_t writes_before = fake.write_calls();
    check(fake.write_raw(kEngine2 + 0x40, payload.data(), payload.size()));
    check(fake.write_calls() == writes_before + 1);

    std::array<std::uint8_t, 4> bytes{};
    check(fake.read_raw(kEngine2 + 0x40, bytes.data(), bytes.size()));
    check(bytes == payload);
}

void check_write_out_of_map(FakeBackend& fake) {
    std::array<std::uint8_t, 4> before{};
    check(fake.read_raw(kEngine2 + kEngine2Span - 4, before.data(), before.size()));

    const std::array<std::uint8_t, 4> payload{0xEE, 0xEE, 0xEE, 0xEE};
    const std::uint32_t writes_before = fake.write_calls();
    check(!fake.write_raw(kEngine2 + kEngine2Span - 1, payload.data(), payload.size()));
    check(!fake.write_raw(kInput + 4096, payload.data(), payload.size()));
    check(fake.write_calls() == writes_before);

    std::array<std::uint8_t, 4> after{};
    check(fake.read_raw(kEngine2 + kEngine2Span - 4, after.data(), after.size()));
    check(after == before);
}

void check_write_batch(FakeBackend& fake) {
    constexpr std::size_t kCount = 100;
    std::vector<WriteReq> reqs(kCount);
    std::vector<WriteResult> results(kCount);
    std::vector<std::array<std::uint8_t, 2>> payloads(kCount);
    for (std::size_t index = 0; index < kCount; ++index) {
        payloads[index] = {static_cast<std::uint8_t>(index), static_cast<std::uint8_t>(255 - index)};
        reqs[index] = WriteReq{kClient + 0x800 + index * 2, payloads[index].data(), 2};
    }
    const std::uint32_t writes_before = fake.write_calls();
    const std::uint32_t batches_before = fake.batch_calls();
    const std::uint64_t written_before = fake.write_bytes();
    fake.write_batch(std::span<const WriteReq>(reqs.data(), reqs.size()),
                     std::span<WriteResult>(results.data(), results.size()));
    check(fake.write_calls() == writes_before + kCount);
    check(fake.batch_calls() == batches_before);
    for (const auto& result : results) {
        check(result.ok && result.written == 2);
    }

    std::array<std::uint8_t, 2> back{};
    check(fake.read_raw(kClient + 0x800 + 7 * 2, back.data(), back.size()));
    check(back == payloads[7]);

    check(fake.write_bytes() == written_before + 2 * kCount);
}

void check_batch_limit(FakeBackend& fake) {
    constexpr std::size_t kCount = ovr::memory::kMaxReadRequests + 1;
    std::vector<ReadReq> reqs(kCount);
    std::vector<std::array<std::uint8_t, 4>> buffers(kCount);
    std::vector<ovr::memory::ReadResult> results(kCount);
    for (std::size_t index = 0; index < kCount; ++index) {
        buffers[index].fill(0xAA);
        // окно 0x400..0x800: вне set_fail_range, который выставил предыдущий тест
        reqs[index] = ReadReq{kClient + 0x400 + index * 4, buffers[index].data(), 4, &results[index]};
        results[index] = {buffers[index].data(), 4, true};
    }
    buffers[0] = {0, 1, 2, 3};  // полностью совпадает с запрошенными байтами фикстуры
    results[0] = {};
    reqs[1].addr = 0;    // отказ внутри пачки, следующие запросы должны выполниться
    const std::uint32_t batches_before = fake.batch_calls();
    allocations = 0;
    counting = true;
    fake.read_batch(as_span(reqs));
    counting = false;

    check(allocations == 0);
    check(fake.batch_calls() == batches_before + 1);
    for (std::size_t index = 0; index < ovr::memory::kMaxReadRequests; ++index) {
        if (index == 1) {
            check(!results[index].ok && !results[index].data && results[index].size == 0);
            check(buffers[index][0] == 0xAA);
        } else {
            check(results[index].ok && results[index].data == buffers[index].data());
            check(results[index].size == 4 && buffers[index][0] != 0xAA);
        }
    }
    check(!results.back().ok && !results.back().data && results.back().size == 0);
    for (const std::uint8_t byte : buffers[ovr::memory::kMaxReadRequests]) {
        check(byte == 0xAA);
    }
}

void check_zero_size(FakeBackend& fake) {
    // size == 0 — вырожденный запрос: читать нечего, ответ false, буфер не тронут.
    std::array<std::uint8_t, 4> bytes{};
    bytes.fill(0xAA);
    check(!fake.read_raw(kClient, bytes.data(), 0));
    check(bytes.front() == 0xAA && bytes.back() == 0xAA);

    const std::uint64_t written_before = fake.write_bytes();
    const std::uint8_t payload = 0x5A;
    check(!fake.write_raw(kClient + 0x40, &payload, 0));
    check(fake.write_bytes() == written_before);
}

void check_map_errors(const std::filesystem::path& binary_dir) {
    namespace fs = std::filesystem;
    fs::path dir;
    for (unsigned n = 0;; ++n) {
        dir = binary_dir / ("test_fake_backend_" + std::to_string(n));
        if (fs::create_directory(dir)) {
            break;
        }
    }
    const auto map = dir / "map.json";
    const auto data = dir / "bytes.bin";
    std::ofstream(data, std::ios::binary) << "abc";
    const auto max_addr = std::numeric_limits<std::uintptr_t>::max();
    const auto top = std::to_string(max_addr - 1);
    const std::string record = R"({"module":"m","file":"by\u0074es.bin","base":")" +
                               top + R"(","file_offset":1,"size":2})";
    const std::string valid = "[" + record + "]";
    std::ofstream(map) << valid;
    FakeBackend fake;
    check(fake.load(map.string()) && fake.attach(0));
    std::array<char, 2> bytes{};
    fake.set_fail_range(max_addr, 0);
    check(fake.read_raw(max_addr - 1, bytes.data(), 2));
    check((bytes == std::array{'b', 'c'}));
    fake.set_fail_range(max_addr, 2);
    check(!fake.read_raw(max_addr, bytes.data(), 1));
    check(!fake.read_raw(max_addr - 1, bytes.data(), 2));
    check(!fake.read_raw(max_addr, bytes.data(), 2));
    fake.set_write_fail_range(max_addr, 1);
    const char payload = 'x';
    const std::array<WriteReq, 2> reqs{{{max_addr, &payload, 1}, {max_addr - 1, &payload, 1}}};
    std::array<WriteResult, 2> results{};
    fake.write_batch(reqs, results);
    check(!results[0].ok && results[0].written == 0);
    check(results[1].ok && results[1].written == 1 && fake.write_bytes() == 1);
    fs::remove(data);
    const std::array<ReadReq, 1> reads{{{max_addr - 1, bytes.data(), 1}}};
    fake.read_batch(reads);
    check(bytes[0] == 'x');

    std::ofstream(data, std::ios::binary) << "abc";
    auto reject = [&](const std::string& json) {
        std::ofstream(map) << json;
        check(!fake.load(map.string()));
        check(!fake.attach(0));
        check(fake.module_base("m") == 0);
    };
    reject(valid + "extra");
    reject("[" + record + ",]");
    const auto size_pos = valid.find("\"size\":");
    const auto prefix = valid.substr(0, size_pos);
    for (const char* bad : {"-1", "1x", "01", "\"2\"", "18446744073709551615", "3"}) {
        reject(prefix + "\"size\":" + bad + "}]");
    }
    reject(prefix + "\"size\":1,\"size\":2}]");
    reject(prefix + "\"size\":1,}]");
    std::ofstream(map) << R"([{"module":"m","base":"0x1","file":"bytes.bin","size":1}])";
    check(fake.load(map.string()));  // старые карты без file_offset начинаются с нулевого байта
    char first{};
    check(fake.read_raw(1, &first, 1) && first == 'a');
    fs::create_directory(dir / "child");
    const auto nested = dir / "child/map.json";
    std::ofstream(nested) << R"([{"module":"m","base":1,"file":"../bytes.bin","size":1}])";
    check(!fake.load(nested.string()));
    reject(R"([{"module":"m","base":"0x1","file":"bytes.bin","size":18446744073709551614,"file_offset":0}])");
    reject(R"([{"module":"m","base":"0x1","file":"bytes.bin","size":1,"file_offset":9}])");
    fs::remove_all(dir);
}

}  // namespace

// Счётчик аллокаций живёт на паре new/delete поверх malloc: компилятор считает free
// парой для operator new, хотя выделяем мы именно malloc.
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmismatched-new-delete"
#endif

void* operator new(std::size_t size) {
    if (counting) {
        ++allocations;
    }
    void* place = std::malloc(size == 0 ? 1 : size);
    if (place == nullptr) {
        throw std::bad_alloc();
    }
    return place;
}

// память под счётчик аллокаций берём из malloc: new/delete переопределены одной парой
void release(void* place) {
    std::free(place);
}

void operator delete(void* place) noexcept {
    release(place);
}

void operator delete(void* place, std::size_t) noexcept {
    release(place);
}

void* operator new[](std::size_t size) {
    return ::operator new(size);
}

void operator delete[](void* place) noexcept {
    release(place);
}

void operator delete[](void* place, std::size_t) noexcept {
    release(place);
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

int main(int, char** argv) {
    check_map_errors(std::filesystem::absolute(argv[0]).parent_path());
    FakeBackend fake;
    check_setup(fake);
    check_pattern(fake);
    check_batch_equals_reads(fake);
    check_out_of_map(fake);
    check_fail_range(fake);
    check_write_roundtrip(fake);
    check_write_out_of_map(fake);
    check_write_batch(fake);
    check_batch_limit(fake);
    check_zero_size(fake);
    return failures == 0 ? 0 : 1;
}
