//
//  UptimeChanger.c
//  UptimeChanger
//
//  Created by Zhuowei Zhang on 7/25/18.
//  Copyright © 2018 Zhuowei Zhang. All rights reserved.
//

#include <mach/mach_types.h>

kern_return_t UptimeChanger_start(kmod_info_t * ki, void *d);
kern_return_t UptimeChanger_stop(kmod_info_t *ki, void *d);

kern_return_t UptimeChanger_start(kmod_info_t * ki, void *d)
{
    return KERN_SUCCESS;
}

kern_return_t UptimeChanger_stop(kmod_info_t *ki, void *d)
{
    return KERN_SUCCESS;
}
