#include "settings.h"
#ifdef NETWORK

#include <string.h>
#include <gccore.h>
#include <ogcsys.h>
#include <network.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ogc/machine/processor.h>

#include <stdio.h>

#include "dtypes.h"
#include "settings.h"
#include "ipx.h"
#include "multi.h"
#include "error.h"

#include "object.h"

#ifdef RELEASE
#define dprintf(...)
#else
#define dprintf printf
#endif

#define USE_TCP 1
#define CONNECT_RETRIES 200

#define DS_TAG 0xDE5CEA17
#define DS_IDENTIFY 1

#define DEBUG_IPADDRESS "descent.tueidj.net"
s32 server_socket = -1;
ubyte my_mac[6];
ubyte my_server[4];

static int packet_counter = 0;

static ubyte packet_buffer[1024] ATTRIBUTE_ALIGN(32);
static ubyte receive_buffer[1024] ATTRIBUTE_ALIGN(32);

typedef struct {
	unsigned int tag;
	unsigned int size;
	unsigned int extra;
	unsigned char target[6];
	unsigned char source[6];
} dwii_header;

typedef struct {
	void *data;
	int data_size;
} mbox_msg;

static const unsigned char broadcast_node[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static lwp_t startup_thread = LWP_THREAD_NULL;
static mqbox_t rec_mbox = MQ_BOX_NULL;
static int do_exit = -1;

static void Alert(syswd_t timer, lwp_t thread)
{
	if (thread != LWP_THREAD_NULL) {
		LWP_ResumeThread(thread);
	}
}

static void AlertableWait(int secs, int usecs)
{
	syswd_t timer;
	struct timespec tm = {secs, usecs};

	if (SYS_CreateAlarm(&timer)) {
		dprintf("Failed to create timer for alertable wait\n");
		return;
	}

	tm.tv_sec = secs;
	tm.tv_nsec = usecs*1000;

	if (SYS_SetAlarm(timer, &tm, (alarmcallback)Alert, (void*)LWP_GetSelf())) {
		dprintf("Failed to set timer for alertable wait\n");
		SYS_RemoveAlarm(timer);
		return;
	}

	LWP_SuspendThread(LWP_GetSelf());
	SYS_RemoveAlarm(timer);
}

static void* startup(void* p)
{
	dwii_header *hdr = (dwii_header*)receive_buffer;
	int ret, i;
	const int sizeof_hdr = sizeof(dwii_header);

	if (net_init()<0) {
		dprintf("net startup: init failed\n");
		return NULL;
	}

	net_get_mac_address(my_mac);
	ES_GetDeviceID((u32*)&ret);
	my_mac[2] ^= (ret>>24);
	my_mac[3] ^= (ret>>16);
	my_mac[4] ^= (ret>>8);
	my_mac[5] ^= ret;

	MQ_Init(&rec_mbox, 256);

	while (1) {

		Network_active = 0;

		if (server_socket>=0) {
			net_shutdown(server_socket, 3);
			net_close(server_socket);
			if (do_exit>0) {
				printf("Socket shutdown\n");
				return NULL;
			}
			usleep(20000);
			dprintf("Socket died, reconnecting\n");
		}

#ifdef USE_TCP
		server_socket = net_socket(PF_INET, SOCK_STREAM, IPPROTO_IP);
#else
		server_socket = net_socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
#endif
		if (server_socket<0) {
			dprintf("net startup failed to create socket\n");
			return NULL;
		}

		ret = 16384;
		dprintf("net_setsockopt returned %d\n", net_setsockopt(server_socket, SOL_SOCKET, SO_SNDBUF, (char*)&ret, sizeof(ret)));
		ret = 32768;
		dprintf("net_setsockopt returned %d\n", net_setsockopt(server_socket, SOL_SOCKET, SO_RCVBUF, (char*)&ret, sizeof(ret)));
		// make socket non-blocking
		ret = 1;
		net_ioctl(server_socket, FIONBIO, &ret);

#ifdef USE_TCP
		ret = 1;
		net_setsockopt(server_socket, IPPROTO_TCP, /*TCP_NODELAY*/0x2001, &ret, sizeof(ret));

		struct sockaddr_in address;
		memset(&address, 0, sizeof(address));
		address.sin_family = PF_INET;
		address.sin_port = htons((int)p);

		ret = inet_aton(DEBUG_IPADDRESS, &address.sin_addr);
		if (ret <= 0) {
			struct hostent *host;
			dprintf("couldn't read ip address, doing DNS lookup\n");
			host = net_gethostbyname(DEBUG_IPADDRESS);
			if (host && host->h_length==sizeof(address.sin_addr) && host->h_addrtype==PF_INET && host->h_addr_list && host->h_addr_list[0]) {
				memcpy(&address.sin_addr, host->h_addr_list[0], host->h_length);
				dprintf("DNS lookup succeeded, using IP %s\n", inet_ntoa(address.sin_addr));
			}
			else {
				dprintf("DNS lookup for %s failed\n", DEBUG_IPADDRESS);
				return NULL;
			}
		}

		dprintf("net startup connecting...\n");

		for (i=0; i < CONNECT_RETRIES; i++) {
			ret = net_connect(server_socket, (struct sockaddr*)&address, sizeof(address));
			dprintf("net_connect: %d (%d %d)\n", ret, EINPROGRESS, EALREADY);
			if (!do_exit && (ret == -EINPROGRESS || ret == -EALREADY)) {
				usleep(40000);
				continue;
			}
			if (ret == -EISCONN)
				ret = 0;
			break;
		}

		dprintf("net_connect returned %d (port %d)\n", ret, ntohl(address.sin_port));
		if (ret < 0) {
			dprintf("Failed to connect after %d attempts\n", i);
			if (!do_exit)
				AlertableWait(10, 0);
			dprintf("AlertableWait for thread %08X returned\n", LWP_GetSelf());
			// TODO: try a different server
			continue;
		}

#else
		struct sockaddr_in address;
		memset(&address, 0, sizeof(address));
		address.sin_family = PF_INET;
		address.sin_port = htons((int)p);
		address.sin_addr.s_addr = htonl(INADDR_ANY);
		if (net_bind(server_socket, (struct sockaddr*)&address, sizeof(address))<0) {
			dprintf("net_bind failed\n");
			continue;
		}
#endif
		dprintf("Server socket is ready\n");

		while (1) {
#ifdef USE_TCP
			ret = net_read(server_socket, hdr, sizeof_hdr);
			if (ret == -EAGAIN && !do_exit) {
				AlertableWait(1, 0);
				continue;
			}
			if (ret<=0) {
				dprintf("net_recv returned error %d\n", ret);
				break;
			}
			else if (ret==sizeof_hdr) {
				int readed = 0;
				u8 *data;
				hdr->size = ntohl(hdr->size);
				hdr->extra = ntohl(hdr->extra);
				if (hdr->tag != htonl(DS_TAG)) {
					dprintf("tag mismatch %08X\n", ntohl(hdr->tag));
					continue;
				}

				if (!hdr->size) {
					if (hdr->extra == DS_IDENTIFY) {
						dprintf("Sending registration\n");
						memcpy(hdr->source, my_mac, 6);
						net_write(server_socket, hdr, sizeof_hdr);
						Network_active = 1;
					}
					continue;
				}

				if (memcmp(hdr->target, my_mac, 6) || hdr->size > 600 || hdr->extra) {
					dprintf("target mismatch, hdr->size (%d) too large or unknown command %d\n", hdr->size, hdr->extra);
					continue;
				}

				data = (u8*)memalign(32, ((hdr->size+7)&~7)+sizeof(mbox_msg));
				if (data==NULL) {
					dprintf("Failed to allocate receive buffer\n");
					continue;
				}

				// strip packet counter (pc version)
				while (net_read(server_socket, data, 4)==-EAGAIN);
				hdr->size -=4;

				while (readed < hdr->size) {
					ret = net_read(server_socket, data+readed, hdr->size-readed);
					if (ret==-EAGAIN && !do_exit)
						continue;
					if (ret<=0)
						break;

					readed += ret;
				}

				if (ret<=0) {
					dprintf("Error while receiving packet data %d\n", ret);
					free(data);
					break;
				}

				if (readed != hdr->size) {
					dprintf("Received size does not match hdr->size (%d vs %u)\n", readed, hdr->size);
					free(data);
				} else {
					mbox_msg *msg;

					readed = (readed+7)&~7;
					msg = (mbox_msg*)(data+readed);
					msg->data = data;
					msg->data_size = hdr->size;
					if (!MQ_Send(rec_mbox, (mqmsg_t)&msg, MQ_MSG_NOBLOCK)) {
						// this happens if we get sent packets while we're not doing a receive poll
						dprintf("unable to add message to queue\n");
						free(data);
					}
				}
			} else
				dprintf("Bad receive size %d\n", ret);
#else
			socklen_t s_len = sizeof(address);
			ret = net_recvfrom(server_socket, receive_buffer, sizeof(receive_buffer), 0, (struct sockaddr*)&address, &s_len);
			if (ret==-EAGAIN && !do_exit) {
				AlertableWait(1, 0);
				continue;
			}

			if (ret<=0)
				break;

			if (ret>sizeof_hdr) {
				u8 *data;
				hdr->size = ntohl(hdr->size);

				if (hdr->tag != htonl(DS_TAG) || (memcmp(hdr->target, my_mac, 6) && memcmp(hdr->target, broadcast_node, 6)) || hdr->size > 600) {
					dprintf("tag or target mismatch, or hdr.size too large %08X, %d\n", ntohl(hdr->tag), hdr->size);
					continue;
				}

				if (!memcmp(hdr->source, my_mac, 6) || (ret-sizeof_hdr)<hdr->size)
					continue;

				data = (u8*)memalign(32, ((hdr->size+7)&~7)+sizeof(mbox_msg));
				if (data==NULL)
					continue;

				memcpy(data, receive_buffer+sizeof_hdr, hdr->size);
				mbox_msg *msg;
				ret = (hdr->size+7)&~7;
				msg = (mbox_msg*)(data+ret);
				msg->data = data;
				msg->data_size = hdr->size;
				if (!MQ_Send(rec_mbox, (mqmsg_t)&msg, MQ_MSG_NOBLOCK)) {
					dprintf("unable to add message to queue\n");
					free(data);
				}
			}
#endif
		}
	}

	return NULL;
}

static void ipx_deinit()
{
	do_exit = 1;

	if (server_socket>=0) {
		net_shutdown(server_socket, 3);
		net_close(server_socket);
	}

	if (startup_thread != LWP_THREAD_NULL) {
		void *ret;
		LWP_ResumeThread(startup_thread);
		LWP_JoinThread(startup_thread, &ret);
		startup_thread = LWP_THREAD_NULL;
	}

	if (rec_mbox != MQ_BOX_NULL) {
		MQ_Close(rec_mbox);
		rec_mbox = MQ_BOX_NULL;
	}
}

int ipx_init(int socket_number, int show_address)
{
	if (!do_exit)
		return 0;

	Network_active = 0;
	do_exit = 0;
	if (LWP_CreateThread(&startup_thread, startup, (void*)socket_number, NULL, 0, 64))
		return 3;

	atexit(ipx_deinit);
	return 0;
}

int ipx_change_default_socket( ushort socket_number )
{
	return -1;
}

// Returns a pointer to 6-byte address
ubyte * ipx_get_my_local_address()
{
	return my_mac;
}

// Returns a pointer to 4-byte server
ubyte * ipx_get_my_server_address()
{
	memset(my_server, 0, sizeof(my_server));
	return my_server;
}

// Determines the local address equivalent of an internetwork address.
void ipx_get_local_target(ubyte * server, ubyte * node, ubyte * local_target)
{
	memcpy(local_target, node, 6);
}

// If any packets waiting to be read in, this fills data in with the packet data and returns
// the number of bytes read.  Else returns 0 if no packets waiting.
int ipx_get_packet_data(ubyte * data)
{
	int size;
	mbox_msg *msg;

	if (!Network_active || rec_mbox==MQ_BOX_NULL)
		return 0;

	if (!MQ_Receive(rec_mbox, (mqmsg_t*)&msg, MQ_MSG_NOBLOCK)) {
		// wake up NOW, we want more data
		LWP_ResumeThread(startup_thread);
		return 0;
	}

	size = msg->data_size;
	memcpy(data, msg->data, size);
	free(msg->data);
	return size;
}

static void send_data(const void *node, void *data, int datasize)
{
	dwii_header *hdr = (dwii_header*)packet_buffer;
	int ret=0, sent=0;
	hdr->size = htonl(datasize+4);
	hdr->tag = htonl(DS_TAG);
	hdr->extra = htonl(0);
	memcpy(hdr->source, my_mac, 6);
	memcpy(hdr->target, node, 6);

	// add packet counter field (pc version)
	__stwbrx(packet_buffer, sizeof(*hdr), packet_counter++);
	memcpy(packet_buffer+sizeof(*hdr)+4, data, datasize);
	datasize += sizeof(*hdr)+4;
#ifdef USE_TCP
	while (sent < datasize) {
		ret = net_write(server_socket, packet_buffer+sent, datasize-sent);
		if (ret==-EAGAIN) {
			dprintf("net_write blocked\n");
			continue;
		}
		if (ret<=0)
			break;
		sent += ret;
	}
#else
	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(0x5100);
	address.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	ret = net_sendto(server_socket, packet_buffer, datasize, 0, (struct sockaddr*)&address, 8);
#endif

	if (ret<=0) {
		dprintf("net_sendto error %d\n", ret);
		net_close(server_socket);
		return;
	}

}

// Sends a broadcast packet to everyone on this socket.
void ipx_send_broadcast_packet_data(ubyte * data, int datasize)
{
	send_data(broadcast_node, data, datasize);
}

// Sends a packet to a certain address
void ipx_send_packet_data(ubyte * data, int datasize, ubyte *network, ubyte *address, ubyte *immediate_address)
{
	send_data(address, data, datasize);
}

void ipx_send_internetwork_packet_data(ubyte * data, int datasize, ubyte * server, ubyte *address)
{
	send_data(address, data, datasize);
}

void ipx_read_user_file(char * filename)
{
}

void ipx_read_network_file(char * filename)
{
}

#endif