#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <getopt.h>
#include <ctype.h>
#include <cstring>
using namespace std;
/* ******************************************************************
 *  ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose
 *     This code should be used for PA2, unidirectional data transfer 
 *        protocols (from A to B). Network properties:
 *           - one way network delay averages five time units (longer if there
 *                are other messages in the channel for GBN), but can be larger
 *                   - packets can be corrupted (either the header or the data portion)
 *                        or lost, according to user-defined probabilities
 *                           - packets will be delivered in the order in which they were sent
 *                                (although some can be lost).
 *                                 **********************************************************************/

#define BIDIRECTIONAL 0    /* change to 1 if you're doing extra credit */
/* and write a routine called B_output */

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
/*
struct msg {
	char data[20];
};
*/
/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
/*
struct pkt {
	int seqnum;
	int acknum;
	int checksum;
	char payload[20];
};
*/
/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* Statistics 
 *  * Do NOT change the name/declaration of these variables
 *   * You need to set the value of these variables appropriately within your code.
 *    * */
int A_application = 0;
int A_transport = 0;
int B_application = 0;
int B_transport = 0;
void tolayer5(int AorB,char *datasent);
void tolayer3(int AorB,struct pkt packet);
void starttimer(int AorB,float increment);
void stoptimer(int AorB);

/* Globals 
 *  * Do NOT change the name/declaration of these variables
 *   * They are set to zero here. You will need to set them (except WINSIZE) to some proper values.
 *    * */
float TIMEOUT;
/*
 * int WINSIZE ;        Not applicable to ABT
 * int SND_BUFSIZE = 0; Not applicable to ABT
 * int RCV_BUFSIZE = 0; Not applicable to ABT
 * */

int A_seqnum;		   
int A_ignored;		   
bool A_ready;		  
bool reSend;		   
struct msg A_lastsent; 


int B_seqnum;		   
struct pkt B_lastack;  

int checkSum(struct pkt packet)                             
{
	int checksum;
	checksum=packet.acknum+packet.seqnum+packet.checksum;
	for(int i=0;i<sizeof(packet.payload);i++)
	{
		checksum=checksum+packet.payload[i];
	}

	return ~checksum;
}

bool notCorrupt(struct pkt packet)                         
{
	int result=checkSum(packet);
	if(result==0)
	{
		return true;
	}
	else
	{
		return false;
	}
}
int seqnumChange(int seqnum)								
{
	if(seqnum==1)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message) 
{

	if(reSend==false)
	{
		A_application++;
		
	}

	if(A_ready==true)		
	{
		pkt packet;
		packet.acknum=0;
		packet.seqnum=A_seqnum;
		strncpy(packet.payload,message.data,sizeof(packet.payload));
		packet.checksum=0;
		packet.checksum=checkSum(packet);
		A_ready=false;
		A_transport++;
		A_lastsent=message;
		tolayer3(0,packet);
		
		starttimer(0,TIMEOUT);
	}

	else                 
	{
		
		if(reSend==false)
		{
			A_ignored++;
		}
	}
	if(reSend==true)
	{
		reSend=false;
	}
	
}


/* called when A's timer goes off */
void A_timerinterrupt() 
{
	
	A_ready=true;
	reSend=true;
	A_output(A_lastsent);
	
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() //ram's comment - changed the return type to void.
{
	TIMEOUT = 8.0;
	A_seqnum=0;		   
	A_ignored=0;	   
	A_ready = true;	   
	reSend=false;	   
}


/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
	B_transport++;
	if(packet.seqnum==B_seqnum && notCorrupt(packet)==true)
	{
		
		B_application++;
		tolayer5(1,packet.payload);
		struct pkt B_ackpacket;
		B_ackpacket.acknum=B_seqnum;
		B_ackpacket.seqnum=B_seqnum;
		B_ackpacket.checksum=0;
		B_ackpacket.checksum=checkSum(B_ackpacket);
		B_lastack=B_ackpacket;
		/*********send ack********/
		tolayer3(1,B_ackpacket);
		B_seqnum=seqnumChange(B_seqnum);
	}
	else
	{
		
		/*********send last ack********/
		tolayer3(1,B_lastack);
	}
	
}

/* called when B's timer goes off */
void B_timerinterrupt() 
{
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() 
{
	B_seqnum=0;

	B_lastack.acknum=seqnumChange(B_seqnum);
	B_lastack.seqnum=B_lastack.acknum;
	B_lastack.checksum=0;
	B_lastack.checksum=checkSum(B_lastack);
}
