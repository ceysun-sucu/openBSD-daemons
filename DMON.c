/*

  
 ---------------- things todo --------------------------------------------------------------------------------------------

 
 * create initial configfile [doing ]
 * add optimizations
 * clean code
 * makefile
 * IP6 compatablity
 * Add option for ethenet connection
 * 
 

  
 ---------------- initial setup ------------------------------------------------------------------------------------------

------------------- main loop --------------------------------------------------------------------------------------------
		check if lkc exsists and is not empty 
		* then try that ... if fails end continue normal
		* at each succesfull connect and to are lkc file and
		* maintain some var where we at each isactive check if already connected we add to are lkc file, 
		* but then we flag var to never do it again thus only doing this op once at the beginning of the daemon
	  

----------------- possible optimizations ----------------------------------------------------------------------------------
* Use last known configurations [done]
* Find a quick way of loading a config .. i.e [doing]
* syslog -   use openlog function at add to syslog ... keep message to a minimum, started, endded, connect attemt.

------------------------ how to assign ip manully ---------#
* 
*  sudo ifconfig urtw0 nwid virginmedia4484565 wpa wpakey cxfujrgf 
*  sudo ifconfig urtw0 inet 192.168.0.4 netmask 255.255.255.0 up 
*  sudo route add default 192.168.0.1  {if default  route is missing}


 
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
#include <sha1.h>



#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#define RUNNING_DIR	"/etc/DMON"
#define LOCK_FILE	"exampled.lock"
#define LOG_FILE	"exampled.log"
#define CONFIG_FILE	"wifidaemon.confg"
#define LKC_FILE	"lkc.conf"

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
const struct if_status_description if_status_descriptions[] = LINK_STATE_DESCRIPTIONS;
const char *get_linkstate(int, int);
const char *get_string(const char *, const char *, u_int8_t *, int *);
static void hmac_sha1(const u_int8_t *text, size_t text_len, const u_int8_t *key, size_t key_len, u_int8_t digest[SHA1_DIGEST_LENGTH]);
int pkcs5_pbkdf2(const char *, size_t, const char *, size_t, u_int8_t *, size_t, u_int);



void 	test(void);
void	getsock(int);
void 	print_string(const u_int8_t *, int);
void	get_device_type(struct device *);
void	parseConfigFile(struct configReq *);
int		parseLKC(void);
int 	file_exists(const char *);
int		getNetworks(char *, struct networkReq *);
void	getDevices(struct deviceReq *);
void	setnwid(char *, char *);
void	setnwidWPA(char *, char *, char *);
int		IFCHECK(char *, struct deviceReq *); 
int		resolve(struct configReq *, struct deviceReq *);
int		callDHCP(char *);
int		af = AF_INET;
int 	s;
int 	startUp_FLAG = 0;
int 	NETCHECK(char *, struct networkReq *);
void 	format_name(char *);
void 	log_message(char *, char *);
int		isActive(void);
void 	writeToLKC(void);

struct device lkDevice;
struct network lkNetwork;

void log_message(char *filename, char *message)
{
	FILE *logfile;
	logfile=fopen(filename,"a");
	if(!logfile) return;
	fprintf(logfile,"%s\n",message);
	fclose(logfile);
}

void signal_handler(sig)
int sig;
{
	switch(sig) {
	case SIGHUP:
		log_message(LOG_FILE,"hangup signal catched");
		break;
	case SIGTERM:
		log_message(LOG_FILE,"terminate signal catched");
		exit(0);
		break;
	}
}

void daemonize()
{
int i,lfp;
char str[10];
	if(getppid()==1) return; /* already a daemon */
	i=fork();
	if (i<0) exit(1); /* fork error */
	if (i>0) exit(0); /* parent exits */
	/* child (daemon) continues */
	setsid(); /* obtain a new process group */
	for (i=getdtablesize();i>=0;--i) close(i); /* close all descriptors */
	i=open("/dev/null",O_RDWR); dup(i); dup(i); /* handle standart I/O */
	umask(027); /* set newly created file permissions */
	chdir(RUNNING_DIR); /* change running directory */
	lfp=open(LOCK_FILE,O_RDWR|O_CREAT,0640);
	if (lfp<0) exit(1); /* can not open */
	if (lockf(lfp,F_TLOCK,0)<0) exit(0); /* can not lock */
	/* first instance continues */
	sprintf(str,"%d\n",getpid());
	write(lfp,str,strlen(str)); /* record pid to lockfile */
	signal(SIGCHLD,SIG_IGN); /* ignore child */
	signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGHUP,signal_handler); /* catch hangup signal */
	signal(SIGTERM,signal_handler); /* catch kill signal */
}

