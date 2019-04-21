#ifdef _WIN32

#include <stdlib.h>
#include <windows.h>
#include "vendor/include/ViGEm/Client.h"

PVIGEM_CLIENT client = NULL;

PVIGEM_TARGET create_device()
{
    client = vigem_alloc();
    VIGEM_ERROR err = vigem_connect(client);

    if (!VIGEM_SUCCESS(err))
        return NULL;

    PVIGEM_TARGET x360_target = vigem_target_x360_alloc();
    err = vigem_target_add(client, x360_target);

    if (!VIGEM_SUCCESS(err))
        return NULL;

    return x360_target;
}

VIGEM_ERROR send_report(PXUSB_REPORT report, PVIGEM_TARGET x360_target)
{
    return vigem_target_x360_update(client, x360_target, *report);
}

void vigem_cleanup()
{
    if (client != NULL)
        vigem_disconnect(client);

    vigem_free(client);
}

#endif // _WIN32