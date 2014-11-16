#include "fileinfo.h"
#define BUFFER_SIZE 512
extern unsigned int LAST_MOD_TIME; //global variable used to keep track of the last bckpinfo

//struct to hold file information
struct bckpfile{
	char* s_path; //source_path
	char* d_path; //destination_path
	unsigned int modtime; //last modification time
};


//checks if there are files in directory
int checkBackup(char* dest){
	int k=0;
	k=filesindir(dest);
	return k;
}

//copies files
int filecopy(char* source,char* dest){
	int fd1, fd2, nr, nw;
	unsigned char buffer[BUFFER_SIZE];

	//opens source file in READ_ONLY mode
	fd1 = open(source, O_RDONLY);
	if (fd1 == -1) {
		perror(source);
		fprintf(stderr,"erro a abrir o ficheiro\n");
		exit(EXIT_FAILURE);
	}

	//opens destination file (WRITE_ONLY mode)
	fd2 = open(dest, O_WRONLY | O_CREAT | O_EXCL, 0644);
	if (fd2 == -1) {
		perror(dest);
		close(fd1);
		fprintf(stderr,"erro a criar copia ficheiro\n");
		exit(EXIT_FAILURE);
	}

	//reads BUFFER_SIZE bytes into buffer from fd1 and writes nr bytes from buffer into fd2
	while ((nr = read(fd1, buffer, BUFFER_SIZE)) > 0)
	if ((nw = write(fd2, buffer, nr)) <= 0 || nw != nr) {
		perror(dest);
		close(fd1);
		close(fd2);
		fprintf(stderr,"erro a copiar ficheiro\n");
		exit(EXIT_FAILURE);
	}
	close(fd1);
	close(fd2);
	return 0;
}

//creates path/__bckpinfo__ file
int bckpinfo(char* path,char** source_files,char** dest_files,int size,int backuptime){
	if (size>0){
		int i;
		strcat(path,"/__bckpinfo__");
		FILE *file=fopen(path,"a+");
		//printf("bckpinfo path: %s\n",path);
		char strtime[30];
		snprintf(strtime,30,"%d\n",backuptime);
		fputs(strtime,file);
		for (i=0;i<size;i++){
			char* psp="sp:";
			char fpath[PATH_MAX]={'\n'};
			char dpath[PATH_MAX]={'\n'};
			strcpy(fpath,psp);
			strcat(fpath,source_files[i]);
			strcat(fpath,"\n");
			//printf("fpath:%s\n",fpath);
			fputs(fpath,file);
			psp="dp:";
			strcpy(dpath,psp);
			strcat(dpath,dest_files[i]);
			strcat(dpath,"\n");
			fputs(dpath,file);
			char buf[30];
			snprintf(buf,30,"tm:%d\n",lastModTimeLong(source_files[i]));
			//printf("buf:%s",buf);
			fputs(buf,file);
		}
		fclose(file);
		return 0;
	}
	return 1;
}

//creates destination paths from source paths array files_list and dest dir path (size required)
char** createDestPaths(char** files_list,char* dest,int size){
	int i;
	//create destination file path array and allocate memory
	char** files_dest =(char**) malloc(size*sizeof(char*));
	for (i=0;i<size;i++){
		files_dest[i]=(char*) malloc(PATH_MAX*sizeof(char));
	}
	//printf("dest path:%s\n",dest);

		//create new paths
		//printf("Elements_ins(files_list)=%lu\n",Elements_ins(files_list));
		char * fulldest = realpath(dest,NULL);
		strcat(fulldest,"/");
		for (i=0;i<size;i++){
			if (strlen(files_list[i])>0){
				strcpy(files_dest[i],fulldest); //files_dest[i]=destination_path/
				char* filename=(char*) malloc(PATH_MAX*sizeof(char)); //allocate memory for filename
				filename = basename(files_list[i]); //gets files_list[i]'s filename into filename
				strcat(files_dest[i],filename); //files_dest[i]=destination_path/filename
				//printf("dest: %s\n",files_dest[i]);
				//printf("filename: %s\n",filename);
				//printf("src: %s\n",files_list[i]);

			}
		}
	return files_dest;
}

//copies all files from source path array into dest file array
void performFullBackup(char** source,char** dest,int n){
	int i;
	for (i=0;i<n;i++){
		if (strlen(source[i])>0){
			pid_t pid,childpid;
			int status;
			if ( (pid=fork())<0){
				fprintf(stderr,"fork error\n");
				exit(EXIT_FAILURE);
			} else if (pid==0){
				filecopy(source[i],dest[i]);
				//printf("Forked process %d copied %s to %s successfully.\n",getpid(),source[i],dest[i]);
				exit(0);
			} else {
				childpid=wait(&status);
				//printf("Waited for child: %d, child exited with: %d\n",childpid,WEXITSTATUS(status));
				if (WEXITSTATUS(status)!=0){
					fprintf(stderr,"copy error\n");
					exit(EXIT_FAILURE);
				}
			}
		}
	}
}


