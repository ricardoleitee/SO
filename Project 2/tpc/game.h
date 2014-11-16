/*
	GAME HEADERS
*/

//=================================================================================
//Includes
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <limits.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <stdio_ext.h>
#include <errno.h>

extern unsigned int PNUM;

//=================================================================================
//Struct for each card
typedef struct {
	char c[3];
	char s;
} card;

//=================================================================================
//Struct with player information
typedef struct {
	unsigned int num; //player number
	unsigned int FIFOready; //signals FIFO is ready for reading
	unsigned int pexit;
	char nickname[PATH_MAX]; //player's nickname
	char FIFOname[PATH_MAX]; //player's FIFO path
	card hand[13]; //player's cards
	pthread_t *keyboard;
	pthread_t *game_sync;
} player;

//=================================================================================
//Struct for SharedMemory
typedef struct {
	pthread_mutex_t lock_shm; //protects critical shared memory
	pthread_mutex_t lock_plays;
	pthread_mutex_t lock_log;
	pthread_cond_t player_cond; //
	player players[4]; //array of players in game
	unsigned int nplayers; //max number of players
	unsigned int reg_players; //number of registered players in the game
	unsigned int turn; //who's turn it is (0<=turn<=nplayers)
	unsigned int roundnumber; //current round number
	unsigned int dealer; //player number of dealer (unnecessary?)
	card tablecards[4]; //current cards on table
	char log_name[PATH_MAX];
	card p_tablecards[4]; //previous cards on table
	unsigned int turnplayed; //signals if player has played this round
} SharedMem;

//===============================================================================
//Struct for readDealerHand
typedef struct {
	char* shm_name; //shared memory name
	unsigned int num; //player number
	char* hand; //string for receiving hand from FIFO
} readHandArgs;

//=================================================================================
//HandToString()
void handToString(SharedMem* shm, unsigned int num, char *string) {
	char hand_string[5] = {'\0'};
	char final_string[1000] = {'\0'};
	//char *phand_string = &hand_string[0];
	unsigned int i=0;
	unsigned int c=0,s=0,h=0,d=0;
	
	
	for (i=0;i<13;i++){
		if (strcmp(shm->players[num].hand[i].c,"E")==0){
			continue;
		} else {
			if (shm->players[num].hand[i].s=='c'){
				c++;
			}
			if (shm->players[num].hand[i].s=='s'){
				s++;
			}
			if (shm->players[num].hand[i].s=='h'){
				h++;
			}
			if (shm->players[num].hand[i].s=='d'){
				d++;
			}
		}
	}

	for (i=0;i<13;i++){
		if (strcmp(shm->players[num].hand[i].c,"E")==0){
			continue;
		} else {
			if (shm->players[num].hand[i].s=='c'){
				c--;
				if (c>0){
					sprintf(hand_string, "%s%c-", shm->players[num].hand[i].c, shm->players[num].hand[i].s);
					strcat(&final_string[0], hand_string);
				} else if(c==0) {
					sprintf(hand_string, "%s%c / ", shm->players[num].hand[i].c, shm->players[num].hand[i].s);
					strcat(&final_string[0], hand_string);
				}
			}
		}
	}
	for (i=0;i<13;i++){
		if (strcmp(shm->players[num].hand[i].c,"E")==0){
			continue;
		} else {
			if (shm->players[num].hand[i].s=='h'){
				h--;
				if (h>0){
					sprintf(hand_string, "%s%c-",shm->players[num].hand[i].c,shm->players[num].hand[i].s);
					strcat(&final_string[0], hand_string);
				} else if(h==0) {
					sprintf(hand_string, "%s%c / ",shm->players[num].hand[i].c,shm->players[num].hand[i].s);
					strcat(&final_string[0], hand_string);
				}
			}
		}
	}
	for (i=0;i<13;i++){
		if (strcmp(shm->players[num].hand[i].c,"E")==0){
			continue;
		} else {
			if (shm->players[num].hand[i].s=='d'){
				d--;
				if (d>0){
					sprintf(hand_string, "%s%c-",shm->players[num].hand[i].c,shm->players[num].hand[i].s);
					strcat(&final_string[0], hand_string);
				} else if(d==0) {
					sprintf(hand_string, "%s%c / ",shm->players[num].hand[i].c,shm->players[num].hand[i].s);
					strcat(&final_string[0], hand_string);
				}
			}
		}
	}
	for (i=0;i<13;i++){
		if (strcmp(shm->players[num].hand[i].c,"E")==0){
			continue;
		} else {
			if (shm->players[num].hand[i].s=='s'){
				s--;
				if (s>0){
					sprintf(hand_string, "%s%c-",shm->players[num].hand[i].c,shm->players[num].hand[i].s);
					strcat(&final_string[0], hand_string);
				} else if(s==0) {
					sprintf(hand_string, "%s%c / ",shm->players[num].hand[i].c,shm->players[num].hand[i].s);
					strcat(&final_string[0], hand_string);
				}
			}
		}
	}
	strcpy(string, final_string);
}

