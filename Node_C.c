#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <netdb.h>

#define N_PEER 3
#define PORT1 3000
#define PORT2 4000
#define PORT3 6000
#define MAX 4
#define INFINITY 9999

//FUNCTION DEFINITIONS
void parsed_args(int argc, char **argv);
void* Start_LSPF();
int HASH(char x);
void* Recv_LSPF();
void* Forward_LSPF(char fwd_msg[]);
void* Calculate_Route();
int HASH2(char x);
void dijkstra(int G[MAX][MAX], int n, int start_node);
char Reverse_Hash2(int x);

// GLOBAL VARIABLES
int sock;
char buffer[100];
struct sockaddr_in self_addr;
struct sockaddr_in peer_list[N_PEER];
pthread_t recv_forward;
static int count_LSDB=0;
pthread_mutex_t lock;
//int Neighbour[N_PEER][4];

struct peers
{
	char Destination;
	char Source;
	int Cost;
	char Through;
}Neighbour[N_PEER], LSDB[10];

int main(int argc, char **argv)
{
	int i;
	sock=socket(AF_INET, SOCK_DGRAM, 0);
	if(sock<0)
	{
		printf("Error in creating Socket!\n");
	}
	else
	{
		printf("Socket Created!\n");
	}
	

	parsed_args(argc,argv);


	if(bind(sock, (struct sockaddr *)&self_addr, 
	sizeof(self_addr) )<0)
	{
		printf("Error in Binding!\n");
	}

	/*gets(buffer);
	for(i=0;i<2;i++)
	{

		int status=sendto(sock, buffer, sizeof(buffer),0, (struct sockaddr *) &(peer_list[i]), sizeof(struct sockaddr_in));

		if(status==-1)
		{
			printf("Message Not send!\n");
	
		}
		else
		{
			printf("Message Sent!\n");
		}

	}*/
	//Start_LSPF();
	pthread_create(&recv_forward, NULL, Recv_LSPF(), NULL);
	pthread_detach(recv_forward);
	
}

void parsed_args(int argc, char **argv)
{
	if(argc!=2)
	{
		printf("Argument Count is not correct!\n Closing Socket!\n");
		close(sock);
	}
	else
	{
		short self_port=atoi(argv[1]);
		self_addr.sin_family=AF_INET;
		self_addr.sin_addr.s_addr=htonl(INADDR_ANY);
		self_addr.sin_port=htons(self_port);

		short peer_port=PORT1;
		peer_list[0].sin_family=AF_INET;
		peer_list[0].sin_addr.s_addr=htonl(INADDR_ANY);
		peer_list[0].sin_port=htons(peer_port);

		peer_port=PORT2;
		peer_list[1].sin_family=AF_INET;
		peer_list[1].sin_addr.s_addr=htonl(INADDR_ANY);
		peer_list[1].sin_port=htons(peer_port);

		peer_port=PORT3;
		peer_list[2].sin_family=AF_INET;
		peer_list[2].sin_addr.s_addr=htonl(INADDR_ANY);
		peer_list[2].sin_port=htons(peer_port);

		Neighbour[0].Destination='A';//Destination
		Neighbour[0].Source='C'; //Source
		Neighbour[0].Cost=4; //Cost
		Neighbour[0].Through='A'; //Through

		
		Neighbour[1].Destination='B';//Destination
		Neighbour[1].Source='C'; //Source
		Neighbour[1].Cost=5; //Cost
		Neighbour[1].Through='B'; //Through

		
		Neighbour[2].Destination='D';//Destination
		Neighbour[2].Source='C'; //Source
		Neighbour[2].Cost=2; //Cost
		Neighbour[2].Through='D'; //Through

		
		
	}


}