//counts lines in file
int linecounter(char* path){
       FILE * fp;
       char * line = NULL;
       size_t len = 0;
       ssize_t read;
	int lines=0;

       fp = fopen(path, "r");
       if (fp == NULL)
           exit(EXIT_FAILURE);

       while ((read = getline(&line, &len, fp)) != -1) {
		lines++;
       }

       if (line)
           free(line);

	//printf("nlines=%d\n",lines);
	return lines;
}

//loads file __bckpinfo__ from path returning array of bckpfile structs
struct bckpfile* loadBckpinfo(char * path,int lines,unsigned int *bckpinfotime){
	FILE *fp;
	char *line=NULL;
	size_t len=0;
	ssize_t read;

	if((fp=fopen(path,"r"))==NULL){
		fprintf(stderr,"Error opening bckpinfo at %s\n",path);
		exit(EXIT_FAILURE);
	}

	struct bckpfile *filestruct;
	filestruct=malloc(((lines-1)/3)*sizeof *filestruct);
	char** filelines=(char**)malloc(lines*sizeof(char*));
	int i=0;
	int x=0; //s_paths
	int y=0; //d_paths
	int z=0; //modtimes

	//read file into filelines string array
	i=0;
	while ((read = getline(&line,&len,fp))!=-1){
		filelines[i]=(char*)malloc(read*sizeof(char));
		if (line[read-1]=='\n')
			line[read-1]='\0';
		strcpy(filelines[i],line);
		i++;
	}

	//load string identifiers
	char sp_c[2]={'s','p'};
	char dp_c[2]={'d','p'};
	char tm_c[2]={'t','m'};

	//fill bckpfile struct with information from bckpinfo (filelines)
	for (i=0;i<lines;i++){
		char paths[PATH_MAX]={'\n'};
		char *pp=&paths[0];
		strcpy(pp,filelines[i]);
		char *np=&paths[3];
		if (i==0){
			unsigned int time=atoi(filelines[i]);
			(*bckpinfotime)=time;
			//printf("time:%d\n",time);
		} else {
			if (strncmp(paths,sp_c,2)==0){
				filestruct[x].s_path=malloc(PATH_MAX*sizeof(char));
				strcpy(filestruct[x].s_path,np);
				//printf("s_path:%s\n",filestruct[x].s_path);
				x++;
			}
			if (strncmp(paths,dp_c,2)==0){
				filestruct[y].d_path=malloc(PATH_MAX*sizeof(char));
				strcpy(filestruct[y].d_path,np);
				//printf("d_path:%s\n",filestruct[y].d_path);
				y++;
			}
			if (strncmp(paths,tm_c,2)==0){
				char time[PATH_MAX]={'\0'};
				char *pt=&time[0];
				strcpy(pt,filelines[i]);
				pt=&time[3];
				unsigned int modt=atoi(pt);
				filestruct[z].modtime=modt;
				//printf("modtime:%d\n",filestruct[z].modtime);
				z++;
			}
		}
	}
	fclose(fp);
	return filestruct;
}


//formats time_t value into string of format yyyy_mm_dd_hh_mm_ss, stores it on r_string
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
	snprintf(string,21,"%d_%s_%s_%s_%s_%s",year,pcm,pcd,pch,pcmin,pcs);
	char* pstring=&string[0];
	strcpy(r_string,pstring);
}

//returns path of last bckpinfo file
char* getLastBckpinfo(char *pth,char* lbckpinfo){
	int sz=filesindir(pth);
	if (sz==0){
		return NULL;
	} else {
		char copy[PATH_MAX]={'\0'};
		DIR *mydir=opendir(pth);
		struct dirent *entry=NULL;
		struct stat buf;
		char lp[PATH_MAX]={'\0'};
		char *last_path=&lp[0];

		while((entry=readdir(mydir))){
			if (!strcmp(entry->d_name,".") || !strcmp(entry->d_name,".."))
				continue;

			strcpy(copy,pth);
			strcat(copy,"/");
			strcat(copy,entry->d_name);
			stat(copy,&buf);
			char *pcopy;
			pcopy=&(copy[0]);
			unsigned int this_time=0;
			unsigned int *pbtime=&this_time;
			if (S_ISREG(buf.st_mode)){
				if (strcmp(basename(copy),"__bckpinfo__")==0){
					//printf("basename=%s\n",basename(copy));
					int sz=linecounter(pcopy);
					loadBckpinfo(pcopy,sz,pbtime);
					if (this_time>=LAST_MOD_TIME){
						LAST_MOD_TIME=this_time;
						//printf("This_time=%d\n",this_time);
						//printf("LAST_MOD_TIME=%d\n",LAST_MOD_TIME);
						strcpy(last_path,copy);
						strcpy(lbckpinfo,copy);
					}
				}
			}

			if (S_ISDIR(buf.st_mode)){
				char nlp[PATH_MAX]={'\0'};
				char *pnlp=&nlp[0];
				pnlp=getLastBckpinfo(pcopy,pnlp);
				//printf("pnlp:%s\n",pnlp);
				if (strlen(pnlp)>0){
					last_path=pnlp;
					strcpy(lbckpinfo,pnlp);
				}
			}
		}
		return last_path;
	}
	return NULL;
}




