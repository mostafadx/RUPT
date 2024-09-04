#!/bin/bash

# Update and install dependencies
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    rdma-core \
    ibverbs-utils \
    perftest \
    infiniband-diags \
    libibverbs-dev \
    libibumad-dev \
    libmlx4-dev \
    libmlx5-dev \
    librdmacm-dev \
    rdmacm-utils \
    numactl \
    libnuma-dev

sudo apt-get install  -y

# Configure hugepages
# Remove any memory lock limits and ensure hugepages are properly configured

echo "Configuring hugepages..."

# Set number of hugepages
echo "Setting number of hugepages to 2048..."
sudo sysctl -w vm.nr_hugepages=2048

# Make it persistent
echo "vm.nr_hugepages=2048" | sudo tee -a /etc/sysctl.conf

# Remove limits on memory locking
echo "Removing limits on memory locking..."
sudo sysctl -w vm.hugetlb_shm_group=0
sudo sysctl -w vm.overcommit_memory=1
sudo sysctl -w vm.max_map_count=65536

# Ensure unlimited locked memory for all users
echo "* soft memlock unlimited" | sudo tee -a /etc/security/limits.conf
echo "* hard memlock unlimited" | sudo tee -a /etc/security/limits.conf

# Reload the system configuration
echo "Applying changes..."
sudo sysctl -p

echo "RDMA environment setup is complete. You can now compile and run your RDMA C code."
