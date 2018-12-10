iNFekt
===
[![](https://img.shields.io/website-up-down-green-red/https/infekt.ws.svg?label=website)](https://infekt.ws/)
[![](https://img.shields.io/github/tag/syndicodefront/infekt.svg)](https://infekt.ws/index.html#downloads)


A text viewer application that has been carefully designed around its main task: viewing and presenting [NFO files](http://en.wikipedia.org/wiki/.nfo). It comes with three different view modes (Rendered, Classic and Text Only), export functionality and lots of options!

All Windows versions starting with Windows XP are supported. iNFekt works very well with any Windows version and is available in a native 64 bit flavor.</p>

iNFekt is a native application written in C++. It uses the Cairo open source graphics libraries. Where available, rendering uses multi-core CPUs and is accelerated by GPU hardware.

## Key features

- Display NFO files of all common charsets (including CP 437 and Unicode)
- Fix & display many broken encodings that other viewers choke on
- You can fine-tune colors for text, background, block art and more
- Clickable hyperlinks, including links that span multiple lines
- Enhanced rendering mode / Classic mode / Text-only mode
- Basic ANSI Art support (16+ colors)
- Anti-aliasing
- Built-in zoom
- Export functionality
- Auto-refresh and browse folder features
- Integrated updater
- Free and Open Source
- Portable versions available
- Command line version for easy conversions to PNG, Unicode, HTML, PDF...

## Screenshots

<div>
  <img src="https://infekt.ws/screens/infekt-screen-1.png" width="200">
  <img src="https://infekt.ws/screens/infekt-screen-2.png" width="200">
  <img src="https://infekt.ws/screens/infekt-screen-3.png" width="200">
  <img src="https://infekt.ws/screens/infekt-screen-4.png" width="200">
</div>

## Downloads

You can find downloads on the [iNFekt website](https://infekt.ws/index.html#downloads)

## Product Matrix

&nbsp;                      |GPU acceleration      |Shell integration |Plugins           |Command line version|Windows XP
----------------------------|:--------------------:|:----------------:|:----------------:|:------------------:|:----------------:
**Installation package**    |:heavy_check_mark:    |:heavy_check_mark:|:heavy_check_mark:|:heavy_check_mark:  |:heavy_check_mark:
**32 bit portable**         |optional<sup>(1)</sup>|:x:               |:heavy_check_mark:|:heavy_check_mark:  |:heavy_check_mark:
**64 bit portable**         |optional<sup>(1)</sup>|:x:               |:heavy_check_mark:|:heavy_check_mark:  |:x:
**PortableApps.com edition**|optional<sup>(1)</sup>|:x:               |:heavy_check_mark:|:x:                 |:x:

<sup>(1)</sup> Optional GPU support: GPU support will be activated if the operating system is Windows Vista or higher and the latest [Microsoft Visual C++ 2015 Redistributable](https://www.microsoft.com/en-us/download/details.aspx?id=53840) is installed system-wide.

## iNFekt for Linux

The command line version is fully functional on Linux. There's also a basic GTK+ version in the source code repository. If you would like to work on improving it, please contact me via e-mail.