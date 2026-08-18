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
"""CLI leaf test: fabric command."""

from cli.base import TestCliBase


class TestFabric(TestCliBase):
    def test_command(self):
        self.common.print_func_name("")
        msg = f"{self.tab}### amd-smi fabric"
        self.common.print(msg)

        cmds = self.CreateCmds(
            "fabric", "Fabric arguments:", "Device Arguments:", "Command Modifiers:", ""
        )
        self.RunCmds(cmds)
        return

    def test_flag_contract(self):
        self.common.print_func_name("")

        # Parser contract: new flags present, deprecated --info hidden.
        (_, help_out, _) = self.util.RunCmdSync("amd-smi fabric --help")
        help_out = help_out or ""
        self.assertIn("--topology", help_out)
        self.assertIn("--telemetry", help_out)
        self.assertNotIn("--info", help_out)  # deprecated: hidden from --help

        # Routing:
        #   --topology emits the config key
        #   --telemetry the counters key
        # Guards the flag swap, not just that the flags parse.
        # Key presence holds without fabric hardware: the API-error path still stores
        # an "N/A" value under the key.
        # Validating the actual config/counter values needs IFoE/UALink hardware
        # and is deferred.
        (_, topo_out, _) = self.util.RunCmdSync("amd-smi fabric --topology --json")
        topo_out = topo_out or ""
        self.assertIn("fabric_info", topo_out)
        self.assertNotIn("fabric_telemetry", topo_out)

        (_, telem_out, _) = self.util.RunCmdSync("amd-smi fabric --telemetry --json")
        telem_out = telem_out or ""
        self.assertIn("fabric_telemetry", telem_out)
        self.assertNotIn("fabric_info", telem_out)

        # --info still parses and aliases --topology (config output).
        (_, info_out, err) = self.util.RunCmdSync("amd-smi fabric --info --json")
        self.assertNotIn("unrecognized arguments", err or "")
        self.assertIn("fabric_info", info_out or "")
        return
