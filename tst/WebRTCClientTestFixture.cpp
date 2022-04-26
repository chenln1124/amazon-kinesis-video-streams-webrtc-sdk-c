#include "WebRTCClientTestFixture.h"

namespace com {
namespace amazonaws {
namespace kinesis {
namespace video {
namespace webrtcclient {

//
// Global memory allocation counter
//
UINT64 gTotalWebRtcClientMemoryUsage = 0;

//
// Global memory counter lock
//
MUTEX gTotalWebRtcClientMemoryMutex;

STATUS createRtpPacketWithSeqNum(UINT16 seqNum, PRtpPacket *ppRtpPacket) {
    STATUS retStatus = STATUS_SUCCESS;
    BYTE payload[10];
    PRtpPacket pRtpPacket = NULL;

    CHK_STATUS(rtp_packet_create(2, FALSE, FALSE, 0, FALSE,
                               96, seqNum, 100, 0x1234ABCD, NULL, 0, 0, NULL, payload, 10, &pRtpPacket));
    *ppRtpPacket = pRtpPacket;

    CHK_STATUS(rtp_packet_createBytesFromPacket(pRtpPacket, NULL, &pRtpPacket->rawPacketLength));
    CHK(NULL != (pRtpPacket->pRawPacket = (PBYTE) MEMALLOC(pRtpPacket->rawPacketLength)), STATUS_NOT_ENOUGH_MEMORY);
    CHK_STATUS(rtp_packet_createBytesFromPacket(pRtpPacket, pRtpPacket->pRawPacket, &pRtpPacket->rawPacketLength));

    CleanUp:
    return retStatus;
}

WebRtcClientTestBase::WebRtcClientTestBase() :
        mSignalingClientHandle(INVALID_SIGNALING_CLIENT_HANDLE_VALUE),
        mAccessKey(NULL),
        mSecretKey(NULL),
        mSessionToken(NULL),
        mRegion(NULL),
        mCaCertPath(NULL),
        mAccessKeyIdSet(FALSE)
{
    // Initialize the endianness of the library
    endianness_initialize();

    SRAND(12345);

    mStreamingRotationPeriod = TEST_STREAMING_TOKEN_DURATION;
}

void WebRtcClientTestBase::SetUp()
{
    DLOGI("\nSetting up test: %s\n", GetTestName());
    mReadyFrameIndex = 0;
    mDroppedFrameIndex = 0;
    mExpectedFrameCount = 0;
    mExpectedDroppedFrameCount = 0;

    SET_INSTRUMENTED_ALLOCATORS();

    mLogLevel = LOG_LEVEL_DEBUG;

    PCHAR logLevelStr = GETENV(DEBUG_LOG_LEVEL_ENV_VAR);
    if (logLevelStr != NULL) {
        ASSERT_EQ(STATUS_SUCCESS, STRTOUI32(logLevelStr, NULL, 10, &mLogLevel));
    }

    SET_LOGGER_LOG_LEVEL(mLogLevel);

    pc_initWebRtc();

    if (NULL != (mAccessKey = getenv(ACCESS_KEY_ENV_VAR))) {
        mAccessKeyIdSet = TRUE;
    }

    mSecretKey = getenv(SECRET_KEY_ENV_VAR);
    mSessionToken = getenv(SESSION_TOKEN_ENV_VAR);

    if (NULL == (mRegion = getenv(DEFAULT_REGION_ENV_VAR))) {
        mRegion = TEST_DEFAULT_REGION;
    }

    if (NULL == (mCaCertPath = getenv(CACERT_PATH_ENV_VAR))) {
        mCaCertPath = (PCHAR) DEFAULT_KVS_CACERT_PATH;
    }

    if (mAccessKey) {
        ASSERT_EQ(STATUS_SUCCESS,
                  static_credential_provider_create(mAccessKey, 0, mSecretKey, 0, mSessionToken, 0, MAX_UINT64, &mTestCredentialProvider));
    } else {
        mTestCredentialProvider = nullptr;
    }

    // Prepare the test channel name by prefixing with test channel name
    // and generating random chars replacing a potentially bad characters with '.'
    STRCPY(mChannelName, TEST_SIGNALING_CHANNEL_NAME);
    UINT32 testNameLen = STRLEN(TEST_SIGNALING_CHANNEL_NAME);
    const UINT32 randSize = 16;

    PCHAR pCur = &mChannelName[testNameLen];

    for (UINT32 i = 0; i < randSize; i++) {
        *pCur++ = SIGNALING_VALID_NAME_CHARS[RAND() % (ARRAY_SIZE(SIGNALING_VALID_NAME_CHARS) - 1)];
    }

    *pCur = '\0';
}

void WebRtcClientTestBase::TearDown()
{
    DLOGI("\nTearing down test: %s\n", GetTestName());

    pc_deinitWebRtc();

    static_credential_provider_free(&mTestCredentialProvider);

    EXPECT_EQ(STATUS_SUCCESS, RESET_INSTRUMENTED_ALLOCATORS());
}

VOID WebRtcClientTestBase::initializeJitterBuffer(UINT32 expectedFrameCount, UINT32 expectedDroppedFrameCount, UINT32 rtpPacketCount)
{
    UINT32 i, timestamp;
    EXPECT_EQ(STATUS_SUCCESS,
              jitter_buffer_create(testFrameReadyFunc, testFrameDroppedFunc, testDepayRtpFunc, DEFAULT_JITTER_BUFFER_MAX_LATENCY,
                                 TEST_JITTER_BUFFER_CLOCK_RATE, (UINT64) this, &mJitterBuffer));
    mExpectedFrameCount = expectedFrameCount;
    mFrame = NULL;
    if (expectedFrameCount > 0) {
        mPExpectedFrameArr = (PBYTE*) MEMALLOC(SIZEOF(PBYTE) * expectedFrameCount);
        mExpectedFrameSizeArr = (PUINT32) MEMALLOC(SIZEOF(UINT32) * expectedFrameCount);
    }
    mExpectedDroppedFrameCount = expectedDroppedFrameCount;
    if (expectedDroppedFrameCount > 0) {
        mExpectedDroppedFrameTimestampArr = (PUINT32) MEMALLOC(SIZEOF(UINT32) * expectedDroppedFrameCount);
    }

    mPRtpPackets = (PRtpPacket*) MEMALLOC(SIZEOF(PRtpPacket) * rtpPacketCount);
    mRtpPacketCount = rtpPacketCount;

    // Assume timestamp is on time unit ms for test
    for (i = 0, timestamp = 0; i < rtpPacketCount; i++, timestamp += 200) {
        EXPECT_EQ(STATUS_SUCCESS,
                  rtp_packet_create(2, FALSE, FALSE, 0, FALSE, 96, i, timestamp, 0x1234ABCD, NULL, 0, 0, NULL, NULL, 0, mPRtpPackets + i));
    }
}

VOID WebRtcClientTestBase::setPayloadToFree()
{
    UINT32 i;
    for (i = 0; i < mRtpPacketCount; i++) {
        mPRtpPackets[i]->pRawPacket = mPRtpPackets[i]->payload;
    }
}

VOID WebRtcClientTestBase::clearJitterBufferForTest()
{
    UINT32 i;
    EXPECT_EQ(STATUS_SUCCESS, jitter_buffer_free(&mJitterBuffer));
    if (mExpectedFrameCount > 0) {
        for (i = 0; i < mExpectedFrameCount; i++) {
            MEMFREE(mPExpectedFrameArr[i]);
        }
        MEMFREE(mPExpectedFrameArr);
        MEMFREE(mExpectedFrameSizeArr);
    }
    if (mExpectedDroppedFrameCount > 0) {
        MEMFREE(mExpectedDroppedFrameTimestampArr);
    }
    MEMFREE(mPRtpPackets);
    EXPECT_EQ(mExpectedFrameCount, mReadyFrameIndex);
    EXPECT_EQ(mExpectedDroppedFrameCount, mDroppedFrameIndex);
    if (mFrame != NULL) {
        MEMFREE(mFrame);
    }
}

// Connect two RtcPeerConnections, and wait for them to be connected
// in the given amount of time. Return false if they don't go to connected in
// the expected amounted of time
bool WebRtcClientTestBase::connectTwoPeers(PRtcPeerConnection offerPc, PRtcPeerConnection answerPc, PCHAR pOfferCertFingerprint,
                                           PCHAR pAnswerCertFingerprint)
{
    RtcSessionDescriptionInit sdp;

    auto onICECandidateHdlr = [](UINT64 customData, PCHAR candidateStr) -> void {
        if (candidateStr != NULL) {
            std::thread(
                [customData](std::string candidate) {
                    RtcIceCandidateInit iceCandidate;
                    EXPECT_EQ(STATUS_SUCCESS, sdp_deserializeRtcIceCandidateInit((PCHAR) candidate.c_str(), STRLEN(candidate.c_str()), &iceCandidate));
                    EXPECT_EQ(STATUS_SUCCESS, pc_addIceCandidate((PRtcPeerConnection) customData, iceCandidate.candidate));
                },
                std::string(candidateStr))
                .detach();
        }
    };

    EXPECT_EQ(STATUS_SUCCESS, pc_onIceCandidate(offerPc, (UINT64) answerPc, onICECandidateHdlr));
    EXPECT_EQ(STATUS_SUCCESS, pc_onIceCandidate(answerPc, (UINT64) offerPc, onICECandidateHdlr));

    auto onICEConnectionStateChangeHdlr = [](UINT64 customData, RTC_PEER_CONNECTION_STATE newState) -> void {
        ATOMIC_INCREMENT((PSIZE_T) customData + newState);
    };

    EXPECT_EQ(STATUS_SUCCESS, pc_onConnectionStateChange(offerPc, (UINT64) this->stateChangeCount, onICEConnectionStateChangeHdlr));
    EXPECT_EQ(STATUS_SUCCESS, pc_onConnectionStateChange(answerPc, (UINT64) this->stateChangeCount, onICEConnectionStateChangeHdlr));

    EXPECT_EQ(STATUS_SUCCESS, pc_createOffer(offerPc, &sdp));
    EXPECT_EQ(STATUS_SUCCESS, pc_setLocalDescription(offerPc, &sdp));
    EXPECT_EQ(STATUS_SUCCESS, pc_setRemoteDescription(answerPc, &sdp));

    // Validate the cert fingerprint if we are asked to do so
    if (pOfferCertFingerprint != NULL) {
        EXPECT_NE((PCHAR) NULL, STRSTR(sdp.sdp, pOfferCertFingerprint));
    }

    EXPECT_EQ(STATUS_SUCCESS, pc_createAnswer(answerPc, &sdp));
    EXPECT_EQ(STATUS_SUCCESS, pc_setLocalDescription(answerPc, &sdp));
    EXPECT_EQ(STATUS_SUCCESS, pc_setRemoteDescription(offerPc, &sdp));

    if (pAnswerCertFingerprint != NULL) {
        EXPECT_NE((PCHAR) NULL, STRSTR(sdp.sdp, pAnswerCertFingerprint));
    }

    for (auto i = 0; i <= 100 && ATOMIC_LOAD(&this->stateChangeCount[RTC_PEER_CONNECTION_STATE_CONNECTED]) != 2; i++) {
        THREAD_SLEEP(HUNDREDS_OF_NANOS_IN_A_SECOND);
    }

    return ATOMIC_LOAD(&this->stateChangeCount[RTC_PEER_CONNECTION_STATE_CONNECTED]) == 2;
}

// Create track and transceiver and adds to PeerConnection
void WebRtcClientTestBase::addTrackToPeerConnection(PRtcPeerConnection pRtcPeerConnection, PRtcMediaStreamTrack track,
                                                    PRtcRtpTransceiver* transceiver, RTC_CODEC codec, MEDIA_STREAM_TRACK_KIND kind)
{
    MEMSET(track, 0x00, SIZEOF(RtcMediaStreamTrack));

    EXPECT_EQ(STATUS_SUCCESS, pc_addSupportedCodec(pRtcPeerConnection, codec));

    track->kind = kind;
    track->codec = codec;
    EXPECT_EQ(STATUS_SUCCESS, json_generateSafeString(track->streamId, MAX_MEDIA_STREAM_ID_LEN));
    EXPECT_EQ(STATUS_SUCCESS, json_generateSafeString(track->trackId, MAX_MEDIA_STREAM_ID_LEN));

    EXPECT_EQ(STATUS_SUCCESS, pc_addTransceiver(pRtcPeerConnection, track, NULL, transceiver));
}

STATUS awaitGetIceConfigInfoCount(SIGNALING_CLIENT_HANDLE signalingClientHandle, PUINT32 pIceConfigInfoCount)
{
    STATUS retStatus = STATUS_SUCCESS;
    UINT64 elapsed = 0;

    CHK(IS_VALID_SIGNALING_CLIENT_HANDLE(signalingClientHandle) && pIceConfigInfoCount != NULL, STATUS_NULL_ARG);

    while (TRUE) {
        // Get the configuration count
        CHK_STATUS(signaling_client_getIceConfigInfoCount(signalingClientHandle, pIceConfigInfoCount));

        // Return OK if we have some ice configs
        CHK(*pIceConfigInfoCount == 0, retStatus);

        // Check for timeout
        CHK_ERR(elapsed <= TEST_ASYNC_ICE_CONFIG_INFO_WAIT_TIMEOUT, STATUS_OPERATION_TIMED_OUT,
                "Couldn't retrieve ICE configurations in alotted time.");

        THREAD_SLEEP(TEST_ICE_CONFIG_INFO_POLL_PERIOD);
        elapsed += TEST_ICE_CONFIG_INFO_POLL_PERIOD;
    }

CleanUp:

    return retStatus;
}

void WebRtcClientTestBase::getIceServers(PRtcConfiguration pRtcConfiguration)
{
    UINT32 i, j, iceConfigCount, uriCount;
    PIceConfigInfo pIceConfigInfo;

    // Assume signaling client is already created
    EXPECT_EQ(STATUS_SUCCESS, awaitGetIceConfigInfoCount(mSignalingClientHandle, &iceConfigCount));

    // Set the  STUN server
    SNPRINTF(pRtcConfiguration->iceServers[0].urls, MAX_ICE_CONFIG_URI_LEN, KINESIS_VIDEO_STUN_URL, TEST_DEFAULT_REGION);

    for (uriCount = 0, i = 0; i < iceConfigCount; i++) {
        EXPECT_EQ(STATUS_SUCCESS, signaling_client_getIceConfigInfo(mSignalingClientHandle, i, &pIceConfigInfo));
        for (j = 0; j < pIceConfigInfo->uriCount; j++) {
            STRNCPY(pRtcConfiguration->iceServers[uriCount + 1].urls, pIceConfigInfo->uris[j], MAX_ICE_CONFIG_URI_LEN);
            STRNCPY(pRtcConfiguration->iceServers[uriCount + 1].credential, pIceConfigInfo->password, MAX_ICE_CONFIG_CREDENTIAL_LEN);
            STRNCPY(pRtcConfiguration->iceServers[uriCount + 1].username, pIceConfigInfo->userName, MAX_ICE_CONFIG_USER_NAME_LEN);

            uriCount++;
        }
    }
}

PCHAR WebRtcClientTestBase::GetTestName()
{
    return (PCHAR)::testing::UnitTest::GetInstance()->current_test_info()->test_case_name();
}

} // namespace webrtcclient
} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
