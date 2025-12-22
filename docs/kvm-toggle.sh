#!/usr/bin/env bash

#  Como usar
#chmod +x ./kvm-toggle.sh
#./kvm-toggle.sh status
#sudo ./kvm-toggle.sh disable   # para usar VirtualBox
#sudo ./kvm-toggle.sh enable    # para voltar ao QEMU+KVM
################################################

set -euo pipefail

# kvm-toggle.sh — enable/disable KVM modules to allow VirtualBox usage
# Usage:
#   sudo ./kvm-toggle.sh enable
#   sudo ./kvm-toggle.sh disable
#   ./kvm-toggle.sh status

need_root() {
  if [[ "${EUID:-$(id -u)}" -ne 0 ]]; then
    echo "Este comando precisa de root. Use: sudo $0 $*"
    exit 1
  fi
}

detect_vendor_module() {
  if grep -qw vmx /proc/cpuinfo; then
    echo "kvm_intel"
  elif grep -qw svm /proc/cpuinfo; then
    echo "kvm_amd"
  else
    echo ""
  fi
}

is_loaded() {
  local mod="$1"
  lsmod | awk '{print $1}' | grep -qx "$mod"
}

stop_services() {
  # Libvirt costuma manter /dev/kvm aberto
  systemctl stop libvirtd 2>/dev/null || true
  systemctl stop virtqemud 2>/dev/null || true
  systemctl stop virtlogd 2>/dev/null || true
  systemctl stop virtstoraged 2>/dev/null || true
}

start_services() {
  # Em alguns Ubuntus é libvirtd, em outros é split (virtqemud etc.)
  systemctl start virtlogd 2>/dev/null || true
  systemctl start virtqemud 2>/dev/null || true
  systemctl start virtstoraged 2>/dev/null || true
  systemctl start libvirtd 2>/dev/null || true
}

cmd_status() {
  local vendor
  vendor="$(detect_vendor_module)"

  echo "KVM status:"
  echo "  /dev/kvm: $([[ -e /dev/kvm ]] && echo "presente" || echo "ausente")"
  echo "  kvm carregado: $([[ "$(is_loaded kvm && echo yes || echo no)" == "yes" ]] && echo "sim" || echo "não")"
  if [[ -n "$vendor" ]]; then
    echo "  ${vendor} carregado: $([[ "$(is_loaded "$vendor" && echo yes || echo no)" == "yes" ]] && echo "sim" || echo "não")"
  else
    echo "  CPU sem vmx/svm detectável (virtualização pode estar desativada na BIOS/UEFI)."
  fi

  echo
  echo "Processos QEMU (se houver):"
  pgrep -a qemu-system 2>/dev/null || echo "  (nenhum)"
}

cmd_disable() {
  need_root disable

  echo "[disable] Parando serviços que podem estar usando KVM..."
  stop_services

  echo "[disable] Encerrando VMs/QEMU que ainda estejam rodando (se houver)..."
  pkill -TERM -f 'qemu-system' 2>/dev/null || true
  sleep 1
  pkill -KILL -f 'qemu-system' 2>/dev/null || true

  local vendor
  vendor="$(detect_vendor_module)"

  echo "[disable] Removendo módulos KVM..."
  # Remove primeiro o específico do vendor
  if [[ -n "$vendor" ]]; then
    modprobe -r "$vendor" 2>/dev/null || true
  fi
  modprobe -r kvm 2>/dev/null || true

  echo "[disable] OK. (Agora o VirtualBox tende a conseguir pegar VT-x/AMD-V.)"
  cmd_status
}

cmd_enable() {
  need_root enable

  local vendor
  vendor="$(detect_vendor_module)"
  if [[ -z "$vendor" ]]; then
    echo "[enable] Não detectei vmx/svm em /proc/cpuinfo."
    echo "Habilite Intel VT-x / AMD SVM na BIOS/UEFI e tente novamente."
    exit 1
  fi

  echo "[enable] Carregando módulos KVM..."
  modprobe kvm
  modprobe "$vendor"

  echo "[enable] Iniciando serviços de virtualização (libvirt/virtqemud)..."
  start_services

  echo "[enable] OK."
  cmd_status
}

main() {
  local action="${1:-status}"
  case "$action" in
    enable)  cmd_enable ;;
    disable) cmd_disable ;;
    status)  cmd_status ;;
    *)
      echo "Uso: $0 {enable|disable|status}"
      exit 2
      ;;
  esac
}

main "$@"

