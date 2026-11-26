// WebApp.h — the phone page Pip serves (single file, no internet needed).
// Menus: CONTROL (manual override, arm, party) | MODES | FOCUS (timer) | VISION (ToF) | MUSIC | SETTINGS | DEBUG
// Protocol (WebSocket /ws):
//   phone -> Pip text:  j,lx,ly,ry (sticks -100..100) | m,<mode id> | g,<arm gesture> | v,<0/1 ToF viewer>
//                       t,<minutes> (focus timer, 0 = cancel) | k (party trick) | r (reset memory) | s (stop music)
//                       p,<key>,<value> (tuning) | e,<event id> (debug) | x,<perf id> (debug)
//   phone -> Pip binary: [0xA1, bass, mid, treble, beat, ...PCM u8 16 kHz mono]
//   Pip -> phone text:  {"m":[happy,energy,trust, happyCore,energyCore,trustCore],"mood":"..","mode":n,"zz":0/1,
//                        "log":"..","seq":n,"day":n,"focus":secondsLeft}
//   Pip -> phone binary: [1, 64 x uint16 LE mm]  (ToF grid)
// Mode ids = Mode enum in Brain.h. Event ids in the DEBUG list = Event enum in Brain.h.
#pragma once
#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no"><title>Pip</title>
<style>
:root{--bg:#15130f;--ink:#e9dfc8;--dim:#8c8270;--line:#3a352c}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
body{margin:0;background:radial-gradient(#2a2620,#121110);color:var(--ink);font:15px Georgia,serif;touch-action:none;overflow:hidden;height:100vh;display:flex;flex-direction:column}
header{display:flex;justify-content:space-between;align-items:center;padding:8px 14px}
#st{font-size:12px;color:var(--dim)}
.rings{display:flex;justify-content:center;gap:14px}
.ring{width:92px;text-align:center;font-size:10px;letter-spacing:2px;color:var(--dim)}
.ring svg{width:80px;height:80px;display:block;margin:auto}
.ring circle.v{transition:stroke-dasharray .5s}.ring rect{transition:all 1.5s}
.crit .ic{animation:fl .5s infinite alternate}@keyframes fl{to{opacity:.25}}
#mood{text-align:center;font-size:17px;letter-spacing:3px;text-transform:uppercase;margin-top:2px}
#log{height:16px;text-align:center;font-size:12px;color:var(--dim)}
.tabs{display:flex;gap:5px;justify-content:center;padding:4px;flex-wrap:wrap}
button{background:#1b1916;color:var(--ink);border:1px solid var(--line);border-radius:8px;padding:7px 10px;font:13px Georgia,serif;margin:2px}
button.on{background:var(--ink);color:#15130f}
button.big{width:100%;padding:12px;font-size:15px}
.panel{display:none;padding:6px 14px;overflow:auto;max-height:32vh;touch-action:pan-y}
.panel.on{display:block}
.panel h4{margin:8px 0 4px;font-size:11px;letter-spacing:2px;color:var(--dim);font-weight:normal}
#grid{display:grid;grid-template-columns:repeat(8,1fr);gap:2px;width:min(56vw,230px);margin:6px auto}
#grid div{aspect-ratio:1;border-radius:3px;font-size:8px;display:flex;align-items:center;justify-content:center;color:#000;background:#222}
.row{display:flex;align-items:center;gap:8px;margin:5px 0}.row label{width:98px;font-size:12px}.row input{flex:1}
#drop{border:2px dashed var(--line);border-radius:10px;padding:14px;text-align:center;color:var(--dim)}
#drop.over{border-color:var(--ink);color:var(--ink)}
#clock{font-size:34px;text-align:center;letter-spacing:4px;margin:4px}
.sticks{flex:1;display:flex;justify-content:space-between;align-items:flex-end;padding:8px 22px 22px}
.pad{width:38vw;max-width:165px;aspect-ratio:1;border:2px solid var(--line);border-radius:50%;position:relative;background:#1b191688}
.nub{width:42%;height:42%;border-radius:50%;background:var(--ink);opacity:.85;position:absolute;left:29%;top:29%}
.pad small{position:absolute;bottom:-18px;width:100%;text-align:center;color:var(--dim);font-size:10px;letter-spacing:2px}
</style></head><body>
<header><b>PIP</b><span id="st">connecting…</span></header>
<div class="rings" id="rings"></div><div id="mood">—</div><div id="log"></div>
<div class="tabs" id="tabs"></div>

<div class="panel" id="p_control">
  <button class="big" id="manual">Manual control: OFF</button>
  <h4>ARM</h4><div id="arm"></div>
  <h4>TRICKS</h4><button onclick="send('k')">Party trick</button>
</div>
<div class="panel" id="p_modes"><h4>WHAT PIP DOES ON HIS OWN</h4><div id="modes"></div></div>
<div class="panel" id="p_focus">
  <div id="clock">--:--</div>
  <div id="mins" style="text-align:center"></div>
  <div class="row"><label>Custom (min)</label><input type="number" id="cmin" min="1" max="180" value="30" style="flex:0 0 70px"><button onclick="send('t,'+$('cmin').value)">Start</button><button onclick="send('t,0')">Cancel</button></div>
</div>
<div class="panel" id="p_vision"><button class="big" id="see">Show what Pip sees</button><div id="grid"></div></div>
<div class="panel" id="p_music">
  <div id="drop">Drop a song here, or tap to choose<input type="file" id="file" accept="audio/*" hidden></div>
  <div style="text-align:center"><button id="stop">Stop</button> <span id="mst"></span></div><div id="mvol"></div>
</div>
<div class="panel" id="p_settings"><div id="tune"></div><h4>MEMORY</h4><div id="days"></div><button id="reset">Reset Pip's memory</button></div>
<div class="panel" id="p_debug"><h4>FAKE AN EVENT</h4><div id="dbg"></div></div>

<div class="sticks"><div class="pad" id="L"><div class="nub"></div><small>DRIVE</small></div><div class="pad" id="R"><div class="nub"></div><small>ARM</small></div></div>
<script>
const $=id=>document.getElementById(id);let ws,ok=false,mode=0,seeing=false;
function send(m){if(ok)ws.send(m)}
function conn(){ws=new WebSocket('ws://'+location.host+'/ws');ws.binaryType='arraybuffer';
 ws.onopen=()=>{ok=true;$('st').textContent='connected';send('v,'+(seeing?1:0))};
 ws.onclose=()=>{ok=false;$('st').textContent='reconnecting…';setTimeout(conn,1500)};
 ws.onmessage=e=>{if(typeof e.data=='string')state(JSON.parse(e.data));else tof(new DataView(e.data))}}
// ---- mood rings: outer ring = right now, inner fill = how his days have been ----
const R=[['HAPPINESS','#f2e6c9','<circle cx="-7" cy="-5" r="1.6"/><circle cx="7" cy="-5" r="1.6"/><path d="M-9 5Q0 15 9 5"/>'],
         ['ENERGY','#f0b84a','<path d="M3-14L-7 2H0L-3 14L7-2H0Z"/>'],
         ['TRUST','#e58a9a','<path d="M0 12C-16 0-9-12 0-4C9-12 16 0 0 12Z"/>']];
$('rings').innerHTML=R.map((r,i)=>`<div class="ring" id="rg${i}"><svg viewBox="-42 -42 84 84">
<circle r="36" fill="none" stroke="#3a352c" stroke-width="5"/>
<circle class="v" r="36" fill="none" stroke="${r[1]}" stroke-width="5" stroke-linecap="round" stroke-dasharray="0 999" transform="rotate(-90)"/>
<clipPath id="c${i}"><circle r="28"/></clipPath><circle r="29" fill="#1b1916" stroke="#4a4337"/>
<rect x="-29" y="29" width="58" height="0" fill="${r[1]}" opacity=".25" clip-path="url(#c${i})"/>
<g class="ic" fill="none" stroke="${r[1]}" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round">${r[2]}</g></svg>${r[0]}</div>`).join('');
let seq=-1;const fmt=s=>String(Math.floor(s/60)).padStart(2,'0')+':'+String(s%60).padStart(2,'0');
function state(d){const C=2*Math.PI*36;for(let i=0;i<3;i++){const el=$('rg'+i),v=d.m[i],c=d.m[i+3];
  el.querySelector('.v').setAttribute('stroke-dasharray',C*v/100+' '+C);
  const h=58*c/100,rc=el.querySelector('rect');rc.setAttribute('y',29-h);rc.setAttribute('height',h);el.classList.toggle('crit',v<15)}
 $('mood').textContent=d.mood;if(d.seq!=seq){seq=d.seq;$('log').textContent=d.log||''}
 mode=d.mode;document.querySelectorAll('#modes button').forEach(b=>b.classList.toggle('on',+b.dataset.m==mode));
 $('manual').textContent='Manual control: '+(mode==1?'ON':'OFF');$('manual').classList.toggle('on',mode==1);
 $('clock').textContent=d.focus>0?fmt(d.focus):'--:--';$('days').textContent='Day '+d.day+' with you';
 $('st').textContent=mode==5?'focus nap':d.zz?'asleep':'connected'}
// ---- menus ----
let tab='';[['control','CONTROL'],['modes','MODES'],['focus','FOCUS'],['vision','VISION'],['music','MUSIC'],['settings','SETTINGS'],['debug','DEBUG']].forEach(([n,l])=>{
 const b=document.createElement('button');b.textContent=l;b.id='t_'+n;
 b.onclick=()=>{tab=tab==n?'':n;document.querySelectorAll('.panel').forEach(p=>p.classList.toggle('on',p.id=='p_'+tab));
  document.querySelectorAll('#tabs button').forEach(x=>x.classList.toggle('on',x.id=='t_'+tab))};$('tabs').append(b)});
function btns(box,list,pre){list.forEach(([n,v])=>{const b=document.createElement('button');b.textContent=n;b.dataset.m=v;b.onclick=()=>send(pre+v);$(box).append(b)})}
$('manual').onclick=()=>send('m,'+(mode==1?0:1));                       // manual override toggle
btns('modes',[['Free',0],['Follow my hand',2],['Wander',3],['Sleep',4]],'m,');
btns('arm',[['Wave',1],['Nod',2],['Shrug',3],['Flex',4],['Show',5],['Droop',6],['Bump',7],['Bob',8],['High five',9]],'g,');
btns('mins',[['15 min',15],['25 min',25],['45 min',45],['60 min',60]],'t,');
btns('dbg',[['Pet',3],['Tap',4],['Overpet',5],['Hand',6],['Fast hand',7],['Hand lost',8],['Covered',9],['Peekaboo',10],['Fist bump',11],['Picked up',12],['Shaken',13],['Upside',14],['Knock',15],['Set down',16],['Edge',17],['Bump',18],['High five',23],['Morning',24]],'e,');
$('reset').onclick=()=>{if(confirm('Erase everything Pip remembers?'))send('r')};
// ---- sliders ----
function slider(box,n,k,a,b,v){const d=document.createElement('div');d.className='row';d.innerHTML=`<label>${n}</label><input type="range" min="${a}" max="${b}" step="${(b-a)/100}" value="${v}"><span>${v}</span>`;
 const i=d.querySelector('input');i.oninput=()=>{d.querySelector('span').textContent=(+i.value).toFixed(3);send('p,'+k+','+i.value)};$(box).append(d)}
slider('mvol','Music volume','mvol',0,1,.8);
[['Volume','vol',0,1,.6],['Glow','glow',0,1,.45],['Max speed','spd',.2,1,1],['Follow Kp turn','kpt',0,.6,.22],['Follow Kd turn','kdt',0,.4,.1],['Follow Kp dist','kpd',0,.01,.004]].forEach(a=>slider('tune',...a));
// ---- vision: what the ToF sensor sees (8x8, ~2 KB/s) ----
for(let i=0;i<64;i++)$('grid').append(document.createElement('div'));
$('see').onclick=()=>{seeing=!seeing;$('see').classList.toggle('on',seeing);$('see').textContent=seeing?'Hide':'Show what Pip sees';send('v,'+(seeing?1:0))};
function tof(v){if(v.getUint8(0)!=1)return;const c=$('grid').children;for(let i=0;i<64;i++){const d=v.getUint16(1+i*2,true);
  if(!d){c[i].style.background='#222';c[i].textContent='';continue}
  const t=Math.min(1,Math.sqrt(Math.max(0,d-40)/1500));c[i].style.background=`hsl(${30+200*t} ${85-45*t}% ${62-45*t}%)`;c[i].textContent=d<1000?d:''}}
// ---- joysticks ----
const J={lx:0,ly:0,ry:0};
function pad(el,cb){const nub=el.querySelector('.nub');let id=null;
 const mv=e=>{const r=el.getBoundingClientRect();let x=(e.clientX-r.left)/r.width*2-1,y=(e.clientY-r.top)/r.height*2-1;const m=Math.hypot(x,y);if(m>1){x/=m;y/=m}
  nub.style.left=(29+x*29)+'%';nub.style.top=(29+y*29)+'%';cb(x,-y)};
 el.onpointerdown=e=>{id=e.pointerId;el.setPointerCapture(id);mv(e)};
 el.onpointermove=e=>{if(e.pointerId==id)mv(e)};
 el.onpointerup=el.onpointercancel=e=>{if(e.pointerId!=id)return;id=null;nub.style.left='29%';nub.style.top='29%';cb(0,0)}}
pad($('L'),(x,y)=>{J.lx=x;J.ly=y});pad($('R'),(x,y)=>{J.ry=y});
let idle=0;setInterval(()=>{const z=!J.lx&&!J.ly&&!J.ry;if(z&&idle>3)return;idle=z?idle+1:0;
 send('j,'+Math.round(J.lx*100)+','+Math.round(J.ly*100)+','+Math.round(J.ry*100))},50);
// ---- music: decode on the phone, resample to 16 kHz mono 8-bit, stream in real time with beat features ----
let mt=null;
$('stop').onclick=()=>{clearInterval(mt);mt=null;send('s');$('mst').textContent=''};
const drop=$('drop');drop.onclick=()=>$('file').click();
drop.ondragover=e=>{e.preventDefault();drop.classList.add('over')};drop.ondragleave=()=>drop.classList.remove('over');
drop.ondrop=e=>{e.preventDefault();drop.classList.remove('over');if(e.dataTransfer.files[0])play(e.dataTransfer.files[0])};
$('file').onchange=e=>{if(e.target.files[0])play(e.target.files[0])};
async function play(f){clearInterval(mt);$('mst').textContent='decoding '+f.name+'…';
 const AC=window.AudioContext||window.webkitAudioContext,ab=await f.arrayBuffer(),src=await new AC().decodeAudioData(ab);
 const off=new OfflineAudioContext(1,Math.ceil(src.duration*16000),16000),bs=off.createBufferSource();bs.buffer=src;bs.connect(off.destination);bs.start();
 const pcm=(await off.startRendering()).getChannelData(0);let pos=0,lp=0,avg=.01,t0=performance.now();const N=1600;
 $('mst').textContent='playing '+f.name;
 mt=setInterval(()=>{const want=Math.floor((performance.now()-t0)*16)+6400;   // stay ~0.4 s ahead
  while(pos<want&&pos<pcm.length){const n=Math.min(N,pcm.length-pos),pkt=new Uint8Array(5+n);let bass=0,all=0;
   for(let i=0;i<n;i++){const s=pcm[pos+i];lp+=(s-lp)*.06;bass+=lp*lp;all+=s*s;pkt[5+i]=Math.max(0,Math.min(255,128+s*127))}
   bass=Math.sqrt(bass/n);all=Math.sqrt(all/n);const beat=bass>avg*1.35&&bass>.04;avg+=(bass-avg)*.08;
   pkt[0]=0xA1;pkt[1]=Math.min(255,bass*600);pkt[2]=Math.min(255,all*400);pkt[3]=Math.min(255,Math.max(0,all-bass)*600);pkt[4]=beat?1:0;
   if(ok)ws.send(pkt);pos+=n}
  if(pos>=pcm.length){clearInterval(mt);mt=null;$('mst').textContent='done'}},50)}
conn();
</script></body></html>)HTML";
