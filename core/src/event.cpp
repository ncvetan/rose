#include <event.hpp>
#include <cassert>

namespace rose
{
    Event::Event(){};

    Event::Event(EventType type) : type(type){};

    Event::~Event()
    {
        switch (type)
        {
        case EventType::NONE:
            break;
        case EventType::EVENT_KEY_W:
            break;
        case EventType::EVENT_KEY_A:
            break;
        case EventType::EVENT_KEY_S:
            break;
        case EventType::EVENT_KEY_D:
            break;
        case EventType::EVENT_KEY_Q:
            break;
        case EventType::EVENT_KEY_E:
            break;
        case EventType::EVENT_KEY_SPACE:
            break;
        case EventType::EVENT_KEY_L_SHIFT:
            break;
        case EventType::INVALID:
            break;
        default:
            assert(false);
        }
    }

    Event::Event(Event&& other) noexcept
    {
        type = other.type;
        switch (type)
        {
        case EventType::NONE:
            break;
        case EventType::EVENT_KEY_W:
            break;
        case EventType::EVENT_KEY_A:
            break;
        case EventType::EVENT_KEY_S:
            break;
        case EventType::EVENT_KEY_D:
            break;
        case EventType::EVENT_KEY_Q:
            break;
        case EventType::EVENT_KEY_E:
            break;
        case EventType::EVENT_KEY_SPACE:
            break;
        case EventType::EVENT_KEY_L_SHIFT:
            break;
        case EventType::INVALID:
            break;
        default:
            assert(false);
        }
    }

    Event& Event::operator=(Event&& other) noexcept
    {
        if (this == &other) return *this;
        this->~Event();
        new (this) Event(std::move(other));
        return *this;
    }

    std::optional<Event> EventQueue::poll_event()
    {
        if (read_idx == write_idx) return std::nullopt;
        Event result = std::move(queue[read_idx]);
        read_idx = (read_idx + 1) % N;
        return result;
    }

    void EventQueue::push_event(Event& event)
    {
        if (event.type == EventType::INVALID) return; // Don't push events that wont be handled
        queue[write_idx] = std::move(event);
        write_idx = (write_idx + 1) % N;
    }

    void EventQueue::push_event(Event&& event)
    {
        if (event.type == EventType::INVALID) return;
        queue[write_idx] = std::move(event);
        write_idx = (write_idx + 1) % N;
    }
} // namespace rose