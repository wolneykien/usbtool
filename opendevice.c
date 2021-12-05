/* Name: opendevice.c
 * Project: usbtool
 * Author: Christian Starkjohann, Andrey Tolstoy
 * Creation Date: 2016-05-26
 * Tabsize: 4
 * Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 *            (c) 2016 by Andrey Tolstoy, LLC Enistek
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 */

/*
General Description:
The functions in this module can be used to find and open a device based on
libusb-1.0
*/

#include <stdio.h>
#include "opendevice.h"

extern libusb_context* usbCtx;

/* ------------------------------------------------------------------------- */

#define MATCH_SUCCESS			1
#define MATCH_FAILED			0
#define MATCH_ABORT				-1

/* private interface: match text and p, return MATCH_SUCCESS, MATCH_FAILED, or MATCH_ABORT. */
static int  _shellStyleMatch(char *text, char *p)
{
int last, matched, reverse;

    for(; *p; text++, p++){
        if(*text == 0 && *p != '*')
            return MATCH_ABORT;
        switch(*p){
        case '\\':
            /* Literal match with following character. */
            p++;
            /* FALLTHROUGH */
        default:
            if(*text != *p)
                return MATCH_FAILED;
            continue;
        case '?':
            /* Match anything. */
            continue;
        case '*':
            while(*++p == '*')
                /* Consecutive stars act just like one. */
                continue;
            if(*p == 0)
                /* Trailing star matches everything. */
                return MATCH_SUCCESS;
            while(*text)
                if((matched = _shellStyleMatch(text++, p)) != MATCH_FAILED)
                    return matched;
            return MATCH_ABORT;
        case '[':
            reverse = p[1] == '^';
            if(reverse) /* Inverted character class. */
                p++;
            matched = MATCH_FAILED;
            if(p[1] == ']' || p[1] == '-')
                if(*++p == *text)
                    matched = MATCH_SUCCESS;
            for(last = *p; *++p && *p != ']'; last = *p)
                if (*p == '-' && p[1] != ']' ? *text <= *++p && *text >= last : *text == *p)
                    matched = MATCH_SUCCESS;
            if(matched == reverse)
                return MATCH_FAILED;
            continue;
        }
    }
    return *text == 0;
}

/* public interface for shell style matching: returns 0 if fails, 1 if matches */
static int shellStyleMatch(char *text, char *pattern)
{
    if(pattern == NULL) /* NULL pattern is synonymous to "*" */
        return 1;
    return _shellStyleMatch(text, pattern) == MATCH_SUCCESS;
}

/* ------------------------------------------------------------------------- */

static int usbGetStringAsciiOrWarn(libusb_device_handle *handle, int index,
								   char *buf, int buflen, FILE *err) {
	int len;

    if (!handle) return -1;
    if (index <= 0) return -1;

	len = libusb_get_string_descriptor_ascii(handle, index, buf, buflen);

	if (len < 0 && err) {
		fprintf(err, "WARNING: Cannot query string: %s\n",
				libusb_error_name(len));
	}

	return len;
}

static void printClass(int classNum, FILE *out) {
	switch (classNum) {
	   case LIBUSB_CLASS_PER_INTERFACE:
		  fprintf(out, "per interface");
		  break;
	   case LIBUSB_CLASS_AUDIO:
		  fprintf(out, "audio");
		  break;
	   case LIBUSB_CLASS_COMM:
		  fprintf(out, "communications");
		  break;
	   case LIBUSB_CLASS_HID:
		  fprintf(out, "HID");
		  break;
	   case LIBUSB_CLASS_PHYSICAL:
		  fprintf(out, "physical");
		  break;
	   case LIBUSB_CLASS_IMAGE:
		  fprintf(out, "image");
		  break;
	   case LIBUSB_CLASS_PRINTER:
		  fprintf(out, "printer");
		  break;
	   case LIBUSB_CLASS_MASS_STORAGE:
		  fprintf(out, "mass storage");
		  break;
	   case LIBUSB_CLASS_HUB:
		  fprintf(out, "hub");
		  break;
	   case LIBUSB_CLASS_DATA:
		  fprintf(out, "data");
		  break;
	   case LIBUSB_CLASS_SMART_CARD:
		  fprintf(out, "smart card");
		  break;
	   case LIBUSB_CLASS_CONTENT_SECURITY:
		  fprintf(out, "content security");
		  break;
	   case LIBUSB_CLASS_VIDEO:
		  fprintf(out, "video");
		  break;
	   case LIBUSB_CLASS_PERSONAL_HEALTHCARE:
		  fprintf(out, "personal healthcare");
		  break;
	   case LIBUSB_CLASS_DIAGNOSTIC_DEVICE:
		  fprintf(out, "diagnostic device");
		  break;
	   case LIBUSB_CLASS_WIRELESS:
		  fprintf(out, "wireless");
		  break;
	   case LIBUSB_CLASS_MISCELLANEOUS:
		  fprintf(out, "misc");
		  break;
	   case LIBUSB_CLASS_APPLICATION:
		  fprintf(out, "app");
	   case LIBUSB_CLASS_VENDOR_SPEC:
		  fprintf(out, "vendor-specific");
		  break;
	   default:
		  fprintf(out, "UNKNOWN!");
	}
}

