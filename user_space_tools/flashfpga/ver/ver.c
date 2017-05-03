#define _GNU_SOURCE

#include <stdint.h>
#include <sched.h>

#include "fpga_config_new.h"
#define MAP_SIZE 1024 

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
                                bar_address = (uint64_t)strtol(line,NULL,16);
                        	//printf("bar address= %lx\n", bar_address);
                                break;
                        }
                        else count++;
                }
                fclose(file);
        }
        else
                printf("File not opened. FPGA is not detected.\n");
        return bar_address;
}

int main()
{
	int memfd;
	uint32_t* version_address;
	uint64_t paddr = NULL;
        char* temp;

	paddr = get_bar_address();
	//printf("paddr = %p\n", paddr);

	memfd = open( "/dev/mem", O_RDWR | O_SYNC );

	if(memfd < 0){
		perror("open");
		return 0;
	}
	temp = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, memfd, paddr);
	
	if (temp == MAP_FAILED)
		errx(1, "mmap failure");
	
	version_address = (uint32_t*)(temp + 0x1c);
        //printf("%02x\n",*version_address);
	printf("%02x.%02x.%02x\n",
        (uint8_t)(((*version_address) & 0x00ff0000)>>16),
        (uint8_t)(((*version_address) & 0x0000ff00)>>8),
        (uint8_t)(((*version_address) & 0x000000ff)>>0));
        
	return 0;
}
