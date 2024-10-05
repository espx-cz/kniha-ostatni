const char edit_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <title>Edit file</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body {
        background-color: #f7f7f7;
      }
      #submit {
        width:120px;
      }
      #spacer_50 {
        height: 50px;
      }
      #spacer_20 {
        height: 20px;
      }
      fieldset {
        width:800px;
        background-color: #f7f7f7;
      }
      table {
        background-color: #dddddd;
      }
      td, th {
        text-align: center;
        padding: 15px;
      }
      textarea {
        width: 700px;
        height: 200px;
        padding: 12px 20px;
        box-sizing: border-box;
        border: 2px solid #ccc;
        border-radius: 4px;
        resize: none;
      }
    </style>
    <script>
      function validateForm()
      {
        var allowedExtensions = "%ALLOWED_EXTENSIONS_EDIT%";
        var inputMessage = document.getElementById('save_path').value;
        var dotIndex = inputMessage.lastIndexOf(".")+1;
        var inputMessageExtension = inputMessage.substring(dotIndex);
        var extIndex = allowedExtensions.indexOf(inputMessageExtension);
        var isSlash = inputMessage.substring(0,1);

        if(inputMessage == "")
        {
          alert("Enter the file name! \ne.g.: /new.txt");
          return false;
        }
        if(isSlash != "/")
        {
          alert("The slash at the beginning of the file is missing!");
          return false;
        }
        if(dotIndex == 0)
        {
          alert("The extension is missing at the end of the file!");
          return false;
        }
        if(inputMessageExtension == "")
        {
          alert("The extension is missing at the end of the file!");
          return false;
        }
        if(extIndex == -1)
        {
          alert("Extension not supported!");
          return false;
        }
      }
    </script>
  </head>
  <body>
    <center>
      <h2>Edit file</h2>
      <div id="spacer_20"></div>
      
      <fieldset>
        <legend>Edit text file</legend>
        <div id="spacer_20"></div>
        <table><tr><td colspan="2">
        <form name="edit_file" action="/save" onsubmit="return validateForm()">
          <textarea name="edit_textarea">%TEXTAREA_CONTENT%</textarea>
          <div id="spacer_20"></div>
        </td></tr><tr><td>
          %SAVE_PATH_INPUT%
          <button type "submit" id="submit" >Save</button>
        </form>
        </td><td>
        <button id="submit" onclick="window.location.href='/manager';">Cancel</button>
        </td></tr></table>
        <div id="spacer_50"></div>
      </fieldset>
      
      <iframe style="display:none" name="self_page"></iframe>
    </center>
  </body>
</html> )rawliteral";
