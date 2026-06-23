#include <assert.h>
// #include <stdio.h>
// #include <string.h>
// #include <stdbool.h>

#include <libopenthread/platform/openthread-system.h>
#include <libopenthread/platform/plat.h>
// #include <openthread/platform/alarm-milli.h>
#include <openthread/dataset.h>
#include <openthread/instance.h>
// #include <openthread/link.h>
#include <openthread/tasklet.h>
#include <openthread/thread.h>
#include <openthread/udp.h>

// #include <libtock-sync/services/alarm.h>

#include <libtock/kernel/ipc.h>
#include <libtock/tock.h>

// Forward declarations
static void setNetworkConfiguration(otInstance* aInstance);
void initUdp(otInstance* aInstance);
void sendUdpTemperature(otInstance* aInstance, uint8_t temperature);

// UDP socket
static otUdpSocket sUdpSocket;

// Temperature transmission state
static uint8_t pending_temperature = 0;
static bool transmit_pending = false;
static int client_pid = -1;

// IPC callback - receives temperature from sensor app
static void openthread_ipc_callback(int pid, int len, int buf,
                                    __attribute__((unused)) void* ud) {
  if (len < (int)sizeof(uint8_t)) {
    // printf("[OT] ERROR: IPC buffer too small (%d bytes, expected %zu)\n",
    //        len, sizeof(uint8_t));
    return;
  }

  pending_temperature = *((uint8_t*)buf);
  transmit_pending = true;
  client_pid = pid;

  // printf("[OT] Received temperature from sensor (PID %d): %d°F\n",
  //        pid, pending_temperature);
}

// static libtock_alarm_t ot_alarm;
// static bool ot_alarm_fired = false;

// static void ot_alarm_cb(__attribute__((unused)) uint32_t now,
//                         __attribute__((unused)) uint32_t scheduled,
//                         __attribute__((unused)) void* opaque) {
//     ot_alarm_fired = true;
// }

int main(__attribute__((unused)) int argc, __attribute__((unused)) char* argv[]) {
  ipc_register_service_callback("openthread_app", openthread_ipc_callback, NULL);

  // printf("OpenThread Service\n");

  otSysInit(argc, argv);
  otInstance* instance = otInstanceInitSingle();
  assert(instance);

  setNetworkConfiguration(instance);
  initUdp(instance);
  
  otIp6SetEnabled(instance, true);
  otThreadSetEnabled(instance, true);

  // // Attachment loop WITH yield
  // while (otThreadGetDeviceRole(instance) < OT_DEVICE_ROLE_CHILD) {
  //     otTaskletsProcess(instance);
  //     otSysProcessDrivers(instance);
  //     if (!otTaskletsArePending(instance) && !openthread_platform_pending_work()) {
  //         yield();
  //     }
  //     if (otThreadGetDeviceRole(instance) == OT_DEVICE_ROLE_CHILD){
  //       printf("Attached to nwk as child\n");
  //     }
  // }

  // otLinkModeConfig mode;
  // memset(&mode, 0, sizeof(mode));
  // mode.mRxOnWhenIdle = false;
  // mode.mDeviceType   = false;
  // mode.mNetworkData  = false;
  // otThreadSetLinkMode(instance, mode);
  // otLinkSetPollPeriod(instance, 240000);


  // uint32_t start = otPlatAlarmMilliGetNow();
  // while (otPlatAlarmMilliGetNow() - start < 5000) {
  //     otTaskletsProcess(instance);
  //     otSysProcessDrivers(instance);
  //     if (!otTaskletsArePending(instance) && !openthread_platform_pending_work()) {
  //         yield();
  //     }
  // }
  // printf("Attached successfully! Transitioning to main loop.\n");

  for (;;) {
    otTaskletsProcess(instance);
    otSysProcessDrivers(instance);

    if (transmit_pending) {
      otDeviceRole role = otThreadGetDeviceRole(instance);
      // printf("Role: %d\n", role);

      if (role >= OT_DEVICE_ROLE_CHILD) {
        // printf("[OT] Transmitting temperature: %d°F\n", pending_temperature);
        // otLinkSendDataRequest(instance);
        sendUdpTemperature(instance, pending_temperature);
      } 
      // else {
      //   // printf("[OT] WARNING: Temperature pending but network not ready (role: %d)\n", role);
      // }

      if (client_pid >= 0) {
        ipc_notify_client(client_pid);
        // printf("[OT] Sent ACK to sensor client (PID %d)\n", client_pid);
      }

      transmit_pending = false;
    }

    if (!otTaskletsArePending(instance) && !openthread_platform_pending_work()) {
      yield();
      // yield_no_wait();
      // libtocksync_alarm_delay_ms(100);
    }
    // if (!otTaskletsArePending(instance) && !openthread_platform_pending_work()) {
    //     ot_alarm_fired = false;
    //     libtock_alarm_in_ms(1000, ot_alarm_cb, NULL, &ot_alarm);
    //     libtocksync_alarm_yield_for_with_timeout(&ot_alarm_fired, 1000);
    // }
  }

  return 0;
}

static void setNetworkConfiguration(otInstance* aInstance) {
  otOperationalDataset aDataset;
  memset(&aDataset, 0, sizeof(otOperationalDataset));

  aDataset.mChannel = 26;
  aDataset.mComponents.mIsChannelPresent = true;

  aDataset.mPanId = (otPanId)0xabcd;
  aDataset.mComponents.mIsPanIdPresent = true;

  uint8_t key[OT_NETWORK_KEY_SIZE] = {
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
  };
  memcpy(aDataset.mNetworkKey.m8, key, sizeof(aDataset.mNetworkKey));
  aDataset.mComponents.mIsNetworkKeyPresent = true;

  otError error = otDatasetSetActive(aInstance, &aDataset);
  assert(error == OT_ERROR_NONE);
}

void initUdp(otInstance* aInstance) {
  otSockAddr listenSockAddr;

  memset(&sUdpSocket, 0, sizeof(sUdpSocket));
  memset(&listenSockAddr, 0, sizeof(listenSockAddr));

  listenSockAddr.mPort = 1212;

  otUdpOpen(aInstance, &sUdpSocket, NULL, NULL);
  otUdpBind(aInstance, &sUdpSocket, &listenSockAddr, OT_NETIF_THREAD);

  // printf("UDP socket opened on port 1212\n");
}

void sendUdpTemperature(otInstance* aInstance, uint8_t temperature) {
  otError error;
  otMessage* message;
  otMessageInfo messageInfo;
  otIp6Address destinationAddr;

  memset(&messageInfo, 0, sizeof(messageInfo));

  otIp6AddressFromString("ff02::02", &destinationAddr);
  messageInfo.mPeerAddr = destinationAddr;
  messageInfo.mPeerPort = 1212;

  message = otUdpNewMessage(aInstance, NULL);
  if (message == NULL) {
    // printf("[OT] ERROR: Failed to create UDP message\n");
    return;
  }

  error = otMessageAppend(message, &temperature, 1);
  if (error != OT_ERROR_NONE) {
    // printf("[OT] ERROR: Failed to append data to UDP message\n");
    otMessageFree(message);
    return;
  }

  error = otUdpSend(aInstance, &sUdpSocket, message, &messageInfo);
  if (error != OT_ERROR_NONE) {
    // printf("[OT] ERROR: Failed to send UDP packet (error: %d)\n", error);
    otMessageFree(message);
  } else {
    // printf("[OT] UDP packet sent successfully to ff02::02:1212\n");
  }
}