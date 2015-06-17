# Qemu-DPT

This extention to Qemu/KVM allows you to measure the memory update speed of your VM.
This is developed in the Yabusame project of AIST.



## Compile

```
$ git clone https://github.com/grivon/yabusame-qemu-dpt.git
```

```
$ CC=gcc CFLAGS=-lrt ./configure --target-list=x86_64-softmmu --disable-docs
$ make -j 8
```

## Usage

Through the Qemu monitor interface, you can start and stop dirty page tracking.
While dirty page tracking is active, Qemu periodically outputs memory update speed to stderr.


```
$ ./x86_64-softmmu/qemu-system-x86_64 -m 4g -machine accel=kvm -hda your-vm-disk.img -monitor stdio
phys_dirty is reallocated. old size 0, new size 1048576
phys_dirty is reallocated. old size 1048576, new size 1048608
phys_dirty is reallocated. old size 1048608, new size 1048640
phys_dirty is reallocated. old size 1048640, new size 1050688
phys_dirty is reallocated. old size 1050688, new size 1050704
phys_dirty is reallocated. old size 1050704, new size 1050736
QEMU 1.0.91 monitor - type 'help' for more information
(qemu)


(qemu) dpt_start
create bitmap /tmp/qemu.bm-14528.1434530585 (131344 bytes), dirty 54 / 1050736 (0.211 MB/s)
create bitmap /tmp/qemu.bm-14528.1434530586 (131344 bytes), dirty 45 / 1050736 (0.176 MB/s)
create bitmap /tmp/qemu.bm-14528.1434530587 (131344 bytes), dirty 74 / 1050736 (0.289 MB/s)
create bitmap /tmp/qemu.bm-14528.1434530588 (131344 bytes), dirty 37 / 1050736 (0.145 MB/s)
create bitmap /tmp/qemu.bm-14528.1434530589 (131344 bytes), dirty 71 / 1050736 (0.277 MB/s)
create bitmap /tmp/qemu.bm-14528.1434530590 (131344 bytes), dirty 64 / 1050736 (0.250 MB/s)
create bitmap /tmp/qemu.bm-14528.1434530591 (131344 bytes), dirty 58 / 1050736 (0.227 MB/s)

(qemu) dpt_stop
```

## Author and Contact

* Takahiro Hirofuchi, AIST

Copyright (c) 2011-2015 National Institute of Advanced Industrial Science and Technology
