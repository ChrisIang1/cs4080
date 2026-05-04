//> Chunks of Bytecode memory-c
#include <stdlib.h>
#include <string.h>

//> Garbage Collection memory-include-compiler
#include "compiler.h"
//< Garbage Collection memory-include-compiler
#include "memory.h"
//> Strings memory-include-vm
#include "vm.h"
//< Strings memory-include-vm
//> Garbage Collection debug-log-includes

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif
//< Garbage Collection debug-log-includes
//> Garbage Collection heap-grow-factor

#define GC_HEAP_GROW_FACTOR 2
//< Garbage Collection heap-grow-factor

static uint8_t* copyStart = NULL;
static uint8_t* copyEnd = NULL;
static uint8_t* copyAlloc = NULL;

static size_t alignSize(size_t size) {
  size_t alignment = sizeof(void*);
  return (size + alignment - 1) & ~(alignment - 1);
}

static size_t objectSize(Obj* object) {
  switch (object->type) {
    case OBJ_BOUND_METHOD: return sizeof(ObjBoundMethod);
    case OBJ_CLASS: return sizeof(ObjClass);
    case OBJ_CLOSURE: return sizeof(ObjClosure);
    case OBJ_FUNCTION: return sizeof(ObjFunction);
    case OBJ_INSTANCE: return sizeof(ObjInstance);
    case OBJ_NATIVE: return sizeof(ObjNative);
    case OBJ_STRING: return sizeof(ObjString);
    case OBJ_UPVALUE: return sizeof(ObjUpvalue);
  }

  return sizeof(Obj);
}

static Obj* forwardObject(Obj* object) {
  if (object == NULL) return NULL;
  if (object->isMarked) return object->next;

  size_t size = objectSize(object);
  size_t alignedSize = alignSize(size);
  if (copyAlloc + alignedSize > copyEnd) exit(1);

  Obj* moved = (Obj*)copyAlloc;
  memcpy(moved, object, size);
  copyAlloc += alignedSize;

  if (object->type == OBJ_UPVALUE) {
    ObjUpvalue* oldUpvalue = (ObjUpvalue*)object;
    ObjUpvalue* newUpvalue = (ObjUpvalue*)moved;
    if (oldUpvalue->location == &oldUpvalue->closed) {
      newUpvalue->location = &newUpvalue->closed;
    }
  }

  object->isMarked = true;
  object->next = moved;

#ifdef DEBUG_LOG_GC
  printf("%p move ", (void*)object);
  printValue(OBJ_VAL(moved));
  printf("\n");
#endif

  return moved;
}

static void freeObject(Obj* object) {
#ifdef DEBUG_LOG_GC
  printf("%p free type %d\n", (void*)object, object->type);
#endif

  switch (object->type) {
    case OBJ_BOUND_METHOD:
      break;
    case OBJ_CLASS: {
      ObjClass* klass = (ObjClass*)object;
      freeTable(&klass->methods);
      break;
    }
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*)object;
      FREE_ARRAY(ObjUpvalue*, closure->upvalues,
                 closure->upvalueCount);
      break;
    }
    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)object;
      freeChunk(&function->chunk);
      break;
    }
    case OBJ_INSTANCE: {
      ObjInstance* instance = (ObjInstance*)object;
      freeTable(&instance->fields);
      break;
    }
    case OBJ_NATIVE:
      break;
    case OBJ_STRING: {
      ObjString* string = (ObjString*)object;
      FREE_ARRAY(char, string->chars, string->length + 1);
      break;
    }
    case OBJ_UPVALUE:
      break;
  }
}

static void growHeap(size_t minSize);

void* allocateObjectMemory(size_t size) {
  size_t alignedSize = alignSize(size);

#ifdef DEBUG_STRESS_GC
  collectGarbage();
#endif

  if (vm.bytesAllocated + alignedSize > vm.nextGC ||
      vm.allocPtr + alignedSize > vm.fromSpace + vm.heapCapacity) {
    collectGarbage();
  }

  if (vm.allocPtr + alignedSize > vm.fromSpace + vm.heapCapacity) {
    growHeap(alignedSize);
  }

  void* result = vm.allocPtr;
  vm.allocPtr += alignedSize;
  vm.bytesAllocated += alignedSize;
  return result;
}

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
//> Garbage Collection updated-bytes-allocated
  vm.bytesAllocated += newSize - oldSize;
