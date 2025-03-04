// license:BSD-3-Clause
// copyright-holders:Nicola Salmoria
/***************************************************************************

  mrdo.cpp

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "emu.h"
#include "includes/mrdo.h"


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Mr. Do! has two 32 bytes palette PROM and a 32 bytes sprite color lookup
  table PROM.
  The palette PROMs are connected to the RGB output this way:

  U2:
  bit 7 -- unused
        -- unused
        -- 100 ohm resistor  -diode- BLUE
        --  75 ohm resistor  -diode- BLUE
        -- 100 ohm resistor  -diode- GREEN
        --  75 ohm resistor  -diode- GREEN
        -- 100 ohm resistor  -diode- RED
  bit 0 --  75 ohm resistor  -diode- RED

  T2:
  bit 7 -- unused
        -- unused
        -- 150 ohm resistor  -diode- BLUE
        -- 120 ohm resistor  -diode- BLUE
        -- 150 ohm resistor  -diode- GREEN
        -- 120 ohm resistor  -diode- GREEN
        -- 150 ohm resistor  -diode- RED
  bit 0 -- 120 ohm resistor  -diode- RED

  200 ohm pulldown on all three components

***************************************************************************/

void mrdo_state::mrdo_palette(palette_device &palette) const
{
	constexpr int R1 = 150;
	constexpr int R2 = 120;
	constexpr int R3 = 100;
	constexpr int R4 = 75;
	constexpr int pull = 220;
	constexpr float potadjust = 0.7f;   /* diode voltage drop */

	float pot[16];
	int weight[16];
	for (int i = 0x0f; i >= 0; i--)
	{
		float par = 0;

		if (i & 1) par += 1.0f / float(R1);
		if (i & 2) par += 1.0f / float(R2);
		if (i & 4) par += 1.0f / float(R3);
		if (i & 8) par += 1.0f / float(R4);
		if (par)
		{
			par = 1 / par;
			pot[i] = pull/(pull+par) - potadjust;
		}
		else
			pot[i] = 0;

		weight[i] = 0xff * pot[i] / pot[0x0f];
		if (weight[i] < 0)
			weight[i] = 0;
	}

	const uint8_t *color_prom = memregion("proms")->base();

	for (int i = 0; i < 0x100; i++)
	{
		int bits0, bits2;

		int const a1 = ((i >> 3) & 0x1c) + (i & 0x03) + 0x20;
		int const a2 = ((i >> 0) & 0x1c) + (i & 0x03);

		// red component
		bits0 = (color_prom[a1] >> 0) & 0x03;
		bits2 = (color_prom[a2] >> 0) & 0x03;
		int const r = weight[bits0 + (bits2 << 2)];

		// green component
		bits0 = (color_prom[a1] >> 2) & 0x03;
		bits2 = (color_prom[a2] >> 2) & 0x03;
		int const g = weight[bits0 + (bits2 << 2)];

		// blue component
		bits0 = (color_prom[a1] >> 4) & 0x03;
		bits2 = (color_prom[a2] >> 4) & 0x03;
		int const b = weight[bits0 + (bits2 << 2)];

		palette.set_indirect_color(i, rgb_t(r, g, b));
	}

	// color_prom now points to the beginning of the lookup table
	color_prom += 0x40;

	// characters
	for (int i = 0; i < 0x100; i++)
		palette.set_pen_indirect(i, i);

	// sprites
	for (int i = 0; i < 0x40; i++)
	{
		uint8_t ctabentry = color_prom[i & 0x1f];

		if (i & 0x20)
			ctabentry >>= 4;    // high 4 bits are for sprite color n + 8
		else
			ctabentry &= 0x0f;  // low 4 bits are for sprite color n

		palette.set_pen_indirect(i + 0x100, ctabentry + ((ctabentry & 0x0c) << 3));
	}
}



/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

TILE_GET_INFO_MEMBER(mrdo_state::get_bg_tile_info)
{
	uint8_t attr = m_bgvideoram[tile_index];
	tileinfo.set(1,
			m_bgvideoram[tile_index + 0x400] + ((attr & 0x80) << 1),
			attr & 0x3f,
			(attr & 0x40) ? TILE_FORCE_LAYER0 : 0);
}

TILE_GET_INFO_MEMBER(mrdo_state::get_fg_tile_info)
{
	uint8_t attr = m_fgvideoram[tile_index];
	tileinfo.set(0,
			m_fgvideoram[tile_index+0x400] + ((attr & 0x80) << 1),
			attr & 0x3f,
			(attr & 0x40) ? TILE_FORCE_LAYER0 : 0);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

void mrdo_state::video_start()
{
	m_bg_tilemap = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(mrdo_state::get_bg_tile_info)), TILEMAP_SCAN_ROWS, 8,8, 32,32);
	m_fg_tilemap = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(mrdo_state::get_fg_tile_info)), TILEMAP_SCAN_ROWS, 8,8, 32,32);

	m_bg_tilemap->set_transparent_pen(0);
	m_fg_tilemap->set_transparent_pen(0);

	m_flipscreen = 0;

	save_item(NAME(m_flipscreen));
}



/***************************************************************************

  Memory handlers

***************************************************************************/

