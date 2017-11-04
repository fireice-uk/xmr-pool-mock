#include <iostream>
#include "socks.h"

using namespace std;

SOCKET cli_sck;
bool process_line(size_t& n, char* line, size_t lnlen)
{
    const char* rsp;
    switch(n)
    {
    case 0:
        rsp = "{\"id\":0,\"jsonrpc\":\"2.0\",\"error\":null,\"result\":{\"id\":\"decafbad0\",\"job\":{\"blob\":\"0000000000000000000000000000000000000000000000000000000000"
            "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\",\"job_id\":\"00000000\",\"target\":\"ffff3f00\"},\"status\":\"OK\"}}\n";
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

int main(int argc, char** argv)
{
    if(argc != 2)
        return 0;

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
