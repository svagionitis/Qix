#ifndef QIX_LIB_COLLISION_DETECTOR_H
#define QIX_LIB_COLLISION_DETECTOR_H

#include "Fuse.h"
#include "Marker.h"
#include "Qix.h"
#include "Sparx.h"
#include <vector>

namespace qix {

/// @brief Possible fatal collision events during a tick.
enum class CollisionEvent : std::uint8_t { None = 0, MarkerHitBySparx, MarkerHitByFuse, StixHitByQix };

/// @class CollisionDetector
/// @brief Discrete spatial collision system auditing interactions between game entities.
class CollisionDetector {
public:
    /// @brief Check for any fatal collisions between player and hazards.
    /// @param[in] marker Active player marker.
    /// @param[in] qixList Collection of active Qix entities.
    /// @param[in] sparxList Collection of active Sparx entities.
    /// @param[in] fuse Active fuse entity.
    /// @return CollisionEvent indicating outcome.
    [[nodiscard]] static CollisionEvent check(const Marker& marker, const std::vector<Qix>& qixList,
        const std::vector<Sparx>& sparxList, const Fuse& fuse) noexcept;
};

} // namespace qix

#endif // QIX_LIB_COLLISION_DETECTOR_H
