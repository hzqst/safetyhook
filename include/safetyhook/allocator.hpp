/// @file safetyhook/allocator.hpp
/// @brief Allocator for allocating memory near target addresses.

#pragma once

#include <cstdint>
#include <expected>
#include <memory>
#include <mutex>
#include <vector>

namespace safetyhook {
class Allocator;

/// @brief A memory allocation.
class Allocation final {
public:
    Allocation() = default;
    Allocation(const Allocation&) = delete;
    Allocation(Allocation&& other) noexcept;
    Allocation& operator=(const Allocation&) = delete;
    Allocation& operator=(Allocation&& other) noexcept;
    ~Allocation();

    /// @brief Frees the allocation.
    /// @note This is called automatically when the Allocation object is destroyed.
    void free();

    /// @brief Returns the address of the allocation.
    /// @return The address of the allocation.
    [[nodiscard]] uintptr_t address() const noexcept { return m_address; }

    /// @brief Returns the size of the allocation.
    /// @return The size of the allocation.
    [[nodiscard]] size_t size() const noexcept { return m_size; }

    /// @brief Tests if the allocation is valid.
    /// @return True if the allocation is valid, false otherwise.
    explicit operator bool() const noexcept { return m_address != 0 && m_size != 0; }

protected:
    friend Allocator;

    Allocation(std::shared_ptr<Allocator> allocator, uintptr_t address, size_t size) noexcept;

private:
    std::shared_ptr<Allocator> m_allocator{};
    uintptr_t m_address{};
    size_t m_size{};
};

/// @brief Allocates memory near target addresses.
class Allocator final : public std::enable_shared_from_this<Allocator> {
public:
    /// @brief Returns the global Allocator.
    /// @return The global Allocator.
    [[nodiscard]] static std::shared_ptr<Allocator> global();

    /// @brief Creates a new Allocator.
    /// @return The new Allocator.
    [[nodiscard]] static std::shared_ptr<Allocator> create();

    Allocator(const Allocator&) = delete;
    Allocator(Allocator&&) noexcept = delete;
    Allocator& operator=(const Allocator&) = delete;
    Allocator& operator=(Allocator&&) noexcept = delete;
    ~Allocator() = default;

    /// @brief The error type returned by the allocate functions.
    enum class Error : uint8_t {
        BAD_VIRTUAL_ALLOC,  ///< VirtualAlloc failed.
        NO_MEMORY_IN_RANGE, ///< No memory in range.
    };

    /// @brief Allocates memory.
    /// @param size The size of the allocation.
    /// @return The Allocation or an Allocator::Error if the allocation failed.
    [[nodiscard]] std::expected<Allocation, Error> allocate(size_t size);

    /// @brief Allocates memory near a target address.
    /// @param desired_address The target address.
    /// @param size The size of the allocation.
    /// @param max_distance The maximum distance from the target address.
    /// @return The Allocation or an Allocator::Error if the allocation failed.
    [[nodiscard]] std::expected<Allocation, Error> allocate_near(
        const std::vector<uintptr_t>& desired_addresses, size_t size, size_t max_distance = 0x7FFF'FFFF);

protected:
    friend Allocation;

    void free(uintptr_t address, size_t size);

private:
    struct FreeNode {
        std::unique_ptr<FreeNode> next{};
        uintptr_t start{};
        uintptr_t end{};
    };

    struct Memory {
        uintptr_t address{};
        size_t size{};
        std::unique_ptr<FreeNode> freelist{};

        ~Memory();
    };

    std::vector<std::unique_ptr<Memory>> m_memory{};
    std::mutex m_mutex{};

    Allocator() = default;

    [[nodiscard]] std::expected<Allocation, Error> internal_allocate_near(
        const std::vector<uintptr_t>& desired_addresses, size_t size, size_t max_distance = 0x7FFF'FFFF);
    void internal_free(uintptr_t address, size_t size);

    static void combine_adjacent_freenodes(Memory& memory);
    [[nodiscard]] static std::expected<uintptr_t, Error> allocate_nearby_memory(
        const std::vector<uintptr_t>& desired_addresses, size_t size, size_t max_distance);
    [[nodiscard]] static bool in_range(
        uintptr_t address, const std::vector<uintptr_t>& desired_addresses, size_t max_distance);
};
} // namespace safetyhook