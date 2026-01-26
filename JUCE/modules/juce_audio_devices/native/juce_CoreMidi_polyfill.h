#pragma once
#include <CoreFoundation/CoreFoundation.h>
#include <CoreMIDI/CoreMIDI.h>
#include <cstdint>

// =============================================================================
// CoreMIDI Polyfill for macOS 10.13 compatibility
// Provides MIDI 2.0 API structures and functions that gracefully degrade to MIDI 1.0
// =============================================================================

#if ! __has_include(<Availability.h>) || ! defined(__MAC_11_0) || __MAC_OS_X_VERSION_MAX_ALLOWED < __MAC_11_0

// Protocol IDs - MIDI 2.0 will be treated as MIDI 1.0 on older systems
typedef CF_ENUM(SInt32, MIDIProtocolID) {
    kMIDIProtocol_1_0 = 1,
    kMIDIProtocol_2_0 = 2,
};

// MIDIEventPacket structure (matches macOS 11+ layout)
// Use uint32_t for words[] to match JUCE's ump::Iterator expectations
struct MIDIEventPacket {
    MIDITimeStamp timeStamp;
    uint32_t wordCount;
    uint32_t words[64];
};
typedef struct MIDIEventPacket MIDIEventPacket;

// MIDIEventList structure
struct MIDIEventList {
    MIDIProtocolID protocol;
    uint32_t numPackets;
    MIDIEventPacket packet[1];
};
typedef struct MIDIEventList MIDIEventList;

// MIDIReceiveBlock type for new API callbacks
typedef void (^MIDIReceiveBlock)(const MIDIEventList *evtlist, void * __nullable srcConnRefCon);

// =============================================================================
// Inline helper: advance to next packet in an event list
// =============================================================================
CF_INLINE MIDIEventPacket *MIDIEventPacketNext(const MIDIEventPacket *pkt)
{
    return (MIDIEventPacket *)(&pkt->words[pkt->wordCount]);
}

// =============================================================================
// Internal helper: Convert UMP word to MIDI 1.0 bytes
// Returns number of bytes written (0-3), or 0 if not a convertible message
// =============================================================================
static inline int juce_polyfill_umpToBytes(UInt32 word, UInt8 *bytes)
{
    // UMP Message Type is in bits 28-31
    UInt8 mt = (word >> 28) & 0x0F;
    
    if (mt == 0x2) {
        // MIDI 1.0 Channel Voice Message (32-bit)
        // Format: [mt=2][group][status][data1][data2]
        UInt8 status = (word >> 16) & 0xFF;
        UInt8 data1 = (word >> 8) & 0xFF;
        UInt8 data2 = word & 0xFF;
        
        UInt8 msgType = status & 0xF0;
        
        // Program Change and Channel Pressure only have 1 data byte
        if (msgType == 0xC0 || msgType == 0xD0) {
            bytes[0] = status;
            bytes[1] = data1;
            return 2;
        } else {
            bytes[0] = status;
            bytes[1] = data1;
            bytes[2] = data2;
            return 3;
        }
    }
    else if (mt == 0x1) {
        // System Real Time and System Common (except SysEx) - 32-bit
        UInt8 status = (word >> 16) & 0xFF;
        UInt8 data1 = (word >> 8) & 0xFF;
        UInt8 data2 = word & 0xFF;
        
        // Check what kind of system message
        if (status >= 0xF8) {
            // Real-time: single byte
            bytes[0] = status;
            return 1;
        } else if (status == 0xF1 || status == 0xF3) {
            // MTC Quarter Frame, Song Select: 2 bytes
            bytes[0] = status;
            bytes[1] = data1;
            return 2;
        } else if (status == 0xF2) {
            // Song Position Pointer: 3 bytes
            bytes[0] = status;
            bytes[1] = data1;
            bytes[2] = data2;
            return 3;
        } else if (status == 0xF6) {
            // Tune Request: single byte
            bytes[0] = status;
            return 1;
        }
        return 0;
    }
    else if (mt == 0x4) {
        // MIDI 2.0 Channel Voice Message (64-bit) - degrade to MIDI 1.0
        // We only handle the first word here; caller should skip the second word
        // For simplicity, extract what we can from the first word
        UInt8 status = (word >> 16) & 0xF0;
        UInt8 channel = (word >> 16) & 0x0F;
        UInt8 index = (word >> 8) & 0xFF;   // Note number or controller
        
        // This is incomplete without the second word, but we do our best
        // The actual data is in the second 32-bit word for MIDI 2.0
        // For now, just skip these - they need special handling
        (void)status;
        (void)channel;
        (void)index;
        return 0; // Skip MIDI 2.0 CV messages as we can't properly convert without second word
    }
    
    return 0; // Unknown or unsupported message type
}

