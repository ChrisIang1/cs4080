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
  chunk->linesCount = 0;
  chunk->linesCapacity = 0;
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
  FREE_ARRAY(LineRun, chunk->lines, chunk->linesCapacity);
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
//< write-chunk-line
  }

  chunk->code[chunk->count] = byte;
//> chunk-write-line
  if (chunk->linesCount > 0 &&
      chunk->lines[chunk->linesCount - 1].line == line) {
    chunk->lines[chunk->linesCount - 1].runLength++;
  } else {
    if (chunk->linesCapacity < chunk->linesCount + 1) {
      int oldLinesCapacity = chunk->linesCapacity;
      chunk->linesCapacity = GROW_CAPACITY(oldLinesCapacity);
      chunk->lines = GROW_ARRAY(LineRun, chunk->lines,
          oldLinesCapacity, chunk->linesCapacity);
    }

    chunk->lines[chunk->linesCount].line = line;
    chunk->lines[chunk->linesCount].runLength = 1;
    chunk->linesCount++;
  }
//< chunk-write-line
  chunk->count++;
}
//< write-chunk

int getLine(Chunk* chunk, int instruction) {
  int offset = 0;

  for (int i = 0; i < chunk->linesCount; i++) {
    offset += chunk->lines[i].runLength;
    if (instruction < offset) {
      return chunk->lines[i].line;
    }
  }

  return -1;
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
