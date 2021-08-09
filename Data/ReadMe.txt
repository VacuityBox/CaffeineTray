################################################################################
##                                                                            ##
##                          CaffeineTray ReadMe.txt                           ##
##                                                                            ##
################################################################################

0. CONTENTS OF THIS FILE
========================

    1. INTRODUCTION
    2. REQUIREMENTS
    3. INSTALLATION
    4. CONFIGURATION
    5. USING CUSTOM ICONS
    6. HOW TO USE
    7. TROUBLESHOOTING

1. INTRODUCTION
===============

    CaffeineTray is a program to prevent your computer from going into sleep mode.
    Website: https://github.com/VacuityBox/CaffeineTray

2. REQUIREMENTS
===============

    Recommended
        * Windows 10 202H 32/64 bit

    Minimum
        * Windows 7 SP1 32/64 bit (not tested)

    * Program should work on all hardware that can run minimum required OS

3. INSTALLATION
===============

    Standard installation
    ---------------------

    Run CaffeineTray-{version}-Setup.exe and follow the instructions.
    Depending on what installation mode you choose program and settings are stored inside:

        All Users (Requires Administrator Privileges)
            Program is installed in C:/Program Files/ folder.
            Settings and logs are stored in %AppData%/CaffeineTray/ folder.

        Current User
            Program is installed in %AppData%/Local/CaffeineTray folder.
            Settings and logs are stored in %AppData%/CaffeineTray/ folder.

    Portable installation
    ---------------------
    
    Extract CaffeineTray-{version}-{bits}.zip to desired location.

4. CONFIGURATION
================

    Settings file are stored in json format.
    Depending on installation mode, settings are stored in:
        * %AppData%/CaffeineTray/CaffeineTray.json - All Users installation
        * %AppData%/CaffeineTray/CaffeineTray.json - Current User installation
        * ./CaffeineTray.Portable.json             - Portable installation

    Configuration options:
        Name                       Available Values                  Default Value           Description
        --------------------------------------------------------------------------------------------------
        Mode                       [0=Disabled, 1=Enabled, 2=Auto]   0                       Specifies run mode 
        UseNewIcons                [true/false]                      true                    Icon pack to use
        +Standard                                                                            Settings used when Mode=Standard
            KeepDisplayOn          [true/false]                      true                    Keep display be on when Caffeine is active
            DisableOnLockScreen    [true/false]                      true                    Don't keep display on when desktop is locked
        +Auto                                                                                Settings used when Mode=Auto
            ScanInterval           [1000-Int]                        2000                    Interval (in milliseconds) at which scanning for running processes is done.
            KeepDisplayOn          [true/false]                      true                    Keep display be on when Caffeine is active
            DisableOnLockScreen    [true/false]                      true                    Don't keep display on when desktop is locked
            ProcessNames           [Array(string)]                   []                      List of process names used by scanner
            ProcessPaths           [Array(string)]                   []                      List of process paths used by scanner
            WindowTitles           [Array(string)]                   []                      List of window titles used by scanner

    To modify program settings right click on CaffeineTray icon in Notification Area
    and select Settings. CaffeineSettings dialog should pop-up.
    
    You can also modify settings using text editor.
    But remember if you make a mistake default settings will be loaded,
    and your old settings file will be overwritten at program exit.

5. USING CUSTOM ICONS
=====================

    To use custom icons copy your icons into Icons/ directory:
        * %AppData%/CaffeineTray/Icons/ - All Users installation
        * %AppData%/CaffeineTray/Icons/ - Current User installation
        * ./Icons/                      - Portable installation

    There are 8 icons that you can overwrite:

        CaffeineDisabledDark.ico
        CaffeineDisabledLight.ico

        CaffeineEnabledDark.ico
        CaffeineEnabledLight.ico

        CaffeineAutoInactiveDark.ico
        CaffeineAutoInactiveLight.ico

        CaffeineAutoActiveDark.ico
        CaffeineAutoActiveLight.ico

    Each icon relate to one of the states that program is in and current system theme active.
    To load custom icon the name must be one of the above.

    Custom icons are loaded automatically whenever they exists.
    If one of icons is not found, default one would be used.

6. HOW TO USE
=============

    After the installation when you launch the program Caffeine is in Disabled mode.
    To understand how it works we need to know what modes program can be in.
    There are 3 modes:
        * Disabled:      | Computer can enter sleep mode                    | icon looks like empty cup
        * Enabled        | Computer is prevented from entering sleep mode   | icon looks like filled cup with steam above
        * Auto           | Auto has two possible states
            - Inactive   | Caffeine Inactive                                | icon looks like Disabled + little letter 'A' in top right corner
            - Active     | Caffeine Active                                  | icon looks like Enabled  + little letter 'A' in top right corner

    In order to change the Caffeine mode we can:
        * Left click on icon to cycle through modes (Disabled -> Enabled -> Auto -> Disabled)
        * Right click on icon and select appropriate mode

    Where Disabled and Enabled modes are straight forward, Auto mode will only
    activate Caffeine when specific process is running or specific window is found.

    To modify process/window list you can use settings dialog. To open settings, right click on
    notification icon and select Settings. A CaffeineSettings window should show.
    Now you can alter the list with following buttons:
        - Add:         manually add name/path/title
        - Add Wizard:  window with currently running processes
        - Edit:        edit selected item
        - Remove:      remove selected item
    Press OK to save.

7. TROUBLESHOOTING
==================

    Question: I add process path and Auto mode is not activating. What's wrong?
    Answer:   Due to system API that this program use, you need permission to be able to read process path that you not an owner.

    Q: How to add Caffeine to system startup?
    A: Create shortcut to CaffeineTray.exe and copy it to C:\Users\[Your username]\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup\.

    Q: There are already programs like this, why another one?
    A: This one have Auto mode :)