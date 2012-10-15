bin: arm_abs32
retval: 0
srcs:
arm_abs32.s
cc: arm-linux-gcc
stdout:
relocs:
file: arm_abs32.o
# Offset Type     Symdx    Value
00000020 00000001 00000002 ca000014
00000054 00000001 00000002 ea000004
00000060 00000002 00000008 00000000
file: arm_abs32
# Addr   Value
08000020 ca00000c
08000054 eaffffef
08000060 08000064
