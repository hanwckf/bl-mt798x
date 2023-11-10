# Extract and modify from tableknife project
# make it as a python libray that mangement GPT table
# Author: Sam Shih
# Email: sam.shih@mediatek.com
#
# Based on software by:
# https://github.com/dereulenspiegel/tableknife
#
# Original license:
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#
# Using structures defined in Wikipedia('http://en.wikipedia.org/wiki/GUID_Partition_Table')
#

import struct
import uuid
import zlib
import sys
import binascii
import uuid
import os
import stat
from fcntl import ioctl
import array
import platform

LBA_SIZE = int(512)

OFFSET_CRC32_OF_HEADER = 16
GPT_HEADER_FORMAT = "<8sIIIIQQQQ16sQIII420x"
GUID_PARTITION_ENTRY_FORMAT = "<16s16sQQQ72s"


class gpt_image_file:

    device = None
    device_path = None

    def __init__(self, path):
        self.device_path = path
        self.device = open(path, "r+b")

    def close(self):
        self.device.close()

    def read_sector(self, number, count):
        self.device.seek(number * LBA_SIZE)
        buf = self.device.read(count * LBA_SIZE)
        return buf

    def write_sector(self, offset, buf):
        self.device.seek(offset * LBA_SIZE)
        self.device.write(buf)


class GPT_Header:

    signatur = None
    revision = None
    header_size = None
    header_checksum = None
    own_offset = None
    other_offset = None
    first_usable_lba = None
    last_usable_lba = None
    disk_guid = None
    table_start_lba = None
    table_entry_count = None
    table_entry_size = None
    table_checksum = None

    def __init__(self, gpt_buf):
        gpt_header = struct.unpack(GPT_HEADER_FORMAT, gpt_buf)
        self.signatur = gpt_header[0]
        self.revision = gpt_header[1]
        self.header_size = gpt_header[2]
        self.header_checksum = gpt_header[3]
        self.own_offset = gpt_header[5]
        self.other_offset = gpt_header[6]
        self.first_usable_lba = gpt_header[7]
        self.last_usable_lba = gpt_header[8]
        self.disk_guid = gpt_header[9]
        self.table_start_lba = gpt_header[10]
        self.table_entry_count = gpt_header[11]
        self.table_entry_size = gpt_header[12]
        self.table_checksum = gpt_header[13]

    def serialize(self):
        gbuf = struct.pack(
            GPT_HEADER_FORMAT,
            self.signatur,
            self.revision,
            self.header_size,
            self.header_checksum,
            0x00,
            self.own_offset,
            self.other_offset,
            self.first_usable_lba,
            self.last_usable_lba,
            self.disk_guid,
            self.table_start_lba,
            self.table_entry_count,
            self.table_entry_size,
            self.table_checksum,
        )
        return gbuf


class GPT_Entry:

    partition_type_guid = None
    unique_guid = None
    first_lba = None
    last_lba = None
    attributes = None
    name = None

    def __init__(self, entry_buf):
        entry = struct.unpack(GUID_PARTITION_ENTRY_FORMAT, entry_buf)
        self.partition_type_guid = entry[0]
        self.unique_guid = entry[1]
        self.first_lba = entry[2]
        self.last_lba = entry[3]
        self.attributes = entry[4]
        # change partition name to unicode format
        self.name = entry[5].decode("utf-8")

    def serialize(self):
        ebuf = struct.pack(
            GUID_PARTITION_ENTRY_FORMAT,
            self.partition_type_guid,
            self.unique_guid,
            self.first_lba,
            self.last_lba,
            self.attributes,
            # change partition name to unicode format
            self.name.encode("utf-8"),
        )

        return ebuf

    def get_attribute(self):
        value = self.attributes
        if value == 0:
            return "System Partition"
        elif value == 2:
            return "Legacy BIOS Bootable"
        elif value == 60:
            return "Read-Only"
        elif value == 62:
            return "Hidden"
        elif value == 63:
            return "Do not automount"
        else:
            return "UNKNOWN"


