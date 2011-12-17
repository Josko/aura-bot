/**
 * BNCSutil
 * Battle.Net Utility Library
 *
 * Copyright (C) 2004-2006 Eric Naeseth
 *
 * Message Buffers
 * January 11, 2006
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License is included in the BNCSutil
 * distribution in the file COPYING.  If you did not receive this copy,
 * write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA  02111-1307  USA
 */

#include <bncsutil/mutil.h>

typedef struct msg_buffer *msg_buffer_t;
typedef struct msg_buffer_impl *msg_buffer_impl_t;
typedef struct msg_reader *msg_reader_t;
typedef struct msg_reader_impl *msg_reader_impl_t;

// Callbacks

/**
 * Called to determine the number of bytes at the beginning of an output
 * buffer to reserve for a message/packet header.
 */
typedef size_t __stdcall (*buffer_bytes_to_reserve)();

/**
 * Called to generate the header on an outgoing packet.
 */
typedef int __stdcall (*buffer_create_header)(msg_buffer_t);


/**
 * Creates a new generic buffer for an outgoing packet.
 */
msg_buffer_t create_buffer(size_t initial_size);

/**
 * Creates a new BNCS buffer.
 */
msg_buffer_t create_bncs_buffer(size_t initial_size);

/**
 * Destroys a message buffer.
 */
void destroy_buffer(msg_buffer_t);

// Adders

void buffer_add_8(msg_buffer_t, int8_t);
void buffer_add_u8(msg_buffer_t, uint8_t);
void buffer_add_16(msg_buffer_t, int16_t);
void buffer_add_u16(msg_buffer_t, uint16_t);
void buffer_add_32(msg_buffer_t, int32_t);
void buffer_add_u32(msg_buffer_t, uint32_t);

msg_reader_t create_reader(size_t initial_size);
