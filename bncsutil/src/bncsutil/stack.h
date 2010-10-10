/**
 * BNCSutil
 * Battle.Net Utility Library
 *
 * Copyright (C) 2004-2006 Eric Naeseth
 *
 * Stack
 * August 16, 2005
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

/*
 * Mule Server
 * Copyright (c) 2004-2006 Eric Naeseth.
 *
 * Stack
 * May 13, 2005
 */

#ifndef CM_STACK_H_INCLUDED
#define CM_STACK_H_INCLUDED 1

typedef struct cm_stack_node {
	void* value;
	struct cm_stack_node* next;
} cm_stack_node_t;

typedef struct cm_stack {
	unsigned int size;
	cm_stack_node_t* top;
} *cm_stack_t;

cm_stack_t cm_stack_create();
void cm_stack_destroy(cm_stack_t stack);
void cm_stack_push(cm_stack_t stack, void* item);
void* cm_stack_pop(cm_stack_t stack);
void* cm_stack_peek(cm_stack_t stack);
unsigned int cm_stack_size(cm_stack_t stack);

#endif
