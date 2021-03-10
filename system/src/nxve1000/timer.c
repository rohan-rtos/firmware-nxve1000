/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2014 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <cmsis_device.h>
#include <timer.h>
#include <io.h>
#include <config.h>

#ifdef TIMER_ENABLED

#define TIMER_CLOCK_HZ			1000000
#define	TIMER_MAX_COUNT			-(1UL)
#define TIMER_MUX_SEL			0 				/* bypass */

#define	TIMER_CH_OFFSET			(0x100)

#define TCFG0_PRESCALER_MASK	0xff
#define TCFG1_MUX_MASK          0x7
#define TCON_START				BIT(0)
#define TCON_MANUALUPDATE   	BIT(1)
#define TCON_INVERT         	BIT(2)
#define TCON_AUTORELOAD    		BIT(3)

#define TINT_STATUS    			BIT(5)
#define TINT_ENABLE    			BIT(0)

typedef struct {
  __IOM uint32_t TCFG0;			/* 0x00 */
  __IOM uint32_t TCFG1;			/* 0x04 */
  __IOM uint32_t TCON;			/* 0x08 */
  __IOM uint32_t TCNTB;			/* 0x0C */
  __IOM uint32_t TCMPB;			/* 0x10 */
  __IOM uint32_t TCNTO;			/* 0x14 */
  __IOM uint32_t TINT_CSTAT;	/* 0x18 */
} Timer_Reg;

/*
 * Timer HW
 */
static struct __attribute__((__packed__)) _TIMER_ {
	Timer_Reg *base;
	uint64_t timestamp;
	uint32_t lastdec;
} _timer = {
	.timestamp = 0,
	.lastdec = 0,
};

void TIMER_Delay(int us)
{
#if TIMER_IRQ_MODE
#else
	uint64_t end = TIMER_GetTick() + (uint64_t)us;

	while (TIMER_GetTick() < end) {
			;
	};
#endif
}

uint64_t TIMER_GetTick(void)
{
	uint64_t time = _timer.timestamp;
	uint32_t lastdec = _timer.lastdec;
	uint32_t now = TIMER_MAX_COUNT - readl(&_timer.base->TCNTO);

	if (now >= lastdec)
		time += now - lastdec;
	else
		time += now + TIMER_MAX_COUNT - lastdec;

	_timer.lastdec = now;
	_timer.timestamp = time;

	return _timer.timestamp;
}

static void TIMER_Config(int mux, int scale, unsigned int count)
{
	writel(_mask(_timer.base->TCFG1, TCFG1_MUX_MASK) | (uint32_t)mux, &_timer.base->TCFG1);
	writel(_mask(_timer.base->TCFG0, TCFG0_PRESCALER_MASK) | (uint32_t)(scale - 1), &_timer.base->TCFG0);
	writel(count, &_timer.base->TCNTB);
	writel(count, &_timer.base->TCMPB);
}

static void TIMER_Start(void)
{
#if TIMER_IRQ_MODE
	writel(readl(&_timer.base->TINT_CSTAT) | TINT_ENABLE, &_timer.base->TINT_CSTAT);
#endif
	writel((readl(&_timer.base->TCON) | TCON_MANUALUPDATE), &_timer.base->TCON);
	writel(TCON_AUTORELOAD | TCON_START, &_timer.base->TCON);
}

static void TIMER_Stop(void)
{
	writel(_mask(_timer.base->TCON, TCON_START), &_timer.base->TCON);
}

int TIMER_Init(int ch, unsigned int clock)
{
	unsigned int count = TIMER_MAX_COUNT;
	int scale;
	
	if (ch > 7)
		return -1;

	_timer.base = (void *)(TIMER_PHY_BASE + (TIMER_CH_OFFSET * ch));

#if TIMER_IRQ_MODE
	count = TIMER_CLOCK_HZ;
#endif
	scale = (int)clock / TIMER_CLOCK_HZ;

	TIMER_Stop();
	TIMER_Config(TIMER_MUX_SEL, scale, count);
	TIMER_Start();

	return 0;
}
#endif
