Building CaffeineTray
---------------------

Before build you need to meet these requirements:
1. Installed Visual Studio 2019 (with MSVC)

To build the project:
1. Open CaffeineTray.sln
2. Build CaffeineTray

Building Icons
--------------

Before build you need to meet these requirements:
1. Installed Python 3
2. Installed Inkscape (inkscape visible in PATH)
3. Installed ImageMagick (convert visible in PATH)

To build the icons:
1. Open command line in project root directory
2. Run `python3 ./Scripts/build_icons.py Resources/Caffeine.svg Resources/icon_description.json`
