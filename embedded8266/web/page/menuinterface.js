//Copyright (C) 2015 <>< Charles Lohr, see LICENSE file for more info.
//
//This particular file may be licensed under the MIT/x11, New BSD or ColorChord Licenses.
var wsUri = "ws://" + location.host + "/d/ws/issue";
var output;
var websocket;
var commsup = 0;

//Push objects that have:
// .request
// .callback = function( ref (this object), data );

var workqueue = [];
var wifilines = [];
var workarray = {};
var lastitem;

var SystemMessageTimeout = null;
function IssueSystemMessage( msg )
{
	var elem = $( "#SystemMessage" );
	elem.hide();
	elem.html(  "<font size=+2>" + msg + "</font>" );
	elem.slideToggle( 'fast' );
	if( SystemMessageTimeout != null ) clearTimeout(SystemMessageTimeout);
	SystemMessageTimeout = setTimeout( function() { SystemMessageTimeout = null; $( "#SystemMessage" ).fadeOut( 'slow' ) }, 3000 );
}

function QueueOperation( command, callback )
{
	if( workarray[command] == 1 )
	{
		return;
	}

	workarray[command] = 1;
	var vp = new Object();
	vp.callback = callback;
	vp.request = command;
	workqueue.push( vp );
}


function init()
{
	$('#MainMenu > tbody:first-child').before( "\
		<tr><td width=1> \
		<input type=submit onclick=\"ShowHideEvent( 'SystemStatus' );\" value='System Status' id=SystemStatusClicker></td><td> \
		<div id=SystemStatus class='collapsible'> \
		<table width=100% border=1><tr><td> \
<div id=output> \n		</td></tr></table></div></td></tr>" );

	$('#MainMenu > tbody:last-child').after( "\
		<tr><td width=1> \
		<input type=submit onclick=\"ShowHideEvent( 'WifiSettings' ); KickWifiTicker();\" value=\"Wifi Settings\"></td><td> \
		<div id=WifiSettings class=\"collapsible\"> \
		<table width=100% border=1><tr><td> \
		Current Configuration: (May deviate from default configuration, reset here if in doubt)<form name=\"wifisection\" action=\"javascript:ChangeWifiConfig();\"> \
		<table border=1 width=1%> \
		<tr><td width=1>Type:</td><td><input type=\"radio\" name=\"wifitype\" value=1 onclick=\"ClickOpmode(1);\">Station (Connect to infrastructure)<br><input type=\"radio\" name=\"wifitype\" value=2 onclick=\"ClickOpmode(2);\">AP (Broadcast a new AP)</td></tr> \
		<tr><td>SSID:</td><td><input type=\"text\" id=\"wificurname\"></td></tr> \
		<tr><td>PASS:</td><td><input type=\"text\" id=\"wificurpassword\"></td></tr> \
		<tr><td>MAC:</td><td><input type=\"text\" id=\"wifimac\"> (Ignored in softAP mode)</td></tr> \
		<tr><td>Chan:</td><td><input type=\"text\" id=\"wificurchannel\"> (Ignored in Station mode)</td></tr></tr> \
		<tr><td></td><td><input type=submit value=\"Change Settings\"> (Automatically saves to flash)</td></tr> \
		</table></form> \
		Scanned Stations: \
		<div id=WifiStations></div> \
		<input type=submit onclick=\"ScanForWifi();\" value=\"Scan For Stations (Will disconnect!)\"> \
		</td></tr></table></div></td></tr> \
		 \
		<tr><td width=1> \
		<input type=submit onclick=\"ShowHideEvent( 'CustomCommand' );\" value=\"Custom Command\"></td><td> \
		<div id=CustomCommand class=\"collapsible\"> \
		<table width=100% border=1><tr><td> \
		Command: <input type=text id=custom_command> \
		<input type=submit value=\"Submit\" onclick=\"IssueCustomCommand()\"><br> \
		<textarea id=custom_command_response readonly rows=15 cols=80></textarea> \
		</td></tr></table></td></tr> \
		 \
		<tr><td width=1> \
		<input type=submit onclick=\"ShowHideEvent( 'GPIOs' ); GPIODataTicker();\" value=\"GPIOs\"></td><td> \
		<div id=GPIOs class=\"collapsible\"> \
		<table width=100% border=1><tr> \
		<td align=center>0<input type=button id=ButtonGPIO0 value=0 onclick=\"TwiddleGPIO(0);\"><input type=button id=BGPIOIn0 value=In onclick=\"GPIOInput(0);\" class=\"inbutton\"></td> \
		<td align=center>1<input type=button id=ButtonGPIO1 value=0 onclick=\"TwiddleGPIO(1);\"><input type=button id=BGPIOIn1 value=In onclick=\"GPIOInput(1);\" class=\"inbutton\"></td> \
		<td align=center>2<input type=button id=ButtonGPIO2 value=0 onclick=\"TwiddleGPIO(2);\"><input type=button id=BGPIOIn2 value=In onclick=\"GPIOInput(2);\" class=\"inbutton\"></td> \
		<td align=center>3<input type=button id=ButtonGPIO3 value=0 onclick=\"TwiddleGPIO(3);\"><input type=button id=BGPIOIn3 value=In onclick=\"GPIOInput(3);\" class=\"inbutton\"></td> \
		<td align=center>4<input type=button id=ButtonGPIO4 value=0 onclick=\"TwiddleGPIO(4);\"><input type=button id=BGPIOIn4 value=In onclick=\"GPIOInput(4);\" class=\"inbutton\"></td> \
		<td align=center>5<input type=button id=ButtonGPIO5 value=0 onclick=\"TwiddleGPIO(5);\"><input type=button id=BGPIOIn5 value=In onclick=\"GPIOInput(5);\" class=\"inbutton\"></td> \
		<td>...</td> \
		<td align=center>12<input type=button id=ButtonGPIO12 value=0 onclick=\"TwiddleGPIO(12);\"><input type=button id=BGPIOIn12 value=In onclick=\"GPIOInput(12);\" class=\"inbutton\"></td> \
		<td align=center>13<input type=button id=ButtonGPIO13 value=0 onclick=\"TwiddleGPIO(13);\"><input type=button id=BGPIOIn13 value=In onclick=\"GPIOInput(13);\" class=\"inbutton\"></td> \
		<td align=center>14<input type=button id=ButtonGPIO14 value=0 onclick=\"TwiddleGPIO(14);\"><input type=button id=BGPIOIn14 value=In onclick=\"GPIOInput(14);\" class=\"inbutton\"></td> \
		<td align=center>15<input type=button id=ButtonGPIO15 value=0 onclick=\"TwiddleGPIO(15);\"><input type=button id=BGPIOIn15 value=In onclick=\"GPIOInput(15);\" class=\"inbutton\"></td> \
		</tr></table></div></td></tr>");
		



	$( ".collapsible" ).each(function( index ) {
		if( localStorage["sh" + this.id] > 0.5 )
		{
			$( this ).show().toggleClass( 'opened' );
		}
	});

	$("#custom_command_response").val( "" );


	output = document.getElementById("output");
	Ticker();

	KickWifiTicker();
	GPIODataTickerStart();
}