//creates array of bkcpfile structs with information from regular files in source directory
struct bckpfile* createSourceStruct(char** source,int size){
	int i=0;
	struct bckpfile* filestruct;
	filestruct=malloc(size*sizeof *filestruct);
	for (;i<size;i++){
		filestruct[i].s_path=malloc(PATH_MAX*sizeof(char));
		filestruct[i].d_path=malloc(PATH_MAX*sizeof(char));
		strcpy(filestruct[i].s_path,source[i]);
//		strcat(filestruct[i].s_path,"\n");
		filestruct[i].modtime=lastModTimeLong(source[i]);
	}

	return filestruct;
}


//check for file deletions on loaded bckpfile structs
char** check_file_deletion(struct bckpfile *sourcefiles,struct bckpfile *bckpinfo,int source_size,int b_size,int *ndf){
	int i=0;
	int k=0;
	int j=0;
	int exists=0;
	// **check for file deletions**
	//count deleted files
	for (i=0;i<b_size;i++){
		exists=0;
		for (k=0;k<source_size;k++){
			if(strcmp(bckpinfo[i].s_path,sourcefiles[k].s_path)==0){
				exists=1;
			}
		}
		if (exists==0){
			(*ndf)++;	
		}
	}
	char** del_paths=NULL;
	if ((*ndf)>0){ //if there are deleted files, get their bckpinfo paths

		//allocate memory for deleted files' paths
		del_paths=(char**)malloc((*ndf)*sizeof(char*));
		i=0;
		for (;i<(*ndf);i++){
			del_paths[i]=(char*)malloc(PATH_MAX*sizeof(char));
		}
		//get paths
		for (i=0;i<b_size;i++){
			exists=0;
			for (k=0;k<source_size;k++){
				if(strcmp(bckpinfo[i].s_path,sourcefiles[k].s_path)==0){
					exists=1;
				}
			}
			if (exists==0){
				strcpy(del_paths[j],bckpinfo[i].s_path);
				j++;
			}
		}
		return del_paths;
	}
	return NULL;
}

//check for file creations on loaded bckpfile structs
char** check_file_creation(struct bckpfile *sourcefiles,struct bckpfile *bckpinfo,int source_size,int b_size,int *ncf){
	int i=0;
	int k=0;
	int j=0;
	int exists=0;
	//count added files
	for (i=0;i<source_size;i++){
		exists=0;
		for (k=0;k<b_size;k++){
			if (strcmp(bckpinfo[k].s_path,sourcefiles[i].s_path)==0){
				exists=1;
			}
		}
		if (exists==0){
			(*ncf)++;
		}
	}
	char** c_paths=NULL;
	if ((*ncf)>0){ //if there are created files, get their source paths
		//allocate memory for created files' paths
		c_paths=(char**)malloc((*ncf)*sizeof(char*));
		for (i=0;i<(*ncf);i++){
			c_paths[i]=(char*)malloc(PATH_MAX*sizeof(char));
		}

		//get paths
		for (i=0;i<source_size;i++){
			exists=0;
			for (k=0;k<b_size;k++){
				if (strcmp(bckpinfo[k].s_path,sourcefiles[i].s_path)==0){
					exists=1;
				}
			}
			if (exists==0){
				strcpy(c_paths[j],sourcefiles[i].s_path);
				j++;
			}
		}	
		return c_paths;
	}
	return NULL;
}

//check for file modifications on loaded bckpfile structs
char** check_file_mods(struct bckpfile *sourcefiles,struct bckpfile *bckpinfo,int source_size,int b_size,int *nmf){
	int i=0;
	int k=0;
	int j=0;
	//count modified files
	for (i=0;i<b_size;i++){
		for (k=0;k<source_size;k++){
			if(strcmp(bckpinfo[i].s_path,sourcefiles[k].s_path)==0){
				if (bckpinfo[i].modtime!=sourcefiles[k].modtime){
					(*nmf)++;
				}
			}
		}
	}

	char** m_paths=NULL;
	if ((*nmf)>0){	//if there are modified files, get their source paths

		//allocate memory for modified files' paths
		m_paths=(char**)malloc((*nmf)*sizeof(char*));
		for (i=0;i<(*nmf);i++){
			m_paths[i]=(char*)malloc(PATH_MAX*sizeof(char));
		}

		for (i=0;i<b_size;i++){
			for (k=0;k<source_size;k++){
				if(strcmp(bckpinfo[i].s_path,sourcefiles[k].s_path)==0){
					if (bckpinfo[i].modtime!=sourcefiles[k].modtime){
						strcpy(m_paths[j],sourcefiles[k].s_path);
						j++;
					}
				}
			}
		}
		return m_paths;
	}
	return NULL;
}

//returns string array of source file paths from array of bckpfile structs 
char** getSourcePaths(struct bckpfile *files,int size){
	char** paths=(char**)malloc(size*sizeof(char*));
	int i=0;
	for (;i<size;i++){
		paths[i]=(char*)malloc(PATH_MAX*sizeof(char));
	}
	for (i=0;i<size;i++){
		strcpy(paths[i],files[i].s_path);
	}

	return paths;
}

