
### RAII for a DirectX 11 Physics Simulator

**Executive summary**

Resource Acquisition Is Initialization (RAII) is the C++ technique of binding resource lifetime (GPU objects, OS handles, threads, locks, timers) to object lifetime so that construction acquires and destruction releases—automatically, deterministically, and correctly across early returns and exceptions. RAII also implies reverse-order release (resources are released in reverse order of acquisition/initialization), and if a constructor throws, already-constructed members are cleaned up automatically. [1]
In a Win32 + CMake DirectX 11 physics simulator, RAII is practical engineering: it reduces leaks, prevents shutdown races between physics, worker threads, and the GPU command stream, and makes multi-stage initialization safe when any HRESULT step can fail. The Direct3D 11 runtime also defers destruction, so “releasing” a COM pointer is necessary but sometimes not sufficient to reclaim resources immediately; you often need a deliberate “GPU flush + wait” only at specific boundaries (e.g., shutdown) to avoid use-after-free patterns. [2]
Unspecified items: you did not specify (1) C++ standard, (2) whether you allow exceptions, or (3) toolchain. The examples assume C++20 (for std::jthread) and work with MSVC or clang-cl on Windows when linked against D3D11/DXGI libraries. [3]
Core RAII principles and why they matter
RAII’s core guarantees (as usually taught by Bjarne Stroustrup[4] and Scott Meyers[5]) are:
•	Ownership is explicit: the resource is a class invariant—if the object exists, it owns a valid resource (or a defined “empty” state). [6]
•	Cleanup is automatic and deterministic: destructors run on scope exit and unwind paths; resources are released in reverse order, and partially-constructed objects don’t leak their already-acquired subresources. [7]
•	Destructors must not fail: letting a destructor throw during stack unwinding calls std::terminate; guidelines explicitly state destructors/deallocation/swap must not fail. [8]
In a simulator, RAII is especially valuable because initialization is multi-step (window → device → swap chain → resources → threads → physics world). If step N fails, you want steps 1..N−1 cleaned up automatically without a bespoke error-path ladder. This is a key reason RAII is emphasized in modern C++ resource management guidance. [9]
RAII across simulator subsystems
D3D11 device / contexts / swap chain (COM objects). These are COM interfaces: lifetime is reference-counted and released by calling Release when you’re done. Using Microsoft::WRL::ComPtr<T> automates reference counting and releases the interface automatically when the last ComPtr reference goes away. [10]
GPU synchronization in D3D11 (“fence/event” equivalent). D3D11 doesn’t expose D3D12-style fences, but the official pattern to determine when the GPU has finished queued work is: call Flush (submits queued commands asynchronously), create an event query (D3D11_QUERY_EVENT), then poll GetData until it returns S_OK (and the BOOL result is TRUE). This is appropriate for boundary conditions like teardown or one-off readback—not per-frame simulation—because it stalls/serializes CPU↔GPU. [11]
GPU resources and uploads. Upload-like patterns in D3D11 often map a dynamic resource (Map) and later Unmap. RAII wrappers ensure that if an update path throws or returns early, Unmap still runs. (Also remember: Map can fail with device-removed or “still drawing” conditions depending on flags and usage.) [12]
File handles, event handles, other Win32 HANDLEs. Handles must be closed with CloseHandle; RAII handle wrappers prevent leaks and double-closes (especially in error paths). [13]
Threads / job system. std::jthread is an RAII thread: it automatically requests stop and joins when destroyed (if joinable), which is a safer default than std::thread (whose destructor calls std::terminate if still joinable). This maps cleanly to a physics job system’s “shut down workers before destroying shared state” requirement. [14]
Mutexes and locks. Prefer RAII locking (std::scoped_lock, std::lock_guard) over manual lock/unlock. scoped_lock explicitly provides RAII-style ownership for one or more mutexes. [15]
Timers. Wrap QueryPerformanceCounter spans in a small scoped timer to guarantee timing end markers are emitted even on early returns; Microsoft documents QPC as a high-resolution timestamp suitable for interval measurement. [16]
Patterns you’ll actually use
Wrapper choice and locking patterns
Need in your simulator	Typical RAII tool	When it’s best	Key caveat
COM (D3D11 device/context/swap chain/resources)	WRL::ComPtr<T>	Default choice for DirectX COM ownership; auto Release. [17]
Avoid reference cycles (ref-counting won’t collect cycles). [18]

Win32 HANDLE (events/files/etc.)	Move-only Handle class (calls CloseHandle)	Encodes the correct closer and prevents leaks/double-close. [13]
Don’t mix raw-handle ownership with RAII ownership. [13]

