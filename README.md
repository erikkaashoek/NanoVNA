tinySA - tiny Spectrum Analyzer
==========================================================

[![GitHub release](http://img.shields.io/github/release/erikkaashoek/tinySA.svg?style=flat)][release]
[![CircleCI](https://circleci.com/gh/erikkaashoek/tinySA.svg?style=shield)](https://circleci.com/gh/erikkaashoek/tinySA)

[release]: https://github.com/erikkaashoek/tinySA/releases

<div align="center">
<img src="/doc/tinySA.jpg" width="480px">
</div>

# About

tinySA is very tiny handheld Spectrum Analyzer (SA). It is
standalone with lcd display, portable device with battery. This
project aim to provide an useful instrument for the RF 
enthusiast.

This repository contains source of tinySA firmware.

## Prepare ARM Cross Tools

**UPDATE**: Recent gcc version works to build tinySA, no need old version.

### MacOSX

Install cross tools and firmware updating tool.

    $ brew tap px4/px4
    $ brew install gcc-arm-none-eabi-80
    $ brew install dfu-util

### Linux (ubuntu)

Download arm cross tools from [here](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads).

    $ wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/8-2018q4/gcc-arm-none-eabi-8-2018-q4-major-linux.tar.bz2
    $ sudo tar xfj gcc-arm-none-eabi-8-2018-q4-major-linux.tar.bz2 -C /usr/local
    $ PATH=/usr/local/gcc-arm-none-eabi-8-2018-q4-major/bin:$PATH
    $ sudo apt install -y dfu-util

## Fetch source code

Fetch source and submodule.

    $ git clone https://github.com/erikkaashoek/tinySA.git
    $ cd tinySA
    $ git submodule update --init --recursive

## Build

Just make in the directory.

    $ make

### Build firmware using docker

Using [this docker image](https://hub.docker.com/r/edy555/arm-embedded) and without installing arm toolchain, you can build the firmware.

    $ cd tinySA
    $ docker run -it --rm -v $(PWD):/work edy555/arm-embedded:8.2 make

## Flash firmware

First, make device enter DFU mode by one of following methods.

* Jumper BOOT0 pin at powering device
* Select menu Config->DFU (needs recent firmware)

Then, flash firmware using dfu-util via USB.

    $ dfu-util -d 0483:df11 -a 0 -s 0x08000000:leave -D build/ch.bin

Or simply use make.

    $ make flash

## Companion Tools

There are several numbers of great companion PC tools from third-party.

* [Soon to come](https://github.com/erikkaashoek/tinySA-Win) by Erik
* [tinySASaver](https://github.com/erikkaashoek/tinySA-saver) by Erik

## Documentation

* [tinySA User Guide](https://tinySA.org/wiki/)
* [tinySA Firmware Release Notes](https://github.com/erikkaashoek/tinySA/tree/master/doc/releases.txt)

## Reference

* [Specification](https://tinysa.org/wiki/pmwiki.php?n=Main.Specification)
* [Technical info](https://tinysa.org/wiki/pmwiki.php?n=Main.TechnicalDescription)

## Note

tinySA is a trademark owned by its respective owner. Unauthorized use the the name tinySA not permitted

## Authorized Distributor

* [See the Wiki](https://tinysa.org/wiki/pmwiki.php?n=Main.Buying)

## Credit

* [@erikkaashoek](https://github.com/erikkaashoek)

### Contributors

* [@edy555](https://github.com/edy555)
* [@hugen79](https://github.com/hugen79)
* [@cho45](https://github.com/cho45)
* [@DiSlord](https://github.com/DiSlord/)
