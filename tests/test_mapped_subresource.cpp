#include <gtest/gtest.h>

#include <d3d11.h>

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <type_traits>

#include "gfx/MappedSubresource.h"

namespace {

// This file is a TDD scaffold for exercise 4.
//
// The original teaching plan was:
// - start with disabled tests
// - enable them one by one
// - let each one drive a small design decision in the production code
//
// The final implementation now exists, but the test file still reflects the
// learning goal of the exercise: understand how to design RAII code that is
// both production-usable and unit-testable.
//
// The important architectural lesson is the seam around Map/Unmap:
// instead of forcing tests to use a real ID3D11DeviceContext COM object, we
// test the ownership logic through a tiny backend interface with the same two
// operations we care about.

struct MapCall
{
    // What resource did the SUT ask to map?
    ID3D11Resource* resource = nullptr;
    // Which subresource index was requested?
    UINT subresource = 0;
    // Which D3D11 mapping mode was requested?
    D3D11_MAP map_type = D3D11_MAP_READ;
    // Which map flags were used?
    UINT flags = 0;
};

struct UnmapCall
{
    ID3D11Resource* resource = nullptr;
    UINT subresource = 0;
};

// This fake records exactly what the SUT asked for and optionally returns a
// scripted mapping result. It is the key reason these are true unit tests:
// the fake gives us precise observability without requiring real GPU state.
struct FakeMapBackend : IMapBackend
{
    // Tests can choose whether Map should succeed or fail.
    HRESULT next_map_result = S_OK;
    // If Map succeeds, the fake copies this data into the output mapping.
    D3D11_MAPPED_SUBRESOURCE next_mapping{};
    // Simple counters let us prove the lifetime behavior.
    int map_call_count = 0;
    int unmap_call_count = 0;
    std::optional<MapCall> last_map_call;
    std::optional<UnmapCall> last_unmap_call;

    HRESULT Map(
        ID3D11Resource* resource,
        UINT subresource,
        D3D11_MAP map_type,
        UINT flags,
        D3D11_MAPPED_SUBRESOURCE* mapping) override
    {
        // Record exactly what the production object asked us to do.
        ++map_call_count;
        last_map_call = MapCall{resource, subresource, map_type, flags};

        if (SUCCEEDED(next_map_result) && mapping != nullptr)
        {
            // Simulate D3D11 filling out the mapping result structure.
            *mapping = next_mapping;
        }

        return next_map_result;
    }

    void Unmap(ID3D11Resource* resource, UINT subresource) override
    {
        ++unmap_call_count;
        last_unmap_call = UnmapCall{resource, subresource};
    }
};

// We only need a stable pointer value for tests; no real D3D resource is
// required for unit-level call recording.
ID3D11Resource* SentinelResource()
{
    return reinterpret_cast<ID3D11Resource*>(static_cast<std::uintptr_t>(0x1234));
}

} // namespace

// These static_asserts act as a compile-time design contract.
// If someone later makes the type copyable by accident, the test target will
// fail to compile before any runtime test even starts.
static_assert(!std::is_copy_constructible_v<MappedSubresource>);
static_assert(!std::is_copy_assignable_v<MappedSubresource>);
static_assert(std::is_move_constructible_v<MappedSubresource>);
static_assert(std::is_move_assignable_v<MappedSubresource>);
static_assert(std::is_nothrow_destructible_v<MappedSubresource>);

TEST(MappedSubresourceTdd, TypeIsMoveOnly)
{
    // The actual checks for this "test" live in the static_asserts above.
    // SUCCEED keeps GoogleTest happy with a concrete runtime test entry.
    SUCCEED();
}

TEST(MappedSubresourceTdd, ConstructingGuardCallsMapWithExpectedArguments)
{
    FakeMapBackend backend;
    backend.next_mapping.pData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(0xCAFE));

    ID3D11Resource* resource = SentinelResource();

    // Construction should immediately acquire the mapping.
    MappedSubresource mapped(backend, resource);

    EXPECT_EQ(backend.map_call_count, 1);
    ASSERT_TRUE(backend.last_map_call.has_value());
    EXPECT_EQ(backend.last_map_call->resource, resource);
    EXPECT_EQ(backend.last_map_call->subresource, 0u);
    EXPECT_EQ(backend.last_map_call->map_type, D3D11_MAP_WRITE_DISCARD);
    EXPECT_EQ(backend.last_map_call->flags, 0u);
}

