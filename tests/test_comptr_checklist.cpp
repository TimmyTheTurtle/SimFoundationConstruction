#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

// This test is intentionally a source-level "lint" rather than a runtime test.
// We want to guard against dangerous ComPtr patterns such as taking `&m_rtv`
// directly or sneaking raw Release calls back into the file later.
std::string LoadDx11DeviceSource()
{
    const std::filesystem::path sourcePath =
        std::filesystem::path(SIMFOUNDATION_SOURCE_DIR) / "src" / "gfx" / "Dx11Device.cpp";
    std::ifstream input(sourcePath);
    if (!input)
    {
        throw std::runtime_error("Failed to open Dx11Device.cpp");
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

} // namespace

TEST(ComPtrChecklist, Dx11DeviceUsesExplicitOutParamHelpers)
{
    // We read the implementation as plain text and search for the specific
    // patterns we want to forbid or require.
    const std::string source = LoadDx11DeviceSource();

    // These patterns would indicate raw COM management slipping back in.
    EXPECT_EQ(source.find("->Release("), std::string::npos);
    EXPECT_EQ(source.find(".Release("), std::string::npos);
    EXPECT_EQ(source.find("IID_PPV_ARGS(&"), std::string::npos);
    EXPECT_EQ(source.find("&m_device"), std::string::npos);
    EXPECT_EQ(source.find("&m_context"), std::string::npos);
    EXPECT_EQ(source.find("&m_swapChain"), std::string::npos);
    EXPECT_EQ(source.find("&m_rtv"), std::string::npos);

    // These patterns are the healthy replacements we expect to see.
    EXPECT_NE(source.find("ReleaseAndGetAddressOf"), std::string::npos);
    EXPECT_NE(source.find("ReportLiveDeviceObjects"), std::string::npos);
}
