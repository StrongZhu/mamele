// license:BSD-3-Clause
// copyright-holders:Curt Coder
/***************************************************************************

    Intel 8257 DMA Controller emulation

****************************************************************************
                            _____   _____
                 _I/OR   1 |*    \_/     | 40  A7
                 _I/OW   2 |             | 39  A6
                 _MEMR   3 |             | 38  A5
                 _MEMW   4 |             | 37  A4
                  MARK   5 |             | 36  TC
                 READY   6 |             | 35  A3
                  HLDA   7 |             | 34  A2
                 ADSTB   8 |             | 33  A1
                   AEN   9 |             | 32  A0
                   HRQ  10 |     8257    | 31  Vcc
                   _CS  11 |             | 30  D0
                   CLK  12 |             | 29  D1
                 RESET  13 |             | 28  D2
                _DACK2  14 |             | 27  D3
                _DACK3  15 |             | 26  D4
                  DRQ3  16 |             | 25  _DACK0
                  DRQ2  17 |             | 24  _DACK1
                  DRQ1  18 |             | 23  D5
                  DRQ0  19 |             | 22  D6
                   GND  20 |_____________| 21  D7

***************************************************************************/

#ifndef MAME_MACHINE_I8257_H
#define MAME_MACHINE_I8257_H

#pragma once




