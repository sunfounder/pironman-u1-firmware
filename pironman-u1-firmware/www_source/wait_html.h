#pragma once
#include <ESP.h>

static const char waitHtml[] PROGMEM = R"(
<!DOCTYPE html>
<html lang='en'>
<head>
    <meta charset='UTF-8'>
    <meta http-equiv="refresh" content="3">
<body>
    <h2>Network congestion, please wait ...</h2>
    <p>(Refresh automatically after 3 seconds)</p>
</body>
</html>
)";
