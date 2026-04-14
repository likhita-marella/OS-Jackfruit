🚀 OS-Jackfruit: Multi-Container Runtime

A lightweight Linux container runtime built in C, implementing:

Process isolation via Linux namespaces
A long-running supervisor daemon
Real-time structured logging
A kernel-space memory monitor via a custom Loadable Kernel Module (LKM)
📌 Overview

OS-Jackfruit is a stripped-down container runtime inspired by Docker — without layers, registries, or networking.

It focuses purely on core OS primitives:

Namespace isolation using clone()
Filesystem isolation using chroot()
Process lifecycle management via a supervisor daemon
Structured logging using pipe + fork
Kernel-level monitoring via /dev/container_monitor
🏗️ Architecture
engine (user-space)
 ├── CLI
 ├── Supervisor
 ├── Container Process (clone + namespaces)
 └── Log Manager (pipe + fork)
          │
          │ ioctl
          ▼
monitor.ko (kernel module)
 └── /dev/container_monitor
     └── Tracks PID & memory usage
⚙️ Features
run → Launch interactive container (foreground)
start → Run container in background with logging
ps → List running containers
stop → Gracefully terminate containers
Filesystem isolation per container
Real-time logging (stdout + file)
Kernel module monitoring
CI-safe build support
📁 Project Structure
.
├── boilerplate/
│   ├── engine.c
│   ├── monitor.c
│   ├── monitor_ioctl.h
│   ├── Makefile
│   ├── cpu_hog.c
│   ├── memory_hog.c
│   ├── io_pulse.c
│   └── environment-check.sh
├── rootfs-alpha/
├── rootfs-beta/
└── README.md

🛠️ Setup & Installation

✅ Requirements

Ubuntu 22.04 / 24.04
build-essential
linux-headers
Secure Boot OFF
❌ WSL not supported

1. Install Dependencies
sudo apt update
sudo apt install -y build-essential linux-headers-$(uname -r)

3. Run Environment Check
cd boilerplate
chmod +x environment-check.sh
sudo ./environment-check.sh

5. Prepare Root Filesystem
mkdir rootfs-base

wget https://dl-cdn.alpinelinux.org/alpine/v3.20/releases/x86_64/alpine-minirootfs-3.20.3-x86_64.tar.gz

tar -xzf alpine-minirootfs-3.20.3-x86_64.tar.gz -C rootfs-base

cp -a ./rootfs-base ./rootfs-alpha
cp -a ./rootfs-base ./rootfs-beta

⚠️ Do NOT commit rootfs directories

4. Build
cd boilerplate
make
make ci
▶️ Usage
Run Interactive Container
sudo ./engine run alpha ../rootfs-alpha
Start Background Container
sudo ./engine start alpha ../rootfs-alpha
List Containers
./engine ps

Example:

NAME     PID     UPTIME
alpha    13245   00:02:14
Stop Container
sudo ./engine stop alpha
View Logs
cat ../rootfs-alpha/logs/alpha.log

🧠 Kernel Module
Load / Unload
sudo insmod monitor.ko
sudo rmmod monitor
Verify Device
ls /dev/container_monitor
Kernel Logs
dmesg | tail -20

🧪 Workload Experiments
Workload	Behavior
cpu_hog	CPU intensive
memory_hog	Memory pressure
io_pulse	I/O bursts
Run
./cpu_hog
./memory_hog
./io_pulse

💡 Summary

This project demonstrates:

Linux namespaces
Process isolation
Kernel-user interaction
Scheduling behavior

A clean way to understand how containers work under the hood 🚀