//returns string array of destination file paths from array of bckpfile structs
char** getDestinationPaths(struct bckpfile *files,int size){
	char** paths=(char**)malloc(size*sizeof(char*));
	int i=0;
	for (;i<size;i++){
		paths[i]=(char*)malloc(PATH_MAX*sizeof(char));
	}
	for (i=0;i<size;i++){
		strcpy(paths[i],files[i].d_path);
	}

	return paths;
}

//creates bckpinfo and performs incremental backup in case of file modification
void case_mod(struct bckpfile *bckpinfo_s,char** m_paths,char* dest_path,int b_size,int nmf){
	time_t rawtime;
	struct tm * timeinfo;
	int i=0;
	int k=0;
	time(&rawtime);
	timeinfo=localtime(&rawtime);
	int thistime=mktime(timeinfo);
	//printf("thistime:%d\n",thistime);
	char* stime=(char*)malloc(21*sizeof(char));
	getFormattedTimeString(thistime,stime);
	//printf("stime:%s\n",stime);
	char* dirpath=(char*)malloc(PATH_MAX*sizeof(char));
	strcpy(dirpath,realpath(dest_path,NULL));
	strcat(dirpath,"/");
	strcat(dirpath,stime);

	//update bckpfile struct with new last modification times
	for (i=0;i<b_size;i++){
		for (k=0;k<nmf;k++){
			if (strcmp(bckpinfo_s[i].s_path,m_paths[k])==0){
				bckpinfo_s[i].modtime=lastModTimeLong(realpath(m_paths[k],NULL));
				char* newp=(char*)malloc(PATH_MAX*sizeof(char));
				strcpy(newp,dirpath);
				strcat(newp,"/");
				strcat(newp,basename(m_paths[k]));
				bckpinfo_s[i].d_path=newp;
			}
		}
	}
	
	//create new directory, copy files and create new bckpinfo from the updated struct information
	if (mkdir(dirpath,0755)!=0){
		fprintf(stderr,"error creating dir %s\n",dirpath);
		perror("mkdir");
		exit(EXIT_FAILURE);
	} else {
		char** s_paths=getSourcePaths(bckpinfo_s,b_size);
		char** dest_paths=getDestinationPaths(bckpinfo_s,b_size);
		char** backup_paths=createDestPaths(m_paths,dirpath,nmf);
		performFullBackup(m_paths,backup_paths,nmf);
		bckpinfo(dirpath,s_paths,dest_paths,b_size,thistime);
	}
}

//creates bckpinfo and performs incremental backup in case of file deletion
void case_del(struct bckpfile *bckpinfo_s,char** d_paths,char* dest_path,int b_size,int ndf){
	time_t rawtime;
	struct tm * timeinfo;
	int i=0;
	int k=0;
	time(&rawtime);
	timeinfo=localtime(&rawtime);
	int thistime=mktime(timeinfo);
	//printf("thistime:%d\n",thistime);
	char* stime=(char*)malloc(21*sizeof(char));
	getFormattedTimeString(thistime,stime);
	//printf("stime:%s\n",stime);
	char* dirpath=(char*)malloc(PATH_MAX*sizeof(char));
	strcpy(dirpath,realpath(dest_path,NULL));
	strcat(dirpath,"/");
	strcat(dirpath,stime);

	for (i=0;i<b_size;i++){
			for (k=0;k<ndf;k++){
				if (strcmp(bckpinfo_s[i].s_path,d_paths[k])==0){
					bckpinfo_s[i].s_path="DELETED";
				}
			}
		}

		struct bckpfile *newstruct;
		int new_size=b_size-ndf;
		newstruct=malloc(new_size*sizeof(*newstruct));
		k=0;
		for (i=0;i<b_size;i++){
			if (strcmp(bckpinfo_s[i].s_path,"DELETED")!=0){
				newstruct[k].s_path=(char*)malloc(PATH_MAX*sizeof(char));
				newstruct[k].d_path=(char*)malloc(PATH_MAX*sizeof(char));
				strcpy(newstruct[k].s_path,bckpinfo_s[i].s_path);
				strcpy(newstruct[k].d_path,bckpinfo_s[i].d_path);
				newstruct[k].modtime=bckpinfo_s[i].modtime;
				k++;
			}
		}

		if (mkdir(dirpath,0755)!=0){
			fprintf(stderr,"error creating dir %s\n",dirpath);
			perror("mkdir");
			exit(EXIT_FAILURE);
		} else {
			char** s_paths=getSourcePaths(newstruct,new_size);
			char** dest_paths=getDestinationPaths(newstruct,new_size);
			bckpinfo(dirpath,s_paths,dest_paths,new_size,thistime);
		}
}

