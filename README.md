# ATF and u-boot for mt798x

![](/u-boot.gif)

## Prepare

```
sudo apt install gcc-aarch64-linux-gnu
```

## Build
```
Usage: SOC=[mt7981|mt7986] BOARD=<board name> FIXED_MTDPARTS=[1|0] MULTI_LAYOUT=[0|1] ./build.sh
eg: SOC=mt7981 BOARD=360t7 ./build.sh
eg: SOC=mt7981 BOARD=wr30u MULTI_LAYOUT=1 ./build.sh
eg: SOC=mt7986 BOARD=redmi_ax6000 ./build.sh
```

---

### xiaomi-wr30u multi-layout uboot firmware compatibility
|Firmware type|uboot (default)|uboot (immortalwrt-112m)|uboot (qwrt)|
|:----:|:----:|:----:|:----:|
|xiaomi stock mtd8 (ubi)|√|×|×|
|immortalwrt stock|√|×|×|
|X-Wrt stock|√|×|×|
|immortalwrt 112m|×|√|×|
|GL.iNet by 237176253|×|√|×|
|QWRT|×|×|√|
|**X-Wrt ubootmod**|×|×|×|
