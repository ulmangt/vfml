#include "sysdefines.h"
#include "memory.h"
#include "lists.h"
#include <limits.h>

void (*_AllocFailFunction)(int allocationSize);
int _IsAllocFailFunctionSet = 0;

void MSetAllocFailFunction(void (*AllocFail)(int allocationSize)) {
   _AllocFailFunction = AllocFail;
   _IsAllocFailFunctionSet = 1;
}

void *__MAllocPtrWrapper(int size) {
   //ULong free;
   //ULong max;

   void *tmp = __SystemNewPtr(size);
   
   //MemHeapFreeBytes(MemPtrHeapID(tmp), &free, &max);
      
   if(tmp == NULL) {
      if(_IsAllocFailFunctionSet) {
         (*_AllocFailFunction)(size);

         //MemHeapFreeBytes(MemPtrHeapID(tmp), &free, &max);
      
         tmp = __SystemNewPtr(size);
      }
   }
   
   return tmp;
}
void **__MAllocHandleWrapper(int size) {
   void **tmp = __SystemNewHandle(size);
   
   if(tmp == NULL) {
      if(_IsAllocFailFunctionSet) {
         (*_AllocFailFunction)(size);
         tmp = __SystemNewHandle(size);
      }
   }
   
   return tmp;
}

#if defined(UNIX) | defined(CYGNUS) | defined(WIN32)

void **__MMakeHandle(long size) {
   void **handle;

   handle = malloc(sizeof(void **));   
   *handle = malloc(size);

   return handle;
}

void __MFreeHandle(void **handle) {
   free(*handle);
   free(handle);
}

#endif /* UNIX */

#ifdef DEBUGMEMORY

/* pool 0 isn't counted against the total allocated size */
unsigned int _allocatedSize = 0;
unsigned int _poolSizes[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int  _currentPool   = 1;

// USHRT_MAX
// UINT_MAX

//void *_MDebugNewPtr(long size) {
   //return __MAllocPtrWrapper(size + (2 * sizeof(unsigned int)));
//   return __SystemNewPtr(size+ (4 * sizeof(unsigned int)));
//}

void *_MDebugNewPtr(long size) {
   unsigned int *tmp;
   unsigned int *tag;
   
   tmp = __MAllocPtrWrapper(size + (4 * sizeof(unsigned int)));

   /* add in for the tag stuff */
   size += 4 * sizeof(unsigned int);

   if(tmp == 0) {
     //printf("ALLOCATION FAILED!\n");
      /* allocation failed! */
      return tmp;
   }
   
   tag = tmp;
   tmp += 4;
   
   *tag = size;
   tag++;

   *tag = _currentPool;

   _poolSizes[_currentPool] += size;

   if(_currentPool != 0) {
      _allocatedSize += size;
   }

   return (void *)tmp;
}

//void _MDebugFreePtr(void *ptr) {
//   __SystemFreePtr(ptr);
//}

void _MDebugFreePtr(void *ptr) {
   unsigned int *tmp = (unsigned int *)ptr;
   unsigned int *tag = tmp - 3;
   unsigned int size;
   unsigned int pool;

   pool = *tag;
   pool++; pool--; /* trick the stupid compiler */

   tag = tag - 1;
   size = *tag;
   size++; size--; /* trick the stupid compiler */

   _poolSizes[pool] -= size;
   if(pool != 0) {
      _allocatedSize -= size;
   }

   __SystemFreePtr(tmp - 4);
}

/* NOTE this handle stuff is never called, I can't make it allocate
      an extra short like I can for the ptr */
void **_MDebugNewHandle(long size) {
   void **tmp;
   short *tag;
   
   tmp = __SystemNewHandle(size + (1 * sizeof(short)));
   tag = (short *)__SystemLockHandle(tmp);
   
   *tag = size;
   
   __SystemUnlockHandle(tmp);
   
   _allocatedSize += size;
   return tmp;
}

/* NOTE this handle stuff is never called, I can't make it allocate
      an extra short like I can for the ptr */
void _MDebugFreeHandle(void **handle) {
   short *tag = (short *)__SystemLockHandle(handle);
   
   _allocatedSize -= *tag;

   __SystemUnlockHandle(handle);
   __SystemFreeHandle(handle);
}

/* NOTE this handle stuff is never called, I can't make it allocate
      an extra short like I can for the ptr */
void *_MDebugLockHandle(void **handle) {
   short *tag = (short *)__SystemLockHandle(handle);
   return (tag + 1);
}

/* NOTE this handle stuff is never called, I can't make it allocate
      an extra short like I can for the ptr */
void _MDebugUnlockHandle(void **handle) {
   __SystemUnlockHandle(handle);
}

long MGetTotalAllocation(void) {
   return _allocatedSize;
}

void MSetActivePool(int poolID) {
   _currentPool = poolID;
}

int MGetActivePool(void) {
   return _currentPool;
}

long MGetPoolAllocation(int poolID) {
   return _poolSizes[poolID];
}

void MMovePtrToPool(void *ptr, int poolID) {
   short *tmp = (short *)ptr;
   short *tag = tmp - 1;
   //short *poolLoc = tmp - 1;
   short size;
   short pool;

   pool = *tag;
   pool++; pool--; /* trick the stupid compiler */

   tag = tag - 1;
   size = *tag;
   size++; size--; /* trick the stupid compiler */

   _poolSizes[pool] -= size;
   _poolSizes[poolID] += size;

   if(pool == 0) {
      _allocatedSize += size;
   }

   if(poolID == 0) {
      _allocatedSize -= size;
   }
}

#endif /* DEBUGMEMORY */
