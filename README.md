This is the README file for `usbtool` --- a general purpose command
line utility which can send USB requests to arbitrary USB devices.
Usbtool is based on [`libusb`](https://libusb.info/).


WHAT USBTOOL IS GOOD FOR?
-------------------------

When you implement an USB-based communication protocol, you usually
have to write two programs: one on each end of the communication. For
USB, this means that you have to write a firmware for the device and
a driver for the host.

Usbtool can help you for both parts during development and testing.
Usbtool can send control-in and -out requests to arbitrary devices and
send and receive data on interrupt- and bulk-endpoints.

Usbtool is not only a useful development tool, it's also an example of
`libusb` programming.


SYNOPSIS
--------

    usbtool [options] <command>


COMMANDS
--------

  * `list`:  This command prints a list of devices found on all available
    USB busses. Options `-v`, `-V`, `-p` and `-P` can be used to filter
    the list.

  * `info`: Prints information about each matching device. Options `-v`,
      `-V`, `-p` and `-P` can be used to filter the list.

  * `control in|out <type> <recipient> <request> <value> <index>`:
    Sends a control-in or control-out request to the device. The request
    parameters are:

    * `<type>`:  the type of the request, which can be `standard`,
      `class`, `vendor` or `reserved`. The type determines which
      software module in the device is responsible for answering the
      request.  Standard requests are answered by the driver, class
      requests by the class implementation (e. g. HID, CDC) and
      vendor requests by custom code.

    * `<recipient>`:  the recipient of the request in the device, which
       can be `device`, `interface`, `endpoint` or `other`. For standard
       and class requests, the specification defines a recipient for
       each request. For vendor requests, choose whatever your code
       expects.

    * `<request>`: 8 bit numeric value identifying the request.

    * `<value>`: 16 bit numeric value passed to the device.

    * `<index>`: another 16 bit numeric value passed to the device.

    Use options `-v`, `-V`, `-p` and `-P` to select out the particular
    device. Use options `-d` or `-D` to to send data in an OUT request.
    Use options `-n`, `-O` and `-b` to determine what to do with data
    received in an IN request.

  * `interrupt in|out`:  Sends or receives data on an OUT or IN
    interrupt endpoint respectively. Use options `-v`, `-V`, `-p` and
    `-P` to select out the particular device. Use options `-d` or `-D`
    to to send data to an OUT endpoint. Use options `-n`, `-O` and
    `-b` to determine what to do with data received from an IN
    endpoint. Use the option `-e` to set the endpoint number, `-c` to
    choose a configuration and `-i` to claim the particular interface.

  * `bulk in|out`: Same as `interrupt in` and `interrupt out` but for
    bulk endpoints.


OPTIONS
-------

Most options have already been mentioned with the commands which use them.
Here is a complete list:

  * `-h or -?`:  Prints a short help.

  * `-v <vendor-id>`:  Numeric vendor ID. Selects devices with
    matching vendor IDs only. The default is `*` which allows any VID.

  * `-p <product-id>`:  Numeric product IDSelects devices with
    matching vendor IDs only. The default is `*` which allows any PID.

  * `-V <vendor-name-pattern>`:  Shell style matching pattern for
    vendor name. Only the devices which have a vendor name that
    matches this pattern are taken into account. The default is `*`
    (any name).

  * `-P <product-name-pattern>`:  Shell style matching pattern for
    product name. Only the devices which have a product name that
    matches this pattern are taken into account. The default is `*`
    (any name).

  * `-S <serial-pattern>`:  Shell style matching pattern for serial
    number. Only the devices which have a serial that matches this
    pattern are taken into account. The default is `*` (any serial).

  * `-d <databytes>`:  The string of byte values to send to the
    device. Comma-separated list of numeric values, e. g.: `1,2,3,4,5`
    or `0x06, 0x07, 0x08, 0x09, 0x0a`.

  * `-D <file>`:  The file containing binary data to sent to the device.

  * `-O <file>`: Write received data to this file. Format is either hex or
    binary, depending on the `-b` flag. By default, received data is printed
    to standard output.

  * `-b`:  Request binary output format for files and standard output.
    Default is a hexadecimal listing.

  * `-n <count>`:  The maximum number of bytes to receive.

  * `-e <endpoint>`:  The endpoint number for the `interrupt` and `bulk`
    commands.

  * `-t <timeout>`:  Timeout in milliseconds for the request.

  * `-c <configuration>`:  The configuration _number_. Interrupt and
    bulk endpoints can usually only be used if a configuration and an
    interface (see below) is chosen.

  * `-i <interface>`:  The interface _number_.

  * `-w`:  Suppress warnings (from `usbtool` and `libusb`).

  * `-I`:  Show more information about each device in the list (to use
    with the `list` command).


NUMERIC VALUES
--------------

All numeric values can be given in hexadecimal, decimal or octal. Hex values
are identified by their `0x` or `0X` prefix, octal values by a leading
"0" (the digit zero) and decimal values because they start with a
non-zero digit. An optional sign character is allowed. The special
value `*` is translated to zero and stands for "any value" in some contexts.


SHELL STYLE MATCHING PATTERNS
-----------------------------

Some options take shell style matching patterns as an argument. This refers
to UNIX shells and their file wildcard operations:

  + `*` (the asterisk character) matches any number (0 to infinite) of any
    characters.

  + `?`:  matches exactly one arbitrary character.

  + A list of characters in square brackets (e. g. `[abc]`) matches
    any of the characters in the list.

    * Use the dash (`-`) to specify a character range, otherwise it
    should be the first or the last character in the list.

    * Use caret (`^`) as the first character to specify a
    complementary list (i. e.: `[^abc]` indicates "not `a`, `b` or `c`
    but any other character. To suppress this behavior place `^` at an
    other place but the first.

    * To specify the literal `]` place it as the first character.

    * The entire construct matches only one character.

  + `\` (backslash) followed by any character matches that literal
    character. This can be used to avoid special treatment of `*`,
    `?`, `[` and the `\` itself.


BUILDING USBTOOL
----------------

Usbtool uses `libusb` on UNIX and `libusb-win32` on Windows. These
libraries can be obtained from http://libusb.sourceforge.net/ and
http://libusb-win32.sourceforge.net/ respectively. On UNIX, a simple
"make" should compile the sources (although you may need to edit
`Makefile` to include or remove additional libraries; you can also
redifine its variables on the command line). On Windows, we recommend
that you use MinGW and MSYS. See the top level README file for
details. Edit `Makefile.windows` according to your library
installation paths and build with `make -f Makefile.windows`.


EXAMPLES
--------

To list all devices connected to your computer, use

    usbtool list

To print the detailed information about specific device, use

    usbtool -v <vendorID> -p <productID> info

You can also send commands to the device using `usbtool`. For example,
we know that the set-status request of the `LEDControl` device has
numeric value 1 and the get-status request is 2. We can therefore
query the status with

    usbtool -P LEDControl control in vendor device 2 0 0

It will print `0x00` if the LED is off or `0x01` if it is on.
Them to turn the LED on, use

    usbtool -P LEDControl control out vendor device 1 1 0

and to turn it off, use

    usbtool -w -P LEDControl control out vendor device 1 0 0


COPYRIGHT
---------

* (C) 2008 by OBJECTIVE DEVELOPMENT Software GmbH. http://www.obdev.at/

* (C) 2016 Andrey Tolstoy, LLC Enistek.

* (C) 2021 Paul Wolneykien.

Released under GNU GPL v3 (see COPYING).
