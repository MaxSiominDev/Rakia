#!/bin/sh
set -eu

if [ "$(uname)" != "Darwin" ]; then
  echo "install-mac.sh only runs on macOS" >&2
  exit 1
fi

if ! curl -fsSL -o rakia https://github.com/MaxSiominDev/Rakia/releases/latest/download/rakia-macos; then
  echo "failed to download rakia-macos from the latest GitHub release" >&2
  exit 1
fi

chmod +x rakia
./rakia
