#pragma once
#include <CoreFoundation/CoreFoundation.h>
#include <CoreMIDI/CoreMIDI.h>

typedef CF_ENUM(SInt32, MIDIProtocolID) {
    kMIDIProtocol_1_0 = 1,
    kMIDIProtocol_2_0 = 2,
};
struct MIDIEventPacket {
    MIDITimeStamp timeStamp;
    UInt32 wordCount;
    UInt32 words[64];
};
typedef struct MIDIEventPacket MIDIEventPacket;
struct MIDIEventList {
    MIDIProtocolID protocol;
    UInt32 numPackets;
    MIDIEventPacket packet[1];
};
typedef struct MIDIEventList MIDIEventList;