/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MCFG_I8257_OUT_HRQ_CB(_devcb) \
	downcast<i8257_device &>(*device).set_out_hrq_callback(DEVCB_##_devcb);

#define MCFG_I8257_OUT_TC_CB(_devcb) \
	downcast<i8257_device &>(*device).set_out_tc_callback(DEVCB_##_devcb);

#define MCFG_I8257_IN_MEMR_CB(_devcb) \
	downcast<i8257_device &>(*device).set_in_memr_callback(DEVCB_##_devcb);

#define MCFG_I8257_OUT_MEMW_CB(_devcb) \
	downcast<i8257_device &>(*device).set_out_memw_callback(DEVCB_##_devcb);

#define MCFG_I8257_IN_IOR_0_CB(_devcb) \
	downcast<i8257_device &>(*device).set_in_ior_callback<0>(DEVCB_##_devcb);

#define MCFG_I8257_IN_IOR_1_CB(_devcb) \
	downcast<i8257_device &>(*device).set_in_ior_callback<1>(DEVCB_##_devcb);

#define MCFG_I8257_IN_IOR_2_CB(_devcb) \
	downcast<i8257_device &>(*device).set_in_ior_callback<2>(DEVCB_##_devcb);

#define MCFG_I8257_IN_IOR_3_CB(_devcb) \
	downcast<i8257_device &>(*device).set_in_ior_callback<3>(DEVCB_##_devcb);

#define MCFG_I8257_OUT_IOW_0_CB(_devcb) \
	downcast<i8257_device &>(*device).set_out_iow_callback<0>(DEVCB_##_devcb);

#define MCFG_I8257_OUT_IOW_1_CB(_devcb) \
	downcast<i8257_device &>(*device).set_out_iow_callback<1>(DEVCB_##_devcb);

#define MCFG_I8257_OUT_IOW_2_CB(_devcb) \
	downcast<i8257_device &>(*device).set_out_iow_callback<2>(DEVCB_##_devcb);

#define MCFG_I8257_OUT_IOW_3_CB(_devcb) \
	downcast<i8257_device &>(*device).set_out_iow_callback<3>(DEVCB_##_devcb);

#define MCFG_I8257_OUT_DACK_0_CB(_devcb) \
	downcast<i8257_device &>(*device).set_out_dack_callback<0>(DEVCB_##_devcb);

#define MCFG_I8257_OUT_DACK_1_CB(_devcb) \
	downcast<i8257_device &>(*device).set_out_dack_callback<1>(DEVCB_##_devcb);

#define MCFG_I8257_OUT_DACK_2_CB(_devcb) \
	downcast<i8257_device &>(*device).set_out_dack_callback<2>(DEVCB_##_devcb);

#define MCFG_I8257_OUT_DACK_3_CB(_devcb) \
	downcast<i8257_device &>(*device).set_out_dack_callback<3>(DEVCB_##_devcb);

// HACK: the radio86 and alikes require this, is it a bug in the soviet clone or is there something else happening?
#define MCFG_I8257_REVERSE_RW_MODE(_flag) \
	downcast<i8257_device &>(*device).set_reverse_rw_mode(_flag);

// ======================> i8257_device

class i8257_device :  public device_t,
						public device_execute_interface
{
public:
	// construction/destruction
	i8257_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	DECLARE_READ8_MEMBER( read );
	DECLARE_WRITE8_MEMBER( write );

	DECLARE_WRITE_LINE_MEMBER( hlda_w );
	DECLARE_WRITE_LINE_MEMBER( ready_w );

	DECLARE_WRITE_LINE_MEMBER( dreq0_w );
	DECLARE_WRITE_LINE_MEMBER( dreq1_w );
	DECLARE_WRITE_LINE_MEMBER( dreq2_w );
	DECLARE_WRITE_LINE_MEMBER( dreq3_w );

	template <class Object> devcb_base &set_out_hrq_callback(Object &&cb) { return m_out_hrq_cb.set_callback(std::forward<Object>(cb)); }
	template <class Object> devcb_base &set_out_tc_callback(Object &&cb) { return m_out_tc_cb.set_callback(std::forward<Object>(cb)); }

	template <class Object> devcb_base &set_in_memr_callback(Object &&cb) { return m_in_memr_cb.set_callback(std::forward<Object>(cb)); }
	template <class Object> devcb_base &set_out_memw_callback(Object &&cb) { return m_out_memw_cb.set_callback(std::forward<Object>(cb)); }

	template <unsigned Ch, class Object> devcb_base &set_in_ior_callback(Object &&cb) { return m_in_ior_cb[Ch].set_callback(std::forward<Object>(cb)); }
	template <unsigned Ch, class Object> devcb_base &set_out_iow_callback(Object &&cb) { return m_out_iow_cb[Ch].set_callback(std::forward<Object>(cb)); }

	template <unsigned Ch, class Object> devcb_base &set_out_dack_callback(Object &&cb) { return m_out_dack_cb[Ch].set_callback(std::forward<Object>(cb)); }

	void set_reverse_rw_mode(bool flag) { m_reverse_rw = flag; }

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void execute_run() override;

	int m_icount;

private:
	inline void dma_request(int channel, int state);
	inline bool is_request_active(int channel) const;
	inline void set_hreq(int state);
	inline void set_dack();
	inline void dma_read();
	inline void dma_write();
	inline void advance();
	inline void set_tc(int state);
	bool next_channel();

	bool m_reverse_rw;
	bool m_tc;
	int m_msb;
	int m_hreq;
	int m_hack;
	int m_ready;
	int m_state;
	int m_current_channel;
	int m_last_channel;
	uint8_t m_transfer_mode;
	uint8_t m_status;
	uint8_t m_request;
	uint8_t m_temp;

	devcb_write_line   m_out_hrq_cb;
	devcb_write_line   m_out_tc_cb;

	/* accessors to main memory */
	devcb_read8        m_in_memr_cb;
	devcb_write8       m_out_memw_cb;

	/* channel accessors */
	devcb_read8        m_in_ior_cb[4];
	devcb_write8       m_out_iow_cb[4];
	devcb_write_line   m_out_dack_cb[4];

	struct
	{
		uint16_t m_address;
		uint16_t m_count;
		uint8_t m_mode;
	} m_channel[4];
};


// device type definition
DECLARE_DEVICE_TYPE(I8257, i8257_device)

#endif // MAME_MACHINE_I8257_H
