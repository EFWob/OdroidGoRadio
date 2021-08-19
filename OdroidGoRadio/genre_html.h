// genre.html file in raw data format for PROGMEM
//
#define genre_html_version 1210514
const char genre_html[] PROGMEM = R"=====(<!DOCTYPE html>
<html>
 <head>
  <title>ODROID-GO-Radio</title>
  <meta http-equiv="content-type" content="text/html; charset=ISO-8859-1">
  <link rel="stylesheet" type="text/css" href="radio.css">
  <link rel="Shortcut Icon" type="image/ico" href="favicon.ico">
 </head>
 <body>
  <ul>
   <li><a class="pull-left" href="#">ODROID-GO-Radio</a></li>
   <li><a class="pull-left" href="/index.html">Control</a></li>
   <li><a class="pull-left active" href="/genre.html">Genre</a></li>
   <li><a class="pull-left" href="/config.html">Config</a></li>
   <li><a class="pull-left" href="/mp3play.html">MP3 player</a></li>
   <li><a class="pull-left" href="/about.html">About</a></li>
  </ul>
  <br><br><br>
  <center>
   <div id="genreMain">
   <h1>** ODROID-GO-Radio Genre Manager **</h1>
   <h3> Maintain loaded Genres </h3>
   <div id="genreArea">
      <table id="genreDir" width="800" class="pull-left">
        <tr>
          <td>Loading...</td>
        </tr>
      </table>
   </div>
   <div id="southArea">
      <hr>
          <!--
      <div id="syncInput">
          Or <button class="button" onclick=loadExternalGenres()>Synchronize</button>  
          genres with that other radio with the IP: 
          <input type="text" id="inputOtherRadio" placeholder="192.168.x.y">
      </div>
      <div id="syncRunning" hidden>
          Sync is running....
      </div>
      <hr>
              -->
      <h3> Load stations from Radio Database </h3>
      <div id="apply">
   
      </div>
      <table class="table2" id="filterTable" width="800">
      </table>
      <br><hr><br>
        Or <button class="button" onclick=startEditSettings()>Edit settings here!</button>  
     </div>
   </div>
   <div id="genreSet" hidden>
    <h1>** ESP32 Radio Genre Settings **</h1>
    <table class="pull-left">    
        <tr>
          <td colspan=2 style="text-align:center">
            <hr>
            Functional settings<br><hr>
          </td>
        </tr>
        <tr hidden id="fsNotOpen">
          <td colspan=2 style="text-align:center">
            Directory or Filesystem for Genre Playlists could not be opened.<br>
            So probably something is wrong with the Path-setting below!<hr>
          </td>
        </tr>
    
        <tr>
          <td style="text-align:right">
            IP address of RDBS Server:
          </td>
          <td style="text-align:left">
            <input type="text" id="cfgRdbs">
          </td>
        </tr>
        <!--
        <tr>
          <td style="text-align:right">
            Use SD-Card (not Flash):
          </td>
          <td style="text-align:left">
            <input type="checkbox" id="cfgIsSD">
          </td>
        </tr>
        <tr>
          <td style="text-align:right" 
          title="Path must be given with leading slash '/'.&#013;Preceed with literal 'sd:' to store playlists on SD card.">
            Full path to genre storage directory:
          </td>
          <td style="text-align:left">
            <input type="text" id="cfgPath">
          </td>
        </tr>
        <tr>
          <td style="text-align:right">
            Do not store Station Names:
          </td>
          <td style="text-align:left">
            <input type="checkbox" id="cfgNoname">
          </td>
        </tr>
        -->
        <tr>
          <td style="text-align:right">
            Disable Genres on Odroid API:
          </td>
          <td style="text-align:left">
            <input type="checkbox" id="cfgDisable">
          </td>
        </tr>

        <tr>
          <td colspan=2 style="text-align:center">
            <br>
            <hr>
            Debug support<br><hr>
          </td>
        </tr>

        <tr>
          <td style="text-align:right">
            Verbose output on Serial:
          </td>
          <td style="text-align:left">
            <input type="checkbox" id="cfgVerbose">
          </td>
        </tr>


        <tr>
          <td style="text-align:right">
            Show internal ID-Number:
          </td>
          <td style="text-align:left">
            <input type="checkbox" id="cfgShowID">
          </td>
        </tr>
       <tr>
          <td colspan=2 style="text-align:center">
            <hr>
          </td>
        </tr>
        <tr>
          <td style="text-align:center">
            <button class="button" onclick=cancelEditSettings()>Cancel</button>  
          </td>
          <td style="text-align:center">
            <button class="button" onclick=useEditSettings(0)>Apply</button>  
          </td>
        </tr>
       <tr>
          <td colspan=2 style="text-align:center">
            <hr>Apply and Store permanently.<br>
            <button class="button" onclick=useEditSettings(1)>Store to NVS preferences</button>  

          </td>
        </tr>
      </table>
   </div>
  </center>
 </body>
</html>

