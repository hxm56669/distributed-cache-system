# 04_Ubuntu_VM开发环境配置基线

> 文档性质：Windows 11 宿主机下，为本项目重新创建 Ubuntu VM 的完整开发环境基线。  
> 适用项目：C++ 分布式分层缓存与数据放置系统。  
> 目标：安装完成后可以直接进入 V0-A，支持 C++20、CMakePresets、vcpkg、gRPC、Docker、Samba、SSH、性能分析和后续多进程/多容器模拟。

---

# 1. 最终推荐配置

宿主机当前硬件：

```text
CPU:
Intel Core i7-13700F

物理核心:
16 Core
= 8 P-Core + 8 E-Core

逻辑线程:
24

RAM:
32 GB DDR5

Storage:
NVMe SSD
约 300 GB 可用

GPU:
RTX 5060 8 GB
```

本项目 VM 推荐：

```text
OS:
Ubuntu Server 24.04.5 LTS amd64

Virtual CPU:
12 vCPU

CPU topology:
1 Socket
12 Cores per Socket

RAM:
20 GB 固定

可选高负载模式:
24 GB

System Disk:
80 GB 动态磁盘

Data Disk:
140 GB 动态磁盘

总虚拟磁盘上限:
220 GB

Network:
NIC 1 = NAT
NIC 2 = Host-Only / Internal Network

Filesystem:
ext4

Development:
Windows VS Code
        ↓
Remote SSH
        ↓
Ubuntu VM
```

---

# 2. 为什么选 Ubuntu Server 24.04.5 LTS

截至 2026 年 9 月：

```text
最新 LTS:
Ubuntu Server 26.04.1 LTS

上一代长期支持:
Ubuntu Server 24.04.5 LTS
```

本项目仍推荐：

```text
Ubuntu Server 24.04.5 LTS
```

而不是立即采用 26.04.1。

原因：

1. 24.04 已经经历多个 Point Release；
2. 软件包、驱动、Docker、vcpkg 兼容路径成熟；
3. Ubuntu 24.04 自带 GCC 13 系列，已经完全满足 C++20 项目需求；
4. 本项目使用的 Linux 能力不要求必须采用最新发行版；
5. 后续 Benchmark 更需要稳定环境，而不是频繁升级内核和编译器；
6. vcpkg 已固定独立基线，操作系统也应该保持稳定。

官方来源：

```text
Ubuntu Server:
https://ubuntu.com/download/server

Ubuntu 24.04.5 Release:
https://lists.ubuntu.com/archives/ubuntu-announce/2026-September/000327.html
```

项目生命周期内：

> 默认不进行 24.04 → 26.04 大版本升级。

普通安全更新可以正常安装。

---

# 3. 为什么使用 Ubuntu Server 而不是 Desktop

推荐：

```text
Ubuntu Server
```

不安装完整 GUI。

原因：

```text
更少后台服务
更少内存占用
更稳定的 Benchmark 环境
更接近真实 Linux Server
SSH 开发更自然
Docker 部署更自然
```

开发界面仍然在 Windows：

```text
Windows VS Code
      ↓
Remote SSH
      ↓
Ubuntu Server
```

VM 内无需 GNOME/KDE。

---

# 4. CPU 配置

宿主：

```text
i7-13700F
16 Core
24 Thread
```

VM 推荐：

```text
Virtual Sockets:
1

Cores per Socket:
12

Total vCPU:
12
```

如果虚拟机软件显示为：

```text
Processors
+
Cores per Processor
```

设置：

```text
Processors = 1
Cores per Processor = 12
```

不要设置：

```text
2 Socket × 6 Core
4 Socket × 3 Core
```

因为：

- 项目不需要模拟 NUMA；
- 单 Socket 拓扑更简单；
- 避免虚拟拓扑造成不必要差异；
- 对编译、多 Worker、Docker 已经足够。

---

## 4.1 为什么不是 24 vCPU

不要把宿主 24 个逻辑线程全部交给 VM。

Windows 仍需要：

```text
浏览器
VS Code
VM Hypervisor
文件系统
网络
后台服务
```

12 vCPU 已经能够同时运行：

```text
Control Plane

3~4 Cache Worker

Benchmark Client

MySQL
Redis
Kafka
MinIO
Prometheus
Grafana
```

而且还能给 Windows 留出调度空间。

---

## 4.2 CPU 可选模式

普通开发：

```text
8~10 vCPU
```

完整项目 / Benchmark：

