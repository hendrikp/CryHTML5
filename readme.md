![CryHTML5 - chromium for CryEngine 3](http://i.imgur.com/9Ird5pb.png)
=====================================
**Chromium** is an open-source browser project that can be used to render web pages and dynamic graphics at lightning speed. ([see](http://www.chromium.org/developers/design-documents/displaying-a-web-page-in-chrome)).

**CryHTML5** is a bridge between the [Chromium Embedded Framework](https://code.google.com/p/chromiumembedded/) and the CryEngine renderer, which allows developers to rapidly build UI for their games using the highly prevalent HTML5 format. Furthermore, developers can use JavaScript and the numerous open source JavaScript libraries available to browser based apps in the game engine.

CryHTML5 is licensed under the terms of GNU LGPL 2.1 license.

Installation / Integration
==========================
Clone (or add as a submodule) this repository to the `Code` directory. This plugin depends on `Plugin_D3D` and `Plugin_SDK` which should be checked out and compiled in the same way.

The plugin manager will automatically load up the plugin (from Bin32/Plugins) when the game/editor is restarted or if you directly load it.

CVars / Commands
================
* ```cm5_active``` Activate 1 Deactivate 0
* ```cm5_devtools``` Opens the Developer tools
* ```cm5_url``` Open URL (Syntax cry://... will open files in game directories and pak files)
* ```cm5_js``` Execute Javascript
* ```cm5_input``` Input Mode 1 Keys only, 2 Mouse + Emulation (requires virtual cursor), 3 Hardware Mouse

Flownodes
=========
FlowNode control interface is planned.
