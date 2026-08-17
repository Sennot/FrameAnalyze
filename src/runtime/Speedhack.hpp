#pragma once

namespace fwa {

class Speedhack {
public:
    void setEnabled(bool value);
    void setSpeed(float value);
    void setAudioFollow(bool value);

    // Re-applies pitch to the current FMOD master group. Calling this from the
    // scheduler makes audio changes immediate and resilient to attempt restarts.
    void syncAudio(bool allowFollow = true) const;
    void resetAudio() const;

    [[nodiscard]] bool enabled() const { return m_enabled; }
    [[nodiscard]] bool audioFollow() const { return m_audioFollow; }
    [[nodiscard]] float speed() const { return m_speed; }
    [[nodiscard]] float effectiveSpeed() const { return m_enabled ? m_speed : 1.0f; }

private:
    bool m_enabled = false;
    bool m_audioFollow = true;
    float m_speed = 1.0f;
};

} // namespace fwa
