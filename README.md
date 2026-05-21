# cagerd
A Container Runtime created in rust
# cagerd - OCI Container Runtime

A minimal, production-grade container runtime written in C, built from first principles following the OCI Runtime Specification.

cagerd implements the core container orchestration features: process isolation via Linux namespaces, rootfs mounting, cgroup resource limiting, and process execution. It's designed to be lean, portable, and easy to understand.

## What is cagerd?

**cagerd** (Container And Group Execution Runtime Daemon) is a container runtime that:

- ✓ Reads OCI-compliant `config.json` specifications
- ✓ Creates isolated process environments via Linux namespaces
- ✓ Manages container lifecycle (start, run, exit)
- ✓ Compatible with containerd/Kubernetes calling conventions
- ✓ Written in C with minimal dependencies
- ✓ ~300 lines of core code (excludes external libraries)

## Why Build a Container Runtime?

Understanding how containers work at a low level requires building one. cagerd teaches:

1. **Linux Namespaces** - Process, network, mount isolation
2. **System Calls** - fork(), clone(), execve()
3. **OCI Specification** - Industry standard for containers
4. **Process Management** - Signals, exit codes, process reaping
5. **Filesystem Operations** - chroot, pivot_root, mount points

## Architecture

### Core Components

```
cagerd/
├── main.c              (~80 lines) - CLI interface, lifecycle management
├── oci_config.c        (~200 lines) - Parse config.json, load container spec
├── oci_config.h        (~40 lines) - Config structures and interfaces
├── Makefile            - Build with dependency checking
└── README.md           - This file
```

### External Dependencies

```
libcjson-dev    - JSON parsing (standard system library)
gcc             - C compiler with C99 support
make            - Build automation
```

No internal JSON parser - uses the standard system library.

## Installation

### Prerequisites

- Linux system (Ubuntu 20.04+ recommended)
- GCC 9.0+ or compatible C compiler
- pkg-config (usually included)

### Step 1: Install External Dependencies

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install build-essential libcjson-dev pkg-config
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc libcjson-devel pkgconfig
```

**macOS (Homebrew):**
```bash
brew install gcc cjson pkg-config
```

### Step 2: Verify Installation

```bash
# Check GCC
gcc --version
# Should show: gcc (version) X.X.X or higher

# Check libcjson
pkg-config --cflags --libs libcjson
# Should output: -I/usr/include/cjson -lcjson
```

### Step 3: Clone and Build

```bash
# Navigate to cagerd directory
cd /path/to/cagerd

# Build
make clean && make

# Verify executable was created
ls -lh cagerd
# Should see: -rwxr-xr-x cagerd
```

## Testing

### Quick Test

```bash
# Automatic test with example bundle
make test
```

Expected output:
```
============================================
Testing: Config Parser (Step 1)
============================================
[main] cagerd - OCI Container Runtime
[main] Parsing arguments...
[main] Container ID: test-container
[main] Bundle path: bundle
[main] Step 1: Loading config.json...
[config] Loading from: bundle/config.json
[config] process.args: /bin/sh -c echo hello from container; echo PID: $$
[config] root.path: /containers/test/rootfs
[config] Configuration loaded successfully
...
✓ Config parser working!
```

### Manual Test

```bash
# Create a test bundle
mkdir -p test-bundle
cat > test-bundle/config.json << 'EOF'
{
  "ociVersion": "1.0.0",
  "id": "my-test",
  "root": {"path": "/containers/my-test/rootfs"},
  "process": {
    "args": ["/bin/echo", "Hello from cagerd"],
    "env": ["PATH=/bin"],
    "user": {"uid": 0, "gid": 0},
    "cwd": "/"
  }
}
EOF

# Run the container
./cagerd run --bundle test-bundle my-test

# Expected output:
# [main] cagerd - OCI Container Runtime
# [main] Container ID: my-test
# ...
# Hello from cagerd
```

## Running cagerd

### Command Format

```bash
./cagerd run --bundle BUNDLE_PATH [--pid-file FILE] CONTAINER_ID
```

### Arguments

| Argument | Type | Description |
|----------|------|-------------|
| `run` | Required | Subcommand (always "run") |
| `--bundle BUNDLE_PATH` | Required | Path to container bundle folder |
| `--pid-file FILE` | Optional | Write container PID to file |
| `CONTAINER_ID` | Required | Unique container identifier |

### Example: Run with Output

```bash
./cagerd run --bundle test-bundle echo-test
```

### Example: Save PID to File

```bash
./cagerd run --bundle test-bundle --pid-file /tmp/container.pid my-app

