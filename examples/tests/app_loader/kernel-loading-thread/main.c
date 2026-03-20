#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <libtock-sync/services/alarm.h>
#include <libtock/interface/led.h>
#include <libtock/tock.h>
#include <libtock/kernel/app_loader.h>

#include <libopenthread/platform/openthread-system.h>
#include <libopenthread/platform/plat.h>
#include <openthread/dataset_ftd.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/tasklet.h>
#include <openthread/thread.h>
#include <openthread/udp.h>

// ------------ Framing (same as your serial variant) ------------
enum {
  CMD_PING  = 0x01,
  CMD_SETUP = 0x10,
  CMD_DATA  = 0x11,
  CMD_FINAL = 0x12,
  CMD_ACK   = 0x80,
  CMD_ERR   = 0x81,
};

typedef struct __attribute__((packed)) {
  uint8_t  magic[2];   // 'D','L'
  uint8_t  cmd;
  uint8_t  flags;
  uint32_t length;     // payload bytes
  uint32_t meta;       // type (SETUP) or offset (DATA)
  uint16_t seq;        // host’s sequence
  uint32_t crc32;      // 0 = unused
} FrameHdr;

#define FRAME_MAGIC0 'D'
#define FRAME_MAGIC1 'L'

// ------------ Transport / sizing ------------
#define RX_UDP_PORT  12122   // device listens here
#define TX_UDP_PORT  12123   // device replies here
#define MAX_CHUNK    512

// ------------ OpenThread globals ------------
static otUdpSocket sUdpSocket;

// ------------ Loader state & callbacks ------------
static bool setup_done=false, finalize_done=false, load_done=false;
static size_t total_expected=0, binary_type=0;

static void cb_setup(__attribute__((unused)) int a0, __attribute__((unused)) int a1,
                     __attribute__((unused)) int a2, __attribute__((unused)) void* ud) { setup_done=true; }
static void cb_final(__attribute__((unused)) int a0, __attribute__((unused)) int a1,
                     __attribute__((unused)) int a2, __attribute__((unused)) void* ud) { finalize_done=true; }
static void cb_load (int a0, __attribute__((unused)) int a1,
                     __attribute__((unused)) int a2, __attribute__((unused)) void* ud) { 
                      if (a0 != RETURNCODE_SUCCESS) {
                        printf("[Error] Process creation failed: %d.\n", a0);
                      } else {
                        printf("[Success] Process created successfully.\n");
                      }
                      load_done = true; }

// ------------ Helpers: ACK/ERR over UDP ------------
static void send_ack(otInstance* ot, const otMessageInfo* peer,
                     uint16_t seq, uint8_t echoed_cmd, int32_t status)
{
  FrameHdr h = { .magic = {FRAME_MAGIC0, FRAME_MAGIC1}, .cmd = CMD_ACK,
                 .flags = 0, .length = sizeof(int32_t), .meta = echoed_cmd,
                 .seq = seq, .crc32 = 0 };
  otMessage* m = otUdpNewMessage(ot, NULL);
  if (!m) return;
  (void)otMessageAppend(m, &h, sizeof(h));
  (void)otMessageAppend(m, &status, sizeof(status));
  otMessageInfo mi = *peer; mi.mPeerPort = TX_UDP_PORT;
  (void)otUdpSend(ot, &sUdpSocket, m, &mi);
}

static void send_err(otInstance* ot, const otMessageInfo* peer,
                     uint16_t seq, uint8_t echoed_cmd, int32_t err)
{
  FrameHdr h = { .magic = {FRAME_MAGIC0, FRAME_MAGIC1}, .cmd = CMD_ERR,
                 .flags = 0, .length = sizeof(int32_t), .meta = echoed_cmd,
                 .seq = seq, .crc32 = 0 };
  otMessage* m = otUdpNewMessage(ot, NULL);
  if (!m) return;
  (void)otMessageAppend(m, &h, sizeof(h));
  (void)otMessageAppend(m, &err, sizeof(err));
  otMessageInfo mi = *peer; mi.mPeerPort = TX_UDP_PORT;
  (void)otUdpSend(ot, &sUdpSocket, m, &mi);
}

