#!/bin/sh
cd "$(dirname "$0")"
# Select host
PS3="Select host: "
select host in $(ls hosts/ | sed 's/\.nix$//'); do
  [[ -z "$host" ]] && echo "Invalid choice." && continue
  break
done
# Ask for password
TEMPORARY_PLAIN_TEXT_PASSWORD_FILE=$(grep "temporaryPlainTextFile" modules/shared.nix -A 10 | grep "default" |  head -n 1 | awk -F '"' '{print $2}' | xargs)
PERMANENT_HASHED_PASSWORD_FILE=$(grep "permanentHashedFile" modules/shared.nix -A 10 | grep "default" |  head -n 1 | awk -F '"' '{print $2}' | xargs)
sudo sudo rm -f "$TEMPORARY_PLAIN_TEXT_PASSWORD_FILE" && mkdir -p "$(dirname "$TEMPORARY_PLAIN_TEXT_PASSWORD_FILE")" &&  sudo touch "$TEMPORARY_PLAIN_TEXT_PASSWORD_FILE"
sudo sudo rm -f "$PERMANENT_HASHED_PASSWORD_FILE" && mkdir -p "$(dirname "$PERMANENT_HASHED_PASSWORD_FILE")" && sudo touch "$PERMANENT_HASHED_PASSWORD_FILE"
while true; do
  read -r -s -p "Set your password: " password
  echo
  read -r -s -p "Confirm your password: " confirm_password
  echo
  if [[ "$password" == "$confirm_password" ]]; then
    echo "$password" > "$TEMPORARY_PLAIN_TEXT_PASSWORD_FILE"
    mkpasswd -m sha-512 "$password" > "$PERMANENT_HASHED_PASSWORD_FILE"
    break
  else
    echo "Passwords do not match. Please try again."
  fi
done
# Select disk
PS3="Select disk: "
select disk in $(lsblk -o PATH,NAME,SIZE,TYPE | grep disk | awk '{print $1}'); do
  [[ -z "$disk" ]] && echo "Invalid choice." && continue
  break
done
# Ask to format disk
until [[ "$configureDisk" =~ ^[yYnN]$ ]]; do
  read -r -p "Configure $disk for formatting? (y/n): " configureDisk
done
if [[ "$configureDisk" == [yY] ]]; then
  echo "Configuring disk for formatting"
  # Copy hosts/$host.nix to /tmp/partition.nix.
  sudo cp modules/partition.nix /tmp/partition.nix
  # Mangle the /tmp/partition.nix, and modify variables, so that it works with disko.
  SWAP_SIZE=$(grep "shared.driveConfig.swapSize" "hosts/$host.nix" | awk -F '"' '{print $2}' | xargs)
  ESP_MOUNT_POINT=$(grep "efiSysMountPoint" modules/shared.nix -A 10 | grep "default" |  head -n 1 | awk -F '"' '{print $2}' | xargs)
  sudo sed -i "s/{ config, ... }:/ /" /tmp/partition.nix # disko calls this file without any parameters, so remove them
  sudo sed -i "s|/dev/disk/by-id/some-disk-id|$disk|" /tmp/partition.nix # replace the disk that will be formatted
  sudo sed -i "s|swap.swapfile.size = config.shared.driveConfig.swapSize;|swap.swapfile.size = \"$SWAP_SIZE\";|g" /tmp/partition.nix # manually set the swap size
  sudo sed -i "s|mountpoint = config.shared.driveConfig.efiSysMountPoint;|mountpoint = \"$ESP_MOUNT_POINT\";|g" /tmp/partition.nix # manually set the ESP mount point
  sudo sed -i "s|passwordFile = config.shared.password.temporaryPlainTextFile;|passwordFile = \"$TEMPORARY_PLAIN_TEXT_PASSWORD_FILE\";|g" /tmp/partition.nix # manyally set the plain text password file
  # Confirm configuration
  until [[ "$format" =~ ^[yYnN]$ ]]; do
    echo
    cat /tmp/partition.nix
    echo
    read -r -p "Confirm you want to format $disk with above configuration? This will erase EVERYTHING on $disk. (y/n): " format
  done
  # Format the disk
  if [[ "$format" =~ ^[yY]$ ]]; then
    sudo nix --extra-experimental-features "nix-command flakes" run github:nix-community/disko/latest -- --mode destroy,format,mount /tmp/partition.nix
    echo "Formatted disk"
  else
    echo "Skipping formatting"
  fi
  sudo rm /tmp/partition.nix
fi
# Confirm installation
until [[ "$install" =~ ^[yYnN]$ ]]; do
  read -r -p "Install nixos? (y/n): " install
done
if [[ "$install" =~ ^[yY]$ ]]; then
  echo "Installing $host to $disk…"
  sudo nix --extra-experimental-features "nix-command flakes" run "github:nix-community/disko/latest#disko-install" -- --flake ".#$host" --mode mount --disk main "$disk" --extra-files /etc/nixos /etc/nixos --mount-point /mnt
fi
# Confirm restart
until [[ "$restart" =~ ^[yYnN]$ ]]; do
  read -r -p "Restart now? (y/n): " restart
done
if [[ "$restart" =~ ^[yY]$ ]]; then
  sudo shutdown -r now
fi
exit 0