main()
{
	
			
	

	daemonize();
	while(1){
		if(isActive() != 0)
		{
			
			
			
			if((!file_exists(LKC_FILE)) && (startUp_FLAG == 0))
			{ 
				parseLKC();
				if(strlen(lkNetwork.key) > 0){
					getsock(af);
					setnwidWPA(lkDevice.name, lkNetwork.name, lkNetwork.key);		
				}else{
					setnwid(lkDevice.name, lkNetwork.name);
				}
				
				int res = callDHCP(lkDevice.name);
				startUp_FLAG = 1;
				continue;
				
			}else{
				
				log_message(LOG_FILE, "is not active, staring initization seqeuence");
				struct configReq cr;
				struct deviceReq dr;
				struct networkReq nr;
				dr.device_cnt = 0;
				getDevices(&dr);
				log_message(LOG_FILE, "completed getDevices");
				parseConfigFile(&cr);
				log_message(LOG_FILE, "completed parseConfig");
				resolve(&cr, &dr);
				log_message(LOG_FILE, "completed loop");
				
			}
			
			
			
			
			
		}else{
		
			log_message(LOG_FILE, "skip");
			
		}
		
		sleep(5);
		
	}
	
	
}


int isActive(){
	
	
	
	struct ifreq ifr;
	struct ifaddrs *ifaddr;
	struct if_data *ifdata;
	struct ifmediareq ifmr;
	const struct ifmedia_description *desc;
	struct sockaddr_dl *sdl;
	const char *name;
	int family, type;
	
	if (getifaddrs(&ifaddr) == -1) {
		//perror("getifaddrs failed");
		exit(1);
	}
	
	
	struct ifaddrs *ifa = ifaddr;
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) { 
		if (ifa->ifa_addr != NULL) {   
		  int family = ifa->ifa_addr->sa_family; 
		  
		  sdl = (struct sockaddr_dl *)ifa->ifa_addr;
		  ifdata = ifa->ifa_data;
		  if(sdl->sdl_type == IFT_ETHER){ 
			getsock(af);
			
			
		
			if(strcmp(get_linkstate(sdl, ifdata->ifi_link_state), "active") == 0){
				//printf("name: %s, status: %s, strcmp: %d\n", ifa->ifa_name,get_linkstate(sdl, ifdata->ifi_link_state), x);
				return 0;
			}
				 	 
		  }  
		  
		}
		
	}

	freeifaddrs(ifaddr);
	
	return 1;
}

const char *get_linkstate(int mt, int link_state)
{
	const struct if_status_description *p;
	static char buf[8];

	for (p = if_status_descriptions; p->ifs_string != NULL; p++) {
		if (LINK_STATE_DESC_MATCH(p, mt, link_state))
			return (p->ifs_string);
	}
	snprintf(buf, sizeof(buf), "[#%d]", link_state);
	return buf;
}

