#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "vessel.h"
#include <time.h>

#ifndef LEDGER_H
#define LEDGER_H
class ledgerentry
{
public:
    vessel ship;
    time_t parktime;
    time_t deptime;
    time_t parkoktime;
    double cost;
    char spotsize;
    char status;
    ledgerentry();
    ~ledgerentry();
    void printentry(FILE* fp);
};

#endif
