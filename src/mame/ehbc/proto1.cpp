// license: BSD-3-Clause
/****************************************************************************

    EHBC proto1

    Hardware:
    - MC68030
    - MX8315 Clock Generator
	- FPM/EDO DRAM up to 64 MiB

    TODO:
	

****************************************************************************/

#include <cstring>

#include "emu.h"
#include "bus/rs232/rs232.h"
#include "bus/isa/isa.h"
#include "bus/isa/isa_cards.h"
#include "bus/ata/hdd.h"
#include "cpu/m68000/m68030.h"
#include "machine/idectrl.h"
#include "machine/mc68901.h"
#include "machine/mc68681.h"
#include "machine/mc146818.h"
#include "machine/hd63450.h"
#include "machine/keyboard.h"
#include "machine/8042kbdc.h"
#include "machine/ram.h"
#include "machine/upd765.h"
#include "video/mc6845.h"
#include "video/pc_vga.h"
#include "video/pc_vga_cirrus.h"
#include "sound/ad1848.h"
#include "imagedev/floppy.h"

namespace {


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class proto1_state : public driver_device
{
public:
	proto1_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_ram(*this, RAM_TAG),
		m_mfp(*this, "mfp%u", 0U),
		m_duart(*this, "duart"),
		m_dmac(*this, "dmac%u", 0U),
		m_serial(*this, "serial%u", 0U),
		m_ide(*this, "ide"),
		m_flash(*this, "flash"),
		m_vga(*this, "vga"),
		m_fdc(*this, "fdc"),
		m_rtc(*this, "rtc"),
		m_kbdc(*this, "kbdc"),
		m_snd(*this, "snd"),
		m_switches(*this, "switches")
	{ }

	void proto1(machine_config &config);

protected:
	virtual void machine_start() override;
	virtual void machine_reset() override;

private:
	required_device<m68030_device> m_maincpu;
	required_device<ram_device> m_ram;
	required_device_array<mc68901_device, 2> m_mfp;
	required_device<mc68681_device> m_duart;
	required_device_array<hd63450_device, 2> m_dmac;  // mc68440 in real machine
	required_device_array<rs232_port_device, 2> m_serial;
	required_device<ide_controller_32_device> m_ide;
	required_memory_region m_flash;
	required_device<vga_device> m_vga;
	required_device<pc8477b_device> m_fdc;
	required_device<mc146818_device> m_rtc;
	required_device<kbdc8042_device> m_kbdc;
	required_device<ad1848_device> m_snd;
	required_ioport m_switches;

	void mem_map(address_map &map);

	uint16_t scu_hptb_tdr[3];

	uint8_t scu_ccr;
	uint8_t scu_abr[8];
	uint32_t scu_isr;
	uint8_t scu_icr[24];
	void scu_w(offs_t offset, uint8_t data);
	uint8_t scu_r(offs_t offset);
	void scu_isr_set(int vec, int state);

	void port_e9_w(offs_t offset, uint8_t data);

	void irq1_handler(int state);
	void irq3_handler(int state);
	void irq4_handler(int state);
	void irq5_handler(int state);
	void irq6_handler(int state);
	void irq7_handler(int state);
	void irq8_handler(int state);
	void irq9_handler(int state);
	void irq10_handler(int state);
	void irq11_handler(int state);
	void irq12_handler(int state);
	void irq14_handler(int state);
	void irq15_handler(int state);

	void irq_duart_handler(int state);
	void irq_mfp0_handler(int state);
	void irq_mfp1_handler(int state);
	void irq_audio_handler(int state);
	void irq_dmac0_handler(int state);
	void irq_dmac1_handler(int state);
};


//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

void proto1_state::mem_map(address_map &map)
{
	map(0x00000000, 0x000FFFFF).ram();

	map(0xFD000000, 0xFDFFFFFF).rom().region("flash", 0);

	// pc i/o ports
	map(0xFE000060, 0xFE000064).rw(m_kbdc, FUNC(kbdc8042_device::data_r), FUNC(kbdc8042_device::data_w));
	map(0xFE000070, 0xFE000070).w(m_rtc, FUNC(mc146818_device::address_w));
	map(0xFE000071, 0xFE000071).rw(m_rtc, FUNC(mc146818_device::data_r), FUNC(mc146818_device::data_w));
	map(0xFE0000E9, 0xFE0000E9).w(FUNC(proto1_state::port_e9_w));
	map(0xFE0001F0, 0xFE0001F7).rw(m_ide, FUNC(ide_controller_32_device::cs0_r), FUNC(ide_controller_32_device::cs0_w));
	map(0xFE0003B0, 0xFE0003DF).m(m_vga, FUNC(cirrus_gd5428_vga_device::io_map));
	map(0xFE0003F0, 0xFE0003F7).rw(m_ide, FUNC(ide_controller_32_device::cs1_r), FUNC(ide_controller_32_device::cs1_w));
	map(0xFE0003F0, 0xFE0003F7).m(m_fdc, FUNC(pc8477b_device::map));

	// pc memory map (~16MiB)
	map(0xFE0A0000, 0xFE0BFFFF).rw(m_vga, FUNC(cirrus_gd5428_vga_device::mem_r), FUNC(cirrus_gd5428_vga_device::mem_w));
	map(0xFE100000, 0xFE1FFFFF).rw(m_vga, FUNC(cirrus_gd5446_vga_device::mem_linear_r), FUNC(cirrus_gd5446_vga_device::mem_linear_w));

	// mmio
	map(0xFF000000, 0xFF0000FF).rw(FUNC(proto1_state::scu_r), FUNC(proto1_state::scu_w));  // for SCU
	map(0xFF000100, 0xFF00010F).rw(m_mfp[0], FUNC(mc68901_device::read), FUNC(mc68901_device::write));
	map(0xFF000110, 0xFF00011F).rw(m_mfp[1], FUNC(mc68901_device::read), FUNC(mc68901_device::write));
	map(0xFF000200, 0xFF00020F).rw(m_duart, FUNC(mc68681_device::read), FUNC(mc68681_device::write));
	map(0xFF000300, 0xFF0003FF).rw(m_dmac[0], FUNC(hd63450_device::read), FUNC(hd63450_device::write));
	map(0xFF000400, 0xFF0004FF).rw(m_dmac[1], FUNC(hd63450_device::read), FUNC(hd63450_device::write));
	map(0xFF000500, 0xFF000503).rw(m_snd, FUNC(ad1848_device::read), FUNC(ad1848_device::write));
}

//**************************************************************************
//  INPUT DEFINITIONS
//**************************************************************************

static INPUT_PORTS_START( proto1 )
	PORT_START("switches")
	PORT_DIPNAME(0x01, 0x01, "IO Mode")
	PORT_DIPLOCATION("DIL:1")
	PORT_DIPSETTING(   0x00, "Terminal")
	PORT_DIPSETTING(   0x01, "Internal")
	PORT_DIPNAME(0x02, 0x02, "Columns")
	PORT_DIPLOCATION("DIL:2")
	PORT_DIPSETTING(   0x00, "40")
	PORT_DIPSETTING(   0x02, "80")
	// DIL:3 and DIL:4 select parallel keyboard strobe polarity
INPUT_PORTS_END

//**************************************************************************
//  MACHINE EMULATION
//**************************************************************************

void proto1_state::machine_start()
{
	machine_reset();
}

void proto1_state::machine_reset()
{
	scu_ccr = 0;
	scu_isr = 0;
	memset(scu_hptb_tdr, 0, sizeof(scu_hptb_tdr));
	memset(scu_icr, 0, sizeof(scu_icr));
	m_maincpu->space(0).write_dword(0, 0x00000000);
	m_maincpu->space(0).write_dword(4, 0xFD000000);
}

void proto1_state::port_e9_w(offs_t offset, uint8_t data)
{
	printf("%c", data);
}

void proto1_state::scu_w(offs_t offset, uint8_t data)
{
	if (offset >= 32) return;

	const XTAL clock_table[] = {
		33_MHz_XTAL, 80_MHz_XTAL, 66_MHz_XTAL, 50_MHz_XTAL,
		40_MHz_XTAL, 60_MHz_XTAL, 25_MHz_XTAL, 20_MHz_XTAL
	};

	switch (offset) {
		case 0:	 // CCR
			scu_ccr = data;
			if (data & 0x80) {
				m_maincpu->set_clock(8_MHz_XTAL);
			} else {
				m_maincpu->set_clock(clock_table[(data >> 4) & 0x7]);
			}
			break;
		case 1:  // DCR
		case 2:  // FCR
		case 3:  // PCR
		case 4:  // IDER0
		case 5:  // IDER1
		case 6:  // ISAR
			break;
		case 8:  // ABR0-7
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			
			scu_abr[offset - 8] = data;
			break;
		case 16:  // ICR0-11
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
			scu_icr[((offset - 16) << 1)] = (data >> 4) & 0xF;
			scu_icr[((offset - 16) << 1) + 1] = data & 0xF;
			break;
		case 28:
		case 29:
		case 30:
		case 31:
			break;
		default:
			break;
	}
}

uint8_t proto1_state::scu_r(offs_t offset)
{
	switch (offset) {
		case 0:  // CCR
			return scu_ccr;
		case 1:  // DCR
		case 2:  // FCR
		case 3:  // PCR
		case 4:  // IDER0
		case 5:  // IDER1
		case 6:  // ISAR
			break;
		case 8:  // ABR0-7
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			return scu_abr[offset - 8];
			break;
		case 16:  // ISR0-2
			return (scu_isr >> 16) & 0xFF;
		case 17:
			return (scu_isr >> 8) & 0xFF;
		case 18:
			return scu_isr & 0xFF;
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		case 25: {
			int irqline = offset - 18;
			int cnt = 0;
			for (int i = 0; i < 24; i++) {
				if ((scu_icr[i] & 7) == irqline) {
					scu_isr &= ~(1 << i);
					cnt++;
				}
			}
			m_maincpu->set_input_line(irqline, CLEAR_LINE);
			return cnt;
		}
		default:
			break;
	}

	return 0;
}

void proto1_state::scu_isr_set(int irq, int state)
{
	if (!(scu_icr[irq] & 0x08)) {  // irq disabled
		return;
	}

	if (state) {
		scu_isr |= (1 << irq);
	} else {
		scu_isr &= ~(1 << irq);
	}
	
	m_maincpu->set_input_line(scu_icr[irq] & 0x7, CLEAR_LINE);
	if (state) {
		m_maincpu->set_input_line(scu_icr[irq] & 0x7, ASSERT_LINE);
	}
}

//**************************************************************************
//  INTERRUPT HANDLERS
//**************************************************************************

void proto1_state::irq1_handler(int state)
{
	scu_isr_set(1, state);
}

void proto1_state::irq3_handler(int state)
{
	scu_isr_set(3, state);
}

void proto1_state::irq4_handler(int state)
{
	scu_isr_set(4, state);
}

void proto1_state::irq5_handler(int state)
{
	scu_isr_set(5, state);
}

void proto1_state::irq6_handler(int state)
{
	scu_isr_set(6, state);
}
void proto1_state::irq7_handler(int state)
{
	scu_isr_set(7, state);
}

void proto1_state::irq8_handler(int state)
{
	scu_isr_set(8, state);
}

void proto1_state::irq9_handler(int state)
{
	scu_isr_set(9, state);
}

void proto1_state::irq10_handler(int state)
{
	scu_isr_set(10, state);
}

void proto1_state::irq11_handler(int state)
{
	scu_isr_set(11, state);
}

void proto1_state::irq12_handler(int state)
{
	scu_isr_set(12, state);
}

void proto1_state::irq14_handler(int state)
{
	scu_isr_set(14, state);
}

void proto1_state::irq15_handler(int state)
{
	scu_isr_set(15, state);
}

void proto1_state::irq_duart_handler(int state)
{
	scu_isr_set(16, state);
}

void proto1_state::irq_mfp0_handler(int state)
{
	scu_isr_set(17, state);
}

void proto1_state::irq_mfp1_handler(int state)
{
	scu_isr_set(18, state);
}

void proto1_state::irq_audio_handler(int state)
{
	scu_isr_set(19, state);
}

void proto1_state::irq_dmac0_handler(int state)
{
	scu_isr_set(20, state);
}

void proto1_state::irq_dmac1_handler(int state)
{
	scu_isr_set(21, state);
}

//**************************************************************************
//  MACHINE DEFINTIONS
//**************************************************************************

void proto1_state::proto1(machine_config &config)
{
	M68030(config, m_maincpu, 8_MHz_XTAL);
	m_maincpu->set_addrmap(AS_PROGRAM, &proto1_state::mem_map);

	RAM(config, m_ram)
		.set_default_size("512K")
		.set_extra_options("1M,2M,4M,8M,16M,32M,64M");

	MC68901(config, m_mfp[0], 4_MHz_XTAL);
	m_mfp[0]->out_irq_cb().set(FUNC(proto1_state::irq_mfp0_handler));

	MC68901(config, m_mfp[1], 4_MHz_XTAL);
	m_mfp[1]->out_irq_cb().set(FUNC(proto1_state::irq_mfp1_handler));

	HD63450(config, m_dmac[0], 10_MHz_XTAL, "maincpu");
	m_dmac[0]->set_clocks(attotime::from_nsec(120), attotime::from_nsec(120), attotime::from_nsec(120), attotime::from_nsec(120));
	m_dmac[0]->set_burst_clocks(attotime::from_nsec(360), attotime::from_nsec(360), attotime::from_nsec(360), attotime::from_nsec(360));
	m_dmac[0]->irq_callback().set(FUNC(proto1_state::irq_dmac0_handler));
	m_dmac[0]->dma_end().set("fdc", FUNC(pc8477b_device::tc_line_w));
	m_dmac[0]->dma_read<0>().set("fdc", FUNC(pc8477b_device::dma_r));
	m_dmac[0]->dma_write<0>().set("fdc", FUNC(pc8477b_device::dma_w));
	m_dmac[0]->dma_read<1>().set("ide", FUNC(ide_controller_32_device::read_dma));
	m_dmac[0]->dma_write<1>().set("ide", FUNC(ide_controller_32_device::write_dma));

	HD63450(config, m_dmac[1], 10_MHz_XTAL, "maincpu");
	m_dmac[1]->set_clocks(attotime::from_nsec(120), attotime::from_nsec(120), attotime::from_nsec(120), attotime::from_nsec(120));
	m_dmac[1]->set_burst_clocks(attotime::from_nsec(360), attotime::from_nsec(360), attotime::from_nsec(360), attotime::from_nsec(360));
	m_dmac[1]->irq_callback().set(FUNC(proto1_state::irq_dmac1_handler));

	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER));
	screen.set_raw(25.175_MHz_XTAL, 800, 0, 640, 525, 0, 480);
	screen.set_screen_update(m_vga, FUNC(cirrus_gd5428_vga_device::screen_update));

	CIRRUS_GD5428_VGA(config, m_vga, 0);
	m_vga->set_screen("screen");
	m_vga->set_vram_size(0x200000);

	IDE_CONTROLLER_32(config, m_ide);
	m_ide->options(ata_devices, "hdd", nullptr, true);
	m_ide->irq_handler().set(FUNC(proto1_state::irq15_handler));
	m_ide->dmarq_handler().set(m_dmac[0], FUNC(hd63450_device::drq1_w));

	MC68681(config, m_duart, 8_MHz_XTAL);
	m_duart->irq_cb().set(FUNC(proto1_state::irq_duart_handler));
	m_duart->a_tx_cb().set(m_serial[0], FUNC(rs232_port_device::write_txd));
	m_duart->outport_cb().append(m_serial[0], FUNC(rs232_port_device::write_rts)).bit(0);
	m_duart->b_tx_cb().set(m_serial[1], FUNC(rs232_port_device::write_txd));
	m_duart->outport_cb().append(m_serial[1], FUNC(rs232_port_device::write_rts)).bit(1);

	RS232_PORT(config, m_serial[0], default_rs232_devices, "terminal");
	m_serial[0]->rxd_handler().set(m_duart, FUNC(mc68681_device::rx_a_w));
	m_serial[0]->cts_handler().set(m_duart, FUNC(mc68681_device::ip0_w));

	RS232_PORT(config, m_serial[1], default_rs232_devices, nullptr);
	m_serial[1]->rxd_handler().set(m_duart, FUNC(mc68681_device::rx_b_w));
	m_serial[1]->cts_handler().set(m_duart, FUNC(mc68681_device::ip1_w));

	PC8477B(config, m_fdc, 24_MHz_XTAL, pc8477b_device::mode_t::PS2);
	m_fdc->intrq_wr_callback().set(FUNC(proto1_state::irq6_handler));
	m_fdc->drq_wr_callback().set(m_dmac[0], FUNC(hd63450_device::drq0_w));

	AD1848(config, m_snd, 24.576_MHz_XTAL);
	m_snd->irq().set(FUNC(proto1_state::irq_audio_handler));

	MC146818(config, m_rtc, 32.768_kHz_XTAL);
	m_rtc->irq().set(FUNC(proto1_state::irq8_handler));

	KBDC8042(config, m_kbdc);
	m_kbdc->set_keyboard_type(kbdc8042_device::KBDC8042_STANDARD);
	m_kbdc->set_keyboard_tag("at_keyboard");
	m_kbdc->input_buffer_full_callback().set(FUNC(proto1_state::irq1_handler));
	m_kbdc->input_buffer_full_mouse_callback().set(FUNC(proto1_state::irq12_handler));

	floppy_connector &fdconn0(FLOPPY_CONNECTOR(config, "fdc:0"));
	fdconn0.option_add("35hd", FLOPPY_35_HD);
	fdconn0.option_add("35dd", FLOPPY_35_DD);
	fdconn0.option_add("525hd", FLOPPY_525_HD);
	fdconn0.option_add("525dd", FLOPPY_525_DD);
	fdconn0.set_default_option("35hd");
	fdconn0.set_formats(floppy_image_device::default_pc_floppy_formats);

	floppy_connector &fdconn1(FLOPPY_CONNECTOR(config, "fdc:1"));
	fdconn1.option_add("35hd", FLOPPY_35_HD);
	fdconn1.option_add("35dd", FLOPPY_35_DD);
	fdconn1.option_add("525hd", FLOPPY_525_HD);
	fdconn1.option_add("525dd", FLOPPY_525_DD);
	fdconn1.set_default_option("35hd");
	fdconn1.set_formats(floppy_image_device::default_pc_floppy_formats);

	at_keyboard_device &at_keyb(AT_KEYB(config, "at_keyboard", pc_keyboard_device::KEYBOARD_TYPE::AT, 1));
	at_keyb.keypress().set("kbdc", FUNC(kbdc8042_device::keyboard_w));
}


//**************************************************************************
//  ROM DEFINITIONS
//**************************************************************************

ROM_START( proto1 )
	ROM_REGION32_BE(0x1000000, "flash", 0)
	ROM_DEFAULT_BIOS("v0")
	ROM_SYSTEM_BIOS(0, "v0",  "Dev")
	ROM_LOAD("proto1_firmware.bin", 0, 0x100000, "")
ROM_END


} // anonymous namespace


//**************************************************************************
//  SYSTEM DRIVERS
//**************************************************************************

//   YEAR  NAME    PARENT  COMPAT  MACHINE INPUT   CLASS         INIT        COMPANY    FULLNAME       FLAGS
COMP(2024, proto1, 0,      0,      proto1, proto1, proto1_state, empty_init, "kms1212", "EHBC Proto1", MACHINE_IMPERFECT_GRAPHICS | MACHINE_NO_SOUND | MACHINE_SUPPORTS_SAVE)
