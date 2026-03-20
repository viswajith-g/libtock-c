// #include <stdint.h>
// #include <string.h>
// #include <stdio.h>
// #include <libtock-sync/interface/console.h>
// #include <libtock/kernel/app_loader.h>
// #include <libtock/tock.h>
// #include <libtock-sync/services/alarm.h>

// #define READ_SOFT_TIMEOUT_MS  1500
// #define READ_SPIN_SLICE_MS      10
// #define MAX_CHUNK 4096

// static bool setup_done=false;
// static bool finalize_done=false;
// static bool load_done=false;

// enum {
//   CMD_PING    = 0x01,
//   CMD_SETUP   = 0x10,
//   CMD_DATA    = 0x11,
//   CMD_FINAL   = 0x12,
//   CMD_SYNC    = 0x7F,
//   CMD_ACK     = 0x80,
//   CMD_ERR     = 0x81,
// };

// typedef struct __attribute__((packed)) {
//   uint8_t  magic[2];  // 'D','L'
//   uint8_t  cmd;
//   uint8_t  flags;
//   uint32_t length;    // LE
//   uint32_t meta;      // type or offset
//   uint16_t seq;
//   uint32_t crc32;     // 0 = unused
// } FrameHdr;

// static uint8_t payload_buf[MAX_CHUNK];

// /******************************************************************************************************
//  * App Loader callbacks
//  *****************************************************************************************************/
// static void app_setup_done_callback(__attribute__((unused)) int   arg0,
//                                     __attribute__((unused)) int   arg1,
//                                     __attribute__((unused)) int   arg2,
//                                     __attribute__((unused)) void* ud) {
//   setup_done = true;
// }

// static void app_finalize_done_callback(__attribute__((unused)) int   arg0,
//                                        __attribute__((unused)) int   arg1,
//                                        __attribute__((unused)) int   arg2,
//                                        __attribute__((unused)) void* ud) {
//   finalize_done = true;
// }

// static void app_load_done_callback(int                           arg0,
//                                    __attribute__((unused)) int   arg1,
//                                    __attribute__((unused)) int   arg2,
//                                    __attribute__((unused)) void* ud) {

//   if (arg0 != RETURNCODE_SUCCESS) {
//     printf("[Error] Process creation failed: %d.\n", arg0);
//   } else {
//     printf("[Success] Process created successfully.\n");
//   }
//   load_done = true;
// }

// static int read_exact(uint8_t* dst, size_t len) {
//   size_t got = 0;
//   while (got < len) {
//     int n = 0;
//     int rc = libtocksync_console_read(dst + got, (int)(len - got), &n);
//     if (rc != RETURNCODE_SUCCESS || n <= 0) return -1;
//     got += (size_t)n;
//   }
//   return 0;
// }

// static int read_exact_console(uint8_t *dst,
//                               size_t    len,
//                               uint32_t  overall_timeout_ms,
//                               uint32_t  idle_slice_ms)
// {
//   if (len == 0) return RETURNCODE_SUCCESS;

//   size_t got = 0;
//   const uint32_t max_attempts =
//       (overall_timeout_ms + (idle_slice_ms ? idle_slice_ms : 1) - 1) /
//       (idle_slice_ms ? idle_slice_ms : 1);

//   uint32_t attempts = 0;

//   while (got < len) {
//     int just = 0;
//     int rc   = libtocksync_console_read(dst + got, (int)(len - got), &just);
//     if (rc != RETURNCODE_SUCCESS) {
//       return rc;
//     }

//     if (just > 0) {
//       got += (size_t)just;
//       attempts = 0;
//       continue;
//     }

//     if (attempts++ >= max_attempts) {
//       return -999;  // timeout
//     }
//     (void)libtocksync_alarm_delay_ms(idle_slice_ms ? idle_slice_ms : 1);
//   }

//   return RETURNCODE_SUCCESS;
// }

// static int send_ack(uint16_t seq, uint8_t echoed_cmd, int32_t status) {
//   FrameHdr h = { .magic = {'D','L'}, .cmd = CMD_ACK, .flags = 0,
//                  .length = sizeof(int32_t), .meta = echoed_cmd, .seq = seq, .crc32 = 0 };
//   int32_t st = status;
//   fwrite(&h, 1, sizeof(h), stdout);
//   fwrite(&st, 1, sizeof(st), stdout);
//   fflush(stdout);
//   return 0;
// }

// static int send_err(uint16_t seq, uint8_t echoed_cmd, int32_t err) {
//   FrameHdr h = { .magic = {'D','L'}, .cmd = CMD_ERR, .flags = 0,
//                  .length = sizeof(int32_t), .meta = echoed_cmd, .seq = seq, .crc32 = 0 };
//   int32_t e = err;
//   fwrite(&h, 1, sizeof(h), stdout);
//   fwrite(&e, 1, sizeof(e), stdout);
//   fflush(stdout);
//   return 0;
// }

