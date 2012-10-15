bin: ia32_hello
retval: 0
srcs:
ia32_hello.s
cc: gcc
exec: True
stdout:
Hello, world!
relocs:
file: ia32_hello.o
# Offset Type     Symdx    Value
0000000a 00000001 00000002 00000000
00000010 00000001 00000002 0000000e
file: ia32_hello
# Addr   Value
0800000a 08000020
08000010 0800002e
