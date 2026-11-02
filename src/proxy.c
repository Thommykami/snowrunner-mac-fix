/*
 * iphlpapi.dll proxy for running SnowRunner (Windows) under Wine on macOS.
 *
 * Wine's UuidCreateSequential() takes the MAC address of the first adapter
 * returned by GetAdaptersInfo(). On macOS the first adapters are gif0 and
 * stf0, which have no MAC address, so every sequential GUID ends in
 * 000000000000. SnowRunner crashes shortly after startup because of that.
 *
 * This DLL forwards every iphlpapi export to Wine's own iphlpapi.dll and
 * only changes GetAdaptersInfo(): adapters with a real 6-byte MAC address are
 * moved to the front of the list. Nothing else is touched.
 */
#include <windows.h>
#include <iphlpapi.h>
#include "names.h"

extern void *real_ptrs[FW_COUNT];
typedef DWORD (WINAPI *GetAdaptersInfo_fn)(IP_ADAPTER_INFO *, ULONG *);
static GetAdaptersInfo_fn real_GetAdaptersInfo;

static DWORD WINAPI not_supported(void) { return ERROR_NOT_SUPPORTED; }

static int has_real_mac(const IP_ADAPTER_INFO *a)
{
    if (a->AddressLength != 6) return 0;
    for (int i = 0; i < 6; i++) if (a->Address[i]) return 1;
    return 0;
}

DWORD WINAPI my_GetAdaptersInfo(IP_ADAPTER_INFO *info, ULONG *size)
{
    DWORD ret = real_GetAdaptersInfo ? real_GetAdaptersInfo(info, size) : ERROR_NOT_SUPPORTED;
    if (ret != NO_ERROR || !info) return ret;

    IP_ADAPTER_INFO *slot[64];
    UINT n = 0;
    for (IP_ADAPTER_INFO *a = info; a && n < 64; a = a->Next) slot[n++] = a;

    IP_ADAPTER_INFO *tmp = HeapAlloc(GetProcessHeap(), 0, n * sizeof(*tmp));
    if (!tmp) return ret;

    /* stable partition: adapters with a MAC first, the rest after */
    UINT k = 0;
    for (UINT i = 0; i < n; i++) if (has_real_mac(slot[i])) tmp[k++] = *slot[i];
    for (UINT i = 0; i < n; i++) if (!has_real_mac(slot[i])) tmp[k++] = *slot[i];
    for (UINT i = 0; i < n; i++)
    {
        *slot[i] = tmp[i];
        slot[i]->Next = (i + 1 < n) ? slot[i + 1] : NULL;
    }
    HeapFree(GetProcessHeap(), 0, tmp);
    return ret;
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    DisableThreadLibraryCalls(inst);

    /* load Wine's real iphlpapi.dll from system32 */
    char path[MAX_PATH];
    GetSystemDirectoryA(path, MAX_PATH);
    lstrcatA(path, "\\iphlpapi.dll");
    HMODULE real = LoadLibraryA(path);
    if (real == (HMODULE)inst) real = NULL;

    for (int i = 0; i < FW_COUNT; i++)
    {
        void *p = real ? (void *)GetProcAddress(real, fw_names[i]) : NULL;
        real_ptrs[i] = p ? p : (void *)not_supported;
    }
    real_GetAdaptersInfo = real ? (GetAdaptersInfo_fn)GetProcAddress(real, "GetAdaptersInfo") : NULL;
    return TRUE;
}
