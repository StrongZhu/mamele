// license:BSD-3-Clause
// copyright-holders:Nicola Salmoria
/***************************************************************************

Universal 8106-A2 + 8106-B PCB set

driver by Nicola Salmoria

  Confirmed to use the 8106-A2/8106-B PCB:
    Lady Bug
    Snap Jack
    Cosmic Avenger

  Similar or bootleg PCB:
    Dorodon (by Falcon)
    Space Raider

Memory map (preliminary)

0000-5fff ROM
6000-6fff RAM
d000-d3ff video RAM
          d000-d007/d020-d027/d040-d047/d060-d067 contain the column scroll
          registers (not used by Lady Bug)
d400-d7ff color RAM (4 bits wide)

memory mapped ports:

read:
9000      IN0
9001      IN1
9002      DSW0
9003      DSW1
e000      IN2 (not used by Lady Bug)
8000      interrupt enable? (toggle)?
see the input_ports definition below for details on the input bits

write:
7000-73ff sprites
a000      flip screen
b000      sound port 1
c000      sound port 2

interrupts:
There is no vblank interrupt. The vblank status is read from IN1.
Coin insertion in left slot generates a NMI, in right slot an IRQ.

TODO:
- Coin lockouts are missing. The game only accepts 9 coins, so there has to be
  a lockout call somewhere.

***************************************************************************/

/*
 * Space Raider todo list:
 *
 * decode cpu#2 writes to port 0x30 and 0x38 - resistors for sound
 * decode cpu#2 writes to port 0x28-0x2f - ???
 * examine other bits from cpu#2 write to 0xe800
 * one unknown dip
 */

#include "emu.h"
#include "includes/ladybug.h"

#include "cpu/z80/z80.h"
#include "machine/74259.h"
#include "sound/sn76496.h"
#include "screen.h"
#include "speaker.h"

#include <algorithm>


// Protection?
READ8_MEMBER(sraider_state::sraider_8005_r)
{
	// This must return X011111X or cpu #1 will hang
	// see code at rst $10
	return 0x3e;
}

// Unknown IO
WRITE8_MEMBER(sraider_state::sraider_misc_w)
{
	switch(offset)
	{
	// These 8 bits are stored in the latch at A7
	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
		m_weird_value[offset & 7] = data & 1;
		break;
	// These 6 bits are stored in the latch at N7
	case 0x08:
		m_sraider_0x30 = data & 0x3f;
		break;
	// These 6 bits are stored in the latch at N8
	case 0x10:
		m_sraider_0x38 = data & 0x3f;
		break;
	default:
		logerror("(%s) write to %02X\n", machine().describe_context(), offset);
		break;
	}
}


void ladybug_state::ladybug_map(address_map &map)
{
	map(0x0000, 0x5fff).rom();
	map(0x6000, 0x6fff).ram();
	map(0x7000, 0x73ff).w(m_video, FUNC(ladybug_video_device::spr_w));
	map(0x8000, 0x8fff).nopr();
	map(0x9000, 0x9000).portr("IN0");
	map(0x9001, 0x9001).portr("IN1");
	map(0x9002, 0x9002).portr("DSW0");
	map(0x9003, 0x9003).portr("DSW1");
	map(0xa000, 0xa007).w("videolatch", FUNC(ls259_device::write_d0));
	map(0xb000, 0xbfff).w("sn1", FUNC(sn76489_device::command_w));
	map(0xc000, 0xcfff).w("sn2", FUNC(sn76489_device::command_w));
	map(0xd000, 0xd7ff).rw(m_video, FUNC(ladybug_video_device::bg_r), FUNC(ladybug_video_device::bg_w));
	map(0xe000, 0xe000).portr("IN2");
}


void dorodon_state::decrypted_opcodes_map(address_map &map)
{
	map(0x0000, 0x5fff).rom().share("decrypted_opcodes");
}


void sraider_state::sraider_cpu1_map(address_map &map)
{
	map(0x0000, 0x5fff).rom();
	map(0x6000, 0x6fff).ram();
	map(0x7000, 0x73ff).w("video", FUNC(ladybug_video_device::spr_w));
	map(0x8005, 0x8005).r(FUNC(sraider_state::sraider_8005_r));  // protection check?
	map(0x8006, 0x8006).writeonly().share("sound_low");
	map(0x8007, 0x8007).writeonly().share("sound_high");
	map(0x9000, 0x9000).portr("IN0");
	map(0x9001, 0x9001).portr("IN1");
	map(0x9002, 0x9002).portr("DSW0");
	map(0x9003, 0x9003).portr("DSW1");
	map(0xd000, 0xd7ff).w("video", FUNC(ladybug_video_device::bg_w));
	map(0xe000, 0xe000).nopw();  //unknown 0x10 when in attract, 0x20 when coined/playing
}

void sraider_state::sraider_cpu2_map(address_map &map)
{
	map(0x0000, 0x5fff).rom();
	map(0x6000, 0x63ff).ram();
	map(0x8000, 0x8000).readonly().share("sound_low");
	map(0xa000, 0xa000).readonly().share("sound_high");
	map(0xc000, 0xc000).nopr(); //some kind of sync
	map(0xe000, 0xe0ff).writeonly().share("grid_data");
	map(0xe800, 0xe800).w(FUNC(sraider_state::sraider_io_w));
}

void sraider_state::sraider_cpu2_io_map(address_map &map)
{
	map.global_mask(0xff);
	map(0x00, 0x00).w("sn1", FUNC(sn76489_device::command_w));
	map(0x08, 0x08).w("sn2", FUNC(sn76489_device::command_w));
	map(0x10, 0x10).w("sn3", FUNC(sn76489_device::command_w));
	map(0x18, 0x18).w("sn4", FUNC(sn76489_device::command_w));
	map(0x20, 0x20).w("sn5", FUNC(sn76489_device::command_w));
	map(0x28, 0x3f).w(FUNC(sraider_state::sraider_misc_w));  // lots unknown
}


