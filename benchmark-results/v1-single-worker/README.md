# V1 single-worker baseline

- Commit under test: `aba20705b0371f15bd153d51bc05b35dc3d70250` plus the uncommitted V1 diff
- Build: `bench-release`, GCC, C++20, no sanitizer
- Environment: Ubuntu VM, Linux 6.8.0-139-generic, ext4
- CPU: 12 vCPU, Intel Core i7-13700F
- RAM: 15 GiB
- Topology: one client and one worker on loopback; one object equals one chunk
- Repetitions: one baseline run per size
- Page cache: not dropped; each source and destination was newly created, then removed
- Input: sparse zero-filled files made with `truncate`; all bytes still traversed the client,
  gRPC stream, checksum pipeline, and worker file IO
- Put includes source BLAKE3 pass, upload, worker BLAKE3 verification, `fdatasync`, and commit
- Get includes download, destination `fdatasync`, BLAKE3 verification, and commit

Raw measurements are in `results.csv`. This is a functional V1 baseline from a single VM, not a
multi-machine performance claim. The largest observed client RSS in the final run was 642,180 KiB
for a 5 GiB Get; the data path uses fixed 3 MiB application buffers and a 256 MiB gRPC resource
quota and never allocates an object-sized buffer.