```text
12 vCPU
```

推荐长期固定：

```text
12 vCPU
```

这样 Benchmark 环境保持一致。

---

# 5. 内存配置

宿主：

```text
32 GB
```

推荐 VM：

```text
20 GB
```

不要默认直接给：

```text
28~30 GB
```

否则 Windows 宿主容易发生内存压力和分页。

---

## 5.1 为什么推荐 20 GB

完整环境大致：

```text
Ubuntu OS               1~2 GB

Worker 1                1~2 GB
Worker 2                1~2 GB
Worker 3                1~2 GB

Control Plane           < 1 GB

MySQL                   ~1 GB
Redis                   < 512 MB
Kafka                   1~2 GB
MinIO                   ~1 GB

Prometheus/Grafana      ~1 GB

Build / Link
Linux Page Cache
Benchmark                剩余
```

20 GB 已足够完成项目。

---

## 5.2 什么时候给 24 GB

进入：

```text
V8
V9
V10
V11
```

并且需要同时启动整个 Docker Stack 时，可以：

```text
关闭 Windows 大型程序
↓
把 VM 临时调整为 24 GB
```

但日常推荐仍是：

```text
20 GB
```

---

## 5.3 固定内存还是动态内存

如果 Hypervisor 支持 Dynamic Memory：

普通开发可以使用动态内存。

但正式 Benchmark 时：

```text
建议固定内存
```

推荐：

```text
20 GB fixed
```

避免测试期间宿主和 Guest 动态争抢内存。

---

# 6. Swap

建议保留：

```text
4 GB Swap
```

作用：

- 防止偶发编译峰值直接 OOM；
- Docker 全栈误配置时留出缓冲。

但正式性能测试：

```bash
free -h
swapon --show
```

必须确认：

```text
Swap 基本没有被使用
```

可以降低 swappiness：

```bash
echo 'vm.swappiness=10' | sudo tee /etc/sysctl.d/99-storage-lab.conf

sudo sysctl --system
```

不要依赖 Swap 弥补内存不足。

---

# 7. 磁盘配置

宿主剩余：

```text
约 300 GB NVMe
```

推荐 VM 使用两块逻辑虚拟磁盘。

---

## 7.1 Disk 1：系统盘

```text
80 GB
Dynamic / Thin Provisioned
```

用于：

```text
Ubuntu
/home
源码
vcpkg build cache
编译结果
基础工具
```

---

## 7.2 Disk 2：数据盘

```text
140 GB
Dynamic / Thin Provisioned
```

专门挂载：

```text
/data
```

用于：

```text
Worker Cache
MinIO
Docker Data
Benchmark Dataset
Benchmark Results
Fault Injection
```

总上限：

```text
80 + 140
=
220 GB
```

Windows 仍然保留约：

```text
80 GB
```

以上余量。

---

# 8. 为什么要分两个虚拟磁盘

虽然最终仍然位于同一块物理 NVMe 上，但逻辑分离非常有价值：

```text
System Disk
→ OS / Source / Build

Data Disk
→ Storage Workload
```

优点：

- 数据目录不会把 `/` 写满；
- Docker 不会吃光系统盘；
- Benchmark Dataset 可以快速清理；
- VM 重新安装时数据盘可以单独处理；
- 目录和容量边界更加清晰。

注意：

> 两个虚拟磁盘不代表两块真实 NVMe。

因此 Benchmark 报告仍然必须注明：

```text
Single Physical NVMe
Multiple Virtual Disks
```

---

# 9. 虚拟磁盘格式

推荐：

```text
Dynamic
Thin Provisioned
```

原因：

```text
不会立即占用 220 GB
```

但注意：

随着数据写入，虚拟磁盘实际文件会增长。

正式磁盘极限性能：

> 不使用 VM 数字代表真实 NVMe 极限性能。

项目内主要比较：

```text
优化前
vs
优化后
```

这种相对性能。

---

# 10. Ubuntu 安装时磁盘建议

系统盘：

```text
80 GB
```

Ubuntu 安装器可以使用默认 ext4/LVM。

建议：

```text
/
→ ext4
```

不需要复杂分区。

数据盘：

> 安装完成后再格式化。

---

# 11. 配置 /data 数据盘

首先查看设备：

```bash
lsblk -o NAME,SIZE,FSTYPE,MOUNTPOINTS,MODEL
```

假设第二块虚拟磁盘是：

```text
/dev/sdb
```

注意：

