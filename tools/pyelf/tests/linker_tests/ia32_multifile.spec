bin: ia32_multifile
retval: 42
srcs:
ia32_multifile_1.s
ia32_multifile_2.s
ia32_multifile_3.s
cc: gcc
exec: True
stdout:
relocs:
file: ia32_multifile_1.o
# Offset Type     Symdx    Value
00000012 00000002 00000005 fffffffc
file: ia32_multifile_2.o
# Offset Type     Symdx    Value
00000007 00000002 00000005 fffffffc
file: ia32_multifile_3.o
# Offset Type     Symdx    Value
00000007 00000002 00000005 fffffffc
file: ia32_multifile
# Addr   Value
08000012 0000001a
08000037 00000005
08000047 ffffffda