Legacy C API resource	std::unique_ptr<T, Deleter>	Great when a destroy function exists (free, custom destroy). [19]
Deleter type affects size; keep it simple. [20]


Locking approach	Safety under exceptions	Recommendation
std::scoped_lock(m1, m2, …)	Strong; RAII unlocks on scope exit	Prefer for multi-lock and general engine code. [21]

Manual lock() / unlock()	Fragile	Avoid except in narrow, proven cases. [22]

Exception-safety and error-safety in initialization/teardown
RAII works whether you use exceptions or error codes, but it’s most powerful when constructors/factories either succeed fully or fail fast (throw or return an error) while preserving invariants. RAII’s core language property—“already-constructed subobjects are destroyed in reverse order if initialization fails”—is the mechanism that prevents leaks in partial initialization. [23]
Two concrete DirectX practices that interact with RAII:
•	D3D11 deferred destruction: dropping your last ComPtr reference may not immediately destroy the underlying resource; on shutdown (or when you must force release), Microsoft recommends releasing references, calling ClearState, then Flush to force deferred destruction. [24]
•	Never throw from destructors: if you need to report teardown problems, record/log them, return status from an explicit Shutdown() method, or use debug-only assertions—don’t throw in destructors. [25]
Deterministic ordering across physics, render snapshots, and GPU submission
You want to preserve this shutdown order:
1) stop producing new simulation work (stop main loop)
2) stop workers / job system (no more writes to physics state)
3) stop GPU consumption of render snapshots (flush + event query wait)
4) release GPU resources, then the D3D11 context/device/swap chain
RAII helps by encoding dependencies in member order: members destroy in reverse order, so declare “most dependent” objects last (renderer last if it depends on physics snapshots and GPU objects). The reverse-order release property is part of RAII’s guarantee. [26]
Minimal code examples and CMake integration
The snippet below is designed to be dropped into src/core/raii_utils.hpp. It provides: a move-only UniqueHandle, a ScopedQpcTimer, a D3D11 “GPU fence” using D3D11_QUERY_EVENT + Flush + GetData, a MappedSubresource guard, a dynamic upload buffer wrapper, and a worker-group RAII guard using std::jthread. [27]
// src/core/raii_utils.hpp  (assume C++20; MSVC or clang-cl; D3D11)
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <cstring>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using Microsoft::WRL::ComPtr;

inline void ThrowIfFailed(HRESULT hr, const char* msg = "HRESULT failed") {
  if (FAILED(hr)) throw std::runtime_error(msg);
}

// Win32 HANDLE RAII (CloseHandle)
class UniqueHandle {
  HANDLE h_ = nullptr;
public:
  UniqueHandle() = default;
  explicit UniqueHandle(HANDLE h) : h_(h) {}
  ~UniqueHandle() noexcept { reset(); }
  UniqueHandle(UniqueHandle&& o) noexcept : h_(o.h_) { o.h_ = nullptr; }
  UniqueHandle& operator=(UniqueHandle&& o) noexcept {
    if (this != &o) { reset(); h_ = o.h_; o.h_ = nullptr; }
    return *this;
  }
  UniqueHandle(const UniqueHandle&) = delete;
  UniqueHandle& operator=(const UniqueHandle&) = delete;
  HANDLE get() const noexcept { return h_; }
  void reset(HANDLE nh = nullptr) noexcept { if (h_) CloseHandle(h_); h_ = nh; }
};

// Scoped QPC timer (debug output)
class ScopedQpcTimer {
  const char* label_;
  LARGE_INTEGER freq_{}, start_{};
public:
  explicit ScopedQpcTimer(const char* label) : label_(label) {
    QueryPerformanceFrequency(&freq_);
    QueryPerformanceCounter(&start_);
  }
  ~ScopedQpcTimer() noexcept {
    LARGE_INTEGER end{}; QueryPerformanceCounter(&end);
    double ms = (end.QuadPart - start_.QuadPart) * 1000.0 / double(freq_.QuadPart);
    std::string s = std::string(label_) + ": " + std::to_string(ms) + " ms\n";
    OutputDebugStringA(s.c_str());
  }
};

