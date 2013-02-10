#include "./sysdefines.h"
#include "./memory.h"
#include "./lists.h"


/* the last element in the list will contain a pointer to 0 */
VoidLListPtr VLLNew(void) {
   VoidLListPtr tmp;
   
   tmp = MNewPtr(sizeof(VoidLList));
   tmp->element = 0;
   tmp->next = 0;
   
   return tmp;
}

void VLLAppend(VoidLListPtr list, void *element) {   
   /* append the element to the end of the list */
   while(list->next != 0) {
      list = list->next;
   }
   
   list->next = VLLNew();
   list->element = element;
}

void VLLPush(VoidLListPtr list, void *element) {
   /* push element on the head of the list */
   VoidLListPtr tmpList;
   
   tmpList = VLLNew();
   tmpList->next = list->next;
   tmpList->element = list->element;
   list->next = tmpList;
   list->element = element;
  
}

void VLLInsert(VoidLListPtr list, void *element, long index) {
   int curIndex = 0;
   VoidLListPtr tmpList;
   
   while((list->next != 0) && (curIndex < index)) {
      list = list->next;
      curIndex++;
   }
  
   tmpList = VLLNew();
   tmpList->next = list->next;
   list->next = tmpList;
   tmpList->element = list->element;
   list->element = element;
}

int VLLLength(VoidLListPtr list) {
   int len = 0;
   
   while(list->next != 0) {
      list = list->next;
      len++;
   }
   
   return len;
}


inline void *VLLIndex(VoidLListPtr list, long index) {
   int curIndex = 0;
   
   while((list->next != 0) && (curIndex < index)) {
      list = list->next;
      curIndex++;
   }
   
   return list->element;
}

static void _llswap(VoidLListPtr list, int one, int two) {
   void *f, *s;
   
   f = VLLIndex(list, one);
   s = VLLIndex(list, two);
   
   VLLSet(list, one, s);
   VLLSet(list, two, f);
}

static void _LListQSort(VoidLListPtr list, int (*cmp)(const void *, const void *), int left,
                         int right) {
   int i, last;
   
   if(left >= right) {
      return; /* done! */
   }
   
   _llswap(list, left, (left + right) / 2);
   last = left;
   for(i = left + 1 ; i <= right ; i++) {
      if((*cmp)(VLLIndex(list, i), VLLIndex(list, left)) < 0) {
         _llswap(list, ++last, i);
      }
   }
   
   _llswap(list, left, last);
   _LListQSort(list, cmp, left, last - 1);
   _LListQSort(list, cmp, last + 1, right);
}

static void _ArrayQSort(void **data, int (*cmp)(const void *, const void *), int left,
                         int right) {
   int i, last;
   void *tmp;
   
   if(left >= right) {
      return; /* done! */
   }
   
   tmp = data[left];
   data[left] = data[(left+right)/2];
   data[(left+right)/2] = tmp;
   
   last = left;
   for(i = left + 1 ; i <= right ; i++) {
      if((*cmp)(data[i], data[left]) < 0) {
         last++;
         tmp = data[last];
         data[last] = data[i];
         data[i] = tmp;
      }
   }
   
   tmp = data[left];
   data[left] = data[last];
   data[last] = tmp;
   _ArrayQSort(data, cmp, left, last - 1);
   _ArrayQSort(data, cmp, last + 1, right);
}

void VLLSort(VoidLListPtr list, int (*cmp)(const void *, const void *)) {
//   _ListQSort(list, cmp, 0, VLLLength(list) - 1);
   int i;
   int length = VLLLength(list);
   
   void **data = MNewPtr(sizeof(void *) * length);
   
   for(i = 0 ; i < length ; i++) {
      data[i] = VLLRemove(list, 0);
   }
   

   qsort(data, length, sizeof(void *), cmp);

   //_ArrayQSort(data, cmp, 0, length - 1);

   for(i = length - 1 ; i >= 0 ; i--) {
      VLLPush(list, data[i]);
   }

   MFreePtr(data);
}

void *VLLRemove(VoidLListPtr list, long index) {
   int curIndex = 0;
   void *tmp;
   VoidLListPtr trail;
   
   trail = 0;
   
   while((list->next != 0) && (curIndex < index)) {
      trail = list;
      list = list->next;
      curIndex++;
   }
   
   tmp = list->element;

   if(trail == 0) { /* index is 0 or list is empty */
      if(list->element != 0) { /* list is not empty */
         trail = list->next;
         list->element = trail->element;
         list->next = trail->next;
         MFreePtr(trail);
      }
   } else {
      trail->next = list->next;
      MFreePtr(list);
   }
   
   return tmp;
}

