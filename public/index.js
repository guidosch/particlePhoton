var token;
//var deviceId = '3b003a001147363335343834'; //neighbor Marcel
var deviceId;

var particle = new Particle();

//only used in index.html static version
var username = "guido.schnider@gmail.com";
var password;

var selectedRoom;
var labelMap = new Map();

var deviceMap = new Map();
deviceMap.set("B14_2_21", "270039001447363336383438");
deviceMap.set("B14_2_25", "3e0038000e47363336383437");
deviceMap.set("B14_2_26", "340038001247363336383437");

/** 
labelMap.set("title","Roller automation");
labelMap.set("document-title","Rollers down/up");
labelMap.set("button-open","up");
labelMap.set("button-close","down");
labelMap.set("status-relays","Rollers down?");
*/

labelMap.set("title", "Fenster automation");
labelMap.set("document-title", "Fenster open/close");
labelMap.set("button-open", "open");
labelMap.set("button-close", "close");
labelMap.set("status-relays", "Fenster offen?");

function moveRollers(command) {
  var fnPr = particle.callFunction({ deviceId: deviceId, name: 'callShellysOpenClose', argument: command, auth: token });

  fnPr.then(
    function (data) {
      $(".button").each(function (index) {
        $(this).removeClass("is-loading");
      });
      console.log('Function called successfully:', data);
      fetchValues();
    }, function (err) {
      showNotification("Error calling function 'callShellysOpenClose': " + err);
    });
}

function getVar(varName) {
  particle.getVariable({ deviceId: deviceId, name: varName, auth: token }).then(function (data) {
    console.log('Device variables retrieved successfully:', data);
    try {
      if (data.statusCode == 200) {
        formatData(varName, data);
      }

    } catch (error) {
      showNotification('An error occurred while parsing response data: ' + error);
    }
  }, function (err) {
    showNotification('Device variables cloud not be loaded: ' + err);
  });
}


function formatData(varName, data) {
  let value = data.body.result;
  if (typeof value === "boolean") {
    let natural = value ? "ja" : "nein";
    $("#" + varName).text(natural);
  } else if (typeof value === "number") {
    if (!Number.isInteger(value)) {
      $("#" + varName).text(value.toFixed(1));
    } else {
      $("#" + varName).text(value);
    }
  }
  formatLastConnectionTime(data.body.coreInfo.connected, data.body.coreInfo.last_heard);
}

function formatLastConnectionTime(connected, last_heard) {
  let connected_style = connected ? "green" : "red";
  let connected_label = connected ? "Online" : "Offline";
  let last_heard_formatted = moment(last_heard).format("LLL");
  $("#connected").html(`<span style='color:${connected_style}'>${connected_label}</span>: Zuletzt gesehen um ${last_heard_formatted}`);

}

function showNotification(text) {
  $("#notification_text").text(text);
  $("#notification_text").parent().show();
  console.log(text);
}

function hideNotification() {
  $("#notification_text").parent().hide();
}

function getRoomName(roomId) {
  var roomName = "";
  $("#roomSelect option").each(function () {
    if ($(this).val() === roomId) {
      roomName = $(this).text();
    }
  });
  return roomName;
}

function fetchValues() {
  if (!deviceId) {
    showNotification('Choose room to fetch current measurements and enable actions.');
    return;
  } else {
    hideNotification();
  }
  getVar("roomtemperature");
  getVar("outsidetemperature");
  getVar("humidity");
  getVar("sunshine");
  // getVar("rollersDown");
  getVar("windowOpen");
  getVar("outsidehumidity");
  getVar("precipitation");
  getVar("gustPeak");
}

function setLables() {
  //update labels in table table
  $("*[data-text]").each(function (index) {
    let attrName = $(this).attr("data-text");
    $(this).text(labelMap.get(attrName));
  });
}

function doLogin() {
  particle.login({ username: username, password: password }).then(
    function (data) {
      token = data.body.access_token;
      fetchValues();
    },
    function (err) {
      showNotification('Cloud not login to API: ' + err);
    }
  );
}

$(document).ready(function () {

  $("#close").click(function () {
    $(this).addClass("is-loading");
    moveRollers("close");
  });

  $("#open").click(function () {
    $(this).addClass("is-loading");
    moveRollers("open");
  });

  $(".notification .delete").click(function () {
    $(this).parent().hide();
  });

  //only used in express.js multiroom context
  if (Cookies.get('room') && !selectedRoom) {
    selectedRoom = Cookies.get('room');
    deviceId = deviceMap.get(selectedRoom);
    labelMap.set('document-title', getRoomName(selectedRoom) + ": Fenster open/close");
    setLables();
  }

  //only used in express.js multiroom context
  if (!Cookies.get('room') && !selectedRoom) {
    $("#roomSelector").show();
  }
  //only used in express.js multiroom context
  $("#roomSelect").change(function (event) {
    $("#roomSelect option:selected").each(function () {
      selectedRoom = $(this).val();
    });
    Cookies.set('room', selectedRoom, { expires: 365, path: '' });
    deviceId = deviceMap.get(selectedRoom);
    labelMap.set('document-title', getRoomName(selectedRoom) + ": Fenster open/close");
    $("#roomSelector").hide();
    hideNotification();
    setLables();
    fetchValues();
  });

  if (!Cookies.get('password') && !password && !token) {
    $("#login").show();
  }

  //only used in static index.html version
  if (Cookies.get('password') && !token) {
    password = Cookies.get('password');
    doLogin();
  }

  //only used in static index.html version
  $("#doLogin").click(function () {
    password = $("#password").val().trim();
    Cookies.set('password', password, { expires: 365, path: '' });
    $("#login").hide();
    doLogin();
  });


  // set texts in table/title
  setLables();

  //refresh values every 5 minutes
  setInterval(fetchValues, (5 * 60 * 1000));

});