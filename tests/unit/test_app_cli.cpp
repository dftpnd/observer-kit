#include "../../src/app/app.hpp"
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
namespace {
bool verify(std::initializer_list<std::string> input, int code, const std::string& expected) {
    std::vector<std::string> args(input);
    std::vector<char*> argv;
    for (auto& arg : args) argv.push_back(arg.data());
    std::ostringstream captured;
    auto* before = std::cout.rdbuf(captured.rdbuf());
    const int result = ovr::app::run(static_cast<int>(argv.size()), argv.data());
    std::cout.rdbuf(before);
    if (result == code && captured.str().find(expected) != std::string::npos) return true;
    std::cerr << "unexpected CLI result " << result << " output " << captured.str() << '\n';
    return false;
}
}
int main() {
    return !(verify({"ovr","--help"},0,"--once") &&
             verify({"ovr","--once","3","--window","800x600"},0,"frame=3 mode=offline primitives=0 window=800x600\n") &&
             verify({"ovr","--build","6985","--config","tests/fixtures/config/good.json"},0,"frame=1") &&
             verify({"ovr","--build","123"},1,"") &&
             verify({"ovr","--config","tests/fixtures/config/bad_type.json"},2,"") &&
             verify({"ovr","--window","0x300"},2,"") && verify({"ovr","--once","0"},2,"") &&
             verify({"ovr","--once","3oops"},2,"") && verify({"ovr","--unknown"},2,""));
}