// =============================================================================
// MIDIEventListInit - Initialize an event list for building
// =============================================================================
static inline MIDIEventPacket *MIDIEventListInit(MIDIEventList *evtlist, MIDIProtocolID protocol)
{
    evtlist->protocol = protocol;
    evtlist->numPackets = 0;
    evtlist->packet[0].timeStamp = 0;
    evtlist->packet[0].wordCount = 0;
    return &evtlist->packet[0];
}

// =============================================================================
// MIDIEventListAdd - Add an event to the list
// =============================================================================
static inline MIDIEventPacket *MIDIEventListAdd(
    MIDIEventList *evtlist,
    ByteCount listSize,
    MIDIEventPacket *curPacket,
    MIDITimeStamp time,
    ByteCount wordCount,
    const UInt32 *words)
{
    (void)listSize; // We trust the caller provides adequate space
    
    if (curPacket == NULL || wordCount == 0 || wordCount > 64)
        return NULL;
    
    // If current packet is empty, use it; otherwise advance
    if (curPacket->wordCount == 0) {
        curPacket->timeStamp = time;
    } else {
        // Advance to next packet position
        curPacket = MIDIEventPacketNext(curPacket);
        curPacket->timeStamp = time;
        curPacket->wordCount = 0;
        evtlist->numPackets++;
    }
    
    // Copy words
    for (ByteCount i = 0; i < wordCount; i++) {
        curPacket->words[curPacket->wordCount++] = words[i];
    }
    
    // Ensure numPackets reflects at least one packet if we have data
    if (evtlist->numPackets == 0 && curPacket->wordCount > 0)
        evtlist->numPackets = 1;
    
    return curPacket;
}

