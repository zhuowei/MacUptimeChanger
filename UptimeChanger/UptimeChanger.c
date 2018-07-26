//
//  UptimeChanger.c
//  UptimeChanger
//
//  Created by Zhuowei Zhang on 7/25/18.
//  Copyright © 2018 Zhuowei Zhang. All rights reserved.
//

#include <mach/mach_types.h>
#include <sys/sysctl.h>
#include <kern/clock.h>
#include <IOKit/IOLib.h>
#include "kernel_resolver.h"

/*
 $ lldb /System/Library/Kernels/kernel
 (lldb) target create "/System/Library/Kernels/kernel"
 Current executable set to '/System/Library/Kernels/kernel' (x86_64).
 (lldb) b clock_get_boottime_microtime
 Breakpoint 1: where = kernel`clock_get_boottime_microtime, address = 0xffffff80003ddf60
 (lldb) x/32i 0xffffff80003ddf60
 0xffffff80003ddf60: 55                    pushq  %rbp
 0xffffff80003ddf61: 48 89 e5              movq   %rsp, %rbp
 0xffffff80003ddf64: 41 57                 pushq  %r15
 0xffffff80003ddf66: 41 56                 pushq  %r14
 0xffffff80003ddf68: 41 54                 pushq  %r12
 0xffffff80003ddf6a: 53                    pushq  %rbx
 0xffffff80003ddf6b: 49 89 f6              movq   %rsi, %r14
 0xffffff80003ddf6e: 49 89 fc              movq   %rdi, %r12
 0xffffff80003ddf71: 9c                    pushfq
 0xffffff80003ddf72: 5b                    popq   %rbx
 0xffffff80003ddf73: f6 c7 02              testb  $0x2, %bh
 0xffffff80003ddf76: 74 01                 je     0xffffff80003ddf79
 0xffffff80003ddf78: fa                    cli
 0xffffff80003ddf79: 4c 8d 3d f8 86 8b 00  leaq   0x8b86f8(%rip), %r15
 0xffffff80003ddf80: 4c 89 ff              movq   %r15, %rdi
 0xffffff80003ddf83: e8 08 42 12 00        callq  0xffffff8000502190
 0xffffff80003ddf88: 48 8b 05 09 73 a4 00  movq   0xa47309(%rip), %rax
 0xffffff80003ddf8f: 49 89 04 24           movq   %rax, (%r12)
 0xffffff80003ddf93: 8b 05 f7 72 a4 00     movl   0xa472f7(%rip), %eax
 0xffffff80003ddf99: 41 89 06              movl   %eax, (%r14)
 0xffffff80003ddf9c: 4c 89 ff              movq   %r15, %rdi
 0xffffff80003ddf9f: e8 ac 43 01 00        callq  0xffffff80003f2350
 0xffffff80003ddfa4: 9c                    pushfq
 0xffffff80003ddfa5: 58                    popq   %rax
 0xffffff80003ddfa6: f6 c7 02              testb  $0x2, %bh
 0xffffff80003ddfa9: 75 08                 jne    0xffffff80003ddfb3
 0xffffff80003ddfab: f6 c4 02              testb  $0x2, %ah
 0xffffff80003ddfae: 74 21                 je     0xffffff80003ddfd1
 0xffffff80003ddfb0: fa                    cli
 0xffffff80003ddfb1: eb 1e                 jmp    0xffffff80003ddfd1
 0xffffff80003ddfb3: fb                    sti
 0xffffff80003ddfb4: 90                    nop
*/

static void (*ptr_clock_get_boottime_microtime)(clock_sec_t *secs, clock_usec_t *microsecs);
static void (*ptr_commpage_update_boottime)(uint64_t value);
static clock_sec_t *ptr_clock_boottime;
static void findUptimeVar() {
    // disassemble
    ptr_clock_get_boottime_microtime = lookup_symbol("_clock_get_boottime_microtime");
    if (!ptr_clock_get_boottime_microtime) {
        IOLog("Cannot find clock_get_boottime_microtime");
        return;
    }
    ptr_commpage_update_boottime = lookup_symbol("_commpage_update_boottime");
    if (ptr_commpage_update_boottime == NULL) {
        IOLog("Cannot find commpage_update_boottime\n");
        return;
    }
    uint8_t* code = (uint8_t*)ptr_clock_get_boottime_microtime;
    for (int i = 0; i < 0x100; i++) {
        //  0xffffff80003ddf88: 48 8b 05 09 73 a4 00  movq   0xa47309(%rip), %rax
        if (code[i] == 0x48 && code[i+1] == 0x8b && code[i+2] == 0x05) {
            int32_t offset = *((int32_t*)(code + i + 3));
            // rip is relative to the next instruction, so add length of cur instr
            ptr_clock_boottime = (clock_sec_t*)(code + i + 7 + offset);
        }
    }
    clock_sec_t boottime_secs;
    clock_usec_t boottime_usecs;
    ptr_clock_get_boottime_microtime(&boottime_secs, &boottime_usecs);
    if (*ptr_clock_boottime != boottime_secs) {
        IOLog("Invalid boottime\n");
        ptr_clock_boottime = NULL;
    }
}

static int UptimeChanger_sysctl_kern_changeboottime(struct sysctl_oid * oidp, void* arg1, int arg2, struct sysctl_req * req) {
    if (!ptr_clock_boottime) {
        IOLog("Cannot find clock_boottime\n");
        return ENOTSUP;
    }
    int retval = sysctl_handle_long(oidp, ptr_clock_boottime, arg2, req);
    clock_sec_t boottime_secs;
    clock_usec_t boottime_usecs;
    ptr_clock_get_boottime_microtime(&boottime_secs, &boottime_usecs);
    if (*ptr_clock_boottime != boottime_secs) {
        IOLog("Can't set boottime\n");
        return ENOTSUP;
    }
    // update the commpage
    ptr_commpage_update_boottime(boottime_secs * USEC_PER_SEC + boottime_usecs);
    return retval;
}

SYSCTL_PROC(_kern, OID_AUTO, changeboottime, CTLTYPE_INT | CTLFLAG_WR, NULL, 0, UptimeChanger_sysctl_kern_changeboottime, "L", "change uptime by setting boot time, in seconds since Unix epoch");


kern_return_t UptimeChanger_start(kmod_info_t * ki, void *d);
kern_return_t UptimeChanger_stop(kmod_info_t *ki, void *d);

kern_return_t UptimeChanger_start(kmod_info_t * ki, void *d)
{
    findUptimeVar();
    sysctl_register_oid(&sysctl__kern_changeboottime);
    return KERN_SUCCESS;
}

kern_return_t UptimeChanger_stop(kmod_info_t *ki, void *d)
{
    sysctl_unregister_oid(&sysctl__kern_changeboottime);
    return KERN_SUCCESS;
}
