#include "../common.h"

#define OUTSIDE_PORT 8889
#define INSIDE_PORT 80

#define BUFLEN 8192

int inside_connect();
static int port=INSIDE_PORT;

pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;

void handle_outside(int fd)
{
	char buf[BUFSIZ];
	int n;
	int size;
	fd_set rfd;
	int err;
	int maxfd;
	struct timeval timeout;
	int fd_inside=-1;

	FD_ZERO(&rfd);
	while(1){

		if(fd_inside<0)
			fd_inside=inside_connect();

		timeout.tv_sec=2;
		timeout.tv_usec=0;

		FD_SET(fd_inside, &rfd);
		FD_SET(fd, &rfd);
		maxfd=max(fd_inside, fd);

		err=select(maxfd+1, &rfd, NULL, NULL, &timeout);
		if(err<=0)
			break;

		if(FD_ISSET(fd_inside, &rfd)){
			memset(buf, 0, BUFSIZ);
			pthread_mutex_lock(&lock);
			n=recv(fd_inside, buf, BUFSIZ-1, 0);
			if(n<=0){
				pthread_mutex_unlock(&lock);
				break;
			}
			printf("%s\n", buf);
			send(fd, buf, strlen(buf),0);
			pthread_mutex_unlock(&lock);
		}
		if(FD_ISSET(fd, &rfd)){
			memset(buf, 0, BUFSIZ);
			pthread_mutex_lock(&lock);
			n=recv(fd, buf, BUFSIZ-1, 0);
			if(n<=0){
				pthread_mutex_unlock(&lock);
				break;
			}
			printf("%s\n", buf);
			send(fd_inside, buf, strlen(buf), 0);
			pthread_mutex_unlock(&lock);
		}
	}
	close(fd_inside);
}

void  outside_connect(char *addr)
{
	int client;
	struct sockaddr_in srv, cliaddr;
	socklen_t len;

	if((client=socket(AF_INET, SOCK_STREAM, 0))<0)
		display_error("outside soket()");
	bzero(&srv, sizeof(srv));
	srv.sin_family=AF_INET;
	srv.sin_port=htons(OUTSIDE_PORT);
	inet_pton(AF_INET, addr, &srv.sin_addr);

	
	if(connect(client, (SA *)&srv, sizeof(srv))<0)
		display_error("outside connect()");

	while(1)
		handle_outside(client);

}

int inside_connect()
{
	int fd;
	struct sockaddr_in srv;

	if((fd=socket(AF_INET, SOCK_STREAM, 0))<0)
		display_error("inside socket()");

	bzero(&srv, sizeof(srv));
	srv.sin_family=AF_INET;
	srv.sin_port=htons(port);
	srv.sin_addr.s_addr=htonl(INADDR_ANY);

	if(connect(fd, (SA *)&srv, sizeof(srv))<0)
		display_error("inside connect()");

	return fd;
}

void usage()
{
	printf("usage: a.out <gateway IP address> <service PORT>\n");
	printf("usage: a.out <gateway IP address>\n");
	return ;
}
int main(int argc, char **argv)
{
	if(argc>3)
		usage();

	if(argc==3)
		port=atoi(argv[2]);
	
	outside_connect(argv[1]);

	return 0;
}


