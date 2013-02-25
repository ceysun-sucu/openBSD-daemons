/*
  * how can i be sure if a network work is up
  * how can i determin signal strength
  * should i parse the config file every loop interaction or on daemon startup only 
  *
  * 
  * figur out what networks we connect to when there are multiple networks with the same name 
  * we can do this by finding out the signal strength of these same networks nd just comparing what 
  * ifconfig connects to 
  * 
 ---------------- things todo --------------------------------------------------------------------------------------------
 
 
 
  * make changes to deviceReq struct to include device struct  ///DONE
  * customize and go through all ifconfig methods , and remove redundant code
  * get a list of availbe devices such as in are getDevices.. compare to config file devices. if they exsist keep them
  * replace arrays in configReq, with structs for network and device
  * !!! was working on main--- find the next working device with IFCHECK, will knw if it fails due to getNETORKS
  * !!! returns -1, get nodes... then setnwid with the device used in IFCHECK to a network which exsist( common up with a method
  *  !!! to deal with wether a network exsists.. like IFCHECK .... or replace both methods .. with an sorting method ...
  * !!! the next step will be to call dhcpclint ..  recording the output mayb we can detremin success or not
  * !!! fix gsetnwid not working
  * 
  * remmeber to add other values to network struct ... bssid etc ... if needed 
  * ==================
  * setnwid def works
  * the arguments being passed to it are wrong
  * so confirm getnetworks by printing all of nrs contents at the end .. and look for issues with new check
  * 
 ---------------- initial setup ------------------------------------------------------------------------------------------
	->try resolove an wired connection, if fails continue else end
	->get config info
		->try last used config, if not specified or fails continue
	->add config device and network info to the highest priority of there q's (does not check if the deivices are valid)
	->create another q of found networks, where the evaluated best connection has highest priority
	-> starting from the the first working device found, first try the networks from config file, if they all fail try
		from the second q which contains the the networks found.
	
------------------- main loop --------------------------------------------------------------------------------------------
		
	1 -> check if not connected to a network || network is  || interface is disappeard 
	  -> run resolveNetwork() 
	  -> wait 10 seconds, loop back to 1
	  

----------------- possible optimizations ----------------------------------------------------------------------------------

	* connecting to the strongest strength signal when networks of the same name are available
	* at load up run last know config regardless of if it exsists or not ..  avoid if config dhclient
	* and manully just assign the last ip 


 
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
#include <ctype.h>

#define MAX_LINE_LEN 256
#define MAX_DEVICES 20
#define MAX_NETWORKS 50
#define MAX_IFSIZE 20
#define MAX_NODESIZE 20

struct network {
	char name[20];
	char key[20];
};
struct device {
	char name[10];
	char type[10];
};
struct configReq {
	struct network networks[20];
	struct device devices[MAX_DEVICES];
	int device_cnt;
	int network_cnt;
};
struct deviceReq {
	struct device devices[MAX_DEVICES];
	int device_cnt;
};
struct networkReq {

	struct network networks[MAX_NETWORKS]; // are key will hold are encryption type
	int network_cnt;
};
const struct ifmedia_description ifm_type_descriptions[] = IFM_TYPE_DESCRIPTIONS;
const char *get_string(const char *, const char *, u_int8_t *, int *);

void 	test(void);
void	getsock(int);
void 	printNode(struct ieee80211_nodereq *);
void 	print_string(const u_int8_t *, int);
void	get_device_type(struct device *);
void	parseConfigFile(struct configReq *);
int		getNetworks(char *, struct networkReq *);
void	getDevices(struct deviceReq *);
void	setnwid(char *, char *);
int		IFCHECK(char *, struct deviceReq *); 
int		resolve(struct configReq *, struct deviceReq *);
int		callDHCP(char *);
int		af = AF_INET;
int 	s;
int 	NETCHECK(char *, struct networkReq *);
void 	format_name(char *);


int main()
{
	struct configReq cr;
	struct deviceReq dr;
	struct networkReq nr;
	dr.device_cnt = 0;
	
	getDevices(&dr);
	
	parseConfigFile(&cr); // getting are device and network info from are config file
	//printf("1:%s, 2:%s ",cr.networks[0].name,cr.networks[1].name);
	printf("we are here!!!!\n");
	resolve(&cr, &dr);
	
	//getNetworks("urtw0",&nr); 
	//printf("1:%s, 2:%s ",cr.networks[0].name,cr.networks[1].name);
	


}
int callDHCP(char *name)
{
	int res;
	res = system("dhclient urtw0");
	printf("\n res = %d\n", res);
	
}
void setnwid(char *name, char *nwidName)
{
	struct ifreq ifr;
	int d = 0;
	struct ieee80211_nwid nwid;
	int len;


	
	bzero(&ifr, sizeof(ifr));
	strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	
	if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifr) < 0)
		printf("SIOCGIFFLAGS");
	
	ifr.ifr_flags |= IFF_UP;


	if (ioctl(s, SIOCSIFFLAGS, (caddr_t)&ifr) != 0)
		printf("SIOCSIFFLAGS");

	len = sizeof(nwid.i_nwid);
	if (get_string(nwidName, NULL, nwid.i_nwid, &len) == NULL)
		return;

	nwid.i_len = len;
	ifr.ifr_data = (caddr_t)&nwid;
	if (ioctl(s, SIOCS80211NWID, (caddr_t)&ifr) < 0)
		warn("SIOCS80211NWID"); 
	

}
int resolve(struct configReq *cr, struct deviceReq *dr)
{ 
	char name[MAX_IFSIZE];
	struct networkReq nr;
	int b_flag = 0;
	int i;
	nr.network_cnt = 0;
	int res = 8;
	for(i = 0;i < dr->device_cnt;i++) //??????? should b cr  for all devices 
	{
		
		if(IFCHECK((char *)cr->devices[i].name, dr) == 0){ // each REAL config device which
			format_name(cr->devices[i].name);
			if(res = getNetworks(cr->devices[i].name, &nr) == 0)
			{ // if getnetworks works
				
				//now try and setnwid to all REAL networks in cr one by one until setwid returns succsess
				b_flag = 1; 
				strcpy(name, cr->devices[i].name);
				break;
			}
		}		
	}
	
	
	
	if(b_flag == 1)
	{
		for(i = 0; i < cr->network_cnt;i++)
		{
			
			if(NETCHECK(cr->networks[i].name, &nr) == 0)
			{
				
				format_name(cr->networks[i].name);
				printf("about to setnwid if name: %s, network name: %s, \n",name , cr->networks[i].name);
				setnwid(name, cr->networks[i].name);
				//setnwid("urtw0", "KINGSWIRELESS");
				callDHCP("urtw0");
				break;
				
			}
		}
		
	}
	
}
int getNetworks(char *name, struct networkReq *nR)
{
	printf("executing  getnetworks|%s|\n", name);  
	struct ieee80211_nodereq_all na;
	struct ieee80211_nodereq nr[512];
	struct ifreq ifr;

	bzero(&ifr, sizeof(ifr));
	strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));

	if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifr) < 0)
		printf("SIOCGIFFLAGS");
		
	ifr.ifr_flags |= IFF_UP;
	
	if (ioctl(s, SIOCSIFFLAGS, (caddr_t)&ifr) != 0)
		printf("SIOCSIFFLAGS");

	if (ioctl(s, SIOCS80211SCAN, (caddr_t)&ifr) != 0) {
		
			printf("\t\tno permission to scan\n");
		//goto done;
	} 


	bzero(&na, sizeof(na));
	bzero(&nr, sizeof(nr));
	na.na_node = nr;
	na.na_size = sizeof(nr);
	strlcpy(na.na_ifname, name, sizeof(na.na_ifname));
	
	if (ioctl(s, SIOCG80211ALLNODES, &na) != 0) {
		warn("SIOCG80211ALLNODES");
		return 1; //fail
	}
	
	if (!na.na_nodes)
		printf("\t\tnone\n");
		//return 1;
		
	int i;
	printf("\t%d",na.na_nodes);
	for (i = 0; i < na.na_nodes; i++) {
		printf("\t%d: ",i);
		char netname[IEEE80211_NWID_LEN];
		memcpy(netname,nr[i].nr_nwid , IEEE80211_NWID_LEN); 
		printf("%s ", netname);		
		strlcpy(nR->networks[i].name, netname,sizeof(nR->networks[i].name));
		nR->network_cnt++;
		printNode(&nr[i]);
		putchar('\n');
	}
	int x;
	for(x = 0; x < nR->network_cnt; x++)
	{
		
		printf("%d %s of total:%d \n", x, nR->networks[x].name, nR->network_cnt);
	}
	return 0; //succsess
	
	
	
	

	
}
void getDevices(struct deviceReq *dr)
{
	struct ifreq ifr;
	struct ifaddrs *ifaddr;
	struct if_data *ifdata;
	struct ifmediareq ifmr;
	const struct ifmedia_description *desc;
	struct sockaddr_dl *sdl;
	const char *name;
	int family, type;
	
	if (getifaddrs(&ifaddr) == -1) {
		perror("getifaddrs failed");
		exit(1);
	}
	
	
	struct ifaddrs *ifa = ifaddr;
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) { 
		if (ifa->ifa_addr != NULL) {   
		  int family = ifa->ifa_addr->sa_family; 
		  
		  sdl = (struct sockaddr_dl *)ifa->ifa_addr;
		  if(sdl->sdl_type == IFT_ETHER){ 
			getsock(af);
			
			strlcpy(dr->devices[dr->device_cnt].name, ifa->ifa_name, sizeof(dr->devices[dr->device_cnt].name)); 
			get_device_type(&dr->devices[dr->device_cnt]);
			dr->device_cnt++;
								 
		  }  
		  
		}
		
	}



	freeifaddrs(ifaddr);
	printf("done main\n");

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
					strlcpy(cr->devices[cr->device_cnt].name, &line[1],sizeof(cr->devices[cr->device_cnt].name));
					//get_device_type(&cr->devices[cr->device_cnt]);
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
					
						strlcpy(cr->networks[cr->network_cnt].name, &line[i],sizeof(cr->networks[cr->network_cnt].name));						
					
					}else if(strncmp(line+1,"key = ", 3) == 0)
					{
						i += 6;
						while(line[i] == '=' || line[i] == ' ')
							i++;
						
						strlcpy(cr->networks[cr->network_cnt].key, &line[i],sizeof(cr->networks[cr->network_cnt].key));
										
	
					}

					
				}


			
			}


            
        }
	
    }
//===================================================================================================	
	// remove tests below
	/*for(i = 0; i < (int)cr->device_cnt; ++i)
			printf("%d: %s", i+1, cr->devices[i]);
			
		
	for(i = 0; i < (int)cr->network_cnt + 1; ++i)
	{
			printf("%d: name: %s", i+1, cr->networks[i].name);

			if(cr->networks[i].key[0] == '\0' )
				printf("\n");
			else
				printf("key: %s \n", cr->networks[i].key);

	}*/
    printf("done parse\n");