//creates bckpinfo and performs incremental backup in case of file creation
void case_create(struct bckpfile *bckpinfo_s,char** c_paths,char* dest_path,int b_size,int ncf){
	time_t rawtime;
	struct tm * timeinfo;
	int i=0;
	int k=0;
	time(&rawtime);
	timeinfo=localtime(&rawtime);
	int thistime=mktime(timeinfo);
	//printf("thistime:%d\n",thistime);
	char* stime=(char*)malloc(21*sizeof(char));
	getFormattedTimeString(thistime,stime);
	//printf("stime:%s\n",stime);
	char* dirpath=(char*)malloc(PATH_MAX*sizeof(char));
	strcpy(dirpath,realpath(dest_path,NULL));
	strcat(dirpath,"/");
	strcat(dirpath,stime);

	struct bckpfile *newstruct;
	int new_size=b_size+ncf;
	newstruct=malloc(new_size*sizeof(*newstruct));
	k=0;
	for (i=0;i<b_size;i++){
		newstruct[k].s_path=(char*)malloc(PATH_MAX*sizeof(char));
		newstruct[k].d_path=(char*)malloc(PATH_MAX*sizeof(char));
		strcpy(newstruct[k].s_path,bckpinfo_s[i].s_path);
		strcpy(newstruct[k].d_path,bckpinfo_s[i].d_path);
		newstruct[k].modtime=bckpinfo_s[i].modtime;
		k++;
	}

	
	if (mkdir(dirpath,0755)!=0){
		fprintf(stderr,"error creating dir %s\n",dirpath);
		perror("mkdir");
		exit(EXIT_FAILURE);
	} else {
		char** newfiles=createDestPaths(c_paths,dirpath,ncf);
		for (i=0;i<ncf;i++){
			newstruct[k].s_path=(char*)malloc(PATH_MAX*sizeof(char));
			newstruct[k].d_path=(char*)malloc(PATH_MAX*sizeof(char));
			strcpy(newstruct[k].s_path,c_paths[i]);
			strcpy(newstruct[k].d_path,newfiles[i]);
			newstruct[k].modtime=lastModTimeLong(c_paths[i]);
		}
		char** dest_paths=getDestinationPaths(newstruct,new_size);
		performFullBackup(c_paths,newfiles,ncf);
		char** s_paths=getSourcePaths(newstruct,new_size);
		bckpinfo(dirpath,s_paths,dest_paths,new_size,thistime);
	}
}

//creates bckpinfo and performs incremental backup in case of file modification and creation
void case_create_mod(struct bckpfile *bckpinfo_s,char** c_paths,char** m_paths,char* dest_path,int b_size,int ncf,int nmf){
	time_t rawtime;
	struct tm * timeinfo;
	int i=0;
	int k=0;
	time(&rawtime);
	timeinfo=localtime(&rawtime);
	int thistime=mktime(timeinfo);
	//printf("thistime:%d\n",thistime);
	char* stime=(char*)malloc(21*sizeof(char));
	getFormattedTimeString(thistime,stime);
	//printf("stime:%s\n",stime);
	char* dirpath=(char*)malloc(PATH_MAX*sizeof(char));
	strcpy(dirpath,realpath(dest_path,NULL));
	strcat(dirpath,"/");
	strcat(dirpath,stime);

	for (i=0;i<b_size;i++){
		for (k=0;k<nmf;k++){
			if (strcmp(bckpinfo_s[i].s_path,m_paths[k])==0){
				bckpinfo_s[i].modtime=lastModTimeLong(realpath(m_paths[k],NULL));
				char* newp=(char*)malloc(PATH_MAX*sizeof(char));
				strcpy(newp,dirpath);
				strcat(newp,"/");
				strcat(newp,basename(m_paths[k]));
				bckpinfo_s[i].d_path=newp;
			}
		}
	}

	struct bckpfile *newstruct;
	int new_size=b_size+ncf;
	newstruct=malloc(new_size*sizeof(*newstruct));
	k=0;
	for (i=0;i<b_size;i++){
		newstruct[k].s_path=(char*)malloc(PATH_MAX*sizeof(char));
		newstruct[k].d_path=(char*)malloc(PATH_MAX*sizeof(char));
		strcpy(newstruct[k].s_path,bckpinfo_s[i].s_path);
		strcpy(newstruct[k].d_path,bckpinfo_s[i].d_path);
		newstruct[k].modtime=bckpinfo_s[i].modtime;
		k++;
	}

	if (mkdir(dirpath,0755)!=0){
		fprintf(stderr,"error creating dir %s\n",dirpath);
		perror("mkdir");
		exit(EXIT_FAILURE);
	} else {
		char** newfiles=createDestPaths(c_paths,dirpath,ncf);
		for (i=0;i<ncf;i++){
			newstruct[k].s_path=(char*)malloc(PATH_MAX*sizeof(char));
			newstruct[k].d_path=(char*)malloc(PATH_MAX*sizeof(char));
			strcpy(newstruct[k].s_path,c_paths[i]);
			strcpy(newstruct[k].d_path,newfiles[i]);
			newstruct[k].modtime=lastModTimeLong(c_paths[i]);
		}
		char** s_paths=getSourcePaths(newstruct,new_size);
		char** dest_paths=getDestinationPaths(newstruct,new_size);
		char** mod_paths=createDestPaths(m_paths,dirpath,nmf);
		performFullBackup(m_paths,mod_paths,nmf);
		performFullBackup(c_paths,newfiles,ncf);
		bckpinfo(dirpath,s_paths,dest_paths,new_size,thistime);
	}
}

