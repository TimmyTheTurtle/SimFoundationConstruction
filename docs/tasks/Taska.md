Exercises

1. [x] Convert device/context/swap chain ownership to ComPtr  

    Goal: I want all D3D11 core objects owned by RAII, no raw Release in normal paths. [8]  
    Tasks: Replace raw pointers with ComPtr; use ReleaseAndGetAddressOf()/GetAddressOf() correctly for out-params. [9]  
    Success: No leaks reported by D3D11 live object reporting at exit. [10]  
    Checklist: (a) zero manual Release on success path, (b) no operator& confusion, (c) shutdown calls live-object report.  
    Hint (≤10 lines):  
```
ComPtr<ID3D11Device> dev;
ComPtr<ID3D11DeviceContext> ctx;
UINT flags = D3D11_CREATE_DEVICE_DEBUG; // Debug only
ThrowIfFailed(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE,nullptr,flags,
  nullptr,0,D3D11_SDK_VERSION, dev.ReleaseAndGetAddressOf(), nullptr, ctx.ReleaseAndGetAddressOf()));
  ```
Validate: enable debug layer + ID3D11Debug::ReportLiveDeviceObjects. [11]

    Implementation notes: This work lives in `src/gfx/Dx11Device.h` and `src/gfx/Dx11Device.cpp`. The device, immediate context, swap chain, and render-target view are now stored as `Microsoft::WRL::ComPtr` members, all COM-producing calls use `ReleaseAndGetAddressOf()` instead of taking the address of the smart pointer directly, and shutdown clears bindings before resetting COM objects. In Debug builds the destructor also calls `ID3D11Debug::ReportLiveDeviceObjects` so leaks are visible at exit.

    Tests: There are two layers of coverage. `tests/test_comptr_checklist.cpp` is a source-level guard that scans `Dx11Device.cpp` and fails if raw `Release()` calls or unsafe `&m_device` / `&m_rtv` patterns return. `tests/test_dx11_device.cpp` is an integration test that constructs a real D3D11 device, clears, presents, and resizes so the RAII-backed wrapper is exercised through normal use.

    Testability changes: To make the graphics code testable, `Dx11Device` gained `Dx11DeviceOptions` in `src/gfx/Dx11Device.h` so tests can request a WARP software device instead of relying on a specific hardware adapter or debug-layer availability. The build was also split so `src/gfx/Dx11Device.cpp` is compiled into the reusable `app_support` library in `CMakeLists.txt`, which lets the integration test target link the same production code as the application.
 
2. [ ] Wrap Win32 HANDLE ownership (UniqueHandle)
Goal: I want to never leak or double-close kernel handles. [5]
Tasks: Implement move-only UniqueHandle calling CloseHandle in destructor; convert any raw CreateEvent/CreateFile handles to it. [5]
Success: Handle count stable; no “invalid handle” crashes on shutdown.
Checklist: (a) move-only, (b) reset() closes old, (c) safe “null” state.
Hint:
```
struct UniqueHandle{ HANDLE h=nullptr; ~UniqueHandle(){ if(h) CloseHandle(h); }
  UniqueHandle()=default; explicit UniqueHandle(HANDLE x):h(x){}
  UniqueHandle(UniqueHandle&& o) noexcept : h(std::exchange(o.h,nullptr)) {}
  UniqueHandle& operator=(UniqueHandle&& o) noexcept { std::swap(h,o.h); return *this; }
};
```
Validate: run under CRT leak checks for non-D3D leaks (Debug build). [12]

Implementation notes: The wrapper itself is implemented in `src/core/UniqueHandle.h`. It is move-only, calls `CloseHandle` through `reset()`, supports `release()` and `swap()`, and treats both `nullptr` and `INVALID_HANDLE_VALUE` as non-owning sentinel states. This gives the project the RAII type needed for Win32 handle ownership.

Tests: `tests/test_unique_handle.cpp` covers the wrapper thoroughly: default construction, `INVALID_HANDLE_VALUE`, destruction closing the owned handle, move construction, move assignment, `reset()`, `release()`, and a repeated-scope process-handle-count check to catch leaks.