INPUT_CHANGED_MEMBER(ladybug_state::coin1_inserted)
{
	/* left coin insertion causes an NMI */
	m_maincpu->set_input_line(INPUT_LINE_NMI, newval ? ASSERT_LINE : CLEAR_LINE);
}

INPUT_CHANGED_MEMBER(ladybug_state::coin2_inserted)
{
	/* right coin insertion causes an IRQ */
	if (newval)
		m_maincpu->set_input_line(0, HOLD_LINE);
}


CUSTOM_INPUT_MEMBER(ladybug_state::ladybug_p1_control_r)
{
	return m_p1_control->read();
}

CUSTOM_INPUT_MEMBER(ladybug_state::ladybug_p2_control_r)
{
	// upright cabinet only uses a single set of controls */
	return ((m_port_dsw0->read() & 0x20) ? m_p2_control : m_p1_control)->read();
}


static INPUT_PORTS_START( ladybug )
	PORT_START("IN0")
	PORT_BIT( 0x1f, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_CUSTOM_MEMBER(DEVICE_SELF, ladybug_state, ladybug_p1_control_r, nullptr)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START("IN1")
	PORT_BIT( 0x1f, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_CUSTOM_MEMBER(DEVICE_SELF, ladybug_state, ladybug_p2_control_r, nullptr)
	// This should be connected to the 4V clock. I don't think the game uses it.
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	// Note that there are TWO VBlank inputs, one is active low, the other active
	// high. There are probably other differencies in the hardware, but emulating
	// them this way is enough to get the game running.
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_CUSTOM ) PORT_VBLANK("screen")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_VBLANK("screen")

	PORT_START("IN2")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("DSW0")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x04, 0x04, "High Score Names" )
	PORT_DIPSETTING(    0x00, "3 Letters" )
	PORT_DIPSETTING(    0x04, "10 Letters" )
	PORT_DIPNAME( 0x08, 0x08, "Rack Test (Cheat)" ) PORT_CODE(KEYCODE_F1)
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Freeze" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x80, "3" )
	PORT_DIPSETTING(    0x00, "5" )

	PORT_START("DSW1")
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	/* settings 0x00 through 0x05 all give 1 Coin/1 Credit */
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	/* settings 0x00 through 0x50 all give 1 Coin/1 Credit */

	PORT_START("COIN")  /* FAKE */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_CHANGED_MEMBER(DEVICE_SELF, ladybug_state,coin1_inserted, 0)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_CHANGED_MEMBER(DEVICE_SELF, ladybug_state,coin2_inserted, 0)

	PORT_START("CONTP1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("CONTP2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static INPUT_PORTS_START( snapjack )
	PORT_START("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	/* This should be connected to the 4V clock. I don't think the game uses it. */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* Note that there are TWO VBlank inputs, one is active low, the other active */
	/* high. There are probably other differencies in the hardware, but emulating */
	/* them this way is enough to get the game running. */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_CUSTOM ) PORT_VBLANK("screen")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_VBLANK("screen")

	PORT_START("IN2")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("DSW0")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x04, 0x04, "High Score Names" )
	PORT_DIPSETTING(    0x00, "3 Letters" )
	PORT_DIPSETTING(    0x04, "10 Letters" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x10, 0x00, "unused1?" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "unused2?" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x40, "5" )

	PORT_START("DSW1")
	/* coinage is slightly different from Lady Bug and Cosmic Avenger */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	/* settings 0x00 through 0x04 all give 1 Coin/1 Credit */
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	/* settings 0x00 through 0x04 all give 1 Coin/1 Credit */

	PORT_START("COIN")  /* FAKE */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_CHANGED_MEMBER(DEVICE_SELF, ladybug_state,coin1_inserted, 0)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_CHANGED_MEMBER(DEVICE_SELF, ladybug_state,coin2_inserted, 0)
INPUT_PORTS_END

static INPUT_PORTS_START( cavenger )
	PORT_START("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	/* This should be connected to the 4V clock. I don't think the game uses it. */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* Note that there are TWO VBlank inputs, one is active low, the other active */
	/* high. There are probably other differencies in the hardware, but emulating */
	/* them this way is enough to get the game running. */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_CUSTOM ) PORT_VBLANK("screen")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_VBLANK("screen")

	PORT_START("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("DSW0")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x04, 0x04, "High Score Names" )
	PORT_DIPSETTING(    0x00, "3 Letters" )
	PORT_DIPSETTING(    0x04, "10 Letters" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x30, 0x00, "Initial High Score" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x30, "5000" )
	PORT_DIPSETTING(    0x20, "8000" )
	PORT_DIPSETTING(    0x10, "10000" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x40, "5" )

	PORT_START("DSW1")
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	/* settings 0x00 through 0x05 all give 1 Coin/1 Credit */
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	/* settings 0x00 through 0x50 all give 1 Coin/1 Credit */

	PORT_START("COIN")  /* FAKE */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_CHANGED_MEMBER(DEVICE_SELF, ladybug_state,coin1_inserted, 0)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_CHANGED_MEMBER(DEVICE_SELF, ladybug_state,coin2_inserted, 0)
INPUT_PORTS_END

static INPUT_PORTS_START( dorodon )
	PORT_START("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	/* This should be connected to the 4V clock. I don't think the game uses it. */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* Note that there are TWO VBlank inputs, one is active low, the other active */
	/* high. There are probably other differencies in the hardware, but emulating */
	/* them this way is enough to get the game running. */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_CUSTOM ) PORT_VBLANK("screen")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_VBLANK("screen")

	PORT_START("IN2")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("DSW0")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "20000" )
	PORT_DIPSETTING(    0x00, "30000" )
	PORT_DIPNAME( 0x08, 0x08, "Rack Test (Cheat)" ) PORT_CODE(KEYCODE_F1)
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Freeze" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x80, "3" )
	PORT_DIPSETTING(    0x00, "5" )

	PORT_START("DSW1")
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	/* settings 0x00 through 0x05 all give 1 Coin/1 Credit */
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	/* settings 0x00 through 0x50 all give 1 Coin/1 Credit */

	PORT_START("COIN")  /* FAKE */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_CHANGED_MEMBER(DEVICE_SELF, ladybug_state,coin1_inserted, 0)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_CHANGED_MEMBER(DEVICE_SELF, ladybug_state,coin2_inserted, 0)