//creates bckpinfo and performs incremental backup in case of file creation, modification and deletion
void case_create_mod_del(struct bckpfile *bckpinfo_s,char** c_paths,char** m_paths,char** d_paths,char* dest_path,int b_size,int ncf,int nmf,int ndf){
	time_t rawtime;
	struct tm * timeinfo;
	int i=0;
	int k=0;
	time(&rawtime);
	timeinfo=localtime(&rawtime);
	int thistime=mktime(timeinfo);
	//printf("thistime:%d\n",thistime);
	char* stime=(char*)malloc(21*sizeof(char));
	getFormattedTimeString(thistime,stime);
	//printf("stime:%s\n",stime);
	char* dirpath=(char*)malloc(PATH_MAX*sizeof(char));
	strcpy(dirpath,realpath(dest_path,NULL));
	strcat(dirpath,"/");
	strcat(dirpath,stime);

	for (i=0;i<b_size;i++){
		for (k=0;k<nmf;k++){
			if (strcmp(bckpinfo_s[i].s_path,m_paths[k])==0){
				bckpinfo_s[i].modtime=lastModTimeLong(realpath(m_paths[k],NULL));
				char* newp=(char*)malloc(PATH_MAX*sizeof(char));
				strcpy(newp,dirpath);
				strcat(newp,"/");
				strcat(newp,basename(m_paths[k]));
				bckpinfo_s[i].d_path=newp;
			}
		}
	}

	for (i=0;i<b_size;i++){
		for (k=0;k<ndf;k++){
			if (strcmp(bckpinfo_s[i].s_path,d_paths[k])==0){
				bckpinfo_s[i].s_path="DELETED";
			}
		}
	}
	struct bckpfile *tempnewstruct;
	int new_size=b_size-ndf;
	tempnewstruct=malloc(new_size*sizeof(*tempnewstruct));
	k=0;
	for (i=0;i<b_size;i++){
		if (strcmp(bckpinfo_s[i].s_path,"DELETED")!=0){
			tempnewstruct[k].s_path=(char*)malloc(PATH_MAX*sizeof(char));
			tempnewstruct[k].d_path=(char*)malloc(PATH_MAX*sizeof(char));
			strcpy(tempnewstruct[k].s_path,bckpinfo_s[i].s_path);
			strcpy(tempnewstruct[k].d_path,bckpinfo_s[i].d_path);
			tempnewstruct[k].modtime=bckpinfo_s[i].modtime;
			k++;
		}
	}

	struct bckpfile *newstruct;
	new_size=new_size+ncf;
	newstruct=malloc(new_size*sizeof(*newstruct));
	k=0;
	for (i=0;i<(new_size-ncf);i++){
		newstruct[k].s_path=(char*)malloc(PATH_MAX*sizeof(char));
		newstruct[k].d_path=(char*)malloc(PATH_MAX*sizeof(char));
		strcpy(newstruct[k].s_path,tempnewstruct[i].s_path);
		strcpy(newstruct[k].d_path,tempnewstruct[i].d_path);
		newstruct[k].modtime=tempnewstruct[i].modtime;
		k++;
	}

	if (mkdir(dirpath,0755)!=0){
		fprintf(stderr,"error creating dir %s\n",dirpath);
		perror("mkdir");
		exit(EXIT_FAILURE);
	} else {
		char** newfiles=createDestPaths(c_paths,dirpath,ncf);
		for (i=0;i<ncf;i++){
			newstruct[k].s_path=(char*)malloc(PATH_MAX*sizeof(char));
			newstruct[k].d_path=(char*)malloc(PATH_MAX*sizeof(char));
			strcpy(newstruct[k].s_path,c_paths[i]);
			strcpy(newstruct[k].d_path,newfiles[i]);
			newstruct[k].modtime=lastModTimeLong(c_paths[i]);
		}
		char** s_paths=getSourcePaths(newstruct,new_size);
		char** dest_paths=getDestinationPaths(newstruct,new_size);
		char** mod_paths=createDestPaths(m_paths,dirpath,nmf);
		performFullBackup(m_paths,mod_paths,nmf);
		performFullBackup(c_paths,newfiles,ncf);
		bckpinfo(dirpath,s_paths,dest_paths,new_size,thistime);

	}
}