//=================================================================================
//GetFormattedTimeString()
void getFormattedTimeString(time_t rawtime,char* r_string){

    struct tm *timeinfo;

    char string[21];

    char cm[3]; char* pcm=&cm[0];

    char cd[3]; char* pcd=&cd[0];

    char ch[3]; char* pch=&ch[0];

    char cmin[3]; char* pcmin=&cmin[0];

    char cs[3]; char* pcs=&cs[0];

    timeinfo=localtime(&rawtime);

    int year,month,day,hour,minute,second;

    year=timeinfo->tm_year+1900;

    month=timeinfo->tm_mon+1;

    if (month<=9){

        snprintf(cm,3,"0%d",month);

    } else {

        snprintf(cm,3,"%d",month);

    }

    day=timeinfo->tm_mday;

    if (day<=9){

        snprintf(cd,3,"0%d",day);

    } else {

        snprintf(cd,3,"%d",day);

    }

    hour=timeinfo->tm_hour;

    if (hour<=9){

        snprintf(ch,3,"0%d",hour);

    } else {

        snprintf(ch,3,"%d",hour);

    }

    minute=timeinfo->tm_min;

    if (minute<=9){

        snprintf(cmin,3,"0%d",minute);

    } else {

        snprintf(cmin,3,"%d",minute);

    }

    second=timeinfo->tm_sec;

    if (second<=9){

        snprintf(cs,3,"0%d",second);

    } else {

        snprintf(cs,3,"%d",second);

    }

    snprintf(string,21,"%d-%s-%s %s:%s:%s",year,pcm,pcd,pch,pcmin,pcs);

    char* pstring=&string[0];

    strcpy(r_string,pstring);
}

//=================================================================================
//LogInfo() - Saves info to file.log
void logInfo(SharedMem* shm, time_t when, int who, char* what, char* result) {
	int fd;
	char string[120] = {'\0'};
	char buffer[2] = {'\0'};
	char play_date[30] = {'\0'};
	char who_player[15] = {'\0'};
	char *pstring;	
	
	//Format WHEN string
	pstring = &string[0];
	getFormattedTimeString(when, play_date);

	printf("WHEN string: %s\n", play_date);

	//Format WHO string
	if (strcmp(what, "deal") == 0) {
		strcat(&who_player[0], "Dealer- ");
		strcat(&who_player[0], shm->players[who].nickname);
	} else {
		strcat(&who_player[0], "Player");
		sprintf(buffer,"%d",shm->players[who].num);
		strcat(&who_player[0], buffer);
		strcat(&who_player[0], "-");
		strcat(&who_player[0], shm->players[who].nickname);
	 }

	printf("WHO string: %s\n", who_player);

	//Format string to append
	strcat(pstring, play_date);
	strcat(pstring, "  |  ");
	strcat(pstring, who_player);
	strcat(pstring, "  |  ");
	strcat(pstring, what);
	strcat(pstring, "  |  ");
	strcat(pstring, result);
	strcat(pstring, "\n");

	printf("APPEND string %s\n", pstring);
	printf("logname:%s\n",&shm->log_name[0]);
	pthread_mutex_lock(&shm->lock_log);
	fd = open(&shm->log_name[0],O_WRONLY | O_APPEND,0777);
	if (fd == -1) {
		perror(&shm->log_name[0]);
		fprintf(stderr,"erro a abrir o ficheiro\n");
		exit(EXIT_FAILURE);
	}

	write(fd, pstring, strlen(pstring));
	close(fd);
	pthread_mutex_unlock(&shm->lock_log);

}
//=================================================================================
//Random_in_range() - Select random number within min,max range
int random_in_range(unsigned int min, unsigned int max){
	int base_random = rand(); /* in [0, RAND_MAX] */
	if (RAND_MAX == base_random) return random_in_range(min, max);
	/* now guaranteed to be in [0, RAND_MAX) */
	int range       = max - min,
	remainder   = RAND_MAX % range,
	bucket      = RAND_MAX / range;
  /* There are range buckets, plus one smaller interval
     within remainder of RAND_MAX */
	if (base_random < RAND_MAX - remainder) {
		return min + base_random/bucket;
	} else {
		return random_in_range (min, max);
	}
}

