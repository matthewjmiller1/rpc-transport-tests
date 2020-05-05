These are utility scripts to run an generate stats.

E.g., to do a write runs with grpc and flatbuffers over different block sizes:
```
$ run_transports.py -w write -t grpc flatbuffers -s 512 1024 2048
  Output is in /tmp/tmpfwxjjget/
```

This will generate a .png comparing the transports in the given output
directory.

`sample_runs.py` will collect data over a few sample runs into a given
directory.

```
$ mkdir -p sample_data/run1
$ sample_runs.py --out-dir sample_data/run1
```