> 实际设备名可能是 `/dev/sdb`、`/dev/vdb`、`/dev/nvme1n1`，必须根据 `lsblk` 确认，不能直接照抄。

例如：

```bash
sudo mkfs.ext4 /dev/sdb
```

创建：

```bash
sudo mkdir -p /data
```

查询 UUID：

```bash
sudo blkid /dev/sdb
```

例如得到：

```text
UUID="xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
```

编辑：

```bash
sudo vim /etc/fstab
```

加入：

```text
UUID=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx /data ext4 defaults,noatime 0 2
```

挂载：

```bash
sudo mount -a
```

检查：

```bash
df -hT /data
```

---

# 12. 数据目录规划

建立：

```bash
sudo mkdir -p \
    /data/docker \
    /data/storage-lab \
    /data/storage-lab/workers \
    /data/storage-lab/backend \
    /data/storage-lab/benchmark \
    /data/storage-lab/results

sudo chown -R "$USER":"$USER" /data/storage-lab
```

后续：

```text
/data/storage-lab/

workers/
    worker1/
    worker2/
    worker3/
    worker4/

backend/

benchmark/

results/
```

---

# 13. Worker 磁盘预算

140 GB 数据盘建议：

```text
Worker 1 Cache     15~20 GB
Worker 2 Cache     15~20 GB
Worker 3 Cache     15~20 GB
Worker 4 Cache     可选 15 GB

Backend / MinIO    40~50 GB

Benchmark          20~30 GB

Logs / Metrics     余量
```

不要第一天就创建 100GB 测试文件。

按照阶段增长。

---

# 14. TRIM

确认：

```bash
systemctl status fstrim.timer
```

如果没有启用：

```bash
sudo systemctl enable --now fstrim.timer
```

是否能真正传递到物理 NVMe 取决于 Hypervisor。

---

# 15. 网络推荐架构

不要只使用 Bridge。

当前最稳方案：

```text
NIC 1
NAT
→ Internet / apt / git / Docker

NIC 2
Host-Only / Internal
→ Windows ↔ Ubuntu
→ SSH
→ Samba
→ 固定开发 IP
```

结构：

```text
                Internet
                   │
                Windows
                   │
              Hypervisor
              /          \
           NAT          Host-Only
            │              │
            └──── Ubuntu ───┘
```

---

# 16. 为什么采用双网卡

之前使用纯 NAT 时会遇到：

```text
Windows 可以访问 VM
但 LAN 其他设备不一定能访问
```

纯桥接又会遇到：

```text
校园网 IP 动态
多 MAC 限制
网络切换
IP 改变
```

所以开发环境最稳的是：

```text
NAT
+
Host-Only
```

这样：

```text
Internet
→ NAT

Windows VS Code
→ 固定 Host-Only IP

Windows SMB
→ 固定 Host-Only IP
```

项目以后需要真实 LAN Client 时，再临时添加/切换 Bridge。

---

# 17. 推荐 Host-Only 地址

示例：

```text
Windows Host-Only Adapter:
192.168.56.1

Ubuntu:
192.168.56.10

Mask:
/24
```

实际网段以 Hypervisor 创建的 Host-Only 网段为准。

---

# 18. Ubuntu 查看网卡名

```bash
ip -br addr
```

可能看到：

```text
enp0s3
enp0s8
```

例如：

```text
enp0s3
→ NAT

enp0s8
→ Host-Only
```

实际名称必须根据机器确认。

---

# 19. Netplan 示例

查看：

```bash
ls /etc/netplan/
```

编辑对应 YAML。

示例：

```yaml
network:
  version: 2
  ethernets:
    enp0s3:
      dhcp4: true

    enp0s8:
      dhcp4: false
      addresses:
        - 192.168.56.10/24
```

注意：

```text
enp0s3 / enp0s8
```

只是示例。

应用：

```bash
sudo netplan try
```

确认正常后：

```bash
sudo netplan apply
```

检查：

```bash
ip -br addr
ip route
```

---

# 20. Hostname

建议：

```bash
sudo hostnamectl set-hostname storage-dev
```

检查：

```bash
hostnamectl
```

之后 VS Code SSH 可以看到清晰机器名。

---

# 21. 系统首次更新

安装完成后第一步：

```bash
sudo apt update
sudo apt full-upgrade -y
sudo reboot
```

重启后：

```bash
cat /etc/os-release
uname -a
```

保存环境基线。

---

# 22. SSH

安装：

