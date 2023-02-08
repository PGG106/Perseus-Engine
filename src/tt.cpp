#include "tt.h"
#include "constants.h"
#include "move.h"

ttEntry::ttEntry(HashKey h, U16 b, Depth d, U8 f, Score s) {
    hashKey = h;
    bestMove = b;
    depth = d;
    flags = f;
    score = s;
}

ttEntry::ttEntry() {
    hashKey = 0;
    bestMove = 0;
    depth = 0;
    flags = hashINVALID;
    score = 0;
}

// Transposition table and evaluation hash table
ttEntry* tt;