// D3D11 "fence": D3D11_QUERY_EVENT + Flush + GetData polling
class GpuFence11 {
  ComPtr<ID3D11Query> q_;
public:
  static GpuFence11 Create(ID3D11Device* dev) {
    GpuFence11 f;
    D3D11_QUERY_DESC d{}; d.Query = D3D11_QUERY_EVENT;
    ThrowIfFailed(dev->CreateQuery(&d, f.q_.ReleaseAndGetAddressOf()), "CreateQuery");
    return f;
  }
  void Signal(ID3D11DeviceContext* ctx) { ctx->End(q_.Get()); ctx->Flush(); }
  void Wait(ID3D11DeviceContext* ctx) {
    BOOL done = FALSE;
    for (;;) {
      HRESULT hr = ctx->GetData(q_.Get(), &done, sizeof(done), D3D11_ASYNC_GETDATA_DONOTFLUSH);
      if (hr == S_OK && done) break;
      if (hr != S_FALSE) ThrowIfFailed(hr, "GetData");
      SwitchToThread(); // yield while waiting
    }
  }
};

// RAII Map/Unmap guard
class MappedSubresource {
  ID3D11DeviceContext* ctx_ = nullptr;
  ID3D11Resource* res_ = nullptr;
  UINT sub_ = 0;
public:
  D3D11_MAPPED_SUBRESOURCE m{};
  MappedSubresource(ID3D11DeviceContext* ctx, ID3D11Resource* res, UINT sub,
                    D3D11_MAP type, UINT flags)
    : ctx_(ctx), res_(res), sub_(sub) { ThrowIfFailed(ctx_->Map(res_, sub_, type, flags, &m), "Map"); }
  ~MappedSubresource() noexcept { if (ctx_ && res_) ctx_->Unmap(res_, sub_); }
  MappedSubresource(MappedSubresource&& o) noexcept { *this = std::move(o); }
  MappedSubresource& operator=(MappedSubresource&& o) noexcept {
    if (this != &o) { ctx_ = o.ctx_; res_ = o.res_; sub_ = o.sub_; m = o.m; o.ctx_ = nullptr; o.res_ = nullptr; }
    return *this;
  }
  MappedSubresource(const MappedSubresource&) = delete;
  MappedSubresource& operator=(const MappedSubresource&) = delete;
};

// Dynamic "upload" buffer wrapper (Map WRITE_DISCARD)
class DynamicUploadBuffer {
  ComPtr<ID3D11Buffer> buf_;
  UINT cap_ = 0;
public:
  static DynamicUploadBuffer Create(ID3D11Device* dev, UINT bytes, UINT bindFlags) {
    DynamicUploadBuffer b; b.cap_ = bytes;
    D3D11_BUFFER_DESC bd{}; bd.Usage = D3D11_USAGE_DYNAMIC; bd.ByteWidth = bytes;
    bd.BindFlags = bindFlags; bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ThrowIfFailed(dev->CreateBuffer(&bd, nullptr, b.buf_.ReleaseAndGetAddressOf()), "CreateBuffer");
    return b;
  }
  void Update(ID3D11DeviceContext* ctx, const void* src, UINT nbytes) {
    if (nbytes > cap_) throw std::runtime_error("Update too large");
    MappedSubresource map(ctx, buf_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0);
    std::memcpy(map.m.pData, src, nbytes);
  }
  ID3D11Buffer* get() const noexcept { return buf_.Get(); }
};

