all : flash

TARGET := pingpong
TARGET_MCU := CH570
TARGET_MCU_PACKAGE := CH570D
DEBUGPRINTF := 1

ifneq ($(filter listener,$(MAKECMDGOALS)),)
    TARGET := listener
endif

ifneq ($(filter blaster,$(MAKECMDGOALS)),)
    TARGET := blaster
endif

ifneq ($(filter pingpong,$(MAKECMDGOALS)),)
    TARGET := pingpong
endif

ifneq ($(filter v208 ch32v208,$(MAKECMDGOALS)),)
    TARGET_MCU := CH32V208
    TARGET_MCU_PACKAGE := CH32V208WBU6
    EXTRA_CFLAGS += -DUSB_USE_USBD # for some specifically cheap v208 boards
endif
ifneq ($(filter ch570,$(MAKECMDGOALS)),)
    TARGET_MCU := CH570
    TARGET_MCU_PACKAGE := CH570D
endif
ifneq ($(filter ch573,$(MAKECMDGOALS)),)
    TARGET_MCU := CH573
    TARGET_MCU_PACKAGE := CH573F
endif
ifneq ($(filter ch582,$(MAKECMDGOALS)),)
    TARGET_MCU := CH582
    TARGET_MCU_PACKAGE := CH582F
endif
ifneq ($(filter ch585,$(MAKECMDGOALS)),)
    TARGET_MCU := CH585
    TARGET_MCU_PACKAGE := CH585M
endif
ifneq ($(filter ch591,$(MAKECMDGOALS)),)
    TARGET_MCU := CH591
    TARGET_MCU_PACKAGE := CH591D
endif
ifneq ($(filter ch592,$(MAKECMDGOALS)),)
    TARGET_MCU := CH592
    TARGET_MCU_PACKAGE := CH592F
endif

ifneq ($(filter usb,$(MAKECMDGOALS)),)
    DEBUGPRINTF := 0
endif

CONFIG_TARGETS := usb v208 ch32v208 ch570 ch573 ch582 ch585 ch591 ch592 listener blaster pingpong
.PHONY: $(CONFIG_TARGETS)
$(CONFIG_TARGETS):
	@:

include ~/temp/ch32fun/ch32fun/ch32fun.mk
CFLAGS+=-DDEBUGPRINTF=$(DEBUGPRINTF)

flash : cv_flash
clean : cv_clean
