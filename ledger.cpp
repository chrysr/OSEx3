#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "ledger.h"

using namespace std;


ledgerentry::ledgerentry()
{
    int x=1;
}
void ledgerentry::printentry(FILE* fp)//print an entry on the file pointer specified
{
    fprintf(fp,"ShipID:            %d\n\
ShipName:          %s\n\
ShipSize:          %c\n\
ShipUpgradeChoice: %d\n\
ShipManeuverTime:  %d\n\
ShipOrder:         %d\n\
ShipArrivalTime:   %ld\n\
ShipParkingOKTime: %ld\n\
ShipParkingTime:   %ld\n\
ShipDepartureTime: %ld\n\
ShipWaitedFor:     %ld\n\
ShipPayed:         %f\n\
ShipParkingSpot:   %c\n\
ShipStayedFor:     %ld\n\
ShipCurrentStatus: %c\n"\
,ship.id,ship.name,ship.size,ship.upgrade,ship.maneuvert,ship.order,ship.arrtime,parkoktime,parktime,deptime,parkoktime-ship.arrtime,cost,spotsize,deptime-parktime,status);
fflush(fp);
}
