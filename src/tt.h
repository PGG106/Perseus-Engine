#pragma once
#include "types.h"
#include "constants.h"

// Table sizes (TODO: Make these configurable at runtime)
constexpr U64 ttEntryCount = 1048576;                      // 2^20
constexpr U64 ttBucketSize = 4;                            // 2^2
constexpr U64 ttBucketCount = ttEntryCount / ttBucketSize; // 2^18
constexpr U64 evalHashSize = 1048576;                      // 2^20

// The enum of hashFlags
enum HashFlag {
	hashALPHA = 1,
	hashBETA = 2,
	hashEXACT = 4,
	hashINVALID = 8,
	hashOLD = 16,
	hashSINGULAR = 32
};

struct ttEntry {
    ttEntry(HashKey h, U16 b, Depth d, U8 f, Score s);
    ttEntry();
    HashKey hashKey;         // 8
    PackedMove bestMove;     // 2
    Depth depth;             // 1
    U8 flags = hashINVALID;  // 1
    Score score = -infinity; // 2
    Score eval = -infinity;  // 2
};


// Transposition table and evaluation hash table
extern ttEntry* tt;
extern evalHashEntry* evalHash;

/**
 * @brief The initTT function initializes the transposition table and evaluation cache.
 * @note The size is not configurable at runtime yet.
 */
//void initTT();

/**
 * @brief The probeTT function probes the transposition table. It checks if the entry exists and if it is valid.
 * @param key The key.
 * @return The entry, or nullptr if it does not exist.
 */
//ttEntry *probeTT(HashKey key);

/**
 * @brief The writeTT function writes the entry to the transposition table.
 * @param key The key.
 * @param score The score.
 * @param depth The depth.
 * @param flags The flags.
 * @param move The move.
 * @param ply The ply.
 */
//void writeTT(HashKey key, Score score, Score staticEval, Depth depth, U8 flags, Move move, Ply ply);