//=================================================================================
//GenerateShuffledDeck() - Generates shuffled deck of 52 cards (includes 8s,9s and 10s)
void generateShuffledDeck(card* cards){
	srand (time(NULL));
	int j=0;
	int i=2;
	//generate cards 2-10 of clubs
	char buf[3]={'\0'};
	for (;i<=10;i++){
		sprintf(buf,"%d",i);
		strcpy(&cards[j].c[0],&buf[0]);
		cards[j].s='c';
		j++;
	}
	i=2;
	//generate cards 2-10 of spades
	for (;i<=10;i++){
		sprintf(buf,"%d",i);
		strcpy(&cards[j].c[0],&buf[0]);
		cards[j].s='s';
		j++;
	}
	i=2;
	//generate cards 2-10 of hearts
	for (;i<=10;i++){
		sprintf(buf,"%d",i);
		strcpy(&cards[j].c[0],&buf[0]);
		cards[j].s='h';
		j++;
	}
	i=2;
	//generate cards 2-10 of diamonds
	for (;i<=10;i++){
		sprintf(buf,"%d",i);
		strcpy(&cards[j].c[0],&buf[0]);
		cards[j].s='d';
		j++;
	}
	char face[3]={'J','\0','\0'};
	//generate jacks
	for (i=0;i<4;i++){
		strcpy(&cards[j].c[0],&face[0]);
		if (i==0){
			cards[j].s='c';
		}
		if (i==1){
			cards[j].s='s';
		}
		if (i==2){
			cards[j].s='h';
		}
		if (i==3){
			cards[j].s='d';
		}
		j++;
	}
	strcpy(&face[0],"Q");
	//generate queens
	for (i=0;i<4;i++){
		strcpy(&cards[j].c[0],&face[0]);
		if (i==0){
			cards[j].s='c';
		}
		if (i==1){
			cards[j].s='s';
		}
		if (i==2){
			cards[j].s='h';
		}
		if (i==3){
			cards[j].s='d';
		}
		j++;
	}
	strcpy(&face[0],"K");
	//generate kings
	for (i=0;i<4;i++){
		strcpy(&cards[j].c[0],&face[0]);
		if (i==0){
			cards[j].s='c';
		}
		if (i==1){
			cards[j].s='s';
		}
		if (i==2){
			cards[j].s='h';
		}
		if (i==3){
			cards[j].s='d';
		}
		j++;
	}
	//generate aces
	strcpy(&face[0],"A");
	for (i=0;i<4;i++){
		strcpy(&cards[j].c[0],&face[0]);
		if (i==0){
			cards[j].s='c';
		}
		if (i==1){
			cards[j].s='s';
		}
		if (i==2){
			cards[j].s='h';
		}
		if (i==3){
			cards[j].s='d';
		}
		j++;
	}

	/* //Print unshuffled cards
	printf("Unshuffled:\n");
	for (i=0;i<52;i++){
		printf("%s%c\n",&cards[i].c[0],cards[i].s);
	}
	*/

	//shuffle deck (using Fisher-Yates Shuffle a.k.a. Knuth Shuffle)
	card aux;
	j--;
	for (;j>=0;j--){
		unsigned int k = random_in_range(0,52);
		strcpy(&aux.c[0],&cards[j].c[0]);
		aux.s=cards[j].s;
		strcpy(&cards[j].c[0],&cards[k].c[0]);
		cards[j].s=cards[k].s;
		strcpy(&cards[k].c[0],&aux.c[0]);
		cards[k].s=aux.s;
	}

	/* //Print shuffled cards
	printf("Shuffled:\n");
	for (i=0;i<52;i++){
		printf("%s%c\n",&cards[i].c[0],cards[i].s);
	}
	*/
	

}

