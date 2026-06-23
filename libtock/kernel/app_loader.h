#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "libtock/kernel/syscalls/app_loader_syscalls.h"
#include "libtock/tock.h"

returncode_t libtock_app_loader_setup(uint32_t app_length, uint32_t binary_type, subscribe_upcall cb);

returncode_t libtock_app_loader_write(uint32_t offset, uint8_t* chunk_data, size_t chunk_len);

returncode_t libtock_app_loader_finalize(subscribe_upcall cb);

returncode_t libtock_app_loader_load(subscribe_upcall cb);

// returncode_t libtock_app_loader_load_xip(uintptr_t address,
//                                           size_t size,
//                                           subscribe_upcall cb);

returncode_t libtock_app_loader_abort(subscribe_upcall cb);

returncode_t libtock_app_loader_unload(uint32_t app_short_id, subscribe_upcall cb);


#ifdef __cplusplus
}
#endif
