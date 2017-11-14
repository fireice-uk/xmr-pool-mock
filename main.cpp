#include <iostream>
#include <stdio.h>
#include <stdlib.h> 
#include "socks.h"

using namespace std;

char motd[2048] = {0};

SOCKET cli_sck;
bool process_line(size_t& n, char* line, size_t lnlen)
{
    char buffer[1024];
    const char* rsp;
    const char* fmt;

    switch(n)
    {
    case 0:
        fmt = "{\"id\":0,\"jsonrpc\":\"2.0\",\"error\":null,\"result\":{\"id\":\"decafbad0\",\"job\":{\"blob\":\"0000000000000000000000000000000000000000000000000000000000"
            "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\",\"job_id\":\"00000000\",\"target\":\"ffff3f00\",\"motd\":\"%s\"},"
            "\"extensions\":[\"backend\",\"hashcount\",\"algo\",\"motd\"],\"status\":\"OK\"}}\n";
	snprintf(buffer, sizeof(buffer), fmt, motd);
	rsp = buffer;
        break;
    default:
        rsp = "{\"id\":1,\"jsonrpc\":\"2.0\",\"error\":null,\"result\":{\"status\":\"OK\"}}\n";
        break;
    }
    n++;

    send(cli_sck, rsp, strlen(rsp), 0);
    line[lnlen] = '\0';
    printf("RECV %s\nSEND %s\n", line, rsp);
    return true;
}

inline char hf_bin2hex(unsigned char c)
{
	if (c <= 0x9)
		return '0' + c;
	else
		return 'a' - 0xA + c;
}

void bin2hex(const unsigned char* in, unsigned int len, char* out)
{
	for (unsigned int i = 0; i < len; i++)
	{
		out[i * 2] = hf_bin2hex((in[i] & 0xF0) >> 4);
		out[i * 2 + 1] = hf_bin2hex(in[i] & 0x0F);
	}
}

int main(int argc, char** argv)
{
    if(argc != 3)
        return 0;

    bin2hex((const unsigned char*)argv[2], strlen(argv[2]), motd);

    sockaddr_in my_addr;

    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);

    if(s == INVALID_SOCKET)
        return false;

	int enable = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		fputs("setsockopt(SO_REUSEADDR) failed\n", stderr);

	/*enable = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0)
		fputs("setsockopt(SO_REUSEPORT) failed\n", stderr);*/

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(atoi(argv[1]));

    if(bind(s, (sockaddr*) &my_addr, sizeof(my_addr)) < 0)
        return false;

    if(listen(s, SOMAXCONN) < 0)
        return false;

    while(true)
    {
        socklen_t ln = sizeof(sockaddr_in);
        cli_sck = accept(s, nullptr, &ln);

        char buf[2048];
        size_t datalen = 0;
        int ret;
        size_t n = 0;
        while (true)
        {
            ret = recv(cli_sck, buf + datalen, sizeof(buf) - datalen, 0);

            if(ret == 0)
                break;
            if(ret == SOCKET_ERROR || ret < 0)
                break;

            datalen += ret;

            if (datalen >= sizeof(buf))
                break;

            char* lnend;
            char* lnstart = buf;
            while ((lnend = (char*)memchr(lnstart, '\n', datalen)) != nullptr)
            {
                lnend++;
                int lnlen = lnend - lnstart;

                if (!process_line(n, lnstart, lnlen))
                    break;

                datalen -= lnlen;
                lnstart = lnend;
            }

            //Got leftover data? Move it to the front
            if (datalen > 0 && buf != lnstart)
                memmove(buf, lnstart, datalen);
        }
    }
}
