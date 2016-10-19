/*
 * Copyright (c) 2016, OpenAV Productions,
 * Harry van Haaren <harryhaaren@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef OPENAV_CTLR_DEVICE_IMPL
#define OPENAV_CTLR_DEVICE_IMPL

#include "../event.h"

struct ctlr_dev_t;

/* Functions each device must implement */
typedef uint32_t (*ctlr_dev_impl_poll)(struct ctlr_dev_t *dev);
typedef int32_t (*ctlr_dev_impl_disconnect)(struct ctlr_dev_t *dev);
typedef int32_t (*ctlr_dev_impl_light_set)(struct ctlr_dev_t *dev,
					   uint32_t light_id,
					   uint32_t light_status);

struct ctlr_dev_t {
	ctlr_event_func event_func;
	void* event_func_userdata;

	ctlr_dev_impl_poll poll;
	ctlr_dev_impl_disconnect disconnect;

	/* Feedback to device */
	ctlr_dev_impl_light_set light_set;
};

/** Connect function to instantiate a dev from the driver */
typedef struct ctlr_dev_t *(*ctlr_dev_connect_func)(ctlr_event_func event_func,
						    void *userdata,
						    void *future);

/* Macro extern declaration for the connect function */
#define DECLARE_DEV_CONNECT_FUNC(name)					\
extern struct ctlr_dev_t *name(ctlr_event_func event_func,		\
			    void *userdata, void *future)

#endif /* OPENAV_CTLR_DEVICE_IMPL */