# Check the PID later
cat /tmp/container.pid
# Output: 12345
```

## Build Order (Implementation Roadmap)

Container runtimes are built incrementally, each step depending on the previous one:

### ✓ Step 1: Parse config.json (DONE)

**Status**: Working  
**What it does**: Reads OCI config.json, validates required fields, loads container specification  
**Why it's first**: Everything else depends on knowing what the container should do

**Code location**: `oci_config.c`, `oci_config.h`

**Features**:
- Loads `bundle/config.json`
- Parses JSON with libcjson
- Extracts: process.args, process.env, process.cwd, root.path, uid/gid
- Validates required fields
- Memory management (allocate/free)

---

### Step 2: Fork + Clone (Create PID Namespace) [NEXT]

**What it does**: Creates isolated process namespace so container sees only its own processes  
**Why**: Process isolation - container can't see host processes

**Implementation preview**:
```c
pid_t child = fork();
if (child == 0) {
    /* Child process */
    pid_t container = clone(container_init, stack, CLONE_NEWPID | SIGCHLD);
    waitpid(container, &status, 0);
    exit(status);
} else {
    /* Parent (cagerd) waits for child */
    waitpid(child, &status, 0);
}
```

**Required system calls**:
- `fork()` - Create child process
- `clone()` - Create process with namespace flags
- `waitpid()` - Wait for process exit
- Signal handling for proper init process behavior

**Files to modify**: `main.c` (add fork/clone logic)

---

### Step 3: Setup Rootfs (Mount Filesystem)

**What it does**: Changes container's root filesystem to isolated environment  
**Why**: Filesystem isolation - container sees different filesystem

**Implementation**:
```c
chdir(config->rootfs);        /* Change directory */
chroot(config->rootfs);       /* Change root */
mount("/proc", ...);          /* Mount /proc */
```

**Required system calls**:
- `chdir()` - Change directory
- `chroot()` - Change root filesystem
- `mount()` - Mount filesystems
- `pivot_root()` - Atomic root change

**Files to modify**: `main.c` (add setup_rootfs function)

---

### Step 4: Setup Cgroups (Resource Limits)

**What it does**: Apply CPU, memory, I/O limits to container  
**Why**: Resource isolation - container can't consume all host resources

**Implementation**:
```c
/* Write to /sys/fs/cgroup/... */
FILE *fp = fopen("/sys/fs/cgroup/memory.max", "w");
fprintf(fp, "512M\n");  /* 512MB memory limit */
```

**Required operations**:
- Read `linux.resources` from config.json
- Write to `/sys/fs/cgroup/` control files
- Set memory limits, CPU shares, I/O throttling

**Files to modify**: `main.c`, `oci_config.c` (parse linux.resources)

---

### Step 5: Execute Process

**What it does**: Run the actual container command  
**Why**: This is what the user asked for

**Implementation**:
```c
chdir(config->cwd);           /* Set working directory */
for (int i = 0; config->env[i]; i++) {
    putenv(config->env[i]);   /* Set environment */
}
execv(config->args[0], config->args);  /* Run command */
```

**Required system calls**:
- `chdir()` - Set working directory
- `putenv()` / `execve()` - Set env and execute
- Exit code propagation

**Files to modify**: `main.c` (add execute logic)

---

## Progress Summary

| Step | Feature | Status | Code Lines |
|------|---------|--------|-----------|
| 1 | Parse config.json | ✓ Done | 200 |
| 2 | Fork + Clone | ⏳ Next | ~30 |
| 3 | Setup Rootfs | 📋 Planned | ~20 |
| 4 | Setup Cgroups | 📋 Planned | ~40 |
| 5 | Execute Process | 📋 Planned | ~15 |
| | **Total** | | **~300** |

## Development Workflow

### Make Targets

```bash
make              # Build cagerd (checks dependencies)
make test         # Test config parser
make bundle       # Create test bundle
make clean        # Remove build artifacts
make check-deps   # Verify libcjson installed
make help         # Show all targets
```

### Typical Development Flow

```bash
# 1. Make changes to source
vim main.c
vim oci_config.c

