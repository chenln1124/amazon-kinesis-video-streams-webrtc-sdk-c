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
#ifndef __KINESIS_VIDEO_WEBRTC_WSS_CLIENT_H__
#define __KINESIS_VIDEO_WEBRTC_WSS_CLIENT_H__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <wslay/wslay.h>
#include "network_api.h"

// wslay related lib.
// SIGNALING_SERVICE_WSS_PING_PONG_INTERVAL_IN_SECONDS
#define WSS_CLIENT_RFC6455_UUID                "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WSS_CLIENT_RFC6455_UUID_LEN            STRLEN(WSS_CLIENT_RFC6455_UUID)
#define WSS_CLIENT_RANDOM_SEED_LEN             16
#define WSS_CLIENT_BASED64_RANDOM_SEED_LEN     24
#define WSS_CLIENT_SHA1_RANDOM_SEED_W_UUID_LEN 20
#define WSS_CLIENT_ACCEPT_KEY_LEN              28
#define WSS_CLIENT_POLLING_INTERVAL            100 // unit:ms.
#define WSS_CLIENT_PING_PONG_INTERVAL          10  // unit:sec.
#define WSS_CLIENT_PING_PONG_COUNTER           (WSS_CLIENT_PING_PONG_INTERVAL * 1000) / WSS_CLIENT_POLLING_INTERVAL
#define WSS_CLIENT_PING_MAX_ACC_NUM            1

typedef STATUS (*MessageHandlerFunc)(PVOID pUserData, PCHAR pMessage, UINT32 messageLen);
typedef STATUS (*CtrlMessageHandlerFunc)(PVOID pUserData, UINT8 opcode, PCHAR pMessage, UINT32 messageLen);
typedef STATUS (*TerminationHandlerFunc)(PVOID pUserData, STATUS errCode);

typedef struct {
    wslay_event_context_ptr event_ctx;            //!< the event context of wslay.
    struct wslay_event_callbacks event_callbacks; //!< the callback of event context.
    NetworkContext_t* pNetworkContext;
    UINT64 pingCounter;
    MUTEX clientLock;                          //!< the lock for the control of the whole wss client api.
    MUTEX listenerLock;                        //!< the lock for the listener thread.
    PVOID pUserData;                           //!< the arguments of the message handler. ref: PSignalingClient
    MessageHandlerFunc messageHandler;         //!< the handler of receive the non-ctrl messages.
    CtrlMessageHandlerFunc ctrlMessageHandler; //!< the handler of receive the ctrl messages.
    TerminationHandlerFunc terminationHandler;
} WssClientContext, *PWssClientContext;

STATUS wss_client_generateClientKey(PCHAR buf, UINT32 bufLen);
STATUS wss_client_validateAcceptKey(PCHAR clientKey, UINT32 clientKeyLen, PCHAR acceptKey, UINT32 acceptKeyLen);
/**
 * @brief create the context of wss client.
 *
 * @param[in] ppWssClientCtx the context of wss client.
 * @param[in] pNetworkContext pass the handler of network io to wss client, and wss client is in charge of shutdown the network io.
 * @param[in] pUserData the user data passed to these callback functions.
 * @param[in] pFunc the callback to handle received data messages.
 * @param[in] pCtrlFunc the callback to handle received ctrl messages.
 * @param[in] pTerminationHandlerFunc the callback to handle terminated connection.
 *
 * @return STATUS status of execution.
 */
VOID wss_client_create(PWssClientContext* ppWssClientCtx, NetworkContext_t* pNetworkContext, PVOID pUserData, MessageHandlerFunc pFunc,
                       CtrlMessageHandlerFunc pCtrlFunc, TerminationHandlerFunc pTerminationHandlerFunc);
/**
 * @brief start the wss thread to handle the wss connection.
 *
 * @param[in] pWssClientCtx the context of wss client.
 *
 * @return STATUS status of execution.
 */
PVOID wss_client_start(PWssClientContext pWssClientCtx);
/**
 * @brief send the text message through the wss connection.
 *
 * @param[in] pWssClientCtx the context of wss client.
 * @param[in] buf the buffer of text.
 * @param[in] len the length of the text.
 *
 * @return STATUS status of execution.
 */
STATUS wss_client_sendText(PWssClientContext pCtx, PUINT8 buf, UINT32 len);
/**
 * @brief send the binary message through the wss connection.
 *
 * @param[in] pWssClientCtx the context of wss client.
 * @param[in] buf the buffer.
 * @param[in] len the length of the buffer.
 *
 * @return STATUS status of execution.
 */
STATUS wss_client_sendBinary(PWssClientContext pCtx, UINT8* buf, UINT32 len);
/**
 * @brief send the ping through the wss connection.
 *
 * @param[in] pWssClientCtx the context of wss client.
 *
 * @return STATUS status of execution.
 */
STATUS wss_client_sendPing(PWssClientContext pCtx);
/**
 * @brief start the wss thread to handle the wss connection.
 *
 * @param[in] pWssClientCtx the context of wss client.
 *
 * @return STATUS status of execution.
 */
VOID wss_client_close(PWssClientContext pWssClientCtx);

#ifdef __cplusplus
}
#endif
#endif /* __KINESIS_VIDEO_WEBRTC_WSS_CLIENT_H__ */