Testability changes: This exercise needed very little indirection because Win32 event handles are cheap to create in a test. The main design tweak made for correctness and testability was adding `UniqueHandle::IsValid()` so both common invalid-handle sentinels are treated consistently by production code and assertions. This exercise is still left unchecked because the wrapper exists and is tested, but the second half of the task, converting real production `CreateEvent` / `CreateFile` ownership sites to use it, has not been completed yet.
 
3. [x] Build a scoped QPC timer for profiling spans
Goal: I want time spans to always end (even on early return). [13]
Tasks: Implement ScopedQpcTimer that logs to OutputDebugStringA; add scopes around physics step and render submission.
Success: Every frame logs consistent “physics_ms”/“render_ms” without missing end events.
Checklist: (a) caches frequency once, (b) noexcept destructor, (c) minimal overhead.
Hint:
```
struct ScopedQpcTimer{
  const char* tag; LARGE_INTEGER f,s;
  ScopedQpcTimer(const char* t):tag(t){QueryPerformanceFrequency(&f);QueryPerformanceCounter(&s);}
  ~ScopedQpcTimer() noexcept { LARGE_INTEGER e; QueryPerformanceCounter(&e); /* log ms */ }
};
```
Validate: correlate with PIX timing captures for CPU/GPU scheduling intuition. [14]

Implementation notes: The timer implementation lives in `src/core/ScopedQpcTimer.h` and `src/core/ScopedQpcTimer.cpp`, and it is used in `src/app/main.cpp` around the `resize_ms`, `physics_ms`, and `render_ms` spans. Construction captures the start counter, destruction captures the end counter and writes `<label>: <ms> ms` to `OutputDebugStringA`, and the counter frequency is cached once in shared runtime state so normal timer construction stays cheap.

Tests: `tests/test_scoped_qpc_timer.cpp` verifies the three key promises of the exercise. One test checks that a completed scope logs the expected label and millisecond value, one checks that an early return still produces a log entry because the destructor runs during scope exit, and one checks that multiple timers only query the QPC frequency once. There is also a compile-time `static_assert` that the type is nothrow-destructible.

Testability changes: The original timer directly called `QueryPerformanceFrequency`, `QueryPerformanceCounter`, and `OutputDebugStringA`, which made deterministic unit tests difficult. To solve that, `ScopedQpcTimer` gained test hooks under `scoped_qpc_timer::testing` with `SetHooks()` and `Reset()`. Tests install fake QPC/debug-output functions so they can script counter values and capture log messages without relying on wall-clock timing or an attached debugger. The timer implementation also resets its cached frequency whenever hooks change so tests remain isolated from one another.
 
4. [ ] RAII Map/Unmap guard (MappedSubresource)
Goal: I want Unmap to be guaranteed, preventing “forgot to unmap” bugs. [15]
Tasks: Create a move-only MappedSubresource that calls Map in ctor and Unmap in dtor; use it for dynamic constant buffers. [15]
Success: No code path leaves without unmapping; debug layer stays quiet. [16]
Checklist: (a) ctor checks HRESULT, (b) dtor noexcept, (c) non-copyable.
Hint:
```
struct MapGuard{ ID3D11DeviceContext* c; ID3D11Resource* r; D3D11_MAPPED_SUBRESOURCE m{};
  MapGuard(ID3D11DeviceContext* c_,ID3D11Resource* r_):c(c_),r(r_){ThrowIfFailed(c->Map(r,0,D3D11_MAP_WRITE_DISCARD,0,&m));}
  ~MapGuard() noexcept { if(r) c->Unmap(r,0); }
};
```
Validate: add an intentional early-return inside the update function and confirm state remains correct.
 
5. [ ] Dynamic upload buffer wrapper (D3D11_USAGE_DYNAMIC)
Goal: I want a safe “upload buffer” class for per-frame instance transforms of rigid bodies.
Tasks: Wrap an ID3D11Buffer + size; expose Update(span) that internally uses exercise 4’s map guard. Dynamic resource guidance emphasizes Map/Unmap patterns. [17]
Success: No overruns; stable updates under stress (10k instances).
Checklist: (a) bounds check size, (b) uses WRITE_DISCARD, (c) no stored raw pointers to mapped memory.
Hint:
```
struct DynBuf{ ComPtr<ID3D11Buffer> b; UINT cap=0;
  void Update(ID3D11DeviceContext* ctx,const void* src,UINT n){
    if(n>cap) throw std::runtime_error("too big");
    MapGuard g(ctx,b.Get()); std::memcpy(g.m.pData,src,n);
  }
};
```
Validate: D3D11 debug layer warnings + PIX GPU capture if updates cause stalls. [18]
 
