// Name: Rajarshi Biswas
// The Ohio State University
// CSE 6421
// Modified TAGE predictor

#include "predictor.h"

// Total number of global tables = 12 (TAGE componenets)
// 1 bimodal table.
// History length and corresponding tag lengths are taken from L-TAGE
// paper. The 0th Indexed table has the higer hisotry length.
// Global history len.
UINT32 tage_history_lengths[NTAGE] = {640, 403, 254, 160, 101, 64, 40, 25, 16, 10, 6, 4};
// Global tag len.
UINT32 tage_tag_widths[NTAGE] = {15,15,15,15,15,15,15,15,15,15,15,14};

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

PREDICTOR::PREDICTOR(void){
    ghist.reset();
	phist = 0;

	bimodal_len = (1 << BIMODAL_BITS);
	bimodal = new bimodal_t[bimodal_len];

	// Initialize the bimodal table.
	for (UINT32 i = 0; i< bimodal_len; i++) {
		bimodal[i].ctr = BIMODAL_PRED_CTR_INIT;
		//bimodal[i].m = RESET;
	}

	// Init the tag tables and corresponding helper tables.
	tage_len = (1 << TAGE_BITS);

	for (int i = 0; i < NTAGE ; i++) {
		tage[i] = new global_t[tage_len];
		for (UINT32 j = 0; j < tage_len; j++) {
			tage[i][j].ctr = TAGE_PRED_CTR_INIT;
			tage[i][j].tag = RESET;
			tage[i][j].u = RESET;
		}

		tag_width[i] = tage_tag_widths[i];
		history_length[i] = tage_history_lengths[i];
		BitsFoldingInit(&folded_index[i], history_length[i], TAGE_BITS);
		BitsFoldingInit(&folded_tag[0][i], history_length[i], tag_width[i]);
		BitsFoldingInit(&folded_tag[1][i], history_length[i], tag_width[i] - 1);

		tage_index[i] = RESET;
		tage_tag[i] = RESET;
	}

	use_alt = USE_ALT_CTR_INIT;
	tick = RESET;
	toggle = SET;

}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

bool   PREDICTOR::GetPrediction(UINT32 PC) {

    PredictionReset();
    // Get the Index and tags for the current branch.
    for (int i = 0 ; i < NTAGE; i++) {
        tage_tag[i] = GetTag(PC, i, tag_width[i]);
        tage_index[i] = GetIndex(PC, i);
    }
    // Primary prediction.
    for (int i = 0; i < NTAGE; i++) {
        if (tage[i][tage_index[i]].tag == tage_tag[i]) {
            // Found primary prediction.
            pred.primary_table = i;
            break;
        }
    }
    // See if we can find an alternate prediction that is the
    // lesser length history tables.
    for (int i = pred.primary_table + 1; i < NTAGE; i++) {
        if (tage[i][tage_index[i]].tag == tage_tag[i]) {
            // Found an alternate prediction.
            pred.alt_table = i;
            break;
        }
    }

    // at the end get the bimodal prediction.
    bimodalIndex = (PC & ((1 << BIMODAL_BITS ) - 1));
    if (bimodal[bimodalIndex].ctr > BIMODAL_PRED_CTR_MAX/2) {
        pred.bimodal =  TAKEN;
    } else {
        pred.bimodal =  NOT_TAKEN;
    }

    if (pred.primary_table == NOT_FOUND) {
        // Ok No primary prediction found, Hence return the
        // bimodal one.
        return pred.bimodal;
    } else {
        if (pred.alt_table == NTAGE) {
            pred.alt = pred.bimodal;
        } else {
            if (tage[pred.alt_table][tage_index[pred.alt_table]].ctr
                >= TAGE_PRED_CTR_MAX/2) {
                pred.alt = TAKEN;
            } else {
                pred.alt = NOT_TAKEN;
            }
        }

        if ((tage[pred.primary_table][tage_index[pred.primary_table]].ctr  != WEAK_NOT_TAKEN) ||
            (tage[pred.primary_table][tage_index[pred.primary_table]].ctr != WEAK_TAKEN) ||
            (tage[pred.primary_table][tage_index[pred.primary_table]].u != RESET) ||
            (use_alt < 8)) {
            if (tage[pred.primary_table][tage_index[pred.primary_table]].ctr
                > TAGE_PRED_CTR_MAX/2) {
                pred.primary = TAKEN;
            } else  {
                pred.primary = NOT_TAKEN;
            }
            return pred.primary;
        } else {
            // In case of a weak entry take the alternate
            // prediction.
            return pred.alt;
        }
    }
}