//< Garbage Collection updated-bytes-allocated
//> Garbage Collection call-collect
  if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
    collectGarbage();
#endif
//> collect-on-next

    if (vm.bytesAllocated > vm.nextGC) {
      collectGarbage();
    }
//< collect-on-next
  }

//< Garbage Collection call-collect
  if (newSize == 0) {
    free(pointer);
    return NULL;
  }

  void* result = realloc(pointer, newSize);
//> out-of-memory
  if (result == NULL) exit(1);
//< out-of-memory
  return result;
}

//> Garbage Collection mark-object
void markObject(Obj** object) {
  if (object == NULL || *object == NULL) return;
  *object = forwardObject(*object);
}
//< Garbage Collection mark-object
//> Garbage Collection mark-value
void markValue(Value* value) {
  if (value != NULL && IS_OBJ(*value)) {
    Obj* object = AS_OBJ(*value);
    *value = OBJ_VAL(forwardObject(object));
  }
}
//< Garbage Collection mark-value
//> Garbage Collection mark-array
static void markArray(ValueArray* array) {
  for (int i = 0; i < array->count; i++) {
    markValue(&array->values[i]);
  }
}
//< Garbage Collection mark-array
//> Garbage Collection blacken-object
static void blackenObject(Obj* object) {
//> log-blacken-object
#ifdef DEBUG_LOG_GC
  printf("%p blacken ", (void*)object);
  printValue(OBJ_VAL(object));
  printf("\n");
#endif

//< log-blacken-object
  switch (object->type) {
//> Methods and Initializers blacken-bound-method
    case OBJ_BOUND_METHOD: {
      ObjBoundMethod* bound = (ObjBoundMethod*)object;
      markValue(&bound->receiver);
      markObject((Obj**)&bound->method);
      break;
    }
//< Methods and Initializers blacken-bound-method
//> Classes and Instances blacken-class
    case OBJ_CLASS: {
      ObjClass* klass = (ObjClass*)object;
      markObject((Obj**)&klass->name);
//> Methods and Initializers mark-methods
      markTable(&klass->methods);
//< Methods and Initializers mark-methods
      break;
    }
//< Classes and Instances blacken-class
//> blacken-closure
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*)object;
      markObject((Obj**)&closure->function);
      for (int i = 0; i < closure->upvalueCount; i++) {
        markObject((Obj**)&closure->upvalues[i]);
      }
      break;
    }
//< blacken-closure
//> blacken-function
    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)object;
      markObject((Obj**)&function->name);
      markArray(&function->chunk.constants);
      break;
    }
//< blacken-function
//> Classes and Instances blacken-instance
    case OBJ_INSTANCE: {
      ObjInstance* instance = (ObjInstance*)object;
      markObject((Obj**)&instance->klass);
      markTable(&instance->fields);
      break;
    }
//< Classes and Instances blacken-instance
//> blacken-upvalue
    case OBJ_UPVALUE: {
      ObjUpvalue* upvalue = (ObjUpvalue*)object;
      if (upvalue->location == &upvalue->closed) {
        markValue(&upvalue->closed);
        upvalue->location = &upvalue->closed;
      }
      upvalue->next = (ObjUpvalue*)forwardObject((Obj*)upvalue->next);
      break;
    }
//< blacken-upvalue
    case OBJ_NATIVE:
    case OBJ_STRING:
      break;
  }
}
//< Garbage Collection blacken-object
//> Garbage Collection mark-roots
static void markRoots() {
  for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
    markValue(slot);
  }
//> mark-closures

  for (int i = 0; i < vm.frameCount; i++) {
    markObject((Obj**)&vm.frames[i].closure);
  }
//< mark-closures
//> mark-open-upvalues

  markObject((Obj**)&vm.openUpvalues);
//< mark-open-upvalues
//> mark-globals

  markTable(&vm.globals);
//< mark-globals
//> call-mark-compiler-roots
  markCompilerRoots();