void* Start_LSPF()
{
	char lspf_buff[5];
	int i,j,k,status;
	int no_of_peers=N_PEER;
	for(i=0;i<no_of_peers;i++)
	{

		bzero(lspf_buff,sizeof(lspf_buff));
		lspf_buff[0]=Neighbour[i].Destination;
		lspf_buff[1]=Neighbour[i].Source;
		lspf_buff[2]=Neighbour[i].Cost;
		lspf_buff[3]=Neighbour[i].Through;
		lspf_buff[4]='\0';
		k=0;
		for(k=0;k<no_of_peers;k++)
		{
			if(HASH(lspf_buff[0])!=peer_list[k].sin_port)
			{
				pthread_mutex_lock(&lock);
			 status=sendto(sock, lspf_buff, sizeof(lspf_buff),0, (struct sockaddr *) &(peer_list[k]), sizeof(struct sockaddr_in));
				pthread_mutex_unlock(&lock);
				if(status==-1)
				{
					printf("Message Not send!\n");
	
				}
				else
				{
					printf("LSPF Sent!\n");
				}
			}
		}
	}

	bzero(buffer,sizeof(buffer));
	buffer[0]='E';
	buffer[1]='N';
	buffer[2]='D';
	buffer[3]='E';
	buffer[4]='N';
	buffer[5]='D';
	buffer[6]='E';
	buffer[7]='N';
	buffer[8]='D';
	buffer[9]='\0';

	k=0;
	for(k=0;k<no_of_peers;k++)
	{
		pthread_mutex_lock(&lock);
		 status=sendto(sock, buffer, sizeof(buffer),0, (struct sockaddr *) &(peer_list[k]), sizeof(struct sockaddr_in));
		pthread_mutex_unlock(&lock);
			if(status==-1)
			{
				printf("Message Not send!\n");

			}
			else
			{
				printf("Termination of LSPF Sent!\n");
			}
		
	}

	pthread_create(&recv_forward, NULL, Recv_LSPF(), NULL);
	pthread_detach(recv_forward);
	
}

int HASH(char x)
{
	int ret_val=-1;

	if(x=='A')
	{
		ret_val=PORT1;
	}
	if(x=='B')
	{
		ret_val=PORT2;
	}
	if(x=='C')
	{
		ret_val=-1;
	}

	if(x=='D')
	{
		ret_val=PORT3;
	}
	
	return(htons(ret_val));
	
}


void* Recv_LSPF()
{
	
	char lspf_msg[15];
	int status,flag1=1,flag2=1,j,k;
	int no_of_peers=N_PEER;

	while(1)
	{
	bzero(lspf_msg,sizeof(lspf_msg));
	pthread_mutex_lock(&lock);
	status=recvfrom(sock, lspf_msg, sizeof(lspf_msg), 0, NULL, 10);
	pthread_mutex_unlock(&lock);
	if(strcmp(lspf_msg,"ENDENDENDEND")==0)
	{
		pthread_create(&recv_forward, NULL, Calculate_Route(), NULL);
		pthread_detach(recv_forward);
	}
	else if(strcmp(lspf_msg,"ENDEND")==0)
	{
		pthread_create(&recv_forward, NULL, Start_LSPF(), NULL);
		pthread_detach(recv_forward);
	}
	else{
	j=0;
	for(j=0;j<no_of_peers;j++)
	{
		if(lspf_msg[0]==Neighbour[j].Destination && lspf_msg[1]==Neighbour[j].Source)
		{
			flag1=0;
		}
	}
	
	k=0;
	for(k=0;k<count_LSDB;k++)
	{
		if(lspf_msg[0]==LSDB[k].Destination && lspf_msg[1]==LSDB[k].Source)
		{
			flag2=0;
		}
	}

	if(flag1==1&&flag2==1)
	{
	if(strcmp(lspf_msg,"END")!=0&&strcmp(lspf_msg,"ENDEND")!=0&&strcmp(lspf_msg,"ENDENDEND")!=0&&strcmp(lspf_msg,"ENDENDENDEND")!=0)
		{
			LSDB[count_LSDB].Destination=lspf_msg[0];
			LSDB[count_LSDB].Source=lspf_msg[1];
			LSDB[count_LSDB].Cost=lspf_msg[2];
			LSDB[count_LSDB].Through=lspf_msg[3];
			count_LSDB++;

			pthread_create(&recv_forward, NULL, Forward_LSPF(lspf_msg), NULL);
			pthread_detach(recv_forward);
		}

		
	}
	}//end of else
	}//end of while;
}

void* Forward_LSPF(char fwd_msg[])
{
	int j=0, no_of_peers=N_PEER, status;
	for(j=0;j<no_of_peers;j++)
	{
if(fwd_msg[0]!=Neighbour[j].Destination&&fwd_msg[1]!=Neighbour[j].Source&&fwd_msg[0]!=Neighbour[j].Source&&fwd_msg[1]!=Neighbour[j].Destination)
		{
			pthread_mutex_lock(&lock);
			status=sendto(sock, fwd_msg, sizeof(fwd_msg),0, (struct sockaddr *) &(peer_list[j]), sizeof(struct sockaddr_in));
			pthread_mutex_unlock(&lock);
			if(status==-1)
			{
				printf("Message Not send!\n");

			}
			else
			{
				printf("LSPF Forwarded to %d peer Sent!\n", j);
			}
		}
	}
	
	
	
		pthread_create(&recv_forward, NULL, Recv_LSPF(), NULL);
		pthread_detach(recv_forward);
	
}