// =============================================================================
// Internal: Convert MIDIEventList to MIDIPacketList and send
// =============================================================================
static inline OSStatus juce_polyfill_sendEventListAsPacketList(
    MIDIPortRef port,
    MIDIEndpointRef dest,
    const MIDIEventList *evtlist,
    Boolean isReceived)  // true = MIDIReceived, false = MIDISend
{
    // Allocate a packet list on stack (max 65536 bytes per spec)
    UInt8 pktListStorage[4096];
    MIDIPacketList *pktList = (MIDIPacketList *)pktListStorage;
    MIDIPacket *pkt = MIDIPacketListInit(pktList);
    
    const MIDIEventPacket *evtPkt = evtlist->packet;
    
    for (UInt32 i = 0; i < evtlist->numPackets; ++i) {
        // Convert each word in the event packet to MIDI 1.0 bytes
        UInt32 wordIdx = 0;
        while (wordIdx < evtPkt->wordCount) {
            UInt32 word = evtPkt->words[wordIdx];
            UInt8 mt = (word >> 28) & 0x0F;
            
            UInt8 midiBytes[16];
            int numBytes = 0;
            
            if (mt == 0x4) {
                // MIDI 2.0 Channel Voice - 64-bit message
                // Convert to MIDI 1.0 equivalent
                if (wordIdx + 1 < evtPkt->wordCount) {
                    UInt32 word2 = evtPkt->words[wordIdx + 1];
                    
                    UInt8 status = (word >> 16) & 0xF0;
                    UInt8 channel = (word >> 16) & 0x0F;
                    UInt8 index = (word >> 8) & 0x7F;
                    
                    // High-res velocity/value in word2
                    UInt16 velocity16 = (word2 >> 16) & 0xFFFF;
                    UInt8 velocity7 = velocity16 >> 9; // Convert 16-bit to 7-bit
                    
                    midiBytes[0] = status | channel;
                    midiBytes[1] = index;
                    
                    if (status == 0xC0 || status == 0xD0) {
                        // Program Change / Channel Pressure (no velocity byte)
                        numBytes = 2;
                    } else {
                        midiBytes[2] = velocity7 > 0 ? velocity7 : (velocity16 > 0 ? 1 : 0);
                        numBytes = 3;
                    }
                    wordIdx += 2;
                } else {
                    wordIdx++;
                }
            } else {
                numBytes = juce_polyfill_umpToBytes(word, midiBytes);
                wordIdx++;
            }
            
            if (numBytes > 0) {
                pkt = MIDIPacketListAdd(pktList, sizeof(pktListStorage), pkt, 
                                       evtPkt->timeStamp, numBytes, midiBytes);
                if (pkt == NULL) {
                    // Packet list full, send what we have and restart
                    OSStatus err;
                    if (isReceived) {
                        err = MIDIReceived(dest, pktList);
                    } else {
                        err = MIDISend(port, dest, pktList);
                    }
                    if (err != noErr) return err;
                    
                    pkt = MIDIPacketListInit(pktList);
                }
            }
        }
        
        evtPkt = MIDIEventPacketNext(evtPkt);
    }
    
    // Send remaining packets
    if (pktList->numPackets > 0) {
        if (isReceived) {
            return MIDIReceived(dest, pktList);
        } else {
            return MIDISend(port, dest, pktList);
        }
    }
    
    return noErr;
}

// =============================================================================
// MIDISendEventList - Send events through an output port
// =============================================================================
static inline OSStatus MIDISendEventList(
    MIDIPortRef port,
    MIDIEndpointRef dest,
    const MIDIEventList *evtlist)
{
    return juce_polyfill_sendEventListAsPacketList(port, dest, evtlist, false);
}

// =============================================================================
// MIDIReceivedEventList - Distribute events from a virtual source
// =============================================================================
static inline OSStatus MIDIReceivedEventList(
    MIDIEndpointRef src,
    const MIDIEventList *evtlist)
{
    return juce_polyfill_sendEventListAsPacketList(0, src, evtlist, true);
}

// =============================================================================
// Internal: Context for bridging new block-based API to old callback API
// =============================================================================
typedef struct {
    MIDIReceiveBlock receiveBlock;
    MIDIProtocolID protocol;
} juce_polyfill_ReceiveContext;

