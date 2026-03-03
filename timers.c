#include "ch32fun.h"
#include <stdio.h>

void tmr0_cb();
void tmr1_cb();
void tmr2_cb();
#define ISLER_TMR0_CALLBACK tmr0_cb
#define ISLER_TMR1_CALLBACK tmr1_cb
#define ISLER_TMR2_CALLBACK tmr2_cb
#if defined(CH570_CH572) || defined(CH584_CH585) || defined(CH591_CH592)
#define ISLER_TMR3_CALLBACK tmr3_cb
#define ISLER_TMR4_CALLBACK tmr4_cb
void tmr3_cb();
void tmr4_cb();
#endif
#include "iSLER.h"

#ifdef USB_USE_USBD
// the USBD peripheral is not recommended, but there are really nice
// v208 boards with only those pins brought out, so support is hacked in
#include "usbd.h"
#define USBFSSetup         USBDSetup
#define USBFS_SetupReqLen  USBD_SetupReqLen
#define USBFS_SetupReqType USBD_SetupReqType
#define USBFS_Endp_Busy    USBD_Endp_Busy
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
#else
static int is_pressed;
void handle_debug_input( int numbytes, uint8_t * data ) {
	if(numbytes == 1 && data[0] == 'p') {
		is_pressed = 1;
	}
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
			putchar('.');
		}
	}
	else if (endp == EP_BULK_OUT) {
#ifdef CH5xx
		if(len == 4 && ((uint32_t*)data)[0] == 0x010001a2) {
			USBFSReset();
			blink(2);
			jump_isprom();
		}
#endif

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

void tmr0_cb() { _write(0, "tmr0\r\n", 6); }
void tmr1_cb() { _write(0, "tmr1\r\n", 6); }
void tmr2_cb() { _write(0, "tmr2\r\n", 6); }
void tmr3_cb() { _write(0, "tmr3\r\n", 6); }
void tmr4_cb() { _write(0, "tmr4\r\n", 6); }

int main() {
	SystemInit();
	printf( "iSLER timer test\n" );

	funGpioInitAll(); // no-op on ch5xx
	funPinMode( LED, GPIO_CFGLR_OUT_2Mhz_PP );

#ifndef CH32V20x
	// BUG: currently USBFSSetup interferes with the RF clock on the v208
	USBFSSetup();
#endif
	iSLERInit(LL_TX_POWER_0_DBM); // we just init the whole thing, we don't know yet which exact bit enable the timers

#if defined(INT_ISLER_TMR0) && defined(ISLER_TMR0_CALLBACK)
	uint32_t int_en = INT_ISLER_TMR0;
	LL->TMR0 = (1 << 21); // 2M, about one second
#endif
#if defined(INT_ISLER_TMR1) && defined(ISLER_TMR1_CALLBACK)
	int_en |= INT_ISLER_TMR1;
	LL->TMR1 = (1 << 21) + (1 << 19); // 1s + ~250ms
#endif
#if defined(INT_ISLER_TMR2) && defined(ISLER_TMR2_CALLBACK)
	int_en |= INT_ISLER_TMR2;
	LL->TMR2 = (1 << 21) + (1 << 20); // 1s + ~500ms
#endif
#if defined(INT_ISLER_TMR3) && defined(ISLER_TMR3_CALLBACK)
	int_en |= INT_ISLER_TMR3;
	LL->TMR3 = (1 << 21) + (1 << 20) + (1 << 19); // 1s + ~750ms
#endif
#if defined(INT_ISLER_TMR4) && defined(ISLER_TMR4_CALLBACK)
	int_en |= INT_ISLER_TMR4;
	LL->TMR4 = (1 << 21) + (1 << 21); // 2s
#endif

	LL->INT_EN = int_en;

	blink(5);
	while(1) {
#if defined(FUNCONF_USE_DEBUGPRINTF) && FUNCONF_USE_DEBUGPRINTF
		poll_input();
		if(is_pressed) {
			putchar('.');
			is_pressed = 0;
		}
#endif
	}
}
