#ifdef _WIN32

#include <stdlib.h>
#include <windows.h>
#include "vendor/include/ViGEm/Client.h"

#ifdef __cplusplus
extern "C"
{
#endif

PVIGEM_CLIENT client = NULL;

int init_vigem()
{
    client = vigem_alloc();
    VIGEM_ERROR err = vigem_connect(client);

    if (!VIGEM_SUCCESS(err))
        return -1;

    return 0;
}

PVIGEM_TARGET new_x360_target(PFN_VIGEM_TARGET_ADD_RESULT fn)
{
    PVIGEM_TARGET x360_target = vigem_target_x360_alloc();
    err = vigem_target_add_async(client, x360_target);

    if (!VIGEM_SUCCESS(err))
        return NULL;

    return x360_target;
}

PVIGEM_TARGET new_ds4_target(PFN_VIGEM_TARGET_ADD_RESULT fn)
{
    PVIGEM_TARGET ds4_target = vigem_target_ds4_alloc();
    err = vigem_target_add_async(client, ds4_target, fn);

    if (!VIGEM_SUCCESS(err))
        return NULL;

    return ds4_target;
}

VIGEM_TARGET_TYPE get_target_type(PVIGEM_TARGET target)
{
    return vigem_target_get_type(target);
}

VIGEM_ERROR x360_send_report(PXUSB_REPORT report, PVIGEM_TARGET x360_target)
{
    assert(vigem_target_get_type(x360_target) == Xbox360Wired);
    return vigem_target_x360_update(client, x360_target, *report);
}

VIGEM_ERROR ds4_send_report(PXDS4_REPORT report, PVIGEM_TARGET ds4_target)
{
    assert(vigem_target_get_type(ds4_target) == DualShock4Wired);
    return vigem_target_ds4_update(client, ds4_target, *report);
}

void vigem_cleanup()
{
    if (client != NULL)
        vigem_disconnect(client);

    vigem_free(client);
}

#ifdef __cplusplus
}
#endif

#endif // _WIN32