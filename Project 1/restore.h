#include "backup.h"

//checks validity of arguments passed, returns true if valid, false otherwise
int argcheck_restore(char* args[]){
	int size=3;
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
	}
	return 1;
}

void restoreFiles(char *restorepoint, char *destin_dir) {
	char copy[PATH_MAX] = {'\0'};
	char *pcopy = &copy[0];
	strcpy(pcopy, restorepoint);
	strcat(pcopy, "/__bckpinfo__");

	unsigned int btime = 0;
	unsigned int * pbtime = &btime;
	unsigned int lines = linecounter(pcopy);

	char ** source_paths = getDestinationPaths(loadBckpinfo(pcopy, lines, pbtime),(lines-1)/3 );
	char** dest_paths = createDestPaths(source_paths,destin_dir,(lines-1)/3);
	performFullBackup(source_paths,dest_paths,(lines-1)/3);
}

char ** addRestoreList(char ** restore_list, int size, char * first_restore) {
	char ** restore = (char **) malloc ((size+1)*sizeof(char *));

	int x=0;
	int y=1;

	for (; x<size+1 ; x++) {
		restore[x] = (char *) malloc (PATH_MAX*sizeof(char));
	}

	strcpy(restore[0], first_restore);

	for (; y<size+1 ; y++) {
		strcpy(restore[y], restore_list[y-1]);
	}

	return restore;
}

void printSubdirs(char ** subdirs, int size) {
	int j;

	printf("List of available restore points: \n");
	for (j=0 ; j<size ; j++) {
		printf("%s\n", basename(subdirs[j]));
	}
}

int subdirindir(char *dir2) {
	//printf("dir: %s\n", dir2);
	char copy[PATH_MAX] = {'\0'};
	DIR *mydir = opendir(dir2);
	struct dirent *entry = NULL;
	struct stat buf;
	int sz = 0;

	while((entry = readdir(mydir))) {
		if (!strcmp(entry->d_name,".") || !strcmp(entry->d_name,".."))
			continue;

		//If don't add "dirname/" to the string, realpath and stat don't work
		strcpy(copy, dir2);
		strcat(copy,"/");
		strcat(copy, entry->d_name);

		stat(copy, &buf);
		if (S_ISDIR(buf.st_mode)) {
			//printf("is dir\n");
			//char * fullpath = realpath(copy, NULL);
			//printf("FP: %s\n", fullpath);
			sz++;
		}
	}
	closedir(mydir);
	//printf("sz = %d\n", sz);
	return sz;
}

char ** restoreList(char *dir2) {
	int sz = subdirindir(dir2);
	if (sz == 0) {
		return NULL;
	}

	else {
		char ** subdirs = (char **) malloc (sz*sizeof(char *));
		int i = 0;

		for (; i<sz ; i++) {
			subdirs[i] = (char *) malloc (PATH_MAX*sizeof(char));
		}

		char copy[PATH_MAX] = {'\0'};
		DIR *mydir = opendir(dir2);
		struct dirent *entry = NULL;
		struct stat buf;
		i=0;

		while((entry = readdir(mydir))) {
			if (!strcmp(entry->d_name,".") || !strcmp(entry->d_name,".."))
				continue;

			//If don't add "dirname/" to the string, realpath and stat don't work
			strcpy(copy, dir2);
			strcat(copy, "/");
			strcat(copy, entry->d_name);

			stat(copy, &buf);
			if (S_ISDIR(buf.st_mode)) {
				//printf("is dir\n");
				//printf("copy: %s\n",copy);
				char * fullpath = realpath(copy, NULL);
				//printf("BN: %s\n", fullpath);
				strcpy(subdirs[i], fullpath);
				i++;
			}
		}

		/*for(i=0 ; i<sz ; i++) {
			printf("%s\n", subdirs[i]);
		}*/

		closedir(mydir);
		return subdirs;
	}
}