<script>

 var dirArr = [];
 var actionArray = [];
 var stationArr = [];
 var genreLoadArr = [];
 var genreLinkList = "";
 var actionRunErrorFlag = false;
 
 var config;

 var validGenres = 0;
 var validStations = 0;
 var xhr ;
 var requestId = 0; 

 var loadIdx = 0;
 var progressAlert;

  function startEditSettings()
  {
    document.getElementById("genreSet").hidden = false;
    document.getElementById("genreMain").hidden = true;
  }

  function locationReload()
  {
    location.reload();
  }

  function setConfig()
  {
    config.rdbs = document.getElementById("cfgRdbs").value;
    //config.path = document.getElementById("cfgPath").value;
    if (document.getElementById("cfgVerbose").checked == true)
      config.verbose = 1;
    else
      config.verbose = 0;
    if (document.getElementById("cfgDisable").checked == true)
      config.disable = 1;
    else
      config.disable = 0;
    //if (document.getElementById("cfgNoname").checked == true)
    //  config.noname = 1;
    //else
      config.noname = 0;
    if (document.getElementById("cfgShowID").checked == true)
      config.showid = 1;
    else
      config.showid = 0;
  }

  function useEditSettings(store)
  {
    if (store)
      if (!confirm("Are you sure you want to save these settings to NVS preferences?"))
        return;
    document.getElementById("genreSet").hidden = true;
    document.getElementById("southArea").hidden = true;
    document.getElementById("genreMain").hidden = false;
    document.getElementById("genreDir").innerHTML = "<tr><td>Loading...</td></tr>";
    setConfig();
    config.save = store;
    sendConfig();
  }

  function cancelEditSettings()
  {
    document.getElementById("genreSet").hidden = true;
    document.getElementById("genreMain").hidden = false;

  }

  function getAction ( actionId, actions )
  {
//    var actions = ["none", "refresh", "delete"];
    var select = document.createElement("select");
    select.name = actionId;
    select.id = actionId;
    for (const val of actions)
        {
            var option = document.createElement("option");
            option.value = val;
            option.text = val.charAt(0).toUpperCase() + val.slice(1);
            select.appendChild(option);
        }
    return select;
  }

  function playGenre(id)
  {
    var xhr = new XMLHttpRequest() ;
    xhr.open ( "GET", "genre?" + id + "&version=" + Math.random() ) ;
    xhr.send() ;
  }

  function showLinksCB (idx, result)
  {
    clearTimeout ( resultTimeout );resultTimeout = null;
    var txt ="The " + dirArr[idx].presets + " stations ";
    //result = atob(result);
    result = decodeUnicode(result);
    result = result.trim();
    if (result.length > 0)
    {
      var resultArr = result.split(",");
      if (resultArr.length > 0) 
      {
        txt = txt +"in cluster '" + dirArr[idx].name + "' are from the following genres:\n\n";
        var i;
        for (i = 1;i <= resultArr.length;i++)
          txt = txt + i + ":" +resultArr[i-1] + "\n";
      }
    }
    else
      txt = txt +"are from genre '" + dirArr[idx].name+"'"; 
    alert (txt);
  }

  function showLinks(idx)
  {
    var theUrl = "link64=" + dirArr[idx].id; // + ",genre=" + dirArr[idx].name ; 
    runActionRequest ( idx, theUrl, 3000, showLinksCB);
  }

 
  function transferExternalGenre(id, newGenres)
  {
    var xhr = new XMLHttpRequest() ;
    var theUrl1 = "createwithlinks,genre=" + newGenres[id - 1].name + '|' + newGenres[id - 1].links;
    newGenres[id - 1].id = id;
    newGenres[id - 1].action = "Refresh";
    //document.getElementById("syncRunning").innerHTML = "genreaction?" + theUrl1 + "&version=" + Math.random();
    var theUrl="genreaction?" + 
    //btoa(theUrl1) 
    encodeUnicode(theUrl1)
    + "&version=" + Math.random();
    xhr.onreadystatechange = function() 
    {
      if ( this.readyState == XMLHttpRequest.DONE )
      {
        if (id < newGenres.length)
        { 
          transferExternalGenre(id + 1, newGenres);
        }
        else
        {  
          //document.getElementById("syncInput").hidden = false;
          document.getElementById("genreArea").hidden = false;
          //document.getElementById("syncRunning").hidden = true;
          dirArr = newGenres;
          runActions(dirArr);
          //listDir( null );
        }   
      }
    }  
  xhr.open ( "GET", theUrl ) ;
  xhr.send() ;
  }

  function transferExternalLinks(id, newGenres)
  {
//    if (newGenres[id - 1].links.length > 0)
    if (false)
    {
      var xhr = new XMLHttpRequest() ;
      var theUrl1 = "createwithlinks,genre=" + newGenres[id - 1].name + '|' + newGenres[id - 1].links;
      var theUrl="genreaction?" + theUrl1 + "&version=" + Math.random();
      //document.getElementById("syncRunning").innerHTML = theUrl;
      theUrl="genreaction?" + 
      //btoa(theUrl1) 
      encodeUnicode(theUrl1)
      + "&version=" + Math.random();
      xhr.onreadystatechange = function() 
      {
        if ( this.readyState == XMLHttpRequest.DONE )
        {
          if (id < newGenres.length) 
            transferExternalLinks(id, newGenres);
          else
          {  
            //document.getElementById("syncInput").hidden = false;
            //document.getElementById("syncRunning").hidden = true;
          }   
        }
      }  
      xhr.open ( "GET", theUrl ) ;
      xhr.send() ;
    }
    else
    {
      transferExternalGenre(id + 1, newGenres);
    }
  }



 function transferExternalGenresStep1(newGenres)
 {

   var theUrl = "/genreformat?version=" + Math.random();
   var xhr = new XMLHttpRequest() ;
   document.getElementById("genreArea").hidden = true;
   //document.getElementById("syncRunning").innerHTML = "Formatting Flash file system...";

   xhr.onreadystatechange = function() 
   {
     if ( this.readyState == XMLHttpRequest.DONE )
     {
       transferExternalGenre(1, newGenres);
     }
    }
  xhr.open ( "GET", theUrl ) ;
  xhr.send() ;
 } 


 function loadExternalGenres() 
 {
   var host = document.getElementById("inputOtherRadio").value.trim();
   //document.getElementById("syncInput").hidden = true;
   //document.getElementById("syncRunning").innerHTML = "Try to load external Genres from host: " + host;
   //document.getElementById("syncRunning").hidden = false;
   var theUrl = "http://" + host + "/genrelist?version=" + Math.random();
   var xhr = new XMLHttpRequest() ;
   xhr.onreadystatechange = function() 
   {
     if ( this.readyState == XMLHttpRequest.DONE )
     {
      //alert(this.responseText); 
      var genreArr = JSON.parse ( 
        //atob (this.responseText) 
        decodeUnicode (this.responseText)
        ) ;
      if (genreArr.length == 0)
      {
        alert ("The host at IP:" + host + " did not return any valid genre information!");
        //document.getElementById("syncInput").hidden = false;
        //document.getElementById("syncRunning").hidden = true;
      }
      else 
      {
        var response = "The radio at IP:" + host + " has " + genreArr.length + " genres loaded.\n" + 
                       "Do you want to transfer those to this radio?\n\n" +
                       "!!WARNING!!\n\n" +
                       "All genres and associated stations on this radio will be gone (replaced)!\n";
        if (confirm( response )) {
            genreArr = genreArr.sort(function(a, b) {
            return b.name < a.name? 1
                   : b.name > a.name? -1
                   :0
          });
          transferExternalGenresStep1(genreArr);
        }
        else
        {
          //document.getElementById("syncInput").hidden = false;
          //document.getElementById("syncRunning").hidden = true;
        }
      }
     }
    }
  xhr.open ( "GET", theUrl ) ;
  //xhr.setRequestHeader("Access-Control-Allow-Origin", "*");
  //xhr.setRequestHeader("Access-Control-Allow-Headers", "*");
  xhr.send() ;
 } 

 function listDir (  )
 {
  var table = document.getElementById('genreDir') ;
  table.innerHTML = "Please wait for the list of stored genres to load...";
  //var theUrl = "genredir";//?version=" + Math.random() ;
  var theUrl = "genredir?version=" + Math.random() ;
  xhr = new XMLHttpRequest() ;
  xhr.onreadystatechange = function() 
  {
    if ( this.readyState == XMLHttpRequest.DONE )
    {
      var table = document.getElementById('genreDir') ;
      table.innerHTML = "" ;
      validGenres = 0;
      validStations = 0;
      var row = table.insertRow() ;
      var cell1 = row.insertCell(0) ;
      dirArr = JSON.parse ( 
        decodeUnicode(this.responseText)
        ) ;
      if (dirArr.length > 0)
      {
        var cell2 = row.insertCell(1) ;
        var cell3 = row.insertCell(2) ;
        cell1.innerHTML = "Genre (click to play)" ;
        cell2.innerHTML = "Stations" ;
        cell3.innerHTML = "Action" ;
      }
      else
      {
        cell1.colSpan = 3 ;
        cell1.style.textAlign = "center";
        cell1.innerHTML = "There are no genre lists stored!";
      }
      //alert (this.responseText);
      //alert (atob(this.responseText));

      var i ;
      dirArr = dirArr.sort(function(a, b) {
            return b.name < a.name? 1
                   : b.name > a.name? -1
                   :0
      });
      for ( i = 0 ; i <= dirArr.length ; i++ )
      {
        if ((i == dirArr.length) && (validGenres > 0))
          {
            var row = table.insertRow() ;
            var cell1 = row.insertCell(0) ;
            var cell2 = row.insertCell(1) ;
            var cell3 = row.insertCell(2) ;
            cell1.innerHTML = "Total:";
            cell2.innerHTML = validStations ;
            var select = getAction("totaldir", ["None", "Refresh"]);
            select.onchange=function() 
            {
                var i;
                for (i = 0;i < dirArr.length;i++)
                {
                    var selector = document.getElementById("action" + i);
                    if (selector != null)
                    {
                      selector.value = this.value;
                      dirArr[i].action = this.value;
                    }
                }
            }
            cell3.appendChild(select);
            row = table.insertRow();
            cell1 = row.insertCell(0) ;
            cell1.colSpan = 3 ;
            cell1.innerHTML = 
             'Press  <button class="button" onclick="runActions(dirArr)">HERE</button> to perform the Actions!' ;

          }  
        else if (dirArr[i].mode == "<valid>")
          {
            var row = table.insertRow() ;
            var cell1 = row.insertCell(0) ;
            var cell2 = row.insertCell(1) ;
            var cell3 = row.insertCell(2) ;
            var txt = '<a onClick="playGenre(' + dirArr[i].id + ')">' + dirArr[i].name;
            if (config.showid)
              txt = txt + ' (id: ' + dirArr[i].id + ')';
            txt = txt + '<a>';
            cell1.innerHTML = txt;
            cell2.innerHTML = '<a onClick="showLinks(' + i + ')">'+dirArr[i].presets +'</a>';
            validGenres++ ;
            validStations = validStations + dirArr[i].presets ;
            var select=getAction("action"+i, ["None", "Refresh", "Delete"]);
            dirArr[i].action = "None" ;
            select.onchange=function() 
            {
                var idx = this.id.substring(6);
 //               alert("Change for " + idx + " to: " + this.value);
                dirArr[idx].action = this.value;
            }

/*            
            var select = document.createElement("select");
            var actions = ["none", "refresh", "delete"];
            select.name = "action" + i;
            select.id = "action" + i;
            for (const val of actions)
              {
                var option = document.createElement("option");
                option.value = val;
                option.text = val.charAt(0).toUpperCase() + val.slice(1);
                select.appendChild(option);
              }
*/
            cell3.appendChild(select);  
          }
      }
    }
  }
  xhr.open ( "GET", theUrl ) ;
  xhr.send() ;
  drawFilterTable();
 }

