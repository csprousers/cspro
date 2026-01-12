#!/bin/bash
set -e

# create a working directory
rm -rf temp/codemirror
mkdir -p temp/codemirror
cd temp/codemirror


# find the latest version number here: https://github.com/codemirror/codemirror5/releases/latest/
CODE_MIRROR_VERSION=5.65.18


# get the latest version
TEMP_CSS=./codemirror_tmp.css
curl -f -s https://cdnjs.cloudflare.com/ajax/libs/codemirror/$CODE_MIRROR_VERSION/codemirror.min.css > "$TEMP_CSS"

TEMP_JS=./codemirror_tmp.js
curl -f -s https://cdnjs.cloudflare.com/ajax/libs/codemirror/$CODE_MIRROR_VERSION/codemirror.min.js > "$TEMP_JS"
curl -f -s https://cdnjs.cloudflare.com/ajax/libs/codemirror/$CODE_MIRROR_VERSION/mode/xml/xml.min.js >> "$TEMP_JS"
curl -f -s https://cdnjs.cloudflare.com/ajax/libs/codemirror/$CODE_MIRROR_VERSION/mode/javascript/javascript.min.js >> "$TEMP_JS"
curl -f -s https://cdnjs.cloudflare.com/ajax/libs/codemirror/$CODE_MIRROR_VERSION/mode/css/css.min.js >> "$TEMP_JS"
curl -f -s https://cdnjs.cloudflare.com/ajax/libs/codemirror/$CODE_MIRROR_VERSION/mode/htmlmixed/htmlmixed.min.js >> "$TEMP_JS"


# copy files to be used by CSPro
cp -f "$TEMP_CSS" ../../../../cspro/html/external/codemirror/codemirror.min.css
cp -f "$TEMP_JS" ../../../../cspro/html/external/codemirror/codemirror.min.js


# update the license
curl -f -s -o ../../../Licenses/Licenses/codemirror.txt https://raw.githubusercontent.com/codemirror/codemirror5/refs/tags/$CODE_MIRROR_VERSION/LICENSE
