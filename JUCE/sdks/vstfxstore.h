// Substitute for Steinberg's vstfxstore.h
// WARNING: Not tested

#pragma once
#pragma pack(push,1)
struct fxBank {
    int chunkMagic, byteSize, fxMagic, version, fxID, fxVersion, numPrograms;
    char reserved[128];
    struct {
        struct {
            int size;
            char chunk[1];
        } data;
    } content;
};
#pragma pack(pop)
