#include "CppUnitTest.h"

#include "../PexReader/LineNumberBuffer.h"

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PexReaderTests
{
    TEST_CLASS(LineNumberBufferTests)
    {
    public:
        TEST_METHOD(CopyAndFreeRepeatedlyPreservesValues)
        {
            const std::vector<std::uint16_t> expected{ 0, 1, 255, 256, 4096, 65535 };

            for (std::size_t iteration = 0; iteration < 10000; ++iteration)
            {
                std::uint16_t* buffer = pex::interop::CopyLineNumbers(expected);
                Assert::IsNotNull(buffer);

                for (std::size_t index = 0; index < expected.size(); ++index)
                    Assert::AreEqual(expected[index], buffer[index]);

                pex::interop::FreeLineNumbers(buffer);
            }
        }

        TEST_METHOD(EmptyInputDoesNotAllocateBuffer)
        {
            const std::vector<std::uint16_t> empty;

            std::uint16_t* buffer = pex::interop::CopyLineNumbers(empty);

            Assert::IsNull(buffer);
            pex::interop::FreeLineNumbers(buffer);
        }
    };
}
