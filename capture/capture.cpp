
#include <iostream>
#include "wifipcap/wifipcap.h"

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <set>
#include <string>
#include <sstream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
//#include "protectedmutex.h"

using namespace std;

/// Cool templates
#define ALL(x) x.begin(), x.end()
#define PV(v,type) copy(ALL(v),ostream_iterator<type>(cout," "))
#define IT(c) __typeof((c).begin())
#define FOREACH(i,c) for(__typeof((c).begin()) i=(c).begin();i!=(c).end();++i)
#define VERBOSE(x) if(param.verbose >= x) 
#define DBG(l, x) if (DEBUG(l)) cout << x;
#define CEIL(VARIABLE) ( (VARIABLE - (int)VARIABLE)==0 ? (int)VARIABLE : (int)VARIABLE+1 )
#define EPSILON 1e-4
#define GETDIR(x) (x.find_last_of("/")!=string::npos?x.substr(0,x.find_last_of("/")):".")
//define DEBUG(t,m) printf("%s: %s\n",__FUNCTION__,m)
#define CRITICAL 1
#define DEBUGP(t,m, ...) printf("%s: ",__FUNCTION__);printf(m,__VA_ARGS__);if(t == CRITICAL) exit(-1)

#define FATALERROR(m, ...) {fprintf(stderr, "FATAL ERROR at %s %d: ",__FILE__, __LINE__);fprintf(stderr,m, ## __VA_ARGS__);fflush(stderr);exit(EXIT_FAILURE);}
#define PI 3.14159265
#define RADIANS(X) ((X)*PI/180)
#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)
#define GETPOTIFY(g,prefix, x) x = (g)(STR(prefix) STR(x), x)
#define GETPOTIFYSTR(g,prefix, x) x = (g)(STR(prefix) STR(x), (x).c_str())



#define MAX_APS 100
#define MAX_NODES 200
class TestCB : public WifipcapCallbacks
{
private:
  int m_id;
  double m_x,m_y;
  bool fcs_ok;
  int last_rssi;

  int cumm_len ;
  long int next_update;
  long int last_tstamp;
  /// in seconds
  int update_interval;
  std::set<uint64_t> aps;
  std::set<uint64_t> nodes;
  std::map<std::string, int> cumm_rssi_per_ssid;
  std::map<std::string, int> n_rssi_per_ssid;
  /*
     * Mutex to control the access to the listAllNodes
     */
    pthread_mutex_t m_mutex;
    /**
     * Thread
     */
    pthread_t m_thread;
  
  typedef enum e_pType
    {
    NONE,
    BEACON,
    DATATOAP,
    DATAFROMAP,
    } PacketType;
  PacketType last_ptype;
  void
  updateBandwidthUsage(const struct timeval &t, int len)
  {
    cumm_len += len;
    //    printf("%ld sec\n", t.tv_sec);
  }
public:
  TestCB(int id, double x, double y)
  {
    cumm_len = 0;
    next_update = 0;
    update_interval = 10;
    m_id = id;
    m_x = x;
    m_y = y;
  }
	/*
    void PacketBegin(const struct timeval& t, const u_char *pkt, int len, int origlen)
    {
	cout << t << " {" << endl;
    }
    void PacketEnd()
    {
	cout << "}" << endl;
    }
	*/

  static void * UDPThreadEntryFunc(void * ptr) 
  {
    (( TestCB *) ptr)->UDPThreadFunc();
    return NULL;
  }

