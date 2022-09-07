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

import json
import re
import traceback
import os
import uuid
import argparse
from libgpt import GPT_Entry
from libgpt import GPT_Header
from libgpt import gpt_image_file
from libgpt import GPT
from json import JSONDecoder
from collections import OrderedDict


def get_template_GPT(template="MTK_MBR"):
    template_file = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "template", template
    )
    try:
        fp = gpt_image_file(template_file)
        gpt = GPT(fp)
        return gpt

    except:
        traceback.print_exc()
        return None


def get_template_GPT_header(template="MTK_MBR"):
    gpt = get_template_GPT(template)
    if not gpt:
        return None
    else:
        return gpt.get_gpt_header()


def get_template_GPT_entry(template="MTK_MBR"):
    gpt = get_template_GPT(template)
    if not gpt:
        return None
    else:
        return gpt.get_table()[0]


def read_gpt(path):
    fp = gpt_image_file(path)
    gpt = GPT(fp)
    primary_header = gpt.get_gpt_header()
    primary_table_entries = gpt.get_table()
    primary_header_checksum = gpt._calc_header_crc32(
        primary_header.serialize(), primary_header.header_size
    )
    primary_table_checksum = gpt._calc_table_crc32(
        gpt_entries=primary_table_entries
    )
    if primary_header.header_checksum != primary_header_checksum:
        print("[WARNING] Checksum mismatch for primary header:")
        print("\t Expected: %.8x" % primary_header.header_checksum)
        print("\t Calculated: %.8x" % primary_header_checksum)
    if primary_header.table_checksum != primary_table_checksum:
        print("[WARNING] Checksum mismatch for primary table")
        print("\t Expected: %.8x" % primary_header.table_checksum)
        print("\t Calculated: %.8x" % primary_table_checksum)
    print("Start of primary table: %d" % primary_header.table_start_lba)
    for entry in primary_table_entries:
        print("[PARTITION] Name: %s" % entry.name)
        print(
            "\t type guid:\t{%s}"
            % str(uuid.UUID(bytes_le=entry.partition_type_guid))
        )
        print(
            "\t unique guid:\t{%s}" % str(uuid.UUID(bytes_le=entry.unique_guid))
        )
        print("\t start blk:\t%d (0x%x)" % (entry.first_lba, entry.first_lba))
        print("\t last blk:\t%d (0x%x)" % (entry.last_lba, entry.last_lba))
        flash_cmd_size = entry.last_lba - entry.first_lba + 1
        blksize = 512
        size = flash_cmd_size * blksize
        unit = ""
        if size >= 1024:
            unit = "K"
            size = int(size / 1024)
        if size >= 1024:
            unit = "M"
            size = int(size / 1024)

        print(
            "\t blk count:\t%d (0x%x) (%d%s)"
            % (flash_cmd_size, flash_cmd_size, size, unit)
        )
        print(
            "\t byte range:\t%x-%x"
            % (entry.first_lba * blksize, entry.last_lba * blksize)
        )
        print("\t attrbitues:\t%s" % str(entry.attributes))


def to_mtk_partition_name(name):
    new_name = ""
    for c in name:
        new_name = new_name + c + str(unichr(0x00))
    return new_name


# MTK partition table contain MBR in offset 0
# We need to write MBR for GPT table image
def write_mbr(output_gpt, sdmmc):
    mtk_template_gpt = None
    if sdmmc is True:
        mtk_template_gpt = get_template_GPT("MTK_SD_HDR")
    else:
        mtk_template_gpt = get_template_GPT()
    data = mtk_template_gpt.blockdev.read_sector(0x0, 1)
    output_gpt.blockdev.write_sector(0x0, data)


def create_gpt(in_path, out_path, sdmmc=False):
    jobj = None
    try:
        f_in = open(in_path, "r")
        f_new = open(out_path, "w")
        f_new.close()
        f_out = gpt_image_file(out_path)
    except:
        traceback.print_exc()
        print(
            "[ERROR] input file '%s' or output file '%s' not found"
            % (in_path, out_path)
        )
        return
    try:
        comment_regex = "[\ \t]*#.+"
        data = f_in.read()
        data = re.sub(comment_regex, "", data)
        customdecoder = JSONDecoder(object_pairs_hook=OrderedDict)
        jobj = customdecoder.decode(data)
    except:
        print("[ERROR] unable to read json input")
        traceback.print_exc()
        return
    f_in.close()
    write_partitions = []
    for name in jobj:
        partition_data = jobj[name]
        assert "start" in partition_data
        assert "end" in partition_data
        new_uuid = None
        if "uuid" in partition_data:
            try:
                new_uuid = uuid.UUID(partition_data["uuid"]).bytes_le
            except:
                print(
                    "[ERROR] unable to parse uuid '%s'"
                    % partition__data["uuid"]
                )
        else:
            new_uuid = uuid.uuid1().bytes_le
        entry = get_template_GPT_entry()
        assert entry != None
        entry.unique_guid = new_uuid
        # convert ascii json partition name like "fip" into unicode version bytes
        # "f.i.p." ['f', 0x00, 'i', 0x00, 'p', 0x00]
        entry.name = to_mtk_partition_name(name)
        entry.first_lba = partition_data["start"]
        entry.last_lba = partition_data["end"]
        if "attributes" in partition_data:
            entry.attributes = partition_data["attributes"]
        write_partitions.append(entry)

    header = get_template_GPT_header()
    header.last_usable_lba = write_partitions[-1].last_lba
    assert header != None
    gpt = GPT(f_out)
    gpt.write_gpt(header, write_partitions)
    write_mbr(gpt, sdmmc)
    read_gpt(out_path)


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
        help="specify the output GPT image path",
        dest="output",
    )
    parser.add_argument(
        "--sdmmc",
        action="store_true",
        required=False,
        help="set this flag if output gpt is for mtk sdmmc",
        dest="sdmmc",
    )
    parser.add_argument(
        "--show",
        nargs=1,
        default=None,
        required=False,
        help="do not convert, just show the GPT image information",
        dest="show",
    )
    args = parser.parse_args()

    if args.show:
        read_gpt(args.show[0])
    elif args.output and args.input:
        in_path = args.input[0]
        out_path = args.output[0]
        create_gpt(in_path, out_path, args.sdmmc)
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
