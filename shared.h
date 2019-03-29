#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include "port.h"
#include "vessel.h"
#include "ledger.h"

using namespace std;

#ifndef SHARED_H
#define SHARED_H
class shared//definition of shared memory struct
{
public:
    time_t progstart;
    int terminatemaster;
    int terminatemonitor;
    sem_t wayout;
    sem_t master;
    sem_t mutex;
    sem_t sleep;
    int outside;
    int todepart;
    int parked;
    vessel ships;
    vessel shipm;
    vessel shipl;
    sem_t sstart;
    sem_t mstart;
    sem_t lstart;
    sem_t console;
    sem_t send;
    sem_t mend;
    sem_t lend;
    sem_t finalcheck;
    sem_t file;
    int qs;
    int qm;
    int ql;
    int order;
    int small;
    int medium;
    int large;
    int csmall;
    int cmedium;
    int clarge;
    int sdep;
    int mdep;
    int ldep;
    time_t wsmall;
    time_t wlarge;
    time_t wmedium;
    time_t ssmall;
    time_t smedium;
    time_t slarge;
    port Port;
    void init(int arrsize);
    int leavingid;
    int arraysize;
    ledgerentry ledstart;

};
#endif