INPUT_PORTS_END

static INPUT_PORTS_START( sraider )
	PORT_START("IN0")   /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN1")   /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START("DSW0")  /* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x04, 0x04, "High Score Names" )
	PORT_DIPSETTING(    0x00, "3 Letters" )
	PORT_DIPSETTING(    0x04, "10 Letters" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x08, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x40, "5" )

	/* Free Play setting works when it's set for both */
	PORT_START("DSW1")  /* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	/* settings 0x00 through 0x05 all give 1 Coin/1 Credit */
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	/* settings 0x00 through 0x50 all give 1 Coin/1 Credit */
	PORT_DIPSETTING(    0x60, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
INPUT_PORTS_END


static const gfx_layout charlayout =
{
	8,8,    /* 8*8 characters */
	512,    /* 512 characters */
	2,  /* 2 bits per pixel */
	{ 0, 512*8*8 }, /* the two bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};
static const gfx_layout charlayout2 =
{
	8,8,    /* 8*8 characters */
	512,    /* 512 characters */
	2,  /* 2 bits per pixel */
	{ 0, 512*8*8 }, /* the two bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};

static const gfx_layout spritelayout =
{
	16,16,  /* 16*16 sprites */
	128,    /* 128 sprites */
	2,  /* 2 bits per pixel */
	{ 1, 0 },   /* the two bitplanes are packed in two consecutive bits */
	{ 0, 2, 4, 6, 8, 10, 12, 14,
			8*16+0, 8*16+2, 8*16+4, 8*16+6, 8*16+8, 8*16+10, 8*16+12, 8*16+14 },
	{ 23*16, 22*16, 21*16, 20*16, 19*16, 18*16, 17*16, 16*16,
			7*16, 6*16, 5*16, 4*16, 3*16, 2*16, 1*16, 0*16 },
	64*8    /* every sprite takes 64 consecutive bytes */
};
static const gfx_layout spritelayout2 =
{
	8,8,    /* 8*8 sprites */
	512,    /* 512 sprites */
	2,  /* 2 bits per pixel */
	{ 1, 0 },   /* the two bitplanes are packed in two consecutive bits */
	{ 0, 2, 4, 6, 8, 10, 12, 14 },
	{ 7*16, 6*16, 5*16, 4*16, 3*16, 2*16, 1*16, 0*16 },
	16*8    /* every sprite takes 16 consecutive bytes */
};

static const gfx_layout gridlayout =
{
	8,8,    /* 8*8 characters */
	512,    /* 512 characters */
	1,  /* 1 bit per pixel */
	{ 0 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
	8*8 /* every char takes 8 consecutive bytes */
};
static const gfx_layout gridlayout2 =
{
	8,8,    /* 8*8 characters */
	512,    /* 512 characters */
	1,  /* 1 bit per pixel */
	{ 0 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	8*8 /* every char takes 8 consecutive bytes */
};

static GFXDECODE_START( gfx_ladybug )
	GFXDECODE_ENTRY( "gfx1", 0, charlayout,      0,  8 )
	GFXDECODE_ENTRY( "gfx2", 0, spritelayout,  4*8, 16 )
	GFXDECODE_ENTRY( "gfx2", 0, spritelayout2, 4*8, 16 )
GFXDECODE_END

static GFXDECODE_START( gfx_sraider )
	GFXDECODE_ENTRY( "gfx1", 0, charlayout2,             0,  8 )
	GFXDECODE_ENTRY( "gfx2", 0, spritelayout,          4*8, 16 )
	GFXDECODE_ENTRY( "gfx2", 0, spritelayout2,         4*8, 16 )
	GFXDECODE_ENTRY( "gfx3", 0, gridlayout,    4*8+4*16+32,  1 )
	GFXDECODE_ENTRY( "gfx3", 0, gridlayout2,   4*8+4*16+32,  1 )
GFXDECODE_END


void sraider_state::machine_start()
{
	ladybug_base_state::machine_start();

	save_item(NAME(m_grid_color));
	save_item(NAME(m_sraider_0x30));
	save_item(NAME(m_sraider_0x38));
	save_item(NAME(m_weird_value));
}

void sraider_state::machine_reset()
{
	ladybug_base_state::machine_reset();

	m_grid_color = 0;
	m_sraider_0x30 = 0;
	m_sraider_0x38 = 0;
	std::fill(std::begin(m_weird_value), std::end(m_weird_value), 0);
}


MACHINE_CONFIG_START(ladybug_state::ladybug)

	/* basic machine hardware */
	MCFG_DEVICE_ADD("maincpu", Z80, 4000000)   /* 4 MHz */
	MCFG_DEVICE_PROGRAM_MAP(ladybug_map)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500) /* not accurate */)
	MCFG_SCREEN_SIZE(32*8, 32*8)
	MCFG_SCREEN_VISIBLE_AREA(1*8, 31*8-1, 4*8, 28*8-1)
	MCFG_SCREEN_UPDATE_DRIVER(ladybug_state, screen_update_ladybug)
	MCFG_SCREEN_PALETTE("palette")

	GFXDECODE(config, "gfxdecode", "palette", gfx_ladybug);
	PALETTE(config, "palette", FUNC(ladybug_state::ladybug_palette), 4*8 + 4*16, 32);

	LADYBUG_VIDEO(config, m_video, 4000000).set_gfxdecode_tag("gfxdecode");

	ls259_device &videolatch(LS259(config, "videolatch")); // L5 on video board or H3 on single board
	videolatch.q_out_cb<0>().set(FUNC(ladybug_state::flipscreen_w)); // no other outputs used

	/* sound hardware */
	SPEAKER(config, "mono").front_center();

	SN76489(config, "sn1", 4000000).add_route(ALL_OUTPUTS, "mono", 1.0);
	SN76489(config, "sn2", 4000000).add_route(ALL_OUTPUTS, "mono", 1.0);
MACHINE_CONFIG_END

MACHINE_CONFIG_START(dorodon_state::dorodon)
	ladybug(config);

	MCFG_DEVICE_MODIFY("maincpu")
	MCFG_DEVICE_OPCODES_MAP(decrypted_opcodes_map)
MACHINE_CONFIG_END

MACHINE_CONFIG_START(sraider_state::sraider)

	/* basic machine hardware */
	MCFG_DEVICE_ADD("maincpu", Z80, 4000000)   /* 4 MHz */
	MCFG_DEVICE_PROGRAM_MAP(sraider_cpu1_map)
	MCFG_DEVICE_VBLANK_INT_DRIVER("screen", sraider_state,  irq0_line_hold)

	MCFG_DEVICE_ADD("sub", Z80, 4000000)   /* 4 MHz */
	MCFG_DEVICE_PROGRAM_MAP(sraider_cpu2_map)
	MCFG_DEVICE_IO_MAP(sraider_cpu2_io_map)
	MCFG_DEVICE_VBLANK_INT_DRIVER("screen", sraider_state,  irq0_line_hold)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500) /* not accurate */)
	MCFG_SCREEN_SIZE(32*8, 32*8)
	MCFG_SCREEN_VISIBLE_AREA(1*8, 31*8-1, 4*8, 28*8-1)
	MCFG_SCREEN_UPDATE_DRIVER(sraider_state, screen_update_sraider)
	MCFG_SCREEN_VBLANK_CALLBACK(WRITELINE(*this, sraider_state, screen_vblank_sraider))
	MCFG_SCREEN_PALETTE(m_palette)

	GFXDECODE(config, m_gfxdecode, m_palette, gfx_sraider);
	PALETTE(config, m_palette, FUNC(sraider_state::sraider_palette), 4*8 + 4*16 + 32 + 2, 32 + 32 + 1);

	LADYBUG_VIDEO(config, m_video, 4000000).set_gfxdecode_tag(m_gfxdecode);
	ZEROHOUR_STARS(config, m_stars, 0);

	/* sound hardware */
	SPEAKER(config, "mono").front_center();

	SN76489(config, "sn1", 4000000).add_route(ALL_OUTPUTS, "mono", 1.0);
	SN76489(config, "sn2", 4000000).add_route(ALL_OUTPUTS, "mono", 1.0);
	SN76489(config, "sn3", 4000000).add_route(ALL_OUTPUTS, "mono", 1.0);
	SN76489(config, "sn4", 4000000).add_route(ALL_OUTPUTS, "mono", 1.0);
	SN76489(config, "sn5", 4000000).add_route(ALL_OUTPUTS, "mono", 1.0);
