#pragma once

#include "utility/Deque.h"
#include "utility/endian.h"

#include "AppleMIDI_Defs.h"

#include "AppleMIDI_Settings.h"
#include "AppleMIDI_Platform.h"

#include "AppleMIDI_Namespace.h"

BEGIN_APPLEMIDI_NAMESPACE

template <class UdpClass, class Settings, class Platform>
class AppleMIDISession;

template <class UdpClass, class Settings, class Platform>
class AppleMIDIParser
{
public:
	AppleMIDISession<UdpClass, Settings, Platform> *session;

	parserReturn parse(RtpBuffer_t &buffer, const amPortType &portType)
	{
		conversionBuffer cb;

		byte signature[2]; // Signature "Magic Value" for Apple network MIDI session establishment
		byte command[2];   // 16-bit command identifier (two ASCII characters, first in high 8 bits, second in low 8 bits)

		size_t minimumLen = (sizeof(signature) + sizeof(command)); // Signature + Command
		if (buffer.size() < minimumLen)
            return parserReturn::NotSureGiveMeMoreData;

		size_t i = 0;

		signature[0] = buffer[i++];
		signature[1] = buffer[i++];
		if (0 != memcmp(signature, amSignature, sizeof(amSignature)))
		{
            return parserReturn::UnexpectedData;
		}

		command[0] = buffer[i++];
		command[1] = buffer[i++];

		if (false)
		{
		}
#ifdef APPLEMIDI_LISTENER
		else if (0 == memcmp(command, amInvitation, sizeof(amInvitation)))
		{
			byte protocolVersion[4];

			minimumLen += (sizeof(protocolVersion) + sizeof(initiatorToken_t) + sizeof(ssrc_t));
			if (buffer.size() < minimumLen)
			{
                return parserReturn::NotEnoughData;
			}

			// 2 (stored in network byte order (big-endian))
			protocolVersion[0] = buffer[i++];
			protocolVersion[1] = buffer[i++];
			protocolVersion[2] = buffer[i++];
			protocolVersion[3] = buffer[i++];
			if (0 != memcmp(protocolVersion, amProtocolVersion, sizeof(amProtocolVersion)))
			{
                return parserReturn::UnexpectedData;
			}

			AppleMIDI_Invitation invitation;

			// A random number generated by the session's APPLEMIDI_INITIATOR.
			cb.buffer[0] = buffer[i++];
			cb.buffer[1] = buffer[i++];
			cb.buffer[2] = buffer[i++];
			cb.buffer[3] = buffer[i++];
			invitation.initiatorToken = ntohl(cb.value32);
			
			// The sender's synchronization source identifier.
			cb.buffer[0] = buffer[i++];
			cb.buffer[1] = buffer[i++];
			cb.buffer[2] = buffer[i++];
			cb.buffer[3] = buffer[i++];
			invitation.ssrc = ntohl(cb.value32);

#ifdef KEEP_SESSION_NAME
            uint16_t bi = 0;
            while ((i < buffer.size()) && (buffer[i] != 0x00))
            {
                if (bi <= DefaultSettings::MaxSessionNameLen)
                    invitation.sessionName[bi++] = buffer[i];
                i++;
            }
            invitation.sessionName[bi++] = '\0';
#else
            while ((i < buffer.size()) && (buffer[i] != 0x00))
                i++;
#endif
			if (i == buffer.size() || buffer[i++] != 0x00)
                return parserReturn::NotEnoughData;

            while (i--)
                buffer.pop_front(); // consume all the bytes that made up this message

			session->ReceivedInvitation(invitation, portType);

            return parserReturn::Processed;
		}
		else if (0 == memcmp(command, amEndSession, sizeof(amEndSession)))
		{
			byte protocolVersion[4];

			minimumLen += (sizeof(protocolVersion) + sizeof(initiatorToken_t) + sizeof(ssrc_t));
			if (buffer.size() < minimumLen)
                return parserReturn::NotEnoughData;

			// 2 (stored in network byte order (big-endian))
			protocolVersion[0] = buffer[i++];
			protocolVersion[1] = buffer[i++];
			protocolVersion[2] = buffer[i++];
			protocolVersion[3] = buffer[i++];
			if (0 != memcmp(protocolVersion, amProtocolVersion, sizeof(amProtocolVersion)))
			{
                return parserReturn::UnexpectedData;
			}

			AppleMIDI_EndSession_t endSession;

			// A random number generated by the session's APPLEMIDI_INITIATOR.
			cb.buffer[0] = buffer[i++];
			cb.buffer[1] = buffer[i++];
			cb.buffer[2] = buffer[i++];
			cb.buffer[3] = buffer[i++];
			endSession.initiatorToken = ntohl(cb.value32);
            
			// The sender's synchronization source identifier.
			cb.buffer[0] = buffer[i++];
			cb.buffer[1] = buffer[i++];
			cb.buffer[2] = buffer[i++];
			cb.buffer[3] = buffer[i++];
			endSession.ssrc = ntohl(cb.value32);

            while (i--)
                buffer.pop_front(); // consume all the bytes that made up this message

			session->ReceivedEndSession(endSession);

            return parserReturn::Processed;
		}
		else if (0 == memcmp(command, amSynchronization, sizeof(amSynchronization)))
		{
			AppleMIDI_Synchronization_t synchronization;

			// minimum amount : 4 bytes for sender SSRC, 1 byte for count, 3 bytes padding and 3 times 8 bytes
			minimumLen += (4 + 1 + 3 + (3 * 8));
			if (buffer.size() < minimumLen)
                return parserReturn::NotEnoughData;

			// The sender's synchronization source identifier.
			cb.buffer[0] = buffer[i++];
			cb.buffer[1] = buffer[i++];
			cb.buffer[2] = buffer[i++];
			cb.buffer[3] = buffer[i++];
			synchronization.ssrc = ntohl(cb.value32);

			synchronization.count = buffer[i++];
			buffer[i++];
			buffer[i++];
			buffer[i++]; // padding, unused
			cb.buffer[0] = buffer[i++];
			cb.buffer[1] = buffer[i++];
			cb.buffer[2] = buffer[i++];
			cb.buffer[3] = buffer[i++];
			cb.buffer[4] = buffer[i++];
			cb.buffer[5] = buffer[i++];
			cb.buffer[6] = buffer[i++];
			cb.buffer[7] = buffer[i++];
			synchronization.timestamps[0] = ntohll(cb.value64);

			cb.buffer[0] = buffer[i++];
			cb.buffer[1] = buffer[i++];
			cb.buffer[2] = buffer[i++];
			cb.buffer[3] = buffer[i++];
			cb.buffer[4] = buffer[i++];
			cb.buffer[5] = buffer[i++];
			cb.buffer[6] = buffer[i++];
			cb.buffer[7] = buffer[i++];
			synchronization.timestamps[1] = ntohll(cb.value64);

			cb.buffer[0] = buffer[i++];
			cb.buffer[1] = buffer[i++];
			cb.buffer[2] = buffer[i++];
			cb.buffer[3] = buffer[i++];
			cb.buffer[4] = buffer[i++];
			cb.buffer[5] = buffer[i++];
			cb.buffer[6] = buffer[i++];
			cb.buffer[7] = buffer[i++];
			synchronization.timestamps[2] = ntohll(cb.value64);

            while (i--)
                buffer.pop_front(); // consume all the bytes that made up this message

			session->ReceivedSynchronization(synchronization);

            return parserReturn::Processed;
		}
		else if (0 == memcmp(command, amReceiverFeedback, sizeof(amReceiverFeedback)))
		{
			AppleMIDI_ReceiverFeedback_t receiverFeedback;

			minimumLen += (sizeof(receiverFeedback.ssrc) + sizeof(receiverFeedback.sequenceNr) + sizeof(receiverFeedback.dummy));
			if (buffer.size() < minimumLen)
                return parserReturn::NotEnoughData;

			cb.buffer[0] = buffer[i++];
			cb.buffer[1] = buffer[i++];
			cb.buffer[2] = buffer[i++];
			cb.buffer[3] = buffer[i++];
			receiverFeedback.ssrc = ntohl(cb.value32);

			cb.buffer[0] = buffer[i++];
			cb.buffer[1] = buffer[i++];
			receiverFeedback.sequenceNr = ntohs(cb.value16);
			cb.buffer[0] = buffer[i++];
			cb.buffer[1] = buffer[i++];
			receiverFeedback.dummy = ntohs(cb.value16);

            while (i--)
                buffer.pop_front(); // consume all the bytes that made up this message

			session->ReceivedReceiverFeedback(receiverFeedback);

            return parserReturn::Processed;
		}
#endif
#ifdef APPLEMIDI_INITIATOR
		else if (0 == memcmp(command, amInvitationAccepted, sizeof(amInvitationAccepted)))
		{
            byte protocolVersion[4];

            minimumLen += (sizeof(protocolVersion) + sizeof(initiatorToken_t) + sizeof(ssrc_t));
            if (buffer.size() < minimumLen)
            {
                return parserReturn::NotEnoughData;
            }

            // 2 (stored in network byte order (big-endian))
            protocolVersion[0] = buffer[i++];
            protocolVersion[1] = buffer[i++];
            protocolVersion[2] = buffer[i++];
            protocolVersion[3] = buffer[i++];
            if (0 != memcmp(protocolVersion, amProtocolVersion, sizeof(amProtocolVersion)))
            {
                return parserReturn::UnexpectedData;
            }

            AppleMIDI_InvitationAccepted_t invitationAccepted;

            // A random number generated by the session's APPLEMIDI_INITIATOR.
            cb.buffer[0] = buffer[i++];
            cb.buffer[1] = buffer[i++];
            cb.buffer[2] = buffer[i++];
            cb.buffer[3] = buffer[i++];
            invitationAccepted.initiatorToken = ntohl(cb.value32);
            
            // The sender's synchronization source identifier.
            cb.buffer[0] = buffer[i++];
            cb.buffer[1] = buffer[i++];
            cb.buffer[2] = buffer[i++];
            cb.buffer[3] = buffer[i++];
            invitationAccepted.ssrc = ntohl(cb.value32);

 #ifdef KEEP_SESSION_NAME
            uint16_t bi = 0;
            while ((i < buffer.size()) && (buffer[i] != 0x00))
            {
                if (bi <= DefaultSettings::MaxSessionNameLen)
                    invitationAccepted.sessionName[bi++] = buffer[i];
                i++;
            }
            invitationAccepted.sessionName[bi++] = '\0';
#else
            while ((i < buffer.size()) && (buffer[i] != 0x00))
                i++;
#endif
            if (i == buffer.size() || buffer[i++] != 0x00)
                return parserReturn::NotEnoughData;

            while (i--)
                buffer.pop_front(); // consume all the bytes that made up this message

            session->ReceivedInvitationAccepted(invitationAccepted, portType);

            return parserReturn::Processed;
		}
		else if (0 == memcmp(command, amInvitationRejected, sizeof(amInvitationRejected)))
		{
            byte protocolVersion[4];

            minimumLen += (sizeof(protocolVersion) + sizeof(initiatorToken_t) + sizeof(ssrc_t));
            if (buffer.size() < minimumLen)
            {
                return parserReturn::NotEnoughData;
            }

            // 2 (stored in network byte order (big-endian))
            protocolVersion[0] = buffer[i++];
            protocolVersion[1] = buffer[i++];
            protocolVersion[2] = buffer[i++];
            protocolVersion[3] = buffer[i++];
            if (0 != memcmp(protocolVersion, amProtocolVersion, sizeof(amProtocolVersion)))
            {
                return parserReturn::UnexpectedData;
            }

            AppleMIDI_InvitationRejected_t invitationRejected;

            // A random number generated by the session's APPLEMIDI_INITIATOR.
            cb.buffer[0] = buffer[i++];
            cb.buffer[1] = buffer[i++];
            cb.buffer[2] = buffer[i++];
            cb.buffer[3] = buffer[i++];
            invitationRejected.initiatorToken = ntohl(cb.value32);
            
            // The sender's synchronization source identifier.
            cb.buffer[0] = buffer[i++];
            cb.buffer[1] = buffer[i++];
            cb.buffer[2] = buffer[i++];
            cb.buffer[3] = buffer[i++];
            invitationRejected.ssrc = ntohl(cb.value32);

#ifdef KEEP_SESSION_NAME
            uint16_t bi = 0;
            while ((i < buffer.size()) && (buffer[i] != 0x00))
            {
                if (bi <= DefaultSettings::MaxSessionNameLen)
                    invitationRejected.sessionName[bi++] = buffer[i];
                i++;
            }
            invitationRejected.sessionName[bi++] = '\0';
#else
            while ((i < buffer.size()) && (buffer[i] != 0x00))
                i++;
#endif
            if (i == buffer.size() || buffer[i++] != 0x00)
                return parserReturn::NotEnoughData;

            while (i--)
                buffer.pop_front(); // consume all the bytes that made up this message

            session->ReceivedInvitationRejected(invitationRejected);

            return parserReturn::Processed;
		}
        else if (0 == memcmp(command, amBitrateReceiveLimit, sizeof(amBitrateReceiveLimit)))
        {
            AppleMIDI_BitrateReceiveLimit bitrateReceiveLimit;

            minimumLen += (sizeof(bitrateReceiveLimit.ssrc) + sizeof(bitrateReceiveLimit.bitratelimit));
            if (buffer.size() < minimumLen)
                return parserReturn::NotEnoughData;

            cb.buffer[0] = buffer[i++];
            cb.buffer[1] = buffer[i++];
            cb.buffer[2] = buffer[i++];
            cb.buffer[3] = buffer[i++];
            bitrateReceiveLimit.ssrc = ntohl(cb.value32);

            cb.buffer[0] = buffer[i++];
            cb.buffer[1] = buffer[i++];
            cb.buffer[2] = buffer[i++];
            cb.buffer[3] = buffer[i++];
            bitrateReceiveLimit.bitratelimit = ntohl(cb.value32);

            while (i--)
                buffer.pop_front(); // consume all the bytes that made up this message

            session->ReceivedBitrateReceiveLimit(bitrateReceiveLimit);

            return parserReturn::Processed;
        }
#endif
        return parserReturn::UnexpectedData;
	}
};

END_APPLEMIDI_NAMESPACE