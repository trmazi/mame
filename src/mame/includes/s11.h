// license:BSD-3-Clause
// copyright-holders:Miodrag Milanovic, Robbbert
/*
 * s11.h
 *
 *  Created on: 1/01/2013
 */

#ifndef MAME_INCLUDES_S11_H
#define MAME_INCLUDES_S11_H

#include "cpu/m6800/m6800.h"
#include "audio/pinsnd88.h"
#include "audio/s11c_bg.h"
#include "machine/6821pia.h"
#include "machine/genpin.h"
#include "machine/input_merger.h"
#include "machine/rescap.h"
#include "sound/dac.h"
#include "sound/flt_biquad.h"
#include "sound/hc55516.h"
#include "sound/ymopm.h"

// 6802/8 CPU's input clock is 4MHz
// but because it has an internal /4 divider, its E clock runs at 1/4 that frequency
#define E_CLOCK (XTAL(4'000'000)/4)

// Length of time in cycles between IRQs on the main 6808 CPU
// This length is determined by the settings of the W14 and W15 jumpers
// IRQ pulse width is always 32 cycles
// All machines I've looked at so far have W14 present and W15 absent
// which makes the timer int fire every 0x380 E-clocks (1MHz/0x380, ~1.116KHz)
// It is possible to have W15 present and W14 absent instead,
// which makes the timer fire every 0x700 E-clocks (1MHz/0x700, ~558Hz)
// but I am unaware of any games which make use of this feature.
// define the define below to enable the W15-instead-of-W14 feature.
#undef S11_W15

class s11_state : public genpin_class
{
public:
	s11_state(const machine_config &mconfig, device_type type, const char *tag)
		: genpin_class(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_mainirq(*this, "mainirq")
		, m_piairq(*this, "piairq")
		, m_audiocpu(*this, "audiocpu")
		, m_audioirq(*this, "audioirq")
		, m_hc55516(*this, "hc55516")
		, m_cvsd_filter(*this, "cvsd_filter")
		, m_cvsd_filter2(*this, "cvsd_filter2")
		, m_dac(*this, "dac")
		, m_pias(*this, "pias")
		, m_pia21(*this, "pia21")
		, m_pia24(*this, "pia24")
		, m_pia28(*this, "pia28")
		, m_pia2c(*this, "pia2c")
		, m_pia30(*this, "pia30")
		, m_pia34(*this, "pia34")
		, m_bg(*this, "bg")
		, m_ps88(*this, "ps88")
		, m_digits(*this, "digit%u", 0U)
		, m_swarray(*this, "SW.%u", 0U)
		, m_timer_irq_active(false)
		, m_pia_irq_active(false)
		{ }

	void s11(machine_config &config);
	void s11_only(machine_config &config);
	void s11_bgs(machine_config &config);
	void s11_bgm(machine_config &config);

	void init_s11();

	DECLARE_INPUT_CHANGED_MEMBER(main_nmi);
	DECLARE_INPUT_CHANGED_MEMBER(audio_nmi);

protected:

	u8 sound_r();
	void bank_w(u8 data);
	void dig1_w(u8 data);
	void lamp0_w(u8 data);
	void lamp1_w(u8 data) { }
	void sol2_w(u8 data) { } // solenoids 8-15
	void sol3_w(u8 data); // solenoids 0-7
	void sound_w(u8 data);

	void pia2c_pa_w(u8 data);
	void pia2c_pb_w(u8 data);
	void pia34_pa_w(u8 data);
	void pia34_pb_w(u8 data);
	DECLARE_WRITE_LINE_MEMBER(pia34_cb2_w);

	DECLARE_WRITE_LINE_MEMBER(pias_ca2_w);
	DECLARE_WRITE_LINE_MEMBER(pias_cb2_w);
	DECLARE_WRITE_LINE_MEMBER(pia21_ca2_w);
	DECLARE_WRITE_LINE_MEMBER(pia21_cb2_w) { } // enable solenoids
	DECLARE_WRITE_LINE_MEMBER(pia24_cb2_w) { } // dummy to stop error log filling up
	DECLARE_WRITE_LINE_MEMBER(pia28_ca2_w) { } // comma3&4
	DECLARE_WRITE_LINE_MEMBER(pia28_cb2_w) { } // comma1&2
	DECLARE_WRITE_LINE_MEMBER(pia30_cb2_w) { } // dummy to stop error log filling up
	DECLARE_WRITE_LINE_MEMBER(pia_irq);
	DECLARE_WRITE_LINE_MEMBER(main_irq);

	u8 switch_r();
	void switch_w(u8 data);
	u8 pia28_w7_r();

	void s11_main_map(address_map &map);
	void s11_audio_map(address_map &map);

	virtual void machine_start() override { m_digits.resolve(); }
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param) override;
	virtual void machine_reset() override;

