#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <wait.h>
#include <libgen.h>
#include <limits.h>

//analyzes provided path and returns
//0 if directory,
//-1 if symlink,
//1 if regular file
int whatIsIt(char *path){
	struct stat sb;
	if (stat(path, &sb) == -1){
		perror("stat");
		exit(EXIT_FAILURE);
	}
	int itis=-1;
	switch(sb.st_mode & S_IFMT){
		case S_IFDIR: itis=0; break;  //is directory
		case S_IFLNK: itis=-1; break; //is symlink
		case S_IFREG: itis=1; break;  //is regular file
		default: itis=-1; break;
	}
	return itis;
}

//get last file modification time (returns unsigned int)
int lastModTimeLong(char* path){
	struct stat sb;
	if (stat(path,&sb) == -1){
		perror("stat");
		exit(EXIT_FAILURE);
	} else {
		return (unsigned int) sb.st_mtime;
	}
}


//checks validity of arguments passed, returns true if valid, false otherwise
int argcheck(char* args[]){
	int size=4;
	//printf("args[] size: %d\n",size);
	int i=0;
	for (;i<size;i++){
		if(i==1 || i==2){
			//printf("File: %s\n",args[i]);
			switch(whatIsIt(args[i])){
			case -1: return 0; break;
			case 1: return 0; break;
			}
		}
		if (i==3){
			int k=atoi(args[i]); //converts backup_interval from char* to int
			//printf("k=%d\n",k);
			if (k<=0){            //check for valid time (k>=0)
				perror("atoi");
				fprintf(stderr,"time convert failure\n");
				exit(EXIT_FAILURE);
			}
		}
	}
	return 1;
}

//returns number of files in directory
int filesindir(char *pth){
//	printf("dir:%s\n",pth);
	char copy[PATH_MAX]={'\0'};
	DIR *mydir = opendir(pth);
	struct dirent *entry = NULL;
	struct stat buf;
	int sz=0;
	while((entry = readdir(mydir))) /* If we get EOF, the expression is 0 and
                                     * the loop stops. */
    	{
		if (!strcmp(entry->d_name,".") || !strcmp(entry->d_name,".."))
			continue;

		//If you don't add "dirname/" to the string, realpath and stat don't work
		strcpy(copy,pth);
		strcat(copy,"/");
		strcat(copy,entry->d_name);

		stat(copy,&buf);
		if (S_ISREG(buf.st_mode)){
			//printf("is reg\n");
			//char * fullpath = realpath(copy,NULL);
			//printf("FP: %s\n",fullpath);
			sz++;
		}
    	}
	closedir(mydir);
//	printf("sz=%d\n",sz);
	return sz;
}

//reads directory and returns pointer-to-pointer of chars with paths of regular files
char ** readDir(char *pth){
	int sz=filesindir(pth);
	if (sz==0){
		return NULL;
	} else {
		char ** files = (char **) malloc (sz*sizeof(char*));
		int i=0;
		for (;i<sz;i++){
			files[i] = (char *) malloc(PATH_MAX*sizeof(char));
		}
		char copy[PATH_MAX]={'\0'};
		DIR *mydir = opendir(pth);
	        struct dirent *entry = NULL;
	        struct stat buf;
		i=0;
	        while((entry = readdir(mydir))) /* If we get EOF, the expression is 0 and
	                                     * the loop stops. */
	        {
	                if (!strcmp(entry->d_name,".") || !strcmp(entry->d_name,".."))
	                        continue;

	                //If you don't add "dirname/" to the string, realpath and stat don't work
	                strcpy(copy,pth);
	                strcat(copy,"/");
	                strcat(copy,entry->d_name);

        	        stat(copy,&buf);
	                if (S_ISREG(buf.st_mode)){
        	                //printf("is reg\n");
                	        char * fullpath = realpath(copy,NULL);
                        	//printf("FP: %s\n",fullpath);
				strcpy(files[i],fullpath);
				i++;
	                }
	        }

//print paths added to the array
/*	for (i=0;i<sz;i++){
		printf("%s\n",files[i]);
	}*/
        closedir(mydir);
	return files;
	}
}