class GPT:

    blockdev = None

    def __init__(self, gpt_image_file):
        self.blockdev = gpt_image_file

    def _make_nop(self, byte):
        nop_code = 0x00
        pk_nop_code = struct.pack("=B", nop_code)
        nop = pk_nop_code * byte
        return nop

    def _calc_header_crc32(self, fbuf, header_size=None):
        if not header_size:
            header_size = len(fbuf)
        nop = self._make_nop(4)
        clean_header = (
            fbuf[:OFFSET_CRC32_OF_HEADER]
            + nop
            + fbuf[OFFSET_CRC32_OF_HEADER + 4 : header_size]
        )
        crc32_header_value = self._unsigned32(zlib.crc32(clean_header))
        return crc32_header_value

    def _calc_table_crc32(
        self, table_area_buf=None, gpt_entries=None, gpt_header=None
    ):
        if table_area_buf:
            buf = table_area_buf
        elif gpt_entries:
            buf = self._serialize_gpt_table(gpt_entries, gpt_header)
        else:
            buf = self.get_part_table_area()
        crc = self._unsigned32(zlib.crc32(buf))
        return crc

    def _unsigned32(self, n):
        return n & 0xFFFFFFFFL

    def _serialize_gpt_table(self, gpt_entries, header=None):
        buf = ""
        if not header:
            header = self.get_gpt_header()
        total_table_size = header.table_entry_count * header.table_entry_size
        for entry in gpt_entries:
            buf += entry.serialize()
        if len(buf) < total_table_size:
            buf = buf.ljust((total_table_size), str(unichr(0x00)))
        return buf

    def _stringify_uuid(binary_uuid):
        uuid_str = str(uuid.UUID(bytes_le=binary_uuid))
        return uuid_str.upper()

    def write_gpt(self, gpt_header, gpt_entries=None):
        header_offset = gpt_header.own_offset
        if gpt_entries:
            gpt_header.table_checksum = self._calc_table_crc32(
                gpt_entries=gpt_entries, gpt_header=gpt_header
            )

        gpt_header.header_checksum = self._calc_header_crc32(
            gpt_header.serialize(), gpt_header.header_size
        )

        self.blockdev.write_sector(header_offset, gpt_header.serialize())
        if gpt_entries:
            table_offset = gpt_header.table_start_lba
            buf = self._serialize_gpt_table(gpt_entries)
            self.blockdev.write_sector(table_offset, buf)

    def get_gpt_header(self, secondary=False):
        lba = 1
        if secondary:
            lba = self.get_gpt_header().other_offset
        fbuf = self.blockdev.read_sector(lba, 1)
        gpt_header = GPT_Header(fbuf)
        return gpt_header

    def get_table(self, secondary=False):
        raw = self.get_part_table_area(secondary)
        result = list()
        for i in range(0, len(raw), 128):
            raw_entry = raw[i : (i + 128)]
            entry = GPT_Entry(raw_entry)
            # Both boot partition bl2 and data partition gpt start from block0,
            # so we should change "first_lba > 0" to "first_lba >= 0" in here
            if entry.first_lba >= 0 and (entry.last_lba - entry.first_lba) != 0:
                result.append(entry)
        return result

    def get_part_table_area(self, secondary=False):
        if secondary:
            gpt_header = self.get_gpt_header(True)
        else:
            gpt_header = self.get_gpt_header()

        part_entry_start_lba = gpt_header.table_start_lba
        part_entry_count = gpt_header.table_entry_count
        part_entry_size_in_bytes = gpt_header.table_entry_size
        part_table_lbas = (
            part_entry_count * part_entry_size_in_bytes
        ) / LBA_SIZE
        fbuf = self.blockdev.read_sector(part_entry_start_lba, part_table_lbas)
        return fbuf
