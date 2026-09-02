#include "CollisionDetector.h"

namespace qix {

CollisionEvent CollisionDetector::check(const Marker& marker, const std::vector<Qix>& qixList,
    const std::vector<Sparx>& sparxList, const Fuse& fuse) noexcept
{
    const auto markerPos = marker.getPosition();

    // 1. Audit Sparx collisions against marker
    for (const auto& sparx : sparxList) {
        if (sparx.checkCollision(markerPos)) {
            return CollisionEvent::MarkerHitBySparx;
        }
    }

    // 2. Audit Fuse collision if actively drawing
    if (marker.isDrawing() && fuse.checkCollision(markerPos)) {
        return CollisionEvent::MarkerHitByFuse;
    }

    // 3. Audit Qix collision with active Stix trail
    if (marker.isDrawing()) {
        const auto& trail = marker.getTrail();
        for (const auto& qix : qixList) {
            if (qix.intersectsTrail(trail)) {
                return CollisionEvent::StixHitByQix;
            }
        }
    }

    return CollisionEvent::None;
}

} // namespace qix
