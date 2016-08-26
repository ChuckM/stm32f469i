/*
 * OTM8009a.h
 *
 * Define stuff associated with the LCD display. This from the data
 * sheet.
 */

#pragma once
/*
 * Op codes: C_ command, R_ read, W_ write
 */
#define C_NOP		0x0	/* NOP */
#define C_SWRESET	0x1	/* Software Reset */
#define R_RDNUMED	0x05	/* Read # of Errors on DSI */
#define R_RDDPM		0x0a	/* Read Display Power Mode */
#define R_RDDMADCTL	0x0b	/* Read Display MADCTL */
#define R_RDDCOLMOD	0x0c	/* Read Display Pixel Format */
#define R_RDDIM		0x0d	/* Read Display Image Mode */
#define R_RDDSM		0x0e	/* Read Display Signal Mode */
#define	R_RDDSDR	0x0f	/* Read Display Self Diagnostic Result */
#define C_SLPIN		0x10	/* Sleep In */
#define C_SLPOUT	0x11	/* Sleep Out */
#define C_PTLON		0x12	/* Partial Mode on */
#define C_NORON		0x13	/* Normal Display on */
#define C_INVOFF	0x20	/* Turn of display inversion */
#define C_INVON		0x21	/* Turn on display inversion */
#define C_ALLPOFF	0x22	/* All pixels off */
#define C_ALLPON	0x23	/* All pixels on */
#define W_GAMSET	0x26	/* Gamma set */
#define C_DISPOFF	0x28	/* Display off */
#define C_DISPON	0x29	/* Display on */
#define W_CASET		0x2a	/* Column Address set */
#define W_PASET		0x2b	/* Page address set */
#define W_RAMWR		0x2c	/* Memory write */
#define R_RAMRD		0x2e	/* Memory read */
#define W_PLTAR		0x30	/* Partial area */
#define C_TEOFF		0x34	/* Tearing effect line off */
#define W_TEEON		0x35	/* Tearing effect line on */
#define W_MADCTL	0x36	/* Memory Access control */
#define C_IDMOFF	0x38	/* Idle mode off */
#define C_IDMON		0x39	/* Idle mode on */
#define C_COLMOD	0x3a	/* Interface Pixel Format */
#define W_RAMWRC	0x3c	/* Memory Write continue */
#define R_RAMRDC	0x3e	/* Memory Read continue */
#define W_WRTESCN	0x44	/* Write TE scan line */
#define R_RDSCNL	0x45	/* Read scan line */
#define W_WRDISBV	0x51	/* Write Display Brightness */
#define R_RDDISBV	0x52	/* Read Display Brightness */
#define W_WRCTRLD	0x53	/* Write CTRL Display */
#define R_RDCTRLD	0x54	/* Read CTRL Display */
#define W_WRCABC	0x55	/* Write Content Adaptive Brightness Control */
#define R_RDCABC	0x56	/* Read Content Adaptive brightness control */
#define W_WRCABCMB	0x5e	/* Write CABC minimum brightness */
#define R_RDCABCMB	0x5f	/* Read CABC minimum brightness */
#define R_RDABCSDR	0x68	/* Read Auto-bright-control diag result */
#define R_RDBWLB	0x70	/* Read black/white low bits */
#define R_RDBKX		0x71	/* Read Black "x" */
#define R_RDBKY		0x72	/* Read Black "y" */
#define R_RDWX		0x73	/* Read White "x" */
#define R_RDWY		0x74	/* Read White "y" */
#define R_RDRGLB	0x75	/* Read Red/Green low bits */
#define R_RDRX		0x76	/* Read RED x */
#define R_RDRY		0x77	/* Read RED y */
#define R_RDGX		0x78	/* Read GREEN x */
#define R_RDGY		0x79	/* Read GREEN y */
#define R_RDBALB	0x7a	/* Read Blue/Alpha low bits */
#define R_RDBX		0x7b	/* Read BLUE x */
#define R_RDBY		0x7c	/* Read BLUE y */
#define R_RDAX		0x7d	/* Read Alpha x */
#define R_RDAY		0x7e	/* Read Alpha y */
#define R_RDDDBS	0xa1	/* Read DDB Start */
#define R_RDDDBC	0xa8	/* Read DDB continue */
#define R_RDFCS		0xaa	/* Read First Checksum */
#define R_RDCCS		0xaf	/* Read Continue checksum */
#define R_RDID1		0xda	/* Read ID1 */
#define R_RDID2		0xdb	/* Read ID2 */
#define R_RDID3		0xdc	/* Read ID3 */

/*
 * Two byte commands
 * Only available in MIPI Low Power (LPDT) mode.
 */