/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void  PREDICTOR::UpdatePredictor(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {

    // Steal an entry only if the prediction was wrong and it
 	// did not use the longest histroy to predict.
 	if ((pred.primary_table > 0) && (predDir != resolveDir)) {
 		bool found_empty = false;
 		for (int i = 0; i < pred.primary_table; i++) {
 			if (tage[i][tage_index[i]].u == 0) {
 				found_empty = true;
 				break;
 			}
 		}

 		if (found_empty == true) {
 			int random_num = rand() % 100;
 			for (int i = pred.primary_table - 1; i >= 0; i--) {
 				if (tage[i][tage_index[i]].u == 0) {
                    // Take biased decision whether to update the table or not.
 					if (random_num <= 99/ ( pred.primary_table - i+1)) {
 						if (resolveDir == TAKEN) {
 							tage[i][tage_index[i]].ctr = 4;
 						} else {
 							tage[i][tage_index[i]].ctr = 3;
 						}
 						tage[i][tage_index[i]].tag = tage_tag[i];
 						tage[i][tage_index[i]].u = 0;
 					}
 				}
 			}
 		 } else {
             // Choose a random table and steal it.
 			 int i = rand() % NTAGE;
 			 if(resolveDir == TAKEN) {
 				 tage[i][tage_index[i]].ctr = WEAK_TAKEN;
 			 } else {
 				 tage[i][tage_index[i]].ctr = WEAK_NOT_TAKEN;
 			 }
 			 tage[i][tage_index[i]].tag = tage_tag[i];
 			 tage[i][tage_index[i]].u = 0;
 		 }
 	}

 	if (pred.primary_table == NOT_FOUND) {
 		// We have only bimodal prediction.
 		if(resolveDir == TAKEN) {
 			bimodal[bimodalIndex].ctr = SatIncrement(bimodal[bimodalIndex].ctr,
 			    BIMODAL_PRED_CTR_MAX);
 		} else {
 			bimodal[bimodalIndex].ctr = SatDecrement(bimodal[bimodalIndex].ctr);
 		}

 	} else {
 		// Update the counters in the primary table.
 		if (resolveDir == TAKEN) {
 			tage[pred.primary_table][tage_index[pred.primary_table]].ctr =
 			    SatIncrement(tage[pred.primary_table][tage_index[pred.primary_table]].ctr,
 			    TAGE_PRED_CTR_MAX);
 		} else {
 			tage[pred.primary_table][tage_index[pred.primary_table]].ctr =
 			SatDecrement(tage[pred.primary_table][tage_index[pred.primary_table]].ctr);
 		}

 		// Update the useful counters of the primary table
 		if (pred.primary == resolveDir) {
 			tage[pred.primary_table][tage_index[pred.primary_table]].u =
 				SatIncrement(tage[pred.primary_table][tage_index[pred.primary_table]].u,
 				TAGE_USEFUL_CTR_MAX);
 		} else {
 			tage[pred.primary_table][tage_index[pred.primary_table]].u =
 			    SatDecrement(tage[pred.primary_table][tage_index[pred.primary_table]].u);
 		}

 		// Was it a weak entry ?
 		if (((tage[pred.primary_table][tage_index[pred.primary_table]].ctr == WEAK_NOT_TAKEN) ||
 		    (tage[pred.primary_table][tage_index[pred.primary_table]].ctr == WEAK_TAKEN)) &&
 		    (tage[pred.primary_table][tage_index[pred.primary_table]].u == RESET)) {
 			if ((pred.primary != resolveDir) &&
 			    (pred.alt == resolveDir)) {
 				// Alternate prediction was good.
 				SatIncrement(use_alt, USE_ALT_CTR_MAX);
 				if (pred.alt_table != NTAGE) {
 					if (resolveDir == TAKEN) {
 						tage[pred.alt_table][tage_index[pred.alt_table]].u =
 						SatIncrement(tage[pred.alt_table][tage_index[pred.alt_table]].u,
 						TAGE_USEFUL_CTR_MAX);
 					} else {
 						tage[pred.alt_table][tage_index[pred.alt_table]].u =
 						SatDecrement(tage[pred.alt_table][tage_index[pred.alt_table]].u);

 					}
 					// IF the alternate prediction came
 					// from a TAGE table increment the
 					// useful counter.
 					tage[pred.alt_table][tage_index[pred.alt_table]].u =
 					SatIncrement(tage[pred.alt_table][tage_index[pred.alt_table]].u,
 					TAGE_USEFUL_CTR_MAX);
 				}
 			} else {
 				SatDecrement(use_alt);
 				if (pred.alt_table != NTAGE) {
 					// IF the alternate prediction came
 					// from a TAGE table decrement the
 					// useful counter.
 					tage[pred.alt_table][tage_index[pred.alt_table]].u =
 					SatDecrement(tage[pred.alt_table][tage_index[pred.alt_table]].u);
 				}
 			}
 		}
 	}


 	// update the Global history and path history.
 	ghist = (ghist << 1);
 	phist = (phist << 1);
 	if (resolveDir == TAKEN) {
 		ghist.set(0,1);
 		phist = phist + 1;
 	}

 	phist = phist % PATH_HISTORY_LEN;

 	for (int i = 0; i < NTAGE; i++) {
 		BitsFolding(&folded_index[i], ghist);
 		BitsFolding(&folded_tag[0][i], ghist);
 		BitsFolding(&folded_tag[1][i], ghist);
 	}

 	tick++;
 	if (tick == 262144) { // RESET the Useful bits every 256K instructions.
 		if (toggle == SET) {
 			for (int i = 0; i < NTAGE; i++) {
 				for (UINT32 j = 0; j < tage_len; j++) {
 					tage[i][j].u = tage[i][j].u & 1;
 				}
 			}
 			toggle = RESET;
 		} else {
 			for (int i = 0; i < NTAGE; i++) {
 				for (UINT32 j = 0; j < tage_len; j++) {
 					tage[i][j].u = tage[i][j].u & 2;
 				}
 			}
 			toggle = SET;
 		}
 		tick = RESET;
 	}
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void    PREDICTOR::TrackOtherInst(UINT32 PC, OpType opType, UINT32 branchTarget) {

  // This function is called for instructions which are not
  // conditional branches, just in case someone decides to design
  // a predictor that uses information from such instructions.
  // We expect most contestants to leave this function untouched.

  return;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void PREDICTOR::PredictionReset(void) {
	pred.primary = -1;
	pred.alt = -1;
	pred.primary_table = NOT_FOUND;
	pred.alt_table = NOT_FOUND;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////


void PREDICTOR::BitsFoldingInit(fold_history_t *f,  UINT32 original_length,
    UINT32 compressed_length) {
	f->comp = 0;
	f->orig_length = original_length;
	f->comp_length = compressed_length;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////


void PREDICTOR::BitsFolding(fold_history_t *f, GHIST ghist) {
	f->comp = (f->comp << 1) | ghist[0];
	f->comp ^= ghist[f->orig_length] << (f->orig_length % f->comp_length);
	f->comp ^= (f->comp >> f->comp_length);
	f->comp &= (1 << f->comp_length) - 1;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////


int PREDICTOR::GetIndex(UINT32 PC, int table) {
	int index = PC ^ (PC >> (TAGE_BITS - table )) ^ folded_index[table].comp ^ (phist>> (TAGE_BITS - table));
	return (index & ((1 << TAGE_BITS) - 1));
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////


int PREDICTOR::GetTag(UINT32 PC, int table, int tag_width) {
	int tag = (PC ^ folded_tag[0][table].comp ^ (folded_tag[1][table].comp << 1));
	return (tag & ((1 << tag_width) - 1));
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
