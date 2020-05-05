#!/usr/bin/python3.8 -u
"""
Run transport test for the give parameters and graph results.
"""

import matplotlib.pyplot as plt
import argparse
import subprocess
import tempfile
import re

class Stats:
    def __init__(self):
        self.block_count = None
        self.block_size = None
        self.tput_avg = None
        self.tput_dev = None
        self.lat_avg = None
        self.lat_dev = None

    def __repr__(self):
        return (f"block (count={self.block_count}, size={self.block_size}), "
                f"tput (avg={self.tput_avg}, dev={self.tput_dev}), "
                f"lat (avg={self.lat_avg}, dev={self.lat_dev})")

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

    def client_file(self, out_dir, transport):
        return f"{out_dir}/client_out_{transport}.txt"

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
        fname = self.client_file(out_dir, "echo")
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
                fname = self.client_file(out_dir, transport)
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
            try:
                self.run_client(t, out_dir)
            finally:
                self.stop_server(p)

    def parse_file(self, fname):
        re_header = re.compile(r'.* for (\d+) blocks of size (\d+) .*')
        re_tput = re.compile(r'Throughput.* avg=(\d+\.\d+) dev=(\d+\.\d+) .*') 
        re_lat = re.compile(r'Latency.* avg=(\d+\.\d+) dev=(\d+\.\d+) .*') 
        re_passed = re.compile(r'Test passed') 

        bc_is_key = len(self.args.block_count) > 1

        stats = {}
        with open(fname, 'r') as f:
            s = None
            for line in f:
                m = re_header.match(line)
                if m:
                    s = Stats()
                    (s.block_count, s.block_size) = \
                        tuple(float(i) for i in m.group(1, 2))

                m = re_tput.match(line)
                if m:
                    (s.tput_avg, s.tput_dev) = \
                        tuple(float(i) for i in m.group(1, 2))

                m = re_lat.match(line)
                if m:
                    (s.lat_avg, s.lat_dev) = \
                        tuple(float(i) for i in m.group(1, 2))

                m = re_passed.search(line)
                if m:
                    if (bc_is_key):
                        stats[s.block_count] = s
                    else:
                        stats[s.block_size] = s
                    s = None

        return stats

    def generate_graphs(self, out_dir):
        transport_data = {}
        for t in self.args.transports:
            fname = self.client_file(out_dir, t)
            transport_data[t] = self.parse_file(fname)

        if (self.args.do_debug):
            print(f"Transport data: {transport_data}")

        bc_is_x = len(self.args.block_count) > 1
        if (bc_is_x):
            title_substr = f"block size={self.args.block_size[0]} bytes"
            xlabel = "block count/op"
        else:
            title_substr = f"block count/op={self.args.block_count[0]}"
            xlabel = "block size (bytes)"

        title = (f"{self.args.workload}, {title_substr}, "
                 f"N={self.args.op_count}")

        fig, axs = plt.subplots(2)
        axs[0].set(title="Throughput", xlabel=xlabel,
                   ylabel="Throughput (Mbps)")
        axs[1].set(title="Latency", xlabel=xlabel, ylabel="Latency (ms)")

        for (k1, v1) in sorted(transport_data.items()):
            y_vals = []
            e_vals = []
            y_vals.extend([[], []])
            e_vals.extend([[], []])
            x_vals = sorted(list(v1.keys()))

            for (k2, v2) in sorted(v1.items()):
                y_vals[0].append(v2.tput_avg)
                e_vals[0].append(v2.tput_dev)
                y_vals[1].append(v2.lat_avg)
                e_vals[1].append(v2.lat_dev)

            for (i, ax) in enumerate(axs):
                if (self.args.do_debug):
                    print(x_vals)
                    print(y_vals[i])
                    print(e_vals[i])
                ax.errorbar(x_vals, y_vals[i], e_vals[i], capsize=5, fmt="o-",
                            label=k1)

        # Do these after all the data is plotted
        for ax in axs:
            ax.set_ylim(bottom=0)
            ax.legend()

        # Set a title for both subplots
        st = fig.suptitle(title)

        fig.tight_layout()

        # Shift subplots down to avoid overlapping with master title
        st.set_y(0.95)
        fig.subplots_adjust(top=0.85)

        plt.savefig(f"{out_dir}/transport_data.png")

    def run_transports(self):
        dir = tempfile.mkdtemp(dir=self.args.out_dir)

        self.run_echo_test(dir)
        self.run_tests(dir)
        self.generate_graphs(dir)

        print(f"  Output is in {dir}/")

    def create_parser(self):
        parser = argparse.ArgumentParser(description="""
                                         Run transport tests and graph results.
                                         """,
                                         formatter_class=
                                         argparse.ArgumentDefaultsHelpFormatter)

        parser.add_argument("-d", "--debug", dest="do_debug",
                            action="store_true", default=False,
                            help="""Show debugging info.""")

        parser.add_argument("--out-directory", dest="out_dir",
                            default="/tmp",
                            help="""Directory for output""")

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
