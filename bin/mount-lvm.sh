#!/bin/bash
#
# Script to check for drives before mounting LVM drive
# Usage: mount-lvm.sh <volume_group> <logical_volume> <mount_point>
#

set -e

# Color codes for output
NORMAL_COLOR="\033[0m"
GREEN="\033[32m"
BLUE="\033[34m"
RED="\033[31m"
YELLOW="\033[33m"

# Check if running as root
if [[ $EUID -ne 0 ]]; then
   echo -e "${RED}[ERROR]${NORMAL_COLOR} This script must be run as root"
   exit 1
fi

# Check arguments
if [ "$#" -ne 3 ]; then
    echo -e "${RED}[ERROR]${NORMAL_COLOR} Invalid number of arguments"
    echo "Usage: $0 <volume_group> <logical_volume> <mount_point>"
    echo "Example: $0 vg_data lv_memo /mnt/memo"
    exit 1
fi

VG_NAME="$1"
LV_NAME="$2"
MOUNT_POINT="$3"

echo -e "${BLUE}[INFO]${NORMAL_COLOR} Starting LVM mount check for ${VG_NAME}/${LV_NAME}"

# Check if required LVM tools are available
echo -e "${BLUE}[INFO]${NORMAL_COLOR} Checking for LVM tools..."
for cmd in vgdisplay pvdisplay lvdisplay pvscan vgchange lvchange vgs pvs lvs blkid; do
    if ! command -v "$cmd" &> /dev/null; then
        echo -e "${RED}[ERROR]${NORMAL_COLOR} LVM tool '$cmd' not found. Please install lvm2 package"
        exit 1
    fi
done
echo -e "${GREEN}[SUCCESS]${NORMAL_COLOR} All required LVM tools are available"

# Scan for LVM physical volumes
echo -e "${BLUE}[INFO]${NORMAL_COLOR} Scanning for LVM physical volumes..."
if ! pvscan > /dev/null 2>&1; then
    echo -e "${RED}[ERROR]${NORMAL_COLOR} Failed to scan for physical volumes"
    exit 1
fi

# Check if volume group exists
echo -e "${BLUE}[INFO]${NORMAL_COLOR} Checking volume group: ${VG_NAME}"
if ! vgdisplay "${VG_NAME}" > /dev/null 2>&1; then
    echo -e "${RED}[ERROR]${NORMAL_COLOR} Volume group '${VG_NAME}' not found"
    echo -e "${YELLOW}[INFO]${NORMAL_COLOR} Available volume groups:"
    vgs --noheadings -o vg_name 2>/dev/null || echo "  None found"
    exit 1
fi

# Ensure volume group is active (vgchange -ay is idempotent)
echo -e "${BLUE}[INFO]${NORMAL_COLOR} Activating volume group..."
if ! vgchange -ay "${VG_NAME}" > /dev/null 2>&1; then
    echo -e "${RED}[ERROR]${NORMAL_COLOR} Failed to activate volume group '${VG_NAME}'"
    exit 1
fi
echo -e "${GREEN}[SUCCESS]${NORMAL_COLOR} Volume group is active"

# Check physical volumes in the volume group
echo -e "${BLUE}[INFO]${NORMAL_COLOR} Checking physical volumes for volume group..."
PV_COUNT=$(pvs --noheadings -o pv_name -S vg_name="${VG_NAME}" 2>/dev/null | wc -l)
if [ "${PV_COUNT}" -eq 0 ]; then
    echo -e "${RED}[ERROR]${NORMAL_COLOR} No physical volumes found for volume group '${VG_NAME}'"
    exit 1
fi

echo -e "${GREEN}[SUCCESS]${NORMAL_COLOR} Found ${PV_COUNT} physical volume(s) for volume group '${VG_NAME}'"
pvs --noheadings -o pv_name,pv_size,pv_used -S vg_name="${VG_NAME}" 2>/dev/null | while read -r line; do
    echo -e "${BLUE}[INFO]${NORMAL_COLOR}   PV: ${line}"
done

