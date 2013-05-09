/*
 * (C) Copyright 2012 SAMSUNG Electronics
 * Padmavathi Venna <padma.v@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <malloc.h>
#include <spi.h>
#include <fdtdec.h>
#include <asm/arch/clk.h>
#include <asm/arch/clock.h>
#include <asm/arch/cpu.h>
#include <asm/arch/gpio.h>
#include <asm/arch/pinmux.h>
#include <asm/arch-exynos/spi.h>
#include <asm/io.h>

DECLARE_GLOBAL_DATA_PTR;

/* Information about each SPI controller */
struct spi_bus {
	enum periph_id periph_id;
	s32 frequency;		/* Default clock frequency, -1 for none */
	struct exynos_spi *regs;
	int inited;		/* 1 if this bus is ready for use */
	int node;
	uint deactivate_delay_us;	/* Delay to wait after deactivate */
};

/* A list of spi buses that we know about */
static struct spi_bus spi_bus[EXYNOS5_SPI_NUM_CONTROLLERS];
static unsigned int bus_count;

struct exynos_spi_slave {
	struct spi_slave slave;
	struct exynos_spi *regs;
	unsigned int freq;		/* Default frequency */
	unsigned int mode;
	enum periph_id periph_id;	/* Peripheral ID for this device */
	unsigned int fifo_size;
	struct spi_bus *bus;		/* Pointer to our SPI bus info */
	ulong last_transaction_us;	/* Time of last transaction end */
};

static struct spi_bus *spi_get_bus(unsigned dev_index)
{
	if (dev_index < bus_count)
		return &spi_bus[dev_index];
	debug("%s: invalid bus %d", __func__, dev_index);

	return NULL;
}

static inline struct exynos_spi_slave *to_exynos_spi(struct spi_slave *slave)
{
	return container_of(slave, struct exynos_spi_slave, slave);
}

/**
 * Setup the driver private data
 *
 * @param bus		ID of the bus that the slave is attached to
 * @param cs		ID of the chip select connected to the slave
 * @param max_hz	Required spi frequency
 * @param mode		Required spi mode (clk polarity, clk phase and
 *			master or slave)
 * @return new device or NULL
 */
struct spi_slave *spi_setup_slave(unsigned int busnum, unsigned int cs,
			unsigned int max_hz, unsigned int mode)
{
	struct exynos_spi_slave *spi_slave;
	struct spi_bus *bus;

	if (!spi_cs_is_valid(busnum, cs)) {
		debug("%s: Invalid bus/chip select %d, %d\n", __func__,
		      busnum, cs);
		return NULL;
	}

	spi_slave = spi_alloc_slave(struct exynos_spi_slave, busnum, cs);
	if (!spi_slave) {
		debug("%s: Could not allocate spi_slave\n", __func__);
		return NULL;
	}

	bus = &spi_bus[busnum];
	spi_slave->bus = bus;
	spi_slave->regs = bus->regs;
	spi_slave->mode = mode;
	spi_slave->periph_id = bus->periph_id;
	if (bus->periph_id == PERIPH_ID_SPI1 ||
	    bus->periph_id == PERIPH_ID_SPI2)
		spi_slave->fifo_size = 64;
	else
		spi_slave->fifo_size = 256;

	spi_slave->last_transaction_us = timer_get_us();

	spi_slave->freq = bus->frequency;
	if (max_hz)
		spi_slave->freq = min(max_hz, spi_slave->freq);

	return &spi_slave->slave;
}

/**
 * Setup the driver private data for half duplex slaves
 *
 * @param bus		ID of the bus that the slave is attached to
 * @param cs		ID of the chip select connected to the slave
 * @param max_hz	Required spi frequency
 * @param mode		Required spi mode (clk polarity, clk phase and
 *			master or slave)
 * @param timeout	timeout, in milliseconds, to wait for slave responses
 * @param frame_header  the first byte sent by the slave in the response frame
 * @return new device or NULL
 */
