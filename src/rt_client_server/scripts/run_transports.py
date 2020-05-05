#!/usr/bin/python3.8 -u
"""
Run transport test for the give parameters and graph results.
"""

import numpy as np
import matplotlib.pyplot as plt
import argparse
import subprocess
import tempfile
import re

class Env:

    def __init__(self):
        self.args = self.create_parser().parse_args()

        if (len(self.args.block_count) != 1 and
            len(self.args.block_size) != 1):
            raise RuntimeError("block count or block size must be one")

    def server_prog(self, transport):
        prog = "rt_server"
        if transport == "flatbuffers":
            prog = "fb_rt_server"

        return prog

    def client_prog(self, transport):
        prog = "rt_client"
        if transport == "flatbuffers":
            prog = "fb_rt_client"

        return prog

    def start_server(self, transport):
        cmd = [self.server_prog(transport), "--transport", transport]
        if (self.args.do_debug):
            print(f'Running server command:\n  {" ".join(cmd)}')
        return subprocess.Popen(cmd)

    def run_echo_client(self, transport, out_dir):
        """Sanity test that echo works for transport"""

        cmd = [self.client_prog(transport), "--transport",
               transport, "--workload", "echo", "--block_count", "2",
               "--block_size", "4096", "--op_count", "1"]
        if (self.args.do_debug):
            print(f'Running client command:\n  {" ".join(cmd)}')

        test_passed = False
        fname = f"{out_dir}/echo_out.txt"
        with open(fname, 'w+') as f:
            run = subprocess.run(cmd, stdout=f, stderr=f)

        pat = re.compile(r'Test passed')
        with open(fname) as f:
            for line in f:
                if pat.search(line):
                    test_passed = True
                    break

        if not test_passed:
            raise RuntimeError("Echo test failed for {}".format(transport))

    def run_client(self, transport, out_dir):
        for bc in self.args.block_count:
            for bs in self.args.block_size:
                cmd = [self.client_prog(transport), "--transport",
                       transport, "--workload", self.args.workload,
                       "--block_count", str(bc),
                       "--block_size", str(bs), "--op_count",
                       str(self.args.op_count)]
                if (self.args.do_debug):
                    print(f'Running client command:\n  {" ".join(cmd)}')

                test_passed = False
                fname = f"{out_dir}/client_out_{transport}.txt"
                with open(fname, 'a+') as f:
                    run = subprocess.run(cmd, stdout=f, stderr=f)

    def stop_server(self, process):
        if (self.args.do_debug):
            print(f"Killing server")
        process.terminate()

    def run_echo_test(self, out_dir):
        for t in self.args.transports:
            if (self.args.do_debug):
                print(f"Testing echo for {t}")
            p = self.start_server(t)
            try:
                self.run_echo_client(t, out_dir)
            finally:
                self.stop_server(p)

    def run_tests(self, out_dir):
        for t in self.args.transports:
            p = self.start_server(t)
            self.run_client(t, out_dir)
            self.stop_server(p)

    def run_transports(self):
        dir = tempfile.mkdtemp(dir="/tmp")

        self.run_echo_test(dir)
        self.run_tests(dir)

        if (self.args.delete_temp):
            shutil.rmtree(dirpath)

    def create_parser(self):
        parser = argparse.ArgumentParser(description="""
                                         Run transport tests and graph results.
                                         """,
                                         formatter_class=
                                         argparse.ArgumentDefaultsHelpFormatter)

        parser.add_argument("--debug", dest="do_debug",
                            action="store_true", default=False,
                            help="""Show debugging info.""")
        parser.add_argument("--delete-temp", dest="delete_temp",
                            action="store_true", default=False,
                            help="""Delete temp files for run""")
        parser.add_argument("-o", "--op-count", dest="op_count",
                            default=35,
                            help="""Ops to run per value""")
        parser.add_argument("-c", "--block-count", dest="block_count",
                            default=[16], nargs="+",
                            help="""Block counts to run""")
        parser.add_argument("-s", "--block-size", dest="block_size",
                            default=[4096], nargs="+",
                            help="""Block sizes to run""")

        workloads = ["write", "read"]
        parser.add_argument("-w", "--workload", dest="workload",
                            metavar="workload", default=workloads[0],
                            choices=workloads,
                            help="""Workloads to run.
                            Allowed values are: {}.""". \
                            format(", ".join(workloads)))

        transports = ["grpc", "flatbuffers"]
        parser.add_argument("-t", "--transports", dest="transports",
                            metavar="transports", default=[transports[0]],
                            choices=transports, nargs="+",
                            help="""Transports to run.
                            Allowed values are: {}.""". \
                            format(", ".join(transports)))

        return parser

def main():
    env = Env()
    env.run_transports()

if __name__ == "__main__":
    main()
