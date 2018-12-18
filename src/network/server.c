#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include "tswoole_config.h"
#include "../../include/server.h"
#include "../../include/epoll.h"

tswServer *tswServer_new(void)
{
	tswServer *serv;

	serv = (tswServer *)malloc(sizeof(tswServer));
	if (serv == NULL) {
		return NULL;
	}
	serv->onStart = NULL;
	serv->onConnect = NULL;
	serv->onReceive = NULL;
	serv->onClose = NULL;

	return serv;
}

static int tswServer_start_proxy(tswServer *serv)
{
	tswReactor *main_reactor = malloc(sizeof(tswReactor));
	if (main_reactor == NULL) {
		tswWarn("%s", "malloc error");
		return TSW_ERR;
	}

	if (tswReactor_create(main_reactor, MAXEVENTS) < 0) {
		tswWarn("%s", "tswReactor_create error");
		return TSW_ERR;
	}

	if (tswReactorThread_create(serv) < 0) {
		tswWarn("%s", "tswReactorThread_create error");
		return TSW_ERR;
	}

	if (tswReactorThread_start(serv) < 0) {
		tswWarn("%s", "tswReactorThread_start error");
		return TSW_ERR;
	}

	if (listen(serv->serv_sock, LISTENQ) < 0) {
		tswWarn("%s", strerror(errno));
	}
	if (serv->onStart != NULL) {
		serv->onStart();
	}

	if (main_reactor->add(main_reactor, serv->serv_sock, TSW_EVENT_READ, tswServer_master_onAccept) < 0) {
		tswWarn("%s", "reactor add error");
		return TSW_ERR;
	}

	for (;;) {
		int nfds;

		nfds = main_reactor->wait(main_reactor);

		for (int i = 0; i < nfds; i++) {
			int connfd;
		    tswReactorEpoll *reactor_epoll_object = main_reactor->object;

			tswEvent *tswev = (tswEvent *)reactor_epoll_object->events[i].data.ptr;
			if (tswev->event_handler(main_reactor, tswev) < 0) {
				tswWarn("%s", "event_handler error");
				continue;
			}
		}
	}

	close(serv->serv_sock);

	return TSW_OK;
}

int tswServer_start(tswServer *serv)
{
	if (tswServer_start_proxy(serv) < 0) {
		tswWarn("%s", "tswServer_start_proxy error");
		return TSW_ERR;
	}

	return TSW_OK;
}

int tswServer_master_onAccept(tswReactor *reactor, tswEvent *tswev)
{
	int connfd;
	socklen_t len;
	struct sockaddr_in cliaddr;

	len = sizeof(cliaddr);
	connfd = accept(tswev->fd, (struct sockaddr *)&cliaddr, &len);
	if (connfd < 0) {
		tswWarn("%s", "accept error");
		return TSW_ERR;
	}
	TSwooleG.serv->onConnect(connfd);

	if (reactor->add(reactor, connfd, TSW_EVENT_READ, tswServer_master_onReceive) < 0) {
		tswWarn("%s", "reactor add error");
		return TSW_ERR;
	}

	return TSW_OK;
}

int tswServer_master_onReceive(tswReactor *reactor, tswEvent *tswev)
{
	int n;
	char buffer[MAX_BUF_SIZE];
	tswReactorEpoll *reactor_epoll_object = reactor->object;

	n = read(tswev->fd, buffer, MAX_BUF_SIZE);
	if (n == 0) {
		reactor->del(reactor, tswev->fd);
		close(tswev->fd);
		free(tswev);
		reactor->event_num -= 1;
		return TSW_OK;
	}
	buffer[n] = 0;
	TSwooleG.serv->onReceive(TSwooleG.serv, tswev->fd, buffer);

	return TSW_OK;
}

int tswServer_tcp_send(tswServer *serv, int fd, const void *data, size_t length)
{
	return send(fd, data, length, 0);
}
