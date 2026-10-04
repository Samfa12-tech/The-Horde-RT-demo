#include "vulkan/RetirementOwner.h"

#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace
{
struct Counts
{
    int hostDetach = 0, attempts = 0, objectDestroyed = 0, gpuMembersDestroyed = 0;
    bool hostAttached = true, hostRunning = true, proved = false;
    enum class Result { Success, Failure, Throw } result = Result::Success;
    std::array<int, 4u> order{};
    std::size_t next = 0u;
};
struct GpuMember
{
    Counts& counts;
    ~GpuMember() { ++counts.gpuMembersDestroyed; counts.order.at(counts.next++) = 4; }
};
struct Context
{
    explicit Context(Counts& c) : counts(c), gpu{c} {}
    ~Context() { ++counts.objectDestroyed; counts.order.at(counts.next++) = 3; }
    Counts& counts;
    GpuMember gpu;
};
using Owner = horde::vulkan::RetirementOwner<Context>;
void Detach(Context& context) noexcept
{
    auto& c = context.counts;
    ++c.hostDetach; c.hostAttached = false; c.hostRunning = false;
    c.order[static_cast<std::size_t>(c.next++)] = 1;
}
bool Retire(Context& context)
{
    auto& c = context.counts;
    ++c.attempts; c.order.at(c.next++) = 2;
    if (c.hostAttached || c.hostRunning) throw std::runtime_error("host was not detached before retirement");
    if (c.result == Counts::Result::Throw) throw std::runtime_error("native proof threw");
    c.proved = c.result == Counts::Result::Success;
    return c.proved;
}
int assertions = 0;
void Require(bool condition, const char* description)
{
    ++assertions;
    if (!condition) throw std::runtime_error(description);
}
void SuccessfulRetirement()
{
    Counts c;
    {
        Owner owner(std::make_unique<Context>(c), Retire, Detach);
        Require(owner.Get() != nullptr && owner.RetirementState() == Owner::State::Owned, "initial complete object owner");
        Require(owner.Retire(), "proved retirement succeeds");
        Require(owner.Get() == nullptr && owner.RetirementState() == Owner::State::Retired, "retired object cannot be used again");
        Require(owner.Retire(), "repeat retirement returns cached success");
    }
    Require(c.hostDetach == 1 && c.attempts == 1 && c.objectDestroyed == 1 && c.gpuMembersDestroyed == 1, "success exact-once ownership");
    Require(c.order == std::array<int, 4u>{1, 2, 3, 4} && c.proved, "host detach precedes proof and destruction");
}
void RetainedRetirement(Counts::Result result)
{
    // Observe actual owner retention first. The test keeps a separate witness
    // solely to reclaim this synthetic CPU object after simulating completion.
    // Production retains its fatal-path owner until process exit.
    Counts c; c.result = result;
    Context* witness = nullptr;
    {
        Owner owner(std::make_unique<Context>(c), Retire, Detach);
        witness = owner.Get();
        Require(!owner.Retire(), "failed or exceptional retirement fails closed");
        Require(owner.Get() == nullptr && owner.RetirementState() == Owner::State::Retained, "retained owner exposes no actionable pointer");
        Require(!owner.Retire(), "retention cannot retry native operations");
    }
    Require(c.hostDetach == 1 && c.attempts == 1, "retention detaches and attempts only once");
    Require(c.objectDestroyed == 0 && c.gpuMembersDestroyed == 0 && !c.proved, "all RAII members survive failed proof");
    Require(c.order == std::array<int, 4u>{1, 2, 0, 0}, "no destructor or native action after retention");
    c.proved = true; // Test-only simulated later completion; no Vulkan operation.
    delete witness;
    Require(c.objectDestroyed == 1 && c.gpuMembersDestroyed == 1 && c.attempts == 1,
            "test-only reclamation does not retry native retirement");
}
void ScopeUnwind(Counts::Result result)
{
    Counts c; c.result = result;
    Context* witness = nullptr;
    try
    {
        Owner owner(std::make_unique<Context>(c), Retire, Detach);
        witness = owner.Get();
        throw std::runtime_error("application/capture failure before explicit retirement");
    }
    catch (const std::runtime_error&) {}
    Require(c.hostDetach == 1 && c.attempts == 1, "exception unwind uses same retirement owner");
    const int expected = result == Counts::Result::Success ? 1 : 0;
    Require(c.objectDestroyed == expected && c.gpuMembersDestroyed == expected, "unwind honors native proof before RAII cleanup");
    if (result != Counts::Result::Success)
    {
        c.proved = true; // Reclaim only after all retention assertions above.
        delete witness;
        Require(c.objectDestroyed == 1 && c.gpuMembersDestroyed == 1 && c.attempts == 1,
                "test-only unwind reclamation leaves native attempt count unchanged");
    }
}
void ImplicitScopeSuccess()
{
    Counts c;
    { Owner owner(std::make_unique<Context>(c), Retire, Detach); }
    Require(c.hostDetach == 1 && c.attempts == 1 && c.objectDestroyed == 1 && c.gpuMembersDestroyed == 1,
            "early ordinary return automatically retires once");
}
} // namespace

int main()
{
    try
    {
        SuccessfulRetirement(); RetainedRetirement(Counts::Result::Failure);
        RetainedRetirement(Counts::Result::Throw); ScopeUnwind(Counts::Result::Success);
        ScopeUnwind(Counts::Result::Failure); ScopeUnwind(Counts::Result::Throw);
        ImplicitScopeSuccess();
        std::cout << assertions << " retirement ownership assertions PASS; no Vulkan calls\n";
        return 0;
    }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