void* Calculate_Route()
{

	int i,j,k,no_of_peers=N_PEER;
	int G[4][4], n=4, start_node=2 ;

	printf("Job of calculating route in Progress...\n");
	printf("Preview of LSDB...\n");
	
	for(i=0;i<no_of_peers;i++)
	{
		printf("%c %c %d %c\n", Neighbour[i].Destination, Neighbour[i].Source, Neighbour[i].Cost, Neighbour[i].Through);
	}

	for(j=0;j<count_LSDB;j++)
	{
		printf("%c %c %d %c\n",LSDB[j].Destination, LSDB[j].Source, LSDB[j].Cost, LSDB[j].Through);
	}

	i=0;
	j=0;
	k=0;
	
	for(i=0;i<n;i++)
	{
		j=0;
		for(j=0;j<n;j++)
		{
			G[i][j]=0;
		}
	}

	i=0;
	j=0;
	k=0;

	for(i=0;i<no_of_peers;i++)
	{
		//printf("1\n");
		j=HASH2(Neighbour[i].Source);
		k=HASH2(Neighbour[i].Destination);
		//printf("2\n");
		G[j][k]=Neighbour[i].Cost;
		G[k][j]=Neighbour[i].Cost;
	}

	i=0;
	for(i=0;i<count_LSDB;i++)
	{
		//printf("3\n");
		j=HASH2(LSDB[i].Source);
		k=HASH2(LSDB[i].Destination);
		if(j==-1||k==-1)
		{
			printf("error\n");
		}
		//printf("4\n");
		G[j][k]=LSDB[i].Cost;
		G[k][j]=LSDB[i].Cost;
	}
	
	

	dijkstra(G, n, start_node);


}

int HASH2(char x)
{
	int ret_val=-1;
	if(x=='A')
	{
		ret_val=0;
	}
	if(x=='B')
	{
		ret_val=1;
	}
	if(x=='C')
	{
		ret_val=2;
	}
	if(x=='D')
	{
		ret_val=3;
	}
	return(ret_val);

}

void dijkstra(int G[MAX][MAX], int n, int startnode)
{

	FILE *file_ptr=fopen("Routing_Table_C.txt","w");
        int cost[MAX][MAX], distance[MAX], pred[MAX];
        int visited[MAX], count, mindistance, nextnode, i,j;
        for(i=0;i < n;i++)
                for(j=0;j < n;j++)
                        if(G[i][j]==0)
                                cost[i][j]=INFINITY;
                        else
                                cost[i][j]=G[i][j];

        for(i=0;i< n;i++)
        {
                distance[i]=cost[startnode][i];
                pred[i]=startnode;
                visited[i]=0;
        }
        distance[startnode]=0;
        visited[startnode]=1;
        count=1;
        while(count < n-1){
                mindistance=INFINITY;
                for(i=0;i < n;i++)
                        if(distance[i] < mindistance&&!visited[i])
                        {
                                mindistance=distance[i];
                                nextnode=i;
                        }
                visited[nextnode]=1;
                for(i=0;i < n;i++)
                        if(!visited[i])
                                if(mindistance+cost[nextnode][i] < distance[i])
                                {
                                        distance[i]=mindistance+cost[nextnode][i];
                                        pred[i]=nextnode;
                                }
                        count++;
        }
	

	
	//printf("Routing Table: \n");
	fprintf(file_ptr,"Routing Table: \n");
        for(i=0;i < n;i++)
                if(i!=startnode)
                {
                        //printf("\nDistance of %c = %d", Reverse_Hash2(i), distance[i]);
			fprintf(file_ptr,"\nDistance of %c = %d", Reverse_Hash2(i), distance[i]);
                        //printf("\nPath = %c", Reverse_Hash2(i));
			fprintf(file_ptr,"\nPath = %c", Reverse_Hash2(i));
                        j=i;
                        do
                        {
                                j=pred[j];
                               // printf(" <-%c", Reverse_Hash2(j));
				fprintf(file_ptr," <-%c", Reverse_Hash2(j));
                        }
                        while(j!=startnode);
                }


	printf("Routing_Table_C.txt Successfully created!\n");

	fclose(file_ptr);

	//pthread_detach(recv_forward);
}

char Reverse_Hash2(int x)
{
	char ret_char='\0';
	if(x==0)
	{
		ret_char='A';
	}
	if(x==1)
	{
		ret_char='B';
	}
	if(x==2)
	{
		ret_char='C';
	}
	if(x==3)
	{
		ret_char='D';
	}


	return(ret_char);


}
