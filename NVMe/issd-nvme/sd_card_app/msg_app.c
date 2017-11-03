#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<sched.h>
#include<mqueue.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#define TAR_MQ "/tar_mq"
#define TAR_MQ_S "/tar_mq_s"
#define SUCCESS 0
#define ERROR -1
int main()
{
	cpu_set_t cpuset;
	int ret,tar_mq,tar_mq_s;
	char *data;
	struct mq_attr attr;

	CPU_ZERO (&cpuset); /* Initializing the CPU set to be the empty set */
	CPU_SET (1 , &cpuset);  /* setting CPU on cpuset */

	ret = pthread_setaffinity_np (pthread_self(), sizeof (cpu_set_t), &cpuset);
	if (ret != 0) {
		perror ("pthread_setset_thread_affinity_np");
	}
	ret = pthread_getaffinity_np (pthread_self(), sizeof (cpu_set_t), &cpuset);
	if (ret != 0) {
		perror ("pthread_getset_thread_affinity_np");
	}
	/*creation of tar_mq*/
	if((tar_mq=mq_open(TAR_MQ,O_RDWR|O_CREAT,S_IWUSR|S_IRUSR,NULL))<0){
		printf("Error in opening/creating the tar_mq\n");
		return ERROR;
	}
	if((tar_mq_s=mq_open(TAR_MQ_S,O_RDWR|O_CREAT,S_IWUSR|S_IRUSR,NULL))<0){
		printf("Error in opening/creating the tar_mq\n");
		return ERROR;
	}
	/*recieving the attributes ofthe MSG Queue*/
	if(mq_getattr(tar_mq,&attr)==ERROR){
		printf("unable to fetch the tar_mq attribute \n");
		return ERROR;
	}
	if((data=(char *)malloc(attr.mq_msgsize*(sizeof(char))))==NULL){
		printf("allocation of data buffer for tar_mq failed\n");
		return ERROR;
	}
	while(1){
		/*recieve the MSG from Queue*/
		ret=1;
		if(mq_receive(tar_mq,data,(attr.mq_msgsize*sizeof(char)),&ret)==ERROR){
			printf("Error in recieving data from Tar_mq  \n");
			return ERROR;
		}
		ret=data[0];
		switch(ret){
			case 1:
					do{
						if(access("/run/ftl_pmt",F_OK)){
							system("mkdir -p /run/ftl_pmt &> /dev/null");
							system("chmod 777 /run/ftl_pmt &> /dev/null");
							if(!(access("/dev/mmcblk0p1",F_OK))){
								system ("mount /dev/mmcblk0p1 /run/ftl_pmt &>/dev/null");
							}else{
								printf("Memory not found \n");
							}
						}else{
							printf("Memory is already mounted\n");
						}
					}while(access("/run/ftl_pmt",F_OK));
					printf("tar operation has been started\n");
					system("tar -cvzf metaData.tar.gz ftl.dat abm.dat");
					printf("tar operation finished copy the tar file to the memory_card\n");
					system("cp -v metaData.tar.gz ftl_pmt/ && sync");
					printf("data is copied in the memory card...\n");
					printf("STORING FLASH MANAGEMENT INFO SUCCESS...\n READY to Poweroff or insert the nvme module in host\n");
					break;
			case 2:
					if(!(access("/run/ftl_pmt/metaData.tar.gz",F_OK))){
					printf("untar operation has been started\n");
					system ("cp ftl_pmt/metaData.tar.gz .");
					printf("data is copied from the memory card...\n");
					system ("tar -xvzf metaData.tar.gz ");
					printf("untar operation has been done\n");
					} else {
						printf("metaData.tar.gz not found in memory card \n");
					}
					printf("Assign Data Value\n");
					data[0]=5;
					printf("sending msg iss_nvme.. You can proceed\n");
					usleep(1);
					if(mq_send(tar_mq_s,data,attr.mq_msgsize,1)==ERROR){
						printf("Error in Transfer data to tar_mq msg queue by TAR app \n");
					}
					break;
			default:
					printf("closing and removing the tar_mq\n");
					/*close the MSG Queue*/
					if(mq_close(tar_mq)<0){
						printf("error in closing the tar_mq \n");
						return ERROR;
					}
					/*remove the MSG Queue*/
					if(mq_unlink(TAR_MQ)<0){
						printf("error in removing the tar_mq queue \n");
						return ERROR;
					}
					if(mq_close(tar_mq_s)<0){
						printf("error in closing the tar_mq \n");
						return ERROR;
					}
					/*remove the MSG Queue*/
					if(mq_unlink(TAR_MQ_S)<0){
						printf("error in removing the tar_mq queue \n");
						return ERROR;
					}
					break;
		}
		if(ret>=3){
			break;
		}
	}
	return SUCCESS;
}
