#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <libopenthread/platform/openthread-system.h>
#include <libopenthread/platform/plat.h>
#include <openthread/dataset_ftd.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/tasklet.h>
#include <openthread/thread.h>
#include <openthread/coap.h>
#include <openthread/udp.h>

#include <libtock-sync/services/alarm.h>
#include <libtock/kernel/ipc.h>
#include <libtock/services/alarm.h>
#include <libtock/tock.h>

// Forward declarations
static void setNetworkConfiguration(otInstance* aInstance);
static void stateChangeCallback(uint32_t flags, void* context);
static void print_ip_addr(otInstance* instance);
void initUdp(otInstance* aInstance);
void sendUdpTemperature(otInstance* aInstance, uint8_t temperature);
void handleUdpRecvTemperature(void* aContext, otMessage* aMessage,
                              const otMessageInfo* aMessageInfo);

// UDP socket
static otUdpSocket sUdpSocket;

// Temperature transmission state
static uint8_t pending_temperature = 0;
static bool transmit_pending = false;
static int client_pid = -1;
static bool network_up = false;

// IPC callback - receives temperature from sensor apps
static void openthread_ipc_callback(int pid, int len, int buf,
                                    __attribute__((unused)) void* ud) {
  // Validate buffer size
  if (len < (int)sizeof(uint8_t)) {
    // printf("[OT] ERROR: IPC buffer too small (%d bytes, expected %zu)\n",
    //        len, sizeof(uint8_t));
    return;
  }

  // Read temperature from sensor's shared buffer
  pending_temperature = *((uint8_t*)buf);
  transmit_pending = true;
  client_pid = pid;

  // printf("[OT] Received temperature from sensor (PID %d): %d°F\n",
  //        pid, pending_temperature);
}

int main(__attribute__((unused)) int argc, __attribute__((unused)) char* argv[]) {
  // printf("OpenThread Service\n");
  // printf("==================\n\n");

  // Initialize OpenThread instance
  otSysInit(argc, argv);
  otInstance* instance = otInstanceInitSingle();
  assert(instance);

  // Register this application as an IPC service
  ipc_register_service_callback("openthread-ipc",
                                openthread_ipc_callback,
                                NULL);

  // Set child timeout to 60 seconds
  otThreadSetChildTimeout(instance, 60);

  // Set callback to be notified when Thread state changes
  otSetStateChangedCallback(instance, stateChangeCallback, instance);

  // Configure Thread network
  // printf("Configuring Thread network...\n");
  setNetworkConfiguration(instance);

  // Initialize UDP
  initUdp(instance);

  // Enable network interface
  // printf("Starting network interface...\n");
  while (otIp6SetEnabled(instance, true) != OT_ERROR_NONE) {
    printf("Failed to start Thread network interface, retrying...\n");
    libtocksync_alarm_delay_ms(100);
  }

  // Print IPv6 address
  print_ip_addr(instance);

  // Start Thread network
  // printf("Starting Thread stack...\n");
  while (otThreadSetEnabled(instance, true) != OT_ERROR_NONE) {
    // printf("Failed to start Thread stack, retrying...\n");
    libtocksync_alarm_delay_ms(100);
  }

  // printf("OpenThread service ready!\n\n");

  // OpenThread main loop
  for ( ;;) {
    // Execute any pending OpenThread related work
    otTaskletsProcess(instance);

    // Execute any platform related work (e.g. check radio buffer for new packets)
    otSysProcessDrivers(instance);

    // Transmit pending temperature if available
    if (transmit_pending && network_up) {
      // printf("[OT] Transmitting temperature: %d°F\n", pending_temperature);
      sendUdpTemperature(instance, pending_temperature);
      
      // Acknowledge to the sensor client
      if (client_pid >= 0) {
        ipc_notify_client(client_pid);
        // printf("[OT] Sent ACK to sensor client (PID %d)\n", client_pid);
      }
      
      transmit_pending = false;
    } else if (transmit_pending && !network_up) {
      // printf("[OT] WARNING: Temperature pending but network not ready\n");
      // Still acknowledge so sensor doesn't time out
      if (client_pid >= 0) {
        ipc_notify_client(client_pid);
      }
      transmit_pending = false;
    }

    // If there is no pending platform or OpenThread related work, yield
    if (!otTaskletsArePending(instance) &&
        !openthread_platform_pending_work()) {
      yield();
    }
  }

  return 0;
}