var lastInGenres;
var lastInPresets;
var lastAdd = "";
var genreArr = [];

function setAddGenre()
{
  var i;
  var newAdd = document.getElementById("inputAdd").value.trim().toLowerCase();
  if (newAdd[0] === newAdd[0].toUpperCase())
  {
    alert("First character must be a letter, reset to " + lastAdd + "!");
    document.getElementById("inputAdd").value = lastAdd;
  }
  else
  {
    newAdd = newAdd.charAt(0).toUpperCase() + newAdd.slice(1);
    document.getElementById("inputAdd").value = newAdd;
  }
  //if (newAdd[0] === newAdd[0].toUpperCase())
  {
    lastAdd = document.getElementById("inputAdd").value;

    for (i = 0;i < genreArr.length;i++)
    {
      var x = document.getElementById("add" + i);
      x.innerHTML = lastAdd;
    }
  }
}

var defaultGenreArr = [];

function loadDefaultGenreArr()
{

  if (defaultGenreArr.length == 0)
    drawFilterTable();
  else
  {
    var genre = defaultGenreArr.shift().trim();
    stationArr = [];
    listStats(0, genre, 5000, loadDefaultGenreArrCB, false);
  }
}


function loadDefaultGenreArrCB(id, genre, loaded, deleteFirst)
{
  clearTimeout ( resultTimeout );resultTimeout = null;
  if (loaded)
  {
    var newEntry = {
      "name" : genre,
      "stationcount" : stationArr.length,
      "action": "None"
    };
    genreArr.push(newEntry);
  }
  loadDefaultGenreArr();
}