#define	W_ADRSFT		0x0000	/* Address Shift Function */
#define W_CMD2ENA1		0xff00	/* Enable multibyte commands */
#define W_CMD2ENA2		0xff80	/* Enable vendor (Orise) commands */
#define RW_OTPSEL		0xa000	/* OTP Select region */
#define RW_MIPISET1		0xb080	/* MIPI Setting 1 */
#define RW_MIPISET2		0xb0a1	/* MIPI Setting 2 */
#define RW_IF_PARA1		0xb280	/* IF Parameter 1 */
#define RW_IF_PARA2		0xb282	/* IF Parameter 2 */
#define RW_PADPARA		0xb390	/* IO pad parameter */
#define RW_RAMPWRSET		0xb3c0	/* RAM Power setting */
#define	RW_TSP1			0xc080	/* TCON power setting */
#define RW_PTSP1		0xc092	/* Panel timing setting parameter 1 */
#define RW_PTSP2		0xc094	/* Panel timing setting parameter 2 */
#define RW_SD_CTRL		0xc0a2	/* Source driver timing setting */
#define RW_P_DRV_M		0xc0b4	/* Panel driving mode */
#define RW_OSC_ADJ		0xc181	/* Oscillator adjustment for idle/normal mode */
#define RW_RGB_VIDEO_SET	0xc1a1	/* Set RGB / BRG mode */
#define RW_SD_PCH_CTRL		0xc480	/* Set source driver precharage */
#define RW_PWR_CTRL1		0xc580	/* Power control setting 1 */
#define RW_PWR_CTRL2		0xc590	/* Power control setting 2 */
#define RW_PWR_CTRL3		0xc5a0	/* Power control setting 3 */
#define RW_PWR_CTRL4		0xc5b0	/* Power control setting 4 */
#define RW_PWMPARA1		0xc680	/* pwm parameter 1 ... */
#define RW_PWMPARA2		0xc6b0	/* ... 2 */
#define RW_PWMPARA3		0xc6b1	/* ... 3 */
#define RW_PWMPARA4		0xc6b2	/* ... 4 */
#define RW_PWMPARA5		0xc6b3	/* ... 5 */
#define RW_PWMPARA6		0xc6b4	/* ... 6 */
#define RW_CABCSET1		0xc700	/* CABC settting */
#define RW_CABCSET2		0xc800	/* CABC settting */
#define RW_AIESET		0xc900 	/* AIE Setting */
/*
 * GOA mode, what the heck is GOA mode?
 */
#define RW_GOAVST		0xce80	/* GOA VST setting */
#define RW_GOAVEND		0xce90	/* GOA VEND setting */
#define RW_GOAGPSET		0xce9c	/* GOA Group settting */

#define RW_GOACLKA1		0xcea0
#define RW_GOACLKA2		0xcea7
#define RW_GOACLKA3		0xceb0
#define RW_GOACLKA4		0xceb7

#define RW_GOACLKB1		0xcec0
#define RW_GOACLKB2		0xcec7
#define RW_GOACLKB3		0xced0
#define RW_GOACLKB4		0xced7

#define RW_GOACLKC1		0xcf80
#define RW_GOACLKC2		0xcf87
#define RW_GOACLKC3		0xcf90
#define RW_GOACLKC4		0xcf97

#define RW_GOACLKD1		0xcfa0
#define RW_GOACLKD2		0xcfa7
#define RW_GOACLKD3		0xcfb0
#define RW_GOACLKD4		0xcfb7

#define RW_GOAECLK		0xcfc0	/* ECLK setting */
#define RW_GOAOPT1		0xcfc6	/* Other options 1 */
#define RW_GOATGOPT		0xcfc7	/* Signal toggle option */

/*
 * Misc functions
 */
#define RW_WRID1		0xd000	/* Write ID1 */
#define RW_WRID2		0xd100	/* Write ID2 */
#define RW_WRDDB		0xd200	/* Write DDB setting */
#define R_EXTCCHK		0xd300	/* Check ExtC */
#define RW_CESET1		0xd400	/* CE Correction settings 1 */
#define RW_CESET2		0xd500	/* CE Correction settings 2 */
#define RW_CEEN			0xd680	/* CE Enable */
#define RW_AIEEN		0xd700	/* AIE Enable */
#define RW_GVDDSET		0xd800	/* Vdd setting */
#define RW_VCOMDC		0xd900	/* VCOM voltage setting */
#define RW_GMCT22P		0xe100	/* Positive Gamma correction (2.2) */
#define RW_GMCT22N		0xe200	/* Negative Gamma correction (2.2) */
#define RW_GMCT18P		0xe300	/* Gamma correction (1.8) */
#define RW_GMCT18N		0xe400
#define RW_GMCT25P		0xe500	/* Gamma correction (2.5) */
#define RW_GMCT25N		0xe600
#define RW_GMCT10P		0xe700	/* Gamma Correction 1.0 */
#define RW_GMCT10N		0xe800
#define W_NVMIN			0xeb00	/* NV Memory write */
#define RW_GAMR			0xec00	/* Gamma correction (Red) */
#define RW_GAMG			0xed00	/* Gamma correction (Green) */
#define RW_GAMB			0xee00	/* Gamma correction (Blue) */
#define R_PRG_FLAH		0xf101	/* OTP Program flag state */

/*
 * Packets are either 4 bytes (short) or 6 - 64K bytes (long)
 */

/*
 * Timing parameters for an 800 x 480 screen
 * 	Vertical timings are in terms of "hsyncs" and
 * 	Horizontal timings are in terms of the Pixel clock
 */
#define DISP_VCP	832		/* Vertical Cycle period */
#define DISP_VLPW	1		/* Vertical sync pulse width */
#define DISP_VFP	16		/* Vertical Front Porch */
#define DISP_VBP	15		/* Vertical Back Porch */
#define DISP_VBLNKP	32		/* Vertical Blanking Period */
#define DISP_VAAREA	800		/* Vertical Active area */
#define DISP_VRR	60		/* Vertical Refresh rate */

#define DISP_HCP	522		/* Horizontal Cycle period */
#define DISP_HLPW	2		/* Horizontal sync pulse width */
#define DISP_HFP	20		/* Horizontal Front Porch */
#define DISP_HBP	20		/* Horizontal Back Porch */
#define DISP_HBLNKP	42		/* Horizontal Blanking Period */
#define DISP_HAAREA	480		/* Horizontal Active area */
/* Typical 26.37Mhz pixel clock rate (24Mhz, 30.74Mhz) (min, max) */
