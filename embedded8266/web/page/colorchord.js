var wsUri = "ws://" + location.host + "/d/ws/issue";
var output;
var websocket;
var commsup = 0;

//Push objects that have:
// .request
// .callback = function( ref (this object), data );

var workqueue = [];
var lastitem;

function QueueOperation( command, callback )
{
	var vp = new Object();
	vp.callback = callback;
	vp.request = command;
	workqueue.push( vp );
}


function init()
{
	$( ".collapsible" ).each(function( index ) {
		if( localStorage["sh" + this.id] > 0.5 )
		{
			$( this ).show().toggleClass( 'opened' );
		}
	});

	$("#custom_command_response").val( "" );

	output = document.getElementById("output");
	Ticker();


	WifiDataTicker();
}

function StartWebSocket()
{
	output.innerHTML = "Connecting...";
	if( websocket ) websocket.close();
	workqueue = [];
	lastitem = null;
	websocket = new WebSocket(wsUri);
	websocket.onopen = function(evt) { onOpen(evt) };
	websocket.onclose = function(evt) { onClose(evt) };
	websocket.onmessage = function(evt) { onMessage(evt) };
	websocket.onerror = function(evt) { onError(evt) };
}

function onOpen(evt)
{
	doSend('e' );
}

function onClose(evt)
{
	$('#SystemStatusClicker').css("color", "red" );
	commsup = 0;
}

var msg = 0;
var tickmessage = 0;
var lasthz = 0;

function Ticker()
{
	lasthz = (msg - tickmessage);
	tickmessage = msg;
	if( lasthz == 0 )
	{
		$('#SystemStatusClicker').css("color", "red" );
		commsup = 0;
		StartWebSocket();
	}

	setTimeout( Ticker, 1000 );
}

function onMessage(evt)
{
	msg++;
	output.innerHTML = "<p>Messages: " + msg + "</p><p>Hz:" + lasthz + "</p>";
	if( commsup != 1 )
	{
		commsup = 1;
		$('#SystemStatusClicker').css("color", "green" );
	}

	if( lastitem && lastitem.callback )
	{
		lastitem.callback( lastitem, evt.data );
		lastitem = null;
	}

	if( workqueue.length )
	{
		var elem = workqueue.shift();
		if( elem.request )
		{
			doSend( elem.request );
			lastitem = elem;
			return;
		}
	}


	doSend('e');
}

function onError(evt)
{
	$('#SystemStatusClicker').css("color", "red" );
	commsup = 0;
}

function doSend(message)
{
	websocket.send(message);
}



function ShowHideEvent( objname )
{
	var obj = $( "#" + objname );
	obj.slideToggle( 'fast' ).toggleClass( 'opened' );
	var opened = obj.is( '.opened' );
	localStorage["sh" + objname] = opened?1:0;
}


function IssueCustomCommand()
{
	QueueOperation( $("#custom_command").val(), function( req,data) { $("#custom_command_response").val( data ); } );
}



window.addEventListener("load", init, false);







///////// Various functions that are not core appear down here.


did_wifi_get_config = false;

function WifiDataTicker()
{
	if( !did_wifi_get_config )
	{
		QueueOperation( "WI", function(req,data)
		{
			var params = data.split( "\t" );
			
			document.wifisection.wifitype.value = Number( params[0].substr(2) );
			document.wifisection.wificurname.value = params[1];
			document.wifisection.wificurpassword.value = params[2];
			document.wifisection.wificurenctype.value = params[3];
			document.wifisection.wificurchannel.value = Number( params[4] );

			did_wifi_get_config = true;
		} );
	}


	QueueOperation( "WR", function(req,data) {
		var lines = data.split( "\n" );
		var innerhtml;
		if( lines.length < 3 )
		{
			innerhtml = "No APs found.  Did you scan?";
		}
		else
		{
			innerhtml = "<TABLE border=1><TR><TH>SSID</TH><TH>MAC</TH><TH>RS</TH><TH>Ch</TH><TH>Enc</TH></TR>"
			for( i = 1; i < lines.length-1; i++ )
			{
				var dats = lines[i].split( "\t" );
				innerhtml += "<TR><TD>" + dats[0] + "</TD><TD>" + dats[1] + "</TD><TD>" + dats[2] + "</TD><TD>" + dats[3] + "</TD><TD>" + dats[4] + "</TD></TR>";
			}
		}
		innerhtml += "</HTML>";
		document.getElementById("WifiStations").innerHTML = innerhtml;
	} );

	setTimeout( WifiDataTicker, 1000 );
}

function ChangeWifiConfig()
{
	
	var st = "W";
	st += document.wifisection.wifitype.value;
	st += ":" + document.wifisection.wificurname.value;
	st += ":" + document.wifisection.wificurpassword.value;
	QueueOperation( st );
	did_wifi_get_config = false;
}


