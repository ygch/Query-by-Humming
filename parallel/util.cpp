#include "util.h"


void error(int id)
{
    if(id==1)//open file failed
    {
        fprintf(stderr,"open file failed!\n");
    }
    else if(id==2)//have wrong number of the parameters
    {
        fprintf(stderr,"parameter should be:./spring query.txt sequence.txt R!\n");
    }
    else
    {

    }

    exit(-1);
}
