#ifndef _PREDICTOR_H_
#define _PREDICTOR_H_

// Name: Rajarshi Biswas
// The Ohio State University
// CSE 6421
// Modified TAGE predictor

#include "utils.h"
#include "tracer.h"
#include <bitset>

#define SET	1
#define RESET	0

// Global History //
#define GHIST_MAX_LEN 641 // History length is taken from L-TAGE paper.
#define GHIST bitset<GHIST_MAX_LEN>
#define PATH_HISTORY_LEN 32 // modified path history. L-TAGE says only 16. Increasing it improves performance. 
#define USE_ALT_CTR_INIT 8
#define USE_ALT_CTR_MAX 15

// Bimodal Table
#define BIMODAL_BITS   13
#define BIMODAL_PRED_CTR_INIT 2
#define BIMODAL_PRED_CTR_MAX  3 // 2 bit counter.
typedef struct bimodal {
	UINT32 ctr; // Prediction counter. 2 bits.
//	UINT32 m; // Not used in L-TAGE paper.
} bimodal_t;

// Global TAG Tables
#define WEAK_NOT_TAKEN	3
#define WEAK_TAKEN	4
#define NTAGE 12	// Total number of TAGE tables.
#define TAGE_BITS 11	// Total number of entries per table = 2^TAGE_BITS
#define TAGE_PRED_CTR_INIT 0
#define TAGE_PRED_CTR_MAX  7 // 3 bit counter.
#define TAGE_USEFUL_CTR_MAX 3
#define NOT_FOUND	12
typedef struct global {
    UINT32 ctr; // Prediction counter 3 bits.
    UINT32 tag; // Tag for each entry
    UINT32 u; // Useful bits. L-TAGE paper says use 2 bits.
} global_t;

typedef struct fold_history {
	UINT32 comp;
	UINT32 comp_length;
	UINT32 orig_length;
} fold_history_t;

typedef struct prediction {
	// Bimodal prediction.
	bool bimodal;
	// Primary prediction.
	bool primary;
	int primary_table;
	// Alternate prediction.
	bool alt;
	int alt_table;
} prediction_t;


/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

class PREDICTOR {

  // The state is defined for Gshare, change for your design

 private:
     // histories
     GHIST ghist; // Global history 641 bits.
     unsigned long long phist; // Path History. used 32 bits.

     prediction_t pred;
     UINT32 use_alt; // Global 4 bit counter to check whether to use alternate prediction or not.
     
     // Bimodal Table. Indexed by PC.
     bimodal_t *bimodal;
     UINT32 bimodalIndex;
     UINT32 bimodal_len;

     // Global TAGE tables.
     global_t *tage[NTAGE];
     UINT32 tage_len;
     UINT32 history_length[NTAGE];
     UINT32 tag_width[NTAGE];

     // Aid to history folding.
     fold_history_t folded_index[NTAGE];
     fold_history_t folded_tag[2][NTAGE];

     // Store the index and tags for each table.
     UINT32 tage_index[NTAGE];
     UINT32 tage_tag[NTAGE];
     bool toggle;
     UINT32 tick;

 public:
     // The interface to the four functions below CAN NOT be changed
     PREDICTOR(void);
     bool    GetPrediction(UINT32 PC);
     void    UpdatePredictor(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget);
     void    TrackOtherInst(UINT32 PC, OpType opType, UINT32 branchTarget);

     // Contestants can define their own functions below
     void BitsFoldingInit(fold_history_t *f,  UINT32 original_length,
             UINT32 compressed_length);
     void BitsFolding(fold_history *f, GHIST ghist);
     int GetTag(UINT32 PC, int table, int mask);
     int GetIndex(UINT32 PC, int table);
     void PredictionReset(void);
};

/***********************************************************/
#endif
