#include "HashTable.h"

HashTable *HashTableNew(int size) {
   HashTable* table = NULL;
   int i = 0;

   table = MNewPtr(sizeof(HashTable));

   table->size = size;
   table->array = MNewPtr(sizeof(VoidListPtr) * size);

   for(i = 0; i < size; i++) {
      table->array[i] = VLNew();
      DebugError(table->array[i] == NULL, "Failed allocate in HashTableNew");
   }

   return table;
}


void HashTableInsert(HashTable *table, int index, void* element) {
   DebugError(index < 0, "Invalid index in HashTableInsert");

   VLAppend(table->array[index % (table->size)], element);
}

void* HashTableFind(HashTable *table, int index, 
		    int (*cmp)(const void *, const int)) {
   int i, found, foundIndex;
   VoidListPtr list;

   DebugError(index < 0, "Invalid index in HashTableFind");

   list = table->array[index % (table->size)];

   found = 0;
   foundIndex = -1;
   for(i = 0 ; i < VLLength(list) && !found ; i++) {
      if((*cmp)(VLIndex(list, i), index) >= 0) {
         found = 1;
         foundIndex = i;
      }
   }

   if(found == 1) {
      return VLIndex(list, foundIndex);
   } else {
      return 0;
   }
}

void HashTableFree(HashTable *table) {
   int i = 0;
  
   for(i = 0; i < table->size; i++) {
      VLFree(table->array[i]);
   }

   MFreePtr(table);
}

void *HashTableRemove(HashTable *table, int index,
		      int (*cmp)(const void *, const int)) {
   int i, found, foundIndex;
   VoidListPtr list;

   DebugError(index < 0, "Invalid index in HashTableRemove");

   list = table->array[index % (table->size)];

   found = 0;
   foundIndex = -1;
   for(i = 0 ; i < VLLength(list) && !found ; i++) {
      if((*cmp)(VLIndex(list, i), index) >= 0) {
         found = 1;
         foundIndex = i;
      }
   }

   if(found == 1) {
      return VLRemove(list, foundIndex);
   } else {
      return 0;
   }
}
      
      
      
      
  
