import sys
import os
import subprocess
import time
import shutil
import xml.etree.ElementTree as ET
import json

TEMP_DIR = "Obj/icon_build_temp/"
OUTPUT_DIR = "Resources/Icons/"
ICON_WIDTH = 256
ICON_HEIGHT = 256

ICON_SIZE = [
    16, 32, 48, 64, 128, 256
]

PATH_TAG = "{http://www.w3.org/2000/svg}path"
LABEL_ATTRIB = "{http://www.inkscape.org/namespaces/inkscape}label"
STYLE_ATTRIB = "style"

# Export .png from .svg using inkscape.
def export_png(svg, filename, x, y, w, h, size):
    cmd =  [
        "inkscape",
        "--export-filename={}".format(filename),
        "--export-area={}:{}:{}:{}".format(x, y, x + w, y + h),
        "--export-width={}".format(size),
        "--export-height={}".format(size),
        svg
    ]

    print("Exporting image '{}'.".format(filename))
    ret = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    #time.sleep(0.2)
    
    return ret.returncode == 0

# Build .ico file.
def build_icon(svg, name, x, y):
    names = []
    # Create png for every size.
    for size in ICON_SIZE:
        filename = TEMP_DIR + name + "_{0}x{0}.png".format(size)
        if not export_png(svg, filename, x, y, ICON_WIDTH, ICON_HEIGHT, size):
            print("export_png() failed.")
            return False

        names.append(filename)

    # Build icon.
    ico = "{}{}.ico".format(OUTPUT_DIR, name)
    cmd = [ "convert" ]
    cmd.extend(names)
    cmd.append(ico)

    print("Building icon '{}'...".format(ico))
    ret = subprocess.run(cmd)

    return ret.returncode == 0

# Update style of svg path.
def update_style(path, style_desc):
    def parse_style(style_str):
        return dict(x.split(":") for x in style_str.split(";"))

    def style_to_str(style_dict):
        style_str = ""
        for key, value in style_dict.items():
            style_str = style_str + "{}:{};".format(key, value)
        return style_str[:-1]

    style = parse_style(path.attrib[STYLE_ATTRIB])
    style["fill"] = style_desc["fill"]
    style["fill-opacity"] = style_desc["fill-opacity"]
    style["stroke"] = style_desc["stroke"]
    style["stroke-opacity"] = style_desc["stroke-opacity"]

    return style_to_str(style)

# Build icon svg.
def build_svg(xml, filename, cup_desc):    
    root = xml.getroot()
    for child in root.iter(PATH_TAG):
        label = child.attrib[LABEL_ATTRIB]
        new_style = update_style(child, cup_desc[label])
        child.set(STYLE_ATTRIB, new_style)

    print("Building svg '{}'...".format(filename))
    xml.write(filename)

    return True

# Create cup description based on 4 colors proporties.
def create_cup_desc(frame, fill, coffee, steam):
    def create_style_desc(fc, fo, sc, so):
        return {
            "fill": fc,
            "fill-opacity": fo,
            "stroke": sc,
            "stroke-opacity": so,
        }

    return {
        "Coffee": create_style_desc(coffee, 1.0, coffee, 1.0),
        "Handle": create_style_desc("none", 1.0, frame, 1.0),
        "FrameFill": create_style_desc(fill, 1.0, "none", 1.0),
        "FrameBottom": create_style_desc("none", 1.0, frame, 1.0),
        "FrameTop": create_style_desc(frame, 1.0, frame, 1.0),
        "Steam": create_style_desc(steam, 1.0, steam, 1.0),
    }

def build(xml, desc):
    def get_prop(d, key):
        try:
            return d[key]
        except KeyError:
            raise KeyError("{} not found in icon description file".format(key))

    for k, v in desc.items():
        cup = create_cup_desc(
            get_prop(v, "FrameColor"),
            get_prop(v, "FillColor"),
            get_prop(v, "CoffeeColor"),
            get_prop(v, "SteamColor")
        )

        svg = TEMP_DIR + k + ".svg"
        if not build_svg(xml, svg, cup):
            print("build_svg() failed.")
            return False

        if not build_icon(svg, k, 0, 0):
            print("build_icon() failed.")
            return False
        
        print("OK.")

    return True

def read_desc(filename):
    with open(filename) as f:
        try:
            return json.loads(f.read())
        except ValueError as e:
            raise ValueError("'{}' is not a valid .json file.\n{}".format(filename, e))

def check_executables():
    if not shutil.which("inkscape"):
        print("Can't find executable 'inkscape'.")
        print("Install Inkscape and make sure it's in PATH.")
        return False

    if not shutil.which("convert"):
        print("Can't find executable 'convert'.")
        print("Install ImageMagick and make sure it's in PATH.")
        return False

    return True

def main():
    if len(sys.argv) < 3:
        print("Usage: {} svg desc [keepImages]\n".format(sys.argv[0]))
        sys.exit(1)

    keepImages = False
    if len(sys.argv) == 4:
        keepImages = True

    svg = sys.argv[1]
    if not os.path.isfile(svg):
        sys.exit("File '{}' not found".format(svg))

    desc = sys.argv[2]
    if not os.path.isfile(desc):
        sys.exit("File '{}' not found".format(desc))

    # Check for executables.
    if not check_executables():
        sys.exit(1)

    # Create temp directory.
    if not os.path.isdir(TEMP_DIR):
        os.makedirs(TEMP_DIR)

    # Create output directory
    if not os.path.isdir(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR)

    # Start build process.
    desc = read_desc(desc)
    tree = ET.parse(svg)
    if not build(tree, desc):
        sys.exit(1)

    # Cleanup
    if not keepImages:
        shutil.rmtree(TEMP_DIR)


if __name__ == "__main__":
    main()
