// File generated by the Adl compiler.   Do not edit.

#include "AdlStructures.h"
#include "Utils.h"

extern const unsigned BitMasks[32] = {
    0x1,      0x3,      0x7,       0xf,       0x1f,      0x3f,      0x7f,       0xff,       0x1ff,      0x3ff,      0x7ff,
    0xfff,    0x1fff,   0x3fff,    0x7fff,    0xffff,    0x1ffff,   0x3ffff,    0x7ffff,    0xfffff,    0x1fffff,   0x3fffff,
    0x7fffff, 0xffffff, 0x1ffffff, 0x3ffffff, 0x7ffffff, 0xfffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff,
};

void BitStream::Add(int val, int cnt)
{
    val &= BitMasks[cnt - 1];
    int v = 8 - (bits & 7);
    //	std::cout << val << ";" << cnt << std::endl;
    if (cnt > v)
    {
        if (v != 8)
        {
            // assumes won't cross byte boundary
            bytes[bits >> 3] |= (val >> (cnt - v));
            cnt -= v;
            bits += v;
        }
        while (cnt > 0)
        {
            if (bigEndian)
            {
                Utils::Fatal("big endian not supported in BitStream::Add");
            }
            bytes[bits >> 3] = (val & 0xff);
            val >>= 8;
            bits += 8;
            cnt -= 8;
        }
    }
    else
    {
        bytes[bits >> 3] |= val << (v - cnt);
        bits += (cnt);
    }
}
