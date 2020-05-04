#!/usr/bin/python3 -u
"""
Run transport test for the give parameters and graph results.
"""

import numpy as np
import matplotlib.pyplot as plt
import argparse
import subprocess

class Env:

    def __init__(self):
        self.args = self.create_parser().parse_args()

        if (len(self.args.block_count) != 1 and
            len(self.args.block_size) != 1):
            raise RuntimeError("block count or block size must be one")

    def start_server(self, transport):
        pass

    def run_client(self, transport, workload):
        pass

    def stop_server(self, process):
        pass

    def test_echo(self):
        for t in self.args.transports:
            p = self.start_server(t)
            self.run_client(t, "echo")
            self.stop_server(p)

    def run_transports(self):
        self.test_echo()

    def create_parser(self):
        parser = argparse.ArgumentParser(description="""
                                         Run transport tests and graph results.
                                         """,
                                         formatter_class=
                                         argparse.ArgumentDefaultsHelpFormatter)

        parser.add_argument("-o", "--op_count", dest="op_count",
                            default=35,
                            help="""Ops to run per value""")
        parser.add_argument("-c", "--block_count", dest="block_count",
                            default=[16], nargs="+",
                            help="""Block counts to run""")
        parser.add_argument("-s", "--block_size", dest="block_size",
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
