#include "ch32fun.h"
#include <stdio.h>

#include "iSLER.h"

#ifdef USB_USE_USBD
// the USBD peripheral is not recommended, but there are really nice
// v208 boards with only those pins brought out, so support is hacked in
#include "usbd.h"
#define USBFSSetup         USBDSetup
#define USBFS_SetupReqLen  USBD_SetupReqLen
#define USBFS_SetupReqType USBD_SetupReqType
#define USBFS_PACKET_SIZE  DEF_USBD_UEP0_SIZE
int USBFS_SendEndpointNEW(int ep, uint8_t *data, int len, int copy) { return USBD_SendEndpoint(ep, data, len); }
#else
#include "fsusb.h"
#endif

#define EP_CDC_IRQ   1
#define EP_CDC_OUT   2
#define EP_CDC_IN    3
#define EP_BULK_OUT  6
#define EP_BULK_IN   5

#ifdef CH570_CH572
#define LED PA9
#else
#define LED PA8
#endif

#ifndef ROM_CFG_MAC_ADDR // should be updated in ch5xxhw.h
#define ROM_CFG_MAC_ADDR        ((const u32*)0x0007f018)
#endif

#define ACCESS_ADDRESS 0x63683332
#define ISLER_CHANNEL  35
#define ISLER_PHY_MODE PHY_1M
__attribute__((aligned(4))) static uint8_t payload[] = "UUUUS[ch32fun iSLER ping pong]";

#if !defined(FUNCONF_USE_DEBUGPRINTF) || !FUNCONF_USE_DEBUGPRINTF
int _write(int fd, const char *buf, int size) {
	if(USBFS_SendEndpointNEW(EP_CDC_IN, (uint8_t*)buf, size, /*copy*/1) == -1) { // -1 == busy
		// wait for 1ms to try again once more
		Delay_Ms(1);
		USBFS_SendEndpointNEW(EP_CDC_IN, (uint8_t*)buf, size, /*copy*/1);
	}
	return size;
}

int putchar(int c) {
	uint8_t single = c;
	if(USBFS_SendEndpointNEW(EP_CDC_IN, &single, 1, /*copy*/1) == -1) { // -1 == busy
		// wait for 1ms to try again once more
		Delay_Ms(1);
		USBFS_SendEndpointNEW(EP_CDC_IN, &single, 1, /*copy*/1);
	}
	return 1;
}
#endif

void blink(int n) {
	for(int i = n-1; i >= 0; i--) {
		funDigitalWrite( LED, FUN_LOW ); // Turn on LED
		Delay_Ms(33);
		funDigitalWrite( LED, FUN_HIGH ); // Turn off LED
		if(i) Delay_Ms(33);
	}
}

void tx() {
	iSLERTX(ACCESS_ADDRESS, (uint8_t*)payload, sizeof(payload), ISLER_CHANNEL, ISLER_PHY_MODE);	
}

// -----------------------------------------------------------------------------
// IN Handler (Called by IRQ when Packet Sent to PC)
// -----------------------------------------------------------------------------
int HandleInRequest(struct _USBState *ctx, int endp, uint8_t *data, int len) {
	return 0;
}

// -----------------------------------------------------------------------------
// OUT Handler (Called by IRQ when Packet Received from PC)
// -----------------------------------------------------------------------------
void HandleDataOut(struct _USBState *ctx, int endp, uint8_t *data, int len) {
	if (endp == 0) {
		ctx->USBFS_SetupReqLen = 0;
	}
	else if( endp == EP_CDC_OUT ) {
		// cdc tty input
		if(len == 1 && data[0] == 'p') {
			tx();
		}
	}
	else if (endp == EP_BULK_OUT) {
		if(len == 4 && ((uint32_t*)data)[0] == 0x010001a2) {
			USBFSReset();
			blink(2);
			jump_isprom();
		}

		ctx->USBFS_Endp_Busy[EP_BULK_OUT] = 0;
	}
}

int HandleSetupCustom( struct _USBState *ctx, int setup_code ) {
	int ret = -1;
	if ( ctx->USBFS_SetupReqType & USB_REQ_TYP_CLASS ) {
		switch ( setup_code ) {
		case CDC_SET_LINE_CODING:
		case CDC_SET_LINE_CTLSTE:
		case CDC_SEND_BREAK:
			ret = ( ctx->USBFS_SetupReqLen ) ? ctx->USBFS_SetupReqLen : -1;
			break;
		case CDC_GET_LINE_CODING:
			ret = ctx->USBFS_SetupReqLen;
			break;
		default:
			ret = 0;
			break;
		}
	}
	else {
		ret = 0; // Go to STALL
	}
	return ret;
}

int main() {
	SystemInit();
	printf( "iSLER blaster test\n" );

	funGpioInitAll(); // no-op on ch5xx
	funPinMode( LED, GPIO_CFGLR_OUT_2Mhz_PP );

	USBFSSetup();

	*(uint32_t*)payload = *ROM_CFG_MAC_ADDR;
	payload[4] = sizeof(payload) -5;
	iSLERInit(LL_TX_POWER_0_DBM);

	blink(5);
	while(1) {
		tx();
	}
}
