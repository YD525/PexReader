#include "../PexReader/PexReaderApi.h"

#if defined(_WIN64)
typedef char PexReaderHandleSizeMustBeEight[(sizeof(PexReaderHandle) == 8) ? 1 : -1];
#else
typedef char PexReaderHandleSizeMustBeFour[(sizeof(PexReaderHandle) == 4) ? 1 : -1];
#endif

typedef char PexReaderValueSizeMustBeFour[(sizeof(PexReaderValue) == 4) ? 1 : -1];
typedef char PexReaderStatusSizeMustBeFour[(sizeof(PexReaderStatus) == 4) ? 1 : -1];

int PexReaderCHeaderContract(void)
{
    PexReaderValue value = { 0 };
    value.Integer = 42;
    return PEX_READER_ABI_VERSION == 1u && value.Integer == 42
        ? PEX_READER_STATUS_OK
        : PEX_READER_STATUS_INTERNAL_ERROR;
}