```bash
sudo apt install -y openssh-server
```

启用：

```bash
sudo systemctl enable --now ssh
```

检查：

```bash
systemctl status ssh
ss -lntp | grep ':22'
```

Windows 测试：

```powershell
ssh <linux_user>@192.168.56.10
```

---

# 23. SSH Key

Windows PowerShell：

```powershell
ssh-keygen -t ed25519
```

然后把：

```text
%USERPROFILE%\.ssh\id_ed25519.pub
```

内容加入 Ubuntu：

```text
~/.ssh/authorized_keys
```

Ubuntu 权限：

```bash
chmod 700 ~/.ssh
chmod 600 ~/.ssh/authorized_keys
```

验证免密登录成功后，再考虑关闭 SSH Password Authentication。

---

# 24. VS Code Remote SSH

Windows：

安装扩展：

```text
Remote - SSH
C/C++
CMake Tools
```

`~/.ssh/config`：

```text
Host storage-dev
    HostName 192.168.56.10
    User <linux_user>
    IdentityFile ~/.ssh/id_ed25519
```

以后：

```text
Remote SSH
→ storage-dev
```

---

# 25. Samba 的定位

Samba 只用于：

```text
Windows ↔ Ubuntu 文件交换
文档
Benchmark Result
临时文件
```

不建议把项目源码放在 Windows NTFS 后再通过 SMB 编译。

正式开发：

```text
源码
→ Ubuntu ext4

VS Code
→ Remote SSH
```

正式 Benchmark：

```text
Dataset
→ /data ext4
```

Samba 不进入性能测试路径。

---

# 26. 安装 Samba

```bash
sudo apt install -y \
    samba \
    smbclient \
    cifs-utils
```

官方 Ubuntu 使用：

```text
/etc/samba/smb.conf
```

进行配置。

---

# 27. 创建 Samba Share

创建目录：

```bash
sudo mkdir -p /srv/devshare

sudo chown "$USER":"$USER" /srv/devshare

sudo chmod 2775 /srv/devshare
```

这里专门作为：

```text
Windows ↔ Ubuntu
```

交换目录。

---

# 28. 配置 Samba 用户

直接使用当前 Linux 用户：

```bash
sudo smbpasswd -a "$USER"
```

按照提示设置一个 Samba 密码。

启用：

```bash
sudo smbpasswd -e "$USER"
```

---

# 29. smb.conf

备份：

```bash
sudo cp \
    /etc/samba/smb.conf \
    /etc/samba/smb.conf.bak
```

编辑：

```bash
sudo vim /etc/samba/smb.conf
```

文件末尾加入：

```ini
[devshare]
    comment = Windows Ubuntu Development Share
    path = /srv/devshare
    browseable = yes
    read only = no
    guest ok = no

    valid users = YOUR_LINUX_USERNAME

    create mask = 0664
    directory mask = 0775
```

必须把：

```text
YOUR_LINUX_USERNAME
```

替换为：

```bash
whoami
```

显示的真实 Linux 用户名。

---

# 30. 检查 Samba 配置

```bash
testparm
```

没有配置错误后：

```bash
sudo systemctl restart smbd
sudo systemctl enable smbd
```

查看：

```bash
systemctl status smbd
```

---

# 31. Windows 访问 Samba

资源管理器地址栏：

```text
\\192.168.56.10\devshare
```

输入：

```text
Ubuntu Linux username
+
Samba password
```

可以映射网络驱动器，例如：

```text
Z:
```

但：

> 不要从 `Z:` 盘执行项目 Linux Benchmark。

---

# 32. Samba 故障检查

Ubuntu：

```bash
smbclient -L localhost -U "$USER"
```

检查监听：

```bash
ss -lntup | grep -E '445|139|137|138'
```

Windows：

```powershell
Test-NetConnection 192.168.56.10 -Port 445
```

---

# 33. 防火墙建议

开发初期先确认：

```text
SSH
Samba
NAT
Host-Only
```

全部正常，再启用 UFW。

安装：

```bash
sudo apt install -y ufw
```

只允许 Host-Only 网段 SSH：

```bash
sudo ufw allow from 192.168.56.0/24 \
    to any port 22 proto tcp
```

Samba：

```bash
sudo ufw allow from 192.168.56.0/24 \
    to any port 445 proto tcp

sudo ufw allow from 192.168.56.0/24 \
    to any port 139 proto tcp

sudo ufw allow from 192.168.56.0/24 \
    to any port 137 proto udp

sudo ufw allow from 192.168.56.0/24 \
    to any port 138 proto udp
```

