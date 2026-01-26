#pragma once
#include <CoreFoundation/CoreFoundation.h>
#include <CoreMIDI/CoreMIDI.h>
#include <cstdint>
#include <Block.h>

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
// Internal helper: Get MIDI 1.0 message length from status byte
// Returns expected data bytes (0, 1, or 2), or -1 for special/variable length
// =============================================================================
static inline int juce_polyfill_getMidi1DataLength(uint8_t status)
{
    if (status < 0x80) return -1;  // Not a status byte
    
    uint8_t msgType = status & 0xF0;
    
    if (status < 0xF0) {
        // Channel messages
        switch (msgType) {
            case 0x80: return 2;  // Note Off
            case 0x90: return 2;  // Note On
            case 0xA0: return 2;  // Poly Aftertouch
            case 0xB0: return 2;  // Control Change
            case 0xC0: return 1;  // Program Change
            case 0xD0: return 1;  // Channel Pressure
            case 0xE0: return 2;  // Pitch Bend
            default:   return -1;
        }
    } else {
        // System messages
        switch (status) {
            case 0xF0: return -1; // SysEx Start (variable)
            case 0xF1: return 1;  // MTC Quarter Frame
            case 0xF2: return 2;  // Song Position Pointer
            case 0xF3: return 1;  // Song Select
            case 0xF4: return -1; // Undefined
            case 0xF5: return -1; // Undefined
            case 0xF6: return 0;  // Tune Request
            case 0xF7: return -1; // SysEx End
            default:   return 0;  // Real-time (F8-FF): no data bytes
        }
    }
}

