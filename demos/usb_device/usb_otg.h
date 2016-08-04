/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2016 Chuck McManis <cmcmanis@mcmanis.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This file is intended to be included by stm32/usb_otg.h
 */

#pragma once

#define USB_FS			USB_OTG_FS_BASE
#define USB_HS			USB_OTG_HS_BASE

/***********************************************************************/
/* Core Global Control and Status Registers */
#define OTG_GOTGCTL(usb)	MMIO32((usb) + 0x000)
#define OTG_GOTGINT(usb)	MMIO32((usb) + 0x004)
#define OTG_GAHBCFG(usb)	MMIO32((usb) + 0x008)
#define OTG_GUSBCFG(usb)	MMIO32((usb) + 0x00C)
#define OTG_GRSTCTL(usb)	MMIO32((usb) + 0x010)
#define OTG_GINTSTS(usb)	MMIO32((usb) + 0x014)
#define OTG_GINTMSK(usb)	MMIO32((usb) + 0x018)
#define OTG_GRXSTSR(usb)	MMIO32((usb) + 0x01C)
#define OTG_GRXSTSP(usb)	MMIO32((usb) + 0x020)
#define OTG_GRXFSIZ(usb)	MMIO32((usb) + 0x024)
/* host mode - non-periodic xmit FIFO size */
#define OTG_HNPTXFSIZ(usb)	MMIO32((usb) + 0x028)
/* device mode - EPTX F0 */
#define OTG_DIEPTXF0(usb)	MMIO32((usb) + 0x028)
#define OTG_GNPTXSTS(usb)	MMIO32((usb) + 0x02C)
#define OTG_GCCFG(usb)		MMIO32((usb) + 0x038)
#define OTG_CID(usb)		MMIO32((usb) + 0x03C)
#define OTG_GLPMCFG(usb)	MMIO32((usb) + 0x054)
#define OTG_HPTXFSIZ(usb)	MMIO32((usb) + 0x100)
#define OTG_DIEPTXF(usb, x)	MMIO32((usb) + (0x104 + 4*((x)-1)))

/* Host-mode Control and Status Registers */
#define OTG_HCFG(usb)		MMIO32((usb) + 0x400)
#define OTG_HFIR(usb)		MMIO32((usb) + 0x404)
#define OTG_HFNUM(usb)		MMIO32((usb) + 0x408)
#define OTG_HPTXSTS(usb)	MMIO32((usb) + 0x410)
#define OTG_HAINT(usb)		MMIO32((usb) + 0x414)
#define OTG_HAINTMSK(usb)	MMIO32((usb) + 0x418)
#define OTG_HPRT(usb)		MMIO32((usb) + 0x440)
/*
 * Each host channel has :
 *  +0x00 Characteristics
 *  +0x04 Split control
 *  +0x08 Interrupt
 *  +0x0c Interrupt Mask
 *  +0x10 Transfer Size
 *  +0x14 DMA Address (HS only)
 *
 * 15 channels in HS, 11 channels in FS
 */
#define OTG_HCCHAR(usb, x)	MMIO32((usb) + (0x500 + 0x20 * (x)))
#define OTG_HCSPLT(usb, x)	MMIO32((usb) + (0x504 + 0x20 * (x)))
#define OTG_HCINT(usb, x)	MMIO32((usb) + (0x508 + 0x20 * (x)))
#define OTG_HCINTMSK(usb, x)	MMIO32((usb) + (0x50C + 0x20 * (x)))
#define OTG_HCTSIZ(usb, x)	MMIO32((usb) + (0x510 + 0x20 * (x)))
#define OTG_HCDMA(usb, x)	MMIO32((usb) + (0x514 + 0x20 * (x)))


/* Device-mode Control and Status Registers */
#define OTG_DCFG(usb)		MMIO32((usb) + 0x800)
#define OTG_DCTL(usb)		MMIO32((usb) + 0x804)
#define OTG_DSTS(usb)		MMIO32((usb) + 0x808)
#define OTG_DIEPMSK(usb)	MMIO32((usb) + 0x810)
#define OTG_DOEPMSK(usb)	MMIO32((usb) + 0x814)
#define OTG_DAINT(usb)		MMIO32((usb) + 0x818)
#define OTG_DAINTMSK(usb)	MMIO32((usb) + 0x81C)
#define OTG_DVBUSDIS(usb)	MMIO32((usb) + 0x828)
#define OTG_DVBUSPULSE(usb)	MMIO32((usb) + 0x82C)
#define OTG_DTHRCTL(usb)	MMIO32((usb) + 0x830)
#define OTG_DIEPEMPMSK(usb)	MMIO32((usb) + 0x834)
#define OTG_DEACHINT(usb)	MMIO32((usb) + 0x838)
#define OTG_DEACHINTMSK(usb)	MMIO32((usb) + 0x83c)

/* short hand for endpoint 0 values */
#define OTG_DIEPCTL0(usb)	MMIO32((usb) + 0x900)
#define OTG_DIEPTSIZ0(usb)	MMIO32((usb) + 0x910)
#define OTG_DOEPTSIZ0(usb)	MMIO32((usb) + 0xB10)
/*
 * IN Endpoints
 *  +00 - Control register
 *  +08 - Interrupt register
 *  +10 - Transfer size
 *  +18 - xmit FIFO status
 * OUT Endpoints
 *  +00 - Control register
 *  +08 - Interrupt register
 *  +10	- Transfer size
 *
 * 5 endpoints FS, 7 endpoint HS. Endpoint 0 is always CONTROL
 */