	// devices
	required_device<cpu_device> m_maincpu;
	required_device<input_merger_device> m_mainirq;
	required_device<input_merger_device> m_piairq;
	// the following devices are optional because certain board variants (i.e. system 11c) do not have the audio section on the mainboard populated
	optional_device<m6802_cpu_device> m_audiocpu;
	optional_device<input_merger_device> m_audioirq;
	optional_device<hc55516_device> m_hc55516;
	optional_device<filter_biquad_device> m_cvsd_filter;
	optional_device<filter_biquad_device> m_cvsd_filter2;
	optional_device<mc1408_device> m_dac;
	optional_device<pia6821_device> m_pias;
	required_device<pia6821_device> m_pia21;
	required_device<pia6821_device> m_pia24;
	required_device<pia6821_device> m_pia28;
	required_device<pia6821_device> m_pia2c;
	required_device<pia6821_device> m_pia30;
	required_device<pia6821_device> m_pia34;
	optional_device<s11c_bg_device> m_bg;
	optional_device<pinsnd88_device> m_ps88;
	output_finder<63> m_digits;
	required_ioport_array<8> m_swarray;

	// getters/setters
	u8 get_strobe() { return m_strobe; }
	void set_strobe(u8 s) { m_strobe = s; }
	u8 get_diag() { return m_diag; }
	void set_diag(u8 d) { m_diag = d; }
	uint32_t get_segment1() { return m_segment1; }
	void set_segment1(uint32_t s) { m_segment1 = s; }
	uint32_t get_segment2() { return m_segment2; }
	void set_segment2(uint32_t s) { m_segment2 = s; }
	void set_timer(emu_timer* t) { m_irq_timer = t; }

	static const device_timer_id TIMER_IRQ = 0;

private:
	void dig0_w(u8 data);
	u8 m_sound_data;
	u8 m_strobe;
	u8 m_switch_col;
	u8 m_diag;
	uint32_t m_segment1;
	uint32_t m_segment2;
	uint32_t m_timer_count;
	emu_timer* m_irq_timer;
	bool m_timer_irq_active;
	bool m_pia_irq_active;
};


class s11a_state : public s11_state
{
public:
	s11a_state(const machine_config &mconfig, device_type type, const char *tag)
		: s11_state(mconfig, type, tag)
	{ }

	void s11a_base(machine_config &config);
	void s11a(machine_config &config);
	void s11a_obg(machine_config &config);

	void init_s11a();

protected:
	void s11a_dig0_w(u8 data);
};


class s11b_state : public s11a_state
{
public:
	s11b_state(const machine_config &mconfig, device_type type, const char *tag)
		: s11a_state(mconfig, type, tag)
	{ }

	void s11b_base(machine_config &config);
	void s11b(machine_config &config);
	void s11b_jokerz(machine_config &config);

	void init_s11b();
	void init_s11b_invert();

protected:
	virtual void machine_reset() override;
	void set_invert(bool inv) { m_invert = inv; }

	void s11b_dig1_w(u8 data);
	void s11b_pia2c_pa_w(u8 data);
	void s11b_pia2c_pb_w(u8 data);
	void s11b_pia34_pa_w(u8 data);

private:
	bool m_invert;  // later System 11B games start expecting inverted data to the display LED segments.
};


class s11c_state : public s11b_state
{
public:
	s11c_state(const machine_config &mconfig, device_type type, const char *tag)
		: s11b_state(mconfig, type, tag)
	{ }

	void s11c(machine_config &config);

	void init_s11c();

protected:
	virtual void machine_reset() override;
};


#endif // MAME_INCLUDES_S11_H