// =============================================================================
// Internal helper: Convert UMP word to MIDI 1.0 bytes
// Returns number of bytes written (0-3), or 0 if not a convertible message
// =============================================================================
static inline int juce_polyfill_umpToBytes(uint32_t word, uint8_t *bytes)
{
    // UMP Message Type is in bits 28-31
    uint8_t mt = (word >> 28) & 0x0F;
    
    if (mt == 0x2) {
        // MIDI 1.0 Channel Voice Message (32-bit)
        // Format: [mt:4][group:4][status:8][data1:8][data2:8]
        uint8_t status = (word >> 16) & 0xFF;
        uint8_t data1 = (word >> 8) & 0xFF;
        uint8_t data2 = word & 0xFF;
        
        int dataLen = juce_polyfill_getMidi1DataLength(status);
        if (dataLen < 0) return 0;
        
        bytes[0] = status;
        if (dataLen >= 1) bytes[1] = data1 & 0x7F;  // Ensure 7-bit
        if (dataLen >= 2) bytes[2] = data2 & 0x7F;
        return 1 + dataLen;
    }
    else if (mt == 0x1) {
        // System Real Time and System Common (except SysEx) - 32-bit
        // Format: [mt:4][group:4][status:8][data1:8][data2:8]
        uint8_t status = (word >> 16) & 0xFF;
        uint8_t data1 = (word >> 8) & 0xFF;
        uint8_t data2 = word & 0xFF;
        
        int dataLen = juce_polyfill_getMidi1DataLength(status);
        if (dataLen < 0) return 0;  // SysEx or undefined
        
        bytes[0] = status;
        if (dataLen >= 1) bytes[1] = data1 & 0x7F;
        if (dataLen >= 2) bytes[2] = data2 & 0x7F;
        return 1 + dataLen;
    }
    // MT 0x4 (MIDI 2.0 CV) is handled separately as it's 64-bit
    // MT 0x3 (SysEx) requires special handling
    // MT 0x0 (Utility) and MT 0x5 (Data) are not converted
    
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
// Note: Uses UInt32* to match Apple's CoreMIDI API signature
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
    
    if (curPacket == NULL || words == NULL || wordCount == 0 || wordCount > 64)
        return NULL;
    
    // If current packet is empty, use it; otherwise advance
    if (curPacket->wordCount == 0) {
        curPacket->timeStamp = time;
    } else {
        // Check if we'd overflow the packet's word capacity
        if (curPacket->wordCount + wordCount > 64) {
            // Advance to next packet position
            curPacket = MIDIEventPacketNext(curPacket);
            curPacket->timeStamp = time;
            curPacket->wordCount = 0;
            evtlist->numPackets++;
        }
    }
    
    // Copy words (UInt32 and uint32_t are compatible)
    for (ByteCount i = 0; i < wordCount; i++) {
        if (curPacket->wordCount >= 64) return NULL;  // Safety check
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
    uint8_t pktListStorage[4096];
    MIDIPacketList *pktList = (MIDIPacketList *)pktListStorage;
    MIDIPacket *pkt = MIDIPacketListInit(pktList);
    
    const MIDIEventPacket *evtPkt = evtlist->packet;
    
    for (uint32_t i = 0; i < evtlist->numPackets; ++i) {
        // Convert each word in the event packet to MIDI 1.0 bytes
        uint32_t wordIdx = 0;
        while (wordIdx < evtPkt->wordCount) {
            uint32_t word = evtPkt->words[wordIdx];
            uint8_t mt = (word >> 28) & 0x0F;
            
            uint8_t midiBytes[16];
            int numBytes = 0;
            
            if (mt == 0x4) {
                // MIDI 2.0 Channel Voice - 64-bit message
                // Format: [mt:4][group:4][status:4][channel:4][index:8][attrType:8]
                //         [data/velocity:16][attrData:16]
                // Convert to MIDI 1.0 equivalent
                if (wordIdx + 1 < evtPkt->wordCount) {
                    uint32_t word2 = evtPkt->words[wordIdx + 1];
                    
                    // Correct parsing for MIDI 2.0 CV
                    uint8_t statusNibble = (word >> 20) & 0x0F;  // Note On=9, Note Off=8, etc.
                    uint8_t channel = (word >> 16) & 0x0F;
                    uint8_t index = (word >> 8) & 0x7F;
                    
                    // Construct MIDI 1.0 status byte
                    uint8_t status = (statusNibble << 4) | channel;
                    
                    // High-res velocity/value in word2 upper 16 bits
                    uint16_t data16 = (word2 >> 16) & 0xFFFF;
                    uint8_t data7 = data16 >> 9;  // Convert 16-bit to 7-bit
                    
                    midiBytes[0] = status;
                    midiBytes[1] = index;
                    
                    // Program Change (0xC) and Channel Pressure (0xD) have different format
                    if (statusNibble == 0xC || statusNibble == 0xD) {
                        // Program Change: index is program, no velocity
                        // Channel Pressure: data is in word2
                        numBytes = 2;
                    } else {
                        // Note On/Off, CC, etc: use velocity/data from word2
                        midiBytes[2] = data7 > 0 ? data7 : (data16 > 0 ? 1 : 0);
                        numBytes = 3;
                    }
                    wordIdx += 2;
                } else {
                    // Incomplete 64-bit message, skip
                    wordIdx++;
                }
            } else {
                numBytes = juce_polyfill_umpToBytes(word, midiBytes);
                wordIdx++;
            }
            
            if (numBytes > 0) {
                pkt = MIDIPacketListAdd(pktList, sizeof(pktListStorage), pkt, 
                                       evtPkt->timeStamp, (ByteCount)numBytes, midiBytes);
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
    uint8_t evtListStorage[4096];
    MIDIEventList *evtList = (MIDIEventList *)evtListStorage;
    MIDIEventPacket *evtPkt = MIDIEventListInit(evtList, ctx->protocol);
    
    const MIDIPacket *pkt = pktlist->packet;
    
    for (uint32_t i = 0; i < pktlist->numPackets; ++i) {
        const uint8_t *data = pkt->data;
        uint16_t len = pkt->length;
        uint16_t pos = 0;
        
        while (pos < len) {
            uint8_t status = data[pos];
            
            // Skip if not a status byte (running status not fully supported)
            if (!(status & 0x80)) {
                pos++;
                continue;
            }
            
            UInt32 umpWord = 0;
            int msgLen = 0;
            
            if (status >= 0xF8) {
                // System Real-Time (1 byte) - no data bytes
                // UMP Format: mt=1, group=0, status, 0, 0
                umpWord = ((UInt32)0x1 << 28) | ((UInt32)status << 16);
                msgLen = 1;
            }
            else if (status == 0xF0) {
                // SysEx start - skip the entire sysex message
                pos++;  // Skip F0
                while (pos < len && data[pos] != 0xF7) pos++;
                if (pos < len) pos++;  // Skip F7
                continue;
            }
            else if (status == 0xF7) {
                // Stray SysEx end - skip
                pos++;
                continue;
            }
            else if (status >= 0xF0) {
                // System Common (F1-F6)
                int dataBytes = juce_polyfill_getMidi1DataLength(status);
                if (dataBytes < 0) {
                    pos++;
                    continue;  // Skip undefined
                }
                
                // Check we have enough data
                if (pos + dataBytes >= len) {
                    pos++;
                    continue;  // Incomplete message, skip
                }
                
                umpWord = ((UInt32)0x1 << 28) | ((UInt32)status << 16);
                if (dataBytes >= 1) umpWord |= ((UInt32)(data[pos + 1] & 0x7F) << 8);
                if (dataBytes >= 2) umpWord |= (UInt32)(data[pos + 2] & 0x7F);
                msgLen = 1 + dataBytes;
            }
            else {
                // Channel Voice Message (0x80 - 0xEF)
                int dataBytes = juce_polyfill_getMidi1DataLength(status);
                if (dataBytes < 0) {
                    pos++;
                    continue;
                }
                
                // Check we have enough data
                if (pos + dataBytes >= len) {
                    pos++;
                    continue;  // Incomplete message, skip
                }
                
                // UMP Format for MIDI 1.0 CV: mt=2, group=0, status, data1, data2
                umpWord = ((UInt32)0x2 << 28) | ((UInt32)status << 16);
                if (dataBytes >= 1) umpWord |= ((UInt32)(data[pos + 1] & 0x7F) << 8);
                if (dataBytes >= 2) umpWord |= (UInt32)(data[pos + 2] & 0x7F);
                msgLen = 1 + dataBytes;
            }
            
            if (msgLen > 0 && evtPkt != NULL) {
                evtPkt = MIDIEventListAdd(evtList, sizeof(evtListStorage), evtPkt,
                                         pkt->timeStamp, 1, &umpWord);
            }
            
            pos += (uint16_t)msgLen;
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