//=================================================================================
//ReadSHM()
void readSHM(SharedMem* shm){
	printf("------READING SHARED MEMORY------\n");
	printf("SHM->nplayers:%d\n",shm->nplayers);
	printf("SHM->reg_players:%d\n",shm->reg_players);
	printf("SHM->dealer:%d\n",shm->dealer);
	unsigned int i=0;
	for (;i<shm->nplayers;i++){
		printf("Player[%d]->num:%d\n",i,shm->players[i].num);
		printf("Player[%d]->nickname:%s\n",i,shm->players[i].nickname);
		printf("Player[%d]->FIFO_P:%s\n",i,shm->players[i].FIFOname);
	}
	printf("------FINISHED READING SHARED MEMORY------\n");
}

//=================================================================================
//SyncSharedMemoryObjects()
void syncSharedMemoryObjects(SharedMem *shm) {
	//Initialize Mutexs in shared memory
	pthread_mutexattr_t mattr;
	pthread_mutexattr_init(&mattr);
	pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);

	pthread_mutex_init(&shm->lock_shm, &mattr);
	pthread_mutex_init(&shm->lock_plays, &mattr);

	//Initialize CVs in shared memory
	pthread_condattr_t cattr;
	pthread_condattr_init(&cattr);
	pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);

	pthread_cond_init(&shm->player_cond, &cattr);
}
//=================================================================================
//DestroySharedMemory()
void destroySharedMemory(SharedMem *shm, char* shm_name) {
	char SHM_NAME[50] = {'\0'};
	char *ps;

	//Create SHM_NAME correctly
	ps = &SHM_NAME[0];
	strcat(ps,"/");
	strcat(ps,shm_name); // name = /shmname

	if (munmap(shm, sizeof(SharedMem)) < 0) {
		perror("Failure in munmap()\n");
		exit(EXIT_FAILURE);
	}
	if (shm_unlink(SHM_NAME) < 0) {
		perror("Failure in shm_unlink()\n");
		exit(EXIT_FAILURE);
	}
}

//=================================================================================
//Check if player pname is dealer
unsigned int isDealer(SharedMem* shm, char* pname){
	unsigned int i;
	for (i=0;i<shm->reg_players;i++){
		if (strcmp(pname,shm->players[i].nickname)==0){
			if (shm->players[i].num==0)
				return 1;
		}
	}
	return 0;
}

//=================================================================================
//ReadLine()
int readline(int fd, char *str) { 
	int n;
	do{
		n = read(fd,str,1); 
	} while (n>0 && *str++ != '\0'); 

	return (n>0); 
}

//=================================================================================
//PrintDeck() - Prints 52 card deck
void printDeck(card* deck){
	unsigned int i;
	for (i=0;i<52;i++){
		printf("%d:%s%c\n",i,deck[i].c,deck[i].s);
	}
}

//=================================================================================
//ReadFIFO() - Dealer reads player FIFO
void readFIFO(SharedMem *shm,unsigned int num){
	int fd;
	char str[100];
	pthread_mutex_lock(&shm->lock_shm);
	shm->players[num].FIFOready=1;
	pthread_mutex_unlock(&shm->lock_shm);
	mkfifo(shm->players[num].FIFOname,0777);
	fd=open(shm->players[num].FIFOname,O_RDONLY);
	//printf("ready to read\n");
	readline(fd,str);
	close(fd);
	pthread_mutex_lock(&shm->lock_shm);
	shm->players[num].FIFOready=0;
	pthread_mutex_unlock(&shm->lock_shm);
}

