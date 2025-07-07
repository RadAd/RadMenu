#include "RadFile.h"


BOOL WINAPI NoCloseHandle(_In_ _Post_ptr_invalid_ HANDLE /*hObject*/)
{
    return TRUE;
}