//creates bckpinfo and performs incremental backup in case file modification and deletion
void case_mod_del(struct bckpfile *bckpinfo_s,char** m_paths,char** d_paths,char* dest_path,int b_size,int nmf,int ndf){
	time_t rawtime;
	struct tm * timeinfo;
	int i=0;
	int k=0;
	time(&rawtime);
	timeinfo=localtime(&rawtime);
	int thistime=mktime(timeinfo);
	//printf("thistime:%d\n",thistime);
	char* stime=(char*)malloc(21*sizeof(char));
	getFormattedTimeString(thistime,stime);
	//printf("stime:%s\n",stime);
	char* dirpath=(char*)malloc(PATH_MAX*sizeof(char));
	strcpy(dirpath,realpath(dest_path,NULL));
	strcat(dirpath,"/");
	strcat(dirpath,stime);

	for (i=0;i<b_size;i++){
		for (k=0;k<nmf;k++){
			if (strcmp(bckpinfo_s[i].s_path,m_paths[k])==0){
				bckpinfo_s[i].modtime=lastModTimeLong(realpath(m_paths[k],NULL));
				char* newp=(char*)malloc(PATH_MAX*sizeof(char));
				strcpy(newp,dirpath);
				strcat(newp,"/");
				strcat(newp,basename(m_paths[k]));
				bckpinfo_s[i].d_path=newp;
			}
		}
	}

	for (i=0;i<b_size;i++){
		for (k=0;k<ndf;k++){
			if (strcmp(bckpinfo_s[i].s_path,d_paths[k])==0){
				bckpinfo_s[i].s_path="DELETED";
			}
		}
	}
	struct bckpfile *tempnewstruct;
	int new_size=b_size-ndf;
	tempnewstruct=malloc(new_size*sizeof(*tempnewstruct));
	k=0;
	for (i=0;i<b_size;i++){
		if (strcmp(bckpinfo_s[i].s_path,"DELETED")!=0){
			tempnewstruct[k].s_path=(char*)malloc(PATH_MAX*sizeof(char));
			tempnewstruct[k].d_path=(char*)malloc(PATH_MAX*sizeof(char));
			strcpy(tempnewstruct[k].s_path,bckpinfo_s[i].s_path);
			strcpy(tempnewstruct[k].d_path,bckpinfo_s[i].d_path);
			tempnewstruct[k].modtime=bckpinfo_s[i].modtime;
			k++;
		}
	}

	if (mkdir(dirpath,0755)!=0){
		fprintf(stderr,"error creating dir %s\n",dirpath);
		perror("mkdir");
		exit(EXIT_FAILURE);
	} else {
		char** s_paths=getSourcePaths(tempnewstruct,new_size);
		char** dest_paths=getDestinationPaths(tempnewstruct,new_size);
		char** backup_paths=createDestPaths(m_paths,dirpath,nmf);
		performFullBackup(m_paths,backup_paths,nmf);
		bckpinfo(dirpath,s_paths,dest_paths,new_size,thistime);
	}

}

//creates bckpinfo and performs incremental backup in case of file creation and deletion
void case_create_del(struct bckpfile *bckpinfo_s,char** c_paths,char** d_paths,char* dest_path,int b_size,int ncf,int ndf){
	time_t rawtime;
	struct tm * timeinfo;
	int i=0;
	int k=0;
	time(&rawtime);
	timeinfo=localtime(&rawtime);
	int thistime=mktime(timeinfo);
	//printf("thistime:%d\n",thistime);
	char* stime=(char*)malloc(21*sizeof(char));
	getFormattedTimeString(thistime,stime);
	//printf("stime:%s\n",stime);
	char* dirpath=(char*)malloc(PATH_MAX*sizeof(char));
	strcpy(dirpath,realpath(dest_path,NULL));
	strcat(dirpath,"/");
	strcat(dirpath,stime);

	for (i=0;i<b_size;i++){
		for (k=0;k<ndf;k++){
			if (strcmp(bckpinfo_s[i].s_path,d_paths[k])==0){
				bckpinfo_s[i].s_path="DELETED";
			}
		}
	}
	struct bckpfile *tempnewstruct;
	int new_size=b_size-ndf;
	tempnewstruct=malloc(new_size*sizeof(*tempnewstruct));
	k=0;
	for (i=0;i<b_size;i++){
		if (strcmp(bckpinfo_s[i].s_path,"DELETED")!=0){
			tempnewstruct[k].s_path=(char*)malloc(PATH_MAX*sizeof(char));
			tempnewstruct[k].d_path=(char*)malloc(PATH_MAX*sizeof(char));
			strcpy(tempnewstruct[k].s_path,bckpinfo_s[i].s_path);
			strcpy(tempnewstruct[k].d_path,bckpinfo_s[i].d_path);
			tempnewstruct[k].modtime=bckpinfo_s[i].modtime;
			k++;
		}
	}

	struct bckpfile *newstruct;
	new_size=new_size+ncf;
	newstruct=malloc(new_size*sizeof(*newstruct));
	k=0;
	for (i=0;i<(new_size-ncf);i++){
		newstruct[k].s_path=(char*)malloc(PATH_MAX*sizeof(char));
		newstruct[k].d_path=(char*)malloc(PATH_MAX*sizeof(char));
		strcpy(newstruct[k].s_path,tempnewstruct[i].s_path);
		strcpy(newstruct[k].d_path,tempnewstruct[i].d_path);
		newstruct[k].modtime=tempnewstruct[i].modtime;
		k++;
	}

	if (mkdir(dirpath,0755)!=0){
		fprintf(stderr,"error creating dir %s\n",dirpath);
		perror("mkdir");
		exit(EXIT_FAILURE);
	} else {
		char** newfiles=createDestPaths(c_paths,dirpath,ncf);
		for (i=0;i<ncf;i++){
			newstruct[k].s_path=(char*)malloc(PATH_MAX*sizeof(char));
			newstruct[k].d_path=(char*)malloc(PATH_MAX*sizeof(char));
			strcpy(newstruct[k].s_path,c_paths[i]);
			strcpy(newstruct[k].d_path,newfiles[i]);
			newstruct[k].modtime=lastModTimeLong(c_paths[i]);
		}
		char** s_paths=getSourcePaths(newstruct,new_size);
		char** dest_paths=getDestinationPaths(newstruct,new_size);
		performFullBackup(c_paths,newfiles,ncf);
		bckpinfo(dirpath,s_paths,dest_paths,new_size,thistime);

	}

}

