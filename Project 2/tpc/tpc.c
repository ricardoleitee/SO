/*
	MAIN GAME PROGRAM
*/

//===============================================================================
//Includes
#include "game.h"
unsigned int PNUM; //player number

//===============================================================================
//Print Player Orders()
void printPlayerOrders() {
	printf("1. Show the cards on the table\n");
	printf("2. Show own hand\n");
	printf("3. Show previous round of cards\n");
	printf("4. Show elapsed time\n");
}

//===============================================================================
//Show_table_cards()
void show_table_cards(SharedMem *shm) {
	unsigned int k;
	system("clear");
	printf("Tablecards: ");
	for (k=0 ; k<=shm->nplayers-1 ; k++) {
		if (shm->tablecards[k].s !='c' && shm->tablecards[k].s !='h' && shm->tablecards[k].s !='d' && shm->tablecards[k].s !='s') {
			printf("- ");
		}
		else printf("%s%c ",&shm->tablecards[k].c[0], shm->tablecards[k].s);
	}
	printf("\n\n");
}

//===============================================================================
//Show_own_hand()
void show_own_hand(SharedMem *shm) {
	system("clear");
	printHand(shm,PNUM);
	printf("\n\n");
}

//===============================================================================
//Show_previous_tablecards() - prints previous table cards
void show_prev_tablecards(SharedMem *shm){
	unsigned int i=0;
	system("clear");
	printf("Previous tablecards: ");
	for (i=0;i<4;i++){
		printf("%s%c ",&shm->p_tablecards[i].c[0], shm->p_tablecards[i].s);
	}
	printf("\n\n");
}

//================================================================================
//Show_time_elapsed() - prints time elapsed since the beginning of a player's turn
void show_time_elapsed(time_t start) {
	system("clear");
	double seconds;
	time_t end;
	time(&end);

	seconds = difftime(end, start);
	printf("Time elapsed: %.f seconds\n", seconds);
	printf("\n\n");
}

//===============================================================================
//converts struct card c into char* and loads into string (e.g. 10s or Jc)
void cardToString(card c,char* string){
	char buf[4]={'\0'};
	sprintf(buf,"%s%c",&c.c[0],c.s);
	char* pb=&buf[0];
	strcpy(string,pb);
}

//===============================================================================
//checks if player still has cards to play, if true returns 0 if false returns 1
unsigned int checkPlayedAllCards(SharedMem* shm){
	unsigned int i=0;
	for (;i<13;i++){
		if (strcmp(shm->players[PNUM].hand[i].c,"E")!=0){
			return 0;
		}
	}
	return 1;
}


//===============================================================================
//pick_card_from_hand();
unsigned int pick_card_from_hand(SharedMem *shm){
	printHand(shm,PNUM);
	char selection[4]={'\0'};
	unsigned int valid=0;
	unsigned int i=0;
	char* s_card=(char*)malloc(4*sizeof(char));
	printf("Select card: ");

	while(valid!=1){
		
		scanf("%s",&selection[0]);		
		printf("Selected card: %s\n",selection);

		for(i=0;i<13;i++){
			cardToString(shm->players[PNUM].hand[i],s_card); //convert card to string so you can compare with player selected card
			if (strcmp(selection,s_card)==0){ //if card exists, extract card index on hand array
				valid=1;
				return i;
			}
		}
		printf("The card you selected is invalid.\n");
		printf("Please select a card from your hand (e.g. As for Ace of Spades).\n");
		printf("Select card: ");
	}
	return 100;
}

//===============================================================================
//play_card()
void play_card(SharedMem *shm){
	unsigned int card_index_on_hand;
	card_index_on_hand = pick_card_from_hand(shm);
	unsigned int i;
	unsigned int indice;
	time_t play_time;
	char *card_played = (char*)malloc(4*sizeof(char));

	for (i=0 ; i<4 ; i++) {
		if (shm->tablecards[i].s !='c' && shm->tablecards[i].s !='h' && shm->tablecards[i].s !='d' && shm->tablecards[i].s !='s') {
			indice = i;
			break;
		}
	}

	time(&play_time);				
	cardToString(shm->players[PNUM].hand[card_index_on_hand], card_played);
	logInfo(shm, play_time, PNUM, "play", card_played);
	
	pthread_mutex_lock(&shm->lock_shm);
	shm->tablecards[indice] = shm->players[PNUM].hand[card_index_on_hand];
	strcpy(shm->players[PNUM].hand[card_index_on_hand].c, "E");
	pthread_mutex_unlock(&shm->lock_shm);

	time(&play_time);
	char *s_hand=(char*)malloc(1000*sizeof(char));
	handToString(shm,PNUM,s_hand);
	logInfo(shm,play_time,PNUM,"hand",s_hand);

	printf("\n\n");
}

