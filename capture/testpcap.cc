/**********************************************************************
* file:   testpcap2.c
* date:   2001-Mar-14 12:14:19 AM 
* Author: Martin Casado
* Last Modified:2001-Mar-14 12:14:11 AM
*
* Description: Q&D proggy to demonstrate the use of pcap_loop
*
**********************************************************************/

#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h> 
//#include "radiotap-parser.h"
//#include <asm/unaligned.h>
#include "radiotapdecap.h"
#include "wifipcap/wifipcap.h"


//#include "ieee802_11.h"
//#include "extract.h"
//#include "ieee802_11_radio.h"



void
process_packet(const struct pcap_pkthdr* pkthdr, const u_char * buf)
{
  int buflen = pkthdr->caplen;
  static int count = 1;


  int pkt_rate_100kHz = 0, antenna = 0, pwr = 0;
  u_int8_t rssi=0;
  u_char *flags;
  int i;
#if __debug
  for(i=0; i<36; i++)
    {
      printf("%02x ", buf[i]);
      
    }
#endif

  const u_char *b= parse_radiotap(buf,buflen);
  //  ieee802_11_if_print(pkthdr, b);

  
  return;
  struct ieee80211_radiotap_iterator iterator;
  int ret =
    ieee80211_radiotap_iterator_init(&iterator,
				     ( struct ieee80211_radiotap_header * ) buf, buflen);
  //  printf("iterator init return %d\n", ret);

  
  printf("%d %d\n", iterator.max_length,
	 iterator.bitmap_shifter);

  



  for(i=0; i<iterator.max_length; i++)
    {
      printf("%02x ", buf[i]);
      
    }
  printf("\n");
  
  flags = (u_char *)(&iterator.bitmap_shifter);
  for(i=0; i<8; i++)
    {
      printf("%02x ", flags[i]);
      
    }
  while (ret >= 0)
    {

    ret = ieee80211_radiotap_iterator_next(&iterator);
    printf("iterator next %d\n", ret);
    if (ret < 0)
      continue;

    /* see if this argument is something we can use */

    printf("arg index %d\n", iterator.this_arg_index);
    switch (iterator.this_arg_index) {
      /*
       * You must take care when dereferencing iterator.this_arg
       * for multibyte types... the pointer is not aligned.  Use
       * get_unaligned((type *)iterator.this_arg) to dereference
       * iterator.this_arg for type "type" safely on all arches.
       */
    case IEEE80211_RADIOTAP_RATE:
      /* radiotap "rate" u8 is in
       * 500kbps units, eg, 0x02=1Mbps
       */
      pkt_rate_100kHz = (*iterator.this_arg) * 5;
      printf("rate %d\n",pkt_rate_100kHz);
      break;

    case IEEE80211_RADIOTAP_ANTENNA:
      /* radiotap uses 0 for 1st ant */
      antenna = *iterator.this_arg;
      break;

    case IEEE80211_RADIOTAP_DBM_TX_POWER:
      pwr = *iterator.this_arg;
      break;
    case IEEE80211_RADIOTAP_DBM_ANTSIGNAL:
      rssi = (uint8_t) ( *((u_int8_t *)iterator.this_arg));
      int val = (int)  rssi;
      printf("rssi %d\n",val);
      break;

    }
  }  /* while more rt headers */


  /* discard the radiotap header part */
  buf += iterator.max_length;
  buflen -= iterator.max_length;
    
  fflush(stdout);
  count++;
}


/* callback function that is passed to pcap_loop(..) and called each time 
 * a packet is recieved  
 */
void my_callback(u_char *useless,
		 const struct pcap_pkthdr* pkthdr,
		 const u_char*  buf)
{

  process_packet(pkthdr, buf);

}

int main(int argc,char **argv)
{ 
  int i;
  char *dev; 
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_t* handle;
  const u_char *packet;
  struct pcap_pkthdr hdr;     /* pcap.h */
  struct ether_header *eptr;  /* net/ethernet.h */

  struct ieee80211_radiotap_header *rth;
  if(argc != 2){ fprintf(stdout,"Usage: %s numpackets\n",argv[0]);return 0;}

  /* grab a device to peak into... */
  //dev = pcap_lookupdev(errbuf);
  //if(dev == NULL)
  //{ printf("%s\n",errbuf); exit(1); }
#if MONITOR_LIVE
  /* open device for reading */
    handle = pcap_open_live(argv[1],BUFSIZ,1,-1,errbuf);
    int ltype = pcap_datalink(handle);
    printf("capturing link-layer header type %d\n", ltype);
    if( ltype != DLT_IEEE802_11_RADIO)
      {
	printf("invalid capture type!\n");
	exit(-1);
      }
    if(handle == NULL)
    { printf("pcap_open_live(): %s\n",errbuf); exit(1); }

    /* allright here we call pcap_loop(..) and pass in our callback function */
    /* int pcap_loop(pcap_t *p, int cnt, pcap_handler callback, u_char *user)*/
    /* If you are wondering what the user argument is all about, so am I!!   */
    pcap_loop(handle,atoi(argv[1]),my_callback,NULL);
#else
    //----------------- 
    //open the pcap file 
    handle = pcap_open_offline(argv[1], errbuf);   //call pcap library function 
 
    if (handle == NULL) { 
      fprintf(stderr,"Couldn't open pcap file %s: %s\n", argv[1], errbuf); 
      return(2); 
    }
    printf("opened %s\n", argv[1]);
 
    //----------------- 
    //begin processing the packets in this particular file, one at a time 
 
    while (packet = pcap_next(handle,&hdr))
      { 
      // header contains information about the packet (e.g. timestamp) 
      u_char *pkt_ptr = (u_char *)packet; //cast a pointer to the packet data
      rth = ( struct ieee80211_radiotap_header * ) packet;
      int len = rth->it_len;
      //      printf("%#1x",(unsigned int)pkt_ptr[4]);
      //      printf("%d", len);
      process_packet(&hdr, pkt_ptr);
      //      break;
    }
#endif
    

    fprintf(stdout,"\nDone processing packets... wheew!\n");
    return 0;
}