MACHINE_CONFIG_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( ladybug )
	ROM_REGION( 0x10000, "maincpu", 0 ) /* Located on the UNIVERSAL 8106-A2 CPU PCB */
	ROM_LOAD( "l1.c4", 0x0000, 0x1000, CRC(d09e0adb) SHA1(ddc1f849cbcefb64b70a26c2a4c993f0516af814) ) /* PCB silkscreened ROM1 */
	ROM_LOAD( "l2.d4", 0x1000, 0x1000, CRC(88bc4a0a) SHA1(193c9f90b7550020c0923cb158dff7d5faa53bc6) ) /* PCB silkscreened ROM2 */
	ROM_LOAD( "l3.e4", 0x2000, 0x1000, CRC(53e9efce) SHA1(1960e9cd896b6a65197aefc3f10348103552b598) ) /* PCB silkscreened ROM3 */
	ROM_LOAD( "l4.h4", 0x3000, 0x1000, CRC(ffc424d7) SHA1(2a4b9533e61e265bdd38c126add8c26d5bc048d5) ) /* PCB silkscreened ROM4 */
	ROM_LOAD( "l5.j4", 0x4000, 0x1000, CRC(ad6af809) SHA1(276275d56c725b9d90eeb44c317ceb06bac27ae7) ) /* PCB silkscreened ROM5 */
	ROM_LOAD( "l6.k4", 0x5000, 0x1000, CRC(cf1acca4) SHA1(c05de7de4bd05d5c2af6aa752e057a9286f3effc) ) /* PCB silkscreened ROM6 */

	// also found on an original PCB with 3x 0x2000 program ROMs (identical code-wise)
	//ROM_LOAD( "2a", 0x0000, 0x2000, CRC(b01c773b) SHA1(4e79eba05e92a614f707488b0e4245f3f86f2531) )
	//ROM_LOAD( "2c", 0x2000, 0x2000, CRC(600b7302) SHA1(bd933d22e261f7d8e37a514725dcad204fab0c68) )
	//ROM_LOAD( "2e", 0x4000, 0x2000, CRC(9a96396a) SHA1(d355092ef1666e4fd4479160c9baf4dffcbad4c5) )

	ROM_REGION( 0x2000, "gfx1", 0 ) /* Located on the UNIVERSAL 8106-B video PCB */
	ROM_LOAD( "l9.f7", 0x0000, 0x1000, CRC(77b1da1e) SHA1(58cb82417396a3d96acfc864f091b1a5988f228d) )
	ROM_LOAD( "l0.h7", 0x1000, 0x1000, CRC(aa82e00b) SHA1(83a5b745e58844b6dd7d05dfe9dbb5959aaf5c40) )

	ROM_REGION( 0x2000, "gfx2", 0 ) /* Located on the UNIVERSAL 8106-A2 CPU PCB */
	ROM_LOAD( "l8.l7", 0x0000, 0x1000, CRC(8b99910b) SHA1(0bc812cf872f04eacedb50feed53f1aa8a1f24b9) )
	ROM_LOAD( "l7.m7", 0x1000, 0x1000, CRC(86a5b448) SHA1(f8585a6fcf921e3e21f112dd2de474cb53cef290) )

	ROM_REGION( 0x0060, "proms", 0 ) /* BPROMs are 82s123 & located on the UNIVERSAL 8106-B video PCB */
	ROM_LOAD( "10-2.k1", 0x0000, 0x0020, CRC(df091e52) SHA1(4d7fea6d9ab31e5f280b1dc198a325f00c3826ef) ) /* palette */
	ROM_LOAD( "10-1.f4", 0x0020, 0x0020, CRC(40640d8f) SHA1(85d13a9b78c47174cff7c869f52b30263bae575e) ) /* sprite color lookup table */
	ROM_LOAD( "10-3.c4", 0x0040, 0x0020, CRC(27fa3a50) SHA1(7cf59b7a37c156640d6ea91554d1c4276c1780e0) ) /* ?? */
