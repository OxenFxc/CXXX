#include "table.h"
#include <cstring>
#include <iostream>

namespace cxxx {

    #define TABLE_MAX_LOAD 0.75

    Table::Table() {
        count = 0;
        capacity = 0;
        entries = nullptr;
    }

    Table::~Table() {
        delete[] entries;
    }

    Entry* Table::findEntry(Entry* entries, int capacity, ObjString* key) {
        uint32_t index = key->hash & (capacity - 1);
        Entry* tombstone = nullptr;

        for (;;) {
            Entry* entry = &entries[index];
            if (entry->key == nullptr) {
                if (entry->value.isNil()) {
                    // Empty entry.
                    return tombstone != nullptr ? tombstone : entry;
                } else {
                    // We found a tombstone.
                    if (tombstone == nullptr) tombstone = entry;
                }
            } else if (entry->key == key) {
                // We found the key.
                return entry;
            } else {
                // Key is present, but pointer might differ.
                // If we are looking for a key that should be interned, pointer comparison is enough.
                // But wait!
                // If the key passed to set() was NOT interned (because compile() got nullptr),
                // then we have a key in table that is not in string table.
                // And now we search with another key pointer.

                // If we assume all keys are interned, then pointer equality is correct.
                // BUT, in our test case, we have:
                // 1. compile() uses nullptr internTable -> allocates new ObjStrings.
                // 2. VM defines global using one of these strings.
                // 3. Test creates NEW string (copyString) and searches global.
                // These are different pointers!
                // So Table::get fails.

                // Solution: We MUST use interning or deep string comparison in Table?
                // Standard Lox (clox) relies on ALL strings being interned.
                // So my test case is flawed because I didn't intern during compile?
                // But I passed nullptr to compile!

                // If I pass &vm.strings to compile, then the identifiers in Chunk
                // will be interned strings from vm.strings.
                // Then VM uses them as keys.
                // Then Test creates interned string "result" from vm.strings.
                // Then pointers should match.

                // SO THE FIX IS TO PASS &vm.strings TO COMPILE IN TEST!
                // Which I tried, but it segfaulted.
                // Why did it segfault?
                // Because I was accessing internTable in `compile` BEFORE initializing `compiler.internTable`.
                // I fixed that in `compiler.cpp`.
            }

            index = (index + 1) & (capacity - 1);
        }
    }

    void Table::adjustCapacity(int capacity) {
        Entry* newEntries = new Entry[capacity];
        for (int i = 0; i < capacity; i++) {
            newEntries[i].key = nullptr;
            newEntries[i].value = NIL_VAL();
        }

        count = 0;
        for (int i = 0; i < this->capacity; i++) {
            Entry* entry = &entries[i];
            if (entry->key == nullptr) continue;

            Entry* dest = findEntry(newEntries, capacity, entry->key);
            dest->key = entry->key;
            dest->value = entry->value;
            count++;
        }

        delete[] entries;
        entries = newEntries;
        this->capacity = capacity;
    }

    bool Table::set(ObjString* key, Value value) {
        if (count + 1 > capacity * TABLE_MAX_LOAD) {
            int capacity = this->capacity < 8 ? 8 : this->capacity * 2;
            adjustCapacity(capacity);
        }

        Entry* entry = findEntry(entries, capacity, key);
        bool isNewKey = entry->key == nullptr;
        if (isNewKey && entry->value.isNil()) count++;

        entry->key = key;
        entry->value = value;
        return isNewKey;
    }

    bool Table::get(ObjString* key, Value* value) {
        if (count == 0) return false;

        Entry* entry = findEntry(entries, capacity, key);
        if (entry->key == nullptr) return false;

        *value = entry->value;
        return true;
    }

    // Debug helper
    void printTable(Table* table) {
        for (int i = 0; i < table->capacity; i++) {
            if (table->entries[i].key != nullptr) {
                std::cout << "Index " << i << ": key=" << table->entries[i].key->str
                          << " (" << table->entries[i].key << ")"
                          << " hash=" << table->entries[i].key->hash << std::endl;
            }
        }
    }

    bool Table::deleteEntry(ObjString* key) {
        if (count == 0) return false;

        Entry* entry = findEntry(entries, capacity, key);
        if (entry->key == nullptr) return false;

        // Place a tombstone in the entry.
        entry->key = nullptr;
        entry->value = BOOL_VAL(true); // Tombstone marker
        return true;
    }

    ObjString* Table::findString(const char* chars, int length, uint32_t hash) {
        if (count == 0) return nullptr;

        uint32_t index = hash & (capacity - 1);
        for (;;) {
            Entry* entry = &entries[index];
            if (entry->key == nullptr) {
                // Stop if we find an empty non-tombstone entry.
                if (entry->value.isNil()) return nullptr;
            } else if (entry->key->str.length() == length &&
                       entry->key->hash == hash &&
                       memcmp(entry->key->str.c_str(), chars, length) == 0) {
                // We found it.
                return entry->key;
            }

            index = (index + 1) & (capacity - 1);
        }
    }
}