function loadDefaultGenresCB(idx, result)
{
  clearTimeout ( resultTimeout );resultTimeout = null;
  result.trim();
  //result = atob(result);
  result = decodeUnicode(result);
  alert(result);
  result = result.trim();
  defaultGenreArr = result.split(",");
  
  if (defaultGenreArr.length > 0)
  {
    alert ("Entries: " + defaultGenreArr.length + "= " + result);
    genreArr = [] ;
    loadDefaultGenreArr();
  }
}

function loadDefaultGenres()
{
  if (confirm("Load default '$$genres' from preferences?"))
  {
    var theUrl = "nvsgenres" ; 
    runActionRequest ( 0, theUrl, 3000, loadDefaultGenresCB);
  }
}

function drawFilterTable()
{
    var table =  document.getElementById("filterTable");
      table.innerHTML = ""; 
      var row = table.insertRow() ;
      var cell1 = row.insertCell(0) ;
      var cell2 = row.insertCell(1) ;
      var cell3 = row.insertCell(2) ;
      var cell4 = row.insertCell(3) ;
      cell1.innerHTML = "Genre" ;
      cell2.innerHTML = "Stations" ;
      cell3.innerHTML = "Action" ;
      cell4.innerHTML = "Add to:"

      var row = table.insertRow() ;
      row.id="applyGenreFilter" ;
      cell1 = row.insertCell(0) ;
      cell2 = row.insertCell(1) ;
      cell3 = row.insertCell(2) ;
      cell4 = row.insertCell(3) ;

      cell1.innerHTML = '<input type="text" id="inputGenre" placeholder="Enter substring here">' ;
      cell2.innerHTML = '<input type="number" id="inputPresets" placeholder="Minimum">' ;
      cell3.innerHTML = '<button class="button" onclick=getConfig(loadGenres)>Apply Filter</button>' ;
//      cell4.innerHTML = '<input type="text" id="inputAdd" value="' + lastAdd + '" onchange="setAddGenre()">' ;
      cell4.innerHTML = '<input type="text" id="inputAdd" value="' + lastAdd + '" placeholder="Clustername" onchange="setAddGenre()">' ;
      var idx;
      for (idx = 0; idx < genreArr.length;idx++)
      {
        if (genreArr[idx].stationcount < lastInPresets)
          break;
        var row = table.insertRow() ;
        if (0 == idx)
        {
          var cell1 = row.insertCell(0);
          cell1.colSpan = 3;
          cell1.innerHTML = "<HR>" ;
          row = table.insertRow() ;
        }
        var cell1 = row.insertCell(0) ;
        var cell2 = row.insertCell(1) ;
        var cell3 = row.insertCell(2) ;
        var cell4 = row.insertCell(3) ;
        var x = document.createElement("DIV");
        x.id = "add" + idx;
        x.hidden = true;
        x.innerHTML = lastAdd;
        cell4.appendChild(x);
        cell1.innerHTML = genreArr[idx].name ;
        cell2.innerHTML = genreArr[idx].stationcount ;
        var select=getAction("load"+idx, ["None", "Load", "Add to:"]);
        genreArr[idx].action = "None" ;
        select.onchange=function() 
        {
            var idx = this.id.substring(4);
 //               alert("Change for " + idx + " to: " + this.value);
            genreArr[idx].action = this.value;
            if (this.value == "Add to:")
            {
              document.getElementById("add"+idx).hidden = false;
            }
            else
            {
              document.getElementById("add"+idx).hidden = true;
            }
        }
        cell3.appendChild(select);
      }
      if (idx > 0)  
      {
        if (idx > 1)
        {
        var row = table.insertRow() ;
        var cell1 = row.insertCell(0);
        cell1.colSpan = 2;
        cell1.innerHTML="Select action for all";
        cell1 = row.insertCell(1);
        var select=getAction("loadall", ["None", "Load", "Add to:"]);
        select.onchange=function() 
            {
                var i;
                for (i = 0;i < genreArr.length;i++)
                {
                    var selector = document.getElementById("load" + i);
                    if (selector != null)
                    {
                      selector.value = this.value;
                      genreArr[i].action = this.value;
                      if (this.value == "Add to:")
                      {
                        document.getElementById("add"+i).hidden = false;
                      }
                      else
                      {
                        document.getElementById("add"+i).hidden = true;
                      }
                    }
                }
            }
        cell1.appendChild(select);
        window.scrollTo(0,document.body.scrollHeight);

        }
        var row = table.insertRow() ;
        var cell1 = row.insertCell(0);
        cell1.colSpan = 3;
        cell1.innerHTML = "<HR>" ;
        var row = table.insertRow() ;
        var cell1 = row.insertCell();
        cell1.colSpan = 3;
        cell1.innerHTML = 
             'Press  <button class="button" onclick="runActions(genreArr)">HERE</button> to perform the Actions!' ;
      window.scrollTo(0,document.body.scrollHeight);
      }
    if (!(typeof lastInGenres === 'undefined'))
      document.getElementById("inputGenre").value = lastInGenres;
    document.getElementById("inputPresets").value = lastInPresets;

}