ROM_END

ROM_START( ladybugb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "lb1a.cpu", 0x0000, 0x1000, CRC(ec135e54) SHA1(69fc6db04b28c25eda329fc88c235267ca93a09f) )
	ROM_LOAD( "lb2a.cpu", 0x1000, 0x1000, CRC(3049c5c6) SHA1(51ceb70fa4789ff91c9bb1e157be5b6c09ff3c8e) )
	ROM_LOAD( "lb3a.cpu", 0x2000, 0x1000, CRC(b0fef837) SHA1(37e9d8d157c3af12cd97534a42dd21f621ac501b) )
	ROM_LOAD( "l4.h4",    0x3000, 0x1000, CRC(ffc424d7) SHA1(2a4b9533e61e265bdd38c126add8c26d5bc048d5) )
	ROM_LOAD( "l5.j4",    0x4000, 0x1000, CRC(ad6af809) SHA1(276275d56c725b9d90eeb44c317ceb06bac27ae7) )
	ROM_LOAD( "lb6a.cpu", 0x5000, 0x1000, CRC(88c8002a) SHA1(ffff1b8d4c1521710c988eee12081d28ed491ccf) )

	ROM_REGION( 0x2000, "gfx1", 0 )
	ROM_LOAD( "l9.f7", 0x0000, 0x1000, CRC(77b1da1e) SHA1(58cb82417396a3d96acfc864f091b1a5988f228d) )
	ROM_LOAD( "l0.h7", 0x1000, 0x1000, CRC(aa82e00b) SHA1(83a5b745e58844b6dd7d05dfe9dbb5959aaf5c40) )

	ROM_REGION( 0x2000, "gfx2", 0 ) /* Located on the UNIVERSAL 8106-A2 CPU PCB */
	ROM_LOAD( "l8.l7", 0x0000, 0x1000, CRC(8b99910b) SHA1(0bc812cf872f04eacedb50feed53f1aa8a1f24b9) )
	ROM_LOAD( "l7.m7", 0x1000, 0x1000, CRC(86a5b448) SHA1(f8585a6fcf921e3e21f112dd2de474cb53cef290) )

	ROM_REGION( 0x0060, "proms", 0 ) /* BPROMs are 82s123 or compatible */
	ROM_LOAD( "10-2.k1", 0x0000, 0x0020, CRC(df091e52) SHA1(4d7fea6d9ab31e5f280b1dc198a325f00c3826ef) ) /* palette */
	ROM_LOAD( "10-1.f4", 0x0020, 0x0020, CRC(40640d8f) SHA1(85d13a9b78c47174cff7c869f52b30263bae575e) ) /* sprite color lookup table */
	ROM_LOAD( "10-3.c4", 0x0040, 0x0020, CRC(27fa3a50) SHA1(7cf59b7a37c156640d6ea91554d1c4276c1780e0) ) /* ?? */
ROM_END

ROM_START( ladybugb2 ) // bootleg by Model Racing, PCB marked CS299, manual names it "Coccinelle" (ladybugs in Italian)
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "lb1b.cpu", 0x0000, 0x1000, CRC(35d61e65) SHA1(43b797f1882e0acbf6685deea82de77e78d2c917) )
	ROM_LOAD( "lb2b.cpu", 0x1000, 0x1000, CRC(a13e0fe4) SHA1(9e2876d8390d2b072d064b197057089a25c13a4a) )
	ROM_LOAD( "lb3b.cpu", 0x2000, 0x1000, CRC(ee8ac716) SHA1(ead222d2cd022ea3a4559e3cff08cabf2486eb68) )
	ROM_LOAD( "l4.h4",    0x3000, 0x1000, CRC(ffc424d7) SHA1(2a4b9533e61e265bdd38c126add8c26d5bc048d5) )
	ROM_LOAD( "l5.j4",    0x4000, 0x1000, CRC(ad6af809) SHA1(276275d56c725b9d90eeb44c317ceb06bac27ae7) )
	ROM_LOAD( "lb6b.cpu", 0x5000, 0x1000, CRC(dc906e89) SHA1(ffa5c3fc9d438e85cbe4fb33343dec664406fda7) )

	ROM_REGION( 0x2000, "gfx1", 0 )
	ROM_LOAD( "l9.f7", 0x0000, 0x1000, CRC(77b1da1e) SHA1(58cb82417396a3d96acfc864f091b1a5988f228d) )
	ROM_LOAD( "l0.h7", 0x1000, 0x1000, CRC(aa82e00b) SHA1(83a5b745e58844b6dd7d05dfe9dbb5959aaf5c40) )

	ROM_REGION( 0x2000, "gfx2", 0 )
	ROM_LOAD( "l8.l7", 0x0000, 0x1000, CRC(8b99910b) SHA1(0bc812cf872f04eacedb50feed53f1aa8a1f24b9) )
	ROM_LOAD( "l7.m7", 0x1000, 0x1000, CRC(86a5b448) SHA1(f8585a6fcf921e3e21f112dd2de474cb53cef290) )

	ROM_REGION( 0x0060, "proms", 0 ) /* BPROMs are 82s123 or compatible */
	ROM_LOAD( "10-2.k1", 0x0000, 0x0020, CRC(df091e52) SHA1(4d7fea6d9ab31e5f280b1dc198a325f00c3826ef) ) /* palette */
	ROM_LOAD( "10-1.f4", 0x0020, 0x0020, CRC(40640d8f) SHA1(85d13a9b78c47174cff7c869f52b30263bae575e) ) /* sprite color lookup table */
	ROM_LOAD( "10-3.c4", 0x0040, 0x0020, CRC(27fa3a50) SHA1(7cf59b7a37c156640d6ea91554d1c4276c1780e0) ) /* ?? */
