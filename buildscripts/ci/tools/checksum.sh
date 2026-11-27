#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# MuseScore-Studio-CLA-applies
#
# MuseScore Studio
# Music Composition & Notation
#
# Copyright (C) 2021 MuseScore Limited and others
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
echo "Checksum"

ARTIFACTS_DIR=build.artifacts

while [[ "$#" -gt 0 ]]; do
    case $1 in
        -a|--artifact) ARTIFACT_PATH="$2"; shift ;;
        *) echo "Unknown parameter passed: $1"; exit 1 ;;
    esac
    shift
done

ARTIFACT_PATHS=()
if [ -z "$ARTIFACT_PATH" ]; then
    # artifact_name.env holds one file name per line: usually one, but a build may
    # produce several packages (e.g. an installer and an archive on Windows).
    while IFS= read -r ARTIFACT_NAME || [ -n "$ARTIFACT_NAME" ]; do
        ARTIFACT_NAME="${ARTIFACT_NAME%$'\r'}"
        if [ -n "$ARTIFACT_NAME" ]; then ARTIFACT_PATHS+=("$ARTIFACTS_DIR/$ARTIFACT_NAME"); fi
    done < "$ARTIFACTS_DIR/env/artifact_name.env"
    if [ ${#ARTIFACT_PATHS[@]} -eq 0 ]; then echo "error: not set ARTIFACT_NAME"; exit 1; fi
else
    ARTIFACT_PATHS=("$ARTIFACT_PATH")
fi

echo "ARTIFACT_PATH: ${ARTIFACT_PATHS[*]}"

openssl sha256 "${ARTIFACT_PATHS[@]}" > $ARTIFACTS_DIR/checksum.txt