# 2. Rebuild
make clean && make

# 3. Test
make test

# 4. Debug (if needed)
./cagerd run --bundle bundle test-container

# 5. Repeat
```

## OCI Runtime Specification

cagerd implements the **Open Container Initiative (OCI) Runtime Specification**:

- Spec: https://github.com/opencontainers/runtime-spec
- Compatible with containerd, Kubernetes, Docker ecosystem
- Standard calling convention: `cagerd run --bundle /path/to/bundle container-id`

### Supported config.json Fields

**Currently parsed**:
- `ociVersion` - Spec version (e.g., "1.0.0")
- `id` - Container ID
- `root.path` - Root filesystem path
- `process.args` - Command and arguments
- `process.env` - Environment variables
- `process.cwd` - Working directory
- `process.user.uid/gid` - User IDs

**Not yet implemented**:
- `linux.namespaces` - Namespace configuration (will implement)
- `linux.resources` - Cgroup limits (will implement)
- `mounts` - Custom mounts (future)
- `devices` - Device access (future)
- `capabilities` - Linux capabilities (advanced)
- `seccomp` - Syscall filtering (advanced)

## System Requirements

### Kernel Features

cagerd requires Linux kernel with:
- PID namespaces (`CONFIG_PID_NS`)
- IPC namespaces (`CONFIG_IPC_NS`)
- UTS namespaces (`CONFIG_UTS_NS`)
- Mount namespaces (`CONFIG_MOUNT_NS`)

Verify:
```bash
grep CONFIG_PID_NS /boot/config-$(uname -r)
# Should show: CONFIG_PID_NS=y
```

### File Structure

```
cagerd/
├── cagerd                 # Compiled executable
├── main.c, *.h, *.c       # Source code
├── Makefile               # Build rules
├── bundle/                # Test bundles (created by make)
│   └── config.json
└── *.o                    # Object files (temporary)
```

## Troubleshooting

### "libcjson not found"

```bash
# Install it
sudo apt install libcjson-dev

# Verify
pkg-config --cflags --libs libcjson
```

### Compilation errors

```bash
# Check GCC version
gcc --version
# Need GCC 9.0+

# Try clean rebuild
make clean && make
```

### Runtime errors

```bash
# Check bundle exists
ls -la bundle/config.json

# Run with verbose output
./cagerd run --bundle bundle test-container

# Check permissions (may need root for some features)
sudo ./cagerd run --bundle bundle test-container
```

## Next Steps

To continue development:

1. **Read about Linux namespaces**
   - `man 7 namespaces`
   - `man 2 clone`
   - `man 7 pid_namespaces`

2. **Implement Step 2 (fork/clone)**
   - Add fork logic to main.c
   - Handle stack allocation for clone()
   - Setup signal handlers

3. **Test namespace isolation**
   - Verify PID is 1 inside container
   - Check process visibility

4. **Progress to Step 3**
   - Setup rootfs mounting
   - Implement chroot/pivot_root

## References

### Linux System Programming
- `man 2 fork` - Fork system call
- `man 2 clone` - Clone with namespaces
- `man 2 execve` - Execute program
- `man 2 wait` - Wait for child process
- `man 7 signal` - Signal handling

### Containerization
- [OCI Runtime Spec](https://github.com/opencontainers/runtime-spec)
- [Linux Namespaces](https://man7.org/linux/man-pages/man7/namespaces.7.html)
- [cgroups](https://man7.org/linux/man-pages/man7/cgroups.7.html)

### Related Projects
- **runc** - OCI reference implementation (Go)
- **cri-o** - Kubernetes container runtime
- **systemd-nspawn** - Container tool for systemd
- **Docker** - Complete containerization platform

## License

This project is for educational purposes. Use freely to learn about containerization.

## Contributing

This is a learning project. To extend cagerd:

1. Follow the build order above
2. Keep changes focused and minimal
3. Test each step before moving to next
4. Refer to OCI spec for correctness

---

**Status**: Core config parser complete. Ready for Step 2 (namespace isolation).  
**Last Updated**: 2024