然后：

```bash
sudo ufw enable
sudo ufw status verbose
```

如果你的 Host-Only 网段不是：

```text
192.168.56.0/24
```

必须修改。

---

# 34. Docker 与 UFW 注意事项

Docker 官方明确提醒：

> Docker 发布的容器端口可能绕过普通 UFW 规则。

因此后期 Docker 阶段：

```text
不要把敏感服务
0.0.0.0 暴露到所有接口
```

例如开发时优先：

```text
127.0.0.1:3306
192.168.56.10:xxxx
```

而不是：

```text
0.0.0.0:3306
```

---

# 35. 基础编译工具

一次安装：

```bash
sudo apt install -y \
    build-essential \
    gcc \
    g++ \
    clang \
    clang-format \
    clang-tidy \
    cmake \
    ninja-build \
    make \
    pkg-config \
    ccache
```

其中：

```text
build-essential
→ gcc
→ g++
→ libc headers
→ make
```

Ubuntu 24.04 提供 GCC 13 系列，满足项目：

```text
C++20
```

要求。

---

# 36. 检查编译器

```bash
gcc --version
g++ --version
clang --version
cmake --version
ninja --version
```

项目基线要求：

```text
GCC >= 13
```

Ubuntu 24.04 满足。

---

# 37. 开发基础工具

安装：

```bash
sudo apt install -y \
    git \
    git-lfs \
    curl \
    wget \
    ca-certificates \
    gnupg \
    unzip \
    zip \
    tar \
    xz-utils \
    jq \
    tree \
    vim \
    tmux \
    htop \
    ripgrep \
    fd-find
```

---

# 38. Python

虽然项目主体是 C++，但：

```text
Benchmark script
数据处理
报告汇总
构建工具
```

可能使用 Python。

安装：

```bash
sudo apt install -y \
    python3 \
    python3-pip \
    python3-venv
```

原则：

> Python 只做工具，不替代核心 C++ 实现。

---

# 39. vcpkg 编译前置工具

建议：

```bash
sudo apt install -y \
    autoconf \
    automake \
    libtool \
    m4 \
    bison \
    flex \
    nasm
```

这些不是全部项目直接依赖，但某些 vcpkg Ports 的构建可能需要。

---

# 40. 网络调试工具

安装：

```bash
sudo apt install -y \
    iproute2 \
    iputils-ping \
    dnsutils \
    netcat-openbsd \
    socat \
    tcpdump \
    lsof \
    iperf3
```

主要用途：

```text
ip / ss
→ 网络状态

ping
→ 连通性

dig
→ DNS

nc / socat
→ 端口测试

tcpdump
→ 数据包

iperf3
→ 网络吞吐测试
```

---

# 41. 性能工具

安装：

```bash
sudo apt install -y \
    strace \
    sysstat \
    fio \
    linux-tools-common \
    linux-tools-generic
```

其中：

```text
strace
→ syscall

perf
→ CPU hotspot

iostat
→ Disk IO

pidstat
→ Process CPU/IO

fio
→ Disk workload
```

验证：

```bash
perf --version
strace --version
iostat -V
pidstat -V
fio --version
```

---

# 42. perf 如果无法使用

查看：

```bash
uname -r
```

尝试：

```bash
sudo apt install \
    linux-tools-common \
    linux-tools-"$(uname -r)"
```

不同 HWE Kernel 的包名可能不同。

不要为了 perf 盲目更换 Kernel。

---

# 43. Debug 工具

安装：

```bash
sudo apt install -y \
    gdb \
    lldb \
    valgrind
```

主要：

```text
gdb
→ GCC Debug

lldb
→ Clang Debug

valgrind
→ 辅助内存分析
```

项目主要内存检测仍然使用：

```text
ASan
UBSan
```

---

# 44. Git 基础配置

推荐：

```bash
git config --global init.defaultBranch main

git config --global core.autocrlf input

git config --global pull.rebase false
```

用户名和邮箱：

```bash
git config --global user.name "YOUR_NAME"

git config --global user.email "YOUR_EMAIL"
```

不要把 Windows 的：

```text
core.autocrlf=true
```

习惯带进 Linux 仓库。

---

# 45. 开发目录

推荐：

```bash
mkdir -p ~/workspace
mkdir -p ~/tools
```

代码：

```text
~/workspace/<project-name>
```

