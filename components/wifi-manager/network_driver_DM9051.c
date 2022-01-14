#include "esp_eth.h"
#include "network_ethernet.h"

static EXT_RAM_ATTR network_ethernet_driver_t DM9051;
static EXT_RAM_ATTR spi_device_interface_config_t devcfg;
static EXT_RAM_ATTR esp_netif_config_t cfg_spi;
static EXT_RAM_ATTR esp_netif_inherent_config_t esp_netif_config;

static esp_err_t start(spi_device_handle_t spi_handle, eth_config_t* ethernet_config) {
#ifdef CONFIG_ETH_SPI_ETHERNET_DM9051
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_dm9051_config_t eth_config = ETH_DM9051_DEFAULT_CONFIG(spi_handle);
    // we assume that isr has been installed already
    eth_config.int_gpio_num = ethernet_config->intr;
    phy_config.phy_addr = -1;
    phy_config.reset_gpio_num = ethernet_config->rst;

    esp_eth_mac_t* mac = esp_eth_mac_new_dm9051(&eth_config, &mac_config);
    esp_eth_phy_t* phy = esp_eth_phy_new_dm9051(&phy_config);
    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    return esp_eth_driver_install(&config, &DM9051.handle);
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

static void init_config(eth_config_t* ethernet_config) {
	esp_netif_inherent_config_t loc_esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
    devcfg.command_bits = 1;  
    devcfg.address_bits = 7;  
    devcfg.mode = 0;
    devcfg.clock_speed_hz = ethernet_config->speed > 0 ? ethernet_config->speed : SPI_MASTER_FREQ_20M;  // default speed
    devcfg.queue_size = 20;
    devcfg.spics_io_num = ethernet_config->cs;
    memcpy(&esp_netif_config, &loc_esp_netif_config, sizeof(loc_esp_netif_config));
    cfg_spi.base = &esp_netif_config,
    cfg_spi.stack = ESP_NETIF_NETSTACK_DEFAULT_ETH;
    DM9051.cfg_netif = &cfg_spi;
    DM9051.devcfg = &devcfg;
    DM9051.start = start;
}

network_ethernet_driver_t* DM9051_Detect(char* Driver) {
    if (!strcasestr(Driver, "DM9051"))
        return NULL;
    DM9051.rmii = false;
    DM9051.spi = true;
#ifdef CONFIG_ETH_SPI_ETHERNET_DM9051
    DM9051.valid = true;
#else
    DM9051.valid = false;
#endif        
    DM9051.init_config = init_config;
    return &DM9051;
}