struct spi_slave *spi_setup_half_duplex_slave(unsigned busnum,
					      unsigned cs,
					      unsigned max_hz,
					      unsigned mode,
					      uint16_t timeout,
					      uint8_t frame_header)
{
	struct spi_slave *slave;


	slave = spi_setup_slave(busnum, cs, max_hz, mode);
	if (slave) {
		slave->half_duplex = 1;
		slave->frame_header = frame_header;
		slave->max_timeout_ms = timeout;
	}
	return slave;
}

/**
 * Free spi controller
 *
 * @param slave	Pointer to spi_slave to which controller has to
 *		communicate with
 */
void spi_free_slave(struct spi_slave *slave)
{
	struct exynos_spi_slave *spi_slave = to_exynos_spi(slave);

	free(spi_slave);
}

/**
 * Flush spi tx, rx fifos and reset the SPI controller
 *
 * @param slave	Pointer to spi_slave to which controller has to
 *		communicate with
 */
static void spi_flush_fifo(struct spi_slave *slave)
{
	struct exynos_spi_slave *spi_slave = to_exynos_spi(slave);
	struct exynos_spi *regs = spi_slave->regs;

	clrsetbits_le32(&regs->ch_cfg, SPI_CH_HS_EN, SPI_CH_RST);
	clrbits_le32(&regs->ch_cfg, SPI_CH_RST);
	setbits_le32(&regs->ch_cfg, SPI_TX_CH_ON | SPI_RX_CH_ON);
}

/**
 * Initialize the spi base registers, set the required clock frequency and
 * initialize the gpios
 *
 * @param slave	Pointer to spi_slave to which controller has to
 *		communicate with
 * @return zero on success else a negative value
 */
int spi_claim_bus(struct spi_slave *slave)
{
	struct exynos_spi_slave *spi_slave = to_exynos_spi(slave);
	struct exynos_spi *regs = spi_slave->regs;
	u32 reg = 0;
	int ret;

	ret = set_spi_clk(spi_slave->periph_id,
					spi_slave->freq);
	if (ret < 0) {
		debug("%s: Failed to setup spi clock\n", __func__);
		return ret;
	}

	exynos_pinmux_config(spi_slave->periph_id, PINMUX_FLAG_NONE);

	spi_flush_fifo(slave);

	reg = readl(&regs->ch_cfg);
	reg &= ~(SPI_CH_CPHA_B | SPI_CH_CPOL_L);

	if (spi_slave->mode & SPI_CPHA)
		reg |= SPI_CH_CPHA_B;

	if (spi_slave->mode & SPI_CPOL)
		reg |= SPI_CH_CPOL_L;

	writel(reg, &regs->ch_cfg);
	writel(SPI_FB_DELAY_180, &regs->fb_clk);

	return 0;
}

/**
 * Reset the spi H/W and flush the tx and rx fifos
 *
 * @param slave	Pointer to spi_slave to which controller has to
 *		communicate with
 */
void spi_release_bus(struct spi_slave *slave)
{
	spi_flush_fifo(slave);
}

static void spi_get_fifo_levels(struct exynos_spi *regs,
	int *rx_lvl, int *tx_lvl)
{
	uint32_t spi_sts = readl(&regs->spi_sts);

	*rx_lvl = (spi_sts >> SPI_RX_LVL_OFFSET) & SPI_FIFO_LVL_MASK;
	*tx_lvl = (spi_sts >> SPI_TX_LVL_OFFSET) & SPI_FIFO_LVL_MASK;
}

/**
 * If there's something to transfer, do a software reset and set a
 * transaction size.
 *
 * @param regs	SPI peripheral registers
 * @param count	Number of bytes to transfer
 * @param step	Number of bytes to transfer in each packet (1 or 4)
 */