#define OTG_DIEPCTL(usb, x)	MMIO32((usb) + (0x900 + 0x20 * (x)))
#define OTG_DIEPINT(usb, x)	MMIO32((usb) + (0x908 + 0x20 * (x)))
#define OTG_DIEPTSIZ(usb, x)	MMIO32((usb) + (0x910 + 0x20 * (x)))
#define OTG_DTXFSTS(usb, x)	MMIO32((usb) + (0x918 + 0x20 * (x)))
#define OTG_DOEPCTL0(usb)	MMIO32((usb) + 0xB00)
#define OTG_DOEPCTL(usb, x)	MMIO32((usb) + (0xB00 + 0x20 * (x)))
#define OTG_DOEPINT(usb, x)	MMIO32((usb) + (0xB08 + 0x20 * (x)))
#define OTG_DOEPTSIZ(usb, x)	MMIO32((usb) + (0xB10 + 0x20 * (x)))

/* Power and clock gating control and status register */
#define OTG_PCGCCTL(usb)	MMIO32((usb) + 0xE00)

/*
 * Get/Set macros
 *
 * These macros might look complicated but they avoid simple typos
 * by automating the creation of _GET and _SET macros for multibit
 * fields in a constant, the _SET version takes the parameter passed
 * and aligns it to the register bit define, the _GET version takes
 * register value and extracts the bitfield to the least significant
 * bits.
 *
 * For each bitfield you define OTG_<register_name>_<field_name>_SHIFT
 * which is the number of the LSB of the bitfield in the register. And
 * OTG_<register_name>_<field_name>_MASK which masks off only the number
 * of bits allowed in the field. Then you can create a "set" macro using
 * #define OTG_REGISTER_FIELD_SET(x) OTG_SET(REGISTER, FIELD, x)
 *
 */