//===============================================================================
//ChangeTurn()
void changeTurn(SharedMem* shm) {
	unsigned int turno_anterior = shm->turn;	

	pthread_mutex_lock(&shm->lock_shm);
	pthread_mutex_lock(&shm->lock_plays);
	if (shm->turn == 3)	shm->turn = 0;
	else	shm->turn++;
	pthread_cond_broadcast(&shm->player_cond);
	pthread_mutex_unlock(&shm->lock_plays);
	pthread_mutex_unlock(&shm->lock_shm);
	printf("Turno Anterior: %d  ;  Proximo Turno: %d\n", turno_anterior, shm->turn);
}

//===============================================================================
//CreatePlayer()
void createPlayer(SharedMem *shm, char *pname) {
	//fifo_name = "FIFOname";
	char fifo_name[50]={'\0'};
	char *FIFO = "FIFO";
	strcat(fifo_name, FIFO);
	strcat(fifo_name, pname);
	//printf("String: %s\n", fifo_name);
	
	pthread_mutex_lock(&shm->lock_shm);
	shm->players[shm->reg_players].pexit=0;
	shm->players[shm->reg_players].num = shm->reg_players;
	strcpy(shm->players[shm->reg_players].nickname,&pname[0]);
	strcpy(shm->players[shm->reg_players].FIFOname,&fifo_name[0]);
	PNUM=shm->reg_players;
	shm->reg_players++;
	//printf("Numero do jogador : %d\n", shm->players[shm->nplayers].num);
	//printf("Nome do jogador : %s\n", shm->players[shm->nplayers].nickname);
	//printf("FIFO do jogador : %s\n", shm->players[shm->nplayers].FIFOname);
	pthread_mutex_unlock(&shm->lock_shm);
}

//===============================================================================
//Thread readDealerHand()
void *readDealerHand(void *arg) {
	readHandArgs* argumnts = arg;

	int shmfd;
	char SHM_NAME[50] = {'\0'};
	char *ps;
	SharedMem *shm;
	int shm_size = sizeof(SharedMem);

	int fd;
	char str[130]={'\0'};

	//Create SHM_NAME correctly
	ps = &SHM_NAME[0];
	strcat(ps,"/");
	strcat(ps,argumnts->shm_name); // name = /shmname

	//Open the shared memory region
	shmfd = shm_open(SHM_NAME,O_RDWR | O_EXCL ,0777);

	if (shmfd < 0) {
		perror("Failure in shm_open()\n");
			fprintf(stderr, "Error: %d\n", errno);
			return NULL;
	}

	//Attach this region to virtual memory
	shm = mmap(0, shm_size, PROT_READ|PROT_WRITE, MAP_SHARED, shmfd, 0);
	if (shm == MAP_FAILED) {
		perror("Failure in mmap()\n");
		return NULL;
	}

	pthread_mutex_lock(&shm->lock_shm);
	shm->players[argumnts->num].FIFOready=1;
	pthread_mutex_unlock(&shm->lock_shm);

	mkfifo(shm->players[argumnts->num].FIFOname,0777);
	fd=open(shm->players[argumnts->num].FIFOname,O_RDONLY);

	char** s_hands=(char**)malloc(26*sizeof(char*));
	unsigned int i=0;
	for (i=0;i<26;i++){
		s_hands[i]=(char*)malloc(3*sizeof(char));
	}

	readline(fd,str);	
	close(fd);

	pthread_mutex_lock(&shm->lock_shm);
	shm->players[argumnts->num].FIFOready=0;
	pthread_mutex_unlock(&shm->lock_shm);
	strcpy(argumnts->hand,str);
	
	pthread_exit(NULL);
}