void mrdo_state::mrdo_bgvideoram_w(offs_t offset, uint8_t data)
{
	m_bgvideoram[offset] = data;
	m_bg_tilemap->mark_tile_dirty(offset & 0x3ff);
}

/* PAL16R6CN used for protection. The game doesn't clear the screen
   if a read from this address doesn't return the value it expects. */
uint8_t mrdo_state::mrdo_secre_r()
{
	return m_pal_u001;
}

void mrdo_state::mrdo_fgvideoram_w(offs_t offset, uint8_t data)
{
	m_fgvideoram[offset] = data;
	m_fg_tilemap->mark_tile_dirty(offset & 0x3ff);

	// protection.  each write latches a new value on IC u001 (PAL16R6)
	const uint8_t i9 = BIT(data,0);
	const uint8_t i8 = BIT(data,1);
//  const uint8_t i7 = BIT(data,2); pin 7 not used in equations
	const uint8_t i6 = BIT(data,3);
	const uint8_t i5 = BIT(data,4);
	const uint8_t i4 = BIT(data,5);
	const uint8_t i3 = BIT(data,6);
	const uint8_t i2 = BIT(data,7);

	// equations extracted from dump using jedutil
	const uint8_t t1 =    i2  & (1^i3) &    i4  & (1^i5) & (1^i6) & (1^i8) &    i9;
	const uint8_t t2 = (1^i2) & (1^i3) &    i4  &    i5  & (1^i6) &    i8  & (1^i9);
	const uint8_t t3 =    i2  &    i3  & (1^i4) & (1^i5) &    i6  & (1^i8) &    i9;
	const uint8_t t4 = (1^i2) &    i3  &    i4  & (1^i5) &    i6  &    i8  &    i9;

	const uint8_t r12 = 0;
	const uint8_t r13 = (1^( t1 ))      << 1 ;
	const uint8_t r14 = (1^( t1 | t2 )) << 2 ;
	const uint8_t r15 = (1^( t1 | t3 )) << 3 ;
	const uint8_t r16 = (1^( t1 ))      << 4 ;
	const uint8_t r17 = (1^( t1 | t3 )) << 5 ;
	const uint8_t r18 = (1^( t3 | t4 )) << 6 ;
	const uint8_t r19 = 0;

	m_pal_u001 = r19 | r18 | r17 | r16 | r15 | r14 | r13 | r12  ;
}

/*
/rf13 :=                                             i2 & /i3 & i4 & /i5 & /i6 & /i8 & i9  t1

/rf14 := /i2 & /i3 & i4 & i5 & /i6 & i8 & /i9   +    i2 & /i3 & i4 & /i5 & /i6 & /i8 & i9  t2 | t1

/rf15 := i2 & i3 & /i4 & /i5 & i6 & /i8 & i9    +    i2 & /i3 & i4 & /i5 & /i6 & /i8 & i9  t3 | t1

/rf16 :=                                             i2 & /i3 & i4 & /i5 & /i6 & /i8 & i9  t1

/rf17 := i2 & i3 & /i4 & /i5 & i6 & /i8 & i9    +    i2 & /i3 & i4 & /i5 & /i6 & /i8 & i9  t3 | t1

/rf18 := /i2 & i3 & i4 & /i5 & i6 & i8 & i9     +    i2 & i3 & /i4 & /i5 & i6 & /i8 & i9   t4 | t2
*/
void mrdo_state::mrdo_scrollx_w(uint8_t data)
{
	m_bg_tilemap->set_scrollx(0, data);
}

void mrdo_state::mrdo_scrolly_w(uint8_t data)
{
	/* This is NOT affected by flipscreen (so stop it happening) */
	if (m_flipscreen)
		m_bg_tilemap->set_scrolly(0,((256 - data) & 0xff));
	else
		m_bg_tilemap->set_scrolly(0, data);
}


void mrdo_state::mrdo_flipscreen_w(uint8_t data)
{
	/* bits 1-3 control the playfield priority, but they are not used by */
	/* Mr. Do! so we don't emulate them */

	m_flipscreen = data & 0x01;
	machine().tilemap().set_flip_all(m_flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
}



/***************************************************************************

  Display refresh

***************************************************************************/

void mrdo_state::draw_sprites( bitmap_ind16 &bitmap,const rectangle &cliprect )
{
	uint8_t *spriteram = m_spriteram;
	int offs;

	for (offs = m_spriteram.bytes() - 4; offs >= 0; offs -= 4)
	{
		if (spriteram[offs + 1] != 0)
		{
			m_gfxdecode->gfx(2)->transpen(bitmap,cliprect,
					spriteram[offs], spriteram[offs + 2] & 0x0f,
					spriteram[offs + 2] & 0x10, spriteram[offs + 2] & 0x20,
					spriteram[offs + 3], 256 - spriteram[offs + 1], 0);
		}
	}
}

uint32_t mrdo_state::screen_update_mrdo(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(0, cliprect);
	m_bg_tilemap->draw(screen, bitmap, cliprect, 0, 0);
	m_fg_tilemap->draw(screen, bitmap, cliprect, 0, 0);
	draw_sprites(bitmap, cliprect);
	return 0;
}