#define OTG_SET(reg, field, x) \
    (((x) & OTG_ ## reg ##_## field ##_MASK) << OTG_## reg ##_## field ##_SHIFT)

#define OTG_GET(reg, field, x) \
    (((x) >> OTG_ ## reg ##_## field ##_SHIFT) & OTG_## reg ##_## field ##_MASK)


/* Global CSRs */

/* OTG USB control registers (OTG_GOTGCTL) */
#define OTG_GOTGCTL_VER			(1 << 20)
#define OTG_GOTGCTL_BSVLD		(1 << 19)
#define OTG_GOTGCTL_ASVLD		(1 << 18)
#define OTG_GOTGCTL_DBCT		(1 << 17)
#define OTG_GOTGCTL_CIDSTS		(1 << 16)
#define OTG_GOTGCTL_EHEN		(1 << 12)
#define OTG_GOTGCTL_DHNPEN		(1 << 11)
#define OTG_GOTGCTL_HSHNPEN		(1 << 10)
#define OTG_GOTGCTL_HNPRQ		(1 << 9)
#define OTG_GOTGCTL_HNGSCS		(1 << 8)
#define OTG_GOTGCTL_BVALOVAL		(1 << 7)
#define OTG_GOTGCTL_BVALOEN		(1 << 6)
#define OTG_GOTGCTL_AVALOVAL		(1 << 5)
#define OTG_GOTGCTL_AVALOEN		(1 << 4)
#define OTG_GOTGCTL_VBVALOVAL		(1 << 3)
#define OTG_GOTGCTL_VBVALOEN		(1 << 2)
#define OTG_GOTGCTL_SRQ			(1 << 1)
#define OTG_GOTGCTL_SRQSCS		(1 << 0)

/* OTG Interrupts (OTG_GOTGINT) */
#define OTG_GOTGINT_IDCHNG		(1 << 20)
#define OTG_GOTGINT_DBCDNE		(1 << 19)
#define OTG_GOTGINT_ADTOCHG		(1 << 18)
#define OTG_GOTGINT_HNGDET		(1 << 17)
#define OTG_GOTGINT_HNSSCHG		(1 << 9)
#define OTG_GOTGINT_SRSSCHG		(1 << 8)
#define OTG_GOTGINT_SEDET		(1 << 2)

/* OTG AHB configuration register (OTG_GAHBCFG) */
#define OTG_GAHBCFG_PTXFELVL		(1 << 8)
#define OTG_GAHBCFG_TXFELVL		(1 << 7)
#define OTG_GAHBCFG_DMAEN		(1 << 5)
#define OTG_GAHBCFG_HBSTLEN_SHIFT	1
#define OTG_GAHBCFG_HBSTLEN_MASK	0xf
#define OTG_GAHBCFG_HBSTLEN_SET(x)	OTG_SET(GAHBCFG, HBSTLEN, x)
#define OTG_GAHBCFG_HBSTLEN_GET(x)	OTG_GET(GAHBCFG, HBSTLEN, x)
#define OTG_GAHBCFG_GINTMSK		(1 << 0)

/* OTG USB configuration register (OTG_GUSBCFG) */
#define OTG_GUSBCFG_FDMOD		(1 << 30)
#define OTG_GUSBCFG_FHMOD		(1 << 29)
#define OTG_GUSBCFG_ULPIIPD		(1 << 25)
#define OTG_GUSBCFG_PTCI		(1 << 24)
#define OTG_GUSBCFG_PCCI		(1 << 23)
#define OTG_GUSBCFG_TSDPS		(1 << 22)
#define OTG_GUSBCFG_ULPIEVBUSI		(1 << 21)
#define OTG_GUSBCFG_ULPIEVBUSD		(1 << 20)
#define OTG_GUSBCFG_ULPICSM		(1 << 19)
#define OTG_GUSBCFG_ULPIAR		(1 << 18)
#define OTG_GUSBCFG_ULPIFSLS		(1 << 17)
#define OTG_GUSBCFG_PHYLPCS		(1 << 15)
#define OTG_GUSBCFG_TRDT_SHIFT		10
#define OTG_GUSBCFG_TRDT_MASK		0xf
#define OTG_GUSBCFG_TRDT_GET(x)		OTG_GET(GUSBCFG, TRDT, x)
#define OTG_GUSBCFG_TRDT_SET(x)		OTG_SET(GUSBCFG, TRDT, x)
#define OTG_GUSBCFG_HNPCAP		(1 << 9)
#define OTG_GUSBCFG_SRPCAP		(1 << 8)
#define OTG_GUSBCFG_PHYSEL		(1 << 6)
#define OTG_GUSBCFG_TOCAL_SHIFT		0
#define OTG_GUSBCFG_TOCAL_MASK		0x7
#define OTG_GUSBCFG_TOCAL_GET(x)	OTG_GET(GUSBCFG, TOCAL, x)
#define OTG_GUSBCFG_TOCAL_SET(x)	OTG_SET(GUSBCFG, TOCAL, x)

/* OTG reset register (OTG_GRSTCTL) */
#define OTG_GRSTCTL_AHBIDL		(1 << 31)
#define OTG_GRSTCTL_DMAREQ		(1 << 30)
/* Bits 29:11 - Reserved */
#define OTG_GRSTCTL_TXFNUM_MASK		0x1f
#define OTG_GRSTCTL_TXFNUM_SHIFT	6
#define OTG_GRSTCTL_TXFNUM_GET(x)	OTG_GET(GRSTCTL, TXFNUM, x)
#define OTG_GRSTCTL_TXFNUM_SET(x)	OTG_SET(GRSTCTL, TXFNUM, x)
#define OTG_GRSTCTL_TXFFLSH		(1 << 5)
#define OTG_GRSTCTL_RXFFLSH		(1 << 4)
/* Bit 3 - Reserved */
#define OTG_GRSTCTL_FCRST		(1 << 2)
#define OTG_GRSTCTL_HSRST		(1 << 1)
#define OTG_GRSTCTL_CSRST		(1 << 0)

/* OTG interrupt status register (OTG_GINTSTS) */
#define OTG_GINTSTS_WKUPINT		(1 << 31)
#define OTG_GINTSTS_SRQINT		(1 << 30)
#define OTG_GINTSTS_DISCINT		(1 << 29)
#define OTG_GINTSTS_CIDSCHG		(1 << 28)
#define OTG_GINTSTS_LPMINT		(1 << 27)
#define OTG_GINTSTS_PTXFE		(1 << 26)
#define OTG_GINTSTS_HCINT		(1 << 25)
#define OTG_GINTSTS_HPRTINT		(1 << 24)
#define OTG_GINTSTS_RSTDET		(1 << 23)
#define OTG_GINTSTS_DATAFSUSP		(1 << 22)
#define OTG_GINTSTS_IPXFR		(1 << 21)
#define OTG_GINTSTS_INCOMPISOOUT	(1 << 21)
#define OTG_GINTSTS_IISOIXFR		(1 << 20)
#define OTG_GINTSTS_OEPINT		(1 << 19)
#define OTG_GINTSTS_IEPINT		(1 << 18)
/* Bits 17:16 - Reserved */
#define OTG_GINTSTS_EOPF		(1 << 15)
#define OTG_GINTSTS_ISOODRP		(1 << 14)
#define OTG_GINTSTS_ENUMDNE		(1 << 13)
#define OTG_GINTSTS_USBRST		(1 << 12)
#define OTG_GINTSTS_USBSUSP		(1 << 11)
#define OTG_GINTSTS_ESUSP		(1 << 10)
/* Bits 9:8 - Reserved */
#define OTG_GINTSTS_GONAKEFF		(1 << 7)
#define OTG_GINTSTS_GINAKEFF		(1 << 6)
#define OTG_GINTSTS_NPTXFE		(1 << 5)
#define OTG_GINTSTS_RXFLVL		(1 << 4)
#define OTG_GINTSTS_SOF			(1 << 3)
#define OTG_GINTSTS_OTGINT		(1 << 2)
#define OTG_GINTSTS_MMIS		(1 << 1)
#define OTG_GINTSTS_CMOD		(1 << 0)

/* OTG interrupt mask register (OTG_GINTMSK) */
#define OTG_GINTMSK_WUIM		(1 << 31)
#define OTG_GINTMSK_SRQIM		(1 << 30)
#define OTG_GINTMSK_DISCINT		(1 << 29)
#define OTG_GINTMSK_CIDSCHGM		(1 << 28)
#define OTG_GINTMSK_LPMINTM		(1 << 27)
#define OTG_GINTMSK_PTXFEM		(1 << 26)
#define OTG_GINTMSK_HCIM		(1 << 25)
#define OTG_GINTMSK_PRTIM		(1 << 24)
#define OTG_GINTMSK_RSTDETM		(1 << 23)
#define OTG_GINTMSK_FSUSPM		(1 << 22)
/* definition of bit 21 varies HS/FS */
#define OTG_GINTMSK_IPXFRM		(1 << 21)
#define OTG_GINTMSK_IISOOXFRM		(1 << 21)
#define OTG_GINTMSK_IISOIXFRM		(1 << 20)
#define OTG_GINTMSK_OEPINT		(1 << 19)
#define OTG_GINTMSK_IEPINT		(1 << 18)
/* 17:16 reserved */
#define OTG_GINTMSK_EOPFM		(1 << 15)
#define OTG_GINTMSK_ISOODRPM		(1 << 14)
#define OTG_GINTMSK_ENUMDENEM		(1 << 13)
#define OTG_GINTMSK_USBRST		(1 << 12)
#define OTG_GINTMSK_USBSUSPM		(1 << 11)
#define OTG_GINTMSK_ESUSPM		(1 << 10)
/* 9:8 reserved */
#define OTG_GINTMSK_GONAKEFFM		(1 << 7)
#define OTG_GINTMSK_GINAKEFFM		(1 << 6)
#define OTG_GINTMSK_NPTXFEM		(1 << 5)
#define OTG_GINTMSK_RXFLVLM		(1 << 4)
#define OTG_GINTMSK_SOFM		(1 << 3)
#define OTG_GINTMSK_OTGINT		(1 << 2)
#define OTG_GINTMSK_MMISM		(1 << 1)
/* 0 reserved */

/* OTG Receive Status Pop Register
 * Pop =  OTG_GRXSTP
 * Read = OTG_GRXSTR
 * Flags are of the form OTG_GRXSTx */
/* Bits 31:25 - Reserved */
#define OTG_GRXSTx_PKTSTS_MASK		0xf
#define OTG_GRXSTx_PKTSTS_SHIFT		17
#define OTG_GRXSTx_DPID_MASK		0x3
#define OTG_GRXSTx_DPID_SHIFT		15
#define OTG_GRXSTx_BCNT_MASK		0x7ff
#define OTG_GRXSTx_BCNT_SHIFT		4
#define OTG_GRXSTx_CHNUM_MASK		0xf
#define OTG_GRXSTx_CHNUM_SHIFT		0

/* read macros to pull these bits out of the status reg */
#define OTG_GRXSTx_PKTSTS_GET(x)	OTG_GET(GRXSTx, PKTSTS, x)
#define OTG_GRXSTx_BCNT_GET(x)		OTG_GET(GRXSTx, BCNT, x)
#define OTG_GRXSTx_DPID(x)		OTG_GET(GRXSTx, DPID, x)
#define OTG_GRXSTx_CHNUM(x)		OTG_GET(GRXSTx, CHNUM, x)

/* OTG_GRXFSIZ */
#define OTG_GRXFSIZ_RXFD_MASK		0xffff
#define OTG_GRXFSIZ_RXFD_SHIFT		0
#define OTG_GRXFSIZ_RXFD_GET(x)		OTG_GET(GRXFSIZ, RXFD, x)
#define OTG_GRXFSIZ_RXFD_SET(x)		OTG_SET(GRXFSIZ, RXFD, x)

/* OTG_HNPTXFSIZE - Host mode */
#define OTG_HNPTXFSIZ_NPTXFD_MASK	0xffff
#define OTG_HNPTXFSIZ_NPTXFD_SHIFT	16
#define OTG_HNPTXFSIZ_NPTXFSA_MASK	0xffff
#define OTG_HNPTXFSIZ_NPTXFSA_SHIFT	0

#define OTG_HNPTXFSIZ_NPTXFD_GET(x)	OTG_GET(HNPTXFSIZ, NPTXFD, x)
#define OTG_HNPTXFSIZ_NPTXFD_SET(x)	OTG_SET(HNPTXFSIZ, NPTXFD, x)
#define OTG_HNPTXFSIZ_NPTXFSA_GET(x)	OTG_GET(HNPTXFSIZ, NPTXFSA, x)
#define OTG_HNPTXFSIZ_NPTXFSA_SET(x)	OTG_SET(HNPTXFSIZ, NPTXFSA, x)

/* OTG_DIEPTXF0 */
#define OTG_DIEPTXF0_TX0FD_MASK		0xffff
#define OTG_DIEPTXF0_TX0FD_SHIFT	16
#define OTG_DIEPTXF0_TX0FSA_MASK	0xffff
#define OTG_DIEPTXF0_TX0FSA_SHIFT	0

#define OTG_DIEPTXF0_TX0FD_GET(x)	OTG_GET(DIEPTXF0, TX0FD, x)
#define OTG_DIEPTXF0_TX0FD_SET(x)	OTG_SET(DIEPTXF0, TX0FD, x)
#define OTG_DIEPTXF0_TX0FSA_GET(x)	OTG_GET(DIEPTXF0, TX0FSA, x)
#define OTG_DIEPTXF0_TX0FSA_SET(x)	OTG_SET(DIEPTXF0, TX0FSA, x)

/* OTG_HNPTXSTS */
#define OTG_HNPTXSTS_NPTXQTOP_MASK	0x7f
#define OTG_HNPTXSTS_NPTXQTOP_SHIFT	24
#define OTG_HNPTXSTS_NPTQXSAV_MASK	0xff
#define OTG_HNPTXSTS_NPTQXSAV_SHIFT	16
#define OTG_HNPTXSTS_NPTXFSAV_MASK	0xffff
#define OTG_HNPTXSTS_NPTXFSAV_SHIFT	0

/* read macros for status fields */
#define OTG_HNPTXSTS_NPTXQTOP_GET(x)	OTG_GET(HNPTXSTS, NPTXQTOP, x)
#define OTG_HNPTXSTS_NPTQXSAV_GET(x)	OTG_GET(HNPTXSTS, NPTQXSAV, x)
#define OTG_HNPTXSTS_NPTXFSAV_GET(x)	OTG_GET(HNPTXSTS, NPTXFSAV, x)

/* OTG_GI2CCTL */
#define OTG_GI2CCTL_BSYDNE		(1 << 31)
#define OTG_GI2CCTL_RW			(1 << 30)
#define OTG_GI2CCTL_I2CDATSE0		(1 << 28)
#define OTG_GI2CCTL_I2CDEVADR_SHIFT	26
#define OTG_GI2CCTL_I2CDEVADR_MASK	0x3
#define OTG_GI2CCTL_ACK			(1 << 24)
#define OTG_GI2CCTL_I2CEN		(1 << 23)
#define OTG_GI2CCTL_ADDR_SHIFT		16
#define OTG_GI2CCTL_ADDR_MASK		0x3
#define OTG_GI2CCTL_REGADDR_SHIFT	8
#define OTG_GI2CCTL_REGADDR_MASK	0x7f
#define OTG_GI2CCTL_RWDATA_SHIFT	0
#define OTG_GI2CCTL_RWDATA_MASK		0xff

/* get/set macros */
#define OTG_GI2CCTL_I2CDEVADR_GET(x)	OTG_GET(GI2CCTL, I2CDEVADR, x)
#define OTG_GI2CCTL_I2CDEVADR_SET(x)	OTG_SET(GI2CCTL, I2CDEVADR, x)
#define OTG_GI2CCTL_ADDR_GET(x)		OTG_GET(GI2CCTL, ADDR, x)
#define OTG_GI2CCTL_ADDR_SET(x)		OTG_SET(GI2CCTL, ADDR, x)
#define OTG_GI2CCTL_REGADDR_GET(x)	OTG_GET(GI2CCTL, REGADDR, x)
#define OTG_GI2CCTL_REGADDR_SET(x)	OTG_SET(GI2CCTL, REGADDR, x)
#define OTG_GI2CCTL_RWDATA_GET(x)	OTG_GET(GI2CCTL, RWDATA, x)
#define OTG_GI2CCTL_RWDATA_SET(x)	OTG_SET(GI2CCTL, RWDATA, x)

/* OTG general core configuration register (OTG_GCCFG) */
/* 31:22 - Reserved */
#define OTG_GCCFG_VBDEN			(1 << 21)
/* 20:17 - Reserved */
#define OTG_GCCFG_PWRDWN		(1 << 16)
/* 15:0 - Reserved */

/* OTG_CID */
#define	OTG_CID_PRODUCT_ID_SHIFT	0
#define OTG_CID_PRODUCT_ID_MASK		0xffffffff
#define OTG_CID_PRODUCT_ID_GET(x)	OTG_GET(CID, PRODUCT_ID, x)

/* OTG_GLPMCFG */
#define OTG_GLPMCFG_ENBESL		(1 << 28)
#define OTG_GLPMCFG_LPMRCNTSTS_SHIFT	25
#define OTG_GLPMCFG_LPMRCNTSTS_MASK	0x7
#define OTG_GLPMCFG_SNDLPM		(1 << 24)
#define OTG_GLPMCFG_LPMRCNT_SHIFT	21
#define OTG_GLPMCFG_LPMRCNT_MASK	0x7
#define OTG_GLPMCFG_LPMCHIDX_SHIFT	17
#define OTG_GLPMCFG_LPMCHIDX_MASK	0xf
#define OTG_GLPMCFG_L1RSMOK		(1 << 16)
#define OTG_GLPMCFG_SLPSTS		(1 << 15)
#define OTG_GLPMCFG_LPMRST_SHIFT	13
#define OTG_GLPMCFG_LPMRST_MASK		0x3
#define OTG_GLPMCFG_L1DSEN		(1 << 12)
#define OTG_GLPMCFG_BESLTHRS_SHIFT	8
#define OTG_GLPMCFG_BESLTHRS_MASK	0xf
#define OTG_GLPMCFG_L1SSEN		(1 << 7)
#define OTG_GLPMCFG_REMWAKE		(1 << 6)
#define OTG_GLPMCFG_BESL_SHIFT		2
#define OTG_GLPMCFG_BESL_MASK		0xf
#define OTG_GLPMCFG_LPMACK		(1 << 1)
#define OTG_GLPMCFG_LPMEN		(1 << 0)

#define OTG_GLPMCFG_LPMRCNTSTS_GET(x)	OTG_GET(GLPMCFG, LPMRCNTSTS, x)
#define OTG_GLPMCFG_LPMRCNTSTS_SET(x)	OTG_SET(GLPMCFG, LPMRCNTSTS, x)
#define OTG_GLPMCFG_LPMRCNT_GET(x)	OTG_GET(GLPMCFG, LPMRCNT, x)
#define OTG_GLPMCFG_LPMRCNT_SET(x)	OTG_SET(GLPMCFG, LPMRCNT, x)
#define OTG_GLPMCFG_LPMRST_GET(x)	OTG_GET(GLPMCFG, LPMRST, x)
#define OTG_GLPMCFG_LPMRST_SET(x)	OTG_SET(GLPMCFG, LPMRST, x)
#define OTG_GLPMCFG_BESLTHRS_GET(x)	OTG_GET(GLPMCFG, BESLTHRS, x)
#define OTG_GLPMCFG_BESLTHRS_SET(x)	OTG_SET(GLPMCFG, BELSTHRS, x)
#define OTG_GLPMCFG_BESL_GET(x)		OTG_GET(GLPMCFG, BESL, x)
#define OTG_GLPMCFG_BESL_SET(x)		OTG_SET(GLPMCFG, BESL, x)

		/* XXX OTG_HPTXFSIZ */

/* Device-mode CSRs */
/* OTG device control register (OTG_DCTL) */
/* Bits 31:12 - Reserved */
#define OTG_DCTL_POPRGDNE	(1 << 11)
#define OTG_DCTL_CGONAK		(1 << 10)
#define OTG_DCTL_SGONAK		(1 << 9)
#define OTG_DCTL_SGINAK		(1 << 8)
#define OTG_DCTL_TCTL_MASK	(7 << 4)
#define OTG_DCTL_GONSTS		(1 << 3)
#define OTG_DCTL_GINSTS		(1 << 2)
#define OTG_DCTL_SDIS		(1 << 1)
#define OTG_DCTL_RWUSIG		(1 << 0)

/* OTG device configuration register (OTG_DCFG) */
#define OTG_DCFG_DSPD		0x0003
#define OTG_DCFG_NZLSOHSK	0x0004
#define OTG_DCFG_DAD		0x07F0
#define OTG_DCFG_PFIVL		0x1800

/* OTG Device IN Endpoint Common Interrupt Mask Register (OTG_DIEPMSK) */
/* Bits 31:10 - Reserved */
#define OTG_DIEPMSK_BIM		(1 << 9)
#define OTG_DIEPMSK_TXFURM	(1 << 8)
/* Bit 7 - Reserved */
#define OTG_DIEPMSK_INEPNEM	(1 << 6)
#define OTG_DIEPMSK_INEPNMM	(1 << 5)
#define OTG_DIEPMSK_ITTXFEMSK	(1 << 4)
#define OTG_DIEPMSK_TOM		(1 << 3)
/* Bit 2 - Reserved */
#define OTG_DIEPMSK_EPDM	(1 << 1)
#define OTG_DIEPMSK_XFRCM	(1 << 0)

/* OTG Device OUT Endpoint Common Interrupt Mask Register (OTG_DOEPMSK) */
/* Bits 31:10 - Reserved */
#define OTG_DOEPMSK_BOIM	(1 << 9)
#define OTG_DOEPMSK_OPEM	(1 << 8)
/* Bit 7 - Reserved */
#define OTG_DOEPMSK_B2BSTUP	(1 << 6)
/* Bit 5 - Reserved */
#define OTG_DOEPMSK_OTEPDM	(1 << 4)
#define OTG_DOEPMSK_STUPM	(1 << 3)
/* Bit 2 - Reserved */
#define OTG_DOEPMSK_EPDM	(1 << 1)
#define OTG_DOEPMSK_XFRCM	(1 << 0)

/* OTG Device Control IN Endpoint 0 Control Register (OTG_DIEPCTL0) */
#define OTG_DIEPCTL0_EPENA		(1 << 31)
#define OTG_DIEPCTL0_EPDIS		(1 << 30)
/* Bits 29:28 - Reserved */
#define OTG_DIEPCTLX_SD0PID		(1 << 28)
#define OTG_DIEPCTL0_SNAK		(1 << 27)
#define OTG_DIEPCTL0_CNAK		(1 << 26)
#define OTG_DIEPCTL0_TXFNUM_MASK	(0xf << 22)
#define OTG_DIEPCTL0_STALL		(1 << 21)
/* Bit 20 - Reserved */
#define OTG_DIEPCTL0_EPTYP_MASK		(0x3 << 18)
#define OTG_DIEPCTL0_NAKSTS		(1 << 17)
/* Bit 16 - Reserved */
#define OTG_DIEPCTL0_USBAEP		(1 << 15)
/* Bits 14:2 - Reserved */
#define OTG_DIEPCTL0_MPSIZ_MASK		(0x3 << 0)
#define OTG_DIEPCTL0_MPSIZ_64		(0x0 << 0)
#define OTG_DIEPCTL0_MPSIZ_32		(0x1 << 0)
#define OTG_DIEPCTL0_MPSIZ_16		(0x2 << 0)
#define OTG_DIEPCTL0_MPSIZ_8		(0x3 << 0)

/* OTG Device Control OUT Endpoint 0 Control Register (OTG_DOEPCTL0) */
#define OTG_DOEPCTL0_EPENA		(1 << 31)
#define OTG_DOEPCTL0_EPDIS		(1 << 30)
/* Bits 29:28 - Reserved */
#define OTG_DOEPCTLX_SD0PID		(1 << 28)
#define OTG_DOEPCTL0_SNAK		(1 << 27)
#define OTG_DOEPCTL0_CNAK		(1 << 26)
/* Bits 25:22 - Reserved */
#define OTG_DOEPCTL0_STALL		(1 << 21)
#define OTG_DOEPCTL0_SNPM		(1 << 20)
#define OTG_DOEPCTL0_EPTYP_MASK		(0x3 << 18)
#define OTG_DOEPCTL0_NAKSTS		(1 << 17)
/* Bit 16 - Reserved */
#define OTG_DOEPCTL0_USBAEP		(1 << 15)
/* Bits 14:2 - Reserved */
#define OTG_DOEPCTL0_MPSIZ_MASK		(0x3 << 0)
#define OTG_DOEPCTL0_MPSIZ_64		(0x0 << 0)
#define OTG_DOEPCTL0_MPSIZ_32		(0x1 << 0)
#define OTG_DOEPCTL0_MPSIZ_16		(0x2 << 0)
#define OTG_DOEPCTL0_MPSIZ_8		(0x3 << 0)

/* OTG Device IN Endpoint Interrupt Register (OTG_DIEPINTx) */
/* Bits 31:8 - Reserved */
#define OTG_DIEPINTX_TXFE		(1 << 7)
#define OTG_DIEPINTX_INEPNE		(1 << 6)
/* Bit 5 - Reserved */
#define OTG_DIEPINTX_ITTXFE		(1 << 4)
#define OTG_DIEPINTX_TOC		(1 << 3)
/* Bit 2 - Reserved */
#define OTG_DIEPINTX_EPDISD		(1 << 1)
#define OTG_DIEPINTX_XFRC		(1 << 0)

/* OTG Device IN Endpoint Interrupt Register (OTG_DOEPINTx) */
/* Bits 31:7 - Reserved */
#define OTG_DOEPINTX_B2BSTUP		(1 << 6)
/* Bit 5 - Reserved */
#define OTG_DOEPINTX_OTEPDIS		(1 << 4)
#define OTG_DOEPINTX_STUP		(1 << 3)
/* Bit 2 - Reserved */
#define OTG_DOEPINTX_EPDISD		(1 << 1)
#define OTG_DOEPINTX_XFRC		(1 << 0)

/* OTG Device OUT Endpoint 0 Transfer Size Register (OTG_DOEPTSIZ0) */
/* Bit 31 - Reserved */
#define OTG_DIEPSIZ0_STUPCNT_1		(0x1 << 29)
#define OTG_DIEPSIZ0_STUPCNT_2		(0x2 << 29)
#define OTG_DIEPSIZ0_STUPCNT_3		(0x3 << 29)
#define OTG_DIEPSIZ0_STUPCNT_MASK	(0x3 << 29)
/* Bits 28:20 - Reserved */
#define OTG_DIEPSIZ0_PKTCNT		(1 << 19)
/* Bits 18:7 - Reserved */
#define OTG_DIEPSIZ0_XFRSIZ_MASK	(0x7f << 0)



/* Host-mode CSRs */
/* OTG Host non-periodic transmit FIFO size register
(OTG_HNPTXFSIZ)/Endpoint 0 Transmit FIFO size (OTG_DIEPTXF0) */
#define OTG_HNPTXFSIZ_PTXFD_MASK	(0xffff0000)
#define OTG_HNPTXFSIZ_PTXSA_MASK	(0x0000ffff)

/* OTG Host periodic transmit FIFO size register (OTG_HPTXFSIZ) */
#define OTG_HPTXFSIZ_PTXFD_MASK		(0xffff0000)
#define OTG_HPTXFSIZ_PTXSA_MASK		(0x0000ffff)

/* OTG Host Configuration Register (OTG_HCFG) */
/* Bits 31:3 - Reserved */
#define OTG_HCFG_FSLSS		(1 << 2)
#define OTG_HCFG_FSLSPCS_48MHz	(0x1 << 0)
#define OTG_HCFG_FSLSPCS_6MHz	(0x2 << 0)
#define OTG_HCFG_FSLSPCS_MASK	(0x3 << 0)

/* OTG Host Frame Interval Register (OTG_HFIR) */
/* Bits 31:16 - Reserved */
#define OTG_HFIR_FRIVL_MASK		(0x0000ffff)

/* OTG Host frame number/frame time remaining register (OTG_HFNUM) */
#define OTG_HFNUM_FTREM_MASK		(0xffff0000)
#define OTG_HFNUM_FRNUM_MASK		(0x0000ffff)

/* OTG Host periodic transmit FIFO/queue status register (OTG_HPTXSTS) */
#define OTG_HPTXSTS_PTXQTOP_MASK			(0xff000000)
#define OTG_HPTXSTS_PTXQTOP_ODDFRM			(1<<31)
#define OTG_HPTXSTS_PTXQTOP_EVENFRM			(0<<31)
#define OTG_HPTXSTS_PTXQTOP_CHANNEL_NUMBER_MASK		(0xf<<27)
#define OTG_HPTXSTS_PTXQTOP_ENDPOINT_NUMBER_MASK	(0xf<<27)
#define OTG_HPTXSTS_PTXQTOP_TYPE_INOUT			(0x00<<25)
#define OTG_HPTXSTS_PTXQTOP_TYPE_ZEROLENGTH		(0x01<<25)
#define OTG_HPTXSTS_PTXQTOP_TYPE_DISABLECMD		(0x11<<25)
#define OTG_HPTXSTS_PTXQTOP_TERMINATE			(1<<24)
#define OTG_HPTXSTS_PTXQSAV_MASK			(0x00ff0000)
#define OTG_HPTXSTS_PTXFSAVL_MASK			(0x0000ffff)

/* OTG Host all channels interrupt mask register (OTG_HAINT) */
/* Bits 31:16 - Reserved */
#define OTG_HAINTMSK_HAINT_MASK		(0x0000ffff)

/* OTG Host all channels interrupt mask register (OTG_HAINTMSK) */
/* Bits 31:16 - Reserved */
#define OTG_HAINTMSK_HAINTM_MASK	(0x0000ffff)

/* OTG Host port control and status register (OTG_HPRT) */
/* Bits 31:19 - Reserved */
#define OTG_HPRT_PSPD_HIGH		(0x0 << 17)
#define OTG_HPRT_PSPD_FULL		(0x1 << 17)
#define OTG_HPRT_PSPD_LOW		(0x2 << 17)
#define OTG_HPRT_PSPD_MASK		(0x3 << 17)
#define OTG_HPRT_PTCTL_DISABLED		(0x0 << 13)
#define OTG_HPRT_PTCTL_J		(0x1 << 13)
#define OTG_HPRT_PTCTL_K		(0x2 << 13)
#define OTG_HPRT_PTCTL_SE0_NAK		(0x3 << 13)
#define OTG_HPRT_PTCTL_PACKET		(0x4 << 13)
#define OTG_HPRT_PTCTL_FORCE_ENABLE	(0x5 << 13)
#define OTG_HPRT_PPWR			(1 << 12)
#define OTG_HPRT_PLSTS_DM		(1 << 11)
#define OTG_HPRT_PLSTS_DP		(1 << 10)
/* Bit 9 - Reserved */
#define OTG_HPRT_PRST		(1 << 8)
#define OTG_HPRT_PSUSP		(1 << 7)
#define OTG_HPRT_PRES		(1 << 6)
#define OTG_HPRT_POCCHNG	(1 << 5)
#define OTG_HPRT_POCA		(1 << 4)
#define OTG_HPRT_PENCHNG	(1 << 3)
#define OTG_HPRT_PENA		(1 << 2)
#define OTG_HPRT_PCDET		(1 << 1)
#define OTG_HPRT_PCSTS		(1 << 0)

/* OTG Host channel-x characteristics register (OTG_HCCHARx) */
#define OTG_HCCHAR_CHENA		(1 << 31)
#define OTG_HCCHAR_CHDIS		(1 << 30)
#define OTG_HCCHAR_ODDFRM		(1 << 29)
#define OTG_HCCHAR_DAD_MASK		(0x7f << 22)
#define OTG_HCCHAR_MCNT_1		(0x1 << 20)
#define OTG_HCCHAR_MCNT_2		(0x2 << 20)
#define OTG_HCCHAR_MCNT_3		(0x3 << 20)
#define OTG_HCCHAR_MCNT_MASK		(0x3 << 20)
#define OTG_HCCHAR_EPTYP_CONTROL	(0 << 18)
#define OTG_HCCHAR_EPTYP_ISOCHRONOUS	(1 << 18)
#define OTG_HCCHAR_EPTYP_BULK		(2 << 18)
#define OTG_HCCHAR_EPTYP_INTERRUPT	(3 << 18)
#define OTG_HCCHAR_EPTYP_MASK		(3 << 18)
#define OTG_HCCHAR_LSDEV		(1 << 17)
/* Bit 16 - Reserved */
#define OTG_HCCHAR_EPDIR_OUT		(0 << 15)
#define OTG_HCCHAR_EPDIR_IN		(1 << 15)
#define OTG_HCCHAR_EPDIR_MASK		(1 << 15)
#define OTG_HCCHAR_EPNUM_MASK		(0xf << 11)
#define OTG_HCCHAR_MPSIZ_MASK		(0x7ff << 0)

/* OTG Host channel-x interrupt register (OTG_HCINTx) */
/* Bits 31:11 - Reserved */
#define OTG_HCINT_DTERR		(1 << 10)
#define OTG_HCINT_FRMOR		(1 << 9)
#define OTG_HCINT_BBERR		(1 << 8)
#define OTG_HCINT_TXERR		(1 << 7)
/* Note: OTG_HCINT_NYET: Only in OTG_HS */
#define OTG_HCINT_NYET		(1 << 6)
#define OTG_HCINT_ACK		(1 << 5)
#define OTG_HCINT_NAK		(1 << 4)
#define OTG_HCINT_STALL		(1 << 3)
/* Note: OTG_HCINT_AHBERR: Only in OTG_HS */
#define OTG_HCINT_AHBERR	(1 << 2)
#define OTG_HCINT_CHH		(1 << 1)
#define OTG_HCINT_XFRC		(1 << 0)

/* OTG Host channel-x interrupt mask register (OTG_HCINTMSKx) */
/* Bits 31:11 - Reserved */
#define OTG_HCINTMSK_DTERRM		(1 << 10)
#define OTG_HCINTMSK_FRMORM		(1 << 9)
#define OTG_HCINTMSK_BBERRM		(1 << 8)
#define OTG_HCINTMSK_TXERRM		(1 << 7)
/* Note: OTG_HCINTMSK_NYET: Only in OTG_HS */
#define OTG_HCINTMSK_NYET		(1 << 6)
#define OTG_HCINTMSK_ACKM		(1 << 5)
#define OTG_HCINTMSK_NAKM		(1 << 4)
#define OTG_HCINTMSK_STALLM		(1 << 3)
/* Note: OTG_HCINTMSK_AHBERR: Only in OTG_HS */
#define OTG_HCINTMSK_AHBERR		(1 << 2)
#define OTG_HCINTMSK_CHHM		(1 << 1)
#define OTG_HCINTMSK_XFRCM		(1 << 0)

/* OTG Host channel-x transfer size register (OTG_HCTSIZx) */
/* Note: OTG_HCTSIZ_DOPING: Only in OTG_HS */
#define OTG_HCTSIZ_DOPING	(1 << 31)
#define OTG_HCTSIZ_DPID_DATA0	(0x0 << 29)
#define OTG_HCTSIZ_DPID_DATA1	(0x2 << 29)
#define OTG_HCTSIZ_DPID_DATA2	(0x1 << 29)
#define OTG_HCTSIZ_DPID_MDATA	(0x3 << 29)
#define OTG_HCTSIZ_DPID_MASK	(0x3 << 29)
#define OTG_HCTSIZ_PKTCNT_MASK	(0x3ff << 19)
#define OTG_HCTSIZ_XFRSIZ_MASK	(0x7ffff << 0)


/* OTG_HS specific registers */

/* Device-mode Control and Status Registers */
#define OTG_DEACHHINT		0x838
#define OTG_DEACHHINTMSK	0x83C
#define OTG_DIEPEACHMSK1	0x844
#define OTG_DOEPEACHMSK1	0x884
#define OTG_DIEPDMA(x)		(0x914 + 0x20*(x))
#define OTG_DOEPDMA(x)		(0xB14 + 0x20*(x))


/* Device-mode CSRs*/
/* OTG device each endpoint interrupt register (OTG_DEACHINT) */
/* Bits 31:18 - Reserved */
#define OTG_DEACHHINT_OEP1INT		(1 << 17)
/* Bits 16:2 - Reserved */
#define OTG_DEACHHINT_IEP1INT		(1 << 1)
/* Bit 0 - Reserved */

/* OTG device each in endpoint-1 interrupt register (OTG_DIEPEACHMSK1) */
/* Bits 31:14 - Reserved */
#define OTG_DIEPEACHMSK1_NAKM		(1 << 13)
/* Bits 12:10 - Reserved */
#define OTG_DIEPEACHMSK1_BIM		(1 << 9)
#define OTG_DIEPEACHMSK1_TXFURM		(1 << 8)
/* Bit 7 - Reserved */
#define OTG_DIEPEACHMSK1_INEPNEM	(1 << 6)
#define OTG_DIEPEACHMSK1_INEPNMM	(1 << 5)
#define OTG_DIEPEACHMSK1_ITTXFEMSK	(1 << 4)
#define OTG_DIEPEACHMSK1_TOM		(1 << 3)
/* Bit 2 - Reserved */
#define OTG_DIEPEACHMSK1_EPDM		(1 << 1)
#define OTG_DIEPEACHMSK1_XFRCM		(1 << 0)

/* OTG device each OUT endpoint-1 interrupt register (OTG_DOEPEACHMSK1) */
/* Bits 31:15 - Reserved */
#define OTG_DOEPEACHMSK1_NYETM		(1 << 14)
#define OTG_DOEPEACHMSK1_NAKM		(1 << 13)
#define OTG_DOEPEACHMSK1_BERRM		(1 << 12)
/* Bits 11:10 - Reserved */
#define OTG_DOEPEACHMSK1_BIM		(1 << 9)
#define OTG_DOEPEACHMSK1_OPEM		(1 << 8)
/* Bits 7:3 - Reserved */
#define OTG_DOEPEACHMSK1_AHBERRM	(1 << 2)
#define OTG_DOEPEACHMSK1_EPDM		(1 << 1)
#define OTG_DOEPEACHMSK1_XFRCM		(1 << 0)

/* Host-mode CSRs */
/* OTG host channel-x split control register (OTG_HCSPLTx) */
#define OTG_HCSPLT_SPLITEN		(1 << 31)
/* Bits 30:17 - Reserved */
#define OTG_HCSPLT_COMPLSPLT		(1 << 16)
#define OTG_HCSPLT_XACTPOS_ALL		(0x3 << 14)
#define OTG_HCSPLT_XACTPOS_BEGIN	(0x2 << 14)
#define OTG_HCSPLT_XACTPOS_MID		(0x0 << 14)
#define OTG_HCSPLT_XACTPOS_END		(0x1 << 14)
#define OTG_HCSPLT_HUBADDR_MASK		(0x7f << 7)
#define OTG_HCSPLT_PORTADDR_MASK	(0x7f << 0)

