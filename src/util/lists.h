#ifndef LISTSH
#define LISTSH


/** \ingroup UtilAPI */

/** \file

\brief Generic list functions.

Void lists store untyped objects (void pointers) and Int and Float
lists are special cased to work with appropriately typed data. The
interfaces for the three are the same except all functions for
VoidLists are prefaced with 'VL', IntList with 'IL', and FloatList
with 'FL'.  \b Important The documentation says 'VAL' instead of just
'VL' as a prefix to all functions, macros do the expansion to the
array version of lists by default so these are the same in practice.

And since these list data structures are basically arrays that are
dynamically sized, inserting or removing elements is O(N) but
accessing elements is O(1).

This module also has linked lists which are used very very rarely in
VFML.  You can access them by adding an additional L to the name, for
example VLNew() -> VLLNew()


<HR>

\b Examples 

To iterate over the values in a VoidList:

\code
VoidListPtr list = (some existing list); 
void *e;
int i;

for(i = 0 ; i < VLLength(list) ; i++) {
   e = VLIndex(list, i);
}
\endcode

The same thing with a FloatList:

\code
FloatListPtr list = (some existing list); 
float f;
int i;

for(i = 0 ; i < FLLength(list) ; i++) {
   f = FLIndex(list, i);
}
\endcode

*/

/* Generic list code.  VoidList is for when you want a list of pointers to
   things and IntList is for when you just want to store a bunch of integers
    in a list.

    IntList is so you don't always have to create a bunch of pointers to ints
    to store them.

   By Geoff Hulten
   Revised 2/5/98
*/

/* HERE add some asserts to this code */

//#define LINKEDLISTS

#ifdef LINKEDLISTS
   #define VoidList 	VoidLList
   #define VoidListPtr 	VoidLListPtr
   #define VoidListHandle 	VoidLListHandle

   #define VLNew		VLLNew
   #define VLAppend		VLLAppend
   #define VLPush		VLLPush
   #define VLLength		VLLLength
   #define VLIndex		VLLIndex
   #define VLSort		VLLSort
   #define VLRemove		VLLRemove
   #define VLInsert		VLLInsert
   #define VLSet		VLLSet
   #define VLFree		VLLFree

#else /* ARRAYLISTS */
   #define VoidList 	VoidAList
   #define VoidListPtr 	VoidAListPtr
   #define VoidListHandle 	VoidAListHandle

   #define VLNew		VALNew
   #define VLAppend		VALAppend
   #define VLPush		VALPush
   #define VLLength		VALLength
   #define VLIndex		VALIndex
   #define VLSort		VALSort
   #define VLRemove		VALRemove
   #define VLInsert		VALInsert
   #define VLSet		VALSet
   #define VLFree		VALFree
#endif

typedef struct _VoidLList_ {
   void *element;
   struct _VoidLList_ *next;
} VoidLList, *VoidLListPtr, **VoidLListHandle;

VoidLListPtr VLLNew(void);
/* append the element to the end of the list */
void VLLAppend(VoidLListPtr list, void *element);
/* push element on the head of the list */
void VLLPush(VoidLListPtr list, void *element);
int VLLLength(VoidLListPtr list);

void *VLLIndex(VoidLListPtr list, long index);
void VLLSort(VoidLListPtr list, int (*cmp)(const void *, const void *));
void *VLLRemove(VoidLListPtr list, long index);
void VLLInsert(VoidLListPtr list, void *element, long index);
void VLLSet(VoidLListPtr list, long index, void *newVal);
void VLLFree(VoidLListPtr list);

/** \struct VoidListPtr
\brief An ADT for a list that holds void pointers.

See lists.h for more detail.
*/


typedef struct _VoidAList_ {
   int size;
   int allocatedSize;
   void **array;
} VoidAList, *VoidAListPtr, **VoidAListHandle;

/** \brief Creates a new list of the specified type. */
VoidAListPtr VALNew(void);
/** \brief Append the element to the end of the list. */
void VALAppend(VoidAListPtr list, void *element);
/** \brief Push element on the head of the list. */
void VALPush(VoidAListPtr list, void *element);

