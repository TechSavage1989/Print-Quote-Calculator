(function(root){
 'use strict';
 const factors={mm:.001,cm:.01,m:1,in:.0254,ft:.3048};
 function length(value,unit){if(!Number.isFinite(value)||value<0||!Object.hasOwn(factors,unit))throw Error('Invalid length');const meters=value*factors[unit],result={};for(const u of Object.keys(factors)){result[u]=meters/factors[u];if(!Number.isFinite(result[u]))throw Error('Length too large');}return result;}
 function filament(meters,grams,diameter,density){if([meters,grams,diameter,density].some(v=>!Number.isFinite(v)||v<0)||diameter<=0||density<=0)throw Error('Invalid filament inputs');const perMeter=density*Math.PI*diameter*diameter/4;const result={mass:meters*perMeter,length:grams/perMeter};if(!Number.isFinite(result.mass)||!Number.isFinite(result.length))throw Error('Filament values too large');return result;}
 const api={length,filament};if(typeof module!=='undefined')module.exports=api;else root.PrintConverters=api;
})(typeof window!=='undefined'?window:globalThis);
