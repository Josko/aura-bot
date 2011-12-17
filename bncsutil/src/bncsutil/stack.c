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

#include <bncsutil/stack.h>
#include <stdlib.h>
 
cm_stack_t cm_stack_create()
{
	cm_stack_t stack = (cm_stack_t) calloc(1, sizeof(struct cm_stack));
	if (!stack)
		return (cm_stack_t) 0;
	return stack;
}

void cm_stack_destroy(cm_stack_t stack)
{
	cm_stack_node_t* node;
	cm_stack_node_t* next;
	
	if (!stack)
		return;
	
	node = stack->top;
	
	while (node) {
		next = node->next;
		free(node);
		node = next;
	}
	
	free(stack);
}

void cm_stack_push(cm_stack_t stack, void* item)
{
	cm_stack_node_t* new_node;
	
	if (!stack || !item)
		return;
	
	new_node = (cm_stack_node_t*) malloc(sizeof(cm_stack_node_t));
	if (!new_node)
		return;
	new_node->next = stack->top;
	new_node->value = item;
	
	stack->size++;
	
	stack->top = new_node;
}

void* cm_stack_pop(cm_stack_t stack)
{
	cm_stack_node_t* next;
	void* value;
	
	if (!stack || !stack->top)
		return (void*) 0;
	
	next = stack->top->next;
	value = stack->top->value;
	free(stack->top);
	
	stack->top = next;
	stack->size--;
	return value;
}

void* cm_stack_peek(cm_stack_t stack)
{
	if (!stack || !stack->top)
		return (void*) 0;
	
	return stack->top->value;
}

unsigned int cm_stack_size(cm_stack_t stack)
{
	if (!stack)
		return 0;
	
	return stack->size;
}
