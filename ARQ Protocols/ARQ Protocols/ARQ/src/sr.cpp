#include "../include/simulator.h"
#include<iostream>
#include<string.h>
#include<stdio.h>
#include<malloc.h>
#include<math.h>

#define BUFSIZE 9999

/* ******************************************************************
 *  *  *  ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose
 *   *   *
 *    *    *     This code should be used for PA2, unidirectional data transfer 
 *     *     *        protocols (from A to B). Network properties:
 *      *      *           - one way network delay averages five time units (longer if there
 *       *       *                are other messages in the channel for GBN), but can be larger
 *        *        *                   - packets can be corrupted (either the header or the data portion)
 *         *         *                        or lost, according to user-defined probabilities
 *          *          *                           - packets will be delivered in the order in which they were sent
 *           *           *                                (although some can be lost).
 *            *            *                                **********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

int NextToBeSent; //NextSeqNum;
int NextPosition; //Next position for storage. Points to the next usable position. Updated after storage.
int window_size; //Window size
float Sender_base_time;
int No_of_Acks_received=0;
float Arrival_time[1500];
float Sent_time[1500];
float Timeout_values[1500];
int No_of_packets_sent;
int No_of_packets_buffered;
float TIMEOUT;
float compare=0.0, compare_stop=-1.0;
float NewTimeout;
float timer_default=99999.0;
bool Receiver_Acked[1500]={false};
int Sender_Base, Receiver_Base,Corrupted_Ack=0, Corrupted_Pack=0;
float timer_started_at, timer_stopped_at;
char Buffered_Messages[1500][100];

struct packetInfo
{
        struct pkt packet;
        bool isSent;
        bool isAcked;
};

struct packetInfo *PacketStored;

int checkSum(struct pkt packet)
{
        int i;
        int Sum=packet.acknum+packet.seqnum;
        for(i=0;i<sizeof(packet.payload);i++)
        {
                Sum+=packet.payload[i];
        }
        return Sum;
}

int check_checkSum(struct pkt packet)
{
        int i;
        int Sum=0;
        Sum=packet.acknum+packet.seqnum;
        for(i=0;i<sizeof(packet.payload);i++)
        {
                Sum+=packet.payload[i];
        }
        return Sum;

}

int FlipBits(int a)
{
        if(a==0)
                return 1;
        else if(a==1)
                return 0;
}

int compare_float(float f1, float f2)
 {
  if(fabs(f1-f2)<0.00001)
  {
    return 1;
  }
  else
  {
    return 0;
  }
 }
void print_timer_array()
{
        int i;
        /*printf("Printing timer array\n");*/
        for(i=Sender_Base;i<Sender_Base+window_size;i++)
        {
                printf("PA: %d %f\n",i, Timeout_values[i]);
        } printf("\n");
}
//############################################ UPDATE "TIMEOUT" BASED ON LOSS PROBABILITY #########################################################//
void update_timeout()
{
    float loss;
  float temp=TIMEOUT;
  printf("No_of_Acks_received is %d and No_of_packets_sent is %d\n", No_of_Acks_received, No_of_packets_sent);
  loss=1-((float)No_of_Acks_received/(float)No_of_packets_sent);
  float a;

  if(loss>=0.8)
	{ TIMEOUT=15.0;
	}		
  if(TIMEOUT<20)
  {
  /*if(loss>=0.8)
  {
    TIMEOUT=33;
   /* printf("TIMEOUT to 1.15\n");
    
  }*/
  if(loss>=0.6)
  {
    TIMEOUT*=1.5;
   /* printf("TIMEOUT to 1.10\n");*/
    
  }
  else if(loss>=0.4)
  {
    TIMEOUT*=1.23;
   /* printf("TIMEOUT to 1.03\n");*/
  }
  else if(loss>=0.2)
  {
    TIMEOUT*=1.15;
  /*  printf("TIMEOUT to 1.01\n");*/
  }
  else if(loss>=0.1)
  {
    TIMEOUT*=1.10;
   /* printf("TIMEOUT to 1.01\n");*/
  }
  else if(loss>=0.01)
  {
    TIMEOUT*=1.05;
   /* printf("TIMEOUT to 1.01\n");*/
  }


}
  printf("Loss is %f and TIMEOUT is %f\n", loss, TIMEOUT);
}