// ------------ Dataset / join config (edit as you like) ------------
static void setNetworkConfiguration(otInstance* aInstance) {
  otOperationalDataset ds;
  memset(&ds, 0, sizeof(ds));

  ds.mChannel = 26; ds.mComponents.mIsChannelPresent = true;
  ds.mPanId   = (otPanId)0xabcd; ds.mComponents.mIsPanIdPresent = true;

  uint8_t key[OT_NETWORK_KEY_SIZE] = {
    0x00,0x11,0x22,0x33, 0x44,0x55,0x66,0x77,
    0x88,0x99,0xaa,0xbb, 0xcc,0xdd,0xee,0xff
  };
  memcpy(ds.mNetworkKey.m8, key, sizeof(ds.mNetworkKey));
  ds.mComponents.mIsNetworkKeyPresent = true;

  otError e = otDatasetSetActive(aInstance, &ds);
  assert(e == OT_ERROR_NONE);
}

// ------------ Debug: print all IPv6 addrs ------------
static void print_ip_addr(otInstance* instance) {
  char addr[64];
  const otNetifAddress* ua = otIp6GetUnicastAddresses(instance);
  printf("[THREAD] IPv6:\n");
  for (const otNetifAddress* p = ua; p; p = p->mNext) {
    otIp6AddressToString(&p->mAddress, addr, sizeof(addr));
    printf("  %s\n", addr);
  }
}

// Helper method that registers a stateChangeCallback to print
// when state changes occur (useful for debugging).
static void stateChangeCallback(uint32_t flags, void* context) {
	otInstance* instance = (otInstance*)context;
	if (!(flags & OT_CHANGED_THREAD_ROLE)) {
		return;
	}

	switch (otThreadGetDeviceRole(instance)) {
    case OT_DEVICE_ROLE_DISABLED:
		printf("[State Change] - Disabled.\n");
		break;
    case OT_DEVICE_ROLE_DETACHED:
		printf("[State Change] - Detached.\n");
		g_connected = false;
		break;
    case OT_DEVICE_ROLE_CHILD:
		printf("[State Change] - Child.\n");
		printf("Successfully attached to Thread network as a child.\n");
		g_connected = true;
		break;
    case OT_DEVICE_ROLE_ROUTER:
		printf("[State Change] - Router.\n");
		break;
    case OT_DEVICE_ROLE_LEADER:
		printf("[State Change] - Leader.\n");
		break;
    default:
		break;
	}

	if (g_connected) {
		libtock_led_on(0);
	} else {
		libtock_led_off(0);
	}
}

