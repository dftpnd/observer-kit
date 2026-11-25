#include "ovr/host_env.hpp"

int main() {
    static_cast<void>(ovr::host_env::is_available());
    return 0;
}