6. [ ] Thread shutdown guard + GPU quiesce boundary
Goal: I want shutdown ordering to be correct: stop workers → stop GPU usage → release resources.
Tasks: Convert workers to std::jthread; in shutdown, request stop and let destruction join; then do a D3D11 “GPU fence” (exercise 7) before releasing buffers/swap chain. std::jthread joins on destruction. [19]
Success: No sporadic shutdown crashes; live objects report clean. [10]
Checklist: (a) never destroy shared state while workers run, (b) quiesce GPU once, (c) destructors don’t throw. [20]
Hint:
```
std::jthread worker([&](std::stop_token st){ while(!st.stop_requested()) DoJobs(); });
/* shutdown */ worker.request_stop(); // join happens in ~jthread
```
```mermaid
sequenceDiagram
  participant Main as Main thread
  participant W as Worker threads
  participant GPU as D3D11 GPU queue

  Main->>W: request_stop()
  W-->>Main: workers return; RAII join completes
  Main->>GPU: End(EVENT_QUERY) + Flush()
  loop poll
    Main->>GPU: GetData(query, DONOTFLUSH?)
    GPU-->>Main: S_FALSE until done
  end
  GPU-->>Main: S_OK (done)
  Main->>Main: release COM objects + handles (RAII)
```
(Flush behavior and event query waiting are explicitly documented.) [21]
 
7. [ ] Implement a D3D11 “fence” using D3D11_QUERY_EVENT
Goal: I want a reusable “wait until GPU finished previous work” helper for teardown/readback edges. An event query indicates completion when GetData returns S_OK and a TRUE BOOL. [22]
Tasks: RAII: create query in ctor/factory; Signal() calls End() + Flush(); Wait() polls GetData.
Success: I can safely release resources after Wait() at shutdown.
Checklist: (a) avoid infinite loop with D3D11_ASYNC_GETDATA_DONOTFLUSH, (b) yields CPU while polling, (c) used only at boundaries. [23]
Hint:
```
ctx->End(q.Get()); ctx->Flush(); // flush submits, not a wait
BOOL done=FALSE;
while(ctx->GetData(q.Get(),&done,sizeof(done),0)==S_FALSE) SwitchToThread();
```
Validate: confirm no deadlock; DONOTFLUSH doc warns about infinite loops. [24]
 
8. [ ] File I/O: RAII for Win32 file handles
Goal: I want robust logging/trace dumping (contacts, perf stats) without leaking file handles.
Tasks: Wrap CreateFileW handle in UniqueHandle; implement WriteAll() with retries; close implicitly in destructor.
Success: Log file closes even on exceptions; handle count stable. (CloseHandle semantics documented.) [5]
Checklist: (a) close-on-scope-exit, (b) no shared raw handle ownership, (c) error paths tested.
Hint:
```
UniqueHandle f(CreateFileW(L"log.bin",GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,0,nullptr));
if(!f.get() || f.get()==INVALID_HANDLE_VALUE) throw std::runtime_error("CreateFileW");
```
Validate: CRT leak dump + manual check file is not locked after crash. [12]
 
9. [ ] unique_ptr + custom deleter exercise (C file API)
Goal: I want to practice custom deleters for non-COM resources.
Tasks: Wrap FILE* (or another C-handle-like resource) in unique_ptr with deleter; integrate into a “save scene snapshot” command. unique_ptr uses its deleter on destruction. [4]
Success: File closes in all paths; no manual fclose.
Checklist: (a) correct deleter type, (b) move-only semantics preserved, (c) no raw FILE* escapes.
Hint:
```
using FilePtr = std::unique_ptr<FILE, int(*)(FILE*)>;
FilePtr f(std::fopen("scene.txt","wb"), &std::fclose);
if(!f) throw std::runtime_error("fopen");
```
Validate: ASan to catch UAF if I mistakenly persist pointers into file-backed buffers. [25]
 
