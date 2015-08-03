var wsUri = "ws://" + location.host + "/d/ws/issue";
var output;
var websocket;
var commsup = 0;

//Push objects that have:
// .request
// .callback = function( ref (this object), data );

var workqueue = [];
var workarray = {};
var lastitem;

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
	$( ".collapsible" ).each(function( index ) {
		if( localStorage["sh" + this.id] > 0.5 )
		{
			$( this ).show().toggleClass( 'opened' );
//			console.log( "OPEN: " + this.id );
		}
	});

	$("#custom_command_response").val( "" );


	output = document.getElementById("output");
	Ticker();

	KickWifiTicker();
	GPIODataTickerStart();
	KickOscilloscope();
	KickDFT();
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
		delete workarray[elem.request];
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



window.addEventListener("load", init, false);







///////// Various functions that are not core appear down here.

is_oscilloscope_running = false;
pause_oscilloscope = false;

function KickOscilloscope()
{
	$( "#OScopePauseButton" ).css( "background-color", (is_oscilloscope_running&&pause_oscilloscope)?"green":"red" );
	if( !is_oscilloscope_running && !pause_oscilloscope)
		OScopeDataTicker();
}

function ToggleOScopePause()
{
	pause_oscilloscope = !pause_oscilloscope;
	KickOscilloscope();
}

function GotOScope(req,data)
{
	var canvas = document.getElementById('OScopeCanvas');
	var ctx = canvas.getContext('2d');
	var h = canvas.height;
	var w = canvas.width;
	if( ctx.canvas.width != canvas.clientWidth )   ctx.canvas.width = canvas.clientWidth;
	if( ctx.canvas.height != canvas.clientHeight ) ctx.canvas.height = canvas.clientHeight;

	$( "#OScopePauseButton" ).css( "background-color", "green" );

	var secs = data.split( ":" );

	var samps = Number( secs[1] );
	var data = secs[2];
	var lastsamp = parseInt( data.substr(0,2),16 );
	ctx.clearRect( 0, 0, canvas.width, canvas.height );
	ctx.beginPath();
	for( var i = 0; i < samps; i++ )
	{
		var x2 = (i+1) * canvas.clientWidth / samps;
		var samp = parseInt( data.substr(i*2+2,2),16 );
		var y2 = ( 1.-samp / 255 ) * canvas.clientHeight;
		
		if( i == 0 )
		{
			var x1 = i * canvas.clientWidth / samps;
			var y1 = ( 1.-lastsamp / 255 ) * canvas.clientHeight;
			ctx.moveTo( x1, y1 );
		}

		ctx.lineTo( x2, y2 );

		lastsamp = samp;
	}
	ctx.stroke();

	var samp = parseInt( data.substr(i*2,2),16 );

	OScopeDataTicker();
} 

function OScopeDataTicker()
{
	if( IsTabOpen('OScope') && !pause_oscilloscope )
	{
		is_oscilloscope_running = true;
		QueueOperation( "CM",  GotOScope );
	}
	else
	{
		is_oscilloscope_running = 0;
	}
	$( "#OScopePauseButton" ).css( "background-color", (is_oscilloscope_running&&!pause_oscilloscope)?"green":"red" );
}









is_dft_running = false;

function KickDFT()
{
	if( !is_dft_running )
		DFTDataTicker();

}

function GotDFT(req,data)
{
	var canvas = document.getElementById('DFTCanvas');
	var ctx = canvas.getContext('2d');
	var h = canvas.height;
	var w = canvas.width;
	if( ctx.canvas.width != canvas.clientWidth )   ctx.canvas.width = canvas.clientWidth;
	if( ctx.canvas.height != canvas.clientHeight ) ctx.canvas.height = canvas.clientHeight;

	var secs = data.split( ":" );

	var samps = Number( secs[1] );
	var data = secs[2];
	var lastsamp = parseInt( data.substr(0,4),16 );
	ctx.clearRect( 0, 0, canvas.width, canvas.height );
	ctx.beginPath();
	for( var i = 0; i < samps; i++ )
	{
		var x2 = (i+1) * canvas.clientWidth / samps;
		var samp = parseInt( data.substr(i*4+4,4),16 );
		var y2 = ( 1.-samp / 2047 ) * canvas.clientHeight;
		
		if( i == 0 )
		{
			var x1 = i * canvas.clientWidth / samps;
			var y1 = ( 1.-lastsamp / 2047 ) * canvas.clientHeight;
			ctx.moveTo( x1, y1 );
		}

		ctx.lineTo( x2, y2 );

		lastsamp = samp;
	}
	ctx.stroke();

	var samp = parseInt( data.substr(i*2,2),16 );

	DFTDataTicker();
} 

function DFTDataTicker()
{
	if( IsTabOpen('DFT') )
	{
		is_dft_running = true;
		QueueOperation( "CB",  GotDFT );
	}
	else
	{
		is_dft_running = 0;
	}
}














did_wifi_get_config = false;
is_data_ticker_running = false;

function KickWifiTicker()
{
	if( !is_data_ticker_running )
		WifiDataTicker();
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
	st += ":" + document.wifisection.wificurname.value;
	st += ":" + document.wifisection.wificurpassword.value;
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