static void spi_request_bytes(struct exynos_spi *regs, int count, int step)
{
	/* For word address we need to swap bytes */
	if (step == 4) {
		setbits_le32(&regs->mode_cfg,
			     SPI_MODE_CH_WIDTH_WORD | SPI_MODE_BUS_WIDTH_WORD);
		count /= 4;
		setbits_le32(&regs->swap_cfg, SPI_TX_SWAP_EN | SPI_RX_SWAP_EN |
			SPI_TX_BYTE_SWAP | SPI_RX_BYTE_SWAP |
			SPI_TX_HWORD_SWAP | SPI_RX_HWORD_SWAP);
	} else {
		/* Select byte access and clear the swap configuration */
		clrbits_le32(&regs->mode_cfg,
			     SPI_MODE_CH_WIDTH_WORD | SPI_MODE_BUS_WIDTH_WORD);
		writel(0, &regs->swap_cfg);
	}

	assert(count < (1 << 16));
	setbits_le32(&regs->ch_cfg, SPI_CH_RST);
	clrbits_le32(&regs->ch_cfg, SPI_CH_RST);
	if (count)
		writel(count | SPI_PACKET_CNT_EN, &regs->pkt_cnt);
}

static int spi_rx_tx(struct exynos_spi_slave *spi_slave, int todo,
			void **dinp, void const **doutp)
{
	struct exynos_spi *regs = spi_slave->regs;
	uchar *rxp = *dinp;
	const uchar *txp = *doutp;
	int rx_lvl, tx_lvl, step;
	uint out_bytes, in_bytes;
	unsigned start_time = get_timer(0);
	int hunting = spi_slave->slave.half_duplex;

	if (hunting)
		start_time = get_timer(0);

	in_bytes = todo;
	out_bytes = txp ? todo : 0;

	/*
	 * Try to transfer words if we can. This helps read performance at
	 * SPI clock speeds above about 20MHz.
	 */
	if (((todo | (uintptr_t)rxp | (uintptr_t)txp) & 3) ||
	    spi_slave->slave.half_duplex)
		step = 1;
	else
		step = 4;

	/*
	 * Do a software reset and maybe set a transaction size (if not
	 * hunting).
	 */
	spi_request_bytes(regs, hunting ? 0 : todo, step);

	/* Keep transmitting no matter what, as it clocks the bus. */
	while (in_bytes) {
		int temp;

		if (hunting &&
		    (get_timer(start_time) > spi_slave->slave.max_timeout_ms)) {
			debug("%s: Did not get reply in %d ms\n",
			      __func__, spi_slave->slave.max_timeout_ms);
			return -1;
		}

		/* Keep the fifos full/empty. */
		spi_get_fifo_levels(regs, &rx_lvl, &tx_lvl);
		/*
		 * Don't completely fill the txfifo, since we don't want our
		 * rxfifo to overflow, and it may already contain data.
		 */
		while (tx_lvl < (spi_slave->fifo_size / 2)) {
			if (out_bytes) {
				if (step == 4) {
					temp = *((uint32_t *)txp);
					txp += 4;
				} else {
					temp = *txp++;
				}
				out_bytes -= step;
			} else {
				temp = -1;
			}
			writel(temp, &regs->tx_data);
			tx_lvl += step;
		}

		while ((rx_lvl >= step) && in_bytes) {
			temp = readl(&regs->rx_data);
			rx_lvl -= step;
			if (hunting) {
				if ((temp & 0xff) ==
				    spi_slave->slave.frame_header)
					hunting = 0;
				break; /* Restart the outer loop. */
			}
			if (step == 4) {
				*((uint32_t *)rxp) = temp;
				rxp += 4;
			} else {
				*rxp++ = temp;
			}
			in_bytes -= step;
		}
	}
	*dinp = rxp;
	*doutp = txp;
	return 0;
}

/**
 * Transfer and receive data
 *
 * @param slave		Pointer to spi_slave to which controller has to
 *			communicate with
 * @param bitlen	No of bits to tranfer or receive
 * @param dout		Pointer to transfer buffer
 * @param din		Pointer to receive buffer
 * @param flags		Flags for transfer begin and end
 * @return zero on success else a negative value
 */
