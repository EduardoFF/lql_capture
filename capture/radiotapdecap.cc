/*
 * radiotapdecap.{cc,hh} -- decapsultates 802.11 packets
 * John Bicket
 *
 * Copyright (c) 2004 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

//#include ieeeradiotap.h>


#include <stdio.h>
#include <errno.h>
#include "byteorder.h"
#include <stdint.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint32_t u_int32_t;
typedef uint16_t u16;
typedef uint8_t u8;
typedef uint8_t u_int8_t;
#include "ieee80211_radiotap.h"
#define NUM_RADIOTAP_ELEMENTS 18

#include "radiotapdecap.h"
static const int radiotap_elem_to_bytes[NUM_RADIOTAP_ELEMENTS] =
	{8, /* IEEE80211_RADIOTAP_TSFT */
	 1, /* IEEE80211_RADIOTAP_FLAGS */
	 1, /* IEEE80211_RADIOTAP_RATE */
	 4, /* IEEE80211_RADIOTAP_CHANNEL */
	 2, /* IEEE80211_RADIOTAP_FHSS */
	 1, /* IEEE80211_RADIOTAP_DBM_ANTSIGNAL */
	 1, /* IEEE80211_RADIOTAP_DBM_ANTNOISE */
	 2, /* IEEE80211_RADIOTAP_LOCK_QUALITY */
	 2, /* IEEE80211_RADIOTAP_TX_ATTENUATION */
	 2, /* IEEE80211_RADIOTAP_DB_TX_ATTENUATION */
	 1, /* IEEE80211_RADIOTAP_DBM_TX_POWER */
	 1, /* IEEE80211_RADIOTAP_ANTENNA */
	 1, /* IEEE80211_RADIOTAP_DB_ANTSIGNAL */
	 1, /* IEEE80211_RADIOTAP_DB_ANTNOISE */
	 2, /* IEEE80211_RADIOTAP_RX_FLAGS */
	 2, /* IEEE80211_RADIOTAP_TX_FLAGS */
	 1, /* IEEE80211_RADIOTAP_RTS_RETRIES */
	 1, /* IEEE80211_RADIOTAP_DATA_RETRIES */
	};

static int rt_el_present(struct ieee80211_radiotap_header *th, u_int32_t element)
{
	if (element > NUM_RADIOTAP_ELEMENTS)
		return 0;
	//	printf("el_present %d %d\n", element, 1 << element);
	return le32_to_cpu(th->it_present) & (1 << element);
}

static int rt_check_header(struct ieee80211_radiotap_header *th, int len)
{
	int bytes = 0;
	int x = 0;
	if (th->it_version != 0) {
		return 0;
	}

	if (le16_to_cpu(th->it_len) < sizeof(struct ieee80211_radiotap_header)) {
		return 0;
	}

	for (x = 0; x < NUM_RADIOTAP_ELEMENTS; x++) {
		if (rt_el_present(th, x))
		    bytes += radiotap_elem_to_bytes[x];
	}

	if (le16_to_cpu(th->it_len) < sizeof(struct ieee80211_radiotap_header) + bytes) {
		return 0;
	}

	if (le16_to_cpu(th->it_len) > len) {
		return 0;
	}

	return 1;
}

static u_int8_t *rt_el_offset(struct ieee80211_radiotap_header *th, u_int32_t element) {
	unsigned int x = 0;
	unsigned int abs_offset=0;
	u_int8_t *offset = ((u_int8_t *) th) + sizeof(ieee80211_radiotap_header) + 8 ;
	for (x = 0; x < NUM_RADIOTAP_ELEMENTS && x < element; x++) {

		if (rt_el_present(th, x))
		  {
			offset += radiotap_elem_to_bytes[x];
			abs_offset += radiotap_elem_to_bytes[x];
		  }
	}
	//	printf("rt_el_offset %d %d\n",element, abs_offset);

	return offset;
}


