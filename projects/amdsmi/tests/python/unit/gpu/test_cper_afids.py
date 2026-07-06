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
"""amdsmi_get_afids_from_cper input-normalization unit tests (hardware-free)."""

from __future__ import annotations

import unittest

from common.common import amdsmi


class TestAmdSmiGetAfidsFromCper(unittest.TestCase):
    """Hardware-free tests for amdsmi_get_afids_from_cper argument normalization.

    The list branch was previously unreachable (guarded by an invalid
    ``isinstance(data, List[Dict[str, Any]])``). These cases exercise the now-reachable
    normalization paths without a GPU.
    """

    def test_empty_list_short_circuits_without_library_call(self):
        # The list branch is now reachable; an empty list yields no records and
        # returns before touching the C library.
        afids, count = amdsmi.amdsmi_interface.amdsmi_get_afids_from_cper([])
        self.assertEqual(afids, [])
        self.assertEqual(count, 0)

    def test_invalid_type_raises_parameter_exception(self):
        with self.assertRaises(amdsmi.AmdSmiParameterException):
            amdsmi.amdsmi_interface.amdsmi_get_afids_from_cper(42)

    def test_malformed_record_raises_parameter_exception(self):
        # A record lacking the "bytes" and "size" keys is rejected before any
        # library call.
        with self.assertRaises(amdsmi.AmdSmiParameterException):
            amdsmi.amdsmi_interface.amdsmi_get_afids_from_cper([{"missing": "keys"}])


if __name__ == "__main__":
    unittest.main()
