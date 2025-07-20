#ifndef __WIFI_CONNECT_H__
#define __WIFI_CONNECT_H__

#define WIFI_CONNECT_DHCP     1 //手动设置IP:0  DHCP:1

int WifiConnect(const char *ssid, const char *pwd);

#endif /* __WIFI_CONNECT_H__ */