//=================================================================================
//ReadHandFIFO() - Player received hand through FIFO
void readHandFIFO(SharedMem *shm,unsigned int num,char* hand){
	int fd;
	char str[130]={'\0'};
	pthread_mutex_lock(&shm->lock_shm);
	shm->players[num].FIFOready=1;
	pthread_mutex_unlock(&shm->lock_shm);
	mkfifo(shm->players[num].FIFOname,0777);
	fd=open(shm->players[num].FIFOname,O_RDONLY);
	char** s_hands=(char**)malloc(26*sizeof(char*));
	unsigned int i=0;
	for (i=0;i<26;i++){
		s_hands[i]=(char*)malloc(3*sizeof(char));
	}
	readline(fd,str);	
	close(fd);
	pthread_mutex_lock(&shm->lock_shm);
	shm->players[num].FIFOready=0;
	pthread_mutex_unlock(&shm->lock_shm);
	strcpy(hand,str);
}

//=================================================================================
//ReadHandToStruct() - Convert hand string to card array in player struct
void readHandToStruct(SharedMem *shm,char* s_hand){
	char* pch;
	card hand[13];
	unsigned int i=0;
	char** aux=(char**)malloc(13*sizeof(char*));
	for(;i<13;i++){
		aux[i]=(char*)malloc(5*sizeof(char));
	}
	i=0;
	pch=strtok(s_hand,",");
	strcpy(aux[i],pch);
	while(pch!=NULL){
		i++;
		if(i==13){
			break;
		}
		pch = strtok(NULL,",");
		strcpy(aux[i],pch);
		//printf("pch:%s\n",pch);
	}
	
	for (i=0;i<13;i++){
		//printf("aux[%d]:%s\n",i,aux[i]);
		pch=strtok(aux[i]," ");
		if (i==0){
			strcpy(hand[i].c,pch+1);
		} else {
			strcpy(hand[i].c,pch);
		}
		//printf("pch:%s\n",pch);
		unsigned int k=0;
		while(pch!=NULL){
			k++;
			if(k==2){
				break;
			}
			pch=strtok(NULL," ");
			hand[i].s=pch[0];
			//printf("pch:%s\n",pch);
		}
	}

/*	for (i=0;i<13;i++){
		printf("card[%d]:%s%c\n",i,hand[i].c,hand[i].s);
	}*/

	for (i=0;i<13;i++){
		strcpy(shm->players[PNUM].hand[i].c,hand[i].c);
		shm->players[PNUM].hand[i].s=hand[i].s;
		/*printf("card[%d]=%s\n",i,shm->players[PNUM].hand[i].c,shm->players[PNUM].hand[i].s);*/
	}


}

//=================================================================================
//PrintHand()
void printHand(SharedMem *shm,unsigned int num){
	unsigned int i=0;
	unsigned int c=0,s=0,h=0,d=0;

	printf("Your hand: ");
	
	for (i=0;i<13;i++){
		if (strcmp(shm->players[num].hand[i].c,"E")==0){
			continue;
		} else {
			if (shm->players[num].hand[i].s=='c'){
				c++;
			}
			if (shm->players[num].hand[i].s=='s'){
				s++;
			}
			if (shm->players[num].hand[i].s=='h'){
				h++;
			}
			if (shm->players[num].hand[i].s=='d'){
				d++;
			}
		}
	}

	for (i=0;i<13;i++){
		if (strcmp(shm->players[num].hand[i].c,"E")==0){
			continue;
		} else {
			if (shm->players[num].hand[i].s=='c'){
				c--;
				if (c>0){
					printf("%s%c-",shm->players[num].hand[i].c,shm->players[num].hand[i].s);
				} else if(c==0) {
					printf("%s%c / ",shm->players[num].hand[i].c,shm->players[num].hand[i].s);
				}
			}
		}
	}
	for (i=0;i<13;i++){
		if (strcmp(shm->players[num].hand[i].c,"E")==0){
			continue;
		} else {
			if (shm->players[num].hand[i].s=='h'){
				h--;
				if (h>0){
					printf("%s%c-",shm->players[num].hand[i].c,shm->players[num].hand[i].s);
				} else if(h==0) {
					printf("%s%c / ",shm->players[num].hand[i].c,shm->players[num].hand[i].s);
				}
			}
		}
	}
	for (i=0;i<13;i++){
		if (strcmp(shm->players[num].hand[i].c,"E")==0){
			continue;
		} else {
			if (shm->players[num].hand[i].s=='d'){
				d--;
				if (d>0){
					printf("%s%c-",shm->players[num].hand[i].c,shm->players[num].hand[i].s);
				} else if(d==0) {
					printf("%s%c / ",shm->players[num].hand[i].c,shm->players[num].hand[i].s);
				}
			}
		}
	}
	for (i=0;i<13;i++){
		if (strcmp(shm->players[num].hand[i].c,"E")==0){
			continue;
		} else {
			if (shm->players[num].hand[i].s=='s'){
				s--;
				if (s>0){
					printf("%s%c-",shm->players[num].hand[i].c,shm->players[num].hand[i].s);
				} else if(s==0) {
					printf("%s%c / ",shm->players[num].hand[i].c,shm->players[num].hand[i].s);
				}
			}
		}
	}

	printf("\n");
}

