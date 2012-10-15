##############################################################################
# Copyright (c) 2006, National ICT Australia
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

from simulators import qemu_kzm_sim, kzm_boot

class kzm_arm11(arm1136js):
    virtual = False
    platform = "imx31"
    # Workaround for issue 3020
    base_vaddr = 0x90000000L
    memory = arm1136js.memory.copy()
    memory['physical']         = [Region(0x80000000L, 0x87800000L)]
    memory['rom']              = [Region(0x87800000L, 0x88000000L)]
#     memory['secure_rom']       = [Region(0x00000000L, 0x00004000L, "dedicated")]
#     memory['internal_rom']     = [Region(0x00400000L, 0x00404000L, "dedicated")]
#     memory['internal_ram']     = [Region(0x1FFFC000L, 0x20000000L, "dedicated")]
#     memory['l2cc_config_regs'] = [Region(0x30000000L, 0x40000000L, "dedicated")]
#     memory['onchip_dev1']      = [Region(0x40000000L, 0x80000000L, "dedicated")]
#     memory['chipsel0']         = [Region(0xA0000000L, 0xA8000000L, "dedicated")]
#     memory['chipsel1']         = [Region(0xA8000000L, 0xB0000000L, "dedicated")]
#     memory['chipsel2']         = [Region(0xB0000000L, 0xB2000000L, "dedicated")]
#     memory['chipsel3']         = [Region(0xB2000000L, 0xB4000000L, "dedicated")]
#     memory['chipsel4']         = [Region(0xB4000000L, 0xB6000000L, "dedicated")]
#     memory['chipsel5']         = [Region(0xB6000000L, 0xB8000000L, "dedicated")]
#     memory['onchip_dev2']      = [Region(0xB8000000L, 0xB8005000L, "dedicated")]
#     memory['pcmcia_cf']        = [Region(0xC0000000L, 0xC4000000L, "dedicated")]
    run_methods = {
        'hardware': kzm_boot,
        'qemu':qemu_kzm_sim }
    default_method = 'qemu'
    zero_bss = True
    boot_binary = True

    # driverv2
    timer_driver = "imx31_timer"
    rtc_driver = "imx31_rtc"
    serial_driver = "imx31_serial"
    dma_driver = "ixm31_sdma"
    device_core = "imx31"
    memory_timer = [Region (0x53f90000, 0x53f94000, "all", "uncached")]
    interrupt_timer = [29]
    memory_serial = [Region(0x43f90000, 0x43f91000, "all", "uncached")]
    interrupt_serial = [45]
    memory_dma = [Region(0x53fd4000, 0x53fd8000, "all", "uncached")]
    interrupt_dma = [34]
    memory_rtc = [Region(0x53fd8000, 0x53fd9000, "all", "uncached")]
    memory_irq_controller = [Region(0x68000000, 0x68001000, "all", "uncached")]
    interrupt_rtc = [25]
    pmu_irq = [23]
    v2_drivers = [
                    ("test_device", "vtest", [], [1,3]),
                    (timer_driver, "vtimer", memory_timer, interrupt_timer),
                    (serial_driver, "vserial", memory_serial, interrupt_serial),
                    (dma_driver, "vdma", memory_dma, interrupt_dma),
                    (rtc_driver, "vrtc", memory_rtc, interrupt_rtc),
                    ("kirq_device", "kinterrupt", memory_irq_controller, [])
                 ]
    platform_dir = "imx31"
