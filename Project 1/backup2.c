#include "backup.h"

unsigned int LAST_MOD_TIME;

//signal handler function
static void signal_handler(int signo){
	if (signo==SIGUSR1){
		printf("Received SIGUSR1, stopping backup process.\n");
		exit(EXIT_SUCCESS);
	}

	if (signo==SIGINT){
		fprintf(stderr,"Process Interrupted, received SIGINT.\n");
		exit(EXIT_FAILURE);
	}

	return;
}





int main(int argc,char* argv[]){
	LAST_MOD_TIME=0;
	time_t rawtime;
	struct tm * timeinfo;

	//argument check
	if (argc!=4){
		fprintf(stderr,"Usage: bckp source_dir dest_dir dt(sec)\n");
		exit(EXIT_FAILURE);
	} else {
		if (argcheck(argv)==1){    //perform argument check (see fileinfo.h)
			while(1){
				time ( &rawtime );
				timeinfo = localtime (&rawtime);
			//	printf("Current time: %ld\n",(long) &rawtime);
				//printf ( "Current local time and date: %s", asctime (timeinfo) );
				char** files_list=readDir(argv[1]);
				int size=filesindir(argv[1]);
				int k;
				char *rbckpi=realpath(argv[2],NULL);
				k=checkBackup(argv[2]);  //check if destination folder has any files
				if (k==0){	//dir2 is empty, first backup
						char** dest_paths=createDestPaths(files_list,argv[2],size); //create destination paths
						performFullBackup(files_list,dest_paths,size);		    //copy files
						time(&rawtime);
						timeinfo=localtime(&rawtime);
						int thistime=mktime(timeinfo);
						if(bckpinfo(rbckpi,files_list,dest_paths,size,thistime)!=0){ //create first bckpinfo
							fprintf(stderr,"failed\n");
						}
				} else {
					//dir2 is not empty, check for modifications and perform incremental backup
					checkModifications(argv[1],argv[2]);
				}

				//signal handling
				if (signal(SIGUSR1,signal_handler)==SIG_ERR){
					fprintf(stderr,"can't catch signal\n");
					exit(EXIT_FAILURE);
				}

				if (signal(SIGINT,signal_handler)==SIG_ERR){
					fprintf(stderr,"can't catch signal\n");
					exit(EXIT_FAILURE);
				}

				//thread sleep for argument-specified time
				int st=0;
				st=atoi(argv[3]);
				//printf("Slept %d seconds, resuming..\n",st);
				sleep(st);
			}

		} else {
			fprintf(stderr,"Argument error.\n");
			exit(EXIT_FAILURE);
		}	


	}
}
