//> Chunks of Bytecode chunk-c
#include <stdlib.h>

#include "chunk.h"
//> chunk-c-include-memory
#include "memory.h"
//< chunk-c-include-memory
//> Garbage Collection chunk-include-vm
#include "vm.h"
//< Garbage Collection chunk-include-vm

void initChunk(Chunk* chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
//> chunk-null-lines
  chunk->lines = NULL;
//< chunk-null-lines
//> chunk-init-constant-array
  initValueArray(&chunk->constants);
//< chunk-init-constant-array
}
//> free-chunk
void freeChunk(Chunk* chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
//> chunk-free-lines
  FREE_ARRAY(int, chunk->lines, chunk->capacity);
//< chunk-free-lines
//> chunk-free-constants
  freeValueArray(&chunk->constants);
//< chunk-free-constants
  initChunk(chunk);
}
//< free-chunk
/* Chunks of Bytecode write-chunk < Chunks of Bytecode write-chunk-with-line
void writeChunk(Chunk* chunk, uint8_t byte) {
*/
//> write-chunk
//> write-chunk-with-line
void writeChunk(Chunk* chunk, uint8_t byte, int line) {
//< write-chunk-with-line
  if (chunk->capacity < chunk->count + 1) {
    int oldCapacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(oldCapacity);
    chunk->code = GROW_ARRAY(uint8_t, chunk->code,
        oldCapacity, chunk->capacity);
//> write-chunk-line
    chunk->lines = GROW_ARRAY(int, chunk->lines,
        oldCapacity, chunk->capacity);
//< write-chunk-line
  }

  chunk->code[chunk->count] = byte;
//> chunk-write-line
  chunk->lines[chunk->count] = line;
//< chunk-write-line
  chunk->count++;
}
//< write-chunk

void writeConstant(Chunk* chunk, Value value, int line) {
  int constant = addConstant(chunk, value);

  if (constant <= UINT8_MAX) {
    writeChunk(chunk, OP_CONSTANT, line);
    writeChunk(chunk, (uint8_t)constant, line);
    return;
  }

  writeChunk(chunk, OP_CONSTANT_LONG, line);
  writeChunk(chunk, (constant >> 16) & 0xff, line);
  writeChunk(chunk, (constant >> 8) & 0xff, line);
  writeChunk(chunk, constant & 0xff, line);
}

//> add-constant
int addConstant(Chunk* chunk, Value value) {
//> Garbage Collection add-constant-push
  push(value);
//< Garbage Collection add-constant-push
  writeValueArray(&chunk->constants, value);
//> Garbage Collection add-constant-pop
  pop();
//< Garbage Collection add-constant-pop
  return chunk->constants.count - 1;
}
//< add-constant
