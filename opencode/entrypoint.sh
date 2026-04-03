#!/usr/bin/env bash
set -e
umask 002

setup_user() {
  if [ -n "$GID" ] && ! getent group "$GID" > /dev/null 2>&1; then
    addgroup -g "$GID" user
    echo "** Group 'user' created with GID ${GID}"
  fi

  if [ -n "$UID" ] && ! getent passwd "$UID" > /dev/null 2>&1; then
    local group_name
    group_name=$(getent group "$GID" | cut -d: -f1)
    adduser -D --uid "${UID}" -G "${group_name:-user}" "user"
    echo "** User 'user' created with UID ${UID}"
  fi
}

configure_shell() {
  local user="$1"
  local bashrc="/home/${user}/.bashrc"

  {
    echo "PS1='\\[\\e[38;5;5m\\]\\[\\e[1m\\](opencode)\\[\\e[m\\] \\[\\e[34m\\]\\[\\e[1m\\]\\W\\[\\e[m\\] \\$ \\033[0m'"
    echo "alias ll='ls -al'"
  } >> "$bashrc"
}

setup_user
USER=$(getent passwd "${UID}" | cut -d: -f1)
GROUP=$(getent group "${GID}" | cut -d: -f1)

addgroup "$USER" "$GROUP" 2>/dev/null || true
configure_shell "$USER"

su -l "$USER" -s /bin/bash -c " \
  cd $PWD || exit 1
  PATH=$PATH:/workspace
  exec $*
"