ROM_END

ROM_START( snapjack )
	ROM_REGION( 0x10000, "maincpu", 0 ) /* Located on the UNIVERSAL 8106-A2 CPU PCB */
	ROM_LOAD( "sj1.c4", 0x0000, 0x1000, CRC(6b30fcda) SHA1(85e4ebbbe8e8d6c79a14387d7a6818abc9430037) ) /* PCB silkscreened ROM1 */
	ROM_LOAD( "sj2.d4", 0x1000, 0x1000, CRC(1f1088d1) SHA1(0fd5204ea27e9bdd811e9ea21e9bbab84b916f4a) ) /* PCB silkscreened ROM2 */
	ROM_LOAD( "sj3.e4", 0x2000, 0x1000, CRC(edd65f3a) SHA1(763d588f0755a22c0f24269e6f38979fd516693f) ) /* PCB silkscreened ROM3 */
	ROM_LOAD( "sj4.h4", 0x3000, 0x1000, CRC(f4481192) SHA1(514bb124a1d75a622e2ca4c2175d819092d4638d) ) /* PCB silkscreened ROM4 */
	ROM_LOAD( "sj5.j4", 0x4000, 0x1000, CRC(1bff7d05) SHA1(47246095313ebba30f42d715a9fb5fc1abb68ea6) ) /* PCB silkscreened ROM5 */
	ROM_LOAD( "sj6.k4", 0x5000, 0x1000, CRC(21793edf) SHA1(11e259161bab3a32a8b52f7baa4fec17be6d4302) ) /* PCB silkscreened ROM6 */

	ROM_REGION( 0x2000, "gfx1", 0 ) /* Located on the UNIVERSAL 8106-B video PCB */
	ROM_LOAD( "sj9.f7", 0x0000, 0x1000, CRC(ff2011c7) SHA1(38409e2318dee3cc0678d4ee9e93d9b895883df6) )
	ROM_LOAD( "sj0.h7", 0x1000, 0x1000, CRC(f097babb) SHA1(461662719bc7f1cf21c41759f4832a92b0fdb4f2) )

	ROM_REGION( 0x2000, "gfx2", 0 ) /* Located on the UNIVERSAL 8106-A2 CPU PCB */
	ROM_LOAD( "sj8.l7", 0x0000, 0x1000, CRC(b7f105b6) SHA1(1135c3188b41cb0ccb24079c613188209b624683) )
	ROM_LOAD( "sj7.m7", 0x1000, 0x1000, CRC(1cdb03a8) SHA1(5f390a672f3adf6392f8060bf7f0bcabc2eba139) )

	ROM_REGION( 0x0060, "proms", 0 ) /* BPROMs are 82s123 & located on the UNIVERSAL 8106-B video PCB */
	ROM_LOAD( "10-2.k1", 0x0000, 0x0020, CRC(cbbd9dd1) SHA1(e267726ba59e9a42ac89dd22eb1508ad21fd32ac) ) /* palette */
	ROM_LOAD( "10-1.f4", 0x0020, 0x0020, CRC(5b16fbd2) SHA1(0a776aeca3947a6f29d527018f5182e758b50c5d) ) /* sprite color lookup table */
	ROM_LOAD( "10-3.c4", 0x0040, 0x0020, CRC(27fa3a50) SHA1(7cf59b7a37c156640d6ea91554d1c4276c1780e0) ) /* ?? */
ROM_END

ROM_START( cavenger )
	ROM_REGION( 0x10000, "maincpu", 0 ) /* Located on the UNIVERSAL 8106-A2 CPU PCB */
	ROM_LOAD( "1.c4", 0x0000, 0x1000, CRC(9e0cc781) SHA1(f23bd6b9f427c26ac996a5c8ba29f356cf45c78a) ) /* PCB silkscreened ROM1 */
	ROM_LOAD( "2.d4", 0x1000, 0x1000, CRC(5ce5b950) SHA1(170e3f8be592dcccb8868474f40f8f2223e8a8b5) ) /* PCB silkscreened ROM2 */
	ROM_LOAD( "3.e4", 0x2000, 0x1000, CRC(bc28218d) SHA1(4b0f1b38a5837b7ffc9aec6c28c6eb72cfa46226) ) /* PCB silkscreened ROM3 */
	ROM_LOAD( "4.h4", 0x3000, 0x1000, CRC(2b32e9f5) SHA1(f8a7ea799d8ff9b4f830d064bb2f34a76729c336) ) /* PCB silkscreened ROM4 */
	ROM_LOAD( "5.j4", 0x4000, 0x1000, CRC(d117153e) SHA1(622c90a6c3f0adc24fe8a1d4969075cbd55add4e) ) /* PCB silkscreened ROM5 */
	ROM_LOAD( "6.k4", 0x5000, 0x1000, CRC(c7d366cb) SHA1(ec4981fe34abf992acbd6325b2c756c58ff80b04) ) /* PCB silkscreened ROM6 */

	ROM_REGION( 0x2000, "gfx1", 0 ) /* Located on the UNIVERSAL 8106-B video PCB */
	ROM_LOAD( "9.f7", 0x0000, 0x1000, CRC(63357785) SHA1(20eaa866b7700535312fd415edaea94408ff3e3d) )
	ROM_LOAD( "0.h7", 0x1000, 0x1000, CRC(52ad1133) SHA1(bc8c52c6ba919287773ff6a4ec793ebd95176130) )

	ROM_REGION( 0x2000, "gfx2", 0 ) /* Located on the UNIVERSAL 8106-A2 CPU PCB */
	ROM_LOAD( "8.l7", 0x0000, 0x1000, CRC(b022bf2d) SHA1(85f78d5a1e5782587bb66ad101a94fd0d62fb790) )
	/* 1000-1fff empty */

	ROM_REGION( 0x0060, "proms", 0 ) /* BPROMs are 82s123 & located on the UNIVERSAL 8106-B video PCB */
	ROM_LOAD( "10-2.k1", 0x0000, 0x0020, CRC(42a24dd5) SHA1(03175ee7f8e11896a89d7cc0d614a78a49923627) ) /* palette */
	ROM_LOAD( "10-1.f4", 0x0020, 0x0020, CRC(d736b8de) SHA1(4c9c76826f3a2a631d01fd2531d55318172b0c12) ) /* sprite color lookup table */
	ROM_LOAD( "10-3.c4", 0x0040, 0x0020, CRC(27fa3a50) SHA1(7cf59b7a37c156640d6ea91554d1c4276c1780e0) ) /* ?? */
