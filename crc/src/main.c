#include <zephyr.h>
#include <crc.h>
#include <sys/printk.h>

#include <drivers/dma.h>

#define CRC_DEV_NAME DT_LABEL( DT_INST( 0, st_stm32_crc) )

#define LOG_LEVEL CONFIG_CRC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(main);


uint8_t buf[6] = {0x00, 0x01, 0x02, 0x03, 0x04};

void dma_callback_complete(struct device *dev, void *user_data, uint32_t channel, int status)
{
	printk("DMA Callback Channel %d Complete with status %d\n", channel, status);
}

int main(void)
{
	//1.) Enable DMA Clock
	struct device *dmadev = device_get_binding("DMA_0");
	uint32_t answer;

	if(dmadev==NULL) {
		LOG_ERR("No DMA device found");
	}

	//2.) Configure DMA
	struct dma_block_config dma_dev_block_configs = {
		.source_address = 0,
		.dest_address = 0,
		.source_addr_adj = 0b10, 	//Increment address = No change
		.dest_addr_adj = 0b00,			//Increment address = Enable
		.source_reload_en = 0, //Reload source addr at end of block transfer
		.dest_reload_en = 0,
		.block_size = 4 //4 bytes = 32 bits
	};

	struct dma_block_config *pdma_dev_block_configs = &dma_dev_block_configs;

	struct dma_config dma_dev_configs = {
		.dma_slot = 0, 				//This is set to 0 if greater than 8 (Channel 0)
		.channel_direction = 0b000, //Memory to Memory
		.channel_priority = 0,		//Low
		.complete_callback_en = 0,	//Call callback when complete only, not when finish block
		.source_data_size = 32,
		.dest_data_size = 32,
		.source_burst_length = 1,	//Single burst for CRC m2m
		.dest_burst_length = 1,
		.head_block = pdma_dev_block_configs,
		.dma_callback = dma_callback_complete
	};

	struct dma_config *pdma_dev_configs = &dma_dev_configs;

	int ret = dma_config(dmadev, 0, pdma_dev_configs);
	if(ret != 0) {
		LOG_INF("DMA config failed");
	}

	//3.) Configure CRC
	//a) Enable CRC Clock
	struct device *crcdev = device_get_binding(CRC_DEV_NAME);
	if(crcdev==NULL) {
		LOG_ERR("No CRC device found");
	}

	//b) Set INIT value
	set_init_value(crcdev, 0x00);

	//c) Set Inversion
	config_in(crcdev, 0);
	config_out(crcdev, 0);

	//d) Set Polynomial
	set_polynomial(crcdev, 8, 0x07);

	//e) RESTET CRC Peripheral (Done in CRC Calculate)

	//4.) Enable the DMA Transfer Complete Interupt
	answer = calculate_crc(crcdev, (uint32_t *)&buf, 5);
	printk("Check = %x\n", answer);
	buf[5] = answer;
	printk("Check Result: %s", calculate_crc(crcdev, (uint32_t *)&buf, 6) ? "Valid" : "Invalid");
	return 0;
}
