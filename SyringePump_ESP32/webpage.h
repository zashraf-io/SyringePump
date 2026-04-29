/*
 * webpage.h
 * ─────────────────────────────────────────────
 * Auto-generated PROGMEM string of index.html
 * for the Syringe Pump Controller dashboard.
 * ─────────────────────────────────────────────
 */

#ifndef WEBPAGE_H
#define WEBPAGE_H

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Syringe Pump Controller</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700;800;900&family=JetBrains+Mono:wght@500;700&display=swap" rel="stylesheet">
<style>
*,*::before,*::after{box-sizing:border-box;margin:0;padding:0}
:root{
  --bg:#0a0e1a;--surface:#111827;--surface2:#1a2235;--border:#1e293b;
  --text:#e2e8f0;--text-muted:#94a3b8;--accent:#38bdf8;--accent-glow:rgba(56,189,248,.15);
  --green:#22c55e;--green-glow:rgba(34,197,94,.2);--red:#ef4444;--red-glow:rgba(239,68,68,.25);
  --yellow:#facc15;--yellow-glow:rgba(250,204,21,.25);--radius:12px;--font:'Inter',sans-serif;--mono:'JetBrains Mono',monospace;
}
html{font-size:16px}
body{font-family:var(--font);background:var(--bg);color:var(--text);min-height:100vh;line-height:1.5}
header{background:linear-gradient(135deg,#0f172a 0%,#1e293b 100%);border-bottom:1px solid var(--border);padding:1rem 2rem;display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:.75rem}
.logo{display:flex;align-items:center;gap:.75rem}
.logo svg{width:36px;height:36px;filter:drop-shadow(0 0 6px var(--accent))}
.logo h1{font-size:1.25rem;font-weight:700;letter-spacing:-.02em}
.logo span{color:var(--accent)}
.status-chip{display:inline-flex;align-items:center;gap:.4rem;padding:.35rem .9rem;border-radius:999px;font-size:.75rem;font-weight:600;text-transform:uppercase;letter-spacing:.05em;background:var(--surface2);border:1px solid var(--border);color:var(--text-muted);transition:all .3s}
.status-chip.running{background:var(--green-glow);border-color:var(--green);color:var(--green)}
.status-chip.scheduled{background:var(--yellow-glow);border-color:var(--yellow);color:var(--yellow)}
.status-chip .dot{width:8px;height:8px;border-radius:50%;background:currentColor}
.status-chip.running .dot{animation:pulse-dot 1.2s ease-in-out infinite}
@keyframes pulse-dot{0%,100%{opacity:1}50%{opacity:.3}}
main{max-width:1280px;margin:0 auto;padding:1.5rem;display:grid;grid-template-columns:1fr 1fr;gap:1.25rem}
@media(max-width:860px){main{grid-template-columns:1fr}}
.card{background:var(--surface);border:1px solid var(--border);border-radius:var(--radius);padding:1.5rem;position:relative;overflow:hidden;transition:border-color .3s,box-shadow .3s}
.card:hover{border-color:rgba(56,189,248,.25);box-shadow:0 0 20px var(--accent-glow)}
.card-title{font-size:.7rem;font-weight:700;text-transform:uppercase;letter-spacing:.1em;color:var(--text-muted);margin-bottom:1rem;display:flex;align-items:center;gap:.5rem}
.card-title svg{width:16px;height:16px;opacity:.6}
.control-panel{grid-column:1}
.input-group{margin-bottom:1rem}
.input-group label{display:block;font-size:.8rem;font-weight:600;color:var(--text-muted);margin-bottom:.35rem}
.input-group input{width:100%;padding:.7rem 1rem;border-radius:8px;border:1px solid var(--border);background:var(--bg);color:var(--text);font-family:var(--mono);font-size:1rem;font-weight:500;outline:none;transition:border-color .2s,box-shadow .2s}
.input-group input:focus{border-color:var(--accent);box-shadow:0 0 0 3px var(--accent-glow)}
.input-row{display:flex;gap:.6rem;align-items:center}
.input-row select{padding:.7rem .85rem;border-radius:8px;border:1px solid var(--border);background:var(--bg);color:var(--text);font-family:var(--font);font-size:.85rem;font-weight:600;letter-spacing:.03em;text-transform:uppercase}
.input-group .unit{font-size:.7rem;color:var(--text-muted);margin-top:.25rem}
.btn-row{display:flex;gap:.75rem;margin-top:1.25rem;flex-wrap:wrap}
.btn{flex:1;min-width:140px;padding:.85rem 1.2rem;border:none;border-radius:10px;font-family:var(--font);font-size:.9rem;font-weight:700;cursor:pointer;text-transform:uppercase;letter-spacing:.06em;display:inline-flex;align-items:center;justify-content:center;gap:.5rem;transition:transform .15s,box-shadow .3s,filter .2s;position:relative;overflow:hidden}
.btn:active{transform:scale(.97)}
.btn-start{background:linear-gradient(135deg,#16a34a,#22c55e);color:#fff;box-shadow:0 4px 20px var(--green-glow)}
.btn-start:hover{box-shadow:0 6px 30px rgba(34,197,94,.35);filter:brightness(1.1)}
.btn-start:disabled{opacity:.5;cursor:not-allowed;filter:none;box-shadow:none}
.btn-stop{background:linear-gradient(135deg,#dc2626,#ef4444);color:#fff;box-shadow:0 4px 20px var(--red-glow)}
.btn-stop:hover{box-shadow:0 6px 30px rgba(239,68,68,.4);filter:brightness(1.1)}
.btn-reverse{background:linear-gradient(135deg,#d97706,#f59e0b);color:#fff;box-shadow:0 4px 20px var(--yellow-glow)}
.btn-reverse:hover{box-shadow:0 6px 30px rgba(245,158,11,.4);filter:brightness(1.1)}
.dir-badge{display:inline-flex;align-items:center;gap:.35rem;padding:.3rem .8rem;border-radius:999px;font-size:.7rem;font-weight:700;text-transform:uppercase;letter-spacing:.06em;background:var(--surface2);border:1px solid var(--border);color:var(--yellow);margin-left:.5rem;transition:all .3s}
.btn-schedule{background:linear-gradient(135deg,#d97706,#f59e0b);color:#fff;box-shadow:0 4px 20px var(--yellow-glow)}
.btn-schedule:hover{box-shadow:0 6px 30px rgba(245,158,11,.4);filter:brightness(1.1)}
.btn-cancel{background:var(--surface2);color:var(--text);border:1px solid var(--border)}
.schedule-panel{grid-column:1}
.schedule-status{display:flex;justify-content:space-between;gap:.5rem;margin-top:.75rem;font-size:.75rem;color:var(--text-muted);font-weight:600}
.schedule-status strong{color:var(--yellow);font-weight:700}
.schedule-counter{display:flex;justify-content:space-between;gap:.5rem;margin-top:.6rem;font-size:.85rem;font-weight:700;color:var(--text)}
.schedule-counter span{color:var(--text-muted);font-weight:600}
.monitor{grid-column:2}
@media(max-width:860px){.monitor{grid-column:1}}
.big-value{font-family:var(--mono);font-size:3.5rem;font-weight:700;text-align:center;padding:1rem 0 .25rem;color:var(--accent);text-shadow:0 0 30px var(--accent-glow);line-height:1}
.big-label{text-align:center;font-size:.75rem;color:var(--text-muted);font-weight:600;text-transform:uppercase;letter-spacing:.08em;margin-bottom:1.25rem}
.progress-track{width:100%;height:22px;border-radius:999px;background:var(--bg);border:1px solid var(--border);overflow:hidden;position:relative}
.progress-fill{height:100%;border-radius:999px;background:linear-gradient(90deg,#0ea5e9,#38bdf8,#7dd3fc);transition:width .4s ease;position:relative;min-width:0}
.progress-fill::after{content:'';position:absolute;inset:0;background:linear-gradient(90deg,transparent 60%,rgba(255,255,255,.15));border-radius:999px}
.progress-text{display:flex;justify-content:space-between;font-size:.7rem;color:var(--text-muted);margin-top:.4rem;font-weight:500}
.alarms{grid-column:1/-1}
.alarm-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:1rem}
.alarm-card{padding:1.25rem;border-radius:var(--radius);border:1px solid var(--border);background:var(--surface2);text-align:center;transition:all .35s;position:relative;overflow:hidden}
.alarm-card .alarm-icon{width:48px;height:48px;margin:0 auto .75rem;border-radius:50%;display:flex;align-items:center;justify-content:center;background:var(--bg);border:1px solid var(--border);transition:all .35s}
.alarm-card .alarm-icon svg{width:24px;height:24px;color:var(--text-muted);transition:color .3s}
.alarm-card .alarm-label{font-size:.8rem;font-weight:600;color:var(--text-muted);transition:color .3s}
.alarm-card .alarm-status{font-size:.65rem;font-weight:700;text-transform:uppercase;letter-spacing:.08em;margin-top:.4rem;color:var(--text-muted);opacity:.5;transition:all .3s}
.alarm-card.active{border-color:var(--red);background:var(--red-glow);box-shadow:0 0 30px var(--red-glow);animation:alarm-flash 1s ease-in-out infinite}
.alarm-card.active .alarm-icon{background:var(--red);border-color:var(--red)}
.alarm-card.active .alarm-icon svg{color:#fff}
.alarm-card.active .alarm-label{color:#fff}
.alarm-card.active .alarm-status{color:var(--red);opacity:1}
@keyframes alarm-flash{0%,100%{opacity:1}50%{opacity:.75}}
footer{text-align:center;padding:1.5rem;font-size:.65rem;color:var(--text-muted);opacity:.5;letter-spacing:.03em}
.toast{position:fixed;bottom:1.5rem;right:1.5rem;padding:.8rem 1.2rem;border-radius:10px;font-size:.8rem;font-weight:600;color:#fff;z-index:999;transform:translateY(120%);opacity:0;transition:all .35s ease;pointer-events:none}
.toast.show{transform:translateY(0);opacity:1}
.toast.success{background:linear-gradient(135deg,#16a34a,#22c55e);box-shadow:0 4px 20px var(--green-glow)}
.toast.error{background:linear-gradient(135deg,#dc2626,#ef4444);box-shadow:0 4px 20px var(--red-glow)}
</style>
</head>
<body>
<header>
  <div class="logo">
    <svg viewBox="0 0 36 36" fill="none"><rect x="2" y="10" width="32" height="16" rx="4" stroke="#38bdf8" stroke-width="2"/><rect x="6" y="14" width="16" height="8" rx="2" fill="#38bdf8" fill-opacity=".2" stroke="#38bdf8" stroke-width="1.5"/><rect x="26" y="15" width="8" height="6" rx="1" fill="#38bdf8" fill-opacity=".1" stroke="#38bdf8" stroke-width="1.5"/><line x1="14" y1="14" x2="14" y2="22" stroke="#38bdf8" stroke-width="1" opacity=".4"/><line x1="10" y1="14" x2="10" y2="22" stroke="#38bdf8" stroke-width="1" opacity=".4"/></svg>
    <h1><span>Syringe</span>Pump Controller</h1>
  </div>
  <div class="status-chip" id="statusChip"><span class="dot"></span><span id="statusText">Idle</span></div>
</header>
<main>
  <div class="card control-panel" id="controlPanel">
    <div class="card-title"><svg viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="1.5"><path d="M8 1v14M1 8h14"/></svg>Control Panel</div>
    <div class="input-group"><label for="targetVol">Target Volume</label><input type="number" id="targetVol" min="0" step="0.1" placeholder="0.0"><div class="unit">mL (millilitres)</div></div>
    <div class="input-group"><label for="flowRate">Flow Rate</label><input type="number" id="flowRate" min="0" step="0.1" placeholder="0.0"><div class="unit">mL/min (millilitres per minute)</div></div>
    <div class="input-group"><label for="timeMins">Time</label><input type="number" id="timeMins" min="0" step="0.1" placeholder="0.0"><div class="unit">Minutes</div></div>
    <div class="btn-row">
      <button class="btn btn-start" id="btnStart" onclick="startInfusion()"><svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor"><path d="M8 5v14l11-7z"/></svg>Start Infusion</button>
      <button class="btn btn-stop" id="btnStop" onclick="emergencyStop()"><svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor"><rect x="4" y="4" width="16" height="16" rx="2"/></svg>Emergency Stop</button>
    </div>
    <div class="btn-row">
      <button class="btn btn-reverse" id="btnReverse" onclick="reverseDirection()"><svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><polyline points="17 1 21 5 17 9"/><path d="M3 11V9a4 4 0 0 1 4-4h14"/><polyline points="7 23 3 19 7 15"/><path d="M21 13v2a4 4 0 0 1-4 4H3"/></svg>Reverse Direction<span class="dir-badge" id="dirBadge">⬆ PULL</span></button>
      <button class="btn" style="background:var(--surface2);color:var(--text);border:1px solid var(--border);" onclick="resetVolume()">Reset Volume</button>
    </div>
  </div>
  <div class="card schedule-panel" id="schedulePanel">
    <div class="card-title"><svg viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="1.5"><circle cx="8" cy="8" r="6"/><path d="M8 4v4l2 2"/></svg>Schedule</div>
    <div class="input-group"><label for="scheduleDelay">Schedule Delay</label><div class="input-row"><input type="number" id="scheduleDelay" min="0" step="1" placeholder="0"><select id="scheduleUnit"><option value="seconds">Seconds</option><option value="minutes">Minutes</option></select></div><div class="unit">Set delay before auto-start</div></div>
    <div class="btn-row">
      <button class="btn btn-schedule" onclick="scheduleStart()"><svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor"><path d="M12 8v5l3 3"/><path d="M12 3a9 9 0 1 0 0 18 9 9 0 0 0 0-18z" fill="none" stroke="currentColor" stroke-width="2"/></svg>Schedule Start</button>
      <button class="btn btn-cancel" onclick="cancelScheduledStart()">Cancel Schedule</button>
    </div>
    <div class="schedule-status">
      <span id="scheduleStatusLabel">Schedule</span>
      <strong id="scheduleCountdown">Not set</strong>
    </div>
    <div class="schedule-counter" id="scheduleCounter"><span>Set</span><strong>0:00</strong></div>
  </div>
  <div class="card monitor" id="monitorPanel">
    <div class="card-title"><svg viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="1.5"><polyline points="1 12 4 5 7 9 10 3 13 8 15 6"/></svg>Real-Time Monitoring</div>
    <div class="big-value" id="deliveredVal">0.00</div>
    <div class="big-label">Volume Delivered (mL)</div>
    <div class="progress-track"><div class="progress-fill" id="progressFill" style="width:0%"></div></div>
    <div class="progress-text"><span id="progressPct">0 %</span><span id="progressTarget">Target: — mL</span></div>
  </div>
  <div class="card alarms" id="alarmPanel">
    <div class="card-title"><svg viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="1.5"><path d="M8 1L1 13h14L8 1zM8 6v4M8 11.5v.5"/></svg>Alarm Dashboard</div>
    <div class="alarm-grid">
      <div class="alarm-card" id="alarmOcclusion"><div class="alarm-icon"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><circle cx="12" cy="12" r="10"/><line x1="4" y1="4" x2="20" y2="20"/></svg></div><div class="alarm-label">Occlusion (Blockage)</div><div class="alarm-status" id="occlusionStatus">Normal</div></div>
      <div class="alarm-card" id="alarmEmpty"><div class="alarm-icon"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M12 2v6M12 18v4M4.93 4.93l4.24 4.24M14.83 14.83l4.24 4.24M2 12h6M18 12h4M4.93 19.07l4.24-4.24M14.83 9.17l4.24-4.24"/></svg></div><div class="alarm-label">Syringe Empty</div><div class="alarm-status" id="emptyStatus">Normal</div></div>
    </div>
  </div>
</main>
<div class="toast" id="toast"></div>
<footer>DIY Syringe Pump Controller &mdash; ESP32 Dashboard &copy; 2026</footer>
<script>
let pollingInterval=null,targetVolume=0,firstError=null;
const elDelivered=document.getElementById('deliveredVal'),elFill=document.getElementById('progressFill'),elPct=document.getElementById('progressPct'),elTarget=document.getElementById('progressTarget'),elChip=document.getElementById('statusChip'),elChipText=document.getElementById('statusText'),elToast=document.getElementById('toast'),elDirBadge=document.getElementById('dirBadge');
const elVol=document.getElementById('targetVol'),elRate=document.getElementById('flowRate'),elTime=document.getElementById('timeMins');
const elScheduleDelay=document.getElementById('scheduleDelay'),elScheduleUnit=document.getElementById('scheduleUnit');
const elScheduleLabel=document.getElementById('scheduleStatusLabel'),elScheduleCountdown=document.getElementById('scheduleCountdown');
const elScheduleCounter=document.getElementById('scheduleCounter');
const MAX_RATE=18.3,MIN_RATE=0.04;
const alarms={occlusion:{card:document.getElementById('alarmOcclusion'),status:document.getElementById('occlusionStatus')},empty:{card:document.getElementById('alarmEmpty'),status:document.getElementById('emptyStatus')},tremor:{card:document.getElementById('alarmTremor'),status:document.getElementById('tremorStatus')}};
function showToast(m,t){elToast.textContent=m;elToast.className='toast '+t+' show';setTimeout(()=>elToast.classList.remove('show'),3000)}

function formatRemaining(ms){if(!ms||ms<=0)return'0:00';const totalSec=Math.ceil(ms/1000),min=Math.floor(totalSec/60),sec=totalSec%60;return min+':'+String(sec).padStart(2,'0')}
function getScheduleSeconds(){const rawValue=parseFloat(elScheduleDelay.value);if(!rawValue||rawValue<=0)return 0;return elScheduleUnit.value==='minutes'?rawValue*60:rawValue}
function updateScheduleCounter(active,remainingMs){if(active){elScheduleCounter.innerHTML='<span>Remaining</span><strong>'+formatRemaining(remainingMs)+'</strong>';return}const seconds=getScheduleSeconds();const label=seconds>0?formatRemaining(seconds*1000):'0:00';elScheduleCounter.innerHTML='<span>Set</span><strong>'+label+'</strong>'}
function updateScheduleState(active,remainingMs){if(active){elScheduleLabel.textContent='Scheduled Start';elScheduleCountdown.textContent='in '+formatRemaining(remainingMs)}else{elScheduleLabel.textContent='Schedule';elScheduleCountdown.textContent='Not set'}updateScheduleCounter(active,remainingMs)}

elScheduleDelay.addEventListener('input',()=>updateScheduleCounter(false,0));
elScheduleUnit.addEventListener('change',()=>updateScheduleCounter(false,0));
updateScheduleCounter(false,0);

function recalculateFields(source){
  let v=parseFloat(elVol.value),r=parseFloat(elRate.value),t=parseFloat(elTime.value);
  if(source==='vol'){
    if(v>0&&r>0)elTime.value=(v/r).toFixed(2);
    else if(v>0&&t>0)elRate.value=(v/t).toFixed(2);
  }else if(source==='rate'){
    if(r>0&&v>0)elTime.value=(v/r).toFixed(2);
    else if(r>0&&t>0)elVol.value=(r*t).toFixed(2);
  }else if(source==='time'){
    if(t>0&&v>0)elRate.value=(v/t).toFixed(2);
    else if(t>0&&r>0)elVol.value=(r*t).toFixed(2);
  }
  validateLimits();
}

function validateLimits(){
  let r=parseFloat(elRate.value);
  if(!isNaN(r)){
    if(r>MAX_RATE){showToast(`Max speed is ${MAX_RATE} mL/min`,'error');elRate.value=MAX_RATE;let v=parseFloat(elVol.value);if(!isNaN(v)&&v>0)elTime.value=(v/MAX_RATE).toFixed(2);}
    else if(r>0&&r<MIN_RATE){showToast(`Min speed is ${MIN_RATE} mL/min`,'error');elRate.value=MIN_RATE;let v=parseFloat(elVol.value);if(!isNaN(v)&&v>0)elTime.value=(v/MIN_RATE).toFixed(2);}
  }
}

elVol.addEventListener('change',()=>recalculateFields('vol'));
elRate.addEventListener('change',()=>recalculateFields('rate'));
elTime.addEventListener('change',()=>recalculateFields('time'));

async function startInfusion(){
  validateLimits();
  const v=parseFloat(elVol.value),r=parseFloat(elRate.value);
  if(!v||v<=0||!r||r<=0){showToast('Please enter valid volume and flow rate.','error');return}
  targetVolume=v;firstError=null;elTarget.textContent='Target: '+v.toFixed(1)+' mL';
  try{
    const res=await fetch('/set_parameters',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({target_vol:v,flow_rate:r})});
    if(!res.ok)throw new Error();
    showToast('Infusion started successfully.','success');setRunningState(true);startPolling();
  }catch(e){showToast('Failed to connect to pump.','error')}
}
async function emergencyStop(){try{await fetch('/emergency_stop',{method:'POST'})}catch(_){}stopPolling();setRunningState(false);showToast('EMERGENCY STOP activated!','error')}
async function scheduleStart(){const rawValue=parseFloat(elScheduleDelay.value),v=parseFloat(elVol.value),r=parseFloat(elRate.value),unit=elScheduleUnit.value,seconds=unit==='minutes'?rawValue*60:rawValue;if(!seconds||seconds<=0||!v||v<=0||!r||r<=0){showToast('Enter time, volume, and flow rate before scheduling.','error');return}try{const res=await fetch('/schedule_start',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({seconds,target_vol:v,flow_rate:r})});if(!res.ok)throw new Error();showToast('Auto-start scheduled.','success');updateScheduleCounter(true,seconds*1000);startPolling()}catch(e){showToast('Failed to schedule start.','error')}}
async function cancelScheduledStart(){try{const res=await fetch('/cancel_scheduled_start',{method:'POST'});if(!res.ok)throw new Error();updateScheduleState(false,0);updateScheduleCounter(false,0);showToast('Schedule cancelled.','success')}catch(e){showToast('Failed to cancel schedule.','error')}}
async function resetVolume(){try{const res=await fetch('/reset_volume',{method:'POST'});if(res.ok){showToast('Volume reset to 0 mL','success');fetchStatus()}}catch(e){}}
async function reverseDirection(){try{const res=await fetch('/reverse_direction',{method:'POST'});if(!res.ok)throw new Error();const d=await res.json();const dir=d.direction||'unknown';updateDirBadge(dir.includes('PUSH')||dir.includes('push')?'push':'puSH');showToast('Direction: '+dir,'success')}catch(e){showToast('Failed to reverse direction.','error')}}
function updateDirBadge(dir){if(dir==='push'){elDirBadge.textContent='\u2B06 PULL';elDirBadge.style.color='var(--accent)'}else{elDirBadge.textContent='\u2B07 PUSH';elDirBadge.style.color='var(--yellow)'}}
function startPolling(){stopPolling();pollingInterval=setInterval(fetchStatus,500)}
function stopPolling(){if(pollingInterval){clearInterval(pollingInterval);pollingInterval=null}}
async function fetchStatus(){try{const res=await fetch('/status');if(!res.ok)throw new Error();const d=await res.json();updateUI(d)}catch(_){}}
function updateUI(d){const vol=d.delivered_vol??0;elDelivered.textContent=vol.toFixed(2);const pct=targetVolume>0?Math.min((vol/targetVolume)*100,100):0;elFill.style.width=pct.toFixed(1)+'%';elPct.textContent=pct.toFixed(1)+' %';const scheduledActive=!!d.scheduled_start_active;setRunningState(!!d.running,scheduledActive);if(!d.running&&pollingInterval)stopPolling();updateScheduleState(scheduledActive,d.scheduled_start_remaining_ms??0);if(!firstError){if(d.empty)firstError='empty';else if(d.occlusion)firstError='occlusion';else if(d.tremor)firstError='tremor';}setAlarm('occlusion',firstError==='occlusion');setAlarm('empty',firstError==='empty');setAlarm('tremor',firstError==='tremor');if(d.direction)updateDirBadge(d.direction)}
function setAlarm(k,a){const al=alarms[k];if(a){al.card.classList.add('active');al.status.textContent='\u26A0 WARNING'}else{al.card.classList.remove('active');al.status.textContent='Normal'}}
function setRunningState(r,scheduled){if(r){elChip.classList.add('running');elChip.classList.remove('scheduled');elChipText.textContent='Running';document.getElementById('btnStart').disabled=true}else if(scheduled){elChip.classList.remove('running');elChip.classList.add('scheduled');elChipText.textContent='Scheduled';document.getElementById('btnStart').disabled=false}else{elChip.classList.remove('running');elChip.classList.remove('scheduled');elChipText.textContent='Idle';document.getElementById('btnStart').disabled=false}}
</script>
</body>
</html>
)rawliteral";

#endif // WEBPAGE_H