void updateTimerArray()
{
        int i;
        /*printf("UT: New Timeout is %f\n", NewTimeout);*/
        /*Timeout_values[Sender_Base]-=NewTimeout;*/

        for(i=Sender_Base;i<Sender_Base+window_size;i++)
        {
                if(PacketStored[i].isAcked==false && PacketStored[i].isSent==true)
                {
                        Timeout_values[i]-=NewTimeout;
                }

                else if(PacketStored[i].isAcked==true)
                {
                        Timeout_values[i]=0;
                }

        }
}



 float Minimum()
 {
        int i;
        float small = 99999.0;
        /*printf("Sender_Base is %d and NextPosition is %d\n", Sender_Base, NextPosition);*/
        for(i=Sender_Base;i<Sender_Base+window_size && PacketStored[i].isSent==true;i++)
        {
                if(PacketStored[i].isAcked==false && Timeout_values[i]<small)
                {
                        small=Timeout_values[i];
                }
        }
        /*
 *  *  *      if(compare_float(small,timer_default))
 *   *   *              {
 *    *    *                              small=0.0;
 *     *     *                                      }
 *      *      *                                      */

        return small;
 }

void A_output(struct msg message)
{	//############################################STORE THE NEW PACKET #########################################################//   
		struct pkt packet;
    packet.acknum = 0;
    packet.seqnum = NextPosition;
    strncpy(packet.payload, message.data, sizeof(packet.payload));
    packet.checksum = checkSum(packet);
    PacketStored[NextPosition].packet = packet;
    PacketStored[NextPosition].isAcked = false;
    PacketStored[NextPosition].isSent = false;

    /* Check if it is ready to be sent and if it lies within the window */
    if(NextPosition<Sender_Base+window_size)
    {
        if(PacketStored[NextPosition].isSent==false && PacketStored[NextPosition].isAcked==false)
            {   /* Send this packet */
                /*Arrival_time[NextPosition]=Sender_base_time;*/

                Arrival_time[NextPosition]=get_sim_time();
                if(compare_float(timer_stopped_at, compare_stop))
                {
                        Timeout_values[NextPosition]= TIMEOUT +(Arrival_time[NextPosition] - timer_started_at);
                }
                else
                {
                        Timeout_values[NextPosition]=TIMEOUT;
                }


                PacketStored[NextPosition].isSent=true;
                tolayer3(0, PacketStored[NextPosition].packet);
               printf("AO: Sending packet:: seqno %d, checksum %d, Timeout_value %f, Arrived at %f\n", packet.seqnum, packet.checksum, Timeout_values[NextPosition], Arrival_time[NextPosition]);
       
/*                print_timer_array();
 *                  */ 
 //############################################ CALCULATE NEW TIMEOUT #########################################################//             
 									NewTimeout=Minimum();
  /*              printf("AO: NewTimeout is %f\n", NewTimeout);*/
                 if(NextPosition==Sender_Base)
                {

                        starttimer(0,NewTimeout);
                 /*       printf("AO: Timer started for %f\n", NewTimeout);*/
                        timer_started_at=get_sim_time();
                        timer_stopped_at=-1.0;
                }

                No_of_packets_sent++;


            }
    }
    //############################################ IF PACKET LIES OUT OF THE WINDOW, BUFFER IT ####################################################//
    else /*Buffer the packet */
    {
        printf("AO: Packet buffered:: seqno %d, at %f\n", PacketStored[NextPosition].packet.seqnum, get_sim_time());
     No_of_packets_buffered++;
    }

    NextPosition++;
    /*printf("AO: NextPosition is %d\n", NextPosition);*/
}


