<img src="Gallery/CaffeineAppLight.svg" width="32" height="32"> Caffeine - Don't let your computer to sleep <img src="Gallery/CaffeineAppDark.svg" width="32" height="32">
===========================================

Installation
------------

Download latest release from [here](https://github.com/VacuityBox/Caffeine/releases)<br />
Extract and run CaffeineTray.exe

How to use
----------

More on wiki [here](https://github.com/VacuityBox/Caffeine/wiki)

Building CaffeineTray
---------------------

Before build you need to meet these requirements:
1. Installed Visual Studio 2019 (with MSVC)

To build the project:
1. Open Caffeine.sln
2. Build CaffeineTray

Building CaffeineSettings
-------------------------

Before build you need to meet these requirements:
1. Installed Visual Studio 2019 (with .NET Core SDK)

To build the project:
1. Open Caffeine.sln
2. Build CaffeineSettings

Building Icons
--------------

Before build you need to meet these requirements:
1. Installed Python 3
2. Installed Inkscape (inkscape visible in PATH)
3. Installed ImageMagick (convert visible in PATH)

To build the icons:
1. Open command line in project root directory
2. Run `python3 ./Scripts/build_icons.py Resources/Caffeine.svg Resources/icon_description.json`