// Internal read proc that converts MIDIPacketList to MIDIEventList
static void juce_polyfill_readProc(
    const MIDIPacketList *pktlist,
    void *readProcRefCon,
    void *srcConnRefCon)
{
    juce_polyfill_ReceiveContext *ctx = (juce_polyfill_ReceiveContext *)readProcRefCon;
    if (!ctx || !ctx->receiveBlock) return;
    
    // Convert packet list to event list
    // Allocate on stack
    UInt8 evtListStorage[4096];
    MIDIEventList *evtList = (MIDIEventList *)evtListStorage;
    MIDIEventPacket *evtPkt = MIDIEventListInit(evtList, ctx->protocol);
    
    const MIDIPacket *pkt = pktlist->packet;
    
    for (UInt32 i = 0; i < pktlist->numPackets; ++i) {
        const UInt8 *data = pkt->data;
        UInt16 len = pkt->length;
        UInt16 pos = 0;
        
        while (pos < len) {
            UInt8 status = data[pos];
            
            // Skip if not a status byte (running status not fully supported)
            if (!(status & 0x80)) {
                pos++;
                continue;
            }
            
            UInt32 umpWord = 0;
            int msgLen = 0;
            
            if (status >= 0xF8) {
                // System Real-Time (1 byte)
                // UMP Format: mt=1, group=0, status, 0, 0
                umpWord = (0x1 << 28) | (0 << 24) | (status << 16);
                msgLen = 1;
            }
            else if (status == 0xF0) {
                // SysEx start - skip for now (complex to handle)
                while (pos < len && data[pos] != 0xF7) pos++;
                if (pos < len) pos++; // skip F7
                continue;
            }
            else if (status >= 0xF0) {
                // System Common
                int dataBytes = 0;
                if (status == 0xF1 || status == 0xF3) dataBytes = 1;
                else if (status == 0xF2) dataBytes = 2;
                
                umpWord = (0x1 << 28) | (0 << 24) | (status << 16);
                if (dataBytes >= 1 && pos + 1 < len) umpWord |= (data[pos + 1] << 8);
                if (dataBytes >= 2 && pos + 2 < len) umpWord |= data[pos + 2];
                msgLen = 1 + dataBytes;
            }
            else {
                // Channel Voice Message
                UInt8 msgType = status & 0xF0;
                int dataBytes = 2;
                if (msgType == 0xC0 || msgType == 0xD0) dataBytes = 1;
                
                // UMP Format for MIDI 1.0 CV: mt=2, group=0, status, data1, data2
                umpWord = (0x2 << 28) | (0 << 24) | (status << 16);
                if (dataBytes >= 1 && pos + 1 < len) umpWord |= (data[pos + 1] << 8);
                if (dataBytes >= 2 && pos + 2 < len) umpWord |= data[pos + 2];
                msgLen = 1 + dataBytes;
            }
            
            if (msgLen > 0) {
                evtPkt = MIDIEventListAdd(evtList, sizeof(evtListStorage), evtPkt,
                                         pkt->timeStamp, 1, &umpWord);
            }
            
            pos += msgLen;
        }
        
        pkt = MIDIPacketNext(pkt);
    }
    
    if (evtList->numPackets > 0) {
        ctx->receiveBlock(evtList, srcConnRefCon);
    }
}

// =============================================================================
// MIDIInputPortCreateWithProtocol - Create input port with block callback
// =============================================================================
static inline OSStatus MIDIInputPortCreateWithProtocol(
    MIDIClientRef client,
    CFStringRef portName,
    MIDIProtocolID protocol,
    MIDIPortRef *outPort,
    MIDIReceiveBlock receiveBlock)
{
    // Allocate context (leaked intentionally - lives for app lifetime)
    juce_polyfill_ReceiveContext *ctx = (juce_polyfill_ReceiveContext *)malloc(sizeof(juce_polyfill_ReceiveContext));
    ctx->receiveBlock = Block_copy(receiveBlock);
    ctx->protocol = protocol;
    
    return MIDIInputPortCreate(client, portName, juce_polyfill_readProc, ctx, outPort);
}

// =============================================================================
// MIDIDestinationCreateWithProtocol - Create virtual destination with block callback
// =============================================================================
static inline OSStatus MIDIDestinationCreateWithProtocol(
    MIDIClientRef client,
    CFStringRef name,
    MIDIProtocolID protocol,
    MIDIEndpointRef *outDest,
    MIDIReceiveBlock readBlock)
{
    // Allocate context (leaked intentionally - lives for app lifetime)
    juce_polyfill_ReceiveContext *ctx = (juce_polyfill_ReceiveContext *)malloc(sizeof(juce_polyfill_ReceiveContext));
    ctx->receiveBlock = Block_copy(readBlock);
    ctx->protocol = protocol;
    
    return MIDIDestinationCreate(client, name, juce_polyfill_readProc, ctx, outDest);
}

// =============================================================================
// MIDISourceCreateWithProtocol - Create virtual source
// =============================================================================
static inline OSStatus MIDISourceCreateWithProtocol(
    MIDIClientRef client,
    CFStringRef name,
    MIDIProtocolID protocol,
    MIDIEndpointRef *outSrc)
{
    (void)protocol; // Ignored on 10.13, always MIDI 1.0
    return MIDISourceCreate(client, name, outSrc);
}

#endif // macOS < 11.0
