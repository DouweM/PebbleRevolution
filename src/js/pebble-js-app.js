// Open app configuration for PebbleRevolution.
Pebble.addEventListener("showConfiguration",
  function(e) {
    Pebble.openURL("http://evilrobot69.github.io/PebbleRevolution");
  }
);

// Store app configuration.
Pebble.addEventListener("webviewclosed",
  function(e) {
    if (e.response != "") {
      var configuration = JSON.parse(decodeURIComponent(e.response));
      Pebble.sendAppMessage(configuration, null, null);
    }
  }
  );
