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
#include <semaphore.h>
#include "shared.h"
#include <time.h>

using namespace std;


int main(int argc,char* argv[])
{
	char logfile[30]="myport.log";//default logfile
	FILE* f;
	int arraysize=10;
	char configfile[20];
	int randomflag=0;
	int monitor,master;
	int vesselcnt=0;
	double every=0;
	int totalships=0;
	if(argc>8||argc<3)//3 arguments mandatory, up to 6
	{
		cout<<"************************************************************************************"<<endl;
		cout<<"This program launches a set of programs that simulate the functionality of a Port"<<endl;
		cout<<"and prints statistics about the Port facilities usage and Ship servicing times"<<endl;
		cout<<"************************************************************************************"<<endl<<endl;

        cout<<"Usage: "<<argv[0]<<" [OPTIONS]..."<<endl<<endl;
        cout<<"Mandatory Arguments: "<<endl;
        cout<<"\t-l <filename>      Configuration File"<<endl;
        cout<<endl<<"Optional Arguments"<<endl;
        cout<<"\t-random <n> <t>    Launch <n> Random Ships <t>(optional, default is 0) seconds apart"<<endl;
		cout<<"\t-log <filename>    LogFile (Default is <myport.log>)"<<endl;
		return -5;
	}
	for(int i=0;i<argc;i++)
	{
		//inline parameters
		if(strcmp(argv[i],"-l")==0)
			strcpy(configfile,argv[i+1]);
		else if(strcmp(argv[i],"-random")==0)
			{randomflag=1; vesselcnt=atoi(argv[i+1]);if(i+2<argc)every=atof(argv[i+2]);} 	//arguments at program's call
		else if(strcmp(argv[i],"-log")==0)//logfile
			strcpy(logfile,argv[i+1]);
	}
	remove(logfile);//remove if logfile exists
	f=fopen(logfile,"a");

	if(f==NULL)
		return -3;
	FILE* fp=fopen(configfile,"r");
    int slots[3];
    double price[3];
    char buffer[300];
	int stats=0;
	int state=0;
	char* token;
	int cnt=0;
    while(fgets(buffer,300,fp)!=NULL)//read initial configuration file arguments
    {
		if(buffer[0]=='#')
			continue;
		token=strtok(buffer,":");
		if(strcmp(token,"S")==0)//get number of small slots
		{
			token=strtok(NULL,":");
			slots[0]=atoi(token);
			token=strtok(NULL," \n\t  ");
			price[0]=atoi(token);
			cnt++;
		}
		else if(strcmp(token,"M")==0)//get number of medium slots
		{
			token=strtok(NULL,":");
			slots[1]=atoi(token);
			token=strtok(NULL," \n\t  ");
			price[1]=atoi(token);
			cnt++;
		}
		else if(strcmp(token,"L")==0)//get number of large slots
		{
			token=strtok(NULL,":");
			slots[2]=atoi(token);
			token=strtok(NULL," \n\t  ");
			price[2]=atoi(token);
			cnt++;
		}
		else
		{
			if(strstr(buffer,"Statistics")!=NULL)//how often to print statistics
			{
				token=strtok(buffer," ");
				token=strtok(NULL," \n\t  ");
				stats=atoi(token);
				cnt++;
			}
			else if(strstr(buffer,"Portstate")!=NULL)//how often to print port state
			{
				token=strtok(buffer," ");
				token=strtok(NULL," \n\t  ");
				state=atoi(token);
				cnt++;
			}
		}
		if(cnt==5)
			break;
    }
	int totalspots=0;
	for(int i=0;i<3;i++)
		totalspots+=slots[i];
	arraysize=2*(totalspots+1);//count total number of slots and public ledger size


	int id=0,err=0;
	shared *s;

	id=shmget(IPC_PRIVATE,arraysize*sizeof(ledgerentry)+sizeof(shared),0666);//create shared memory segment with extra space for public ledger
	if(id==-1)
	{
		cout<<"Creation error"<<endl;
		return -2;
	}

	s=(shared*)shmat(id,0,0);//attach shared memory segment
	if(s==NULL)
	{
		cout<<"Attachment error"<<endl;
		return -3;
	}
	else
	{
		s->init(arraysize);//initialize shared segment variables
		sem_wait(&s->file);//write to logfile
		fprintf(f,"Allocated Shared Memory\n");
		fprintf(f,"My Port Attached Shared Segment\n");fflush(f);
		sem_post(&s->file);
	}
	s->progstart=time(NULL);



	s->Port.set_prices(price[0],price[1],price[2]);//set prices and slot numbers of the port
    s->Port.set_slots(slots[0],slots[1],slots[2]);


	int ch;
	sem_wait(&s->file);
	fprintf(f,"My Port launching Port Master\n");fflush(f); //initialize process for launching the port master program
	sem_post(&s->file);
	ch=fork();
	if(ch<0)
	{
		cout<<"error in creating child"<<endl;
		return -1;
	}
	else if (ch==0)
	{
		char **tmp=new char*[10];
		for(int i=0;i<10-1;i++)
		{
			tmp[i]=new char[30];
		}
		strcpy(tmp[0],"port-master");
		strcpy(tmp[1],"-shm");
		sprintf(tmp[2],"%d",id);
		strcpy(tmp[3],"-l");
		strcpy(tmp[4],configfile);
		strcpy(tmp[5],"-log");
		strcpy(tmp[6],logfile);
		tmp[7]=(char*)NULL;
		execvp("./port-master",tmp);//create child process and overlay port master executable
		return 0;
	}
	master=ch;//save port master pid
	sem_wait(&s->file);
	fprintf(f,"My Port launching Monitor\n");fflush(f);//initialize process for launching the port monitor
	sem_post(&s->file);
	ch=fork();
	if(ch<0)
	{
		cout<<"error int creating child"<<endl;
		return -1;
	}
	else if (ch==0)
	{
		char** tmp=new char*[10];
		for(int i=0;i<10-1;i++)
		{
			tmp[i]=new char[30];
		}
		strcpy(tmp[0],"monitor");
		strcpy(tmp[1],"-shm");
		sprintf(tmp[2],"%d",id);
		strcpy(tmp[3],"-d");
		sprintf(tmp[4],"%d",state);//time
		strcpy(tmp[5],"-t");
		sprintf(tmp[6],"%d",stats);//stats
		strcpy(tmp[7],"-log");
		strcpy(tmp[8],logfile);
		tmp[9]=(char*)NULL;
		execvp("./monitor",tmp);//fork a child and overlay monitor executable
		return 0;
	}
	monitor=ch;//save monitor pid
	int cnter=1;
	rewind(fp);
	while(1)
	{
		if(fp!=NULL)
		{
			while(fgets(buffer,300,fp)!=NULL)//if config file contains specific ships to be launched, we do it here
		    {
		        int flg=0;
				double w;
				if(buffer[0]=='#')//comment line
					continue;
				if(strstr(buffer,"vessel ")==NULL&&strstr(buffer,"wait ")==NULL)//vessel or wait is the only valid ones
					continue;
				if(strstr(buffer,"wait ")!=NULL)//if wait then wait for specified amount and go to next repetition of loop
				{
					strtok(buffer," ");
					w=atof(strtok(NULL," \n\t"));
					usleep(w*1000000);
					continue;
				}
				int pos=0,let=0;
				char args[15][25]={0,0};
		        for(int i=0;i<300;i++)
		        {
					if(buffer[i]=='\0'||buffer[i]=='\n')
		                break;
		            if(buffer[i]!=' '&&buffer[i]!='\t')
		            {
		                args[pos][let]=buffer[i];
		                let++;
		            }
		            else
		            {
		                args[pos][let]='\0';
		                if(pos<13)
		                    pos++;
		                let=0;
		            }
				}
				sem_wait(&s->file);
				fprintf(f,"My Port launching a Vessel (non rand)\n");fflush(f);//write to logfile that we're creating a vessel
				sem_post(&s->file);
				ch=fork();
				if(ch<0)
				{
					cout<<"fork error"<<endl;
					return -2;
				}
				if(ch==0)
				{
					char **tmp=new char*[16];
					for(int i=0;i<16;i++)
					{
						tmp[i]=new char[30];
						if(i<11)
							strcpy(tmp[i],args[i]);
					}
					strcpy(tmp[11],"-shm");
					sprintf(tmp[12],"%d",id);
					strcpy(tmp[13],"-log");
					strcpy(tmp[14],logfile);
					tmp[15]=(char*)NULL;
					execvp("./vessel",tmp);//fork child and overlay vessel executable
					return 0;
				}
				totalships++;
			}
			fclose(fp);//close configuration file if everything has been read
			fp=NULL;
		}
		if(randomflag)//if inline parameters specified random ships
		{
			if(cnter>vesselcnt)
				break;
			sem_wait(&s->file);
			fprintf(f,"My Port launching a Vessel (rand)\n");fflush(f);
			sem_post(&s->file);
			ch=fork();
			if(ch<0)
			{
				cout<<"error forking"<<endl;
				return -2;
			}
			if(ch==0)
			{
				char **tmp=new char*[7];
				for(int i=0;i<6;i++)
				{
					tmp[i]=new char[30];
				}
				strcpy(tmp[0],"vessel");
				strcpy(tmp[1],"-shm");
				sprintf(tmp[2],"%d",id);
				strcpy(tmp[3],"-log");
				strcpy(tmp[4],logfile);
				strcpy(tmp[5],"-random");
				tmp[6]=(char*)NULL;
				execvp("./vessel",tmp);

			}
			totalships++;
			cnter++;
			usleep(every*1000000);
		}
		else break;
	}
	fclose(f);
	sleep(5);//wait some time before issuing termination command
	if((s->parked+s->outside)==0)
	{
		kill(master,SIGKILL);
		s->terminatemonitor=1;
	}
	else s->terminatemaster=1;//signal for port master to initialize termination
	int status;
	totalships+=2;
	while(1)//wait for children
	{
		wait(&status);
		if(WIFEXITED(status))
			totalships--;
		if(totalships==0)
			break;
	}
	err=shmctl(id,IPC_RMID,0);//detach shared memory segment
	if(err==-1)
		cout<<"Error removing"<<endl;
	return 0;//return
}