ROM_END

ROM_START( dorodon )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dorodon.0",   0x0000, 0x2000, CRC(460aaf26) SHA1(c4ea41cba4ac2d93fedec3c117a4470fee2a910f) )
	ROM_LOAD( "dorodon.1",   0x2000, 0x2000, CRC(d2451eb6) SHA1(4154bfe50b7f75444d3f0c9be6bd2475fdba1938) )
	ROM_LOAD( "dorodon.2",   0x4000, 0x2000, CRC(d3c6ee6c) SHA1(6971ecdc968810c19f8601efc3d389450156bb22) )

	ROM_REGION( 0x2000, "gfx1", 0 )
	ROM_LOAD( "dorodon.5",   0x0000, 0x1000, CRC(5eee2b85) SHA1(55ac9566e805d103b6916f51c764e2601cc1f715) )
	ROM_LOAD( "dorodon.6",   0x1000, 0x1000, CRC(395ac25a) SHA1(d8a55e42b8c5d957c2e6a3181d7ac10c6a448f46) )

	ROM_REGION( 0x2000, "gfx2", 0 )
	ROM_LOAD( "dorodon.4",   0x0000, 0x1000, CRC(d70bb50a) SHA1(b9d46862f288c49bb8b660da87b63bd4ecb36379) )
	ROM_LOAD( "dorodon.3",   0x1000, 0x1000, CRC(e44e59e6) SHA1(ff730152804d75ddb9fb19e8ec33cc764d8a50e8) )

	ROM_REGION( 0x0100, "user1", 0 )
	ROM_LOAD_NIB_HIGH( "dorodon.bp4", 0x0000, 0x0100, CRC(f865c135) SHA1(1202f83bfa50afa5a5d24401efa8bf058e7e30b5) )
	ROM_LOAD_NIB_LOW(  "dorodon.bp3", 0x0000, 0x0100, CRC(47b2f0bb) SHA1(640720aa5c1119080c6da928f6d1b0e76b989742) )

	ROM_REGION( 0x0060, "proms", 0 )
	ROM_LOAD( "dorodon.bp0", 0x0000, 0x0020, CRC(8fcf0bc8) SHA1(392d22731b3e4bc663d6e4385f6069ee2b4ee029) ) /* palette */
	ROM_LOAD( "dorodon.bp1", 0x0020, 0x0020, CRC(3f209be4) SHA1(f924494eed357a15ffc11331c163af24585d4ab9) ) /* sprite color lookup table */
	ROM_LOAD( "dorodon.bp2", 0x0040, 0x0020, CRC(27fa3a50) SHA1(7cf59b7a37c156640d6ea91554d1c4276c1780e0) ) /* timing?? */
ROM_END

ROM_START( dorodon2 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "1.3fg",        0x0000, 0x2000, CRC(4d05d6f8) SHA1(db12ad04295f0ce112b6e90fde94a53ed1d6c3b9) )
	ROM_LOAD( "2.3h",         0x2000, 0x2000, CRC(27b43b09) SHA1(12a8a6b8665bb9d1967ec631a794aab564a50570) )
	ROM_LOAD( "3.3k",         0x4000, 0x2000, CRC(38d2f295) SHA1(b4d2cfd6e9f03c3ef18dcf67326f4106749b62b1) )

	ROM_REGION( 0x2000, "gfx1", 0 )
	ROM_LOAD( "6.6a",        0x0000, 0x1000, CRC(2a2d8b9c) SHA1(ba3ce8ed6cafa711bf4c6ed260dd15b38adbd6cc) )
	ROM_LOAD( "7.6bc",       0x1000, 0x1000, CRC(d14f95fa) SHA1(e9ba87602d779d833b8152c077c692e67ef696cc) )

	ROM_REGION( 0x2000, "gfx2", 0 )
	ROM_LOAD( "5.3t",        0x0000, 0x1000, CRC(54c04f58) SHA1(342ef914e6f8bf37472d146bb5e9fb67056d7fc5) )
	ROM_LOAD( "4.3r",        0x1000, 0x1000, CRC(1ebb6493) SHA1(30367d7594118e0fa8620e5d20c66a650ca82c86) )

	ROM_REGION( 0x0100, "user1", 0 )
	ROM_LOAD_NIB_HIGH( "dorodon.bp4", 0x0000, 0x0100, CRC(f865c135) SHA1(1202f83bfa50afa5a5d24401efa8bf058e7e30b5) )
	ROM_LOAD_NIB_LOW(  "dorodon.bp3", 0x0000, 0x0100, CRC(47b2f0bb) SHA1(640720aa5c1119080c6da928f6d1b0e76b989742) )

	/* (from other romset - I think these are correct, they match the Starcade video) */
	ROM_REGION( 0x0060, "proms", 0 )
	ROM_LOAD( "dorodon.bp0", 0x0000, 0x0020, CRC(8fcf0bc8) SHA1(392d22731b3e4bc663d6e4385f6069ee2b4ee029) ) /* palette */
	ROM_LOAD( "dorodon.bp1", 0x0020, 0x0020, CRC(3f209be4) SHA1(f924494eed357a15ffc11331c163af24585d4ab9) ) /* sprite color lookup table */
	ROM_LOAD( "dorodon.bp2", 0x0040, 0x0020, CRC(27fa3a50) SHA1(7cf59b7a37c156640d6ea91554d1c4276c1780e0) ) /* timing?? */