void A_input(struct pkt packet)
{
        int y = check_checkSum(packet);

        if(y==packet.checksum)
        {   No_of_Acks_received++;
                /* Packet Received correctly */
                printf("AI: Received Ack:: seqno %d, checksum %d at %f\n", packet.seqnum, packet.checksum, get_sim_time());
                PacketStored[packet.acknum].isAcked=true;

                if(packet.acknum == Sender_Base)
                { //#################################### IF THE PACKET ACKED IS BASE ##################################################//
                        /*Sender_Base++;*/
                        /*PacketStored[Sender_Base].isAcked=true;*/
                 /*       printf("AI: Timer stopped\n");*/
                        stoptimer(0);
                        timer_stopped_at=get_sim_time();
                  //############################################ calculate how long timer was running ####################################//
                        NewTimeout=timer_stopped_at-timer_started_at;
                        /*print_timer_array();*/
									//############################################Subtract that much time #########################################################//
                        updateTimerArray();

                        /*print_timer_array();*/

                        /* Check how many more Acks next to Sender_base have been received */

                        int i;
                        int j;
                        bool loop=true;
                        int count=0;

                        for(i=Sender_Base;i<Sender_Base+window_size && loop ==true && i<NextPosition;i++)
                        {
                                if(PacketStored[i].isAcked==true)
                                {
                                        count++;
                                }
                                else
                                {
                                        loop=false; /*Break loop */
                                }
                        }
                        /*printf("AI: Count of previous Acks:: %d\n", count);*/

                        Sender_Base=Sender_Base+count;

                        /*printf("AI: Sender Updated to:: %d\n", Sender_Base);*/
                        Sender_base_time=TIMEOUT;

                        j=Sender_Base;
                        /*printf("AI: j is %d and NextPosition is %d\n",j, NextPosition);*/
											//############################################SEND AS MANY PACKETS AS SPACE LEFT ###########################################//
                        while(count>0)
                        {
                                if(j<NextPosition && j<Sender_Base+window_size && PacketStored[j].isSent==false && PacketStored[j].isAcked==false)
                                        {

                                                PacketStored[j].isSent=true;
                                                PacketStored[j].isAcked=false;
                                                Arrival_time[j]=get_sim_time();
                                                if(compare_float(timer_stopped_at, compare_stop))
                                        {
   //############################################ CALCULATE TIMEOUT FOR THE NEW PACKET  #########################################################//
                                                Timeout_values[NextPosition]= TIMEOUT +(Arrival_time[j] - timer_started_at);
                                        }
                                        else
                                        {
                                                Timeout_values[j]=TIMEOUT;
                                        }
                                                /*Timeout_values[j]=TIMEOUT;+(Arrival_time[NextPosition] - Arrival_time[Sender_Base]);*/
                       printf("AI: Sending packet:: seqno %d, checkSum %d, Timeout_value %f, arrived at %f\n", PacketStored[j].packet.seqnum, PacketStored[j].packet.checksum, Timeout_values[j], Arrival_time[j]);
                                                                       tolayer3(0,PacketStored[j].packet);

                                         /*       print_timer_array();*/
                                                /*Sender_Base++;*/
                                                count--;
                                                No_of_packets_sent++;
                                        }
//############################################ PACKET OUT OF THE WINDOW #########################################################//
                                        else if(j>=NextPosition || j>=Sender_Base+window_size)
                                        {
                                                /*printf("AI: Exiting loop\n");*/
                                                count=0;
                                        }
                                        j++;

                        }

                        NewTimeout=Minimum();
   /*                     printf("AI: New Timeout is %f\n", NewTimeout);
 *                     */
                        if(!compare_float(NewTimeout, timer_default))
                        {
                        starttimer(0,NewTimeout);
  /*                      printf("AI: Timer started for %f\n", NewTimeout);
 *                          */                    timer_started_at=get_sim_time();
                        timer_stopped_at=-1.0;
  }

                }
//############################################ PACKET is not base #########################################################//
                else
                { 
                        Timeout_values[packet.acknum]=0;
                /*        print_timer_array();
 *                        */
 								}
                        if(No_of_Acks_received%10==0)
                        {
                            update_timeout();
                        }
                       
        }
//############################################ ACK RECEIVED IS CORRUPTED #########################################################//
        else
        {
                printf("AI: Received corrupted Ack at %f\n", get_sim_time());
                        Corrupted_Ack++;
        }
}

void A_timerinterrupt()
{
        printf("AT: Time Out and NextPosition is %d\n", NextPosition);
 /*       printf("AT: Timer stopped\n");
 *          */    
 			 timer_stopped_at=get_sim_time();

        updateTimerArray();

        int i;
    for(i=Sender_Base;i<Sender_Base+window_size && i<NextPosition;i++)
    {
        /*printf("AT-P1: i is %d, PacketStored[i].isSent is %d and PacketStored[i].isAcked is %d\n",i, PacketStored[i].isSent, PacketStored[i].isAcked);*/
//############################################ RESEND PACKET FOR WHICH TIMER EXPIRED OR SEND NEW PACKETS #################################//
        if(compare_float(Timeout_values[i],compare) && PacketStored[i].isSent && !PacketStored[i].isAcked)
        {
                /*printf("AT: Inside 1st loop\n");*/
                /*PacketStored[i].isAcked=false;
 *  *  *              PacketStored[i].isSent=true;*/
                printf("AT: Resending packet:: seqno %d, checksum %d, Timeout_value %f, arrived at %f for i = %d\n", PacketStored[i].packet.seqnum, PacketStored[i].packet.checksum, Timeout_values[i], Arrival_time[i],i);
          tolayer3(0,PacketStored[i].packet);
            No_of_packets_sent++;
                Arrival_time[i]=get_sim_time();
                if(compare_float(timer_stopped_at, compare_stop))
                {
                        Timeout_values[i]= TIMEOUT +(Arrival_time[i] - timer_started_at);
                }
                else
                {
                        Timeout_values[i]=TIMEOUT;
                }
                /*Timeout_values[i]=TIMEOUT;*/
         /*       print_timer_array();
 *       */
        }
//############################################SEND NEW PACKETS #########################################################//
else if(!PacketStored[i].isSent && i<NextPosition)
        {
                /*printf("AT: Inside 2nd loop for i = %d\n",i);*/
                /*printf("AT-P2: PacketStored[i].isSent is %d and PacketStored[i].isAcked is %d\n",PacketStored[i].isSent, PacketStored[i].isAcked);*/
                PacketStored[i].isAcked=false;
                PacketStored[i].isSent=true;
               printf("AT: Sending packet:: seqno %d, checksum %d, Timeout_value %f, arrived at %f\n", PacketStored[i].packet.seqnum, PacketStored[i].packet.checksum, Timeout_values[i], Arrival_time[i]);
             tolayer3(0,PacketStored[i].packet);
                No_of_packets_sent++;
                Arrival_time[i]=get_sim_time();
                if(compare_float(timer_stopped_at, compare_stop))
                {
                        Timeout_values[i]= TIMEOUT +(Arrival_time[i] - timer_started_at);
                }
                else
                {
                        Timeout_values[i]=TIMEOUT;
                }
                /*Timeout_values[i]=TIMEOUT;*/
     /*           print_timer_array();
 *           */
        }
    }
 //############################################ CALCULATE NEW TIMEOUT #########################################################//   
    NewTimeout= Minimum();
/*    printf("AT-1: New timeout is %f\n", NewTimeout); */

  /*      printf("AT: Timer started for %f\n", NewTimeout);
 *          */    starttimer(0,NewTimeout);
        timer_started_at=get_sim_time();
        timer_stopped_at=-1.0;
}

