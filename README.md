# Qemu-DPT

Qemu-DPT (Dirty Page Tracking) is a tool to measure the memory update speed of your VM.
It is a small extension to Qemu/KVM. This tool itself was developed in [the Yabusame project of AIST](http://grivon.apgrid.org/quick-kvm-migration) around 2012.


## Background

To accurately simulate the behavior of an IaaS cloud systems, you may want to
obtain the memory update intensity of VMs through this extension. This
information will be one of the input parameters to your simulation program.

At the time of writing, [SimGrid](http://http://simgrid.gforge.inria.fr/) is
the only simulation framework with an accurate precopy-based live migration
model. It implements the precopy algorithm used in hypervisors. It can
correctly simulates the duration of live migrations and the amount of
transferred data, considering memory update speeds of VMs and resource
contention of CPU and network.

If you use Qemu-DPT for your research, please cite the below paper in your publication.

Adding a Live Migration Model into SimGrid,  
Takahiro Hirofuchi, Adrien Lèbre, Laurent Pouilloux,  
Proceedings of the 5th IEEE International Conference on Cloud Computing Technology and Services 2013 (CloudCom2013), pp.96-103, Dec 2013  
DOI: [10.1109/CloudCom.2013.20](http://dx.doi.org/10.1109/CloudCom.2013.20)

```
@inproceedings{SimGridVM:CloudCom2013,
	author = {Takahiro Hirofuchi and Adrien Lèbre and Laurent Pouilloux},
	title = {Adding a Live Migration Model into SimGrid: One More Step Toward the Simulation of Infrastructure-as-a-Service Concerns},
	booktitle = {Proceedings of the 2013 IEEE International Conference on Cloud Computing Technology and Science (CloudCom2013)},
	year = {2013},
	pages = {96--103},
	publisher = {IEEE Computer Society},
}
```

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

Takahiro Hirofuchi, Senior researcher of AIST

Copyright (c) 2011-2015 National Institute of Advanced Industrial Science and Technology
