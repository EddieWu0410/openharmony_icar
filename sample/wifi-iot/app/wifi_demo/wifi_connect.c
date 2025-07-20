#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "lwip/api_shell.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"

#include "cmsis_os2.h"
#include "ohos_init.h"
#include "wifi_device.h"
#include "wifi_error_code.h"
#include "wifi_connect.h"

#define DEF_TIMEOUT 15
#define ONE_SECOND 1

static void WiFiInit(void);
static void WaitSacnResult(void);
static int WaitConnectResult(void);
static void OnWifiScanStateChangedHandler(int state, int size);
static void OnWifiConnectionChangedHandler(int state, WifiLinkedInfo *info);
static void OnHotspotStaJoinHandler(StationInfo *info);
static void OnHotspotStateChangedHandler(int state);
static void OnHotspotStaLeaveHandler(StationInfo *info);
static void StaSetLocaladdr(struct netif *netif, const ip4_addr_t *gw, const ip4_addr_t *ipaddr, const ip4_addr_t *netmask);

static int g_staScanSuccess = 0;
static int g_connectSuccess = 0;
static int g_ssid_count = 0;
static struct netif *g_lwip_netif = NULL;
static WifiEvent g_wifiEventHandler = {0};

#define SELECT_WLAN_PORT "wlan0"
#define SELECT_WIFI_SECURITYTYPE WIFI_SEC_TYPE_PSK

// 定义wifi事件回调处理函数
static void WiFiInit(void)
{
    printf("<--Wifi Init-->\r\n");
    g_wifiEventHandler.OnWifiScanStateChanged = OnWifiScanStateChangedHandler;
    g_wifiEventHandler.OnWifiConnectionChanged = OnWifiConnectionChangedHandler;
    g_wifiEventHandler.OnHotspotStaJoin = OnHotspotStaJoinHandler;
    g_wifiEventHandler.OnHotspotStaLeave = OnHotspotStaLeaveHandler;
    g_wifiEventHandler.OnHotspotStateChanged = OnHotspotStateChangedHandler;
    WifiErrorCode error = RegisterWifiEvent(&g_wifiEventHandler);
    if (error != WIFI_SUCCESS)
    {
        printf("register wifi event handler fail!\r\n");
    }
    else
    {
        printf("register wifi event handler succeed!\r\n");
    }
}

int WifiConnect(const char *ssid, const char *pwd)
{
    WifiScanInfo *info = NULL;
    unsigned int size = WIFI_SCAN_HOTSPOT_LIMIT;
    WifiDeviceConfig select_ap_config = {0};
    WifiErrorCode error = WIFI_SUCCESS;

    osDelay(200);
    printf("<--System Init-->\r\n");

    // 初始化WIFI
    WiFiInit();

    // 使能WIFI
    if ((error = EnableWifi()) != WIFI_SUCCESS)
    {
        printf("EnableWifi failed, error = %d\r\n", error);
        return -1;
    }

    // 判断WIFI是否激活
    if (IsWifiActive() == 0)
    {
        printf("Wifi station is not actived.\r\n");
        return -1;
    }

    // 分配空间，保存WiFi信息
    info = malloc(sizeof(WifiScanInfo) * WIFI_SCAN_HOTSPOT_LIMIT);
    if (info == NULL)
    {
        return -1;
    }

    // 轮询查找WiFi列表
    do
    {
        // 重置标志位
        g_ssid_count = 0;
        g_staScanSuccess = 0;

        // 开始扫描
        Scan();

        // 等待扫描结果
        WaitSacnResult();

        // 获取扫描列表
        error = GetScanInfoList(info, &size);

    } while (g_staScanSuccess != 1);

    // 打印WiFi热点列表
    for (uint8_t i = 0; i < g_ssid_count; i++)
    {
        printf("no:%03d, ssid:%-30s, rssi:%5d\r\n", i + 1, info[i].ssid, info[i].rssi / 100);
    }

    // 连接指定的WiFi热点
    for (int i = 0; i < g_ssid_count; i++)
    {
        if (strcmp(ssid, info[i].ssid) == 0)
        {
            int result;

            // 拷贝要连接的热点信息
            strcpy(select_ap_config.ssid, info[i].ssid);
            strcpy(select_ap_config.preSharedKey, pwd);
            select_ap_config.securityType = SELECT_WIFI_SECURITYTYPE;

            if (AddDeviceConfig(&select_ap_config, &result) == WIFI_SUCCESS)
            {
                if (ConnectTo(result) == WIFI_SUCCESS && WaitConnectResult() == 1)
                {
                    printf("WiFi connect succeed!\r\n");
                    g_lwip_netif = netifapi_netif_find(SELECT_WLAN_PORT);
                    break;
                }
            }
        }

        if (i == g_ssid_count - 1)
        {
            printf("ERROR: No wifi as expected\r\n");
            while (1)
                osDelay(100);
        }
    }

#if WIFI_CONNECT_DHCP
    //启动DHCP
    if (g_lwip_netif) {
        dhcp_start(g_lwip_netif);
        printf("begain to dhcp\r\n");
    }

    //等待DHCP
    for (;;) {
        if (dhcp_is_bound(g_lwip_netif) == ERR_OK) {
            printf("<-- DHCP state:OK -->\r\n");

            //打印获取到的IP信息
            netifapi_netif_common(g_lwip_netif, dhcp_clients_info_show, NULL);
            break;
        }
        printf("<-- DHCP state:Inprogress -->\r\n");
        osDelay(100);
    }
#else
    if (g_lwip_netif)
    {
        // IP 地址、网关和子网掩码的定义
        ip4_addr_t ipaddr;
        ip4_addr_t netmask;
        ip4_addr_t gw;

        // 使用 IP4_ADDR 宏正确设置 IP 地址、网关和子网掩码
        IP4_ADDR(&ipaddr, 192, 168, 1, 10);    // 192.168.1.10
        IP4_ADDR(&netmask, 255, 255, 255, 0); // 255.255.255.0
        IP4_ADDR(&gw, 192, 168, 1, 1);        // 192.168.1.1

        // 手动设置IP地址、网关和子网掩码，直接传递 ip4_addr_t 结构体
        StaSetLocaladdr(g_lwip_netif, &gw, &ipaddr, &netmask);

        // // 打印当前的 IP 地址、子网掩码和网关信息
        // printf("IP Address : %s\r\n", ip4addr_ntoa(netif_ip4_addr(g_lwip_netif)));
        // printf("Netmask    : %s\r\n", ip4addr_ntoa(netif_ip4_netmask(g_lwip_netif)));
        // printf("Gateway    : %s\r\n", ip4addr_ntoa(netif_ip4_gw(g_lwip_netif)));
    }
    else
    {
        printf("Network interface not found or down.\r\n");
    }
#endif
  
    // 执行其他操作
    osDelay(100);

    return 0;
}

