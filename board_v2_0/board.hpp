#pragma once
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <mculib/fastwiring.hpp>
#include <mculib/softi2c.hpp>
#include <mculib/softspi.hpp>
#include <mculib/si5351.hpp>
#include <mculib/adf4350.hpp>
#include <mculib/dma_adc.hpp>

#include <array>
#include <stdint.h>
#include <libopencm3/stm32/f1/dma.h>
#include <libopencm3/stm32/adc.h>

#include "../rfsw.hpp"
#include "../common.hpp"

#define BOARD_NAME "NanoVNA V2_0"

using namespace mculib;
using namespace std;


namespace board {

	static constexpr Pad led = PA9;
	static constexpr Pad led2 = PA10;

	static constexpr array<Pad, 2> RFSW_ECAL = {PC13, PC14};
	static constexpr array<Pad, 2> RFSW_BBGAIN = {PB13, PB12};
	static constexpr Pad RFSW_TXSYNTH = PB9;
	static constexpr Pad RFSW_RXSYNTH = PA4;
	static constexpr Pad RFSW_REFL = PB0;
	static constexpr Pad RFSW_RECV = PB1;

	static constexpr Pad ili9341_cs = PA15;
	static constexpr Pad ili9341_dc = PB6;

	static constexpr auto RFSW_ECAL_SHORT = RFSWState::RF2;
	static constexpr auto RFSW_ECAL_OPEN = RFSWState::RF3;
	static constexpr auto RFSW_ECAL_LOAD = RFSWState::RF4;
	static constexpr auto RFSW_ECAL_NORMAL = RFSWState::RF1;

	static constexpr int RFSW_TXSYNTH_LF = 0;
	static constexpr int RFSW_TXSYNTH_HF = 1;

	static constexpr int RFSW_RXSYNTH_LF = 1;
	static constexpr int RFSW_RXSYNTH_HF = 0;

	static constexpr int RFSW_REFL_ON = 1;
	static constexpr int RFSW_REFL_OFF = 0;

	static constexpr int RFSW_RECV_REFL = 1;
	static constexpr int RFSW_RECV_PORT2 = 0;


	// set by board_init()
	extern uint32_t adc_ratecfg;
	extern uint32_t adc_srate; // Hz
	extern uint32_t adc_period_cycles, adc_clk;
	extern uint8_t registers[32];


	extern DMADriver dma;
	extern DMAChannel dmaChannel;
	extern DMAADC dmaADC;

	struct i2cDelay_t {
		void operator()() {
			delayMicroseconds(1);
		}
	};
	struct spiDelay_t {
		void operator()() {
			delayMicroseconds(1);
		}
	};


	extern SoftI2C<i2cDelay_t> si5351_i2c;
	extern Si5351::Si5351Driver si5351;

	extern SoftSPI<spiDelay_t> adf4350_tx_spi;
	extern SoftSPI<spiDelay_t> adf4350_rx_spi;

	struct adf4350_sendWord_t {
		SoftSPI<spiDelay_t>& spi;
		void operator()(uint32_t word) {
			spi.beginTransfer();
			spi.doTransfer_send(word, 32);
			spi.endTransfer();
		}
	};
	extern ADF4350::ADF4350Driver<adf4350_sendWord_t> adf4350_tx;
	extern ADF4350::ADF4350Driver<adf4350_sendWord_t> adf4350_rx;

	struct spiDelay_fast_t {
		void operator()() {
			asm __volatile__ ( "nop" );
		}
	};
	extern SoftSPI<spiDelay_fast_t> ili9341_spi;


	// gain is an integer from 0 to 3, 0 being lowest gain
	static inline RFSWState RFSW_BBGAIN_GAIN(int gain) {
		switch(gain) {
			case 0: return RFSWState::RF1;
			case 1: return RFSWState::RF2;
			case 2: return RFSWState::RF3;
			case 3: return RFSWState::RF4;
			default: return RFSWState::RF4;
		}
		return RFSWState::RF4;
	}

	void boardInit();

	void ledPulse();

	bool si5351_setup();

	void si5351_set(bool isRX, uint32_t freq_khz);
}