int spi_xfer(struct spi_slave *slave, unsigned int bitlen, const void *dout,
	     void *din, unsigned long flags)
{
	struct exynos_spi_slave *spi_slave = to_exynos_spi(slave);
	int upto, todo;
	int bytelen;
	int ret = 0;

	/* spi core configured to do 8 bit transfers */
	if (bitlen % 8) {
		debug("Non byte aligned SPI transfer.\n");
		return -1;
	}

	/* Start the transaction, if necessary. */
	if ((flags & SPI_XFER_BEGIN))
		spi_cs_activate(slave);

	/*
	 * Exynos SPI limits each transfer to 65535 bytes. To keep things
	 * simple, allow a maximum of 65532 bytes. We could allow more in word
	 * mode, but the performance difference is small.
	 */
	bytelen =  bitlen / 8;
	for (upto = 0; !ret && (upto < bytelen); upto += todo) {
		todo = min(bytelen - upto, (1 << 16) - 4);
		ret = spi_rx_tx(spi_slave, todo, &din, &dout);
	}

	/* Stop the transaction, if necessary. */
	if (flags & SPI_XFER_END)
		spi_cs_deactivate(slave);

	return ret;
}

/**
 * Validates the bus and chip select numbers
 *
 * @param bus	ID of the bus that the slave is attached to
 * @param cs	ID of the chip select connected to the slave
 * @return one on success else zero
 */
int spi_cs_is_valid(unsigned int bus, unsigned int cs)
{
	return spi_get_bus(bus) && cs == 0;
}

/**
 * Activate the CS by driving it LOW
 *
 * @param slave	Pointer to spi_slave to which controller has to
 *		communicate with
 */
void spi_cs_activate(struct spi_slave *slave)
{
	struct exynos_spi_slave *spi_slave = to_exynos_spi(slave);

	/* If it's too soon to do another transaction, wait */
	if (spi_slave->bus->deactivate_delay_us &&
		spi_slave->last_transaction_us) {
		ulong delay_us;		/* The delay completed so far */
		delay_us = timer_get_us() - spi_slave->last_transaction_us;
		if (delay_us < spi_slave->bus->deactivate_delay_us)
			udelay(spi_slave->bus->deactivate_delay_us - delay_us);
	}
	clrbits_le32(&spi_slave->regs->cs_reg, SPI_SLAVE_SIG_INACT);
	debug("Activate CS, bus %d\n", spi_slave->slave.bus);

	/* Remember time of this transaction so we can honour the bus delay */
	if (spi_slave->bus->deactivate_delay_us)
		spi_slave->last_transaction_us = timer_get_us();
}

/**
 * Deactivate the CS by driving it HIGH
 *
 * @param slave	Pointer to spi_slave to which controller has to
 *		communicate with
 */
void spi_cs_deactivate(struct spi_slave *slave)
{
	struct exynos_spi_slave *spi_slave = to_exynos_spi(slave);

	setbits_le32(&spi_slave->regs->cs_reg, SPI_SLAVE_SIG_INACT);
	debug("Deactivate CS, bus %d\n", spi_slave->slave.bus);
}

static inline struct exynos_spi *get_spi_base(int dev_index)
{
	if (dev_index < 3)
		return (struct exynos_spi *)samsung_get_base_spi() + dev_index;
	else
		return (struct exynos_spi *)samsung_get_base_spi_isp() +
					(dev_index - 3);
}

#ifdef CONFIG_OF_CONTROL
/*
 * Read the SPI config from the device tree node.
 *
 * @param blob  FDT blob to read from
 * @param node  Node offset to read from
 * @param bus   SPI bus structure to fill with information
 * @return 0 if ok, or -FDT_ERR_NOTFOUND if something was missing
 */
static int spi_get_config(const void *blob, int node, struct spi_bus *bus)
{
	bus->node = node;
	bus->regs = (struct exynos_spi *)fdtdec_get_addr(blob, node, "reg");
	bus->periph_id = pinmux_decode_periph_id(blob, node);

	if (bus->periph_id == PERIPH_ID_NONE) {
		debug("%s: Invalid peripheral ID %d\n", __func__,
			bus->periph_id);
		return -FDT_ERR_NOTFOUND;
	}

	/* Use 500KHz as a suitable default */
	bus->frequency = fdtdec_get_int(blob, node, "spi-max-frequency",
					500000);
	bus->deactivate_delay_us = fdtdec_get_int(blob, node,
					"spi-deactivate-delay", 0);

	return 0;
}

