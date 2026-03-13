#pragma once

#include "gfx/DxError.h"

#include <d3d11.h>

#include <utility>

// Exercise 4 introduces an RAII wrapper around D3D11 Map/Unmap.
//
// The production problem we are solving is:
// - call Map to get CPU-visible access to a GPU resource
// - write into the mapped memory
// - guarantee Unmap happens exactly once even on early return or exception
//
// The testing problem we are solving at the same time is:
// - unit tests should not need to initialize a real D3D device/context
// - unit tests should still be able to prove Map/Unmap ownership behavior
//
// To support both goals, this type uses a tiny backend interface:
// - production code talks to a D3D11MapBackend adapter
// - tests talk to a FakeMapBackend implementation
//
// This is a little more code than the function-pointer approach, but it is also
// much easier to read and explain.
struct IMapBackend
{
    virtual ~IMapBackend() = default;

    virtual HRESULT Map(
        ID3D11Resource* resource,
        UINT subresource,
        D3D11_MAP map_type,
        UINT flags,
        D3D11_MAPPED_SUBRESOURCE* mapping) = 0;

    virtual void Unmap(ID3D11Resource* resource, UINT subresource) = 0;
};

// Small production adapter that forwards the interface calls to the real D3D11
// device context.
class D3D11MapBackend final : public IMapBackend
{
public:
    explicit D3D11MapBackend(ID3D11DeviceContext* context)
        : context_(context)
    {
    }

    HRESULT Map(
        ID3D11Resource* resource,
        UINT subresource,
        D3D11_MAP map_type,
        UINT flags,
        D3D11_MAPPED_SUBRESOURCE* mapping) override
    {
        return context_->Map(resource, subresource, map_type, flags, mapping);
    }

    void Unmap(ID3D11Resource* resource, UINT subresource) override
    {
        context_->Unmap(resource, subresource);
    }

private:
    ID3D11DeviceContext* context_ = nullptr;
};

class MappedSubresource
{
public:
    MappedSubresource() = default;

    // Production-facing constructor.
    //
    // This is the simple "normal app code" path: hand us the real D3D11 device
    // context and the resource you want to map. The constructor immediately
    // calls Map and becomes the owner responsible for later calling Unmap.
    MappedSubresource(
        ID3D11DeviceContext* context,
        ID3D11Resource* resource,
        UINT subresource = 0,
        D3D11_MAP map_type = D3D11_MAP_WRITE_DISCARD,
        UINT flags = 0)
        : owned_d3d_backend_(context)
        , backend_(&owned_d3d_backend_)
        , resource_(resource)
        , subresource_(subresource)
    {
        ThrowIfFailed(backend_->Map(resource_, subresource_, map_type, flags, &mapping_), "Map");
    }

    // Test-friendly constructor.
    //
    // Tests can pass any object implementing IMapBackend. This keeps the public
    // behavior readable: MappedSubresource depends on a tiny interface rather
    // than Direct3D itself.
    MappedSubresource(
        IMapBackend& backend,
        ID3D11Resource* resource,
        UINT subresource = 0,
        D3D11_MAP map_type = D3D11_MAP_WRITE_DISCARD,
        UINT flags = 0)
        : backend_(&backend)
        , resource_(resource)
        , subresource_(subresource)
    {
        ThrowIfFailed(backend_->Map(resource_, subresource_, map_type, flags, &mapping_), "Map");
    }

    // The whole RAII point of this type lives here: if we successfully mapped a
    // resource, we unmap it automatically when the wrapper dies.
    ~MappedSubresource() noexcept
    {
        Cleanup();
    }

    // Moving transfers the "I am responsible for Unmap" ownership from one
    // wrapper to another. The moved-from object is intentionally left empty so
    // it cannot accidentally unmap the same resource a second time.
    MappedSubresource(MappedSubresource&& other) noexcept
    {
        MoveFrom(std::move(other));
    }

    // Move assignment is the subtle case:
    // 1. clean up whatever this object currently owns
    // 2. take ownership from the source object
    // 3. leave the source empty
    MappedSubresource& operator=(MappedSubresource&& other) noexcept
    {
        if (this != &other)
        {
            Cleanup();
            MoveFrom(std::move(other));
        }

        return *this;
    }

    MappedSubresource(const MappedSubresource&) = delete;
    MappedSubresource& operator=(const MappedSubresource&) = delete;

    // Convenience accessor for the most common use case: write directly into the
    // mapped CPU pointer returned by D3D11.
    void* data() noexcept
    {
        return mapping_.pData;
    }

    const void* data() const noexcept
    {
        return mapping_.pData;
    }

    D3D11_MAPPED_SUBRESOURCE& mapping() noexcept
    {
        return mapping_;
    }

    const D3D11_MAPPED_SUBRESOURCE& mapping() const noexcept
    {
        return mapping_;
    }

    // These small observers are mainly helpful in tests so we can prove that a
    // moved-from object became inert after ownership transfer.
    bool valid() const noexcept
    {
        return resource_ != nullptr;
    }

    ID3D11Resource* resource() const noexcept
    {
        return resource_;
    }

private:
    // For the production constructor, the adapter lives inside this object so
    // callers can keep passing a raw ID3D11DeviceContext* without manually
    // creating a separate backend object.
    D3D11MapBackend owned_d3d_backend_{nullptr};
    IMapBackend* backend_ = nullptr;
    ID3D11Resource* resource_ = nullptr;
    UINT subresource_ = 0;
    D3D11_MAPPED_SUBRESOURCE mapping_{};

    // Cleanup is separated into a helper so both the destructor and move
    // assignment can reuse exactly the same "if I still own a mapping, unmap
    // it once" logic.
    void Cleanup() noexcept
    {
        if (resource_ != nullptr && backend_ != nullptr)
        {
            backend_->Unmap(resource_, subresource_);
        }

        // Reset everything after cleanup so the object becomes an empty,
        // harmless state. This also makes moved-from-state checks easy in tests.
        backend_ = nullptr;
        owned_d3d_backend_ = D3D11MapBackend(nullptr);
        resource_ = nullptr;
        subresource_ = 0;
        mapping_ = {};
    }

    // MoveFrom performs a shallow transfer of ownership-related state, then
    // empties the source object so only the destination remains responsible for
    // calling Unmap later.
    void MoveFrom(MappedSubresource&& other) noexcept
    {
        owned_d3d_backend_ = std::move(other.owned_d3d_backend_);
        backend_ = other.backend_;
        if (backend_ == &other.owned_d3d_backend_)
        {
            backend_ = &owned_d3d_backend_;
        }
        resource_ = other.resource_;
        subresource_ = other.subresource_;
        mapping_ = other.mapping_;

        other.backend_ = nullptr;
        other.owned_d3d_backend_ = D3D11MapBackend(nullptr);
        other.resource_ = nullptr;
        other.subresource_ = 0;
        other.mapping_ = {};
    }
};