10. [ ] PImpl lifetime control for a subsystem boundary
Goal: I want to isolate “renderer internals” (D3D buffers, queries) behind a stable interface and control lifetime via RAII + incomplete types. PImpl is a standard technique. [26]
Tasks: Create class Renderer { struct Impl; std::unique_ptr<Impl> p; ~Renderer(); }; with destructor defined in .cpp so Impl is complete where unique_ptr destroys it. (cppreference notes completeness requirements.) [27]
Success: Build times improve; headers no longer include D3D11 headers in physics code.
Checklist: (a) out-of-line dtor, (b) move enabled, copy disabled, (c) shutdown order centralized.
Hint:
```
class Renderer{ struct Impl; std::unique_ptr<Impl> p;
public: Renderer(); ~Renderer(); Renderer(Renderer&&) noexcept; Renderer& operator=(Renderer&&) noexcept;
};
```
Validate: D3D11 live-object report should point only to real leaks, not header coupling artifacts. [10]
 
Validation toolbox I should use throughout
· D3D11 debug layer during development (D3D11_CREATE_DEVICE_DEBUG) and live object reporting at shutdown to catch COM leaks. [28]
· CRT leak detection (_CrtDumpMemoryLeaks) in Debug builds to catch CPU-side allocations not owned by RAII yet. [12]
· AddressSanitizer to find UAF/OOB issues that often appear when I botch ownership or shutdown ordering. [25]
· PIX timing/GPU captures to understand submission vs execution and to sanity-check “GPU quiesce” waits. [14]
 


[1] RAII
https://en.cppreference.com/w/cpp/language/raii.html?utm_source=chatgpt.com
[2] R: Resource management
https://cpp-core-guidelines-docs.vercel.app/resource?utm_source=chatgpt.com
[3] [8] [9] ComPtr Class
https://learn.microsoft.com/en-us/cpp/cppcx/wrl/comptr-class?view=msvc-170&utm_source=chatgpt.com
[4] [27] std::unique_ptr
https://en.cppreference.com/w/cpp/memory/unique_ptr.html?utm_source=chatgpt.com
[5] CloseHandle function (handleapi.h) - Win32 apps
https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle?utm_source=chatgpt.com
[6] add_library — CMake 4.2.0 Documentation
https://cmake.org/cmake/help/latest/command/add_library.html?utm_source=chatgpt.com
[7] cmake-generator-expressions(7)
https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?utm_source=chatgpt.com
[10] ID3D11Debug::ReportLiveDeviceObjects (d3d11sdklayers.h)
https://learn.microsoft.com/en-us/windows/win32/api/d3d11sdklayers/nf-d3d11sdklayers-id3d11debug-reportlivedeviceobjects?utm_source=chatgpt.com
[11] [16] [18] [28] Using the debug layer to debug apps - Win32 apps
https://learn.microsoft.com/en-us/windows/win32/direct3d11/using-the-debug-layer-to-test-apps?utm_source=chatgpt.com
[12] _CrtDumpMemoryLeaks | Microsoft Learn
https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/crtdumpmemoryleaks?view=msvc-170&utm_source=chatgpt.com
[13] QueryPerformanceCounter function - Win32 apps
https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancecounter?utm_source=chatgpt.com
[14] Timing Captures - PIX on Windows
https://devblogs.microsoft.com/pix/timing-captures/?utm_source=chatgpt.com
[15] [17] ID3D11DeviceContext::Map (d3d11.h) - Win32 apps
https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-map?utm_source=chatgpt.com
[19] std::jthread
https://en.cppreference.com/w/cpp/thread/jthread.html?utm_source=chatgpt.com
[20] Best Practices from the C++ Core Guidelines
https://www.modernescpp.org/wp-content/uploads/2023/12/BestPracticesFromTheCCoreGuidelines.pdf?utm_source=chatgpt.com
[21] ID3D11DeviceContext::Flush (d3d11.h) - Win32 apps
https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-flush?utm_source=chatgpt.com
[22] D3D11_QUERY (d3d11.h) - Win32 apps | Microsoft Learn
https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_query?utm_source=chatgpt.com
[23] [24] D3D11_ASYNC_GETDATA_FLAG (d3d11.h) - Win32 apps
https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_async_getdata_flag?utm_source=chatgpt.com
[25] AddressSanitizer
https://learn.microsoft.com/en-us/cpp/sanitizers/asan?view=msvc-170&utm_source=chatgpt.com
[26] PImpl
https://en.cppreference.com/w/cpp/language/pimpl.html?utm_source=chatgpt.com