/*
 * Process a list of nodes, adding them to our list of SPI ports.
 *
 * @param blob          fdt blob
 * @param node_list     list of nodes to process (any <=0 are ignored)
 * @param count         number of nodes to process
 * @param is_dvc        1 if these are DVC ports, 0 if standard I2C
 * @return 0 if ok, -1 on error
 */
static int process_nodes(const void *blob, int node_list[], int count)
{
	int i;

	/* build the i2c_controllers[] for each controller */
	for (i = 0; i < count; i++) {
		int node = node_list[i];
		struct spi_bus *bus;

		if (node <= 0)
			continue;

		bus = &spi_bus[i];
		if (spi_get_config(blob, node, bus)) {
			printf("exynos spi_init: failed to decode bus %d\n",
				i);
			return -1;
		}

		debug("spi: controller bus %d at %p, periph_id %d\n",
		      i, bus->regs, bus->periph_id);
		bus->inited = 1;
		bus_count++;
	}

	return 0;
}

/**
 * Set up a new SPI slave for an fdt node
 *
 * @param blob		Device tree blob
 * @param slave_node	pointer to this SPI slave node in the device tree
 * @param spi_node	cached pointer to the SPI interface this node belongs to
 * @return 0 if ok, -1 on error
 */
struct spi_slave *spi_setup_slave_fdt(const void *blob,
				      int slave_node,
				      int spi_node)
{
	struct spi_bus *bus;
	unsigned int i;

	for (i = 0, bus = spi_bus; i < bus_count; i++, bus++) {
		uint32_t max_freq;
		unsigned cs;
		unsigned mode;

		if (bus->node != spi_node)
			continue;

		/* Decode slave-specific params, providing sendible defaults */
		max_freq = fdtdec_get_int(blob, slave_node,
					  "spi-max-frequency", 500000);
		mode = fdtdec_get_int(blob, slave_node, "spi-mode", 0);
		cs = fdtdec_get_int(blob, slave_node, "reg", 0);
		if (fdtdec_get_bool(blob, slave_node, "spi-half-duplex")) {
			unsigned timeout;
			unsigned frame_header;

			timeout = fdtdec_get_int(blob,
						 slave_node,
						 "spi-max-timeout-ms", 1000);
			frame_header = fdtdec_get_int(blob, slave_node,
						      "spi-frame-header",
						      0x100);
			if (frame_header >= 0x100) {
				debug("%s: frame header not defined "
				      "or invalid!\n", __func__);
				return NULL;
			}
			return spi_setup_half_duplex_slave(i, cs, max_freq,
							   mode, timeout,
							   frame_header);
		}
		return spi_setup_slave(i, cs, max_freq, mode);
	}

	debug("%s: Failed to find bus node %d\n", __func__, spi_node);
	return NULL;
}
#endif /* ^^^^^^^ CONFIG_OF_CONTROL defined */

/* Sadly there is no error return from this function */
void spi_init(void)
{
	int count;

#ifdef CONFIG_OF_CONTROL
	int node_list[EXYNOS5_SPI_NUM_CONTROLLERS];
	const void *blob = gd->fdt_blob;

	count = fdtdec_find_aliases_for_id(blob, "spi",
			COMPAT_SAMSUNG_EXYNOS_SPI, node_list,
			EXYNOS5_SPI_NUM_CONTROLLERS);
	if (process_nodes(blob, node_list, count))
		return;

#else
	struct spi_bus *bus;

	for (count = 0; count < EXYNOS5_SPI_NUM_CONTROLLERS; count++) {
		bus = &spi_bus[count];
		bus->regs = get_spi_base(count);
		bus->periph_id = PERIPH_ID_SPI0 + count;

		/* Although Exynos5 supports upto 50Mhz speed,
		 * we are setting it to 10Mhz for safe side
		 */
		bus->frequency = 10000000;
		bus->inited = 1;
		bus->node = 0;
		bus_count = EXYNOS5_SPI_NUM_CONTROLLERS;
	}
#endif
}
