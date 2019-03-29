#include <iostream>
#include <cstring>


#ifndef VESSEL_H
#define VESSEL_H
class vessel
{
public:
    int id;
    char name[30];
    char size;
    int upgrade;  //0 no 1 +1 2+2
    int maneuvert;
    int order;
    time_t arrtime;
    void set(int i,char* n,char s,int u,int m,int o,time_t a)
    {
        id=i;
        strcpy(name,n);
        size=s;
        upgrade=u;
        maneuvert=m;
        order=o;
        arrtime=a;
    }
    int getid(){return id;}
    char* getname(){return name;}
    char getsize(){return size;}
    int getupgrade(){return upgrade;}
    int getmantime(){return maneuvert;}
    int getorder(){return order;}
    void print(){std::cout<<"Vessel named "<<this->name<<" ("<<this->size<<") with ID: "<<this->id<<std::endl; }
    vessel operator=(const vessel& v)
    {
        this->id=v.id;
        strcpy(this->name,v.name);
        this->size=v.size;
        this->upgrade=v.upgrade;
        this->maneuvert=v.maneuvert;
        this->order=v.order;
        this->arrtime=v.arrtime;
    }
};
#endif
