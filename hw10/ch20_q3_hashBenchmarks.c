#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TABLE_MAX_LOAD 0.75
#define INITIAL_CAPACITY 8
#define KEY_SIZE 32

typedef struct {
  char key[KEY_SIZE];
  int value;
  bool occupied;
  bool tombstone;
} Entry;

typedef struct {
  int count;
  int capacity;
  Entry* entries;
} BenchmarkTable;

static uint32_t hashString(const char* key) {
  uint32_t hash = 2166136261u;
  while (*key != '\0') {
    hash ^= (uint8_t)*key++;
    hash *= 16777619;
  }
  return hash;
}

static void initTable(BenchmarkTable* table) {
  table->count = 0;
  table->capacity = INITIAL_CAPACITY;
  table->entries = calloc((size_t)table->capacity, sizeof(Entry));
}

static void freeTable(BenchmarkTable* table) {
  free(table->entries);
}

static Entry* findEntry(Entry* entries, int capacity, const char* key) {
  uint32_t index = hashString(key) & (uint32_t)(capacity - 1);
  Entry* tombstone = NULL;

  for (;;) {
    Entry* entry = &entries[index];
    if (!entry->occupied) {
      if (!entry->tombstone) return tombstone != NULL ? tombstone : entry;
      if (tombstone == NULL) tombstone = entry;
    } else if (strcmp(entry->key, key) == 0) {
      return entry;
    }

    index = (index + 1) & (uint32_t)(capacity - 1);
  }
}

static void adjustCapacity(BenchmarkTable* table, int capacity) {
  Entry* entries = calloc((size_t)capacity, sizeof(Entry));
  table->count = 0;

  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    if (!entry->occupied) continue;

    Entry* dest = findEntry(entries, capacity, entry->key);
    *dest = *entry;
    table->count++;
  }

  free(table->entries);
  table->entries = entries;
  table->capacity = capacity;
}

static void tableSet(BenchmarkTable* table, const char* key, int value) {
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    adjustCapacity(table, table->capacity * 2);
  }

  Entry* entry = findEntry(table->entries, table->capacity, key);
  bool isNewKey = !entry->occupied;
  if (isNewKey) {
    strncpy(entry->key, key, KEY_SIZE - 1);
    entry->key[KEY_SIZE - 1] = '\0';
    entry->occupied = true;
    entry->tombstone = false;
    table->count++;
  }
  entry->value = value;
}

static bool tableGet(BenchmarkTable* table, const char* key, int* value) {
  Entry* entry = findEntry(table->entries, table->capacity, key);
  if (!entry->occupied) return false;
  *value = entry->value;
  return true;
}

static bool tableDelete(BenchmarkTable* table, const char* key) {
  Entry* entry = findEntry(table->entries, table->capacity, key);
  if (!entry->occupied) return false;
  entry->occupied = false;
  entry->tombstone = true;
  return true;
}

static void makeKey(char* buffer, size_t size, const char* prefix, int i) {
  snprintf(buffer, size, "%s_%08d", prefix, i);
}

static double elapsedSeconds(clock_t start, clock_t end) {
  return (double)(end - start) / CLOCKS_PER_SEC;
}

static void benchmarkSequentialInsertLookup(int count) {
  BenchmarkTable table;
  initTable(&table);
  char key[KEY_SIZE];
  int value;

  clock_t start = clock();
  for (int i = 0; i < count; i++) {
    makeKey(key, sizeof(key), "seq", i);
    tableSet(&table, key, i);
  }
  clock_t insertEnd = clock();

  for (int i = 0; i < count; i++) {
    makeKey(key, sizeof(key), "seq", i);
    tableGet(&table, key, &value);
  }
  clock_t lookupEnd = clock();

  printf("Sequential insert/lookup: insert=%.4fs lookup=%.4fs\n",
         elapsedSeconds(start, insertEnd),
         elapsedSeconds(insertEnd, lookupEnd));
  freeTable(&table);
}

static void benchmarkHotLookups(int distinctKeys, int probesPerKey) {
  BenchmarkTable table;
  initTable(&table);
  char key[KEY_SIZE];
  int value;

  for (int i = 0; i < distinctKeys; i++) {
    makeKey(key, sizeof(key), "hot", i);
    tableSet(&table, key, i);
  }

  clock_t start = clock();
  for (int repeat = 0; repeat < probesPerKey; repeat++) {
    for (int i = 0; i < distinctKeys; i++) {
      makeKey(key, sizeof(key), "hot", i);
      tableGet(&table, key, &value);
    }
  }
  clock_t end = clock();

  printf("Hot lookup workload: %.4fs\n", elapsedSeconds(start, end));
  freeTable(&table);
}

static void benchmarkDeleteHeavy(int count) {
  BenchmarkTable table;
  initTable(&table);
  char key[KEY_SIZE];
  int value;

  for (int i = 0; i < count; i++) {
    makeKey(key, sizeof(key), "del", i);
    tableSet(&table, key, i);
  }

  clock_t start = clock();
  for (int i = 0; i < count; i += 2) {
    makeKey(key, sizeof(key), "del", i);
    tableDelete(&table, key);
  }
  for (int i = 0; i < count; i += 2) {
    makeKey(key, sizeof(key), "del", i);
    tableSet(&table, key, i * 2);
  }
  for (int i = 0; i < count; i++) {
    makeKey(key, sizeof(key), "del", i);
    tableGet(&table, key, &value);
  }
  clock_t end = clock();

  printf("Delete-heavy workload: %.4fs\n", elapsedSeconds(start, end));
  freeTable(&table);
}

static void benchmarkMixed(int count) {
  BenchmarkTable table;
  initTable(&table);
  char key[KEY_SIZE];
  int value;

  clock_t start = clock();
  for (int i = 0; i < count; i++) {
    makeKey(key, sizeof(key), "mix", i);
    tableSet(&table, key, i);
    if ((i % 3) == 0) tableGet(&table, key, &value);
    if ((i % 5) == 0) tableDelete(&table, key);
  }
  clock_t end = clock();

  printf("Mixed insert/get/delete workload: %.4fs\n",
         elapsedSeconds(start, end));
  freeTable(&table);
}

int main(void) {
  benchmarkSequentialInsertLookup(100000);
  benchmarkHotLookups(1000, 500);
  benchmarkDeleteHeavy(100000);
  benchmarkMixed(100000);
  return 0;
}