ROM_END

ROM_START( sraider )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "sraid3.r4",    0x0000, 0x2000, CRC(0f389774) SHA1(c67596e6bf00175ff0a241506cd2f88114d05933) )
	ROM_LOAD( "sraid2.n4",    0x2000, 0x2000, CRC(38a48db0) SHA1(6f4f384d702fb8ee4bb2ef579638239d57e32ddd) )
	ROM_LOAD( "sraid1.m4",    0x4000, 0x2000, CRC(2f302a4e) SHA1(3a902ce6858f38df88b60830bef4b1d45b09b2df) )

	ROM_REGION( 0x10000, "sub", 0 )
	ROM_LOAD( "sraid-s4.h6",  0x0000, 0x2000, CRC(57173a12) SHA1(6cb8fd4826e499f9a4e63621d58bc4b596cc261e) )
	ROM_LOAD( "sraid-s5.j6",  0x2000, 0x2000, CRC(5a459179) SHA1(a261c8f3c7c4cd4587c003bbbe815d2c4e01ffbc) )
	ROM_LOAD( "sraid-s6.l6",  0x4000, 0x2000, CRC(ea3aa25d) SHA1(353c0d075d5e0a3bc25a65e2748f5eb5212a844d) )

	ROM_REGION( 0x2000, "gfx1", 0 )
	ROM_LOAD( "sraid-s0.k6",  0x0000, 0x1000, CRC(a0373909) SHA1(00e3bd5dd90769d670fc3c51edd1cd4b69e6132d) )
	ROM_LOAD( "sraids11.l6",  0x1000, 0x1000, CRC(ba22d949) SHA1(83762ced1df92ff594887e44d5b783826bbfb0c9) )

	ROM_REGION( 0x2000, "gfx2", 0 )
	ROM_LOAD( "sraid-s7.m2",  0x0000, 0x1000, CRC(299f8e07) SHA1(1de71f251286088487da7285d6f8070147002af5) )
	ROM_LOAD( "sraid-s8.n2",  0x1000, 0x1000, CRC(57ba8888) SHA1(2aa1a5f682d146a55a96e471bb78e5c60da02bf9) )

	ROM_REGION( 0x1000, "gfx3", 0 ) /* fixed portion of the grid */
	ROM_LOAD( "sraid-s9.f6",  0x0000, 0x1000, CRC(2380b90f) SHA1(0310554e3f2ec973c2bb6e816d04e5c0c1e0a0b9) )

	ROM_REGION( 0x0060, "proms", 0 )
	ROM_LOAD( "srpr10-1.a2",  0x0000, 0x0020, CRC(121fdb99) SHA1(3bc092da40beb129a4df3db2f55d22bbbcf7bad8) )
	ROM_LOAD( "srpr10-2.l3",  0x0020, 0x0020, CRC(88b67e70) SHA1(e21ee2939e96dffee101bd92c62ed975b6b64001) )
	ROM_LOAD( "srpr10-3.c1",  0x0040, 0x0020, CRC(27fa3a50) SHA1(7cf59b7a37c156640d6ea91554d1c4276c1780e0) ) /* ?? */
ROM_END


void dorodon_state::init_dorodon()
{
	/* decode the opcodes */
	uint8_t *rom = memregion("maincpu")->base();
	uint8_t *table = memregion("user1")->base();

	for (offs_t i = 0; i < 0x6000; i++)
		m_decrypted_opcodes[i] = table[rom[i]];
}


GAME( 1981, cavenger,  0,       ladybug, cavenger, ladybug_state, empty_init,   ROT0,   "Universal",              "Cosmic Avenger",                          MACHINE_SUPPORTS_SAVE )
GAME( 1981, ladybug,   0,       ladybug, ladybug,  ladybug_state, empty_init,   ROT270, "Universal",              "Lady Bug",                                MACHINE_SUPPORTS_SAVE )
GAME( 1981, ladybugb,  ladybug, ladybug, ladybug,  ladybug_state, empty_init,   ROT270, "bootleg",                "Lady Bug (bootleg set 1)",                MACHINE_SUPPORTS_SAVE )
GAME( 1981, ladybugb2, ladybug, ladybug, ladybug,  ladybug_state, empty_init,   ROT270, "bootleg (Model Racing)", "Coccinelle (bootleg of Lady Bug, set 2)", MACHINE_SUPPORTS_SAVE ) // title removed, but manual names it Coccinelle
GAME( 1982, dorodon,   0,       dorodon, dorodon,  dorodon_state, init_dorodon, ROT270, "UPL (Falcon license?)",  "Dorodon (set 1)",                         MACHINE_SUPPORTS_SAVE ) // license or bootleg?
GAME( 1982, dorodon2,  dorodon, dorodon, dorodon,  dorodon_state, init_dorodon, ROT270, "UPL (Falcon license?)",  "Dorodon (set 2)",                         MACHINE_SUPPORTS_SAVE ) // "
GAME( 1982, snapjack,  0,       ladybug, snapjack, ladybug_state, empty_init,   ROT0,   "Universal",              "Snap Jack",                               MACHINE_SUPPORTS_SAVE )
GAME( 1982, sraider,   0,       sraider, sraider,  sraider_state, empty_init,   ROT270, "Universal",              "Space Raider",                            MACHINE_SUPPORTS_SAVE )
