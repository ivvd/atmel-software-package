/* ----------------------------------------------------------------------------
 *         SAM Software Package License
 * ----------------------------------------------------------------------------
 * Copyright (c) 2016, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */
/**
 * \page usb_video USB Video Example
 *
 * \section Purpose
 *
 * The USB Video Example will help you to get familiar with the
 * USB Device Port(UDP) and ISC interface on SAMA5D2 microcontrollers.
 *
 * \section Requirements
 *
 * - On-board ISC interface.
 * - External sensor, in the example, Omnivision OV2643/OV5640/OV7740/OV7670/OV9740
 * sensor could be used.
 * When an EK running this program connected to a host (PC for example), with
 * USB cable, the EK appears as a video camera for the host.
 *
 * \note
 * For the limitation of external memory size, this example only support for
 * VGA/QVGA format.
 *
 * \section Description
 * The USB video can help you to be familiar with the ISC (Image Sensor
 * controller) to connects a CMOS-type image sensor to the processor and
 * provides image capture in various formats.
 * Data stream Pipe line: ISC PFE->RLP(DAT8)->DAM8->USB YUV2 display
 *
 * \section Usage
 *
 * -# On the computer, open and configure a terminal application
 *    (e.g. HyperTerminal on Microsoft Windows) with these settings:
 *   - 115200 baud rates
 *   - 8 bits of data
 *   - No parity
 *   - 1 stop bit
 *   - No flow control
 * -# Start the application.
 * -# In the terminal window, the following text should appear:
 *     \code
 *     -- USB UVC ISC Example xxx --
 *     -- SAMxxxxx-xx
 *     -- Compiled: xxx xx xxxx xx:xx:xx --
 *     \endcode
 * -# When connecting USB cable to windows, the
 *    new "xxx USB Device" appears in the
 *    hardware %device list.
 * -# Once the device is connected and configured on windows XP,
 *    "USB Video Device" will appear in "My Computer", you can double click
 *    it to preview with default resolution - QVGA.
 * -# Other video camera programs can also be used to monitor the capture
 *    output. The demo is tested on windows XP through "AmCap.exe".
 *
 * \section References
 * - usb_uvc_isc/main.c
 */

/**
 * \file
 *
 * This file contains all the specific code for the
 * usb_uvc_isc
 *
 * \section Contents
 *
 * The code can be roughly broken down as follows:
 *    - Configuration functions
 *       - Configure TWI
 *       - Configure pins for OV sensor
 *       - Configure ISC controller
 *    - Interrupt handlers
 *       - ISC_Handler
 *    - The main function, which implements the program behaviour
 */

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "board.h"
#include "chip.h"
#include "trace.h"
#include "compiler.h"

#include "gpio/pio.h"

#include "mm/cache.h"
#include "serial/console.h"

#include "peripherals/pit.h"
#include "peripherals/pmc.h"

#include "video/image_sensor_inf.h"
#include "video/isc.h"
#include "video/iscd.h"

#include "usb/common/uvc/usb_video.h"
#include "usb/common/uvc/uvc_descriptors.h"
#include "usb/device/usbd_driver.h"
#include "usb/device/usbd.h"
#include "usb/device/usbd_hal.h"
#include "usb/device/uvc/uvc_driver.h"
#include "usb/device/uvc/uvc_function.h"

#include "../usb_common/main_usb_common.h"

#include <string.h>
#include <stdbool.h>
#include <stdint.h>

/*----------------------------------------------------------------------------
 *        Local definitions
 *----------------------------------------------------------------------------*/

#define NUM_FRAME_BUFFER     4

#define SENSOR_TWI_BUS BOARD_ISC_TWI_BUS

/*----------------------------------------------------------------------------
 *          External variables
 *----------------------------------------------------------------------------*/

extern const USBDDriverDescriptors usbdDriverDescriptors;

/*----------------------------------------------------------------------------
 *        Local variables
 *----------------------------------------------------------------------------*/

/* Image output bit width */
static uint8_t sensor_output_bit_width;

/* Image resolution */
static uint8_t image_resolution = QVGA;

/* Image size in preview mode */
static uint32_t image_width, image_height;

static uint8_t frame_format;

/** Sensor profile */
static struct sensor_profile *sensor;

static struct _iscd_desc iscd;

/** Video buffers */
CACHE_ALIGNED_DDR
static uint8_t stream_buffers[FRAME_BUFFER_SIZEC(640, 480) * NUM_FRAME_BUFFER];

/*----------------------------------------------------------------------------
 *        Local functions
 *----------------------------------------------------------------------------*/

