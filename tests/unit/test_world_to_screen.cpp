#include "ovr/render/world_to_screen.hpp"
#include <cmath>
using namespace ovr;
int main() {
    render::View v{{{1,0,0,-1, 0,1,0,0, 0,0,-1,-1, 0,0,-1,0}}, {1,0,0}, 90, 2, 1, 10, true, 200, 100};
    core::Vec2 p{};
    core::Vec3 dir{};
    if (!render::world_to_screen(v, {2,1,-2}, p) || p.x != 150 || p.y != 25) return 1;
    if (!render::screen_to_world_ray(v,p,dir)) return 2;
    const auto norm = std::sqrt(6.f);
    if (std::abs(dir.x*norm-1)>1e-5 || std::abs(dir.y*norm-1)>1e-5 || std::abs(dir.z*norm+2)>1e-5) return 3;
    if (render::world_to_screen(v,{1,0,2},p) || render::world_to_screen(v,{20,0,-1},p)
        || p.x != 150 || p.y != 25) return 4;
    v.matrix = {{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}};
    if (!render::world_to_screen(v,{0,0,-1000},p) || p.x!=100 || p.y!=50) return 5;
    v.valid = false;
    if (render::screen_to_world_ray(v,p,dir) || render::world_to_screen(v,{},p)) return 6;
    v.valid = true;
    v.znear = v.zfar;
    return render::world_to_screen(v,{},p) ? 7 : 0;
}
