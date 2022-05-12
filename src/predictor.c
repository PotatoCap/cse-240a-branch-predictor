//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <math.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Valerie Liu";
const char *studentID   = "A14895768";
const char *email       = "rul021@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

//define number of bits required for indexing the BHT here. 
int ghistoryBits = 14; // Number of bits used for Global History

int t_lh_bits = 10;
int t_lh_width = 10;
int t_gh_bits = 12;

int bpType;       // Branch Prediction Type
int verbose;


//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//
//gshare
uint8_t *bht_gshare;
uint64_t ghistory;

//tournament
uint16_t *lht_t;
uint8_t *lhp_t;
uint8_t *ghp_t;
uint8_t *choice_t;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//

//gshare functions
void init_gshare() {
 int bht_entries = 1 << ghistoryBits;
  bht_gshare = (uint8_t*)malloc(bht_entries * sizeof(uint8_t));
  int i = 0;
  for(i = 0; i< bht_entries; i++){
    bht_gshare[i] = WN;
  }
  ghistory = 0;
}



uint8_t 
gshare_predict(uint32_t pc) {
  //get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries-1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries -1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;
  switch(bht_gshare[index]){
    case WN:
      return NOTTAKEN;
    case SN:
      return NOTTAKEN;
    case WT:
      return TAKEN;
    case ST:
      return TAKEN;
    default:
      printf("Warning: Undefined state of entry in GSHARE BHT!\n");
      return NOTTAKEN;
  }
}

void
train_gshare(uint32_t pc, uint8_t outcome) {
  //get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries-1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries -1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;

  //Update state of entry in bht based on outcome
  switch(bht_gshare[index]){
    case WN:
      bht_gshare[index] = (outcome==TAKEN)?WT:SN;
      break;
    case SN:
      bht_gshare[index] = (outcome==TAKEN)?WN:SN;
      break;
    case WT:
      bht_gshare[index] = (outcome==TAKEN)?ST:WN;
      break;
    case ST:
      bht_gshare[index] = (outcome==TAKEN)?ST:WT;
      break;
    default:
      printf("Warning: Undefined state of entry in GSHARE BHT!\n");
  }

  //Update history register
  ghistory = ((ghistory << 1) | outcome); 
}

void
cleanup_gshare() {
  free(bht_gshare);
}


//tournament functions
void init_tournament() {
 int lht_entries = 1 << t_lh_bits;
 int ght_entries = 1 << t_gh_bits;
  lht_t = (uint16_t*)malloc(lht_entries * sizeof(uint16_t));
  lhp_t = (uint8_t*)malloc(lht_entries * sizeof(uint8_t));
  ghp_t = (uint8_t*)malloc(ght_entries * sizeof(uint8_t));
  choice_t = (uint8_t*)malloc(ght_entries * sizeof(uint8_t));
  int i = 0;
  for(i = 0; i< lht_entries; i++){
    lht_t[i] = 0;
    lhp_t[i] = WN;
  }
  for(i = 0; i< ght_entries; i++){
    ghp_t[i] = WN;
    choice_t[i] = WN;
  }
  ghistory = 0;
}

uint8_t 
tournament_predict(uint32_t pc) {
  uint32_t lht_entries = 1 << t_lh_bits;
  uint32_t ght_entries = 1 << t_gh_bits;
  uint32_t pc_lower_bits = pc & (lht_entries-1);
  uint16_t lh = lht_t[pc_lower_bits];
  uint8_t lp = lhp_t[lh]; // local prediction
  lp = counter2Pred(lp, "Warning: Undefined state of entry in Tournament LHT!\n");

  uint32_t ghistory_lower_bits = ghistory & (ght_entries -1);
  uint8_t gp = ghp_t[ghistory_lower_bits]; //global prediction
  gp = counter2Pred(gp, "Warning: Undefined state of entry in Tournament GHP!\n");

  uint8_t choice = choice_t[ghistory_lower_bits]; //choice
  if(choice < WT){
    return lp;
  }else{
    return gp;
  }
}

void
train_tournament(uint32_t pc, uint8_t outcome) {
  //get lower ghistoryBits of pc
  uint32_t lht_entries = 1 << t_lh_bits;
  uint32_t ght_entries = 1 << t_gh_bits;
  uint32_t pc_lower_bits = pc & (lht_entries-1);
  uint32_t ghistory_lower_bits = ghistory & (ght_entries -1);
  uint16_t lh = lht_t[pc_lower_bits];

  //Update choice
  int local_correct = counter2Pred(lhp_t[lh], "T LHP Invalid\n") == outcome;
  int global_correct = counter2Pred(ghp_t[ghistory_lower_bits], "T GHP Invalid\n") == outcome;
  if (local_correct && !global_correct){
    choice_t[ghistory_lower_bits] = updateCounter(choice_t[ghistory_lower_bits], NOTTAKEN, "T Choice Invalid\n");
  }else{
    choice_t[ghistory_lower_bits] = updateCounter(choice_t[ghistory_lower_bits], TAKEN, "T Choice Invalid\n");
  }

  //Update state of entry in lhp and ghp based on outcome
  lhp_t[lh] = updateCounter(lhp_t[lh], outcome, "Warning: Undefined state of entry in Tournament LHP!\n");
  ghp_t[ghistory_lower_bits] = updateCounter(ghp_t[ghistory_lower_bits], outcome, "Warning: Undefined state of entry in Tournament GHP!\n");

  //Update local history
  lht_t[pc_lower_bits] = ((lh << 1) | outcome) & ((1 << t_lh_width)-1);

  //Update history register
  ghistory = ((ghistory << 1) | outcome); 
}

void
cleanup_tournament() {
  free(lht_t);
  free(lhp_t);
  free(ghp_t);
  free(choice_t);
}

void
init_predictor()
{
  switch (bpType) {
    case STATIC:
    case GSHARE:
      init_gshare();
      break;
    case TOURNAMENT:
      init_tournament();
      break;
    case CUSTOM:
    default:
      break;
  }
  
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      return gshare_predict(pc);
    case TOURNAMENT:
    case CUSTOM:
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//

void
train_predictor(uint32_t pc, uint8_t outcome)
{

  switch (bpType) {
    case STATIC:
    case GSHARE:
      return train_gshare(pc, outcome);
    case TOURNAMENT:
    case CUSTOM:
    default:
      break;
  }
  

}

uint8_t counter2Pred(uint8_t counter, char* message){
  switch(counter){
    case WN:
      return NOTTAKEN;
    case SN:
      return NOTTAKEN;
    case WT:
      return TAKEN;
    case ST:
      return TAKEN;
    default:
      printf(message);
      return NOTTAKEN;
  }
}

uint8_t updateCounter(uint8_t counter, uint8_t outcome, char* message){
  switch(counter){
    case WN:
      return (outcome==TAKEN)?WT:SN;
    case SN:
      return (outcome==TAKEN)?WN:SN;
    case WT:
      return (outcome==TAKEN)?ST:WN;
    case ST:
      return (outcome==TAKEN)?ST:WT;
    default:
      printf(message);
      return counter;
  }
}