// ------------ UDP RX handler (kernel loader transport) ------------
static void handle_packet_rx(void* aContext, otMessage* aMessage, const otMessageInfo* aMessageInfo)
{
  otInstance* ot = (otInstance*)aContext;

  // basic length check and header read
  if (otMessageGetLength(aMessage) < (int)sizeof(FrameHdr)) return;
  FrameHdr h;
  (void)otMessageRead(aMessage, 0, &h, sizeof(h));
  if (h.magic[0] != FRAME_MAGIC0 || h.magic[1] != FRAME_MAGIC1) return;

  // payload buffer
  uint8_t buf[MAX_CHUNK];
  if (h.length > sizeof(buf)) { send_err(ot, aMessageInfo, h.seq, h.cmd, -2); return; }
  if (h.length) (void)otMessageRead(aMessage, sizeof(h), buf, h.length);

  switch (h.cmd) {
    case CMD_PING: {
      send_ack(ot, aMessageInfo, h.seq, CMD_PING, 0);
    } break;

    case CMD_SETUP: {
      if (h.length != 8) { send_err(ot, aMessageInfo, h.seq, CMD_SETUP, -10); break; }
      uint32_t total = ((uint32_t*)buf)[0];
      total_expected = total;
      binary_type    = h.meta;

      int rc = libtock_app_loader_setup(total_expected, binary_type, cb_setup);
      if (rc != RETURNCODE_SUCCESS) { send_err(ot, aMessageInfo, h.seq, CMD_SETUP, rc); break; }
      yield_for(&setup_done); setup_done=false;

      printf("[OTLOADER] setup ok total=%lu type=%lu\n",
             (unsigned long)total_expected, (unsigned long)binary_type);
      send_ack(ot, aMessageInfo, h.seq, CMD_SETUP, 0);
    } break;

    case CMD_DATA: {
      size_t off = h.meta, len = h.length;
      if (len == 0 || off + len > total_expected) {
        send_err(ot, aMessageInfo, h.seq, CMD_DATA, -22);
        break;
      }
      int rc = libtock_app_loader_write(off, buf, len);
      if (rc != RETURNCODE_SUCCESS) {
        printf("[OTLOADER] write fail off=%lu len=%lu rc=%d\n",
               (unsigned long)off, (unsigned long)len, rc);
        send_err(ot, aMessageInfo, h.seq, CMD_DATA, rc);
        break;
      }
      send_ack(ot, aMessageInfo, h.seq, CMD_DATA, (int32_t)len);
    } break;

    case CMD_FINAL: {
      int rc = libtock_app_loader_finalize(cb_final);
      if (rc != RETURNCODE_SUCCESS) { send_err(ot, aMessageInfo, h.seq, CMD_FINAL, rc); break; }
      yield_for(&finalize_done); finalize_done=false;

      rc = libtock_app_loader_load(cb_load);
      if (rc != RETURNCODE_SUCCESS) { send_err(ot, aMessageInfo, h.seq, CMD_FINAL, rc); break; }
      yield_for(&load_done); load_done=false;

      send_ack(ot, aMessageInfo, h.seq, CMD_FINAL, 0);
      printf("[OTLOADER] load complete\n");
    } break;

    default:
      send_err(ot, aMessageInfo, h.seq, h.cmd, -99);
      break;
  }
}

// ------------ UDP init ------------
static void initUdp(otInstance* ot) {
  memset(&sUdpSocket, 0, sizeof(sUdpSocket));
  otSockAddr listen;
  memset(&listen, 0, sizeof(listen));
  listen.mPort = RX_UDP_PORT;

  otUdpOpen(ot, &sUdpSocket, handle_packet_rx, ot);
  otUdpBind(ot, &sUdpSocket, &listen, OT_NETIF_THREAD);
}

// ------------ main ------------
int main(__attribute__((unused)) int argc, __attribute__((unused)) char* argv[])
{
  printf("kernel loading thread helper app\n");
  // Initialize OpenThread instance.
  otSysInit(argc, argv);
  otInstance* ot = otInstanceInitSingle();
  assert(ot);

  // set child timeout to 60 seconds.
	otThreadSetChildTimeout(ot, 60);
  // Set callback to be notified when thread state changes.
	otSetStateChangedCallback(ot, stateChangeCallback, ot);
  ///////////////////////////////////////////////////
	// THREAD NETWORK SETUP HERE

	// Configure network.
	setNetworkConfiguration(ot);

	// Init UDP interface.
	initUdp(ot);

  // Enable network interface.
	while (otIp6SetEnabled(ot, true) != OT_ERROR_NONE) {
		printf("Failed to start Thread network interface!\n");
		libtocksync_alarm_delay_ms(100);
	}

  // Print IPv6 address.
	print_ip_addr(ot);

  // Start Thread network.
	while (otThreadSetEnabled(ot, true) != OT_ERROR_NONE) {
		printf("Failed to start Thread stack!\n");
		libtocksync_alarm_delay_ms(100);
	}

  printf("going to run open thread pump");

  // Run OpenThread pump
  for (;;) {
    otTaskletsProcess(ot);
    otSysProcessDrivers(ot);
    if (!otTaskletsArePending(ot)) {
      yield();
    }
  }
  return 0;
}