window.addEventListener("load", init, false);


function StartWebSocket()
{
	output.innerHTML = "Connecting...";
	if( websocket ) websocket.close();
	workarray = {};
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
	setTimeout( Ticker, 1000 );

	lasthz = (msg - tickmessage);
	tickmessage = msg;
	if( lasthz == 0 )
	{
		$('#SystemStatusClicker').css("color", "red" );
		$('#SystemStatusClicker').prop( "value", "System Offline" );
		if( commsup != 0 && !is_waiting_on_stations ) IssueSystemMessage( "Comms Lost." );
		commsup = 0;
		StartWebSocket();
	}
	else
	{
		$('#SystemStatusClicker').prop( "value", "System " + lasthz + "Hz" );
	}
}


function onMessage(evt)
{
	msg++;


	if( commsup != 1 )
	{
		commsup = 1;
		$('#SystemStatusClicker').css("color", "green" );
		IssueSystemMessage( "Comms Established." );
	}


	if( lastitem )
	{
		if( lastitem.callback )
		{
			lastitem.callback( lastitem, evt.data );
			lastitem = null;
		}
	}
	else
	{
		output.innerHTML = "<p>Messages: " + msg + "</p><p>RSSI: " + evt.data.substr(2) + "</p>";	
	}


	if( workqueue.length )
	{
		var elem = workqueue.shift();
		delete workarray[elem.request];

		if( elem.request )
		{
			doSend( elem.request );
			lastitem = elem;
			return;
		}
	}

	doSend('wx'); //Request RSSI.
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

function IsTabOpen( objname )
{
	var obj = $( "#" + objname );
	var opened = obj.is( '.opened' );
	return opened != 0;
}

function ShowHideEvent( objname )
{
	var obj = $( "#" + objname );
	obj.slideToggle( 'fast' ).toggleClass( 'opened' );
	var opened = obj.is( '.opened' );
	localStorage["sh" + objname] = opened?1:0;
	return opened!=0;
}


function IssueCustomCommand()
{
	QueueOperation( $("#custom_command").val(), function( req,data) { $("#custom_command_response").val( data ); } );
}







did_wifi_get_config = false;
is_data_ticker_running = false;
is_waiting_on_stations = false;

function ScanForWifi()
{
	QueueOperation('WS', null);
	is_waiting_on_stations=true;
	IssueSystemMessage( "Scanning for Wifi..." );
}

function KickWifiTicker()
{
	if( !is_data_ticker_running )
		WifiDataTicker();
}

function BSSIDClick( i )
{
	var tlines = wifilines[i];
	document.wifisection.wifitype.value = 1;
	document.wifisection.wificurname.value = tlines[0].substr(1);
	document.wifisection.wificurpassword.value = "";
	document.wifisection.wifimac.value = tlines[1];
	document.wifisection.wificurchannel.value = 0;

	ClickOpmode( 1 );
	return false;
}

function ClickOpmode( i )
{
	if( i == 1 )
	{
		document.wifisection.wificurname.disabled = false;
		document.wifisection.wificurpassword.disabled = false;
		document.wifisection.wifimac.disabled = false;
		document.wifisection.wificurchannel.disabled = true;
	}
	else
	{
		document.wifisection.wificurname.disabled = false;
		document.wifisection.wificurpassword.disabled = true;
		document.wifisection.wificurpassword.value = "";
		document.wifisection.wifimac.disabled = true;
		document.wifisection.wificurchannel.disabled = false;
	}
}

function WifiDataTicker()
{
	if( IsTabOpen('WifiSettings') )
	{
		is_data_ticker_running = true;

		if( !did_wifi_get_config )
		{
			QueueOperation( "WI", function(req,data)
			{
				var params = data.split( "\t" );
			
				var opmode = Number( params[0].substr(2) );
				document.wifisection.wifitype.value = opmode;
				document.wifisection.wificurname.value = params[1];
				document.wifisection.wificurpassword.value = params[2];
				document.wifisection.wifimac.value = params[3];
				document.wifisection.wificurchannel.value = Number( params[4] );

				ClickOpmode( opmode );
				did_wifi_get_config = true;
			} );
		}

		QueueOperation( "WR", function(req,data) {
			var lines = data.split( "\n" );
			var innerhtml;

			if( lines.length < 3 )
			{
				innerhtml = "No APs found.  Did you scan?";
				if( is_waiting_on_stations )
				{
					IssueSystemMessage( "No APs found." );
					is_waiting_on_stations = false;
				}
			}
			else
			{
				if( is_waiting_on_stations )
				{
					IssueSystemMessage( "Scan Complete." );
					is_waiting_on_stations = false;
				}

				innerhtml = "<TABLE border=1><TR><TH>SSID</TH><TH>MAC</TH><TH>RS</TH><TH>Ch</TH><TH>Enc</TH></TR>"
				wifilines = [];
				for( i = 1; i < lines.length-1; i++ )
				{
					tlines = lines[i].split( "\t" );
					wifilines.push(tlines);
					var bssidval = "<a href='javascript:void(0);' onclick='return BSSIDClick(" + (i -1 )+ ")'>" + tlines[1];
					innerhtml += "<TR><TD>" + tlines[0].substr(1) + "</TD><TD>" + bssidval + "</TD><TD>" + tlines[2] + "</TD><TD>" + tlines[3] + "</TD><TD>" + tlines[4] + "</TD></TR>";
				}
			}
			innerhtml += "</TABLE>";
			document.getElementById("WifiStations").innerHTML = innerhtml;
		} );
		setTimeout( WifiDataTicker, 500 );
	}
	else
	{
		is_data_ticker_running = 0;
	}
}

function ChangeWifiConfig()
{
	
	var st = "W";
	st += document.wifisection.wifitype.value;
	st += "\t" + document.wifisection.wificurname.value;
	st += "\t" + document.wifisection.wificurpassword.value;
	st += "\t" + document.wifisection.wifimac.value;
	st += "\t" + document.wifisection.wificurchannel.value;
	QueueOperation( st );
	did_wifi_get_config = false;
}




function TwiddleGPIO( gp )
{
	var st = "GF";
	st += gp;
	QueueOperation( st );
}

function GPIOInput( gp )
{
	var st = "GI";
	st += gp;
	QueueOperation( st );
}

function GPIOUpdate(req,data) {
	var secs = data.split( ":" );
	var op = 0;
	var n = Number(secs[2]);
	var m = Number(secs[1]);

	for( op = 0; op < 16; op++ )
	{
		var b = $( "#ButtonGPIO" + op );
		if( b )
		{
			if( 1<<op & n )
			{
				b.css("background-color","red" );
				b.css("color","black" );
				b.prop( "value", "1" );
			}
			else
			{
				b.css("background-color","black" );
				b.css("color","white" );
				b.prop( "value", "0" );
			}
		}

		b = $( "#BGPIOIn" + op );
		if( b )
		{
			if( 1<<op & m )
			{
				b.css("background-color","blue" );
				b.css("color","white" );
				b.attr( "value", "out" );
			}
			else
			{
				b.css("background-color","green" );
				b.css("color","white" );
				b.attr( "value", "in" );
			}
		}


	}
	if( IsTabOpen('GPIOs') )
		QueueOperation( "GS", GPIOUpdate );
}

function GPIODataTicker()
{
	if( !IsTabOpen('GPIOs') ) return;
	QueueOperation( "GS", GPIOUpdate );
	setTimeout( GPIODataTicker, 500 );
}


function GPIODataTickerStart()
{
	if( IsTabOpen('GPIOs') )
		GPIODataTicker();
}










function tohex8( c )
{
	var hex = c.toString(16);
	return hex.length == 1 ? "0" + hex : hex;
}
