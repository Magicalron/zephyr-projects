#include <zephyr.h>
#include <sys/printk.h>

#include <drivers/dma.h>

#define LOG_LEVEL CONFIG_DMA_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(main);

#define ARRAYSIZE 800

volatile uint32_t status = 0;
volatile uint32_t i;

void dma_callback_complete(struct device *dev, void *user_data, uint32_t channel, int status)
{
	printk("DMA Callback Channel %d Complete with status %d\n", channel, status);
	status = 1;
}

int main(void)
{
	uint32_t source[ARRAYSIZE];
	uint32_t dest[ARRAYSIZE];

	uint32_t user_data[32];

	struct device *dev = device_get_binding("DMA_0");
	struct dma_block_config blk_cfg = {
		.source_address = (uint32_t)source,
		.dest_address = (uint32_t)dest,
		.source_addr_adj = 0,						//00 = Increment, 01 = Decrement, 10 = no change
		.dest_addr_adj = 0							//CRC requires mem = 10, per = 00 (this example incs both)
	};

	//DMA Configuration
	struct dma_config cfg = {
		.dma_slot = 1, 								//DMA Channel 1 (8 available)
		.channel_direction = 0b00, 					//set as M2M (0b00=M2M: S=M, D=P), (0b01=M2P : S=P, D=M), (0b10=P2M : S=M, D=P)
		.complete_callback_en = 0,					//Complete function entirely first
		.channel_priority = 1,						//Medium priority 0 1 2 or 3
		.source_data_size = 32,						//32 bits
		.dest_data_size = 32,
		.source_burst_length = 1,					//M2M Single for CRC
		.dest_burst_length = 1,
		.head_block = &blk_cfg,						//Further configurations as above
		.user_data = &user_data,
		.dma_callback = &dma_callback_complete		//Callback called upon completion
	};


	for (i=0; i<ARRAYSIZE; i++) {
		source[i] = i;
	}

	//DMA Initialisation
	int ret = dma_config(dev, 0x0001, &cfg);
	if(ret!=0) {
		LOG_ERR("Failed to Configure DMA\n");
		return ret;
	}

	ret = dma_start(dev, 1);
	if(ret!=0){
		LOG_ERR("Failed to Start DMA\n");
		return ret;
	}
	while(status==0) {
		for(i=0; i<ARRAYSIZE; i++){
			dest[i] = source[i];
		}
	}
	ret = dma_stop(dev, 1);
	if(ret!=0){
		LOG_ERR("Failed to Configure DMA\n");
		return ret;
	}
	return 0;
}
