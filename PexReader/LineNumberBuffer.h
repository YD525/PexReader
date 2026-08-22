#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

namespace pex::interop
{
    // The caller owns the returned array and must release it with FreeLineNumbers.
    [[nodiscard]] inline std::uint16_t* CopyLineNumbers(
        const std::vector<std::uint16_t>& lineNumbers)
    {
        if (lineNumbers.empty())
            return nullptr;

        auto buffer = std::make_unique<std::uint16_t[]>(lineNumbers.size());
        std::copy(lineNumbers.cbegin(), lineNumbers.cend(), buffer.get());
        return buffer.release();
    }

    inline void FreeLineNumbers(std::uint16_t* lineNumbers) noexcept
    {
        // Reconstruct the exact owning type used by CopyLineNumbers.
        std::unique_ptr<std::uint16_t[]> owner(lineNumbers);
    }
}