  void updatePos(double x, double y)
  {
    pthread_mutex_lock(&m_mutex);
    printf("got pos %.2f %.2f\n", x,y);
    m_x = x;
    m_y;
    
    pthread_mutex_unlock(&m_mutex);
  }
  void
  UDPThreadFunc()
  {
       int sockfd,n;
   struct sockaddr_in servaddr,cliaddr;
   socklen_t len;
   char mesg[1000];
   double x,y;

   sockfd=socket(AF_INET,SOCK_DGRAM,0);

   bzero(&servaddr,sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
   servaddr.sin_port=htons(32000);
   bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

   for (;;)
   {
      len = sizeof(cliaddr);
      n = recvfrom(sockfd,mesg,1000,0,(struct sockaddr *)&cliaddr,&len);
      sendto(sockfd,mesg,n,0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
      printf("-------------------------------------------------------\n");
      mesg[n] = 0;
      printf("Received the following:\n");
      printf("%s",mesg);
      printf("-------------------------------------------------------\n");

      if( !n )
	continue;
      stringstream ss(mesg);
      string cmd;
      ss >> cmd;
      if( cmd == "pos" )
	{
	  ss >> x >> y;
      
      
	  updatePos(x,y);
	}
   }
  }
  
  bool
  runUDPServer()
  {
    //  printf("running\n");
    pthread_mutex_init(&m_mutex, NULL);
    int status = pthread_create(&m_thread, NULL, UDPThreadEntryFunc, this);
    return (status == 0);
  }
  void report(long int t)
  {
    int bw = cumm_len / update_interval;
    printf("===============================\n");
    fflush(stdout);
    printf("report @ %ld\n", t);
    printf("bw %.4f MB/s\n", 1.0*bw/1e6);
    printf("# aps %d\n", (int) aps.size());
    printf("# nodes %d\n", (int) nodes.size());
    printf("rssi_x_ssid:\n");
    stringstream rssi_str;
    int n_rssi =0;
    FOREACH(it, cumm_rssi_per_ssid )
      {
	string ssid = it->first;
	int rssi = it->second;
	int n = n_rssi_per_ssid[ssid];
	if( n < 5 )
	  continue;
	n_rssi++;
	rssi_str << " " << ssid.c_str() << " " << rssi/n;
	printf(" %s %d (%d)\n", ssid.c_str(), rssi/n, n);
      }
    printf("===============================\n\n");

    stringstream fs;
    fs << "/tmp/" << m_id << "_" << t <<".dat";
    ofstream of(fs.str().c_str());
    double x,y;
    pthread_mutex_lock(&m_mutex);
    x = m_x;
    y = m_y;
    pthread_mutex_unlock(&m_mutex);
    of << m_id << " " << x << " " << y
       << " " << 1.0*bw/1e6
       << " " << (int) aps.size()
       << " " << (int) nodes.size()
       << " " << n_rssi << " " << rssi_str.str() << endl;
    of.close();
    stringstream cmd;
    cmd << "scp -q " << fs.str() << " feo@195.176.38.35:lql/";
    int ret = system(cmd.str().c_str());
    //    printf("system returned %d\n", ret);
    if( ret != 0 )
      {
	printf("NO_CONNECTION %ld %.2f %.2f\n", t, x,y);
      }
    
    reset();
    next_update = t + update_interval;
  }
  void registerAP(uint64_t ap_addr)
  {
    if( aps.size() > MAX_APS )
      {
	fprintf(stderr, "reached maximum number of aps - something wrong?\n");
      }
    else
      aps.insert(ap_addr);
  }
  void rssiSSID(std::string  ssid, int rssi)
  {
    if( cumm_rssi_per_ssid.size() > MAX_APS )
      {
	fprintf(stderr, "reached maximum number of ssids - something wrong?\n");
      }
    else
      {
	cumm_rssi_per_ssid[ssid] += rssi;
	n_rssi_per_ssid[ssid] += 1;
	
      }
  }
  void registerNode(uint64_t n_addr)
  {
    if( nodes.size() > MAX_NODES )
      {
	fprintf(stderr, "reached maximum number of nodes - something wrong?\n");
      }
    else
      nodes.insert(n_addr);
  }
  void reset()
  {
    cumm_len = 0;
    aps.clear();
    nodes.clear();
    cumm_rssi_per_ssid.clear();
    n_rssi_per_ssid.clear();
  }

     void PacketBegin(const struct timeval& t, const u_char *pkt, int len, int origlen)
  {
    //     printf("packet BEGIN\n");
    //fflush(stdout);
       last_ptype = NONE;
       last_tstamp = t.tv_sec;
       
       updateBandwidthUsage(t, len);

     }
     void PacketEnd()
  {
    //     printf("packet END\n");
    //  fflush(stdout);
       if( !next_update )
	 {
	   next_update = last_tstamp + update_interval;
	 }
       if( last_tstamp >= next_update )
	 {
	   report(last_tstamp);
	 }
  }

  
    bool Check80211FCS() { return true; }

    void Handle80211DataFromAP(const struct timeval& t, const data_hdr_t *hdr, const u_char *rest, int len) 
    {
      if( hdr == NULL)
	return;
	if (!fcs_ok) {
	  //registerAP(hdr->sa.val);
	  registerNode(hdr->da.val);
	    // cout << "  " << "802.11 data:\t" 
	    // 	 << hdr->sa << " -> " 
	    // 	 << hdr->da << "\t" 
	    // 	 << len << endl;
	}
    }
    void Handle80211DataToAP(const struct timeval& t, const data_hdr_t *hdr, const u_char *rest, int len) 
    {
      if( hdr == NULL )
	return;
	if (!fcs_ok) {
	  //registerAP(hdr->da.val);
	  registerNode(hdr->sa.val);
	    // cout << "  " << "802.11 data:\t" 
	    // 	 << hdr->sa << " -> " 
	    // 	 << hdr->da << "\t" 
	    // 	 << len << endl;
	}
    }

    void Handle80211(const struct timeval& t, u_int16_t fc, const MAC& sa, const MAC& da, const MAC& ra, const MAC& ta, bool fcs_ok) {
	this->fcs_ok = fcs_ok;
    }

	void HandleEthernet(const struct timeval& t, const ether_hdr_t *hdr, const u_char *rest, int len) {
	  //		cout << " Ethernet: " << hdr->sa << " -> " << hdr->da << endl;
	}
    
    void Handle80211MgmtProbeRequest(const struct timeval& t, const mgmt_header_t *hdr, const mgmt_body_t *body)
    {
	// if (!fcs_ok) {
	//     cout << "  " << "802.11 mgmt:\t" 
	// 	 << hdr->sa << "\tprobe\t\"" 
	// 	 << body->ssid.ssid << "\"" << endl;
	// }
    }

    void Handle80211MgmtBeacon(const struct timeval& t, const mgmt_header_t *hdr, const mgmt_body_t *body)
    {
      if( hdr == NULL )
	return;
	if (!fcs_ok) {
	  registerAP(hdr->sa.val);
	    // cout << "  " << "802.11 mgmt:\t" 
	    // 	 << hdr->sa << "\tbeacon\t\"" 
	    // 	 << body->ssid.ssid << "\"" << endl;
	  if( body->ssid.length > 1 )
	    {
	      string ssid_str = (const char *)body->ssid.ssid;
	      if( ssid_str.size() > 1)
	      //printf("ssid %s\n", ssid_str.c_str());
		rssiSSID(ssid_str, last_rssi);
	    }
	}
    }

    void HandleTCP(const struct timeval& t, const ip4_hdr_t *ip4h, const ip6_hdr_t *ip6h, const tcp_hdr_t *hdr, const u_char *options, int optlen, const u_char *rest, int len)
    {
	// if (ip4h && hdr)
	//   {
	//     cout << "  " << "tcp/ip:     \t" 
	// 	 << ip4h->src << ":" << hdr->sport << " -> " 
	// 	 << ip4h->dst << ":" << hdr->dport 
	// 	 << "\t" << ip4h->len << endl;
	//   } else
	//   {
	//     // cout << "  " << "tcp/ip:     \t" << "[truncated]" << endl;
	//   }
    }   

	void HandleUDP(const struct timeval& t, const ip4_hdr_t *ip4h, const ip6_hdr_t *ip6h, const udp_hdr_t *hdr, const u_char *rest, int len)
	{
		// if (ip4h && hdr)
		//   {
		//     // cout << "  " << "udp/ip:     \t" 
		//     // 	 << ip4h->src << ":" << hdr->sport << " -> " 
		//     // 	 << ip4h->dst << ":" << hdr->dport 
		//     // 	 << "\t" << ip4h->len << endl;
		//   }
		// else
		//   {
		//     //cout << " " << "udp/ip:     \t" << "[truncated]" << endl;
		//   }
	}
      void HandleRadiotap(const struct timeval& t, radiotap_hdr *hdr, const u_char *rest, int len)
  {
    if( hdr == NULL)
      return;
    if( hdr->has_signal_dbm )
      {
	//	printf("rssi %d dbm\n ", hdr->signal_dbm);
	last_rssi = hdr->signal_dbm;
      }

    

  }
};

void openSocketConn()
{

}

/**
 * usage: test <pcap_trace_file>
 */
int main(int argc, char **argv)
{

  
  bool live = argc >= 3 && atoi(argv[2]) == 1;
    Wifipcap *wcap = new Wifipcap(argv[1], live);

    int id=0;
    double x,y;
    x=y=0.0;
    if( argc > 3)
      {
	id = atoi(argv[3]);
      }
    if( argc > 4)
      {
	x = atof(argv[4]);
	y = atof(argv[5]);
      }

    TestCB *testCB = new TestCB(id,x,y);
    testCB->runUDPServer();
    wcap->Run(testCB);
    return 0;
}
