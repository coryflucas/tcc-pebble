(function(){
  "use strict";

  var BASE_URL = 'https://rs.alarmnet.com/TotalConnectComfort/ws/MobileV2.asmx';
  var APP_ID = 'a0c7a795-ff44-4bcd-9a99-420fac57ff04';
  var APP_VERSION = '2';
  var UI_LANG = 'English';
  var USERNAME_KEY = 'tcc_service.username';
  var PASSWORD_KEY = 'tcc_service.password';

  var sessionId;

  var parseGetSessionResponse = function(response, err, success) {
    var parser = sax.parser(true, { trim: true });

    var tags = [];
    var currentTag = '';
    parser.onopentag = function (node) {
      tags.push(node.name);
      currentTag = node.name;
    };
    parser.onclosetag = function(node) {
      tags.pop();
      currentTag = tags[tags.length - 1];
    };

    var result;
    var sessionId;
    parser.ontext = function (text) {
      switch(currentTag) {
        case 'Result':
          result = text;
          break;
        case 'SessionID':
          sessionId = text;
          break;
      }
    };

    parser.onend = function() {
      if(result == 'Success') {
        success(sessionId);
      } else {
        err(result);
      }
    };

    parser.onerror = function(e) {
      err(e);
    };

    parser.write(response).close();
  };

  var getSession = function(err, success) {
    console.log('getSession');
    var req = new XMLHttpRequest();

    var params = 'username=' + localStorage.getItem(USERNAME_KEY) +
      '&password=' + localStorage.getItem(PASSWORD_KEY) +
      '&applicationID=' + APP_ID +
      '&applicationVersion=' + APP_VERSION +
      '&uiLanguage=' + UI_LANG;

    req.open('POST', BASE_URL + '/AuthenticateUserLogin', true);

    req.onload = function(e) {
      if (req.readyState == 4) {
        if(req.status == 200) {
          parseGetSessionResponse(req.responseText, err, success);
        } else {
          err(req.responseText);
        }
      }
    };

    req.onerror = function() {
      err('ajax error');
    };

    req.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
    req.send(params);
  };

  var LOCATION_INFO = 'GetLocationsResult.Locations.LocationInfo';
  var THERMOSTAT_INFO = LOCATION_INFO + ".Thermostats.ThermostatInfo";
  var parseLocationsResponse = function(response, err, success) {
    var parser = sax.parser(true, { trim: true });

    var tags = [];
    var path = '';
    var result = '';
    var locations = [];
    var location;
    var thermostat;
    parser.onopentag = function (node) {
      tags.push(node.name);
      path = tags.join('.');

      switch(path) {
        case LOCATION_INFO:
          location = {};
          locations.push(location);
          break;
        case LOCATION_INFO + '.Thermostats':
          location.thermostats = [];
          break;
        case THERMOSTAT_INFO:
          thermostat = {};
          location.thermostats.push(thermostat);
          break;
      }
    };

    parser.onclosetag = function(node) {
      tags.pop();
      path = tags.join('.');
    };

    parser.ontext = function (text) {
      switch(path) {
        case 'GetLocationsResult.Result':
          result = text;
          break;
        case LOCATION_INFO + '.LocationID':
          location.id = text;
          break;
        case LOCATION_INFO + '.Name':
          location.name = text;
          break;
        case THERMOSTAT_INFO + '.UserDefinedDeviceName':
          thermostat.name = text;
          break;
        case THERMOSTAT_INFO + '.UI.OutdoorTemp':
          thermostat.outdoor_temp = parseInt(text);
          break;
        case THERMOSTAT_INFO + '.UI.DispTemperature':
          thermostat.indoor_temp = parseInt(text);
          break;
        case THERMOSTAT_INFO + '.UI.HeatSetpoint':
          thermostat.heat_setpoint = parseInt(text);
          break;
        case THERMOSTAT_INFO + '.UI.CoolSetpoint':
          thermostat.cool_setpoint = parseInt(text);
          break;
        case THERMOSTAT_INFO + '.UI.DisplayedUnits':
          thermostat.display_units = text;
          break;
        case THERMOSTAT_INFO + '.UI.OutdoorHumidity':
          thermostat.outdoor_humidity = parseInt(text);
          break;
        case THERMOSTAT_INFO + '.UI.IndoorHumidity':
          thermostat.indoor_humidity = parseInt(text);
          break;
      }
    };

    parser.onend = function() {
      if(result == 'Success') {
        success(locations);
      } else {
        err(result);
      }
    };

    parser.onerror = function(e) {
      err(e);
    };

    parser.write(response).close();
  };

  var getLocations = function(err, success) {
    console.log('getLocations');
    var req = new XMLHttpRequest();

    var params = 'sessionId=' + sessionId;

    req.open('GET', BASE_URL + '/GetLocations?' + params, true);

    req.onload = function(e) {
      if (req.readyState == 4) {
        if(req.status == 200) {
          parseLocationsResponse(req.responseText, err, success);
        } else {
          err(req.responseText);
        }
      }
    };
    req.onerror = function() {
      err('ajax err');
    };

    req.send();
  };

  var handleError = function(error) {
    console.error('Error in JS: ' + error);
  };

  var updateWatch = function(locations) {
    console.log('updateWatch');
    if(locations.length >= 1) {
      var location = locations[0];
      console.log('have location');
      if(location.thermostats.length >= 1) {
        var thermostat = location.thermostats[0];
        var payload = {
          "CURRENT_TEMP": thermostat.indoor_temp,
          "COOL_SETPOINT": thermostat.cool_setpoint,
          "HEAT_SETPOINT": thermostat.heat_setpoint
        };
        Pebble.sendAppMessage(payload);
      }
    }
  };

  var updateThermostat = function() {
    console.log('updating thermostat');
    if(!sessionId)
      getSession(handleError, function(session) {
        sessionId = session;
        getLocations(handleError, updateWatch);
      });
    else
      getLocations(handleError, updateWatch);
  };

  Pebble.addEventListener('ready', function(e) {
    console.log('pebble js started');
    setTimeout(updateThermostat, 1000);
  });

  Pebble.addEventListener("appmessage", function(e) {
    console.log("got appmessage" + e.payload);
    if(e.payload.ACTION == 'refresh') {
      updateThermostat();
    }
  });

  Pebble.addEventListener('showConfiguration', function(e) {
    Pebble.openURL('https://tcc-pebble.corylucas.com/configure.html');
  });

  Pebble.addEventListener('webviewclosed', function(e) {
    console.log('Configuration window returned: ' + e.response);
    var config = JSON.parse(decodeURIComponent(e.response));
    localStorage.setItem(USERNAME_KEY, config.Username);
    localStorage.setItem(PASSWORD_KEY, config.Password);
    updateThermostat();
  });
})();
