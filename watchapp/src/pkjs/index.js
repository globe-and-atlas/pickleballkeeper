// Request phone location only after a completed match is saved.
Pebble.addEventListener('appmessage', function (event) {
  var payload = event.payload || {};
  if (payload.LOC_REQUEST !== 1) return;
  var slot = payload.LOC_SLOT;
  var matchId = payload.LOC_MATCH_ID;

  function report(status, latitude, longitude, accuracy) {
    Pebble.sendAppMessage({
      LOC_SLOT: slot,
      LOC_MATCH_ID: matchId,
      LOC_STATUS: status,
      LOC_LAT_E6: latitude,
      LOC_LON_E6: longitude,
      LOC_ACCURACY_M: accuracy
    });
  }

  navigator.geolocation.getCurrentPosition(function (position) {
    report(1,
      Math.round(position.coords.latitude * 1000000),
      Math.round(position.coords.longitude * 1000000),
      Math.min(65535, Math.max(0, Math.round(position.coords.accuracy))));
  }, function () {
    report(0, 0, 0, 0);
  }, {enableHighAccuracy: true, maximumAge: 30000, timeout: 12000});
});
