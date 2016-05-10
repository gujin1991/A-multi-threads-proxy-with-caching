/*
 * Andrew ID: jingu
 * proxy.c - A very simple concurrent proxy with caching.
 *           Every time a client sent a request to the proxy,
 *           the proxy create a new thread to deal with it.
 *           If the request have been sent before, the proxy
 *           gets the data from cache. Otherwise it sent the
 *           request to the server and cache the return data.
 *           Finally the proxy respond the client with requested
 *           data. The proxy only works with GET method. The
 *           cache adopt LRU strategy.
 */
#include <stdio.h>
#include <stdbool.h>
#include "csapp.h"
#include "cache.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define LINE_NUM 10

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding = "Accept-Encoding: gzip, deflate\r\n";

/* functions declaration */
void *thread(void *vargp);
void clienterror(int fd, char *cause, char *errnum,
                    char *shortmsg, char *longmsg);
void Proxy(int connfd);
void parseUrl(char *url, char *hostname, char *path);
void getPort(char *hostname, char *port);
void getHeaders(rio_t *rp, char *headers);
void requestData(int connfd, char* hostname, char *path,
                char *version, char *headers, char *port);
int initProxy();

int main(int argc, char **argv)
{
    int listenfd, *connfdp;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid;


    if(argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    /* Ingnore the pipe signal */
    Signal(SIGPIPE, SIG_IGN);
    listenfd = Open_listenfd(argv[1]);
    if(initCache() == 1) {
        printf("The malloced cache is bigger than given size.\n");
        return -1;
    }


    while(1) {
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Pthread_create(&tid, NULL, thread, connfdp);
    }

    return 0;
}



/* Thread routine */
void *thread(void *vargp)
{
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);
    Proxy(connfd);
    Close(connfd);
    return NULL;
}

/*
 * Proxy - Deal with the client request. If the request had been received
 *         before, return client with cache data. Otherwise forward the
 *         request to server and return client with respond data.
 */
void Proxy(int connfd)
{
    char hostname[MAXLINE], path[MAXLINE], url[MAXLINE];
    char method[MAXLINE], version[MAXLINE], headers[MAXLINE];
    char buf[MAXLINE];
    char port[6] = "80";
    rio_t rio;

    /* read request from client */
    Rio_readinitb(&rio, connfd);
    if(!Rio_readlineb(&rio, buf, MAXLINE)) {
        return;
    }

    /* parse method, url and version with buf */
    sscanf(buf, "%s %s %s", method, url, version);

    /* check the mothod */
    if(strcasecmp(method, "GET")) {
        clienterror(connfd, method, "501", "Not implemented method",
                        "Only GET method has been implemented");
        return;
    }

    /* change the version */
    if(strcasecmp(version, "http/1.1")) {
        strcpy(version, "http/1.0");
    }

    /* get hostname, path , port and addtional headers*/
    parseUrl(url, hostname, path);

    getPort(hostname, port);

    getHeaders(&rio, headers);

    /* deal with the request */
    requestData(connfd, hostname, path, version, headers, port);



}

/*
 *  clienterror - return an error page to client
 */
void clienterror(int fd, char *cause, char *errnum,
                    char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXLINE];

    /* Build body */
    sprintf(body, "<html><title>Web Proxy Error</title>");
    sprintf(body, "%s<body bgcolor =\"#FF8680\">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>Alex & Saumya's Web Proxy</em>\r\n", body);

    /* Print out response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

/*
 * paeseUrl - Get hostname and path with url.
 */
void parseUrl(char *url, char *hostname, char *path)
 {
     char *localUrl;
     char *firstIdx;
     char point[MAXLINE];

     memset(hostname, 0, strlen(hostname));
     memset(path, 0, strlen(path));
     strcpy(point, url);
     localUrl = point;
     /* remove the "http://" */
     localUrl += 7;


     if((firstIdx = strchr(localUrl, '/')) == NULL) {
         strcpy(hostname, localUrl);
         strcpy(path, "/");
     } else {
         strcpy(path, firstIdx);
         firstIdx[0] = '\0';
         strcpy(hostname, localUrl);
     }
 }

/*
 *  getPort - get port number with hostname.
 */
void getPort(char *hostname, char *port)
{
    char *charIdx;

    if((charIdx = strchr(hostname, ':')) != NULL) {
        memset(port, 0, strlen(port));
        charIdx[0] = '\0';
        charIdx++;
        strcpy(port, charIdx);

    }
}

/*
 * getHeaders - get other headers information which is not include.
 */
void getHeaders(rio_t *rp, char *headers)
{
    char buf[MAXLINE];


    Rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")) {
        /* find additional header infomation */
        if(strncmp(buf,"Host: ", strlen("Host: ")) &&
           strncmp(buf, "User-Agent: ", strlen("User-Agent: ")) &&
           strncmp(buf, "Accept: ", strlen("Accept: ")) &&
           strncmp(buf, "Accept-Encoding: ", strlen("Accept-Encoding: ")) &&
           strncmp(buf, "Connection: ", strlen("Connection: ")) &&
           strncmp(buf, "Proxy-Connection: ", strlen("Proxy-Connection: ")))
           {
                sprintf(headers, "%s%s", headers, buf);
           }

        Rio_readlineb(rp, buf, MAXLINE);

    }

}

/*
 *  requestData - Deal with the client request by either returning client
 *                the cache data or forwarding the request to theserver
 */
void requestData(int connfd, char* hostname, char *path,
                char *version, char *headers, char *port)
{
    char buf[MAXLINE], url[MAXLINE];
    char responseData[MAXLINE], cacheData[MAXLINE];
    int servefd, returnLen, object_len;
    rio_t rio;

    /* get new url and headers */
    sprintf(url, "%s%s", hostname, path);
    sprintf(buf, "GET %s %s\r\n", path, version);
    sprintf(buf, "%sHost: %s\r\n", buf, hostname);
    sprintf(buf, "%s%s", buf, user_agent_hdr);
    sprintf(buf, "%s%s", buf, accept_hdr);
    sprintf(buf, "%s%s", buf, accept_encoding);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sProxy-Connection: close\r\n", buf);
    sprintf(buf, "%s%s\r\n", buf, headers);

    /* check if the url had been received before */
    if(checkIfExists(url, cacheData)) {
        /* If yes, return the cache data */
        Rio_writen(connfd, cacheData, strlen(cacheData));
        memset(cacheData, 0, strlen(cacheData));

    } else {

        /* otherwise send the request to serve */
        servefd = Open_clientfd(hostname, port);
            if(servefd < - 1) {
                clienterror(servefd, hostname, "404" ,
                        "Server Error", "Can not find the serve");
                return;
            }

        Rio_writen(servefd, buf, strlen(buf));
        memset(buf, 0, strlen(buf));
        memset(responseData, 0, strlen(responseData));

        Rio_readinitb(&rio, servefd);
        object_len = 0;
        returnLen = 0;

        /* get the respond data */
        while((returnLen = Rio_readnb(&rio, responseData,
                                sizeof(responseData))) > 0) {

            object_len += returnLen;
            Rio_writen(connfd, responseData, returnLen);
            sprintf(cacheData, "%s%s", cacheData, responseData);
            memset(responseData, 0, strlen(responseData));
        }

        /*  cache the respond data */
        if(object_len <= MAX_OBJECT_SIZE) {
            //P(&mutex);
            saveObjects(url, cacheData);
            //V(&mutex);
            memset(cacheData, 0, strlen(cacheData));
        }

        close(servefd);
    }

}