// int main(void) {
//   setbuf(stdout, NULL);
  
//   // Print startup message BEFORE entering protocol mode
//   printf("READY\n");
//   fflush(stdout);

//   if (!libtock_app_loader_exists()) { 
//     printf("ERROR_NO_DRIVER\n"); 
//     fflush(stdout);
//     return -1; 
//   }

//   size_t total_expected = 0; 
//   size_t binary_type = 0;

//   while (1) {
//     FrameHdr header;
    
//     // Read header - no debug prints during protocol
//     // if (read_exact((uint8_t*)&header, sizeof(header)) != RETURNCODE_SUCCESS) {
//     //   continue;
//     // }
//     if (read_exact_console((uint8_t*)&header, sizeof(header),
//                           READ_SOFT_TIMEOUT_MS * 3, READ_SPIN_SLICE_MS) != RETURNCODE_SUCCESS) {
//       continue;
//     }

//     // Validate magic
//     if (header.magic[0] != 'D' || header.magic[1] != 'L') {
//       continue;
//     }
    
//     // Right before the "if (header.length > MAX_CHUNK)" check:
//     if (header.length > MAX_CHUNK) { 
//       send_err(header.seq, header.cmd, -991); 
//       continue; 
//     }

//     // Read payload if present
//     if (header.length > 0) {
//       int r = read_exact_console(payload_buf, header.length,
//                                  READ_SOFT_TIMEOUT_MS, READ_SPIN_SLICE_MS);
//       if (r != RETURNCODE_SUCCESS) {
//         send_err(header.seq, header.cmd, r);
//         continue;
//       }
//     }

//     // Process command
//     switch (header.cmd) {
//       case CMD_PING:
//         send_ack(header.seq, CMD_PING, 0);
//         break;

//       case CMD_SYNC:
//         send_ack(header.seq, CMD_SYNC, 0);
//         break;

//       case CMD_SETUP: {
//         if (header.length != 8) { 
//           send_err(header.seq, CMD_SETUP, -992); 
//           break; 
//         }
        
//         uint32_t total = ((uint32_t*)payload_buf)[0];
//         total_expected = total;
//         binary_type    = header.meta;

//         int rc = libtock_app_loader_setup(total_expected, binary_type, app_setup_done_callback);
//         if (rc != RETURNCODE_SUCCESS) {
//           send_err(header.seq, CMD_SETUP, rc);
//           break;
//         }
        
//         yield_for(&setup_done);
//         setup_done = false;
        
//         // Debug output AFTER sending ACK
//         send_ack(header.seq, CMD_SETUP, 0);
//         // printf("[SETUP] total=%u type=%u\n", (unsigned)total_expected, (unsigned)binary_type);
//         // fflush(stdout);
//       } break;

//       case CMD_DATA: {
//         size_t off = header.meta;
//         size_t len = header.length;
        
//         int rc = libtock_app_loader_write(off, payload_buf, len);
//         if (rc == RETURNCODE_SUCCESS) {
//           send_ack(header.seq, CMD_DATA, (int32_t)len);
//           // Optional: Debug after ACK
//           // printf("[DATA] off=%u len=%u\n", (unsigned)off, (unsigned)len);
//         } else {
//           send_err(header.seq, CMD_DATA, rc);
//           // printf("[ERR] off=%u len=%u total=%u rc=%d\n", 
//           //    (unsigned)off, (unsigned)len, (unsigned)total_expected, rc);
//           fflush(stdout);
//         }
//       } break;

//       case CMD_FINAL: {
//         int rc = libtock_app_loader_finalize(app_finalize_done_callback);
//         if (rc != RETURNCODE_SUCCESS) { 
//           send_err(header.seq, CMD_FINAL, rc); 
//           break; 
//         }
        
//         yield_for(&finalize_done);
//         finalize_done = false;

//         rc = libtock_app_loader_load(app_load_done_callback);
//         if (rc != RETURNCODE_SUCCESS) {
//           send_err(header.seq, CMD_FINAL, rc);
//           break;
//         }
        
//         yield_for(&load_done);
//         load_done = false;
        
//         send_ack(header.seq, CMD_FINAL, 0);
//       } break;

//       default:
//         send_err(header.seq, header.cmd, -99);
//         break;
//     }
//   }
// }

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <libtock-sync/services/alarm.h>
#include <libtock/interface/button.h>
#include <libtock/kernel/app_loader.h>
#include <libtock/tock.h>

#include "kernel.h"

/******************************************************************************************************
* Callback Tracking Flags
******************************************************************************************************/
static bool setup_done    = false;   // to check if setup is done
static bool finalize_done = false;   // to check if the process was finalized successfully
static bool load_done     = false;   // to check if the process was loaded successfully
static bool app_load      = false;   // to check if there is a request to load a new app

