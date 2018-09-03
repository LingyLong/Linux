#include "../common.h"

#define OUTSIDE_PORT 80
#define INSIDE_PORT 8889

#define BUFLEN 8192

static int fd_inside=-1;
pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;

void display_error(char *err)
{
	perror(err);
	exit(1);
}

void usage()
{
	printf("usage: a.out <Port>\n");
	printf("usage: a.out   will use default port 80\n");
	exit(0);
}

void sig_int(int signo)
{
	printf("process terminated by user\n");
	exit(1);
}

void sig_child(int signo)
{
	pid_t pid;
	int status;

	while((pid=waitpid(-1, &status, WNOHANG))>0)
		printf("child process %d terminated\n", pid);

}

void * handle_outside(void *arg)
{
	int fd=*((int *)arg);
	char buf[BUFLEN];
	int n;
	int size;
	fd_set rfd;
	struct timeval timeout;
	int maxfd;
	int err;

	timeout.tv_sec=2;
	timeout.tv_usec=0;

	FD_ZERO(&rfd);


	while(1){
		FD_SET(fd_inside, &rfd);
		FD_SET(fd, &rfd);
		maxfd=max(fd_inside, fd);
		err=select(maxfd+1, &rfd, NULL, NULL, &timeout);
		if(err<=0)
			break;
		if(FD_ISSET(fd_inside, &rfd)){
			memset(buf, 0, BUFLEN-1);
			pthread_mutex_lock(&lock);
			n=recv(fd_inside, buf, BUFLEN, 0);
			pthread_mutex_unlock(&lock);
			if(n<=0){
				break;
			}
			pthread_mutex_lock(&lock);
			send(fd, buf, strlen(buf), 0);
			pthread_mutex_unlock(&lock);
		}
		if(FD_ISSET(fd, &rfd)){
			memset(buf, 0, BUFLEN);
			pthread_mutex_lock(&lock);
			n=recv(fd, buf, BUFLEN-1, 0);
			pthread_mutex_unlock(&lock);
			if(n<=0){
				break;
			}
			pthread_mutex_lock(&lock);
			send(fd_inside, buf, strlen(buf),0);
			pthread_mutex_unlock(&lock);
		}
	}
	
	
}

void outside(int port)
{
	int sock, client;
	struct sockaddr_in srv, cliaddr;
	socklen_t len;
	pthread_t thread;
	pid_t pid;

	signal(SIGINT, sig_int);
	signal(SIGPIPE, sig_int);
	signal(SIGCHLD, sig_child);

	if((sock=socket(AF_INET, SOCK_STREAM, 0))<0)
		display_error("outside socket()");
	bzero(&srv, sizeof(srv));
	srv.sin_family=AF_INET;
	srv.sin_port=htons(port);
	srv.sin_addr.s_addr=htonl(INADDR_ANY);

	if(bind(sock, (SA *)&srv, sizeof(srv))<0)
		display_error("outside bind()");
	if(listen(sock, 5)<0)
		display_error("outside listen()");

	while(1){
		len=sizeof(cliaddr);
		client=accept(sock, (SA *)&cliaddr, &len);
		if(client<0)
			display_error("outside accept()");

		pthread_create(&thread, NULL, handle_outside, &client);
		pthread_join(thread, NULL);

//		if((pid=fork())<0)
//			display_error("outside fork()");
//		else if(pid==0){
//			close(sock);

//			handle_outside(client);
			
//			close(client);
//			exit(0);
//		}
//		else{
//			close(client);
//		}
	}
}


void inside()
{
	int sock;
	struct sockaddr_in srv, cliaddr;
	socklen_t len;

	if((sock=socket(AF_INET, SOCK_STREAM, 0))<0)
		display_error("inside socket()");

	bzero(&srv, sizeof(srv));
	srv.sin_family=AF_INET;
	srv.sin_port=htons(INSIDE_PORT);
	srv.sin_addr.s_addr=htonl(INADDR_ANY);

	if(bind(sock, (SA *)&srv, sizeof(srv))<0)
		display_error("inside bind()");
	if(listen(sock, 5)<0)
		display_error("inside listen()");

	len=sizeof(cliaddr);
	fd_inside=accept(sock, (SA *)&cliaddr, &len);
	if(fd_inside<0)
		display_error("inside accept()");
	close(sock);
}

int main(int argc, char **argv)
{
	int port;

	if(argc>2)
		usage();

	if(argc==2)
		port=atoi(argv[1]);
	
	if(argc==1)
		port=OUTSIDE_PORT;

	inside();
	outside(port);	
	return 0;
}