// Configure OpenThread network dataset
void setNetworkConfiguration(otInstance* aInstance) {
  otOperationalDataset aDataset;
  memset(&aDataset, 0, sizeof(otOperationalDataset));

  // Set Channel to 26
  aDataset.mChannel = 26;
  aDataset.mComponents.mIsChannelPresent = true;

  // Set Pan ID to 0xabcd
  aDataset.mPanId = (otPanId)0xabcd;
  aDataset.mComponents.mIsPanIdPresent = true;

  // Set network key
  uint8_t key[OT_NETWORK_KEY_SIZE] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
  };
  memcpy(aDataset.mNetworkKey.m8, key, sizeof(aDataset.mNetworkKey));
  aDataset.mComponents.mIsNetworkKeyPresent = true;

  otError error = otDatasetSetActive(aInstance, &aDataset);
  assert(error == OT_ERROR_NONE);
}

// State change callback
static void stateChangeCallback(uint32_t flags, void* context) {
  otInstance* instance = (otInstance*)context;
  
  if (!(flags & OT_CHANGED_THREAD_ROLE)) {
    return;
  }

  switch (otThreadGetDeviceRole(instance)) {
    case OT_DEVICE_ROLE_DISABLED:
      // printf("[State Change] Disabled\n");
      network_up = false;
      break;
    case OT_DEVICE_ROLE_DETACHED:
      // printf("[State Change] Detached\n");
      network_up = false;
      break;
    case OT_DEVICE_ROLE_CHILD:
      // printf("[State Change] Child\n");
      // printf("Successfully attached to Thread network!\n");
      network_up = true;
      break;
    case OT_DEVICE_ROLE_ROUTER:
      // printf("[State Change] Router\n");
      network_up = true;
      break;
    case OT_DEVICE_ROLE_LEADER:
      // printf("[State Change] Leader\n");
      network_up = true;
      break;
    default:
      break;
  }
}

// Print IPv6 addresses
static void print_ip_addr(otInstance* instance) {
  char addr_string[64];
  const otNetifAddress* unicastAddrs = otIp6GetUnicastAddresses(instance);

  // printf("[THREAD] Device IPv6 Addresses:\n");
  for (const otNetifAddress* addr = unicastAddrs; addr; addr = addr->mNext) {
    const otIp6Address ip6_addr = addr->mAddress;
    otIp6AddressToString(&ip6_addr, addr_string, sizeof(addr_string));
    printf("  %s\n", addr_string);
  }
}

// Initialize UDP
void initUdp(otInstance* aInstance) {
  otSockAddr listenSockAddr;

  memset(&sUdpSocket, 0, sizeof(sUdpSocket));
  memset(&listenSockAddr, 0, sizeof(listenSockAddr));

  listenSockAddr.mPort = 1212;

  otUdpOpen(aInstance, &sUdpSocket, handleUdpRecvTemperature, aInstance);
  otUdpBind(aInstance, &sUdpSocket, &listenSockAddr, OT_NETIF_THREAD);
  
  // printf("UDP socket opened on port 1212\n");
}

// Handle incoming UDP packets
void handleUdpRecvTemperature(void* aContext, otMessage* aMessage,
                              const otMessageInfo* aMessageInfo) {
  OT_UNUSED_VARIABLE(aContext);
  char buf[2];

  const otIp6Address sender_addr = aMessageInfo->mPeerAddr;
  char addr_string[64];
  otIp6AddressToString(&sender_addr, addr_string, sizeof(addr_string));

  otMessageRead(aMessage, otMessageGetOffset(aMessage), buf, sizeof(buf) - 1);
  
  // printf("[OT] Received UDP from %s: temperature = %d°F\n",
  //        addr_string, (uint8_t)buf[0]);
}

// Send temperature via UDP
void sendUdpTemperature(otInstance* aInstance, uint8_t temperature) {
  otError error = OT_ERROR_NONE;
  otMessage* message;
  otMessageInfo messageInfo;
  otIp6Address destinationAddr;

  memset(&messageInfo, 0, sizeof(messageInfo));

  // Multicast to all Thread nodes (ff02::02)
  otIp6AddressFromString("ff02::02", &destinationAddr);
  messageInfo.mPeerAddr = destinationAddr;
  messageInfo.mPeerPort = 1212;

  message = otUdpNewMessage(aInstance, NULL);
  if (message == NULL) {
    printf("[OT] ERROR: Failed to create UDP message\n");
    return;
  }

  // Append 1-byte temperature
  error = otMessageAppend(message, &temperature, 1);
  if (error != OT_ERROR_NONE) {
    printf("[OT] ERROR: Failed to append data to UDP message\n");
    otMessageFree(message);
    return;
  }

  error = otUdpSend(aInstance, &sUdpSocket, message, &messageInfo);
  if (error != OT_ERROR_NONE) {
    printf("[OT] ERROR: Failed to send UDP packet (error: %d)\n", error);
    otMessageFree(message);
  } else {
    printf("[OT] UDP packet sent successfully to ff02::02:1212\n");
  }
}