#ifndef ADDR_H
#define ADDR_H

#define SERV_DBUS_SERVICE       "my.test.Serv_Service"
#define SERV_DBUS_OBJECT_PATH   "/my/test/Serv_Object"
#define SERV_DBUS_INTERFACE     "my.test.Serv_Interface"
#define SERV_DBUS_P2P_ADDRESS   "unix:path=/tmp/qdbus-serv-socket"

#define SERV_DBUS_TYPE_SYSTEM   0
#define SERV_DBUS_TYPE_SESSION  1
#define SERV_DBUS_TYPE_P2P      2

// 0: system, 1: session, 2: peer-to-peer
#define SERV_DBUS_TYPE          SERV_DBUS_TYPE_SYSTEM


#endif  /* ADDR_H */
