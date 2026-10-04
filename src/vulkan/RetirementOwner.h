#pragma once

#include <memory>
#include <stdexcept>
#include <utility>

namespace horde::vulkan
{
// A failed native retirement proof must retain the WHOLE object, including
// members whose ordinary destructors would otherwise destroy GPU resources.
// This is a fatal-path, process-lifetime retention policy, not a retry registry.
template <typename T>
class RetirementOwner
{
public:
    using RetireFunction = bool (*)(T&);
    using DetachHostFunction = void (*)(T&) noexcept;
    enum class State { Owned, Retired, Retained };

    RetirementOwner(std::unique_ptr<T> object, bool (&retire)(T&),
                    void (&detachHost)(T&) noexcept)
        : object_(std::move(object)), retire_(retire), detachHost_(detachHost)
    {
        if (!object_)
            throw std::invalid_argument("RetirementOwner requires an object.");
    }
    RetirementOwner(const RetirementOwner&) = delete;
    RetirementOwner& operator=(const RetirementOwner&) = delete;
    RetirementOwner(RetirementOwner&&) = delete;
    RetirementOwner& operator=(RetirementOwner&&) = delete;
    ~RetirementOwner() noexcept { (void)Retire(); }

    [[nodiscard]] T* Get() const noexcept
    { return state_ == State::Owned ? object_.get() : nullptr; }
    [[nodiscard]] State RetirementState() const noexcept { return state_; }

    [[nodiscard]] bool Retire() noexcept
    {
        if (state_ != State::Owned) return state_ == State::Retired;
        state_ = State::Retained; // Never repeat the native attempt, including on throw.
        detachHost_(*object_); // Nonthrowing: stop joined host work and detach callbacks first.
        bool completed = false;
        try { completed = retire_(*object_); }
        catch (...) { completed = false; }
        if (completed)
        {
            state_ = State::Retired;
            object_.reset(); // Destructors now have the native owner's proof.
            return true;
        }
        // Intentionally do not run any member destructor or atexit callback.
        // OS process cleanup is not advertised as Vulkan retirement proof.
        (void)object_.release();
        return false;
    }

private:
    std::unique_ptr<T> object_;
    RetireFunction retire_;
    DetachHostFunction detachHost_;
    State state_ = State::Owned;
};
} // namespace horde::vulkan
