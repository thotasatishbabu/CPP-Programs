# C++ Factory Design Pattern — Senior Interview Guide (15 Years' Experience)

## 1. Definition

The **Factory Pattern** is a creational design pattern that encapsulates object-creation logic and provides a common way for client code to obtain objects without directly depending on their concrete classes.

The client asks for an abstraction (for example, `Food`) and the factory decides which concrete implementation (such as `Pizza` or `Burger`) to create.

**Core idea:** Separate *object creation* from *object usage*.

### Real-world analogy

At a restaurant counter, you order a pizza or burger. You do not need to know which kitchen station, recipe, or preparation steps are involved. The kitchen handles creation and gives you the finished food.

In software, the client uses the common interface and does not need to construct the concrete class itself.

---

## 2. Your original code

```cpp
#include <iostream>
#include <string>
using namespace std;

class Food
{
public:
    virtual void eat() = 0;
    virtual ~Food() = default;
};

class Pizza : public Food
{
public:
    void eat() override
    {
        cout << "eating pizza\n";
    }
};

class Burger : public Food
{
public:
    void eat() override
    {
        cout << "eating burger\n";
    }
};

Food* order(const string& type)
{
    if (type == "pizza")
    {
        return new Pizza();
    }

    return new Burger();
}

int main()
{
    Food* f = order("pizza");
    f->eat();

    delete f;
    return 0;
}
```

### What happens?

1. `Food` defines the abstraction through a pure virtual `eat()` method.
2. `Pizza` and `Burger` provide concrete implementations.
3. `order()` contains the object-creation decision.
4. `main()` asks for a `Food` and calls `eat()` through the base-class interface.
5. The client does not directly call `new Pizza()` or `new Burger()`.

### Important corrections to the original example

- Add a **virtual destructor** to `Food`. Deleting a derived object through a base pointer without a virtual base destructor causes undefined behavior.
- Check the input type. The original code silently returns a `Burger` for every value other than `"pizza"`, including invalid values.
- Define ownership. The original function returns a raw owning pointer, so the caller must remember to `delete` it. Prefer `std::unique_ptr`.
- Use `override` on derived virtual methods to catch signature mistakes.
- Use `const std::string&` to avoid copying the input string.
- Consider whether the example is a simple factory function or the formal **Factory Method** pattern. The term “Factory Pattern” is often used broadly, but these are distinct patterns.

---

## 3. Recommended modern C++ implementation

This version keeps the same simple factory-function idea but uses `std::unique_ptr` for automatic resource management and explicitly handles invalid input.

```cpp
#include <iostream>
#include <memory>
#include <string>
#include <stdexcept>

class Food
{
public:
    virtual void eat() const = 0;
    virtual ~Food() = default;
};

class Pizza final : public Food
{
public:
    void eat() const override
    {
        std::cout << "Eating pizza\n";
    }
};

class Burger final : public Food
{
public:
    void eat() const override
    {
        std::cout << "Eating burger\n";
    }
};

std::unique_ptr<Food> orderFood(const std::string& type)
{
    if (type == "pizza")
    {
        return std::make_unique<Pizza>();
    }

    if (type == "burger")
    {
        return std::make_unique<Burger>();
    }

    throw std::invalid_argument("Unsupported food type: " + type);
}

int main()
{
    try
    {
        std::unique_ptr<Food> food = orderFood("pizza");
        food->eat();
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Order failed: " << ex.what() << '\n';
    }

    return 0;
}
```

### Why this version is better

- **Polymorphism:** Client code depends on `Food`, not on `Pizza` or `Burger`.
- **Encapsulated creation:** The factory owns the decision about which concrete type to instantiate.
- **RAII:** `std::unique_ptr` automatically destroys the object when it goes out of scope.
- **Safe deletion:** The virtual destructor supports destruction through a `Food` pointer.
- **Explicit invalid-input behavior:** Unsupported values are not silently treated as burgers.
- **Extensibility:** New food types can be added without changing the client code, although the factory itself may need modification.

---

## 4. Interview explanation (about 90 seconds)

> The Factory Pattern is a creational design pattern that separates object creation from object usage. The client requests an object through a common abstraction, and the factory encapsulates the decision about which concrete implementation to instantiate.
>
> In my example, `Food` is the abstract base class, and `Pizza` and `Burger` are concrete implementations. The `orderFood()` factory function accepts a type and creates the corresponding object. The client receives a `Food` interface and invokes `eat()` without needing to know the concrete class.
>
> The main benefit is loose coupling. The calling code depends on the abstraction rather than directly constructing concrete classes. It also centralizes creation logic, making it easier to add validation, configuration, logging, caching, or dependency setup in one place.
>
> In production C++, I would return `std::unique_ptr<Food>` rather than a raw owning pointer, provide a virtual destructor in the base class, and handle unsupported factory requests explicitly.
>
> I would also be precise about the pattern terminology: this example is a simple factory function. The GoF Factory Method pattern uses a creator abstraction and lets subclasses decide which product to construct. For families of related products, I would consider Abstract Factory.

