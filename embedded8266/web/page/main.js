//Copyright (C) 2015 <>< Charles Lohr, see LICENSE file for more info.
//
//This particular file may be licensed under the MIT/x11, New BSD or ColorChord Licenses.

var globalParams = {};


function mainticker()
{
	KickOscilloscope();
	KickDFT();
	KickNotes();
	KickLEDs();

	QueueOperation( "CVR", ReceiveParameters );
	setTimeout( mainticker, 1000 );
}

function maininit()
{
	setTimeout( mainticker, 1000 );
}


window.addEventListener("load", maininit, false);




function ChangeParam( p )
{
	var num = Number( p.value );
	var elem = p.id.substr( 5 );
	QueueOperation( "CVW\t" + elem + "\t" + num );
}


var hasCreateParams = false;
function ReceiveParameters(req,data) {
	var elems = data.split( "\t" );

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




var is_oscilloscope_running = false;
var pause_oscilloscope = false;

function KickOscilloscope()
{
	$( "#OScopePauseButton" ).css( "background-color", (is_oscilloscope_running&&!pause_oscilloscope)?"green":"red" );
	if( !pause_oscilloscope)
		OScopeDataTicker();
}

function ToggleOScopePause()
{
	pause_oscilloscope = !pause_oscilloscope;
	KickOscilloscope();
}

function GotOScope(req,data)
{
	var mult = Number(document.getElementById('OSCMultIn').value);
	document.getElementById('OSCMultOut').innerHTML = mult;
	var canvas = document.getElementById('OScopeCanvas');
	var ctx = canvas.getContext('2d');
	var h = canvas.height;
	var w = canvas.width;
	if( ctx.canvas.width != canvas.clientWidth )   ctx.canvas.width = canvas.clientWidth;
	if( ctx.canvas.height != canvas.clientHeight ) ctx.canvas.height = canvas.clientHeight;

	$( "#OScopePauseButton" ).css( "background-color", "green" );

	var secs = data.split( "\t" );

	var samps = Number( secs[1] );
	var data = secs[2];
	var lastsamp = parseInt( data.substr(0,2),16 );
	ctx.clearRect( 0, 0, canvas.width, canvas.height );
	ctx.beginPath();
	for( var i = 0; i < samps; i++ )
	{
		var x2 = (i+1) * canvas.clientWidth / samps;
		var samp = parseInt( data.substr(i*2+2,2),16 );
		var y2 = ( 1.-mult*samp / 255 ) * canvas.clientHeight;
		
		if( i == 0 )
		{
			var x1 = i * canvas.clientWidth / samps;
			var y1 = ( 1.-mult*lastsamp / 255 ) * canvas.clientHeight;
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
		is_oscilloscope_running = false;
	}
	$( "#OScopePauseButton" ).css( "background-color", (is_oscilloscope_running&&!pause_oscilloscope)?"green":"red" );
}









var is_dft_running = false;
var pause_dft = false;

function KickDFT()
{
	$( "#DFTPauseButton" ).css( "background-color", (is_dft_running&&!pause_dft)?"green":"red" );
	if( !pause_dft )
		DFTDataTicker();
}



function ToggleDFTPause()
{
	pause_dft = !pause_dft;
	KickDFT();
}

function GotDFT(req,data)
{
	var mult = Number(document.getElementById('DFTMultIn').value);
	document.getElementById('DFTMultOut').innerHTML = mult;
	var canvas = document.getElementById('DFTCanvas');
	var ctx = canvas.getContext('2d');
	var h = canvas.height;
	var w = canvas.width;
	if( ctx.canvas.width != canvas.clientWidth )   ctx.canvas.width = canvas.clientWidth;
	if( ctx.canvas.height != canvas.clientHeight ) ctx.canvas.height = canvas.clientHeight;

	var secs = data.split( "\t" );

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
		var y2 = ( 1.-mult*samp / 2047 ) * canvas.clientHeight;

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







var is_leds_running = false;
var pause_led = false;

function KickLEDs()
{
	$( "#LEDPauseButton" ).css( "background-color", (is_leds_running&&!pause_led)?"green":"red" );

	if( !pause_led )
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

	var secs = data.split( "\t" );

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










var is_notes_running = false;
var pause_notes = false;

function KickNotes()
{
	$( "#NotesPauseButton" ).css( "background-color", (is_notes_running&&!pause_notes)?"green":"red" );

	if(!pause_notes )
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

	var secs = data.split( "\t" );

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
