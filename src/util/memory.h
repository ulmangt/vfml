#ifndef MEMORYH
#define MEMORYH

/** \ingroup UtilAPI */

/** \file

\brief Tracks the size of allocations made.

A wrapper for the standard memory manager that tracks the size of the
allocations made using it. This requires an extra 8 bytes per
allocation. You can turn this off by editing the memory.h file and
commenting out the definition of DEBUGMEMORY, but this will cause some
of the features of some of the research learners to fail in
unpredictable ways.

*/

/** \brief A wrapper around Malloc that tracks the size of the allocation.

Make sure you use MFreePtr to free any memory allocated by this call
or your program will probably crash.
*/
void *MNewPtr(int size);

/** \brief Frees the memory held by the pointer and tracks the change in the module's records. */
void MFreePtr(void *ptr);

//void **MNewHandle(int size);
//void MFreeHandle(void **handle);

//void *MLockHandle(void **handle);
//void MUnLockHandle(void **handle);


/** \brief Sets a function that is called if an allocation fails.

If you call this, then your AllocFail function will be called if an
allocation fails. After your function returns the memory module will
try the allocation agian, if the allocation fails again, the memory
module returns a NULL pointer. You could use AllocFail to flush
caches, or clean up the program and quit.
*/
void MSetAllocFailFunction(void (*AllocFail)(int allocationSize));

#include "sysdefines.h"

#define DEBUGMEMORY

//void MSetAllocFailFunction(void (*AllocFail)(int allocationSize));
void *__MAllocPtrWrapper(int size);
void **__MAllocHandleWrapper(int size);


#if defined(UNIX) | defined(CYGNUS) | defined(WIN32)
   #define __SystemNewPtr(size)          malloc(size)
   #define __SystemFreePtr(ptr)          free(ptr)

   #define __SystemNewHandle(size)       __MMakeHandle(size)
   #define __SystemFreeHandle(handle)    (__MFreeHandle((void **)handle))

   void **__MMakeHandle(long size);
   void __MFreeHandle(void **handle);

   #define __SystemLockHandle(handle)    (*handle)
   #define __SystemUnlockHandle(handle)  {} /* MUnlockHandle does nothing */

   #define __SystemMoveMemory(dst, src, bytes)	memmove(dst, src, bytes)

#elif defined(PILOT)
   #define __SystemNewPtr(size)          MemPtrNew(size)
   #define __SystemFreePtr(ptr)          MemPtrFree(ptr)

   #define __SystemNewHandle(size)       MemHandleNew(size)
   #define __SystemFreeHandle(handle)    MemHandleFree(handle)

   #define __SystemLockHandle(handle)    MemHandleLock(handle)
   #define __SystemUnlockHandle(handle)  MemHandleUnlock(handle)

   #define __SystemMoveMemory(dst, src, bytes)	MemMove(dst, src, bytes)
#endif /* UNIX | PILOT | WIN32 */

#ifdef DEBUGMEMORY
   void *_MDebugNewPtr(long size);

   //#define _MDebugNewPtr(size)   (__SystemNewPtr(size))

   void _MDebugFreePtr(void *ptr);
   void **_MDebugNewHandle(long size);
   void _MDebugFreeHandle(void **handle);
   void *_MDebugLockHandle(void **handle);
   void _MDebugUnlockHandle(void **handle);

/** \brief Returns the number of bytes that are currently allocated by the module.*/
   long MGetTotalAllocation(void);

/** \brief Set the pool to track future memory allocations.

The module supports up to 9 pools of memory, with id 1-9. You can get
more fine grained tracking by setting the pool and using
MGetPoolAllocation. There is also a special pool, with id 0, and any
allocations made when that pool are set are tracked in the pool but
will not show up in MGetTotalAllocation. The default pool id is 1.
*/
   void MSetActivePool(int poolID);

/** \brief Find which pool is being used to track memory.

The module supports up to 9 pools of memory, with id 1-9. You can get
more fine grained tracking by setting the pool and using
MGetPoolAllocation. There is also a special pool, with id 0, and any
allocations made when that pool are set are tracked in the pool but
will not show up in MGetTotalAllocation. The default pool id is 1.
*/
   int MGetActivePool(void);

/** \brief Returns the number of bytes that are currently allocated in
the specified pool.

The module supports up to 9 pools of memory, with id 1-9. You can get
more fine grained tracking by setting the pool and using
MGetPoolAllocation. There is also a special pool, with id 0, and any
allocations made when that pool are set are tracked in the pool but
will not show up in MGetTotalAllocation. The default pool id is 1.
*/
   long MGetPoolAllocation(int poolID);

/** \brief Moves the memory from one pool to another 

Changes (if needed) the tracking of the ptr so that it is now counted
against the new pool instead of the pool it was allocated into.
*/
   void MMovePtrToPool(void *ptr, int poolID);

   #define MNewPtr(size)          _MDebugNewPtr(size)
   #define MFreePtr(ptr)          _MDebugFreePtr(ptr)

   #define MNewHandle(size)       __SystemNewHandle(size)
   #define MFreeHandle(handle)    __SystemFreeHandle(handle)

   #define MLockHandle(handle)    __SystemLockHandle(handle)
   #define MUnlockHandle(handle)  __SystemUnlockHandle(handle)

/** \brief Wrapper for memmove that works with pointers allocated by this memory module. */
   #define MMemMove(dst, src, bytes) __SystemMoveMemory(dst, src, bytes)

#else /* not debuggin memory */
   #define MGetTotalAllocation()        (0)
   #define MSetActivePool(poolID)       (0)
   #define MGetActivePool()             (0)
   #define MGetPoolAllocation(poolID)   (0)
   #define MMovePtrToPool(ptr, poolID)  (0)

   #define MNewPtr(size)          __MAllocPtrWrapper(size)
   #define MFreePtr(ptr)          __SystemFreePtr(ptr)

   #define MNewHandle(size)       __MAllocHandleWrapper(size)
   #define MFreeHandle(handle)    __SystemFreeHandle(handle)

   #define MLockHandle(handle)    __SystemLockHandle(handle)
   #define MUnlockHandle(handle)  __SystemUnlockHandle(handle)
   #define MMemMove(dst, src, bytes) __SystemMoveMemory(dst, src, bytes)
#endif /* Debugging or not */

#endif /* MEMORYH */
