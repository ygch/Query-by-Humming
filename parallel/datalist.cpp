#include "datalist.h"

static seq *head,*tail;

void append(seq *s)
{
    if(head==NULL)
    {
        head=s;
        tail=s;
        tail->next=NULL;

        return;
    }

    tail->next=s;
    tail=s;
    tail->next=NULL;
}

void traverse()
{
    seq *temp=head;

    while(temp!=NULL)
    {
        printf("file %s has sequence length %d,data 0 is %lf\n",temp->name,temp->length,temp->data[0]);
        temp=temp->next;
    }
}

void release()
{
    seq *temp=head;

    while(temp!=NULL)
    {
        head=temp->next;
        free_seq(temp);
        temp=head;
    }
}

void free_seq(seq *s)
{
    free(s->data);
    free(s);
}

const seq* iterator()
{
    return head;
}
