(function(ext) {

    // Setup of the websocket server
    var socket = null;
    var connected = false;

    // an array to hold possible digital input values for the reporter block
    var received_info = null;
    var myStatus = 1; // initially yellow
    var myMsg = 'not_ready';
    
    // Function that callback received_info when it arrives
    wait_callback = function (callback){
        if (received_info == null){
	    setTimeout(function() {
		wait_callback(callback);
		}, 100); // every 100ms, checks for a new received info
	    return;
	}
	else {
	    callback(received_info);
	    return;
	}
    }

    // Sets the  connection to the websocket server
    ext.cnct = function (callback) {
        window.socket = new WebSocket("ws://127.0.0.1:9000");
        window.socket.onopen = function () {
            var msg = JSON.stringify({
                "command": "ready"
            });
            window.socket.send(msg);
            myStatus = 2;
            // change status light from yellow to green
            myMsg = 'ready';
            connected = true;
            // give the connection time establish
            window.setTimeout(function() {
            callback();
            }, 1500);
        };
        window.socket.onmessage = function (message) {
            var msg = JSON.parse(message.data);
            // handle the only reporter message from the server
            // for changes in digital input state
            var reporter = msg['report'];
            if(reporter === 'measure') {
                var result = msg['result'];
                received_info  = result;
            }
	    if (reporter === 'code'){
		var code = msg['number'];
		received_info = code;
	    }
	    if (reporter === 'close'){
	    	socket = null;
                connected = false;
                myStatus = 1;
                myMsg = 'not_ready'
	    }
            console.log(message.data)
        };
        window.socket.onclose = function (e) { // non fonctionnelle?
            console.log("Connection closed.");
            socket = null;
            connected = false;
            myStatus = 1;
            myMsg = 'not_ready'
        };
    };

    // Cleanup function when the extension is unloaded
    ext._shutdown = function() {
	var msg = JSON.stringify({
            "command": "shutdown"
        });
        window.socket.send(msg);
    };

    // Status reporting code
    // Use this to report missing hardware, plugin or unsupported browser
    ext._getStatus = function() {
        return {status: myStatus, msg: myMsg};
    };

    // Open or close the door
    ext.open_close_door = function (order) {
	if (connected == false) {
            alert("Server Not Connected");
        }
	console.log("door");
	var msg = JSON.stringify({
	    "command": 'open_close_door', 'order': order
	});
	console.log(msg);
        window.socket.send(msg);
    }

    
    // Ask for measures of the sensors
    ext.sensor_measure = function (sensor, callback) {
	if (connected == false) {
            alert("Server Not Connected");
        }
	console.log("sensors");
	var msg = JSON.stringify({
	    "command": 'measure', 'sensor': sensor
	});
	console.log(msg);
	received_info = null;
        window.socket.send(msg);
	received_info = null;
	wait_callback(callback);
    }

    // Displays the string to the lcd
    ext.display = function (text_string, floor) {
	if (connected == false) {
            alert("Server Not Connected");
        }
	console.log("text display");
	if (text_string.length > 16){
	    alert("trop de caractères: 16 maximum par ligne")
	}
	else {
	    var msg = JSON.stringify({
		"command": 'display', 'txt': text_string, 'floor': floor
	    });
	    console.log(msg);
	    window.socket.send(msg);
	}
    }

    ext.display_instr = function () {
	if (connected == false) {
            alert("Server Not Connected");
        }
	console.log("instructions display");
	var msg = JSON.stringify({
            "command": 'display_digicode_instructions'
	});
	console.log(msg);
	window.socket.send(msg);
    }
	
    ext.clear_lcd = function () {
	if (connected == false) {
            alert("Server Not Connected");
        }
	console.log("clear lcd");
	var msg = JSON.stringify({
            "command": 'clear_lcd'
	});
	console.log(msg);
	window.socket.send(msg);
    }

    // Asks for the code and return it
    ext.ask_code = function (callback) {
	if (connected == false) {
	    alert("Server Not Connected");
	}
	console.log("asking the code");
	var msg = JSON.stringify({
	    "command": 'ask_code'
	});
	console.log(msg);
	window.socket.send(msg)
	received_info = null;
	wait_callback(callback);
    }

    // Block and block menu descriptions
    var descriptor = {
        blocks: [
	    // Block type, block name, function name
	    ["w", 'connection au serveur sas', 'cnct'],
	    [" ", '%m.ouvrir_fermer la porte', 'open_close_door','ouvrir'],
	    [" ", "affiche %s à l'etage %m.etage de l'ecran", "display", "Bonjour!", "0"],
	    [" ", "affiche instructions du digicode", "display_instr"],
	    [" ", "effacer l'ecran", "clear_lcd"],
	    ["R", "mesure %m.capteur", "sensor_measure", "etat bouton rouge"],
	    ["R", "demande de code", "ask_code"]
        ],
	"menus": {
	    "ouvrir_fermer": ["ouvrir", "fermer"],
	    "capteur": ["etat bouton rouge", "etat plaque de pression", "distance detecteur"],
	    "etage": ["0", "1"]
	}
    };

    // Register the extension
    ScratchExtensions.register('Extension Sas', descriptor, ext);
})({});