const u8 *
parse_radiotap(const u8 *p, int len)
{
  int8_t rssi=0;
  int pkt_rate_100kHz=0;
  u_int8_t rate =0;
  int i=0;
  struct ieee80211_radiotap_header *th = (struct ieee80211_radiotap_header *) p;

#if __debug
  printf("present %u le %u\n", th->it_present, le32_to_cpu(th->it_present));
  for(i=0; i<8; i++)
    {
      printf("%02x ", (u_int8_t) ((u_int8_t *)(&th->it_present))[i]);
      
    }
  printf("\n");
#endif


  if (rt_check_header(th, len))
    {

      if (rt_el_present(th, IEEE80211_RADIOTAP_FLAGS))
	{
	  //	printf("has flags\n");
	u_int8_t flags = *((u_int8_t *) rt_el_offset(th, IEEE80211_RADIOTAP_FLAGS));
	if (flags & IEEE80211_RADIOTAP_F_DATAPAD) {
	  //ceh->pad = 1;
	}
	if (flags & IEEE80211_RADIOTAP_F_FCS)
	  {
	    //printf("has fcs\n");
	    //th+=4;
	    //	  p+=4;
	  }
      }

    if (rt_el_present(th, IEEE80211_RADIOTAP_RATE)) {
      rate = *((u_int8_t *) rt_el_offset(th, IEEE80211_RADIOTAP_RATE));
      pkt_rate_100kHz =  rate * 500;
      // printf("rate %d\n",pkt_rate_100kHz);
      // printf("rate %02x\n",rate);
      // printf("has rate\n");
      //
      //	ceh->rate = *((u_int8_t *) rt_el_offset(th, IEEE80211_RADIOTAP_RATE));
    }

    if (rt_el_present(th, IEEE80211_RADIOTAP_DBM_ANTSIGNAL))
      {
	rssi = *((int8_t *) rt_el_offset(th, IEEE80211_RADIOTAP_DBM_ANTSIGNAL));
	/* EDUARDO WAS HERE */
	printf("rssi %d\n", (int) rssi);
	//click_chatter("has dbm_antsignal %d\n", static_cast<int8_t>(ceh->rssi));
      }

    if (rt_el_present(th, IEEE80211_RADIOTAP_DBM_ANTNOISE))
      {
	//ceh->silence = *((u_int8_t *) rt_el_offset(th, IEEE80211_RADIOTAP_DBM_ANTNOISE));
      }

    if (rt_el_present(th, IEEE80211_RADIOTAP_DB_ANTSIGNAL))
      {
	//ceh->rssi = *((u_int8_t *) rt_el_offset(th, IEEE80211_RADIOTAP_DB_ANTSIGNAL));
	//click_chatter("has DB_antsignal %d\n", ceh->rssi);
      }

    if (rt_el_present(th, IEEE80211_RADIOTAP_DB_ANTNOISE))
      {
	//ceh->silence = *((u_int8_t *) rt_el_offset(th, IEEE80211_RADIOTAP_DB_ANTNOISE));
      }

    if (rt_el_present(th, IEEE80211_RADIOTAP_RX_FLAGS))
      {
	// u_int16_t flags = le16_to_cpu(*((u_int16_t *) rt_el_offset(th, IEEE80211_RADIOTAP_RX_FLAGS)));
	// if (flags & IEEE80211_RADIOTAP_F_RX_BADFCS)
	// 	ceh->flags |= WIFI_EXTRA_RX_ERR;
      }

    if (rt_el_present(th, IEEE80211_RADIOTAP_TX_FLAGS))
      {
	
	// u_int16_t flags = le16_to_cpu(*((u_int16_t *) rt_el_offset(th, IEEE80211_RADIOTAP_TX_FLAGS)));
	// ceh->flags |= WIFI_EXTRA_TX;
	// if (flags & IEEE80211_RADIOTAP_F_TX_FAIL)
	// 	ceh->flags |= WIFI_EXTRA_TX_FAIL;
      }

    if (rt_el_present(th, IEEE80211_RADIOTAP_DATA_RETRIES))
      {
	//ceh->retries = *((u_int8_t *) rt_el_offset(th, IEEE80211_RADIOTAP_DATA_RETRIES));
      }

    
    //p->pull(le16_to_cpu(th->it_len));
    //p->set_mac_header(p->data());  // reset mac-header pointer
  }

  printf("radiotap len %d\n", le16_to_cpu(th->it_len));
  return p+le16_to_cpu(th->it_len);
}