//=================================================================================
//DealerOverTurn()
unsigned int dealerOverTurn(SharedMem* shm) {

	if (shm->turn == shm->players[0].num) {
		return 1;
	}
	return 0;
}

void printPlayers(SharedMem* shm){
	unsigned int i;
	for (i=0;i<shm->nplayers;i++){
		printf("%d:%s\n",shm->players[i].num,shm->players[i].nickname);
	}
}

//=================================================================================
//AllPlayersReady()
unsigned int allPlayersReady(SharedMem* shm){
	if (shm->reg_players==shm->nplayers)
		return 1;
	return 0;
}

//=================================================================================
//WriteHandInFIFO()
void writeHandInFIFO(SharedMem* shm,card *deck, unsigned int num, int cards_limit_min, int cards_limit_max) {
	unsigned int a;
	char message[130]={'\n'};
	int fd;
	//printf("turn->num=%d\n",num);
	pthread_mutex_lock(&shm->lock_shm);
	shm->turn=num;
	pthread_mutex_unlock(&shm->lock_shm);

	do{
		sleep(0);
	} while  (shm->players[num].FIFOready==0);

	//printf("%s FIFOReady: %d\n",shm->players[num].nickname,shm->players[num].FIFOready);

	do {
		fd=open(shm->players[num].FIFOname,O_WRONLY | O_NONBLOCK);
	} while (fd == -1);

	char buf[13]={'\n'};
	char** s_cards=(char**)malloc(26*sizeof(char*));
	for (a=0;a<26;a++){
		s_cards[a]=(char*)malloc(3*sizeof(char));
	}
	char* pbuf=&buf[0];
	for (a=cards_limit_min ; a<cards_limit_max ; a++) {
/*		strcat(message,deck[a].c);
		char* ps;
		ps=&deck[a].s;
		strcat(message,ps);*/
		sprintf(buf,"%s %c,",deck[a].c,deck[a].s);
		strcat(message,pbuf);
		memset(pbuf,'\n',13);
	}
	strcat(message,"\n");
	//printf("message to send: %s",message);
	write(fd,message,strlen(message)+1);

	if (fd==-1){
		perror("write error: ");
	}

	//printf("written hand player\n");
	//printf("turn:%d\n",shm->turn);
	close(fd);
}

//=================================================================================
//WriteFIFO() - Players sends information through FIFO
void writeFIFO(SharedMem* shm,char* fifoname){
	char message[100];
	int fd,messagelen;

	if (strcmp(shm->players[2].FIFOname,fifoname)==0){
		sprintf(message,"Hello from %s\n",shm->players[2].nickname);
	}
	if (strcmp(shm->players[1].FIFOname,fifoname)==0){
		sprintf(message,"Hello from %s\n",shm->players[1].nickname);
	}
	if (strcmp(shm->players[3].FIFOname,fifoname)==0){
		sprintf(message,"Hello from %s\n",shm->players[3].nickname);
	}

	messagelen=strlen(message)+1;

	do{
		fd=open(fifoname,O_WRONLY | O_NONBLOCK);
	} while (fd==-1);

	write(fd,message,messagelen);

	if (fd==-1){
		perror("write error: ");
	}

	//printf("written message\n");
	close(fd);
}

