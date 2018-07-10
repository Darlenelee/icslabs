/*
 * proxy.c - ICS Web proxy
 *
 *
 */

#include "csapp.h"
#include <stdarg.h>
#include <sys/select.h>

struct para{
    int fd;
    struct sockaddr_in sockaddr;
};
/*
 * Function prototypes
 */
int parse_uri(char *uri, char *target_addr, char *path, char *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, size_t size);
void doit(int fd, struct sockaddr_in sockaddr);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
int Rio_writen_w(int fd, void *usrbuf, size_t n);
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t n);
ssize_t Rio_readnb_w(rio_t *rp, void *usrbuf, size_t n);
void *thread(void* p);
sem_t mutex, mutex_fd;
/*
 * main - Main routine for the proxy program
 */
int main(int argc, char **argv)
{
    /* Check arguments */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
        exit(0);
    }
    int listenfd;
    socklen_t clientlen;
    pthread_t tid;
    sem_init(&mutex, 0, 1);
    sem_init(&mutex_fd, 0, 1);
    struct sockaddr_storage clientaddr;    
    listenfd = open_listenfd(argv[1]);
    clientlen = sizeof(clientaddr);
    signal(SIGPIPE, SIG_IGN);
    while(1){
        struct para *p = (struct para*)Malloc(sizeof(struct para));
        p->fd = accept(listenfd, (SA*)(&p->sockaddr), &clientlen);
        pthread_create(&tid, NULL, thread, (void*)p);
        
    }

}

void *thread(void* p){
    pthread_detach(pthread_self());
    doit(((struct para*)p)->fd,((struct para*)p)->sockaddr);
    free(p);
    return NULL;
}
int Rio_writen_w(int fd, void *usrbuf, size_t n){
    if(rio_writen(fd, usrbuf, n) != n){
        if(errno == EPIPE){
            return 0;
        }
        return 0;
    }
    return 1;
}
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t n){
    ssize_t status;
    if((status=rio_readlineb(rp, usrbuf, n))<0){
        return 0;
    }
    return status;
}
ssize_t Rio_readnb_w(rio_t *rp, void *usrbuf, size_t n){
    ssize_t status;
    if((status=rio_readnb(rp, usrbuf, n))<0){
        return 0;
    }
    return status;
}
void doit(int fd, struct sockaddr_in sockaddr){
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE], pathname[MAXLINE], port[MAXLINE];
    char log[MAXLINE];
    
    size_t size = 0;
    rio_t rio;
    rio_readinitb(&rio, fd);
    Rio_readlineb_w(&rio, buf, MAXLINE);
    
    sscanf(buf, "%s %s %s", method, uri, version);
    parse_uri(uri, hostname, pathname, port);
    
    
    // request line 
    int server_fd;
    P(&mutex_fd);
    server_fd = open_clientfd(hostname, port);
    V(&mutex_fd);
    sprintf(buf,"%s /%s HTTP/1.1\r\n", method,pathname);
    Rio_writen_w(server_fd, buf, strlen(buf));
    int reqlength;
    // header
    while(Rio_readlineb_w(&rio, buf, MAXLINE)>0){
        Rio_writen_w(server_fd, buf, strlen(buf));
        if(strcmp(buf,"\r\n")==0){
            break;
        }
        if(strstr(buf, "Content-Length:")){
            sscanf(buf, "Content-Length: %d\r\n", &reqlength);
        }
    }
    
    // body
    if(!strcmp(method, "POST")){
        while(reqlength > 0){
            Rio_readnb_w(&rio, buf, 1);
            Rio_writen_w(server_fd, buf, 1);
            reqlength--;
        }
    }
    
    rio_t rio2;
    rio_readinitb(&rio2, server_fd);
    int n;
    int length;
   
    while((n=Rio_readlineb_w(&rio2, buf, MAXLINE))!=0){
        if(strcmp(buf, "\r\n")!=0){
            Rio_writen_w(fd, buf, strlen(buf));
            size += n;
            if(strstr(buf, "Content-Length:")){
                sscanf(buf, "Content-Length: %d\r\n", &length);
            }
        }
        else {
            size+=2;  
            break; 
        }
    }
    size += length;
    Rio_writen_w(fd, "\r\n", strlen("\r\n"));
    while(length>0){
        Rio_readnb_w(&rio2, buf, 1);
        Rio_writen_w(fd, buf, 1);
        length--;
    }
 
    P(&mutex);
    format_log_entry(log, &sockaddr, uri, size);
    printf("%s\n",log);
    V(&mutex);
    close(fd);
    close(server_fd);
    return;


}


/*
 * parse_uri - URI parser
 *
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, char *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) {
        hostname[0] = '\0';
        return -1;
    }

    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    if (hostend == NULL)
        return -1;
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';

    /* Extract the port number */
    if (*hostend == ':') {
        char *p = hostend + 1;
        while (isdigit(*p))
            *port++ = *p++;
        *port = '\0';
    } else {
        strcpy(port, "80");
    }

    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL) {
        pathname[0] = '\0';
    }
    else {
        pathbegin++;
        strcpy(pathname, pathbegin);
    }

    return 0;
}

/*
 * format_log_entry - Create a formatted log entry in logstring.
 *
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), the number of bytes
 * from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr,
                      char *uri, size_t size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /*
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 12, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;

    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s %zu", time_str, a, b, c, d, uri, size);
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg){ 
    char buf[MAXLINE], body[MAXLINE];
    // build body
    sprintf(body, "%s: %s\r\n", errnum, shortmsg);
    sprintf(body, "%s%s: %s",body,longmsg,cause);
    // print the http response
    sprintf(buf, "HTTP/1.1 %s %s\r\n", errnum, shortmsg);
    Rio_writen_w(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen_w(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen_w(fd, buf, strlen(buf));
    Rio_writen_w(fd, body, strlen(body));
    
}
