#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>
#include<sys/mman.h>
#include<sys/ipc.h>
#include<time.h>
#include<pwd.h>
#include<grp.h>

#define MAPFILENAME "map"
#define MESSAGEFILENAME "/bin/bash"

struct mymsgbuf{ // for the counter message
	long mtype;
	int counter;
	long int max;
};

int main(){
		
	key_t key;
	if ((key = ftok(MESSAGEFILENAME, 0)) < 0){
		printf("Error: cannot generate the key\n");
		exit(-1);
	}

	int msgdes;
	if((msgdes = msgget(key, 0666|IPC_CREAT)) < 0){
		printf("Error: cannot create the messagebox\n");
		exit(-1);
	}

	struct mymsgbuf mybuf;
	
	if(msgrcv(msgdes, (struct msgbuf*) &mybuf, sizeof(int)+sizeof(long int), 1, 0) < 0){ // receiving the counter message
		printf("Error: cannot receive the message\n");
		exit(-1);
	}

	int counter = mybuf.counter;
	long int max = mybuf.max;

	struct filedata{ // the main struct which we use in the map file
		char name[max];
		mode_t mode;
		uid_t uid;
		gid_t gid;
		off_t size;
		time_t ctime;
		time_t mtime;
	} *begin, *data;

	int fd = open(MAPFILENAME, O_RDWR|O_CREAT, 0666);
	if (fd < 0){
		printf("Error: cannot open the map file\n");
		exit(-1);
	}

	begin = (struct filedata*)mmap(NULL, counter*sizeof(struct filedata), PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);
	if(begin == MAP_FAILED){
		printf("Error: cannot map\n");
		exit(-1);
	}

	data = begin;

	int i;
	
	for(i=0; i < counter; i++){ // reading from the file

		struct passwd* userdata = getpwuid(data->uid);
		if(userdata == NULL){
			printf("Error: cannot get the username\n");
			exit(-1);
		}

		struct group* groupdata = getgrgid(data->gid);
		if(groupdata == NULL){
			printf("Error: cannot get the groupname\n");
			exit(-1);
		}

		char* createtime = ctime(&(data->ctime));
		if(createtime == NULL){
			printf("Error: cannot get the ctime\n");
			exit(-1);
		}

		int i;
		int mode = data->mode;
		int mask = mode%1024;

		char* type;

		if(S_ISLNK(data->mode)) type = "link";
		if(S_ISREG(data->mode)) type = "regular";
		if(S_ISDIR(data->mode)) type = "directory";
		if(S_ISCHR(data->mode)||S_ISBLK(data->mode)) type = "device";
		if(S_ISFIFO(data->mode)) type = "FIFO";
		if(S_ISSOCK(data->mode)) type = "socket";

		printf("File name: %s\n", data->name);
		printf("User name: %s\nGroup name: %s\n", userdata->pw_name, groupdata->gr_name);
		printf("File type: %s\nAccess rights mask: %o\nSize: %d bytes\n", type, mask, data->size);
		printf("Last status change time: %s", createtime);

		char* modiftime = ctime(&(data->mtime));
		if(modiftime == NULL){
			printf("Error: cannot get the mtime\n");
			exit(-1);
		}

		printf("Last modification time: %s\n", modiftime);

		data++;
	}

	if(munmap((void*)begin, counter*sizeof(struct filedata)) < 0){
		printf("Error: cannot unmap\n");
		exit(-1);
	}

	return 0;

}
