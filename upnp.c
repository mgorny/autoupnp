/* autoupnp -- automatic UPnP open port forwarder
 *	miniupnpc interface
 * (c) 2010 Michał Górny
 * Distributed under the terms of the 3-clause BSD license
 */

#include <stdlib.h>

#include <pthread.h>

#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>

#ifdef UPNPCOMMAND_HTTP_ERROR
#	define LIBMINIUPNPC_SO_5
#endif

#include "upnp.h"
#include "notify.h"

static const int discovery_delay = 2000; /* [ms] */

struct igd_data {
	struct UPNPUrls urls;
	struct IGDdatas data;

	char lan_addr[16];
};

static int igd_set_up = 0;
static pthread_mutex_t igd_data_lock = PTHREAD_MUTEX_INITIALIZER;

static void unlock_igd(void) {
	pthread_mutex_unlock(&igd_data_lock);
}

static struct igd_data* setup_igd(void) {
	static struct igd_data igd_data;

	pthread_mutex_lock(&igd_data_lock);
	if (!igd_set_up) {
		struct UPNPDev* devlist = upnpDiscover(discovery_delay, NULL, NULL, 0);

		if (UPNP_GetValidIGD(devlist, &(igd_data.urls), &(igd_data.data),
					igd_data.lan_addr, sizeof(igd_data.lan_addr)))
			igd_set_up = 1;
		else
			user_notify(notify_error, "Unable to find an IGD on the network.");

		freeUPNPDevlist(devlist);
	}

	if (igd_set_up)
		return &igd_data;
	else
		unlock_igd();
	return NULL;
}

#pragma GCC visibility push(hidden)

void dispose_igd(void) {
	if (igd_set_up)
		FreeUPNPUrls(&(setup_igd()->urls));
}

int enable_redirect(struct registered_socket_data* rs) {
	const struct igd_data* igd_data = setup_igd();

	if (igd_data) {
		const int ret = UPNP_AddPortMapping(
				igd_data->urls.controlURL,
#ifdef LIBMINIUPNPC_SO_5
				igd_data->data.first.servicetype,
#else
				igd_data->data.servicetype,
#endif
				rs->port, rs->port, igd_data->lan_addr,
				"AutoUPNP-added port forwarding",
				rs->protocol, NULL);

		if (ret == 0) {
			char extip[16];

			if (!UPNP_GetExternalIPAddress(
					igd_data->urls.controlURL,
#ifdef LIBMINIUPNPC_SO_5
					igd_data->data.first.servicetype,
#else
					igd_data->data.servicetype,
#endif
					extip))
				user_notify(notify_added, "%s:%s (%s) forwarded successfully to %s:%s.",
						extip, rs->port, rs->protocol, igd_data->lan_addr, rs->port);
			else
				user_notify(notify_added, "Port %s (%s) forwarded successfully to %s:%s.",
						rs->port, rs->protocol, igd_data->lan_addr, rs->port);
		} else
			user_notify(notify_error, "UPNP_AddPortMapping(%s, %s, %s) failed: %d (%s).",
					rs->port, igd_data->lan_addr, rs->protocol, ret, strupnperror(ret));

		unlock_igd();
		return ret;
	} else
		return -1;
}

int disable_redirect(struct registered_socket_data* rs) {
	const struct igd_data* igd_data = setup_igd();

	if (igd_data) {
		const int ret = UPNP_DeletePortMapping(
				igd_data->urls.controlURL,
#ifdef LIBMINIUPNPC_SO_5
				igd_data->data.first.servicetype,
#else
				igd_data->data.servicetype,
#endif
				rs->port, rs->protocol, NULL);

		if (ret == 0)
			user_notify(notify_removed, "Port forwarding for port %s (%s) removed successfully.",
					rs->port, rs->protocol);
		else
			user_notify(notify_error, "UPNP_DeletePortMapping(%s, %s) failed: %d (%s).",
					rs->port, rs->protocol, ret, strupnperror(ret));

		unlock_igd();
		return ret;
	} else
		return -1;
}

#pragma GCC visibility pop
