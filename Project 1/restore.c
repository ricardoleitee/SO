#include "restore.h"

unsigned int LAST_MOD_TIME;

int main(int argc, char *argv[]) {
	if (argc != 3) {
		fprintf(stderr, "USAGE: rstr source_dir dest_dir\n");
		exit(EXIT_FAILURE);
	}
	else {

		if (argcheck_restore(argv) == 1) {

		int k = 0;
		k = checkBackup(argv[1]);

		if (k != 0) {

			//Create source and destination fullpath
			char *src_fullpath = realpath(argv[1], NULL);
			char *dest_fullpath = realpath(argv[2], NULL);

			//Create "dir2/__bckpinfo__" path
			char bckp_path[PATH_MAX] = {'\0'};
			strcpy(bckp_path, argv[1]);
			strcat(bckp_path, "/__bckpinfo__");

			//Load "dir2/__bckpinfo__" file
			unsigned int btime = 0;
			unsigned int *pbtime = &btime;
			struct bckpfile *fileinfo;
			unsigned int lines = 0;

			lines = linecounter(bckp_path);
			fileinfo = loadBckpinfo(bckp_path, lines, pbtime);

			//Get first restore point
			printf("Pbtime: %u\n", btime);
			char* stime=(char*)malloc(21*sizeof(char));
			getFormattedTimeString(btime, stime);
			printf("Formated String: %s\n", stime);

			//Search for subdirs in source_dir
			char ** subdirs_list = restoreList(argv[1]);
			int sizeList = subdirindir(argv[1]);

			//Get Full RestoreList
			char ** restorelist = addRestoreList(subdirs_list, sizeList, stime);

			//Print the RestoreList
			printSubdirs(restorelist, sizeList+1);

			//Receive the selected option to restore
			int valid = 1;
			int ind = 0;
			while(valid != 0) {
				char *restore_option = (char *) malloc (PATH_MAX*sizeof(char));
				printf("Which restore point(time): ");
				scanf("%s", restore_option);

				//Validates option
				int l=0;
				for (; l<sizeList+1 ; l++) {
					if (strcmp(basename(restorelist[l]), restore_option) == 0) {
						ind = l;
						valid = 0;
					}
				}
			}			

			//Restore
			if (ind == 0) {			
				restoreFiles(src_fullpath, dest_fullpath);
			}				
			else {
				restoreFiles(restorelist[ind], dest_fullpath);
			}
		}

		else {
			fprintf(stderr, "There is no files in source_dir\n");
			exit(EXIT_FAILURE);
		}
		}
		else {
			fprintf(stderr, "Argument error\n");
			exit(EXIT_FAILURE);
		}
	}
	return 0;
}