function loadGenres()
{
  lastAdd = document.getElementById("inputAdd").value;
  lastInGenres = document.getElementById("inputGenre").value;
  
  if (typeof lastInGenres === 'undefined')
    lastInGenres = "";
  lastInPresets = document.getElementById("inputPresets").value;
  if (typeof lastInPresets === 'undefined')
    lastInPresets = "";
  
  document.getElementById("applyGenreFilter").innerHTML = "Please wait!";
  loadGenresFromRDBS(lastInGenres, lastInPresets);
}

function loadGenresFromRDBS(genreMatch, stations)
{
  var theUrl = "https://" + config.rdbs + "/json/tags/" +
               genreMatch +
               "?hidebroken=true&order=stationcount&reverse=true" ;
  xhr = new XMLHttpRequest() ;
  stationArr = [];
  xhr.onreadystatechange = function() 
  {
   if ( this.readyState == XMLHttpRequest.DONE )
   {
    genreArr = JSON.parse ( this.responseText ) ;
    clearTimeout ( resultTimeout );resultTimeout = null;
    drawFilterTable();
   }
  }
  xhr.open ( "GET", theUrl ) ;
  resultTimeout = setTimeout(function() {
      alert("Error: connect to RDBS failed!");
      drawFilterTable();
    } , 5000);
  xhr.send() ;

}

 var resultContainer = null;
 var resultTimeout = null; 
 function actionDone(id, result)
 {
     clearTimeout ( resultTimeout );resultTimeout = null;
     if (result.substring(0, 2) == "OK")
     {
        if (resultContainer != null)
          if (result.substring(2,3) == "=")
            resultContainer.innerHTML = result.substring(3) ;
          else
            resultContainer.innerHTML = "OK";
         runAction(id + 1);
     }
     else
     {
        if (resultContainer != null)
            resultContainer.innerHTML = "ERROR!" ;
         alert("Action for " + id + " failed with result: " + result + ". Action run will be stopped!");
         runAction(actionArray.length);
     }
 }   

 function showAction (description)
 {
    var table = document.getElementById('genreDir') ;
    var row = table.insertRow() ;
    var cell1 = row.insertCell(0) ;
    var cell2 = row.insertCell(1) ;
    cell1.innerHTML = description;
    resultContainer = cell2;
    cell2.innerHTML = "..busy..";
    window.scrollTo(0,document.body.scrollHeight);
 }
 
 function uploadStatLinkListCB(id, result)
 {
    if (genreLoadArr.length == 0)
      actionDone(id, "OK");
    else
    {
      var i;
      var links = "";
      var l = genreLoadArr.length;
      if (l > 10)
        l = 10;
      for (i = 0;i < l;)
      {
        links = links + genreLoadArr.shift();
        i++;
        if (i < l)
          links = links + ",";
      }
      runActionRequest(id, "link=+" + actionArray[id].id + 
                        ",genre=" + actionArray[id].name +
                         ",list=" + links, 
                        3000, uploadStatLinkListCB);

    }
 }

 function uploadStatChunk(id)
 {
   resultContainer.innerHTML = actionArray[id].subIdx + " stations";
   if (actionArray[id].subIdx >= stationArr.length)
   {
     if (genreLinkList.length == 0) 
      actionDone(id, "OK") ;//=" + actionArray[id].subIdx + " stations");
     else
     {
        genreLoadArr = genreLinkList.split(",");
        //Workaround: Following call will result in an empty link list..
        runActionRequest(id, "link=" + actionArray[id].id + 
                          ",genre=" + actionArray[id].name +
                           ",list=", 
                          3000, uploadStatLinkListCB);
        genreLinkList = "";
     }
   }
   else
   {
      var delta = 10;
      var stations = "";
      var l = stationArr.length - actionArray[id].subIdx;
      if (l > delta)
        l = delta;
      for(i = 0;i < l;i++) 
        {
        //stations=stations + "%7c" + stationArr[i + actionArray[id].subIdx].url_resolved.substring(7); 
        stations=stations + "|" + stationArr[i + actionArray[id].subIdx].url_resolved.substring(7); 
        if (!config.noname)
          stations = stations + "#" + stationArr[i + actionArray[id].subIdx].name;
        }
      //stations = stations.replaceAll("&", "%26");
      //showAction("Next chunk, idx=" + actionArray[id].subIdx + ", l=" + l);
      if (loadIdx > 0)
        delta = 0;
      else
        delta = 1;
      if (actionArray[id].action == "Add to:" )
      {
        //alert("Hier kommt jetzt ein Add to: " + lastAdd + " für genre: " + actionArray[id].name);
        runActionRequest(id, "add=" + actionArray[id].name + 
                          ",genre=" + lastAdd +
                          ",count=" + l +
                          ",idx=" + actionArray[id].subIdx +
                          ",start=" + delta +
                            stations,
                            10000, uploadStatChunkCB);
      }
      else
      {
        //alert(stations); 
        runActionRequest(id, "save=" + actionArray[id].id + 
                          ",genre=" + actionArray[id].name +
                          ",count=" + l +
                          ",idx=" + actionArray[id].subIdx +
                          ",start=" + delta +
                          stations,
                          10000, uploadStatChunkCB);
      }
    actionArray[id].subIdx += l;
    loadIdx += l;
   }
 }

 function uploadStatChunkCB (id, result)
 {
   clearTimeout ( resultTimeout );resultTimeout = null;
   if (result.substr(0,2) == "OK")
   {
    uploadStatChunk(id);
   }
   else
   {
     resultContainer.innerHTML = "ERROR" ;
     showAction(result);
     resultContainer.innerHTML = "skip" ;
     actionRunErrorFlag = true;
     actionDone (id, "OK" ) ;
   }
 }

 function listStatsDelCB ( id, result)
 {
   clearTimeout ( resultTimeout );resultTimeout = null;
   if (result.substring(0, 2) == "OK")
   {
      resultContainer.innerHTML = "OK"
      showAction ("Uploading " + stationArr.length + " stations to radio.");
      resultContainer.innerHTML = "0 stations";
      actionArray[id].subIdx = 0;
      uploadStatChunk(id);
    }
    else
      actionDone(id, "Radio reports error on deleting genre " + actionArray[id].name);
 }

 function listStatsCB ( id, genre, loaded, deleteFirst)
 {
    clearTimeout ( resultTimeout );resultTimeout = null;
    if (loaded && (stationArr.length > 0))
    {
      resultContainer.innerHTML = stationArr.length + " stations";
      /*
      if (deleteFirst)
      {
        showAction(`First deleting genre '${genre}'`); 
        runActionRequest( id, "del=" + actionArray[id].id + ",genre=" + 
              actionArray[id].name, 0, listStatsDelCB) ;
      }
      else
      */
      {         
        showAction ("Uploading " + stationArr.length + " stations to radio.");
        resultContainer.innerHTML = "0 stations";
        actionArray[id].subIdx = 0;
        uploadStatChunk(id);
      }
    } 
    else if (loaded)
    {
      resultContainer = "ERROR";
      showAction ("RDBS returned 0 stations for genre '" + genre + "'.");
      actionRunErrorFlag = true ;
      actionDone(id, "OK");
    }
    else
    {
      actionDone(id, "Connect to RDBS timed out.");
    }
 }