/** \brief Returns the number of elements in the list. */
//int VALLength(VoidAListPtr list);
#define VALLength(list) \
   (((VoidAListPtr)(list))->size)

/** \brief Returns the element at the index (zero based). 

Note that the indexing is 0 based (like a C array).
*/
//void *VALIndex(VoidAListPtr list, long index);
#define VALIndex(list, index) \
    ( (index < ((VoidAListPtr)list)->size) ? (((VoidAListPtr)list)->array[index]) : (0) )

/** \brief Uses quick sort to sort the list in place with the passed
comparision function.

Note this is not implemented for FloatLists. 
*/
void VALSort(VoidAListPtr list, int (*cmp)(const void *, const void *));

/** \brief Removes the item at the specified index and returns it. 

Shifts later elements over. Note that the indexing is 0 based (like a C array).
*/
void *VALRemove(VoidAListPtr list, long index);

/** \brief Inserts the element at the specified index.

The element that used to have that index will have an index one higher
(that is, it shifts later elements over). Note that the indexing is 0
based (like a C array).
*/
void VALInsert(VoidAListPtr list, void *element, long index);

/** \brief Sets the value at the specified index.

The previous value at the index is overwritten. Note that the indexing
is 0 based (like a C array).
*/
void VALSet(VoidAListPtr list, long index, void *newVal);

/** \brief Frees the memory associated with the list.

You are responsible for any memory used by the elements of the list.
*/
void VALFree(VoidAListPtr list);

/** \struct IntListPtr
\brief An ADT for a list that holds void ints.

See lists.h for more detail.
*/

typedef VoidAList    IntList;
typedef VoidAListPtr IntListPtr;

#define ILNew()             VLNew()
#define ILClone(l)          VLClone(l)
#define ILAppend(l, e)      VLAppend(l, (void *)e)
#define ILPush(l, e)        VLPush(l, (void *) e)
#define ILInsert(l, e, i)   VLInsert(l, (void *) e, i)
#define ILLength(l)         VLLength(l)
#define ILIndex(l, i)       (void *)VLIndex(l, i)
#define ILRemove(l, i)      (void *)VLRemove(l, i)
#define ILSet(l, i, e)      VLSet(l, i, (void *)e)
#define ILFree(l)           VLFree(l)
#define ILFind(l, e)        VLFind(l, (void *)e)

//typedef struct _IntList_ {
//   int theInt;
//   struct _IntList_ *next;
//} IntList, *IntListPtr, **IntListHandle;

//IntListPtr ILNew(void);
//IntListPtr ILClone(IntListPtr orig);
/* append the element to the end of the list */
//void ILAppend(IntListPtr list, int element);
/* push element on the head of the list */
//void ILPush(IntListPtr list, int element);
//void ILInsert(IntListPtr list, int element, long index);
//int ILLength(IntListPtr list);
//int ILIndex(IntListPtr list, long index);
//int ILRemove(IntListPtr list, long index);
//void ILSet(IntListPtr list, long index, int newVal);
//void ILFree(IntListPtr list);
//int ILFind(IntListPtr list, int element);


/** \struct FloatListPtr
\brief An ADT for a list that holds floats.

See lists.h for more detail.
*/

typedef struct _FloatList_ {
   int size;
   int allocatedSize;
   float *array;
} FloatList, *FloatListPtr, **FloatListHandle;

FloatListPtr FLNew(void);
FloatListPtr FLClone(FloatListPtr orig);
/* append the element to the end of the list */
void FLAppend(FloatListPtr list, float element);
/* push element on the head of the list */
void FLPush(FloatListPtr list, float element);
void FLInsert(FloatListPtr list, float element, long index);
int FLLength(FloatListPtr list);

float FLIndex(FloatListPtr list, long index);
float FLRemove(FloatListPtr list, long index);
void FLSet(FloatListPtr list, long index, float newVal);
void FLFree(FloatListPtr list);
int FLFind(FloatListPtr list, float element);

#endif
