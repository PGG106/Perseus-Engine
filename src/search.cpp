#include "Game.h"
#include "tables.h"
#include "constants.h"
#include "history.h"
#include "zobrist.h"
#include "evaluation.h"
#include "uci.h"
#include "tt.h"
#include <iostream>
#include <cstdlib>

static inline Score mateIn(U16 ply) { return ((mateScore) - (ply)); }
static inline Score matedIn(U16 ply) { return ((-mateScore) + (ply)); }

Score Game::search(Score alpha, Score beta, Depth depth) {
	
    // Comms
    if (stopped) return 0;
    if ((nodes & comfrequency) == 0) {
        communicate();
        if (stopped) return 0;
    }
    assert(pos.hashKey == pos.generateHashKey());
    // Ply overflow
    if (ply > maxPly) return evaluate();
    
    pvLen[ply] = ply;

    const bool RootNode = ply == 0;

    // Draws - mate distance
    if (!RootNode) {

        if (isRepetition()) return 0; // contempt()
        if (pos.fiftyMove >= 100) return 0; // contempt()

        alpha = std::max(alpha, matedIn(ply));
        beta = std::min(beta, mateIn(ply + 1));
        if (alpha >= beta) return alpha;
    }
    
    // Quiescence drop
    if (depth <= 0) return quiescence(alpha, beta);

    seldepth = std::max(seldepth, (U16)(ply + 1));
    bool inCheck = pos.inCheck();
    Position save = pos;

    // Search
    Score origAlpha = alpha;
    Score bestScore = -infinity;
    Move bestMove = 0;
    U16 moveSearched = 0;
    
    pvLen[ply] = ply;

    MoveList moveList;
    generateMoves(moveList); // Already sorted, except for ttMove


    // Iterate through moves
    for (int i = 0; i < moveList.count; i++) {
        Move currMove = onlyMove(moveList.moves[i]);
        S32 currMoveScore = getScore(moveList.moves[i]) - 16384;
        if (makeMove(currMove)) {
            ++moveSearched;
            ++nodes;
            if (RootNode && depth >= LOGROOTMOVEDEPTH) {
                std::cout << "info depth " << std::dec << (int)currSearch << " currmove " << getMoveString(currMove) << " currmovenumber " << moveSearched << " currmovescore " << currMoveScore << std::endl;
            }
            Score score = -search(-beta, -alpha, depth - 1);
            restore(save);

            if (score > bestScore) {
                bestScore = score;
                bestMove = currMove;
                pvTable[ply][ply] = currMove;
                if (ply + 1 < maxPly) {
                    memcpy(&(pvTable[ply][ply + 1]), &(pvTable[ply + 1][ply + 1]), sizeof(Move)* (static_cast<unsigned long long>(pvLen[ply + 1]) - ply - 1));
                    pvLen[ply] = pvLen[ply + 1];
                }
            }

            if (score > alpha) {
                alpha = score;
                if (score >= beta) {
                    updateHistoryBonus(&historyTable[pos.side][moveSource(currMove)][moveTarget(currMove)], depth, true);
                    break;
                }
            }
        }
        else restore(save);
    }

    //// Check for checkmate / stalemate
    if (moveSearched == 0) return inCheck ? matedIn(ply) : 0;

    U8 ttStoreFlag = hashINVALID;
    if (bestScore >= beta) ttStoreFlag = hashBETA;
    else {
        if (alpha != origAlpha) ttStoreFlag = hashEXACT;
        else ttStoreFlag = hashALPHA;
    }
    
    return bestScore;

}

Score Game::quiescence(Score alpha, Score beta){

    bool inCheck = pos.inCheck();

    Score standPat = evaluate();

    if (ply >= maxPly - 1) return standPat;

    alpha = std::max(alpha, standPat);

    if (standPat >= beta) return standPat;

    // Generate moves
    MoveList moveList;
    inCheck ? generateMoves(moveList) : generateCaptures(moveList);

    if (inCheck) if (isRepetition()) return 0;

    Position save = pos;

    for (int i = 0; i < moveList.count; i++){
        Move move = onlyMove(moveList.moves[i]);
        if (makeMove(move)) {
            Score score = -quiescence(-beta, -alpha);
            restore(save);
            if (score > alpha) {
                alpha = score;
                if (alpha >= beta) return beta;
            }
        }
        else restore(save);
    }

    return alpha;
}

void Game::startSearch(bool halveTT = true){
    nodes = 0ULL;
    stopped = false;
    ply = 0;
    seldepth = 0;

    std::cin.clear();
    //std::cout << std::endl;

    pos.lastMove = 0;
    lastScore = 0;

    startTime = getTime64();

    switch (searchMode){
        case 0: // Infinite search
            depth = maxPly-1; // Avoid overflow
            moveTime = 0xffffffffffffffffULL;
            break;
        case 1: // Fixed depth
            moveTime = 0xffffffffffffffffULL;
            break;
#define UCILATENCYMS 10
        case 2: // Time control
             moveTime = getTime64();
            if (pos.side == WHITE) moveTime += (U64)(wtime / 20) + winc/2 - UCILATENCYMS;
            else moveTime += (U64)(btime / 20) + binc/2 - UCILATENCYMS;
            depth = maxPly - 1;
            break;
        case 3:
            depth = maxPly - 1;
            break;
    }

    // Clear pv len and pv table
    memset(pvLen, 0, sizeof(pvLen));
    memset(pvTable, 0, sizeof(pvTable));
    
    // Always compute a depth 1 search
    rootDelta = 2 * infinity;
    currSearch = 1;

    Score score = search(-infinity, infinity, 1);
    bestMove = pvTable[0][0];
    
    std::cout << "info score depth 1 cp " << score << " nodes " << nodes << " moves " ;
    printMove(bestMove);
    std::cout << std::endl;

    if (depth < 0) depth = maxPly - 1;
    if (stopped) goto bmove;

    for (currSearch = 2; (currSearch <= depth) && currSearch >= 2 && !stopped; currSearch++) {
        seldepth = 0; // Reset seldepth
        ply = 0;
        S64 locNodes = nodes; // Save nodes for nps calculation
        U64 timer1 = getTime64(); // Save time for nps calculation
        score = search(-infinity, infinity, currSearch); // Search at depth currSearch
        if (stopped)
            goto bmove;
        bestMove = pvTable[0][0];
        U64 timer2 = getTime64();
        if (score < -mateValue && score > -mateScore)
            std::cout << std::dec << "info score mate " << -(mateScore + score + 2) / 2 << " depth " << (int)currSearch << " seldepth " << (int)seldepth << " nodes " << nodes << " pv ";
        else if (score > mateValue && score < mateScore)
            std::cout << std::dec << "info score mate " << (mateScore + 1 - score) / 2 << " depth " << (int)currSearch << " seldepth " << (int)seldepth << " nodes " << nodes<< " pv ";
        else
            std::cout << std::dec << "info score cp " << (score >> 1) << " depth " << (int)currSearch << " seldepth " << (int)seldepth << " nodes " << nodes  << " pv ";

        for (int i = 0; i < pvLen[0]; i++){
            printMove(pvTable[0][i]);
            std::cout << " ";
        }
        std::cout << " nps " << ((nodes - locNodes) / (timer2 - timer1 + 1)) * 1000 << std::endl;
    }

bmove:
    std::cout << "bestmove ";
    printMove(bestMove);
    std::cout << std::endl;

}