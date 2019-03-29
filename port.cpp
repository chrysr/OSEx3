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
#include "port.h"

using namespace std;

port::port(int s,int m,int l,double sp,double mp,double lp)
{
    Sslots=s;
    Mslots=m;
    Lslots=l;
    Sprice=sp;
    Mprice=mp;
    Lprice=lp;
    earnings=0;
    searnings=0;
    mearnings=0;
    learnings=0;
}
void port::set_slots(int s,int m,int l)
{
    Sslots=s;
    Mslots=m;
    Lslots=l;
}
port::port()
{
    Sslots=0;
    Mslots=0;
    Lslots=0;
}
port::~port()
{
    cout<<"Destructor"<<endl;
}
void port::set_prices(double s,double m,double l)
{
    Sprice=s;
    Mprice=m;
    Lprice=l;
}
void port::checkout(double cost,char size)//add costs to the appropriate counter
{
    earnings+=cost;
    if(size=='s')
        searnings+=cost;
    else if(size=='m')
        mearnings+=cost;
    else if(size=='l')
        learnings+=cost;
}
double port::getearnings(char size)//return earnings based on category
{
    if(size=='a')
        return earnings;
    else if(size=='s')
        return searnings;
    else if(size=='m')
        return mearnings;
    else if(size=='l')
        return learnings;
}
int port::gets(){return Sslots;}
int port::getl(){return Lslots;}
int port::getm(){return Mslots;}
int port::gettotalspots(){return Sslots+Mslots+Lslots;}
double port::getps(){return Sprice;}
double port::getpm(){return Mprice;}
double port::getpl(){return Lprice;}