function runActionRequest (id, theUrl, timeout, callback)
{   
  xhr = new XMLHttpRequest() ;
  //xhr.setRequestHeader("Cache-Control", "no-cache, no-store, max-age=0");
 // xhr.setRequestHeader("Cache-Control",  "no-cache, no-store, must-revalidate");
  theUrl="genreaction?" + 
  //btoa(theUrl)
  encodeUnicode(theUrl)
   + "&version=" + Math.random();
  xhr.onreadystatechange = function() 

  {
    if ( this.readyState == XMLHttpRequest.DONE )
    {
      callback ( id, this.responseText.trim() ) ;
    }
  }  
  clearTimeout(resultTimeout);resultTimeout = null;
  timeout=0;
  if (timeout)
    if (id > 0)
      resultTimeout = setTimeout(actionDone, timeout, id, `ERR: Timeout for URL: ${theUrl}`);
    else 
      resultTimeout = setTimeout(callback, id, `ERR: Timeout for URL: ${theUrl}`);
  xhr.open ( "GET", theUrl ) ;
  xhr.send() ;
  
}


 function doDelete( idx )
 {
     resultTimeout = setTimeout( actionDone, 2000, idx, "OK: Delete but Timeout!") ;
 }

  function reloadOneGenre(idx)
  {
    if (genreLoadArr.length == 0)
    {
      //alert("Das wars von der GenreListe!");
      if (stationArr.length > 0)
        listStatsCB(idx, actionArray[idx].name, true, false);
      else
        actionDone(idx, "OK");
    }
    else
    {
      var loadGenre = genreLoadArr.shift();
      if (loadGenre == actionArray[idx].name)
      {
        stationArr = [];    
      }
      showAction(`Reload stations from genre '${loadGenre}'`)
      listStats(idx, loadGenre, 5000, reloadOneGenreCB, false);
    }
  }

  function reloadOneGenreCB(idx, genre, loaded, deleteFirst)
  {
    clearTimeout ( resultTimeout );resultTimeout = null;
    resultContainer.innerHTML = stationArr.length ;
    reloadOneGenre(idx);
  }

  function reloadPresetsLinkCB(idx, result)            
  {
    clearTimeout ( resultTimeout );resultTimeout = null;
    resultContainer.innerHTML = "started";
    //result = atob(result);
    result = decodeUnicode(result);
    if (result.startsWith("ERR"))
    {
      resultContainer.innerHTML = "OK";
      showAction(`Error getting linked genres for '${actionArray[idx].name}'`); 
      resultContainer.innerHTML = "skipping";
      result = "";
    }
    result = result.trim();
    genreLinkList = result; 
//    if (result.length > 0)
//      result = actionArray[idx].name + "," + result;
//    else
    if (result.length == 0)
      result = actionArray[idx].name;
    genreLoadArr = result.split(",");
    //alert(genreLinkList + "Length of genreLoadArr: " + genreLoadArr.length);
    reloadOneGenre(idx);
  }

  function sendConfig()
  {
    var theUrl = "setconfig=" + JSON.stringify(config);
    xhr = new XMLHttpRequest() ;
    theUrl="genreaction?" + 
      encodeUnicode(theUrl)
      + "&version=" + Math.random();
    xhr.onreadystatechange = function() 
    {
      if ( this.readyState == XMLHttpRequest.DONE )
      {
        location.reload();
      }
    }  
    xhr.open ( "GET", theUrl ) ;
    xhr.send() ;  
  }

  function reloadPresets(idx)
  {
    var theUrl = "link64=" + actionArray[idx].id +",genre=" + actionArray[idx].name ; 
    runActionRequest ( idx, theUrl, 3000, reloadPresetsLinkCB);
  }

 function runAction( idx )
 {
//      alert(idx + ":" + actionArray[idx].action + "(" + actionArray[idx].name + ")");

     clearTimeout (resultTimeout );
     resultTimeout = null ;
     if (idx < actionArray.length)
        while ((idx < actionArray.length) && 
          ((actionArray[idx].action == "None") || (actionArray[idx].action == undefined)))
            idx++;
     if ( idx >= actionArray.length)
     {
        if (actionRunErrorFlag)
          alert("There have been errors while running the actions!");
        else
          ;//alert("ActionRunComplete");
        //document.getElementById("southArea").hidden = true;
        location.reload();
     }
     else
     {
        stationArr = [] ;     //Workaround

         //alert(idx + ":" + actionArray[idx].action + "(" + actionArray[idx].name + ")");
         if ( actionArray[idx].action == "Delete")
         {
            //listDir();
            //alert("Just running listDir....");
            //alert(`Deleting genre '${actionArray[idx].name}'...`);
            showAction(`Deleting genre '${actionArray[idx].name}'`); 
            runActionRequest( idx, "del=" + actionArray[idx].id + ",genre=" + 
              actionArray[idx].name, 0, actionDone) ;
         }
         else if ( actionArray[idx].action == "Refresh")
         {
            showAction(`Reload genre '${actionArray[idx].name}'`);
            loadIdx = 0;
            reloadPresets(idx);
         }
         else if ( actionArray[idx].action == "Load")
         {
            showAction(`Load genre '${actionArray[idx].name}'`);
            loadIdx = 0;
            listStats(idx, actionArray[idx].name, 5000, listStatsCB, false) ;
         }
         else if ( actionArray[idx].action == "Add to:" )
         {
            showAction(`Adding stations from genre '${actionArray[idx].name}' to genre '${lastAdd}'.`);
            listStats(idx, actionArray[idx].name, 5000, listStatsCB, false) ;
         }
         else
         {
            resultContainer = null;
            actionDone(idx, `Error: unknown action "${actionArray[idx].action}" for genre '${actionArray[idx].name}'`);
        }
     }
 }

 function startActionRunCB()
 {
   runAction(0);
 }

 function runActions(newActions)
 {
    if (actionArray.length > 0)
    {
      alert("There are already ongoing actions. Please wait and retry!");
      return;
    }  
    if (newActions.length == 0)
    {
      alert("There is nothing to do!");
      return;
    }  
    document.getElementById("southArea").hidden = true;
    actionArray = newActions;
    actionRunErrorFlag = false;
    //alert("ActionArray length= " + actionArray.length);
    var table = document.getElementById('genreDir') ;
    table.innerHTML = "" ;
    var row = table.insertRow() ;
    var cell1 = row.insertCell(0) ;
    cell1.colSpan = 2;
    cell1.innerHTML = "Please wait for page to reload. Do not load any other page now!"; 
    loadIdx = 0;
    getConfig(startActionRunCB);
    //runAction (0) ;
 }


 // Get info from a radiobrowser.  Working servers are:
 // https://de1.api.radio-browser.info, https://fr1.api.radio-browser.info, https://nl1.api.radio-browser.info
 // id: reference to be passed on to callback
 // genre: genre to load
 // timeout: in ms for response from RDBS
 // callback: to be called on either timeout (empty stationArr) or success (filled stationArr)
 function listStats ( id, genre, timeout, callback, deleteFirst )
 {
  var theUrl = "https://" + config.rdbs + "/json/stations/search" ;
  //alert(theUrl);
  xhr = new XMLHttpRequest() ;
  //stationArr = [];
  xhr.onreadystatechange = function() 
  {
   if ( this.readyState == XMLHttpRequest.DONE )
   {
    //var table = document.getElementById('stationsTable') ;
    //table.innerHTML = "" ;
    var thisStationArr = JSON.parse ( this.responseText ) ;
    var i ;
    var snam ;
    var oldsnam = "" ;
    for ( i = 0 ; i < thisStationArr.length ; i++ )
    {
      snam = thisStationArr[i].name ;
      if ( thisStationArr[i].url_resolved.startsWith ( "http:") &&            // https: not supported yet
           snam != oldsnam )
      {
        oldsnam = snam ;
      }
      else
      {
        thisStationArr.splice(i, 1);
        i--;
      }
    }
    stationArr = stationArr.concat(thisStationArr);
    callback(id, genre, true, deleteFirst);
   }
  }
  xhr.open ( "POST", theUrl ) ;
  xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  xhr.send("tag=" + genre + "&tagExact=true&hidebroken=true");
  resultTimeout = setTimeout(callback, timeout, id, genre, false, deleteFirst);
 }



 function encodeUnicode(str) {
  // first we use encodeURIComponent to get percent-encoded UTF-8,
  // then we convert the percent encodings into raw bytes which
  // can be fed into btoa.
  return btoa(encodeURIComponent(str).replace(/%([0-9A-F]{2})/g,
      function toSolidBytes(match, p1) {
          return String.fromCharCode('0x' + p1);
  }));
}