static void isc_vd_callback(uint8_t frame_idx)
{
	uvc_function_update_frame_idx(frame_idx);
}

/**
 * \brief ISC initialization.
 */
static void configure_isc(void)
{
	iscd.cfg.input_bits = sensor_output_bit_width;
	/* For sensor 8-bit output, it is recommand to perform
		12 bits to 10 bits compression is performed skipping two bits.*/
	iscd.pipe.gamma.gamma_enable = false;
	iscd.pipe.cs = NULL;
	iscd.pipe.color_correction = NULL;
	iscd.cfg.input_format = YUV_422;
	iscd.pipe.histo_enable = false;
	iscd.pipe.histo_buf = NULL;

	iscd.cfg.multi_bufs = NUM_FRAME_BUFFER;
	iscd.pipe.rlp_mode = ISCD_RLP_MODE_DAT8;
	iscd.cfg.layout = ISCD_LAYOUT_PACKED8;
	iscd.dma.address0 = (uint32_t)stream_buffers;
	iscd.dma.size = FRAME_BUFFER_SIZEC(image_width, image_height);
	iscd.dma.callback = isc_vd_callback;
	iscd_pipe_start(&iscd);
}

static void start_preview(void)
{
	uint8_t sensor_mode = YUV_422;

	/* Re-configure sensor with giving resolution */
	if (sensor_setup(SENSOR_TWI_BUS, sensor, image_resolution, sensor_mode) != SENSOR_OK) {
		printf("-E- Sensor setup failed.");
		while (1);
	}
	/* Retrieve sensor output format and size */
	sensor_get_output(sensor, image_resolution, sensor_mode, &sensor_output_bit_width,
			&image_width, &image_height);

	printf("-I- Bit width = %d, Image Width = %d, Image Height=%d \n\r",
			(unsigned)(sensor_output_bit_width + 8),
			(unsigned)image_width, (unsigned)image_height);

	/* Configure ISC */
	configure_isc();
}

/**
 *  Invoked whenever a SETUP request is received from the host. Forwards the
 *  request to the standard handler.
 */
void usbd_callbacks_request_received(const USBGenericRequest *request)
{
	uvc_driver_request_handler(request);
}

/**
 * Invoked whenever the active setting of an interface is changed by the
 * host. Reset streaming interface.
 * \param interface Interface number.
 * \param setting Newly active setting.
 */
void usbd_driver_callbacks_interface_setting_changed(uint8_t interface, uint8_t setting )
{
	uvc_driver_interface_setting_changed_handler(interface, setting);
}

/*----------------------------------------------------------------------------
 *        Global functions
 *----------------------------------------------------------------------------*/

/**
 * \brief Application entry point for ISI USB video example.
 *
 * \return Unused (ANSI-C compatibility).
 */
extern int main( void )
{
	bool is_usb_vid_on = false;

	/* Output example information */
	console_example_info("USB UVC ISC Example");

	printf("Image sensor detection:\n\r");
	if ((sensor = sensor_detect(SENSOR_TWI_BUS, true, 0))) {
		if (sensor_setup(SENSOR_TWI_BUS, sensor, VGA, YUV_422) != SENSOR_OK){
			printf("-E- Sensor setup failed.");
			while (1);
		}
	} else {
		printf("-E- Can't detect sensor connected to board\r\n");
		while (1);
	}

	usb_power_configure();

	uvc_driver_initialize(&usbdDriverDescriptors, (uint32_t)stream_buffers, NUM_FRAME_BUFFER);

	/* connect if needed */
	usb_vbus_configure();

	/* clear video buffer */

	while (1) {
		if (usbd_get_state() < USBD_STATE_CONFIGURED) {
			continue;
		}

		if (is_usb_vid_on) {
			if (!uvc_function_is_video_on()) {
				is_usb_vid_on = false;
				isc_stop_capture();
				isc_disable_interrupt(-1);
				printf("CapE\r\n");
				printf("vidE\r\n");
			}
		} else {
			if (uvc_function_is_video_on()) {
				is_usb_vid_on = true;
				frame_format = uvc_function_get_frame_format();
				if (frame_format == 1) {
					image_resolution = QVGA;
				}
				else if (frame_format == 2) {
					image_resolution = VGA;
				} else {
					printf ("-I- Only support VGA and QVGA format\r\n");
					image_resolution = QVGA;
				}
				memset(stream_buffers, 0, sizeof(stream_buffers));
				cache_clean_region(stream_buffers, sizeof(stream_buffers));
				start_preview();
				uvc_function_payload_sent(NULL, USBD_STATUS_SUCCESS, 0, 0);
				printf("vidS\r\n");
			}
		}
	}
}
