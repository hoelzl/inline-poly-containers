// Copyright 2025 Dr. Matthias Hölzl
// Demonstration of inline_poly containers with an Animal hierarchy
// Shows the difference between array (fixed layout) and vector (dynamic) usage

#include <iostream>
#include <string>
#include "../../include/inline_poly.h"

// === Animal Hierarchy ===

class Animal
{
public:
    explicit Animal(std::string name) : name_{std::move(name)} {}

    // Movable but not copyable
    Animal(Animal&&) noexcept            = default;
    Animal& operator=(Animal&&) noexcept = default;
    Animal(const Animal&)                = delete;
    Animal& operator=(const Animal&)     = delete;

    virtual ~Animal() = default;

    virtual void print_food_requirements() const
    {
        std::cout << name_ << " needs: ";
    }

    const std::string& get_name() const
    {
        return name_;
    }

protected:
    std::string name_;
};

class Elephant : public Animal
{
public:
    Elephant() : Animal{"Elephant"} {}

    void print_food_requirements() const override
    {
        Animal::print_food_requirements();
        std::cout << "hay, fruit, vegetables (300kg/day)";
    }
};

class Zebra : public Animal
{
public:
    Zebra() : Animal{"Zebra"} {}

    void print_food_requirements() const override
    {
        Animal::print_food_requirements();
        std::cout << "hay, grass (15kg/day)";
    }
};

class Lion : public Animal
{
public:
    Lion() : Animal{"Lion"} {}

    void print_food_requirements() const override
    {
        Animal::print_food_requirements();
        std::cout << "meat (11kg/day)";
    }
};

class Penguin : public Animal
{
public:
    Penguin() : Animal{"Penguin"} {}

    void print_food_requirements() const override
    {
        Animal::print_food_requirements();
        std::cout << "fish (2kg/day)";
    }
};

// === Demo 1: Fixed Zoo Layout using poly_array ===
void demo_fixed_layout()
{
    std::cout << "=== DEMO 1: Fixed Zoo Layout (using poly_array) ===\n";
    std::cout << "Each enclosure has a fixed position in the zoo.\n\n";

    // Fixed layout with 10 enclosures
    inline_poly::poly_array<Animal, 10, sizeof(Elephant)> enclosures;

    // Assign animals to specific enclosures
    std::cout << "Initial zoo layout:\n";
    enclosures.emplace<Lion>(0);     // Enclosure 0: Lion exhibit
    enclosures.emplace<Zebra>(2);    // Enclosure 2: Zebra habitat
    enclosures.emplace<Elephant>(5); // Enclosure 5: Elephant area
    enclosures.emplace<Penguin>(9);  // Enclosure 9: Penguin pool

    // Display zoo map
    std::cout << "\nZoo Map (10 enclosures):\n";
    std::cout << "------------------------\n";
    for (size_t i = 0; i < enclosures.size(); ++i)
    {
        std::cout << "Enclosure " << i << ": ";
        if (enclosures[i] != nullptr)
        {
            std::cout << enclosures[i]->get_name() << "\n";
        }
        else
        {
            std::cout << "[Empty]\n";
        }
    }

    // Food requirements report
    std::cout << "\nDaily Food Requirements:\n";
    std::cout << "------------------------\n";
    for (const auto* animal : enclosures)
    {
        if (animal != nullptr)
        {
            animal->print_food_requirements();
            std::cout << "\n";
        }
    }

    // Renovate enclosure 2 (replace Zebra with Elephant)
    std::cout << "\nRenovating Enclosure 2 for a new Elephant...\n";
    enclosures.emplace<Elephant>(2);

    std::cout << "Updated Enclosure 2: ";
    enclosures[2]->print_food_requirements();
    std::cout << "\n\n";
}

// === Demo 2: Dynamic Animal List using poly_vector ===
void demo_dynamic_list()
{
    std::cout << "=== DEMO 2: Dynamic Animal List (using poly_vector) ===\n";
    std::cout << "Animals can be added/removed dynamically.\n\n";

    // Dynamic list with capacity for 20 animals
    inline_poly::poly_vector<Animal, 20, sizeof(Elephant)> animal_roster;

    // Add animals as they arrive at the zoo
    std::cout << "Animals arriving at the zoo:\n";

    animal_roster.emplace_back<Lion>();
    std::cout << "  " << animal_roster.back()->get_name() << " arrived\n";

    animal_roster.emplace_back<Zebra>();
    std::cout << "  " << animal_roster.back()->get_name() << " arrived\n";

    animal_roster.emplace_back<Elephant>();
    std::cout << "  " << animal_roster.back()->get_name() << " arrived\n";

    animal_roster.emplace_back<Penguin>();
    std::cout << "  " << animal_roster.back()->get_name() << " arrived\n";

    std::cout << "\nCurrent roster size: " << animal_roster.size()
              << " animals\n";

    // Food ordering system
    std::cout << "\nFood Order List:\n";
    std::cout << "----------------\n";
    for (size_t i = 0; i < animal_roster.size(); ++i)
    {
        std::cout << i + 1 << ". ";
        animal_roster[i]->print_food_requirements();
        std::cout << "\n";
    }

    // Transfer an animal (remove from list)
    std::cout << "\nTransferring the Zebra (position 2) to another zoo...\n";
    animal_roster.erase(animal_roster.begin() + 1);

    std::cout << "Updated roster size: " << animal_roster.size() << " animals\n";

    // Add multiple animals at once
    std::cout << "\nNew arrivals from wildlife rescue:\n";
    animal_roster.emplace_back<Zebra>();
    animal_roster.emplace_back<Zebra>();
    animal_roster.emplace_back<Lion>();
    std::cout << "  Added 2 Zebras and 1 Lion\n";

    std::cout << "Final roster size: " << animal_roster.size() << " animals\n";
    std::cout << "Remaining capacity: "
              << (animal_roster.capacity() - animal_roster.size())
              << " animals\n\n";

    // Final food summary
    std::cout << "Final Daily Food Summary:\n";
    std::cout << "-------------------------\n";
    int meat_count = 0, hay_count = 0, fish_count = 0;
    for (const auto* animal : animal_roster)
    {
        if (dynamic_cast<const Lion*>(animal))
            meat_count++;
        else if (dynamic_cast<const Zebra*>(animal))
            hay_count++;
        else if (dynamic_cast<const Elephant*>(animal))
            hay_count++;
        else if (dynamic_cast<const Penguin*>(animal))
            fish_count++;
    }

    std::cout << "  Meat eaters: " << meat_count << " animals\n";
    std::cout << "  Hay eaters: " << hay_count << " animals\n";
    std::cout << "  Fish eaters: " << fish_count << " animals\n";
}

// === Main ===
int main()
{
    std::cout << "Animal Food Requirements Management System\n";
    std::cout << "==========================================\n\n";

    demo_fixed_layout();

    std::cout << "\n";

    demo_dynamic_list();

    std::cout << "\n=== Key Differences ===\n";
    std::cout << "- array: Best for fixed layouts where position matters\n";
    std::cout << "- vector: Best for dynamic lists that grow/shrink\n";
    std::cout << "- Both: Zero heap allocations, full polymorphism\n";

    return 0;
}
