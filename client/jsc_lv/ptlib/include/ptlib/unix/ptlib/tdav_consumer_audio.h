/*
* Copyright (C) 2009-2010 Mamadou Diop.
*
* Contact: Mamadou Diop <diopmamadou(at)doubango.org>
*	
* This file is part of Open Source Doubango Framework.
*
* DOUBANGO is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*	
* DOUBANGO is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*	
* You should have received a copy of the GNU General Public License
* along with DOUBANGO.
*
*/

/**@file tdav_consumer_audio.h
 * @brief Base class for all Audio consumers.
 *
 * @author Mamadou Diop <diopmamadou(at)doubango.org>
 *
 * @date Created: Sat Nov 8 16:54:58 2009 mdiop
 */
#ifndef TINYDAV_CONSUMER_AUDIO_H
#define TINYDAV_CONSUMER_AUDIO_H

#include "stdint.h"
typedef void* tsk_object_t ;
typedef int tsk_size_t;
#define TDAV_CONSUMER_AUDIO(self)		((tdav_consumer_audio_t*)(self))

typedef struct tdav_consumer_audio_s
{
//	TMEDIA_DECLARE_CONSUMER;

	uint8_t channels;
	uint32_t rate;
	uint8_t bits_per_sample;
	uint8_t ptime;

}
tdav_consumer_audio_t;

 int tdav_consumer_audio_init(tdav_consumer_audio_t* self);
 int tdav_consumer_audio_cmp(const tsk_object_t* consumer1, const tsk_object_t* consumer2);
#define tdav_consumer_audio_prepare(self, codec) tmedia_consumer_prepare(TDAV_CONSUMER_AUDIO(self), codec)
#define tdav_consumer_audio_start(self) tmedia_consumer_start(TDAV_CONSUMER_AUDIO(self))
#define tdav_consumer_audio_consume(self, buffer, size) tmedia_consumer_consume(TDAV_CONSUMER_AUDIO(self), buffer, size)
#define tdav_consumer_audio_pause(self) tmedia_consumer_pause(TDAV_CONSUMER_AUDIO(self))
#define tdav_consumer_audio_stop(self) tmedia_consumer_stop(TDAV_CONSUMER_AUDIO(self))
 int tdav_consumer_audio_put(tdav_consumer_audio_t* self, void** data, tsk_size_t size, const tsk_object_t* proto_hdr);
 void* tdav_consumer_audio_get(tdav_consumer_audio_t* self);
 int tdav_consumer_audio_reset(tdav_consumer_audio_t* self);
 int tdav_consumer_audio_deinit(tdav_consumer_audio_t* self);

#define TDAV_DECLARE_CONSUMER_AUDIO tdav_consumer_audio_t __consumer_audio__

TDAV_END_DECLS

#endif /* TINYDAV_CONSUMER_AUDIO_H */