//checks for changes in dir1 (loaded on sourcefiles array of structs) compared with last bckpinfo created (loaded on bckpinfo_s array of structs)
void new_bckpinfo(char* dest_path,struct bckpfile *sourcefiles,struct bckpfile *bckpinfo_s,int source_size,int b_size){
	int i=0;
	int k=0;
	int ndf=0; //number of deleted files
	int ncf=0; //number of created files
	int nmf=0; //number of modified files
	int* pndf=&ndf;
	int* pncf=&ncf;
	int* pnmf=&nmf;
	//print array of structs

	/*
	printf("source_size:%d\n",source_size);
	printf("b_size:%d\n",b_size);
	for (i=0;i<source_size;i++){
		printf("sourcefiles[%d].s_path:%s\n",i,sourcefiles[i].s_path);
		printf("sourcefiles[%d].d_path:%s\n",i,sourcefiles[i].d_path);
		printf("sourcefiles[%d].modtime:%d\n\n",i,sourcefiles[i].modtime);
	}

	for (i=0;i<b_size;i++){
		printf("bckpinfo[%d].s_path:%s\n",i,bckpinfo[i].s_path);
		printf("bckpinfo[%d].d_path:%s\n",i,bckpinfo[i].d_path);
		printf("bckpinfo[%d].modtime:%d\n\n",i,bckpinfo[i].modtime);
	}
	*/

	struct bckpfile *newstruct;

	//check for file deletions**
	char** d_paths=check_file_deletion(sourcefiles,bckpinfo_s,source_size,b_size,pndf);

	//check for file additions**
	char** c_paths=check_file_creation(sourcefiles,bckpinfo_s,source_size,b_size,pncf);

	//check for modifications
	char** m_paths=check_file_mods(sourcefiles,bckpinfo_s,source_size,b_size,pnmf);

	if (ncf==0 && ndf==0 && nmf>0){
		case_mod(bckpinfo_s,m_paths,dest_path,b_size,nmf);
	}


	if (ncf==0 && ndf>0 && nmf==0){
		case_del(bckpinfo_s,d_paths,dest_path,b_size,ndf);
	}

	if (ncf>0 && ndf==0 && nmf==0){
		case_create(bckpinfo_s,c_paths,dest_path,b_size,ncf);
	}

	if (ncf>0 && ndf==0 && nmf>0){
		case_create_mod(bckpinfo_s,c_paths,m_paths,dest_path,b_size,ncf,nmf);
	}
	if (ncf>0 && ndf>0 && nmf>0){
		case_create_mod_del(bckpinfo_s,c_paths,m_paths,d_paths,dest_path,b_size,ncf,nmf,ndf);
	}
	if (ncf==0 && ndf>0 && nmf>0){
		case_mod_del(bckpinfo_s,m_paths,d_paths,dest_path,b_size,nmf,ndf);
	}
	if(ncf>0 && ndf>0 && nmf==0){
		case_create_del(bckpinfo_s,c_paths,d_paths,dest_path,b_size,ncf,ndf);
	}

//	printf("ndf:%d\n",ndf);
//	printf("ncf:%d\n",ncf);
//	printf("nmf:%d\n",nmf);

}


//checks for modifications on source directory path
int checkModifications(char* path,char* dest_path){
	char** source_paths=readDir(path);
	int size=filesindir(path);
	struct bckpfile *sourcefiles;
	struct bckpfile *bckpinfo;
	sourcefiles=createSourceStruct(source_paths,size);
	char* lbckpinfo=(char*)malloc(PATH_MAX*sizeof(char));
	getLastBckpinfo(dest_path,lbckpinfo);
	//printf("lb:%s\n",lbckpinfo);
	//printf("dirname:%s\n",lbckpinfo);
	int filen=linecounter(realpath(lbckpinfo,NULL));
	//printf("filen:%d\n",filen);
	unsigned int btime=0;
	unsigned int *pbtime=&btime;
	bckpinfo=loadBckpinfo(realpath(lbckpinfo,NULL),filen,pbtime);
	new_bckpinfo(dest_path,sourcefiles,bckpinfo,size,(filen-1)/3);
	return 0;
}