//=================================================================================
//CheckArgs() - check arguments for existing shared memory space
int checkArgs(SharedMem* shm, char* pname, int nplayers){
	unsigned int i;

	if (nplayers!=shm->nplayers){
		fprintf(stderr,"Error: Different max player number for selected table.\n");
		exit(EXIT_FAILURE);
	}

	if (shm->reg_players==shm->nplayers){
		fprintf(stderr,"Error: Max number of players reached, please use a different table.\n");
		exit(EXIT_FAILURE);
	}

	for (i=0;i<shm->reg_players;i++){
		if (strcmp(shm->players[i].nickname,pname)==0){
			fprintf(stderr, "Error: Player name already exists: %s\n", pname);
			exit(EXIT_FAILURE);
		}
	}

	return 0;

}

//=================================================================================
//CreateSharedMemory()
SharedMem *createSharedMemory(char *shm_name, char *player_name, int nplayers) {

	int shmfd,fd;
	char SHM_NAME[50] = {'\0'};
	char *ps;
	SharedMem *shm;
	int shm_size = sizeof(SharedMem);
	char aux_log_name[50] = {'\0'};
	char *file_append = "        WHEN        |        WHO        |        WHAT        |        RESULT        \n";

	//Create SHM_NAME correctly
	ps = &SHM_NAME[0];
	strcat(ps,"/");
	strcat(ps,shm_name); // name = /shmname
	
	//Open the shared memory region
	shmfd = shm_open(SHM_NAME,O_RDWR | O_EXCL | O_CREAT ,0777);

	if (errno == EEXIST){

		//Attach this region to virtual memory
		shmfd = shm_open(SHM_NAME,O_RDWR,0777);
		shm = mmap(0, shm_size, PROT_READ|PROT_WRITE, MAP_SHARED, shmfd, 0);

		if (shm == MAP_FAILED) {
		perror("Failure in mmap()\n");
		return NULL;
		}

		if (checkArgs(shm,player_name,nplayers)!=0){
			exit(EXIT_FAILURE);
		}


	} else {
		if (shmfd < 0) {
			perror("Failure in shm_open()\n");
			fprintf(stderr, "Error: %d\n", errno);
			return NULL;
		}
		//Specify the size of the shared memory region
		if (ftruncate(shmfd, shm_size) < 0) {
			perror("Failure in ftruncate()\n");
			return NULL;
		}
		//Attach this region to virtual memory
		shm = mmap(0, shm_size, PROT_READ|PROT_WRITE, MAP_SHARED, shmfd, 0);
		if (shm == MAP_FAILED) {
			perror("Failure in mmap()\n");
			return NULL;
		}
		//SyncSharedMemoryObjects
		syncSharedMemoryObjects(shm);			

		//Initialize data in shared memory
		shm->nplayers = nplayers;
		shm->reg_players = 0;
		shm->turn = 0;
		shm->roundnumber = 0;
		shm->dealer = 0;
		shm->turnplayed = 0;
		strcat(&aux_log_name[0], shm_name);
		strcat(&aux_log_name[0], ".log");
		strcpy(&shm->log_name[0],aux_log_name);
		
		//Create file.log
		fd = open(shm->log_name, O_WRONLY | O_CREAT | O_EXCL, 0777);
		if (fd == -1) {
		perror(&shm->log_name[0]);
		fprintf(stderr,"erro a criar o ficheiro\n");
		exit(EXIT_FAILURE);
		}
		close(fd);
		
		//Apend string to file.log
		fd = open(&shm->log_name[0],O_WRONLY | O_APPEND, 0777);
		if (fd == -1) {
		perror(&shm->log_name[0]);
		fprintf(stderr,"erro a abrir o ficheiro\n");
		exit(EXIT_FAILURE);
		}
		write(fd, file_append, strlen(file_append));
		close(fd);
	}

	return (SharedMem *) shm;
}
