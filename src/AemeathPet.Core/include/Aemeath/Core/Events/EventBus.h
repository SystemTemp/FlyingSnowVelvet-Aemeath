#pragma once

#include <any>
#include <functional>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <cstddef>
#include <algorithm>

namespace Aemeath::Core::Events {

class Subscription final {
public:
    Subscription() = default;
    explicit Subscription(std::function<void()> unsubscribe) : m_unsubscribe(std::move(unsubscribe)) {}
    ~Subscription() { Unsubscribe(); }

    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;

    Subscription(Subscription&& other) noexcept : m_unsubscribe(std::move(other.m_unsubscribe)) {
        other.m_unsubscribe = nullptr;
    }

    Subscription& operator=(Subscription&& other) noexcept {
        if (this != &other) {
            Unsubscribe();
            m_unsubscribe = std::move(other.m_unsubscribe);
            other.m_unsubscribe = nullptr;
        }
        return *this;
    }

    void Unsubscribe() {
        if (m_unsubscribe) {
            m_unsubscribe();
            m_unsubscribe = nullptr;
        }
    }

private:
    std::function<void()> m_unsubscribe;
};

class EventBus final {
public:
    template <typename TEvent>
    Subscription Subscribe(std::function<void(const TEvent&)> handler) {
        std::lock_guard lock(m_mutex);
        const auto key = std::type_index(typeid(TEvent));
        const std::size_t token = ++m_nextToken;
        m_handlers[key].push_back(HandlerEntry{
            token,
            [handler = std::move(handler)](const std::any& payload) {
                handler(std::any_cast<const TEvent&>(payload));
            }
        });

        return Subscription([this, key, token]() {
            std::lock_guard innerLock(m_mutex);
            auto it = m_handlers.find(key);
            if (it == m_handlers.end()) {
                return;
            }
            auto& list = it->second;
            list.erase(std::remove_if(list.begin(), list.end(), [token](const HandlerEntry& entry) {
                return entry.token == token;
            }), list.end());
        });
    }

    template <typename TEvent>
    void Publish(const TEvent& event) {
        std::vector<HandlerEntry> snapshot;
        {
            std::lock_guard lock(m_mutex);
            const auto key = std::type_index(typeid(TEvent));
            auto it = m_handlers.find(key);
            if (it == m_handlers.end()) {
                return;
            }
            snapshot = it->second;
        }

        const std::any payload = event;
        for (const auto& entry : snapshot) {
            entry.callback(payload);
        }
    }

private:
    struct HandlerEntry {
        std::size_t token {};
        std::function<void(const std::any&)> callback;
    };

    std::mutex m_mutex;
    std::size_t m_nextToken { 0 };
    std::unordered_map<std::type_index, std::vector<HandlerEntry>> m_handlers;
};

} // namespace Aemeath::Core::Events