**Senior-level delivery:** Explain not only what the factory does, but why it reduces coupling, how ownership is managed, how it evolves as product types grow, and which factory variant is appropriate.

---

## 5. Architecture and object flow

```text
Client
  |
  | orderFood("pizza")
  v
Factory Function
  |
  | decides concrete type
  +----------------------+
  |                      |
  v                      v
Pizza                  Burger
  |                      |
  +----------+-----------+
             |
             v
        Food interface
             |
             v
          eat()
```

The important dependency direction is that client code uses the `Food` abstraction. It does not need to know the construction details of `Pizza` or `Burger`.

---

## 6. Why use a Factory?

Without a factory, client code may contain repeated concrete construction decisions:

```cpp
Food* food = nullptr;

if (type == "pizza")
    food = new Pizza();
else if (type == "burger")
    food = new Burger();
```

If many parts of an application repeat these decisions, adding a new type can require changes in many places.

With a factory:

```cpp
std::unique_ptr<Food> food = orderFood(type);
food->eat();
```

The creation decision is centralized.

### Benefits

- Encapsulates object creation.
- Reduces direct coupling to concrete classes.
- Provides one place for validation and construction rules.
- Can centralize configuration, logging, metrics, or dependency setup.
- Makes it easier to substitute implementations.
- Helps keep client/business logic focused on behavior rather than construction.

### Trade-offs

- Adds another abstraction and can be unnecessary for a tiny, stable set of types.
- A large conditional factory can become a maintenance bottleneck.
- Adding a new type may still require modifying the factory.
- Overuse can lead to excessive indirection and hard-to-follow creation flows.

---

## 7. Advanced interview questions and answers

### Q1. What problem does the Factory Pattern solve?

**Answer:** It separates object creation from object usage. Client code depends on an abstraction and delegates the choice and construction of a concrete type to a factory. This reduces coupling and centralizes creation rules.

### Q2. Is your `order()` function the GoF Factory Method pattern?

**Answer:** Strictly speaking, it is a simple factory function, sometimes informally called a factory pattern. The GoF Factory Method pattern defines a creator abstraction with a factory method, and subclasses override that method to choose the concrete product. I would clarify the terminology rather than claim the example implements every GoF factory variant.

### Q3. What is the difference between Simple Factory, Factory Method, and Abstract Factory?

**Answer:**

- **Simple Factory:** A function or class centralizes a selection decision and creates one of several concrete products. It is a useful idiom, but not one of the original GoF patterns.
- **Factory Method:** A creator interface or base class declares a creation method; derived creators decide which concrete product to create.
- **Abstract Factory:** An interface creates families of related products without exposing their concrete classes.

### Q4. What is the difference between Factory and Abstract Factory?

**Answer:** A basic factory usually creates one product from a set of alternatives. Abstract Factory creates a family of related products designed to work together. For example, a Windows UI factory could create a Windows button and Windows checkbox, while a Linux UI factory creates matching Linux controls.

### Q5. Why return a base-class pointer or smart pointer?

**Answer:** It enables runtime polymorphism. The caller uses the common interface without knowing the concrete derived type. In modern C++, `std::unique_ptr<Base>` clearly expresses exclusive ownership and automatically destroys the derived object, provided the base destructor is virtual.

### Q6. Why must the base class have a virtual destructor?

**Answer:** When an object is deleted through a pointer to a base class, the base destructor must be virtual so the derived destructor is invoked correctly. Without it, deleting a derived object through a base pointer has undefined behavior.

### Q7. Why prefer `std::unique_ptr` over a raw pointer?

**Answer:** `std::unique_ptr` expresses ownership, provides deterministic cleanup through RAII, and prevents accidental leaks on early returns or exceptions. A raw pointer can still be used as a non-owning observer, but ownership should be explicit.

### Q8. What happens if an unsupported type is passed?

**Answer:** The factory should define a clear contract. It could throw an exception, return `nullptr`, return an expected/error result, or use a registered fallback if the business rules require one. Silently returning a Burger for an unknown string is dangerous because it hides input errors.

### Q9. Does Factory automatically follow the Open/Closed Principle?

**Answer:** Not automatically. A conditional factory that must be edited every time a new product is added has a modification point. The overall design may still be well-factored, but if new types must be added independently, a registration mechanism, dependency injection, or polymorphic creator hierarchy may be more suitable.

