/* autoupnp -- automatic UPnP open port forwarder
 *	user notification
 * (c) 2010 Michał Górny
 * Distributed under the terms of the 3-clause BSD license
 */

#include <stdio.h>
#include <stdarg.h>

#include <syslog.h>

#ifdef HAVE_LIBNOTIFY
#	include <libnotify/notify.h>
#endif

#include "notify.h"

#pragma GCC visibility push(hidden)

void dispose_notify(void) {
#ifdef HAVE_LIBNOTIFY
	if (notify_is_initted())
		notify_uninit();
#endif
}

void user_notify(enum notify_type type, const char* const format, ...) {
	va_list ap;
	char buf[1024];
	int syslog_type;
	const char* notify_icon;

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	switch (type) {
		case notify_error:
			syslog_type = LOG_ERR;
			notify_icon = "network-error";
			break;
		case notify_info:
		default:
			syslog_type = LOG_INFO;
			notify_icon = "network-receive";
			break;
	}

#ifdef HAVE_LIBNOTIFY
	if (notify_is_initted() || notify_init("autoupnp")) {
		NotifyNotification* n = notify_notification_new(
				"AutoUPnP", buf, notify_icon, NULL);

		notify_notification_show(n, NULL);
	}
#endif

	syslog(syslog_type, "(AutoUPnP) %s", buf);
}

#pragma GCC visibility pop