void A_init()
{

    NextPosition=0;
    Sender_Base=0;
    NextToBeSent=0;
    window_size=getwinsize();
    PacketStored=(struct packetInfo*) malloc(BUFSIZE*sizeof(struct packetInfo));
    if(window_size<=50)
        {TIMEOUT=20;
        }
    else if(window_size<=100)
        {TIMEOUT=22;
        }
    else if(window_size<=150)
        {TIMEOUT=24;
        }
    else if(window_size<=200)
        {TIMEOUT=26;/**** was 10***/
        }
    else if(window_size<=500)
        {TIMEOUT=28;
        }
    else
        {TIMEOUT=30;
        }

    NewTimeout=TIMEOUT;
}

void B_input(struct pkt packet)
{
        int y=check_checkSum(packet);
        bool loop=true;
        int i;
        int countr=0;
        if(y==packet.checksum)
        {
                /* Correct Ack */
               printf("BI-1: Received packet:: seqnum %d, checksum %d at %f\n", packet.seqnum, packet.checksum, get_sim_time());
               
                int index=packet.seqnum;

                Receiver_Acked[index]=true;
                strncpy(Buffered_Messages[index], packet.payload, sizeof(packet.payload));

                /* -------------------------------------------------------------------------------------------------------------------- */



        /* -------------------------------------------------------------------------------------------------------------------- */

                if(packet.seqnum>=Receiver_Base && packet.seqnum<Receiver_Base+window_size)
                {
                        /*printf("BI-1: Inside If\n");*/
                        for(i=Receiver_Base;i<Receiver_Base+window_size && loop==true;i++)
                {
                        if(Receiver_Acked[i]==true)
                        {
                                countr++;
                                char message_tolayer5[100];
                                strncpy(message_tolayer5, Buffered_Messages[i], sizeof(Buffered_Messages[i]));
  /*                              printf("Sending message to layer 5:: message: %s\n", message_tolayer5);
 *                                  */                            tolayer5(1, Buffered_Messages[i]);
                        }
 else
                        {
                                loop=false;
                        }
                }

            Receiver_Base+=countr;
            /*printf("BI-1: Previously received ack:: count %d\n", countr);*/
            /*printf("Receiver_Base incremented to %d\n", Receiver_Base);*/
                struct pkt Ack;
                        Ack.seqnum=packet.seqnum;
                    Ack.acknum=packet.seqnum;
                    strncpy(Ack.payload,packet.payload,sizeof(packet.payload));
                    Ack.checksum=checkSum(Ack);
                    printf("BI-1: Sending ack with acknum %d and checksum %d at %f\n",Ack.seqnum, Ack.checksum, get_sim_time());
                                 tolayer3(1,Ack);



                }

                else if(packet.seqnum>=Receiver_Base-window_size && packet.seqnum<Receiver_Base)
                {
                        struct pkt Ack;
                        Ack.seqnum=packet.seqnum;
                    Ack.acknum=packet.seqnum;
                    strncpy(Ack.payload,packet.payload,sizeof(packet.payload));
                    Ack.checksum=checkSum(Ack);
                    printf("BI-1: Sending ack with acknum %d and checksum %d at %f\n",Ack.seqnum, Ack.checksum, get_sim_time());
                            tolayer3(1,Ack);
                }
        }

                else
                {
                        Corrupted_Pack++;
                }


}

void B_init()
{
        Receiver_Base=0;
}


