#include "sysdefines.h"
#include "bitfield.h"
#include "memory.h"

BitField BitFieldNew(int byteSize) {
   BitField b = (BitFieldStruct *)MNewPtr(sizeof(BitFieldStruct));

   b->numBytes = byteSize;
   b->data = (unsigned char *)MNewPtr(byteSize);

   return b;
}

void BitFieldSetSizeData(BitField b, int byteSize, unsigned char *data) {
   MFreePtr(b->data);

   b->numBytes = byteSize;
   b->data = data;
}

void BitFieldFree(BitField b) {
   MFreePtr(b->data);
   MFreePtr(b);
}

unsigned char *BitFieldFreeSaveData(BitField b) {
   unsigned char *data = b->data;

   MFreePtr(b);


   return data;
}

int BitFieldGetNumBytes(BitField b) {
   return b->numBytes;
}

static unsigned char *_FindByteOfBit(BitField b, long offset) {
   int index = offset / 8;

   return (b->data + index);
}

int BitFieldGetBit(BitField b, long offset) {
   unsigned char *byte = _FindByteOfBit(b, offset);
   unsigned char theByte = *byte;
   int bitOfByte = offset % 8;

   theByte = theByte >> bitOfByte;

   return theByte & 0x01;
}

void BitFieldSetBit(BitField b, long offset, int val) {
   unsigned char *byte = _FindByteOfBit(b, offset);
   unsigned char mask;
   int bitOfByte = offset % 8;

   mask = 0x01;
   mask = mask << bitOfByte;

   if(val) {
      *byte = *byte | mask;
   } else {
      mask = ~mask;

      *byte = *byte & mask;
   }
}