# Check if all physical volumes are available and healthy
PV_MISSING=$(vgs --noheadings -o vg_missing_pv_count "${VG_NAME}" 2>/dev/null | tr -d ' ')
if [ "${PV_MISSING}" != "0" ]; then
    echo -e "${RED}[ERROR]${NORMAL_COLOR} ${PV_MISSING} physical volume(s) missing from volume group '${VG_NAME}'"
    exit 1
fi

# Check if logical volume exists
echo -e "${BLUE}[INFO]${NORMAL_COLOR} Checking logical volume: ${LV_NAME}"
if ! lvdisplay "${VG_NAME}/${LV_NAME}" > /dev/null 2>&1; then
    echo -e "${RED}[ERROR]${NORMAL_COLOR} Logical volume '${VG_NAME}/${LV_NAME}' not found"
    echo -e "${YELLOW}[INFO]${NORMAL_COLOR} Available logical volumes in '${VG_NAME}':"
    lvs --noheadings -o lv_name -S vg_name="${VG_NAME}" 2>/dev/null || echo "  None found"
    exit 1
fi

# Ensure logical volume is active (lvchange -ay is idempotent)
echo -e "${BLUE}[INFO]${NORMAL_COLOR} Activating logical volume..."
if ! lvchange -ay "${VG_NAME}/${LV_NAME}" > /dev/null 2>&1; then
    echo -e "${RED}[ERROR]${NORMAL_COLOR} Failed to activate logical volume '${VG_NAME}/${LV_NAME}'"
    exit 1
fi
echo -e "${GREEN}[SUCCESS]${NORMAL_COLOR} Logical volume is active"

# Get device path
LV_PATH="/dev/${VG_NAME}/${LV_NAME}"
if [ ! -b "${LV_PATH}" ]; then
    echo -e "${RED}[ERROR]${NORMAL_COLOR} Logical volume device not found: ${LV_PATH}"
    exit 1
fi

echo -e "${GREEN}[SUCCESS]${NORMAL_COLOR} Logical volume device found: ${LV_PATH}"

# Check if already mounted
if mount | grep -q "${LV_PATH}"; then
    CURRENT_MOUNT=$(mount | grep "${LV_PATH}" | awk '{print $3}')
    echo -e "${YELLOW}[WARN]${NORMAL_COLOR} Logical volume already mounted at: ${CURRENT_MOUNT}"
    if [ "${CURRENT_MOUNT}" = "${MOUNT_POINT}" ]; then
        echo -e "${GREEN}[SUCCESS]${NORMAL_COLOR} Already mounted at correct location"
        exit 0
    else
        echo -e "${RED}[ERROR]${NORMAL_COLOR} Mounted at different location. Please unmount first."
        exit 1
    fi
fi

# Create mount point if it doesn't exist
if [ ! -d "${MOUNT_POINT}" ]; then
    echo -e "${BLUE}[INFO]${NORMAL_COLOR} Creating mount point: ${MOUNT_POINT}"
    mkdir -p "${MOUNT_POINT}"
fi

# Check filesystem on logical volume
echo -e "${BLUE}[INFO]${NORMAL_COLOR} Checking filesystem on logical volume..."
FS_TYPE=$(blkid -o value -s TYPE "${LV_PATH}" 2>/dev/null || echo "unknown")
if [ "${FS_TYPE}" = "unknown" ]; then
    echo -e "${RED}[ERROR]${NORMAL_COLOR} No filesystem detected on ${LV_PATH}"
    echo -e "${YELLOW}[INFO]${NORMAL_COLOR} You may need to create a filesystem first (e.g., mkfs.ext4 ${LV_PATH})"
    exit 1
fi

echo -e "${GREEN}[SUCCESS]${NORMAL_COLOR} Filesystem type: ${FS_TYPE}"

# Mount the logical volume
echo -e "${BLUE}[INFO]${NORMAL_COLOR} Mounting ${LV_PATH} to ${MOUNT_POINT}..."
if mount "${LV_PATH}" "${MOUNT_POINT}"; then
    echo -e "${GREEN}[SUCCESS]${NORMAL_COLOR} Logical volume successfully mounted"
    echo -e "${BLUE}[INFO]${NORMAL_COLOR} Mount details:"
    df -h "${MOUNT_POINT}"
    exit 0
else
    echo -e "${RED}[ERROR]${NORMAL_COLOR} Failed to mount logical volume"
    exit 1
fi