//===================================================================================================	
}
void getsock(int naf)
{
	static int oaf = -1;

	if (oaf == naf)
		return;
	if (oaf != -1)
		close(s);
	s = socket(naf, SOCK_DGRAM, 0);
	if (s < 0)
		oaf = -1;
	else
		oaf = naf;
}
void get_device_type(struct device *d)
{
	
	struct ifmediareq ifmr;
	memset(&ifmr, 0, sizeof(ifmr));
	strlcpy(ifmr.ifm_name, d->name, sizeof(ifmr.ifm_name));
	getsock(af);
	if (ioctl(s, SIOCGIFMEDIA, (caddr_t)&ifmr) < 0)
		err(1, "SIOCGIFMEDIA");
	
	if(IFM_TYPE(ifmr.ifm_current) == IFM_ETHER)
		strlcpy(d->type, "ethernet", sizeof(d->type));		
	else if(IFM_TYPE(ifmr.ifm_current) == IFM_IEEE80211)           
		strlcpy(d->type, "wireless", sizeof(d->type));
	
	
}
void printNode(struct ieee80211_nodereq *nr)
{

	int len;

	if (nr->nr_flags & IEEE80211_NODEREQ_AP) {
		len = nr->nr_nwid_len;
		if (len > IEEE80211_NWID_LEN)
			len = IEEE80211_NWID_LEN;
		printf("nwid ");
		print_string(nr->nr_nwid, len);
		putchar(' ');

		printf("chan %u ", nr->nr_channel);
	}

	if (nr->nr_flags & IEEE80211_NODEREQ_AP)
		printf("bssid %s ",
		    ether_ntoa((struct ether_addr*)nr->nr_bssid));
	else
		printf("lladdr %s ",
		    ether_ntoa((struct ether_addr*)nr->nr_macaddr));



}
void print_string(const u_int8_t *buf, int len)
{
	int i = 0, hasspc = 0;

	if (len < 2 || buf[0] != '0' || tolower(buf[1]) != 'x') {
		for (; i < len; i++) {
			/* Only print 7-bit ASCII keys */
			if (buf[i] & 0x80 || !isprint(buf[i]))
				break;
			if (isspace(buf[i]))
				hasspc++;
		}
	}
	if (i == len) {
		if (hasspc || len == 0)
			printf("\"%.*s\"", len, buf);
		else
			printf("%.*s", len, buf);
	} else {
		printf("0x");
		for (i = 0; i < len; i++)
			printf("%02x", buf[i]);
	}
}
int	IFCHECK(char *name, struct deviceReq *dr)
{
	
	int i;
	for(i = 0;i < dr->device_cnt; i++)
	{	
		if(strncmp(name,dr->devices[i].name, strlen(dr->devices[i].name)) == 0)
		{
			return 0;
		}
	}
	
	return 1;
}
int NETCHECK(char *name, struct networkReq *nr)
{
	
	int i;
	for(i = 0; i < nr->network_cnt; i++)
	{
		printf("checking %s against %s\n",name, nr->networks[i].name);
		if(strncmp(name,nr->networks[i].name,strlen(nr->networks[i].name)) == 0){
			//printf("cr network: %s is equal to nr network %s \n", name , nr->networks[i].name);
			//memcpy(name,nr->networks[i].name , strlen(nr->networks[i].name)); 
			//name = nr->networks[i].name;
			return 0;
		}
	}	
	printf("return failure\n");
	return 1;
		
}
void format_name(char *name)
{	
	printf("formating name: %s of size: %d \n", name, strlen(name));
	//name[strlen(name) -1] = 0;
	/*int len = strlen(name);
	
	while(!isalpha(name[len - 1 ]) && !isdigit(name[len - 1 ]) )
		name[len -1] = 0;
		len--;
	name[len - 1] = '\0';
	printf("...%s...",name);
	printf("\n");*/
	int pos = strlen(name) - 1;
	while(!isalpha(name[pos]) && !isdigit(name[pos]))
	{
		name[pos] = 0;
		pos--;
	}
	
}
const char *get_string(const char *val, const char *sep, u_int8_t *buf, int *lenp)
{
	int len = *lenp, hexstr;
	u_int8_t *p = buf;

	hexstr = (val[0] == '0' && tolower((u_char)val[1]) == 'x');
	if (hexstr)
		val += 2;
	for (;;) {
		if (*val == '\0')
			break;
		if (sep != NULL && strchr(sep, *val) != NULL) {
			val++;
			break;
		}
		if (hexstr) {
			if (!isxdigit((u_char)val[0]) ||
			    !isxdigit((u_char)val[1])) {
				warnx("bad hexadecimal digits");
				return NULL;
			}
		}
		if (p > buf + len) {
			if (hexstr)
				warnx("hexadecimal digits too long");
			else
				warnx("strings too long");
			return NULL;
		}
		if (hexstr) {
#define	tohex(x)	(isdigit(x) ? (x) - '0' : tolower(x) - 'a' + 10)
			*p++ = (tohex((u_char)val[0]) << 4) |
			    tohex((u_char)val[1]);
#undef tohex
			val += 2;
		} else {
			if (*val == '\\' &&
			    sep != NULL && strchr(sep, *(val + 1)) != NULL)
				val++;
			*p++ = *val++;
		}
	}
	len = p - buf;
	if (len < *lenp)
		memset(p, 0, *lenp - len);
	*lenp = len;
	return val;
}