function decodeUnicode(str) {
  //return atob(str);

  // Going backwards: from bytestream, to percent-encoding, to original string.
  return decodeURIComponent(atob(str).split('').map(function (c) {
    return '%' + ('00' + c.charCodeAt(0).toString(16)).slice(-2);
  }).join(''));
}

 function setStation ( inx )
 {
  var table = document.getElementById('stationsTable') ;
  var snam ;
  var theUrl ;

  snam = stationArr[inx].url_resolved ;
  snam = snam.substr ( 7 ) ;
  theUrl = "/?station=" + snam + "&version=" + Math.random() ;
  //table.rows[inx].cells[0].style.backgroundColor = "#333333" 
  xhr = new XMLHttpRequest() ;
  xhr.onreadystatechange = function() 
  {
   if ( this.readyState == XMLHttpRequest.DONE )
   {
     resultstr.value = xhr.responseText ;
   }
  }
  xhr.open ( "GET", theUrl ) ;
  xhr.send() ;
 }
   
  function getConfig(callback)
  {
    var theUrl="/genreconfig&version=" + Math.random();
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() 
    {
      if ( this.readyState == XMLHttpRequest.DONE )
      {
      config = JSON.parse ( this.responseText ) ;
      config.store = false;
      var path = config.path;
      document.getElementById("cfgRdbs").value = config.rdbs;
      //document.getElementById("cfgIsSD").checked = config.isSD;
      //if (config.isSD)
        //path = "sd:" + path;
      //document.getElementById("cfgPath").value = path;
      //document.getElementById("cfgNoname").checked = config.noname;
      document.getElementById("cfgShowID").checked = config.showid;      
      document.getElementById("cfgVerbose").checked = config.verbose;      
      document.getElementById("cfgDisable").checked = config.disable;      
      document.getElementById("fsNotOpen").hidden = (config.open != 0);
      callback();
/*
      if (actionArray.length == 0)
        listDir ( null );
      else
        runAction ( 0 );
*/
      }
    }
  xhr.open ( "GET", theUrl ) ;
  xhr.send() ;
  }

   getConfig(listDir);
   //listDir( null ) ;
</script>

)=====" ;
