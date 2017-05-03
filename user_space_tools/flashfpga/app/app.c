#include <stdint.h>
//#include <stdio.h>
#include <sched.h>

#include "fpga_config_new.h"
#define MAP_SIZE 1024 

unsigned int    *bar_base;

void reset_asmi_ip(void)
{
	int i;

	bar_base[CTRL_REG] = 0;
	for(i=0;i<10;i++);
	bar_base[CTRL_REG] = 1 << ASMI_RST;
	for (i=0; i<40; i++);
	bar_base[CTRL_REG] = 0;
	for (i=0; i<40; i++);
	while ((bar_base[CTRL_REG] & (1 << ASMI_RST)) != 0) {
		printf("Reseting ASMI\n");
	}
	printf("ASMI IP Reset Done\n");
}

int read_epcq_device_id(void)
{
	int i;

RETRY_ID:
	bar_base[CTRL_REG] = 1 << EPCQ_ID;
	for (i=0; i<20; i++);
	while ((bar_base[CTRL_REG] & (1 << EPCQ_ID)) != 0) {
		printf("Reading device id \r");
	}
	if (bar_base[EPCQ_ID_REG] != 0x19) goto RETRY_ID;
	return bar_base[EPCQ_ID_REG];
}

void erase_epcq_device(void)
{
	int i;
	time_t curtime1, curtime2;
	time(&curtime1);
	printf("Erase start time = %s", ctime(&curtime1));
	bar_base[CTRL_REG] = 1 << BLK_ERS;
	for (i=0; i<20; i++);
	while ((bar_base[CTRL_REG] & (1 << BLK_ERS)) != 0) {
		printf("Erasing\r");
	}
	time(&curtime2);
	printf("Erase stop time = %s \n", ctime(&curtime2));
	printf("EPCQ is erased completly\n");
}

void en_4_byte_add(void)
{
	int i;

	bar_base[CTRL_REG] = 1 << EN_ADDR_MODE;
	for (i=0; i<20; i++);
	while ((bar_base[CTRL_REG] & (1 << EN_ADDR_MODE)) != 0) {
		printf("Enabling four byte address \r");
	}
	printf("4 byte address mode is enabled \n");
}

unsigned int reverse_bits(unsigned int n)
{
	unsigned int temp = 0;
	int i;

	for (i = 0; i < 32; i++)
		if (n & (1 << i))
			temp |= 1 << ((32 -1) - i);

	return temp;
}

unsigned int swap_byte(unsigned int n)
{
	unsigned int b0 = (0x000000FF & n);
	unsigned int b1 = (0x0000FF00 & n);
	unsigned int b2 = (0x00FF0000 & n);
	unsigned int b3 = (0xFF000000 & n);

	n ^= b0 | b1 | b2 | b3;

	b0 = reverse_bits(b0);
	b1 = reverse_bits(b1);
	b2 = reverse_bits(b2);
	b3 = reverse_bits(b3);

	n |= (b0 >> 24) | (b1 >> 8) | (b2 << 8) | (b3 << 24);

	return n;
}

