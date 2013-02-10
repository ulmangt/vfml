#ifndef HASHTABLE_H
#define HASHTABLE_H

/** \ingroup UtilAPI */

/** \file

\brief A hash table.

\thanks to Chun-Hsiang Hung for implementing the HashTable ADT.

\wish A cleaner hash table interface (without the compare function).
I think that the sprint learner is the only thing that uses this
currently so there isn't much to change to fix this.

*/

#include "vfml.h"

/** \brief A hash table ADT 

See HashTable.h for more detail.
*/
typedef struct {
  VoidListPtr *array;
  int size;
} HashTable, *HashTablePtr;

/** \brief Creates a new hash table with the specified number of entries */
HashTable *HashTableNew(int size);

/** \brief Inserts the element into the has table at the appropriate place */
void HashTableInsert(HashTable *table, int index, void *element);

/** \brief Looks up index in the hash table.

Not only must you use the same index, but you must also supply a
function that returns non-zero when passed the element you want to
find and the index (which you passed to the function in the first
place...)
*/
void *HashTableFind(HashTable *table, int index, 
		    int (*cmp)(const void *, const int));

/** \brief Frees the memory being used by the hash table.

But doesn't touch the memory being used by the elements in the table.
It is your responsibility to free these if you like.
*/
void HashTableFree(HashTable *table);

/** \brief Removes the element from the hash table.

Not only must you use the same index, but you must also supply a
function that returns non-zero when passed the element you want to
find and the index (which you passed to the function in the first
place...)
*/
void *HashTableRemove(HashTable *table, int index,
		      int (*cmp)(const void *, const int));

#endif
