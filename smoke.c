#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include "uthread.h"
#include "uthread_mutex_cond.h"

#define NUM_ITERATIONS 1000

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__);
#else
#define VERBOSE_PRINT(S, ...) ;
#endif

struct Agent {
  uthread_mutex_t mutex;
  uthread_cond_t  match;
  uthread_cond_t  paper;
  uthread_cond_t  tobacco;
  uthread_cond_t  smoke;
};

struct Agent* createAgent() {
  struct Agent* agent = malloc (sizeof (struct Agent));
  agent->mutex   = uthread_mutex_create();
  agent->paper   = uthread_cond_create (agent->mutex);
  agent->match   = uthread_cond_create (agent->mutex);
  agent->tobacco = uthread_cond_create (agent->mutex);
  agent->smoke   = uthread_cond_create (agent->mutex);
  return agent;
}

//
// TODO
// You will probably need to add some procedures and struct etc.
//

/**
 * You might find these declarations helpful.
 *   Note that Resource enum had values 1, 2 and 4 so you can combine resources;
 *   e.g., having a MATCH and PAPER is the value MATCH | PAPER == 1 | 2 == 3
 */
enum Resource            {    MATCH = 1, PAPER = 2,   TOBACCO = 4};
char* resource_name [] = {"", "match",   "paper", "", "tobacco"};

int signal_count [5];  // # of times resource signalled
int smoke_count  [5];  // # of times smoker with resource smoked
struct Agent* giver;
int resourceCount = 0;


/**
 * This is the agent procedure.  It is complete and you shouldn't change it in
 * any material way.  You can re-write it if you like, but be sure that all it does
 * is choose 2 random reasources, signal their condition variables, and then wait
 * wait for a smoker to smoke.
 */
void* agent (void* av) {
  struct Agent* a = av;
  static const int choices[]         = {MATCH|PAPER, MATCH|TOBACCO, PAPER|TOBACCO};
  static const int matching_smoker[] = {TOBACCO,     PAPER,         MATCH};
  
  uthread_mutex_lock (a->mutex);
    for (int i = 0; i < NUM_ITERATIONS; i++) {
      int r = random() % 3;
      
      signal_count [matching_smoker [r]] ++;
      int c = choices [r];
      if (c & MATCH) {
        VERBOSE_PRINT ("match available\n");
       // printf("match available\n");
        uthread_cond_signal (a->match);
      }
      if (c & PAPER) {
        VERBOSE_PRINT ("paper available\n");
        //printf("paper available\n");
        uthread_cond_signal (a->paper);
      }
      if (c & TOBACCO) {
        //printf("tobacco available\n");
        VERBOSE_PRINT ("tobacco available\n");
        uthread_cond_signal (a->tobacco);
      }
      VERBOSE_PRINT ("agent is waiting for smoker to smoke\n");
      uthread_cond_wait (a->smoke);
    }
  uthread_mutex_unlock (a->mutex);
  return NULL;
}


void selectSmoker(){
  if(resourceCount == MATCH + PAPER) {
    //printf("smoker 1 smoked\n");
    resourceCount = 0;
    smoke_count[4]++;
    uthread_cond_signal(giver->smoke);
  } else if (resourceCount == TOBACCO + PAPER) {
    //printf("smoker 2 smoked\n");
    resourceCount = 0;
    smoke_count[1]++;
    uthread_cond_signal(giver->smoke);
  } else if (resourceCount == TOBACCO + MATCH) {
    //printf("smoker 3 smoked\n");
    resourceCount = 0;
    smoke_count[2]++;
    uthread_cond_signal(giver->smoke);
  }
}

//waiting for tabacco
void* tobacco(void* av){
  uthread_mutex_lock(giver->mutex);
  while(1){
    //printf("waiting for tobacco\n");
    uthread_cond_wait(giver->tobacco);
    //printf("tobacco recieved, adding to count\n");
    resourceCount += TOBACCO;
    selectSmoker();
  }
  uthread_mutex_unlock(giver->mutex);
}

// waiting for paper
void* paper(void* av){
  uthread_mutex_lock(giver->mutex);
  while(1){
    //printf("waiting for paper\n");
    uthread_cond_wait(giver->paper);
    //printf("paper recieved, adding to count\n");
    resourceCount += PAPER;
    selectSmoker();
  }
  uthread_mutex_unlock(giver->mutex);
}

//waiting for match
void* match(void* av){
  uthread_mutex_lock(giver->mutex);
  while(1){
    //printf("waiting for match\n");
    uthread_cond_wait(giver->match);
    //printf("match recieved, adding to count\n");
    resourceCount += MATCH;
    selectSmoker();
  }
  uthread_mutex_unlock(giver->mutex);
}





int main (int argc, char** argv) {
  uthread_init (7);
  struct Agent*  a = createAgent();
  giver = a;
  uthread_create(tobacco, 0);
  uthread_create(match, 0);
  uthread_create(paper, 0);
  // TODO
  uthread_join (uthread_create (agent, a), 0);
  assert (signal_count [MATCH]   == smoke_count [MATCH]);
  assert (signal_count [PAPER]   == smoke_count [PAPER]);
  assert (signal_count [TOBACCO] == smoke_count [TOBACCO]);
  assert (smoke_count [MATCH] + smoke_count [PAPER] + smoke_count [TOBACCO] == NUM_ITERATIONS);
  printf ("Smoke counts: %d matches, %d paper, %d tobacco\n",
          smoke_count [MATCH], smoke_count [PAPER], smoke_count [TOBACCO]);
}