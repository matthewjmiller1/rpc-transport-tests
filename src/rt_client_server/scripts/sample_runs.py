#!/usr/bin/python3.8 -u

import subprocess
import argparse

class Env:

    def __init__(self):
        self.args = self.create_parser().parse_args()
        self.workloads = ["write", "read"]
        self.transports = ["grpc", "flatbuffers", "capnproto"]

        self.block_sizes = []
        self.block_sizes.append([2**v for v in range(6, 12)])
        self.block_sizes.append([2**v for v in range(12, 15)])

        self.block_counts = []
        self.block_counts.append([2**v for v in range(0, 7)])
        self.block_counts.append([2**v for v in range(7, 11)])

    def do_sample_runs(self):
        for w in self.workloads:
            for ranges in self.block_sizes:
                cmd = ["run_transports.py", "--out-directory",
                       self.args.out_dir, "-w", w, "-o", "50", "-t"]
                cmd.extend(self.transports)
                cmd.append("-s")
                cmd.extend([str(v) for v in ranges])
                if (self.args.do_debug):
                    chd.append("-d")
                print(f'Running command:\n  {" ".join(cmd)}')
                subprocess.run(cmd)

        for w in self.workloads:
            for ranges in self.block_counts:
                cmd = ["run_transports.py", "--out-directory",
                       self.args.out_dir, "-w", w, "-o", "50", "-t"]
                cmd.extend(self.transports)
                cmd.append("-c")
                cmd.extend([str(v) for v in ranges])
                if (self.args.do_debug):
                    chd.append("-d")
                print(f'Running command:\n  {" ".join(cmd)}')
                subprocess.run(cmd)

    def create_parser(self):
        parser = argparse.ArgumentParser(description="""
                                         Do sample runs.
                                         """,
                                         formatter_class=
                                         argparse.ArgumentDefaultsHelpFormatter)

        parser.add_argument("-d", "--debug", dest="do_debug",
                            action="store_true", default=False,
                            help="""Show debugging info.""")

        parser.add_argument("--out-directory", dest="out_dir",
                            default="/tmp",
                            help="""Directory for output""")

        return parser

def main():
    env = Env()
    env.do_sample_runs()

if __name__ == "__main__":
    main()