工具：

```text
~/tools/
```

Benchmark 数据：

```text
/data/storage-lab/
```

不要：

```text
/home
```

里堆 100GB 测试数据。

---

# 46. vcpkg

环境阶段：

> 只安装 vcpkg 所需系统工具。

正式 vcpkg 仓库建议由 V0-A 作为项目的一部分加入：

```text
third_party/vcpkg
```

并 checkout：

```text
9e593bb18ea69cc5095e012465dcd675a822ed0d
```

对应项目已确定基线：

```text
vcpkg 2026.07.29
```

不要现在：

```text
apt install 各种 C++ 第三方 dev package
```

例如不要分别：

```text
apt install libgrpc-dev
apt install libprotobuf-dev
apt install libspdlog-dev
```

因为项目统一使用：

```text
vcpkg
```

管理 C++ 三方依赖。

---

# 47. Docker Engine

建议直接安装在：

```text
Ubuntu VM
```

不要让项目依赖：

```text
Windows Docker Desktop
→ WSL
→ 再进入 Ubuntu VM
```

结构保持：

```text
Windows
 ↓
Ubuntu VM
 ↓
Docker Engine
```

---

# 48. Docker 官方安装

先移除可能冲突包：

```bash
sudo apt remove -y \
    docker.io \
    docker-compose \
    docker-compose-v2 \
    docker-doc \
    podman-docker \
    containerd \
    runc || true
```

添加 Docker 官方仓库：

```bash
sudo apt update

sudo apt install -y \
    ca-certificates \
    curl

sudo install -m 0755 -d /etc/apt/keyrings

sudo curl -fsSL \
    https://download.docker.com/linux/ubuntu/gpg \
    -o /etc/apt/keyrings/docker.asc

sudo chmod a+r /etc/apt/keyrings/docker.asc
```

创建：

```bash
sudo tee /etc/apt/sources.list.d/docker.sources >/dev/null <<EOF
Types: deb
URIs: https://download.docker.com/linux/ubuntu
Suites: $(. /etc/os-release && echo "${UBUNTU_CODENAME:-$VERSION_CODENAME}")
Components: stable
Architectures: $(dpkg --print-architecture)
Signed-By: /etc/apt/keyrings/docker.asc
EOF
```

安装：

```bash
sudo apt update

sudo apt install -y \
    docker-ce \
    docker-ce-cli \
    containerd.io \
    docker-buildx-plugin \
    docker-compose-plugin
```

验证：

```bash
sudo docker run hello-world
```

官方文档：

```text
https://docs.docker.com/engine/install/ubuntu/
```

---

# 49. 当前用户使用 Docker

```bash
sudo usermod -aG docker "$USER"
```

注销 SSH：

```bash
exit
```

重新登录后：

```bash
docker version
docker compose version
```

验证：

```bash
docker run hello-world
```

注意：

> `docker` group 实际拥有接近 root 的权限，只在自己的开发 VM 中使用。

---

# 50. Docker Data Root

为了不把：

```text
/
```

系统盘写满，建议把 Docker 放到：

```text
/data/docker
```

在新环境且 Docker 尚无重要数据时：

```bash
sudo mkdir -p /data/docker
```

创建：

```bash
sudo mkdir -p /etc/docker
```

编辑：

```bash
sudo vim /etc/docker/daemon.json
```

写入：

```json
{
  "data-root": "/data/docker"
}
```

然后：

```bash
sudo systemctl restart docker
```

检查：

```bash
docker info | grep "Docker Root Dir"
```

应该看到：

```text
Docker Root Dir: /data/docker
```

---

# 51. VM Guest Tools

根据 Hypervisor 选择。

## VMware

```bash
sudo apt install -y open-vm-tools
```

## VirtualBox

使用：

```text
VirtualBox Guest Additions
```

或对应 Ubuntu guest package。

## Hyper-V

Ubuntu 内核已经集成主要 Hyper-V 驱动。

通常无需额外安装完整 Guest Tools。

不要同时安装多套 Hypervisor Guest Tools。

---

# 52. VS Code 文件监控

大型 C++ 仓库可能需要提高：

```text
inotify
```

可以创建：

```bash
sudo tee /etc/sysctl.d/98-development.conf >/dev/null <<EOF
fs.inotify.max_user_watches=524288
fs.inotify.max_user_instances=1024
EOF
```

应用：

```bash
sudo sysctl --system
```

---

# 53. Open Files

