/* autoupnp -- automatic UPnP open port forwarder
 *	The open socket registry
 * (c) 2010 Michał Górny
 * Distributed under the terms of the 3-clause BSD license
 */

#include <stdlib.h>

struct registered_socket {
	int fd;
	/* XXX: sockaddr etc. */

	struct registered_socket* next;
};

static struct registered_socket* socket_registry = NULL;

void registry_add(const int fildes) {
	struct registered_socket* new_socket = malloc(sizeof(*new_socket));

	if (!new_socket)
		return;

	new_socket->fd = fildes;
	new_socket->next = socket_registry;
	socket_registry = new_socket;
}

void registry_remove(const int fildes) {
	struct registered_socket *i, *prev;

	for (i = socket_registry, prev = NULL; i; prev = i, i = i->next) {
		if (i->fd == fildes) {
			if (prev)
				prev->next = i->next;
			else
				socket_registry = i->next;
			free(i);
			return;
		}
	}
}
