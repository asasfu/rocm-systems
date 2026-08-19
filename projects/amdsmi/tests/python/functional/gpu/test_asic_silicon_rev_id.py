#!/usr/bin/env python3
#
# Copyright (C) Advanced Micro Devices. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
"""Cross-checks asic_info["silicon_rev_id"] against the amdgpu DRM ioctl.

The library reads external_rev out of AMDGPU_INFO_DEV_INFO. This test issues the
same ioctl independently so a plumbing regression (wrong struct offset, stale
cache, truncation) cannot pass unnoticed.
"""

import ctypes
import fcntl
import glob
import os
import struct
import unittest

import common.common as common
from common.common import amdsmi

_IOC_NRBITS = 8
_IOC_TYPEBITS = 8
_IOC_SIZESHIFT = 16
_IOC_DIRSHIFT = 30
_IOC_WRITE = 1

_DRM_IOCTL_BASE = ord("d")
_DRM_COMMAND_BASE = 0x40
_DRM_AMDGPU_INFO = 0x05
_AMDGPU_INFO_DEV_INFO = 0x16


class _DrmAmdgpuInfo(ctypes.Structure):
    """struct drm_amdgpu_info from amdgpu_drm.h."""

    _fields_ = [
        ("return_pointer", ctypes.c_uint64),
        ("return_size", ctypes.c_uint32),
        ("query", ctypes.c_uint32),
        ("_union", ctypes.c_uint8 * 16),
    ]


_DRM_IOCTL_AMDGPU_INFO = (
    (_IOC_WRITE << _IOC_DIRSHIFT)
    | (_DRM_IOCTL_BASE << _IOC_NRBITS)
    | (_DRM_COMMAND_BASE + _DRM_AMDGPU_INFO)
    | (ctypes.sizeof(_DrmAmdgpuInfo) << _IOC_SIZESHIFT)
)


def _render_node_for_bdf(bdf):
    """Return the /dev/dri/renderD* node backing a PCI BDF, or None."""
    for link in glob.glob("/sys/class/drm/renderD*/device"):
        if os.path.basename(os.path.realpath(link)) == bdf:
            return os.path.join("/dev/dri", os.path.basename(os.path.dirname(link)))
    return None


def _query_external_rev(render_node):
    """Read drm_amdgpu_info_device.external_rev via AMDGPU_INFO_DEV_INFO."""
    fd = os.open(render_node, os.O_RDWR | os.O_CLOEXEC)
    try:
        buf = ctypes.create_string_buffer(1024)
        request = _DrmAmdgpuInfo()
        request.return_pointer = ctypes.addressof(buf)
        request.return_size = ctypes.sizeof(buf)
        request.query = _AMDGPU_INFO_DEV_INFO
        fcntl.ioctl(fd, _DRM_IOCTL_AMDGPU_INFO, request)
    finally:
        os.close(fd)
    # struct drm_amdgpu_info_device: __u32 device_id, chip_rev, external_rev, ...
    return struct.unpack_from("<III", buf.raw, 0)[2]


class TestGpuAsicSiliconRevId(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.common = common.Common(common.verbose)

    def setUp(self):
        self.common.amdsmi_smart_init()
        self.common.processors = amdsmi.amdsmi_get_processor_handles()

    def tearDown(self):
        amdsmi.amdsmi_shut_down()

    def test_silicon_rev_id_matches_drm_external_rev(self):
        self.common.print_func_name("")
        checked = 0
        for processor in self.common.processors:
            asic_info = amdsmi.amdsmi_get_gpu_asic_info(processor)
            self.assertIn("silicon_rev_id", asic_info)

            bdf = amdsmi.amdsmi_get_gpu_device_bdf(processor)
            render_node = _render_node_for_bdf(bdf)
            if render_node is None:
                continue

            expected = _query_external_rev(render_node)
            self.assertEqual(
                int(asic_info["silicon_rev_id"], 16),
                expected,
                f"{bdf}: silicon_rev_id {asic_info['silicon_rev_id']} != "
                f"DRM external_rev {hex(expected)}",
            )
            checked += 1

        if checked == 0:
            self.skipTest("no GPU exposed a matching /dev/dri/renderD* node")


if __name__ == "__main__":
    unittest.main()
