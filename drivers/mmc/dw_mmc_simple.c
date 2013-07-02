#include <common.h>
#include <errno.h>
#include <mmc.h>
#include <dwmmc.h>
#include <dwmmc_simple.h>
#include <asm/arch/clk.h>
#include <asm/arch/dwmmc.h>

int dwmci_simple_init(struct dwmci_host *host)
{
	u32 div;
	unsigned long sclk;
	unsigned long start;
	int timeout = 1000;

	host->dev_index = DWMMC_SIMPLE_INDEX;
	host->ioaddr = (void *)(samsung_get_base_mmc()
			+ (host->dev_index << 16));

	dwmci_writel(host, DWMCI_PWREN, 1);

	/* Reset all */
	dwmci_writel(host, DWMCI_CTRL, DWMCI_RESET_ALL);

	/* Wait for reset to complete */
	start = get_timer(0);
	while (1) {
		u32 ctrl = dwmci_readl(host, DWMCI_CTRL);
		if (!(ctrl & DWMCI_RESET_ALL))
			break;

		if (get_timer(start) > timeout)
			return -ETIMEDOUT;
	}

	dwmci_writel(host, DWMCI_CLKENA, 0);
	dwmci_writel(host, DWMCI_CLKSRC, 0);

	sclk = get_mmc_clk(host->dev_index);

	/* Clock divisor is 2*n */
	div = DIV_ROUND_UP(sclk, 2 * DWMMC_SIMPLE_FREQ);

	dwmci_writel(host, DWMCI_CLKDIV, div);

	dwmci_writel(host, DWMCI_CMD, DWMCI_CMD_PRV_DAT_WAIT |
			DWMCI_CMD_UPD_CLK | DWMCI_CMD_START);

	start = get_timer(0);
	while (dwmci_readl(host, DWMCI_CMD) & DWMCI_CMD_START)
		if (get_timer(start) > timeout)
			return -ETIMEDOUT;

	dwmci_writel(host, DWMCI_CLKENA, DWMCI_CLKEN_ENABLE |
			DWMCI_CLKEN_LOW_PWR);

	dwmci_writel(host, DWMCI_CMD, DWMCI_CMD_PRV_DAT_WAIT |
			DWMCI_CMD_UPD_CLK | DWMCI_CMD_START);

	start = get_timer(0);
	while (dwmci_readl(host, DWMCI_CMD) & DWMCI_CMD_START)
		if (get_timer(start) > timeout)
			return -ETIMEDOUT;

	dwmci_writel(host, DWMCI_TMOUT, 0xFFFFFFFF);

	return 0;
}

int dwmci_simple_send_cmd(struct dwmci_host *host, struct mmc_cmd *cmd)
{
	int flags = 0;
	u32 mask = 0;
	int timeout = 1000;
	unsigned long start;

	start = get_timer(0);
	while (dwmci_readl(host, DWMCI_STATUS) & DWMCI_BUSY)
		if (get_timer(start) > timeout)
			return -ETIMEDOUT;

	dwmci_writel(host, DWMCI_CMDARG, cmd->cmdarg);

	flags |= DWMCI_CMD_PRV_DAT_WAIT;

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		flags |= DWMCI_CMD_RESP_EXP;
		if (cmd->resp_type & MMC_RSP_136)
			flags |= DWMCI_CMD_RESP_LENGTH;
	}

	if (cmd->resp_type & MMC_RSP_CRC)
		flags |= DWMCI_CMD_CHECK_CRC;

	flags |= (cmd->cmdidx | DWMCI_CMD_START | DWMCI_CMD_USE_HOLD_REG);

	dwmci_writel(host, DWMCI_CMD, flags);

	start = get_timer(0);
	while (1) {
		mask = dwmci_readl(host, DWMCI_RINTSTS);
		if (mask & DWMCI_INTMSK_CDONE) {
			dwmci_writel(host, DWMCI_RINTSTS, mask);
			break;
		}

		if (get_timer(start) > timeout)
			return -ETIMEDOUT;
	}

	if (mask & DWMCI_INTMSK_RTO)
		return -ETIMEDOUT;
	else if (mask & DWMCI_INTMSK_RE)
		return -EIO;

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		if (cmd->resp_type & MMC_RSP_136) {
			cmd->response[0] = dwmci_readl(host, DWMCI_RESP3);
			cmd->response[1] = dwmci_readl(host, DWMCI_RESP2);
			cmd->response[2] = dwmci_readl(host, DWMCI_RESP1);
			cmd->response[3] = dwmci_readl(host, DWMCI_RESP0);
		} else {
			cmd->response[0] = dwmci_readl(host, DWMCI_RESP0);
		}
	}

	return 0;
}

int dwmci_simple_startup(struct dwmci_host *host)
{
	struct mmc_cmd cmd;
	u32 mmc_ocr;
	int ret;
	int timeout = 1000;
	unsigned long start;

	/* Ask card its capabilities
	 * Keep trying until mmc is responding after power-on */
	start = get_timer(0);
	do {
		cmd.cmdidx = MMC_CMD_SEND_OP_COND;
		cmd.resp_type = MMC_RSP_R3;
		cmd.cmdarg = 0;
		ret = dwmci_simple_send_cmd(host, &cmd);

		if (get_timer(start) > timeout)
			return -ETIMEDOUT;
	} while (ret == -ETIMEDOUT);
	if (ret)
		return ret;

	mmc_ocr = cmd.response[0];

	/* Verfiy which card voltages we are compatible with,
	 * wait until card is no longer busy. */
	start = get_timer(0);
	do {
		cmd.cmdidx = MMC_CMD_SEND_OP_COND;
		cmd.resp_type = MMC_RSP_R3;
		cmd.cmdarg =
			(DWMMC_SIMPLE_VOLTAGES &
			(mmc_ocr & OCR_VOLTAGE_MASK)) |
			(mmc_ocr & OCR_ACCESS_MODE) |
			OCR_HCS;

		ret = dwmci_simple_send_cmd(host, &cmd);
		if (ret)
			return ret;

		if (get_timer(start) > timeout)
			return -ETIMEDOUT;
		else
			udelay(100);
	} while (!(cmd.response[0] & OCR_BUSY));

	/* Put the card in Identify Mode */
	cmd.cmdidx = MMC_CMD_ALL_SEND_CID;
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = 0;
	ret = dwmci_simple_send_cmd(host, &cmd);
	if (ret)
		return ret;

	/*
	 * For MMC cards, set the Relative Address.
	 * This also puts the cards into Standby State
	 */
	cmd.cmdidx = MMC_CMD_SET_RELATIVE_ADDR;
	cmd.cmdarg = DWMMC_SIMPLE_RCA << 16;
	cmd.resp_type = MMC_RSP_R1;
	ret = dwmci_simple_send_cmd(host, &cmd);
	if (ret)
		return ret;

	/* Select the card, and put it into Transfer Mode */
	cmd.cmdidx = MMC_CMD_SELECT_CARD;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = DWMMC_SIMPLE_RCA << 16;
	return dwmci_simple_send_cmd(host, &cmd);
}
