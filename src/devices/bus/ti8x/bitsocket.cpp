// license:BSD-3-Clause
// copyright-holders:Vas Crabb
/*
 Sends raw assert/release signals over a socket.  Seriously limits
 transfer rates and probably won't work if there's much latency, but it
 allows communication between instances using non-standard protocols.

 bit 0 = data
 bit 1 = set tip
 bit 2 = set ring
 */

#include "emu.h"
#include "bitsocket.h"

#include "imagedev/bitbngr.h"


namespace {

class bit_socket_device
		: public device_t
		, public device_ti8x_link_port_interface
{
public:
	bit_socket_device(machine_config const &mconfig, char const *tag, device_t *owner, uint32_t clock);

protected:
	enum
	{
		TIMER_ID_POLL = 1
	};

	virtual void device_add_mconfig(machine_config &config) override;
	virtual void device_start() override;
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param) override;

	virtual DECLARE_WRITE_LINE_MEMBER(input_tip) override;
	virtual DECLARE_WRITE_LINE_MEMBER(input_ring) override;

private:
	required_device<bitbanger_device>   m_stream;
	emu_timer *                         m_poll_timer;
	bool                                m_tip_in, m_ring_in;
};


bit_socket_device::bit_socket_device(
		machine_config const &mconfig,
		char const *tag,
		device_t *owner,
		uint32_t clock)
	: device_t(mconfig, TI8X_BIT_SOCKET, tag, owner, clock)
	, device_ti8x_link_port_interface(mconfig, *this)
	, m_stream(*this, "stream")
	, m_poll_timer(nullptr)
	, m_tip_in(true)
	, m_ring_in(true)
{
}


void bit_socket_device::device_add_mconfig(machine_config &config)
{
	BITBANGER(config, m_stream, 0);
}


void bit_socket_device::device_start()
{
	m_poll_timer = timer_alloc(TIMER_ID_POLL);

	save_item(NAME(m_tip_in));
	save_item(NAME(m_ring_in));

	m_tip_in = m_ring_in = true;

	m_poll_timer->adjust(attotime::from_hz(200000), 0, attotime::from_hz(200000));
}


void bit_socket_device::device_timer(emu_timer &timer, device_timer_id id, int param)
{
	switch (id)
	{
	case TIMER_ID_POLL:
		{
			u8 data;
			while (m_stream->input(&data, 1))
			{
				if (BIT(data, 1)) output_tip(BIT(data, 0));
				if (BIT(data, 2)) output_ring(BIT(data, 0));
			}
		}
		break;

	default:
		break;
	}
}


WRITE_LINE_MEMBER(bit_socket_device::input_tip)
{
	m_tip_in = bool(state);
	m_stream->output((m_tip_in ? 0x01 : 0x00) | 0x02);
}


WRITE_LINE_MEMBER(bit_socket_device::input_ring)
{
	m_ring_in = bool(state);
	m_stream->output((m_ring_in ? 0x01 : 0x00) | 0x04);
}

} // anonymous namespace


DEFINE_DEVICE_TYPE_PRIVATE(TI8X_BIT_SOCKET, device_ti8x_link_port_interface, bit_socket_device, "ti8x_bitsock", "TI-8x Bit Socket")
