# Copyright (c) 2020 MediaTek Inc
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

from PIL import Image, ImageDraw, ImageFont
import json
import os
import hashlib
import traceback
import re
from json import JSONDecoder
from collections import OrderedDict
import argparse


def get_color_table():
    template_file = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "template", "color.conf"
    )
    colors = []
    try:
        fp = open(template_file, "r")
        data = fp.read()
        fp.close()
        for desc in data.split("\n"):
            color = desc.split(":")[0].strip().replace("\n", "")
            if color.strip() != "" and "#" not in color:
                colors.append(color)
    except:
        traceback.print_exc()
        return None
    return colors


def get_default_font():
    template_file = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "template", "Arial.ttf"
    )
    return template_file


def parse(in_path):
    f_in = None
    jobj = None
    try:
        f_in = open(in_path, "r")
    except:
        traceback.print_exc()
        print("[ERROR] input file '%s' not found" % (in_path))
        return None
    try:
        comment_regex = "[\ \t]*#.+"
        data = f_in.read()
        data = re.sub(comment_regex, "", data)
        customdecoder = JSONDecoder(object_pairs_hook=OrderedDict)
        jobj = customdecoder.decode(data)
    except:
        print("[ERROR] unable to read json input")
        traceback.print_exc()
        return None
    f_in.close()
    return jobj


def draw(json_file, png_file):
    jobj = parse(json_file)
    assert jobj != None
    colors = get_color_table()
    partition_num = 0
    partition_name = []
    partition_start = []
    partition_size = []
    for key, value in jobj.iteritems():
        partition_name.append(key)
        blksize = 512
        start = int(value["start"]) * blksize
        end = int(value["end"]) * blksize
        size = (end - start) + blksize
        unit = ""
        if size >= 1024:
            unit = "K"
            size = int(size / 1024)
        if size >= 1024:
            unit = "M"
            size = int(size / 1024)
        partition_start.append(start)
        partition_size.append("%d%s" % (size, unit))
        partition_num += 1
    img = Image.new("RGB", (400, 80 * partition_num), color="white")
    for num, part in enumerate(partition_name):
        color_id = int(hashlib.sha1(part).hexdigest(), 16) % len(colors)
        d = ImageDraw.Draw(img)
        font = ImageFont.truetype(get_default_font(), 24)
        d.rectangle([0, 80 * num, 200, 80 * (num + 1)], fill="black")
        d.rectangle(
            [0 + 1, 80 * num + 1, 200 - 1, 80 * (num + 1) - 1], fill=colors[color_id]
        )
        size = partition_size[num]
        d.text((10, 80 * num), "%s (%s)" % (part, size) , fill="black", font=font)
        font = ImageFont.truetype(get_default_font(), 16)
        offset = "0x" + ("%x" % partition_start[num]).upper().zfill(8)
        d.text((200 + 2, 80 * num), offset, fill="black", font=font)
    img.save(png_file)

def main():
    parser = argparse.ArgumentParser(
        description="convert json description to GPT image"
    )
    parser.add_argument(
        "--i",
        nargs=1,
        default=None,
        required=False,
        help="specify the input json file path",
        dest="input",
    )
    parser.add_argument(
        "--o",
        nargs=1,
        default=None,
        required=False,
        help="specify the output layout png path",
        dest="output",
    )
    args = parser.parse_args()

    if args.output and args.input:
        in_path = args.input[0]
        out_path = args.output[0]
        draw(in_path, out_path)
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
