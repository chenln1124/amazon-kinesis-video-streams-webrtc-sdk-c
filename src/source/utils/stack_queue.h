/*
 * Copyright 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
#ifndef __AWS_KVS_WEBRTC_STACK_QUEUE_INCLUDE__
#define __AWS_KVS_WEBRTC_STACK_QUEUE_INCLUDE__

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************
 * HEADERS
 ******************************************************************************/
#include "single_linked_list.h"

/******************************************************************************
 * DEFINITIONS
 ******************************************************************************/
typedef SingleList StackQueue;
typedef PSingleList PStackQueue;
typedef PSingleListNode StackQueueIterator;
typedef StackQueueIterator* PStackQueueIterator;

#define IS_VALID_ITERATOR(x) ((x) != NULL)

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
/**
 * Create a new stack queue
 */
STATUS stack_queue_create(PStackQueue*);

/**
 * Frees and de-allocates the stack queue
 */
STATUS stack_queue_free(PStackQueue);

/**
 * Clears and de-allocates all the items
 */
STATUS stack_queue_clear(PStackQueue, BOOL);

/**
 * Gets the number of items in the stack/queue
 */
STATUS stack_queue_getCount(PStackQueue, PUINT32);

/**
 * Gets the item at the given index
 */
STATUS stackQueueGetAt(PStackQueue, UINT32, PUINT64);

/**
 * Sets the item value at the given index
 */
STATUS stackQueueSetAt(PStackQueue, UINT32, UINT64);

/**
 * Gets the index of an item
 */
STATUS stackQueueGetIndexOf(PStackQueue, UINT64, PUINT32);

/**
 * Removes the item at the given index
 */
STATUS stackQueueRemoveAt(PStackQueue, UINT32);

/**
 * Removes the item at the given item
 */
STATUS stackQueueRemoveItem(PStackQueue, UINT64);

/**
 * Whether the stack queue is empty
 */
STATUS stack_queue_isEmpty(PStackQueue, PBOOL);

/**
 * Pushes an item onto the stack
 */
STATUS stackQueuePush(PStackQueue, UINT64);

/**
 * Pops an item from the stack
 */
STATUS stackQueuePop(PStackQueue, PUINT64);

/**
 * Peeks an item from the stack without popping
 */
STATUS stackQueuePeek(PStackQueue, PUINT64);

/**
 * Enqueues an item in the queue
 */
STATUS stack_queue_enqueue(PStackQueue, UINT64);
/**
 * @brief Dequeues an item from the queue.
 *
 * @param[in] pStackQueue the context of the stack queue.
 * @param[in] pItem the user data inside the stack queue.
 *
 * @return STATUS code of the execution. STATUS_SUCCESS on success.
 */
STATUS stack_queue_dequeue(PStackQueue pStackQueue, PUINT64 pItem);

/**
 * Gets the iterator
 */
STATUS stack_queue_iterator_get(PStackQueue, PStackQueueIterator);

/**
 * Iterates to next
 */
STATUS stack_queue_iterator_getNext(PStackQueueIterator);

/**
 * Gets the data
 */
STATUS stack_queue_iterator_getItem(StackQueueIterator, PUINT64);

#ifdef __cplusplus
}
#endif
#endif /* __AWS_KVS_WEBRTC_STACK_QUEUE_INCLUDE__ */
