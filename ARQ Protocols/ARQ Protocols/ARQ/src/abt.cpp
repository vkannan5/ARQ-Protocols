#include "../include/simulator.h"
#include<iostream>
#include<string.h>
#include<stdio.h>
#include<vector>
#include<stdlib.h>

using namespace std;

#define TIMEOUT 35.0

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
bool Ready, isRetransmission;
int nextSeqNum, RequestedSeq, ExpectedAck, RetransmittedCount=0, DroppedPacketsCount=0, invalidAcks=0, noofAcks=0, duplicateAcksSent=0;

struct msg LastMessage, LastMessageSent;
struct pkt LastPacket, LastPacketSent;
int LastCorrectlySent;
struct pkt LastAckSent;
std::vector<pkt> BufferedPackets;

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
/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
        /**************************************************
 *  *  *      //State variable maintains the state of the Host A 
 *   *   *              //1. Ready to send and new transmission
 *    *    *                      //2. Ready to send and retransmission
 *     *     *                              //3. Not Ready - drop packets
 *      *      *                                      **************************************************/
        /*char *buffer;
 *  *  *
 *   *   *      strncpy(buffer, message.data, strlen(message.data));*/
        /*printf("A: A Output called \n");
 *  *         printf("A: Message is %s New Packet\n", message.data);*/

        if(Ready==true && isRetransmission==false) /*New packet*/
        {

                        if(BufferedPackets.empty())
                {
                        struct pkt packet;
                        packet.acknum=0;
                        packet.seqnum=nextSeqNum;
                        strncpy(packet.payload,message.data,sizeof(message.data));
                        packet.checksum=checkSum(packet);
                        Ready=false;
                        tolayer3(0,packet);
                        printf("A: Transmitting packet with message %s, sequence number %d, checksum %d\n", message.data, nextSeqNum, packet.checksum);
                        starttimer(0,TIMEOUT);
                        LastMessage=message;
                        LastMessageSent=message;
                        LastPacket=packet;
                        LastPacketSent=packet;
                        LastCorrectlySent=packet.seqnum; /* ------ was = NextSeqNum ------ */
                        nextSeqNum=FlipBits(nextSeqNum);
                    }

                    else if(!BufferedPackets.empty())
                    {
                        struct pkt packet=BufferedPackets.front();
                        BufferedPackets.erase(BufferedPackets.begin());
                        /*printf("Sending message %s wih sequence number %d and checksum %d at %f\n", packet.payload,packet.seqnum, packet.checksum, get_sim_time());*/
                        printf("A: Transmitting packet with message %s, sequence number %d and checksum %d\n", packet.payload, packet.seqnum, packet.checksum);
                        tolayer3(0,packet);
                        LastCorrectlySent=packet.seqnum;
                        starttimer(0,TIMEOUT);
                        strncpy(LastMessageSent.data,packet.payload,sizeof(packet.payload));
                        LastPacket=packet;
                        LastPacketSent=packet;
                        Ready=false;
                        struct pkt packet2;
                        packet2.acknum=0;
                        packet2.seqnum=nextSeqNum;
                        strncpy(packet2.payload,message.data,sizeof(message.data));
                        LastMessage=message;
                        packet2.checksum=checkSum(packet2);
                        BufferedPackets.push_back(packet2);
                        /*printf("Storing packet with message %s and sequence number %d\n", message.data, nextSeqNum);*/
                        nextSeqNum=FlipBits(nextSeqNum);

                    }


        }
        else if(Ready==true && isRetransmission==true)
        {
                struct pkt packet;
                packet.acknum=0;
                packet.seqnum=LastCorrectlySent;
                strncpy(packet.payload,LastMessageSent.data,sizeof(LastMessageSent.data));
                packet.checksum=checkSum(packet);
                printf("A: Retransmitting packet with message %s, sequence number %d, checksum %d\n", packet.payload, packet.seqnum, packet.checksum);
                Ready=false;
                        /*printf("A: Retransmitting packet with message %s and sequence number %d \n", LastPacketSent.data, LastPacketSent.seqnum);*/
                tolayer3(0,packet);
                starttimer(0,TIMEOUT);
                /*nextSeqNum=FlipBits(nextSeqNum);*/
                /*LastMessage=message;*/
                /*LastPacketSent=packet;*/
                RetransmittedCount++;
                /*isRetransmission=false;*/
        }
  else if(Ready==false)
        {
                struct pkt packet;
                packet.acknum=0;
                packet.seqnum=nextSeqNum;
                strncpy(packet.payload, message.data, sizeof(message.data));

                /*printf("A: Buffer not ready. Packet stored with seqnum %d at %f\n", nextSeqNum, get_sim_time());*/

                packet.checksum=checkSum(packet);
                BufferedPackets.push_back(packet);
                nextSeqNum=FlipBits(nextSeqNum);
                DroppedPacketsCount++;
        }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
        /*printf("A: A Input called at %f\n", get_sim_time());*/
        int y=check_checkSum(packet);
        if(packet.acknum==ExpectedAck && y==packet.checksum)
        {
                ExpectedAck=FlipBits(ExpectedAck);
                Ready=true;
                stoptimer(0);
                printf("A: Received Ack for seqnum %d, checksum %d\n", packet.acknum, packet.checksum);
                isRetransmission=false;
        }
        else
        {
                invalidAcks++;
                printf("A: Received an invalid Ack with acknum %d, checksum %d\n", packet.acknum, y);
                isRetransmission=true;
        }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
        Ready=true;
        isRetransmission=true;
        printf("A: Timer Interrupted at %f\n", get_sim_time());
        A_output(LastMessage);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{

        nextSeqNum=0;
        ExpectedAck=0;
        isRetransmission=false;
        Ready=true;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
        int z,y=0;

        if(packet.seqnum==RequestedSeq)
        {
                y=1;
        }

        int s=checkSum(packet);
        if(s==packet.checksum)
        {
                z=1;
        }
        else
        {
                z=0;
        }

        if(packet.seqnum==RequestedSeq && s==packet.checksum)
        {
                char Message_toLayer5[20];
                RequestedSeq=FlipBits(RequestedSeq);
                strncpy(Message_toLayer5,packet.payload,sizeof(packet.payload));
                tolayer5(1,Message_toLayer5);
                /*Generate Ack Packet*/
                struct pkt Ack;
                Ack.seqnum=packet.seqnum;
                Ack.acknum=packet.seqnum;
                strncpy(Ack.payload,packet.payload,sizeof(packet.payload));
                Ack.checksum=checkSum(Ack);
                tolayer3(1,Ack);
                LastAckSent=Ack;
                noofAcks++;
                printf("B: Received packet with message %s, seqnum %d, checksum %d sending ack acknum %d and checksum %d\n", packet.payload,packet.seqnum, packet.checksum, Ack.acknum, Ack.checksum);

                }

        else
        {
                tolayer3(1,LastAckSent);
                duplicateAcksSent++;
                printf("B: Received invalid Packet/Duplicate packet with seqnum %d and checksum %d \n", packet.seqnum, s);
        }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
        RequestedSeq=0;
}


