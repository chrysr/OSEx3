#include "shared.h"

using namespace std;

void shared::init(int arrsize)//initialize semaphores and other shared memory variables
{
    int ret;
    ret=sem_init(&wayout,1,0);
    if(ret!=0) cout<<"problem init semaphore"<<endl;
    ret=sem_init(&master,1,0);
    if(ret!=0) cout<<"problemd init semaphore"<<endl;
    ret=sem_init(&mutex,1,1);
    if(ret!=0) cout<<"problemd init semaphorssde"<<endl;
    ret=sem_init(&file,1,1);
    if(ret!=0) cout<<"problemd init semaphorssde"<<endl;
    ret=sem_init(&console,1,1);
    if(ret!=0) cout<<"problemd init semaphorssde"<<endl;
    ret=sem_init(&sstart,1,0);
    if(ret!=0) cout<<"problemd init semaphorssde"<<endl;
    ret=sem_init(&mstart,1,0);
    if(ret!=0) cout<<"problemd init semaphorssde"<<endl;
    ret=sem_init(&lstart,1,0);
    if(ret!=0) cout<<"problemd init semaphorssde"<<endl;
    ret=sem_init(&send,1,0);
    if(ret!=0) cout<<"problemd init semaphorssde"<<endl;
    ret=sem_init(&mend,1,0);
    if(ret!=0) cout<<"problemd init semaphorssde"<<endl;
    ret=sem_init(&lend,1,0);
    if(ret!=0) cout<<"problemd init semaphorssde"<<endl;
    ret=sem_init(&sleep,1,0);
    if(ret!=0) cout<<"problemd init semaphorssde"<<endl;
    ret=sem_init(&finalcheck,1,0);
    if(ret!=0) cout<<"problemd init semaphorssde"<<endl;
    todepart=0;
    outside=0;
    parked=0;
    qs=qm=ql=0;
    order=0;
    wsmall=wmedium=wlarge=0;
    ssmall=smedium=slarge=0;
    sdep=mdep=ldep=0;
    small=medium=large=0;
    csmall=cmedium=clarge=0;
    arraysize=arrsize;
    terminatemaster=terminatemonitor=0;
}