//< call-mark-compiler-roots
//> Methods and Initializers mark-init-string
  markObject((Obj**)&vm.initString);
//< Methods and Initializers mark-init-string
}
//< Garbage Collection mark-roots
//> Garbage Collection trace-references
static void traceReferences() {
  uint8_t* scan = copyStart;
  while (scan < copyAlloc) {
    Obj* object = (Obj*)scan;
    blackenObject(object);
    scan += alignSize(objectSize(object));
  }
}
//< Garbage Collection trace-references

static void sweepDeadObjects(uint8_t* oldSpace, uint8_t* oldAlloc) {
  uint8_t* scan = oldSpace;
  while (scan < oldAlloc) {
    Obj* object = (Obj*)scan;
    size_t size = alignSize(objectSize(object));
    if (!object->isMarked) {
      vm.bytesAllocated -= size;
      freeObject(object);
    }
    scan += size;
  }
}

static void relocateLiveObjects(uint8_t* destination,
                                size_t destinationCapacity) {
  copyStart = destination;
  copyEnd = destination + destinationCapacity;
  copyAlloc = copyStart;

  markRoots();
  traceReferences();
  tableRemoveWhite(&vm.strings);
}

static void growHeap(size_t minSize) {
  size_t used = (size_t)(vm.allocPtr - vm.fromSpace);
  size_t newCapacity = vm.heapCapacity;
  while (used + minSize > newCapacity) {
    newCapacity *= GC_HEAP_GROW_FACTOR;
  }

  uint8_t* newFromSpace = (uint8_t*)malloc(newCapacity);
  uint8_t* newToSpace = (uint8_t*)malloc(newCapacity);
  if (newFromSpace == NULL || newToSpace == NULL) exit(1);

  uint8_t* oldFromSpace = vm.fromSpace;
  uint8_t* oldToSpace = vm.toSpace;

  relocateLiveObjects(newFromSpace, newCapacity);

  free(oldFromSpace);
  free(oldToSpace);

  vm.fromSpace = newFromSpace;
  vm.toSpace = newToSpace;
  vm.allocPtr = copyAlloc;
  vm.heapCapacity = newCapacity;
  vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;
  if (vm.nextGC < vm.heapCapacity) {
    vm.nextGC = vm.heapCapacity;
  }
}

//> Garbage Collection collect-garbage
void collectGarbage() {
//> log-before-collect
#ifdef DEBUG_LOG_GC
  printf("-- gc begin\n");
//> log-before-size
  size_t before = vm.bytesAllocated;
//< log-before-size
#endif
//< log-before-collect
  uint8_t* oldFromSpace = vm.fromSpace;
  uint8_t* oldAlloc = vm.allocPtr;

//> call-mark-roots
  relocateLiveObjects(vm.toSpace, vm.heapCapacity);
//< call-mark-roots
//> sweep-strings
  sweepDeadObjects(oldFromSpace, oldAlloc);
//< sweep-strings
//> call-sweep
  vm.fromSpace = vm.toSpace;
  vm.toSpace = oldFromSpace;
  vm.allocPtr = copyAlloc;
//< call-sweep
//> update-next-gc

  vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;
  if (vm.nextGC < vm.heapCapacity) {
    vm.nextGC = vm.heapCapacity;
  }
//< update-next-gc
//> log-after-collect

#ifdef DEBUG_LOG_GC
  printf("-- gc end\n");
//> log-collected-amount
  printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
         before - vm.bytesAllocated, before, vm.bytesAllocated,
         vm.nextGC);
//< log-collected-amount
#endif
//< log-after-collect
}
//< Garbage Collection collect-garbage
//> Strings free-objects
void freeObjects() {
  if (vm.fromSpace != NULL) {
    uint8_t* scan = vm.fromSpace;
    while (scan < vm.allocPtr) {
      Obj* object = (Obj*)scan;
      size_t size = alignSize(objectSize(object));
      freeObject(object);
      scan += size;
    }
  }

  free(vm.fromSpace);
  free(vm.toSpace);
  vm.fromSpace = NULL;
  vm.toSpace = NULL;
  vm.allocPtr = NULL;
  vm.heapCapacity = 0;
  vm.objects = NULL;
}
//< Strings free-objects
