/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#ifndef AUTHD_ADDR_H
#define AUTHD_ADDR_H

#define AUTHD_DBUS_SERVICE       "org.test.Authd_Service"
#define AUTHD_DBUS_OBJECT_PATH   "/org/test/Authd_Object"
#define AUTHD_DBUS_INTERFACE     "org.test.Authd_Interface"
#define AUTHD_DBUS_P2P_ADDRESS   "unix:path=/tmp/authd-socket"

#define AUTHD_DBUS_TYPE_SYSTEM   0
#define AUTHD_DBUS_TYPE_SESSION  1
#define AUTHD_DBUS_TYPE_P2P      2

#define AUTHD_DBUS_TYPE          AUTHD_DBUS_TYPE_P2P

#endif  /* AUTHD_ADDR_H */