项目后期大量连接时可能需要提高：

```text
nofile
```

但 V0 不需要立即调整。

到多 Client Benchmark 时，再检查：

```bash
ulimit -n
```

如果成为瓶颈，再明确提高。

不要第一天无意义改大量 Kernel 参数。

---

# 54. 时间同步

检查：

```bash
timedatectl
```

确认：

```text
System clock synchronized: yes
```

如未同步：

```bash
sudo systemctl enable --now systemd-timesyncd
```

分布式日志和 latency 测试需要时钟稳定。

---

# 55. 基础环境一次性安装命令

可以分组执行，不要求必须一次粘贴全部。

## Build + Development

```bash
sudo apt update

sudo apt install -y \
    build-essential \
    gcc \
    g++ \
    clang \
    clang-format \
    clang-tidy \
    cmake \
    ninja-build \
    make \
    pkg-config \
    ccache \
    git \
    git-lfs \
    curl \
    wget \
    ca-certificates \
    gnupg \
    unzip \
    zip \
    tar \
    xz-utils \
    jq \
    tree \
    vim \
    tmux \
    htop \
    ripgrep \
    fd-find \
    python3 \
    python3-pip \
    python3-venv
```

## Build Helpers

```bash
sudo apt install -y \
    autoconf \
    automake \
    libtool \
    m4 \
    bison \
    flex \
    nasm
```

## Network

```bash
sudo apt install -y \
    openssh-server \
    iproute2 \
    iputils-ping \
    dnsutils \
    netcat-openbsd \
    socat \
    tcpdump \
    lsof \
    iperf3
```

## Performance

```bash
sudo apt install -y \
    gdb \
    lldb \
    valgrind \
    strace \
    sysstat \
    fio \
    linux-tools-common \
    linux-tools-generic
```

## Samba

```bash
sudo apt install -y \
    samba \
    smbclient \
    cifs-utils
```

---

# 56. 不要通过 apt 安装这些项目依赖

不要：

```bash
sudo apt install libgrpc-dev
sudo apt install libprotobuf-dev
sudo apt install libspdlog-dev
sudo apt install libgtest-dev
```

因为这些 C++ Library 统一交给：

```text
vcpkg
```

管理。

系统工具：

```text
gcc
cmake
ninja
perf
strace
docker
```

才通过 apt 管理。

---

# 57. 安装完成后版本记录

执行：

```bash
{
    echo "=== OS ==="
    cat /etc/os-release

    echo
    echo "=== Kernel ==="
    uname -a

    echo
    echo "=== CPU ==="
    lscpu

    echo
    echo "=== Memory ==="
    free -h

    echo
    echo "=== Disk ==="
    lsblk -o NAME,SIZE,FSTYPE,MOUNTPOINTS

    echo
    echo "=== Compiler ==="
    gcc --version | head -1
    g++ --version | head -1
    clang --version | head -1

    echo
    echo "=== Build ==="
    cmake --version | head -1
    ninja --version

    echo
    echo "=== Tools ==="
    git --version
    python3 --version
    docker --version
    docker compose version
} | tee ~/environment-baseline.txt
```

这个文件建议后续复制到：

```text
docs/environment/
```

或者 Benchmark 报告中。

---

# 58. 性能 Benchmark 前环境检查

正式测试前：

```bash
free -h
swapon --show
df -hT
uptime
```

保证：

```text
Windows 没有大型后台任务
VM 内没有 apt upgrade
Docker 不运行无关容器
Swap 基本未使用
Dataset 在 /data
```

检查：

```bash
docker ps
ps aux --sort=-%cpu | head
```

Benchmark 报告记录：

```text
VM vCPU
VM RAM
Kernel
Compiler
Filesystem
Virtual Disk
Physical NVMe
```

---

# 59. Snapshot 建议

新 VM 推荐创建几个 Snapshot。

## Snapshot 1

```text
clean-ubuntu-24.04.5
```

时间：

```text
系统刚安装完成
更新完成
SSH 正常
```

## Snapshot 2

```text
base-dev-tools
```

时间：

```text
编译工具
Samba
SSH
性能工具
全部正常
```

## Snapshot 3

```text
docker-ready
```

时间：

```text
Docker 安装完成
/data 配置完成
```

之后不要每个开发阶段都依赖 VM Snapshot。

项目版本仍以：

```text
Git
```

为准。

---

# 60. 最终环境结构

完成后整机：

