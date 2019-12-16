#include <stdlib.h>
#include <string.h>

char readChar(char**pptr);
void writeChar(char **pptr, char c);

#define Packet_RC_sub      0x01
#define Packet_RC_suback   0x02
#define Packet_RC_pub      0x03
#define Packet_RC_puback   0x04
#define Packet_RC_unsub    0x05
#define Packet_RC_unsuback 0x06
#define Packet_RC_errfull  0x07
#define Packet_RC_errmax   0x08
#define Packet_RC_errno    0x09

char readChar(char **pptr)
{
	char c = **pptr;
	(*pptr)++;
	return c;
}

void writeChar(char **pptr, char c)
{
	**pptr = c;
	(*pptr)++;
}
