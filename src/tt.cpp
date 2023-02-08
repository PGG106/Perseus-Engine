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

void initTT() {
	if (tt != nullptr) delete[] tt;
	tt = new ttEntry[ttEntryCount];
}

ttEntry* probeTT(HashKey key)
{
	uint64_t index = key % (ttEntryCount);
	ttEntry* entry = tt[index];
	if (entry->hashKey == key) {
		entry->flags &= ~hashOLD;
		return entry;
	}
	return nullptr;

}

oid writeTT(HashKey key, Score score, Score staticEval, Depth depth, U8 flags, Move move, Ply ply) {
	score -= ply * (score < -mateValue);
	score += ply * (score > mateValue);
	uint64_t index = key % (ttEntryCount);
	ttEntry* entry = tt[index];
	// Iterate through the entries.
	if (depth >= entry->depth) 
	{
		entry->score = score;
		entry->eval = staticEval;
		entry->depth = depth;
		entry->flags = flags;
		entry->bestMove = move;
		return;
	}
	return;
}

// Transposition table and evaluation hash table
ttEntry* tt;




