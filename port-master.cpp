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
#include "vessel.h"

using namespace std;


int main(int argc,char* argv[])
{
    int id;
    shared *s;
    char configfile[20];
    char logfile[30];
    FILE* fp;
    for(int i=0;i<argc;i++)
    {
        if(strcmp(argv[i],"-shm")==0)
            id=atoi(argv[i+1]);
        else if(strcmp(argv[i],"-l")==0)
            strcpy(configfile,argv[i+1]);
        else if(strcmp(argv[i],"-log")==0)
            strcpy(logfile,argv[i+1]);
    }//get inline parameters

    fp=fopen(logfile,"a");//open file for logging
    if(fp==NULL)
        cout<<"error opening file"<<endl;
    s=(shared*)shmat(id,0,0);//attach shared memory segment specified by inline parameters
    if(s==NULL)
	{
		cout<<"Attachment error"<<endl;
		return -3;
	}
    else {sem_wait(&s->file);fprintf(fp,"Master Attached\n");fflush(fp);sem_post(&s->file);}
    ledgerentry* ledger=&s->ledstart; //specify start of public ledger;

    int ret;
    int entry=0;
    FILE* f;
    f=fopen("Ledger.txt","w");//open file to write public ledger entries that have left the port
    int departed=0;
    int part=0;
    int ledcnt=0;
    while(1)
    {
        int flag=1;
        char parkingspot;
        sem_wait(&s->file);
        fprintf(fp,"Available slots |S: %d| |M: %d| |L: %d|\n",s->Port.gets(),s->Port.getm(),s->Port.getl());fflush(fp);
        fprintf(fp,"Status |outside: %d| |ToDepart: %d| |Parked: %d|\n",s->outside,s->todepart,s->parked);fflush(fp);
        sem_post(&s->file);
        if((s->parked+s->outside)==0)//if no ship is parked and none is waiting outside
        {
            if(s->terminatemaster)//if initial program has issued terminating command
            {
                if(part==0)//1st or secondd part of public ledger to write to file
                {
                    for(int i=0;i<s->arraysize/2;i++)
                    {
                        if(ledger[i].ship.id==0)
                            continue;
                        ledcnt++;
                        fprintf(f,"%d-------------------------------------%d\n",ledcnt,ledcnt);
                        ledger[i].printentry(f);
                        departed--;
                        fprintf(f,"%d-------------------------------------%d\n\n",ledcnt,ledcnt);

                    }
                }
                else if(part==1)//2nd part
                {

                    for(int i=s->arraysize/2;i<s->arraysize;i++)
                    {
                        if(ledger[i].ship.id==0)
                            continue;
                        ledcnt++;
                        fprintf(f,"%d-------------------------------------%d\n",ledcnt,ledcnt);
                        ledger[i].printentry(f);
                        departed--;
                        fprintf(f,"%d-------------------------------------%d\n\n",ledcnt,ledcnt);
                    }
                }
                fflush(f);
                sem_wait(&s->file);
                fprintf(fp,"Wrote Remaining Public Ledger Entries to File\n");fflush(fp);
                sem_post(&s->file);
                s->terminatemonitor=1;//request monitor to terminate
                fclose(fp);
                fclose(f);//close open files
                int err;
                err=shmdt(s);//detach shared memory segment
                if(err==-1)
                    cout<<"Error Detaching Master"<<endl;
                exit(1);
            }//else sleep
            sem_wait(&s->file);
            fprintf(fp,"Port Master will sleep\n");fflush(fp);
            sem_post(&s->file);
            sem_wait(&s->sleep);
            sem_wait(&s->file);
            fprintf(fp,"Port Master woke up\n");fflush(fp);
            sem_post(&s->file);
        }
        if(s->outside!=0)//if there are ships outside
        {
            if(s->Port.gettotalspots()>0)//and there are parking spaces available in port
            {
                flag=0;
                if(s->qs==0&&s->small)//signal small ship to fill out its form
                    {sem_post(&s->sstart);sem_wait(&s->master);}
                if(s->qm==0&&s->medium)//signal medium ship to fill out its form
                    {sem_post(&s->mstart);sem_wait(&s->master);}
                if(s->ql==0&&s->large)//signal large ship to fill out its form
                    {sem_post(&s->lstart);sem_wait(&s->master);}
                int flags=0;
                sem_wait(&s->file);
                fprintf(fp,"Orders: |S: %d| |M: %d| |L: %d|\n",s->ships.order,s->shipm.order,s->shipl.order);fflush(fp);
                sem_post(&s->file);
                int choice=-1,upgrade=-1;
                //we have 3 queues, one for each ship type. Pick the queue with the highest priority if that kind of spots are available
                if(s->qs==1&&(((s->ships.order>s->shipm.order||s->qm==0)&&(s->ships.order>s->shipl.order||s->ql==0)))&&s->Port.gets()>0)
                {
                    choice=1;
                    upgrade=0;
                }
                else if(s->qm==1&&(((s->shipm.order>s->ships.order||s->qs==0)&&(s->shipm.order>s->shipl.order||s->ql==0)))&&s->Port.getm()>0)
                {
                    choice=2;
                    upgrade=0;
                }
                else if(s->ql==1&&(((s->shipl.order>s->shipm.order||s->qm==0)&&(s->shipl.order>s->ships.order||s->qs==0)))&&s->Port.getl()>0)
                {
                    choice=3;
                    upgrade=0;
                }
                else//if no spots are available for that ship
                {
                    int f=1;
                    //select the ship with the highest priority which can and wants to upgrade
                    if(s->qm==1&&(s->shipm.order>s->ships.order||s->qs==0))
                    {
                        if(s->shipm.upgrade==1&&s->Port.getl()>0)
                        {
                            choice=2;
                            upgrade=1;
                            f=0;
                        }
                    }
                    else if(s->qs==1&&(s->ships.order>s->shipm.order||s->qm==0))
                    {
                        if(s->ships.upgrade>=1&&s->Port.getm()>0)
                        {
                            choice=1;
                            upgrade=1;
                            f=0;
                        }
                        else if(s->ships.upgrade==2&&s->Port.getl()>0)
                        {
                            choice=1;
                            upgrade=2;
                            f=0;
                        }
                    }
                    if(f==1)//if the previous criteria did not find a match, pick the second highest priority
                    {
                        if(s->qs==1&&(s->Port.getm()==0||s->Port.getl()==0)&&s->Port.gets()>0&&(s->ships.order>s->shipm.order||s->qm==0))
                        {
                            choice=1;
                            upgrade=0;
                        }
                        else if(s->qm==1&&(s->Port.gets()==0||s->Port.getl()==0)&&s->Port.getm()>0&&(s->shipm.order>s->ships.order||s->qs==0))
                        {
                            choice=2;
                            upgrade=0;
                        }
                    }
                }
                time_t tmp;
                if(choice==1)//if choice was to serve the small ship
                {
                    s->qs=0;
                    sem_post(&s->send);//signal it to start moving
                    tmp=time(NULL);
                    sem_wait(&s->master);//master waits for ship to park
                    sem_wait(&s->mutex);
                    flags=1;
                    //see if it got a spot where it needed to upgrade or not
                    if(upgrade==0)
                    {
                        s->Port.set_slots(s->Port.gets()-1,s->Port.getm(),s->Port.getl());
                        parkingspot='s';


                    }
                    else if(upgrade==1)
                    {
                        s->Port.set_slots(s->Port.gets(),s->Port.getm()-1,s->Port.getl());
                        parkingspot='m';


                    }
                    else if(upgrade==2)
                    {
                        s->Port.set_slots(s->Port.gets(),s->Port.getm(),s->Port.getl()-1);
                        parkingspot='l';

                    }
                }
                else if(choice==2)//if choice was to serve the medium ship
                {
                    s->qm=0;
                    sem_post(&s->mend);
                    tmp=time(NULL);
                    sem_wait(&s->master);//master waits for ship to park
                    sem_wait(&s->mutex);
                    flags=1;
                    if(upgrade==0)
                    {
                        s->Port.set_slots(s->Port.gets(),s->Port.getm()-1,s->Port.getl());
                        parkingspot='m';

                    }
                    else if(upgrade==1)//if a large spot was chosen
                    {
                        s->Port.set_slots(s->Port.gets(),s->Port.getm(),s->Port.getl()-1);
                        parkingspot='l';


                    }
                }
                else if(choice==3)//if large ship was chosen
                {
                    s->ql=0;
                    sem_post(&s->lend);
                    tmp=time(NULL);
                    sem_wait(&s->master);//master waits for ship to park
                    sem_wait(&s->mutex);
                    flags=1;
                    s->Port.set_slots(s->Port.gets(),s->Port.getm(),s->Port.getl()-1);
                    parkingspot='l';

                }
                else {flag=1;}
                if(flags)//if a ship was actually served
                {

                    if(entry==s->arraysize)//reset entry number in public ledger
                        entry=0;

                    ledger[entry].parkoktime=tmp;//fill out info on public ledger
                    ledger[entry].parktime=time(NULL);
                    ledger[entry].spotsize=parkingspot;
                    ledger[entry].status='p';
                    switch(choice)//depending on which ship was chosen, increase the coresponding counters
                    {
                        case 1:
                            ledger[entry].ship=s->ships;
                            s->csmall++;
                            s->wsmall=s->wsmall+((double)ledger[entry].parkoktime-(double)ledger[entry].ship.arrtime);
                            break;
                        case 2:
                            ledger[entry].ship=s->shipm;
                            s->cmedium++;
                            s->wmedium=s->wmedium+((double)ledger[entry].parkoktime-(double)ledger[entry].ship.arrtime);
                            break;
                        case 3:
                            ledger[entry].ship=s->shipl;
                            s->clarge++;
                            s->wlarge=s->wlarge+((double)ledger[entry].parkoktime-(double)ledger[entry].ship.arrtime);
                            break;
                        default:
                            break;

                    }
                    s->outside-=1;
                    s->parked++;
                    sem_post(&s->mutex);
                    entry++;//next entry on public ledger

                }
            }

        }
        if(departed>(s->arraysize/2)+1)//if more than half of the public ledger has been filled, write half of it on an file
        {
            if(part==0)
            {
                for(int i=0;i<s->arraysize/2;i++)
                {
                    ledcnt++;
                    fprintf(f,"%d-------------------------------------%d\n",ledcnt,ledcnt);
                    ledger[i].printentry(f);
                    departed--;
                    fprintf(f,"%d-------------------------------------%d\n\n",ledcnt,ledcnt);
                    ledger[i].ship.id=0;

                }
            }
            else if(part==1)
            {
                for(int i=s->arraysize/2;i<s->arraysize;i++)
                {
                    ledcnt++;
                    fprintf(f,"%d-------------------------------------%d\n",ledcnt,ledcnt);
                    ledger[i].printentry(f);
                    departed--;
                    fprintf(f,"%d-------------------------------------%d\n\n",ledcnt,ledcnt);
                    ledger[i].ship.id=0;

                }
            }
            part=1-part;//pick next half for next time
            fflush(f);
            sem_wait(&s->file);
            fprintf(fp,"Wrote Public Ledger Entries to File\n");fflush(fp);
            sem_post(&s->file);

        }
        if(s->todepart!=0)//if ships are waiting to depart
        {
            sem_post(&s->wayout);
            sem_wait(&s->master);//master waits for the vessel to go out

            sem_wait(&s->mutex);
            s->todepart-=1;
            s->parked--;
            //vessel coming out
            int i;//vessel which is departing needs to provide its id in order to leave
            for(i=0;i<s->arraysize;i++)
            {
                if(ledger[i].ship.getid()==s->leavingid)
                    break;
            }
            ledger[i].deptime=time(NULL);
            ledger[i].status='d';
            if(ledger[i].spotsize=='s')//calculate cost and set available slots in port
            {
                s->Port.set_slots(s->Port.gets()+1,s->Port.getm(),s->Port.getl());
                ledger[i].cost=(double)((ledger[i].deptime-ledger[i].parktime)/30 +1)*s->Port.getps();
                s->ssmall+=((double)(ledger[i].deptime)-(double)(ledger[i].parktime));
                s->sdep++;
            }
            else if(ledger[i].spotsize=='m')
            {
                s->Port.set_slots(s->Port.gets(),s->Port.getm()+1,s->Port.getl());
                ledger[i].cost=(double)((ledger[i].deptime-ledger[i].parktime)/30 +1)*s->Port.getpm();
                s->smedium+=((double)(ledger[i].deptime)-(double)(ledger[i].parktime));
                s->mdep++;
            }
            else if(ledger[i].spotsize=='l')
            {
                s->Port.set_slots(s->Port.gets(),s->Port.getm(),s->Port.getl()+1);
                ledger[i].cost=(double)((ledger[i].deptime-ledger[i].parktime)/30 +1)*s->Port.getpl();
                s->slarge+=((double)(ledger[i].deptime)-(double)(ledger[i].parktime));
                s->ldep++;
            }
            s->Port.checkout(ledger[i].cost,ledger[i].ship.size);//add earnings to port
            sem_post(&s->mutex);

            sem_wait(&s->file);
            fprintf(fp,"Cost is: %f for %ld minutes\n",ledger[i].cost,ledger[i].deptime-ledger[i].parktime); fflush(fp);
            sem_post(&s->file);
            sem_post(&s->finalcheck);
            departed++;//another ship departed

            flag=0;
        }
        if(flag)//if no ship was out and no ship wanted to leave then wait 1 before checking again
            sleep(1);
    }
    return 0;
}
