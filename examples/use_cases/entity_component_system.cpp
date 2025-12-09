// Copyright 2025 Dr. Matthias Hölzl
#include <algorithm>
#include <functional>
#include <iostream>
#include "../../include/inline_poly.h"

// Example: Entity Component System pattern with fixed-size component storage

struct Component
{
    virtual ~Component()                           = default;
    virtual void update(float dt)                  = 0;
    [[nodiscard]] virtual const char* type() const = 0;
};

struct PositionComponent final : Component
{
    float x, y;
    float vx, vy;

    PositionComponent(float x, float y, float vx = 0, float vy = 0) :
        x{x}, y{y}, vx{vx}, vy{vy}
    {}

    void update(float dt) override
    {
        x += vx * dt;
        y += vy * dt;
    }

    [[nodiscard]] const char* type() const override
    {
        return "Position";
    }
};

struct HealthComponent final : Component
{
    int current;
    int max;

    explicit HealthComponent(int max) : current{max}, max{max} {}

    void update(float /*dt*/) override
    {
        // Health regeneration could go here
    }

    [[nodiscard]] const char* type() const override
    {
        return "Health";
    }

    void damage(int amount)
    {
        current = std::max(0, current - amount);
        std::cout << "Taking damage...\n";
        std::cout << "Health: " << current << "/" << max << std::endl;
    }

    [[nodiscard]] bool alive() const
    {
        return current > 0;
    }
};

struct TimerComponent final : Component
{
    float                 elapsed;
    float                 duration;
    std::function<void()> on_trigger;
    bool                  triggered;

    explicit TimerComponent(
        float duration, std::function<void()> on_trigger = []() {}) :
        elapsed{0}, duration{duration}, on_trigger{std::move(on_trigger)},
        triggered{false}
    {}

    void update(float dt) override
    {
        elapsed += dt;
        if (elapsed >= duration && !triggered)
        {
            on_trigger();
            triggered = true;
        }
    }

    [[nodiscard]] const char* type() const override
    {
        return "Timer";
    }
};

// Use type_list to compute slot configuration at compile time
using ComponentTypes =
    inline_poly::type_list<PositionComponent, HealthComponent, TimerComponent>;
using ComponentConfig = inline_poly::slot_config<ComponentTypes>;

// An entity with a fixed number of component slots
class Entity
{
public:
    static constexpr size_t MaxComponents = 4;

    template <typename T, typename... Args>
    T* addComponent(Args&&... args)
    {
        for (size_t i = 0; i < components_.size(); ++i)
        {
            if (components_[i] == nullptr)
            {
                return components_.emplace<T>(i, std::forward<Args>(args)...);
            }
        }
        return nullptr; // No free slot
    }

    void update(float dt)
    {
        for (Component* comp : components_)
        {
            if (comp != nullptr)
            {
                comp->update(dt);
            }
        }
    }

    void printComponents() const
    {
        std::cout << "Components:\n";
        for (size_t i = 0; i < components_.size(); ++i)
        {
            if (components_[i] != nullptr)
            {
                std::cout << "  [" << i << "] " << components_[i]->type() << "\n";
            }
        }
    }

    // Access component by index
    Component* operator[](size_t i)
    {
        return components_[i];
    }

    Component* at(size_t i)
    {
        return components_.at(i);
    }

    template <typename T>
    T* at(size_t i)
    {
        return dynamic_cast<T*>(components_.at(i));
    }

    template <typename T>
    T* get()
    {
        for (Component* comp : components_)
        {
            if (auto* typedComp = dynamic_cast<T*>(comp))
            {
                return typedComp;
            }
        }
        return nullptr;
    }

private:
    inline_poly::poly_array<Component, MaxComponents, ComponentConfig::size,
                            ComponentConfig::alignment>
        components_;
};

int main()
{
    std::cout << "=== Entity Component Example ===\n\n";
    std::cout << "Max component size: " << ComponentConfig::size << " bytes\n";
    std::cout << "Max component alignment: " << ComponentConfig::alignment
              << " bytes\n\n";

    Entity player;

    auto damagePlayer = [&player]()
    {
        if (auto* healthComp = player.get<HealthComponent>())
        {
            healthComp->damage(10);
        }
    };

    // Add components
    const auto* pos =
        player.addComponent<PositionComponent>(0.0f, 0.0f, 1.0f, 0.5f);
    auto*       health = player.addComponent<HealthComponent>(100);
    const auto* timer  = player.addComponent<TimerComponent>(2.0f, damagePlayer);

    player.printComponents();

    // Simulate a few update frames
    std::cout << "\nSimulating updates...\n";
    for (int frame = 0; frame < 5; ++frame)
    {
        player.update(0.5f);

        std::cout << "Frame " << frame << ": "
                  << "pos=(" << pos->x << ", " << pos->y << ") "
                  << "timer=" << timer->elapsed << "s "
                  << (timer->triggered ? "[TRIGGERED]" : "") << "\n";
    }

    // Damage the health component
    health->damage(30);

    // Using checked access
    try
    {
        std::cout << "\nAccessing out-of-bounds component...\n";
        // Deliberately access an out-of-bounds value
        for (size_t i = 0; i < Entity::MaxComponents + 1; ++i)
        {
            auto* comp = player.at(i);
            std::cout << "Component at index " << i << ": "
                      << (comp ? comp->type() : "nullptr") << "\n";
        }
    }
    catch (const std::out_of_range& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }

    std::cout << "\n=== Done ===\n";
    return 0;
}
