##############################################################################
# Copyright (c) 2007 Open Kernel Labs, Inc. (Copyright Holder).
# All rights reserved.
# 
# 1. Redistribution and use of OKL4 (Software) in source and binary
# forms, with or without modification, are permitted provided that the
# following conditions are met:
# 
#     (a) Redistributions of source code must retain this clause 1
#         (including paragraphs (a), (b) and (c)), clause 2 and clause 3
#         (Licence Terms) and the above copyright notice.
# 
#     (b) Redistributions in binary form must reproduce the above
#         copyright notice and the Licence Terms in the documentation and/or
#         other materials provided with the distribution.
# 
#     (c) Redistributions in any form must be accompanied by information on
#         how to obtain complete source code for:
#        (i) the Software; and
#        (ii) all accompanying software that uses (or is intended to
#        use) the Software whether directly or indirectly.  Such source
#        code must:
#        (iii) either be included in the distribution or be available
#        for no more than the cost of distribution plus a nominal fee;
#        and
#        (iv) be licensed by each relevant holder of copyright under
#        either the Licence Terms (with an appropriate copyright notice)
#        or the terms of a licence which is approved by the Open Source
#        Initative.  For an executable file, "complete source code"
#        means the source code for all modules it contains and includes
#        associated build and other files reasonably required to produce
#        the executable.
# 
# 2. THIS SOFTWARE IS PROVIDED ``AS IS'' AND, TO THE EXTENT PERMITTED BY
# LAW, ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE, OR NON-INFRINGEMENT, ARE DISCLAIMED.  WHERE ANY WARRANTY IS
# IMPLIED AND IS PREVENTED BY LAW FROM BEING DISCLAIMED THEN TO THE
# EXTENT PERMISSIBLE BY LAW: (A) THE WARRANTY IS READ DOWN IN FAVOUR OF
# THE COPYRIGHT HOLDER (AND, IN THE CASE OF A PARTICIPANT, THAT
# PARTICIPANT) AND (B) ANY LIMITATIONS PERMITTED BY LAW (INCLUDING AS TO
# THE EXTENT OF THE WARRANTY AND THE REMEDIES AVAILABLE IN THE EVENT OF
# BREACH) ARE DEEMED PART OF THIS LICENCE IN A FORM MOST FAVOURABLE TO
# THE COPYRIGHT HOLDER (AND, IN THE CASE OF A PARTICIPANT, THAT
# PARTICIPANT). IN THE LICENCE TERMS, "PARTICIPANT" INCLUDES EVERY
# PERSON WHO HAS CONTRIBUTED TO THE SOFTWARE OR WHO HAS BEEN INVOLVED IN
# THE DISTRIBUTION OR DISSEMINATION OF THE SOFTWARE.
# 
# 3. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ANY OTHER PARTICIPANT BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
# BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import copy
from simulators import versatile_boot, qemu_versatile_sim

############################################################################
# Versatile machines
############################################################################

class versatile(arm926ejs):
    device_core = "versatile"
    virtual = False
    platform = "versatile"
    memory = arm926ejs.memory.copy()
    memory['physical'] = [Region(0x04100000L, 0x07900000L)]
    memory['rom'] = [Region(0x07900000L, 0x08000000L)]
    timer_driver_v2 = "sp804_timer"
    memory_timer = [Region(0x101e3000, 0x101e4000, "all", "uncached")]
    interrupt_timer = [5]
    serial_driver_v2 = "pl011_uart_v2"
    memory_serial = [Region(0x101f1000, 0x101f2000, "all", "uncached")]
    interrupt_serial = [12]
    memory_eth = [Region(0x10010000, 0x10020000, "all", "uncached")]
    interrupt_eth = [25]
    memory_sys = [Region(0x10000000, 0x10001000, "all", "uncached")]
    memory_clcd = [Region(0x10120000, 0x10121000, "all", "uncached")]
    interrupt_clcd = [16]
    memory_kmi0 = [Region(0x10006000, 0x10007000, "all", "uncached")]
    interrupt_kmi0 = [35]
    memory_kmi1 = [Region(0x10007000, 0x10008000, "all", "uncached")]
    interrupt_kmi1 = [36]
    v2_drivers = [
                  (timer_driver_v2, "vtimer", memory_timer, interrupt_timer),
                  (serial_driver_v2, "vserial", memory_serial, interrupt_serial),
                  ("eth_device", "veth", memory_eth, interrupt_eth),
                  ("versatilesys_device", "vversatilesys", memory_sys, []),
                  ("kmi0_device", "vkmi0", memory_kmi0, interrupt_kmi0),
                  ("kmi1_device", "vkmi1", memory_kmi1, interrupt_kmi1),
                  ("clcd_device", "vclcd", memory_clcd, interrupt_clcd),
                  ("test_device", "vtest", [], [6,7])
                 ]
    cpp_defines = arm926ejs.cpp_defines + ["VERSATILE_BOARD"]
    zero_bss = True
    copy_elf = True


class versatile_uboot(versatile):
    device_core = "versatile"
    timer_driver_v2 = "sp804_timer"
    memory_timer = [Region(0x101e3000, 0x101e4000, "all", "uncached")]
    interrupt_timer = [5]
    serial_driver_v2 = "pl011_uart_v2"
    memory_serial = [Region(0x101f1000, 0x101f2000, "all", "uncached")]
    interrupt_serial = [12]
    memory_irq_controller = [Region(0x10140000, 0x10141000, "all", "uncached")]
    memory_eth = [Region(0x10010000, 0x10020000, "all", "uncached")]
    interrupt_eth = [25]
    memory_sys = [Region(0x10000000, 0x10001000, "all", "uncached")]
    memory_clcd = [Region(0x10120000, 0x10121000, "all", "uncached")]
    interrupt_clcd = [16]
    memory_kmi0 = [Region(0x10006000, 0x10007000, "all", "uncached")]
    interrupt_kmi0 = [35]
    memory_kmi1 = [Region(0x10007000, 0x10008000, "all", "uncached")]
    interrupt_kmi1 = [36]
    v2_drivers = [
                  (timer_driver_v2, "vtimer", memory_timer, interrupt_timer),
                  (serial_driver_v2, "vserial", memory_serial, interrupt_serial),
                  ("eth_device", "veth", memory_eth, interrupt_eth),
                  ("versatilesys_device", "vversatilesys", memory_sys, []),
                  ("kmi0_device", "vkmi0", memory_kmi0, interrupt_kmi0),
                  ("kmi1_device", "vkmi1", memory_kmi1, interrupt_kmi1),
                  ("clcd_device", "vclcd", memory_clcd, interrupt_clcd),
                  ("kirq_device", "kinterrupt", memory_irq_controller, []),
                  ("test_device", "vtest", [], [6,7])
                 ]
    run_methods = { 'hardware': versatile_boot, 
        'qemu':qemu_versatile_sim }
    default_method = 'qemu'
    uart = "serial"
