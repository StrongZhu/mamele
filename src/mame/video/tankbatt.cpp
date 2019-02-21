// license:BSD-3-Clause
// copyright-holders:Brad Oliver
/***************************************************************************

  tankbatt.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "emu.h"
#include "includes/tankbatt.h"


/***************************************************************************

  Convert the color PROMs into a more useable format.

***************************************************************************/
void tankbatt_state::tankbatt_palette(palette_device &palette) const
{
	uint8_t const *const color_prom = memregion("proms")->base();

	constexpr int RES_1 = 0xc0; // this is a guess
	constexpr int RES_2 = 0x3f; // this is a guess

	// create a lookup table for the palette
	for (int i = 0; i < 0x100; i++)
	{
		int const bit0 = BIT(color_prom[i], 0); // intensity
		int const bit1 = BIT(color_prom[i], 1); // red
		int const bit2 = BIT(color_prom[i], 2); // green
		int const bit3 = BIT(color_prom[i], 3); // blue

		// red component
		int const r = bit1 * (RES_1 + (RES_2 * bit0));

		// green component
		int const g = bit2 * (RES_1 + (RES_2 * bit0));

		// blue component
		int const b = bit3 * (RES_1 + (RES_2 * bit0));

		palette.set_indirect_color(i, rgb_t(r, g, b));
	}

	for (int i = 0; i < 0x200; i += 2)
	{
		palette.set_pen_indirect(i + 0, 0);
		palette.set_pen_indirect(i + 1, i >> 1);
	}
}

WRITE8_MEMBER(tankbatt_state::videoram_w)
{
	m_videoram[offset] = data;
	m_bg_tilemap->mark_tile_dirty(offset);
}

TILE_GET_INFO_MEMBER(tankbatt_state::get_bg_tile_info)
{
	int code = m_videoram[tile_index];
	int color = m_videoram[tile_index] | 0x01;

	SET_TILE_INFO_MEMBER(0, code, color, 0);
}

void tankbatt_state::video_start()
{
	m_bg_tilemap = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(FUNC(tankbatt_state::get_bg_tile_info),this), TILEMAP_SCAN_ROWS, 8, 8, 32, 32);
}

void tankbatt_state::draw_bullets(bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	for (int offs = 0;offs < m_bulletsram.bytes();offs += 2)
	{
		int color = 0xff;   /* cyan, same color as the tanks */
		int x = m_bulletsram[offs + 1];
		int y = 255 - m_bulletsram[offs] - 2;

		m_gfxdecode->gfx(1)->opaque(bitmap,cliprect,
			0,  /* this is just a square, generated by the hardware */
			color,
			0,0,
			x,y);
	}
}

uint32_t tankbatt_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	m_bg_tilemap->draw(screen, bitmap, cliprect, 0, 0);
	draw_bullets(bitmap, cliprect);
	return 0;
}