/******************************************************************************************************
* Variables to hold loadable app information
******************************************************************************************************/
const char* new_app_name    = NULL;
unsigned char* new_app_data = NULL;
size_t new_app_size         = 0;
size_t new_binary_size      = 0;
size_t new_binary_type      = 1;
uint32_t write_buffer_size  = 4096;

/******************************************************************************************************
* Callback functions
*
* Set button callback to initiate the dynamic app load process on pressing buttons
******************************************************************************************************/

static void app_setup_done_callback(__attribute__((unused)) int   arg0,
                                    __attribute__((unused)) int   arg1,
                                    __attribute__((unused)) int   arg2,
                                    __attribute__((unused)) void* ud) {
  setup_done = true;
}

static void app_finalize_done_callback(__attribute__((unused)) int   arg0,
                                       __attribute__((unused)) int   arg1,
                                       __attribute__((unused)) int   arg2,
                                       __attribute__((unused)) void* ud) {
  finalize_done = true;
}

static void app_load_done_callback(int                           arg0,
                                   __attribute__((unused)) int   arg1,
                                   __attribute__((unused)) int   arg2,
                                   __attribute__((unused)) void* ud) {

  if (arg0 != RETURNCODE_SUCCESS) {
    printf("[Error] Process creation failed: %d.\n", arg0);
  } else {
    printf("[Success] Process created successfully.\n");
  }
  load_done = true;
}

// Callback for button presses.
static void button_callback(__attribute__ ((unused)) returncode_t retval, int btn_num, __attribute__ (
                              (unused)) bool pressed) {
  // Callback for button presses.
  if (btn_num == 0) {
    app_load        = true;
    new_app_name    = "kernel";
    new_app_data    = (uint8_t*)(uintptr_t)kernel_bin;
    new_app_size    = sizeof(kernel_bin);
    new_binary_size = sizeof(kernel_bin);
  }
}


/******************************************************************************************************
* Helper Function for the apploader machine
*
* Takes app size and the app binary as arguments
******************************************************************************************************/

static void appload(size_t binary_size, size_t app_size, size_t binary_type, uint8_t binary[]) {
  int ret = libtock_app_loader_setup(app_size, binary_type, app_setup_done_callback);
  if (ret != RETURNCODE_SUCCESS) {
    printf("[Error] Setup Failed: %d.\n", ret);
    tock_exit(ret);
  }

  // wait on setup done callback
  yield_for(&setup_done);
  setup_done = false;

  printf("[Success] Setup successful. Writing app to flash.\n");

  size_t offset = 0;
  while (offset < binary_size) {
    size_t chunk_len = (binary_size - offset > write_buffer_size)
                        ? write_buffer_size
                        : binary_size - offset;
    int ret1 = libtock_app_loader_write(offset, &binary[offset], chunk_len);
    if (ret1 != RETURNCODE_SUCCESS) {
      printf("[Error] Chunk write failed at offset %d\n", offset);
      break;
    }
    offset += chunk_len;
  }

  int ret2 = libtock_app_loader_finalize(app_finalize_done_callback);
  if (ret2 != RETURNCODE_SUCCESS) {
    printf("[Error] Finalizing app failed: %d.\n", ret2);
    tock_exit(ret2);
  }

  // wait on finalize done callback
  yield_for(&finalize_done);
  finalize_done = false;

  printf("[Success] App flashed successfully. Creating process now.\n");
  int ret3 = libtock_app_loader_load(app_load_done_callback);
  if (ret3 != RETURNCODE_SUCCESS) {
    printf("[Error] Process creation failed: %d.\n", ret3);
    tock_exit(ret3);
  }

  // wait on load done callback
  yield_for(&load_done);
  load_done = false;

  printf("[Log] Waiting for a button press.\n");
}

/******************************************************************************************************
* Main
******************************************************************************************************/

int main(void) {
  printf("[Log] Simple test app to load a kernel dynamically.\n");

  // check if app loader driver exists
  if (!libtock_app_loader_exists()) {
    printf("No App Loader driver!\n");
    return -1;
  }

  // int count;
  // int err = libtock_button_count(&count);
  // // Ensure there is a button to use.
  // if (err < 0) return err;
  // printf("[Log] There are %d buttons on this board.\n", count);

  // Enable interrupts on each button.
  // for (int i = 0; i < count; i++) {
  //   libtock_button_notify_on_press(i, button_callback);
  // }
  libtock_button_notify_on_press(0, button_callback);

  while (1) {
    if (app_load) {
      printf("[Event] Button for %s pressed!\n", new_app_name);
      printf("size: %d bytes\n", new_app_size);
      appload(new_binary_size, new_app_size, new_binary_type, new_app_data);
      app_load = false;
    }
    yield();
  }
}
