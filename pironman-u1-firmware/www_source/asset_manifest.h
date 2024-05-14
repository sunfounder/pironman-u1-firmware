#pragma once
#include <ESP.h>

static const char asset_manifest[] PROGMEM = R"(
{
  "files": {
    "main.css": "./static/css/main.css",
    "main.js": "./static/js/main.js",
    "index.html": "./index.html",
  },
  "entrypoints": [
    "static/css/main.css",
    "static/js/main.js"
  ]
}
)";