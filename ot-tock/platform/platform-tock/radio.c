// TODO COPYRIGHT

#include <ieee802154.h>
#include <openthread/platform/radio.h>
#include <openthread-system.h>
#include <stdio.h>
#include <plat.h>

#define ACK_SIZE 3

static uint8_t tx_mPsdu[OT_RADIO_FRAME_MAX_SIZE];
static uint8_t ack_mPSdu[ACK_SIZE] = {0x02, 0x00, 0x00};

static otRadioFrame transmitFrame = {
    .mPsdu = tx_mPsdu,
    .mLength = OT_RADIO_FRAME_MAX_SIZE
};

static otRadioFrame ackFrame = {
    .mPsdu = ack_mPSdu,
    .mLength = 3
};

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64) {
    // TODO HARDCODED FOR NOW
    OT_UNUSED_VARIABLE(aInstance);
    aIeeeEui64[0] = 0xf4;
    aIeeeEui64[1] = 0xce;
    aIeeeEui64[2] = 0x36;
    aIeeeEui64[3] = 0x67;
    aIeeeEui64[4] = 0x13;
    aIeeeEui64[5] = 0x12;
    aIeeeEui64[6] = 0x3f;
    aIeeeEui64[7] = 0xa6;
}

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t aPanid) {
    OT_UNUSED_VARIABLE(aInstance);
    ieee802154_set_pan(aPanid);
    ieee802154_config_commit();
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddress) {
    OT_UNUSED_VARIABLE(aInstance);

    // convert aExtAddress to be big endian in temp variable
    otExtAddress temp;
    for (int i = 0; i < 8; i++) {
        temp.m8[i] = aExtAddress->m8[7-i];
    }


    int retCode = ieee802154_set_address_long((unsigned char*) &temp);
    assert(retCode==0);

    if (retCode != 0) printf("Error setting long address");
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aShortAddress) {
    OT_UNUSED_VARIABLE(aInstance);
    ieee802154_set_address(aShortAddress);
    ieee802154_config_commit();
}

bool otPlatRadioIsEnabled(otInstance *aInstance) {
    OT_UNUSED_VARIABLE(aInstance);
    return ieee802154_is_up();
}

otError otPlatRadioEnable(otInstance *aInstance) {
    OT_UNUSED_VARIABLE(aInstance);

    int retCode = ieee802154_up();
    if (retCode == 0)
        return OT_ERROR_NONE;
    else {
        printf("Initializing Radio Failed!\n");
        return OT_ERROR_FAILED;
    }
}

otError otPlatRadioDisable(otInstance *aInstance) {
    OT_UNUSED_VARIABLE(aInstance);

    int retCode = ieee802154_down();

    if (retCode == 0)
        return OT_ERROR_NONE;
    else {
        printf("Disable Radio Failed!\n");
        return OT_ERROR_FAILED;
    }
}

otError otPlatRadioSleep(otInstance *aInstance) {
    // TODO: There is no sleep function.
    OT_UNUSED_VARIABLE(aInstance);
    return OT_ERROR_NONE;
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel) {
    // TODO
    OT_UNUSED_VARIABLE(aInstance);
    
    // switch to channel TODO
    if (aChannel != 26) {
        return OT_ERROR_NONE;
    }

    otError result = otTockStartReceive(aChannel);
    return result;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame) {
    // TODO
    OT_UNUSED_VARIABLE(aInstance);
    // For some reason, the frame length is 2 bytes longer than the actual frame length
    // this is corrected here. I have looked within openthread as to why this is 
    // happening, but I cannot find the source of the problem. This "magic number"
    // fix is not ideal, but fixes the issue.

    aFrame->mLength = aFrame->mLength-2;

    if (aFrame->mChannel != 26) {
            otPlatRadioTxDone(aInstance, aFrame, NULL, OT_ERROR_NONE);

        return OT_ERROR_NONE;
    }

    int send_result =  ieee802154_send_direct((char*) aFrame->mPsdu, aFrame->mLength);
    
    // Send direct does support ACK so no ACK is also considered a successful transmission
    if (send_result != RETURNCODE_SUCCESS && send_result != RETURNCODE_ENOACK){
        return OT_ERROR_FAILED;
    }

    otPlatRadioTxDone(aInstance, aFrame, &ackFrame, OT_ERROR_NONE);

    return OT_ERROR_NONE;
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance) {
    OT_UNUSED_VARIABLE(aInstance);
    return &transmitFrame;
}

int8_t otPlatRadioGetRssi(otInstance *aInstance) {
    // TODO
    OT_UNUSED_VARIABLE(aInstance);
    // printf("%s:%d in %s\n", __FILE__, __LINE__, __func__);
    return -50;
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance) {
    // TODO: This is temporarily saying there are no capabilities.
    OT_UNUSED_VARIABLE(aInstance);
    return (otRadioCaps)(OT_RADIO_CAPS_CSMA_BACKOFF);
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance) {
    // Tock sets its radio to promiscuous mode by default. OT will not
    // operate when the radio is in promiscuous mode. We return false 
    // here, but this is not true.
    OT_UNUSED_VARIABLE(aInstance);
    return false;
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable) {
    // TODO
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aEnable);
    printf("%s:%d in %s\n", __FILE__, __LINE__, __func__);
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable) {
    // TODO
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aEnable);    
    printf("%s:%d in %s\n", __FILE__, __LINE__, __func__);
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, uint16_t aShortAddress) {
    // TODO
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aShortAddress);
    printf("%s:%d in %s\n", __FILE__, __LINE__, __func__);
    return OT_ERROR_NONE;
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress) {
    // TODO
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aExtAddress);
    printf("%s:%d in %s\n", __FILE__, __LINE__, __func__);
    return OT_ERROR_NONE;
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, uint16_t aShortAddress) {
    // TODO
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aShortAddress);
    printf("%s:%d in %s\n", __FILE__, __LINE__, __func__);
    return OT_ERROR_NONE;
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress) {
    // TODO
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aExtAddress);
    printf("%s:%d in %s\n", __FILE__, __LINE__, __func__);
    return OT_ERROR_NONE;
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance) {
    // TODO
    OT_UNUSED_VARIABLE(aInstance);
    printf("%s:%d in %s\n", __FILE__, __LINE__, __func__);
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance) {
    // TODO
    OT_UNUSED_VARIABLE(aInstance);
    printf("%s:%d in %s\n", __FILE__, __LINE__, __func__);
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration) {
    // TODO
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aScanChannel);
    OT_UNUSED_VARIABLE(aScanDuration);
    printf("%s:%d in %s\n", __FILE__, __LINE__, __func__);
    return OT_ERROR_NONE;
}

otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower) {
    // TODO
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aPower);
    printf("%s:%d in %s\n", __FILE__, __LINE__, __func__);
    return OT_ERROR_NONE;
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower) {
    // TODO
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aPower);
    printf("%s:%d in %s\n", __FILE__, __LINE__, __func__);
    return OT_ERROR_NONE;
}

otError otPlatRadioGetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t *aThreshold) {
    // TODO
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aThreshold);

    printf("%s:%d in %s\n", __FILE__, __LINE__, __func__);
    return OT_ERROR_NONE;
}

otError otPlatRadioSetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t aThreshold) {
    // TODO
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aThreshold);
    printf("%s:%d in %s\n", __FILE__, __LINE__, __func__);
    return OT_ERROR_NONE;
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance) {
    // TODO
    OT_UNUSED_VARIABLE(aInstance);
    // printf("%s:%d in %s\n", __FILE__, __LINE__, __func__);
    return -50;
}