TEST(MappedSubresourceTdd, SuccessfulMapExposesMappedMemoryView)
{
    FakeMapBackend backend;
    backend.next_mapping.pData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(0xBEEF));
    backend.next_mapping.RowPitch = 256;
    backend.next_mapping.DepthPitch = 1024;

    // The fake backend scripts the mapping; the SUT should expose it back to
    // callers so they can actually write/read the mapped region.
    MappedSubresource mapped(backend, SentinelResource());

    EXPECT_EQ(mapped.data(), backend.next_mapping.pData);
    EXPECT_EQ(mapped.mapping().RowPitch, 256u);
    EXPECT_EQ(mapped.mapping().DepthPitch, 1024u);
}

TEST(MappedSubresourceTdd, DestructorCallsUnmapAfterSuccessfulMap)
{
    FakeMapBackend backend;
    backend.next_mapping.pData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(0xABCD));
    ID3D11Resource* resource = SentinelResource();

    {
        // While the wrapper is alive, it still owns the mapping, so no Unmap yet.
        MappedSubresource mapped(backend, resource);
        EXPECT_EQ(backend.unmap_call_count, 0);
    }

    // Leaving scope should trigger the destructor and therefore exactly one Unmap.
    EXPECT_EQ(backend.unmap_call_count, 1);
    ASSERT_TRUE(backend.last_unmap_call.has_value());
    EXPECT_EQ(backend.last_unmap_call->resource, resource);
    EXPECT_EQ(backend.last_unmap_call->subresource, 0u);
}

TEST(MappedSubresourceTdd, FailedMapThrowsAndDoesNotCallUnmap)
{
    FakeMapBackend backend;
    backend.next_map_result = E_FAIL;

    // A failed constructor should throw immediately, and because ownership was
    // never acquired, there must not be any compensating Unmap call afterward.
    EXPECT_THROW(MappedSubresource(backend, SentinelResource()), std::runtime_error);
    EXPECT_EQ(backend.unmap_call_count, 0);
}

TEST(MappedSubresourceTdd, MoveConstructorTransfersUnmapResponsibility)
{
    FakeMapBackend backend;
    backend.next_mapping.pData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(0x1111));

    {
        MappedSubresource original(backend, SentinelResource());
        // After the move, the new object owns the mapping and the old one must
        // become inert.
        MappedSubresource moved(std::move(original));

        EXPECT_FALSE(original.valid());
        EXPECT_TRUE(moved.valid());
    }

    // If the moved-from object still thought it owned the mapping, this would
    // be 2 instead of 1.
    EXPECT_EQ(backend.unmap_call_count, 1);
}

TEST(MappedSubresourceTdd, MoveAssignmentUnmapsOldMappingThenTakesNewOne)
{
    FakeMapBackend backend;
    backend.next_mapping.pData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(0x2222));

    ID3D11Resource* first_resource = SentinelResource();
    ID3D11Resource* second_resource = reinterpret_cast<ID3D11Resource*>(static_cast<std::uintptr_t>(0x5678));

    {
        MappedSubresource first(backend, first_resource);
        MappedSubresource second(backend, second_resource);

        // second already owns one mapping here. Move assignment must first
        // clean that one up before taking over first's mapping.
        second = std::move(first);

        EXPECT_FALSE(first.valid());
        EXPECT_TRUE(second.valid());
        EXPECT_EQ(second.resource(), first_resource);
    }

    // One Unmap happens during move assignment for second's old mapping.
    // The second Unmap happens when the surviving owner is destroyed at scope exit.
    EXPECT_EQ(backend.unmap_call_count, 2);
    ASSERT_TRUE(backend.last_unmap_call.has_value());
    EXPECT_EQ(backend.last_unmap_call->resource, first_resource);
}

TEST(MappedSubresourceTdd, EarlyReturnStillUnmaps)
{
    FakeMapBackend backend;
    backend.next_mapping.pData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(0x3333));

    auto update_once = [&](bool stop_early) {
        // This is the real ergonomic win of RAII: even if control flow leaves
        // early, cleanup still happens automatically.
        MappedSubresource mapped(backend, SentinelResource());
        if (stop_early)
        {
            return;
        }
    };

    update_once(true);
    EXPECT_EQ(backend.unmap_call_count, 1);
}

TEST(MappedSubresourceTdd, MovedFromGuardDoesNotUnmapAgain)
{
    FakeMapBackend backend;
    backend.next_mapping.pData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(0x4444));

    MappedSubresource* moved_from_address = nullptr;

    {
        MappedSubresource original(backend, SentinelResource());
        moved_from_address = &original;
        // We keep a pointer to the moved-from object so we can inspect its
        // post-move state before the scope ends.
        MappedSubresource moved(std::move(original));

        EXPECT_FALSE(moved_from_address->valid());
        EXPECT_EQ(moved_from_address->resource(), nullptr);
        EXPECT_TRUE(moved.valid());
    }

    // Only the moved-to object should have remained responsible for cleanup.
    EXPECT_EQ(backend.unmap_call_count, 1);
}
