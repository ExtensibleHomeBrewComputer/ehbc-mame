// license:BSD-3-Clause
// copyright-holders:Nathan Woods, Miodrag Milanovic
/*********************************************************************

    bitbngr.c

*********************************************************************/

#include "emu.h"
#include "bitbngr.h"

#include "softlist_dev.h"

#include <cstring>



/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

DEFINE_DEVICE_TYPE(BITBANGER, bitbanger_device, "bitbanger", "Bitbanger")

/*-------------------------------------------------
    ctor
-------------------------------------------------*/

bitbanger_device::bitbanger_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, BITBANGER, tag, owner, clock),
	device_image_interface(mconfig, *this),
	m_interface(nullptr),
	m_is_readonly(false)
{
}



/*-------------------------------------------------
    native_output - outputs data to a file
-------------------------------------------------*/

void bitbanger_device::output(uint8_t data)
{
	if (!loaded_through_softlist() && exists())
		fwrite(&data, 1);
}

/*-------------------------------------------------
	native_output - outputs data buffer to a file
-------------------------------------------------*/

int bitbanger_device::send(uint8_t *buf, int len)
{
	if (exists())
	{
		return fwrite(buf, len);
	}
	// Announce we transfered everything
	return len;
}

/*-------------------------------------------------
    native_input - inputs data from a file
-------------------------------------------------*/

uint32_t bitbanger_device::input(void *buffer, uint32_t length)
{
	if (exists())
		return fread(buffer, length);
	else
		return 0;
}

/*-------------------------------------------------
	flush_rx - Flushes all receive data from stream
-------------------------------------------------*/
void bitbanger_device::flush_rx(void)
{
	if (exists())
	{
		int rx_bytes;
		auto tmp_buf = std::make_unique<uint8_t[]>(512);
		while ((rx_bytes = fread(tmp_buf.get(), 512)))
		{
			//printf("Flushing rx %d bytes\n", rx_bytes);
		}
	}
}


/*-------------------------------------------------
    device_start
-------------------------------------------------*/

void bitbanger_device::device_start()
{
}



/*-------------------------------------------------
    get_software_list_loader
-------------------------------------------------*/

const software_list_loader &bitbanger_device::get_software_list_loader() const
{
	return image_software_list_loader::instance();
}



/*-------------------------------------------------
    call_load
-------------------------------------------------*/

image_init_result bitbanger_device::call_load()
{
	// we don't need to do anything special
	return image_init_result::PASS;
}

image_init_result bitbanger_device::call_create(int format_type, util::option_resolution *format_options)
{
	// we don't need to do anything special
	return image_init_result::PASS;
}

/*-------------------------------------------------
    call_unload
-------------------------------------------------*/

void bitbanger_device::call_unload()
{
	// we don't need to do anything special
}
