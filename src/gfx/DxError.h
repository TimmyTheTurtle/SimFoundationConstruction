#pragma once

#include <winerror.h>

#include <stdexcept>

inline void ThrowIfFailed(HRESULT hr, const char* msg)
{
    // D3D APIs report failures through HRESULT rather than exceptions, so this
    // helper converts "failed HRESULT" into normal C++ control flow.
    if (FAILED(hr))
    {
        throw std::runtime_error(msg);
    }
}
