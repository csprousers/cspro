#!/bin/bash
set -e

CODE_MIRROR_VERSION=5.62.3
TEMP_CSS=./codemirror_tmp.css
curl -f -s https://cdnjs.cloudflare.com/ajax/libs/codemirror/$CODE_MIRROR_VERSION/codemirror.min.css > "$TEMP_CSS"

TEMP_JS=./codemirror_tmp.js
curl -f -s https://cdnjs.cloudflare.com/ajax/libs/codemirror/$CODE_MIRROR_VERSION/codemirror.min.js > "$TEMP_JS"
curl -f -s https://cdnjs.cloudflare.com/ajax/libs/codemirror/$CODE_MIRROR_VERSION/mode/xml/xml.min.js >> "$TEMP_JS"
curl -f -s https://cdnjs.cloudflare.com/ajax/libs/codemirror/$CODE_MIRROR_VERSION/mode/javascript/javascript.min.js >> "$TEMP_JS"
curl -f -s https://cdnjs.cloudflare.com/ajax/libs/codemirror/$CODE_MIRROR_VERSION/mode/css/css.min.js >> "$TEMP_JS"
curl -f -s https://cdnjs.cloudflare.com/ajax/libs/codemirror/$CODE_MIRROR_VERSION/mode/htmlmixed/htmlmixed.min.js >> "$TEMP_JS"

mv "$TEMP_CSS" ./codemirror.min.css
mv "$TEMP_JS" ./codemirror.min.js