/*-----------Writing EPCQ----------*/
void program_epcq(char *path)
{
	int i ,j;
	unsigned int addr, data[70], add;
	int fd, count = 0;
	char temp[4];
	struct stat buff;
	int size;
	unsigned int f_data, s_data;
	time_t curtime1, curtime2;
	time_t curtime3, curtime4;
	int pg_wrt_cnt, pg_skp_cnt = 0;

	fd = open(path,  O_RDWR);
	if ( fd < 0) {
		perror("open");
		return;
	}
	fstat(fd, &buff);

	size = buff.st_size;
	printf("File Size = %d\n", size);
	printf("No of Pages = %d\n", size/256);
	addr = 0x00000000;

	time(&curtime1);
	printf("Write start time = %s\n", ctime(&curtime1));
	printf("Begin writing\n");
	
	while ( size >= 0)
	{
		for (i = 0; i < 64; i++)
		{
			read(fd, &f_data, sizeof(f_data));
			data[i] = swap_byte(f_data);
			if ((data[i] ^ 0xFFFFFFFF) != 0)
				pg_wrt_cnt++;
		}	
		size -= 256;
		if ( pg_wrt_cnt == 0) {
			pg_skp_cnt++;
			addr = addr + 0x100;
			continue;
		}
		
		pg_wrt_cnt = 0;
		bar_base[0x06] = 0xff;
		
		for(i=0;i<64;i++)   
		{   
			bar_base[EPCQ_WR_DATA_REG] = data[i];
		}
		
		bar_base[EPCQ_ADDR_REG] = addr;              //Giving address
		bar_base[CTRL_REG] = 1 << EPCQ_WR;                //Giving write command      
		while ((bar_base[CTRL_REG] & (1 << EPCQ_WR)) != 0) {
			printf("Writing\r");
  			usleep(1000);
		}

		addr = addr + 0x100;
		count++;
		if (count % 10000 == 0)
		{	
			printf("-------%d Pages written--------\n", count);
		}
	}
	
	time(&curtime2);

	printf("Write stop time = %s\n", ctime(&curtime2));
	printf("Writing done\n");
	printf("Total Page written = %d\n", count);
	printf("Total page skipped = %d\n", pg_skp_cnt);
}

/*------------Reading EPCQ Device-------------*/
void read_epcq(void)
{
	unsigned int addr;
	int i, j;
	printf("Begin reading\n");
	addr = 0x00000000;
	for(j=0;j<1;j++)
	{
		for(i=0;i<256;i++)  
		{   
			bar_base[EPCQ_ADDR_REG] = addr;                    //  giving adress       
			bar_base[CTRL_REG ] = 0x00000010;            //     read command
				while ((bar_base[0x00] & (0x00000010)) != 0) {
					printf("Reading\r");
				}  
			printf("addr = %x data = %x \n" , addr ,bar_base[0x04]);
			addr = addr + 0x01;
		}
	}
	printf("\nReading done\n");
}

uint64_t get_bar_address()
{
 	int lineNumber;
        int count = 0;
        uint64_t bar_address;
        lineNumber=15;
        char filename[] = "/sys/bus/pci/devices/0000:01:00.0/resource";
        FILE *file = fopen(filename, "r");
        if ( file != NULL )
        {
                char line[20];
                while(fgets(line, sizeof line, file)!= NULL)
                {
                        if(count == lineNumber){
                                //printf("string = %s\n",line);
                                bar_address = (uint64_t)strtol(line,NULL,16);
                        	//printf("bar address= %lx\n", bar_address);
                                break;
                        }
                        else count++;
                }
                fclose(file);
        }
        else
                printf("File not opened. FPGA is not detected\n");
        return bar_address;
}

int main(int argc, char *argv[])
{
	printf("New flashfpga \n");
	printf("New flashfpga \n");
	FILE 			*fptr;
	unsigned int 	device_id;
	int 			memfd, i = 0;
	char 			path[200] = "/sys/bus/pci/devices/0000";
	char 			*temp;
	char            bus[100] = "";
	struct stat st;
#if 0
	cpu_set_t cs;
	CPU_ZERO (&cs);
	CPU_SET (1, &cs); /*go for cpu-1*/
	pthread_setaffinity_np (pthread_self (), sizeof (cpu_set_t), &cs);
#endif
	if (argc != 2) {
		printf ("Usage: /upg <path-to-rpd> <bar3 address>\n");
		return -1;
	}
	
	uint64_t paddr = NULL;
        paddr = get_bar_address();
	printf("paddr = %p\n", paddr);

	memfd = open( "/dev/mem", O_RDWR | O_SYNC );

	if(memfd < 0){
		perror("open");
		return 0;
	}
	temp = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, memfd, paddr);
	
	if (temp == MAP_FAILED)
		errx(1, "mmap failure");
	bar_base = (unsigned int *)temp;
	reset_asmi_ip();

	device_id = read_epcq_device_id();
	printf ("\nDevice ID: %x\n", device_id);

	if (device_id == PREDEFINE_ID)
	{
		erase_epcq_device();

		en_4_byte_add();

		program_epcq(argv[1]);

	}

	 close(memfd);
}