void VLLSet(VoidLListPtr list, long index, void *newVal) {
   /* if there is an index greater than the list size, do nothing */
   int curIndex = 0;
   
   if(index < VLLLength(list)) {
      while((list->next != 0) && (curIndex < index)) {
         list = list->next;
         curIndex++;
      }
   
      list->element = newVal;
   }
}

void VLLFree(VoidLListPtr list) {
   VoidLListPtr tmpList;
      
   while(list->next != 0) {
      tmpList = list->next;
      MFreePtr(list);
      list = tmpList;
   }
   
   /* free the final node */
   MFreePtr(list);
}


/* array based vlists */
#define kALGrowSize   32

static void _alGrow(VoidAListPtr list) {
   void **newArray;
   
   newArray = MNewPtr(sizeof(void *) * (list->allocatedSize + kALGrowSize));
   MMemMove(newArray, list->array, list->size * sizeof(void *));
   
   list->allocatedSize += kALGrowSize;
   
   if(list->array != 0) {
      MFreePtr(list->array);
   }
   
   list->array = newArray;
}

/* the last element in the list will contain a pointer to 0 */
VoidAListPtr VALNew(void) {
   VoidAListPtr tmp;
   
   tmp = MNewPtr(sizeof(VoidAList));
   tmp->size = 0;
   tmp->allocatedSize = 0;
   tmp->array = 0;
   
   _alGrow(tmp);
   
   return tmp;
}

void VALAppend(VoidAListPtr list, void *element) {   
   /* append the element to the end of the list */
   VALInsert(list, element, list->size);
}

void VALPush(VoidAListPtr list, void *element) {
   /* push element on the head of the list */
   VALInsert(list, element, 0);
}

void VALInsert(VoidAListPtr list, void *element, long index) {
   if(list->size == list->allocatedSize) {
      _alGrow(list);
   }

   MMemMove(list->array + index + 1, list->array + index, (list->size - index) * sizeof(void *));
   list->array[index] = element;
   list->size++;
}

//int VALLength(VoidAListPtr list) {
//   return list->size;
//}


//inline void *VALIndex(VoidAListPtr list, long index) {
//   if(index < list->size) {
//      return list->array[index];
//   } else {
//      return 0;
//   }
//}

void VALSort(VoidAListPtr list, int (*cmp)(const void *, const void *)) {      
   _ArrayQSort(list->array, cmp, 0, list->size - 1);
}

void *VALRemove(VoidAListPtr list, long index) {
   void *tmp = 0;
   
   if(index < list->size) {
      tmp = list->array[index];
      MMemMove(list->array + index, list->array + (index + 1), ((list->size - index) - 1) * sizeof(void *));
      list->size--;
   }
   return tmp;
}

void VALSet(VoidAListPtr list, long index, void *newVal) {
   /* if there is an index greater than the list size, do nothing */
   if(index < list->size) {
      list->array[index] = newVal;
   }
}

void VALFree(VoidAListPtr list) {
   MFreePtr(list->array);
   MFreePtr(list);
}

#ifdef ZERO
/* int list stuff */

/* the last element in the list will contain an bogus int */
IntListPtr ILLNew(void) {
   IntListPtr tmp;
   
   tmp = MNewPtr(sizeof(IntList));
   tmp->theInt = 0;
   tmp->next = 0;
   
   return tmp;
}


IntListPtr ILLClone(IntListPtr orig) {
   /* EFF this could be more efficient by doing it directly in the 
        underlying structure instead of with the API */
   int len = ILLength(orig);
   IntListPtr tmp = ILNew();
   int i;
   
   for(i = 0 ; i < len ; i++) {
      ILAppend(tmp, ILIndex(orig, i));
   }

   return tmp;
}

void ILLAppend(IntListPtr list, int element) {   
   /* append the element to the end of the list */
   while(list->next != 0) {
      list = list->next;
   }
   
   list->next = ILNew();
   list->theInt = element;
}

void ILLPush(IntListPtr list, int element) {
   /* push element on the head of the list */
   IntListPtr tmpList;
   
   tmpList = ILNew();
   tmpList->next = list->next;
   tmpList->theInt = list->theInt;
   list->next = tmpList;
   list->theInt = element;
}

void ILLInsert(IntListPtr list, int element, long index) {
   int curIndex = 0;
   IntListPtr tmpList;
   
   while((list->next != 0) && (curIndex < index)) {
      list = list->next;
      curIndex++;
   }
  
   tmpList = ILNew();
   tmpList->next = list->next;
   list->next = tmpList;
   tmpList->theInt = list->theInt;
   list->theInt = element;
}