static void printDetails(libusb_device_handle *handle,
						 libusb_device *dev, FILE *out, FILE *err) {
	struct libusb_device_descriptor desc;
	unsigned char stringbuf[256];

    if (!out) return;

	int r = libusb_get_device_descriptor(dev, &desc);
	if (r < 0) {
        fprintf(err, "Warning: Failed to get device descriptor.\n");
		return;
	}

    fprintf(out, "  Device class: %02Xh ", desc.bDeviceClass);
	printClass(desc.bDeviceClass, out);
	fprintf(out, "\n");

	fprintf(out, "  Subclass: %02Xh\n", desc.bDeviceSubClass);

	fprintf(out, "  Protocol: %02Xh\n", desc.bDeviceProtocol);

    fprintf(out, "  Configurations (%i):\n", desc.bNumConfigurations);

	for (int c = 0; c < desc.bNumConfigurations; c++) {
		struct libusb_config_descriptor *config = NULL;
		libusb_get_config_descriptor(dev, c, &config);

		fprintf(out, "    [%i] Configuration: %i %02Xh\n",
				c, config->bConfigurationValue,
				config->bConfigurationValue);

		if (usbGetStringAsciiOrWarn(handle, config->iConfiguration,
									stringbuf, sizeof(stringbuf),
									err) > 0) {
			fprintf(out, "      Description: %s\n", stringbuf);
		}

		fprintf(out, "      Interfaces (%i):\n", config->bNumInterfaces);

		const struct libusb_interface *inter;
		const struct libusb_interface_descriptor *interdesc;
		const struct libusb_endpoint_descriptor *epdesc;

		for (int i = 0; i < config->bNumInterfaces; i++) {
			inter = &config->interface[i];

			fprintf(out, "        [%i] Alternate settings (%i):\n",
					i, inter->num_altsetting);

			for(int j = 0; j < inter->num_altsetting; j++) {
				interdesc = &inter->altsetting[j];

				fprintf(out, "          [%i] Setting: %i %02Xh\n",
						j, interdesc->bAlternateSetting,
						interdesc->bAlternateSetting);

				fprintf(out, "            Interface number: %i %02Xh\n",
						interdesc->bInterfaceNumber,
						interdesc->bInterfaceNumber);

				fprintf(out, "            Interface class: %02Xh ",
						interdesc->bInterfaceClass);
				printClass(interdesc->bInterfaceClass, out);
				fprintf(out, "\n");

				fprintf(out, "            Subclass: %02Xh\n",
						interdesc->bInterfaceSubClass);

				fprintf(out, "            Protocol: %02Xh\n",
						interdesc->bInterfaceProtocol);

				if (usbGetStringAsciiOrWarn(handle, interdesc->iInterface,
											stringbuf, sizeof(stringbuf),
											err) > 0) {
					fprintf(out, "            Description: %s\n", stringbuf);
				}

				fprintf(out, "            Endpoints (%i):\n",
						interdesc->bNumEndpoints);

				for(int k = 0; k < interdesc->bNumEndpoints; k++) {
					epdesc = &interdesc->endpoint[k];

					fprintf(out, "              [%i] Endpoint: %i %02Xh ",
							k, epdesc->bEndpointAddress,
							epdesc->bEndpointAddress);
                    switch (epdesc->bmAttributes & 0x04) {
                    switch (epdesc->bmAttributes & 0x03) {
    				   case LIBUSB_ENDPOINT_TRANSFER_TYPE_CONTROL:
    					  fprintf(out, "control");
    					  break;
    				   case LIBUSB_ENDPOINT_TRANSFER_TYPE_ISOCHRONOUS:
    					  fprintf(out, "isochronous");
    					  switch (epdesc->bmAttributes & 0x0c) {
    						 case LIBUSB_ISO_SYNC_TYPE_NONE:
    							fprintf(out, " nosync");
    							break;
    						 case LIBUSB_ISO_SYNC_TYPE_ASYNC:
    							fprintf(out, " async");
    							break;
    						 case LIBUSB_ISO_SYNC_TYPE_ADAPTIVE:
    							fprintf(out, " adaptive");
    							break;
    						 case LIBUSB_ISO_SYNC_TYPE_SYNC:
    							fprintf(out, " sync");
    							break;
    						 default:
    							fprintf(out, " UNKNOWN!");
    					  }
    					  switch (epdesc->bmAttributes & 0x30) {
    						 case LIBUSB_ISO_USAGE_TYPE_DATA:
    							fprintf(out, " data");
    							break;
    						 case LIBUSB_ISO_USAGE_TYPE_FEEDBACK:
    							fprintf(out, " feedback");
    							break;
    						 case LIBUSB_ISO_USAGE_TYPE_IMPLICIT:
    							fprintf(out, " implicit");
    						 default:
    							fprintf(out, " UNKNOWN!");
    					  }
    					  break;
    				   case LIBUSB_ENDPOINT_TRANSFER_TYPE_BULK:
    					  fprintf(out, "bulk");
    					  break;
    				   case LIBUSB_ENDPOINT_TRANSFER_TYPE_INTERRUPT:
    					  fprintf(out, "interrupt");
    					  break;
    				   default:
    					  fprintf(out, "UNKNOWN!");
    				}
    				fprintf(out, "\n");
				}
			}
		}

		libusb_free_config_descriptor(config);
	}
}

int usbOpenDevice(libusb_device_handle **device, int vendorID, char *vendorNamePattern, int productID, char *productNamePattern, char *serialNamePattern, FILE *printMatchingDevicesFp, FILE *warningsFp, int verbose)
{
    libusb_device_handle      *handle = NULL;
    int                 errorCode = USBOPEN_ERR_NOTFOUND;

    libusb_device **devs;

    int cnt = libusb_get_device_list(usbCtx, &devs);
    for (int i = 0; i < cnt; i++) {
        libusb_device* dev = devs[i];
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0)
            continue;

        if((vendorID == 0 || desc.idVendor == vendorID) 
           && (productID == 0 || desc.idProduct == productID)) {
            unsigned char vendor[256], product[256], serial[256];
            int len = 0;
            r = libusb_open(dev, &handle);
            if (r < 0) {
                errorCode = USBOPEN_ERR_ACCESS;
                if(warningsFp != NULL)
                    fprintf(warningsFp, "Warning: cannot open VID=0x%04x PID=0x%04x: %s\n", desc.idVendor, desc.idProduct, libusb_error_name(r));
                continue;
            }

            if (desc.idVendor == vendorID && desc.idProduct == productID)
                break;

            len = vendor[0] = 0;
            if (desc.iManufacturer > 0) {
                len = libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, vendor, sizeof(vendor));
            }

            if (len < 0) {
                errorCode = USBOPEN_ERR_ACCESS;
                if(warningsFp != NULL)
                    fprintf(warningsFp, "Warning: cannot query manufacturer for VID=0x%04x PID=0x%04x: %s\n", desc.idVendor, desc.idProduct, libusb_error_name(len));
            } else {
                errorCode = USBOPEN_ERR_NOTFOUND;
                /* printf("seen device from vendor ->%s<-\n", vendor); */
                if(shellStyleMatch(vendor, vendorNamePattern)){
                    len = product[0] = 0;
                    if(desc.iProduct > 0){
                        len = libusb_get_string_descriptor_ascii(handle, desc.iProduct, product, sizeof(product));
                    }
                    if(len < 0){
                        errorCode = USBOPEN_ERR_ACCESS;
                        if(warningsFp != NULL)
                            fprintf(warningsFp, "Warning: cannot query product for VID=0x%04x PID=0x%04x: %s\n", desc.idVendor, desc.idProduct, libusb_error_name(len));
                    }else{
                        errorCode = USBOPEN_ERR_NOTFOUND;
                        /* printf("seen product ->%s<-\n", product); */
                        if(shellStyleMatch(product, productNamePattern)){
                            len = serial[0] = 0;
                            if(desc.iSerialNumber > 0){
                                len = libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, serial, sizeof(serial));
                            }
                            if(len < 0){
                                errorCode = USBOPEN_ERR_ACCESS;
                                if(warningsFp != NULL)
                                    fprintf(warningsFp, "Warning: cannot query serial for VID=0x%04x PID=0x%04x: %s\n", desc.idVendor, desc.idProduct, libusb_error_name(len));
                            }
                            if(shellStyleMatch(serial, serialNamePattern)){
                                if(printMatchingDevicesFp != NULL){
                                    if(serial[0] == 0){
                                        fprintf(printMatchingDevicesFp, "VID=0x%04x PID=0x%04x vendor=\"%s\" product=\"%s\"\n", desc.idVendor, desc.idProduct, vendor, product);
                                    }else{
                                        fprintf(printMatchingDevicesFp, "VID=0x%04x PID=0x%04x vendor=\"%s\" product=\"%s\" serial=\"%s\"\n", desc.idVendor, desc.idProduct, vendor, product, serial);
                                    }
									if (verbose)
										printDetails(handle, dev, printMatchingDevicesFp, warningsFp);
                                }else{
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            libusb_close(handle);
            handle = NULL;
        }

    }
    libusb_free_device_list(devs, 1);

    if(handle != NULL){
        errorCode = 0;
        *device = handle;
    }
    if(printMatchingDevicesFp != NULL)  /* never return an error for listing only */
        errorCode = 0;
    return errorCode;
}

/* ------------------------------------------------------------------------- */
