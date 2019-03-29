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
#include "ledger.h"
#include "port.h"
#include "shared.h"
#include <semaphore.h>
#include <time.h>
#include "vessel.h"
#include "ledger.h"

using namespace std;


int main(int argc,char* argv[])
{
    int id;
    shared *s;
    char logfile[30];
    int randomflag=0;
    int order;
    vessel ship;
    int upgrade;
    char size;
    int parkperiod;
    int maneuvertime;
    char name[30];
    time_t arrive=time(NULL);
    for(int i=0;i<argc;i++)//inline parameters
    {
        if(strcmp(argv[i],"-shm")==0)
            id=atoi(argv[i+1]);
        else if(strcmp(argv[i],"-log")==0)
            strcpy(logfile,argv[i+1]);
        else if(strcmp(argv[i],"-random")==0)
            randomflag=1;
        else if(strcmp(argv[i],"-t")==0)
            size=(char)(argv[i+1][0])+32;
        else if(strcmp(argv[i],"-u")==0)
            upgrade=atoi(argv[i+1]);
        else if(strcmp(argv[i],"-p")==0)
            parkperiod=atoi(argv[i+1]);
        else if(strcmp(argv[i],"-m")==0)
            maneuvertime=atoi(argv[i+1]);
        else if(strcmp(argv[i],"-name")==0)
            strcpy(name,argv[i+1]);

    }
    if(size=='s')//upgrade is invalid for specified size, return
    {
        if(upgrade>2&&random==0)
            return -5;
    }
    else if(size=='m')
    {
        if(upgrade>1&&random==0)
            return -5;
    }
    else if(size=='l')
    {
        if(upgrade>0&&random==0)
            return -5;
    }
    FILE* fp;
    fp=fopen(logfile,"a");//open file for logging
    if(fp==NULL)
        cout<<"error opening file"<<endl;
    s=(shared*)shmat(id,0,0);//attach shared memory segment
    if(s==NULL)
	{
		cout<<"Attachment error"<<endl;
		return -3;
	}
    else {sem_wait(&s->file);fprintf(fp,"Vessel %d Attached Shared Segment\n",getpid());fflush(fp);sem_post(&s->file);}
    sem_wait(&s->mutex);
    if((s->parked+s->outside)==0)//if we're the first ship, wake up the port master
        sem_post(&s->sleep);
    s->outside++;//signal that we are outside waiting
    s->order--;
    order=s->order;//assign an order
    if(randomflag)//if random flag was in program's parameters
    {
        srand(time(NULL));
        int sh=(getpid()+rand())%3;//random size and random upgrade
        if(sh==0)
        {
            size='s';
            upgrade=rand()%3;
        }
        else if(sh==1)
        {
            size='m';
            upgrade=rand()%2;
        }
        else if(sh==2)
        {
            size='l';
            upgrade=0;
        }
        maneuvertime=rand()%10 +2;//random maneuver time
        parkperiod=rand()%100 +10;//random parking period
        sprintf(name,"%s_%d","Vessel",getpid());//our name will be Vessel_pid
    }


    sem_wait(&s->file);
    fprintf(fp,"%d Vessel (%c) waiting to get in with order %d and upgrade %d\n",getpid(),size,order,upgrade);fflush(fp);
    sem_post(&s->file);
    if(size=='s')//if we are a small ship
    {
        s->small++;//increase small counter
        sem_post(&s->mutex);
        sem_wait(&s->sstart);//wait on small queue
        s->qs=1;//signal that small queue is not empty
        s->ships.set(getpid(),name,size,upgrade,maneuvertime,order,arrive);//fill out ship info form
        ship=s->ships;//save a copy locally
        sem_post(&s->master);//notify the port master that we have filled out the form
        sem_wait(&s->send);//wait for port master to give us the ok to get inside the port
        sem_wait(&s->mutex);
        s->small--;//-1 ship in small queue
        sem_post(&s->mutex);
    }
    else if(size=='m')//if it's a medium ship. Same as above but in medium
    {
        s->medium++;
        sem_post(&s->mutex);
        sem_wait(&s->mstart);
        s->qm=1;
        s->shipm.set(getpid(),name,size,upgrade,maneuvertime,order,arrive);
        ship=s->shipm;
        sem_post(&s->master);
        sem_wait(&s->mend);
        sem_wait(&s->mutex);
        s->medium--;
        sem_post(&s->mutex);
    }
    else if(size=='l')//if it's a large ship. same as above but in large
    {
        s->large++;
        sem_post(&s->mutex);
        sem_wait(&s->lstart);
        s->ql=1;
        s->shipl.set(getpid(),name,size,upgrade,maneuvertime,order,arrive);
        ship=s->shipl;
        sem_post(&s->master);
        sem_wait(&s->lend);
        sem_wait(&s->mutex);
        s->large--;
        sem_post(&s->mutex);
    }
    else cout<<"Invalid Size"<<endl;//and error has occured
    sem_wait(&s->file);
    fprintf(fp,"%d Parking...\n",ship.id);fflush(fp);//vessel got the ok and is parking
    sem_post(&s->file);
    sleep(maneuvertime);//sleep simulates the ship maneuvering into parking spot
    sem_post(&s->master);//notify port master that we have parked
    sem_wait(&s->file);
    fprintf(fp,"%d Vessel (%c) parked with order %d and upgrade %d and waited in queue for %ld\n",getpid(),size,order,upgrade,time(NULL)-arrive);fflush(fp);
    sem_post(&s->file);
    sleep(parkperiod);//sleep simulates the amount of time we'll be sleeping for
    sem_wait(&s->mutex);
    s->todepart=s->todepart+1;//signal that we want to depart
    sem_post(&s->mutex);
    sem_wait(&s->wayout);//wait for port master to signal that we're clear to leave
    s->leavingid=ship.getid();//provide him with our id
    sem_wait(&s->file);
    fprintf(fp,"%d Departing...\n",ship.id);fflush(fp);
    sem_post(&s->file);
    sleep(maneuvertime);//sleep simulates maneuvering out of port
    sem_post(&s->master);//notify port master that we're out of the port
    sem_wait(&s->finalcheck);//make sure everything is okay with the port
    ledgerentry* ledger=&s->ledstart;
    double amountpayed;
    for(int i=0;i<s->arraysize;i++)//find the amount we were charged by the port master and print it to log file
    {
        if(ledger[i].ship.id==getpid())
        {
            amountpayed=ledger[i].cost;
            break;
        }
    }
    sem_wait(&s->file);
    fprintf(fp,"%d Vessel (%c) departed and payed %f \n",getpid(),size,amountpayed);fflush(fp);
    sem_post(&s->file);
    int err;
    err=shmdt(s);//detach shared memory segment
    if(err==-1)
        cout<<"Error Detaching Vessel"<<endl;

    exit(1);
}
