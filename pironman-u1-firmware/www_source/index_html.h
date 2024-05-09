#pragma once
#include <ESP.h>

static const char index_html[] PROGMEM = R"(
<!doctype html>
<html lang="en">
    <head>
        <meta charset="utf-8"/>
        <link rel="icon" href="./favicon.ico"/>
        <meta name="viewport" content="width=device-width,initial-scale=1"/>
        <link rel="stylesheet" href="./index.css">
        <title>Pironman Dashboard</title>
        <script defer="defer" src="./static/js/main.js"></script>
        <link href="./static/css/main.css" rel="stylesheet">
    </head>
    <body>
        <noscript>You need to enable JavaScript to run this app.</noscript>
        <div id="root"></div>
    </body>
</html>
)";