```text
Windows 11 Host
│
├── Browser
├── VS Code
│     │
│     └── Remote SSH
│
└── Ubuntu Server 24.04.5 VM
      │
      ├── 12 vCPU
      ├── 20 GB RAM
      │
      ├── 80 GB System Disk
      │
      ├── 140 GB /data Disk
      │
      ├── NAT
      ├── Host-Only 192.168.56.10
      │
      ├── SSH
      ├── Samba
      │
      ├── GCC / Clang
      ├── CMake / Ninja
      ├── Git
      ├── Python
      │
      ├── perf / strace
      ├── iostat / pidstat
      ├── fio / iperf3
      │
      ├── Docker Engine
      │
      └── ~/workspace/
            └── <project>
```

数据：

```text
/data/
├── docker/
└── storage-lab/
    ├── workers/
    ├── backend/
    ├── benchmark/
    └── results/
```

Windows 文件交换：

```text
\\192.168.56.10\devshare
```

---

# 61. 最终推荐值汇总

| 配置项 | 最终建议 |
|---|---|
| Ubuntu | Server 24.04.5 LTS amd64 |
| CPU | 12 vCPU |
| CPU 拓扑 | 1 Socket × 12 Core |
| RAM | 20 GB |
| 高负载 RAM | 可临时 24 GB |
| Swap | 4 GB |
| 系统盘 | 80 GB 动态 |
| 数据盘 | 140 GB 动态 |
| 文件系统 | ext4 |
| 开发网络 | NAT + Host-Only |
| 固定开发 IP | 示例 192.168.56.10 |
| LAN 测试 | 需要时再加 Bridge |
| 源码位置 | `~/workspace` |
| Benchmark 数据 | `/data/storage-lab` |
| Docker 数据 | `/data/docker` |
| Windows 共享 | `/srv/devshare` |
| 编译器 | GCC 13+ / Clang |
| C++ | C++20 |
| Build | CMake + Ninja |
| Dependency | vcpkg |
| IDE | Windows VS Code Remote SSH |

---

# 62. VM 环境最终验收

全部完成后逐项检查：

```text
[ ] Ubuntu Server 24.04.5

[ ] 12 vCPU
[ ] 20 GB RAM

[ ] 80 GB system disk
[ ] 140 GB /data

[ ] ext4

[ ] NAT Internet 正常
[ ] Host-Only 固定地址正常

[ ] Windows → Ubuntu SSH 正常
[ ] VS Code Remote SSH 正常

[ ] Samba 可从 Windows 访问

[ ] GCC / G++ 正常
[ ] Clang 正常
[ ] CMake 正常
[ ] Ninja 正常

[ ] Git 正常
[ ] Python 正常

[ ] perf 正常
[ ] strace 正常
[ ] iostat 正常
[ ] pidstat 正常
[ ] fio 正常
[ ] iperf3 正常

[ ] Docker Engine 正常
[ ] docker compose 正常
[ ] Docker Root Dir = /data/docker

[ ] /data/storage-lab 已创建

[ ] environment-baseline.txt 已生成
```

只有环境通过这些验收后，才正式进入：

```text
V0-A
仓库初始化
+
vcpkg submodule
+
vcpkg.json
+
CMakePresets.json
+
顶层 CMake
```

---

# 63. 当前不要做的环境优化

新 VM 创建完成后，不要立即：

```text
改 CPU Governor
HugePages
NUMA Binding
IRQ Affinity
手工 Kernel Upgrade
自编译 Kernel
关闭大量内核安全特性
调几十个 sysctl
安装 CUDA
安装 Kubernetes
```

这些都不是当前开发前置条件。

原则：

> 先保持一个干净、稳定、可复现的 Ubuntu 基线。

后续某个 Benchmark 明确证明系统参数成为瓶颈时，再单独调整并记录 A/B 数据。

---

# 64. 官方参考

Ubuntu Server：

```text
https://ubuntu.com/download/server
```

Ubuntu 24.04.5 Release：

```text
https://lists.ubuntu.com/archives/ubuntu-announce/2026-September/000327.html
```

Ubuntu Server System Requirements：

```text
https://ubuntu.com/server/docs/reference/installation/system-requirements/
```

Ubuntu Samba File Server：

```text
https://documentation.ubuntu.com/server/how-to/samba/file-server/
```

Docker Engine on Ubuntu：

```text
https://docs.docker.com/engine/install/ubuntu/
```

这些文档用于确认发行版、Samba 和 Docker 的基础配置方式。