int ILLLength(IntListPtr list) {
   int len = 0;
   
   while(list->next != 0) {
      list = list->next;
      len++;
   }
   
   return len;
}

inline int ILLIndex(IntListPtr list, long index) {
   // if the index is greater than the length of the list this returns
   //    zero.
   int curIndex = 0;
   
   while((list->next != 0) && (curIndex < index)) {
      list = list->next;
      curIndex++;
   }
   
   return list->theInt;
}

int ILLRemove(IntListPtr list, long index) {
   int curIndex = 0;
   int tmpInt;

   IntListPtr trail;
   
   trail = 0;
   
   while((list->next != 0) && (curIndex < index)) {
      trail = list;
      list = list->next;
      curIndex++;
   }
   
   tmpInt = list->theInt;

   if(trail == 0) { /* index is 0 or list is empty */
      if(list->next != 0) { /* list is not empty */
         trail = list->next;
         list->theInt = trail->theInt;
         list->next = trail->next;
         MFreePtr(trail);
      }
   } else {
      trail->next = list->next;
      MFreePtr(list);
   }
   
   return tmpInt;
}

void ILLSet(IntListPtr list, long index, int newVal) {
   /* if there is an index greater than the list size, do nothing */
   int curIndex = 0;
   
   if(index < ILLength(list)) {
      while((list->next != 0) && (curIndex < index)) {
         list = list->next;
         curIndex++;
      }
   
      list->theInt = newVal;
   }
}


void ILLFree(IntListPtr list) {
   IntListPtr tmpList;
      
   while(list->next != 0) {
      tmpList = list->next;
      MFreePtr(list);
      list = tmpList;
   }
   
   /*free the final node */
   MFreePtr(list);
}

int ILLFind(IntListPtr list, int element) {
   long index = 0;
   
   while((list->next != 0) && (list->theInt != element)) {
      list = list->next;
      index++;
   }

   if((list->next != 0) && (list->theInt == element)) {
      return index;
   } else {
      return -1;
   }
}
#endif /* 0 */

/* array based float lists */
#define kFLGrowSize   32

static void _flGrow(FloatListPtr list) {
   float *newArray;
   
   newArray = MNewPtr(sizeof(float) * (list->allocatedSize + kFLGrowSize));
   MMemMove(newArray, list->array, list->size * sizeof(float));
   
   list->allocatedSize += kFLGrowSize;
   
   if(list->array != 0) {
      MFreePtr(list->array);
   }
   
   list->array = newArray;
}

/* the last element in the list will contain a pointer to 0 */
FloatListPtr FLNew(void) {
   FloatListPtr tmp;
   
   tmp = MNewPtr(sizeof(FloatList));
   tmp->size = 0;
   tmp->allocatedSize = 0;
   tmp->array = 0;
   
   _flGrow(tmp);
   
   return tmp;
}

void FLAppend(FloatListPtr list, float element) {   
   /* append the element to the end of the list */
   FLInsert(list, element, list->size);
}

void FLPush(FloatListPtr list, float element) {
   /* push element on the head of the list */
   FLInsert(list, element, 0);
}

void FLInsert(FloatListPtr list, float element, long index) {
   if(list->size == list->allocatedSize) {
      _flGrow(list);
   }

   MMemMove(list->array + index + 1, list->array + index, (list->size - index) * sizeof(float));
   list->array[index] = element;
   list->size++;
}

int FLLength(FloatListPtr list) {
   return list->size;
}

inline float FLIndex(FloatListPtr list, long index) {
   if(index < list->size) {
      return list->array[index];
   } else {
      return 0;
   }
}

//void ALSort(VoidAListPtr list, int (*cmp)(const void *, const void *)) {
//   _ArrayQSort(list->array, cmp, 0, list->size - 1);
//}

float FLRemove(FloatListPtr list, long index) {
   float tmp = 0;
   
   if(index < list->size) {
      tmp = list->array[index];
      MMemMove(list->array + index, list->array + (index + 1), ((list->size - index) - 1) * sizeof(float));
      list->size--;
   }
   return tmp;
}

void FALSet(FloatListPtr list, long index, float newVal) {
   /* if there is an index greater than the list size, do nothing */
   if(index < list->size) {
      list->array[index] = newVal;
   }
}

void FLFree(FloatListPtr list) {
   MFreePtr(list->array);
   MFreePtr(list);
}
