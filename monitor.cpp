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
#include "shared.h"
#include "port.h"
#include "vessel.h"

using namespace std;

int main(int argc,char* argv[])
{
    int id;
    int tim;
    int statimes;
    char logfile[30];
    int err;
    int lastone=1;
    for(int i=0;i<argc;i++)//pass inline parameters
    {
        if(strcmp(argv[i],"-shm")==0)
            id=atoi(argv[i+1]);
        else if(strcmp(argv[i],"-d")==0)
            tim=atoi(argv[i+1]);
        else if(strcmp(argv[i],"-t")==0)
            statimes=atoi(argv[i+1]);
        else if(strcmp(argv[i],"-log")==0)
            strcpy(logfile,argv[i+1]);
    }
    FILE* fp=fopen(logfile,"a");//open file for logging
    shared *s;
    s=(shared*)shmat(id,0,0);//attach shared memory segment
    if(s==NULL)
	{
		cout<<"Attachment error"<<endl;
		return -3;
	}
	else {sem_wait(&s->file);fprintf(fp,"Monitor Attached Shared Segment\n");fflush(fp);sem_post(&s->file);}
    ledgerentry* ledger=&s->ledstart;//pointer to public ledger
    int t=fork();//fork a process that prints the elapsed time
    if(t<0)
    {
        cout<<"error forking"<<endl;
        return -2;
    }
    else if(t==0)
    {
        int perc=0;
        int percp=0;
        while(1)
        {
            usleep(1*1000000);//wait for 0.9 seconds
            sem_wait(&s->console);
            if(s->small+s->medium+s->large+s->csmall+s->cmedium+s->clarge!=0)
                perc=(s->sdep+s->mdep+s->ldep)*100/(s->small+s->medium+s->large+s->csmall+s->cmedium+s->clarge);
            if(perc>percp)
                percp=perc;
            else perc=percp;
            if(s->terminatemonitor==2)
                perc=100;
            cout<<"\r\033[1;31m";
            cout<<"Elapsed Time: "<<(time(NULL)-s->progstart)<<" seconds ("<<perc<<"%)";
            cout<<"\033[0m";fflush(stdout);
            sem_post(&s->console);//print elapsed time
            if(s->terminatemonitor==2)//if we are asked to terminate, print elapsed time once again
            {
                if(lastone)
                    lastone--;
                else
                {
                    err=shmdt(s);//detach shared memory segment and terminate
                    if(err==-1)
                        cout<<"Error Detaching Monitor"<<endl;
                    exit(1);
                }
            }
        }
        return 0;
    }
    int ch=fork();//second child forks for Port State
    if(ch<0)
    {
        cout<<"fork error"<<endl;
        return -1;
    }
    else if(ch==0)
    {
        double earn;
        int ss,m,l;
        while(1)
        {
            sleep(tim);//this will help us print every tim seconds as specified by program's call
            sem_wait(&s->mutex);
            if(ss!=s->Port.gets()||m!=s->Port.getm()||l!=s->Port.getl()||earn!=s->Port.getearnings()||lastone==0)//if a change has occured since the last time we printed
            {//print port state including ships currently in port, available spots, ships waiting to come in, ships waiting to go out
                ss=s->Port.gets();
                m=s->Port.getm();
                l=s->Port.getl();
                earn=s->Port.getearnings();
                sem_wait(&s->console);
                cout<<"\r";
                cout<<"\033[1;33m";
                cout<<"********************Port State********************"<<endl;
                cout<<"\033[0m";
                cout<<"Ships Currently Parked in Port("<<s->parked<<"):"<<endl;
                for(int i=0;i<s->arraysize;i++)
                    if(ledger[i].status=='p'&&ledger[i].ship.id!=0)
                    {
                        cout<<"  ";
                        ledger[i].ship.print();
                    }
                cout<<"Available slots:"<<endl<<"  Small:  "<<ss<<endl<<"  Medium: "<<m<<endl<<"  Large:  "<<l<<endl<<"  Total:  "<<ss+m+l<<endl;
                cout<<"Ships Waiting Outside:"<<endl<<"  Small:  "<<s->small<<endl<<"  Medium: "<<s->medium<<endl<<"  Large:  "<<s->large<<endl<<"  Total:  "<<s->small+s->medium+s->large<<endl;
                cout<<"Ships waiting to Depart:  "<<s->todepart<<endl;
                cout<<"\033[1;33m";
                cout<<"**************************************************"<<endl;
                cout<<"\033[0m"<<endl;fflush(stdout);
                sem_post(&s->console);
            }
            sem_post(&s->mutex);
            if(s->terminatemonitor)//if we're asked to terminate
            {
                if(lastone)//print one more time
                    lastone--;
                else
                {
                    err=shmdt(s);//detach shared memory segment and termintate
                    if(err==-1)
                        cout<<"Error Detaching Monitor(2)"<<endl;
                    exit(1);
                }
            }
        }
        return 0;
    }
    double savg=0,mavg=0,lavg=0;
    double totalavg=0,stotalavg=0;
    double ssavg=0,mmavg=0,llavg=0;
    int ss=0,mm=0,ll=0;
    double stotal=0,sstotal=0;
    while(1)
    {
        sleep(statimes);//this will help us print every statimes as specified by program's call
        sem_wait(&s->mutex);
        totalavg=0;
        stotalavg=0;
        if(s->csmall)//if any small ships are in the port
        {
            savg=(double)s->wsmall/(double)s->csmall;//average waiting time for small ships
            totalavg+=s->wsmall;//total avg
            ss=s->csmall;
            if(s->sdep)
            {
                ssavg=s->ssmall/s->sdep;
                stotalavg+=s->ssmall;
            }

        }
        if(s->cmedium)//same for medium
        {
            mavg=(double)s->wmedium/(double)s->cmedium;
            totalavg+=s->wmedium;
            mm=s->cmedium;
            if(s->mdep)
            {
                mmavg=s->smedium/s->mdep;
                stotalavg+=s->smedium;
            }

        }
        if(s->clarge)//same for large
        {
            lavg=(double)s->wlarge/(double)s->clarge;
            totalavg+=s->wlarge;
            ll=s->clarge;
            if(s->ldep)
            {
                llavg=s->slarge/s->ldep;
                stotalavg+=s->slarge;
            }

        }

        if(totalavg)//calculate for total time waited
            totalavg=totalavg/(double)(ss+mm+ll);
        if(stotalavg)
            stotalavg=stotalavg/(double)(s->sdep+s->mdep+s->ldep);

        if(totalavg!=stotal||stotalavg!=sstotal||lastone==0)//if we got different results than the previous time print, otherwise skip
        {
            sem_wait(&s->console);
            cout<<"\r";
            cout<<"\033[1;36m";
            cout<<"********************Statistics********************"<<endl;
            cout<<"\033[0m";
            cout<<"Average Waiting Times For Ships: "<<endl<<"  Small:  "<<savg<<endl<<"  Medium: "<<mavg<<endl<<"  Large:  "<<lavg<<endl<<"  Overall Wait:  "<<totalavg<<endl;
            cout<<"Average Stay For Ships: "<<endl<<"  Small:  "<<ssavg<<endl<<"  Medium: "<<mmavg<<endl<<"  Large:  "<<llavg<<endl<<"  Overall Stay:  "<<stotalavg<<endl;
            cout<<"Ships That Used/Using the Port: "<<endl<<"  Small:  "<<s->csmall<<endl<<"  Medium: "<<s->cmedium<<endl<<"  Large:  "<<s->clarge<<endl<<"  Total:  "<<(s->csmall+s->cmedium+s->clarge)<<endl;
            cout<<"Current Port Earnings:  "<<endl<<"  Small:  "<<s->Port.getearnings('s')<<"\t(Price per ship: "<<s->Port.getps()<<")"<<endl<<"  Medium: "<<s->Port.getearnings('m')<<"\t(Price per ship: "<<s->Port.getpm()<<")"<<endl<<"  Large:  "<<s->Port.getearnings('l')<<"\t(Price per ship: "<<s->Port.getpl()<<")"<<endl<<"  Total:  "<<s->Port.getearnings()<<endl;
            cout<<"(Prices are without upgrades)"<<endl<<"(Large Ships cannot upgrade)"<<endl;
            cout<<"\033[1;36m";
            cout<<"**************************************************"<<endl;
            cout<<"\033[0m"<<endl;fflush(stdout);
            sem_post(&s->console);


        }
        sem_post(&s->mutex);
        stotal=totalavg;
        sstotal=stotalavg;//save this time's results so we can compare to next time
        if(s->terminatemonitor)//if we're asked to terminate
        {
            if(lastone)
                lastone--;//print one more time
            else
            {
                //wait for the other 2 children to terminate
                int c=1;
                int status;
                while(1)
                {
                    wait(&status);
                    if(WIFEXITED(status))
                        c--;
                    if(c==0)
                        break;

                }
                c=1;
                s->terminatemonitor=2;
                while(1)
                {
                    wait(&status);
                    if(WIFEXITED(status))
                        c--;
                    if(c==0)
                        break;

                }
                err=shmdt(s);//detach shared memory segment
                if(err==-1)
                    cout<<"Error Detaching Monitor"<<endl;
                fflush(stdout);
                cout<<endl;//terminate
                exit(1);
            }
        }

    }
    return 0;
}
