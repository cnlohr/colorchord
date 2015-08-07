var wsUri = "ws://" + location.host + "/d/ws/issue";
var output;
var websocket;
var commsup = 0;
globalParams = {};

//Push objects that have:
// .request
// .callback = function( ref (this object), data );

var workqueue = [];
var wifilines = [];
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
	KickNotes();
	KickLEDs();
}

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
		commsup = 0;
		StartWebSocket();
	}
	else
	{
		$('#SystemStatusClicker').prop( "value", "System " + lasthz + "Hz" );
	}


	QueueOperation( "CVR", ReceiveParameters );
}


function onMessage(evt)
{
	msg++;


	if( commsup != 1 )
	{
		commsup = 1;
		$('#SystemStatusClicker').css("color", "green" );
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



window.addEventListener("load", init, false);







///////// Various functions that are not core appear down here.











function ChangeParam( p )
{
	var num = Number( p.value );
	var elem = p.id.substr( 5 );
	QueueOperation( "CVW:" + elem + ":" + num );
}


var hasCreateParams = false;
function ReceiveParameters(req,data) {
	var elems = data.split( ":" );

	for( var v = 0; v < elems.length; v++ )
	{
		var pair = elems[v].split( "=" );
		if( pair.length == 2 )
		{
			globalParams[pair[0]] = Number( pair[1] );
		}
	}

	if( !hasCreateParams )
	{
		hasCreateParams = true;
		var htv = "<table border=1><tr><th>Value</th><th width=100%>Parameter</th></tr>";

		for( var v in globalParams )
		{
			var vp = globalParams[v];
			htv += "<tr><td><INPUT TYPE=TEXT ID=param" + v + " VALUE=" + vp + " onchange=ChangeParam(this)></td><td>" + v + "</td></tr>";
		}

		$("#InnerParameters").html( htv + "</table>" );

		for( var v in globalParams )
		{
			if( v.substr(0,1) == 'r' )
			{
				$("#param" + v).prop( "disabled", true );
			}
		}		

	}

	for( var v in globalParams )
	{
		var vp = globalParams[v];
		var p = $("#param"+v);
		if( !p.is(":focus" ) )
			p.val(vp);
	}

}









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
pause_dft = false;

function KickDFT()
{
	$( "#DFTPauseButton" ).css( "background-color", (is_dft_running&&!pause_dft)?"green":"red" );
	if( !is_dft_running && !pause_dft )
		DFTDataTicker();
}



function ToggleDFTPause()
{
	pause_dft = !pause_dft;
	KickDFT();
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

	$( "#DFTPauseButton" ).css( "background-color", "green" );

	var samps = Number( secs[1] );
	var data = secs[2];
	var lastsamp = parseInt( data.substr(0,4),16 );
	ctx.clearRect( 0, 0, canvas.width, canvas.height );
	ctx.beginPath();

	for( var i = 0; i < samps; i++ )
	{
		var x2 = i * canvas.clientWidth / samps;
		var samp = parseInt( data.substr(i*4,4),16 );
		var y2 = ( 1.-samp / 2047 ) * canvas.clientHeight;

		ctx.fillStyle = CCColor( i % globalParams["rFIXBPERO"] );
		ctx.fillRect( x2, y2, canvas.clientWidth / samps, canvas.clientHeight-y2 );

		ctx.strokeStyle = "#000000";
		ctx.strokeRect( x2, y2, canvas.clientWidth / samps, canvas.clientHeight-y2 );
	}
	ctx.stroke();

	var samp = parseInt( data.substr(i*2,2),16 );

	DFTDataTicker();
} 

function DFTDataTicker()
{
	if( IsTabOpen('DFT') && !pause_dft )
	{
		is_dft_running = true;
		QueueOperation( "CB" + $("#WhichCanvas").val(),  GotDFT );
	}
	else
	{
		is_dft_running = 0;
	}
	$( "#DFTPauseButton" ).css( "background-color", (is_dft_running&&!pause_dft)?"green":"red" );
}







is_leds_running = false;
pause_led = false;

function KickLEDs()
{
	$( "#LEDPauseButton" ).css( "background-color", (is_leds_running&&!pause_led)?"green":"red" );

	if( !is_leds_running && !pause_led )
		LEDDataTicker();

}

function ToggleLEDPause()
{
	pause_led = !pause_led;
	KickLEDs();
}


function GotLED(req,data)
{
	var ls = document.getElementById('LEDCanvasHolder');
	var canvas = document.getElementById('LEDCanvas');
	var ctx = canvas.getContext('2d');
	var h = ls.height;
	var w = ls.width;
	if( canvas.width != ls.clientWidth-10 )   canvas.width = ls.clientWidth-10;
	if( ctx.canvas.width != canvas.clientWidth )   ctx.canvas.width = canvas.clientWidth;

	var secs = data.split( ":" );

	$( "#LEDPauseButton" ).css( "background-color", "green" );

	var samps = Number( secs[1] );
	var data = secs[2];
	var lastsamp = parseInt( data.substr(0,4),16 );
	ctx.clearRect( 0, 0, canvas.width, canvas.height );

	for( var i = 0; i < samps; i++ )
	{
		var x2 = i * canvas.clientWidth / samps;
		var samp = data.substr(i*6,6);
		var y2 = ( 1.-samp / 2047 ) * canvas.clientHeight;

		ctx.fillStyle = "#" + samp.substr( 2, 2 ) + samp.substr( 0, 2 ) + samp.substr( 4, 2 );
		ctx.lineWidth = 0;
		ctx.fillRect( x2, 0, canvas.clientWidth / samps+1, canvas.clientHeight );
	}

	var samp = parseInt( data.substr(i*2,2),16 );

	LEDDataTicker();
} 

function LEDDataTicker()
{
	if( IsTabOpen('LEDs') && !pause_led )
	{
		is_leds_running = true;
		QueueOperation( "CL",  GotLED );
	}
	else
	{
		is_leds_running = 0;
	}
	$( "#LEDPauseButton" ).css( "background-color", (is_leds_running&&!pause_led)?"green":"red" );

}










is_notes_running = false;
pause_notes = false;

function KickNotes()
{
	$( "#NotesPauseButton" ).css( "background-color", (is_notes_running&&!pause_notes)?"green":"red" );

	if( !is_notes_running && !pause_notes )
		NotesTicker();
}



function ToggleNotesPause()
{
	pause_notes = !pause_notes;
	KickNotes();
}

function GotNotes(req,data)
{
	var canvas = document.getElementById('NotesCanvas');
	var ctx = canvas.getContext('2d');

	var secs = data.split( ":" );

	var elems = Number(secs[1] );

	ctx.canvas.width =  400;
	ctx.canvas.height = elems*25;


	$( "#NotesPauseButton" ).css( "background-color", "green" );

	var data = secs[2];
	ctx.fillStyle = "#000000";
	ctx.font = "18px serif"
	ctx.fillRect( 0, 0, canvas.width, canvas.height );

	for( var i = 0; i < elems; i++ )
	{
		var peak   = parseInt( data.substr(i*12+0,2),16 );
		var amped  = parseInt( data.substr(i*12+2,4),16 );
		var amped2 = parseInt( data.substr(i*12+6,4),16 );
		var jump   = parseInt( data.substr(i*12+10,2),16 );

		if( peak == 255 )
		{
			ctx.fillStyle = "#ffffff";
			ctx.fillText( jump, 10, i*25 + 20 );
			continue;
		}

		ctx.fillStyle = CCColorDetail( peak );
		ctx.lineWidth = 0;
		ctx.fillRect( 0, i*25, 100,25);
		ctx.fillRect( 101, i*25, amped/200,25);
		ctx.fillRect( 229, i*25, amped2/200,25);

		ctx.fillStyle = "#000000";
		ctx.fillText( peak, 10, i*25 + 20 );
		ctx.fillStyle = "#ffffff";
		ctx.fillText( amped, 121, i*25 + 20 );
		ctx.fillText( amped2, 240, i*25 + 20 );
	}

	var samp = parseInt( data.substr(i*2,2),16 );

	NotesTicker();
} 

function NotesTicker()
{
	if( IsTabOpen('Notes')&& !pause_notes )
	{
		is_notes_running = true;
		QueueOperation( "CN", GotNotes );
	}
	else
	{
		is_notes_running = 0;
	}
	$( "#NotesPauseButton" ).css( "background-color", (is_notes_running&&!pause_notes)?"green":"red" );
}






did_wifi_get_config = false;
is_data_ticker_running = false;

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
			
				document.wifisection.wifitype.value = Number( params[0].substr(2) );
				document.wifisection.wificurname.value = params[1];
				document.wifisection.wificurpassword.value = params[2];
				document.wifisection.wifimac.value = params[3];
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









function CCColor( note )
{
	return ECCtoHEX( (note * globalParams["rNOTERANGE"] / globalParams["rFIXBPERO"] + globalParams["gROOT_NOTE_OFFSET"] + globalParams["rNOTERANGE"] )%globalParams["rNOTERANGE"], 255, 255 );
}

function CCColorDetail( note )
{
	return ECCtoHEX( (note + globalParams["gROOT_NOTE_OFFSET"] + globalParams["rNOTERANGE"] )%globalParams["rNOTERANGE"], 255, 255 );
}

function ECCtoHEX( note, sat, val )
{
	var hue = 0;
	var third = 65535/3;
	var scalednote = note;
	var renote = note * 65536 / globalParams["rNOTERANGE"];

	//Note is expected to be a vale from 0..(NOTERANGE-1)
	//renote goes from 0 to the next one under 65536.


	if( renote < third )
	{
		//Yellow to Red.
		hue = (third - renote) >> 1;
	}
	else if( renote < (third<<1) )
	{
		//Red to Blue
		hue = (third-renote);
	}
	else
	{
		//hue = ((((65535-renote)>>8) * (uint32_t)(third>>8)) >> 1) + (third<<1);
		hue = (((65536-renote)<<16) / (third<<1)) + (third>>1); // ((((65535-renote)>>8) * (uint32_t)(third>>8)) >> 1) + (third<<1);
	}
	hue >>= 8;

	return EHSVtoHEX( hue, sat, val );
}

function EHSVtoHEX( hue, sat, val )  //0..255 for all
{
	var SIXTH1 = 43;
	var SIXTH2 = 85;
	var SIXTH3 = 128;
	var SIXTH4 = 171;
	var SIXTH5 = 213;

	var or = 0, og = 0, ob = 0;

	hue -= SIXTH1; //Off by 60 degrees.

	hue = (hue+256)%256;
	//TODO: There are colors that overlap here, consider 
	//tweaking this to make the best use of the colorspace.

	if( hue < SIXTH1 ) //Ok: Yellow->Red.
	{
		or = 255;
		og = 255 - (hue * 255) / (SIXTH1);
	}
	else if( hue < SIXTH2 ) //Ok: Red->Purple
	{
		or = 255;
		ob = hue*255 / SIXTH1 - 255;
	}
	else if( hue < SIXTH3 )  //Ok: Purple->Blue
	{
		ob = 255;
		or = ((SIXTH3-hue) * 255) / (SIXTH1);
	}
	else if( hue < SIXTH4 ) //Ok: Blue->Cyan
	{
		ob = 255;
		og = (hue - SIXTH3)*255 / SIXTH1;
	}
	else if( hue < SIXTH5 ) //Ok: Cyan->Green.
	{
		og = 255;
		ob = ((SIXTH5-hue)*255) / SIXTH1;
	}
	else //Green->Yellow
	{
		og = 255;
		or = (hue - SIXTH5) * 255 / SIXTH1;
	}

	var rv = val;
	if( rv > 128 ) rv++;
	var rs = sat;
	if( rs > 128 ) rs++;

	//or, og, ob range from 0...255 now.
	//Need to apply saturation and value.

	or = (or * val)>>8;
	og = (og * val)>>8;
	ob = (ob * val)>>8;

	//OR..OB == 0..65025
	or = or * rs + 255 * (256-rs);
	og = og * rs + 255 * (256-rs);
	ob = ob * rs + 255 * (256-rs);
//printf( "__%d %d %d =-> %d\n", or, og, ob, rs );

	or >>= 8;
	og >>= 8;
	ob >>= 8;

	if( or > 255 ) or = 255;
	if( og > 255 ) og = 255;
	if( ob > 255 ) ob = 255;

	return "#" + tohex8(og) + tohex8(or) + tohex8(ob);
}

function tohex8( c )
{
	var hex = c.toString(16);
	return hex.length == 1 ? "0" + hex : hex;
}
