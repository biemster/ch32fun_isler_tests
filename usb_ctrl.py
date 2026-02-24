#!/usr/bin/env python
"""
requires pyusb, which should be pippable
SUBSYSTEM=="usb", ATTR{idVendor}=="1209", ATTR{idProduct}=="d035", MODE="666"
sudo udevadm control --reload-rules && sudo udevadm trigger
"""
import os
import argparse
import usb.core
import usb.util
import socket
import struct
from binascii import unhexlify
import threading
import subprocess
from time import sleep

CH_USB_VENDOR_ID    = 0x1209    # VID
CH_USB_PRODUCT_ID   = 0xd035    # PID
CH_USB_INTERFACE    = 0         # interface number
CH_USB_EP_IN        = 0x85      # BULK_IN endpoint
CH_USB_EP_OUT       = 0x06      # BULK_OUT endpoint
CH_USB_PACKET_SIZE  = 2048      # packet size, pyUSB does reassembly of 64 byte frames
CH_USB_TIMEOUT_MS   = 100       # timeout for USB operations

CH_CMD_REBOOT       = 0xa2
CH_STR_REBOOT       = (CH_CMD_REBOOT, 0x01, 0x00, 0x01)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-b', '--bootloader', help='Reboot to bootloader', action='store_true')
    parser.add_argument('-c', '--chip', help='Chip type to compile for')
    parser.add_argument('-l', '--linkE-build-and-flash', help='Build and flash over linkE', action='store_true')
    parser.add_argument('-t', '--target', help='Target to compile')
    parser.add_argument('-u', '--USB-build-and-flash', help='Build and flash over USB ISP', action='store_true')
    args = parser.parse_args()

    usb_dev = usb.core.find(idVendor=CH_USB_VENDOR_ID, idProduct=CH_USB_PRODUCT_ID)

    if usb_dev is None:
        print("MCU not found")
        exit(0)

    if args.bootloader:
        print('rebooting to bootloader')
        bootloader(usb_dev)
        sleep(.3)
    elif args.linkE_build_and_flash:
        print('Starting build and flash over linkE')
    elif args.USB_build_and_flash:
        print('Starting build and flash over USB ISP')
    else:
        parser.print_help()
    
    print('done')

def bootloader(usb_dev):
    usb_dev.write(CH_USB_EP_OUT, CH_STR_REBOOT)


if __name__ == '__main__':
    main()