// Simple worker-group RAII (jthread requests stop + joins on destruction)
class WorkerGroup {
  std::vector<std::jthread> threads_;
public:
  template<class F> WorkerGroup(size_t n, F&& fn) {
    threads_.reserve(n);
    for (size_t i = 0; i < n; ++i) threads_.emplace_back(fn); // fn(std::stop_token) preferred
  }
};
CMake: add this as a header-only INTERFACE library and link it into your simulator targets. Use generator expressions for Debug-only checks so behavior is consistent across single-config (CMAKE_BUILD_TYPE) and multi-config (CMAKE_CONFIGURATION_TYPES) generators. [28]
add_library(raii_utils INTERFACE)
target_include_directories(raii_utils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(raii_utils INTERFACE $<$<CONFIG:Debug>:PHYSIM_RAII_DEBUG=1>)
target_link_libraries(physics_sim PRIVATE raii_utils)
Pitfalls, testing, debugging, and a checklist
Common pitfalls
Circular ownership leaks still happen with reference counting; COM cycles won’t be collected automatically—design ownership DAGs and break cycles explicitly. [18]
Mixing raw handles and RAII leads to double-close or dangling ownership; CloseHandle semantics are explicit—close exactly once, by the owner. [13]
Throwing from destructors is dangerous; during stack unwinding it triggers termination, and guidelines explicitly forbid destructor failure. [25]
Global/static initialization order can break RAII assumptions across translation units; prefer explicit construction in main/WinMain or “construct on first use.” [29]
Multi-threaded destruction races: if workers might touch physics state or D3D11 context, you must stop/join workers before destroying shared data; std::jthread’s join-on-destruction helps encode this. [14]
Practical testing and debugging
For Direct3D 11 leaks, use the debug layer (D3D11_CREATE_DEVICE_DEBUG) and call ID3D11Debug::ReportLiveDeviceObjects at shutdown to see what’s still alive. [30]
For general heap leaks in Debug builds, use CRT leak detection (_CrtDumpMemoryLeaks / CRT leak-check flags) to report leaked allocations to the debugger output. [31]
Use AddressSanitizer to find use-after-free and out-of-bounds errors that often appear as “shutdown crashes” when RAII/ordering is wrong; MSVC supports /fsanitize=address, and clang/clang-cl supports -fsanitize=address. [32]
Checklist for applying RAII across the simulator
•	Wrap every ownership-bearing COM pointer in ComPtr; use ReleaseAndGetAddressOf() (or &comPtr) for out-parameters to avoid leaks. [33]
•	Wrap every Win32 HANDLE you own in a move-only handle type that calls CloseHandle. [13]
•	Use RAII for lock ownership (std::scoped_lock) and avoid manual unlock() patterns. [15]
•	Join/stop worker threads before destroying physics state; prefer std::jthread-based workers. [14]
•	On shutdown, quiesce the GPU: Flush + D3D11_QUERY_EVENT wait before releasing resources that could still be in-flight. [11]
•	Keep destructors noexcept and non-throwing; surface errors via explicit Shutdown() or logs. [25]
•	Avoid global owning singletons for graphics/physics; avoid SIOF. [29]
________________________________________
[1] [6] [7] [9] [23] [26] RAII - cppreference.com
https://en.cppreference.com/w/cpp/language/raii.html
[2] [4] [11] [24] [27] ID3D11DeviceContext::Flush (d3d11.h) - Win32 apps | Microsoft Learn
https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-flush
[3] [14] std::jthread - cppreference.com
https://en.cppreference.com/w/cpp/thread/jthread.html
[5] [8] Destructors - cppreference.com
https://en.cppreference.com/w/cpp/language/destructor.html
[10] [17] [33] ComPtr Class
https://learn.microsoft.com/en-us/cpp/cppcx/wrl/comptr-class?view=msvc-170&utm_source=chatgpt.com
[12] nf-d3d11-id3d11devicecontext-map.md
https://github.com/MicrosoftDocs/sdk-api/blob/docs/sdk-api-src/content/d3d11/nf-d3d11-id3d11devicecontext-map.md?utm_source=chatgpt.com
[13] CloseHandle function (handleapi.h) - Win32 apps
https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle?utm_source=chatgpt.com
[15] [21] std::scoped_lock
https://en.cppreference.com/w/cpp/thread/scoped_lock.html?utm_source=chatgpt.com
[16] QueryPerformanceCounter function - Win32 apps
https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancecounter?utm_source=chatgpt.com
[18] IUnknown::Release - Win32 apps - Microsoft Learn
https://learn.microsoft.com/en-us/windows/win32/api/unknwn/nf-unknwn-iunknown-release?utm_source=chatgpt.com
[19] [20] std::unique_ptr
https://en.cppreference.com/w/cpp/memory/unique_ptr/unique_ptr.html?utm_source=chatgpt.com
[22] std::lock_guard
https://en.cppreference.com/w/cpp/thread/lock_guard.html?utm_source=chatgpt.com
[25] E: Error handling | Cpp-Core-Guidelines
https://madduci.gitbooks.io/cpp-core-guidelines/content/e_error_handling.html?utm_source=chatgpt.com
[28] add_library — CMake 4.2.3 Documentation
https://cmake.org/cmake/help/latest/command/add_library.html?utm_source=chatgpt.com
[29] Static Initialization Order Fiasco
https://en.cppreference.com/w/cpp/language/siof.html?utm_source=chatgpt.com
[30] Using the debug layer to debug apps - Win32 apps
https://learn.microsoft.com/en-us/windows/win32/direct3d11/using-the-debug-layer-to-test-apps?utm_source=chatgpt.com
[31] find-memory-leaks-using-the-crt-library.md - cpp-docs
https://github.com/MicrosoftDocs/cpp-docs/blob/main/docs/c-runtime-library/find-memory-leaks-using-the-crt-library.md?utm_source=chatgpt.com
[32] fsanitize (Enable sanitizers)
https://learn.microsoft.com/en-us/cpp/build/reference/fsanitize?view=msvc-170&utm_source=chatgpt.com

