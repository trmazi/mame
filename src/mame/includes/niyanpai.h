// license:BSD-3-Clause
// copyright-holders:Takahiro Nogi

#include "machine/tmp68301.h"
#include "screen.h"
#include "audio/nichisnd.h"
#include "machine/nb1413m3.h"
#include "emupal.h"

#define VRAM_MAX    3

class niyanpai_state : public driver_device
{
public:
	niyanpai_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) ,
		m_maincpu(*this, "maincpu"),
		m_screen(*this, "screen"),
		m_palette(*this, "palette") { }

	void musobana(machine_config &config);
	void zokumahj(machine_config &config);
	void mhhonban(machine_config &config);
	void niyanpai(machine_config &config);

	void init_niyanpai();

	DECLARE_READ_LINE_MEMBER(musobana_outcoin_flag_r);

private:
	enum
	{
		TIMER_BLITTER
	};

	required_device<tmp68301_device> m_maincpu;
	required_device<screen_device> m_screen;
	required_device<palette_device> m_palette;

	// common
	int m_scrollx[VRAM_MAX];
	int m_scrolly[VRAM_MAX];
	int m_blitter_destx[VRAM_MAX];
	int m_blitter_desty[VRAM_MAX];
	int m_blitter_sizex[VRAM_MAX];
	int m_blitter_sizey[VRAM_MAX];
	int m_blitter_src_addr[VRAM_MAX];
	int m_blitter_direction_x[VRAM_MAX];
	int m_blitter_direction_y[VRAM_MAX];
	int m_dispflag[VRAM_MAX];
	int m_flipscreen[VRAM_MAX];
	int m_clutmode[VRAM_MAX];
	int m_transparency[VRAM_MAX];
	int m_clutsel[VRAM_MAX];
	int m_screen_refresh;
	int m_nb19010_busyctr;
	int m_nb19010_busyflag;
	bitmap_ind16 m_tmpbitmap[VRAM_MAX];
	std::unique_ptr<uint16_t[]> m_videoram[VRAM_MAX];
	std::unique_ptr<uint16_t[]> m_videoworkram[VRAM_MAX];
	std::unique_ptr<uint16_t[]> m_palette_ptr;
	std::unique_ptr<uint8_t[]> m_clut[VRAM_MAX];
	int m_flipscreen_old[VRAM_MAX];
	emu_timer *m_blitter_timer;

	// musobana and derived machine configs
	int m_musobana_inputport;
	int m_musobana_outcoin_flag;
	uint8_t m_motor_on;

	// common
	uint16_t dipsw_r();
	uint16_t palette_r(offs_t offset);
	void palette_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);
	void blitter_0_w(offs_t offset, uint8_t data);
	void blitter_1_w(offs_t offset, uint8_t data);
	void blitter_2_w(offs_t offset, uint8_t data);
	uint8_t blitter_0_r(offs_t offset);
	uint8_t blitter_1_r(offs_t offset);
	uint8_t blitter_2_r(offs_t offset);
	void clut_0_w(offs_t offset, uint8_t data);
	void clut_1_w(offs_t offset, uint8_t data);
	void clut_2_w(offs_t offset, uint8_t data);
	void clutsel_0_w(uint8_t data);
	void clutsel_1_w(uint8_t data);
	void clutsel_2_w(uint8_t data);
	void tmp68301_parallel_port_w(uint16_t data);

	// musobana and derived machine configs
	uint16_t musobana_inputport_0_r();
	void musobana_inputport_w(uint16_t data);

	virtual void video_start() override;
	DECLARE_MACHINE_START(musobana);

	uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	int blitter_r(int vram, int offset);
	void blitter_w(int vram, int offset, uint8_t data);
	void clutsel_w(int vram, uint8_t data);
	void clut_w(int vram, int offset, uint8_t data);
	void vramflip(int vram);
	void update_pixel(int vram, int x, int y);
	void gfxdraw(int vram);

	DECLARE_WRITE_LINE_MEMBER(vblank_irq);

	void mhhonban_map(address_map &map);
	void musobana_map(address_map &map);
	void niyanpai_map(address_map &map);
	void zokumahj_map(address_map &map);

	virtual void device_timer(emu_timer &timer, device_timer_id id, int param) override;
};