//===============================================================================
//Keyborad's Thread()
void *keyboard(void *arg) {
	unsigned int option;
	unsigned int valid;
	unsigned int played_card;
	unsigned int l;
	time_t start;
	SharedMem *shm=arg;
	system("clear");
	//printf("PNUM: %d\n", PNUM);

	while(shm->roundnumber != 13) {
		played_card = 0;
		if (shm->roundnumber==13){
			pthread_mutex_lock(&shm->lock_plays);
			pthread_cond_broadcast(&shm->player_cond);
			pthread_mutex_unlock(&shm->lock_plays);
			printf("Game ended!\n");
			pthread_exit(NULL);
		}

		if (shm->turn == PNUM) {
			time(&start);
			while (played_card != 1) {
				valid = 1;


				//Print Player Orders
				printPlayerOrders();
				printf("5. Play Card\n");

				//Receive the player order and validates
				while (valid != 0 && shm->roundnumber!=13) {
					printf("Select option to play: ");
					scanf("%d", &option); //because scanf with %d argument only reads digits and keeps other chars in buffer,
					__fpurge(stdin);      //purging of stdin is necessary to prevent leakage into other functions that read from stdin
					
					//Validate Option
					if (option > 0 && option <= 5) valid = 0;
	
					else printf("Insert a valid number\n\n");
				}
			
				switch (option) {
					case 1:
						show_table_cards(shm);
						break;
					case 2:
						show_own_hand(shm);
						break;
					case 3:
						show_prev_tablecards(shm);
						break;
					case 4:
						show_time_elapsed(start);
						break;
					case 5: 
						play_card(shm);
						pthread_mutex_lock(&shm->lock_shm);
						shm->turnplayed++;
						pthread_mutex_unlock(&shm->lock_shm);
						//Validate all players play the round
						if (shm->turnplayed == 4){
							pthread_mutex_lock(&shm->lock_shm);
							for (l=0 ; l<shm->nplayers ; l++) {
								shm->p_tablecards[l] = shm->tablecards[l];
								strcpy(shm->tablecards[l].c, "E");
								shm->tablecards[l].s = ' ';	
							}
							shm->turnplayed = 0;
							shm->roundnumber++;
							pthread_mutex_unlock(&shm->lock_shm);
						}
						if (checkPlayedAllCards(shm)==1){
							shm->players[PNUM].pexit=1;
						}
						changeTurn(shm);
						system("clear");
						played_card = 1;
						break;
				}
			}
		} else {
			time(&start);
			while (shm->turn != PNUM) {
				valid = 1;
					
				//printf("%s's Turn!\n\n", shm->players[shm->turn].nickname);

				//Print Player Orders
				printPlayerOrders();

				//Receive the player order and validates
				while (valid != 0 && shm->roundnumber!=13) {
					printf("Select option to play: ");
					fflush(stdin);
					scanf("%d", &option); //because scanf with %d argument only reads digits and keeps other chars in buffer,
					__fpurge(stdin);      //purging of stdin is necessary to prevent leakage into other functions that read from stdin

					//Validate Option
					if (option > 0 && option <= 5) valid = 0;

					else printf("Insert a valid number\n\n");
				}
			
				switch (option) {
					case 1:
						show_table_cards(shm);
						break;
					case 2:
						show_own_hand(shm);
						break;
					case 3:
						show_prev_tablecards(shm);
						break;
					case 4:
						show_time_elapsed(start);
						break;
					case 5: 
						if (shm->turn == PNUM) {
						play_card(shm);
						pthread_mutex_lock(&shm->lock_shm);
						shm->turnplayed++;
						pthread_mutex_unlock(&shm->lock_shm);
						//Validate all players play the round
						if (shm->turnplayed == 4){
							pthread_mutex_lock(&shm->lock_shm);
							for (l=0 ; l<shm->nplayers ; l++) {
								shm->p_tablecards[l] = shm->tablecards[l];
								strcpy(shm->tablecards[l].c, "E");
								shm->tablecards[l].s = ' ';	
							}
							shm->turnplayed = 0;
							shm->roundnumber++;
							pthread_mutex_unlock(&shm->lock_shm);
						}
						changeTurn(shm);
						system("clear");
						played_card = 1;
						break;
						}
						else printf("Insert a valid number\n");
				}
			}
		}		
	}
	
	pthread_exit(NULL);
}

//===============================================================================
//Game Action's Thread()
void *game_sync(void *arg) {

	SharedMem *shm=arg;
	
	unsigned int tempturn;
	
	tempturn = shm->turn;

	while (shm->roundnumber!=13) {
			if (shm->roundnumber==13){
				printf("Game ended!1\n");
				pthread_exit(NULL);
			}
			pthread_mutex_lock(&shm->lock_plays);
			while(shm->turn==tempturn)
				pthread_cond_wait(&shm->player_cond,&shm->lock_plays);
			pthread_mutex_unlock(&shm->lock_plays);
			if (shm->roundnumber==13){
				printf("Game ended!2\n");
				pthread_cancel((*shm->players[PNUM].keyboard));
				pthread_exit(NULL);
			}
			if (shm->turn != tempturn) {						
				system("clear");
				if (shm->turn == PNUM) {
					show_table_cards(shm);
					printf("YOUR TURN TO PLAY!\n");
					//Print Player Orders
					printPlayerOrders();
					printf("5. Play Card\n");
				printf("Round: %d\n", shm->roundnumber);

					printf("Select option to play: ");
				}

				else {
					show_table_cards(shm);
					printf("%s's Turn!\n\n", shm->players[shm->turn].nickname);
					//Print Player Orders
					printPlayerOrders();
				printf("Round: %d\n", shm->roundnumber);
					printf("Select option to play: ");

				}
			tempturn = shm->turn;			
			}
		//else sleep(0);
	}
	pthread_exit(NULL);
}

