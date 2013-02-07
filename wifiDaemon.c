/*
	->try resolove an wired connection, if fails continue else end
	->get config info
		->try last used config, if not specified or fails continue
	->add config device and network info to the highest priority of there q's (does not check if the deivices are valid)
	->create another q of found networks, where the evaluated best connection has highest priority
	-> starting from the the first working device found, first try the networks from config file, if they all fail try
		from the second q which contains the the networks found.
	


*/


#include <arpa/inet.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <netdb.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <net/if_types.h>
#include <net/if_media.h>
#include <net80211/ieee80211.h>
#include <net80211/ieee80211_ioctl.h>
#include <string.h>


#define MAX_LINE_LEN 256
#define MAX_DEVICES 20

struct network {
	char name[20];
	char key[20];
};

struct configReq {
	struct network networks[20];
	char devices[MAX_DEVICES][10];
	int device_cnt;
	int network_cnt;
};
struct deviceReq {

	char devices[MAX_DEVICES][10];
	int device_cnt;
};
struct networkReq {

	struct network networks[20]; // are key will hold are encryption type
	int network_cnt;
};

void parseConfigFile(struct configReq *);
void getNetworks(char *);
void getDevices(struct deviceReq *);
void setnwid();

int main()
{
	struct configReq cr;
	parseConfigFile(&cr); // getting are device and network info from are config file
	



    return 0;

}

void getDevices(struct deviceReq *dr)
{

}


void parseConfigFile(struct configReq *cr)
{

	cr->device_cnt = 0;
	cr->network_cnt = -1;

	char line[MAX_LINE_LEN + 1];
    int m_FLAG = 0;
	int i;

	FILE* config_fp;
    config_fp = fopen( "wifidaemon.config", "r" );
	
	
    while(fgets(line, MAX_LINE_LEN, config_fp ) != NULL) //each line
    { 

        if(line != NULL && line[0] != '#' && strnlen(line,MAX_LINE_LEN) > 1)
        {
			
			if(strncmp(line,"devices",7) == 0){

				m_FLAG = 0;
				continue;
			} else if(strncmp(line,"network", 7) == 0){
				
				m_FLAG = 1;
				cr->network_cnt++;
				cr->networks[cr->network_cnt].key[0] = 0;//intitialize key[0] so we can test for empty strings
				continue;
			}
			
			
           	
    	    if(m_FLAG == 0) //if at a device
			{

				if(line[0] == '\t')
					strcpy(cr->devices[cr->device_cnt], &line[1]);
					cr->device_cnt++;



			}else if(m_FLAG == 1){  //if at a network 

				i = 0;
				if(line[0] == '\t')
				{
					
					if(strncmp(line+1,"name = ", 4) == 0)
					{
						i += 7;

						while(line[i] == '=' || line[i] == ' ')
							i++;				
					
						strcpy(cr->networks[cr->network_cnt].name, &line[i]);						
					
					}else if(strncmp(line+1,"key = ", 3) == 0)
					{
						i += 6;
						while(line[i] == '=' || line[i] == ' ')
							i++;
						
						strcpy(cr->networks[cr->network_cnt].key, &line[i]);
										
	
					}

					
				}


			
			}


            
        }
	
    }
//===================================================================================================	
	// remove tests below
	for(i = 0; i < (int)cr->device_cnt; ++i)
			printf("%d: %s", i+1, cr->devices[i]);
			
		
	for(i = 0; i < (int)cr->network_cnt + 1; ++i)
	{
			printf("%d: name: %s", i+1, cr->networks[i].name);

			if(cr->networks[i].key[0] == '\0' )
				printf("\n");
			else
				printf("key: %s \n", cr->networks[i].key);

	}
    
//===================================================================================================	
}