static void OnWifiScanStateChangedHandler(int state, int size)
{
    (void)state;
    if (size > 0)
    {
        g_ssid_count = size;
        g_staScanSuccess = 1;
    }
    printf("callback function for wifi scan:%d, %d\r\n", state, size);
    return;
}

static void OnWifiConnectionChangedHandler(int state, WifiLinkedInfo *info)
{
    (void)info;

    if (state > 0)
    {
        g_connectSuccess = 1;
        printf("callback function for wifi connect\r\n");
    }
    else
    {
        printf("connect error,please check password\r\n");
    }
}

static void OnHotspotStaJoinHandler(StationInfo *info)
{
    (void)info;
    printf("STA join AP\n");
    return;
}

static void OnHotspotStaLeaveHandler(StationInfo *info)
{
    (void)info;
    printf("HotspotStaLeave:info is null.\n");
    return;
}

static void OnHotspotStateChangedHandler(int state)
{
    printf("HotspotStateChanged:state is %d.\n", state);
    return;
}

static void WaitSacnResult(void)
{
    int scanTimeout = DEF_TIMEOUT;
    while (scanTimeout > 0)
    {
        sleep(ONE_SECOND);
        scanTimeout--;
        if (g_staScanSuccess == 1)
        {
            printf("WaitSacnResult:wait success[%d]s\n", (DEF_TIMEOUT - scanTimeout));
            break;
        }
    }
    if (scanTimeout <= 0)
    {
        printf("WaitSacnResult:timeout!\n");
    }
}

static int WaitConnectResult(void)
{
    int ConnectTimeout = DEF_TIMEOUT;
    while (ConnectTimeout > 0)
    {
        sleep(ONE_SECOND);
        ConnectTimeout--;
        if (g_connectSuccess == 1)
        {
            printf("WaitConnectResult:wait success[%d]s\n", (DEF_TIMEOUT - ConnectTimeout));
            break;
        }
    }
    if (ConnectTimeout <= 0)
    {
        printf("WaitConnectResult:timeout!\n");
        return 0;
    }

    return 1;
}
static void StaSetLocaladdr(struct netif *netif, const ip4_addr_t *gw, const ip4_addr_t *ipaddr, const ip4_addr_t *netmask)
{
    if (netif == NULL)
    {
        printf("StaSetLocaladdr::Null param of netdev\r\n");
        return;
    }

    // 调用 LWIP API 将 IP 地址、子网掩码和网关应用到网络接口上
    netifapi_netif_set_addr(netif, ipaddr, netmask, gw);

    // 打印确认信息
    printf("IP Address, Netmask, and Gateway set.\r\n");
}