int callDHCP(char *name)
{
	char buf[256];
	snprintf(buf, sizeof buf, "dhclient %s", name);
	
	int res;
	res = system(buf);
	if(res == 0)
		writeToLKC();
	return res;
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
		log_message(LOG_FILE, "SIOCGIFFLAGS");
	
	ifr.ifr_flags |= IFF_UP;


	if (ioctl(s, SIOCSIFFLAGS, (caddr_t)&ifr) != 0)
		log_message(LOG_FILE, "SIOCSIFFLAGS");

	len = sizeof(nwid.i_nwid);
	if (get_string(nwidName, NULL, nwid.i_nwid, &len) == NULL)
		return;

	nwid.i_len = len;
	ifr.ifr_data = (caddr_t)&nwid;
	if (ioctl(s, SIOCS80211NWID, (caddr_t)&ifr) < 0)
		log_message(LOG_FILE, "SIOCS80211NWID"); 
	

}
void setnwidWPA(char *name, char *nwidName, char *wpakey){

	struct ifreq ifr;
	int d = 0;
	struct ieee80211_nwid nwid1;
	int len;




	
	bzero(&ifr, sizeof(ifr));
	strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	
	if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifr) < 0)
		log_message(LOG_FILE, "SIOCGIFFLAGS");
	
	ifr.ifr_flags |= IFF_UP;


	if (ioctl(s, SIOCSIFFLAGS, (caddr_t)&ifr) != 0)
		log_message(LOG_FILE, "SIOCSIFFLAGS");

	len = sizeof(nwid1.i_nwid);
	if (get_string(nwidName, NULL, nwid1.i_nwid, &len) == NULL)
		return;

	nwid1.i_len = len;
	ifr.ifr_data = (caddr_t)&nwid1;
	if (ioctl(s, SIOCS80211NWID, (caddr_t)&ifr) < 0)
		log_message(LOG_FILE, "SIOCS80211NWID"); 
		
	// set wpa	
	struct ieee80211_wpaparams wpa;

	memset(&wpa, 0, sizeof(wpa));
	(void)strlcpy(wpa.i_name, name, sizeof(wpa.i_name));
	if (ioctl(s, SIOCG80211WPAPARMS, (caddr_t)&wpa) < 0)
		log_message(LOG_FILE, "here 1: SIOCG80211WPAPARMS");
	wpa.i_enabled = 0;
	if (ioctl(s, SIOCS80211WPAPARMS, (caddr_t)&wpa) < 0)
		err(1, "here 2: SIOCS80211WPAPARMS");
		
	// set wpakey
	
	
	struct ieee80211_wpaparams wpa2;
	struct ieee80211_wpapsk psk;
	struct ieee80211_nwid nwid;
	int passlen;

	memset(&psk, 0, sizeof(psk));
	if (d != -1) {
		memset(&ifr, 0, sizeof(ifr));
		ifr.ifr_data = (caddr_t)&nwid;
		strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
		if (ioctl(s, SIOCG80211NWID, (caddr_t)&ifr))
			log_message(LOG_FILE, "SIOCG80211NWID");

		passlen = strlen(wpakey);
		if (passlen == 2 + 2 * sizeof(psk.i_psk) &&
		    wpakey[0] == '0' && wpakey[1] == 'x') {
			/* Parse a WPA hex key (must be full-length) */
			passlen = sizeof(psk.i_psk);
			wpakey = get_string(wpakey, NULL, psk.i_psk, &passlen);
			if (wpakey == NULL || passlen != sizeof(psk.i_psk))
				log_message(LOG_FILE, "wpakey: invalid pre-shared key");
		} else {
			/* Parse a WPA passphrase */ 
			if (passlen < 8 || passlen > 63)
				log_message(LOG_FILE, "wpakey: passphrase must be between 8 and 63 characters");
			if (nwid.i_len == 0)
				log_message(LOG_FILE, "wpakey: nwid not set");
			if (pkcs5_pbkdf2(wpakey, passlen, nwid.i_nwid, nwid.i_len,
			    psk.i_psk, sizeof(psk.i_psk), 4096) != 0)
				log_message(LOG_FILE, "wpakey: passphrase hashing failed");
		}
		psk.i_enabled = 1;
	} else
		psk.i_enabled = 0;

	(void)strlcpy(psk.i_name, name, sizeof(psk.i_name));
	if (ioctl(s, SIOCS80211WPAPSK, (caddr_t)&psk) < 0)
		log_message(LOG_FILE, "SIOCS80211WPAPSK");

	/* And ... automatically enable or disable WPA */
	memset(&wpa2, 0, sizeof(wpa2));
	(void)strlcpy(wpa2.i_name, name, sizeof(wpa2.i_name));
	if (ioctl(s, SIOCG80211WPAPARMS, (caddr_t)&wpa2) < 0)
		log_message(LOG_FILE, "SIOCG80211WPAPARMS");
	wpa2.i_enabled = psk.i_enabled;
	if (ioctl(s, SIOCS80211WPAPARMS, (caddr_t)&wpa2) < 0)
		log_message(LOG_FILE, "SIOCS80211WPAPARMS");
		
	
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
	
	
	char buf[256];
	if(b_flag == 1)
	{
		for(i = 0; i < cr->network_cnt;i++)
		{
			
			if(NETCHECK(cr->networks[i].name, &nr) == 0)
			{
				
				format_name(cr->networks[i].name);
				
				if(strlen(cr->networks[i].key) > 0){
					format_name(cr->networks[i].key);
					setnwidWPA(name, cr->networks[i].name,cr->networks[i].key);
					strcpy(lkNetwork.key, cr->networks[i].key);
				}else{
					setnwid(name, cr->networks[i].name);
					
				}			
				strcpy(lkNetwork.name, cr->networks[i].name);
				strcpy(lkDevice.name, name);
								
				callDHCP(name);
				break;
				
			}
		}
		
	}
	
}
int getNetworks(char *name, struct networkReq *nR)
{
	log_message(LOG_FILE, "executing  getnetworks");  
	struct ieee80211_nodereq_all na;
	struct ieee80211_nodereq nr[512];
	struct ifreq ifr;

	bzero(&ifr, sizeof(ifr));
	strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));

	if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifr) < 0)
		log_message(LOG_FILE, "SIOCGIFFLAGS");
		
	ifr.ifr_flags |= IFF_UP;
	
	if (ioctl(s, SIOCSIFFLAGS, (caddr_t)&ifr) != 0)
		log_message(LOG_FILE, "SIOCSIFFLAGS");

	if (ioctl(s, SIOCS80211SCAN, (caddr_t)&ifr) != 0) {
		
			log_message(LOG_FILE, "\t\tno permission to scan\n");
		
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
		log_message(LOG_FILE, "\t\tnone\n");
		//return 1;
		
	int i;
	//printf("\t%d",na.na_nodes);
	for (i = 0; i < na.na_nodes; i++) {
		//printf("\t%d: ",i);
		char netname[IEEE80211_NWID_LEN];
		memcpy(netname,nr[i].nr_nwid , IEEE80211_NWID_LEN); 
		//printf("%s ", netname);		
		strlcpy(nR->networks[i].name, netname,sizeof(nR->networks[i].name));
		nR->network_cnt++;
		//printNode(&nr[i]);
		//putchar('\n');
	}
	int x;
	for(x = 0; x < nR->network_cnt; x++)
	{
		
		//printf("%d %s of total:%d \n", x, nR->networks[x].name, nR->network_cnt);
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
	log_message(LOG_FILE, "done main");

}

/*
 * lkc will take the format of 
 * [device]
 * [networkname]
 * 
 * ------------------ 
 * SIOCGIFFLAGS"
 * SIOCS80211NWID
 * SIOCG80211WPAPARMS");
 * SIOCS80211WPAPSK
 * SIOCG80211WPAPARMS
 */
int	parseLKC(void){
	
	lkNetwork.key[0] = 0;

	FILE* config_fp;
	config_fp = fopen(LKC_FILE, "r" );
	char line[MAX_LINE_LEN + 1];
    while(fgets(line, MAX_LINE_LEN, config_fp ) != NULL) //each line
    { 
	
        if(line != NULL && line[0] != '#' && strnlen(line,MAX_LINE_LEN) > 1)
        {
			if(strncmp(line,"device:",7) == 0){
				strlcpy(lkDevice.name, &line[8],sizeof(lkDevice.name));
				format_name(lkDevice.name);
				
			}else if(strncmp(line,"network:",8) == 0){
				strlcpy(lkNetwork.name, &line[9],sizeof(lkNetwork.name));
				format_name(lkNetwork.name);
				//printf("net : %s\n", lkNetwork.name );
			
			}else if(strncmp(line,"key:",4) == 0){
				strlcpy(lkNetwork.key, &line[5],sizeof(lkNetwork.key));
				//printf("key before: %s\n", lkNetwork.key );
				format_name(lkNetwork.key);
				//printf("key after: %s\n", lkNetwork.key );
			}
			
		}
		
	}
	return 0;
}
void writeToLKC(void){
	
	if(!file_exists(LKC_FILE)){
		//clear file
		fopen(LKC_FILE, "W" );
	}
	char buf[256];
	snprintf(buf, sizeof buf, "device: %s", lkDevice.name);
	log_message(LKC_FILE, buf); 
	snprintf(buf, sizeof buf, "network: %s", lkNetwork.name);
	log_message(LKC_FILE, buf); 
	if(lkNetwork.key > 0){
		snprintf(buf, sizeof buf, "network: %s", lkNetwork.key);
		log_message(LKC_FILE, buf); 
	}
	
	
}

int file_exists(const char *filename)
{
	FILE *file = fopen(filename, "r");
    if(file) 
    {
        fclose(file);
        return 0; //true 
    }
    return 1; //false;
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

   log_message(LOG_FILE, "done parse");

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
		log_message(LOG_FILE, "SIOCGIFMEDIA");
	
	if(IFM_TYPE(ifmr.ifm_current) == IFM_ETHER)
		strlcpy(d->type, "ethernet", sizeof(d->type));		
	else if(IFM_TYPE(ifmr.ifm_current) == IFM_IEEE80211)           
		strlcpy(d->type, "wireless", sizeof(d->type));
	
	
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
		
		if(strncmp(name,nr->networks[i].name,strlen(nr->networks[i].name)) == 0){
			return 0;
		}
	}	

	return 1;
		
}
void format_name(char *name)
{	

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
				//warnx("bad hexadecimal digits");
				return NULL;
			}
		}
		if (p > buf + len) {
			if (hexstr)
				log_message(LOG_FILE, "hexadecimal digits too long");
			else
				log_message(LOG_FILE, "strings too long");
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

int
pkcs5_pbkdf2(const char *pass, size_t pass_len, const char *salt, size_t salt_len,
    u_int8_t *key, size_t key_len, u_int rounds)
{
	u_int8_t *asalt, obuf[20];
	u_int8_t d1[20], d2[20];
	u_int i, j;
	u_int count;
	size_t r;

	if (rounds < 1 || key_len == 0)
		return -1;
	if (salt_len == 0 || salt_len > SIZE_MAX - 4)
		return -1;
	if ((asalt = malloc(salt_len + 4)) == NULL)
		return -1;

	memcpy(asalt, salt, salt_len);

	for (count = 1; key_len > 0; count++) {
		asalt[salt_len + 0] = (count >> 24) & 0xff;
		asalt[salt_len + 1] = (count >> 16) & 0xff;
		asalt[salt_len + 2] = (count >> 8) & 0xff;
		asalt[salt_len + 3] = count & 0xff;
		hmac_sha1(asalt, salt_len + 4, pass, pass_len, d1);
		memcpy(obuf, d1, sizeof(obuf));

		for (i = 1; i < rounds; i++) {
			hmac_sha1(d1, sizeof(d1), pass, pass_len, d2);
			memcpy(d1, d2, sizeof(d1));
			for (j = 0; j < sizeof(obuf); j++)
				obuf[j] ^= d1[j];
		}

		r = MIN(key_len, SHA1_DIGEST_LENGTH);
		memcpy(key, obuf, r);
		key += r;
		key_len -= r;
	};
	bzero(asalt, salt_len + 4);
	free(asalt);
	bzero(d1, sizeof(d1));
	bzero(d2, sizeof(d2));
	bzero(obuf, sizeof(obuf));

	return 0;
}

static void hmac_sha1(const u_int8_t *text, size_t text_len, const u_int8_t *key,
    size_t key_len, u_int8_t digest[SHA1_DIGEST_LENGTH])
{
	SHA1_CTX ctx;
	u_int8_t k_pad[SHA1_BLOCK_LENGTH];
	u_int8_t tk[SHA1_DIGEST_LENGTH];
	int i;

	if (key_len > SHA1_BLOCK_LENGTH) {
		SHA1Init(&ctx);
		SHA1Update(&ctx, key, key_len);
		SHA1Final(tk, &ctx);

		key = tk;
		key_len = SHA1_DIGEST_LENGTH;
	}

	bzero(k_pad, sizeof k_pad);
	bcopy(key, k_pad, key_len);
	for (i = 0; i < SHA1_BLOCK_LENGTH; i++)
		k_pad[i] ^= 0x36;

	SHA1Init(&ctx);
	SHA1Update(&ctx, k_pad, SHA1_BLOCK_LENGTH);
	SHA1Update(&ctx, text, text_len);
	SHA1Final(digest, &ctx);

	bzero(k_pad, sizeof k_pad);
	bcopy(key, k_pad, key_len);
	for (i = 0; i < SHA1_BLOCK_LENGTH; i++)
		k_pad[i] ^= 0x5c;

	SHA1Init(&ctx);
	SHA1Update(&ctx, k_pad, SHA1_BLOCK_LENGTH);
	SHA1Update(&ctx, digest, SHA1_DIGEST_LENGTH);
	SHA1Final(digest, &ctx);
}

