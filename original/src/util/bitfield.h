#ifndef BITFIELDH
#define BITFIELDH

/** \ingroup UtilAPI
*/

/** \file

\brief Compactly represent a bit field.

*/

/** \brief ADT for compactly representing a bit field 

See bitfield.h for more detail.
*/
typedef struct _BITFIELD_ {
   int numBytes;
   unsigned char *data;
} BitFieldStruct, *BitFieldPtr;

typedef BitFieldPtr BitField;

/** \brief Create a new BitField with the specified number of bits */
BitField BitFieldNew(int byteSize);

void BitFieldSetSizeData(BitField b, int byteSize, unsigned char *data);

/** \brief Frees the memory associated with the BitField */
void BitFieldFree(BitField b);

unsigned char *BitFieldFreeSaveData(BitField b);

/** \brief Returns the number of bytes being used to represent the BitField */
int BitFieldGetNumBytes(BitField b);

/** \brief Returns the value of the specifed bit */
int BitFieldGetBit(BitField b, long offset);

/** \brief Sets the value of the specified bit */
void BitFieldSetBit(BitField b, long offset, int val);

#endif
