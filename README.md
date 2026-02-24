# ch32fun_isler_tests
Experiments with ch32fun iSLER

## playground to test and improve iSLER RF in ch32fun
Current available applications:
1. listener
2. blaster
3. pingpong

The names say it all currently. Build for ch570 (default) and flash using linkE (default) for example:
```bash
make pingpong clean all monitor
```
or for ch582 using USB ISP:
```bash
./usb_ctrl.py -b && make pingpong usb ch582 clean all
```
