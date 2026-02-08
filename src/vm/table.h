#ifndef cxxx_table_h
#define cxxx_table_h

#include "common.h"
#include "value.h"
#include "object.h"

namespace cxxx {

    struct Entry {
        ObjString* key;
        Value value;
    };

    class Table {
    public:
        Table();
        ~Table();

        bool set(ObjString* key, Value value);
        bool get(ObjString* key, Value* value);

        // debug
        friend void printTable(Table* table);
        bool deleteEntry(ObjString* key);
        ObjString* findString(const char* chars, int length, uint32_t hash);

        int count;
        int capacity;
        Entry* entries;

    private:
        Entry* findEntry(Entry* entries, int capacity, ObjString* key);
        void adjustCapacity(int capacity);
    };

}

#endif
