var wsUri = "ws://" + location.host + "/d/ws/echo";

var output;

function init()
{
	output = document.getElementById("output");
	console.log( wsUri );
	testWebSocket();
}

function testWebSocket()
{
	websocket = new WebSocket(wsUri);
	websocket.onopen = function(evt) { onOpen(evt) };
	websocket.onclose = function(evt) { onClose(evt) };
	websocket.onmessage = function(evt) { onMessage(evt) };
	websocket.onerror = function(evt) { onError(evt) };
}

function repeatsend()
{
	doSend('Hello!' );
	setTimeout( repeatsend, 1000 );
}

function onOpen(evt)
{
	writeToScreen("CONNECTED");
	doSend('Hello.' );
	//repeatsend();
	//    doSend('{"args": ["entity", 1000], "kwargs": {}, "op": "ClientUpdater__requestUpdates", "seq": 1, "context": ["ClientUpdater", 0]}');
}

function onClose(evt)
{
	writeToScreen("DISCONNECTED");
}

var msg = 0;
var tickmessage = 0;
var lasthz = 0;

function Ticker()
{
	lasthz = msg - tickmessage;
	tickmessage = msg;
	setTimeout( Ticker, 1000 );
}

function onMessage(evt)
{
//	eval( evt.data );

	output.innerHTML = msg++ + " " + lasthz;
	doSend('x' );

	//	obj = JSON.parse(evt.data);
	//	console.log( obj );
	//	writeToScreen('<span style="color: blue;">RESPONSE: ' + evt.data+'</span>');
	//  websocket.close();
}

function onError(evt)
{
	writeToScreen('<span style="color: red;">ERROR:</span> ' + evt.data);
}

function doSend(message)
{
	writeToScreen("SENT: " + message); 
	websocket.send(message);
}

function writeToScreen(message)
{
	var pre = document.createElement("p");
	pre.style.wordWrap = "break-word";
	pre.innerHTML = message;
	output.appendChild(pre);
}

window.addEventListener("load", init, false);

Ticker();

