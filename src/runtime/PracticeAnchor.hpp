#pragma once

#include "core/Types.hpp"

#include <Geode/Geode.hpp>

namespace fwa {

class PracticeAnchor {
public:
    ~PracticeAnchor();

    bool capture(PlayLayer* layer);
    bool captureSupplemental(PlayLayer* layer);
    void clear();
    [[nodiscard]] bool validFor(PlayLayer* layer) const;
    [[nodiscard]] bool restoreNow(PlayLayer* layer) const;
    void applySupplemental(PlayLayer* layer) const;
    [[nodiscard]] CheckpointObject* checkpoint() const { return m_checkpoint; }
    [[nodiscard]] AnchorInputState const& inputs() const { return m_inputs; }

private:
    CheckpointObject* m_checkpoint = nullptr;
    PlayLayer* m_owner = nullptr;
    AnchorInputState m_inputs;
    GJGameState m_gameState{};
    EffectManagerState m_effectState{};
    bool m_hasSupplemental = false;
};

} // namespace fwa
