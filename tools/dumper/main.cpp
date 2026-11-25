#include <charconv>
#include <iostream>
#include <string_view>
int main(int argc, char** argv) {
    bool dry_run = false;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--help") {
            std::cout << "ovr_dumper [--build N] [--out DIR] [--env FILE] [--dry-run]\n"
                         "offline mock; no files are written\n";
            return 0;
        }
        if (arg == "--dry-run") { dry_run = true; continue; }
        if (arg != "--build" && arg != "--env" && arg != "--out") return 2;
        if (++i == argc || std::string_view(argv[i]).empty() || std::string_view(argv[i]).starts_with("--")) return 2;
        if (arg == "--build") {
            unsigned build{};
            std::string_view value = argv[i];
            auto [end, ec] = std::from_chars(value.data(), value.data()+value.size(), build);
            if (ec != std::errc{} || end != value.data()+value.size()) return 2;
        }
    }
    std::cerr << "not-established: offline mock, no captured fields\n";
    return dry_run ? 0 : 1;
}