### Q10. How would you avoid a large `if/else` or `switch` in a factory?

**Answer:** For a stable, small set of types, a switch is often clear and maintainable. For extensible systems, use a registry mapping keys to creator functions, dependency injection, or plugin registration. The choice depends on whether types are known at compile time or must be added at runtime.

### Q11. How would you implement a registry-based factory?

**Answer:** Store creator callables, such as `std::function<std::unique_ptr<Food>()>`, in a map keyed by a type identifier. Register each supported creator and invoke the matching callable when requested. Define duplicate-registration behavior, thread safety, plugin lifetime, and error handling.

### Q12. Is a factory thread-safe?

**Answer:** The pattern itself does not guarantee thread safety. A stateless factory that only creates independent local objects may need no synchronization. A factory with a shared mutable registry or cache needs appropriate synchronization around registration and lookup. The created objects have their own thread-safety contracts.

### Q13. Can a factory return cached or shared objects?

**Answer:** Yes, if the requirements call for reuse. It could return `std::shared_ptr`, a reference, or a handle to a cached object. But caching changes ownership and lifecycle semantics, and the cache needs concurrency and invalidation policies. Do not use shared ownership unless it is actually required.

### Q14. How do you unit-test a factory?

**Answer:** Test each supported key, invalid keys, construction failures, and the concrete behavior exposed through the interface. For consumers, inject a factory interface or creator function so tests can provide fake products without invoking real construction or external dependencies.

### Q15. How would you add a new food type without modifying the factory?

**Answer:** A registration-based factory can allow a new module to register a creator under a key. This moves extension to registration rather than a central conditional. It introduces concerns such as registration order, duplicate keys, static initialization order, and plugin unloading, so it should be used only when runtime extensibility is a real requirement.

### Q16. What are the risks of using strings such as `"pizza"` as factory keys?

**Answer:** Strings are vulnerable to spelling mistakes, inconsistent casing, runtime-only errors, and unclear discoverability. An enum or strongly typed identifier is better for a closed set of types. A string or GUID can be appropriate for configuration-driven or plugin-based systems, with validation.

### Q17. What if object construction requires dependencies?

**Answer:** The factory can receive dependencies through constructor parameters or be composed with a dependency-injection container. It should not secretly create a large dependency graph or become a service locator. Construction dependencies should remain explicit and testable.

### Q18. What if a constructor throws?

**Answer:** The factory should allow the exception to propagate or translate it into the documented error type. With `std::make_unique`, allocation and construction are exception-safe: if construction throws, the allocation is released. The client must still define how it handles failure.

### Q19. How would you design a factory for a plugin architecture?

**Answer:** Define a stable product interface and an explicit registration/export contract. Each plugin registers a creator or exposes a known entry point. Consider ABI compatibility, compiler/runtime boundaries, ownership across DLL boundaries, plugin unloading, versioning, thread safety, and error reporting. Avoid passing standard-library types across incompatible binary boundaries unless the toolchain and ABI are controlled.

### Q20. How is Factory different from Dependency Injection?

**Answer:** A factory is responsible for creating or selecting an object. Dependency Injection is a broader technique in which dependencies are supplied to a component rather than constructed internally. A factory can be injected into a client, and a dependency-injection container may use factories internally.

### Q21. How is Factory different from Builder?

**Answer:** Factory focuses on selecting and creating an object. Builder focuses on constructing a complex object step by step, often with multiple optional configuration choices. A factory may use a builder internally.

### Q22. How is Factory different from Prototype?

**Answer:** Factory creates an object using construction logic. Prototype creates a new object by cloning an existing prototype. Prototype is useful when the concrete type or configuration is already represented by an existing object.

### Q23. Would you use a factory for two classes only?

**Answer:** Not automatically. If construction is trivial and there is only one caller, direct construction may be simpler. A factory is valuable when it hides meaningful variation, centralizes policy, or prevents widespread dependencies on concrete types.

### Q24. How do you prevent a factory from becoming a God object?

**Answer:** Keep it focused on a cohesive product family or creation responsibility. Split factories by domain or product family, keep business behavior in the products/services, and avoid making the factory responsible for unrelated workflows.

### Q25. How would you evolve this example for a large enterprise application?

**Answer:** I would first define stable interfaces and explicit ownership. Then I would choose a simple factory for a closed set of products, a creator hierarchy for Factory Method, or Abstract Factory for product families. If runtime extension is needed, I would add a registry/plugin contract. I would also include validation, structured errors, observability, dependency injection, tests, and concurrency/lifetime rules where appropriate.

---

## 8. Factory Method example

