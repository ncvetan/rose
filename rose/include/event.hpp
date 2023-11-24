#ifndef ROSE_INCLUDE_EVENT
#define ROSE_INCLUDE_EVENT

#include <array>
#include <optional>

namespace rose {

enum class EventType {
    NONE = 0,
    EVENT_QUIT,
    EVENT_KEY_W,
    EVENT_KEY_A,
    EVENT_KEY_S,
    EVENT_KEY_D,
    EVENT_KEY_Q,
    EVENT_KEY_E,
    EVENT_KEY_SPACE,
    EVENT_KEY_L_SHIFT,
    INVALID
};

struct Event {
    EventType type = EventType::INVALID;

    Event();
    Event(EventType type);
    ~Event();
    // Event(const Event& other) = delete;
    // Event& operator=(const Event& other) = delete;
    Event(Event&& other) noexcept;
    Event& operator=(Event&& other) noexcept;
};

struct EventQueue {
    EventQueue(){};

    static constexpr int N = 512;

    std::array<Event, N> queue;

    std::optional<Event> poll_event();

    void push_event(Event& event);

    void push_event(Event&& event);

    int write_idx = 0;
    int read_idx = 0;
};

} // namespace rose

#endif