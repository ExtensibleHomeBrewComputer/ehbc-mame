// license:BSD-3-Clause
// copyright-holders:Robbbert
/***************************************************************************

        Intel Intellec MDS

        28/06/2011 Skeleton driver.

This is a prototyping machine, very complex and expensive.

It does all i/o via uarts, that is TTY, paper-tape puncher, CRT terminal.
It also has proms which can be dynamically programmed to alter the machine
architecture, so internal ports change meaning mid-stream. (e.g. port 80).

The code below is just a sample; it does not represent any aspect of the
real hardware.

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "machine/terminal.h"


class imds_state : public driver_device
{
public:
	imds_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_terminal(*this, "terminal")
	{ }

	void imds(machine_config &config);

private:
	DECLARE_READ8_MEMBER(term_r);
	DECLARE_READ8_MEMBER(term_status_r);
	void kbd_put(u8 data);
	uint8_t m_term_data;
	virtual void machine_reset() override;
	void imds_io(address_map &map);
	void imds_mem(address_map &map);

	required_device<cpu_device> m_maincpu;
	required_device<generic_terminal_device> m_terminal;
};

READ8_MEMBER( imds_state::term_status_r )
{
	uint8_t ret = m_term_data;
	m_term_data = 0;
	return ret;
}

READ8_MEMBER( imds_state::term_r )
{
	return (m_term_data) ? 0x21 : 0x20;
}

void imds_state::imds_mem(address_map &map)
{
	map.unmap_value_high();
	map(0x0000, 0x1fff).rom();
	map(0x2000, 0xffff).ram();
}

void imds_state::imds_io(address_map &map)
{
	map.unmap_value_high();
	map.global_mask(0xff);
}

/* Input ports */
static INPUT_PORTS_START( imds )
INPUT_PORTS_END

void imds_state::kbd_put(u8 data)
{
	m_term_data = data;
}

void imds_state::machine_reset()
{
	m_term_data = 0;
}

MACHINE_CONFIG_START(imds_state::imds)
	/* basic machine hardware */
	MCFG_DEVICE_ADD("maincpu", I8080, 4_MHz_XTAL) // no idea of clock.
	MCFG_DEVICE_PROGRAM_MAP(imds_mem)
	MCFG_DEVICE_IO_MAP(imds_io)

//  INS8250(config, "ins8250", 1843200);

	/* video hardware */
	MCFG_DEVICE_ADD(m_terminal, GENERIC_TERMINAL, 0)
	MCFG_GENERIC_TERMINAL_KEYBOARD_CB(PUT(imds_state, kbd_put))
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( imds )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "a62_2716.bin", 0x0000, 0x0800, CRC(86a55b2f) SHA1(21033f7abb2c3e08028613e0c35ffecb703ff4f1))
	ROM_LOAD( "a51_2716.bin", 0x0800, 0x0800, CRC(ee55c448) SHA1(16c2f7e3b5baeb398adcc59603943910813e6a9b))
	ROM_LOAD( "a52_2716.bin", 0x1000, 0x0800, CRC(8db1b33e) SHA1(6fc5e438009636dd6d7185071b152b0491d3baeb))
	ROM_LOAD( "a53_2716.bin", 0x1800, 0x0800, CRC(01690f4f) SHA1(eadef30a3797f41e08d28e7691f8de44c0f3b8ea))
ROM_END

/* Driver */

/*    YEAR  NAME  PARENT  COMPAT  MACHINE  INPUT  CLASS       INIT        COMPANY  FULLNAME        FLAGS */
COMP( 1983, imds, 0,      0,      imds,    imds,  imds_state, empty_init, "Intel", "Intellec MDS", MACHINE_NOT_WORKING | MACHINE_NO_SOUND)