//===============================================================================
//Main
int main(int argc, char *argv[]) {
	setbuf(stdout, NULL);
	unsigned int argnp=atoi(argv[3]);
	if (argc != 4 || argnp!=4) {
		fprintf(stderr,"Usage: tpc <player's name> <shm name> <n. players>\n");
		fprintf(stderr,"For this version, please use 4 as number of players\n");
		exit(EXIT_FAILURE);
	}

	else {
		pthread_t ktid, gstid;	
		SharedMem *shmem;
		//unsigned int *owner = 0;
		card deck[52];
		card* pdeck=&deck[0];
		char *player_name = argv[1];		
		char *shm_name = argv[2];		
		unsigned int nplayers = atoi(argv[3]);

		//Create shared memory
		if ((shmem = createSharedMemory(shm_name, player_name, nplayers)) == NULL) {  //argument check inside createSharedMemory()
			perror("Failure in createSharedMemory()\n");
			exit(EXIT_FAILURE);
		}

		//SyncSharedMemoryObjects
		syncSharedMemoryObjects(shmem);
	
		
		createPlayer(shmem, player_name);

		if(isDealer(shmem,argv[1]) == 1){
			printf("waiting for all players to join...");
			do {
				printf(".");
				sleep(1);
			} while (allPlayersReady(shmem)!=1);
			printf("\n");
		} else {
			printf("waiting for all players to join...");
			do {
				printf(".");
				sleep(1);
			} while (allPlayersReady(shmem)!=1);
			printf("\n");
		}

		printf("%s table complete: game may start. dealer is %s\n", shm_name, shmem->players[0].nickname);
		

		//printf("REGplayers: %d\n", shmem->reg_players);
		readHandArgs* arguments;
		arguments = malloc(sizeof(readHandArgs));
		arguments->shm_name = argv[2];
		arguments->num = PNUM;
		char* dealer_hand=(char*)malloc(50*sizeof(char));
		arguments->hand = dealer_hand;
		unsigned int dealerChoice;
		time_t tempo;
		if (isDealer(shmem, argv[1]) == 1) {
			//Create deck
			generateShuffledDeck(pdeck);
			//printDeck(pdeck);			
			unsigned int valid = 0;
			pthread_t read_hand_dealer_tid;
			
			//Receive the dealer order and validates
			while (valid != 1) {
				printf("Choose first player to play/receive cards: \n");
				printPlayers(shmem);
				printf("--------------\n");
				printf("Player number: ");
				scanf("%d", &dealerChoice);
				__fpurge(stdin);

				//Validate Option
				if (dealerChoice >= 0 && dealerChoice < 4)	valid = 1;
				else printf("Insert a valid player number!\n");
			}

			printf("Dealing cards...\n");
		
			time(&tempo);
			logInfo(shmem, tempo, PNUM, "deal", "-");
			
			//Deal cards
			if(shmem->players[0].num == dealerChoice) {
				arguments = malloc(sizeof(readHandArgs));
				arguments->shm_name = argv[2];
				arguments->num = PNUM;
				arguments->hand = dealer_hand;

				if (pthread_create(&read_hand_dealer_tid, NULL, readDealerHand, arguments)) {
					perror("Could not create readDealerHand thread\n");
					exit(EXIT_FAILURE);
				}
				writeHandInFIFO(shmem,deck, shmem->players[0].num, 0, 13);
				pthread_join(read_hand_dealer_tid, NULL);
				//printf("Received hand: %s\n",dealer_hand);
				readHandToStruct(shmem,dealer_hand);
				printHand(shmem,PNUM);

				time(&tempo);				
				handToString(shmem, PNUM, dealer_hand);
				logInfo(shmem, tempo, PNUM, "receive-cards", dealer_hand);	

				writeHandInFIFO(shmem,deck, shmem->players[1].num, 13, 26);
				writeHandInFIFO(shmem,deck, shmem->players[2].num, 26, 39);
				writeHandInFIFO(shmem,deck, shmem->players[3].num, 39, 52);			
			}

			if(shmem->players[1].num == dealerChoice) {
				writeHandInFIFO(shmem,deck, shmem->players[1].num, 0, 13);
				writeHandInFIFO(shmem,deck, shmem->players[2].num, 13, 26);
				writeHandInFIFO(shmem,deck, shmem->players[3].num, 26, 39);

				if (pthread_create(&read_hand_dealer_tid, NULL, readDealerHand, arguments)) {
					perror("Could not create readDealerHand thread\n");
					exit(EXIT_FAILURE);
				}
				writeHandInFIFO(shmem,deck, shmem->players[0].num, 39, 52);
				pthread_join(read_hand_dealer_tid, NULL);
				
				//printf("Received hand: %s\n",dealer_hand);
				readHandToStruct(shmem,dealer_hand);
				printHand(shmem,PNUM);

				time(&tempo);				
				handToString(shmem, PNUM, dealer_hand);
				logInfo(shmem, tempo, PNUM, "receive-cards", dealer_hand);
			}

			if (shmem->players[2].num == dealerChoice) {
				writeHandInFIFO(shmem,deck, shmem->players[2].num, 0, 13);
				writeHandInFIFO(shmem,deck, shmem->players[3].num, 13, 26);
				writeHandInFIFO(shmem,deck, shmem->players[1].num, 26, 39);

				if (pthread_create(&read_hand_dealer_tid, NULL, readDealerHand, arguments)) {
					perror("Could not create readDealerHand thread\n");
					exit(EXIT_FAILURE);
				}
				writeHandInFIFO(shmem,deck, shmem->players[0].num, 39, 52);
				pthread_join(read_hand_dealer_tid, NULL);
				//printf("Received hand: %s\n",dealer_hand);
				readHandToStruct(shmem,dealer_hand);
				printHand(shmem,PNUM);	

				time(&tempo);				
				handToString(shmem, PNUM, dealer_hand);
				logInfo(shmem, tempo, PNUM, "receive-cards", dealer_hand);
			}

			if (shmem->players[3].num == dealerChoice) {
				writeHandInFIFO(shmem,deck, shmem->players[3].num, 0, 13);
				writeHandInFIFO(shmem,deck, shmem->players[2].num, 13, 26);
				writeHandInFIFO(shmem,deck, shmem->players[1].num, 26, 39);

				if (pthread_create(&read_hand_dealer_tid, NULL, readDealerHand, arguments)) {
					perror("Could not create readDealerHand thread\n");
					exit(EXIT_FAILURE);
				}
				writeHandInFIFO(shmem,deck, shmem->players[0].num, 39, 52);
				pthread_join(read_hand_dealer_tid, NULL);
				//printf("Received hand: %s\n",dealer_hand);
				readHandToStruct(shmem,dealer_hand);
				printHand(shmem,PNUM);	

				time(&tempo);				
				handToString(shmem, PNUM, dealer_hand);
				logInfo(shmem, tempo, PNUM, "receive-cards", dealer_hand);
			}
				
			printf("Hands dealt\n");
			shmem->turn=dealerChoice;

		}else {
			printf("...waiting for hand to be dealt...\n");
			do {
				sleep(0);
			} while (shmem->turn!=PNUM);
			char* hand=(char*)malloc(50*sizeof(char));
			//printf("PNUM:%d\n",PNUM);
			readHandFIFO(shmem,PNUM,hand);
			//printf("Received hand: %s\n",hand);
			readHandToStruct(shmem,hand);
			printHand(shmem,PNUM);

			time(&tempo);				
			handToString(shmem, PNUM, dealer_hand);
			logInfo(shmem, tempo, PNUM, "receive-cards", dealer_hand);
		}
		
		//NEXT TO DO
		sleep(2);
		shmem->players[PNUM].keyboard=&ktid;
		shmem->players[PNUM].game_sync=&gstid;


		if (pthread_create(&gstid, NULL, game_sync, shmem)) {
			perror("Could not create game_sync thread\n");
			exit(EXIT_FAILURE);
		}
		if (pthread_create(&ktid, NULL, keyboard, shmem)) {
				perror("Could not create keyboard thread\n");
				exit(EXIT_FAILURE);
		}
		pthread_join(gstid, NULL);
		pthread_join(ktid, NULL);
		
		if (PNUM==0){
		destroySharedMemory(shmem, shm_name);
		printf("Shared memory destroyed..\n");
		}
	}

	exit(EXIT_SUCCESS);	
}
