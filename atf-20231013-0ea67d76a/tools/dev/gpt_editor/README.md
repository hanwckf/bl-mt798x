MTK GPT tool
===

This tool generates GPT table for MTK BL2 to recognize on SD/MMC boot.

After generating GPT table by this tool, flash it to mmc data partition0 (blk #0 ~ blk #255) through u-boot/kernel commands(e.g, fdisk). BL2(preloader)/BL33(u-boot)/kernel will parse GPT table to load partitions.

## Description file
First, you should prepare a partition desciption file in json format. Each partition has following fields:
* start: start address (must specified)
* end: end address (must specified)
* uuid: unique ID(optional) -> If not specified, this program will generate random value.

There are presets in example/

Note: You don't need to add 'gpt' and 'bl2' partitions in description file because BROM will load them automatically.

## Usage
When description file is ready, you can run mtk_gpt.py to convert description file into GPT table.

Quickstart:
`python mtk_gpt.py --i example/bpir64.json --o GPT_EMMC`

If you need more information, just execute program with help:
`python mtk_gpt.py -h`
