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

using namespace std;

#ifndef PORT_H
#define PORT_H
class port
{
public:
    port(int s,int m,int l,double sp,double mp,double lp);
    port();
    ~port();
    void set_slots(int s,int m,int l);
    void set_prices(double s,double m,double l);
    int gets();
    int getl();
    int getm();
    int gettotalspots();
    double getps();
    double getpm();
    double getpl();
    void checkout(double cost,char size);
    double getearnings(char size='a');
private:
    int Sslots;
    int Mslots;
    int Lslots;
    double Sprice;
    double Mprice;
    double Lprice;
    double earnings;
    double searnings;
    double mearnings;
    double learnings;
};

#endif
