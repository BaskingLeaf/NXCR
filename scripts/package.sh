#!/usr/bin/env bash
# NXCR MODIFICATION NOTICE (2026-09 to 2026-11).
# NXCR packaging script evolved from the 2026 recorder prototype.
# Modifications include microSD recording, recording controls, markers,
# configuration, capture recovery and NXCR packaging as applicable.
# Original authors and copyright holders retain their rights.
# See LICENSE, THIRD_PARTY_NOTICES.md and SOURCE_PROVENANCE.md for
# the distribution licence and third-party acknowledgements.
set -euo pipefail
TID=00FF0000A53BB667
rm -rf release
mkdir -p "release/atmosphere/contents/$TID/flags"
mkdir -p "release/switch/.overlays"
mkdir -p "release/switch/NXCR"
cp exefs.nsp "release/atmosphere/contents/$TID/exefs.nsp"
cp toolbox.json "release/atmosphere/contents/$TID/toolbox.json"
printf '.\n' > "release/atmosphere/contents/$TID/flags/boot2.flag"
cp overlay/NXCR.ovl "release/switch/.overlays/NXCR.ovl"
cp default_config.ini "release/switch/NXCR/config.example.ini"
(
  cd release
  zip -qr ../NXCR.zip atmosphere switch
)
echo "Created NXCR.zip"