This illustrates the formal GoF **Factory Method** structure: the base creator declares a virtual creation operation, and concrete creators choose which product to instantiate.

```cpp
#include <iostream>
#include <memory>

class Food
{
public:
    virtual void eat() const = 0;
    virtual ~Food() = default;
};

class Pizza final : public Food
{
public:
    void eat() const override
    {
        std::cout << "Eating pizza\n";
    }
};

class Burger final : public Food
{
public:
    void eat() const override
    {
        std::cout << "Eating burger\n";
    }
};

class FoodCreator
{
public:
    virtual std::unique_ptr<Food> createFood() const = 0;
    virtual ~FoodCreator() = default;
};

class PizzaCreator final : public FoodCreator
{
public:
    std::unique_ptr<Food> createFood() const override
    {
        return std::make_unique<Pizza>();
    }
};

class BurgerCreator final : public FoodCreator
{
public:
    std::unique_ptr<Food> createFood() const override
    {
        return std::make_unique<Burger>();
    }
};

int main()
{
    PizzaCreator creator;

    std::unique_ptr<Food> food = creator.createFood();
    food->eat();
}
```

**What changed?** The creation decision moved from a conditional function into specialized creator classes. This is useful when creation behavior varies by creator or needs to be customized by subclasses. It also adds classes and indirection, so it is not automatically better for every small example.

---

## 9. Abstract Factory example — concept

Imagine supporting two UI themes/platforms:

```text
UIFactory
  |-- createButton()
  |-- createCheckbox()

WindowsUIFactory
  |-- creates WindowsButton
  |-- creates WindowsCheckbox

LinuxUIFactory
  |-- creates LinuxButton
  |-- creates LinuxCheckbox
```

The application chooses one factory and obtains a compatible family of controls without directly constructing platform-specific classes.

Use Abstract Factory when the application needs **families of related products**, not merely one of several alternatives.

---

## 10. Senior-level design trade-offs

| Situation | Suitable approach |
|---|---|
| Two simple, fixed product types | Simple factory function or direct construction |
| Different subclasses decide which product to create | Factory Method |
| Multiple related products must be compatible | Abstract Factory |
| Types are added dynamically by modules/plugins | Registry or plugin-based factory |
| Construction has many steps/options | Builder |
| New objects are created by copying configured instances | Prototype |
| Dependencies should be supplied by application composition | Dependency Injection |

### Key design principles to mention

- **Program to an interface:** Client code uses `Food`, not concrete product types.
- **Single Responsibility:** Keep creation policy separate from product behavior.
- **Open/Closed Principle:** Choose an extensible design if adding product types frequently must not require editing central selection logic.
- **RAII and ownership:** Return smart pointers when the factory transfers ownership.
- **Error contract:** Define behavior for unsupported keys and construction failures.
- **Testability:** Make creation replaceable or injectable where consumers need isolated tests.

---

## 11. Senior-level evaluation checklist

- [ ] Define Factory and explain separation of creation from usage.
- [ ] Explain the `Food` abstraction and concrete product classes.
- [ ] Explain why the client calls `eat()` through the base interface.
- [ ] Identify the original code as a simple factory function.
- [ ] Distinguish Simple Factory, Factory Method, and Abstract Factory.
- [ ] Explain virtual destructors and polymorphic deletion.
- [ ] Explain raw-pointer ownership versus `std::unique_ptr`.
- [ ] Explain invalid input and construction failure handling.
- [ ] Discuss Open/Closed Principle without claiming every factory automatically satisfies it.
- [ ] Explain registry factories, plugins, and thread safety.
- [ ] Compare Factory with Builder, Prototype, and Dependency Injection.
- [ ] Explain when a factory is unnecessary abstraction.

---

## 12. Strong closing statement

> I use the Factory Pattern to separate object construction from business usage. The client depends on a stable abstraction, while the factory encapsulates the concrete creation decision and ownership contract. For a small, fixed set of types, a simple factory function is clear and sufficient. When the system needs product families, subclass-specific creation, or runtime extensibility, I choose Abstract Factory, Factory Method, or a registry-based design based on the actual requirement. In modern C++, I prefer smart-pointer ownership and explicit error handling, and I keep the factory focused so it does not become a central dependency bottleneck.

**At 15 years of experience:** Interviewers expect you to explain the pattern's intent, precise variant, ownership and lifetime, extensibility, testing, concurrency, ABI/plugin concerns, and trade-offs—not just write an `if/else` that returns `new`.
"""
path = Path("/mnt/data/Cpp_Factory_Pattern_Senior_Interview_Guide.md")
path.write_text(content, encoding="utf-8")
print(f"Created: {path}\nSize: {path.stat().st_size:,} bytes")
