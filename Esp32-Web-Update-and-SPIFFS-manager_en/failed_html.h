const char failed_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <title>Update unsuccessful</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body {
        background-color: #f7f7f7;
      }
      #spacer_50 {
        height: 50px;
      }
    </style>
  </head>
  <body>
    <center>
      <h2>The update has failed.</h2>
      <div id="spacer_50"></div>
      <button onclick="window.location.href='/manager';">to homepage</button>
    </center>
  </body>
</html> )rawliteral";