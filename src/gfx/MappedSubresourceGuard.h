#pragma once

#include "gfx/DxError.h"

#include <d3d11.h>

// This is the original, very small RAII guard for D3D11 Map/Unmap.
//
// It demonstrates the core production idea clearly:
// - call Map in the constructor
// - call Unmap in the destructor
//
// That is enough for runtime use, but it is tightly coupled to the real
// ID3D11DeviceContext COM interface. Because it directly calls c->Map(...) and
// c->Unmap(...), it is awkward to unit test without creating a real D3D device
// or building a larger COM mock.
//
// Exercise 4 added the more testable MappedSubresource type beside this one.
// That newer type keeps the same RAII behavior while also allowing a fake
// backend in tests.
struct MappedSubresourceGuard
{
    ID3D11DeviceContext* c;
    ID3D11Resource* r;
    D3D11_MAPPED_SUBRESOURCE m{};

    // Acquire the mapping immediately. If Map fails, construction fails and the
    // object never takes ownership, so the destructor will not later try to
    // unmap anything.
    MappedSubresourceGuard(ID3D11DeviceContext* c_, ID3D11Resource* r_)
        : c(c_)
        , r(r_)
    {
        ThrowIfFailed(c->Map(r, 0, D3D11_MAP_WRITE_DISCARD, 0, &m), "ID3D11DeviceContext::Map failed");
    }

    // Release the mapping automatically on scope exit.
    ~MappedSubresourceGuard() noexcept
    {
        if (r)
        {
            c->Unmap(r, 0);
        }
    }
};
